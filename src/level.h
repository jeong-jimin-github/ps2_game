/**
 * @file level.h
 * @brief 레벨 파일 파싱 및 타일 쿼리
 */
#ifndef LEVEL_H
#define LEVEL_H

#include "types.h"

/** 레벨 파일 파싱 (width=, height=, spawn=, data: 형식) */
int parse_level_file(const char *path, Level *level);

/** levels.txt에서 레벨 목록 로딩 */
int load_level_list(const char *path, LevelList *list);

/** 특정 타일 좌표의 문자 반환 (범위 밖이면 '#' 또는 '.') */
char tile_at(const Level *level, int tx, int ty);

/** 솔리드 타일 여부 */
int is_solid(char tile);

/** 골 타일 여부 */
int is_goal(char tile);

/** 트랩 타일 여부 */
int is_trap(char tile);

#endif /* LEVEL_H */
