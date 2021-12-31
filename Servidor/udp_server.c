// Adriana Gomes da Silva Leocádio Bernardo         2019218086
// Pedro Duarte Santos Henriques                    2019217793
#include "server.h"


int udp_fd;
struct sockaddr_in udp_addr, udp_client_addr, adress_to_send;
socklen_t slen;

group_ips current;

void udp_server(){
    //To update file if needed
    signal(SIGTSTP, update_list);
    signal(SIGINT, close_server);
    current.first = 1;
    current.second = 1;
    current.third = 1;
    current.fourth = 230;
    udp_fd = -1;

    firstgroup = NULL;
    slen = sizeof(udp_client_addr);

	int recv_len;
	
	
    
	// Cria um socket para recepção de pacotes UDP
	if((udp_fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		printf("Erro na criação do socket");
	}

    // Preenchimento da socket address structure
	udp_addr.sin_family = AF_INET;
    inet_aton(SERVER_IP, &udp_addr.sin_addr);
	udp_addr.sin_port = htons(customer_port);

    // Associa o socket à informação de endereço
    if(bind(udp_fd,(struct sockaddr*)&udp_addr, sizeof(udp_addr)) == -1){
        printf("Erro no bind\n");
        close(udp_fd);
        exit(0);
    }


    printf("UDP -> A receber comunicações no endereço %s:%d\n", SERVER_IP, customer_port);
    char buf[BUFLEN];
    char *save_ptr;
    char *command;
    while(TRUE){
        // Create required string
        // Espera recepção de mensagem (a chamada é bloqueante)
        if((recv_len = recvfrom(udp_fd, buf, BUFLEN, 0, (struct sockaddr *) &udp_client_addr, (socklen_t *)&slen)) == -1)
            printf("Erro no recvfrom");

        // Para ignorar o restante conteúdo (anterior do buffer)
        buf[recv_len-1]='\0';
        
        printf("Recebi uma mensagem do sistema com o endereço %s e o porto %d\n", inet_ntoa(udp_client_addr.sin_addr), ntohs(udp_client_addr.sin_port));
        printf("Conteúdo da mensagem: %s\n" , buf);
        fflush(stdout);
        command = strtok_r(buf, ":", &save_ptr);
        
        if(strcmp(command, "LOGIN") == 0)
            user_login(save_ptr);
       
        else if(strcmp(command, "USERS") == 0){
            send_online_users();
        }
        //Server Cliente message
        else if(strcmp(command, "ServerMSG") == 0)
            send_c2c_message(save_ptr);
        
        //P2P message
        else if(strcmp(command, "P2PMSG") == 0)
            process_P2P(save_ptr);
        
        //Create a multilink group
        else if(strcmp(command, "CREATEGROUP") == 0)
            create_multilink(save_ptr);

        //Join a multilink group
        else if(strcmp(command, "JOINGROUP") == 0){
            join_multilink(save_ptr);
        }
        else if(strcmp(command, "LEAVE") == 0){
            client_leave();
        }
        
    }

    // Fecha socket e termina programa
    close(udp_fd);
    

    exit(0);
}

void user_login(char *data){
    char send_str[BUFLEN];
    bzero((void *) send_str, sizeof(send_str));

    client *connecting = NULL;
    socklen_t slen = sizeof(udp_client_addr);

    connecting = validate_user(data, udp_client_addr);
    if(connecting != NULL){
        snprintf(send_str, BUFLEN, "Accepted:%d:%d:%d", connecting->perms.client_server, connecting->perms.P2P, connecting->perms.group);
        if(sendto(udp_fd, send_str, strlen(send_str) + 1, 0, (struct sockaddr *) &udp_client_addr, slen) == -1) 
            printf("Erro no sendto");
        connecting->logged_in = TRUE;
        connecting->logged_in_port = udp_client_addr.sin_port;
    }
    else{
        snprintf(send_str, BUFLEN, "Rejected");
        if(sendto(udp_fd, send_str, strlen(send_str) + 1, 0, (struct sockaddr *) &udp_client_addr, slen) == -1)
            printf("Erro no sendto");
    }
}

client* validate_user(char *string, struct sockaddr_in udp_client_addr){
    char *username;
    char *password;
    username = strtok_r(string, ":", &string);
    password = strtok_r(string, ":", &string);
    if(username == NULL || password == NULL)
        return NULL;

    client * temp_c = list.first_client;
    while (temp_c != NULL){
        if(strcmp(temp_c->user_id, username) == 0 && strcmp(temp_c->password, password) == 0)
            if(strcmp(temp_c->ip_address, inet_ntoa(udp_client_addr.sin_addr)) == 0)
                return temp_c;
        temp_c = temp_c->next;
    }
    return NULL;
}


void send_c2c_message(char *string){
    char *save_ptr;
    char *username = strtok_r(string, ":", &save_ptr);
    char *message = strtok_r(save_ptr, ":", &save_ptr);
    char *send_str = malloc(sizeof(char) * BUFLEN);
    send_str[0] = '\0';


    client *to_send = search_user_username(username);
    client *sender = search_user_ip(inet_ntoa(udp_client_addr.sin_addr), udp_client_addr.sin_port);
    

    if(to_send == NULL){
        snprintf(send_str, BUFLEN, "REJECTED:Server Info:User does not exists or is not online");
        if(sendto(udp_fd, send_str, strlen(send_str) + 1, 0, (struct sockaddr *) &udp_client_addr, slen) == -1)
            printf("Erro no sendto");
        return;
    }    

    
    // Preenchimento da socket address structure
    adress_to_send.sin_family = AF_INET;
    inet_aton(to_send->ip_address, &adress_to_send.sin_addr);
	adress_to_send.sin_port = to_send->logged_in_port;

    //Generate message

    strcat(send_str, "MESSAGE:");
    strcat(send_str, sender->user_id);
    strcat(send_str, ":");
    strcat(send_str, message);
   

    if(sendto(udp_fd, send_str, strlen(send_str) + 1, 0, (struct sockaddr *) &adress_to_send, slen) == -1)
        printf("Erro no sendto");
    
    snprintf(send_str, BUFLEN, "RESCSC:Server Info:Message sent");
    if(sendto(udp_fd, send_str, strlen(send_str) + 1, 0, (struct sockaddr *) &udp_client_addr, slen) == -1)
        printf("Erro no sendto");
    free(send_str);
}

void process_P2P(char *string){
    char *save_ptr;
    char *username = strtok_r(string, ":", &save_ptr);
    char *send_str = malloc(sizeof(char) * BUFLEN);

    send_str[0] = '\0';

    client *to_send = search_user_username(username);

    if(to_send == NULL){
        snprintf(send_str, BUFLEN, "REJECTED:Server Info:User does not exists or is not online");
        if(sendto(udp_fd, send_str, strlen(send_str) + 1, 0, (struct sockaddr *) &udp_client_addr, slen) == -1)
            printf("Erro no sendto");
        return;
    }
    //RESP2P:dest_ip:dest_port
    snprintf(send_str, BUFLEN, "RESP2P:%s:%d", to_send->ip_address, to_send->logged_in_port);

    if(sendto(udp_fd, send_str, strlen(send_str) + 1, 0, (struct sockaddr *) &udp_client_addr, slen) == -1)
        printf("Erro no sendto");
    free(send_str);
}


void create_multilink(char *string){
    char *save_ptr;
    char *group_name = strtok_r(string, ":", &save_ptr);
    char *send_str = malloc(sizeof(char) * BUFLEN);

    if(search_group(group_name) != NULL){
        snprintf(send_str, BUFLEN, "REJECTED:Server Info:Group name already exists, please insert a new one");
        if(sendto(udp_fd, send_str, strlen(send_str) + 1, 0, (struct sockaddr *) &udp_client_addr, slen) == -1)
            printf("Erro no sendto");
        return;
    }

    char *ip = generate_ip();
    if(ip == NULL){
        snprintf(send_str, BUFLEN, "REJECTED:Server Info:Server can't handle more groups :(");
        if(sendto(udp_fd, send_str, strlen(send_str) + 1, 0, (struct sockaddr *) &udp_client_addr, slen) == -1)
            printf("Erro no sendto");
        return;
    }

    client *creator = search_user_ip(inet_ntoa(udp_client_addr.sin_addr), udp_client_addr.sin_port);
    
    group *new = add_group(group_name, ip, creator);
    snprintf(send_str, BUFLEN, "RESGROUP:%s:%d", new->ip_address, new->port);
    if(sendto(udp_fd, send_str, strlen(send_str) + 1, 0, (struct sockaddr *) &udp_client_addr, slen) == -1)
        printf("Erro no sendto");
    free(send_str);

}


char *generate_ip(){
    char *ip = malloc(sizeof(char) * INET_ADDRSTRLEN);
    snprintf(ip, INET_ADDRSTRLEN, "%d.%d.%d.%d", current.fourth, current.third, current.second, current.first);
    current.first++;
    if(current.first == 255){
        current.first = 0;
        current.second++;
        if(current.second == 255){
            current.second = 0;
            current.third++;
            if(current.third > 255){
                current.third = 0;
                current.fourth++;
                if(current.fourth > 239){
                    return NULL;
                }
                else return ip;
            }
            else return ip;
        }
        else return ip;
    }
    else return ip;
}

group* search_group(char *name){
    group *current = firstgroup;
    while(current != NULL){
        if(strcmp(name, current->name) == 0)
            return current;
        current = current->next;
    }
    return NULL;
}

group *add_group(char* group_name, char* ip, client* creator){
    group *new = malloc(sizeof(group));
    new->ip_address = ip;
    new->port = 10000;
    new->name = malloc((strlen(group_name) + 1) * sizeof(char));
    strcpy(new->name, group_name);

    new->next = NULL;


    if(firstgroup == NULL)
        firstgroup = new;
    else{
        group *temp = firstgroup;
        while (temp->next != NULL)
            temp = temp->next;
        temp->next = new;
    }
    return new;
}

client *search_user_username(char *string){
    char *username;
    username = strtok_r(string, ":", &string);
    client * temp_c = list.first_client;
    while (temp_c != NULL){
        if(strcmp(temp_c->user_id, username) == 0)
            if(temp_c->logged_in == TRUE)
                return temp_c;
        temp_c = temp_c->next;
    }
    
    return NULL;
}

client *search_user_ip(char *string, uint16_t port){
    client * temp_c = list.first_client;
    while (temp_c != NULL){
        if(strcmp(temp_c->ip_address, string) == 0 && temp_c->logged_in_port == port)
            if(temp_c->logged_in == TRUE)
                return temp_c;
        temp_c = temp_c->next;
    }
    return NULL;
}

void join_multilink(char *string){
    char *save_ptr;
    char *group_name = strtok_r(string, ":", &save_ptr);
    char *send_str = malloc(sizeof(char) * BUFLEN);
    group *ToJoin = search_group(group_name);
    if(ToJoin == NULL){
        snprintf(send_str, BUFLEN, "REJECTED:Server Info:Group was not found");
        if(sendto(udp_fd, send_str, strlen(send_str) + 1, 0, (struct sockaddr *) &udp_client_addr, slen) == -1)
            printf("Erro no sendto");
        return;
    }

    snprintf(send_str, BUFLEN, "RESGROUP:%s:%d", ToJoin->ip_address, ToJoin->port);
    if(sendto(udp_fd, send_str, strlen(send_str) + 1, 0, (struct sockaddr *) &udp_client_addr, slen) == -1)
        printf("Erro no sendto");
    free(send_str);
}

void send_online_users(){
    char *input = malloc(sizeof(char) * BUFLEN);
    char *temp = malloc(sizeof(char) * BUFLEN);
    bzero(input, BUFLEN);
    client * temp_c = list.first_client;
    while (temp_c != NULL){
        if(temp_c->logged_in == TRUE){
            snprintf(temp, BUFLEN, "\t\033[0;30m-> \033[0;32m%s\n", temp_c->user_id);
            strcat(input, temp);
        }
        else{
            snprintf(temp, BUFLEN, "\t\033[0;30m-> \033[1;31m%s\n", temp_c->user_id);
            strcat(input, temp);
        }
        temp_c = temp_c->next;
    }
    strcat(input, "\033[0;30m");
    if(sendto(udp_fd, input, strlen(input) + 1, 0, (struct sockaddr *) &udp_client_addr, slen) == -1)
        printf("Erro no sendto");
    free(input);
    free(temp);
}

void client_leave(){
    client *sender = search_user_ip(inet_ntoa(udp_client_addr.sin_addr), udp_client_addr.sin_port);
    sender->logged_in = FALSE;
}

void close_server(){
    if(udp_fd>-1)
        close(udp_fd);
    printf("UDP SERVER LEAVING\n");
    exit(0);
}
