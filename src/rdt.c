//
// Created by syang on 12/2/17.
//
#include <stdio.h>
#include <malloc.h>
#include "rdt.h"
#include "timer.h"

void init_up_pool(up_pool_t *pool, int max_conn) {
    pool->max_conn = max_conn;
    pool->conn = 0;
    pool->conns = malloc(max_conn* sizeof(up_conn_t *));
}

up_conn_t *make_up_conn(bt_peer_t *peer, data_packet_t **pkts) { // 用数组控制在发送数据时更为方便
    assert(pkts!=NULL);
    up_conn_t *conn = malloc(sizeof(up_conn_t));
    conn->receiver = peer;
    conn->rwnd = 8;
    conn->pkts = pkts;
    conn->last_ack = 0;
    conn->last_sent = 0;
    conn->available = conn->last_ack+conn->rwnd;
    conn->duplicate = 1;
    return conn;
}

up_conn_t *get_up_conn(up_pool_t *pool, bt_peer_t *peer) {
    up_conn_t **conns = pool->conns;
    for(int i = 0; i < pool->max_conn; i++) {
        if(conns[i]!=NULL && conns[i]->receiver->id==peer->id) {
            return conns[i];
        }
    }
    return NULL;
}

up_conn_t *add_to_up_pool(up_pool_t *pool, bt_peer_t *peer, data_packet_t **pkts){
    up_conn_t *conn = make_up_conn(peer, pkts);
    for(int i = 0; i < pool->max_conn; i++) {
        if(pool->conns[i] == NULL) {
            pool->conns[i] = conn;
        }
    }
    pool->conn++;
    return conn;
}

void remove_from_up_pool(up_pool_t *pool, bt_peer_t *peer) {
    up_conn_t **conns = pool->conns;
    for(int i = 0; i < pool->max_conn; i++) {
        if(conns[i]!=NULL && conns[i]->receiver->id==peer->id) {
            for(int j = 0; j<512; j++) {
                free(conns[i]->pkts[j]);
            }
            free(conns[i]->pkts); // since the queue is not use
            free(conns[i]);
            conns[i] = NULL;
            pool->conn--;
            break;
        }
    }
}

void init_down_pool(down_pool_t *pool, int max_conn) {
    pool->max_conn = max_conn;
    pool->conn = 0;
    pool->conns = malloc(max_conn* sizeof(down_conn_t *));
}

down_conn_t *make_down_conn(bt_peer_t *peer, chunk_t *chunk) {
    assert(chunk!=NULL);
    down_conn_t *conn = malloc(sizeof(down_conn_t));
    conn->next_ack = 1;
    conn->pos = 0;
    conn->chunk = chunk;
    conn->sender = peer;
    timer_start(&conn->timer);
    return conn;
}


down_conn_t *get_down_conn(down_pool_t *pool, bt_peer_t *peer) {
    down_conn_t **conns = pool->conns;
    for(int i = 0; i < pool->max_conn; i++) {
        if(conns[i]!=NULL && conns[i]->sender->id==peer->id) {
            return conns[i];
        }
    }
    return NULL;
}

down_conn_t *add_to_down_pool(down_pool_t *pool, bt_peer_t *peer, chunk_t *chunk) {
    down_conn_t *conn = make_down_conn(peer, chunk);
    for(int i = 0; i < pool->max_conn; i++) {
        if(pool->conns[i] == NULL) {
            pool->conns[i] = conn;
            break;
        }
    }
    pool->conn++;
    return conn;
}
void remove_from_down_pool(down_pool_t *pool, bt_peer_t *peer) {
    down_conn_t **conns = pool->conns;
    for(int i = 0; i < pool->max_conn; i++) {
        if(conns[i]!=NULL && conns[i]->sender->id==peer->id) {
            free(conns[i]);
            conns[i] = NULL; // data不需要free
            pool->conn--;
            break;
        }
    }
}

