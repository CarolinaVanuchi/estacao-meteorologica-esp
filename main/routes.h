#ifndef _ROUTES_
#define _ROUTES_

#include <esp_https_server.h>
#include "model.h"
#include <cJSON.h>
#include "embbeded_files.h"
#include "config_symbols.h"

const char *TAG_ROUTE = "routes.h"; 


/* -------------------- Handlers ------------------------*/


static esp_err_t httpd_uri_api_weather_station_handler(httpd_req_t *req) {
   
    weather_station_data_t data[] = {
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

    
    char *buffer = weather_station_array_to_json(data, 2);

    httpd_resp_sendstr(req, buffer);

    free(buffer);

    return ESP_OK;
}

static esp_err_t httpd_uri_login_handler(httpd_req_t *req){
    httpd_resp_set_type(req,"text/html");
    httpd_resp_send(req, (const char*)index_html_start, index_html_end - index_html_start - 1);
    return ESP_OK;
}

static esp_err_t httpd_uri_assets_css_style_css_handler(httpd_req_t *req){
    httpd_resp_set_type(req,"text/css");
    httpd_resp_send(req, (const char*)style_css_start, style_css_end - style_css_start - 1);
    return ESP_OK;
}

static esp_err_t httpd_uri_assets_js_index_js_handler(httpd_req_t *req){
    httpd_resp_set_type(req,"text/javascript");
    httpd_resp_send(req, (const char*)index_js_start, index_js_end - index_js_start - 1);
    return ESP_OK;
}

static esp_err_t httpd_uri_form_handler(httpd_req_t *req){
    ESP_LOGI(TAG_ROUTE,"[DEBUG] Receive POST");

    size_t body_size = ((req->content_len <= 0) ? 1 : req->content_len + 1);
    
    char *body = malloc(body_size);

    int code = httpd_req_recv(req, body, body_size);

    ESP_LOGI(TAG_ROUTE, "[DEBUG] Receive POST code: %i", code);

    if(code <= 0){
        if(code == HTTPD_SOCK_ERR_TIMEOUT){
            httpd_resp_send_408(req);
        }

        free(body);
        return ESP_FAIL;
    }

    body[req->content_len] = '\0';

    ESP_LOGI(TAG_ROUTE,"[DEBUG] Received: %s",body);

    cJSON *credentials_received = cJSON_Parse(body);
    free(body);

    cJSON *username_received_json = cJSON_GetObjectItem(credentials_received,"username");
    cJSON *password_received_json = cJSON_GetObjectItem(credentials_received,"password");

    char *username_received = cJSON_GetStringValue(username_received_json);
    char *password_received = cJSON_GetStringValue(password_received_json);


    if(!strcmp(SERVER_USERNAME, username_received) && !strcmp(SERVER_PASSWORD, password_received)){ // on error
        httpd_resp_set_type(req, "text/plain")
        httpd_resp_send(req,"[ALTERAR] Login feito",22);
        
        ESP_LOGI(TAG_ROUTE, "Login Realizado. User: [%s - %s] / Password: [%s - %s]",SERVER_USERNAME, username_received, SERVER_PASSWORD, password_received);
        cJSON_Delete(credentials_received);
        return ESP_OK;
    }
    else{
        httpd_resp_set_type(req, "text/plain")    
        httpd_resp_send(req,"[ALTERAR] Erro de login",24);
        
        ESP_LOGI(TAG_ROUTE, "Login Negado. User: [%s - %s] / Password: [%s - %s]",SERVER_USERNAME, username_received, SERVER_PASSWORD, password_received);
        cJSON_Delete(credentials_received);
        return ESP_FAIL;
    }

}

/* -------------------- URI Structs ------------------------*/


static const httpd_uri_t httpd_uri_api_weather_station = {
    .uri        = "/api/weather_station",
    .method     = HTTP_GET,
    .handler    = httpd_uri_api_weather_station_handler,
};

static const httpd_uri_t httpd_uri_login = {
    .uri        = "/login",
    .method     = HTTP_GET,
    .handler    = httpd_uri_login_handler,
};

static const httpd_uri_t httpd_uri_assets_css_style_css = {
    .uri        = "/assets/css/style.css",
    .method     = HTTP_GET,
    .handler    = httpd_uri_assets_css_style_css_handler,
};

static const httpd_uri_t httpd_uri_assets_js_index_js = {
    .uri        = "/assets/js/index.js",
    .method     = HTTP_GET,
    .handler    = httpd_uri_assets_js_index_js_handler,
};

static const httpd_uri_t httpd_uri_form = {
    .uri        = "/form",
    .method     = HTTP_POST,
    .handler    = httpd_uri_form_handler,
};


/* -------------------- Register URI's ------------------------*/


esp_err_t httpd_register_uri_routes(httpd_handle_t server){

    httpd_register_uri_handler(server, &httpd_uri_api_weather_station);
    httpd_register_uri_handler(server, &httpd_uri_login);
    httpd_register_uri_handler(server, &httpd_uri_assets_js_index_js);
    httpd_register_uri_handler(server, &httpd_uri_assets_css_style_css);
    httpd_register_uri_handler(server, &httpd_uri_form);

    return ESP_OK;
}

#endif