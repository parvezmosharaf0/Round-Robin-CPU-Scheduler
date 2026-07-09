/* logger.c — Per-quantum CSV logging */

#include <stdio.h>
#include <string.h>
#include "../include/logger.h"

void logger_init(Logger *l) {
    l->count = 0;
}

void logger_add(Logger *l, int time, int pid, int dur,
                const char *mode, const char *dvfs, float bat) {
    if (l->count >= MAX_LOG_ROWS) return;
    LogRow *r = &l->rows[l->count++];
    r->time     = time;
    r->pid      = pid;
    r->duration = dur;
    r->battery  = bat;
    strncpy(r->mode, mode, sizeof(r->mode) - 1);
    strncpy(r->dvfs, dvfs, sizeof(r->dvfs) - 1);
    r->mode[sizeof(r->mode)-1] = '\0';
    r->dvfs[sizeof(r->dvfs)-1] = '\0';
}

int logger_export_csv(const Logger *l, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) { perror("fopen"); return -1; }

    fprintf(fp, "time,pid,duration,mode,dvfs,battery_pct\n");
    for (int i = 0; i < l->count; i++) {
        const LogRow *r = &l->rows[i];
        fprintf(fp, "%d,P%d,%d,%s,%s,%.2f\n",
                r->time, r->pid, r->duration,
                r->mode, r->dvfs, r->battery);
    }
    fclose(fp);
    printf("\n  CSV saved -> %s  (%d rows)\n", filename, l->count);
    return 0;
}
