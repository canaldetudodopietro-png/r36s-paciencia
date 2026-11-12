/* Renderizador por software em um framebuffer 640x480 ARGB. Sem SDL. */
#ifndef RENDER_H
#define RENDER_H
#include <stdint.h>

#define SW 640
#define SH 480
#define CW 64
#define CH 88

extern uint32_t fb[SW * SH];

void r_clear(uint32_t c);
void r_fill(int x, int y, int w, int h, uint32_t c);
void r_rrect(int x, int y, int w, int h, int r, uint32_t fill, uint32_t border);
void r_text(int x, int y, const char *s, int scale, uint32_t c);
int  r_text_w(const char *s, int scale);
void r_suit(int suit, int cx, int cy, int size, uint32_t color);
void r_card(int x, int y, int id, int up);
void r_slot(int x, int y, int kind); /* 0 vazio, 1 monte vazio (recicla), 2 fundacao (A) */
void r_cursor(uint32_t *buf, int x, int y);
#endif
