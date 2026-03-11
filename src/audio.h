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

/** BGM 플레이어 종료 및 메모리 해제 (audsrv는 종료하지 않음) */
void shutdown_bgm_player(BgmPlayer *bgm);

/** audsrv를 유지한 채 WAV만 교체 (씬 전환용) */
int bgm_swap(BgmPlayer *bgm, const char *path);

/** 현재 오디오 스트림 정지 (audioReady = 0) */
void bgm_stop(BgmPlayer *bgm);

/** BGM 포맷 설정 후 재생 활성화 (cursor 리셋) */
void bgm_activate(BgmPlayer *bgm);

/** 마스터 볼륨 설정 (0–100) */
void bgm_set_volume(int vol);

/* ── CD 스트리밍 BGM (RAM 절약) ───────────────────── */

/** CD에서 WAV 헤더만 읽고, 스트리밍 준비 (버퍼 할당 + audsrv init) */
int  bgms_open(BgmStream *s, const char *path, int audioOk);

/** 매 프레임 호출: CD에서 읽어 audsrv 에 전송 */
void bgms_update(BgmStream *s);

/** 재생 시작 (처음부터) */
void bgms_play(BgmStream *s);

/** 재생 정지 */
void bgms_stop(BgmStream *s);

/** 파일 핸들 + 버퍼 해제 */
void bgms_close(BgmStream *s);

#endif /* AUDIO_H */
