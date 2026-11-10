#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define CURSOR_SPEED 6

// Regras e Estruturas do Paciência
#define CARD_WIDTH 62
#define CARD_HEIGHT 88
#define TOTAL_CARDS 52

typedef enum { COPAS, OUROS, PAUS, ESPADAS } Naipe;

typedef struct {
    int valor;    // 1 (Ás) até 13 (Rei)
    Naipe naipe;  // 0 a 3
    bool aberta;  // Se a carta está virada para cima
} Carta;

// Estruturas para organizar o tabuleiro
typedef struct {
    Carta cartas[TOTAL_CARDS];
    int topo;
} Pilha;

// Estado Global do Jogo
Carta baralho_completo[TOTAL_CARDS];
Pilha estoque;               // Pilha de compra (virada para baixo)
Pilha descarte;              // Pilha de descarte (virada para cima)
Pilha colunas[7];            // As 7 colunas principais do jogo
Pilha fundacoes[4];          // As 4 pilhas de destino (Ás até Rei)

// Estrutura do Mouse Virtual
typedef struct {
    float x;
    float y;
    bool left_click;
} MouseSim;

// --- LÓGICA DO JOGO PACIÊNCIA ---

void InicializarBaralho() {
    int index = 0;
    for (int n = 0; n < 4; n++) {
        for (int v = 1; v <= 13; v++) {
            baralho_completo[index].valor = v;
            baralho_completo[index].naipe = (Naipe)n;
            baralho_completo[index].aberta = false;
            index++;
        }
    }
}

void Embaralhar() {
    srand(time(NULL));
    for (int i = TOTAL_CARDS - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Carta temp = baralho_completo[i];
        baralho_completo[i] = baralho_completo[j];
        baralho_completo[j] = temp;
    }
}

void DistribuirJogo() {
    int card_idx = 0;
    
    // Inicializar topos das pilhas
    estoque.topo = 0;
    descarte.topo = 0;
    for (int i = 0; i < 7; i++) colunas[i].topo = 0;
    for (int i = 0; i < 4; i++) fundacoes[i].topo = 0;

    // Distribuir as 7 colunas em cascata
    for (int i = 0; i < 7; i++) {
        for (int j = i; j < 7; j++) {
            colunas[j].cartas[colunas[j].topo] = baralho_completo[card_idx++];
            if (i == j) {
                colunas[j].cartas[colunas[j].topo].aberta = true; // Abre a última carta
            }
            colunas[j].topo++;
        }
    }

    // O restante vai para o estoque (pilha de compra)
    while (card_idx < TOTAL_CARDS) {
        estoque.cartas[estoque.topo] = baralho_completo[card_idx++];
        estoque.topo++;
    }
}

// --- CONTROLES E RENDERIZAÇÃO ---

void HandleInput(SDL_Event *event, bool *running, MouseSim *mouse) {
    while (SDL_PollEvent(event)) {
        if (event->type == SDL_QUIT) {
            *running = false;
        }
        if (event->type == SDL_KEYDOWN) {
            switch (event->key.keysym.sym) {
                case SDLK_ESCAPE: 
                    *running = false; 
                    break;
                case SDLK_z: // Botão A no R36S (Ação de clique)
                case SDLK_SPACE:
                    mouse->left_click = true;
                    break;
            }
        }
        if (event->type == SDL_KEYUP) {
            switch (event->key.keysym.sym) {
                case SDLK_z:
                case SDLK_SPACE:
                    mouse->left_click = false;
                    break;
            }
        }
    }

    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_UP])    mouse->y -= CURSOR_SPEED;
    if (state[SDL_SCANCODE_DOWN])  mouse->y += CURSOR_SPEED;
    if (state[SDL_SCANCODE_LEFT])  mouse->x -= CURSOR_SPEED;
    if (state[SDL_SCANCODE_RIGHT]) mouse->x += CURSOR_SPEED;

    if (mouse->x < 0) mouse->x = 0;
    if (mouse->y < 0) mouse->y = 0;
    if (mouse->x > SCREEN_WIDTH) mouse->x = SCREEN_WIDTH;
    if (mouse->y > SCREEN_HEIGHT) mouse->y = SCREEN_HEIGHT;
}

void DesenharCarta(SDL_Renderer *renderer, int x, int y, Carta carta) {
    SDL_Rect rect = { x, y, CARD_WIDTH, CARD_HEIGHT };
    
    if (!carta.aberta) {
        // Carta virada para baixo: Azul clássico
        SDL_SetRenderDrawColor(renderer, 0, 0, 170, 255);
        SDL_RenderFillRect(renderer, &rect);
    } else {
        // Carta virada para cima: Fundo branco
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &rect);
        
        // Cor do naipe (Vermelho para Copas/Ouros, Preto para Paus/Espadas)
        if (carta.naipe == COPAS || carta.naipe == OUROS) {
            SDL_SetRenderDrawColor(renderer, 220, 0, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        }
        
        // Renderiza um indicador visual simples para o valor e o naipe
        // (Isso substitui o uso de texturas complexas provisoriamente)
        SDL_Rect mini_simbolo = { x + 5, y + 5, 12, 12 };
        SDL_RenderFillRect(renderer, &mini_simbolo);
    }
    
    // Borda da Carta
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &rect);
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;

    SDL_Window *window = SDL_CreateWindow("Paciencia Win3.1 - R36S",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Inicializa e distribui o jogo de paciência
    InicializarBaralho();
    Embaralhar();
    DistribuirJogo();

    MouseSim virtualMouse = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, false };
    bool running = true;
    SDL_Event event;

    while (running) {
        HandleInput(&event, &running, &virtualMouse);

        // Fundo Verde Clássico do Windows 3.1 Solitaire (#008080)
        SDL_SetRenderDrawColor(renderer, 0, 128, 128, 255);
        SDL_RenderClear(renderer);

        // 1. Desenhar a pilha de Estoque (Compra) se houver cartas
        if (estoque.topo > 0) {
            DesenharCarta(renderer, 20, 20, estoque.cartas[estoque.topo - 1]);
        } else {
            // Desenha um retângulo vazio indicador
            SDL_Rect vazio = { 20, 20, CARD_WIDTH, CARD_HEIGHT };
            SDL_SetRenderDrawColor(renderer, 0, 100, 100, 255);
            SDL_RenderDrawRect(renderer, &vazio);
        }

        // 2. Desenhar as 7 colunas do tabuleiro em cascata
        for (int i = 0; i < 7; i++) {
            int posX = 20 + i * (CARD_WIDTH + 24); // Alinhamento horizontal espaçado
            int posY = 130;
            
            if (colunas[i].topo == 0) {
                SDL_Rect vazio = { posX, posY, CARD_WIDTH, CARD_HEIGHT };
                SDL_SetRenderDrawColor(renderer, 0, 100, 100, 255);
                SDL_RenderDrawRect(renderer, &vazio);
            } else {
                for (int j = 0; j < colunas[i].topo; j++) {
                    DesenharCarta(renderer, posX, posY + (j * 16), colunas[i].cartas[j]); // Efeito cascata
                }
            }
        }

        // 3. Desenhar Cursor do Mouse Virtual
        SDL_Rect cursorRect = { (int)virtualMouse.x, (int)virtualMouse.y, 8, 8 };
        if (virtualMouse.left_click) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Vermelho ao segurar/clicar
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Amarelo repouso
        }
        SDL_RenderFillRect(renderer, &cursorRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
