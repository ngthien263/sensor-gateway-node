#include <string.h>  
#include <unistd.h>  
#include <sys/time.h>
#include "sensor_sender.h"
#include "common.h"

sensor_node_t* sensor_create(int sensor_id){
    sensor_node_t* new_sensor = (sensor_node_t*)malloc(sizeof(sensor_node_t));
    new_sensor->id = sensor_id;
    new_sensor->next = NULL;
    return new_sensor;
}

void add_sensor(sensor_node_t** head_sensor, int sensor_id){
    sensor_node_t* new_sensor = sensor_create(sensor_id);
    if(*head_sensor == NULL){
        *head_sensor = new_sensor;
        return;
    }
    sensor_node_t* temp = *head_sensor;
    while(temp->next != NULL){
        temp = temp->next;
    }
    temp->next = new_sensor;
}

void rm_sensor(sensor_node_t** head_sensor, int sensor_id){
    sensor_node_t* curr = *head_sensor;
    sensor_node_t* prev = NULL;
    while (curr != NULL) {
        if(curr->id == sensor_id){
            if(prev == NULL) {
                *head_sensor = curr->next;
            } else
                prev->next = curr->next;
            free(curr);
            printf("Sensor ID %d removed\n", sensor_id);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
    printf("Sensor ID %d not found\n", sensor_id);
}

void dump_sensor(sensor_node_t** head_sensor) {
    int count = 1;
    sensor_node_t* temp = *head_sensor;
    while(temp != NULL){
        printf("%d. ID: %d\n", count++, temp->id);
        temp = temp->next; 
    }
}

static void generate_packet(int id, sensor_packet_t* pkt)
{
    pkt->timestamp   = (uint32_t)time(NULL);
    pkt->id          = id;
    pkt->data.temperature = rand() % 100 + 1;
    pkt->data.humidity    = rand() % 100 + 1;
}

int sensor_send_message(sensor_node_t** sensor_head, int server_fd) {
    int ret;
    sensor_packet_t pkt;
    
    sensor_node_t* curr = *sensor_head;
    while(curr != NULL){
        generate_packet(curr->id, &pkt);
        ret = write(server_fd, &pkt, sizeof(pkt));
        if(ret != sizeof(pkt)) {
            perror("write()");
            ret = -1;
        }
        curr = curr->next;
    }
    return ret;
}