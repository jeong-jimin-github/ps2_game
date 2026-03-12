Texture pipeline (all textures)

1) Put PNG files in: assets/src/
2) Convert all:
   make convert-sprites
   (or) python tools/png_to_ps2tex.py --all assets/src assets
3) Run/build game. The runtime loader reads .ps2tex files in assets/

Expected source names (assets/src) and output names (assets):
- player.png -> player.ps2tex
- tile_solid.png -> tile_solid.ps2tex
- tile_trap.png -> tile_trap.ps2tex
- tile_goal.png -> tile_goal.ps2tex
- bg.png -> bg.ps2tex

Note:
- If you accidentally named it tile_soild.png, runtime fallback now also accepts tile_soild.ps2tex.
- Preferred/correct name is tile_solid.png.

Player animation source naming (state-based):
- player_idle_00.png, player_idle_01.png, ...
- player_walk_00.png, player_walk_01.png, ...
- player_run_00.png, player_run_01.png, ...
- player_clear_00.png, player_clear_01.png, ...
- player_dead_00.png, player_dead_01.png, ...
- player_hurt_00.png, player_hurt_01.png, ...

Animation loading rule:
- The game loads numbered frames from 00 upward until a frame is missing.
- If numbered frames are missing, it tries single file fallback:
   player_idle.png, player_walk.png, ...
- Maximum frames per state: 16

PNG size guide:
- player.png: recommended 32x32 or 64x64 (rendered at 24x30 in game)
- player_idle_XX.png: 32x32 or 64x64
- player_walk_XX.png: 32x32 or 64x64
- player_run_XX.png: 32x32 or 64x64
- player_clear_XX.png: 32x32 or 64x64
- player_dead_XX.png: 32x32 or 64x64
- player_hurt_XX.png: 32x32 or 64x64
- tile_solid.png: recommended 32x32 (1 tile)
- tile_trap.png: recommended 32x32 (1 tile)
- tile_goal.png: recommended 32x32 (1 tile)
- bg.png: recommended 640x448 (screen size) or 1024x512
- General limit in converter: width/height each <= 1024

File format (.ps2tex):
- 4 bytes magic: P2TX
- 2 bytes width (little-endian)
- 2 bytes height (little-endian)
- width*height*4 bytes RGBA32 pixel data

In-game behavior:
- If a texture file exists and loads, the texture is rendered.
- If a texture file is missing, color fallback rendering is used.

Transparency note:
- Converter now clears RGB for very low alpha pixels (alpha < 8) to reduce halo/background artifacts.
- If PNG transparency looks wrong, reconvert textures again after edits:
   make convert-sprites
