#include <gb/gb.h>
#include <gb/cgb.h>

#include "tileset.h"
#include "level_data.h"
#include "harry.h"
#include "bird_tiles.h"
#include "bird_data.h"

/* cgb.h already defines RGB(r,g,b) */
#define COL_BLACK   RGB( 0,  0,  0)
#define COL_GREEN   RGB( 0, 31,  0)
#define COL_MAGENTA RGB(31,  0, 31)
#define COL_YELLOW  RGB(31, 31,  0)
#define COL_CYAN    RGB( 0, 31, 31)
#define COL_WHITE   RGB(31, 31, 31)

static const uint16_t bg_palettes[8][4] = {
    /* 0 EMPTY           */ {COL_BLACK,   COL_BLACK,   COL_BLACK,   COL_BLACK  },
    /* 1 PLATFORM        */ {COL_BLACK,   COL_GREEN,   COL_BLACK,   COL_GREEN  },
    /* 2 LADDER          */ {COL_BLACK,   COL_MAGENTA, COL_BLACK,   COL_MAGENTA},
    /* 3 EGG             */ {COL_BLACK,   COL_YELLOW,  COL_BLACK,   COL_YELLOW },
    /* 4 GRAIN           */ {COL_BLACK,   COL_MAGENTA, COL_BLACK,   COL_MAGENTA},
    /* 5 SOLID           */ {COL_GREEN,   COL_GREEN,   COL_GREEN,   COL_GREEN  },
    /* 6 LADDER_PLATFORM */ {COL_BLACK,   COL_GREEN,   COL_MAGENTA, COL_WHITE  },
    /* 7 HUD             */ {COL_BLACK,   COL_WHITE,   COL_BLACK,   COL_WHITE  },
};

static const uint16_t spr_palettes[2][4] = {
    /* 0 Harry  */ {COL_BLACK, COL_YELLOW, COL_BLACK, COL_YELLOW},
    /* 1 Chicks */ {COL_BLACK, COL_CYAN,   COL_BLACK, COL_CYAN  },
};

/* ---- Harry constants ---- */
#define HARRY_W         8
#define HARRY_H        16
#define WALK_SPEED      1
#define CLIMB_SPEED     1
#define JUMP_VEL        5
#define MAX_FALL        4
#define ANIM_PERIOD     8

#define HARRY_TILE_BASE  8
#define HARRY_TILE_W0   (HARRY_TILE_BASE + 0)
#define HARRY_TILE_W1   (HARRY_TILE_BASE + 2)
#define HARRY_TILE_CL   (HARRY_TILE_BASE + 4)

/* ---- Bird constants ---- */
#define MAX_BIRDS        BIRDS_PER_LEVEL
#define BIRD_W           8
#define BIRD_H          16
#define BIRD_MOVE_PERIOD 4
#define BIRD_ANIM_PERIOD 16
#define BIRD_EAT_FRAMES  80
#define BIRD_DIR_MIN     45
#define BIRD_DIR_RANGE   64

#define BIRD_TILE_BASE   14
#define BIRD_TILE_W0     (BIRD_TILE_BASE + 0)
#define BIRD_TILE_W1     (BIRD_TILE_BASE + 2)
#define BIRD_TILE_EAT    (BIRD_TILE_BASE + 4)

/* ---- Game constants ---- */
#define HARRY_START_LIVES  5U
#define HARRY_START_X      8
#define HARRY_START_ROW    17

#define SCORE_EGG         250U
#define SCORE_GRAIN        50U

#define DEATH_FLASH_FRAMES   80U
#define INVINCIBLE_FRAMES   180U  /* 3 seconds */
#define LEVEL_CLEAR_FRAMES   90U
#define GAME_OVER_FRAMES   180U

#define STATE_PLAYING     0
#define STATE_DYING       1
#define STATE_LEVEL_CLEAR 2
#define STATE_GAME_OVER   3

#define COLL_INSET_H  2
#define COLL_INSET_V  3

