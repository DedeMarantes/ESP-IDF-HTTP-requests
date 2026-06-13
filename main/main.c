//Projeto para conectar o ESP32 a minha rede Wi-Fi
#include "wifi.h"
#include "api-temperature.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "lcd.h"

#define ESP_INTR_FLAG_DEFAULT 0
#define BUTTON_DEBOUNCE_TIME 50000

//Constantes
static const char* WEATHER_TASK_TAG = "WeatherTask";
static const gpio_num_t BUTTON_PIN =  GPIO_NUM_35; // pino onde botão esta conectado
static const gpio_num_t SCL_PIN =  GPIO_NUM_22; // pino onde SCL esta conectado
static const gpio_num_t SDA_PIN =  GPIO_NUM_21; // pino onde SDA esta conectado


//Variáveis globais
static TaskHandle_t weather_task_handle = NULL;
static volatile uint32_t debounce_time = 0;
static i2c_master_dev_handle_t i2c_dev_handle;
static i2c_master_bus_handle_t i2c_bus_handle;


static weather_params_t sao_paulo = {
        .cidade = "SP",
        .latitude = -23.585f,
        .longitude = -46.659f
};

//Função de Interrupção
void IRAM_ATTR button_isr_handler(void *arg) {
    uint32_t now = esp_timer_get_time();

    if(now - debounce_time < BUTTON_DEBOUNCE_TIME) {
        return;
    }

    debounce_time = now;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(weather_task_handle, &xHigherPriorityTaskWoken);
    if(xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR(); // desbloqueia a tarefa com prioridade mais alta
    }
}


void configure_button_gpio() {
    gpio_config_t button_config = {
        .pin_bit_mask = 1ULL << BUTTON_PIN, // bit mask para o pino do botão
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE // interrupção por borda de subida
    };
    gpio_config(&button_config);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);
}

void configure_i2c() {
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = SCL_PIN,
        .sda_io_num = SDA_PIN,
        .flags.enable_internal_pullup = true,
        .glitch_ignore_cnt = 7 
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = 7,
        .device_address = I2C_DEV_ADDR,
        .scl_speed_hz = 100000
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &i2c_dev_handle));
}


static void get_weather_task(void *pvParameters) {
    weather_params_t *params = (weather_params_t *) pvParameters;
    weather_data_t data;
    char mensagem[32];
    while(1) {
        //Bloqueia Task até receber notificação do botão
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if(gpio_get_level(BUTTON_PIN) == 1) {
            ESP_LOGI(WEATHER_TASK_TAG, "Botão pressionado GPIO 35, Buscando temperatura...");
            //zera estrutura de dados para evitar lixo anterior
            memset(&data, 0, sizeof(weather_data_t));
            http_get_temperature(params, &data);
            snprintf(mensagem, sizeof(mensagem), "%s: %d C", data.time, data.temperature);
            lcd_send_string(i2c_dev_handle, mensagem);//Envia mensagem para o LCD
            ESP_LOGI(WEATHER_TASK_TAG, "%s: %d C",data.time, data.temperature);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

    }
}

void app_main() {
    wifi_connect();
    configure_button_gpio(); // Configura o botão para interrupção
    configure_i2c(); // Configura o I2C para comunicação com o LCD
    lcd_init(i2c_dev_handle);// Inicializa o LCD


    xTaskCreate(get_weather_task, "WeatherTaskSP", 4096, &sao_paulo, 5, &weather_task_handle);
}