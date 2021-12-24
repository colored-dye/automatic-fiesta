#include "myprot.h"
#include <bits/pthreadtypes.h>
#include <stdlib.h>

const int PORT = 5994;

pthread_t g_accept_thread, g_send_thread;
int g_connected;
int g_received;
struct Packet g_packet;
sem_t g_sem;
pthread_mutex_t g_mutex;
void *thread_result;
struct sockaddr_in server_addr;

inline void prompt();
inline void help();
void *accept_thread(void *arg);
void *send_thread(void *arg);
void *thread_worker(void * arg);

void c_connect(int *sockfd);
void c_disconnect(int sock_fd);
void c_getname();
void c_gettime();
void c_getclients();
void c_send();

int main(int argc, char *argv[])
{
    if(argc != 2){
        printf("Format: ./client <IP>\n");
        return 1;
    }
    int sock_fd;

    bzero(&server_addr, sizeof(server_addr));//编辑服务端地址信息
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    sem_init(&g_sem, 0, 0);

    if(inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0){
        perror("convert_servaddr error");
        exit(EXIT_FAILURE);
    }

    char sel = 0;

    pthread_mutex_init(&g_mutex, NULL);

    while(1){
        while(1){
            prompt();
            sel = getchar();
            if((sel >= '0' && sel <= '6') || sel == 'h'){
                getchar();
                break;
            }
        }
        switch (sel) {
        case '0': // exit
            if(g_connected){
                c_disconnect(sock_fd);
            }
            sem_destroy(&g_sem);
            goto main_exit;
            break;
        case '1': // connect
            if(!g_connected){
                c_connect(&sock_fd);
            }else{
                printf("\nAlready connected!\n");
            }
            break;
        case '2': // disconnect
            if(!g_connected){
                printf("\nNot connected!\n");
            }else{
                c_disconnect(sock_fd);
            }
            break;
        case '3': // get time
            if(!g_connected)
                continue;
            c_gettime();
            break;
        case '4': // get name
            if(!g_connected)
                continue;
            c_getname();
            break;
        case '5': // get all clients
            if(!g_connected)
                continue;
            c_getclients();
            break;
        case '6': // send message
            if(!g_connected)
                continue;
            c_send();
            break;
        case 'h': // help
            help();
            break;
        default:
            continue;
        }
    }

main_exit:
    printf("\nBye bye~\n");

    return 0;
}


