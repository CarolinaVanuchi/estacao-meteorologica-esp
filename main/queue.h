#ifndef _QUEUE_HEADER_
#define _QUEUE_HEADER_

#include <stdlib.h>

typedef struct{
    double *array;
    size_t size;
    size_t elements;
}queue_double_t;

queue_double_t *queue_new(size_t size){
    queue_double_t *queue = (queue_double_t *)calloc(1, sizeof(*queue));

    queue->size = size;
    queue->elements = 0;
    queue->array = (double *)calloc(size, sizeof(double));

    return queue;
}

void queue_delete(queue_double_t *queue){
    free(queue->array);
    free(queue);
}

void queue_add(queue_double_t *queue, double value){
    for(__int64_t i = queue->size - 1; i >= 0; i--){
        if(i != 0)
            queue->array[i] = queue->array[i - 1];
    }

    queue->array[0] = value;

    if(queue->elements < queue->size)
        queue->elements++;
}

double queue_average(queue_double_t *queue){
    double avr = 0;
    for(__int64_t i = queue->elements - 1; i >= 0; i--){
        avr += queue->array[i];
    }

    avr /= queue->elements;

    return avr;
}

#endif