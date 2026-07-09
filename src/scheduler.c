/*
 * scheduler.c — EnergyAware Round Robin Scheduler
 *
 * Improvements over standard RR:
 *   1. Adaptive time quantum (battery-aware)
 *   2. DVFS P-state selection per quantum
 *   3. Non-linear battery drain model
 *   4. Starvation prevention (force-promote after MAX_SKIP quanta)
 *   5. EPI metric tracking
 */

#include <stdio.h>
#include "../include/scheduler.h"

/* ANSI color codes */
#define COL_RESET  "\033[0m"
#define COL_GREEN  "\033[32m"
#define COL_YELLOW "\033[33m"
#define COL_RED    "\033[31m"
#define COL_CYAN   "\033[36m"
#define COL_BOLD   "\033[1m"
#define COL_DIM    "\033[2m"

static const char *mode_color(PowerMode mode) {
    switch (mode) {
        case MODE_PERFORMANCE: return COL_GREEN;
        case MODE_BALANCED:    return COL_YELLOW;
        case MODE_SURVIVAL:    return COL_RED;
        default:               return COL_RESET;
    }
}

int compute_quantum(float battery, int remaining) {
    int base = mode_quantum(get_power_mode(battery));
    return (remaining < base) ? remaining : base;
}

void run_scheduler(Process *procs, int n,
                   float    init_battery,
                   Metrics *metrics,
                   Logger  *logger) {
    Queue        q;
    BatteryState bat = { init_battery, 0.0f };
    int          time = 0;
    PowerMode    prev_mode;

    queue_init(&q);
    metrics_init(metrics);
    logger_init(logger);

    for (int i = 0; i < n; i++) {
        procs[i].start_time  = -1;
        procs[i].finish_time = 0;
        procs[i].wait_time   = 0;
        procs[i].starvation  = 0;
        procs[i].remaining   = procs[i].burst;
        queue_enqueue(&q, procs[i]);
    }

    prev_mode = get_power_mode(bat.level);

    /* Print column header */
    printf(COL_BOLD "\n  EnergyAware-RR Scheduler -- Starting\n");
    printf("  Initial battery: %.1f%%    Processes: %d\n\n" COL_RESET,
           bat.level, n);
    printf("  %-5s  %-5s  %-4s  %-13s  %-13s  %-6s  %-7s\n",
           "Time", "PID", "Q", "Mode", "DVFS", "Bat%", "Rem");
    printf("  %s\n", "----------------------------------------------------------------");

    while (!queue_is_empty(&q) && bat.level > 0.0f) {

        PowerMode  mode = get_power_mode(bat.level);
        DVFSLevel *dvfs = select_dvfs(bat.level);

        if (mode != prev_mode) {
            printf(COL_BOLD "  >> Mode switch: %s%s" COL_RESET "\n",
                   mode_color(mode), mode_name(mode));
            prev_mode = mode;
        }

        /* Promote starved process to front of queue */
        int starved_idx = queue_find_starved(&q);
        if (starved_idx > 0) {
            queue_promote(&q, starved_idx);
            Process starved = queue_peek(&q);
            printf(COL_RED "  !! P%d promoted (starvation)\n" COL_RESET,
                   starved.pid);
        }

        Process proc = queue_dequeue(&q);
        int quantum  = compute_quantum(bat.level, proc.remaining);

        if (proc.start_time == -1) proc.start_time = time;
        proc.remaining  -= quantum;
        proc.starvation  = 0;

        float old_bat = bat.level;
        bat.level     = drain_battery(bat.level, mode,
                                      dvfs->freq, proc.burst);
        float drain   = old_bat - bat.level;
        bat.total_drain += drain;

        logger_add(logger, time, proc.pid, quantum,
                   mode_name(mode), dvfs->label, bat.level);

        int new_time = time + quantum;

        if (proc.remaining <= 0) {
            proc.finish_time = new_time;
            int tat = proc.finish_time - proc.arrival_time;
            metrics_add(metrics, proc.pid, proc.burst, tat, proc.wait_time);
            printf("  %s%-5d" COL_RESET "  P%-3d  %-3d  %s%-13s" COL_RESET
                   "  %-13s  " COL_CYAN "%-6.1f" COL_RESET
                   "  " COL_GREEN "done   " COL_RESET "\n",
                   COL_DIM, time, proc.pid, quantum,
                   mode_color(mode), mode_name(mode),
                   dvfs->label,
                   bat.level);
        } else {
            printf("  %s%-5d" COL_RESET "  P%-3d  %-3d  %s%-13s" COL_RESET
                   "  %-13s  " COL_CYAN "%-6.1f" COL_RESET "  %-7d\n",
                   COL_DIM, time, proc.pid, quantum,
                   mode_color(mode), mode_name(mode),
                   dvfs->label,
                   bat.level,
                   proc.remaining);
            queue_enqueue(&q, proc);
        }

        queue_tick_waiting(&q, quantum);

        time = new_time;
    }

    metrics_finalize(metrics, bat.total_drain);
    queue_free(&q);

    printf("  %s\n", "----------------------------------------------------------------");
    printf(COL_BOLD "  Simulation complete at t=%d ms\n" COL_RESET, time);

    if (bat.level <= 0.0f)
        printf(COL_RED "  Warning: Battery depleted!\n" COL_RESET);
}
