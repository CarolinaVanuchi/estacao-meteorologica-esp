#ifndef _RAIN_GAUGE_
#define _RAIN_GAUGE_

#include <esp_types.h>
#include <esp_err.h>

typedef struct{
    uint32_t counts;
    float ratio_counts_per_mm;
    float precipitation_inst;  
    float precipitation_mm_min;  
}rain_data_t;

#endif