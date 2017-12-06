//
// Created by syang on 12/2/17.
// A rdt based on TCP
//

#ifndef PEER_RDT_H
#define PEER_RDT_H

#include "queue.h"
#include "bt_parse.h"
#include "packet.h"
#include "task.h"

#define WINDOW_SIZE 8
#define CHUNK_SIZE 512 // every chunk is 512K

typedef struct down_conn_s {
    uint next_ack;
    chunk_t *chunk;
    int pos;
    struct timeval timer;
    bt_peer_t *sender;
} down_conn_t;

typedef struct up_conn_s {
    int last_ack;
    int last_sent;
    int available;
    int duplicate; // if the duplicate is more than 3, we should send the packet!
    int rwnd; // 流量控制 控制相应窗口
    data_packet_t **pkts;
    bt_peer_t *receiver;
} up_conn_t;

typedef struct down_pool_s {
    down_conn_t **conns; // more easier
    int conn;
    int max_conn;
} down_pool_t;

typedef struct up_pool_s {
    up_conn_t **conns;
    int conn;
    int max_conn;
} up_pool_t;

void init_up_pool(up_pool_t *pool, int max_conn);
up_conn_t *get_up_conn(up_pool_t *, bt_peer_t *);
up_conn_t *add_to_up_pool(up_pool_t *pool, bt_peer_t *peer, data_packet_t **pkts);
void remove_from_up_pool(up_pool_t *pool, bt_peer_t *peer);
up_conn_t *make_up_conn(bt_peer_t *peer, data_packet_t **pkts);

void init_down_pool(down_pool_t *pool, int max_conn);
down_conn_t *get_down_conn(down_pool_t *, bt_peer_t *);
down_conn_t *add_to_down_pool(down_pool_t *pool, bt_peer_t *peer, chunk_t *chunk);
void remove_from_down_pool(down_pool_t *pool, bt_peer_t *peer);
down_conn_t *make_down_conn(bt_peer_t *peer, chunk_t *chunk);


#endif //PEER_RDT_H
