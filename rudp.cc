/* rudp.cc */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>       // stat()
#include <unistd.h>
#include <netdb.h>          // hostent
#include <errno.h>          // error

#include "rudp.h"

#define sendrecvflag 0
#define databuffersize 1024
#define PORT_NO 15050

#define MSG_TYPE_FILENAME               1
#define MSG_TYPE_FILE_DATA              2
#define MSG_TYPE_FILE_END               3
#define MSG_TYPE_ACK                    4
#define MSG_TYPE_ERROR_INVALID_FILE     5

typedef struct _MessageRecord {
    uint32_t        msg_type;
    uint32_t        seq;
    uint32_t        datasize;
    uint8_t         data[databuffersize];
} MessageRecord;

typedef struct _DataRecord {
    int                     msg_type;
    unsigned int            seq;
    int                     datasize;
    void*                   data;
    struct sockaddr_in*     addr_con;
    char*                   ip;
    int                     port;
} DataRecord;

struct sockaddr_in  curr_addr;
static int          currentClient = 0;
char                fileName[databuffersize];

void convertfrom(MessageRecord* pmsgRcd, struct sockaddr_in* paddr_con, DataRecord *pdataRecord)
{
    char                buf[INET_ADDRSTRLEN];

    memset(pdataRecord, 0, sizeof(DataRecord));
    pdataRecord->msg_type = ntohl(pmsgRcd->msg_type);
    pdataRecord->seq = ntohl(pmsgRcd->seq);
    pdataRecord->datasize = ntohl(pmsgRcd->datasize);
    pdataRecord->data = &pmsgRcd->data[0];
    pdataRecord->addr_con = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
    memcpy(pdataRecord->addr_con, paddr_con, sizeof(struct sockaddr_in));
    inet_ntop(AF_INET, &((*paddr_con).sin_addr), buf, INET_ADDRSTRLEN);
    pdataRecord->ip = (char*) malloc(strlen(buf) + 1);
    strcpy(pdataRecord->ip, buf);
    pdataRecord->port = ntohs(paddr_con->sin_port);
}

void freeDataRecord(DataRecord* p)
{
    if (p != NULL){
        if (p->ip != NULL){
            free(p->ip);
            p->ip = NULL;
        }
        if (p->addr_con != NULL){
            free(p->addr_con);
            p->addr_con = NULL;
        }
        free(p);
    }
}

const char* getMessageType(uint32_t i)
{
    switch(i){
    case MSG_TYPE_FILENAME:
        return ("MSG_TYPE_FILENAME");
        break;
    case MSG_TYPE_FILE_DATA:
        return ("MSG_TYPE_FILE_DATA");
        break;
    case MSG_TYPE_ACK:
        return ("MSG_TYPE_ACK");
        break;
    case MSG_TYPE_ERROR_INVALID_FILE:
        return ("MSG_TYPE_ERROR_INVALID_FILE");
    default:
        return ("MSG_TYPE_INVALID");
    }
}

DataRecord* rudpRecvfrom(int sockfd, MessageRecord* pmsgRcd, struct sockaddr_in* paddr_rev)
{
    int                 nBytes;
    socklen_t           addrlen = sizeof(struct sockaddr_in);
    DataRecord*         pDataRecord = NULL;

    fprintf(stderr, "rudp:rudpRecvfrom: calling recvfrom() ...\n");
    nBytes = recvfrom(sockfd, pmsgRcd, sizeof(MessageRecord), sendrecvflag,
        (sockaddr*) paddr_rev, &addrlen);
    if (nBytes == (-1)){
        fprintf(stderr, "rudp:rudpRecvfrom: ERROR: recvfrom() return (%d}\n", errno);
        exit(0);
    }
    pDataRecord = (DataRecord*) malloc(sizeof(DataRecord));
    convertfrom(pmsgRcd, paddr_rev, pDataRecord);
    fprintf(stderr, "rcv:rudpRecvfrom: type %s, receive %d bytes, sequence (%d) from '%s' port %d\n",
            getMessageType(pDataRecord->msg_type),
            pDataRecord->datasize, pDataRecord->seq,
            pDataRecord->ip, pDataRecord->port);
    return (pDataRecord);
}

// return -1 if error
int rudpSend(int sockfd, MessageRecord* pmsgRcd)
{
    int                 nBytes;
    
    fprintf(stderr, "rudpSend: calling send() to send\n");
    nBytes = send(sockfd, pmsgRcd, sizeof(MessageRecord), sendrecvflag);
    if (nBytes == (-1)){
        fprintf(stderr, "rudpSend:ERROR: cannot send\n");
        exit(nBytes);
    }
    return (nBytes);
}

