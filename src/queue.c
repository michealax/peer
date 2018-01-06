//
// Created by syang on 2017/11/30.
//

#include <stdio.h>
#include <malloc.h>
#include <string.h>
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

pqueue_t *make_pqueue() {
    pqueue_t *q=malloc(sizeof(pqueue_t));
    q->head = q->tail = NULL;
    q->n = 0;
    return q;
}

void free_pqueue(pqueue_t *q) {
    while (q->n) {
        void *ret = depqueue(q);
    }
    free(q);
}

void enpqueue(pqueue_t *q, void *data, int p) {
    pnode_t *n = malloc(sizeof(pnode_t));
    n->data = data;
    n->priority = p;
    n->next = NULL;
    if (q->head == NULL) {
        q->head = n;
        q->tail = n;
    } else {
        if (n->priority<q->head->priority) {
            n->next = q->head;
            q->head = n;
        } else if ((n->priority)>(q->tail->priority)) {
            q->tail->next = n;
            q->tail = n;
        } else {
            pnode_t *pre=q->tail;
            pnode_t *cur;
            for(cur=pre->next; cur!=NULL; cur=cur->next) {
                if(cur->priority>n->priority) {
                    break;
                } else {
                    pre = cur;
                }
            }
            pre->next = n;
            n->next = cur;
        }
    }
    q->n += 1;
}

void *depqueue(pqueue_t *q) {
    if (q->n == 0) { // null to ret
        return NULL;
    }
    pnode_t *head = q->head;
    void *ret = head->data;
    q->head = head->next;
    q->n -= 1;
    free(head);
    if (q->n == 0) {
        q->tail = NULL;
    }
    return ret;
}

queue *queue_copy(queue *src, size_t data_len) {
    queue *dst = make_queue();
    node *current = src->head;
    while (current!=NULL) {
        void *data=malloc(data_len);
        memcpy(data, current->data, data_len);
        enqueue(dst, data);
        current = current->next;
    }
    return dst;
}

int is_in_queue(queue *src, void *data, size_t data_len) {
    node *current = src->head;
    while (current!=NULL) {
        if (memcmp(data, current->data, data_len) == 0) {
            return 1;
        }
    }
    return 0;
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