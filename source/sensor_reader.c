#include "sensor_reader.h"
#include <string.h>      
#include <sys/socket.h>         
#include <errno.h>
#include "common.h"

static void time_to_string(time_t t, char* buffer) {
    struct tm timeinfo;
    localtime_r(&t, &timeinfo);
    snprintf(buffer, 64, "%02d:%02d:%02d",
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);
}

int sensor_read(sensor_info_t* sensor, int socket_fd){
    sensor_packet_t pkt;
    ssize_t recv_bytes;
    int empty_index = -1;
    size_t total_read = 0;

    char* ptr = (char*)&pkt;

    while(total_read < sizeof(sensor_packet_t)) {
        recv_bytes = recv(socket_fd, ptr + total_read, sizeof(sensor_packet_t) - total_read,0);
        if (recv_bytes == 0) {
            return -1; 
        }
        if (recv_bytes < 0) {
            perror("recv");
            return -1;
        }
        total_read += recv_bytes;
    }

    for (int i = 0; i < MAX_SENSORS; i++) {
        if (sensor[i].state.connected && sensor[i].id == pkt.id) {
            sensor[i].data = pkt.data;
            time_to_string((time_t)pkt.timestamp,
                        sensor[i].timestamp);

            return 0; 
        }

        if (!sensor[i].state.connected && empty_index == -1) {
            empty_index = i;
        }
    }

    if (empty_index != -1) {
        sensor[empty_index].id = pkt.id;
        sensor[empty_index].data = pkt.data;
        sensor[empty_index].state.new = true;
        sensor[empty_index].state.connected = true;

        time_to_string((time_t)pkt.timestamp,
                    sensor[empty_index].timestamp);

        return 0;
    }

    return -1;
}
