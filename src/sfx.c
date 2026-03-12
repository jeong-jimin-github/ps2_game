/**
 * @file sfx.c
 * @brief 효과음 모듈 구현 - audsrv ADPCM
 */
#include "sfx.h"
#include "system.h"

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <audsrv.h>

/* 파일명 테이블 (CD상 대문자) */
static const char *sfx_filenames[SFX_COUNT] = {
    "sfx_jump.adpcm",
    "sfx_coin.adpcm",
    "sfx_powerup.adpcm",
    "sfx_1up.adpcm",
    "sfx_hurt.adpcm",
    "sfx_death.adpcm",
    "sfx_clear.adpcm",
    "sfx_select.adpcm"
};

static audsrv_adpcm_t sfx_samples[SFX_COUNT];
static int sfx_loaded[SFX_COUNT];
static int sfx_ready = 0;
static int sfx_volume = MAX_VOLUME;

int sfx_init(void)
{
    int i;
    int loaded = 0;

    memset(sfx_samples, 0, sizeof(sfx_samples));
    memset(sfx_loaded, 0, sizeof(sfx_loaded));

    if (audsrv_adpcm_init() != 0) {
        LOG("sfx: audsrv_adpcm_init failed");
        return 0;
    }

    for (i = 0; i < SFX_COUNT; i++) {
        FILE *fp = open_binary_file(sfx_filenames[i]);
        if (fp == NULL) {
            LOG("sfx: missing %s", sfx_filenames[i]);
            continue;
        }

        fseek(fp, 0, SEEK_END);
        {
            long size = ftell(fp);
            unsigned char *buf;

            fseek(fp, 0, SEEK_SET);

            buf = (unsigned char *)memalign(64, (size_t)size);
            if (buf == NULL) {
                fclose(fp);
                LOG("sfx: alloc failed for %s (%ld bytes)", sfx_filenames[i], size);
                continue;
            }

            if ((long)fread(buf, 1, (size_t)size, fp) != size) {
                fclose(fp);
                free(buf);
                LOG("sfx: read failed for %s", sfx_filenames[i]);
                continue;
            }
            fclose(fp);

            if (audsrv_load_adpcm(&sfx_samples[i], buf, (int)size) != 0) {
                free(buf);
                LOG("sfx: audsrv_load_adpcm failed for %s", sfx_filenames[i]);
                continue;
            }

            free(buf);  /* 데이터는 SPU2 RAM에 업로드됨 */
            sfx_loaded[i] = 1;
            loaded++;
            LOG("sfx: loaded %s (%ld bytes)", sfx_filenames[i], size);
        }
    }

    sfx_ready = 1;
    LOG("sfx: %d/%d effects loaded", loaded, SFX_COUNT);
    return loaded;
}

void sfx_play(SfxId id)
{
    int ch;
    if (!sfx_ready || id < 0 || id >= SFX_COUNT || !sfx_loaded[id]) return;

    ch = audsrv_ch_play_adpcm(-1, &sfx_samples[id]);
    if (ch >= 0) {
        audsrv_adpcm_set_volume_and_pan(ch, sfx_volume, 0);
    }
}

void sfx_set_volume(int vol)
{
    sfx_volume = vol;
}

void sfx_shutdown(void)
{
    int i;
    for (i = 0; i < SFX_COUNT; i++) {
        if (sfx_loaded[i]) {
            audsrv_free_adpcm(&sfx_samples[i]);
            sfx_loaded[i] = 0;
        }
    }
    sfx_ready = 0;
}
