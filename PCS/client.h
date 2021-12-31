#ifndef client   /* Include guard */
#define client

// Adriana Gomes da Silva Leocádio Bernardo         2019218086
// Pedro Duarte Santos Henriques                    2019217793

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/wait.h>

#define DEBUG

#define MAX_USERNAME_SIZE 32
#define MIN_USERNAME_SIZE 5
#define TRUE 1
#define FALSE 0

#define BUFLEN 512



//permissions struct
typedef struct{
  int client_server;
  int P2P;
  int group; 
}permissions;

typedef struct group{
    char *name;
    struct sockaddr_in send_addr;
    struct sockaddr_in recv_addr;
    int socket;
    struct group *next;
}group;

group *firstgroup;
//Funtions
void users();


//Login
int user_login();

//Recieve message
void process_message(char *input);
char* receive_sv_response();

//Send message
char *choose_dest();
void server_message();
void direct_message();


//Group Functions
void group_menu();
void create_group();
group *choose_group();
void list_my_groups();
void send_message_g();
void join_group();
void leave_group();
void remove_group(group *leaving);
group *add_group_s(char *name, char *ip, char *port);
group *search_group(char *g_name);


//Password
char* getpassword(int max_size);

//Close
void close_client(char *msg);
void sigint(int signum);

//Global variables
int udp_fd, group_fd;
struct sockaddr_in server, incoming_con, dest_connection;
socklen_t slen;
permissions *client_perms;
char *username;

#endif