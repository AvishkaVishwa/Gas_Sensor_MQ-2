idf_component_register(
    SRCS "main.c" "mq2_sensor.c" "thingsboard.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi mqtt nvs_flash json driver esp_adc
)