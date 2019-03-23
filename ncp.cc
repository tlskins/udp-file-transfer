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

/*
// funtion sending file
int sendFile(FILE* fp, char* buf, int s)
{
    int i, len;
    if (fp == NULL) {
        strcpy(buf, nofile);
        len = strlen(nofile);
        buf[len] = EOF;
        for (i = 0; i <= len; i++)
            buf[i] = Cipher(buf[i]);
        return 1;
    }

    char ch, ch2;
    for (i = 0; i < s; i++) {
        ch = fgetc(fp);
        ch2 = Cipher(ch);
        buf[i] = ch2;
        if (ch == EOF)
            return 1;
    }
    return 0;
}
*/

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

    processingArgv(argc, argv);

    sockfd = clientInit(remoteMachine);
    
    sendFileName(sockfd, localFileName, remoteFileName);

    exit(0);
/*

    while (1) {
        printf("\nWaiting for file name...\n");

        // receive file name
        clearBuf(net_buf);

        nBytes = recvfrom(sockfd, net_buf,
                          NET_BUF_SIZE, sendrecvflag,
                          (struct sockaddr*)&addr_con, &addrlen);

        fp = fopen(net_buf, "r");
        printf("\nFile Name Received: %s\n", net_buf);
        if (fp == NULL)
            printf("\nFile open failed!\n");
        else
            printf("\nFile Successfully opened!\n");

        while (1) {

            // process
            if (sendFile(fp, net_buf, NET_BUF_SIZE)) {
                sendto(sockfd, net_buf, NET_BUF_SIZE,
                       sendrecvflag,
                    (struct sockaddr*)&addr_con, addrlen);
                break;
            }

            // send
            sendto(sockfd, net_buf, NET_BUF_SIZE,
                   sendrecvflag,
                (struct sockaddr*)&addr_con, addrlen);
            clearBuf(net_buf);
        }
        if (fp != NULL)
            fclose(fp);
    }
*/

    return 0;
}
