/* Captura headless: renderiza a mesa com o proprio codigo do jogo e grava um PPM. */
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void save(const char *path) {
    FILE *f = fopen(path, "wb");
    fprintf(f, "P6\n%d %d\n255\n", SW, SH);
    for (int i = 0; i < SW * SH; i++) {
        unsigned char rgb[3] = { (fb[i] >> 16) & 255, (fb[i] >> 8) & 255, fb[i] & 255 };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "play";
    uint32_t seed = argc > 2 ? (uint32_t)atoi(argv[2]) : 7;
    int draw3 = argc > 3 ? atoi(argv[3]) : 0;
    Game g; Status st = { 0, 0, NULL, draw3 }; Drag d; memset(&d, 0, sizeof d);
    game_new(&g, seed, draw3);
    if (!strcmp(mode, "play")) {
        for (int step = 0; step < 90; step++) {
            if (step % 3 == 0) game_draw(&g);
            for (int s = 0; s < 8; s++) if (g.waste.n) game_smart_move(&g, LOC_WASTE, g.waste.n - 1);
            for (int t = 0; t < 7; t++) if (g.tab[t].n) game_smart_move(&g, t, g.tab[t].n - 1);
        }
        st.score = g.score; st.secs = 154;
        ui_draw_scene(&g, &d, &st);
        save("/tmp/shot.ppm");
    } else if (!strcmp(mode, "deal")) {
        st.secs = 0; ui_draw_scene(&g, &d, &st); save("/tmp/shot.ppm");
    } else if (!strcmp(mode, "drag")) {
        for (int i = 0; i < 6; i++) game_draw(&g);
        d.active = 1; d.src = LOC_WASTE; d.idx = g.waste.n - 1; d.n = 1; d.ox = 30; d.oy = 40; d.x = 300; d.y = 250;
        st.score = g.score; ui_draw_scene(&g, &d, &st);
        r_cursor(fb, d.x, d.y); save("/tmp/shot.ppm");
    } else if (!strcmp(mode, "win")) {
        /* monta fundacoes completas */
        memset(&g, 0, sizeof g);
        for (int s = 0; s < 4; s++) for (int r = 0; r < 13; r++) { g.found[s].c[r].id = (uint8_t)(s * 13 + r); g.found[s].c[r].up = 1; }
        for (int s = 0; s < 4; s++) g.found[s].n = 13;
        st.score = 740; st.secs = 301;
        ui_draw_scene(&g, &d, &st); ui_win_start(&g, 99);
        for (int i = 0; i < 700 && ui_win_step(); i++) {}
        ui_win_banner(); save("/tmp/shot.ppm");
    }
    return 0;
}
