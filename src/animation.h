/**
 * @file animation.h
 * @brief 플레이어 애니메이션 상태 머신 및 프레임 관리
 */
#ifndef ANIMATION_H
#define ANIMATION_H

#include "types.h"
#include <libpad.h>

/** 클립 안전 검색 (요청 상태 → idle → 아무거나 fallback) */
AnimationClip *get_player_clip_safe(AnimationClip clips[PLAYER_ANIM_COUNT], PlayerAnimState state);

/** 애니메이터 초기화 */
void animator_init(PlayerAnimator *anim);

/** 입력/상태에 따른 애니메이션 상태 결정 */
PlayerAnimState pick_player_anim_state(const Player *p,
                                       unsigned int padData,
                                       int gameOver,
                                       int hurtTimer,
                                       int clearTimer,
                                       int *groundedGrace);

/** 애니메이터 매 프레임 업데이트 */
void animator_step(PlayerAnimator *anim,
                   AnimationClip clips[PLAYER_ANIM_COUNT],
                   const Player *p,
                   unsigned int padData,
                   int gameOver,
                   int hurtTimer,
                   int clearTimer);

#endif /* ANIMATION_H */
