#ifndef _PACKET_H_
#define _PACKET_H_

#include <assert.h>
#include <sys/socket.h>
#include <winsock2.h>
#include <string.h>
#include <stdio.h>

#define BUFLEN 100
#define PACKETLEN 1500
#define HEADERLEN 16
#define DATALEN PACKETLEN-HEADERLEN

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
    char data[BUFLEN];
} data_packet_t;

void init_packet(data_packet_t *, char, short, u_int, u_int, char *);
data_packet_t *make_packet(char, short, u_int, u_int, char *);
void free_packet(data_packet_t *);

data_packet_t *make_whohas_packet(short, void *);
data_packet_t *make_ihave_packet(short, void *);
data_packet_t *make_get_packet();
data_packet_t *make_data_packet();
data_packet_t *make_ack_packet();
data_packet_t *make_denied_packet();


void host2net(data_packet_t *);


#endif /* _PACKET_H_ */