/* ---- Sound frequency constants (GB: freq_x = 2048 - 131072/hz) ---- */
#define FREQ_E4  1651U   /* E4 ~330Hz */
#define FREQ_A4  1750U   /* A4 ~440Hz */
#define FREQ_C5  1797U   /* C5 ~523Hz */
#define FREQ_E5  1849U   /* E5 ~659Hz */
#define FREQ_G5  1881U   /* G5 ~784Hz */
#define FREQ_C6  1923U   /* C6 ~1047Hz */
#define FREQ_LO(f) ((uint8_t)((f) & 0xFFU))
#define FREQ_HI(f) ((uint8_t)(((f) >> 8) & 0x07U))

/* ---- Bird struct ---- */
typedef struct {
    int16_t  x, y;
    int8_t   dx;
    uint8_t  climbing;
    int8_t   climb_dir;
    uint8_t  climb_start_row;
    uint8_t  eating;
    uint8_t  eat_timer;
    uint8_t  move_timer;
    uint8_t  anim_timer;
    uint8_t  anim_frame;
    uint8_t  dir_timer;
} Bird;

static Bird    birds[MAX_BIRDS];
static uint8_t prng;

/* ---- Level state ---- */
static uint8_t  level_state[LEVEL_ROWS * LEVEL_COLS];
static uint8_t  eggs_collected;
static uint8_t  grain_collected;
static uint8_t  total_eggs;

/* ---- Harry state ---- */
static int16_t harry_x;
static int16_t harry_y;
static int8_t  harry_vy;
static uint8_t on_ground;
static uint8_t climbing;
static uint8_t climb_lock;
static uint8_t climb_start_row;
static uint8_t facing_left;
static uint8_t invincible_timer;
static uint8_t anim_timer;
static uint8_t anim_frame;

#define CLIMB_LOCK_FRAMES 20U

/* ---- Game state ---- */
static uint8_t  game_state;
static uint8_t  harry_lives;
static uint16_t score;
static uint8_t  current_level;
static uint8_t  state_timer;

/* ---- Sound effects ---- */

static void sfx_jump(void) {
    NR10_REG = 0x15;    /* sweep: period=1, direction=up, shift=5 */
    NR11_REG = 0xC0;    /* duty=75% */
    NR12_REG = 0x73;    /* vol=7, decrease, period=3 */
    NR13_REG = 0x00;
    NR14_REG = 0x87;    /* trigger, freq_hi=7 */
}

static void sfx_egg(void) {
    NR10_REG = 0x00;
    NR11_REG = 0xC0;
    NR12_REG = 0xF2;    /* vol=15, decrease, period=2 */
    NR13_REG = FREQ_LO(FREQ_C6);
    NR14_REG = (uint8_t)(0x80U | FREQ_HI(FREQ_C6));
}

static void sfx_grain(void) {
    NR21_REG = 0x80;    /* duty=50% */
    NR22_REG = 0xA2;    /* vol=10, decrease, period=2 */
    NR23_REG = FREQ_LO(FREQ_C5);
    NR24_REG = (uint8_t)(0x80U | FREQ_HI(FREQ_C5));
}

/* Descending A-minor arpeggio: E5→C5→A4→E4 — one note per call */
static void sfx_death_tune(uint16_t f) {
    NR10_REG = 0x00;
    NR11_REG = 0x80;    /* 50% duty */
    NR12_REG = 0xD3;    /* vol=13, decrease, period=3 (~47ms/step) */
    NR13_REG = FREQ_LO(f);
    NR14_REG = (uint8_t)(0x80U | FREQ_HI(f));
}

static void sfx_death(void) {
    NR41_REG = 0x00;
    NR42_REG = 0xF4;    /* vol=15, decrease, period=4 */
    NR43_REG = 0x56;    /* clock_shift=5, 15-stage poly, div=6 */
    NR44_REG = 0x80;    /* trigger */
}

