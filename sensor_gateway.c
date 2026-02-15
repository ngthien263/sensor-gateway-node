#include "thread_handle.h"
#include "sensor_reader.h"
#include "sensor_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "common.h"

pthread_t connect_thr, datamanager_thr, storagemanager_thr;
pid_t child_pid;

static void signal_handler(int sig) {
    unlink("./log_fifo");
    exit(0);
}

int main(int argc, const char* argv[]) {
    signal(SIGINT, signal_handler);
    int status;
    char buffer[4096];
    if (mkfifo("./log_fifo", 0666) < 0)
        handle_error("mkfifo");
    child_pid = fork();
    if (child_pid < 0) {
        handle_error("fork");
    }
    if(child_pid == 0) {
        int f_fd = open("./log_fifo", O_RDONLY);
        if (f_fd < 0) handle_error("open fifo");
        int log_fd = open("gateway.log", O_CREAT | O_WRONLY | O_APPEND , 0666);
        if (log_fd < 0) handle_error("open log");
        while(1) {
            ssize_t bytes_read = read(f_fd, buffer, sizeof(buffer));
            if(bytes_read > 0){
                write(log_fd, buffer, bytes_read);
            }   else if (bytes_read == 0) {
                sleep(1);
            } else {
                handle_error("read fifo");
            }
        }
        close(log_fd);
        close(f_fd);
        exit(0);
    }
    else if(child_pid > 0) {
        if (argc < 2) {
            printf("No port provided\ncommand: ./server <port number>\n");
            exit(EXIT_FAILURE);
        } 
        int port = atoi(argv[1]);
        socket_t socket_config = {
            .port = port,
            .opt = 1,
            .serv_addr = {
                .sin_family = AF_INET,
                .sin_port = htons(port),
                .sin_addr.s_addr = INADDR_ANY,
            },
        };
        thr_handle_t thr_handle = {
            .sequence_number = 1,
            .thr_socket = socket_config,
        };
        pthread_mutex_init(&thr_handle.mlock, NULL);
        int ret;
        ret = pthread_create(&connect_thr, NULL, connect_thread_handler, &thr_handle);
        if (ret != 0) {
            printf("pthread_create() error number=%d\n", ret);
            return -1;
        }
        ret = pthread_create(&datamanager_thr, NULL, data_manager_thread_handler, &thr_handle);
        if (ret != 0) {
            printf("pthread_create() error number=%d\n", ret);
            return -1;
        }
        ret = pthread_create(&storagemanager_thr, NULL, database_thread, &thr_handle);
        if (ret != 0) {
            printf("pthread_create() error number=%d\n", ret);
            return -1;
        }
        pthread_join(connect_thr, NULL);
        pthread_join(datamanager_thr, NULL);
        pthread_join(storagemanager_thr, NULL);
        wait(&status);
    }

    unlink("./log_fifo");
    return 0;
}