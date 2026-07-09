#ifndef METRICS_H
#define METRICS_H

/*
 * metrics.h — Per-process and system-wide metrics
 *
 * TAT = Turnaround Time = finish_time - arrival_time
 * WT  = Waiting Time    = TAT - burst
 * EPI = Energy Per Instruction = total_drain / total_burst
 */

#define MAX_METRICS 50

typedef struct {
    int pid;
    int burst;
    int tat;
    int wait_time;
} ProcMetric;

typedef struct {
    ProcMetric entries[MAX_METRICS];
    int        count;

    /* Aggregates — filled by metrics_finalize() */
    float avg_tat;
    float avg_wt;
    float total_drain;
    float epi;
    int   total_burst;
} Metrics;

void metrics_init(Metrics *m);
void metrics_add(Metrics *m, int pid, int burst, int tat, int wt);
void metrics_finalize(Metrics *m, float total_drain);
void metrics_print(const Metrics *m);

#endif /* METRICS_H */
