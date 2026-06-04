#ifndef LIFT_DATA_H
#define LIFT_DATA_H

#include <stdint.h>

/* Lift column per level (0 = no lift on this level).
   Derived from scenes.basm .lift EQUB data. */
static const uint8_t lift_cols[8] = { 0, 0, 5, 11, 16, 9, 18, 0 };

#define LIFT_TOP     8U    /* minimum lift_y in pixels */
#define LIFT_BOTTOM  112U  /* maximum lift_y — keeps car above HUD */
#define LIFT_W       8U    /* car width in pixels */

#endif /* LIFT_DATA_H */
