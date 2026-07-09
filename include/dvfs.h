#ifndef DVFS_H
#define DVFS_H

/*
 * dvfs.h — Dynamic Voltage & Frequency Scaling (DVFS)
 * 5 discrete P-states (like Intel SpeedStep / ARM big.LITTLE)
 *
 * Power relationship: P ≈ freq × volt²
 * Cutting freq by half reduces power by ~75%.
 */

#define DVFS_LEVELS 5

typedef struct {
    int   id;
    float freq;     /* fraction of max frequency (0.0-1.0) */
    float volt;     /* fraction of max voltage   (0.0-1.0) */
    float power;    /* relative power draw       (0.0-1.0) */
    float speed;    /* relative execution speed  (0.0-1.0) */
    char  label[20];
} DVFSLevel;

/* Global P-state table (defined in dvfs.c) */
extern DVFSLevel DVFS_TABLE[DVFS_LEVELS];

/* Select the appropriate P-state based on current battery level. */
DVFSLevel *select_dvfs(float battery);

/* Print full DVFS table to stdout */
void dvfs_print_table(void);

#endif /* DVFS_H */
