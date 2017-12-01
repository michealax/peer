//
// Created by syang on 2017/11/30.
//
#include <stdio.h>
#include <malloc.h>
#include "chunk.h"
#include "packet.h"
#include "work.h"
#include "bt_parse.h"

extern bt_config_t config;

queue *init_whohas_queue(char *chunkfile) {
    FILE *fp = fopen(chunkfile, "r");
    char read_buf[64];
    char sha_buf[2*SHA1_HASH_SIZE];
    queue *chunks = make_queue();
    uint8_t *chunk;
    int i;
    while(fgets(read_buf, 64, fp) != NULL) {
        sscanf(read_buf, "%d %s", &i, sha_buf);
        chunk = malloc(SHA1_HASH_SIZE);
        hex2binary(sha_buf, SHA1_HASH_SIZE, chunk);
        enqueue(chunks, chunk);
    }
    fclose(fp);
    queue *pkts = make_queue();

    while (chunks->n) {
        size_t pkt_num = chunks->n<MAX_CHANK_NUM?chunks->n:MAX_CHANK_NUM;
        size_t data_len = pkt_num*SHA1_HASH_SIZE+4;
        char *data_t = malloc(data_len);
        *(int *)data_t = (int)pkt_num;
        i = 0;
        while((chunk = dequeue(chunks)) != NULL) {
            memcpy(data_t+4+i*SHA1_HASH_SIZE, chunk, SHA1_HASH_SIZE);
        }
        data_packet_t *pkt = make_whohas_packet((short)data_len, data_t);
        enqueue(pkts, pkt);
    }
    free_queue(chunks);
    return pkts;
}

void flood_whohas(queue *pkts) {
    data_packet_t *pkt;
    while ((pkt=dequeue(pkts))!=NULL) {
        bt_peer_t *current = config.peers;
        while (current!=NULL) {
            if (current->id != config.identity) {
                send_packet()
            }
            current = current->next;
        }
    }
}