#include <malloc.h>
#include <netinet/in.h>
#include "packet.h"
#include "spiffy.h"

void init_packet(data_packet_t *pkt, char type, short len, u_int seq, u_int ack, char *data) {
    pkt->header.magicnum = 15441;
    pkt->header.version = 1;
    pkt->header.packet_type = type;
    pkt->header.header_len = HEADERLEN;
    pkt->header.packet_len = len;
    pkt->header.seq_num = seq;
    pkt->header.ack_num = ack;
    if (pkt->data != NULL) {
        memcpy(pkt->data, data, len - HEADERLEN);
    }
    return;
}

data_packet_t *make_packet(char type, short len, u_int seq, u_int ack, char *data) {
    data_packet_t *pkt = malloc(sizeof(data_packet_t));
    init_packet(pkt, type, len, seq, ack, data);
    return pkt;
}

void free_packet(data_packet_t *pkt){
    if(pkt->data != NULL) {
        free(pkt->data);
    }
    free(pkt);
}

data_packet_t *make_whohas_packet(short len, void *data) {
    data_packet_t *pkt = make_packet(PKT_WHOHAS, len, 0, 0, data);
    return pkt;
}

data_packet_t *make_ihave_packet(short len, void *data) {
    return make_packet(PKT_IHAVE, len, 0, 0, data);
}

data_packet_t *make_get_packet(short len, char *data) {
    return make_packet(PKT_GET, len, 0, 0, data);
}

data_packet_t *make_data_packet(short len, uint ack_num, uint seq_num, char *data) {
    return make_packet(PKT_DATA, len, seq_num, ack_num, data);
}

data_packet_t *make_ack_packet(uint ack_num, uint seq_num) {
    return make_packet(PKT_ACK, HEADERLEN, seq_num, ack_num, NULL);
}

data_packet_t *make_denied_packet() {
    return make_packet(PKT_DENIED, HEADERLEN, 0, 0, NULL);
}



/* check whether it is a right packet or error */
int packet_parser(void* packet) {
    data_packet_t* pkt = (data_packet_t*)packet;
    header_t header = pkt->header;
    /* check magic number */
    if(header.magicnum != MAGICNUM) {
        return -1;
    }
    /* check the version */
    if(header.version != VERSION)
        return -1;
    /* check the packet_type */
    if(header.packet_type < PKT_WHOHAS || header.packet_type > PKT_DENIED)
        return -1;
    return header.packet_type;
}

void host2net(data_packet_t *pkt) {
    pkt->header.magicnum = htons(pkt->header.magicnum);
    pkt->header.header_len = htons(pkt->header.header_len);
    pkt->header.packet_len = htons(pkt->header.packet_len);
    pkt->header.seq_num = htons(pkt->header.seq_num);
    pkt->header.ack_num = htons(pkt->header.ack_num);
}

void net2host(data_packet_t *pkt) {
    pkt->header.magicnum = ntohs(pkt->header.magicnum);
    pkt->header.header_len = ntohs(pkt->header.header_len);
    pkt->header.packet_len = ntohs(pkt->header.packet_len);
    pkt->header.seq_num = ntohs(pkt->header.seq_num);
    pkt->header.ack_num = ntohs(pkt->header.ack_num);
}

void send_packet(int sock, data_packet_t *pkt, struct sockaddr *to) {
    size_t len = pkt->header.packet_len;
    host2net(pkt);
    spiffy_sendto(sock, pkt, len, 0, to, sizeof(*to));
    net2host(pkt);
}

void print_packet(data_packet_t *pkt) {
    printf("magicnum: %d\n", pkt->header.magicnum);
    printf("version: %d\n", pkt->header.version);
    printf("type: %d\n", pkt->header.packet_type);
    printf("header len: %d\n", pkt->header.header_len);
    printf("packet len: %d\n", pkt->header.packet_type);
    printf("seq: %d\n", pkt->header.seq_num);
    printf("ack: %d\n", pkt->header.ack_num);
    printf("data: %s\n", pkt->data);
}