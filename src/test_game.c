/* Teste de invariantes da logica: jogadas aleatorias legais em muitas partidas. */
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int check(const Game *g, const char *where) {
    int seen[52] = {0}, total = 0;
    for (int l = 0; l < 13; l++) {
        const Pile *p = game_pile_c(g, l);
        for (int i = 0; i < p->n; i++) {
            int id = p->c[i].id;
            if (id > 51 || seen[id]++) { printf("FALHA(%s): carta duplicada/invalida %d\n", where, id); return 0; }
            total++;
        }
        if (IS_TAB(l) && p->n && !p->c[p->n - 1].up) { printf("FALHA(%s): topo virado na coluna %d\n", where, l); return 0; }
        if (IS_TAB(l))
            for (int i = 0; i + 1 < p->n; i++)
                if (p->c[i].up && p->c[i+1].up) {
                    int a = p->c[i].id, b = p->c[i+1].id;
                    if (CARD_RED(a) == CARD_RED(b) || CARD_RANK(a) != CARD_RANK(b) + 1) { printf("FALHA(%s): sequencia invalida col %d\n", where, l); return 0; }
                }
        if (IS_TAB(l))
            for (int i = 0; i + 1 < p->n; i++)
                if (p->c[i].up && !p->c[i+1].up) { printf("FALHA(%s): carta virada sobre aberta\n", where); return 0; }
        if (IS_FOUND(l))
            for (int i = 0; i < p->n; i++)
                if (CARD_RANK(p->c[i].id) != i || CARD_SUIT(p->c[i].id) != CARD_SUIT(p->c[0].id)) { printf("FALHA(%s): fundacao invalida\n", where); return 0; }
        if ((l == LOC_STOCK) )
            for (int i = 0; i < p->n; i++) if (p->c[i].up) { printf("FALHA(%s): monte com carta aberta\n", where); return 0; }
        if (l == LOC_WASTE)
            for (int i = 0; i < p->n; i++) if (!p->c[i].up) { printf("FALHA(%s): descarte com carta virada\n", where); return 0; }
    }
    if (total != 52) { printf("FALHA(%s): total %d\n", where, total); return 0; }
    return 1;
}

int main(void) {
    static Undo u;
    int wins = 0, games = 2000;
    srand(12345);
    for (int gi = 0; gi < games; gi++) {
        Game g;
        game_new(&g, (uint32_t)(gi * 7919 + 1), gi & 1);
        if (!check(&g, "new")) return 1;
        if (g.stock.n != 24) { printf("monte deveria ter 24\n"); return 1; }
        undo_clear(&u);
        for (int step = 0; step < 3000 && !g.won; step++) {
            Game before = g;
            int did = 0;
            int r = rand() % 10;
            if (r < 2) did = game_draw(&g);
            else if (r < 4) did = game_auto_found(&g, rand() % 2 ? LOC_WASTE : rand() % 7);
            else {
                int src = rand() % 12; if (src > 7 + 3 && src != LOC_WASTE) src = LOC_WASTE;
                const Pile *p = game_pile_c(&g, src);
                if (p->n) did = game_smart_move(&g, src, IS_TAB(src) ? rand() % p->n : p->n - 1);
            }
            if (!did) { if (memcmp(&before, &g, sizeof g)) { printf("FALHA: estado mudou sem retorno 1\n"); return 1; } }
            else {
                undo_push(&u, &before);
                if (!check(&g, "step")) return 1;
            }
            if (rand() % 50 == 0 && u.n) { Game b; undo_pop(&u, &b); g = b; if (!check(&g, "undo")) return 1; }
        }
        game_auto_finish(&g);
        if (!check(&g, "finish")) return 1;
        wins += g.won;
    }
    /* validacao de regras especificas */
    Game g; game_new(&g, 1, 0);
    memset(&g, 0, sizeof g);
    g.tab[0].c[0] = (Card){ 12 + 13*0, 1 }; g.tab[0].n = 1;             /* Rei de paus */
    g.tab[1].c[0] = (Card){ 11 + 13*2, 1 }; g.tab[1].n = 1;             /* Dama de copas */
    if (!game_can_place(&g, 0, 1, 0)) { puts("Dama vermelha deveria ir no Rei preto"); return 1; }
    g.tab[1].c[0].id = 11 + 13*3; /* Dama de espadas (preta) */
    if (game_can_place(&g, 0, 1, 0)) { puts("Dama preta nao pode ir no Rei preto"); return 1; }
    if (game_can_place(&g, 2, 0, 0) == 0) { puts("Rei deveria ir para coluna vazia"); return 1; }
    printf("OK: %d partidas aleatorias, %d vitorias por acaso, invariantes preservados.\n", games, wins);
    return 0;
}
