/**
 * @file level.h
 * @brief 레벨 파일 파싱 및 타일 쿼리
 */
#ifndef LEVEL_H
#define LEVEL_H

#include "types.h"

/** 레벨 파일 파싱 (width=, height=, spawn=, data:, moving: 형식) */
int parse_level_file(const char *path, Level *level);

/** 레벨에서 이동 엔티티 파싱 (moving: 섹션) */
int parse_moving_entities(const char *path, GameWorld *world);

/** levels.txt에서 레벨 목록 로딩 */
int load_level_list(const char *path, LevelList *list);

/** 특정 타일 좌표의 문자 반환 (범위 밖이면 '#' 또는 '.') */
char tile_at(const Level *level, int tx, int ty);

/** 솔리드 타일 여부 (# ? E 포함) */
int is_solid(char tile);

/** 골 타일 여부 */
int is_goal(char tile);

/** 트랩 타일 여부 */
int is_trap(char tile);

/** 코인 블록 여부 */
int is_coin_block(char tile);

/** 수집 가능한 아이템 여부 (C, M, 1) */
int is_collectible(char tile);

/** 사용된 빈 블록 여부 */
int is_empty_block(char tile);

#endif /* LEVEL_H */
