/**
 * @file render.c
 * @brief 렌더링 구현
 */
#include "render.h"
#include "animation.h"
#include "level.h"
#include "font.h"

#include <gsKit.h>
#include <stdio.h>

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
                    gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->tileTrap,
                                              px, py, 0.0f, 0.0f,
                                              px + TILE_SIZE, py + TILE_SIZE,
                                              (float)textures->tileTrap.Width, (float)textures->tileTrap.Height,
                                              2, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
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
            } else if (t == '?') {
                /* 코인 블록 */
                if (textures->hasTileCoinBlock) {
                    gsKit_prim_sprite(gsGlobal, px, py, px + TILE_SIZE, py + TILE_SIZE, 1,
                                      GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x80, 0x00));
                    gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->tileCoinBlock,
                                              px, py, 0.0f, 0.0f,
                                              px + TILE_SIZE, py + TILE_SIZE,
                                              (float)textures->tileCoinBlock.Width, (float)textures->tileCoinBlock.Height,
                                              1, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
                } else {
                    gsKit_prim_sprite(gsGlobal, px, py, px + TILE_SIZE, py + TILE_SIZE, 1,
                                      GS_SETREG_RGBAQ(0xE0, 0xB0, 0x20, 0x80, 0x00));
                    gsKit_prim_sprite(gsGlobal, px + 6, py + 6, px + TILE_SIZE - 6, py + TILE_SIZE - 6, 1,
                                      GS_SETREG_RGBAQ(0xFF, 0xD7, 0x00, 0x80, 0x00));
                }
            } else if (t == 'E') {
                /* 사용된 빈 블록 */
                if (textures->hasTileEmptyBlock) {
                    gsKit_prim_sprite(gsGlobal, px, py, px + TILE_SIZE, py + TILE_SIZE, 1,
                                      GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x80, 0x00));
                    gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->tileEmptyBlock,
                                              px, py, 0.0f, 0.0f,
                                              px + TILE_SIZE, py + TILE_SIZE,
                                              (float)textures->tileEmptyBlock.Width, (float)textures->tileEmptyBlock.Height,
                                              1, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
                } else {
                    gsKit_prim_sprite(gsGlobal, px, py, px + TILE_SIZE, py + TILE_SIZE, 1,
                                      GS_SETREG_RGBAQ(0x50, 0x40, 0x30, 0x80, 0x00));
                }
            } else if (t == 'C') {
                /* 코인 */
                if (textures->hasTileCoin) {
                    gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->tileCoin,
                                              px, py, 0.0f, 0.0f,
                                              px + TILE_SIZE, py + TILE_SIZE,
                                              (float)textures->tileCoin.Width, (float)textures->tileCoin.Height,
                                              2, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
                } else {
                    gsKit_prim_sprite(gsGlobal, px + 8, py + 4, px + TILE_SIZE - 8, py + TILE_SIZE - 4, 1,
                                      GS_SETREG_RGBAQ(0xFF, 0xD7, 0x00, 0x80, 0x00));
                }
            } else if (t == 'M') {
                /* 버섯 파워업 */
                if (textures->hasTileMushroom) {
                    gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->tileMushroom,
                                              px, py, 0.0f, 0.0f,
                                              px + TILE_SIZE, py + TILE_SIZE,
                                              (float)textures->tileMushroom.Width, (float)textures->tileMushroom.Height,
                                              2, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
                } else {
                    gsKit_prim_sprite(gsGlobal, px + 4, py + 2, px + TILE_SIZE - 4, py + TILE_SIZE / 2, 1,
                                      GS_SETREG_RGBAQ(0xE0, 0x30, 0x30, 0x80, 0x00));
                    gsKit_prim_sprite(gsGlobal, px + 8, py + TILE_SIZE / 2, px + TILE_SIZE - 8, py + TILE_SIZE - 2, 1,
                                      GS_SETREG_RGBAQ(0xF0, 0xE0, 0xC0, 0x80, 0x00));
                }
            } else if (t == '1') {
                /* 1UP */
                if (textures->hasTile1up) {
                    gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->tile1up,
                                              px, py, 0.0f, 0.0f,
                                              px + TILE_SIZE, py + TILE_SIZE,
                                              (float)textures->tile1up.Width, (float)textures->tile1up.Height,
                                              2, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
                } else {
                    gsKit_prim_sprite(gsGlobal, px + 4, py + 2, px + TILE_SIZE - 4, py + TILE_SIZE / 2, 1,
                                      GS_SETREG_RGBAQ(0x30, 0xC0, 0x30, 0x80, 0x00));
                    gsKit_prim_sprite(gsGlobal, px + 8, py + TILE_SIZE / 2, px + TILE_SIZE - 8, py + TILE_SIZE - 2, 1,
                                      GS_SETREG_RGBAQ(0xF0, 0xE0, 0xC0, 0x80, 0x00));
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

/* ── 이동 엔티티 렌더링 ─────────────────────────── */
void render_moving_entities(GSGLOBAL *gsGlobal, const GameWorld *world, float camX, const GameTextures *textures)
{
    int i;
    (void)textures;

    for (i = 0; i < world->movingEntCount; i++) {
        const MovingEntity *ent = &world->movingEnts[i];
        float px, py;
        if (!ent->active) continue;

        px = ent->x - camX;
        py = ent->y;

        if (px + ent->width < 0 || px > SCREEN_W) continue;

        if (ent->isTrap) {
            /* 이동 함정 - 빨간색 */
            gsKit_prim_sprite(gsGlobal, px, py, px + ent->width, py + ent->height, 1,
                              GS_SETREG_RGBAQ(0xC0, 0x20, 0x20, 0x80, 0x00));
            gsKit_prim_sprite(gsGlobal, px + 2, py + 2, px + ent->width - 2, py + ent->height - 2, 1,
                              GS_SETREG_RGBAQ(0xFF, 0x40, 0x40, 0x80, 0x00));
        } else {
            /* 이동 플랫폼 - 파란색 */
            gsKit_prim_sprite(gsGlobal, px, py, px + ent->width, py + ent->height, 1,
                              GS_SETREG_RGBAQ(0x30, 0x60, 0xA0, 0x80, 0x00));
            gsKit_prim_sprite(gsGlobal, px + 2, py + 2, px + ent->width - 2, py + ent->height - 2, 1,
                              GS_SETREG_RGBAQ(0x50, 0x90, 0xD0, 0x80, 0x00));
        }
    }
}

/* ── 스폰된 아이템 렌더링 ────────────────────────── */
void render_spawned_items(GSGLOBAL *gsGlobal, const GameWorld *world, float camX, const GameTextures *textures)
{
    int i;

    for (i = 0; i < world->itemCount; i++) {
        const SpawnedItem *item = &world->items[i];
        float px, py;
        if (!item->active) continue;

        px = item->x - camX;
        py = item->y;

        if (px + TILE_SIZE < 0 || px > SCREEN_W) continue;

        if (item->type == 'C') {
            /* 바운스 코인 */
            if (textures->hasTileCoin) {
                gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->tileCoin,
                                          px, py, 0.0f, 0.0f,
                                          px + TILE_SIZE, py + TILE_SIZE,
                                          (float)textures->tileCoin.Width, (float)textures->tileCoin.Height,
                                          2, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
            } else {
                gsKit_prim_sprite(gsGlobal, px + 8, py + 4, px + TILE_SIZE - 8, py + TILE_SIZE - 4, 2,
                                  GS_SETREG_RGBAQ(0xFF, 0xD7, 0x00, 0x80, 0x00));
            }
        } else if (item->type == 'M') {
            /* 버섯 */
            if (textures->hasTileMushroom) {
                gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->tileMushroom,
                                          px, py, 0.0f, 0.0f,
                                          px + TILE_SIZE, py + TILE_SIZE,
                                          (float)textures->tileMushroom.Width, (float)textures->tileMushroom.Height,
                                          2, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
            } else {
                gsKit_prim_sprite(gsGlobal, px + 4, py + 2, px + TILE_SIZE - 4, py + TILE_SIZE / 2, 2,
                                  GS_SETREG_RGBAQ(0xE0, 0x30, 0x30, 0x80, 0x00));
                gsKit_prim_sprite(gsGlobal, px + 8, py + TILE_SIZE / 2, px + TILE_SIZE - 8, py + TILE_SIZE - 2, 2,
                                  GS_SETREG_RGBAQ(0xF0, 0xE0, 0xC0, 0x80, 0x00));
            }
        } else if (item->type == '1') {
            /* 1UP */
            if (textures->hasTile1up) {
                gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->tile1up,
                                          px, py, 0.0f, 0.0f,
                                          px + TILE_SIZE, py + TILE_SIZE,
                                          (float)textures->tile1up.Width, (float)textures->tile1up.Height,
                                          2, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
            } else {
                gsKit_prim_sprite(gsGlobal, px + 4, py + 2, px + TILE_SIZE - 4, py + TILE_SIZE / 2, 2,
                                  GS_SETREG_RGBAQ(0x30, 0xC0, 0x30, 0x80, 0x00));
                gsKit_prim_sprite(gsGlobal, px + 8, py + TILE_SIZE / 2, px + TILE_SIZE - 8, py + TILE_SIZE - 2, 2,
                                  GS_SETREG_RGBAQ(0xF0, 0xE0, 0xC0, 0x80, 0x00));
            }
        }
    }
}

/* ── HUD 렌더링 ──────────────────────────────────── */
/* ── 7-세그먼트 숫자 렌더러 ──────────────────────── */
/*
 *  Segment layout (each digit is sw x sh pixels):
 *   _0_
 *  |   |
 *  5   1
 *  |_6_|
 *  |   |
 *  4   2
 *  |_3_|
 *
 *  Segments for each digit (bitmask: bit0=top .. bit6=middle):
 */
static const unsigned char SEGS[10] = {
    0x3F, /* 0: 0,1,2,3,4,5    */
    0x06, /* 1: 1,2              */
    0x5B, /* 2: 0,1,3,4,6       */
    0x4F, /* 3: 0,1,2,3,6       */
    0x66, /* 4: 1,2,5,6         */
    0x6D, /* 5: 0,2,3,5,6       */
    0x7D, /* 6: 0,2,3,4,5,6     */
    0x07, /* 7: 0,1,2            */
    0x7F, /* 8: all              */
    0x6F, /* 9: 0,1,2,3,5,6     */
};

static void draw_digit(GSGLOBAL *gs, float x, float y, int digit, u64 color)
{
    unsigned char s;
    /* digit size: 10w x 16h, bar thickness 3 */
    const float W = 10.0f, H = 16.0f, T = 3.0f;

    if (digit < 0 || digit > 9) return;
    s = SEGS[digit];

    /* seg 0: top horizontal */
    if (s & 0x01)
        gsKit_prim_sprite(gs, x + T, y, x + W - T, y + T, 4, color);
    /* seg 1: top-right vertical */
    if (s & 0x02)
        gsKit_prim_sprite(gs, x + W - T, y, x + W, y + H / 2, 4, color);
    /* seg 2: bottom-right vertical */
    if (s & 0x04)
        gsKit_prim_sprite(gs, x + W - T, y + H / 2, x + W, y + H, 4, color);
    /* seg 3: bottom horizontal */
    if (s & 0x08)
        gsKit_prim_sprite(gs, x + T, y + H - T, x + W - T, y + H, 4, color);
    /* seg 4: bottom-left vertical */
    if (s & 0x10)
        gsKit_prim_sprite(gs, x, y + H / 2, x + T, y + H, 4, color);
    /* seg 5: top-left vertical */
    if (s & 0x20)
        gsKit_prim_sprite(gs, x, y, x + T, y + H / 2, 4, color);
    /* seg 6: middle horizontal */
    if (s & 0x40)
        gsKit_prim_sprite(gs, x + T, y + H / 2 - T / 2, x + W - T, y + H / 2 + T / 2, 4, color);
}

static void draw_number(GSGLOBAL *gs, float x, float y, int value, u64 color)
{
    char buf[12];
    int len, i;

    if (value < 0) value = 0;
    /* 숫자를 문자열로 변환 */
    len = 0;
    if (value == 0) {
        buf[len++] = 0;
    } else {
        int tmp = value;
        while (tmp > 0 && len < 10) {
            buf[len++] = tmp % 10;
            tmp /= 10;
        }
        /* reverse */
        for (i = 0; i < len / 2; i++) {
            char t = buf[i];
            buf[i] = buf[len - 1 - i];
            buf[len - 1 - i] = t;
        }
    }

    for (i = 0; i < len; i++) {
        draw_digit(gs, x + (float)(i * 14), y, buf[i], color);
    }
}

void render_hud(GSGLOBAL *gsGlobal, const Player *player, const BitmapFont *font)
{
    float hx = 16.0f;
    float hy = 12.0f;
    u64 white  = GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00);
    u64 red    = GS_SETREG_RGBAQ(0xFF, 0x30, 0x30, 0x80, 0x00);
    u64 yellow = GS_SETREG_RGBAQ(0xFF, 0xD7, 0x00, 0x80, 0x00);
    char buf[32];

    if (font && font->texValid) {
        /* 하트 아이콘 */
        gsKit_prim_sprite(gsGlobal, hx, hy, hx + 14.0f, hy + 14.0f, 4, red);
        snprintf(buf, sizeof(buf), "x%d", player->lives);
        font_draw_string(gsGlobal, font, hx + 18.0f, hy - 2.0f, 5, white, buf);

        /* 코인 아이콘 */
        {
            float cy = hy + 22.0f;
            gsKit_prim_sprite(gsGlobal, hx, cy, hx + 14.0f, cy + 14.0f, 4, yellow);
            snprintf(buf, sizeof(buf), "x%d", player->coins);
            font_draw_string(gsGlobal, font, hx + 18.0f, cy - 2.0f, 5, white, buf);
        }

        /* 파워업 상태 */
        if (player->powered) {
            font_draw_string(gsGlobal, font, hx, hy + 44.0f, 5,
                             GS_SETREG_RGBAQ(0x40, 0xA0, 0xFF, 0x80, 0x00),
                             "POWER");
        }
    } else {
        /* 폰트 없을 때 폴백: 7-세그먼트 숫자 */
        gsKit_prim_sprite(gsGlobal, hx, hy, hx + 14.0f, hy + 14.0f, 4, red);
        gsKit_prim_sprite(gsGlobal, hx + 18.0f, hy + 5.0f, hx + 22.0f, hy + 9.0f, 4, white);
        draw_number(gsGlobal, hx + 26.0f, hy, player->lives, white);

        {
            float cy = hy + 22.0f;
            gsKit_prim_sprite(gsGlobal, hx, cy, hx + 14.0f, cy + 14.0f, 4, yellow);
            gsKit_prim_sprite(gsGlobal, hx + 18.0f, cy + 5.0f, hx + 22.0f, cy + 9.0f, 4, white);
            draw_number(gsGlobal, hx + 26.0f, cy, player->coins, white);
        }

        if (player->powered) {
            gsKit_prim_sprite(gsGlobal, hx, hy + 44.0f, hx + 14.0f, hy + 58.0f, 4,
                              GS_SETREG_RGBAQ(0xE0, 0x30, 0x30, 0x80, 0x00));
            gsKit_prim_sprite(gsGlobal, hx + 16.0f, hy + 44.0f, hx + 30.0f, hy + 58.0f, 4,
                              GS_SETREG_RGBAQ(0x40, 0xA0, 0xFF, 0x80, 0x00));
        }
    }
}

/* ── 메인 메뉴 ───────────────────────────────────── */
void render_main_menu(GSGLOBAL *gsGlobal, const GameTextures *textures,
                      const BitmapFont *font, int selectedIndex)
{
    u64 white   = GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00);
    u64 yellow  = GS_SETREG_RGBAQ(0xFF, 0xD7, 0x00, 0x80, 0x00);
    u64 cyan    = GS_SETREG_RGBAQ(0x50, 0xC0, 0xFF, 0x80, 0x00);
    u64 dimBg   = GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x50, 0x00);
    u64 panelBorder = GS_SETREG_RGBAQ(0x20, 0x70, 0xD0, 0x80, 0x00);
    float cx = (float)SCREEN_W / 2.0f;
    const char *items[2];
    int i;
    float startY;

    items[0] = "START GAME";
    items[1] = "SETTINGS";

    /* 메뉴 전용 배경 */
    if (textures->hasMenuBackground) {
        gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->menuBackground,
                                  0.0f, 0.0f, 0.0f, 0.0f,
                                  (float)SCREEN_W, (float)SCREEN_H,
                                  (float)textures->menuBackground.Width, (float)textures->menuBackground.Height,
                                  0, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
    }

    /* 반투명 패널 */
    gsKit_prim_sprite(gsGlobal, cx - 180.0f, 88.0f, cx + 180.0f, 360.0f, 1, dimBg);
    gsKit_prim_sprite(gsGlobal, cx - 180.0f, 88.0f, cx + 180.0f, 92.0f, 2, panelBorder);
    gsKit_prim_sprite(gsGlobal, cx - 180.0f, 356.0f, cx + 180.0f, 360.0f, 2, panelBorder);
    gsKit_prim_sprite(gsGlobal, cx - 180.0f, 88.0f, cx - 176.0f, 360.0f, 2, panelBorder);
    gsKit_prim_sprite(gsGlobal, cx + 176.0f, 88.0f, cx + 180.0f, 360.0f, 2, panelBorder);

    /* 타이틀 */
    if (font && font->texValid) {
        const char *title = "PS2 TEST GAME";
        int tw = font_measure_string(font, title);
        const char *subtitle = "PRESS O TO START";
        int sw = font_measure_string(font, subtitle);
        font_draw_string(gsGlobal, font, cx - (float)tw / 2.0f, 122.0f, 5, cyan, title);
        font_draw_string(gsGlobal, font, cx - (float)sw / 2.0f, 154.0f, 5, white, subtitle);
    }

    /* 메뉴 항목 */
    startY = 218.0f;
    for (i = 0; i < 2; i++) {
        u64 color = (i == selectedIndex) ? yellow : white;
        if (font && font->texValid) {
            int mw = font_measure_string(font, items[i]);
            float y = startY + (float)(i * 40);
            if (i == selectedIndex) {
                gsKit_prim_sprite(gsGlobal, cx - 128.0f, y - 2.0f, cx + 128.0f, y + 22.0f, 2,
                                  GS_SETREG_RGBAQ(0x20, 0x20, 0x20, 0x50, 0x00));
                font_draw_string(gsGlobal, font, cx - (float)mw / 2.0f - 20.0f, y, 5, yellow, ">");
            }
            font_draw_string(gsGlobal, font, cx - (float)mw / 2.0f, y, 5, color, items[i]);
        }
    }

    if (font && font->texValid) {
        const char *footer = "D-PAD: MOVE / TRIANGLE: BACK";
        int fw = font_measure_string(font, footer);
        font_draw_string(gsGlobal, font, cx - (float)fw / 2.0f, 320.0f, 5, white, footer);
    }
}

