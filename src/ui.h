/* Layout, desenho da mesa, hit-test e animacao de vitoria. Sem SDL. */
#ifndef UI_H
#define UI_H
#include "game.h"
#include "render.h"

typedef struct { int active, src, idx, n, ox, oy, sx, sy, x, y; } Drag;
typedef struct { int score, secs; const char *msg; int draw3; } Status;

void ui_draw_scene(const Game *g, const Drag *d, const Status *st);
int  ui_hit_test(const Game *g, int mx, int my, int *loc, int *idx);
void ui_card_origin(const Game *g, int loc, int idx, int *x, int *y);
int  ui_drop_target(const Game *g, const Drag *d); /* -1 se nenhum destino valido */

void ui_win_start(const Game *g, uint32_t seed);   /* chamar depois de ui_draw_scene */
int  ui_win_step(void);                            /* 1 enquanto animando, 0 ao terminar */
void ui_win_banner(void);
#endif
