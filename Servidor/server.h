// Adriana Gomes da Silva Leocádio Bernardo         2019218086
// Pedro Duarte Santos Henriques                    2019217793

#ifndef server   /* Include guard */
#define server


 /* Udp Message types:
  * 
  * Recieve:
  *  - LOGIN:username:password   //To login a user
  *  - DIRECT:dest_username:Message
  *  - P2P:dest_username
  *  - CREATEGROUP:group_name
  *  
  * Send
  *  - Accepted      //To send if login compleeted with success
  *  - Rejected      //To send if login failed
  *  
  *  - RESCSC:message
  *  - RESP2P:dest_ip:dest_port  //To send the Ip Address to send the P2P message
  *  - RESGROUP:multicast_ip:multicast_port //To send the multicast Ip Address to send the Group message (in case of join/create request)
  *  - MESSAGE:source_userID:string //To send a direct message
  * 
  */


#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#define BUFLEN	512
#define MAX_LINE 200
#define N_PARAM 6

#define TRUE 1
#define FALSE 0

#define SERVER_IP "10.90.0.2"
//#define SERVER_IP "127.0.0.1"

#define MAX_GROUP_ELEM 10

//client permisions
typedef struct{
  int client_server;
  int P2P;
  int group; 
}permissions;

//Client list
typedef struct client{
  char *user_id;
  char *password;
  permissions perms;
  int logged_in;
  char *ip_address;
  uint16_t logged_in_port;
  struct client *next;
}client;

typedef struct group_ips{
  int first;
  int second;
  int third;
  int fourth;
}group_ips;


typedef struct group{
  char *name;
  char *ip_address;
  uint16_t port;
  struct group *next;
}group;

group *firstgroup;

typedef struct client_list{
  client *first_client;
  client *new_list;
  int n_clients;
}client_list;

client_list list;

void read_registry_file(int new);
void process_user(char *line, int new);
void add_user(char *user_id, char *address, char *password, int client_server, int P2P, int group, int new);
char * concat(char *s1, char *s2);
void process_client(int client_fd, struct sockaddr_in client_addr);
void erro(char *msg);
void update_list();
// TCP SERVER
void tcp_server();
int delete_user(char *buffer);
void send_client_list();
void cleanup_client(int signo);
void cleanup_server(int signo);
void update_file();

//UDP SERVER
void udp_server();
client* validate_user(char *string, struct sockaddr_in udp_client_addr);
void udp_server();
void user_login(char *data);
void send_c2c_message(char *string);
client *search_user_username(char *string);
client *search_user_ip(char *string, uint16_t port);
group *add_group(char* group_name, char* ip, client* creator);
char *generate_ip();
void create_multilink(char *string);
group* search_group(char *name);
void close_server();
void send_online_users();
void process_P2P(char *string);
void join_multilink(char *string);
void client_leave();

int customer_port;
int config_port;
char *file_name;
struct in_addr server_ip;
pid_t udp_server_pid, tcp_server_pid;

#endif