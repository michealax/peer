//
// Created by syang on 12/2/17.
// Merge on 12/6/17
//

#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "packet.h"
#include "task.h"
#include "chunk.h"
#include "timer.h"
#include "trans.h"

/* up_conn */
void init_up_pool(up_pool_t *pool, int max_conn) {
    pool->max_conn = max_conn;
    pool->conn = 0;
    pool->conns = malloc(max_conn * sizeof(up_conn_t *));
    memset(pool->conns, 0, max_conn * sizeof(up_conn_t *));
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
            break;
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

/* down_conn函数 */
void init_down_pool(down_pool_t *pool, int max_conn) {
    pool->max_conn = max_conn;
    pool->conn = 0;
    pool->conns = malloc(max_conn* sizeof(down_conn_t *));
    memset(pool->conns, 0, max_conn* sizeof(down_conn_t *));
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

/* chunk函数 */
chunk_t *make_chunk(int id, const uint8_t *sha1) {
    chunk_t *chunk = malloc(sizeof(chunk_t));
    chunk->num = 0;
    chunk->flag = 0;
    chunk->inuse = 0;
    chunk->id = id;
    memcpy(chunk->sha1, sha1, SHA1_HASH_SIZE);
    chunk->data = malloc(BT_CHUNK_SIZE);
    chunk->peers = make_queue();
    return chunk;
}

void free_chunk(chunk_t *chunk) {
    free_queue(chunk->peers, 0);
    free(chunk->data);
    free(chunk);
}

task_t *init_task(const char *output_file, const char *chunk_file, int max_conn) {
    task_t *task = malloc(sizeof(task_t));
    strcpy(task->output_file, output_file);
    task->max_conn = max_conn;
    task->chunks = make_queue();
    // init the chunks
    FILE *fp = fopen(chunk_file, "r");
    char read_buf[64];
    char sha_buf[2 * SHA1_HASH_SIZE];
    int i = -1;
    chunk_t *chunk;
    uint8_t sha1[SHA1_HASH_SIZE];
    while (fgets(read_buf, 64, fp) != NULL) {
        sscanf(read_buf, "%d %s", &i, sha_buf);
        hex2binary(sha_buf, 2 * SHA1_HASH_SIZE, sha1);
        chunk = make_chunk(i, sha1);
        enqueue(task->chunks, chunk);
    }
    task->chunk_num = 0;
    task->need_num = i+1;
    fclose(fp);
    return task;
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

char *find_chunk_data(task_t *task, uint8_t *sha1) {
    chunk_t *chunk;
    for(node *cur=task->chunks->head; cur!=NULL; cur=cur->next){
        chunk = cur->data;
        if(memcmp(chunk->sha1, sha1, SHA1_HASH_SIZE)==0){
            return chunk->data;
        }
    }
    return NULL;
}

void flood_get(task_t *task, int sock, down_pool_t *pool){
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
            free_packet(pkt);// 连接创建
            down_conn_t *down_conn = add_to_down_pool(pool, peers[i], chunks[i]);
            timer_start(&down_conn->timer);
        }
    }
}

task_t *finish_task(task_t *task) {
    chunk_t *chunk;
    FILE *fp = fopen(task->output_file, "wb+"); // 内存映射方式直接读写存在一定问题
    for(uint j = 0; j < task->need_num; j++) {
        chunk = dequeue(task->chunks);
        fwrite(chunk->data, 512, 1024, fp);
    }
    fclose(fp);
    free_queue(task->chunks, 0);
    free(task);
    return NULL;
}

int check_task(task_t *task){
    int flag = 1;
    chunk_t *chunk;
    uint8_t sha1[SHA1_HASH_SIZE];
    for(node *cur=task->chunks->head; cur!=NULL; cur=cur->next) {
        chunk=cur->data;
        shahash((uint8_t *)chunk->data, BT_CHUNK_SIZE, sha1);
        if (memcmp(sha1, chunk->sha1, SHA1_HASH_SIZE)!=0) {
            flag=0;
            chunk->flag = 0;
            chunk->inuse = 0;
        }
    }
    return flag;
}

chunk_t *choose_chunk(task_t *task, queue *chunks, bt_peer_t *peer){
    uint8_t *sha1;
    chunk_t *chunk;
    chunk_t *ret=NULL;
    while ((sha1 = dequeue(chunks)) != NULL) {
        for (node *cur = task->chunks->head; cur!=NULL; cur=cur->next) {
            chunk = cur->data;
            if(!chunk->inuse && !chunk->flag && memcmp(sha1, chunk->sha1, SHA1_HASH_SIZE) == 0){
                if (ret==NULL) {
                    ret = chunk;
                }
                enqueue(chunk->peers, peer);
                break;
            }
        }
    }
    return ret;
}

void continue_task(task_t *task, down_pool_t *pool, int sock) {
    chunk_t *chunk=NULL;
    bt_peer_t *peer=NULL;
    int flag = 0;
    for(node *cur=task->chunks->head; cur!=NULL; cur=cur->next) {
        chunk = cur->data;
        if (!chunk->flag && !chunk->inuse) { // 查找chunk是否需要下载&&是否正在被下载
            for(node *n=chunk->peers->head; n!=NULL; n=n->next) {
                peer=n->data;
                if(get_down_conn(pool, peer)==NULL) { // 查找节点是否可用
                    flag = 1;
                    break;
                }
            }
        }
        if (flag) {
            break;
        }
    }
    if (flag) {
        add_to_down_pool(pool, peer, chunk);
        chunk->inuse = 1;
        data_packet_t *pkt = make_get_packet(SHA1_HASH_SIZE + HEADERLEN, (char *) chunk->sha1);
        send_packet(sock, pkt, (struct sockaddr *)&peer->addr);
        free_packet(pkt);
    }
}

int remove_stalled_chunks(down_pool_t *pool) { // 检查所有chunk是否处于停滞状态
    struct timeval now;
    gettimeofday(&now, NULL); // 获得时间
    chunk_t *chunk;
    int ret = 0;
    for (int i = 0; i < pool->max_conn; i++) {
        if (pool->conns[i]!=NULL) {
            chunk=pool->conns[i]->chunk;
            if (chunk->inuse&&timer_now(&pool->conns[i]->timer, &now) > 30000) {
                chunk->inuse = 0;
                remove_from_down_pool(pool, pool->conns[i]->sender);
                ret = 1;
            }
        }
    }
    return ret;
}

void remove_unack_peers(up_pool_t *pool, int sock) {
    struct timeval now;
    gettimeofday(&now, NULL); // 获得时间
    for (int i = 0; i < pool->max_conn; i++) {
        if (pool->conns[i]!=NULL) {
            if (timer_now(&pool->conns[i]->timer, &now) > 30000) {
                remove_from_up_pool(pool, pool->conns[i]->receiver);
            } else if (timer_now(&pool->conns[i]->timer, &now) > 3000) {
                send_data_packets(pool->conns[i], sock, (struct sockaddr *)&pool->conns[i]->receiver->addr);
            }
        }
    }
}


void print_data(char *data, int size) {
    for (int i = 0; i < size; i++) {
        fprintf(stderr, "%02x", data[i]);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}