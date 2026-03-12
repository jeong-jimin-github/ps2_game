/**
 * @file sprite.h
 * @brief 스프라이트 로딩 - PS2TEX 파일, 스프라이트 팩, VRAM 업로드
 */
#ifndef SPRITE_H
#define SPRITE_H

#include "types.h"

/** PS2TEX 파일에서 스프라이트 데이터를 RAM으로 읽기 */
int read_sprite_data(const char *path, SpriteData *sd);

/** 스프라이트 데이터를 GS VRAM에 업로드 */
void upload_sprite_to_vram(GSGLOBAL *gsGlobal, const SpriteData *sd,
                           GSTEXTURE *tex, void **pixelMem,
                           s16 *outOffsetX, s16 *outOffsetY);

/** 애니메이션 클립용 연번 스프라이트 일괄 로딩 (baseName_00, _01, ...) */
int read_anim_clip_data(const char *baseName, SpriteData frames[], int maxFrames);

/** SPRITES.PAK 파일 로딩 */
int load_spritepack(const char *path, SpritePack *pack);

/** 팩에서 이름으로 스프라이트 검색 */
unsigned char *find_in_pack(const SpritePack *pack, const char *name, u32 *outSize);

/** 팩에서 스프라이트 데이터 추출 */
int read_sprite_from_pack(const SpritePack *pack, const char *name, SpriteData *sd);

/** 스프라이트 팩 메모리 해제 */
void free_spritepack(SpritePack *pack);

#endif /* SPRITE_H */
