#ifndef _GC_ContextConstants_h
#define _GC_ContextConstants_h

#define CONFIG_ATHLETE           0x1        // includes default weight, height etc
#define CONFIG_ZONES             0x2        // CP, FTP, useCPforFTP, zone config etc
#define CONFIG_GENERAL           0x4        // includes default weight, w'bal formula, directories
#define CONFIG_USERMETRICS       0x8        // user defined metrics
#define CONFIG_APPEARANCE        0x10
#define CONFIG_FIELDS            0x20       // metadata fields
#define CONFIG_NOTECOLOR         0x40       // ride coloring from "notes" fields
#define CONFIG_METRICS           0x100      // metrics to show for intervals, bests and summary
#define CONFIG_DEVICES           0x200
#define CONFIG_SEASONS           0x400      // includes seasons, events and LTS/STS seeded values
#define CONFIG_UNITS             0x800      // metric / imperial
#define CONFIG_PMC               0x1000     // PMC constants
#define CONFIG_WBAL              0x2000     // which w'bal formula to use ?
#define CONFIG_WORKOUTS          0x4000     // workout location / files
#define CONFIG_DISCOVERY         0x8000     // interval discovery
#define CONFIG_WORKOUTTAGMANAGER 0x10000    // workout tags

#endif