static void sfx_note(uint16_t f) {
    NR10_REG = 0x00;
    NR11_REG = 0xC0;
    NR12_REG = 0xF2;
    NR13_REG = FREQ_LO(f);
    NR14_REG = (uint8_t)(0x80U | FREQ_HI(f));
}

/* ---- Tile helpers ---- */

static uint8_t tile_at_px(int16_t px, int16_t py) {
    uint8_t col, row;
    if (px < 0) return TILE_SOLID;
    if (py < 0) return TILE_EMPTY;
    col = (uint8_t)((uint16_t)px >> 3);
    row = (uint8_t)((uint16_t)py >> 3);
    if (col >= LEVEL_COLS || row >= LEVEL_ROWS) return TILE_SOLID;
    return level_state[(uint16_t)row * LEVEL_COLS + col];
}

static uint8_t is_floor(uint8_t t) {
    return t == TILE_SOLID || t == TILE_PLATFORM || t == TILE_LADDER_PLATFORM;
}

static uint8_t is_ceiling(uint8_t t) {
    return t == TILE_SOLID;
}

static uint8_t is_wall(uint8_t t) {
    return t == TILE_SOLID;
}

static uint8_t is_ladder(uint8_t t) {
    return t == TILE_LADDER || t == TILE_LADDER_PLATFORM;
}

/* ---- Level helpers ---- */

static void redraw_tile(uint8_t col, uint8_t row, uint8_t tile) {
    VBK_REG = 0; set_bkg_tiles(col, row, 1, 1, &tile);
    VBK_REG = 1; set_bkg_tiles(col, row, 1, 1, &tile);
    VBK_REG = 0;
}

static void draw_level(void) {
    uint8_t row, col, tile;
    for (row = 0; row < LEVEL_ROWS; row++) {
        for (col = 0; col < LEVEL_COLS; col++) {
            tile = level_state[row * LEVEL_COLS + col];
            VBK_REG = 0; set_bkg_tiles(col, row, 1, 1, &tile);
            VBK_REG = 1; set_bkg_tiles(col, row, 1, 1, &tile);
        }
    }
    VBK_REG = 0;
}

static void load_level(uint8_t idx) {
    uint16_t i;
    const uint8_t *src = levels[idx];
    total_eggs = 0;
    for (i = 0; i < (uint16_t)(LEVEL_ROWS * LEVEL_COLS); i++) {
        level_state[i] = src[i];
        if (src[i] == TILE_EGG) total_eggs++;
    }
    eggs_collected  = 0;
    grain_collected = 0;
    draw_level();
}

/* ---- HUD ---- */

static void hud_update(void) {
    uint8_t  buf[20];
    uint8_t  i;
    uint16_t s = score;

    for (i = 0; i < 5U; i++) {
        buf[i] = (i < harry_lives) ? (uint8_t)TILE_HEART : (uint8_t)TILE_EMPTY;
    }
    buf[5] = TILE_EMPTY;
    for (i = 11U; i >= 6U; i--) {
        buf[i] = (uint8_t)(TILE_DIGIT_0 + (uint8_t)(s % 10U));
        s /= 10U;
    }
    for (i = 12U; i < 20U; i++) buf[i] = TILE_EMPTY;

    VBK_REG = 0; set_win_tiles(0, 0, 20, 1, buf);
    for (i = 0; i < 20U; i++) buf[i] = 7U;
    VBK_REG = 1; set_win_tiles(0, 0, 20, 1, buf);
    VBK_REG = 0;
}

/* ---- Bird helpers ---- */

static uint8_t rand8(void) {
    uint8_t lsb = prng & 1;
    prng >>= 1;
    if (lsb) prng ^= 0xB8;
    return prng;
}

static uint8_t active_birds;

