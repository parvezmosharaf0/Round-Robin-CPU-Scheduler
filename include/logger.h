#ifndef LOGGER_H
#define LOGGER_H

/*
 * logger.h — Per-quantum CSV log
 * Output: data/schedule_log.csv
 */

#define MAX_LOG_ROWS 5000

typedef struct {
    int   time;
    int   pid;
    int   duration;
    char  mode[16];
    char  dvfs[24];
    float battery;
} LogRow;

typedef struct {
    LogRow rows[MAX_LOG_ROWS];
    int    count;
} Logger;

void logger_init(Logger *l);
void logger_add(Logger *l, int time, int pid, int dur,
                const char *mode, const char *dvfs, float bat);
int  logger_export_csv(const Logger *l, const char *filename);

#endif /* LOGGER_H */
