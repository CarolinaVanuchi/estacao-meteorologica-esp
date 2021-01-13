#include "cJSON.h"
#include <stdint.h>

#ifndef _MODEL_
#define _MODEL_

typedef struct weather_station_data_t weather_station_data_t;

volatile weather_station_data_t *global_internal_weather_station_data_pointer;

struct weather_station_data_t {
    double temp;
    double humidity;
    double incidency_sun;
    double precipitation;
};

void set_weather_station(weather_station_data_t *data) {
    global_internal_weather_station_data_pointer = data;
}

weather_station_data_t get_weather_station() {
    return *global_internal_weather_station_data_pointer;
}

static char * weather_station_to_json(weather_station_data_t field) {

    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "temp", field.temp);
    cJSON_AddNumberToObject(root, "humidity", field.humidity);
    cJSON_AddNumberToObject(root, "incidency_sun", field.incidency_sun);
    cJSON_AddNumberToObject(root, "precipitation", field.precipitation);  
    

    char *buffer = cJSON_Print(root);

    cJSON_Delete(root);

    return buffer;
}

#endif