int rudpSendTo(int sockfd, MessageRecord* pmsgRcd, struct sockaddr_in* paddr_con)
{
    int                 nBytes;
    socklen_t           addrlen = sizeof(struct sockaddr);
    char                buf[INET_ADDRSTRLEN];
    struct sockaddr_in  sockaddr_local;
    
    memcpy(&sockaddr_local, paddr_con, sizeof(struct sockaddr));
    inet_ntop(AF_INET, &sockaddr_local.sin_addr, buf, INET_ADDRSTRLEN);

    fprintf(stderr, "rudpSendTo: calling sendto() to send to '%s', port %d\n",
        buf, ntohs(sockaddr_local.sin_port));
    nBytes = sendto(sockfd, pmsgRcd, sizeof(MessageRecord), sendrecvflag,
                (struct sockaddr*) &sockaddr_local, addrlen);
    if (nBytes == (-1)){
        fprintf(stderr, "rudpSendTo:ERROR: cannot send\n");
        exit(nBytes);
    }
    return (nBytes);
}

int receiveFileData(int sockfd, int fsize)
{
    MessageRecord       msgRcd;
    DataRecord*         pdataRecord;
    struct sockaddr_in  addr_con;
    FILE*               fp = NULL;
    int                 nBytes;

    // use receiveListen client information saved at 
    memcpy(&addr_con, &curr_addr, sizeof(struct sockaddr_in));
    while(fsize > 0){
        fprintf(stderr, "rcv:receiveFileData: calling rudpRecvfrom() ...\n");
        pdataRecord = rudpRecvfrom(sockfd, &msgRcd, &addr_con);
        fsize -= pdataRecord->datasize;
        if (fp == NULL){
            fp = fopen(fileName, "w");
            if (fp == NULL){
                fprintf(stderr, "rcv:receiveFileData: cannot open file '%s' for write\n",
                    fileName);
                exit (1);
            }
        }
        nBytes = fwrite(pdataRecord->data, 1, pdataRecord->datasize, fp);
        if (nBytes != pdataRecord->datasize){
            fprintf(stderr, "rcv:receiveFileData: suppose to write %d bytes but "
                " written %d bytes\n", pdataRecord->datasize, nBytes);
            exit(1);
        }

        // send ack back to client
        memset(&msgRcd, 0, sizeof(MessageRecord));
        msgRcd.msg_type = htonl(MSG_TYPE_ACK);
        msgRcd.seq = htonl(pdataRecord->seq);
        fprintf(stderr, "rcv:receiveFileData: calling rudpSendTo() to send ACK for seq (%d)\n",
            pdataRecord->seq);
        rudpSendTo(sockfd, &msgRcd, &addr_con);
    }
    
    // reset the curr_addr and counter
    currentClient--;
    memset(&curr_addr, 0, sizeof(struct sockaddr_in));
    if (fp != NULL){
        fclose(fp);
        fp = NULL;
    }
    fprintf(stderr, "rcv: receive whole file\n");
    return (fsize);
}

int sameClient(struct sockaddr_in* addr_con)
{
    int ret = memcmp(&curr_addr, &addr_con, sizeof(struct sockaddr_in));
    if (ret != 0){
        // this is a new client, save the client into linked-list and telling it to wait
        fprintf(stderr, "rcv: ERROR: new client (ask to wait and saved to linked-list\n");
        return (0);
    }else{
        return (1);
    }
}

