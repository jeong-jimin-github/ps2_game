/**
 * @file level.c
 * @brief 레벨 파일 파싱 및 타일 쿼리 구현
 */
#include "level.h"
#include "system.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int parse_level_file(const char *path, Level *level)
{
    FILE *fp;
    char line[512];
    int row = 0;
    int inMap = 0;

    fp = open_level_file(path);
    if (fp == NULL) {
        LOG("level open failed: %s", path);
        return 0;
    }

    memset(level, 0, sizeof(*level));
    level->width = 0;
    level->height = 0;
    level->spawnX = 2.0f * TILE_SIZE;
    level->spawnY = 2.0f * TILE_SIZE;

    while (fgets(line, sizeof(line), fp) != NULL) {
        trim_line(line);
        if (line[0] == '\0' || line[0] == ';') {
            continue;
        }

        if (!inMap) {
            if (strncmp(line, "width=", 6) == 0) {
                level->width = atoi(line + 6);
                continue;
            }
            if (strncmp(line, "height=", 7) == 0) {
                level->height = atoi(line + 7);
                continue;
            }
            if (strncmp(line, "spawn=", 6) == 0) {
                int sx = 2;
                int sy = 2;
                sscanf(line + 6, "%d,%d", &sx, &sy);
                level->spawnX = (float)(sx * TILE_SIZE);
                level->spawnY = (float)(sy * TILE_SIZE);
                continue;
            }
            if (strcmp(line, "data:") == 0) {
                inMap = 1;
                continue;
            }
            continue;
        }

        if (row >= MAX_LEVEL_HEIGHT) {
            break;
        }

        strncpy(level->tiles[row], line, MAX_LEVEL_WIDTH);
        level->tiles[row][MAX_LEVEL_WIDTH] = '\0';

        if ((int)strlen(level->tiles[row]) > level->width) {
            level->width = (int)strlen(level->tiles[row]);
        }

        row++;
    }

    fclose(fp);

    if (row > 0) {
        level->height = row;
    }

    if (level->width <= 0 || level->height <= 0) {
        LOG("invalid level: %s", path);
        return 0;
    }

    LOG("level loaded: %s (%dx%d)", path, level->width, level->height);
    return 1;
}

int load_level_list(const char *path, LevelList *list)
{
    FILE *fp;
    char line[512];

    memset(list, 0, sizeof(*list));
    fp = open_level_file(path);

    if (fp == NULL) {
        LOG("level list open failed: %s", path);
        strncpy(list->paths[0], "level1.txt", MAX_PATH_LEN - 1);
        list->paths[0][MAX_PATH_LEN - 1] = '\0';
        list->count = 1;
        return 1;
    }

    while (fgets(line, sizeof(line), fp) != NULL && list->count < MAX_LEVELS) {
        trim_line(line);
        if (line[0] == '\0' || line[0] == ';') {
            continue;
        }
        strncpy(list->paths[list->count], line, MAX_PATH_LEN - 1);
        list->paths[list->count][MAX_PATH_LEN - 1] = '\0';
        list->count++;
    }

    fclose(fp);

    if (list->count <= 0) {
        strncpy(list->paths[0], "level1.txt", MAX_PATH_LEN - 1);
        list->paths[0][MAX_PATH_LEN - 1] = '\0';
        list->count = 1;
    }

    LOG("level list loaded: %d levels", list->count);
    return 1;
}

char tile_at(const Level *level, int tx, int ty)
{
    if (tx < 0 || tx >= level->width || ty < 0) {
        return '#';
    }

    if (ty >= level->height) {
        return '.';
    }

    if ((int)strlen(level->tiles[ty]) <= tx) {
        return '.';
    }

    return level->tiles[ty][tx];
}

int is_solid(char tile)
{
    return tile == '#' || tile == '?' || tile == 'E';
}

int is_goal(char tile)
{
    return tile == 'G';
}

int is_trap(char tile)
{
    return tile == 'X';
}

int is_coin_block(char tile)
{
    return tile == '?';
}

int is_collectible(char tile)
{
    return tile == 'C' || tile == 'M' || tile == '1';
}

int is_empty_block(char tile)
{
    return tile == 'E';
}

int parse_moving_entities(const char *path, GameWorld *world)
{
    FILE *fp;
    char line[512];
    int inMoving = 0;

    world->movingEntCount = 0;
    world->itemCount = 0;

    fp = open_level_file(path);
    if (fp == NULL) {
        return 0;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        trim_line(line);
        if (line[0] == '\0' || line[0] == ';') {
            continue;
        }

        if (!inMoving) {
            if (strcmp(line, "moving:") == 0) {
                inMoving = 1;
            }
            continue;
        }

        if (world->movingEntCount < MAX_MOVING_ENTITIES) {
            MovingEntity *ent = &world->movingEnts[world->movingEntCount];
            char typeChar = 0;
            int tx = 0, ty = 0;
            char dirChar = 'H';
            int range = 3;
            float speed = MOVING_ENTITY_DEFAULT_SPEED;

            if (sscanf(line, "%c %d,%d %c %d %f", &typeChar, &tx, &ty, &dirChar, &range, &speed) >= 5) {
                ent->startX = (float)(tx * TILE_SIZE);
                ent->startY = (float)(ty * TILE_SIZE);
                ent->x = ent->startX;
                ent->y = ent->startY;
                ent->prevX = ent->x;
                ent->prevY = ent->y;
                ent->dirHorizontal = (dirChar == 'H' || dirChar == 'h') ? 1 : 0;
                ent->rangePixels = (float)(range * TILE_SIZE);
                ent->speed = speed;
                ent->isTrap = (typeChar == 'T' || typeChar == 't') ? 1 : 0;
                ent->active = 1;
                ent->forward = 1;
                ent->width = (float)TILE_SIZE * 2;
                ent->height = (float)TILE_SIZE;
                world->movingEntCount++;
            }
        }
    }

    fclose(fp);
    LOG("moving entities loaded: %d", world->movingEntCount);
    return 1;
}
