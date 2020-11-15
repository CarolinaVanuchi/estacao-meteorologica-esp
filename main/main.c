#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/adc.h>
#include <esp_https_server.h>
#include "esp_netif.h"
#include "esp_eth.h"
#include "protocol_examples_common.h"
#include "model.h"
#include "routes.h"
#include "embbeded_files.h"
#include "config_symbols.h"
#include "one_wire_hg_sensor.h"

static const char *TAG = "estacao-meteorologica";

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err) {
    if(strcmp("/api/temperature", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "URI is not available");
        return ESP_OK;
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "ERROR 404");
    return ESP_FAIL;
}

static httpd_handle_t start_webserver(void){

    httpd_handle_t server = NULL;

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    conf.port_secure = 3333;
    ESP_LOGI(TAG, "Starting server: '%d'", conf.port_secure);
    
    conf.cacert_pem = cacert_pem_start;
    conf.cacert_len = cacert_pem_end - cacert_pem_start;

    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    esp_err_t ret = httpd_ssl_start(&server, &conf);

     if (ESP_OK != ret) {
        ESP_LOGI(TAG, "Error starting server!");
        return NULL;
    }

    ESP_LOGI(TAG, "Registering URI handlers");

    httpd_register_uri_routes(server);

    return server;
}


static void stop_webserver(httpd_handle_t server) {
    httpd_ssl_stop(server);
}


static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        stop_webserver(*server);
        *server = NULL;
    }
}


static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        *server = start_webserver();
    }
}

volatile int HALL = 0;

static void IRAM_ATTR isr_hall_sensor(void *arg)
{
    HALL = 1;
}

void app_main(void) {
    
    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    #ifdef CONFIG_EXAMPLE_CONNECT_WIFI
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
    #endif // CONFIG_EXAMPLE_CONNECT_WIFI
    #ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
        ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
    #endif // CONFIG_EXAMPLE_CONNECT_ETHERNET

    ESP_ERROR_CHECK(example_connect());

    gpio_config_t adc0_gpio = { .mode = GPIO_MODE_INPUT, .pin_bit_mask = (1ULL<<GPIO_TEMPERATURE) };
    gpio_config_t hall_gpio = { .intr_type = GPIO_INTR_NEGEDGE, .mode = GPIO_MODE_INPUT, .pin_bit_mask = (1ULL<<GPIO_HALL_SENSOR) };
    gpio_config_t hg_gpio   = { .intr_type = GPIO_INTR_DISABLE, .mode = GPIO_MODE_INPUT_OUTPUT_OD, .pin_bit_mask = (1ULL<<GPIO_HUMIDITY) };
    gpio_config(&adc0_gpio);
    gpio_config(&hall_gpio);
    gpio_config(&hg_gpio);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_11);

    gpio_install_isr_service(0);

    // gpio_isr_handler_add(GPIO_HALL_SENSOR, isr_hall_sensor, (void*) GPIO_HALL_SENSOR);

    while(1){

        uint16_t temp_raw  = adc1_get_raw(ADC1_CHANNEL_0);
        float temp_voltage = (temp_raw*3.3/4096);
        float temp         = (temp_voltage*100.0);
        
        hg_info_t humidity_info = hg_read(GPIO_HUMIDITY);

        ESP_LOGI(TAG, "Hall: [%i] - Temperatura: [%f] - Humidade HG: [%f] - Temperatura HG: [%f]", gpio_get_level(GPIO_HALL_SENSOR), temp, humidity_info.humidity/10.0, humidity_info.temperature/10.0);

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
