#ifndef BIRD_DATA_H
#define BIRD_DATA_H

#include <stdint.h>

/* Per-level bird starting positions.
   Format: {gb_col, gb_platform_row}
   Derived from scenes.basm .chickens data using bbc_to_gb_row(bbc_row - 1),
   where bbc_row is the chicken's standing row (platform is one row below). */

#define BIRDS_PER_LEVEL 5U

/* Number of birds active at level start (from scenario header last field) */
static const uint8_t bird_counts[8] = { 2, 3, 3, 4, 4, 4, 3, 3 };

static const uint8_t bird_starts[8][5][2] = {
    /* level 0 */ { {5,6}, {8,2}, {4,10}, {6,13}, {12,17} },
    /* level 1 */ { {6,2}, {1,17}, {18,10}, {11,10}, {13,2} },
    /* level 2 */ { {2,7}, {9,4}, {17,14}, {0,17}, {8,13} },
    /* level 3 */ { {10,2}, {17,2}, {17,17}, {4,17}, {10,13} },
    /* level 4 */ { {1,13}, {3,10}, {1,6}, {14,10}, {15,13} },
    /* level 5 */ { {1,6}, {1,17}, {18,6}, {13,13}, {18,10} },
    /* level 6 */ { {13,2}, {1,6}, {14,11}, {0,15}, {2,10} },
    /* level 7 */ { {17,17}, {10,10}, {10,2}, {3,6}, {17,6} },
};

#endif /* BIRD_DATA_H */
