//
// Created by syang on 12/2/17.
//

#ifndef PEER_TASK_H
#define PEER_TASK_H

#include <stdint.h>
#include "sha.h"
#include "bt_parse.h"
#include "queue.h"
#include "rdt.h"

typedef struct chunk_s {
    int id;
    uint8_t sha1[SHA1_HASH_SIZE];
    int flag; // finished
    int current_size;
    int num;
    char *data;
    queue *peers;
} chunk_t;

typedef struct task_s {
    int chunk_num;
    int need_num;
    int max_conn;
    queue *chunks;
    char get_chunk_file[BT_FILENAME_LEN];
    down_pool_t *pool;
    struct timeval timer;
} task_t;

void init_task(task_t *task, const char*, int, int);

chunk_t *make_chunk(int, const uint8_t*);
void available_peer(task_t *task, uint8_t *sha1, bt_peer_t *peer);

void add_to_chunks(chunk_t **chunks, chunk_t *chunk, int num);

// task to do
void flood_get(task_t *task, int sock);


#endif //PEER_TASK_H
