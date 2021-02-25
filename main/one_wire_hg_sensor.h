#ifndef _ONE_WIRE_HG_SENSOR_
#define _ONE_WIRE_HG_SENSOR_

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_types.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <driver/periph_ctrl.h>
#include <driver/timer.h>
#include <soc/gpio_struct.h>
#include <esp_err.h>

/* --------------------------------------------------------- */

#define PACKET_BYTES_QTY 5 

// if kconfig option is defined then its value is used, otherwise has a 500 ms cooldown
#ifdef CONFIG_HUMIDITY_READ_COOLDOWN_MS
    #define HG_SENSOR_COOLDOWN_TIME_US (CONFIG_HUMIDITY_READ_COOLDOWN_MS*1000)
#else
    #define HG_SENSOR_COOLDOWN_TIME_US 500000 
#endif 


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

// enumerator with errors
typedef enum{
    ESP_ERR_HG_TIMEOUT                      =  -1, 
    ESP_ERR_HG_POINTER_TO_NULL              =  -2, 
    ESP_ERR_HG_FAIL_TO_READ_RESPONSE_BITS   =  -3, 
    ESP_ERR_HG_SENSOR_ON_COOLDOWN_TIME      =  -4, 
    ESP_ERR_HG_OK                           =   0, 
    ESP_ERR_HG_CHECKSUM_FAILED              =   1, 
}ESP_ERR_HG_t; 

// macro for building a struct with value 'err' and also name "err"
#define ERR_TO_STRUCT_HG(err)  {err, #err} 

// struct for code and messages
typedef struct{
    int8_t code;
    const char *msg;
}esp_err_msg_custom_main_component_t;

// table for error messages
static const esp_err_msg_custom_main_component_t esp_err_msg_hg_table[] = {
    ERR_TO_STRUCT_HG(ESP_ERR_HG_TIMEOUT),
    ERR_TO_STRUCT_HG(ESP_ERR_HG_POINTER_TO_NULL),
    ERR_TO_STRUCT_HG(ESP_ERR_HG_FAIL_TO_READ_RESPONSE_BITS), 
    ERR_TO_STRUCT_HG(ESP_ERR_HG_SENSOR_ON_COOLDOWN_TIME),   
    ERR_TO_STRUCT_HG(ESP_ERR_HG_OK),
    ERR_TO_STRUCT_HG(ESP_ERR_HG_CHECKSUM_FAILED)
};

const char *esp_err_hg_to_name(int8_t code){
    size_t esp_err_msg_hg_table_size = sizeof(esp_err_msg_hg_table)/sizeof(esp_err_msg_hg_table[0]);

    for(int i = 0; i < esp_err_msg_hg_table_size; i++){
        if(esp_err_msg_hg_table[i].code == code){
            return esp_err_msg_hg_table[i].msg;
        }
    }

    return "one_wire_hg_sensor.h: Not a valid error code!";
}

/* --------------------------------------------------------- */

esp_err_t hg_read(gpio_num_t port, hg_sensor_t *sensor_data){
    
    int64_t micro_s = 0;
    int64_t temp[PACKET_BYTES_QTY*8*2 + 2];
    static int64_t last_time_called;

    if(sensor_data == NULL){                   
        return ESP_ERR_HG_POINTER_TO_NULL;                          // pointer to NULL struct
    }
    else if(esp_timer_get_time() - last_time_called < HG_SENSOR_COOLDOWN_TIME_US){     
        return ESP_ERR_HG_SENSOR_ON_COOLDOWN_TIME;                  // each call should have a 1 sec time interval
    }

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
    // wait for the sensor low time (80 us), until it pulls the bus high (for another 80 us)
    micro_s = esp_timer_get_time();
    while(gpio_get_level(port) != 1){ 
        if( (esp_timer_get_time() - micro_s) > 80 ){ // no response from sensor 
            xTaskResumeAll(); // since logging is handled by the rtos, we should give it control back by the rtos task to act, by resuming all tasks
            return ESP_ERR_HG_TIMEOUT;
        }
    }

    micro_s = esp_timer_get_time();

    // grab times for each transition
    for(int i = 0; i < (PACKET_BYTES_QTY*8*2 + 2); i += 2){
        // there are 5 incoming bytes, therefore a maximum of , 40 high bits that account for 50+70 us each, 
        // 120us*40bits = 4800 us to be waited for all the bits to come ~ 7000 us
        while(gpio_get_level(port) != 0){ if( (esp_timer_get_time() - micro_s) > 7000 ){ xTaskResumeAll(); return ESP_ERR_HG_FAIL_TO_READ_RESPONSE_BITS;}}
        temp[i + 0] = esp_timer_get_time();
        
        while(gpio_get_level(port) != 1){ if( (esp_timer_get_time() - micro_s) > 7000 ){ xTaskResumeAll(); return ESP_ERR_HG_FAIL_TO_READ_RESPONSE_BITS;}}
        temp[i + 1] = esp_timer_get_time();
    }

    xTaskResumeAll();

    // for every packet
    for(int i = 0; i < PACKET_BYTES_QTY; i++){
        sensor_data->raw.byte[PACKET_BYTES_QTY-1-i] = 0;

        // for every bit in a byte
        for(int j = 0; j < 8; j++){
            // if high time is bigger than the low time then it is a high bit, else is a low bit
            int bit = (temp[2*(j+i*8) + 2] - temp[2*(j+i*8) + 1]) > ( temp[2*(j+i*8) + 1] - temp[2*(j+i*8) + 0]);
            sensor_data->raw.byte[PACKET_BYTES_QTY-1-i] |= (bit<<(8-1-j)); 
        }
    }

    //calculate checksum
    uint16_t checksum = 0;
    for(uint8_t i = PACKET_BYTES_QTY-1; i > 0; i--){
        checksum += sensor_data->raw.byte[i];
    }

    //errors
    if((checksum & 0xFF) != sensor_data->raw.checksum){
        return ESP_ERR_HG_CHECKSUM_FAILED;
        // ESP_LOGW("Humidity sensor", "Humidity sensor checksum failed! [%04X] - [%02X]", checksum, sensor_data->raw.checksum);    
    }

    sensor_data->humidity = sensor_data->raw.humidity / 10.0;

    if(sensor_data->raw.temperature & 0x8000) // if last bit is one then is a negative temperature
        sensor_data->temperature = -(sensor_data->raw.temperature & (~0x8000)) / 10.0;
    else
        sensor_data->temperature = sensor_data->raw.temperature / 10.0;

    last_time_called = esp_timer_get_time();
    return ESP_ERR_HG_OK;
}

#endif