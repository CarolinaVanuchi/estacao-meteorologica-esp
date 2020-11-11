#include "cJSON.h"
#include <stdint.h>

#ifndef _MODEL_
#define _MODEL_

typedef struct {
    double temp_maxima;
    double temp_minina;
    double temp_instantanea;

    double chuva_intensidade;

    double inc_maxima;
    double inc_minima;
    double inc_instantanea;
} weather_station_data_t;


static char * weather_station_array_to_json(weather_station_data_t fields[], uint16_t size) {

    cJSON *root = cJSON_CreateObject();

    for (uint16_t i = 0; i < size; i++) {    
        cJSON_AddNumberToObject(root, "temp_maxima", fields[i].temp_maxima);
        cJSON_AddNumberToObject(root, "temp_minina", fields[i].temp_minina);
        cJSON_AddNumberToObject(root, "temp_instantanea", fields[i].temp_instantanea);

        cJSON_AddNumberToObject(root, "chuva_intensidade", fields[i].chuva_intensidade);

        cJSON_AddNumberToObject(root, "inc_maxima", fields[i].inc_maxima);
        cJSON_AddNumberToObject(root, "inc_minima", fields[i].inc_maxima);
        cJSON_AddNumberToObject(root, "inc_instantanea", fields[i].inc_maxima);
    }
    
    char *buffer = cJSON_Print(root);

    cJSON_Delete(root);

    return buffer;
}

#endif