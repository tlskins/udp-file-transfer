/* rudp.cc */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "rudp.h"

#define IP_PROTOCOL 0
#define sendrecvflag 0

int serverInit()
{
    int sockfd;
    int bd;
    struct sockaddr_in addr_con;
    socklen_t addrlen = sizeof(addr_con);
    addr_con.sin_family = AF_INET;
    addr_con.sin_port = htons(PORT_NO);
    addr_con.sin_addr.s_addr = INADDR_ANY; // bind to all intefaces

    sockfd = socket(AF_INET, SOCK_DGRAM, IP_PROTOCOL);

    if (sockfd < 0){
        fprintf("Error: Cannot create a socket\n");
        exit(1);
    }

    fprintf(stderr, "Created a file descriptor %d\n", sockfd);

    bd = bind(sockfd, (struct sockaddr*)&addr_con, sizeof(addr_con)) == 0)
    if (bd == 0){
        fprintf(stderr, "\nBinded Successfully\n");
    }else{
        fprintf(stderr, "\nBinding Failed (%d)!\n", );
    }
    return (sockfd);
}
