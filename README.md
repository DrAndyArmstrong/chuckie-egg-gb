# Chuckie Egg GB

A Game Boy Color port of the classic 1983 BBC Micro game **Chuckie Egg** by A&F Software.

The original game was written by Nigel Alderton. This port is a fan project built with [GBDK-2020](https://github.com/gbdk-2020/gbdk-2020) and is not affiliated with or endorsed by the original rights holders.

---

## Play it now

Open `web/index.html` in a browser — it loads the compiled ROM in an in-browser emulator with no install required. Or drag `roms/chuckie.gb` into any Game Boy emulator.

---

## Controls

| Button | Action |
|--------|--------|
| D-Pad Left / Right | Walk |
| D-Pad Up / Down | Grab and climb a ladder |
| A | Jump (also jumps off a ladder) |

---

## Gameplay

- Collect all the **eggs** on each level to advance to the next
- Avoid the **hens** — touching one costs a life
- Pick up **grain** for bonus points
- You start with **5 lives**; a short invincibility window protects you on each spawn
- All **8 original BBC scenarios** are included, each with authentic hen starting positions and counts taken from the original source data

### Scoring

| Item | Points |
|------|--------|
| Egg | 250 |
| Grain | 50 |

---

## Building from source

### Prerequisites

1. **GBDK-2020** — download the latest release from [github.com/gbdk-2020/gbdk-2020/releases](https://github.com/gbdk-2020/gbdk-2020/releases) and unpack it into an `sdk/gbdk/` folder inside this repo:

   ```
   chuckie-egg-gb/
   └── sdk/
       └── gbdk/
           ├── bin/
           │   └── lcc
           └── ...
   ```

2. **Python 3** — needed to regenerate level data from the BBC source (optional; `src/chuckie/level_data.h` is already included).

### Build

```bash
make
```

This produces `roms/chuckie.gb`.

If you want to point at a GBDK install elsewhere:

```bash
make GBDK_HOME=/path/to/gbdk
```

To regenerate the level data from the BBC Micro source file:

```bash
python3 tools/convert_levels.py > src/chuckie/level_data.h
```

### Clean

```bash
make clean
```

---

## Project structure

```
src/chuckie/
  main.c          — game loop, Harry, birds, HUD, sound, game states
  level_data.h    — auto-generated 20×18 tile maps for all 8 levels
  bird_data.h     — per-level hen starting positions and counts
  tileset.h       — background tile pixel data (platforms, eggs, HUD digits)
  harry.h         — Harry sprite pixel data (walk, climb frames)
  bird_tiles.h    — hen sprite pixel data (walk, eat frames)

tools/
  convert_levels.py — converts scenes.basm BBC source into level_data.h

scenes.basm       — original BBC Micro level data (walls, ladders, eggs, hens)
sprites.basm      — original BBC Micro sprite data (reference)

roms/
  chuckie.gb      — compiled Game Boy Color ROM

web/
  index.html      — browser-based player (EmulatorJS)
```

---

## Acknowledgements

- Original game by **Nigel Alderton** (1983, A&F Software)
- BBC Micro disassembly by [mungre](https://github.com/mungre/chuckie) — the `scenes.basm` and `sprites.basm` files are derived from this work
- Built with [GBDK-2020](https://github.com/gbdk-2020/gbdk-2020)
- In-browser emulation via [EmulatorJS](https://emulatorjs.org)
