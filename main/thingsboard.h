#ifndef THINGSBOARD_H
#define THINGSBOARD_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialize WiFi connection
 * 
 * @return ESP_OK on success
 */
esp_err_t thingsboard_wifi_init(void);

/**
 * @brief Initialize ThingsBoard MQTT client
 * 
 * @param device_token ThingsBoard device access token
 * @return ESP_OK on success
 */
esp_err_t thingsboard_mqtt_init(const char *device_token);

/**
 * @brief Send sensor data to ThingsBoard
 * 
 * @param sensor_value Gas sensor reading value
 * @param is_alarm Boolean indicating if alarm threshold was exceeded
 * @return ESP_OK on success
 */
esp_err_t thingsboard_send_telemetry(int sensor_value, bool is_alarm);

#endif /* THINGSBOARD_H */