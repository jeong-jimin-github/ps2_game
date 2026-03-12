/**
 * @file font.c
 * @brief 비트맵 폰트 렌더링 구현
 */
#include "font.h"
#include "sprite.h"
#include "system.h"

#include <string.h>
#include <stdio.h>

/* ── UTF-8 디코딩 ────────────────────────────────── */
static u32 utf8_decode(const char **p)
{
    const unsigned char *s = (const unsigned char *)*p;
    u32 cp;

    if (s[0] < 0x80) {
        cp = s[0];
        *p += 1;
    } else if ((s[0] & 0xE0) == 0xC0) {
        cp = ((u32)(s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        *p += 2;
    } else if ((s[0] & 0xF0) == 0xE0) {
        cp = ((u32)(s[0] & 0x0F) << 12) | ((u32)(s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        *p += 3;
    } else if ((s[0] & 0xF8) == 0xF0) {
        cp = ((u32)(s[0] & 0x07) << 18) | ((u32)(s[1] & 0x3F) << 12) |
             ((u32)(s[2] & 0x3F) << 6) | (s[3] & 0x3F);
        *p += 4;
    } else {
        cp = '?';
        *p += 1;
    }
    return cp;
}

/* ── 글리프 검색 (이진 탐색) ─────────────────────── */
static const FontGlyph *find_glyph(const BitmapFont *font, u32 codepoint)
{
    int lo = 0, hi = font->glyphCount - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (font->glyphs[mid].codepoint == codepoint) {
            return &font->glyphs[mid];
        } else if (font->glyphs[mid].codepoint < codepoint) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return NULL;
}

/* ── .fnt 파일 로드 ──────────────────────────────── */
int load_font_metrics(const char *fntPath, BitmapFont *font)
{
    FILE *fp;
    unsigned char header[16];
    u32 magic, count;
    int i;

    memset(font, 0, sizeof(*font));

    fp = open_binary_file(fntPath);
    if (!fp) {
        LOG("font metrics open failed: %s", fntPath);
        return 0;
    }

    if (fread(header, 1, 16, fp) != 16) {
        fclose(fp);
        return 0;
    }

    magic = read_u32_le(header);
    if (magic != FONT_MAGIC) {
        fclose(fp);
        LOG("font magic mismatch: 0x%08X", (unsigned)magic);
        return 0;
    }

    font->atlasW = read_u16_le(header + 4);
    font->atlasH = read_u16_le(header + 6);
    font->cellW  = read_u16_le(header + 8);
    font->cellH  = read_u16_le(header + 10);
    count         = read_u32_le(header + 12);

    if (count > MAX_FONT_GLYPHS) {
        count = MAX_FONT_GLYPHS;
    }

    for (i = 0; i < (int)count; i++) {
        unsigned char entry[12];
        if (fread(entry, 1, 12, fp) != 12) break;
        font->glyphs[i].codepoint  = read_u32_le(entry);
        font->glyphs[i].atlasX     = read_u16_le(entry + 4);
        font->glyphs[i].atlasY     = read_u16_le(entry + 6);
        font->glyphs[i].advance    = entry[8];
        font->glyphs[i].bearingX   = (s8)entry[9];
        font->glyphs[i].bearingY   = (s8)entry[10];
        font->glyphs[i].glyphWidth = entry[11];
    }
    font->glyphCount = i;

    fclose(fp);
    LOG("font loaded: %s (%d glyphs, atlas %dx%d)", fntPath, i, font->atlasW, font->atlasH);
    return 1;
}

/* ── 폰트 텍스처 업로드 ─────────────────────────── */
int load_font_texture(GSGLOBAL *gsGlobal, const char *texPath, BitmapFont *font)
{
    SpriteData sd;
    memset(&sd, 0, sizeof(sd));

    if (!read_sprite_data(texPath, &sd)) {
        LOG("font texture load failed: %s", texPath);
        return 0;
    }

    upload_sprite_to_vram(gsGlobal, &sd, &font->tex, &font->texPixels, NULL, NULL);
    font->texValid = 1;
    LOG("font texture uploaded: %s (%dx%d)", texPath, sd.width, sd.height);
    return 1;
}

/* ── 문자열 렌더링 ───────────────────────────────── */
void font_draw_string(GSGLOBAL *gsGlobal, const BitmapFont *font,
                      float x, float y, int z, u64 color, const char *utf8str)
{
    float curX = x;
    const char *p = utf8str;

    if (!font->texValid || !utf8str) return;

    while (*p) {
        u32 cp = utf8_decode(&p);
        const FontGlyph *g = find_glyph(font, cp);
        float gx, gy;
        float u0, v0, u1, v1;

        if (!g) {
            curX += font->cellW / 2;
            continue;
        }

        gx = curX;
        gy = y;

        u0 = (float)g->atlasX;
        v0 = (float)g->atlasY;
        u1 = u0 + (float)font->cellW;
        v1 = v0 + (float)font->cellH;

        gsKit_prim_sprite_texture(gsGlobal, (GSTEXTURE *)&font->tex,
                                  gx, gy, u0, v0,
                                  gx + (float)font->cellW, gy + (float)font->cellH, u1, v1,
                                  z, color);

        curX += (float)g->advance;
    }
}

/* ── 문자열 너비 측정 ────────────────────────────── */
int font_measure_string(const BitmapFont *font, const char *utf8str)
{
    int width = 0;
    const char *p = utf8str;

    if (!utf8str) return 0;

    while (*p) {
        u32 cp = utf8_decode(&p);
        const FontGlyph *g = find_glyph(font, cp);
        if (g) {
            width += g->advance;
        } else {
            width += font->cellW / 2;
        }
    }
    return width;
}
