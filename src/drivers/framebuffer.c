#include "framebuffer.h"
#include "font.h"

#define INFO_FLAG_FRAMEBUFFER (1u << 12)
#define TYPE_RGB 1

#define MAX_COLS 256
#define MAX_ROWS 128
#define DEFAULT_ATTR 0x07
#define TAB_WIDTH 5
#define CURSOR_ROWS 2

struct info {
    unsigned int flags;
    unsigned int reserved[21];
    unsigned int address_low;
    unsigned int address_high;
    unsigned int pitch;
    unsigned int width;
    unsigned int height;
    unsigned char bpp;
    unsigned char type;
    unsigned char reserved_color[2];
    unsigned char red_position;
    unsigned char red_size;
    unsigned char green_position;
    unsigned char green_size;
    unsigned char blue_position;
    unsigned char blue_size;
} __attribute__((packed));

static const unsigned char colors[16][3] = {
    {0, 0, 0}, {0, 0, 170}, {0, 170, 0}, {0, 170, 170},
    {170, 0, 0}, {170, 0, 170}, {170, 85, 0}, {170, 170, 170},
    {85, 85, 85}, {85, 85, 255}, {85, 255, 85}, {85, 255, 255},
    {255, 85, 85}, {255, 85, 255}, {255, 255, 85}, {255, 255, 255}
};

static volatile unsigned char *base;
static unsigned int pitch;
static unsigned int width;
static unsigned int height;
static unsigned int bytes;
static unsigned int palette[16];
static unsigned short cells[MAX_COLS * MAX_ROWS];

static int cols;
static int rows;
static int cursor_x;
static int cursor_y;
static int cursor_enabled = 1;
static int redraw_needed;

static unsigned int channel(unsigned int value, unsigned int position, unsigned int size) {
    if (size == 0 || size > 8) {
        return 0;
    }
    return (value >> (8 - size)) << position;
}

static void write_pixel(volatile unsigned char *p, unsigned int color) {
    if (bytes == 4) {
        *(volatile unsigned int *)p = color;
    } else if (bytes == 3) {
        p[0] = (unsigned char)color;
        p[1] = (unsigned char)(color >> 8);
        p[2] = (unsigned char)(color >> 16);
    } else {
        *(volatile unsigned short *)p = (unsigned short)color;
    }
}

static const unsigned char *glyph(unsigned char c) {
    if (c < FONT_FIRST || c > FONT_LAST) {
        c = '?';
    }
    return font_glyphs[c - FONT_FIRST];
}

static void draw_cell(int col, int row) {
    unsigned short cell = cells[row * cols + col];
    const unsigned char *rows_bits = glyph((unsigned char)(cell & 0xFF));
    unsigned int fg = palette[(cell >> 8) & 0x0F];
    unsigned int bg = palette[(cell >> 12) & 0x0F];

    for (int line = 0; line < FONT_HEIGHT; line++) {
        volatile unsigned char *p = base + (unsigned int)(row * FONT_HEIGHT + line) * pitch
            + (unsigned int)(col * FONT_WIDTH) * bytes;
        unsigned char bits = rows_bits[line];

        for (int bit = 0; bit < FONT_WIDTH; bit++) {
            write_pixel(p, (bits & (0x80 >> bit)) ? fg : bg);
            p += bytes;
        }
    }
}

static void redraw_all(void) {
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            draw_cell(col, row);
        }
    }
}

static void cursor_draw(void) {
    if (!cursor_enabled) {
        return;
    }

    unsigned int color = palette[(cells[cursor_y * cols + cursor_x] >> 8) & 0x0F];

    for (int line = FONT_HEIGHT - CURSOR_ROWS; line < FONT_HEIGHT; line++) {
        volatile unsigned char *p = base + (unsigned int)(cursor_y * FONT_HEIGHT + line) * pitch
            + (unsigned int)(cursor_x * FONT_WIDTH) * bytes;

        for (int bit = 0; bit < FONT_WIDTH; bit++) {
            write_pixel(p, color);
            p += bytes;
        }
    }
}

static void cursor_erase(void) {
    if (cursor_enabled) {
        draw_cell(cursor_x, cursor_y);
    }
}

