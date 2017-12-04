/*
 * peer.c
 * 
 * Author: Zhigang Wu <16212010025@fudan.edu.cn>,
 *
 * Modified from CMU 15-441,
 * Original Authors: Ed Bardsley <ebardsle+441@andrew.cmu.edu>,
 *                   Dave Andersen
 * 
 * Class: Computer Network (Autumn 2017)
 *
 */

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "spiffy.h"
#include "bt_parse.h"
#include "input_buffer.h"
#include "packet.h"
#include "queue.h"
#include "trans.h"
#include "task.h"
#include "timer.h"

/* global variable */
static int run = 1; // whether to run
bt_config_t config; // bt config
int sock; // the sock about the program
task_t *task;
queue *has_chunks;
up_pool_t up_pool;
down_pool_t down_pool;

void peer_run(bt_config_t *config);

int main(int argc, char **argv) {

    bt_init(&config, argc, argv);

    DPRINTF(DEBUG_INIT, "peer.c main beginning\n");

#ifdef TESTING
    config.identity = 1; // your student number here
  strcpy(config.chunk_file, "chunkfile");
  strcpy(config.has_chunk_file, "haschunks");
#endif

    bt_parse_command_line(&config);

#ifdef DEBUG
    if (debug & DEBUG_INIT) {
    bt_dump_config(&config);
  }
#endif

    peer_run(&config);
    return 0;
}


void process_inbound_udp(int sock) {
#define BUFLEN 1500
    struct sockaddr_in from;
    socklen_t fromlen;
    char buf[BUFLEN];
    data_packet_t *pkt;
    fromlen = sizeof(from);
    up_conn_t *up_conn;
    down_conn_t *down_conn;

    while (spiffy_recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &from, &fromlen) != -1) {
        // printf("PROCESS_INBOUND_UDP SKELETON -- replace!\n""Incoming message from %s:%d\n%s\n\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port), buf);
        pkt = (data_packet_t *)buf;
        net2host(pkt); // as the data is converted to net format
        int packet_type = packet_parser(pkt);
        bt_peer_t *peer = get_peer(&config, from);// who send the packet

        switch (packet_type) {
            case PKT_WHOHAS: { // parse the who has pkt and answer
                queue *chunks = data2chunks_queue(pkt->data); // since every queue made by malloc so we don't care about NULL ptr
                queue *ihave = which_i_have(chunks);
                queue *pkts = init_ihave_queue(ihave);
                send_pkts(sock, (struct sockaddr *) &from, pkts);
                free_queue(pkts, 0);
                free_queue(ihave, 0);
                free_queue(chunks, 0);
                break;
            }
            case PKT_IHAVE: {
                queue *chunks = data2chunks_queue(pkt->data);
                uint8_t *chunk;
                while((chunk=dequeue(chunks))!=NULL){
                    available_peer(task, chunk, peer);
                }
                break;
            }
            case PKT_GET: {
                up_conn = get_up_conn(&up_pool, peer);
                if (up_conn == NULL) {
                    if (up_pool.conn >= up_pool.max_conn) {
                        data_packet_t *denied_pkt = make_denied_packet();
                        send_packet(sock, denied_pkt, (struct sockaddr *) &from);
                        free_packet(denied_pkt);
                    } else {
                        queue *pkts = init_data_queue((uint8_t *)pkt->data);
                        up_conn = add_to_up_pool(&up_pool, peer, pkts);
                        if(up_conn != NULL) {

                        }
                    }
                } else {
                    //
                    printf("already in pool!");
                }
                break;
            }
            case PKT_DATA: {
                down_conn = get_down_conn(&down_pool, peer);
                if (down_conn != NULL) {
                    uint ack = pkt->header.ack_num;
                    uint seq = pkt->header.seq_num;
                    if (seq == down_conn->next_ack) {
                        if(timer_now(&down_conn->timer) > 500) {
                            data_packet_t *ack_packet = make_ack_packet(seq, 0);
                            send_packet(sock, ack_packet, (struct sockaddr *) &from);
                            timer_start(&down_conn->timer);
                            free_packet(ack_packet);
                        }
                        down_conn->next_ack+=1;
                    } else { //
                        data_packet_t *ack_packet = make_ack_packet(down_conn->next_ack-1, 0);
                        send_packet(sock, ack_packet, (struct sockaddr *) &from);
                        free_packet(ack_packet);
                    }
                }

                break;
            }
            case PKT_ACK: {

                break;
            }
            case PKT_DENIED: {
                break; // be rejected _(xз」∠)_
            }
            default: {
                break; // since packet is error
            }
        }
    }


}

void process_get(char *chunkfile, char *outputfile) {
    printf("PROCESS GET SKELETON CODE CALLED.  Fill me in!  (%s, %s)\n", chunkfile, outputfile);
    // set the output file of the bt config
    set_peer_file(config.output_file, outputfile);
    // make the who_has packet queue
    queue *who_has_queue = init_whohas_queue(chunkfile);
    // init task
    init_task(task, outputfile, who_has_queue->n, config.max_conn);
    // ask all in the network who has the chunks
    flood_whohas(sock, who_has_queue);
    // free the queue we create as it is useless
    free_queue(who_has_queue, 0);
    // start timer
    timer_start(&task->timer);
}

void handle_user_input(char *line, void *cbdata) {
    char chunkf[128], outf[128];

    bzero(chunkf, sizeof(chunkf));
    bzero(outf, sizeof(outf));

    if (sscanf(line, "GET %120s %120s", chunkf, outf)) { // 接受用户手动输入参数
        if (strlen(outf) > 0) {
            process_get(chunkf, outf); // 处理get请求
        }
    }
}


void peer_run(bt_config_t *config) {
    struct sockaddr_in myaddr;
    fd_set readfds;
    struct user_iobuf *userbuf;

    if ((userbuf = create_userbuf()) == NULL) {
        perror("peer_run could not allocate userbuf");
        exit(-1);
    }

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
        perror("peer_run could not create socket");
        exit(-1);
    }

    bzero(&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(config->myport);

    if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
        perror("peer_run could not bind socket");
        exit(-1);
    }

    spiffy_init(config->identity, (struct sockaddr *) &myaddr, sizeof(myaddr)); // spiffy_init 初始化spiffy

    while (run) {
        int nfds;
        FD_SET(STDIN_FILENO, &readfds); // 键盘输入加入fd set中
        FD_SET(sock, &readfds);

        // nfds 可供读写的fd值
        nfds = select(sock + 1, &readfds, NULL, NULL, NULL); //sock是最大fd描述符，需要设置成最大描述符+1

        if (nfds > 0) {
            if (FD_ISSET(sock, &readfds)) { //检查文件符是否可供读写
                process_inbound_udp(sock);
            }

            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                process_user_input(STDIN_FILENO, userbuf, handle_user_input, "Currently unused");
            }
        } else {
            if (task != NULL) {
                if(task->pool == NULL && timer_now(&task->timer) > 500) {
                    flood_get(task, sock); // to get the chunk
                }

            }
        }
    }
}
