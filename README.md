# texel-rot

Frontend libretro em C++/SDL3 que roda um core de emulador e corrompe o
**visual** em tempo real: texturas na VRAM e a imagem final. Não altera o
código do conteúdo emulado.

## Exemplos

| | | |
|---|---|---|
| ![](docs/exemplos/01-paredes.png) | ![](docs/exemplos/02-texturas.png) | ![](docs/exemplos/03-faixas.png) |
| texturas das paredes corrompidas | texturas apagadas em padrão de pontos | faixas deslocadas na imagem final |
| ![](docs/exemplos/04-cor.png) | ![](docs/exemplos/05-faixas-cor.png) | ![](docs/exemplos/06-mosaico.png) |
| paleta alterada, forma intacta | faixas e cor juntas | mosaico de blocos e cores |
| ![](docs/exemplos/07-cores.png) | ![](docs/exemplos/08-vram.png) | |
| cores trocadas nas superfícies | visualização da VRAM (F1) | |

## Compilar e rodar

Requer SDL3, CMake, Ninja e um core libretro (padrão: Beetle PSX,
pacote `libretro-beetle-psx`). A BIOS vai em `sistema/` (ex.: `scph5501.bin`).

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/app caminho/do/conteudo.cue [caminho/do/core.so]
```

### Opções de qualidade (Beetle PSX)

```bash
./build/app jogo.cue --escala 4 --pgxp --sem-dither
```

| Opção | Efeito |
|---|---|
| `--escala N` | resolução interna 1, 2, 4, 8 ou 16: o 3D é desenhado com N× mais pixels em cada eixo |
| `--pgxp` | geometria mais precisa, menos tremido nos polígonos |
| `--sem-dither` | sem pontilhado, cores de 32 bits |

A VRAM lida e corrompida continua sendo a original de 1024×512; o core
redesenha a cena em alta resolução a partir dela, então os moldes seguem
funcionando. A renderização é por CPU e os efeitos de imagem (0–9) custam
proporcionalmente aos pixels. Medido com L'Invasion Vient de l'Espace (PAL,
50 quadros/s necessários), build otimizado:

| Escala | Quadro | Sem efeitos de imagem | Intensidade 3 | Intensidade 9 |
|---|---|---|---|---|
| 1 | 350×288 | 196/s | — | 155/s |
| 2 | 700×576 | — | — | 75/s |
| 4 | 1400×1152 | 64/s | 43/s | 26/s |
| 8 | 2800×2304 | — | — | 7,5/s |

Para tocar ao vivo com efeitos de imagem, use `--escala 2`. `--escala 4`
roda em tempo real com corrupção de VRAM, mas desacelera com os efeitos 1–9.

As corrupções de VRAM dependem de cores cujo estado salvo guarda a VRAM
(`GPURAM`, Beetle PSX). Com outros cores, só os efeitos de imagem funcionam.

## Resolução e capturas

O quadro mantém a largura e a altura entregues pelo core, inclusive quando
mudam durante o jogo. F12 salva um PNG nessa resolução, sem redução para
320×240 nem ampliação artificial. Em F1, a visualização da VRAM é 1024×512.
A janela ajusta apenas a apresentação; os pixels exportados não são reduzidos.
Capturas antigas permanecem intactas. Testes: `ctest --test-dir build --output-on-failure`.

## Teclas

| | |
|---|---|
| setas, Z X A S, Enter, Backspace, Q W E R | RetroPad |
| **C** | aplica o molde atual na região |
| **K / L** | molde anterior / próximo |
| **F1** | mostra a VRAM e a região selecionada |
| **, .** | página de textura anterior / próxima |
| **B** | região: página ↔ VRAM inteira |
| **U** | desfaz a última corrupção |
| **0–9** | intensidade de faixas/blocos na imagem final |
| **Tab / M** | roteiro automático de 16 s / mapa radial ou uniforme |
| **F5–F7 / F9–F11** | salva / carrega estado |
| **P / F12 / Esc** | pausa / captura PNG em `capturas/` / sai |

## Moldes

poeira · deriva · eco · cisalhamento · febre · erosão · espelho

## Créditos

`third_party/libretro/libretro.h` — MIT, The RetroArch team.
