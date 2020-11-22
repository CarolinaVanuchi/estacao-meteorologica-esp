#ifndef _ONE_WIRE_HG_SENSOR_
#define _ONE_WIRE_HG_SENSOR_

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_types.h>
#include <esp_event.h>
#include <esp_log.h>
#include <driver/periph_ctrl.h>
#include <driver/timer.h>
#include <soc/gpio_struct.h>

/* --------------------------------------------------------- */

#define PACKET_BYTES_QTY 5 

#pragma pack(push,1)

    typedef union{

        struct{
            uint8_t checksum;
            uint16_t temperature;
            uint16_t humidity;
        };
        
        uint8_t byte[PACKET_BYTES_QTY];

    } hg_packet_t;

#pragma pack(pop)


typedef struct{
    hg_packet_t raw;
    float humidity;
    float temperature;
} hg_sensor_t;

/* --------------------------------------------------------- */

volatile int64_t micro_s = 0;
volatile hg_sensor_t sensor_data;
volatile uint8_t pos_edge_gpio_humidity;
volatile int64_t temp[PACKET_BYTES_QTY*8*2 + 2];
volatile int i;

/* --------------------------------------------------------- */

hg_sensor_t hg_read(gpio_num_t port){
    
    vTaskSuspendAll(); // prevent other tasks

    // send a high-low-high signal to sensor to send data
    // set low for 1 ms
    gpio_set_level(port,0);
    micro_s = esp_timer_get_time();
    while(esp_timer_get_time() < micro_s + 1000);
    // set high for 40 us
    micro_s = esp_timer_get_time();
    gpio_set_level(port,1);
    while(esp_timer_get_time() < micro_s + 40);
    // wait for the sensor low time, until it pulls the bus high
    // pos_edge_gpio_humidity = 0;
    while(gpio_get_level(port) != 1);

    for(int i = 0; i < (PACKET_BYTES_QTY*8*2 + 2); i += 2){
        while(gpio_get_level(port) != 0);
        temp[i + 0] = esp_timer_get_time();
        while(gpio_get_level(port) != 1);
        temp[i + 1] = esp_timer_get_time();
    }

    xTaskResumeAll();

    for(int i = 0; i < PACKET_BYTES_QTY; i++){
        sensor_data.raw.byte[PACKET_BYTES_QTY-1-i] = 0;

        for(int j = 0; j < 8; j++){
            int bit = (temp[2*(j+i*8) + 2] - temp[2*(j+i*8) + 1]) > ( temp[2*(j+i*8) + 1] - temp[2*(j+i*8) + 0]);
            sensor_data.raw.byte[PACKET_BYTES_QTY-1-i] |= (bit<<(8-1-j)); 
        }
    }

    // ESP_LOGW("Humidity sensor", "Sensor raw: 0x%02X%02X%02X%02X%02X - Sizeof(hg_info_t) = [%u]", sensor_data.byte[4], sensor_data.byte[3], sensor_data.byte[2], sensor_data.byte[1], sensor_data.byte[0], sizeof(hg_info_t));

    //calculate checksum
    uint16_t checksum = 0;
    for(uint8_t i = PACKET_BYTES_QTY-1; i > 0; i--){
        checksum += sensor_data.raw.byte[i];
    }

    //errors
    if((checksum & 0xFF) != sensor_data.raw.checksum){
        ESP_LOGW("Humidity sensor", "Humidity sensor checksum failed! [%04X] - [%02X]", checksum, sensor_data.raw.checksum);    
    }

    sensor_data.humidity = sensor_data.raw.humidity / 10.0;

    if(sensor_data.raw.temperature & 0x8000) // if last bit is one then is a negative temperature
        sensor_data.temperature = -(sensor_data.raw.temperature & (~0x8000)) / 10.0;
    else
        sensor_data.temperature = sensor_data.raw.temperature / 10.0;

    return sensor_data; 
}

#endif