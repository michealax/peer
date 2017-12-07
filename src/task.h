//
// Created by syang on 12/2/17.
// this is the source code about down && up tasks
//

#ifndef PEER_TASK_H
#define PEER_TASK_H

#include <stdint.h>
#include "sha.h"
#include "bt_parse.h"
#include "queue.h"

#define WINDOW_SIZE 8
#define CHUNK_SIZE 512
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
void continue_task(task_t *task, down_pool_t *pool, int sock); // 继续task中的下载
task_t *finish_task(task_t *task);

/* 上传conn */
void init_up_pool(up_pool_t *pool, int max_conn);
up_conn_t *get_up_conn(up_pool_t *, bt_peer_t *);
up_conn_t *add_to_up_pool(up_pool_t *pool, bt_peer_t *peer, data_packet_t **pkts);
void remove_from_up_pool(up_pool_t *pool, bt_peer_t *peer);
up_conn_t *make_up_conn(bt_peer_t *peer, data_packet_t **pkts);

/* 下载coon */
void init_down_pool(down_pool_t *pool, int max_conn);
down_conn_t *get_down_conn(down_pool_t *, bt_peer_t *);
down_conn_t *add_to_down_pool(down_pool_t *pool, bt_peer_t *peer, chunk_t *chunk);
void remove_from_down_pool(down_pool_t *pool, bt_peer_t *peer);
down_conn_t *make_down_conn(bt_peer_t *peer, chunk_t *chunk);

/* 用于实现最稀缺块优先的函数 可能在pj中无用 */
char *find_chunk_data(task_t *task, uint8_t *sha1);
void add_to_chunks(chunk_t **chunks, chunk_t *chunk, int num);
void available_peer(task_t *task, uint8_t *sha1, bt_peer_t *peer);

// task to do
void flood_get(task_t *task, int sock);

chunk_t *choose_chunk(task_t *task, queue *chunks, bt_peer_t *peer);

#endif //PEER_TASK_H
