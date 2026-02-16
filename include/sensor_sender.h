#ifndef SENSOR_SENDER_H
#define SENSOR_SENDER_H
#include "sensor_types.h"

typedef struct sensor_node {
    int id;
    struct sensor_node* next;
} sensor_node_t;

sensor_info_t sensor_init(int sensor_id);
sensor_node_t* sensor_create(int sensor_id);
void add_sensor(sensor_node_t** head_sensor, int sensor_id);
void rm_sensor(sensor_node_t** head_sensor, int sensor_id);
void dump_sensor(sensor_node_t** head_sensor);
int sensor_send_message(sensor_node_t** sensor_head, int server_fd);
#endif