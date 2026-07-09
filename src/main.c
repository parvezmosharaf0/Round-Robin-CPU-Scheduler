/*
 * main.c — EnergyAware-RR: Battery-State-Driven CPU Scheduling
 * Usage:
 *   ./energyaware              - sample 8-process workload
 *   ./energyaware custom       - enter processes manually
 *   ./energyaware experiment   - Pareto experiment + ASCII curve
 */

#define _POSIX_C_SOURCE 200809L   /* expose fileno(), dup(), dup2() */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#include "../include/process.h"
#include "../include/battery.h"
#include "../include/dvfs.h"
#include "../include/scheduler.h"
#include "../include/metrics.h"
#include "../include/logger.h"

#define COL_RESET  "\033[0m"
#define COL_BOLD   "\033[1m"
#define COL_CYAN   "\033[36m"
#define COL_GREEN  "\033[32m"
#define COL_YELLOW "\033[33m"
#define COL_RED    "\033[31m"
#define COL_PURPLE "\033[35m"

/* Pareto data point */
typedef struct {
    float  tat;        /* avg turnaround time */
    float  drain;      /* total battery drain % */
    float  epi;
    float  bat_start;
    char   label[32];
} ParetoPoint;

static void print_banner(void) {
    printf(COL_BOLD COL_CYAN);
    printf("\n  EnergyAware-RR CPU Scheduler\n");
    printf("  Battery-State-Driven Scheduling - OS Lab\n");
    printf(COL_RESET "\n");
}

static int load_sample(Process *procs) {
    int bursts[] = {12, 5, 20, 8, 15, 3, 18, 7};
    int n = 8;
    printf(COL_BOLD "  Sample Workload (%d processes):\n" COL_RESET, n);
    for (int i = 0; i < n; i++) {
        procs[i].pid          = i + 1;
        procs[i].burst        = bursts[i];
        procs[i].remaining    = bursts[i];
        procs[i].arrival_time = 0;
        procs[i].start_time   = -1;
        procs[i].finish_time  = 0;
        procs[i].wait_time    = 0;
        procs[i].starvation   = 0;
        printf("  P%d  burst=%d\n", procs[i].pid, procs[i].burst);
    }
    printf("\n");
    return n;
}

static int load_custom(Process *procs) {
    int n;
    printf("  How many processes? (max %d): ", MAX_PROCESSES);
    scanf("%d", &n);
    if (n < 1 || n > MAX_PROCESSES) n = 5;
    for (int i = 0; i < n; i++) {
        int burst;
        printf("  P%d burst time: ", i + 1);
        scanf("%d", &burst);
        if (burst < 1)   burst = 1;
        if (burst > 100) burst = 100;
        procs[i].pid          = i + 1;
        procs[i].burst        = burst;
        procs[i].remaining    = burst;
        procs[i].arrival_time = 0;
        procs[i].start_time   = -1;
        procs[i].finish_time  = 0;
        procs[i].wait_time    = 0;
        procs[i].starvation   = 0;
    }
    printf("\n");
    return n;
}

static const char *bat_mode_str(float bat) {
    return (bat > 70.0f) ? "Perf" : (bat > 30.0f) ? "Bal" : "Surv";
}

static const char *bat_color(float bat) {
    return (bat > 70.0f) ? COL_GREEN : (bat > 30.0f) ? COL_YELLOW : COL_RED;
}

/*
 * ascii_pareto — Scatter plot: X = Avg TAT (ms), Y = Battery Drain %
 *
 * - X-axis has 5 evenly-spaced tick labels showing real TAT values
 * - Axis range is padded 12% each side so labels don't crowd the edges
 * - Collision detection: two points on the same row are nudged apart
 * - Data table below the plot shows all values aligned
 */
