#ifndef _RUDP_H_
#define _RUDP_H_


extern int serverInit();
extern int serverListen(int sockfd);

extern int clientInit(char* serverName);
extern int sendFileName(int sockfd, char* localFileName, char* remoteFileName);

#endif
