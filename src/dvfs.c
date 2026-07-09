/*
 * dvfs.c — DVFS P-state table and selection logic
 *
 * Power relationship: power_factor ≈ freq × volt²
 * This matches Intel SpeedStep and ARM DynamIQ behavior.
 *
 * P0: max performance, max power
 * P4: minimum performance, minimum power (7% of max)
 */

#include <stdio.h>
#include "../include/dvfs.h"

/* P-state table */
DVFSLevel DVFS_TABLE[DVFS_LEVELS] = {
    /*  id   freq   volt   power  speed   label         */
    {   0,  1.00f, 1.00f, 1.00f, 1.00f, "P0-Max"      },
    {   1,  0.80f, 0.90f, 0.65f, 0.80f, "P1-High"     },
    {   2,  0.60f, 0.80f, 0.38f, 0.60f, "P2-Balanced" },
    {   3,  0.40f, 0.70f, 0.20f, 0.40f, "P3-Low"      },
    {   4,  0.20f, 0.60f, 0.07f, 0.20f, "P4-Survival" },
};

/* Select P-state based on battery level */
DVFSLevel *select_dvfs(float battery) {
    if (battery > 70.0f) return &DVFS_TABLE[0];
    if (battery > 50.0f) return &DVFS_TABLE[1];
    if (battery > 30.0f) return &DVFS_TABLE[2];
    if (battery > 15.0f) return &DVFS_TABLE[3];
    return &DVFS_TABLE[4];
}

void dvfs_print_table(void) {
    printf("\n  %-14s %6s %6s %6s %6s\n",
           "P-State", "Freq%", "Volt%", "Pwr%", "Spd%");
    printf("  %s\n", "----------------------------------------------");
    for (int i = 0; i < DVFS_LEVELS; i++) {
        DVFSLevel *d = &DVFS_TABLE[i];
        printf("  %-14s %5.0f%% %5.0f%% %5.0f%% %5.0f%%\n",
               d->label,
               d->freq  * 100.0f,
               d->volt  * 100.0f,
               d->power * 100.0f,
               d->speed * 100.0f);
    }
    printf("\n");
}
