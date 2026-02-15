#ifndef THREAD_HANDLE_H
#define THREAD_HANDLE_H

#include "socket.h"
#include <pthread.h>
#include "sensor_types.h"
typedef struct {
    pthread_mutex_t mlock;
    pthread_cond_t cvar;
    int sequence_number;
    socket_t thr_socket;
    
} thr_handle_t;
void* connect_thread_handler(void* args);
void* data_manager_thread_handler(void* args);
void* database_thread(void* args);
#endif