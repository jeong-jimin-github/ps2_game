# Copilot Instructions for ps2_game

## Project Overview

This is a PlayStation 2 homebrew 2D platformer game written in C, targeting the PS2 Emotion Engine (EE) CPU. It uses **ps2sdk**, **gsKit** (graphics), and **audsrv** (audio) libraries. The game runs natively on PS2 hardware or emulators and is distributed as an ELF binary or an ISO image.

## Repository Structure

```
main.c              - Entry point: game loop, scene management (menu / settings / play)
src/                - Game modules (see below)
assets/             - Compiled sprite assets (.ps2tex), loaded at runtime
assets/src/         - Source PNG files for sprites
levels/             - Level definition text files (tile maps)
tools/              - Python and shell helper scripts
  level_editor.py   - GUI level editor (Tkinter)
  png_to_ps2tex.py  - Convert PNG → .ps2tex (custom PS2 texture format)
  make_spritepack.py- Bundle .ps2tex files into sprites.pak
  make_iso.sh       - Build a CD-ROM ISO image
  wav_convert.py    - Convert WAV audio for PS2
Makefile            - Build system (wraps ps2sdk Makefile rules)
build.sh            - Convenience wrapper: build / iso / clean / rebuild
env.sh              - Environment variable setup (PS2DEV, PS2SDK paths)
```

## Source Modules (`src/`)

| File | Responsibility |
|---|---|
| `types.h` | All shared structs, enums, and `#define` constants (single source of truth) |
| `system.c/h` | PS2 hardware init, controller input, IOP module loading |
| `audio.c/h` | BGM streaming (`BgmStream`) and sound-effect playback via audsrv |
| `sprite.c/h` | Low-level .ps2tex loading and `SpritePack` unpacking |
| `asset.c/h` | High-level asset loading: textures, animation frames, BGM into `AssetBank` |
| `animation.c/h` | Frame-based player animation state machine (`PlayerAnimator`, `AnimationClip`) |
| `level.c/h` | Level file parsing and `LevelList` management |
| `physics.c/h` | Player movement, AABB collision, moving entities, spawned items |
| `render.c/h` | gsKit draw calls: background, tiles, player, HUD, debug overlay |
| `font.c/h` | Bitmap font rendering (uses TTF-converted glyph atlas) |
| `sfx.c/h` | Sound-effect trigger helpers |

## Build Instructions

Requires **ps2sdk** (MIPS EE cross-compiler + libraries), **MSYS2** on Windows (or a Linux PS2 dev environment), **Python 3**, and **xorriso** (for ISO creation).

```bash
bash build.sh           # Compile ELF
bash build.sh iso       # Compile ELF + create ISO
bash build.sh clean     # Remove build artifacts
bash build.sh rebuild   # Clean + full rebuild
```

The output binary is `game_engine.elf`. ISO output is `SLUS_209.99.PS2_PLATFORMER.iso`.

Asset pipeline (run automatically by `make`):
```bash
python tools/png_to_ps2tex.py --all assets/src assets   # PNG → .ps2tex
python tools/make_spritepack.py assets sprites.pak       # Bundle into pak
```

## Code Conventions

- **Language**: C99, targeting MIPS R5900 (PS2 EE). Avoid C++ features.
- **Types**: Use PS2SDK typedefs (`u8`, `u16`, `u32`, `s16`, `s32`, etc.) from `<tamtypes.h>`. Floating-point is allowed for physics/positions (`float`).
- **Constants and structs**: All shared constants (`#define`) and structs go in `src/types.h`. Do not duplicate definitions across files.
- **Header guards**: Use `#ifndef MODULE_H` / `#define MODULE_H` / `#endif /* MODULE_H */`.
- **Comments**: Write in Korean (한국어). File-level and function-level doc comments use `/** */` style.
- **Naming**: `snake_case` for functions and variables; `UPPER_SNAKE_CASE` for macros/constants; `PascalCase` for struct and enum type names.
- **No dynamic allocation in hot paths**: Avoid `malloc`/`free` per-frame. Use fixed-size arrays declared in `types.h` (e.g., `MAX_MOVING_ENTITIES`, `MAX_SPAWNED_ITEMS`).
- **No standard file I/O on PS2**: Use PS2SDK cdrom / file-system APIs (via `fioOpen`, etc.) for file access in game code. Standard `FILE *` is acceptable in host-side Python tools only.

## Level Format

Level files are plain-text tile maps in `levels/`. Each character represents a tile:

| Char | Tile |
|---|---|
| `#` | Solid block |
| `T` | Trap tile |
| `G` | Goal tile |
| `C` | Coin collectible |
| `B` | Coin block (hittable) |
| `M` | Mushroom item |
| `1` | 1-Up mushroom |
| `P` | Player spawn point |
| `<` / `>` | Moving platform (left/right) |
| `^` / `v` | Moving platform (up/down) |
| `[` / `]` | Moving trap (horizontal) |
| ` ` | Empty space |

Use `python tools/level_editor.py levels/levelN.txt` for the GUI editor.

## Key Game Constants (from `src/types.h`)

- Screen: 640×448
- Tile size: 32×32 pixels
- Player size: 24×30 pixels
- Physics: gravity `0.28f`, max fall speed `6.2f`, jump velocity `-8.4f`
- Coyote time / jump buffer: 6 frames each

## Important Notes

- The PS2 has no GPU shader pipeline — all rendering is done via gsKit primitives (sprites, quads). Avoid abstractions that assume a modern GPU.
- Audio is streamed via audsrv IOP module; BGM is a WAV file read in chunks (`BGM_SREAD_CHUNK = 8 KB`).
- Developer mode (`devMode` in `GameSettings`) enables level-skip shortcuts (R1/L1) and an on-screen debug log overlay.
- The `.gitignore` excludes `.elf`, `.iso`, `.bak`, `.vscode`, `.venv`, and `.iso_staging`.