void c_disconnect(int sock_fd)
{
    pthread_mutex_lock(&g_mutex);
    g_packet.type = T_REQUEST;
    g_packet.option = O_DISCONNECT;
    g_packet.data[0] = 0;
    g_packet.length = 0;
    pthread_mutex_unlock(&g_mutex);
    sem_post(&g_sem);
    printf("Waiting for server response...\n");
    if(pthread_join(g_accept_thread, &thread_result)){
        perror("Accept thread join failed");
        exit(EXIT_FAILURE);
    }
    if(pthread_join(g_send_thread, &thread_result)){
        perror("Send thread join failed");
        exit(EXIT_FAILURE);
    }

    close(sock_fd);
    pthread_mutex_lock(&g_mutex);
    g_connected = 0;
    g_received = 0;
    pthread_mutex_unlock(&g_mutex);
    printf("Connection closed\n");
}
void c_connect(int *psock_fd)
{
    if((*psock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Failed to init socket");
        exit(EXIT_FAILURE);
    }
    if(connect(*psock_fd, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr)) < 0){
        perror("Failed to connect to server");
    }
    pthread_create(&g_accept_thread, NULL, accept_thread, (void*)psock_fd);
    pthread_create(&g_send_thread, NULL, send_thread, (void*)psock_fd);

    pthread_mutex_lock(&g_mutex);
    g_connected = 1;
    printf("Connection established\n");
    pthread_mutex_unlock(&g_mutex);
}
void c_getname()
{
    pthread_mutex_lock(&g_mutex);
    g_packet.length = 0;
    g_packet.type = T_REQUEST;
    g_packet.option = O_GETNAME;
    g_packet.data[0] = 0;
    g_received = 0;
    pthread_mutex_unlock(&g_mutex);

    sem_post(&g_sem);
    while(!g_received)
        ;
    pthread_mutex_lock(&g_mutex);
    g_received = 0;
    printf("\nServer name: %s\n", g_packet.data);
    pthread_mutex_unlock(&g_mutex);
}
void c_gettime()
{
    for(int i=0; i<100; i++)
{
    pthread_mutex_lock(&g_mutex);
    g_packet.length = 0;
    g_packet.type = T_REQUEST;
    g_packet.option = O_GETTIME;
    g_packet.data[0] = 0;
    g_received = 0;
    pthread_mutex_unlock(&g_mutex);

    sem_post(&g_sem);
    while(!g_received)
        ;
    pthread_mutex_lock(&g_mutex);
    g_received = 0;
    printf("\nTime: %s\n", g_packet.data);
    pthread_mutex_unlock(&g_mutex);
}
}
void c_getclients()
{
    pthread_mutex_lock(&g_mutex);
    g_packet.length = 0;
    g_packet.type = T_REQUEST;
    g_packet.option = O_GETCLIENT;
    g_packet.data[0] = 0;
    g_received = 0;
    pthread_mutex_unlock(&g_mutex);

    sem_post(&g_sem);
    while(!g_received)
        ;
    pthread_mutex_lock(&g_mutex);
    g_received = 0;
    printf("\nList of all clients:\n%s\n", g_packet.data);
    pthread_mutex_unlock(&g_mutex);
}
void c_send()
{
    int target_id;
    printf("Please enter target client id:\n");
    scanf("%d", &target_id);
    getchar();
    printf("Please enter message:\n");
    pthread_mutex_lock(&g_mutex);
    fgets(g_packet.data, MAX_DATA_LENGTH, stdin);
    g_packet.data[strlen(g_packet.data)-1] = '\0';
    g_packet.type = T_REQUEST;
    g_packet.option = O_SEND;
    g_packet.recv_id = target_id;
    g_packet.length = strlen(g_packet.data);
    pthread_mutex_unlock(&g_mutex);
    g_received = 0;
    sem_post(&g_sem);
    while(!g_received)
        ;
    pthread_mutex_lock(&g_mutex);
    g_received = 0;
    printf("%s", g_packet.data);
    pthread_mutex_unlock(&g_mutex);
}
void prompt()
{
    printf("\nclient > ");
}
void help()
{
    puts("\nHelp:\n0: exit\n1: connect\n2: disconnect\n3: get time\n4: get name\n5: get all clients\n6: send message\nh: help");
}

void *accept_thread(void *arg)
{
    int fd = *(int*)arg;
    struct Packet p;
    while(1){
        int n = recv(fd, &p, sizeof(struct Packet), 0);
        if(n != sizeof(struct Packet)){
            exit(EXIT_FAILURE);
        }
        pthread_mutex_lock(&g_mutex);
        g_received = 1;
        memcpy(&g_packet, &p, sizeof(struct Packet));
        pthread_mutex_unlock(&g_mutex);

        if(p.type == T_INSTRUCT){
            if(p.option == O_SEND){
                printf("\nMessage received:\n%s\n", p.data);
                prompt();
            }
        }

        if(p.option == O_DISCONNECT){
            p.length = 0;
            p.type = T_RESPONSE;
            send(fd, &p, sizeof(struct Packet), 0);
            break;
        }
    }
    printf("Killed accept thread\n");
    pthread_exit(NULL);
}

void *send_thread(void *arg)
{
    int fd = *(int*)arg;
    
    while(1){
        sem_wait(&g_sem);
        if(send(fd, &g_packet, sizeof(struct Packet), 0) != sizeof(struct Packet)){
            perror("Failed to send packet");
        }
        if(g_packet.type == T_REQUEST && g_packet.option == O_DISCONNECT)
            break;
    }
    printf("\nKilled send thread\n");
    pthread_exit(NULL);
}
