#ifndef _ONE_WIRE_HG_SENSOR_
#define _ONE_WIRE_HG_SENSOR_

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <sys/param.h>
#include <esp_event.h>
#include <esp_log.h>
#include <gpio.h>
#include "config_symbols.h"

volatile uint16_t timer_1us = 0;

static void IRAM_ATTR isr_hall_sensor(void *arg){
    timer_1us += 1;
}

typedef struct{
    uint16_t humidity;
    uint16_t temperature;
    uint8_t CRC32;
} hg_info_t;

hg_info_t hg_read(gpio_num_t port){
    gpio_isr_handler_add(port, isr_hall_sensor, (void*)port);

    timer_1us = 0;
    gpio_set_level(port,0);
    while(timer_1us < 1000){}
    timer_1us = 0;
    gpio_set_level(port,1);
    while(timer_1us < 40){}
    timer_1us = 0;
    while(timer_1us < 2*80){}
    timer_1us = 0;
    
    
    //
}

#endif