static void ascii_pareto(ParetoPoint *pts, int np) {

#define AP_MAX  10      /* max data points */
#define AP_LBLW  9      /* chars per label "100Perf" */
#define AP_W    66      /* plot columns (after axis) */
#define AP_H    22      /* plot rows (before x-axis) */
#define AP_YPAD  9      /* Y-axis label + "|" width */
#define AP_XTIX  5      /* number of X-axis ticks */
#define AP_YTIX  5      /* number of Y-axis ticks */

    float min_tat = pts[0].tat,   max_tat = pts[0].tat;
    float min_d   = pts[0].drain, max_d   = pts[0].drain;
    for (int i = 1; i < np; i++) {
        if (pts[i].tat   < min_tat) min_tat = pts[i].tat;
        if (pts[i].tat   > max_tat) max_tat = pts[i].tat;
        if (pts[i].drain < min_d)   min_d   = pts[i].drain;
        if (pts[i].drain > max_d)   max_d   = pts[i].drain;
    }

    /* Pad 12% each side so labels don't crowd the edges */
    float tm = (max_tat - min_tat) * 0.14f; if (tm < 1.0f) tm = 1.0f;
    float dm = (max_d   - min_d)   * 0.14f; if (dm < 1.0f) dm = 1.0f;
    float lo_tat = min_tat - tm,  hi_tat = max_tat + tm;
    float lo_d   = min_d   - dm,  hi_d   = max_d   + dm;
    if (lo_d < 0.0f) lo_d = 0.0f;
    float tat_span = hi_tat - lo_tat;
    float d_span   = hi_d   - lo_d;

    char lbl[AP_MAX][AP_LBLW + 2];
    int  gx[AP_MAX], gy[AP_MAX];
    int  n = (np < AP_MAX) ? np : AP_MAX;

    for (int i = 0; i < n; i++) {
        snprintf(lbl[i], sizeof(lbl[i]), "%3.0f%s",
                 pts[i].bat_start, bat_mode_str(pts[i].bat_start));
        int llen = (int)strlen(lbl[i]);

        gx[i] = (int)((pts[i].tat   - lo_tat) / tat_span * (AP_W - llen));
        gy[i] = (int)((pts[i].drain - lo_d)   / d_span   * (AP_H - 1));
        gy[i] = (AP_H - 1) - gy[i];   /* flip: high drain = top row */

        if (gx[i] < 0)          gx[i] = 0;
        if (gx[i] > AP_W-llen)  gx[i] = AP_W - llen;
        if (gy[i] < 0)          gy[i] = 0;
        if (gy[i] > AP_H-1)     gy[i] = AP_H - 1;
    }

    /* Nudge same-row labels 1 row apart */
    for (int i = 1; i < n; i++) {
        for (int j = 0; j < i; j++) {
            if (gy[i] == gy[j]) {
                gy[i] = (gy[i] < AP_H - 2) ? gy[i] + 1 : gy[i] - 1;
            }
        }
    }

    /* Title */
    int total_w = AP_YPAD + AP_W;
    printf(COL_BOLD "\n  ");
    for (int c = 0; c < total_w; c++) printf("-");
    printf("\n  ");
    {
        const char *title = " Pareto Curve : Avg TAT (ms)  vs  Battery Drain % ";
        int tlen = (int)strlen(title);
        int pad  = (total_w - tlen) / 2;
        for (int c = 0; c < pad; c++) printf(" ");
        printf("%s\n", title);
    }
    printf("  ");
    for (int c = 0; c < total_w; c++) printf("-");
    printf("\n" COL_RESET);

    printf("  %sDrain%%%s\n", COL_CYAN, COL_RESET);

    for (int r = 0; r < AP_H; r++) {

        int ytick = 0;
        for (int t = 0; t < AP_YTIX; t++) {
            int tr = (int)((float)t / (AP_YTIX - 1) * (AP_H - 1) + 0.5f);
            if (r == tr) { ytick = 1; break; }
        }
        if (ytick) {
            float yv = hi_d - (float)r / (AP_H - 1) * d_span;
            printf("  %s%5.1f%%%s |", COL_CYAN, yv, COL_RESET);
        } else {
            printf("         |");
        }

        int col = 0;
        while (col < AP_W) {
            int hit = -1;
            for (int i = 0; i < n; i++) {
                if (gy[i] == r && gx[i] == col) { hit = i; break; }
            }
            if (hit >= 0) {
                printf("%s%s" COL_RESET, bat_color(pts[hit].bat_start), lbl[hit]);
                col += (int)strlen(lbl[hit]);
            } else {
                /* subtle grid dot every 12 cols x 5 rows */
                int dot = (col % 12 == 0) && (r % 5 == 0);
                putchar(dot ? '.' : ' ');
                col++;
            }
        }
        putchar('\n');
    }

    /* X-axis */
    printf("         +");
    for (int c = 0; c < AP_W; c++) printf("-");
    printf("\n");

    /* X-axis tick labels */
    {
        const int TLBL = 6;
        printf("         ");
        int prev_end = 0;
        for (int t = 0; t < AP_XTIX; t++) {
            float xv   = lo_tat + (float)t / (AP_XTIX - 1) * tat_span;
            int   tcol = (int)((float)t / (AP_XTIX - 1) * (AP_W - TLBL));
            for (int s = prev_end; s < tcol; s++) printf(" ");
            printf(COL_CYAN "%6.1f" COL_RESET, xv);
            prev_end = tcol + TLBL;
        }
        printf("\n");
    }

    {
        int centre = AP_YPAD + AP_W / 2 - 14;
        printf("%*s->  Avg Turnaround Time (ms)\n", centre, "");
    }

    /* Data table */
    printf(COL_BOLD
        "\n  %-28s  %-6s  %-12s  %-8s  %-6s\n"
        COL_RESET,
        "Config", "Bat%", "Avg TAT(ms)", "Drain%", "EPI");
    printf("  %s\n", "--------------------------------------------------------------------");
    for (int i = 0; i < np; i++) {
        const char *c = bat_color(pts[i].bat_start);
        printf("  %s%-28s" COL_RESET "  %s%4.0f%%%s    %6.2f ms     %5.1f%%  %6.4f\n",
               c, pts[i].label,
               c, pts[i].bat_start, COL_RESET,
               pts[i].tat, pts[i].drain, pts[i].epi);
    }
    printf("\n");

#undef AP_MAX
#undef AP_LBLW
#undef AP_W
#undef AP_H
#undef AP_YPAD
#undef AP_XTIX
#undef AP_YTIX
}

