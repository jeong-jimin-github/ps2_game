/**
 * @file audio.c
 * @brief BGM 재생 모듈 구현
 */
#include "audio.h"
#include "system.h"

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <audsrv.h>

int load_wav_pcm(const char *path, BgmPlayer *bgm)
{
    FILE *fp;
    unsigned char riffHeader[12];
    int fmtFound = 0;
    int dataFound = 0;

    fp = open_binary_file(path);
    if (fp == NULL) {
        LOG("bgm open failed: %s", path);
        return 0;
    }

    if (fread(riffHeader, 1, sizeof(riffHeader), fp) != sizeof(riffHeader)) {
        fclose(fp);
        LOG("bgm header read failed");
        return 0;
    }

    if (memcmp(riffHeader, "RIFF", 4) != 0 || memcmp(riffHeader + 8, "WAVE", 4) != 0) {
        fclose(fp);
        LOG("bgm is not RIFF/WAVE: %s", path);
        return 0;
    }

    while (!dataFound) {
        unsigned char chunkHeader[8];
        unsigned int chunkSize;

        if (fread(chunkHeader, 1, sizeof(chunkHeader), fp) != sizeof(chunkHeader)) {
            break;
        }

        chunkSize = read_u32_le(chunkHeader + 4);

        if (memcmp(chunkHeader, "fmt ", 4) == 0) {
            unsigned char fmtBuf[40];
            unsigned int readSize = chunkSize;
            unsigned short audioFormat;
            unsigned short channels;
            unsigned int sampleRate;
            unsigned short bitsPerSample;

            if (readSize < 16) {
                fclose(fp);
                LOG("bgm fmt chunk too small");
                return 0;
            }
            if (readSize > sizeof(fmtBuf)) {
                readSize = sizeof(fmtBuf);
            }
            if (fread(fmtBuf, 1, readSize, fp) != readSize) {
                fclose(fp);
                LOG("bgm fmt read failed");
                return 0;
            }
            if (chunkSize > readSize) {
                fseek(fp, (long)(chunkSize - readSize), SEEK_CUR);
            }

            audioFormat = read_u16_le(fmtBuf + 0);
            channels = read_u16_le(fmtBuf + 2);
            sampleRate = read_u32_le(fmtBuf + 4);
            bitsPerSample = read_u16_le(fmtBuf + 14);

            if (audioFormat != 1 || (channels != 1 && channels != 2) ||
                (bitsPerSample != 8 && bitsPerSample != 16)) {
                fclose(fp);
                LOG("bgm unsupported fmt: format=%u ch=%u bits=%u",
                    (unsigned int)audioFormat,
                    (unsigned int)channels,
                    (unsigned int)bitsPerSample);
                return 0;
            }

            bgm->format.freq = (int)sampleRate;
            bgm->format.bits = (int)bitsPerSample;
            bgm->format.channels = (int)channels;
            fmtFound = 1;
        } else if (memcmp(chunkHeader, "data", 4) == 0) {
            if (chunkSize == 0) {
                fclose(fp);
                LOG("bgm data chunk is empty");
                return 0;
            }

            bgm->pcmData = (unsigned char *)memalign(64, chunkSize);
            if (bgm->pcmData == NULL) {
                fclose(fp);
                LOG("bgm data alloc failed");
                return 0;
            }

            if (fread(bgm->pcmData, 1, chunkSize, fp) != chunkSize) {
                fclose(fp);
                free(bgm->pcmData);
                bgm->pcmData = NULL;
                LOG("bgm data read failed");
                return 0;
            }

            bgm->pcmSize = (int)chunkSize;
            dataFound = 1;
        } else {
            fseek(fp, (long)chunkSize, SEEK_CUR);
        }

        if (chunkSize & 1u) {
            fseek(fp, 1L, SEEK_CUR);
        }
    }

    fclose(fp);

    if (!fmtFound || !dataFound) {
        if (bgm->pcmData != NULL) {
            free(bgm->pcmData);
            bgm->pcmData = NULL;
        }
        bgm->pcmSize = 0;
        LOG("bgm missing fmt/data chunk");
        return 0;
    }

    /* 8bit unsigned → 16bit signed 변환 (SPU2 호환) */
    if (bgm->format.bits == 8 && bgm->pcmData != NULL && bgm->pcmSize > 0) {
        int newSize = bgm->pcmSize * 2;
        unsigned char *src = bgm->pcmData;
        short *dst;
        int i;

        dst = (short *)memalign(64, (size_t)newSize);
        if (dst == NULL) {
            free(bgm->pcmData);
            bgm->pcmData = NULL;
            bgm->pcmSize = 0;
            LOG("bgm 8->16 bit alloc failed");
            return 0;
        }

        for (i = 0; i < bgm->pcmSize; i++) {
            dst[i] = (short)(((int)src[i] - 128) << 8);
        }

        free(bgm->pcmData);
        bgm->pcmData = (unsigned char *)dst;
        bgm->pcmSize = newSize;
        bgm->format.bits = 16;
        LOG("bgm converted 8-bit -> 16-bit (%d bytes)", newSize);
    }

    LOG("bgm loaded: %s (rate=%d ch=%d bits=%d size=%d)",
        path, bgm->format.freq, bgm->format.channels, bgm->format.bits, bgm->pcmSize);
    return 1;
}

