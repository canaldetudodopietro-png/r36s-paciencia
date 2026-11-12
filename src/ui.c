#include "ui.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define COL_X(i)  (48 + (i) * 80)
#define TOP_Y     14
#define TAB_Y     (TOP_Y + CH + 16)
#define BAR_H     22
#define TAB_MAXH  (SH - BAR_H - 6 - TAB_Y)
#define STEP_DOWN 6
#define STEP_UP   20
#define FAN_STEP  14
#define DRAG_STEP 20

#define FELT 0xFF008000u

static int tab_step_up(const Pile *p) {
    int ups = 0, downs = 0;
    for (int i = 0; i < p->n; i++) { if (p->c[i].up) ups++; else downs++; }
    if (ups <= 1) return STEP_UP;
    int s = (TAB_MAXH - CH - downs * STEP_DOWN) / (ups - 1);
    if (s > STEP_UP) s = STEP_UP;
    if (s < 8) s = 8;
    return s;
}
static int tab_card_y(const Pile *p, int idx) {
    int step = tab_step_up(p), y = TAB_Y;
    for (int i = 0; i < idx; i++) y += p->c[i].up ? step : STEP_DOWN;
    return y;
}
static int fan_n(const Game *g) {
    int f = g->draw3 ? g->fan : 1;
    if (f > g->waste.n) f = g->waste.n;
    if (f < 1 && g->waste.n > 0) f = 1;
    return f;
}

void ui_card_origin(const Game *g, int loc, int idx, int *x, int *y) {
    if (IS_TAB(loc)) {
        *x = COL_X(loc);
        *y = tab_card_y(&g->tab[loc], idx < 0 ? g->tab[loc].n : idx);
    } else if (IS_FOUND(loc)) {
        *x = COL_X(3 + loc - LOC_FOUND0); *y = TOP_Y;
    } else if (loc == LOC_WASTE) {
        int f = fan_n(g);
        *x = COL_X(1) + (f > 0 ? f - 1 : 0) * FAN_STEP; *y = TOP_Y;
    } else {
        *x = COL_X(0); *y = TOP_Y;
    }
}

int ui_hit_test(const Game *g, int mx, int my, int *loc, int *idx) {
    for (int c = 0; c < 7; c++) {
        const Pile *p = &g->tab[c];
        int x = COL_X(c);
        if (mx < x || mx >= x + CW) continue;
        if (p->n == 0) {
            if (my >= TAB_Y && my < TAB_Y + CH) { *loc = c; *idx = -1; return 1; }
            continue;
        }
        int step = tab_step_up(p);
        for (int i = p->n - 1; i >= 0; i--) {
            int y = tab_card_y(p, i);
            int h = (i == p->n - 1) ? CH : (p->c[i].up ? step : STEP_DOWN);
            if (my >= y && my < y + h) { *loc = c; *idx = i; return 1; }
        }
    }
    if (my >= TOP_Y && my < TOP_Y + CH) {
        for (int f = 0; f < 4; f++) {
            int x = COL_X(3 + f);
            if (mx >= x && mx < x + CW) { *loc = LOC_FOUND0 + f; *idx = g->found[f].n - 1; return 1; }
        }
        if (mx >= COL_X(0) && mx < COL_X(0) + CW) { *loc = LOC_STOCK; *idx = g->stock.n - 1; return 1; }
        if (g->waste.n > 0) {
            int x, y;
            ui_card_origin(g, LOC_WASTE, g->waste.n - 1, &x, &y);
            if (mx >= x && mx < x + CW) { *loc = LOC_WASTE; *idx = g->waste.n - 1; return 1; }
        } else if (mx >= COL_X(1) && mx < COL_X(1) + CW) {
            *loc = LOC_WASTE; *idx = -1; return 1;
        }
    }
    return 0;
}

static int imin(int a, int b) { return a < b ? a : b; }
static int imax(int a, int b) { return a > b ? a : b; }

int ui_drop_target(const Game *g, const Drag *d) {
    int dx = d->x - d->ox, dy = d->y - d->oy, best = -1, bestov = 700;
    for (int dst = 0; dst < LOC_FOUND0 + 4; dst++) {
        int rx, ry;
        if (!game_can_place(g, dst, d->src, d->idx)) continue;
        if (IS_TAB(dst)) {
            const Pile *p = &g->tab[dst];
            rx = COL_X(dst);
            ry = p->n ? tab_card_y(p, p->n - 1) : TAB_Y;
        } else {
            rx = COL_X(3 + dst - LOC_FOUND0); ry = TOP_Y;
        }
        int ow = imin(dx + CW, rx + CW) - imax(dx, rx);
        int oh = imin(dy + CH, ry + CH) - imax(dy, ry);
        if (ow > 0 && oh > 0 && ow * oh > bestov) { bestov = ow * oh; best = dst; }
    }
    return best;
}

