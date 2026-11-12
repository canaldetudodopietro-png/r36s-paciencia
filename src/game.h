/* Paciencia (Klondike) - logica do jogo, sem dependencia de SDL. */
#ifndef GAME_H
#define GAME_H
#include <stdint.h>

enum { SUIT_CLUBS, SUIT_DIAMONDS, SUIT_HEARTS, SUIT_SPADES };
#define CARD_SUIT(id) ((id) / 13)
#define CARD_RANK(id) ((id) % 13) /* 0 = As, 12 = Rei */
#define CARD_RED(id)  (CARD_SUIT(id) == SUIT_DIAMONDS || CARD_SUIT(id) == SUIT_HEARTS)

/* Locais: 0..6 colunas, 7..10 fundacoes, 11 descarte, 12 monte */
enum { LOC_TAB0 = 0, LOC_FOUND0 = 7, LOC_WASTE = 11, LOC_STOCK = 12 };
#define IS_TAB(l)   ((l) >= 0 && (l) < 7)
#define IS_FOUND(l) ((l) >= LOC_FOUND0 && (l) < LOC_FOUND0 + 4)

typedef struct { uint8_t id; uint8_t up; } Card;
typedef struct { int n; Card c[52]; } Pile;

typedef struct {
    Pile tab[7], found[4], stock, waste;
    int score, moves, draw3, recycles, won, fan;
} Game;

#define UNDO_MAX 300
typedef struct { Game s[UNDO_MAX]; int n; } Undo;

void game_new(Game *g, uint32_t seed, int draw3);
Pile *game_pile(Game *g, int loc);
const Pile *game_pile_c(const Game *g, int loc);
int game_draw(Game *g);                                   /* monte -> descarte / recicla */
int game_can_place(const Game *g, int dst, int src, int idx);
int game_move(Game *g, int src, int idx, int dst);        /* 1 se moveu */
int game_auto_found(Game *g, int src);                    /* topo de src -> fundacao */
int game_smart_move(Game *g, int src, int idx);           /* fundacao, senao coluna */
int game_auto_finish(Game *g);                            /* manda tudo que der p/ fundacoes */

void undo_clear(Undo *u);
void undo_push(Undo *u, const Game *g);
int undo_pop(Undo *u, Game *g);
#endif
