#ifndef PROCESS_H
#define PROCESS_H

/* process.h — Process structure and Ready Queue API */

#define MAX_PROCESSES 50
#define MAX_SKIP      8       /* starvation threshold */

/* Process descriptor */
typedef struct {
    int pid;
    int burst;          /* original burst time */
    int remaining;      /* remaining burst time */
    int arrival_time;
    int start_time;     /* -1 = not yet started */
    int finish_time;
    int wait_time;      /* cumulative wait */
    int starvation;     /* quanta skipped without running */
} Process;

/* Linked-list queue node */
typedef struct QNode {
    Process      proc;
    struct QNode *next;
} QNode;

/* Ready queue */
typedef struct {
    QNode *front;
    QNode *rear;
    int    size;
} Queue;

/* Queue operations */
void    queue_init(Queue *q);
void    queue_enqueue(Queue *q, Process p);
Process queue_dequeue(Queue *q);
Process queue_peek(const Queue *q);
int     queue_is_empty(const Queue *q);
void    queue_free(Queue *q);

/* Starvation helpers */
int  queue_find_starved(Queue *q);       /* returns index, -1 if none */
void queue_promote(Queue *q, int idx);   /* move process at idx to front */

/* Increment wait_time + starvation for every process in queue */
void queue_tick_waiting(Queue *q, int quantum);

/* Print queue state to stdout */
void queue_print(const Queue *q);

#endif /* PROCESS_H */