int init_bgm_player(BgmPlayer *bgm, const char *path, int audioOk)
{
    memset(bgm, 0, sizeof(*bgm));

    if (!load_wav_pcm(path, bgm)) {
        return 0;
    }

    if (!audioOk) {
        LOG("audio IOP modules not available, skipping BGM");
        free(bgm->pcmData);
        memset(bgm, 0, sizeof(*bgm));
        return 0;
    }

    if (audsrv_init() != 0) {
        LOG("audsrv_init failed: %s", audsrv_get_error_string());
        free(bgm->pcmData);
        memset(bgm, 0, sizeof(*bgm));
        return 0;
    }

    LOG("bgm wav format: rate=%d ch=%d bits=%d",
        bgm->format.freq, bgm->format.channels, bgm->format.bits);

    {
        int retry;
        int fmtOk = 0;
        for (retry = 0; retry < 50; retry++) {
            volatile int delay;
            for (delay = 0; delay < 100000; delay++) { }
            if (audsrv_set_format(&bgm->format) == 0) {
                fmtOk = 1;
                break;
            }
        }
        if (!fmtOk) {
            LOG("audsrv_set_format failed after retries: %s", audsrv_get_error_string());
            audsrv_quit();
            free(bgm->pcmData);
            memset(bgm, 0, sizeof(*bgm));
            return 0;
        }
        LOG("audsrv_set_format ok (attempt %d)", retry + 1);
    }

    audsrv_set_volume(80);
    bgm->audioReady = 1;
    bgm->cursor = 0;
    return 1;
}

void update_bgm_stream(BgmPlayer *bgm)
{
    int available;

    if (!bgm->audioReady || bgm->pcmData == NULL || bgm->pcmSize <= 0) {
        return;
    }

    available = audsrv_available();
    while (available > 0) {
        int remain = bgm->pcmSize - bgm->cursor;
        int toSend = remain;
        int sent;

        if (toSend > available) {
            toSend = available;
        }
        if (toSend > BGM_STREAM_CHUNK) {
            toSend = BGM_STREAM_CHUNK;
        }

        if (toSend <= 0) {
            bgm->cursor = 0;
            remain = bgm->pcmSize;
            toSend = remain;
            if (toSend > available) {
                toSend = available;
            }
            if (toSend > BGM_STREAM_CHUNK) {
                toSend = BGM_STREAM_CHUNK;
            }
            if (toSend <= 0) {
                break;
            }
        }

        sent = audsrv_play_audio((const char *)bgm->pcmData + bgm->cursor, toSend);
        if (sent <= 0) {
            break;
        }

        bgm->cursor += sent;
        if (bgm->cursor >= bgm->pcmSize) {
            bgm->cursor = 0;
        }

        available -= sent;
    }
}

void shutdown_bgm_player(BgmPlayer *bgm)
{
    if (bgm->audioReady) {
        audsrv_stop_audio();
    }

    if (bgm->pcmData != NULL) {
        free(bgm->pcmData);
    }

    memset(bgm, 0, sizeof(*bgm));
}

