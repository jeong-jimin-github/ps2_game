/**
 * @file render.h
 * @brief 렌더링 - 배경, 레벨 타일맵, 플레이어 그리기
 */
#ifndef RENDER_H
#define RENDER_H

#include "types.h"
#include "font.h"

/** 배경 텍스처 전체 화면 렌더링 */
void render_background(GSGLOBAL *gsGlobal, const GameTextures *textures);

/** 레벨 타일맵 렌더링 (카메라 오프셋 적용) */
void render_level(GSGLOBAL *gsGlobal, const Level *level, float camX, const GameTextures *textures);

/** 이동 엔티티 렌더링 */
void render_moving_entities(GSGLOBAL *gsGlobal, const GameWorld *world, float camX, const GameTextures *textures);

/** 스폰된 아이템 렌더링 */
void render_spawned_items(GSGLOBAL *gsGlobal, const GameWorld *world, float camX, const GameTextures *textures);

/** HUD 렌더링 (라이프, 코인, 파워업 상태) */
void render_hud(GSGLOBAL *gsGlobal, const Player *player, const BitmapFont *font);

/** 텍스트 렌더링 (폰트가 없으면 무시) */
void render_text(GSGLOBAL *gsGlobal, const BitmapFont *font, float x, float y, const char *text, u64 color);

/** 메인 메뉴 렌더링 */
void render_main_menu(GSGLOBAL *gsGlobal, const GameTextures *textures,
                      const BitmapFont *font, int selectedIndex);

/** 설정 메뉴 렌더링 */
void render_settings_menu(GSGLOBAL *gsGlobal, const GameTextures *textures,
                          const BitmapFont *font, const GameSettings *settings,
                          int selectedIndex);

/** 개발자 HUD 로그 렌더링 */
void render_dev_log(GSGLOBAL *gsGlobal, const BitmapFont *font, const DevLog *log);

/** 로딩 화면 (메뉴 배경 + "Loading..." 텍스트) */
void render_loading_screen(GSGLOBAL *gsGlobal, const GameTextures *textures, const BitmapFont *font);

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
