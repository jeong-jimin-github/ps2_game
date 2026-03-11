/**
 * @file sprite.c
 * @brief 스프라이트 로딩 구현
 */
#include "sprite.h"
#include "system.h"

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <kernel.h>
#include <gsKit.h>

int read_sprite_data(const char *path, SpriteData *sd)
{
    FILE *fp;
    SpriteHeader header;
    size_t pixelBytes;
    void *pixels;

    memset(sd, 0, sizeof(*sd));

    fp = open_level_file(path);
    if (fp == NULL) {
        LOG("sprite open failed: %s", path);
        return 0;
    }

    if (fread(&header, 1, sizeof(header), fp) != sizeof(header)) {
        fclose(fp);
        LOG("sprite header read failed: %s", path);
        return 0;
    }

    if (header.magic != SPRITE_MAGIC || header.width == 0 || header.height == 0) {
        fclose(fp);
        LOG("sprite header invalid: %s", path);
        return 0;
    }

    pixelBytes = (size_t)header.width * (size_t)header.height * 4u;
    pixels = memalign(128, pixelBytes);
    if (pixels == NULL) {
        fclose(fp);
        LOG("sprite mem alloc failed: %s", path);
        return 0;
    }

    if (fread(pixels, 1, pixelBytes, fp) != pixelBytes) {
        fclose(fp);
        free(pixels);
        LOG("sprite pixel read failed: %s", path);
        return 0;
    }

    fclose(fp);

    /* 완전 투명 픽셀의 RGB 클리어 */
    {
        unsigned char *px = (unsigned char *)pixels;
        size_t total = (size_t)header.width * (size_t)header.height;
        size_t pi;
        for (pi = 0; pi < total; pi++) {
            if (px[pi * 4 + 3] == 0) {
                px[pi * 4 + 0] = 0;
                px[pi * 4 + 1] = 0;
                px[pi * 4 + 2] = 0;
            }
        }
    }

    sd->pixels = pixels;
    sd->width = header.width;
    sd->height = header.height;
    sd->offsetX = header.offsetX;
    sd->offsetY = header.offsetY;
    sd->valid = 1;

    FlushCache(0);
    iop_delay();

    LOG("sprite read: %s (%ux%u)", path, (unsigned)header.width, (unsigned)header.height);
    return 1;
}

void upload_sprite_to_vram(GSGLOBAL *gsGlobal, const SpriteData *sd,
                           GSTEXTURE *tex, void **pixelMem,
                           s16 *outOffsetX, s16 *outOffsetY)
{
    memset(tex, 0, sizeof(*tex));
    tex->Width = sd->width;
    tex->Height = sd->height;
    tex->PSM = GS_PSM_CT32;
    tex->Filter = GS_FILTER_NEAREST;
    tex->Mem = sd->pixels;

    FlushCache(0);

    tex->Vram = gsKit_vram_alloc(gsGlobal,
                                 gsKit_texture_size(tex->Width, tex->Height, tex->PSM),
                                 GSKIT_ALLOC_USERBUFFER);
    gsKit_texture_upload(gsGlobal, tex);

    FlushCache(0);

    *pixelMem = sd->pixels;
    if (outOffsetX) *outOffsetX = sd->offsetX;
    if (outOffsetY) *outOffsetY = sd->offsetY;
}

int read_anim_clip_data(const char *baseName, SpriteData frames[], int maxFrames)
{
    int i, loaded = 0;
    char path[256];

    for (i = 0; i < maxFrames; i++) {
        snprintf(path, sizeof(path), "%s_%02d.ps2tex", baseName, i);
        if (!read_sprite_data(path, &frames[i])) {
            break;
        }
        loaded++;

        /* 팬텀 읽기 감지: 이전 프레임과 동일한 데이터면 중단 */
        if (loaded >= 2 && frames[i].width == frames[i - 1].width
                        && frames[i].height == frames[i - 1].height) {
            size_t sz = (size_t)frames[i].width * (size_t)frames[i].height * 4u;
            if (memcmp(frames[i].pixels, frames[i - 1].pixels, sz) == 0) {
                LOG("phantom read detected at %s (dup of prev), stopping", path);
                free(frames[i].pixels);
                memset(&frames[i], 0, sizeof(frames[i]));
                loaded--;
                break;
            }
        }
    }

    if (loaded == 0) {
        snprintf(path, sizeof(path), "%s.ps2tex", baseName);
        if (read_sprite_data(path, &frames[0])) {
            loaded = 1;
        }
    }

    return loaded;
}

