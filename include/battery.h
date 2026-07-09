#ifndef BATTERY_H
#define BATTERY_H

/* battery.h — Battery state and non-linear drain model */

/* Power modes */
typedef enum {
    MODE_PERFORMANCE = 0,   /* battery > 70% */
    MODE_BALANCED    = 1,   /* battery 30-70% */
    MODE_SURVIVAL    = 2    /* battery < 30% */
} PowerMode;

/* Battery state */
typedef struct {
    float level;         /* current % (0.0 - 100.0) */
    float total_drain;   /* cumulative % drained */
} BatteryState;

/* Base drain per quantum per mode (scaled by workload + jitter) */
#define DRAIN_PERFORMANCE  2.5f
#define DRAIN_BALANCED     1.5f
#define DRAIN_SURVIVAL     0.5f
#define DRAIN_IDLE         0.01f   /* always-on background drain */

PowerMode   get_power_mode(float battery);
const char *mode_name(PowerMode mode);
const char *mode_range(PowerMode mode);
int         mode_quantum(PowerMode mode);

/* Lithium-ion tail factor: drain accelerates below 20% */
float tail_factor(float battery);

/*
 * Non-linear battery drain for one quantum.
 *
 * Drain = DRAIN_BASE[mode] * freq_ratio * tail_factor * workload * jitter
 *   workload = min(2.0, proc_burst / 10)  - heavier process drains more
 *   jitter   = random 0.85-1.15           - natural variability
 *
 * @param battery    current battery %
 * @param mode       current power mode
 * @param freq_ratio DVFS frequency ratio (0.0-1.0)
 * @param proc_burst burst time of running process
 * @return new battery level after drain
 */
float drain_battery(float battery, PowerMode mode,
                    float freq_ratio, int proc_burst);

#endif /* BATTERY_H */