static void birds_init(uint8_t level_idx) {
    uint8_t i;
    prng = 0xA5;
    active_birds = bird_counts[level_idx];
    for (i = 0; i < active_birds; i++) {
        uint8_t col = bird_starts[level_idx][i][0];
        uint8_t pr  = bird_starts[level_idx][i][1];
        birds[i].x               = (int16_t)col * 8;
        birds[i].y               = (int16_t)pr  * 8 - BIRD_H;
        birds[i].dx              = (int8_t)(i & 1 ? 1 : -1);
        birds[i].climbing        = 0;
        birds[i].climb_dir       = 0;
        birds[i].climb_start_row = 0;
        birds[i].eating          = 0;
        birds[i].eat_timer       = 0;
        birds[i].move_timer      = 0;
        birds[i].anim_timer      = (uint8_t)(i * 8);
        birds[i].anim_frame      = 0;
        birds[i].dir_timer       = (uint8_t)(BIRD_DIR_MIN + i * 13U);
    }
    /* hide unused bird sprites */
    for (i = active_birds; i < MAX_BIRDS; i++) {
        move_sprite((uint8_t)(i + 1U), 0, 0);
    }
}

static void bird_update_one(uint8_t i) {
    Bird    *b      = &birds[i];
    uint8_t  oam_id = (uint8_t)(i + 1U);
    uint8_t  tile, attr, gcol, gfy, center_row;
    uint16_t gidx;
    int16_t  cx, chk_x, new_pos, center_px, oam_y;

    cx = b->x + BIRD_W / 2;

    if (b->eating) {
        if (b->eat_timer > 0) b->eat_timer--;
        else b->eating = 0;
        tile = BIRD_TILE_EAT;
        attr = 1;

    } else if (b->climbing) {
        b->move_timer++;
        if (b->move_timer >= BIRD_MOVE_PERIOD) {
            b->move_timer = 0;
            new_pos   = b->y + (int16_t)b->climb_dir;
            /* clamp feet to play area — same boundary as Harry */
            if (new_pos + BIRD_H > (int16_t)(HARRY_START_ROW * 8))
                new_pos = (int16_t)(HARRY_START_ROW * 8) - BIRD_H;
            center_px = new_pos + BIRD_H / 2;

            if (center_px < 0 || center_px >= (int16_t)(LEVEL_ROWS * 8)) {
                b->climbing = 0;
                b->dx = (rand8() & 1) ? (int8_t)1 : (int8_t)-1;
            } else {
                center_row = (uint8_t)((uint16_t)center_px >> 3);
                if (tile_at_px(cx, center_px) == TILE_LADDER_PLATFORM &&
                    center_row != b->climb_start_row) {
                    b->y = (int16_t)center_row * 8 - BIRD_H;
                    b->climbing = 0;
                    b->dx = (rand8() & 1) ? (int8_t)1 : (int8_t)-1;
                } else if (!is_ladder(tile_at_px(cx, center_px)) &&
                           center_row != b->climb_start_row) {
                    /* Ladder ended without a LADDER_PLATFORM junction.
                       Snap down to the nearest floor so the bird isn't stuck. */
                    {
                        int16_t sy = (int16_t)center_row * 8;
                        while (sy + BIRD_H < (int16_t)(LEVEL_ROWS * 8)) {
                            if (is_floor(tile_at_px(cx, sy + BIRD_H))) {
                                uint8_t fr = (uint8_t)((uint16_t)(sy + BIRD_H) >> 3);
                                new_pos = (int16_t)fr * 8 - BIRD_H;
                                break;
                            }
                            sy += 8;
                        }
                    }
                    b->y = new_pos;
                    b->climbing = 0;
                    b->dx = (rand8() & 1) ? (int8_t)1 : (int8_t)-1;
                } else {
                    b->y = new_pos;
                }
            }
        }
        tile = BIRD_TILE_EAT;
        attr = 1;

    } else {
        b->move_timer++;
        if (b->move_timer >= BIRD_MOVE_PERIOD) {
            b->move_timer = 0;

            chk_x = (b->dx > 0) ? b->x + BIRD_W : b->x - 1;
            if (!is_floor(tile_at_px(chk_x, b->y + BIRD_H)) ||
                 is_wall( tile_at_px(chk_x, b->y + 4))) {
                b->dx = -b->dx;
            } else {
                b->x += (int16_t)b->dx;
                cx = b->x + BIRD_W / 2;
            }

            if (b->dir_timer > 0) b->dir_timer--;
            if (b->dir_timer == 0) {
                if (rand8() & 1) b->dx = -b->dx;
                b->dir_timer = (uint8_t)(BIRD_DIR_MIN +
                                (rand8() & (uint8_t)(BIRD_DIR_RANGE - 1)));
            }

            {
                uint8_t lad_col = (uint8_t)((uint16_t)cx >> 3);
                int16_t lad_cx  = (int16_t)(lad_col * 8 + 4);
                uint8_t mid_t   = tile_at_px(lad_cx, b->y + BIRD_H / 2);
                if (is_ladder(mid_t)) {
                    uint8_t can_up   = is_ladder(tile_at_px(lad_cx, b->y - 8));
                    uint8_t can_down = is_ladder(tile_at_px(lad_cx,
                                                 b->y + BIRD_H + 8));
                    int8_t  cdir = 0;
                    /* Left-hand rule: moving right → prefer up; moving left → prefer down */
                    if (b->dx > 0) {
                        if      (can_up)   cdir = -1;
                        else if (can_down) cdir =  1;
                    } else {
                        if      (can_down) cdir =  1;
                        else if (can_up)   cdir = -1;
                    }
                    if (cdir != 0) {
                        b->x = (int16_t)lad_col * 8;
                        cx   = b->x + BIRD_W / 2;
                        b->climb_dir       = cdir;
                        b->climb_start_row = (uint8_t)((uint16_t)(b->y + BIRD_H) >> 3);
                        b->climbing        = 1;
                    }
                }
            }
        }

        gcol = (uint8_t)((uint8_t)cx >> 3);
        gfy  = (uint8_t)(b->y + BIRD_H - 1);
        if (gfy < (uint8_t)(LEVEL_ROWS * 8)) {
            gidx = (uint16_t)(gfy >> 3) * LEVEL_COLS + gcol;
            if (level_state[gidx] == TILE_GRAIN) {
                level_state[gidx] = TILE_EMPTY;
                redraw_tile(gcol, (uint8_t)(gfy >> 3), TILE_EMPTY);
                b->eating    = 1;
                b->eat_timer = BIRD_EAT_FRAMES;
            }
        }

        b->anim_timer++;
        if (b->anim_timer >= BIRD_ANIM_PERIOD) {
            b->anim_timer = 0;
            b->anim_frame ^= 1;
        }
        tile = b->anim_frame ? BIRD_TILE_W1 : BIRD_TILE_W0;
        attr = (uint8_t)(1U | (b->dx < 0 ? S_FLIPX : 0U));
    }

    set_sprite_tile(oam_id, tile);
    set_sprite_prop(oam_id, attr);
    oam_y = b->y + 16;
    move_sprite(oam_id, (uint8_t)(b->x + 8), oam_y < 0 ? 0U : (uint8_t)oam_y);
}

