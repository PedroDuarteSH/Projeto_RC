#include "client.h"

// Adriana Gomes da Silva Leocádio Bernardo         2019218086
// Pedro Duarte Santos Henriques                    2019217793
int main(int argc, char *argv[]) {
    firstgroup = NULL;
    udp_fd = -1;
    slen = sizeof(incoming_con);
    struct hostent *hostPtr;
    signal(SIGINT, sigint);
    if (argc != 3) {
        printf("cliente <server-IP> <port>\n");
        exit(-1);
    }

    //Get address of server
    char endServer[100];
    strcpy(endServer, argv[1]);
    if ((hostPtr = gethostbyname(endServer)) == 0){
        printf("Couldn't convert address");
        exit(0);
    }

    //Connection configuration
    bzero((void *) &server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    server.sin_port = htons((short) atoi(argv[2]));

    //Socket connection
    if ((udp_fd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) == -1){
        printf("Error in socket\n");
        exit(0);
    }

    if ((group_fd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) == -1){
        printf("Error in socket\n");
        exit(0);
    }

    int multicastTTL = 254;
    if (setsockopt(group_fd, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicastTTL,sizeof(multicastTTL)) < 0) {
        perror("socket opt");
        return 1;
    }

    struct in_addr sin_addr;
    sin_addr.s_addr = htonl(INADDR_ANY);
    if (setsockopt(group_fd, IPPROTO_IP, IP_MULTICAST_IF, (char*) &sin_addr, sizeof(sin_addr)) < 0){
        perror("setsockopt");
        return 1;
    }


    //Initiate Cliente permissions
    client_perms = malloc(sizeof(permissions));
    client_perms->client_server = FALSE;
    client_perms->P2P = FALSE;
    client_perms->group = FALSE;

    printf("***Program starting***\n");
    int choice;
    char *input = malloc(sizeof(char) * BUFLEN);
    char *temp;
    
    while(TRUE){
        printf("\nChoose an option:\n\t1-> Login\n\t0-> Exit\nChoice: ");
        fgets(input, BUFLEN, stdin);
        choice = strtol(input, &temp, 10);
        if(choice == 1){
            if(user_login() == TRUE) break;
            else printf("=> Invalid Credentials\n");
        }
        else if(choice == 0 && temp != input){
            close_client("=> Client Asked to leave!\n");
        }
        else
            printf("=>Invalid Choice!\n\tPlease Try Again!\n");
    }
    

    int perms_index[3];
    for (int i = 0; i < 3; i++) perms_index[i] = -1;
    
    //Verify User permissions to enable functions to be used
    int index = 2;
    char *print = malloc(sizeof(char) * BUFLEN);
    snprintf(print, BUFLEN, "\nChoose an option:\n\t1 -> List Online Users\n");
    if(client_perms->client_server == TRUE){
        snprintf(input, BUFLEN, "\t%d -> Send message to other user through server\n", index);
        perms_index[0] = index++;
        strcat(print,input);
    }

    if(client_perms->P2P == TRUE){
        snprintf(input, BUFLEN, "\t%d -> Send message to other user directly\n", index);
        perms_index[1] = index++;
        strcat(print,input);
    }
    if(client_perms->group == TRUE){
        snprintf(input, BUFLEN, "\t%d -> Group Menu\n", index);
        perms_index[2] = index++;
        strcat(print,input);
    } 
    strcat(print,"\t0 -> Exit\nChoice: ");

    choice = -1;
   
    fd_set read_set;
    FD_ZERO(&read_set);
    int max_fd = udp_fd;
    while(TRUE){
        //Set UDP and Multicast Groups sockets
        FD_SET(0, &read_set);
        FD_SET(udp_fd, &read_set);
        group *temporary = firstgroup;
        while(temporary != NULL){
            FD_SET(temporary->socket, &read_set);
            if(temporary->socket > max_fd)
                max_fd = temporary->socket;
            temporary = temporary->next;
        }
        printf("%s", print);
        fflush(stdout);
        if(select(max_fd+1, &read_set, NULL, NULL, NULL) > 0 ) {
            if(FD_ISSET(udp_fd, &read_set)){
                while(recvfrom(udp_fd,  input, BUFLEN, MSG_DONTWAIT, (struct sockaddr *) &incoming_con, (socklen_t *)&slen) > 0){
                    printf("\n\n***Recieved message***\n");
                    process_message(input);
                }
            }
            if(FD_ISSET(0, &read_set)){
                fgets(input, BUFLEN, stdin);
                choice = strtol(input, &temp, 10);
                if(choice == 1)
                    users();

                else if(choice == perms_index[0] && client_perms->client_server == TRUE)
                    // Send message through server
                    server_message();
                
                else if(choice == perms_index[1] && client_perms->P2P == TRUE)
                    // Send direct message
                    direct_message();
                
                else if(choice == perms_index[2] && client_perms->group == TRUE)
                    // Send message to group
                    group_menu();
                
                else if(choice == 0 && temp != input) close_client("Client Asked to leave!");
                else printf("=>Invalid Choice!\n\tPlease Try Again!\n");
            }
            temporary = firstgroup;
            while(temporary != NULL){
                if(FD_ISSET(temporary->socket, &read_set)){
                    while(recvfrom(temporary->socket,  input, BUFLEN, MSG_DONTWAIT, (struct sockaddr *) &incoming_con, (socklen_t *)&slen) > 0){
                        printf("\n\n***Recieved message***\n");
                        process_message(input);
                    }
                }temporary = temporary->next;
            }
        }
    }

    close(udp_fd);
    exit(0);
}

void sigint(int signum){
    close_client("Client Leaved with success\n");
    
}


void close_client(char *msg){
    printf("%s\n", msg);
    if(sendto(udp_fd, "LEAVE", strlen("LEAVE") + 1, 0, (struct sockaddr *) &server, slen) == -1)
        printf("Error in sendto\n");

    close(udp_fd);
    exit(0);
}


//List  users
void users(){
    char *input = malloc(sizeof(char) * BUFLEN);
    bzero(input, BUFLEN);
    snprintf(input, BUFLEN , "USERS");
    if(sendto(udp_fd, input, strlen(input) + 1, 0, (struct sockaddr *) &server, slen) == -1)
        printf("Error in sendto\n");
    input = receive_sv_response();

    printf("\n---Users---\n");
    printf("%s", input);
}

//User login function
int user_login(){
    char *buf = malloc(sizeof(char) * BUFLEN);
    bzero(buf, BUFLEN);
    username = malloc(sizeof(char) * BUFLEN);
    bzero(username, BUFLEN);
    char *password = malloc(sizeof(char) * BUFLEN);
    bzero(password, BUFLEN);

    //Get Username
    printf("\nYour username and password must bigger than %d and smaller then %d", MIN_USERNAME_SIZE, MAX_USERNAME_SIZE);
    printf("\nYour password must be composed by numbers or letters without special characters(!, á, ?, /,...)\nInsert your login credentials:\nUsername: ");
    fgets(username, BUFLEN, stdin);
    username[strlen(username)-1] = '\0';
    
    //Get Password
    printf("Password: ");
    password = getpassword(BUFLEN);
    printf("\n");
    if(strlen(username) < MIN_USERNAME_SIZE || strlen(username) > MAX_USERNAME_SIZE || strlen(password) < MIN_USERNAME_SIZE || strlen(password) > MAX_USERNAME_SIZE){
        return FALSE;
    }
    //Send Login message to server
    snprintf(buf, BUFLEN , "LOGIN:%s:%s", username, password);
    if(sendto(udp_fd, buf, strlen(buf) + 1, 0, (struct sockaddr *) &server, slen) == -1)
        printf("Error in sendto\n");
    free(password);

    //Wait for response from server/Process other messages
    buf = receive_sv_response();



    char *save_ptr;
    char *token;
    token = strtok_r(buf, ":", &save_ptr);
    if(strcmp(token, "Accepted") == 0){
        token = strtok_r(save_ptr, ":\n", &save_ptr);
        if(strcmp(token, "1") == 0)
            client_perms->client_server = TRUE;
        
        token = strtok_r(save_ptr, ":", &save_ptr);
        if(strcmp(token, "1") == 0)
            client_perms->P2P = TRUE;
        
        token = strtok_r(save_ptr, ":", &save_ptr);
        if(strcmp(token, "1") == 0)
            client_perms->group = TRUE;
        free(buf);
        return TRUE;
    }
    free(buf);
    free(username);
    return FALSE;
}

void process_message(char *input){
    char *save_ptr = NULL;
    char *command = strtok_r(input, ":", &save_ptr);
    char *username = strtok_r(save_ptr, ":", &save_ptr);
    char *message = strtok_r(save_ptr, ":", &save_ptr);
    if(command == NULL || username == NULL || message == NULL){
        printf("Error recieving message\n");
        return;
    }
    printf("Message Recived from User/Group: %s\n\tMessage: %s", username, message);
}

char *choose_dest(){
    char *username = malloc(sizeof(char) * MAX_USERNAME_SIZE);
    printf("\nWho do you want to send the message:\nUsername: ");
    fgets(username, MAX_USERNAME_SIZE, stdin);
    username[strlen(username)-1] = '\0';
    return username;
}

//Send a Message through server
void server_message(){
    char *input = malloc(sizeof(char) * BUFLEN);
    bzero(input, BUFLEN);

    strcat(input, "ServerMSG:");
    strcat(input,choose_dest());
    strcat(input,":");
    char message[BUFLEN-strlen(input)];
    printf("Write your message: ");
    fgets(message, BUFLEN-strlen(input), stdin);
    strcat(input,message);


    if(sendto(udp_fd, input, strlen(input) + 1, 0, (struct sockaddr *) &server, slen) == -1)
        printf("Error in sendto\n");
    input = receive_sv_response();

#ifdef DEBUG
    printf("%s\n", input);
#endif
}


//Send a P2P through server
void direct_message(){
    char *input = malloc(sizeof(char) * BUFLEN);
    bzero(input, BUFLEN);

    strcat(input, "P2PMSG:");
    strcat(input,choose_dest());
    if(sendto(udp_fd, input, strlen(input) + 1, 0, (struct sockaddr *) &server, slen) == -1)
        printf("Error in sendto\n");

    input = receive_sv_response();
    

    char *save_ptr;
    char *command = strtok_r(input, ":", &save_ptr);
    char *dest_ip = strtok_r(save_ptr, ":", &save_ptr);
    char *dest_port = strtok_r(save_ptr, ":", &save_ptr);
    //TO DO
    //VERIFICAR COMANDO
    if(strcmp(command, "RESP2P") != 0){
        printf("Message Recived from User/Group: %s\n\tMessage: %s", dest_ip, dest_port);
        return;
    }
    
    //Connection configuration
    bzero((void *) &dest_connection, sizeof(dest_connection));
    dest_connection.sin_family = AF_INET;
    inet_aton(dest_ip, &dest_connection.sin_addr);
    dest_connection.sin_port = atoi(dest_port);

    char message[BUFLEN-strlen(input)];
    printf("Write your message: ");
    fgets(message, BUFLEN-strlen(input), stdin);
    
    input[0] = '\0';
    strcat(input, "MESSAGE:");
    strcat(input, username);
    strcat(input, ":");
    strcat(input, message);

    if(sendto(udp_fd, input, strlen(input) + 1, 0, (struct sockaddr *) &dest_connection, slen) == -1)
        printf("Erro no sendto");
    free(input);
}

char* receive_sv_response(){
    int recv_len;
    char *input = malloc(sizeof(char) * BUFLEN);
    bzero(input, BUFLEN);
    char *copy = malloc(sizeof(char) * BUFLEN);
    bzero(copy, BUFLEN);
    char *token;
    char *save_ptr;

    while(1){
        if((recv_len = recvfrom(udp_fd, input, BUFLEN, 0, (struct sockaddr *) &incoming_con, (socklen_t *)&slen)) == -1){
            printf("Erro no recvfrom");
        }

        strcpy(copy, input);
        token = strtok_r(copy, ":", &save_ptr);
        
        if(strcmp(token, "MESSAGE") == 0){
            process_message(input);
        }
        
        else{
            free(copy);
            return input;
        }
    }    
    
}

// Group Message
// Group Menu
void group_menu(){
    int choice;
    char *input = malloc(sizeof(char) * BUFLEN);
    char *temp;
    while(TRUE){
        printf("--- Group Menu ---\n");
        printf("\t1 -> List my Groups\n");
        printf("\t2 -> Send message to Group\n");
        printf("\t3 -> Create a new Group\n");
        printf("\t4 -> Join an existent Group\n");
        printf("\t5 -> Leave a Group\n");
        printf("\t0 -> Leave menu\nChoice: ");
        fgets(input, BUFLEN, stdin);
        choice = strtol(input, &temp, 10);

        if(choice == 1){
            list_my_groups();
        }
        else if(choice == 2){
            send_message_g();
        }
        else if(choice == 3)
            create_group();
        
        else if(choice == 4)
            join_group();
        
        else if(choice == 5)
            leave_group();
        
        else if(choice == 0 && temp != input){
            printf("Leaved Group Menu!");
            break;
        } 
        else printf("=>Invalid Choice!\n\tPlease Try Again!\n");
    }
    free(input);
    return;
}

void create_group(){
    char *name = malloc(sizeof(char) * BUFLEN);
    printf("Insert new group name:");
    fgets(name, BUFLEN, stdin);
    name[strlen(name) - 1] = '\0';
    
    char *input = malloc(sizeof(char) * BUFLEN);
    bzero(input, BUFLEN);

    strcat(input, "CREATEGROUP:");
    strcat(input, name); 

    if(sendto(udp_fd, input, strlen(input) + 1, 0, (struct sockaddr *) &server, slen) == -1)
        printf("Error in sendto\n");
    input = receive_sv_response();
    
    char *save_ptr;
    char *command = strtok_r(input, ":", &save_ptr);
    char *ip = strtok_r(save_ptr, ":", &save_ptr);
    char *port = strtok_r(save_ptr, ":", &save_ptr);
    if(strcmp(command, "RESGROUP") == 0){
        group *adding = add_group_s(name, ip, port);
        printf("Group: %s Created!", adding->name);
    }else printf("\nMessage Recived from User/Group: %s\n\tMessage: %s\n", ip, port);
}

group *add_group_s(char *name, char *ip, char *port){
    group *adding = malloc(sizeof(group));
    adding->name = name;
    int port_num = (atoi(port));
   
    //Set sending address
    bzero((void *) &adding->send_addr, sizeof(adding->send_addr));
    adding->send_addr.sin_family = AF_INET;
    inet_aton(ip, &adding->send_addr.sin_addr);
    adding->send_addr.sin_port = htons(port_num);
    
    //Set reciever address
    bzero(&adding->recv_addr, sizeof(adding->recv_addr));
    adding->recv_addr.sin_family = AF_INET;
    adding->recv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // differs from sender
    adding->recv_addr.sin_port = htons(port_num);
    
    //Socket connection
    if ((adding->socket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) == -1){
        printf("Error in socket\n");
        exit(0);
    }

    u_int yes = 1;
    if (setsockopt(adding->socket, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes)) < 0){
       perror("Reusing ADDR failed");
       return NULL;
    }
    
    if (bind(adding->socket, (struct sockaddr*) &adding->recv_addr, sizeof(adding->recv_addr)) < 0) {
        perror("bind");
        return NULL;
    }
    
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(adding->socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0){
        perror("setsockopt");
        return NULL;
    }
    

    adding->next = NULL;
    if(firstgroup == NULL)
        firstgroup = adding;
    else{
        group *temp = firstgroup;
        while (temp->next != NULL)
            temp = temp->next;
        temp->next = adding;
    }
    
    return adding;
}

