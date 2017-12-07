//
// Created by syang on 2017/11/30.
//

#include "queue.h"
#include "task.h"
#include <netinet/in.h>

#ifndef PEER_TRANS_H
#define PEER_TRANS_H

queue *init_work();

// init the pkts to send
queue *init_whohas_queue(const char *);
queue *init_ihave_queue(queue *);
data_packet_t **init_data_array(uint8_t *sha);

/* save the chunks i have */
queue *init_chunk_file(const char *);
void init_has_chunks(const char *);
/* convert the data in pkt to the chunks queue */
int check_i_have(uint8_t *);
queue *which_i_have(queue *);
queue *data2chunks_queue(void *);

/* send pkts to special one*/
void send_pkts(int, struct sockaddr *, queue *);
/* send pkts to everyone */
void flood_whohas(int, queue *);
void send_data_packets(up_conn_t *, int, struct sockaddr *);
#endif //PEER_TRANS_H
