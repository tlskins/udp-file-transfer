/* rudp.cc */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>          // hostent

#include "rudp.h"

#define sendrecvflag 0
#define databuffersize 1024
#define PORT_NO 15050

#define MSG_TYPE_FILENAME               1
#define MSG_TYPE_FILE_DATA              2
#define MSG_TYPE_ACKNOWLEDGEMENT        3
#define MSG_TYPE_ERROR_INVALID_FILE     4

typedef struct _MessageRecord {
    uint32_t        msg_type;
    uint32_t        seq;
    uint32_t        datasize;
    uint8_t         data[databuffersize];
} MessageRecord;

const char* getMessageType(uint32_t i)
{
    switch(i){
    case MSG_TYPE_FILENAME:
        return ("MSG_TYPE_FILENAME");
        break;
    case MSG_TYPE_FILE_DATA:
        return ("MSG_TYPE_FILE_DATA");
        break;
    case MSG_TYPE_ACKNOWLEDGEMENT:
        return ("MSG_TYPE_ACKNOWLEDGEMENT");
        break;
    case MSG_TYPE_ERROR_INVALID_FILE:
        return ("MSG_TYPE_ERROR_INVALID_FILE");
    default:
        return ("MSG_TYPE_INVALID");
    }
}

int serverListen(int sockfd)
{
    MessageRecord       msgRcd;
    int                 nBytes;
    uint32_t            msg_type;
    struct sockaddr_in  addr_con;
    socklen_t           addrlen = sizeof(addr_con);
    char                buf[INET_ADDRSTRLEN];

    do{
        fprintf(stderr, "rcv: calling recvfrom() ...\n");
        nBytes = recvfrom(sockfd, &msgRcd, sizeof(MessageRecord), sendrecvflag,
            (struct sockaddr*) &addr_con, &addrlen);
        msg_type = ntohl(msgRcd.msg_type);
        fprintf(stderr, "rev: receive %d bytes with message type : %s\n",
            nBytes, getMessageType(msg_type));
    } while(msg_type != MSG_TYPE_FILENAME);

    printf("Receive a file name: %s from %s port %d\n",
           (char*) msgRcd.data,
           inet_ntop(AF_INET, &addr_con.sin_addr, buf, INET_ADDRSTRLEN),
           ntohs(addr_con.sin_port));

    // check to see if addr_con is a new one

    return (0);
}

int serverInit()
{
    int sockfd;
    int bd;
    struct sockaddr_in addr_con;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0){
        fprintf(stderr, "Error: Cannot create a socket\n");
        exit(1);
    }

    fprintf(stderr, "rcv: Created a file descriptor %d\n", sockfd);

    memset(&addr_con, 0, sizeof(struct sockaddr_in));
    addr_con.sin_family = AF_INET;
    addr_con.sin_port = htons(PORT_NO);
    addr_con.sin_addr.s_addr = INADDR_ANY; // bind to all intefaces

    bd = bind(sockfd, (struct sockaddr*)&addr_con, sizeof(struct sockaddr_in));
    if (bd == 0){
        fprintf(stderr, "rcv: Binded port '%d' Successfully\n", PORT_NO);
    }else{
        fprintf(stderr, "ERROR: Binding Failed (%d)!\n", bd);
    }
    return (sockfd);
}

char* findIp(char* name)
{
    struct hostent*     he;
    struct in_addr**    addr_list;
    int                 i;

    he = gethostbyname(name);
    if (he == NULL){
        fprintf(stderr, "ERROR: cannot find host name '%s' information\n", name);
        exit(1);
    }
    addr_list = (struct in_addr**) he->h_addr_list;
    for(i = 0; addr_list[i] != NULL; i++){
        fprintf(stderr, "IP: %s\n", inet_ntoa(*addr_list[i]));
    }
    return (inet_ntoa(*addr_list[0]));
}

int clientInit(char* serverName)
{
    int                 sockfd;
    int                 ret;
    struct sockaddr_in  addr_con;
    struct sockaddr_in  sock_addr;
    struct sockaddr_in  serv_addr;
    socklen_t           socklen;
    char                buf[INET_ADDRSTRLEN];
    char*               serverIpAddress;

    serverIpAddress = findIp(serverName);
    fprintf(stderr, "ncp: server '%s' ip address: %s\n",
        serverName, serverIpAddress);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0){
        fprintf(stderr, "Error: Cannot create a socket\n");
        exit(1);
    }

    fprintf(stderr, "ncp: Created a file descriptor %d\n", sockfd);

    memset(&addr_con, 0, sizeof(struct sockaddr_in));
    addr_con.sin_family = AF_INET;
    addr_con.sin_port = htons(0); // bind to ephemeral port
    // addr_con.sin_addr.s_addr = INADDR_ANY; // bind to all intefaces

    ret = bind(sockfd, (struct sockaddr*)&addr_con, sizeof(struct sockaddr_in));
    if (ret == 0){
        fprintf(stderr, "ncp: Binded Successfully\n");
    }else{
        fprintf(stderr, "ERROR: Binding Failed!\n");
        exit(-1);
    }

    // print out the client information
    socklen = sizeof(struct sockaddr);
    memset(&sock_addr, 0, sizeof(struct sockaddr_in));
    getsockname(sockfd, (struct sockaddr *) &sock_addr, &socklen);
    fprintf(stderr, "ncp: Client socket bound to %s ephemeral port %d\n",
        inet_ntop(AF_INET, &sock_addr.sin_addr, buf, INET_ADDRSTRLEN),
        ntohs(sock_addr.sin_port));

    // connect to the server
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT_NO);
    inet_pton(AF_INET, serverIpAddress, &serv_addr.sin_addr);

    // UDP connect() always return 0 because it's connectionless
    ret = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
    if (ret != 0){
        fprintf(stderr, "ERROR: failed to connect to server '%s', ip '%s', port '%d'\n",
            serverName, serverIpAddress, PORT_NO);
        exit(-1);
    }

    // print out the server information
    socklen = sizeof(struct sockaddr);
    memset(&sock_addr, 0, sizeof(struct sockaddr_in));
    getpeername(sockfd, (struct sockaddr *) &sock_addr, &socklen);
    fprintf(stderr, "ncp: Client socket connect to server '%s', ip '%s', port %d\n",
        serverName,
        inet_ntop(AF_INET, &sock_addr.sin_addr, buf, INET_ADDRSTRLEN),
        ntohs(sock_addr.sin_port));

    return (sockfd);
}

int sendFileName(int sockfd, char* localFileName, char* remoteFileName)
{
    int                 nBytes;
    MessageRecord       msgRcd;
    FILE*               fp;
    
    fp = fopen(localFileName, "r");
    if (fp == NULL){
        fprintf(stderr, "ERROR: local file name '%s' doesn't exist\n", localFileName);
        exit (1);
    }

    memset(&msgRcd, 0, sizeof(MessageRecord));
    msgRcd.msg_type = htonl(MSG_TYPE_FILENAME);
    strcpy((char*) msgRcd.data, remoteFileName);
    // pass remote file name to server with send()
    fprintf(stderr, "ncp: calling send() to send file name '%s'\n", remoteFileName);
    nBytes = send(sockfd, &msgRcd, sizeof(MessageRecord), sendrecvflag);
    if (nBytes == (-1)){
        fprintf(stderr, "ERROR: cannot send file name '%s' to server\n", remoteFileName);
        exit(1);
    }
    return (0);
}



