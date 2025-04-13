#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"  // New ADC API
#include "driver/gpio.h"

#define MQ2_ADC_CHANNEL  ADC_CHANNEL_6 // GPIO34
#define OUTPUT_PIN       GPIO_NUM_2
#define THRESHOLD        300

void app_main(void)
{
    // Configure output pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << OUTPUT_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Configure ADC
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
    
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, MQ2_ADC_CHANNEL, &chan_config));

    while (true) {
        int mq2Value;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, MQ2_ADC_CHANNEL, &mq2Value));
        printf("MQ2: %d\n", mq2Value);

        if (mq2Value > THRESHOLD) {
            gpio_set_level(OUTPUT_PIN, 1);
        } else {
            gpio_set_level(OUTPUT_PIN, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}