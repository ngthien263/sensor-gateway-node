/*
*@file          sensor_node.c
*@author        ngthien263
*@details       - Connect to a TCP server and simulate multiple sensor nodes sending data.
                - Split into two processes using fork():
                    Parent  -> simple menu.
                    Child   -> wait for user command and periodically sends data.
                - Use shared memory + an unnamed semaphore for IPC between parent and child.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/time.h>
#include "sensor_sender.h"
#include "common.h"
#include "shared_data.h"

/* Menu UI - will update in the future: dump, ... */
void print_menu() {
    printf("\n  ╔═════════════════════════════════════╗\n");
    printf("  ║           SENSOR MAIN MENU          ║\n");
    printf("  ╠═════════════════════════════════════╣\n");
    printf("  ║ 1. Add new sensor                   ║\n");
    printf("  ║ 2. Remove sensors                   ║\n");
    printf("  ║ 3. Exit                             ║\n");
    printf("  ╚═════════════════════════════════════╝\n");
}


int main(int argc, const char* argv[]){
    //Initialize
    int choice;
    int port_no;
    int server_fd;
    int sensor_id;
    int status;
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0,sizeof(serv_addr));

    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand((unsigned int)(tv.tv_sec ^ tv.tv_usec));

    if (argc < 3) {
        printf("command : ./client <server address> <port number>\n");
        exit(1);
    }
    port_no = atoi(argv[2]);
    serv_addr.sin_family =  AF_INET;
    serv_addr.sin_port   = htons(port_no);
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) == -1) 
        handle_error("inet_pton()");

     /* Create TCP socket and connect to gateway */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(connect(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        handle_error("connect()");

    /* Create shared memory for communicating between parent and child process*/
    int shm_fd = shm_open("/temp", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
        handle_error("shm_open");
    if (ftruncate(shm_fd, sizeof(shared_struct_t)) == -1)   
        handle_error("ftruncate");
    shared_struct_t* shr = mmap(0, sizeof(shared_struct_t), PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shr == MAP_FAILED)
        handle_error("mmap");

    /* Initialize unnamed semaphore in shared memory to get command from user input*/
    sem_init(&shr->sem, 1, 0);

    /*Fork*/
    pid_t child_pid = fork();
    if (child_pid < 0) {
        handle_error("fork");
    }
    else if (child_pid == 0){
        /*Child process*/
        sensor_node_t* head = NULL; //linked list head
        while(1){
            // Non-blocking check for command from parent process.
            if(!sem_trywait(&shr->sem) && shr->has_command){
                if(shr->cmd == CMD_ADD){
                    add_sensor(&head, shr->sensor_id);
                } else if(shr->cmd == CMD_REMOVE){
                    rm_sensor(&head, shr->sensor_id);
                } else {
                    //do nothing
                }
                shr->has_command = 0;
            }
            //Send data from sensor after 1 second
            sensor_send_message(&head, server_fd);
            sleep(1);
        }
        close(server_fd);
        exit(0);
    } else {
        while(1){
            /*Parent process*/
            print_menu();
            printf("Input an option: ");
            scanf("%d", &choice);
            if(choice == 1) {
                /*Adding new sensor with input id*/
                /* Write command into shared memory, then release one token to wake child. */
                printf("Input sensor ID: ");
                scanf("%d", &sensor_id);
                shr->sensor_id = sensor_id;
                shr->cmd = CMD_ADD;
                shr->has_command = 1;
                sem_post(&shr->sem);
                printf("New sensor with ID: %d created\n", sensor_id);
            } else if (choice == 2) {
                /*Remove sensor with input id*/
                /* Write command into shared memory, then release one token to wake child. */
                printf("Input sensor ID: ");
                scanf("%d", &sensor_id);
                shr->sensor_id = sensor_id;
                shr->cmd = CMD_REMOVE;
                shr->has_command = 1;
                sem_post(&shr->sem);
            } else if (choice == 3) {
                /*Exit option*/
                //Stop child process
                kill(child_pid, SIGTERM);
                wait(&status);

                //Resource clean-up
                close(server_fd);
                munmap(shr, sizeof(shared_struct_t));
                close(shm_fd);
                shm_unlink("/temp");
                sem_destroy(&shr->sem);
                return 0;
            } else {
                printf("Invalid choice\n");
            }
        }

        close(server_fd);
        munmap(shr, sizeof(shared_struct_t));
        close(shm_fd);
        shm_unlink("/temp");
        sem_destroy(&shr->sem);
        wait(&status);
    }
}