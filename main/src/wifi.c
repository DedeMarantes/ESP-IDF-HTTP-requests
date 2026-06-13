#include "wifi.h"

static uint8_t retry_num = 0;
static EventGroupHandle_t wifi_event_group; //Grupo de eventos para sinalizar o status da conexão Wi-Fi

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if(retry_num < MAX_RETRIES) {
            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(WIFI_TAG, "Tentando se reconectar ao Wi-Fi... Tentativa %d", retry_num);
        } 
        else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(WIFI_TAG, "Falha ao conectar ao Wi-Fi após %d tentativas", MAX_RETRIES);
        }
    } else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "IP obtido: " IPSTR, IP2STR(&event->ip_info.ip));
        retry_num = 0; //Resetar contador de tentativas
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);//Sinalizar conexão bem sucedida
    }
}

//Inicialização Wifi
static void wifi_init_sta() {
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    //Configuração do Wifi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_event_handler_instance_t instance_any_id, inst_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id
    ));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &inst_got_ip
    ));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = SSID,
            .password = PASSWORD,
            .threshold = {
                .authmode = WIFI_AUTH_WPA2_PSK,
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY
    );

    //Verificar resultado da conexão
    if(bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(WIFI_TAG, "Conectado ao SSID: %s", SSID);
    } else {
        ESP_LOGE(WIFI_TAG, "Falha ao conectar ao SSID: %s", SSID);
    }
}

void wifi_connect() {
    //NVS nececessário para driver WiFi
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(WIFI_TAG, "Iniciando Wi-Fi STA...");
    wifi_init_sta();
}