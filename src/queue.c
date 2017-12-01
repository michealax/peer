//
// Created by syang on 2017/11/30.
//

#include <stdio.h>
#include <malloc.h>
#include "queue.h"

void init_queue(queue *q) {
    q->head = q->tail = NULL;
    q->n = 0;
}

queue *make_queue() {
    queue *q = malloc(sizeof(queue));
    init_queue(q);
    return q;
}

void *dequeue(queue *q) {
    if (q->n == 0) { // null to ret
        return NULL;
    }
    node *head = q->head;
    void *ret = head->data;
    q->head = head->next;
    q->n -= 1;
    free(head);
    if (q->n == 0) {
        q->tail = NULL;
    }
    return ret;
}

void enqueue(queue *q, void *data) {
    node *n = malloc(sizeof(node));
    n->data = data;
    n->next = NULL;
    if (q->tail != NULL) {
        q->tail->next = n;
    }
    q->tail = n;
    if (q->head == NULL) {
        q->head = n;
    }
    q->n += 1;
}

void free_queue(queue *q, int r) {
    while (q->n) {
        void *ret = dequeue(q);
        if (r) {
            free(ret);
        }
    }
    free(q);
}


void print_queue(queue *q) {
    if (q == NULL) {
        printf("warning! the queue is empty!!!");
        return;
    }
    node *n = q->head;
    while (n != NULL) {
        printf("%s\n", (char *) n->data); // print the data about the node
        n = n->next;
    }
}