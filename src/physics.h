/**
 * @file physics.h
 * @brief 플레이어 물리/충돌 - 이동, 충돌 감지, 지면 판정
 */
#ifndef PHYSICS_H
#define PHYSICS_H

#include "types.h"

/** 플레이어 X축 이동 + 벽 충돌 처리 */
void move_player_x(const Level *level, Player *p);

/** 플레이어 Y축 이동 + 바닥/천장 충돌 처리 */
void move_player_y(const Level *level, Player *p);

/** 플레이어 바로 아래 지면 존재 여부 */
int check_grounded(const Level *level, const Player *p);

/** 골 타일과 교차 여부 */
int intersects_goal(const Level *level, const Player *p);

/** 트랩 타일과 교차 여부 */
int intersects_trap(const Level *level, const Player *p);

/** 스폰 지점으로 플레이어 초기화 */
void reset_player_to_spawn(const Level *level, Player *player);

/** float 클램프 */
float clampf(float v, float minV, float maxV);

#endif /* PHYSICS_H */