int load_spritepack(const char *path, SpritePack *pack)
{
    FILE *fp;
    u32 header[2];
    u32 dataSize;
    size_t indexSize;
    int i;

    memset(pack, 0, sizeof(*pack));

    fp = open_binary_file(path);
    if (fp == NULL) {
        LOG("pak open failed: %s", path);
        return 0;
    }

    if (fread(header, 4, 2, fp) != 2) {
        fclose(fp);
        LOG("pak header read failed");
        return 0;
    }

    if (header[0] != PAK_MAGIC) {
        fclose(fp);
        LOG("pak magic mismatch: 0x%08X", (unsigned)header[0]);
        return 0;
    }

    pack->count = (int)header[1];
    if (pack->count <= 0 || pack->count > 256) {
        fclose(fp);
        LOG("pak invalid count: %d", pack->count);
        return 0;
    }

    indexSize = (size_t)pack->count * sizeof(PakEntry);
    pack->entries = (PakEntry *)memalign(64, indexSize);
    if (pack->entries == NULL) {
        fclose(fp);
        LOG("pak index alloc failed");
        return 0;
    }

    if (fread(pack->entries, 1, indexSize, fp) != indexSize) {
        fclose(fp);
        free(pack->entries);
        pack->entries = NULL;
        LOG("pak index read failed");
        return 0;
    }

    dataSize = 0;
    for (i = 0; i < pack->count; i++) {
        u32 end = pack->entries[i].offset + pack->entries[i].size;
        if (end > dataSize) dataSize = end;
    }

    pack->dataSection = (unsigned char *)memalign(128, dataSize);
    if (pack->dataSection == NULL) {
        fclose(fp);
        free(pack->entries);
        pack->entries = NULL;
        LOG("pak data alloc failed (%u bytes)", (unsigned)dataSize);
        return 0;
    }

    FlushCache(0);

    /* CD에서 대용량 fread는 실패할 수 있으므로 32KB 청크 단위로 읽기 */
    {
        u32 offset = 0;
        const u32 chunkSize = 32 * 1024;
        while (offset < dataSize) {
            u32 toRead = dataSize - offset;
            size_t got;
            if (toRead > chunkSize) toRead = chunkSize;
            got = fread(pack->dataSection + offset, 1, (size_t)toRead, fp);
            if (got == 0) {
                LOG("pak data read stalled at %u/%u bytes", (unsigned)offset, (unsigned)dataSize);
                break;
            }
            offset += (u32)got;
        }
        if (offset < dataSize) {
            LOG("pak data incomplete: got %u of %u bytes", (unsigned)offset, (unsigned)dataSize);
        }
    }

    fclose(fp);
    FlushCache(0);

    LOG("pak loaded: %d entries, %u bytes data", pack->count, (unsigned)dataSize);
    return 1;
}

unsigned char *find_in_pack(const SpritePack *pack, const char *name, u32 *outSize)
{
    int i;
    for (i = 0; i < pack->count; i++) {
        const char *a = pack->entries[i].name;
        const char *b = name;
        int match = 1;
        while (*a && *b) {
            char ca = (*a >= 'a' && *a <= 'z') ? (char)(*a - 32) : *a;
            char cb = (*b >= 'a' && *b <= 'z') ? (char)(*b - 32) : *b;
            if (ca != cb) { match = 0; break; }
            a++; b++;
        }
        if (match && *a == *b) {
            *outSize = pack->entries[i].size;
            return pack->dataSection + pack->entries[i].offset;
        }
    }
    return NULL;
}

int read_sprite_from_pack(const SpritePack *pack, const char *name, SpriteData *sd)
{
    u32 blobSize;
    unsigned char *blob;
    SpriteHeader header;
    size_t pixelBytes;
    void *pixels;

    memset(sd, 0, sizeof(*sd));

    blob = find_in_pack(pack, name, &blobSize);
    if (blob == NULL) return 0;

    if (blobSize < sizeof(SpriteHeader)) {
        LOG("pak entry too small: %s (%u)", name, (unsigned)blobSize);
        return 0;
    }

    memcpy(&header, blob, sizeof(header));
    if (header.magic != SPRITE_MAGIC || header.width == 0 || header.height == 0) {
        LOG("pak entry bad header: %s (magic=0x%08X)", name, (unsigned)header.magic);
        return 0;
    }

    pixelBytes = (size_t)header.width * (size_t)header.height * 4u;
    if (blobSize < sizeof(SpriteHeader) + pixelBytes) {
        LOG("pak entry truncated: %s", name);
        return 0;
    }

    pixels = memalign(128, pixelBytes);
    if (pixels == NULL) {
        LOG("pak sprite alloc failed: %s", name);
        return 0;
    }

    memcpy(pixels, blob + sizeof(SpriteHeader), pixelBytes);

    {
        unsigned char *px = (unsigned char *)pixels;
        size_t total = (size_t)header.width * (size_t)header.height;
        size_t pi;
        for (pi = 0; pi < total; pi++) {
            if (px[pi * 4 + 3] == 0) {
                px[pi * 4 + 0] = 0;
                px[pi * 4 + 1] = 0;
                px[pi * 4 + 2] = 0;
            }
        }
    }

    sd->pixels = pixels;
    sd->width = header.width;
    sd->height = header.height;
    sd->offsetX = header.offsetX;
    sd->offsetY = header.offsetY;
    sd->valid = 1;

    LOG("pak sprite: %s (%ux%u)", name, (unsigned)header.width, (unsigned)header.height);
    return 1;
}

void free_spritepack(SpritePack *pack)
{
    if (pack->entries) { free(pack->entries); }
    if (pack->dataSection) { free(pack->dataSection); }
    memset(pack, 0, sizeof(*pack));
}
