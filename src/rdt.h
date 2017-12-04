//
// Created by syang on 12/2/17.
// A rdt based on TCP
//

#ifndef PEER_RDT_H
#define PEER_RDT_H

#include "queue.h"
#include "bt_parse.h"

#define WINDOW_SIZE 8
#define CHUNK_SIZE 512 // every chunk is 512K

typedef struct down_conn_s {
    uint next_ack;
    struct timeval timer;
    bt_peer_t *sender;
} down_conn_t;

typedef struct up_conn_s {
    int last_ack;
    int last_sent;
    int last_available;
    queue *pkts;
    bt_peer_t *receiver;
} up_conn_t;

typedef struct down_pool_s {
    queue *pool;
    int conn;
    int max_conn;
} down_pool_t;

typedef struct up_pool_s {
    queue *pool;
    int conn;
    int max_conn;
} up_pool_t;

up_conn_t *get_up_conn(up_pool_t *, bt_peer_t *);
up_conn_t *add_to_up_pool(up_pool_t *pool, bt_peer_t *peer, queue *pkts);

up_conn_t *make_up_conn(bt_peer_t *peer, queue *pkts);

down_conn_t *get_down_conn(down_pool_t *, bt_peer_t *);

#endif //PEER_RDT_H