void join_group(){
    char *name = malloc(sizeof(char) * BUFLEN);
    printf("Insert group name:");
    fgets(name, BUFLEN, stdin);
    name[strlen(name) - 1] = '\0';
    char *input = malloc(sizeof(char) * BUFLEN);
    bzero(input, BUFLEN);

    strcat(input, "JOINGROUP:");
    strcat(input, name); 

    if(sendto(udp_fd, input, strlen(input) + 1, 0, (struct sockaddr *) &server, slen) == -1)
        printf("Error in sendto\n");
    input = receive_sv_response();
    
    char *save_ptr;
    char *command = strtok_r(input, ":", &save_ptr);
    char *ip = strtok_r(save_ptr, ":", &save_ptr);
    char *port = strtok_r(save_ptr, ":", &save_ptr);

    if(strcmp(command, "RESGROUP") == 0){
        group *adding = add_group_s(name, ip, port);
        printf("Group: %s Joined!", adding->name);
    }else printf("Message Recived from User/Group: %s\n\tMessage: %s", ip, port);
}

void send_message_g(){
    group *to_send = choose_group();
    if(to_send == NULL){
        printf("Invalid Group name, Try again\n");
        return;
    }
    char *send = malloc(sizeof(char) * BUFLEN);
    char *message = malloc(sizeof(char) * BUFLEN);
    
    printf("Write your message: ");
    fgets(message, BUFLEN, stdin);
    
    send[0] = '\0';
    strcat(send, "MESSAGE:");
    strcat(send, "Group ");
    strcat(send, to_send->name);
    strcat(send, " User ");
    strcat(send, username);
    strcat(send, ":");
    strcat(send, message);

    printf("Message Sent to Group\n");
    if(sendto(group_fd, send, strlen(send) + 1, 0, (struct sockaddr *) &to_send->send_addr, slen) == -1)
        printf("Erro no sendto");
    free(send);
}

