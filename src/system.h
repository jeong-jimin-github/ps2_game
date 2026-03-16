/**
 * @file system.h
 * @brief PS2 시스템 유틸리티 - IOP 모듈 로딩, 파일 I/O, 경로 변환
 */
#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"
#include <stdio.h>

/** CDROM 경로 변환 (소문자 → 대문자, / → \\, ;1 접미사) */
void make_cdrom_path(const char *src, char *dst, size_t dstSize);

/** IOP 안정화용 짧은 딜레이 */
void iop_delay(void);

/** IOP 모듈 일괄 로딩 (패드, 오디오) */
void load_all_iop_modules(int *padOk, int *audioOk);

/** Little-endian 4바이트 읽기 */
unsigned int read_u32_le(const unsigned char *p);

/** Little-endian 2바이트 읽기 */
unsigned short read_u16_le(const unsigned char *p);

/** 바이너리 파일 오픈 (cdrom / host 다중 경로 시도) */
FILE *open_binary_file(const char *path);

/** 레벨/에셋 파일 오픈 (cdrom / host / levels/ 다중 경로 시도) */
FILE *open_level_file(const char *path);

/** 문자열 끝의 공백/줄바꿈 제거 */
void trim_line(char *s);

/** 개발자 HUD용 본체/ROM 정보를 초기 조회 */
void init_dev_hud_info(DevHudInfo *info);

/** 개발자 HUD용 메모리 통계를 갱신 */
void update_dev_hud_memory(DevHudInfo *info);

#endif /* SYSTEM_H */
