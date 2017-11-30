//
// Created by syang on 2017/11/30.
//

#ifndef PEER_QUEUE_H
#define PEER_QUEUE_H

typedef struct node {
    void *data;
    struct node *next;
} node;

typedef struct queue {
    node *head;
    node *tail;
    int n;
} queue;

queue *make_queue();
void init_queue(queue *);
void free_queue(queue *);
void enqueue(queue *, void *);
void *dequeue(queue *);
#endif //PEER_QUEUE_H
