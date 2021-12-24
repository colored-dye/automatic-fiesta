#include "myprot.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

const int PORT = 5994;

pthread_mutex_t g_mutex;

Client *g_clients[MAX_CLIENTS];
int g_numClients = 0;

int g_count_time = 0;

void print_packet(struct Packet *p);
void add_client(Client *client);
void delete_client(int id);
void get_all_clients(char *buffer, int maxlen);
void *thread_worker(void * arg);
char *get_time();
void *exit_thread(void *arg);

int main(void){
    int sock_fd;
    struct sockaddr_in mysock;
    pthread_t pt;
    socklen_t sin_size = sizeof(struct sockaddr_in);
    struct sockaddr_in client_addr;
    int new_fd;

    //初始化socket
    sock_fd = socket(AF_INET,SOCK_STREAM,0);

    //编辑地址信息
    memset(&mysock, 0, sizeof(mysock));
    mysock.sin_family = AF_INET;
    mysock.sin_port = htons(PORT);
    mysock.sin_addr.s_addr = INADDR_ANY;

    //绑定地址，然后监听
    if(bind(sock_fd,(struct sockaddr *)&mysock,sizeof(struct sockaddr)) < 0){
        perror("Failed to bind address");
        return 1;
    }

    if(listen(sock_fd,10) < -1){
        printf("listen error.\n");
    }

    if(pthread_mutex_init(&g_mutex, NULL)){
        perror("Failed to init mutex");
        exit(EXIT_FAILURE);
    }

    pthread_t pexit;
    pthread_create(&pexit, NULL, exit_thread, NULL);
\
    int sel = 1;
    while(1){
        printf("Waiting for connection...\n");

        new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &sin_size);
        Client *client = (Client*)malloc(sizeof(Client));
        strcpy(client->addr, inet_ntoa(client_addr.sin_addr));
        client->fd = new_fd;

        add_client(client);
        g_numClients++;

        if(pthread_create(&pt, NULL, thread_worker, (void *)client)){
            perror("Failed to create thread");
        }
    }
    close(sock_fd);
    printf("\nServer closed\n");
    return 0;
}

void *thread_worker(void *arg){
    Client *client = (Client*)arg;
    int new_fd = client->fd;

    struct Packet receive_message, send_message, relay_message;

    printf("Established: client %d\n", new_fd);

    while(1){
        int n = recv(new_fd, &receive_message, sizeof(struct Packet), 0);
        if(n < 0){
            perror("Failed to recv from client");
            close(new_fd);
            pthread_exit(NULL);
        }

        if(receive_message.type == T_REQUEST){
            switch(receive_message.option){
            case O_DISCONNECT:
                send_message.type = T_RESPONSE;
                send_message.option = O_DISCONNECT;
                strcpy(send_message.data, "Close connection");
                send_message.length = strlen(send_message.data);
                send(new_fd, &send_message, sizeof(struct Packet), 0);

                printf("\nClosed connection with client %d\n", new_fd);
                pthread_mutex_lock(&g_mutex);
                delete_client(client->id);
                g_numClients--;
                pthread_mutex_unlock(&g_mutex);
                goto thread_exit;
                break;
            // case O_EXIT:
            //     close(new_fd);
            //     goto thread_exit;
            case O_CONNECT:
                break;
            case O_GETNAME:
                gethostname(send_message.data, MAX_DATA_LENGTH);
                send_message.length = strlen(send_message.data);
                send_message.type = T_RESPONSE;
                send_message.option = O_GETNAME;
                send_message.recv_id = client->id;
                break;
            case O_GETTIME:
                strcpy(send_message.data, get_time());
                send_message.length = strlen(send_message.data);
                send_message.type = T_RESPONSE;
                send_message.option = O_GETTIME;
                send_message.recv_id = client->id;
                printf("count time: %d\n", ++g_count_time);
                break;
            case O_GETCLIENT:
                get_all_clients(send_message.data, MAX_DATA_LENGTH);
                send_message.length = strlen(send_message.data);
                send_message.type = T_RESPONSE;
                send_message.option = O_GETCLIENT;
                send_message.recv_id = client->id;
                break;
            case O_SEND:
                if(!g_clients[receive_message.recv_id]){
                    send_message.type = T_RESPONSE;
                    send_message.option = O_SEND;
                    sprintf(send_message.data, "Target client id %d does not exist", receive_message.recv_id);
                }else{
                    relay_message.type = T_INSTRUCT;
                    relay_message.option = O_SEND;
                    relay_message.length = receive_message.length;
                    memcpy(relay_message.data, receive_message.data, sizeof(receive_message.data));
                    relay_message.recv_id = receive_message.recv_id;
                    if(send(g_clients[receive_message.recv_id]->fd, &relay_message, sizeof(struct Packet), 0) < 0){
                        send_message.type = T_RESPONSE;
                        send_message.option = O_SEND;
                        sprintf(send_message.data, "Failed to send message to client %d", receive_message.recv_id);
                    }else{
                        send_message.type = T_RESPONSE;
                        send_message.option = O_SEND;
                        sprintf(send_message.data, "Message to client %d sent successfully", receive_message.recv_id);
                    }
                }
                break;
            default:
                continue;
            }
        }

        printf("Message sent:\n");
        print_packet(&send_message);
        send(new_fd, &send_message, sizeof(struct Packet), 0);
    }

thread_exit:
    close(new_fd);
    printf("Terminated: client %d\n", new_fd);

    pthread_exit(0);
}

