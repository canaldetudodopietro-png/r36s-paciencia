#include <SDL2/SDL.h>
#include <stdbool.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define CURSOR_SPEED 5

typedef struct {
    float x;
    float y;
    bool left_click;
} MouseSim;

void HandleInput(SDL_Event *event, bool *running, MouseSim *mouse) {
    while (SDL_PollEvent(event)) {
        if (event->type == SDL_QUIT) {
            *running = false;
        }
        
        // Mapeamento de botões do R36S (GamePad como Teclado ou Eventos de Joystick)
        // No ArkOS, o D-Pad geralmente mapeia para as setas do teclado por padrão em SDL2 clássico
        if (event->type == SDL_KEYDOWN) {
            switch (event->key.keysym.sym) {
                case SDLK_ESCAPE: // Botão Select/Start comum para sair
                    *running = false; 
                    break;
                case SDLK_z: // Botão A do R36S comumente mapeado
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

    // Movimentação Contínua (Mouse Virtual)
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_UP])    mouse->y -= CURSOR_SPEED;
    if (state[SDL_SCANCODE_DOWN])  mouse->y += CURSOR_SPEED;
    if (state[SDL_SCANCODE_LEFT])  mouse->x -= CURSOR_SPEED;
    if (state[SDL_SCANCODE_RIGHT]) mouse->x += CURSOR_SPEED;

    // Limitar cursor à tela
    if (mouse->x < 0) mouse->x = 0;
    if (mouse->y < 0) mouse->y = 0;
    if (mouse->x > SCREEN_WIDTH) mouse->x = SCREEN_WIDTH;
    if (mouse->y > SCREEN_HEIGHT) mouse->y = SCREEN_HEIGHT;
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("Paciencia Win3.1 - R36S",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    MouseSim virtualMouse = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, false };
    bool running = true;
    SDL_Event event;

    while (running) {
        HandleInput(&event, &running, &virtualMouse);

        // Cor de fundo: Verde clássico do Windows 3.1 Solitaire (#008080)
        SDL_SetRenderDrawColor(renderer, 0, 128, 128, 255);
        SDL_RenderClear(renderer);

        // Exemplo: Desenhar uma carta genérica na tela
        SDL_Rect card = { 50, 50, 70, 100 };
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Corpo da carta
        SDL_RenderFillRect(renderer, &card);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);       // Borda da carta
        SDL_RenderDrawRect(renderer, &card);

        // Desenhar Cursor do Mouse Virtual
        SDL_Rect cursorRect = { (int)virtualMouse.x, (int)virtualMouse.y, 10, 10 };
        if (virtualMouse.left_click) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Vermelho se clicado
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Amarelo normal
        }
        SDL_RenderFillRect(renderer, &cursorRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
