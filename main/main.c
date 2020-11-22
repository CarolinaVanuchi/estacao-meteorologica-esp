#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/adc.h>
#include "esp_netif.h"
#include "esp_eth.h"
#include "model.h"
#include "routes.h"
#include "embbeded_files.h"
#include "one_wire_hg_sensor.h"
#include "utils_https.h"
#include "utils_connect.h"

volatile int HALL = 0;

static void IRAM_ATTR isr_hall_sensor(void *arg) {
    HALL = 1;
}

void app_main(void) {
    
    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    ESP_ERROR_CHECK(connect_wifi_connect());

    gpio_config_t adc0_gpio         = { .mode = GPIO_MODE_INPUT, .pin_bit_mask = (1ULL<<GPIO_TEMPERATURE) };
    gpio_config_t hall_gpio         = { .intr_type = GPIO_INTR_NEGEDGE, .mode = GPIO_MODE_INPUT, .pin_bit_mask = (1ULL<<GPIO_HALL_SENSOR) };
    gpio_config_t hg_gpio           = { .intr_type = GPIO_INTR_ANYEDGE, .mode = GPIO_MODE_INPUT_OUTPUT_OD, .pin_bit_mask = (1ULL<<GPIO_HUMIDITY) };
    gpio_config(&adc0_gpio);
    gpio_config(&hall_gpio);
    gpio_config(&hg_gpio);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_11);

    gpio_install_isr_service(0);

    while(1){

        uint16_t temp_raw  = adc1_get_raw(ADC1_CHANNEL_0);
        float temp_voltage = (temp_raw*3.3/4096);
        float temp         = (temp_voltage*100.0);
        
        hg_sensor_t humidity_info = hg_read(GPIO_HUMIDITY);

        ESP_LOGI(TAG, "Hall: [%i] - Temperatura: [%f] - Humidade HG: [%f] - Temperatura HG: [%f]", gpio_get_level(GPIO_HALL_SENSOR), temp, humidity_info.humidity, humidity_info.temperature);

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
