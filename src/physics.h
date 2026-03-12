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

/** 수집 가능 아이템과 교차 확인 (교차 시 해당 타일 문자 반환, 없으면 0) */
char intersects_collectible(Level *level, const Player *p);

/** 코인 블록 아래에서 머리 부딪힘 감지 (부딪힌 타일 좌표 반환, 없으면 0) */
int check_head_bump_coin_block(Level *level, const Player *p, int *outTx, int *outTy);

/** 이동 엔티티 업데이트 */
void update_moving_entities(GameWorld *world);

/** 이동 플랫폼 위 탑승 검사 + 플레이어 이동 */
int check_on_moving_platform(const GameWorld *world, Player *p);

/** 이동 함정과 교차 여부 */
int intersects_moving_trap(const GameWorld *world, const Player *p);

/** 스폰된 아이템 업데이트 */
void update_spawned_items(GameWorld *world);

/** 스폰된 아이템과 플레이어 교차 (교차 시 type 반환, 없으면 0) */
int collect_spawned_item(GameWorld *world, const Player *p);

/** 아이템 스폰 (코인블록에서) */
void spawn_item_from_block(GameWorld *world, int tx, int ty, int type);

/** 스폰 지점으로 플레이어 초기화 */
void reset_player_to_spawn(const Level *level, Player *player);

/** float 클램프 */
float clampf(float v, float minV, float maxV);

#endif /* PHYSICS_H */
