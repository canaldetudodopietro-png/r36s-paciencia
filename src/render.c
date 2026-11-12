#include "render.h"
#include "game.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

uint32_t fb[SW * SH];

void r_clear(uint32_t c) { for (int i = 0; i < SW * SH; i++) fb[i] = c; }

void r_fill(int x, int y, int w, int h, uint32_t c) {
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > SW ? SW : x + w, y1 = y + h > SH ? SH : y + h;
    for (int yy = y0; yy < y1; yy++)
        for (int xx = x0; xx < x1; xx++) fb[yy * SW + xx] = c;
}

static void rfill(int x, int y, int w, int h, int r, uint32_t c) {
    for (int dy = 0; dy < h; dy++) {
        int inset = 0;
        int d = dy < r ? dy : (dy >= h - r ? h - 1 - dy : r);
        if (d < r) {
            float yy = r - (d + 0.5f);
            inset = (int)(r - sqrtf((float)r * r - yy * yy) + 0.5f);
        }
        r_fill(x + inset, y + dy, w - 2 * inset, 1, c);
    }
}

void r_rrect(int x, int y, int w, int h, int r, uint32_t fill, uint32_t border) {
    rfill(x, y, w, h, r, border);
    rfill(x + 1, y + 1, w - 2, h - 2, r > 0 ? r - 1 : 0, fill);
}

/* ---- fonte 5x7 (colunas, bit0 = linha de cima) ---- */
static const char font_chars[] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ:-./!?+";
static const uint8_t font_data[][5] = {
    {0x00,0x00,0x00,0x00,0x00},
    {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},
    {0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},{0x36,0x49,0x49,0x49,0x36},
    {0x06,0x49,0x49,0x29,0x1E},
    {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},
    {0x00,0x36,0x36,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},
    {0x20,0x10,0x08,0x04,0x02},{0x00,0x00,0x5F,0x00,0x00},{0x02,0x01,0x51,0x09,0x06},
    {0x08,0x08,0x3E,0x08,0x08},
};

void r_text(int x, int y, const char *s, int scale, uint32_t c) {
    for (; *s; s++) {
        char ch = *s;
        if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 32);
        const char *p = strchr(font_chars, ch);
        int idx = p ? (int)(p - font_chars) : 0;
        for (int col = 0; col < 5; col++) {
            uint8_t bits = font_data[idx][col];
            for (int row = 0; row < 7; row++)
                if ((bits >> row) & 1) r_fill(x + col * scale, y + row * scale, scale, scale, c);
        }
        x += 6 * scale;
    }
}
int r_text_w(const char *s, int scale) { int n = (int)strlen(s); return n ? n * 6 * scale - scale : 0; }

/* ---- naipes: mascaras com antialiasing 3x3, em cache ---- */
static int circ(float u, float v, float cx, float cy, float r) {
    float du = u - cx, dv = v - cy;
    return du * du + dv * dv <= r * r;
}
static int heart_eq(float x, float y) {
    float a = x * x + y * y - 1.f;
    return a * a * a - x * x * y * y * y <= 0.f;
}
static int stem(float u, float v, float v0) {
    return v >= v0 && v <= 0.95f && fabsf(u) <= 0.08f + (v - v0) * 0.28f;
}

static int inside(int suit, float u, float v) {
    switch (suit) {
    case SUIT_HEARTS:
        return heart_eq(u * 1.2f, -v * 1.15f + 0.12f);
    case SUIT_SPADES:
        return heart_eq(u * 1.2f, (v + 1.f) * 1.667f - 1.f) || stem(u, v, 0.15f);
    case SUIT_DIAMONDS:
        return fabsf(u) / 0.72f + fabsf(v) <= 1.f;
    default: /* paus */
        return circ(u, v, 0.f, -0.5f, 0.4f) || circ(u, v, -0.48f, 0.08f, 0.4f) ||
               circ(u, v, 0.48f, 0.08f, 0.4f) || circ(u, v, 0.f, -0.05f, 0.3f) || stem(u, v, 0.1f);
    }
}

#define MAXSZ 48
static uint8_t *mask_cache[4][MAXSZ];

static const uint8_t *get_mask(int suit, int size) {
    if (size >= MAXSZ) size = MAXSZ - 1;
    if (!mask_cache[suit][size]) {
        uint8_t *m = (uint8_t *)malloc((size_t)size * size);
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++) {
                int acc = 0;
                for (int sy = 0; sy < 3; sy++)
                    for (int sx = 0; sx < 3; sx++) {
                        float u = ((x + (sx + 0.5f) / 3.f) / size) * 2.f - 1.f;
                        float v = ((y + (sy + 0.5f) / 3.f) / size) * 2.f - 1.f;
                        acc += inside(suit, u, v);
                    }
                m[y * size + x] = (uint8_t)(acc * 255 / 9);
            }
        mask_cache[suit][size] = m;
    }
    return mask_cache[suit][size];
}

void r_suit(int suit, int cx, int cy, int size, uint32_t color) {
    const uint8_t *m;
    int x0, y0;
    if (size >= MAXSZ) size = MAXSZ - 1;
    m = get_mask(suit, size);
    x0 = cx - size / 2; y0 = cy - size / 2;
    for (int y = 0; y < size; y++) {
        int py = y0 + y;
        if (py < 0 || py >= SH) continue;
        for (int x = 0; x < size; x++) {
            int px = x0 + x, a = m[y * size + x];
            if (!a || px < 0 || px >= SW) continue;
            uint32_t d = fb[py * SW + px];
            uint32_t r = (((d >> 16) & 255) * (255 - a) + ((color >> 16) & 255) * a) / 255;
            uint32_t g = (((d >> 8) & 255) * (255 - a) + ((color >> 8) & 255) * a) / 255;
            uint32_t b = ((d & 255) * (255 - a) + (color & 255) * a) / 255;
            fb[py * SW + px] = 0xFF000000u | (r << 16) | (g << 8) | b;
        }
    }
}

