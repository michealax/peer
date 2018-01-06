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

/* 优先队列 */
typedef struct priority_node {
    void *data;
    int priority;
    struct priority_node *next;
} pnode_t;

typedef struct priority_queue {
    pnode_t *head;
    pnode_t *tail;
    int n;
} pqueue_t;

queue *make_queue();

void init_queue(queue *);

void free_queue(queue *, int);

void enqueue(queue *, void *);

void *dequeue(queue *);

/* 优先队列 */
pqueue_t *make_pqueue();

void enpqueue(pqueue_t *, void *, int);

void *depqueue(pqueue_t *);

void free_pqueue(pqueue_t *);

/* 没有使用到的函数 */
queue *queue_copy(queue *, size_t);

int is_in_queue(queue *, void *, size_t);

void print_queue(queue *);

#endif //PEER_QUEUE_H
