/* Paciencia (Klondike) para R36S / PortMaster. SDL2, renderizador por software.
 * Entrada: mouse e teclado sinteticos do gptokeyb2 (ver paciencia.ini). */
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ui.h"

static Game G;
static Undo U;
static Drag D;
static int next_draw3 = 0;
static int mx = SW / 2, my = SH / 2;
static int scene_dirty = 1, present_dirty = 1, running = 1;
static int win_active = 0, win_done = 0;
static Uint32 t_start = 0, t_end = 0;
static int timer_on = 0;
static char msg[48];
static Uint32 msg_until = 0, confirm_deal_until = 0, confirm_quit_until = 0;
static uint32_t out[SW * SH];

static void set_msg(const char *m) {
    snprintf(msg, sizeof msg, "%s", m);
    msg_until = SDL_GetTicks() + 3000;
    scene_dirty = 1;
}

static int elapsed(void) {
    if (!timer_on) return 0;
    return (int)(((G.won ? t_end : SDL_GetTicks()) - t_start) / 1000);
}

static void new_deal(void) {
    game_new(&G, (uint32_t)(SDL_GetTicks() * 2654435761u) ^ (uint32_t)time(NULL), next_draw3);
    undo_clear(&U);
    memset(&D, 0, sizeof D);
    timer_on = 0; win_active = win_done = 0; msg[0] = 0;
    scene_dirty = 1;
}

static void draw_scene(void) {
    Status st = { G.score, elapsed(), msg, G.draw3 };
    ui_draw_scene(&G, &D, &st);
    scene_dirty = 0; present_dirty = 1;
}

/* chamado depois de qualquer acao que alterou o jogo */
static void after_action(const Game *before) {
    undo_push(&U, before);
    if (!timer_on && G.moves > 0) { timer_on = 1; t_start = SDL_GetTicks(); }
    scene_dirty = 1;
    if (G.won && !win_active) {
        t_end = SDL_GetTicks();
        win_active = 1; win_done = 0;
        draw_scene();
        ui_win_start(&G, SDL_GetTicks());
    }
}

static void do_draw(void) { Game b = G; if (game_draw(&G)) after_action(&b); }
static void do_auto_found(int loc) { Game b = G; if (game_auto_found(&G, loc)) after_action(&b); }
static void do_smart(int loc, int idx) { Game b = G; if (game_smart_move(&G, loc, idx)) after_action(&b); }
static void do_finish(void) { Game b = G; if (game_auto_finish(&G)) after_action(&b); }

static void on_left_down(int x, int y) {
    int loc, idx;
    if (win_active) { if (win_done) new_deal(); return; }
    if (!ui_hit_test(&G, x, y, &loc, &idx)) return;
    if (loc == LOC_STOCK) { do_draw(); return; }
    if (idx < 0) return;
    const Pile *p = game_pile_c(&G, loc);
    if (!p->c[idx].up) return;
    D.active = 1; D.src = loc; D.idx = idx; D.n = p->n - idx;
    int cx, cy;
    ui_card_origin(&G, loc, idx, &cx, &cy);
    D.ox = x - cx; D.oy = y - cy; D.sx = x; D.sy = y; D.x = x; D.y = y;
    scene_dirty = 1;
}

static void on_left_up(int x, int y) {
    if (!D.active) return;
    D.x = x; D.y = y;
    int moved = abs(x - D.sx) + abs(y - D.sy) > 6;
    Drag d = D;
    D.active = 0;
    scene_dirty = 1;
    if (!moved) {                       /* clique simples: manda para a fundacao se der */
        if (d.idx == game_pile_c(&G, d.src)->n - 1) do_auto_found(d.src);
        return;
    }
    int dst = ui_drop_target(&G, &d);
    if (dst >= 0) { Game b = G; if (game_move(&G, d.src, d.idx, dst)) after_action(&b); }
}

static void on_right_down(int x, int y) {
    int loc, idx;
    if (win_active || D.active) return;
    if (!ui_hit_test(&G, x, y, &loc, &idx) || loc == LOC_STOCK || idx < 0) return;
    if (!game_pile_c(&G, loc)->c[idx].up) return;
    do_smart(loc, idx);
}

