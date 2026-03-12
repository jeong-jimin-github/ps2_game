/**
 * @file sfx.h
 * @brief 효과음(SFX) 모듈 - SPU2 ADPCM 기반
 */
#ifndef SFX_H
#define SFX_H

#include "types.h"

/* ── SFX 식별자 ──────────────────────────────────── */
typedef enum {
    SFX_JUMP = 0,
    SFX_COIN,
    SFX_POWERUP,
    SFX_1UP,
    SFX_HURT,
    SFX_DEATH,
    SFX_CLEAR,
    SFX_SELECT,
    SFX_COUNT
} SfxId;

/** ADPCM 서브시스템 초기화 + 모든 SFX 파일 로딩 (CD에서) */
int  sfx_init(void);

/** SFX 재생 (다음 빈 SPU2 채널에) */
void sfx_play(SfxId id);

/** SFX 볼륨 설정 (0–MAX_VOLUME) */
void sfx_set_volume(int vol);

/** 모든 SFX 해제 */
void sfx_shutdown(void);

#endif /* SFX_H */
