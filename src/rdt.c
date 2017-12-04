//
// Created by syang on 12/2/17.
//
#include <stdio.h>
#include <malloc.h>
#include "rdt.h"
up_conn_t *make_up_conn(bt_peer_t *peer, queue *pkts) {
    up_conn_t *conn = malloc(sizeof(up_conn_t));
    conn->receiver = peer;
    conn->pkts = pkts;
    conn->last_ack = 0;
    conn->last_sent = 0;
    conn->last_available = 1;
    return conn;
}

up_conn_t *get_up_conn(up_pool_t *pool, bt_peer_t *peer) {
    if (pool->pool==NULL) return NULL;
    for(node *cur = pool->pool->head; cur!=NULL; cur = cur->next) {
        up_conn_t *conn = cur->data;
        if (conn->receiver->id == peer->id) {
            return conn;
        }
    }
    return NULL;
}

up_conn_t *add_to_up_pool(up_pool_t *pool, bt_peer_t *peer, queue *pkts){
    up_conn_t *conn = make_up_conn(peer, pkts);
    enqueue(pool->pool, conn);
    pool->conn++;
    return conn;
}

down_conn_t *get_down_conn(down_pool_t *pool, bt_peer_t *peer) {
    return NULL;
}