static void write_pareto_csv(ParetoPoint *pts, int np) {
    FILE *fp = fopen("data/pareto.csv", "w");
    if (!fp) return;
    fprintf(fp, "label,bat_start,avg_tat,total_drain,epi\n");
    for (int i = 0; i < np; i++) {
        fprintf(fp, "%s,%.1f,%.2f,%.2f,%.4f\n",
                pts[i].label, pts[i].bat_start,
                pts[i].tat, pts[i].drain, pts[i].epi);
    }
    fclose(fp);
    printf("\n  Pareto data saved -> data/pareto.csv\n");
    printf("  Run: python3 plot_pareto.py   (to generate pareto_curve.png)\n");
}

/* Run the scheduler with stdout suppressed, for batch experiments. */
static void run_silent(Process *procs, int n, float bat_start,
                       Metrics *m, Logger *l) {
    FILE *devnull = fopen("/dev/null", "w");
    int old_fd = -1;
    if (devnull) {
        old_fd = dup(fileno(stdout));
        dup2(fileno(devnull), fileno(stdout));
        fclose(devnull);
    }

    run_scheduler(procs, n, bat_start, m, l);

    if (old_fd >= 0) {
        fflush(stdout);
        dup2(old_fd, fileno(stdout));
        close(old_fd);
    }
}

