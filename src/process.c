/* process.c — Ready Queue implementation (singly linked list) */

#include <stdio.h>
#include <stdlib.h>
#include "../include/process.h"

void queue_init(Queue *q) {
    q->front = NULL;
    q->rear  = NULL;
    q->size  = 0;
}

void queue_enqueue(Queue *q, Process p) {
    QNode *node = (QNode *)malloc(sizeof(QNode));
    if (!node) { fprintf(stderr, "ERR: malloc failed\n"); exit(1); }
    node->proc = p;
    node->next = NULL;

    if (q->rear == NULL) {
        q->front = node;
        q->rear  = node;
    } else {
        q->rear->next = node;
        q->rear       = node;
    }
    q->size++;
}

Process queue_dequeue(Queue *q) {
    if (queue_is_empty(q)) {
        fprintf(stderr, "ERR: dequeue on empty queue\n");
        exit(1);
    }
    QNode   *tmp  = q->front;
    Process  proc = tmp->proc;
    q->front = tmp->next;
    if (q->front == NULL) q->rear = NULL;
    free(tmp);
    q->size--;
    return proc;
}

Process queue_peek(const Queue *q) {
    if (queue_is_empty(q)) {
        fprintf(stderr, "ERR: peek on empty queue\n");
        exit(1);
    }
    return q->front->proc;
}

int queue_is_empty(const Queue *q) {
    return q->size == 0;
}

void queue_free(Queue *q) {
    while (!queue_is_empty(q)) queue_dequeue(q);
}

/* Returns the index of the first starved process, or -1 if none. */
int queue_find_starved(Queue *q) {
    QNode *cur = q->front;
    int    idx = 0;
    while (cur) {
        if (cur->proc.starvation >= MAX_SKIP) return idx;
        cur = cur->next;
        idx++;
    }
    return -1;
}

void queue_promote(Queue *q, int idx) {
    if (idx == 0) return;  /* already at front */

    /* Walk to the node just before the target */
    QNode *prev = q->front;
    for (int i = 0; i < idx - 1; i++) prev = prev->next;

    QNode *target = prev->next;

    /* Unlink target */
    prev->next = target->next;
    if (target == q->rear) q->rear = prev;

    /* Insert at front */
    target->next = q->front;
    q->front     = target;
}

/* Advance wait time and starvation counter for all queued processes. */
void queue_tick_waiting(Queue *q, int quantum) {
    QNode *cur = q->front;
    while (cur) {
        cur->proc.wait_time  += quantum;
        cur->proc.starvation += 1;
        cur = cur->next;
    }
}

void queue_print(const Queue *q) {
    QNode *cur = q->front;
    printf("  Queue [%d]: ", q->size);
    while (cur) {
        printf("P%d(rem=%d) ", cur->proc.pid, cur->proc.remaining);
        cur = cur->next;
    }
    printf("\n");
}
