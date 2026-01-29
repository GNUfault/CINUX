#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "drivers/font.h"
#include "drivers/terminal.h"
#include "drivers/mouse.h"

static uint8_t *fb;
static uint32_t fb_width;
static uint32_t fb_height;
static uint32_t fb_pitch;
static uint32_t fb_bpp;
static uint32_t cursor_x;
static uint32_t cursor_y;

static uint32_t term_fg = TERM_COLOR_FG;
static uint32_t term_bg = TERM_COLOR_BG;

void putpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_width || y >= fb_height) return;
    uint32_t bytes_per_pixel = fb_bpp / 8;
    uint32_t offset = y * fb_pitch + x * bytes_per_pixel;
    uint8_t *p = fb + offset;
    if (fb_bpp == 32)
    {
        p[0] = (color >> 16) & 0xFF;
        p[1] = (color >> 8) & 0xFF;
        p[2] = color & 0xFF;
        p[3] = (color >> 24) & 0xFF;
    }
    else if (fb_bpp == 24)
    {
        p[0] = (color >> 16) & 0xFF;
        p[1] = (color >> 8) & 0xFF;
        p[2] = color & 0xFF;
    }
}

uint32_t getpixel(uint32_t x, uint32_t y) {
    if (x >= fb_width || y >= fb_height) return 0;
    uint32_t bytes_per_pixel = fb_bpp / 8;
    uint32_t offset = y * fb_pitch + x * bytes_per_pixel;
    uint8_t *p = fb + offset;
    
    if (fb_bpp == 32)
    {
        return (p[3] << 24) | (p[0] << 16) | (p[1] << 8) | p[2];
    }
    else if (fb_bpp == 24)
    {
        return (p[0] << 16) | (p[1] << 8) | p[2];
    }
    return 0;
}

static void scroll_up(void) {
    for (uint32_t y = FONT_Y_STEP; y < fb_height; y++)
    {
        uint32_t src_offset = y * fb_pitch;
        uint32_t dst_offset = (y - FONT_Y_STEP) * fb_pitch;
        memcpy(fb + dst_offset, fb + src_offset, fb_pitch);
    }
    
    for (uint32_t y = fb_height - FONT_Y_STEP; y < fb_height; y++)
    {
        for (uint32_t x = 0; x < fb_width; x++)
        {
            putpixel(x, y, term_bg);
        }
    }
}

static void draw_char(char ch, uint32_t x, uint32_t y) {
    const uint8_t *glyph = font8x16_basic[(uint8_t)ch];

    for (uint32_t row = 0; row < FONT_HEIGHT; row++)
    {
        uint8_t line = glyph[row];
        for (uint32_t col = 0; col < FONT_WIDTH; col++)
        {
            if (line & (1 << (7 - col)))
                putpixel(x + col, y + row, term_fg);
            else
                putpixel(x + col, y + row, term_bg);
        }
    }
}

void term_putc(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += FONT_Y_STEP;
        if (cursor_y + FONT_HEIGHT > fb_height)
        {
            scroll_up();
            cursor_y = fb_height - FONT_Y_STEP;
        }
        return;
    }
    if (c == '\r') {
        cursor_x = 0;
        return;
    }
    if (c == '\b' || c == 127) {
        if (cursor_x >= FONT_WIDTH) {
            cursor_x -= FONT_WIDTH;
            draw_char(' ', cursor_x, cursor_y);
        }
        else if (cursor_y >= FONT_Y_STEP) {
            cursor_y -= FONT_Y_STEP;
            cursor_x = fb_width - FONT_WIDTH;
            draw_char(' ', cursor_x, cursor_y);
        }
        return;
    }
    if ((uint8_t)c < 32) return;
    if (cursor_x + FONT_WIDTH > fb_width) {
        cursor_x = 0;
        cursor_y += FONT_Y_STEP;
    }
    if (cursor_y + FONT_HEIGHT > fb_height) {
        scroll_up();
        cursor_y = fb_height - FONT_Y_STEP;
    }
    draw_char(c, cursor_x, cursor_y);
    cursor_x += FONT_WIDTH;
}

void term_puts(const char *s) {
    while (*s)
        term_putc(*s++);
}

void term_clear(void) {
    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            putpixel(x, y, TERM_COLOR_BG);
        }
    }

    cursor_x = 0;
    cursor_y = 0;
}

void term_init_fb(uint32_t addr, uint32_t w, uint32_t h, uint32_t pitch, uint32_t bpp) {
    fb = (uint8_t*)(uintptr_t)addr;
    fb_width = w;
    fb_height = h;
    fb_pitch = pitch;
    fb_bpp = bpp;
    term_clear();
}

void term_set_color(uint32_t fg) {
    term_fg = fg;
}

void term_reset_color(void) {
    term_fg = TERM_COLOR_FG;
}
