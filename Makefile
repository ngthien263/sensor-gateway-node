.PHONY := all clean
PRJ_NAME := FINAL
CUR_PATH := .
BIN_DIR := $(CUR_PATH)/bin
INC_DIR := $(CUR_PATH)/include
SRC_DIR := $(CUR_PATH)/source
OBJ_DIR := $(CUR_PATH)/obj

CFLAGS = -I$(INC_DIR) -Wall -g
CC = gcc

make_dir:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)
create_obj:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/database.c -o $(OBJ_DIR)/database.o
	$(CC) $(CFLAGS) -c $(SRC_DIR)/socket.c -o $(OBJ_DIR)/socket.o
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sensor_reader.c -o $(OBJ_DIR)/sensor_reader.o
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sensor_sender.c -o $(OBJ_DIR)/sensor_sender.o
	
	$(CC) $(CFLAGS) -c $(CUR_PATH)/sensor_gateway.c -o $(OBJ_DIR)/sensor_gateway.o 
	$(CC) $(CFLAGS) -c $(SRC_DIR)/thread_handle.c -o $(OBJ_DIR)/thread_handle.o
	$(CC) $(CFLAGS) -c $(CUR_PATH)/sensor_node.c -o $(OBJ_DIR)/sensor_node.o 

all: make_dir create_obj 
	$(CC) -g $(CFLAGS) $(OBJ_DIR)/socket.o $(OBJ_DIR)/sensor_reader.o $(OBJ_DIR)/sensor_gateway.o $(OBJ_DIR)/thread_handle.o $(OBJ_DIR)/database.o -o $(BIN_DIR)/sensor_gateway -lpthread -lsqlite3
	$(CC) -g $(CFLAGS) $(OBJ_DIR)/socket.o $(OBJ_DIR)/sensor_sender.o $(OBJ_DIR)/sensor_node.o -o $(BIN_DIR)/sensor_node -lpthread -lsqlite3

clean:
	rm -rf $(BIN_DIR)/*
	rm -rf $(CUR_PATH)/*.o
	rm -rf $(OBJ_DIR)/*.o