// return the transmitted file size so server can count the left size
int serverListen(int sockfd)
{
    MessageRecord       msgRcd;
    int                 nBytes;
    int                 fsize;
    uint32_t            msg_type;
    struct sockaddr_in  addr_con;
    socklen_t           addrlen = sizeof(addr_con);
    char                buf[INET_ADDRSTRLEN];

    do{
        fprintf(stderr, "rcv:serverListen: calling recvfrom() ...\n");
        nBytes = recvfrom(sockfd, &msgRcd, sizeof(MessageRecord), sendrecvflag,
            (struct sockaddr*) &addr_con, &addrlen);
        if (nBytes == (-1)){
            fprintf(stderr, "ncv: ERROR: recvfrom() return (%d}\n", errno);
            exit(1);
        }
        msg_type = ntohl(msgRcd.msg_type);
    } while(msg_type != MSG_TYPE_FILENAME);

    // save file size and name
    fsize = ntohl(msgRcd.seq);
    strcpy(fileName, (char*) msgRcd.data);
    printf("rcv:serverListen: Receive %d bytes, file name: %s, size %d from %s port %d\n",
           nBytes, (char*) msgRcd.data, fsize,
           inet_ntop(AF_INET, &addr_con.sin_addr, buf, INET_ADDRSTRLEN),
           ntohs(addr_con.sin_port));

    // remember my current client so we know are we deal with the same client
    if (currentClient == 0){
        memcpy(&curr_addr, &addr_con, sizeof(struct sockaddr_in));
        currentClient++;
    }else if (currentClient == 1){
        // are we deal with the same client
        if (sameClient(&addr_con) == 0){
            exit(1);
        }
    }
    return (fsize);
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

    // zero curr_addr which is the curren client
    memset(&curr_addr, 0, sizeof(struct sockaddr_in));
    memset(fileName, 0, databuffersize);

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

FILE* sendFileName(int sockfd, char* localFileName, char* remoteFileName, int* fsize)
{
    int                 nBytes;
    MessageRecord       msgRcd;
    FILE*               fp;
    struct stat         statbuf;
    int                 ret;
    
    fp = fopen(localFileName, "r");
    if (fp == NULL){
        fprintf(stderr, "ERROR: local file name '%s' doesn't exist\n", localFileName);
        exit (1);
    }
    ret = stat(localFileName, &statbuf);
    if (ret != 0){
        fprintf(stderr, "ERROR: Cannot get file '%s' stat (%d)\n", localFileName, errno);
        exit(1);
    }
    nBytes = statbuf.st_size;
    *fsize = nBytes;
    fprintf(stderr, "ncp: file '%s' size '%d'\n", localFileName, nBytes);

    // pass the message type, remote file name and file size (in seq) to server
    memset(&msgRcd, 0, sizeof(MessageRecord));
    msgRcd.msg_type = htonl(MSG_TYPE_FILENAME);
    msgRcd.seq = htonl(nBytes);
    strcpy((char*) msgRcd.data, remoteFileName);
    // pass remote file name to server with send()
    fprintf(stderr, "ncp: calling send() to send file name '%s' information\n", remoteFileName);
    nBytes = rudpSend(sockfd, &msgRcd);
    if (nBytes == (-1)){
        fprintf(stderr, "ERROR: cannot send file name '%s' to server\n", remoteFileName);
        exit(1);
    }
    return (fp);
}

int sendFileData(int sockfd, FILE* fp)
{
    MessageRecord       msgRcd;
    DataRecord*         pdataRecord;
    int                 nBytes;
    uint32_t            seq = 0;
    int                 endOfFile = 0;
    int                 totalBytes = 0;
    struct sockaddr_in  addr_rec;
    
    do {
        seq++;
        memset(&msgRcd, 0, sizeof(MessageRecord));
        msgRcd.msg_type = htonl(MSG_TYPE_FILE_DATA);
        msgRcd.seq = htonl(seq);
        nBytes = fread(&msgRcd.data[0], sizeof(uint8_t), databuffersize, fp);
        if (nBytes != databuffersize){
            if (feof(fp) == 0){
                fprintf(stderr, "ncp: ERROR read file, buffer size (%d), reaed (%d)\n",
                    databuffersize, nBytes);
                exit(1);
            }
            endOfFile = 1;
        }
        if (endOfFile == 1){
            msgRcd.msg_type = htonl(MSG_TYPE_FILE_END);
        }
        msgRcd.datasize = htonl(nBytes);
        totalBytes += nBytes;
        
        // we has nBytes in the buffer, send it out
        fprintf(stderr, "ncp:sendFileData: rudpSend() (%d) bytes, sequence (%d) to server\n",
            nBytes, seq);
        nBytes = rudpSend(sockfd, &msgRcd);
        if (nBytes == (-1)){
            fprintf(stderr, "ERROR: rudpSend() cannot send data to server\n");
            exit(1);
        }
        pdataRecord = rudpRecvfrom(sockfd, &msgRcd, &addr_rec);
        fprintf(stderr, "ncp:sendFileData: rudprecvfrom type %s, seq (%d)\n",
            getMessageType(pdataRecord->msg_type), pdataRecord->seq);
        if (pdataRecord->msg_type != MSG_TYPE_ACK){
            if (pdataRecord->seq != seq){
                fprintf(stderr, "ncp: ERROR exptect seq (%d) but receive (%d)\n",
                    seq, pdataRecord->seq);
                exit(1);
            }
        }
    }while(endOfFile != 1);

    fprintf(stderr, "ncp: send (%d) bytes to server\n", totalBytes);
    return (totalBytes);
}