static void scroll_cells(void) {
    unsigned short blank = (unsigned short)((DEFAULT_ATTR << 8) | ' ');

    for (int i = 0; i < (rows - 1) * cols; i++) {
        cells[i] = cells[i + cols];
    }

    for (int i = (rows - 1) * cols; i < rows * cols; i++) {
        cells[i] = blank;
    }

    redraw_needed = 1;
}

static void newline(void) {
    cursor_x = 0;
    cursor_y++;

    if (cursor_y >= rows) {
        scroll_cells();
        cursor_y = rows - 1;
    }
}

static void put_char(char c) {
    cells[cursor_y * cols + cursor_x] = (unsigned short)((DEFAULT_ATTR << 8) | (unsigned char)c);

    if (!redraw_needed) {
        draw_cell(cursor_x, cursor_y);
    }

    cursor_x++;

    if (cursor_x >= cols) {
        newline();
    }
}

int framebuffer_init(unsigned int info_address) {
    const struct info *info = (const struct info *)info_address;

    if (info_address == 0 || !(info->flags & INFO_FLAG_FRAMEBUFFER) || info->type != TYPE_RGB) {
        return 0;
    }

    if (info->address_high != 0 || info->address_low == 0) {
        return 0;
    }

    if (info->bpp != 16 && info->bpp != 24 && info->bpp != 32) {
        return 0;
    }

    bytes = info->bpp / 8;

    if (info->width < FONT_WIDTH || info->height < FONT_HEIGHT || info->pitch < info->width * bytes) {
        return 0;
    }

    base = (volatile unsigned char *)info->address_low;
    pitch = info->pitch;
    width = info->width;
    height = info->height;

    cols = (int)(width / FONT_WIDTH);
    rows = (int)(height / FONT_HEIGHT);

    if (cols > MAX_COLS) {
        cols = MAX_COLS;
    }
    if (rows > MAX_ROWS) {
        rows = MAX_ROWS;
    }

    for (int i = 0; i < 16; i++) {
        palette[i] = channel(colors[i][0], info->red_position, info->red_size)
            | channel(colors[i][1], info->green_position, info->green_size)
            | channel(colors[i][2], info->blue_position, info->blue_size);
    }

    framebuffer_clear();
    return 1;
}

void framebuffer_print(const char *str) {
    cursor_erase();
    redraw_needed = 0;

    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            newline();
        } else if (str[i] == '\r') {
            continue;
        } else if (str[i] == '\t') {
            for (int j = 0; j < TAB_WIDTH; j++) {
                put_char(' ');
            }
        } else {
            put_char(str[i]);
        }
    }

    if (redraw_needed) {
        redraw_all();
        redraw_needed = 0;
    }

    cursor_draw();
}

void framebuffer_backspace(void) {
    if (cursor_x == 0 && cursor_y == 0) {
        return;
    }

    cursor_erase();

    if (cursor_x == 0) {
        cursor_x = cols - 1;
        cursor_y--;
    } else {
        cursor_x--;
    }

    cells[cursor_y * cols + cursor_x] = (unsigned short)((DEFAULT_ATTR << 8) | ' ');
    draw_cell(cursor_x, cursor_y);
    cursor_draw();
}

void framebuffer_clear(void) {
    unsigned short blank = (unsigned short)((DEFAULT_ATTR << 8) | ' ');

    for (int i = 0; i < cols * rows; i++) {
        cells[i] = blank;
    }

    for (unsigned int line = 0; line < height; line++) {
        volatile unsigned char *p = base + line * pitch;

        for (unsigned int pixel = 0; pixel < width; pixel++) {
            write_pixel(p, palette[0]);
            p += bytes;
        }
    }

    cursor_x = 0;
    cursor_y = 0;
    cursor_draw();
}

int framebuffer_cols(void) {
    return cols;
}

int framebuffer_rows(void) {
    return rows;
}

void framebuffer_put(int x, int y, char c, unsigned char attr) {
    if (x < 0 || y < 0 || x >= cols || y >= rows) {
        return;
    }

    cells[y * cols + x] = (unsigned short)((attr << 8) | (unsigned char)c);
    draw_cell(x, y);
}

void framebuffer_hide_cursor(void) {
    cursor_erase();
    cursor_enabled = 0;
}
