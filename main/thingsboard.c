#include "thingsboard.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "mq2_sensor.h"

static const char *TAG = "ThingsBoard";
static esp_mqtt_client_handle_t mqtt_client = NULL;

/* WiFi configuration */
#define WIFI_SSID      "JJ"
#define WIFI_PASS      "121541127"
#define MAXIMUM_RETRY  5

/* ThingsBoard configuration */
#define THINGSBOARD_HOST   "192.168.66.102"  // Or your server IP
#define THINGSBOARD_PORT   1883

static int s_retry_num = 0;
static bool wifi_connected = false;

/* WiFi event handler */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP");
        }
        wifi_connected = false;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        wifi_connected = true;
    }
}

/* MQTT event handler */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT Error");
            break;
        default:
            break;
    }
}

esp_err_t thingsboard_wifi_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialized");
    
    return ESP_OK;
}

esp_err_t thingsboard_mqtt_init(const char *device_token)
{
    if (!wifi_connected) {
        ESP_LOGE(TAG, "WiFi not connected");
        return ESP_FAIL;
    }

    char uri[100];
    snprintf(uri, sizeof(uri), "mqtt://%s:%d", THINGSBOARD_HOST, THINGSBOARD_PORT);
    
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = uri,
            },
        },
        .credentials = {
            .username = device_token,
        }
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }
    
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    
    return ESP_OK;
}

esp_err_t thingsboard_send_telemetry(int sensor_value, bool is_alarm)
{
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return ESP_FAIL;
    }
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "gas_value", sensor_value);
    cJSON_AddBoolToObject(root, "alarm", is_alarm);
    
    // Add mode information
    mq2_mode_t current_mode = mq2_get_mode();
    cJSON_AddStringToObject(root, "mode", 
        current_mode == NORMAL_MODE ? "normal" : 
        current_mode == TEST_MODE ? "test" : "silent");

    char *post_data = cJSON_PrintUnformatted(root);
    int msg_id = esp_mqtt_client_publish(mqtt_client, "v1/devices/me/telemetry", post_data, 0, 1, 0);
    
    ESP_LOGI(TAG, "Published telemetry, msg_id=%d", msg_id);
    
    free(post_data);
    cJSON_Delete(root);
    
    return ESP_OK;
}