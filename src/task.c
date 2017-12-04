//
// Created by syang on 12/2/17.
//

#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "packet.h"
#include "task.h"
#include "chunk.h"

chunk_t *make_chunk(int id, const uint8_t *sha1) {
    chunk_t *chunk = malloc(sizeof(chunk_t));
    chunk->num = 0;
    chunk->flag = 0;
    chunk->id = id;
    memcpy(chunk->sha1, sha1, SHA1_HASH_SIZE);
    chunk->data = malloc(BT_CHUNK_SIZE);
    return chunk;
}

void free_chunk(chunk_t *chunk) {
    free_queue(chunk->peers, 0);
    free(chunk->data);
    free(chunk);
}

void init_task(task_t *task, const char*chunk_file, int n, int max_conn) {
    task = malloc(sizeof(task_t));
    strcpy(task->get_chunk_file, chunk_file);
    task->chunk_num = n;
    task->need_num = n;
    task->max_conn = max_conn;
    task->chunks = make_queue();
    // init the chunks
    FILE *fp = fopen(chunk_file, "r");
    char read_buf[64];
    char sha_buf[2 * SHA1_HASH_SIZE];
    int i;
    chunk_t *chunk;
    uint8_t sha1[SHA1_HASH_SIZE];
    while (fgets(read_buf, 64, fp) != NULL) {
        sscanf(read_buf, "%d %s", &i, sha_buf);
        hex2binary(sha_buf, 2 * SHA1_HASH_SIZE, sha1);
        chunk = make_chunk(i, sha1);
        enqueue(task->chunks, chunk);
    }
    fclose(fp);
}

void available_peer(task_t *task, uint8_t *sha1, bt_peer_t *peer){
    assert(task != NULL);
    chunk_t *chunk;
    for(node *current=task->chunks->head; current!=NULL; current=current->next) {
        chunk = (chunk_t *)current->data;
        if(memcmp(sha1, chunk->sha1, SHA1_HASH_SIZE) == 0){
            chunk->num+=1;
            if(chunk->peers == NULL) {
                chunk->peers = make_queue();
            }
            if(!check_peers(chunk->peers, peer)) {
                enqueue(chunk->peers, peer);
            }
            return;
        }
    }
}

void add_to_chunks(chunk_t **chunks, chunk_t *chunk, int num){
    for(int i = 0; i < num; i++) {
        if(chunks[i] == NULL) {
            chunks[i] = chunk;
        } else {
            if (chunk->num < chunks[i]->num) {
                for(int j=i; j<num-1; j++){
                    chunks[j+1]=chunks[j];
                }
                chunks[i] = chunk;
            }
        }
    }
}

void flood_get(task_t *task, int sock){
    chunk_t *chunks[task->max_conn];
    for(node *cur=task->chunks->head; cur!=NULL; cur=cur->next) {
        chunk_t *chunk = cur->data;
        if(chunk->flag){
            continue;
        } else {
            add_to_chunks(chunks, chunk, task->max_conn);
        }
    }
    bt_peer_t *peers[task->max_conn];
    int true_num = 0;
    for(int i = 0; i < task->max_conn; i++) {
        for(node *cur = chunks[i]->peers->head; cur != NULL; cur=cur->next) {
            int flag = 0;
            bt_peer_t *peer = cur->data;
            for(int j=0; j<i; j++) {
                if (peers[j]->id == peer->id) {
                    flag = 1;
                    break;
                }
            }
            if(!flag) {
                peers[i] = peer;
                true_num ^= (1<<i);
                break;
            }
        }
    }
    for(int i=0; i<task->max_conn; i++){
        int flag = true_num&(1<<i);
        if (flag) {
            data_packet_t *pkt = make_get_packet(SHA1_HASH_SIZE+HEADERLEN, (char *)chunks[i]->sha1);
            send_packet(sock, pkt, (struct sockaddr *)&peers[i]->addr);
        }
    }
}

void finish_task(task_t *task) {
    chunk_t *chunk;
    // map the file to memory
    // store the whole data to the file
    int data_fd = open(task->get_chunk_file, O_WRONLY);
    char *file = mmap(0, (size_t)task->need_num*BT_CHUNK_SIZE, PROT_READ, MAP_SHARED, data_fd, 0);
    close(data_fd);
    for(uint j = 0; j < task->need_num; j++){
        char *data = file+j*BT_CHUNK_SIZE;
        chunk=dequeue(task->chunks);
        memcpy(data, chunk->data, BT_CHUNK_SIZE);
        free_chunk(chunk);
    }
    munmap(file, (size_t)task->need_num*BT_CHUNK_SIZE); // munmap the memory
    free_queue(task->chunks, 0);
    free(task);
    task = NULL;
}