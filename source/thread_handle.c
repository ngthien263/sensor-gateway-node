#include <fcntl.h>      
#include <stdbool.h>
#include <unistd.h>    
#include <string.h>
#include "common.h"
#include "thread_handle.h"
#include "sensor_reader.h"
#include "database.h"
sensor_info_t sensors[MAX_SENSORS];
bool data_ready_dmng_thr = 0;
bool data_ready_storage_thr = 0;
void* connect_thread_handler(void* args){
    char buffer[1024];
    thr_handle_t* handle = (thr_handle_t*)args;
    int socket_fd = socket_connect(&handle->thr_socket);
    int f_fd = open("./log_fifo", O_WRONLY);
    if (f_fd < 0) handle_error("open fifo ");
    while(1){
        if (sensor_read(sensors, socket_fd) == 0);
        pthread_mutex_lock(&handle->mlock);
        for(int i = 0; i < MAX_SENSORS; i++){
            if(sensors[i].state.connected && sensors[i].state.new){
                int len = sprintf(buffer, "%d. %s A sensor node with ID %d has opened a new connection\n", handle->sequence_number,
                                                                                                           sensors[i].timestamp,
                                                                                                           sensors[i].id);
                printf(buffer, "%d. %s A sensor node with ID %d has opened a new connection\n", handle->sequence_number,
                                                                                                sensors[i].timestamp,
                                                                                                sensors[i].id);
                write(f_fd, buffer, len);
                sensors[i].state.new = false;
                handle->sequence_number++;
            }
        }
        data_ready_dmng_thr = 1;
        data_ready_storage_thr = 1;
        pthread_cond_broadcast(&handle->cvar);   
        pthread_mutex_unlock(&handle->mlock);
    }  
    close(f_fd);
    pthread_exit(NULL);
}

float avg_temp;
float avg_humid;

void* data_manager_thread_handler(void* args){
    thr_handle_t* handle = (thr_handle_t*)args;
    char buffer[1024];
    int f_fd = open("./log_fifo", O_WRONLY);
    if(f_fd < 0)   handle_error("open fifo");
    
    struct storage {int id; float temp; float humid; int division; bool new_data;};
    static struct storage storage_arr[MAX_SENSORS];
    while(1){
        pthread_mutex_lock(&handle->mlock); 
        while(!data_ready_dmng_thr)
            pthread_cond_wait(&handle->cvar, &handle->mlock);
        for(int i = 0; i < MAX_SENSORS; i++){
            if(sensors[i].state.connected){
                storage_arr[i].id = sensors[i].id;
                storage_arr[i].temp += sensors[i].data.temperature;
                storage_arr[i].humid += sensors[i].data.humidity;
                storage_arr[i].division++;
            }
        }
        data_ready_dmng_thr = 0;
        pthread_mutex_unlock(&handle->mlock);

        int len = 0;
        for(int i = 0; i < MAX_SENSORS; i++){
            if(storage_arr[i].division > 0){
                avg_temp = storage_arr[i].temp/storage_arr[i].division;
                avg_humid = storage_arr[i].temp/storage_arr[i].division;
                if(avg_temp > TEMP_UPPER_LIMIT){
                    len = sprintf(buffer, "[Data] Sensor %d: Too hot (avg=%.2f)\n", storage_arr[i].id, avg_temp); 
                    printf(buffer, "[Data] Sensor %d: Too hot (avg=%.2f)\n", storage_arr[i].id, avg_temp); 
                } else if (avg_temp < TEMP_LOWER_LIMIT){
                    len = sprintf(buffer, "[Data] Sensor %d: Too cold (avg=%.2f)\n", storage_arr[i].id, avg_temp);   
                    printf(buffer, "[Data] Sensor %d: Too hot (avg=%.2f)\n", storage_arr[i].id, avg_temp);  
                } else{
                    len = sprintf(buffer, "[Data] Sensor %d: Normal temperature (avg = %.2f)\n", storage_arr[i].id, avg_temp);   
                    printf(buffer, "[Data] Sensor %d: Too hot (avg=%.2f)\n", storage_arr[i].id, avg_temp); 
                }
                if(write(f_fd, buffer, len) < 0)
                    handle_error("write");
            }
        }
    }
    close(f_fd);
    pthread_exit(NULL);
}

void* database_thread(void* args) {
    thr_handle_t* handle = (thr_handle_t*)args;

    if (!db_init("./sensors.db")) {
        perror("db_init");
        pthread_exit(NULL);
    }

    while (1) {
        pthread_mutex_lock(&handle->mlock);
        while(!data_ready_storage_thr)
            pthread_cond_wait(&handle->cvar, &handle->mlock);
        for (int i = 0; i < MAX_SENSORS; i++) {
            if (sensors[i].state.connected) {
                db_save_state(
                    sensors[i].id,
                    sensors[i].timestamp,
                    sensors[i].data.temperature,
                    sensors[i].data.humidity,
                    sensors[i].data.temperature 
                );
            }
        }
        data_ready_storage_thr = 0;
        pthread_mutex_unlock(&handle->mlock);
    }
    db_close();
    pthread_exit(NULL);
}