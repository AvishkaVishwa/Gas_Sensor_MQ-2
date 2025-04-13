#ifndef MQ2_SENSOR_H
#define MQ2_SENSOR_H

#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include <stdbool.h>

/**
 * @brief Operation modes for the MQ2 sensor
 */
typedef enum {
    NORMAL_MODE,  // Normal operation
    TEST_MODE,    // Test mode (forces alarm)
    SILENT_MODE   // Silent mode (disables alarm output)
} mq2_mode_t;

/**
 * @brief Initialize MQ2 sensor and GPIO
 * 
 * @param output_pin GPIO pin number for alarm output
 * @param adc_channel ADC channel connected to MQ2 sensor
 * @param threshold Alarm threshold value
 * @return ESP_OK on success
 */
esp_err_t mq2_init(int output_pin, int adc_channel, int threshold);

/**
 * @brief Read MQ2 sensor value
 * 
 * @param sensor_value Pointer to store the sensor reading
 * @param is_alarm Pointer to store alarm status
 * @return ESP_OK on success
 */
esp_err_t mq2_read(int *sensor_value, bool *is_alarm);

/**
 * @brief Perform sensor warm-up
 * 
 * @param seconds Warm-up time in seconds
 * @return ESP_OK on success
 */
esp_err_t mq2_warmup(int seconds);

/**
 * @brief Get current operating mode
 * 
 * @return Current mode (NORMAL_MODE, TEST_MODE, or SILENT_MODE)
 */
mq2_mode_t mq2_get_mode(void);

#endif /* MQ2_SENSOR_H */