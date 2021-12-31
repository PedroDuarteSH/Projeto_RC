// Simple sender.c program for UDP
//
// Adapted from:
// http://ntrg.cs.tcd.ie/undergrad/4ba2/multicast/antony/example.html
//
// Changes:
// * Compiles for Windows as well as Linux
// * Takes the port and group on the command line
//
// Note that what this program does should be equivalent to NETCAT:
//
//     echo "Hello World" | nc -u 239.255.255.250 1900


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // for sleep()
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
 /* for strncpy */

#include <sys/ioctl.h>
#include <net/if.h>



int main(int argc, char *argv[])
{
    if (argc != 3) {
       printf("Command line args should be multicast group and port\n");
       printf("(e.g. for SSDP, `sender 239.255.255.250 1900`)\n");
       return 1;
    }

    char* group = argv[1]; // e.g. 239.255.255.250 for SSDP
    int port = atoi(argv[2]); // 0 if error, which is an invalid port

    // !!! If test requires, make these configurable via args
    const char *message = "Hello, World!";

    // create what looks like an ordinary UDP socket
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    // set up destination address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(group);
    addr.sin_port = htons(port);
    
    int multicastTTL = 254;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicastTTL,sizeof(multicastTTL)) < 0) {
        perror("socket opt");
        return 1;
    }


    struct in_addr sin_addr;
    sin_addr.s_addr = htonl(INADDR_ANY);
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, (char*) &sin_addr, sizeof(sin_addr)) < 0){
        perror("setsockopt");
        return 1;
    }


    // now just sendto() our destination!
    while (1) {
        char ch = 0;
        int nbytes = sendto(fd,message,strlen(message),0,(struct sockaddr*) &addr,sizeof(addr));
        if (nbytes < 0) {
            perror("sendto");
            return 1;
        }
    }
    return 0;
}