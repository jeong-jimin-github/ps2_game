/**
 * @file render.c
 * @brief 렌더링 구현
 */
#include "render.h"
#include "animation.h"
#include "level.h"

#include <gsKit.h>

void render_background(GSGLOBAL *gsGlobal, const GameTextures *textures)
{
    if (textures->hasBackground) {
        gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->background,
                                  0.0f, 0.0f, 0.0f, 0.0f,
                                  (float)SCREEN_W, (float)SCREEN_H,
                                  (float)textures->background.Width, (float)textures->background.Height,
                                  0, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
    }
}

void render_level(GSGLOBAL *gsGlobal, const Level *level, float camX, const GameTextures *textures)
{
    int x;
    int y;
    int startX = (int)(camX / TILE_SIZE);
    int endX = (int)((camX + SCREEN_W) / TILE_SIZE) + 1;

    if (startX < 0) {
        startX = 0;
    }
    if (endX > level->width) {
        endX = level->width;
    }

    for (y = 0; y < level->height; y++) {
        for (x = startX; x < endX; x++) {
            char t = tile_at(level, x, y);
            float px = (float)(x * TILE_SIZE) - camX;
            float py = (float)(y * TILE_SIZE);

            if (t == '#') {
                if (textures->hasTileSolid) {
                    gsKit_prim_sprite(gsGlobal, px, py, px + TILE_SIZE, py + TILE_SIZE, 1,
                                      GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x80, 0x00));
                    gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->tileSolid,
                                              px, py, 0.0f, 0.0f,
                                              px + TILE_SIZE, py + TILE_SIZE,
                                              (float)textures->tileSolid.Width, (float)textures->tileSolid.Height,
                                              1, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
                } else {
                    gsKit_prim_sprite(gsGlobal, px, py, px + TILE_SIZE, py + TILE_SIZE, 1,
                                      GS_SETREG_RGBAQ(0x40, 0x40, 0x40, 0x80, 0x00));
                }
            } else if (t == 'X') {
                if (textures->hasTileTrap) {
                    gsKit_prim_sprite(gsGlobal, px, py, px + TILE_SIZE, py + TILE_SIZE, 1,
                                      GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x80, 0x00));
                    gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->tileTrap,
                                              px, py, 0.0f, 0.0f,
                                              px + TILE_SIZE, py + TILE_SIZE,
                                              (float)textures->tileTrap.Width, (float)textures->tileTrap.Height,
                                              1, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
                } else {
                    gsKit_prim_sprite(gsGlobal, px, py, px + TILE_SIZE, py + TILE_SIZE, 1,
                                      GS_SETREG_RGBAQ(0xD0, 0x20, 0x20, 0x80, 0x00));
                }
            } else if (t == 'G') {
                if (textures->hasTileGoal) {
                    gsKit_prim_sprite(gsGlobal, px, py, px + TILE_SIZE, py + TILE_SIZE, 1,
                                      GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x80, 0x00));
                    gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->tileGoal,
                                              px, py, 0.0f, 0.0f,
                                              px + TILE_SIZE, py + TILE_SIZE,
                                              (float)textures->tileGoal.Width, (float)textures->tileGoal.Height,
                                              1, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
                } else {
                    gsKit_prim_sprite(gsGlobal, px, py, px + TILE_SIZE, py + TILE_SIZE, 1,
                                      GS_SETREG_RGBAQ(0x20, 0xC0, 0x40, 0x80, 0x00));
                }
            }
        }
    }
}

void render_player(GSGLOBAL *gsGlobal,
                   const Player *p,
                   float camX,
                   const GameTextures *textures,
                   AnimationClip clips[PLAYER_ANIM_COUNT],
                   PlayerAnimState state,
                   int frameIndex,
                   int facingRight)
{
    AnimationClip *clip = get_player_clip_safe(clips, state);

    if (clip != NULL && clip->count > 0) {
        int idx = frameIndex % clip->count;
        GSTEXTURE *tex = &clip->frames[idx];
        float drawX = (p->x - camX) + ((PLAYER_W - (float)tex->Width) * 0.5f);
        float drawY = (p->y + PLAYER_H) - (float)tex->Height + 8.0f;
        float x2 = drawX + (float)tex->Width;
        float y2 = drawY + (float)tex->Height;
        float uL = facingRight ? 0.0f : (float)tex->Width;
        float uR = facingRight ? (float)tex->Width : 0.0f;
        u64 color = GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00);
        gsKit_prim_quad_texture(gsGlobal, tex,
                                drawX, drawY, uL, 0.0f,
                                x2,    drawY, uR, 0.0f,
                                drawX, y2,    uL, (float)tex->Height,
                                x2,    y2,    uR, (float)tex->Height,
                                2, color);
    } else if (textures->hasPlayer) {
        gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->player,
                                  p->x - camX, p->y, 0.0f, 0.0f,
                                  p->x - camX + PLAYER_W, p->y + PLAYER_H,
                                  (float)textures->player.Width, (float)textures->player.Height,
                                  2, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
    } else {
        gsKit_prim_sprite(gsGlobal, p->x - camX, p->y, p->x - camX + PLAYER_W, p->y + PLAYER_H, 2,
                          GS_SETREG_RGBAQ(0xFF, 0xC0, 0x20, 0x80, 0x00));
    }
}
