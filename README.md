# Paciencia para R36S (PortMaster)

Klondike em C + SDL2 no estilo do Paciência do Windows 3.1, com arte própria desenhada por código
(nenhum arquivo da Microsoft). Mouse emulado pelo gptokeyb2.

## Como gerar o port (você não precisa compilar nada no R36S)

1. Crie um repositório no GitHub e envie esta pasta inteira (`git init`, `git add .`, `git commit`, `git push`).
2. A aba **Actions** roda sozinha o workflow `build-aarch64`: compila em arm64 (glibc 2.31),
   roda os testes e gera o artefato **paciencia-port** com o `paciencia.zip`.
3. Baixe o zip, copie para a pasta `autoinstall` do PortMaster no cartão SD e abra o PortMaster
   (ou `harbourmaster install paciencia.zip`).

Alternativa com Docker Desktop no PC: `./build_docker.sh` gera `dist/paciencia.zip`.

## Controles

| Botão | Ação |
|--|--|
| Analógico esquerdo / D-Pad | Move o cursor |
| A | Clique (segure para arrastar; toque rápido manda a carta para a fundação) |
| B | Jogada inteligente (fundação, senão outra coluna) |
| X | Cursor lento (segurar) |
| Y | Desfazer |
| L1 | Alterna 1 carta / 3 cartas |
| R1 | Joga para as fundações tudo que for possível |
| Start | Novo jogo (apertar duas vezes) |
| Select | Sair (apertar duas vezes) |

## Pastas

- `src/` código (`game.c` regras sem SDL, `render.c` cartas/naipes/fonte, `ui.c` mesa, `main.c` SDL2)
- `port/paciencia/` estrutura PortMaster (port.json, README, gameinfo, script .sh, .ini)
- `make test` roda 2000 partidas aleatórias checando as regras (não precisa de SDL)
- `paciencia.aarch64 --windowed` abre em janela se compilar no PC, para testar com mouse de verdade
