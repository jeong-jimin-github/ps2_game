/**
 * @file render.h
 * @brief 렌더링 - 배경, 레벨 타일맵, 플레이어 그리기
 */
#ifndef RENDER_H
#define RENDER_H

#include "types.h"

/** 배경 텍스처 전체 화면 렌더링 */
void render_background(GSGLOBAL *gsGlobal, const GameTextures *textures);

/** 레벨 타일맵 렌더링 (카메라 오프셋 적용) */
void render_level(GSGLOBAL *gsGlobal, const Level *level, float camX, const GameTextures *textures);

/** 플레이어 스프라이트/애니메이션 렌더링 */
void render_player(GSGLOBAL *gsGlobal,
                   const Player *p,
                   float camX,
                   const GameTextures *textures,
                   AnimationClip clips[PLAYER_ANIM_COUNT],
                   PlayerAnimState state,
                   int frameIndex,
                   int facingRight);

#endif /* RENDER_H */
