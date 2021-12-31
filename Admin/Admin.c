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
#include <signal.h>
#define BUFLEN	1024
#define NUM_STR_LENGHT 20

#define MAX_USERNAME_SIZE 32
#define MIN_USERNAME_SIZE 5

void cleanup_client(int signo);
void erro(char *msg);
int fd;

#define LIST "LIST"
#define ADD "ADD"
#define DEL "DEL"
#define EXIT "QUIT"


int main(int argc, char *argv[]) {
  signal(SIGINT, cleanup_client);
  char endServer[100];
  struct sockaddr_in addr;
  struct hostent *hostPtr;
  fd = -1;
  if (argc != 3) {
    printf("cliente <host> <port>\n");
    exit(-1);
  }

  strcpy(endServer, argv[1]);
  if ((hostPtr = gethostbyname(endServer)) == 0)
    erro("Não consegui obter endereço");


  bzero((void *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
  addr.sin_port = htons((short) atoi(argv[2]));

  if ((fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	  erro("socket");
  if (connect(fd,(struct sockaddr *)&addr,sizeof (addr)) < 0)
	  erro("Connect");
  
  int choice;
  char *command = malloc(sizeof(char) * BUFLEN);
  char *response = malloc(sizeof(char) * BUFLEN);
  
  char *temp;

  while(1){
    bzero(response, BUFLEN);
    //Get command from console
    printf("--- ADMIN CONSOLE ---\n\t1 -> List server Users\n\t2 -> Add a User\n\t3 -> Remove a User\n\t0 -> Exit\nChoice: ");
    fgets(command, BUFLEN, stdin);
    command[strcspn(command, "\n")] = 0;
    choice = strtol(command, &temp, 10);

    if(choice == 1){
      write(fd, LIST, 1 + strlen(LIST));
      read(fd, command, BUFLEN-1);
      printf("\nServer User List:\n");
      printf("%s", command);
    }
    //ADD
    else if(choice == 2){ 
      //int c;
      strcat(response, ADD);
      strcat(response, " ");

      printf("--- ADDING USER --- ");
      printf("\nYour username and password must bigger than %d and smaller then %d", MIN_USERNAME_SIZE, MAX_USERNAME_SIZE);
      printf("\nYour password must be composed by numbers or letters without special characters(!, á, ?, /,...)\nInsert Username: ");

      fgets(command, BUFLEN, stdin);
      command[strlen(command) - 1] = '\0';
      strcat(response, command);
      strcat(response, " ");
      if(strlen(command) < MIN_USERNAME_SIZE || strlen(command) > MAX_USERNAME_SIZE){
        printf("Invalid Username Size\n");
        continue;
      }

      printf("Insert Ip Address: ");
      fgets(command, BUFLEN, stdin);
      command[strlen(command) - 1] = '\0';
      strcat(response, command);
      strcat(response, " ");
      
      printf("Insert password: ");
      fgets(command, BUFLEN, stdin);
      command[strlen(command) - 1] = '\0';
      strcat(response, command);
      strcat(response, " ");
      if(strlen(command) < MIN_USERNAME_SIZE || strlen(command) > MAX_USERNAME_SIZE){
        printf("Invalid Username Size\n");
        continue;
      }

      printf("Can cliente communicate Client -> Server -> Client? ");
      fgets(command, BUFLEN, stdin);
      command[strlen(command) - 1] = '\0';
      strcat(response, command);
      strcat(response, " ");
    
      printf("Can cliente communicate Client -> Client (P2P)? ");
      fgets(command, BUFLEN, stdin);
      command[strlen(command) - 1] = '\0';
      strcat(response, command);
      strcat(response, " ");
    
      printf("Can cliente communicate in group? ");
      fgets(command, BUFLEN, stdin);
      command[strlen(command) - 1] = '\0';
      strcat(response, command);
      strcat(response, " ");
    
      write(fd, response, 1 + strlen(response));
    }
    //Del
    else if(choice == 3){
      strcat(response, DEL);
      strcat(response, " ");
      
      printf("--- REMOVING USER ---\nInsert Username: ");
      fgets(command, BUFLEN, stdin);
      command[strlen(command) - 1] = '\0';
      strcat(response, command);
      strcat(response, " ");

      write(fd, response, 1 + strlen(response));
    }
    //Leave
     else if(choice == 0 && temp != command){
      write(fd, EXIT, 1 + strlen(EXIT));
      printf("Cliente saindo\n");
      break;
    }
    else printf("Erro: Comando não existe\n");
    fflush(stdout);
  }
  close(fd);
  exit(0);
}

void erro(char *msg) {
  printf("Erro: %s\n", msg);
	exit(-1);
}


void cleanup_client(int signo) // clean up resources by pressing Ctrl-C
{
  //Close client socket
  printf("Cliente Saindo\n");
  if(fd > 0)  write(fd, EXIT, 1 + strlen(EXIT));
	exit(0);
}
