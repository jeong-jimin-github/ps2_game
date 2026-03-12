/**
 * @file font.h
 * @brief 비트맵 폰트 렌더링 모듈
 *
 * .fnt + .ps2tex 파일로부터 비트맵 폰트를 로드하고
 * UTF-8 문자열을 GS에 렌더링합니다.
 */
#ifndef FONT_H
#define FONT_H

#include "types.h"

#define FONT_MAGIC 0x31544E46u  /* 'FNT1' LE */
#define MAX_FONT_GLYPHS 512

typedef struct {
    u32 codepoint;
    u16 atlasX;
    u16 atlasY;
    u8  advance;
    s8  bearingX;
    s8  bearingY;
    u8  glyphWidth;
} FontGlyph;

typedef struct {
    GSTEXTURE tex;
    void *texPixels;
    int texValid;
    u16 atlasW, atlasH;
    u16 cellW, cellH;
    FontGlyph glyphs[MAX_FONT_GLYPHS];
    int glyphCount;
} BitmapFont;

/** Load font metrics from .fnt file into RAM */
int load_font_metrics(const char *fntPath, BitmapFont *font);

/** Load font atlas texture from .ps2tex and upload to VRAM */
int load_font_texture(GSGLOBAL *gsGlobal, const char *texPath, BitmapFont *font);

/** Draw a UTF-8 string at (x, y) with given color and z-depth */
void font_draw_string(GSGLOBAL *gsGlobal, const BitmapFont *font,
                      float x, float y, int z, u64 color, const char *utf8str);

/** Measure the pixel width of a UTF-8 string */
int font_measure_string(const BitmapFont *font, const char *utf8str);

#endif /* FONT_H */
