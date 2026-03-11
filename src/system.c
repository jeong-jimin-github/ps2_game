/**
 * @file system.c
 * @brief PS2 시스템 유틸리티 구현
 */
#include "system.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <kernel.h>
#include <sbv_patches.h>

void make_cdrom_path(const char *src, char *dst, size_t dstSize)
{
    size_t o = 0;
    size_t i;
    const char prefix[] = "cdrom0:\\";

    for (i = 0; prefix[i] != '\0' && o < dstSize - 1; i++) {
        dst[o++] = prefix[i];
    }

    for (i = 0; src[i] != '\0' && o < dstSize - 3; i++) {
        char c = src[i];
        if (c == '/') {
            c = '\\';
        } else if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        dst[o++] = c;
    }

    if (o < dstSize - 2) {
        dst[o++] = ';';
        dst[o++] = '1';
    }
    dst[o] = '\0';
}

void iop_delay(void)
{
    volatile int d;
    for (d = 0; d < 15000000; d++) { }
}

void load_all_iop_modules(int *padOk, int *audioOk)
{
    int ret;
    char cdPath[MAX_PATH_LEN + 32];

    *padOk = 0;
    *audioOk = 0;

    SifLoadFileInit();

    ret = SifLoadModule("rom0:CDVDMAN", 0, NULL);
    LOG("CDVDMAN: %d", ret);
    ret = SifLoadModule("rom0:CDVDFSV", 0, NULL);
    LOG("CDVDFSV: %d", ret);

    iop_delay();

    /* 패드 모듈 */
    ret = SifLoadModule("rom0:XSIO2MAN", 0, NULL);
    if (ret < 0) {
        LOG("XSIO2MAN failed (%d), try SIO2MAN", ret);
        ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    }
    if (ret >= 0) {
        int padRet = SifLoadModule("rom0:XPADMAN", 0, NULL);
        if (padRet < 0) {
            LOG("XPADMAN failed (%d), try PADMAN", padRet);
            padRet = SifLoadModule("rom0:PADMAN", 0, NULL);
        }
        *padOk = (padRet >= 0) ? 1 : 0;
    }
    LOG("pad modules: %d", *padOk);

    /* 오디오 모듈 */
    *audioOk = 1;
    ret = SifLoadModule("rom0:LIBSD", 0, NULL);
    if (ret < 0) {
        LOG("rom0:LIBSD failed (%d), trying freesd.irx", ret);
        make_cdrom_path("freesd.irx", cdPath, sizeof(cdPath));
        ret = SifLoadModule(cdPath, 0, NULL);
        if (ret < 0) {
            ret = SifLoadModule("host:freesd.irx", 0, NULL);
            if (ret < 0) {
                LOG("freesd.irx load failed (%d)", ret);
                *audioOk = 0;
            }
        }
    }

    if (*audioOk) {
        make_cdrom_path("audsrv.irx", cdPath, sizeof(cdPath));
        ret = SifLoadModule(cdPath, 0, NULL);
        if (ret < 0) {
            ret = SifLoadModule("host:audsrv.irx", 0, NULL);
            if (ret < 0) {
                LOG("audsrv.irx load failed (%d)", ret);
                *audioOk = 0;
            }
        }
    }
    LOG("audio modules: %d", *audioOk);

    SifLoadFileExit();

    FlushCache(0);
    iop_delay();
    iop_delay();
    iop_delay();

    LOG("all IOP modules loaded (pad=%d audio=%d)", *padOk, *audioOk);
}

unsigned int read_u32_le(const unsigned char *p)
{
    return (unsigned int)p[0] |
           ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) |
           ((unsigned int)p[3] << 24);
}

unsigned short read_u16_le(const unsigned char *p)
{
    return (unsigned short)(p[0] | (p[1] << 8));
}

FILE *open_binary_file(const char *path)
{
    FILE *fp;
    char altPath[MAX_PATH_LEN + 32];
    int attempt;

    for (attempt = 0; attempt < 15; attempt++) {
        if (attempt > 0) {
            FlushCache(0);
            iop_delay();
            iop_delay();
            if (attempt >= 5) iop_delay(); /* 5회 이상 실패 시 더 긴 대기 */
        }

        if (strchr(path, ':') == NULL) {
            make_cdrom_path(path, altPath, sizeof(altPath));
            fp = fopen(altPath, "rb");
            if (fp != NULL) {
                LOG("open ok (attempt %d): %s", attempt, altPath);
                return fp;
            }
        }

        fp = fopen(path, "rb");
        if (fp != NULL) {
            return fp;
        }

        if (strchr(path, ':') == NULL) {
            snprintf(altPath, sizeof(altPath), "host:%s", path);
            fp = fopen(altPath, "rb");
            if (fp != NULL) {
                return fp;
            }
            snprintf(altPath, sizeof(altPath), "host:assets/%s", path);
            fp = fopen(altPath, "rb");
            if (fp != NULL) {
                return fp;
            }
        }
    }

    LOG("open failed after %d attempts: %s", attempt, path);
    return NULL;
}

FILE *open_level_file(const char *path)
{
    FILE *fp;
    char altPath[MAX_PATH_LEN + 32];
    int attempt;

    for (attempt = 0; attempt < 15; attempt++) {
        if (attempt > 0) {
            FlushCache(0);
            iop_delay();
            iop_delay();
            if (attempt >= 5) iop_delay();
        }

        if (strchr(path, ':') == NULL) {
            make_cdrom_path(path, altPath, sizeof(altPath));
            fp = fopen(altPath, "rb");
            if (fp != NULL) {
                return fp;
            }
        }

        fp = fopen(path, "rb");
        if (fp != NULL) {
            return fp;
        }

        if (strchr(path, ':') == NULL) {
            snprintf(altPath, sizeof(altPath), "host:%s", path);
            fp = fopen(altPath, "rb");
            if (fp != NULL) {
                return fp;
            }
            snprintf(altPath, sizeof(altPath), "host:assets/%s", path);
            fp = fopen(altPath, "rb");
            if (fp != NULL) {
                return fp;
            }
            snprintf(altPath, sizeof(altPath), "host:levels/%s", path);
            fp = fopen(altPath, "rb");
            if (fp != NULL) {
                return fp;
            }
        }
    }

    LOG("open failed after %d attempts: %s", attempt, path);
    return NULL;
}

void trim_line(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' || s[len - 1] == ' ' || s[len - 1] == '\t')) {
        s[len - 1] = '\0';
        len--;
    }
}
