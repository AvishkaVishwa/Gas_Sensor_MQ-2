#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mq2_sensor.h"
#include "thingsboard.h"
#include "driver/gpio.h" 
#include "driver/adc.h"

static const char *TAG = "Main";

// Configuration
#define MQ2_ADC_CHANNEL  ADC_CHANNEL_6  // GPIO34
#define OUTPUT_PIN       GPIO_NUM_2      // Alarm output pin
#define GAS_THRESHOLD    300            // Alarm threshold
#define DEVICE_TOKEN     "28zMWT2tFgz3kqfyQndk"
#define SAMPLE_PERIOD_MS 2000           // Telemetry send interval

void app_main(void)
{
    ESP_LOGI(TAG, "Starting application...");

    // Initialize MQ2 sensor
    ESP_ERROR_CHECK(mq2_init(OUTPUT_PIN, MQ2_ADC_CHANNEL, GAS_THRESHOLD));
    
    // Perform sensor warmup
    ESP_ERROR_CHECK(mq2_warmup(30));
    
    // Initialize WiFi
    ESP_ERROR_CHECK(thingsboard_wifi_init());
    
    // Wait for WiFi connection
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // Initialize ThingsBoard MQTT
    ESP_ERROR_CHECK(thingsboard_mqtt_init(DEVICE_TOKEN));
    
    // Wait for MQTT connection
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Main loop
    while (true) {
        int sensor_value;
        bool is_alarm;
        
        // Read sensor
        ESP_ERROR_CHECK(mq2_read(&sensor_value, &is_alarm));
        
        // Send telemetry to ThingsBoard
        ESP_ERROR_CHECK(thingsboard_send_telemetry(sensor_value, is_alarm));
        
        // Wait before next reading
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}