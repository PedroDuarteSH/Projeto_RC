#include "server.h"

// Adriana Gomes da Silva Leocádio Bernardo         2019218086
// Pedro Duarte Santos Henriques                    2019217793
pid_t udp_server_pid, tcp_server_pid;

int main(int argc, char *argv[]) {
  signal(SIGTSTP, SIG_IGN);
  signal(SIGINT, SIG_IGN);
  customer_port = -1;
  config_port = -1;
  list.first_client = NULL;
  list.n_clients = 0;
  //Verify user input commands
  if (argc != 4) {
    printf("Correct use: server {port customers} {port config} {registry file name}\n");
    exit(-1);
  }

  char *temp;
  //Read 
  customer_port = strtol(argv[1], &temp, 10);
  if (temp == argv[1] || customer_port < 0){
    printf("%s => Invalid customers port!\n", argv[1]);
    exit(-1);
  }

  config_port = strtol(argv[2], &temp, 10);
  if (temp == argv[2] || config_port == -1){
    printf("%s => Invalid config port!\n", argv[2]);
    exit(-1);
  }
  file_name = malloc(sizeof(char) * MAX_LINE);
  strcpy(file_name, argv[3]);


  read_registry_file(0);
  client *temp_c = list.first_client;
  
  printf("Initiating user List:\n");
  while (temp_c != NULL){
    printf("USER:\n");
    printf("NAME: %s\n", temp_c->user_id);
    printf("ADDRESS: %s\n", temp_c->ip_address);
    printf("password: %s\n", temp_c->password);
    printf("client_server: %d\n", temp_c->perms.client_server);
    printf("P2P: %d\n", temp_c->perms.P2P);
    printf("group: %d\n\n", temp_c->perms.group);
    temp_c = temp_c->next;
  }

  
  if(inet_pton(AF_INET, SERVER_IP, &server_ip) == 0){
    printf("Error converting Server IP string to in_addr Struct\n");
    exit(-1);
  }

  //Create UDP server Process
  udp_server_pid = fork();
  if(udp_server_pid == 0){
    udp_server();
  }

  //Create TCP server Process
  tcp_server_pid = fork();
  if(tcp_server_pid == 0){
    tcp_server();
  }

  waitpid(tcp_server_pid, NULL, 0);
  waitpid(udp_server_pid, NULL, 0);
  exit(0);
}

void read_registry_file(int new){
  if(new == 0) list.first_client = NULL;
  else list.new_list = NULL;
  char *line = malloc(sizeof(char) * MAX_LINE);
  FILE *registry_file =fopen(file_name, "r");
  while(fgets(line, MAX_LINE, registry_file) != NULL){
    process_user(line, new);
  }
  fclose(registry_file);
}

void process_user(char *line, int new){
  char *temporary[N_PARAM];
  int perms[3];
  temporary[0] = strtok_r(line, " \n", &line);
  for (int i = 1; i < N_PARAM; i++){
    temporary[i] = strtok_r(line, " \n", &line);
    if(i >= 3){
      if(strcmp(temporary[i], "yes") == 0)perms[i-3] = 1;
      else perms[i-3] = 0;
    }
  }
  add_user(temporary[0], temporary[1],temporary[2], perms[0], perms[1], perms[2], new);
}

void add_user(char *user_id, char *address, char *password, int client_server, int P2P, int group, int new){
  client *this = malloc(sizeof(client));
  this->user_id = concat("", user_id);
  this->ip_address = concat("", address);
  this->password = concat("", password);
  this->perms.client_server = client_server;
  this->perms.P2P = P2P;
  this->perms.group = group;
  this->next = NULL;

  if(new == 0){
      if(list.first_client == NULL){
        list.first_client = this;
        list.n_clients++;
        return;
      }

      client *temp = list.first_client;
      while (temp->next != NULL)
        temp = temp->next;

      temp->next = this;
      list.n_clients++;
      return;
  } 
  if(list.new_list == NULL){
    list.new_list = this;
    list.n_clients++;
    return;
  }

  client *temp = list.new_list;;
  while (temp->next != NULL)
    temp = temp->next;

  temp->next = this;
  list.n_clients++;
  return;
}


//String auxiliar
char * concat (char * s1, char * s2) {
	char * result = malloc(sizeof(char)*(strlen(s1)+strlen(s2)+1));
	strcpy(result,s1);
	strcat(result,s2);
	return result;
}



void update_list(){
  read_registry_file(1);
  client *f = list.new_list;
  client *d = list.first_client;
  while(f != NULL){
    client *d = list.first_client;
    while( d != NULL){
      if(strcmp(f->user_id, d->user_id) == 0){
        f->logged_in_port = d->logged_in_port;
        f->logged_in = d->logged_in;
      }
      d = d->next;
    }
    f = f->next;
  }
  d = list.first_client;
  client *old = NULL;
  while( d != NULL){
    old = d;
    d = d->next;
    free(old);
  }
  list.first_client = list.new_list;
}