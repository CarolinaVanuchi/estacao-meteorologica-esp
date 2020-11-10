#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"
#include "protocol_examples_common.h"
#include <esp_https_server.h>
#include "model.h"

static const char *TAG = "estacao-meteorologica";


esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err) {
    if(strcmp("/api/temperature", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "URI is not available");
        return ESP_OK;
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "ERROR 404");
    return ESP_FAIL;
}


static esp_err_t send_data(httpd_req_t *req) {
   
    wheater_station_data_t data[] = {
        {
            .temp_maxima        = 27.8,
            .temp_minina        = 13.4,
            .temp_instantanea   = 21.6,
            .chuva_intensidade  = 20.0,
            .inc_maxima         = 2.6,
            .inc_minima         = 3.5,
            .inc_instantanea    = 1.5
        },
        {
            .temp_maxima        = 37.8,
            .temp_minina        = 23.4,
            .temp_instantanea   = 11.6,
            .chuva_intensidade  = 50.0,
            .inc_maxima         = 32.26,
            .inc_minima         = 33.2,
            .inc_instantanea    = 14.55
        }
    };

    
    char *buffer = wheater_station_array_to_json(data, 2);

    httpd_resp_sendstr(req, buffer);

    free(buffer);

    return ESP_OK;
}


static const httpd_uri_t httpd_uri_wheater_station = {
    .uri       = "/api/wheater_station",
    .method    = HTTP_GET,
    .handler   = send_data,
};


static httpd_handle_t start_webserver(void){

    httpd_handle_t server = NULL;

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    conf.port_secure = 3333;
    ESP_LOGI(TAG, "Starting server: '%d'", conf.port_secure);

    extern const unsigned char cacert_pem_start[] asm("_binary_ca_cert_pem_start");
    extern const unsigned char cacert_pem_end[]   asm("_binary_ca_cert_pem_end");
    conf.cacert_pem = cacert_pem_start;
    conf.cacert_len = cacert_pem_end - cacert_pem_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_ca_key_pem_start");
    extern const unsigned char prvtkey_pem_end[]   asm("_binary_ca_key_pem_end");
    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    esp_err_t ret = httpd_ssl_start(&server, &conf);

     if (ESP_OK != ret) {
        ESP_LOGI(TAG, "Error starting server!");
        return NULL;
    }

    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &httpd_uri_wheater_station);
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
}
