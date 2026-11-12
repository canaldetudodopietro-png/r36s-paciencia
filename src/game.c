#include "game.h"
#include <string.h>

static uint32_t rng_next(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *s = x;
}

const Pile *game_pile_c(const Game *g, int loc) {
    if (IS_TAB(loc)) return &g->tab[loc];
    if (IS_FOUND(loc)) return &g->found[loc - LOC_FOUND0];
    if (loc == LOC_WASTE) return &g->waste;
    return &g->stock;
}
Pile *game_pile(Game *g, int loc) { return (Pile *)game_pile_c(g, loc); }

static void add_score(Game *g, int pts) {
    g->score += pts;
    if (g->score < 0) g->score = 0;
}

void game_new(Game *g, uint32_t seed, int draw3) {
    uint8_t deck[52];
    int i, k = 0;
    uint32_t s = seed ? seed : 0x9E3779B9u;
    memset(g, 0, sizeof *g);
    for (i = 0; i < 52; i++) deck[i] = (uint8_t)i;
    for (i = 0; i < 8; i++) rng_next(&s);
    for (i = 51; i > 0; i--) {
        int j = (int)(rng_next(&s) % (uint32_t)(i + 1));
        uint8_t t = deck[i]; deck[i] = deck[j]; deck[j] = t;
    }
    for (int col = 0; col < 7; col++)
        for (int row = 0; row <= col; row++) {
            Card c = { deck[k++], (uint8_t)(row == col) };
            g->tab[col].c[g->tab[col].n++] = c;
        }
    while (k < 52) {
        Card c = { deck[k++], 0 };
        g->stock.c[g->stock.n++] = c;
    }
    g->draw3 = draw3 ? 1 : 0;
}

int game_draw(Game *g) {
    if (g->stock.n > 0) {
        int k = g->draw3 ? 3 : 1;
        if (k > g->stock.n) k = g->stock.n;
        for (int i = 0; i < k; i++) {
            Card c = g->stock.c[--g->stock.n];
            c.up = 1;
            g->waste.c[g->waste.n++] = c;
        }
        g->fan = k;
        g->moves++;
        return 1;
    }
    if (g->waste.n > 0) {
        while (g->waste.n > 0) {
            Card c = g->waste.c[--g->waste.n];
            c.up = 0;
            g->stock.c[g->stock.n++] = c;
        }
        g->recycles++;
        add_score(g, g->draw3 ? -20 : -100);
        g->fan = 0;
        g->moves++;
        return 1;
    }
    return 0;
}

int game_can_place(const Game *g, int dst, int src, int idx) {
    const Pile *sp, *dp;
    int cnt, id;
    if (src == LOC_STOCK || src == dst) return 0;
    if (!(IS_TAB(dst) || IS_FOUND(dst))) return 0;
    sp = game_pile_c(g, src);
    if (idx < 0 || idx >= sp->n || !sp->c[idx].up) return 0;
    if (!IS_TAB(src) && idx != sp->n - 1) return 0;
    cnt = sp->n - idx;
    id = sp->c[idx].id;
    dp = game_pile_c(g, dst);
    if (IS_FOUND(dst)) {
        if (cnt != 1 || IS_FOUND(src)) return 0;
        if (dp->n == 0) return CARD_RANK(id) == 0;
        {
            int top = dp->c[dp->n - 1].id;
            return CARD_SUIT(top) == CARD_SUIT(id) && CARD_RANK(id) == CARD_RANK(top) + 1;
        }
    }
    if (dp->n == 0) return CARD_RANK(id) == 12;
    {
        Card top = dp->c[dp->n - 1];
        if (!top.up) return 0;
        return CARD_RED(top.id) != CARD_RED(id) && CARD_RANK(top.id) == CARD_RANK(id) + 1;
    }
}

static void check_win(Game *g) {
    for (int f = 0; f < 4; f++) if (g->found[f].n != 13) return;
    g->won = 1;
}

int game_move(Game *g, int src, int idx, int dst) {
    Pile *sp, *dp;
    int cnt, pts = 0;
    if (!game_can_place(g, dst, src, idx)) return 0;
    sp = game_pile(g, src);
    dp = game_pile(g, dst);
    cnt = sp->n - idx;
    for (int i = 0; i < cnt; i++) dp->c[dp->n++] = sp->c[idx + i];
    sp->n = idx;
    if (IS_TAB(dst)) {
        if (src == LOC_WASTE) pts += 5;
        else if (IS_FOUND(src)) pts -= 15;
    } else {
        pts += 10;
    }
    if (IS_TAB(src) && sp->n > 0 && !sp->c[sp->n - 1].up) {
        sp->c[sp->n - 1].up = 1;
        pts += 5;
    }
    if (src == LOC_WASTE && g->fan > 1) g->fan--;
    add_score(g, pts);
    g->moves++;
    check_win(g);
    return 1;
}

int game_auto_found(Game *g, int src) {
    const Pile *sp;
    if (src == LOC_STOCK || IS_FOUND(src)) return 0;
    sp = game_pile_c(g, src);
    if (sp->n == 0) return 0;
    for (int f = 0; f < 4; f++)
        if (game_can_place(g, LOC_FOUND0 + f, src, sp->n - 1))
            return game_move(g, src, sp->n - 1, LOC_FOUND0 + f);
    return 0;
}

int game_smart_move(Game *g, int src, int idx) {
    const Pile *sp = game_pile_c(g, src);
    if (idx == sp->n - 1 && game_auto_found(g, src)) return 1;
    for (int pass = 0; pass < 2; pass++)
        for (int t = 0; t < 7; t++) {
            int empty = g->tab[t].n == 0;
            if (t == src || empty != (pass == 1)) continue;
            if (empty && IS_TAB(src) && idx == 0) continue; /* Rei ja esta no fundo */
            if (game_move(g, src, idx, t)) return 1;
        }
    return 0;
}

int game_auto_finish(Game *g) {
    int total = 0, changed = 1;
    while (changed && !g->won) {
        changed = 0;
        if (game_auto_found(g, LOC_WASTE)) { changed = 1; total++; }
        for (int t = 0; t < 7; t++)
            if (game_auto_found(g, t)) { changed = 1; total++; }
    }
    return total;
}

void undo_clear(Undo *u) { u->n = 0; }
void undo_push(Undo *u, const Game *g) {
    if (u->n == UNDO_MAX) {
        memmove(&u->s[0], &u->s[1], sizeof(Game) * (UNDO_MAX - 1));
        u->n--;
    }
    u->s[u->n++] = *g;
}
int undo_pop(Undo *u, Game *g) {
    if (u->n == 0) return 0;
    *g = u->s[--u->n];
    return 1;
}
