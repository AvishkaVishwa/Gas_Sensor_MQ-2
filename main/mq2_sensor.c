#include "mq2_sensor.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MQ2_Sensor";

// Configuration
static int output_pin;
static int alarm_threshold;
static adc_oneshot_unit_handle_t adc1_handle;
static int mq2_adc_channel;
static mq2_mode_t current_mode = NORMAL_MODE;

// Button pins
#define MODE_BUTTON_PIN GPIO_NUM_14
#define SILENCE_BUTTON_PIN GPIO_NUM_13

// Forward declaration
static void IRAM_ATTR mode_button_isr_handler(void* arg);

// Button handler implementation
static void IRAM_ATTR mode_button_isr_handler(void* arg)
{
    // Debounce and switch modes
    static uint32_t last_press_time = 0;
    uint32_t current_time = esp_timer_get_time() / 1000;
    
    if (current_time - last_press_time > 300) {  // 300ms debounce
        current_mode = (current_mode + 1) % 3;  // Cycle through modes
        last_press_time = current_time;
        
        // Set alarm LED according to mode
        if (current_mode == TEST_MODE) {
            gpio_set_level(output_pin, 1); // Turn on in test mode
        } else if (current_mode == SILENT_MODE) {
            gpio_set_level(output_pin, 0); // Turn off in silent mode
        }
    }
}

esp_err_t mq2_init(int output_gpio, int adc_channel, int threshold)
{
    // Save configuration
    output_pin = output_gpio;
    alarm_threshold = threshold;
    mq2_adc_channel = adc_channel;
    
    // Configure output pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << output_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Configure silence button
    gpio_config_t button_conf = {
        .pin_bit_mask = (1ULL << SILENCE_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,      
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&button_conf);

    // Configure mode button with interrupt
    gpio_config_t mode_button_conf = {
        .pin_bit_mask = (1ULL << MODE_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&mode_button_conf);
    
    // Install ISR service and add handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(MODE_BUTTON_PIN, mode_button_isr_handler, NULL);

    // Configure ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
    
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, mq2_adc_channel, &chan_config));
    
    ESP_LOGI(TAG, "MQ2 sensor initialized on ADC channel %d, output pin %d, threshold %d", 
             mq2_adc_channel, output_pin, alarm_threshold);
             
    return ESP_OK;
}

mq2_mode_t mq2_get_mode(void)
{
    return current_mode;
}

esp_err_t mq2_read(int *sensor_value, bool *is_alarm)
{
    // Read sensor value
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, mq2_adc_channel, sensor_value));
    
    // Read silence button (active low with pullup)
    bool silence_pressed = (gpio_get_level(SILENCE_BUTTON_PIN) == 0);
    
    // Handle different modes
    if (current_mode == TEST_MODE) {
        // Test mode - always trigger alarm
        *is_alarm = true;
    } else if (current_mode == SILENT_MODE) {
        // Silent mode - never trigger alarm
        *is_alarm = false;
    } else {
        // Normal mode - trigger alarm if threshold exceeded and not silenced
        *is_alarm = (*sensor_value > alarm_threshold) && !silence_pressed;
    }
    
    // Set output pin accordingly
    gpio_set_level(output_pin, *is_alarm ? 1 : 0);
    
    ESP_LOGI(TAG, "MQ2 reading: %d, alarm: %s, mode: %s, silence: %s", 
             *sensor_value, 
             *is_alarm ? "YES" : "NO",
             current_mode == NORMAL_MODE ? "NORMAL" :
             current_mode == TEST_MODE ? "TEST" : "SILENT",
             silence_pressed ? "YES" : "NO");
    
    return ESP_OK;
}

esp_err_t mq2_warmup(int seconds)
{
    ESP_LOGI(TAG, "Warming up MQ2 sensor...");
    
    for (int i = seconds; i > 0; i--) {
        ESP_LOGI(TAG, "%d seconds remaining for warmup...", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TAG, "MQ2 sensor warmup completed");
    return ESP_OK;
}