//Projeto para conectar o ESP32 a minha rede Wi-Fi
#ifndef WIFI_H
#define WIFI_H

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"


#define MAX_RETRIES 5
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define SSID "" //Preencher com nome rede
#define PASSWORD "" //Preencher com Senha do wifi

#define WIFI_TAG "WiFi"

void wifi_connect();

#endif