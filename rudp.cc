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
#define MSG_TYPE_FILE_START             4
#define MSG_TYPE_FILE_STOP              5
#define MSG_TYPE_ACK                    6
#define MSG_TYPE_INVALID                7

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

typedef struct _ClientList{
    struct sockaddr_in* client;
    int                 fsize;
    char*               fileName;
    struct _ClientList* next;
} ClientList;

char                fileName[databuffersize];
static ClientList*  g_ClientList = NULL;

// same client return 1, otherwise save it and return 0
int sameClient(struct sockaddr_in* paddr_con, int fsize, char* fileName)
{
    ClientList*     p;
    int             ret;

    if (g_ClientList == NULL){
        // this is the first client coming
        g_ClientList = (ClientList*) malloc(sizeof(ClientList));
        memset(g_ClientList, 0, sizeof(ClientList));
        g_ClientList->client = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
        memcpy(g_ClientList->client, paddr_con, sizeof(struct sockaddr_in));
        g_ClientList->fsize = fsize;
        g_ClientList->fileName = (char*) malloc(sizeof(strlen(fileName) + 1));
        strcpy(g_ClientList->fileName, fileName);
        return (1);
    }
    ret = memcmp(g_ClientList->client, paddr_con, sizeof(struct sockaddr_in));
    if (ret == 0){      // same client, return 1
        return (1);
    }
    // this is a new client so save it to the end of list
    p = g_ClientList;
    while (p != NULL){
        ret = memcmp(p->client, paddr_con, sizeof(struct sockaddr_in));
        if (ret == 0){          // same client, we already saved, so just return 0
            return (0);
        }else{  // check next item in the list
            if (p->next != NULL){   // more items, then continue
                p = p->next;
            }else{ // got end of list, so add this client to the end of list
                p->next = (ClientList*) malloc(sizeof(ClientList));
                memset(p->next, 0, sizeof(ClientList));
                p->next->client = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
                memcpy(p->next->client, paddr_con, sizeof(struct sockaddr_in));
                p->next->fsize = fsize;
                p->next->fileName = (char*) malloc(sizeof(strlen(fileName) + 1));
                strcpy(p->next->fileName, fileName);
                return (0);
            }
        }
    }
    return (0);
}

ClientList* getSavedClient()
{
    ClientList*     p = g_ClientList;
    if (p != NULL){
        if (p->next != NULL){
            g_ClientList = p->next;
        }else{
            g_ClientList = NULL;
        }
        return (p);
    }else{
        return (NULL);
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
    case MSG_TYPE_FILE_END:
        return ("MSG_TYPE_FILE_END");
        break;
    case MSG_TYPE_FILE_START:
        return ("MSG_TYPE_FILE_START");
        break;
    case MSG_TYPE_FILE_STOP:
        return ("MSG_TYPE_FILE_STOP");
        break;
    case MSG_TYPE_ACK:
        return ("MSG_TYPE_ACK");
        break;
    default:
        return ("MSG_TYPE_INVALID");
    }
}
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


DataRecord* rudpRecvfrom(int sockfd, MessageRecord* pmsgRcd, struct sockaddr_in* paddr_rev)
{
    int                 nBytes;
    socklen_t           addrlen = sizeof(struct sockaddr_in);
    DataRecord*         pDataRecord = NULL;

    fprintf(stderr, "%s:%d:rudpRecvfrom: calling recvfrom() ...\n", __FILE__, __LINE__);
    nBytes = recvfrom(sockfd, pmsgRcd, sizeof(MessageRecord), sendrecvflag,
        (sockaddr*) paddr_rev, &addrlen);
    if (nBytes == (-1)){
        fprintf(stderr, "%s:%d:rudpRecvfrom: ERROR: recvfrom() return (%d}\n",
        __FILE__, __LINE__, errno);
        exit(0);
    }
    pDataRecord = (DataRecord*) malloc(sizeof(DataRecord));
    convertfrom(pmsgRcd, paddr_rev, pDataRecord);
    fprintf(stderr, "%s:%d:rudpRecvfrom: type %s, receive %d bytes, sequence (%d) "
        "from '%s' port %d\n", __FILE__, __LINE__,
            getMessageType(pDataRecord->msg_type),
            pDataRecord->datasize, pDataRecord->seq,
            pDataRecord->ip, pDataRecord->port);
    return (pDataRecord);
}