static void birds_update(void) {
    uint8_t i;
    for (i = 0; i < active_birds; i++) bird_update_one(i);
}

/* ---- Collision ---- */

static uint8_t check_bird_collision(void) {
    uint8_t i;
    for (i = 0; i < active_birds; i++) {
        int16_t bx = birds[i].x, by = birds[i].y;
        if (harry_x + HARRY_W - COLL_INSET_H > bx + COLL_INSET_H  &&
            harry_x + COLL_INSET_H            < bx + BIRD_W - COLL_INSET_H &&
            harry_y + HARRY_H - COLL_INSET_V > by + COLL_INSET_V  &&
            harry_y + COLL_INSET_V            < by + BIRD_H - COLL_INSET_V) {
            return 1;
        }
    }
    return 0;
}

/* ---- Harry state helpers ---- */

static void respawn_harry(void) {
    harry_x          = HARRY_START_X;
    harry_y          = (int16_t)(HARRY_START_ROW * 8) - HARRY_H;
    harry_vy         = 0;
    on_ground        = 1;
    climbing         = 0;
    climb_lock       = 0;
    facing_left      = 0;
    anim_timer       = 0;
    anim_frame       = 0;
    invincible_timer = INVINCIBLE_FRAMES;
    move_sprite(0, (uint8_t)(harry_x + 8), (uint8_t)(harry_y + 16));
    game_state = STATE_PLAYING;
}