static void draw_status(const Status *st) {
    char buf[64];
    r_fill(0, SH - BAR_H, SW, BAR_H, 0xFFC0C0C0);
    r_fill(0, SH - BAR_H, SW, 1, 0xFFFFFFFF);
    snprintf(buf, sizeof buf, "SCORE: %d", st->score);
    r_text(10, SH - BAR_H + 4, buf, 2, 0xFF000000);
    snprintf(buf, sizeof buf, "TIME: %d:%02d", st->secs / 60, st->secs % 60);
    r_text(200, SH - BAR_H + 4, buf, 2, 0xFF000000);
    if (st->msg && st->msg[0]) snprintf(buf, sizeof buf, "%s", st->msg);
    else snprintf(buf, sizeof buf, "DRAW %d", st->draw3 ? 3 : 1);
    r_text(SW - 10 - r_text_w(buf, 2), SH - BAR_H + 4, buf, 2, st->msg && st->msg[0] ? 0xFF900000 : 0xFF000000);
}

void ui_draw_scene(const Game *g, const Drag *d, const Status *st) {
    r_clear(FELT);
    /* monte */
    if (g->stock.n > 0) {
        if (g->stock.n > 1) r_card(COL_X(0) + 2, TOP_Y + 2, 0, 0);
        r_card(COL_X(0), TOP_Y, 0, 0);
    } else r_slot(COL_X(0), TOP_Y, 1);
    /* descarte */
    {
        int hide = (d && d->active && d->src == LOC_WASTE) ? d->n : 0;
        int n = g->waste.n - hide, f = fan_n(g);
        r_slot(COL_X(1), TOP_Y, 0);
        if (f > n) f = n;
        for (int k = 0; k < f; k++) {
            int i = n - f + k;
            r_card(COL_X(1) + k * FAN_STEP, TOP_Y, g->waste.c[i].id, 1);
        }
        if (f == 0 && n > 0) r_card(COL_X(1), TOP_Y, g->waste.c[n - 1].id, 1);
    }
    /* fundacoes */
    for (int f = 0; f < 4; f++) {
        const Pile *p = &g->found[f];
        int n = p->n - ((d && d->active && d->src == LOC_FOUND0 + f) ? d->n : 0);
        r_slot(COL_X(3 + f), TOP_Y, 2);
        if (n > 0) r_card(COL_X(3 + f), TOP_Y, p->c[n - 1].id, 1);
    }
    /* colunas */
    for (int c = 0; c < 7; c++) {
        const Pile *p = &g->tab[c];
        int n = p->n - ((d && d->active && d->src == c) ? d->n : 0);
        if (p->n == 0 || n == 0) r_slot(COL_X(c), TAB_Y, 0);
        for (int i = 0; i < n; i++) r_card(COL_X(c), tab_card_y(p, i), p->c[i].id, p->c[i].up);
    }
    /* cartas arrastadas */
    if (d && d->active) {
        const Pile *sp = game_pile_c(g, d->src);
        for (int k = 0; k < d->n; k++)
            r_card(d->x - d->ox, d->y - d->oy + k * DRAG_STEP, sp->c[d->idx + k].id, 1);
    }
    if (st) draw_status(st);
}

/* ---- animacao de vitoria (cartas quicando, deixando rastro) ---- */
static struct { int order[52], next, alive; float x, y, vx, vy; uint32_t rng; } W;
static int wrand(void) { W.rng = W.rng * 1664525u + 1013904223u; return (int)(W.rng >> 16) & 0x7FFF; }

void ui_win_start(const Game *g, uint32_t seed) {
    int k = 0;
    for (int rank = 12; rank >= 0; rank--)
        for (int f = 0; f < 4; f++) W.order[k++] = g->found[f].c[rank].id;
    W.next = 0; W.alive = 0; W.rng = seed ? seed : 1u;
}

int ui_win_step(void) {
    if (!W.alive) {
        if (W.next >= 52) return 0;
        int f = W.next % 4;
        W.x = (float)COL_X(3 + f); W.y = (float)TOP_Y;
        W.vx = (float)((wrand() & 1) ? 1 : -1) * (2.f + (wrand() % 5) * 0.6f);
        W.vy = -(float)(wrand() % 4);
        W.alive = 1;
    }
    for (int s = 0; s < 3 && W.alive; s++) {
        r_card((int)W.x, (int)W.y, W.order[W.next], 1);
        W.x += W.vx; W.y += W.vy; W.vy += 0.3f;
        float floor_y = (float)(SH - BAR_H - CH);
        if (W.y > floor_y) {
            W.y = floor_y;
            W.vy = -W.vy * 0.85f;
            if (fabsf(W.vy) < 1.5f) W.vy = -1.5f;
        }
        if (W.x < -CW || W.x > SW) { W.alive = 0; W.next++; }
    }
    return 1;
}

void ui_win_banner(void) {
    const char *a = "VOCE VENCEU!", *b = "APERTE START PARA UM NOVO JOGO";
    int w = r_text_w(b, 2) + 40;
    r_rrect((SW - w) / 2, 180, w, 90, 6, 0xFFFFF4C0, 0xFF000000);
    r_text((SW - r_text_w(a, 4)) / 2, 196, a, 4, 0xFF000000);
    r_text((SW - r_text_w(b, 2)) / 2, 240, b, 2, 0xFF900000);
}