int bgm_swap(BgmPlayer *bgm, const char *path)
{
    /* audsrv를 종료하지 않고 WAV만 교체 */
    if (bgm->audioReady) {
        audsrv_stop_audio();
    }
    if (bgm->pcmData != NULL) {
        free(bgm->pcmData);
        bgm->pcmData = NULL;
    }
    bgm->pcmSize = 0;
    bgm->cursor = 0;
    bgm->audioReady = 0;

    if (!load_wav_pcm(path, bgm)) {
        LOG("bgm_swap: load failed: %s", path);
        return 0;
    }

    {
        int retry, fmtOk = 0;
        for (retry = 0; retry < 50; retry++) {
            volatile int d;
            for (d = 0; d < 100000; d++) { }
            if (audsrv_set_format(&bgm->format) == 0) {
                fmtOk = 1;
                break;
            }
        }
        if (!fmtOk) {
            LOG("bgm_swap: set_format failed: %s", path);
            free(bgm->pcmData);
            bgm->pcmData = NULL;
            bgm->pcmSize = 0;
            return 0;
        }
    }

    bgm->audioReady = 1;
    bgm->cursor = 0;
    LOG("bgm_swap: ok -> %s (rate=%d ch=%d bits=%d size=%d)",
        path, bgm->format.freq, bgm->format.channels, bgm->format.bits, bgm->pcmSize);
    return 1;
}

void bgm_stop(BgmPlayer *bgm)
{
    if (bgm->audioReady) {
        audsrv_stop_audio();
        bgm->audioReady = 0;
    }
}

void bgm_activate(BgmPlayer *bgm)
{
    if (bgm->pcmData != NULL && bgm->pcmSize > 0) {
        audsrv_set_format(&bgm->format);
        bgm->cursor = 0;
        bgm->audioReady = 1;
    }
}

void bgm_set_volume(int vol)
{
    audsrv_set_volume(vol);
}

/* ═══════════════════════════════════════════════════
 *  CD 스트리밍 BGM
 * ═══════════════════════════════════════════════════ */

static int s_audsrvReady = 0;   /* audsrv_init 이미 호출 여부 */

static int ensure_audsrv(int audioOk)
{
    if (s_audsrvReady) return 1;
    if (!audioOk) return 0;
    if (audsrv_init() != 0) {
        LOG("audsrv_init failed: %s", audsrv_get_error_string());
        return 0;
    }
    s_audsrvReady = 1;
    return 1;
}

int bgms_open(BgmStream *s, const char *path, int audioOk)
{
    FILE *fp;
    unsigned char riffHeader[12];
    int fmtFound = 0;

    memset(s, 0, sizeof(*s));

    fp = open_binary_file(path);
    if (fp == NULL) {
        LOG("bgms open failed: %s", path);
        return 0;
    }

    if (fread(riffHeader, 1, 12, fp) != 12 ||
        memcmp(riffHeader, "RIFF", 4) != 0 ||
        memcmp(riffHeader + 8, "WAVE", 4) != 0) {
        fclose(fp);
        LOG("bgms not RIFF/WAVE: %s", path);
        return 0;
    }

    /* WAV 청크 파싱 — fmt + data 위치 찾기 */
    while (1) {
        unsigned char ch[8];
        unsigned int csz;
        if (fread(ch, 1, 8, fp) != 8) break;
        csz = read_u32_le(ch + 4);

        if (memcmp(ch, "fmt ", 4) == 0) {
            unsigned char fb[40];
            unsigned int rd = csz > 40 ? 40 : csz;
            if (fread(fb, 1, rd, fp) != rd) { fclose(fp); return 0; }
            if (csz > rd) fseek(fp, (long)(csz - rd), SEEK_CUR);

            if (read_u16_le(fb) != 1) { fclose(fp); LOG("bgms not PCM"); return 0; }
            s->format.channels = (int)read_u16_le(fb + 2);
            s->format.freq     = (int)read_u32_le(fb + 4);
            s->format.bits     = (int)read_u16_le(fb + 14);
            fmtFound = 1;
        } else if (memcmp(ch, "data", 4) == 0) {
            s->dataStart = ftell(fp);
            s->dataSize  = (int)csz;
            break;              /* data 위치 기록 후 중단 — 데이터를 읽지 않음 */
        } else {
            fseek(fp, (long)csz, SEEK_CUR);
        }
        if (csz & 1u) fseek(fp, 1L, SEEK_CUR);
    }

    if (!fmtFound || s->dataSize <= 0) {
        fclose(fp);
        LOG("bgms missing fmt/data: %s", path);
        return 0;
    }

    /* audsrv 초기화 */
    if (!ensure_audsrv(audioOk)) {
        fclose(fp);
        return 0;
    }

    /* audsrv_set_format */
    {
        int retry, fmtOk = 0;
        for (retry = 0; retry < 50; retry++) {
            volatile int d; for (d = 0; d < 100000; d++) { }
            if (audsrv_set_format(&s->format) == 0) { fmtOk = 1; break; }
        }
        if (!fmtOk) {
            fclose(fp);
            LOG("bgms set_format failed: %s", path);
            return 0;
        }
        LOG("bgms format ok: rate=%d ch=%d bits=%d",
            s->format.freq, s->format.channels, s->format.bits);
    }

    /* 스트림 버퍼 할당 */
    s->buf = (unsigned char *)memalign(64, BGM_SBUF_SIZE);
    if (s->buf == NULL) {
        fclose(fp);
        LOG("bgms buf alloc failed");
        return 0;
    }

    s->fp = fp;
    s->dataPos = 0;
    s->bufLen = 0;
    s->opened = 1;
    s->playing = 0;

    audsrv_set_volume(80);
    LOG("bgms opened: %s (data=%d bytes, buf=%d)", path, s->dataSize, BGM_SBUF_SIZE);
    return 1;
}

