/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>
#include <setjmp.h>
#include <sys/time.h>
#include <strings.h>
#include "rudp.h"

/* Globals */
rudp_srv_state_t *srv_state;
rudp_cli_state_t *cli_state;
// static struct rtt_info rttinfo;
// static int rttinit = 0;
// static struct msghdr msgsend, msgrecv;
// static sigjmp_buf jmpbuf;

/* Header prepended to each UDP datagram - For reliability */
// static struct hdr {
//     uint32_t msg_type;
//     uint32_t seq;
//     uint32_t ts;
//     uint32_t window_size;
// } sendhdr, recvhdr;

/* Defines */
// #define FILE_PAYLOAD_SIZE       (RUDP_PAYLOAD_SIZE - sizeof(struct hdr))
// #define GET_MIN(A, B) ((A) < (B) ? (A) : (B))

/*
 * rudp_srv_init
 *
 * Initializes the RUDP layer for the server
 */
int
rudp_srv_init (rudp_srv_state_t *state)
{
    int i;

    /* Initialize the parameters based on what is sent by the server */
    srv_state = (rudp_srv_state_t *)malloc(sizeof(rudp_srv_state_t));
    if (!srv_state) {
        return -1;
    }

    bzero(srv_state, sizeof(rudp_srv_state_t));

    /* Initialize the state parameters */
    srv_state->max_cwnd_size = state->max_cwnd_size;
    // srv_state->cwnd_size = 1; /* CW starts with size 1 in slow start phase */
    // srv_state->cwnd_free = srv_state->cwnd_size;
    // srv_state->ss_thresh = RUDP_DEFAULT_SSTHRESH;
    // srv_state->cwnd_start = 0;
    // srv_state->cwnd_end = 0;
    srv_state->expected_ack = 1; /* First packet sent has sequence number 0 */
    srv_state->num_acks = 0;
    srv_state->num_dup_acks = 0;
    srv_state->last_dup_ack = 0;
    // srv_state->rudp_state = CONGESTION_STATE_SLOW_START;

    /* Initialize the congestion window */
    srv_state->cwnd =
        (rudp_payload_t *)malloc(srv_state->max_cwnd_size *
                                 sizeof(rudp_payload_t));
    if (!srv_state->cwnd) {
        return -1;
    }
    bzero(srv_state->cwnd, srv_state->max_cwnd_size * sizeof(rudp_payload_t));

    for (i = 0; i < srv_state->max_cwnd_size; i++) {
        srv_state->cwnd[i].data = (uint8_t *)malloc(RUDP_PAYLOAD_SIZE);
        if (!srv_state->cwnd[i].data) {
            return -1;
        }
        bzero(srv_state->cwnd[i].data, RUDP_PAYLOAD_SIZE);
    }

    /* Register for SIGALRM to handle timeouts */
    // signal(SIGALRM, sigalarm_handler);

    return 0;
}