static void try_collect(int16_t px, int16_t py) {
    uint8_t  col, row, t;
    uint16_t idx;
    if (px < 0 || py < 0) return;
    col = (uint8_t)((uint16_t)px >> 3);
    row = (uint8_t)((uint16_t)py >> 3);
    if (col >= LEVEL_COLS || row >= LEVEL_ROWS) return;
    idx = (uint16_t)row * LEVEL_COLS + col;
    t   = level_state[idx];
    if (t == TILE_EGG) {
        level_state[idx] = TILE_EMPTY;
        redraw_tile(col, row, TILE_EMPTY);
        eggs_collected++;
        score += SCORE_EGG;
        hud_update();
        sfx_egg();
        if (eggs_collected >= total_eggs) {
            game_state  = STATE_LEVEL_CLEAR;
            state_timer = LEVEL_CLEAR_FRAMES;
        }
    } else if (t == TILE_GRAIN) {
        level_state[idx] = TILE_EMPTY;
        redraw_tile(col, row, TILE_EMPTY);
        grain_collected++;
        score += SCORE_GRAIN;
        hud_update();
        sfx_grain();
    }
}

/* ---- Harry per-frame update ---- */

static void harry_update(void) {
    uint8_t keys = joypad();
    uint8_t moved_h = 0;
    int16_t new_x, new_y;
    uint8_t cx = (uint8_t)(harry_x + HARRY_W / 2);
    uint8_t tile, attr;

    if (climb_lock > 0) climb_lock--;
    if (!climbing && climb_lock == 0 && (keys & (J_UP | J_DOWN))) {
        uint8_t lad_col = cx >> 3;
        uint8_t lad_t   = tile_at_px((int16_t)(lad_col * 8 + 4),
                                     harry_y + HARRY_H / 2);
        if (is_ladder(lad_t)) {
            climbing        = 1;
            harry_x         = (int16_t)lad_col * 8;
            climb_start_row = (uint8_t)((uint16_t)(harry_y + HARRY_H) >> 3);
        }
    }

    if (climbing) {
        harry_vy = 0;
        if (keys & J_UP) {
            new_y = harry_y - CLIMB_SPEED;
            if (new_y >= 0) harry_y = new_y;
        } else if (keys & J_DOWN) {
            /* clamp so feet never enter the HUD window row */
            new_y = harry_y + CLIMB_SPEED;
            if (new_y + HARRY_H <= (int16_t)(HARRY_START_ROW * 8)) harry_y = new_y;
        }
        cx = (uint8_t)(harry_x + HARRY_W / 2);
        /* auto-land when feet reach a floor tile on a DIFFERENT row from
           where the climb started — prevents instant dismount on the
           originating LADDER_PLATFORM */
        if (!(keys & J_UP)) {
            int16_t foot     = harry_y + HARRY_H;
            uint8_t foot_row = (uint8_t)((uint16_t)foot >> 3);
            if (foot_row != climb_start_row &&
                is_floor(tile_at_px((int16_t)cx, foot))) {
                harry_y   = (int16_t)foot_row * 8 - HARRY_H;
                climbing  = 0;
                on_ground = 1;
            }
        }
        if (climbing &&
            !is_ladder(tile_at_px((int16_t)cx, harry_y + HARRY_H / 2)) &&
            !is_ladder(tile_at_px((int16_t)cx, harry_y + HARRY_H - 1))) {
            climbing = 0;
        }
        if (keys & J_A) {
            climbing   = 0;
            harry_vy   = -JUMP_VEL;
            climb_lock = CLIMB_LOCK_FRAMES;
            sfx_jump();
        }
    }

    if (!climbing) {
        if (keys & J_RIGHT) {
            new_x = harry_x + WALK_SPEED;
            facing_left = 0;
            if (!is_wall(tile_at_px(new_x + HARRY_W - 1, harry_y + 4)) &&
                !is_wall(tile_at_px(new_x + HARRY_W - 1, harry_y + HARRY_H - 2))) {
                harry_x = new_x;
                moved_h = 1;
            }
        } else if (keys & J_LEFT) {
            new_x = harry_x - WALK_SPEED;
            facing_left = 1;
            if (new_x >= 0 &&
                !is_wall(tile_at_px(new_x, harry_y + 4)) &&
                !is_wall(tile_at_px(new_x, harry_y + HARRY_H - 2))) {
                harry_x = new_x;
                moved_h = 1;
            }
        }

        if ((keys & J_A) && on_ground) {
            harry_vy  = -JUMP_VEL;
            on_ground = 0;
            sfx_jump();
        }

        harry_vy++;
        if (harry_vy > MAX_FALL) harry_vy = MAX_FALL;

        new_y     = harry_y + (int16_t)harry_vy;
        on_ground = 0;

        if (harry_vy >= 0) {
            int16_t foot = new_y + HARRY_H;
            if (is_floor(tile_at_px(harry_x + 1,           foot)) ||
                is_floor(tile_at_px(harry_x + HARRY_W - 2, foot))) {
                uint8_t fr = (uint8_t)((uint16_t)foot >> 3);
                new_y      = (int16_t)fr * 8 - HARRY_H;
                harry_vy   = 0;
                on_ground  = 1;
            }
        } else {
            if (new_y < -32) {
                new_y    = -32;
                harry_vy = 0;
            } else if (new_y >= 0 &&
                       (is_ceiling(tile_at_px(harry_x + 1,           new_y)) ||
                        is_ceiling(tile_at_px(harry_x + HARRY_W - 2, new_y)))) {
                uint8_t hr = (uint8_t)((uint16_t)new_y >> 3);
                new_y    = (int16_t)(hr + 1) * 8;
                harry_vy = 0;
            }
        }
        harry_y = new_y;
    }

    if (moved_h || climbing) {
        anim_timer++;
        if (anim_timer >= ANIM_PERIOD) {
            anim_timer = 0;
            anim_frame ^= 1;
        }
    }

    try_collect(harry_x + 2,           harry_y + HARRY_H - 1);
    try_collect(harry_x + HARRY_W - 3, harry_y + HARRY_H - 1);

    tile = climbing ? HARRY_TILE_CL : (anim_frame ? HARRY_TILE_W1 : HARRY_TILE_W0);
    attr = facing_left ? S_FLIPX : 0;

    set_sprite_tile(0, tile);
    set_sprite_prop(0, attr);
    {
        int16_t oam_y = harry_y + 16;
        /* Invincibility flash: slower blink (bit 3) vs death flash (bit 2) */
        if (invincible_timer > 0) {
            invincible_timer--;
            if (invincible_timer & 8U) {
                move_sprite(0, 0, 0);
            } else {
                move_sprite(0, (uint8_t)(harry_x + 8), oam_y < 0 ? 0U : (uint8_t)oam_y);
            }
        } else {
            move_sprite(0, (uint8_t)(harry_x + 8), oam_y < 0 ? 0U : (uint8_t)oam_y);
        }
    }
}