void bgms_update(BgmStream *s)
{
    FILE *fp;
    int available;

    if (!s->opened || !s->playing) return;
    fp = (FILE *)s->fp;

    /* 1. CD에서 버퍼 보충 (반 이하로 줄면) */
    if (s->bufLen < BGM_SBUF_SIZE / 2 && fp != NULL) {
        int space = BGM_SBUF_SIZE - s->bufLen;
        int toRead = space < BGM_SREAD_CHUNK ? space : BGM_SREAD_CHUNK;
        int remain = s->dataSize - s->dataPos;
        int got;

        if (toRead > remain) toRead = remain;
        if (toRead > 0) {
            got = (int)fread(s->buf + s->bufLen, 1, (size_t)toRead, fp);
            if (got > 0) {
                s->bufLen += got;
                s->dataPos += got;
            }
        }

        /* 루프: 끝까지 읽었으면 처음으로 */
        if (s->dataPos >= s->dataSize) {
            fseek(fp, s->dataStart, SEEK_SET);
            s->dataPos = 0;
        }
    }

    /* 2. 버퍼 → audsrv 전송 */
    available = audsrv_available();
    while (available > 0 && s->bufLen > 0) {
        int toSend = s->bufLen < available ? s->bufLen : available;
        int sent;
        if (toSend > BGM_STREAM_CHUNK) toSend = BGM_STREAM_CHUNK;

        sent = audsrv_play_audio((const char *)s->buf, toSend);
        if (sent <= 0) break;

        /* 전송한 만큼 버퍼 앞으로 이동 */
        s->bufLen -= sent;
        if (s->bufLen > 0) {
            memmove(s->buf, s->buf + sent, (size_t)s->bufLen);
        }
        available -= sent;
    }
}

void bgms_play(BgmStream *s)
{
    FILE *fp;
    if (!s->opened) return;
    fp = (FILE *)s->fp;

    /* 처음부터 재생 */
    if (fp != NULL) {
        fseek(fp, s->dataStart, SEEK_SET);
    }
    s->dataPos = 0;
    s->bufLen = 0;
    s->playing = 1;

    /* audsrv 포맷 재설정 (다른 스트림 후에 전환했을 수 있음) */
    audsrv_set_format(&s->format);
}

void bgms_stop(BgmStream *s)
{
    if (s->playing) {
        audsrv_stop_audio();
        s->playing = 0;
        s->bufLen = 0;
    }
}

void bgms_close(BgmStream *s)
{
    if (s->playing) {
        audsrv_stop_audio();
    }
    if (s->fp != NULL) {
        fclose((FILE *)s->fp);
    }
    if (s->buf != NULL) {
        free(s->buf);
    }
    memset(s, 0, sizeof(*s));
}
