/*
 * battery.c — Non-linear battery drain model
 *
 * drain = DRAIN_BASE[mode] x freq_ratio x tail x workload x jitter
 */

#include <stdlib.h>
#include "../include/battery.h"

PowerMode get_power_mode(float battery) {
    if (battery > 70.0f) return MODE_PERFORMANCE;
    if (battery > 30.0f) return MODE_BALANCED;
    return MODE_SURVIVAL;
}

const char *mode_name(PowerMode mode) {
    switch (mode) {
        case MODE_PERFORMANCE: return "Performance";
        case MODE_BALANCED:    return "Balanced";
        case MODE_SURVIVAL:    return "Survival";
        default:               return "Unknown";
    }
}

const char *mode_range(PowerMode mode) {
    switch (mode) {
        case MODE_PERFORMANCE: return ">70%";
        case MODE_BALANCED:    return "30-70%";
        case MODE_SURVIVAL:    return "<30%";
        default:               return "?";
    }
}

int mode_quantum(PowerMode mode) {
    switch (mode) {
        case MODE_PERFORMANCE: return 10;
        case MODE_BALANCED:    return 6;
        case MODE_SURVIVAL:    return 2;
        default:               return 6;
    }
}

/*
 * Lithium-ion tail factor: internal resistance rises steeply at low
 * state-of-charge, so drain accelerates below 20%.
 */
float tail_factor(float battery) {
    if (battery < 20.0f)
        return 1.0f + (20.0f - battery) * 0.05f;
    return 1.0f;
}

float drain_battery(float battery, PowerMode mode,
                    float freq_ratio, int proc_burst) {
    float base;
    switch (mode) {
        case MODE_PERFORMANCE: base = DRAIN_PERFORMANCE; break;
        case MODE_BALANCED:    base = DRAIN_BALANCED;    break;
        case MODE_SURVIVAL:    base = DRAIN_SURVIVAL;    break;
        default:               base = DRAIN_BALANCED;    break;
    }

    /* Workload factor: heavier process drains more, clamped to [0.1, 2.0] */
    float workload = (float)proc_burst / 10.0f;
    if (workload < 0.1f) workload = 0.1f;
    if (workload > 2.0f) workload = 2.0f;

    /* Jitter: 0.85-1.15 for natural variability */
    float jitter = 0.85f + ((float)rand() / (float)RAND_MAX) * 0.30f;

    float active = base * freq_ratio * tail_factor(battery)
                        * workload * jitter;

    float new_bat = battery - active - DRAIN_IDLE;
    return (new_bat < 0.0f) ? 0.0f : new_bat;
}
