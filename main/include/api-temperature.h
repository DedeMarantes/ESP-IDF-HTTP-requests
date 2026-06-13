#ifndef API_TEMPERATURE_H
#define API_TEMPERATURE_H
#include <stdio.h>
#include <stdint.h>
#include "cJSON.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_err.h"
#include "esp_crt_bundle.h"

typedef struct {
    float latitude;
    float longitude;
    char cidade[20];
} weather_params_t;

typedef struct {
    uint16_t temperature;
    char time[20];
} weather_data_t;

typedef struct 
{
    uint8_t *buffer;
    uint32_t length;
    uint32_t capacity; // bytes alocados para o buffer
} http_response_t;



void http_get_temperature(const weather_params_t *params, weather_data_t *data);

#endif