static void run_pareto_experiment(void) {
    printf(COL_BOLD COL_PURPLE
           "\n  Pareto Experiment\n"
           "  Same workload, 7 battery start levels\n"
           "  Goal: show TAT vs Energy tradeoff curve\n"
           COL_RESET "\n");

    float bats[]    = {100.0f, 85.0f, 70.0f, 55.0f, 40.0f, 25.0f, 10.0f};
    int   n_configs = 7;

    int base_bursts[] = {12, 5, 20, 8, 15, 3};
    int n_procs = 6;

    ParetoPoint pts[10];

    printf("  %-28s %-10s %-10s %-10s\n",
           "Config", "Avg TAT", "Drain%", "EPI");
    printf("  %s\n", "----------------------------------------------------");

    for (int c = 0; c < n_configs; c++) {
        Process procs[MAX_PROCESSES];
        for (int i = 0; i < n_procs; i++) {
            procs[i].pid          = i + 1;
            procs[i].burst        = base_bursts[i];
            procs[i].remaining    = base_bursts[i];
            procs[i].arrival_time = 0;
            procs[i].start_time   = -1;
            procs[i].finish_time  = 0;
            procs[i].wait_time    = 0;
            procs[i].starvation   = 0;
        }

        Metrics m;
        Logger  l;
        run_silent(procs, n_procs, bats[c], &m, &l);

        const char *mode_lbl = bats[c] > 70 ? "Performance"
                             : bats[c] > 30 ? "Balanced" : "Survival";
        const char *col = bats[c] > 70 ? COL_GREEN
                        : bats[c] > 30 ? COL_YELLOW : COL_RED;

        snprintf(pts[c].label, sizeof(pts[c].label),
                 "bat=%.0f%% (%s)", bats[c], mode_lbl);
        pts[c].tat       = m.avg_tat;
        pts[c].drain     = m.total_drain;
        pts[c].epi       = m.epi;
        pts[c].bat_start = bats[c];

        printf("  %sbat=%.0f%% (%-11s)%s  %-10.1f %-10.1f %-10.4f\n",
               col, bats[c], mode_lbl, COL_RESET,
               m.avg_tat, m.total_drain, m.epi);
    }

    ascii_pareto(pts, n_configs);
    write_pareto_csv(pts, n_configs);

    printf(COL_BOLD "\n  Pareto Analysis:\n" COL_RESET);
    printf("  " COL_GREEN  "High battery -> Low TAT, High drain  (fast but costly)\n" COL_RESET);
    printf("  " COL_YELLOW "Mid battery  -> Balanced tradeoff\n" COL_RESET);
    printf("  " COL_RED    "Low battery  -> High TAT, Low drain  (slow but saves energy)\n" COL_RESET);
    printf("\n  No single config dominates all others -- this IS the Pareto frontier.\n\n");
}

int main(int argc, char *argv[]) {
    srand((unsigned int)time(NULL));

    print_banner();
    dvfs_print_table();

    printf(COL_BOLD "  Power Mode -> Quantum Mapping:\n" COL_RESET);
    printf("  " COL_GREEN  "Performance (>70%%)   Q=10ms  P0\n" COL_RESET);
    printf("  " COL_YELLOW "Balanced    (30-70%%) Q=6ms   P1/P2\n" COL_RESET);
    printf("  " COL_RED    "Survival    (<30%%)   Q=2ms   P3/P4\n" COL_RESET "\n");

    int mode = 0;
    if (argc > 1) {
        if (strcmp(argv[1], "custom")     == 0) mode = 1;
        if (strcmp(argv[1], "experiment") == 0) mode = 2;
    }

    if (mode == 2) {
        run_pareto_experiment();
        return 0;
    }

    Process procs[MAX_PROCESSES];
    int n = (mode == 1) ? load_custom(procs) : load_sample(procs);

    Metrics metrics;
    Logger  logger;
    run_scheduler(procs, n, 100.0f, &metrics, &logger);

    printf(COL_BOLD "\n  Metrics Summary\n" COL_RESET);
    metrics_print(&metrics);

    logger_export_csv(&logger, "data/schedule_log.csv");

    printf(COL_BOLD COL_CYAN
           "\n  Done! CSV -> data/schedule_log.csv\n"
           "  For Pareto curve: ./energyaware experiment\n"
           COL_RESET "\n");
    return 0;
}
