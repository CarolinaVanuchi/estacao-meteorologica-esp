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
#include "config_symbols.h"

#define COUNTING_FREQUENCY 1000ULL
#define CLOCK_DIVIDER 8ULL
#define COUNTER_VALUE_1_US ((TIMER_BASE_CLK / CLOCK_DIVIDER) / COUNTING_FREQUENCY)


volatile uint32_t timer_1us = 0;

volatile TaskHandle_t task;

typedef union{
    struct{
        uint16_t humidity;
        int16_t temperature;
        uint8_t checksum;
    };

    uint8_t byte[5];

} hg_info_t;


void IRAM_ATTR isr_timer_1_us(void *arg){
    timer_spinlock_take(TIMER_GROUP_0);
    
    timer_idx_t idx = *((timer_idx_t*)arg);
    BaseType_t high_priority = pdFALSE;

    timer_1us += 1;

    timer_group_set_alarm_value_in_isr(TIMER_GROUP_0, idx, COUNTER_VALUE_1_US);
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, idx);
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, idx);

    vTaskNotifyGiveFromISR(task,&high_priority);
    portYIELD_FROM_ISR(high_priority);

    timer_spinlock_give(TIMER_GROUP_0);
}


void gpio_wait_pos_edge(gpio_port_t port, uint8_t initial_state){

    while(1){ // waits for pos_edge
    
        uint8_t level = gpio_get_level(port);

        if(initial_state != level){ // on change
            if(!initial_state && level) // 0 and 1
                return;

            initial_state = level;
        }
    }
}


void gpio_wait_neg_edge(gpio_port_t port, uint8_t initial_state){

    while(1){ // waits for pos_edge
    
        uint8_t level = gpio_get_level(port);

        if(initial_state != level){ // on change
            if(initial_state && !level) // 1 and 0
                return;

            initial_state = level;
        }
    }
}


hg_info_t hg_read(gpio_num_t port){

    timer_config_t timer_1us_cfg = { 
        .divider = 8, 
        .counter_dir = TIMER_COUNT_UP, 
        .counter_en = TIMER_PAUSE,
        .auto_reload = TIMER_AUTORELOAD_EN,
        .alarm_en = TIMER_ALARM_EN
    };

    timer_idx_t timer_0_1_us = TIMER_0;

    task = xTaskGetCurrentTaskHandle();

    timer_init(TIMER_GROUP_0, timer_0_1_us, &timer_1us_cfg);
    timer_enable_intr(TIMER_GROUP_0, timer_0_1_us);
    timer_isr_register(TIMER_GROUP_0, timer_0_1_us, isr_timer_1_us, (void*)(&timer_0_1_us), ESP_INTR_FLAG_IRAM, NULL);
    timer_set_counter_value(TIMER_GROUP_0, timer_0_1_us, 0x00ULL);
    timer_set_alarm_value(TIMER_GROUP_0, timer_0_1_us, COUNTER_VALUE_1_US);
    timer_set_alarm(TIMER_GROUP_0, timer_0_1_us, TIMER_ALARM_EN);
    timer_start(TIMER_GROUP_0, timer_0_1_us);

    hg_info_t sensor_data;
    hg_info_t error_return = { .humidity = 0, .temperature = 0, .checksum = 0};

    
    // send a high-low-high signal to sensor to send data
    // set low for 1 ms
    gpio_set_level(port,0);
    // ESP_LOGI("Timer", "Timer counter value: [0x%08X%08X]", (uint32_t)(COUNTER_VALUE_1_US>>32), (uint32_t)(COUNTER_VALUE_1_US));
    timer_1us = 0;
    while(timer_1us < 1000){ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000));}
    // set high for 40 us
    timer_1us = 0;
    gpio_set_level(port,1);
    while(timer_1us < 40){ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000));}
    // wait for the sensor low time, until it pulls the bus high
    timer_1us = 0;
    while(!gpio_get_level(port)){ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000));}
    // wait 80 us to start reading the incoming bits
    timer_1us = 0;
    while(timer_1us < 80){ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000));}

    // read bytes
    for(uint8_t i = 0; i < 5; i++){
        uint8_t read_byte = 0;
        uint16_t read_bits = 0;

        // read a byte
        while(read_bits < 8){
            // waiting rising edge of a bit
            gpio_wait_pos_edge(port,gpio_get_level(port));
            ESP_LOGI("Bytes","[1] Byte: [%u] - Bit: [%u] - Timer: [%u]",i,read_bits,timer_1us);
            timer_1us = 0;
            gpio_wait_neg_edge(port,gpio_get_level(port));
            ESP_LOGI("Bytes","[2] Byte: [%u] - Bit: [%u] - Timer: [%u]",i,read_bits,timer_1us);

            ESP_LOGI("Bytes","[3] Byte: [%u] - Bit: [%u] - Timer: [%u]",i,read_bits,timer_1us);

            if(timer_1us > 80){ // bigger than 70 us, a high bit, then we reached the end of the communication 
                break;
            }
            else if(timer_1us > 30) // if high state is maitained by more than 30 us, then it is a high bit
                read_byte |= (1<<read_bits);

            read_bits++;
        }

        if(read_bits < (8 - 1)){ // while above breaked for an error or end of communication
            ESP_LOGW("Humidity sensor","Reading from humidity sensor reached only [%u / 5] bytes and [%u / 8] bits.\nRaw humidity sensed: [%u].\nRaw temperature sensed: [%d].\n Checksum: [%X] Caused by an eletrical error occurance or missreading of protocol", i, read_bits, sensor_data.humidity, sensor_data.temperature, sensor_data.checksum);
            timer_pause(TIMER_GROUP_0, timer_0_1_us);
            return error_return;
        }
        
        sensor_data.byte[i] = read_byte;       
    }


    //calculate checksum
    uint16_t checksum = 0;
    for(uint8_t i = 0; i < (5 - 1); i ++){
        checksum += sensor_data.byte[i];
    }

    if((checksum & 0xFF) != sensor_data.checksum){
        ESP_LOGW("Humidity sensor", "Humidity sensor checksum failed!");
        timer_pause(TIMER_GROUP_0, timer_0_1_us);
        return error_return;
    }

    timer_pause(TIMER_GROUP_0, timer_0_1_us);
    return sensor_data; 
}

#endif