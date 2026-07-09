#ifndef SCHEDULER_H
#define SCHEDULER_H

/* scheduler.h — EnergyAware Round Robin Scheduler */

#include "process.h"
#include "battery.h"
#include "dvfs.h"
#include "metrics.h"
#include "logger.h"

/* Compute the adaptive time quantum. Never exceeds remaining burst. */
int compute_quantum(float battery, int remaining);

/*
 * Run the full EnergyAware-RR simulation.
 *
 * @param procs         array of processes to schedule
 * @param n             number of processes
 * @param init_battery  initial battery level (usually 100.0)
 * @param metrics       output: filled with TAT/WT/EPI
 * @param logger        output: per-quantum log rows
 */
void run_scheduler(Process *procs, int n,
                   float init_battery,
                   Metrics *metrics,
                   Logger  *logger);

#endif /* SCHEDULER_H */
