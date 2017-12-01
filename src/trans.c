//
// Created by syang on 2017/11/30.
//
#include <stdio.h>
#include <malloc.h>
#include "chunk.h"
#include "packet.h"
#include "trans.h"
#include "bt_parse.h"

extern bt_config_t config;
extern queue *has_chunks;

queue *init_work() {
    return NULL;
}

queue *init_whohas_queue(char *chunkfile) {
    FILE *fp = fopen(chunkfile, "r");
    char read_buf[64];
    char sha_buf[2 * SHA1_HASH_SIZE];
    queue *chunks = make_queue();
    uint8_t *chunk;
    int i;
    while (fgets(read_buf, 64, fp) != NULL) {
        sscanf(read_buf, "%d %s", &i, sha_buf);
        chunk = malloc(SHA1_HASH_SIZE);
        hex2binary(sha_buf, 2 * SHA1_HASH_SIZE, chunk);
        enqueue(chunks, chunk);
    }
    fclose(fp);
    queue *pkts = make_queue();
    while (chunks->n) {
        size_t pkt_num = chunks->n < MAX_CHANK_NUM ? chunks->n : MAX_CHANK_NUM;
        size_t data_len = pkt_num * SHA1_HASH_SIZE + 4;
        char *data_t = malloc(data_len);
        *(int *) data_t = pkt_num;
        i = 0;
        while ((chunk = dequeue(chunks)) != NULL) {
            memcpy(data_t + 4 + i * SHA1_HASH_SIZE, chunk, SHA1_HASH_SIZE);
            free(chunk);
            i++;
        }
        data_packet_t *pkt = make_whohas_packet((short) data_len, data_t);
        enqueue(pkts, pkt);
    }
    free_queue(chunks, 0); // no need to free recursively
    return pkts;
}


void init_has_chunks(const char *has_chunk_file) {
    FILE *fp = fopen(has_chunk_file, "r");
    has_chunks = make_queue();
    char read_buf[64];
    char sha_buf[2 * SHA1_HASH_SIZE];
    uint8_t *chunk;
    int i;
    while (fgets(read_buf, 64, fp) != NULL) {
        sscanf(read_buf, "%d %s", &i, sha_buf);
        chunk = malloc(SHA1_HASH_SIZE);
        hex2binary(sha_buf, 2 * SHA1_HASH_SIZE, chunk);
        enqueue(has_chunks, chunk);
    }
    fclose(fp);
}

int check_i_have(uint8_t *hash_wanted) {
    if (has_chunks == NULL) {
        init_has_chunks(config.has_chunk_file);
    }
    if (has_chunks->n == 0) {
        return -1;
    }
    node *current = has_chunks->head;
    uint8_t *hash_value;
    while (current != NULL) {
        hash_value = current->data;
        if (memcmp(hash_wanted, hash_value, SHA1_HASH_SIZE) != 0) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

queue *which_i_have(queue *wanted) {
    queue *result = make_queue();
    uint8_t *chunk;
    while ((chunk = dequeue(wanted)) != NULL) {
        int ret = check_i_have(chunk);
        if (ret == -1) {
            free(chunk);
            free_queue(wanted, 1);
            return NULL;
        } else if (ret == 1) {
            enqueue(result, chunk);
        }
        free(chunk);
    }
    // free(wanted); //as wanted is empty simply free it
    return result;
}

/* take method as an arg to simplify the code */
queue *init_chunk_packet_queue(queue *chunks, data_packet_t *(*make_chunk_packet)(short, void *)) {
    queue *pkts = make_queue();
    uint8_t *chunk;
    int i;
    while (chunks->n) {
        size_t pkt_num = (size_t) (chunks->n < MAX_CHANK_NUM ? chunks->n : MAX_CHANK_NUM);
        size_t data_len = pkt_num * SHA1_HASH_SIZE + 4;
        char *data_t = malloc(data_len);
        *(int *) data_t = pkt_num;
        i = 0;
        while ((chunk = dequeue(chunks)) != NULL) {
            memcpy(data_t + 4 + i * SHA1_HASH_SIZE, chunk, SHA1_HASH_SIZE);
            free(chunk);
            i++;
        }
        data_packet_t *pkt = make_chunk_packet((short) data_len, data_t);
        enqueue(pkts, pkt);
    }
    free_queue(chunks, 0); // no need to free recursively
    return pkts;
}

queue *init_ihave_queue(queue *chunks) {
    queue *pkts = make_queue();
    uint8_t *chunk;
    int i;
    while (chunks->n) {
        size_t pkt_num = (size_t) (chunks->n < MAX_CHANK_NUM ? chunks->n : MAX_CHANK_NUM);
        size_t data_len = pkt_num * SHA1_HASH_SIZE + 4;
        char *data_t = malloc(data_len);
        *(int *) data_t = pkt_num;
        i = 0;
        while ((chunk = dequeue(chunks)) != NULL) {
            memcpy(data_t + 4 + i * SHA1_HASH_SIZE, chunk, SHA1_HASH_SIZE);
            free(chunk);
            i++;
        }
        data_packet_t *pkt = make_ihave_packet((short) data_len, data_t);
        enqueue(pkts, pkt);
    }
    free_queue(chunks, 0); // no need to free recursively
    return pkts;
}

queue *data2chunks_queue(void *data) {
    queue *chunks = make_queue();
    int num = *(int *) data;
    char *ptr = data + 4;
    for (int i = 0; i < num; i++) {
        char *chunk = malloc(SHA1_HASH_SIZE);
        memcpy(chunk, ptr + i * 4, SHA1_HASH_SIZE);
        enqueue(chunks, chunk);
    }
    return chunks;
}

void flood_whohas(int sock, queue *pkts) {
    data_packet_t *pkt;
    while ((pkt = dequeue(pkts)) != NULL) { // can use the send_pkts() because we can't free the pkt here
        bt_peer_t *current = config.peers;
        while (current != NULL) {
            if (current->id != config.identity) {
                send_packet(sock, pkt, (struct sockaddr *) &current->addr);
            }
            current = current->next;
        }
        free_packet(pkt); // free the data of the node
    }
}

void send_pkts(int sock, struct sockaddr *dst, queue *pkts) {
    data_packet_t *pkt;
    while ((pkt = dequeue(pkts)) != NULL) {
        send_packet(sock, pkt, dst);
        free_packet(pkt); // free the data of the node
    }
}