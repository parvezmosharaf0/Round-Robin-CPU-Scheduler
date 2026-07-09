/* metrics.c — TAT, WT, EPI metrics */

#include <stdio.h>
#include <string.h>
#include "../include/metrics.h"

void metrics_init(Metrics *m) {
    memset(m, 0, sizeof(Metrics));
}

void metrics_add(Metrics *m, int pid, int burst, int tat, int wt) {
    if (m->count >= MAX_METRICS) return;
    ProcMetric *e = &m->entries[m->count++];
    e->pid       = pid;
    e->burst     = burst;
    e->tat       = tat;
    e->wait_time = wt;
    m->total_burst += burst;
}

void metrics_finalize(Metrics *m, float total_drain) {
    if (m->count == 0) return;
    float sum_tat = 0, sum_wt = 0;
    for (int i = 0; i < m->count; i++) {
        sum_tat += m->entries[i].tat;
        sum_wt  += m->entries[i].wait_time;
    }
    m->avg_tat     = sum_tat / m->count;
    m->avg_wt      = sum_wt  / m->count;
    m->total_drain = total_drain;
    m->epi         = (m->total_burst > 0)
                     ? total_drain / (float)m->total_burst
                     : 0.0f;
}

void metrics_print(const Metrics *m) {
    printf("\n  %-5s  %-8s  %-8s  %-10s\n",
           "PID", "Burst", "TAT(ms)", "Wait(ms)");
    printf("  %s\n", "--------------------------------------");
    for (int i = 0; i < m->count; i++) {
        const ProcMetric *e = &m->entries[i];
        printf("  P%-4d  %-8d  %-8d  %-10d\n",
               e->pid, e->burst, e->tat, e->wait_time);
    }
    printf("  %s\n", "--------------------------------------");
    printf("  %-5s  %-8s  %-8.1f  %-10.1f\n",
           "Avg", "-----", m->avg_tat, m->avg_wt);
    printf("\n");
    printf("  Total drain : %.2f%%\n", m->total_drain);
    printf("  EPI         : %.4f (drain%% / burst unit)\n", m->epi);
    printf("  Processes   : %d completed\n", m->count);
}
