# Compilar no proprio aarch64 (ou em container arm64) com libsdl2-dev instalado.
CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra
SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS   := $(shell sdl2-config --libs 2>/dev/null)
SRC     := src/game.c src/render.c src/ui.c src/main.c
BIN     := paciencia.aarch64
PORTDIR := port/paciencia

all: $(BIN)

$(BIN): $(SRC) src/game.h src/render.h src/ui.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $(SRC) $(SDL_LIBS) -lm
	strip $@

# testes da logica (nao precisam de SDL)
test:
	$(CC) -O2 -Wall -Wextra -o /tmp/test_game src/test_game.c src/game.c && /tmp/test_game

# gera dist/paciencia.zip pronto para a pasta autoinstall do PortMaster
package: $(BIN)
	rm -rf dist && mkdir -p dist/stage/paciencia
	cp "$(PORTDIR)/Paciencia.sh" dist/stage/
	cp -r $(PORTDIR)/paciencia/. dist/stage/paciencia/
	cp $(BIN) dist/stage/paciencia/
	chmod +x dist/stage/paciencia/$(BIN) dist/stage/Paciencia.sh
	cd dist/stage && zip -r ../paciencia.zip Paciencia.sh paciencia
	@echo "Pronto: dist/paciencia.zip"

clean:
	rm -rf $(BIN) dist
.PHONY: all test package clean
