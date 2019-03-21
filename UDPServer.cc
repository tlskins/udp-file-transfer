// server code for UDP socket programming
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <fcntl.h>
#include <strings.h>
#include "rudp.h"
#include "UDPServer.h"

#define IP_PROTOCOL 0
#define PORT_NO 15050
#define NET_BUF_SIZE 32
#define cipherKey 'S'
#define sendrecvflag 0
#define nofile "File Not Found!"

/* Globals */
server_params_t *serv_params;

/*
 * read_server_params
 *
 * Read the input parameters for the server from the file server.in
 */
static int
read_server_params (rudp_srv_state_t *rudp_srv_state)
{
    // int fd, val, lineno = 1;
    // char buf[MAXLINE];
    //
    // /* Open the input file */
    // fd = open(SERVER_INPUT, O_RDONLY);
    // if (!fd) {
    //     return -1;
    // }
    //
    // /* Read the parameters one line at a time */
    // bzero(serv_params, sizeof(server_params_t));
    // bzero(rudp_srv_state, sizeof(rudp_srv_state_t));
    // bzero(buf, MAXLINE);
    // while (readline(fd, buf, MAXLINE)) {
    //     val = atoi(buf);
    //     if (val == 0) {
    //         return -1;
    //     }
    //
    //     if (lineno == 1) {
    //         serv_params->port = val;
    //     } else if (lineno == 2) {
    //         rudp_srv_state->max_cwnd_size = val;
    //     }
    //     lineno++;
    //     bzero(buf, MAXLINE);
    // }

    serv_params->port = 9281;
    rudp_srv_state->max_cwnd_size = 1000;

    printf("\nSERVER PARAMS:\n");
    printf("port: %d\n", serv_params->port);
    printf("sending window size: %d\n", rudp_srv_state->max_cwnd_size);

    return 0;
}

/* Main entry point */
int
main (int argc, char *argv[])
{
    int ret;
    rudp_srv_state_t rudp_srv_state;

    /* Sanity check */
    if (argc != 1) {
        printf("usage: ./server\n");
        return -1;
    }

    /* Register for SIGCHLD */
    // signal(SIGCHLD, sigchild_handler);

    /* Initialize the structure for holding server parameters */
    serv_params = (server_params_t *)malloc(sizeof(server_params_t));
    if (!serv_params) {
        printf("main: failed to initialize server parameters\n");
        return -1;
    }

    /* Read the server parameters from server.in */
    ret = read_server_params(&rudp_srv_state);
    if (ret != 0) {
        printf("main: failed to read server parameters from server.in\n");
        return -1;
    }

    /* Initialize the RUDP library */
    ret = rudp_srv_init(&rudp_srv_state);
    if (ret != 0) {
        printf("main: failed to initialize the RUDP library\n");
        return -1;
    }

    /* Setup connections on all the interfaces */
    // init_listening_sockets();
    //
    // /* Monitor the created sockets using select */
    // conn_listen();

    return 0;
}
