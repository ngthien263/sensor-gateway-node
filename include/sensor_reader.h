#ifndef SENSOR_READER_H
#define SENSOR_READER_H
#include "sensor_types.h"

int sensor_read(sensor_info_t* sensor, int socket_fd);

#endif