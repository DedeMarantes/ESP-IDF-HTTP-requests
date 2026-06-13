set(COMPONENTS "freertos" "esp_http_client" "esp_timer" 
    "esp_system" "esp_event" "esp_netif""nvs_flash"
     "esp_driver_gpio" "esp_driver_i2c" "esp_wifi" "log" "mbedtls")

idf_component_register(
    SRCS 
        "main.c"    
        "src/lcd.c"
        "src/wifi.c"
        "src/api-temperature.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        ${COMPONENTS})