void list_my_groups(){
    printf("\nMy List of groups:\n");
    group *temp = firstgroup;
    while (temp != NULL){
        printf("\t-> %s\n", temp->name);
        temp = temp->next;
    }
}

group *choose_group(){
    char *g_name = malloc(sizeof(char) * BUFLEN);
    list_my_groups();
    printf("Choose a group: ");
    fgets(g_name, BUFLEN, stdin);
    g_name[strlen(g_name) - 1] = '\0';
    return search_group(g_name);
}

group *search_group(char *g_name){
    group *temp = firstgroup;
    while (temp != NULL){
        if(strcmp(temp->name, g_name) == 0)
            return temp;
        temp = temp->next;
    }
    return NULL;
}

void leave_group(){
    group *leaving = choose_group();
    if(leaving == NULL){
        printf("Can't leave a non existant group");
        return;
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = leaving->send_addr.sin_addr.s_addr;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(leaving->socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0){
        perror("setsockopt");
        return;
    }
    remove_group(leaving);
}

void remove_group(group *leaving){
    if(leaving == firstgroup)
        firstgroup = firstgroup->next;
    else{
        group * temp = firstgroup;
        while(temp->next != leaving)
            temp = temp->next;
        temp->next = leaving->next;
    }
    free(leaving);
}





