/**
 * @file audio.h
 * @brief BGM 재생 모듈 - WAV 로딩 및 audsrv 스트리밍
 */
#ifndef AUDIO_H
#define AUDIO_H

#include "types.h"

/** WAV 파일에서 PCM 데이터를 RAM으로 로딩 */
int load_wav_pcm(const char *path, BgmPlayer *bgm);

/** BGM 플레이어 초기화 (WAV 로딩 + audsrv 설정) */
int init_bgm_player(BgmPlayer *bgm, const char *path, int audioOk);

/** 매 프레임 호출: PCM 스트리밍 전송 */
void update_bgm_stream(BgmPlayer *bgm);

/** BGM 플레이어 종료 및 메모리 해제 */
void shutdown_bgm_player(BgmPlayer *bgm);

#endif /* AUDIO_H */
