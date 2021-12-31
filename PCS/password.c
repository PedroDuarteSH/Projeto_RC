// Adriana Gomes da Silva Leocádio Bernardo         2019218086
// Pedro Duarte Santos Henriques                    2019217793

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>


int mygetch () {
  int ch;
  struct termios oldt, newt;
  
  tcgetattr (STDIN_FILENO, &oldt );
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO );
  tcsetattr ( STDIN_FILENO, TCSANOW, &newt );
  ch = getchar();
  tcsetattr ( STDIN_FILENO, TCSANOW, &oldt );
  
  return ch;
} 

char* getpassword(int max_size){
  int ch;
  char *pword = malloc(sizeof(char) * max_size);
  int i = 0;
  
  while ((ch = mygetch()) != EOF 
          && ch != '\n' 
          && ch != '\r' 
          && i < max_size - 1)
  {
    if (ch ==  0x7f && i > 0){
      printf("\b \b");
      fflush(stdout);
      i--;
      pword[i] = '\0';
    }
    else if (isalnum(ch)){
      putchar('*');
      pword[i++] = (char)ch;
    }
  }

  pword[i] = '\0';
  
  
  return pword;
}
