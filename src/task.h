//
// Created by syang on 12/2/17.
//

#ifndef PEER_TASK_H
#define PEER_TASK_H

#include <stdint.h>
#include "sha.h"
#include "bt_parse.h"
#include "queue.h"

typedef struct chunk_s {
    int id;
    uint8_t sha1[SHA1_HASH_SIZE];
    int flag; // finished
    int inuse;
    int num;
    char *data;
    queue *peers;
} chunk_t;

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

typedef struct task_s {
    int chunk_num;
    int need_num;
    int max_conn;
    queue *chunks;
    char output_file[BT_FILENAME_LEN];
    struct timeval timer;
} task_t;

/* chunk函数 */
chunk_t *make_chunk(int, const uint8_t*);
void free_chunk(chunk_t *chunk);

/* task函数 */
task_t *init_task(const char *, const char *, int);
task_t *finish_task(task_t *task);


void add_to_chunks(chunk_t **chunks, chunk_t *chunk, int num);

void available_peer(task_t *task, uint8_t *sha1, bt_peer_t *peer);


char *find_chunk_data(task_t *task, uint8_t *sha1);
// task to do
void flood_get(task_t *task, int sock);

chunk_t *choose_chunk(task_t *task, queue *chunks);
void continue_task(task_t *task, down_pool_t *pool, int sock);

#endif //PEER_TASK_H
