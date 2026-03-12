/**
 * @file asset.h
 * @brief 에셋 관리 - CD에서 RAM 로딩, RAM에서 VRAM 업로드
 */
#ifndef ASSET_H
#define ASSET_H

#include "types.h"

/** 로딩 진행 메시지 출력 */
void loading_msg(int step, int total, const char *name);

/** 모든 에셋을 CD(또는 host)에서 RAM으로 로딩 (Phase 1) */
void load_all_assets_from_cd(AssetBank *bank);

/** RAM의 에셋을 GS VRAM에 업로드 (Phase 2) */
void upload_all_assets_to_vram(GSGLOBAL *gsGlobal, const AssetBank *bank,
                               GameTextures *tex,
                               AnimationClip playerAnims[PLAYER_ANIM_COUNT]);

#endif /* ASSET_H */