// return -1 if error
int rudpSend(int sockfd, MessageRecord* pmsgRcd)
{
    int                 nBytes;
    
    fprintf(stderr, "%s:%d: calling send() to send\n", __FILE__, __LINE__);
    nBytes = send(sockfd, pmsgRcd, sizeof(MessageRecord), sendrecvflag);
    if (nBytes == (-1)){
        fprintf(stderr, "%s:%d:ERROR: cannot send\n", __FILE__, __LINE__);
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

    fprintf(stderr, "%s:%d: calling sendto() to send to '%s', port %d\n",
        __FILE__, __LINE__,
        buf, ntohs(sockaddr_local.sin_port));
    nBytes = sendto(sockfd, pmsgRcd, sizeof(MessageRecord), sendrecvflag,
                (struct sockaddr*) &sockaddr_local, addrlen);
    if (nBytes == (-1)){
        fprintf(stderr, "%s:%d:ERROR: cannot send\n", __FILE__, __LINE__);
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
    uint32_t            seq = 1;
    

    // use receiveListen client information saved at 
    while(fsize > 0){
        fprintf(stderr, "%s:%d:receiveFileData: calling rudpRecvfrom() ...\n",
            __FILE__, __LINE__);
        pdataRecord = rudpRecvfrom(sockfd, &msgRcd, &addr_con);
        if (! (pdataRecord->seq == seq && 
            (pdataRecord->msg_type == MSG_TYPE_FILE_DATA ||
             pdataRecord->msg_type == MSG_TYPE_FILE_END))){
            fprintf(stderr, "%s:%d:receiveFileData:ERROR: suppose to to receive seq (%d) "
                " but received seq (%d)\n", __FILE__, __LINE__, seq, pdataRecord->seq);
            if (pdataRecord->msg_type == MSG_TYPE_FILENAME){  // another client coming
                sameClient(&addr_con, pdataRecord->seq, (char*) pdataRecord->data);
                continue;           // continue retrieve message
            }else{
                exit(1);
            }
        }
        fsize -= pdataRecord->datasize;
        if (fp == NULL){
            fp = fopen(fileName, "w");
            if (fp == NULL){
                fprintf(stderr, "%s:%d:receiveFileData: cannot open file '%s' for write\n",
                    __FILE__, __LINE__, fileName);
                exit (1);
            }
        }
        nBytes = fwrite(pdataRecord->data, 1, pdataRecord->datasize, fp);
        if (nBytes != pdataRecord->datasize){
            fprintf(stderr, "%s:%d:receiveFileData: suppose to write %d bytes but "
                " written %d bytes\n", __FILE__, __LINE__, pdataRecord->datasize, nBytes);
            exit(1);
        }

        // send ack back to client
        memset(&msgRcd, 0, sizeof(MessageRecord));
        msgRcd.msg_type = htonl(MSG_TYPE_ACK);
        msgRcd.seq = htonl(pdataRecord->seq);
        fprintf(stderr, "%s:%d:receiveFileData: calling rudpSendTo() to send ACK for seq (%d)\n",
            __FILE__, __LINE__, pdataRecord->seq);
        rudpSendTo(sockfd, &msgRcd, &addr_con);
        seq++;
    }

    // remove the saved client information
    getSavedClient();
    if (fp != NULL){
        fclose(fp);
        fp = NULL;
    }
    fprintf(stderr, "%s:%d: receive whole file\n", __FILE__, __LINE__);
    return (fsize);
}

int rudpSendFileStartStop(MessageRecord* pmsgRcd, int msg_type, int sockfd, struct sockaddr_in* paddr_con)
{
    memset(pmsgRcd, 0, sizeof(MessageRecord));
    pmsgRcd->msg_type = htonl(msg_type);
    fprintf(stderr, "%s:%d:rudpSendFileStartStop: calling rudpSendTo() to send (%s) ...\n",
        __FILE__, __LINE__, getMessageType(msg_type));
    return rudpSendTo(sockfd, pmsgRcd, paddr_con);
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
    ClientList*         pClient;

    // handle the linked-list client first

    pClient = getSavedClient();
    if (pClient != NULL){
        // process saved client
        memset(&msgRcd, 0, sizeof(MessageRecord));
        memcpy(&addr_con, pClient->client, sizeof(struct sockaddr_in));
        fsize = pClient->fsize;
        strcpy(fileName, pClient->fileName);
        inet_ntop(AF_INET, &addr_con.sin_addr, buf, INET_ADDRSTRLEN),
        printf("%s:%d:serverListen: processing saved client "
            "file name: %s, size %d from %s port %d\n",
               __FILE__, __LINE__, fileName, fsize, buf, ntohs(addr_con.sin_port));
        rudpSendFileStartStop(&msgRcd, MSG_TYPE_FILE_START, sockfd, &addr_con);
        return (fsize);
    }
    do{
        fprintf(stderr, "%s:%d:serverListen: calling recvfrom() ...\n", __FILE__, __LINE__);
        nBytes = recvfrom(sockfd, &msgRcd, sizeof(MessageRecord), sendrecvflag,
            (struct sockaddr*) &addr_con, &addrlen);
        if (nBytes == (-1)){
            fprintf(stderr, "%s:%d: ERROR: recvfrom() return (%d}\n",
                __FILE__, __LINE__, errno);
            exit(1);
        }
        msg_type = ntohl(msgRcd.msg_type);
    } while(msg_type != MSG_TYPE_FILENAME);

    // save file size and name
    fsize = ntohl(msgRcd.seq);
    strcpy(fileName, (char*) msgRcd.data);
    inet_ntop(AF_INET, &addr_con.sin_addr, buf, INET_ADDRSTRLEN),
    printf("%s:%d:serverListen: Receive %d bytes, file name: %s, size %d from %s port %d\n",
           __FILE__, __LINE__,
           nBytes, fileName, fsize, buf, ntohs(addr_con.sin_port));

    if (sameClient(&addr_con, fsize, fileName) == 1){
        // we deal with the same client, send FILE_START back to client
        rudpSendFileStartStop(&msgRcd, MSG_TYPE_FILE_START, sockfd, &addr_con);
    }else{
        // a new client, send FILE_STOP back to client
        rudpSendFileStartStop(&msgRcd, MSG_TYPE_FILE_STOP, sockfd, &addr_con);
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
        fprintf(stderr, "%s:%d:Error: Cannot create a socket\n", __FILE__, __LINE__);
        exit(1);
    }

    fprintf(stderr, "%s:%d: Created a file descriptor %d\n", __FILE__, __LINE__, sockfd);

    // zero curr_addr which is the curren client
    memset(fileName, 0, databuffersize);
    memset(&addr_con, 0, sizeof(struct sockaddr_in));
    addr_con.sin_family = AF_INET;
    addr_con.sin_port = htons(PORT_NO);
    addr_con.sin_addr.s_addr = INADDR_ANY; // bind to all intefaces

    bd = bind(sockfd, (struct sockaddr*)&addr_con, sizeof(struct sockaddr_in));
    if (bd == 0){
        fprintf(stderr, "%s:%d: Binded port '%d' Successfully\n",
        __FILE__, __LINE__, PORT_NO);
    }else{
        fprintf(stderr, "%s:%d:ERROR: Binding Failed (%d)!\n",
            __FILE__, __LINE__, bd);
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
    fprintf(stderr, "%s:%d: server '%s' ip address: %s\n",
        __FILE__, __LINE__, serverName, serverIpAddress);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0){
        fprintf(stderr, "%s:%d:Error: Cannot create a socket\n", __FILE__, __LINE__);
        exit(1);
    }

    fprintf(stderr, "%s:%d: Created a file descriptor %d\n",
        __FILE__, __LINE__, sockfd);

    memset(&addr_con, 0, sizeof(struct sockaddr_in));
    addr_con.sin_family = AF_INET;
    addr_con.sin_port = htons(0); // bind to ephemeral port
    // addr_con.sin_addr.s_addr = INADDR_ANY; // bind to all intefaces

    ret = bind(sockfd, (struct sockaddr*)&addr_con, sizeof(struct sockaddr_in));
    if (ret == 0){
        fprintf(stderr, "%s:%d: Binded Successfully\n", __FILE__, __LINE__);
    }else{
        fprintf(stderr, "%s:%d:ERROR: Binding Failed!\n", __FILE__, __LINE__);
        exit(-1);
    }

    // print out the client information
    socklen = sizeof(struct sockaddr);
    memset(&sock_addr, 0, sizeof(struct sockaddr_in));
    getsockname(sockfd, (struct sockaddr *) &sock_addr, &socklen);
    fprintf(stderr, "%s:%d: Client socket bound to %s ephemeral port %d\n",
        __FILE__, __LINE__,
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
        fprintf(stderr, "%s:%d:ERROR: failed to connect to server '%s', ip '%s', port '%d'\n",
            __FILE__, __LINE__,
            serverName, serverIpAddress, PORT_NO);
        exit(-1);
    }

    // print out the server information
    socklen = sizeof(struct sockaddr);
    memset(&sock_addr, 0, sizeof(struct sockaddr_in));
    getpeername(sockfd, (struct sockaddr *) &sock_addr, &socklen);
    fprintf(stderr, "%s:%d: Client socket connect to server '%s', ip '%s', port %d\n",
        __FILE__, __LINE__, serverName,
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
    struct sockaddr_in  addr_rec;
    DataRecord*         pDataRecord = NULL;
    
    fp = fopen(localFileName, "r");
    if (fp == NULL){
        fprintf(stderr, "%s:%d:sendFileName:ERROR: local file name '%s' doesn't exist\n",
            __FILE__, __LINE__, localFileName);
        exit (1);
    }
    ret = stat(localFileName, &statbuf);
    if (ret != 0){
        fprintf(stderr, "%s:%d::sendFileName:ERROR: Cannot get file '%s' stat (%d)\n",
            __FILE__, __LINE__, localFileName, errno);
        exit(1);
    }
    nBytes = statbuf.st_size;
    *fsize = nBytes;
    fprintf(stderr, "%s:%d:sendFileName: file '%s' size '%d'\n",
        __FILE__, __LINE__, localFileName, nBytes);

    // pass the message type, remote file name and file size (in seq) to server
    memset(&msgRcd, 0, sizeof(MessageRecord));
    msgRcd.msg_type = htonl(MSG_TYPE_FILENAME);
    msgRcd.seq = htonl(nBytes);
    strcpy((char*) msgRcd.data, remoteFileName);
    // pass remote file name to server with send()
    fprintf(stderr, "%s:%d:sendFileName: calling send() to send file name '%s' information\n",
        __FILE__, __LINE__, remoteFileName);
    nBytes = rudpSend(sockfd, &msgRcd);
    if (nBytes == (-1)){
        fprintf(stderr, "%s:%d:sendFileName:ERROR: cannot send file name '%s' to server\n",
            __FILE__, __LINE__, remoteFileName);
        exit(1);
    }

    // wait for MSG_TYPE_FILE_START
    do{
        // this is client so we don't care who sent data to us, ignore addr_rec
        fprintf(stderr, "%s:%d:sendFileName: entering rudpRecvfrom() ...\n",
            __FILE__, __LINE__);
        pDataRecord = rudpRecvfrom(sockfd, &msgRcd, &addr_rec);
        fprintf(stderr, "%s:%d:sendFileName: rudprecvfrom type %s, seq (%d)\n",
            __FILE__, __LINE__,
            getMessageType(pDataRecord->msg_type), pDataRecord->seq);
    }while (pDataRecord->msg_type != MSG_TYPE_FILE_START);
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
                fprintf(stderr, "%s:%d: ERROR read file, buffer size (%d), reaed (%d)\n",
                    __FILE__, __LINE__,
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
        fprintf(stderr, "%s:%d:sendFileData: rudpSend() (%d) bytes, sequence (%d) to server\n",
            __FILE__, __LINE__, nBytes, seq);
        nBytes = rudpSend(sockfd, &msgRcd);
        if (nBytes == (-1)){
            fprintf(stderr, "%s:%d:ERROR: rudpSend() cannot send data to server\n",
                __FILE__, __LINE__);
            exit(1);
        }
        pdataRecord = rudpRecvfrom(sockfd, &msgRcd, &addr_rec);
        fprintf(stderr, "%s:%d:sendFileData: rudprecvfrom type %s, seq (%d)\n",
            __FILE__, __LINE__,
            getMessageType(pdataRecord->msg_type), pdataRecord->seq);
        if (pdataRecord->msg_type != MSG_TYPE_ACK){
            if (pdataRecord->seq != seq){
                fprintf(stderr, "%s:%d: ERROR exptect seq (%d) but receive (%d)\n",
                    __FILE__, __LINE__, seq, pdataRecord->seq);
                exit(1);
            }
        }
    }while(endOfFile != 1);

    fprintf(stderr, "%s:%d: send (%d) bytes to server\n", __FILE__, __LINE__, totalBytes);
    return (totalBytes);
}
