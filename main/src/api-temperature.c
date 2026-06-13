#include "api-temperature.h"
#include "math.h"

const char *HTTP_HANDLER_TAG = "http_client_handler";
const char *HTTP_GET_TAG = "http_get";

static inline void build_url(char *url, float latitude, float longitude) {
    snprintf(
        url, 256, "https://api.open-meteo.com/v1/forecast?latitude=%.6f&longitude=%.6f&current_weather=true&timezone=auto", latitude, longitude
    );
}



//Função para obter a temperatura a partir do Json de resposta da API
static void parse_json_response(const char *json_str, weather_data_t *data) {
    cJSON *json = cJSON_Parse(json_str);
    if(json == NULL) {
        ESP_LOGE(HTTP_HANDLER_TAG,"Erro ao parsear JSON");
        return;
    }
    cJSON *current_weather = cJSON_GetObjectItemCaseSensitive(json, "current_weather");
    if(cJSON_IsObject(current_weather)) {
        cJSON *temperature = cJSON_GetObjectItemCaseSensitive(current_weather, "temperature");
        if(cJSON_IsNumber(temperature)) {
            data->temperature = (uint16_t) round(temperature->valuedouble); //Valor inteiro arredondado da temperatura
        } else {
            ESP_LOGE(HTTP_HANDLER_TAG,"Campo temperature não encontrado ou não é número");
        }
        cJSON *time_json = cJSON_GetObjectItemCaseSensitive(current_weather, "time");
        if(cJSON_IsString(time_json) && time_json->valuestring != NULL) {
            int ano, mes, dia, hora, minuto;
            sscanf(time_json->valuestring, "%d-%d-%dT%d:%d", &ano, &mes, &dia, &hora, &minuto);
            //Montando string com data e hora formatada
            snprintf(data->time, sizeof(data->time), "%02d/%02d/%04d %02d:%02d", dia, mes, ano, hora, minuto);
            ESP_LOGI(HTTP_HANDLER_TAG, "Horário fornecido: %s", data->time);
        } else {
            ESP_LOGE(HTTP_HANDLER_TAG, "Campo time não encontrado ou não é string");
        }
    } else {
        ESP_LOGE(HTTP_HANDLER_TAG,"Campo current_weather não encontrado ou não é objeto");
    }
    cJSON_Delete(json);
}

//Função auxiliar para expandir buffer e ter certeza que tem capacidade suficiente
static esp_err_t ensure_buffer_capacity(http_response_t *response, uint32_t additional_length) {
    const u_int32_t maximum_allowed_capacity = 8192;
    uint32_t size_required = response->length + additional_length + 1; // +1 para null terminator
    //Se a capacidade atual alocada for maior que o necessário, não precisa expandir e ignora o resto da função
    if(response->capacity >= size_required) {
        return ESP_OK;
    }

    if(size_required > maximum_allowed_capacity) {
        ESP_LOGE(HTTP_HANDLER_TAG,"Tamanho requerido de %" PRIu32 " bytes excede o limite máximo permitido de %" PRIu32 " bytes", size_required, maximum_allowed_capacity);
        return ESP_ERR_INVALID_SIZE;
    }
    // se for inicial começa com 256 bytes e depois vai dobrando a capacidade
    uint32_t new_capacity = response->capacity == 0 ? 256 : response->capacity;
    while(new_capacity < size_required) {
        new_capacity *= 2; // Dobrar capacidade até ser suficiente
    }

    uint8_t *new_buffer = realloc(response->buffer, new_capacity); // Realocar buffer para nova capacidade
    if(new_buffer == NULL) {
        ESP_LOGE(HTTP_HANDLER_TAG,"Falha ao realocar buffer para capacidade de %" PRIu32 " bytes", new_capacity);
        return ESP_ERR_NO_MEM;
    }
    response->buffer = new_buffer;
    response->capacity = new_capacity;
    return ESP_OK;
}

//Handler para processar a resposta http
esp_err_t http_request_handler(esp_http_client_event_t *event) {
    http_response_t *response = (http_response_t *) event->user_data; //Recuperar ponteiro para estrutura de resposta
    switch(event->event_id) {
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(HTTP_HANDLER_TAG,"Conectado ao servidor");
            if(response) {
                response->length = 0; // Resetar comprimento para armazenar nova resposta
                //response->capacity = 0; // Resetar capacidade para forçar realocação no próximo ensure_buffer_capacity
                if(response->buffer) {
                    response->buffer[0] = '\0'; // Garantir que o buffer esteja vazio
                }
            }
            break;
        // Headers da resposta recebidos
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(HTTP_HANDLER_TAG,"Header recebido: %s: %s", event->header_key, event->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            if(response == NULL) {
                ESP_LOGE(HTTP_HANDLER_TAG,"Estrutura de resposta não inicializada");
                break;
            }
            if(ensure_buffer_capacity(response, event->data_len) != ESP_OK) {
                ESP_LOGE(HTTP_HANDLER_TAG,"Não foi possível garantir capacidade do buffer para dados adicionais");
                break;
            }
            memcpy(response->buffer + response->length, event->data, event->data_len); //copiar dados recebidos para buffer
            response->length += event->data_len; //Atualiza comprimento total de dados armazenados
            response->buffer[response->length] = '\0';
            break;

        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(HTTP_HANDLER_TAG,"Requisição finalizada, total de bytes recebidos: %" PRIu32, response ? response->length : 0);
            break;
        
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(HTTP_HANDLER_TAG,"Desconectado do servidor");
            break;

        case HTTP_EVENT_ERROR:
            ESP_LOGE(HTTP_HANDLER_TAG,"Erro durante o request HTTP");
            // Limpa buffer para evitar dados corrompidos em futuras requisições
            if(response) {
                response->length = 0;
                if(response->buffer) response->buffer[0] = '\0';
            }
        default:
            break;     
    }
    return ESP_OK;
}

void http_get_temperature(const weather_params_t *params, weather_data_t *data) {
    char url[256];
    build_url(url, params->latitude, params->longitude);
    ESP_LOGI(HTTP_GET_TAG, "URL construída: %s", url);
    http_response_t response = {0}; // Inicializar estrutura de resposta

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET, 
        .event_handler = http_request_handler,
        .user_data = &response, // Ponteiro aponta para estrutura de resposta para ser usada no handler
        .timeout_ms = 5000, // Timeout de 5 segundos
        .crt_bundle_attach = esp_crt_bundle_attach, // Anexar bundle de certificados para conexões HTTPS
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if(!client) {
        ESP_LOGE(HTTP_GET_TAG, "Falha ao inicializar cliente HTTP");
        return;
    }

    esp_err_t err = esp_http_client_perform(client);
    if(err != ESP_OK) {
        ESP_LOGE(HTTP_GET_TAG, "Erro ao realizar requisição HTTP: %s", esp_err_to_name(err));
        goto cleanup; // ir para limpeza de recursos mesmo em caso de erro
    }

    int status = esp_http_client_get_status_code(client);
    if(status != 200) {
        ESP_LOGE(HTTP_GET_TAG, "Resposta HTTP inesperada: %d", status);
        goto cleanup;
    }

    if(!response.buffer || response.length == 0) {
        ESP_LOGE(HTTP_GET_TAG, "Nenhum dado recebido na resposta");
        goto cleanup;
    }
    //Parsear resposta JSON para extrair temperatura e armazenar em estrutura de dados
    parse_json_response((const char*) response.buffer, data); 

    cleanup:
        free(response.buffer); // Liberar buffer alocado para resposta
        esp_http_client_cleanup(client); // Limpar recursos do cliente HTTP
}