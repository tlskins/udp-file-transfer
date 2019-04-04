#ifndef _RUDP_H_
#define _RUDP_H_

#include <stdio.h>

extern int serverInit();
extern int serverListen(int sockfd);

extern int clientInit(char* serverName);
extern FILE* sendFileName(int sockfd, char* localFileName, char* remoteFileName, int* fsize);
extern int receiveFileData(int sockfd, int fsize);
extern int sendFileData(int sockfd, FILE* fp);

#endif
