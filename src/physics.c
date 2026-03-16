/**
 * @file physics.c
 * @brief 플레이어 물리/충돌 구현
 */
#include "physics.h"
#include "level.h"

#include <string.h>

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
    player->invincTimer = 0;
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

/* ── 수집 가능 아이템 교차 ───────────────────────── */
char intersects_collectible(Level *level, const Player *p)
{
    int left = (int)(p->x / TILE_SIZE);
    int right = (int)((p->x + PLAYER_W - 1.0f) / TILE_SIZE);
    int top = (int)(p->y / TILE_SIZE);
    int bottom = (int)((p->y + PLAYER_H - 1.0f) / TILE_SIZE);
    int x, y;

    for (y = top; y <= bottom; y++) {
        for (x = left; x <= right; x++) {
            char t = tile_at(level, x, y);
            if (is_collectible(t)) {
                level->tiles[y][x] = '.';
                return t;
            }
        }
    }
    return 0;
}

/* ── 코인 블록 머리 부딪힘 ───────────────────────── */
int check_head_bump_coin_block(Level *level, const Player *p, int *outTx, int *outTy)
{
    float nextY = p->y + p->vy;
    int left = (int)(p->x / TILE_SIZE);
    int right = (int)((p->x + PLAYER_W - 1.0f) / TILE_SIZE);
    int checkY = (int)(nextY / TILE_SIZE);
    int x;

    if (p->vy >= 0.0f) {
        return 0;
    }

    for (x = left; x <= right; x++) {
        if (is_coin_block(tile_at(level, x, checkY))) {
            level->tiles[checkY][x] = 'E';
            *outTx = x;
            *outTy = checkY;
            return 1;
        }
    }
    return 0;
}

/* ── 이동 엔티티 업데이트 ────────────────────────── */
void update_moving_entities(GameWorld *world)
{
    int i;
    for (i = 0; i < world->movingEntCount; i++) {
        MovingEntity *ent = &world->movingEnts[i];
        float delta;
        if (!ent->active) continue;

        ent->prevX = ent->x;
        ent->prevY = ent->y;

        delta = ent->forward ? ent->speed : -ent->speed;

        if (ent->dirHorizontal) {
            ent->x += delta;
            if (ent->x >= ent->startX + ent->rangePixels) {
                ent->x = ent->startX + ent->rangePixels;
                ent->forward = 0;
            } else if (ent->x <= ent->startX) {
                ent->x = ent->startX;
                ent->forward = 1;
            }
        } else {
            ent->y += delta;
            if (ent->y >= ent->startY + ent->rangePixels) {
                ent->y = ent->startY + ent->rangePixels;
                ent->forward = 0;
            } else if (ent->y <= ent->startY) {
                ent->y = ent->startY;
                ent->forward = 1;
            }
        }
    }
}

/* ── AABB 충돌 헬퍼 ──────────────────────────────── */
static int aabb_overlap(float ax, float ay, float aw, float ah,
                        float bx, float by, float bw, float bh)
{
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

/* ── 이동 플랫폼 탑승 ───────────────────────────── */
int check_on_moving_platform(const GameWorld *world, Player *p, int applyCarry)
{
    int i;
    float footY = p->y + PLAYER_H;

    for (i = 0; i < world->movingEntCount; i++) {
        const MovingEntity *ent = &world->movingEnts[i];
        float dx;
        float minX;
        float maxX;
        float minTop;
        float maxTop;
        float leftBound;
        float rightBound;
        float topMin;
        float topMax;
        if (!ent->active || ent->isTrap) continue;

        dx = ent->x - ent->prevX;
        minX = (ent->x < ent->prevX) ? ent->x : ent->prevX;
        maxX = (ent->x > ent->prevX) ? ent->x : ent->prevX;
        minTop = (ent->y < ent->prevY) ? ent->y : ent->prevY;
        maxTop = (ent->y > ent->prevY) ? ent->y : ent->prevY;

        if (applyCarry) {
            /* 이미 탑승 중인 프레임은 현재 플랫폼 상단 기준으로 엄격히 판정합니다. */
            leftBound = ent->x - 2.0f;
            rightBound = ent->x + ent->width + 2.0f;
            topMin = ent->y - 3.0f;
            topMax = ent->y + 6.0f;
        } else {
            /* 착지 판정은 한 프레임 이동 스윕 범위를 사용합니다. */
            leftBound = minX - 2.0f;
            rightBound = maxX + ent->width + 2.0f;
            topMin = minTop - 2.0f;
            topMax = maxTop + 6.0f;
        }

        if (p->x + PLAYER_W >= leftBound && p->x <= rightBound) {
            if (footY >= topMin && footY <= topMax && p->vy >= 0.0f) {
                if (applyCarry) {
                    p->x += dx;
                }
                p->y = ent->y - PLAYER_H;
                p->vy = 0.0f;
                p->onGround = 1;
                return 1;
            }
        }
    }
    return 0;
}

/* ── 이동 함정 교차 ──────────────────────────────── */
int intersects_moving_trap(const GameWorld *world, const Player *p)
{
    int i;
    for (i = 0; i < world->movingEntCount; i++) {
        const MovingEntity *ent = &world->movingEnts[i];
        if (!ent->active || !ent->isTrap) continue;

        if (aabb_overlap(p->x, p->y, PLAYER_W, PLAYER_H,
                         ent->x, ent->y, ent->width, ent->height)) {
            return 1;
        }
    }
    return 0;
}

/* ── 스폰된 아이템 업데이트 ──────────────────────── */
void update_spawned_items(GameWorld *world)
{
    int i;
    for (i = 0; i < world->itemCount; i++) {
        SpawnedItem *item = &world->items[i];
        if (!item->active) continue;

        if (item->type == 'C') {
            item->y += ITEM_RISE_SPEED;
            item->timer--;
            if (item->timer <= 0) {
                item->active = 0;
            }
        } else {
            if (item->y > item->targetY) {
                item->y += ITEM_RISE_SPEED;
                if (item->y <= item->targetY) {
                    item->y = item->targetY;
                }
            }
        }
    }
}

/* ── 스폰된 아이템 수집 ──────────────────────────── */
int collect_spawned_item(GameWorld *world, const Player *p)
{
    int i;
    for (i = 0; i < world->itemCount; i++) {
        SpawnedItem *item = &world->items[i];
        if (!item->active || item->type == 'C') continue;

        if (item->y <= item->targetY) {
            if (aabb_overlap(p->x, p->y, PLAYER_W, PLAYER_H,
                             item->x, item->y, (float)TILE_SIZE, (float)TILE_SIZE)) {
                int t = item->type;
                item->active = 0;
                return t;
            }
        }
    }
    return 0;
}

/* ── 블록에서 아이템 스폰 ────────────────────────── */
void spawn_item_from_block(GameWorld *world, int tx, int ty, int type)
{
    SpawnedItem *item;
    if (world->itemCount >= MAX_SPAWNED_ITEMS) return;

    item = &world->items[world->itemCount];
    item->x = (float)(tx * TILE_SIZE);
    item->y = (float)(ty * TILE_SIZE);
    item->targetY = (float)((ty - 1) * TILE_SIZE);
    item->vy = 0.0f;
    item->type = type;
    item->active = 1;
    item->timer = COIN_BOUNCE_FRAMES;
    world->itemCount++;
}
