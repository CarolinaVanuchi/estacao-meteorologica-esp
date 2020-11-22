#include "cJSON.h"
#include <stdint.h>

#ifndef _MODEL_
#define _MODEL_

typedef struct weather_station_data_t weather_station_data_t;

volatile weather_station_data_t *global_internal_weather_station_data_pointer;

struct weather_station_data_t {
    double temp_maxima;
    double temp_minina;
    double temp_instantanea;

    double chuva_intensidade;

    double inc_maxima;
    double inc_minima;
    double inc_instantanea;  
} ;

void set_weather_station(weather_station_data_t *data) {
    global_internal_weather_station_data_pointer = data;
}

weather_station_data_t get_weather_station() {
    return *global_internal_weather_station_data_pointer;
}

static char * weather_station_to_json(weather_station_data_t field) {

    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "temp_maxima", field.temp_maxima);
    cJSON_AddNumberToObject(root, "temp_minina", field.temp_minina);
    cJSON_AddNumberToObject(root, "temp_instantanea", field.temp_instantanea);

    cJSON_AddNumberToObject(root, "chuva_intensidade", field.chuva_intensidade);

    cJSON_AddNumberToObject(root, "inc_maxima", field.inc_maxima);
    cJSON_AddNumberToObject(root, "inc_minima", field.inc_maxima);
    cJSON_AddNumberToObject(root, "inc_instantanea", field.inc_maxima);

    char *buffer = cJSON_Print(root);

    cJSON_Delete(root);

    return buffer;
}

#endif