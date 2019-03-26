// client code for UDP socket programming
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>             // getopt()

#include "rudp.h"

int main(int argc, char* argv[])
{
    int         sockfd;
    int         fsize;

    sockfd = serverInit();

    fsize = serverListen(sockfd);

    fsize = receiveFileData(sockfd, fsize);

    fprintf(stderr, "rcv: after received file left %d bytes\n", fsize);

    return (0);

}
