// server code for UDP socket programming
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "rudp.h"

#define NET_BUF_SIZE 32
#define sendrecvflag 0

#define     BUFFSIZE 2014
char        localFileName[BUFFSIZE];
char        remoteFileName[BUFFSIZE];
char        remoteMachine[BUFFSIZE];

void usage(char* av)
{
        fprintf(stderr, "%s <source_file_name> <dest_file_name> @ <comp_name>\n", av);
        exit(1);
}

int processingArgv(int argc, char* argv[])
{
    if (argc < 4){
        usage(argv[0]);
    }
    if (argc > 1){
        strcpy(localFileName, argv[1]);
    }
    if (argc > 2){
        strcpy(remoteFileName, argv[2]);
    }
    if (argc > 3){
        if (strcmp("@", argv[3]) != 0){
            usage(argv[0]);
        }
    }
    if (argc > 4){
        strcpy(remoteMachine, argv[4]);
    }
    fprintf(stderr, "local file name: %s\nremote file name: %s\nremote machine: %s\n",
        localFileName, remoteFileName, remoteMachine);
    return (0);
}

int main(int argc, char* argv[])
{
    int             sockfd;
    FILE*           fp;
    int             fsize;

    processingArgv(argc, argv);

    sockfd = clientInit(remoteMachine);
    
    fp = sendFileName(sockfd, localFileName, remoteFileName, &fsize);

    sendFileData(sockfd, fp);

    return 0;
}
