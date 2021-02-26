#ifndef _RAIN_GAUGE_
#define _RAIN_GAUGE_

#include <esp_types.h>
#include <esp_err.h>

// if a ratio is defined for precipitation use it, otherwise the default value is 1.0.
// the ration is given by a string
#ifdef CONFIG_RAIN_GAUGE_RATIO_COUNTS_PER_MM
    #define RAIN_GAUGE_RATIO_COUNTS_PER_MM (CONFIG_RAIN_GAUGE_RATIO_COUNTS_PER_MM/1000.0);
#else
    #define RAIN_GAUGE_RATIO_COUNTS_PER_MM 1.0;
#endif

typedef struct{
    uint32_t counts;
    float ratio_counts_per_mm;
    float precipitation_inst;  
    float precipitation_mm_min;  
}rain_data_t;

rain_data_t *global_rain_data = NULL;;

#endif