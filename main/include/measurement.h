#pragma once
#ifndef MEASUREMENT_H
#define MEASUREMENT_H

typedef struct
{
    uint32_t timestamp;
    float value;
    char *type;
} measurement_t;

void send_measurement_task(void *pvparameters);

#endif