static void on_key(SDL_Keycode k) {
    Uint32 now = SDL_GetTicks();
    switch (k) {
    case SDLK_F2:
        if (G.moves > 0 && !G.won && now > confirm_deal_until) {
            confirm_deal_until = now + 3000;
            set_msg("START DE NOVO P/ NOVO JOGO");
        } else new_deal();
        break;
    case SDLK_u:
        if (!D.active && !win_active) {
            if (undo_pop(&U, &G)) scene_dirty = 1; else set_msg("NADA PARA DESFAZER");
        }
        break;
    case SDLK_t:
        next_draw3 = !next_draw3;
        if (G.moves == 0 && !win_active) new_deal();
        else { set_msg(next_draw3 ? "PROXIMO JOGO: 3 CARTAS" : "PROXIMO JOGO: 1 CARTA"); }
        break;
    case SDLK_a:
        if (!D.active && !win_active) do_finish();
        break;
    case SDLK_ESCAPE:
        if (now <= confirm_quit_until) running = 0;
        else { confirm_quit_until = now + 3000; set_msg("SELECT DE NOVO P/ SAIR"); }
        break;
    default: break;
    }
}

static void handle_event(const SDL_Event *e) {
    switch (e->type) {
    case SDL_QUIT: running = 0; break;
    case SDL_MOUSEMOTION:
        mx = e->motion.x; my = e->motion.y;
        if (mx < 0) mx = 0;
        if (mx >= SW) mx = SW - 1;
        if (my < 0) my = 0;
        if (my >= SH) my = SH - 1;
        if (D.active) { D.x = mx; D.y = my; scene_dirty = 1; }
        present_dirty = 1;
        break;
    case SDL_MOUSEBUTTONDOWN:
        mx = e->button.x; my = e->button.y;
        if (e->button.button == SDL_BUTTON_LEFT) on_left_down(mx, my);
        else if (e->button.button == SDL_BUTTON_RIGHT) on_right_down(mx, my);
        break;
    case SDL_MOUSEBUTTONUP:
        mx = e->button.x; my = e->button.y;
        if (e->button.button == SDL_BUTTON_LEFT) on_left_up(mx, my);
        break;
    case SDL_KEYDOWN:
        if (!e->key.repeat) on_key(e->key.keysym.sym);
        break;
    default: break;
    }
}

int main(int argc, char **argv) {
    int windowed = 0;
    for (int i = 1; i < argc; i++) if (!strcmp(argv[i], "--windowed")) windowed = 1;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1; }
    SDL_Window *win = SDL_CreateWindow("Paciencia", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        windowed ? 960 : SW, windowed ? 720 : SH,
        windowed ? SDL_WINDOW_RESIZABLE : SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!win) { fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); return 1; }
    /* Renderer por software de proposito: evita o erro "Texture format not supported" em GPUs Mali/GLES2 */
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) { fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); return 1; }
    SDL_RenderSetLogicalSize(ren, SW, SH);
    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SW, SH);
    if (!tex) { fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError()); return 1; }
    SDL_ShowCursor(SDL_DISABLE);

    new_deal();
    int last_sec = -1;
    while (running) {
        SDL_Event e;
        int busy = win_active && !win_done;
        int got = SDL_WaitEventTimeout(&e, busy ? 16 : 100);
        while (got) { handle_event(&e); got = SDL_PollEvent(&e); }

        Uint32 now = SDL_GetTicks();
        if (msg[0] && now > msg_until) { msg[0] = 0; scene_dirty = 1; }
        if (!win_active) {
            int sec = elapsed();
            if (sec != last_sec) { last_sec = sec; scene_dirty = 1; }
        }
        if (win_active) {
            if (!win_done) {
                if (!ui_win_step()) { win_done = 1; ui_win_banner(); }
                present_dirty = 1;
            }
        } else if (scene_dirty) draw_scene();

        if (present_dirty) {
            memcpy(out, fb, sizeof out);
            r_cursor(out, mx, my);
            SDL_UpdateTexture(tex, NULL, out, SW * 4);
            SDL_RenderClear(ren);
            SDL_RenderCopy(ren, tex, NULL, NULL);
            SDL_RenderPresent(ren);
            present_dirty = 0;
        }
    }
    SDL_DestroyTexture(tex); SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit();
    return 0;
}