/* ---- cartas ---- */
static const char *RANKS[13] = { "A","2","3","4","5","6","7","8","9","10","J","Q","K" };
/* posicoes dos naipes: coluna 0/1/2 (esq/centro/dir) e linha em 1/12 do vao vertical */
typedef struct { uint8_t col, row; } Pip;
static const Pip PIPS[11][10] = {
    [2]  = {{1,0},{1,12}},
    [3]  = {{1,0},{1,6},{1,12}},
    [4]  = {{0,0},{2,0},{0,12},{2,12}},
    [5]  = {{0,0},{2,0},{1,6},{0,12},{2,12}},
    [6]  = {{0,0},{2,0},{0,6},{2,6},{0,12},{2,12}},
    [7]  = {{0,0},{2,0},{1,3},{0,6},{2,6},{0,12},{2,12}},
    [8]  = {{0,0},{2,0},{1,3},{0,6},{2,6},{1,9},{0,12},{2,12}},
    [9]  = {{0,0},{2,0},{0,4},{2,4},{1,6},{0,8},{2,8},{0,12},{2,12}},
    [10] = {{0,0},{2,0},{1,2},{0,4},{2,4},{0,8},{2,8},{1,10},{0,12},{2,12}},
};

void r_card(int x, int y, int id, int up) {
    if (!up) {
        r_rrect(x, y, CW, CH, 4, 0xFFFFFFFF, 0xFF000000);
        for (int yy = y + 4; yy < y + CH - 4; yy++)
            for (int xx = x + 4; xx < x + CW - 4; xx++) {
                int a = xx - x, b = yy - y;
                int line = ((a + b) % 8 == 0) || (((a - b) % 8 + 8) % 8 == 0);
                if (xx >= 0 && xx < SW && yy >= 0 && yy < SH)
                    fb[yy * SW + xx] = line ? 0xFF6E6EFF : 0xFF0000A8;
            }
        return;
    }
    int suit = CARD_SUIT(id), rank = CARD_RANK(id);
    uint32_t col = CARD_RED(id) ? 0xFFD80000 : 0xFF000000;
    const char *rs = RANKS[rank];
    int rw = r_text_w(rs, 2);
    r_rrect(x, y, CW, CH, 4, 0xFFFFFFFF, 0xFF000000);
    r_text(x + 4, y + 4, rs, 2, col);
    r_suit(suit, x + 4 + rw + 9, y + 10, 11, col);
    if (rank == 0) {
        r_suit(suit, x + CW / 2, y + CH / 2 + 2, 34, col);
    } else if (rank >= 10) {
        r_rrect(x + 10, y + 22, CW - 20, 44, 2, 0xFFFFF4C0, col);
        r_text(x + 22, y + 26, rs, 4, col);
        r_suit(suit, x + CW / 2, y + 58, 12, col);
    } else {
        static const int colx[3] = { 19, 32, 45 };
        for (int i = 0; i < rank + 1; i++) {
            Pip p = PIPS[rank + 1][i];
            r_suit(suit, x + colx[p.col], y + 28 + p.row * 44 / 12, 14, col);
        }
    }
}

void r_slot(int x, int y, int kind) {
    r_rrect(x, y, CW, CH, 4, 0xFF007000, 0xFF004C00);
    if (kind == 1) {
        int cx = x + CW / 2, cy = y + CH / 2;
        for (int yy = -16; yy <= 16; yy++)
            for (int xx = -16; xx <= 16; xx++) {
                int d = xx * xx + yy * yy;
                if (d <= 16 * 16 && d >= 11 * 11) {
                    int px = cx + xx, py = cy + yy;
                    if (px >= 0 && px < SW && py >= 0 && py < SH) fb[py * SW + px] = 0xFF00B000;
                }
            }
    } else if (kind == 2) {
        r_text(x + (CW - r_text_w("A", 4)) / 2, y + (CH - 28) / 2, "A", 4, 0xFF009000);
    }
}

/* ---- cursor: seta preta com contorno branco ---- */
static const char *CUR[19] = {
    "X           ","XX          ","XXX         ","XXXX        ","XXXXX       ","XXXXXX      ",
    "XXXXXXX     ","XXXXXXXX    ","XXXXXXXXX   ","XXXXXXXXXX  ","XXXXXXXXXXX ","XXXXXXXXXXXX",
    "XXXXXXX     ","XXX XXX     ","XX  XXX     ","X    XXX    ","     XXX    ","      XXX   ",
    "      XXX   " };

void r_cursor(uint32_t *buf, int x, int y) {
    for (int pass = 0; pass < 2; pass++)
        for (int dy = (pass ? 0 : -1); dy <= (pass ? 0 : 1); dy++)
            for (int dx = (pass ? 0 : -1); dx <= (pass ? 0 : 1); dx++)
                for (int r = 0; r < 19; r++)
                    for (int c = 0; c < 12; c++) {
                        if (CUR[r][c] != 'X') continue;
                        int px = x + c + dx, py = y + r + dy;
                        if (px >= 0 && px < SW && py >= 0 && py < SH)
                            buf[py * SW + px] = pass ? 0xFF000000 : 0xFFFFFFFF;
                    }
}