void add_client(Client *client)
{
    int i=0;
    for(i=0; i<MAX_CLIENTS && g_clients[i]; i++)
        ;
    g_clients[i] = client;
    client->id = i;
}

void delete_client(int id)
{
    if(g_clients[id]){
        free(g_clients[id]);
        g_clients[id] = NULL;
    }
}
void get_all_clients(char *buffer, int maxlen)
{
    int len = strlen(buffer);
    char tmp[MAX_DATA_LENGTH];
    buffer[0] = 0;
    for(int i=0; i<MAX_CLIENTS; i++){
        if(g_clients[i]){
            sprintf(tmp, "client id: %d\nclient address: %s\nclient port: %d\n", 
            g_clients[i]->id, g_clients[i]->addr, g_clients[i]->fd);
            len += strlen(tmp);
            if(len > maxlen)
                break;
            strcat(buffer, tmp);
        }
    }
}

char* get_time(){
    time_t t1;
    t1 = time(&t1);
    return ctime(&t1);
}

void print_packet(struct Packet *p)
{
    char buffer[MAX_DATA_LENGTH];
    printf("Type: ");
    switch(p->type){
    case T_REQUEST: strcpy(buffer, "Request");break;
    case T_RESPONSE: strcpy(buffer, "Response");break;
    case T_INSTRUCT: strcpy(buffer, "Instruction");break;
    }
    printf("%s\nOption: ", buffer);
    switch(p->option){
    case O_CONNECT: strcpy(buffer, "Connect");break;
    case O_DISCONNECT: strcpy(buffer, "Disconnect"); break;
    case O_GETCLIENT: strcpy(buffer, "Get client");break;
    case O_GETNAME: strcpy(buffer, "Get name"); break;
    case O_GETTIME: strcpy(buffer, "Get time");break;
    case O_SEND: strcpy(buffer, "Send"); break;
    case O_EXIT: strcpy(buffer, "Exit");break;
    }
    printf("%s\nReceiver: %d\nLength: %d\nData: %s\n", buffer, p->recv_id, p->length, p->data);
}

void *exit_thread(void *arg)
{
    int sel = -1;
    while(1){
        scanf("%d", &sel);
        if(sel == 0)
            break;
    }
    exit(EXIT_SUCCESS);
}
