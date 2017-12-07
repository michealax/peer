#ifndef _PACKET_H_
#define _PACKET_H_

#include <assert.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>

#define BUFLEN 1500
#define PACKETLEN 1500
#define HEADERLEN 16
#define SHA1_HASH_SIZE 20
#define MAX_CHANK_NUM 74
#define DATALEN PACKETLEN-HEADERLEN
#define DATA_PACKET_DATA_LEN 1400
#define SEND_PACKET_DATA_LEN 1024

#define MAGICNUM 15441
#define VERSION 1

#define PKT_WHOHAS 		0
#define PKT_IHAVE		1
#define PKT_GET			2
#define PKT_DATA		3
#define PKT_ACK 		4
#define PKT_DENIED		5

typedef struct header_s {
    short magicnum;
    char version;
    char packet_type;
    short header_len;
    short packet_len;
    u_int seq_num;
    u_int ack_num;
} header_t;

typedef struct data_packet {
    header_t header;
    char data[DATALEN];
} data_packet_t;

// create/free packet
void init_packet(data_packet_t *, char, short, u_int, u_int, char *);
data_packet_t *make_packet(char, short, u_int, u_int, char *);
void free_packet(data_packet_t *);

// make different packet
data_packet_t *make_whohas_packet(short, void *);
data_packet_t *make_ihave_packet(short, void *);
data_packet_t *make_get_packet(short, char *);
data_packet_t *make_data_packet(short, uint, uint, char *);
data_packet_t *make_ack_packet(uint, uint);
data_packet_t *make_denied_packet();

// packet method for net
void host2net(data_packet_t *);
void net2host(data_packet_t *);
void send_packet(int, data_packet_t *, struct sockaddr *);

// check method
int packet_parser(void *);
// debug
void print_packet(data_packet_t *);
#endif /* _PACKET_H_ */