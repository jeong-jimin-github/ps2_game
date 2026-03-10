/**
 * @file physics.c
 * @brief 플레이어 물리/충돌 구현
 */
#include "physics.h"
#include "level.h"

int intersects_goal(const Level *level, const Player *p)
{
    int left = (int)(p->x / TILE_SIZE);
    int right = (int)((p->x + PLAYER_W - 1.0f) / TILE_SIZE);
    int top = (int)(p->y / TILE_SIZE);
    int bottom = (int)((p->y + PLAYER_H - 1.0f) / TILE_SIZE);
    int x;
    int y;

    for (y = top; y <= bottom; y++) {
        for (x = left; x <= right; x++) {
            if (is_goal(tile_at(level, x, y))) {
                return 1;
            }
        }
    }

    return 0;
}

int intersects_trap(const Level *level, const Player *p)
{
    int left = (int)(p->x / TILE_SIZE);
    int right = (int)((p->x + PLAYER_W - 1.0f) / TILE_SIZE);
    int top = (int)(p->y / TILE_SIZE);
    int bottom = (int)((p->y + PLAYER_H - 1.0f) / TILE_SIZE);
    int x;
    int y;

    for (y = top; y <= bottom; y++) {
        for (x = left; x <= right; x++) {
            if (is_trap(tile_at(level, x, y))) {
                return 1;
            }
        }
    }

    return 0;
}

void move_player_x(const Level *level, Player *p)
{
    int top;
    int bottom;
    int checkX;
    int y;

    p->x += p->vx;

    top = (int)(p->y / TILE_SIZE);
    bottom = (int)((p->y + PLAYER_H - 1.0f) / TILE_SIZE);

    if (p->vx > 0.0f) {
        checkX = (int)((p->x + PLAYER_W - 1.0f) / TILE_SIZE);
        for (y = top; y <= bottom; y++) {
            if (is_solid(tile_at(level, checkX, y))) {
                p->x = (float)(checkX * TILE_SIZE) - PLAYER_W;
                p->vx = 0.0f;
                break;
            }
        }
    } else if (p->vx < 0.0f) {
        checkX = (int)(p->x / TILE_SIZE);
        for (y = top; y <= bottom; y++) {
            if (is_solid(tile_at(level, checkX, y))) {
                p->x = (float)((checkX + 1) * TILE_SIZE);
                p->vx = 0.0f;
                break;
            }
        }
    }
}

void move_player_y(const Level *level, Player *p)
{
    int left;
    int right;
    int checkY;
    int x;

    p->y += p->vy;
    p->onGround = 0;

    left = (int)(p->x / TILE_SIZE);
    right = (int)((p->x + PLAYER_W - 1.0f) / TILE_SIZE);

    if (p->vy > 0.0f) {
        checkY = (int)((p->y + PLAYER_H - 1.0f) / TILE_SIZE);
        for (x = left; x <= right; x++) {
            if (is_solid(tile_at(level, x, checkY))) {
                p->y = (float)(checkY * TILE_SIZE) - PLAYER_H;
                p->vy = 0.0f;
                p->onGround = 1;
                break;
            }
        }
    } else if (p->vy < 0.0f) {
        checkY = (int)(p->y / TILE_SIZE);
        for (x = left; x <= right; x++) {
            if (is_solid(tile_at(level, x, checkY))) {
                p->y = (float)((checkY + 1) * TILE_SIZE);
                p->vy = 0.0f;
                break;
            }
        }
    }
}

int check_grounded(const Level *level, const Player *p)
{
    int left = (int)(p->x / TILE_SIZE);
    int right = (int)((p->x + PLAYER_W - 1.0f) / TILE_SIZE);
    int probeY = (int)((p->y + PLAYER_H) / TILE_SIZE);
    int x;

    for (x = left; x <= right; x++) {
        if (is_solid(tile_at(level, x, probeY))) {
            return 1;
        }
    }

    return 0;
}

void reset_player_to_spawn(const Level *level, Player *player)
{
    player->x = level->spawnX;
    player->y = level->spawnY;
    player->vx = 0.0f;
    player->vy = 0.0f;
    player->onGround = 0;
    player->facingRight = 1;
    player->coyoteTimer = 0;
    player->jumpBufferTimer = 0;
}

float clampf(float v, float minV, float maxV)
{
    if (v < minV) {
        return minV;
    }
    if (v > maxV) {
        return maxV;
    }
    return v;
}
