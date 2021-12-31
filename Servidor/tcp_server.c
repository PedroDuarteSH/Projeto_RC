#include "server.h"
#include <errno.h>
int tcp_fd;

// Adriana Gomes da Silva Leocádio Bernardo         2019218086
// Pedro Duarte Santos Henriques                    2019217793
void tcp_server(){
    tcp_server_pid = getpid();
    signal(SIGTSTP, update_list);
    signal(SIGINT, cleanup_server);

    int client_fd;
    struct sockaddr_in tcp_addr, tcp_client_addr;
    int client_addr_size = sizeof(tcp_client_addr);

    bzero((void *) &tcp_addr, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    inet_aton(SERVER_IP, &tcp_addr.sin_addr);
    tcp_addr.sin_port = htons(config_port);

#ifdef DEBUG
    client * temp_c = list.first_client;
    while (temp_c != NULL){
        printf("TCP - USER:\n");
        printf("TCP - NAME: %s\n", temp_c->user_id);
        printf("TCP - ADDRESS: %s\n", temp_c->ip_address);
        printf("TCP - password: %s\n", temp_c->password);
        printf("TCP - client_server: %d\n", temp_c->perms.client_server);
        printf("TCP - P2P: %d\n", temp_c->perms.P2P);
        printf("TCP - group: %d\n\n", temp_c->perms.group);
        temp_c = temp_c->next;
    }
#endif

    if ((tcp_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Erro na funcao socket-TCP\n");
        exit(-1);
    }
        
    if (bind(tcp_fd,(struct sockaddr*)&tcp_addr,sizeof(tcp_addr)) < 0){
        printf("Erro na funcão bind - TCP => %s\n", strerror(errno));
        exit(-1);
    }
        
    if(listen(tcp_fd, 5) < 0){
        printf("Erro na funcao listen-TCP\n");
        exit(-1);
    }

    char ipv4_addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &tcp_addr.sin_addr, ipv4_addr, INET_ADDRSTRLEN);
    printf("Server hosted (IP:port) %s:%d\n", SERVER_IP, config_port);

    //Wait for connections
    while (TRUE) {
        //clean finished child processes, avoiding zombies
        //must use WNOHANG or would block whenever a child process was working
        while(waitpid(-1,NULL,WNOHANG)>0);
        
        //wait for new connection
        client_fd = accept(tcp_fd,(struct sockaddr *)&tcp_client_addr,(socklen_t *)&client_addr_size);
        if (client_fd > 0) {
            //get client ip
            if (fork() == 0) {
                close(tcp_fd);
                process_client(client_fd, tcp_client_addr);
                exit(0);
            }
            close(client_fd);
        }
    }
    exit(0);
}


void process_client(int client_fd, struct sockaddr_in client_addr){
    //Replace global fd with client file descriptor, so can close with SIGINT
    tcp_fd = client_fd;
    //Client server handling
    //signal(SIGINT, cleanup_client);
    
    //Get and print client ip
    char ipv4_addr[INET_ADDRSTRLEN];
    printf("** New client connected in Config Mode **\n");
    inet_ntop(AF_INET, &client_addr.sin_addr, ipv4_addr, INET_ADDRSTRLEN);
    printf ("Client connecting from (IP:port) %s:%d\n", ipv4_addr, client_addr.sin_port);
    fflush(stdout);

    int nread = 0;
    char *buffer = malloc(sizeof(char) * BUFLEN);
    while(1){
        nread = read(client_fd, buffer, BUFLEN-1);
        buffer[nread-1] = '\0';//Remove /n from buffer string
        char *command = strtok_r(buffer, " ", &buffer);
        if(strcmp(command, "LIST") == 0){ //List users
            send_client_list();
        }
        else if(strcmp(command, "ADD") == 0){ //Add a new user
            process_user(buffer, 0);
        }
        else if(strcmp(command, "DEL") == 0){ //Delete a user
            delete_user(buffer);
        }
        else if(strcmp(command, "QUIT") == 0){ //Leave client session
            write(tcp_fd, buffer, strlen(buffer) +1);
            break;
        }
        else{
            buffer = "Client Command does not exists\n";
            write(tcp_fd, buffer, strlen(buffer) +1);
        }
        if(strcmp(command, "ADD") == 0 || strcmp(command, "DEL") == 0){ //Add a new user
            update_file();
        }
    }
    printf("Admin: leaving\n");
    close(client_fd);
}

void send_client_list(){
    char *buffer = malloc(sizeof(char) * BUFLEN);
    char *temp = malloc(sizeof(char) * BUFLEN);
    bzero(buffer, BUFLEN);
    bzero(temp, BUFLEN);
    client * temp_c = list.first_client;
    for (int i = 0; i < list.n_clients; i++){
        snprintf(buffer, BUFLEN, "\t-> %s - %s - %s - %d - %d - %d\n", temp_c->user_id, temp_c->ip_address, temp_c->password, temp_c->perms.client_server, temp_c->perms.P2P, temp_c->perms.group);
        strcat(temp, buffer);
        temp_c = temp_c->next;
    }
    write(tcp_fd, temp, strlen(temp) + 1);
}

int delete_user(char *buffer){
    char *user_id = strtok_r(buffer, " \n", &buffer);
    client * temp_c = list.first_client, *temporary = NULL;
    for (int i = 0; i < list.n_clients; i++){
        if(strcmp(temp_c->user_id, user_id) == 0){
            if(temporary == NULL){
                list.first_client = temp_c->next;
            }
            else temporary->next = temp_c->next;
            list.n_clients--;
            free(temp_c);
            return TRUE;
        }
        temporary = temp_c;
        temp_c = temp_c->next;
    }
    return FALSE;
}


void update_file(){
    signal(SIGINT, SIG_IGN);
    FILE *write = fopen(file_name, "w");
    char *output = malloc(sizeof(char) * BUFLEN);
    client *temp_c = list.first_client;
    while (temp_c != NULL){
        snprintf(output, BUFLEN, "%s %s %s ", temp_c->user_id, temp_c->ip_address, temp_c->password);
        if(temp_c->perms.client_server == 1)
            strcat(output, "yes ");
        else  strcat(output, "no ");
        
        if(temp_c->perms.P2P == 1)
            strcat(output, "yes ");
        else  strcat(output, "no ");
        
        if(temp_c->perms.group == 1)
            strcat(output, "yes\n");
        else strcat(output, "no\n");
        fputs(output, write);
        temp_c = temp_c->next;
    }
    fflush(write);
    kill(tcp_server_pid, SIGTSTP);
    kill(udp_server_pid, SIGTSTP);
    signal(SIGINT, cleanup_client);
}



void cleanup_server(int signo){ // clean up resources by pressing Ctrl-C
    //clean finished child processes, avoiding zombies
    //must use WNOHANG or would block whenever a child process was working
    while(waitpid(-1,NULL,WNOHANG)>0);
    //Close server socket
    shutdown(tcp_fd, SHUT_RDWR);
    close(tcp_fd);
	printf("\nClosing server...\n");
  
	exit(0);
}


void cleanup_client(int signo){ // clean up resources by pressing Ctrl-C{
    //clean finished child processes, avoiding zombies
    //must use WNOHANG or would block whenever a child process was working
    while(waitpid(-1,NULL,WNOHANG)>0);
    //Close client socket
    shutdown(tcp_fd, SHUT_RDWR);
    close(tcp_fd);
	exit(0);
}

