#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <time.h>
#include <stdio.h>      
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#define BUFF_SIZE 256
#define MAX_SENSORS 10
#define TEMP_UPPER_LIMIT 50
#define TEMP_LOWER_LIMIT 20

typedef struct {
    float temperature;
    float humidity;
} sensor_data_t;

typedef struct {
    bool new;
    bool connected;
} sensor_state_t;

typedef struct __attribute__((packed)){
    char timestamp[64];
    uint16_t id;
    sensor_data_t data;
    sensor_state_t state;
} sensor_info_t;

typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    uint16_t id;
    sensor_data_t data;
} sensor_packet_t;

#endif 