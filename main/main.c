#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/adc.h>
#include <esp_netif.h>
#include <esp_eth.h>
#include "model.h"
#include "routes.h"
#include "embbeded_files.h"
#include "one_wire_hg_sensor.h"
#include "rain_gauge.h"
#include "utils_https.h"
#include "utils_connect.h"
#include "queue.h"

void app_main(void) {
    
    static httpd_handle_t server = NULL;
    weather_station_data_t objeto; 
    hg_sensor_t humidity_info;
    rain_data_t rain_data;
    uint8_t hall_sensor;
    uint8_t hall_sensor_tmp;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    ESP_ERROR_CHECK(connect_wifi_connect());

    gpio_config_t adc0_gpio         = { .mode = GPIO_MODE_INPUT, .pin_bit_mask = (1ULL<<CONFIG_GPIO_TEMPERATURE) };
    gpio_config_t hall_gpio         = { .mode = GPIO_MODE_INPUT, .pin_bit_mask = (1ULL<<CONFIG_GPIO_HALL_SENSOR) };
    gpio_config_t hg_gpio           = { .intr_type = GPIO_INTR_ANYEDGE, .mode = GPIO_MODE_INPUT_OUTPUT_OD, .pin_bit_mask = (1ULL<<CONFIG_GPIO_HUMIDITY) };
    gpio_config_t solar_incidence   = { .mode = GPIO_MODE_INPUT, .pin_bit_mask =  (1ULL<<35) };
    
    gpio_config(&adc0_gpio);
    gpio_config(&hall_gpio);
    gpio_config(&hg_gpio);
    gpio_config(&solar_incidence);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);

    gpio_install_isr_service(0);

    set_weather_station(&objeto);                                   // set main weather object
    objeto.humidity = 0.0;
    objeto.incidency_sun = 0.0;
    objeto.precipitation = 0.0;
    objeto.temp = 0.0;
                                                                                    
    rain_data.ratio_counts_per_mm = RAIN_GAUGE_RATIO_COUNTS_PER_MM; // rain gauge initialization information
    rain_data.counts = 0;
    rain_data.precipitation_inst = 0.0;

    hall_sensor_tmp = gpio_get_level(CONFIG_GPIO_HALL_SENSOR);      // save hall state
    int64_t last_call = esp_timer_get_time();                       // get time for calling humidity sensor
    esp_err_t hg_code = ESP_ERR_HG_OK;                              // ret code from humidity sensor reading

    queue_double_t *temp_queue = queue_new(50);
    queue_double_t *solar_queue = queue_new(30);

    global_rain_data = &rain_data;

    while(1){

        /* Temperature */
        // uint16_t temp_raw  = adc1_get_raw(ADC1_CHANNEL_0);
        // float temp_voltage = (temp_raw*3.3/4096);
        // float temp         = (temp_voltage*100.0);
        // queue_add(temp_queue, (double)temp);
        // objeto.temp        = queue_average(temp_queue);
        // // ESP_LOGI(__FILE__, "Temperatura: [%f]", temp);
        
        /* Humidity */  
        if(esp_timer_get_time() - last_call > HG_SENSOR_COOLDOWN_TIME_US){
            hg_code = hg_read(CONFIG_GPIO_HUMIDITY, &humidity_info);

            // if error has occured
            if(hg_code < 0){
                ESP_LOGE(__FILE__, "%s", esp_err_hg_to_name(hg_code));
                objeto.humidity = NaN_double.nan;
                objeto.temp     = NaN_double.nan;
            }
            else{
                // ESP_LOGI(__FILE__, "Humidade: [%f]", humidity_info.humidity);
                objeto.humidity = humidity_info.humidity;
                objeto.temp     = humidity_info.temperature;
            }
            
            last_call = esp_timer_get_time();
        }

        /* Solar incidence */
        objeto.incidency_sun = NaN_double.nan;
        uint16_t solar_incidence_raw = adc1_get_raw(ADC1_CHANNEL_7);
        queue_add(solar_queue, solar_incidence_raw);
        objeto.incidency_sun = queue_average(solar_queue);

        /* Rain gauge */
        hall_sensor = gpio_get_level(CONFIG_GPIO_HALL_SENSOR);
        if(hall_sensor == 0 && hall_sensor_tmp == 1){       // falling edge detection
            rain_data.counts++;
            objeto.precipitation = rain_data.counts;
        }
        hall_sensor_tmp = hall_sensor;



        // ESP_LOGI(__FILE__, "Hall_Counts: [%u] - Precipitation_mm: [%f] - Ratio: [%f]", rain_data.counts, rain_data.precipitation_inst, rain_data.ratio_counts_per_mm);

        // Main log 
        ESP_LOGI(__FILE__, "HALL [%i] - Precipitation: [%f] - Temperature: [%f] - Humidity: [%f] - Solar Incidence: [%f]\n", hall_sensor, objeto.precipitation, objeto.temp, objeto.humidity, objeto.incidency_sun);
    }
}
