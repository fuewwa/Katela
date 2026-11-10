#ifndef FONT_H
#define FONT_H

#define FONT_WIDTH 8
#define FONT_HEIGHT 16
#define FONT_FIRST 32
#define FONT_LAST 126
#define FONT_COUNT (FONT_LAST - FONT_FIRST + 1)

extern const unsigned char font_glyphs[FONT_COUNT][FONT_HEIGHT];

#endif