/* ---- Entry point ---- */

void main(void) {
    uint8_t i;

    SPRITES_8x16;

    /* Sound: enable master, full volume, all channels both speakers */
    NR52_REG = 0x80;
    NR50_REG = 0x77;
    NR51_REG = 0xFF;

    set_bkg_data(0, NUM_TILES, (uint8_t *)tiles);
    set_sprite_data(HARRY_TILE_BASE, HARRY_NUM_TILES, (uint8_t *)harry_tiles);
    set_sprite_data(BIRD_TILE_BASE,  BIRD_NUM_TILES,  (uint8_t *)bird_tiles);

    set_bkg_palette(0, 8, (uint16_t *)bg_palettes);
    set_sprite_palette(0, 2, (uint16_t *)spr_palettes);

    move_win(0, 136);
    SHOW_WIN;

    harry_lives   = HARRY_START_LIVES;
    score         = 0;
    current_level = 0;
    game_state    = STATE_PLAYING;
    state_timer   = 0;

    load_level(current_level);
    birds_init(current_level);

    for (i = 1; i < 40; i++) move_sprite(i, 0, 0);

    harry_x          = HARRY_START_X;
    harry_y          = (int16_t)(HARRY_START_ROW * 8) - HARRY_H;
    harry_vy         = 0;
    on_ground        = 1;
    climbing         = 0;
    climb_lock       = 0;
    facing_left      = 0;
    anim_timer       = 0;
    anim_frame       = 0;
    invincible_timer = INVINCIBLE_FRAMES;

    hud_update();

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;

    while (1) {
        wait_vbl_done();

        switch (game_state) {

            case STATE_PLAYING:
                harry_update();
                birds_update();
                if (invincible_timer == 0 && check_bird_collision()) {
                    game_state  = STATE_DYING;
                    state_timer = DEATH_FLASH_FRAMES;
                    sfx_death();
                    if (harry_lives == 1) sfx_death_tune(FREQ_E5);
                }
                break;

            case STATE_DYING:
                /* sad tune only on the last life */
                if (harry_lives == 1) {
                    if      (state_timer == DEATH_FLASH_FRAMES - 20U) sfx_death_tune(FREQ_C5);
                    else if (state_timer == DEATH_FLASH_FRAMES - 40U) sfx_death_tune(FREQ_A4);
                    else if (state_timer == DEATH_FLASH_FRAMES - 60U) sfx_death_tune(FREQ_E4);
                }
                birds_update();
                if (state_timer & 4U) {
                    int16_t oam_y = harry_y + 16;
                    move_sprite(0, (uint8_t)(harry_x + 8),
                                   oam_y < 0 ? 0U : (uint8_t)oam_y);
                } else {
                    move_sprite(0, 0, 0);
                }
                if (state_timer > 0) {
                    state_timer--;
                } else {
                    move_sprite(0, 0, 0);
                    harry_lives--;
                    hud_update();
                    if (harry_lives == 0) {
                        game_state  = STATE_GAME_OVER;
                        state_timer = GAME_OVER_FRAMES;
                    } else {
                        respawn_harry();
                    }
                }
                break;

            case STATE_LEVEL_CLEAR:
                /* Ascending C-E-G-C fanfare spread over first ~60 frames */
                if      (state_timer == LEVEL_CLEAR_FRAMES - 1U)  sfx_note(FREQ_C5);
                else if (state_timer == LEVEL_CLEAR_FRAMES - 21U) sfx_note(FREQ_E5);
                else if (state_timer == LEVEL_CLEAR_FRAMES - 41U) sfx_note(FREQ_G5);
                else if (state_timer == LEVEL_CLEAR_FRAMES - 61U) sfx_note(FREQ_C6);
                if (state_timer > 0) {
                    state_timer--;
                } else {
                    current_level = (uint8_t)((current_level + 1U) % NUM_LEVELS);
                    load_level(current_level);
                    birds_init(current_level);
                    respawn_harry();
                    hud_update();
                }
                break;

            case STATE_GAME_OVER:
                if (state_timer > 0) {
                    state_timer--;
                } else {
                    harry_lives   = HARRY_START_LIVES;
                    score         = 0;
                    current_level = 0;
                    load_level(0);
                    birds_init(0);
                    respawn_harry();
                    hud_update();
                }
                break;
        }
    }
}
