#ifndef MYPROT_H
#define MYPROT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <semaphore.h>

#define MAX_DATA_LENGTH (1024)
#define MAX_CLIENTS (512)

typedef unsigned char byte;

typedef enum {
    T_REQUEST, T_RESPONSE, T_INSTRUCT 
} TYPE;

typedef enum {
    O_EXIT, O_CONNECT, O_DISCONNECT,
    O_GETTIME, O_GETNAME, O_GETCLIENT,
    O_SEND
} OPTION;

struct Packet {
    byte type; // 请求,响应,指示
    byte option; // 功能: 建立连接,断开,获取时间,获取名字,客户端列表,发送消息,
    unsigned int recv_id; // 接受消息的客户端号
    unsigned int length; // 数据部分的长度
    byte data[MAX_DATA_LENGTH];
};

typedef struct ClientInfo {
    int id;
    int fd;
    char addr[16];
} Client;

#endif