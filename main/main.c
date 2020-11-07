#include <stdio.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include <esp_http_server.h>
#include "cJSON.h"
#include "protocol_examples_common.h"

static const char *TAG = "estacao-meteorologica";

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err) {
    if(strcmp("/api/temperature", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "URI is not available");
        return ESP_OK;
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "ERROR 404");
    return ESP_FAIL;
}

static esp_err_t get_temperature(httpd_req_t *req) {
    // const char resp[] = "GET Temperature";
    // httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN); 
   	
   struct record
    {
        const char *precision;
        double lat;
        double lon;
        const char *city;
        const char *state;
      
    };

    struct record fields[2] =
        {
            {
                "zip",
                37.7668,
                -1.223959e+2,
                "SAN FRANCISCO",
                "CA",      
            },
            {
                "zip",
                37.371991,
                -1.22026e+2,
                "SUNNYVALE",
                "CA",
            }
        };
   

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "precision", fields[0].precision);
    cJSON_AddNumberToObject(root, "lat", fields[0].lat);
    cJSON_AddNumberToObject(root, "lot", fields[0].lon);
    cJSON_AddStringToObject(root, "city", fields[0].city);
    cJSON_AddStringToObject(root, "state", fields[0].state);
    
    char *buffer = cJSON_Print(root);

    httpd_resp_sendstr(req, buffer);

    free(buffer);

    cJSON_Delete(root);

    return ESP_OK;
}

static const httpd_uri_t temperature = {
    .uri       = "/api/temperature",
    .method    = HTTP_GET,
    .handler   = get_temperature,
    .user_ctx  = NULL
};
//rain intensity
//solar_incidence

static httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
      
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &temperature);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server) {
   if (server) httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

void app_main(void) {
    static httpd_handle_t server = NULL;
    
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.
     */
    #ifdef CONFIG_EXAMPLE_CONNECT_WIFI
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
    #endif // CONFIG_EXAMPLE_CONNECT_WIFI
    #ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
        ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
    #endif // CONFIG_EXAMPLE_CONNECT_ETHERNET

    server = start_webserver();
}
