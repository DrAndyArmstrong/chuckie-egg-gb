#ifndef LIFT_TILE_H
#define LIFT_TILE_H

#include <stdint.h>

#define LIFT_NUM_TILES 1U

/* Lift car: solid top two rows + brick-joint row, transparent below.
   Colour index 1 → green via sprite palette 2. */
static const uint8_t lift_tiles[1][16] = {
    { 0xFF,0x00, 0xFF,0x00, 0xBD,0x00, 0x00,0x00,
      0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00 },
};

#endif /* LIFT_TILE_H */
