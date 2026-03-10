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
        audsrv_quit();
    }

    if (bgm->pcmData != NULL) {
        free(bgm->pcmData);
    }

    memset(bgm, 0, sizeof(*bgm));
}