/* ── 설정 메뉴 ───────────────────────────────────── */
void render_settings_menu(GSGLOBAL *gsGlobal, const GameTextures *textures,
                          const BitmapFont *font, const GameSettings *settings,
                          int selectedIndex)
{
    u64 white   = GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00);
    u64 yellow  = GS_SETREG_RGBAQ(0xFF, 0xD7, 0x00, 0x80, 0x00);
    u64 green   = GS_SETREG_RGBAQ(0x30, 0xD0, 0x30, 0x80, 0x00);
    u64 red     = GS_SETREG_RGBAQ(0xD0, 0x30, 0x30, 0x80, 0x00);
    u64 dimBg   = GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x50, 0x00);
    u64 panelBorder = GS_SETREG_RGBAQ(0x20, 0x70, 0xD0, 0x80, 0x00);
    float cx = (float)SCREEN_W / 2.0f;
    char buf[64];
    int i;
    float startY;
    const char *labels[3];

    labels[2] = "BACK";

    /* 메뉴 전용 배경 */
    if (textures->hasMenuBackground) {
        gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->menuBackground,
                                  0.0f, 0.0f, 0.0f, 0.0f,
                                  (float)SCREEN_W, (float)SCREEN_H,
                                  (float)textures->menuBackground.Width, (float)textures->menuBackground.Height,
                                  0, GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
    }

    /* 반투명 패널 */
    gsKit_prim_sprite(gsGlobal, cx - 180.0f, 100.0f, cx + 180.0f, 380.0f, 1, dimBg);
    gsKit_prim_sprite(gsGlobal, cx - 180.0f, 100.0f, cx + 180.0f, 104.0f, 2, panelBorder);
    gsKit_prim_sprite(gsGlobal, cx - 180.0f, 376.0f, cx + 180.0f, 380.0f, 2, panelBorder);
    gsKit_prim_sprite(gsGlobal, cx - 180.0f, 100.0f, cx - 176.0f, 380.0f, 2, panelBorder);
    gsKit_prim_sprite(gsGlobal, cx + 176.0f, 100.0f, cx + 180.0f, 380.0f, 2, panelBorder);

    /* 타이틀 */
    if (font && font->texValid) {
        const char *title = "SETTINGS";
        int tw = font_measure_string(font, title);
        font_draw_string(gsGlobal, font, cx - (float)tw / 2.0f, 120.0f, 5, white, title);
    }

    startY = 180.0f;
    for (i = 0; i < 3; i++) {
        u64 color = (i == selectedIndex) ? yellow : white;
        float y = startY + (float)(i * 50);

        if (!font || !font->texValid) continue;

        if (i == 0) {
            /* Sound: ON/OFF */
            const char *label = "Sound : ";
            const char *val = settings->soundOn ? "ON" : "OFF";
            u64 valColor = settings->soundOn ? green : red;
            snprintf(buf, sizeof(buf), "%s", label);
            int lw = font_measure_string(font, buf);
            int vw = font_measure_string(font, val);
            float totalW = (float)(lw + vw);
            float sx = cx - totalW / 2.0f;
            if (i == selectedIndex) {
                gsKit_prim_sprite(gsGlobal, cx - 152.0f, y - 2.0f, cx + 152.0f, y + 22.0f, 2,
                                  GS_SETREG_RGBAQ(0x20, 0x20, 0x20, 0x50, 0x00));
                font_draw_string(gsGlobal, font, sx - 20.0f, y, 5, yellow, ">");
            }
            font_draw_string(gsGlobal, font, sx, y, 5, color, buf);
            font_draw_string(gsGlobal, font, sx + (float)lw, y, 5, valColor, val);
        } else if (i == 1) {
            /* Developer Mode: ON/OFF */
            const char *label = "Dev Mode : ";
            const char *val = settings->devMode ? "ON" : "OFF";
            u64 valColor = settings->devMode ? green : red;
            snprintf(buf, sizeof(buf), "%s", label);
            int lw = font_measure_string(font, buf);
            int vw = font_measure_string(font, val);
            float totalW = (float)(lw + vw);
            float sx = cx - totalW / 2.0f;
            if (i == selectedIndex) {
                gsKit_prim_sprite(gsGlobal, cx - 152.0f, y - 2.0f, cx + 152.0f, y + 22.0f, 2,
                                  GS_SETREG_RGBAQ(0x20, 0x20, 0x20, 0x50, 0x00));
                font_draw_string(gsGlobal, font, sx - 20.0f, y, 5, yellow, ">");
            }
            font_draw_string(gsGlobal, font, sx, y, 5, color, buf);
            font_draw_string(gsGlobal, font, sx + (float)lw, y, 5, valColor, val);
        } else {
            /* 뒤로 */
            int lw = font_measure_string(font, labels[2]);
            if (i == selectedIndex) {
                gsKit_prim_sprite(gsGlobal, cx - 152.0f, y - 2.0f, cx + 152.0f, y + 22.0f, 2,
                                  GS_SETREG_RGBAQ(0x20, 0x20, 0x20, 0x50, 0x00));
                font_draw_string(gsGlobal, font, cx - (float)lw/2.0f - 20.0f, y, 5, yellow, ">");
            }
            font_draw_string(gsGlobal, font, cx - (float)lw / 2.0f, y, 5, color, labels[2]);
        }
    }

    if (font && font->texValid) {
        const char *footer = "X/O: APPLY   TRIANGLE: BACK";
        int fw = font_measure_string(font, footer);
        font_draw_string(gsGlobal, font, cx - (float)fw / 2.0f, 338.0f, 5, white, footer);
    }
}

/* ── 개발자 HUD 로그 ─────────────────────────────── */
void render_dev_log(GSGLOBAL *gsGlobal, const BitmapFont *font, const DevLog *dlog)
{
    int i, idx;
    float y;
    u64 green = GS_SETREG_RGBAQ(0x30, 0xFF, 0x30, 0x60, 0x00);
    u64 bg    = GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x30, 0x00);

    if (!font || !font->texValid || !dlog || dlog->count == 0) return;

    /* 반투명 배경 */
    gsKit_prim_sprite(gsGlobal, 0.0f, (float)SCREEN_H - (float)(DEV_LOG_LINES * 18 + 4),
                      (float)SCREEN_W, (float)SCREEN_H, 4, bg);

    y = (float)SCREEN_H - (float)(DEV_LOG_LINES * 18);
    for (i = 0; i < dlog->count && i < DEV_LOG_LINES; i++) {
        idx = (dlog->head - dlog->count + i + DEV_LOG_LINES) % DEV_LOG_LINES;
        font_draw_string(gsGlobal, font, 8.0f, y + (float)(i * 18), 5, green, dlog->lines[idx]);
    }
}

void render_text(GSGLOBAL *gsGlobal, const BitmapFont *font, float x, float y, const char *text, u64 color)
{
    if (font && font->texValid) {
        font_draw_string(gsGlobal, font, x, y, 5, color, text);
    }
}

void render_loading_screen(GSGLOBAL *gsGlobal, const GameTextures *textures, const BitmapFont *font)
{
    u64 white = GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00);

    gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x80, 0x00));

    if (textures->hasMenuBackground) {
        gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&textures->menuBackground,
                                  0.0f, 0.0f, 0.0f, 0.0f,
                                  (float)SCREEN_W, (float)SCREEN_H,
                                  (float)textures->menuBackground.Width, (float)textures->menuBackground.Height,
                                  0, white);
    }

    if (font && font->texValid) {
        const char *msg = "Loading...";
        int tw = font_measure_string(font, msg);
        float cx = (float)SCREEN_W / 2.0f;
        float cy = (float)SCREEN_H / 2.0f + 80.0f;
        font_draw_string(gsGlobal, font, cx - (float)tw / 2.0f, cy, 5, white, msg);
    }

    gsKit_queue_exec(gsGlobal);
    gsKit_sync_flip(gsGlobal);
}
