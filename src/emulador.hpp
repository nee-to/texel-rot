// emulador.hpp — frontend libretro mínimo.
//
// Libretro é uma interface padrão entre emuladores ("cores", arquivos .so)
// e programas que os usam ("frontends", como o RetroArch). Aqui o nosso
// programa é o frontend: carrega um core, carrega um jogo e, a cada
// quadro, recebe a imagem. Nada aqui depende de um jogo específico.
//
// A leitura/escrita de VRAM usa o estado salvo do core e só funciona com
// cores que guardam a VRAM numa entrada "GPURAM" (Beetle PSX / Mednafen).
// Com outros cores, essas funções devolvem false e o resto continua.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "imagem.hpp"

// VRAM do PS1: 1 MB visto como uma imagem de 1024 x 512 valores de 16 bits.
inline constexpr int VRAM_LARGURA = 1024;
inline constexpr int VRAM_ALTURA = 512;

// Quantidade de estados salvos. Slot 0 = desfazer automático.
inline constexpr int NUM_SLOTS = 4;

// Carrega o core (.so). "pastaSistema" é onde o core procura a BIOS.
bool emuladorCarregarCore(const std::string& caminhoCore,
                          const std::string& pastaSistema);

// Carrega o conteúdo (.cue, .chd, .pbp, .bin...).
bool emuladorCarregarJogo(const std::string& caminhoJogo);

// Quadros por segundo informados pelo core (ex.: 59,94 NTSC, 50 PAL).
double emuladorFps();

// Emula um quadro.
void emuladorRodarQuadro();

// Copia o último quadro para "destino", reamostrado para LARGURA x ALTURA
// sem suavização.
void emuladorCopiarQuadro(Imagem& destino);

// Estados salvos: guardar e restaurar o console inteiro, em slots 0..3.
bool emuladorSalvarEstado(int slot);
bool emuladorCarregarEstado(int slot);

// Copia a VRAM para "vram" (1024 * 512 valores) e de volta.
// Escrever na VRAM muda só o VISUAL (texturas, telas), não o código.
bool emuladorLerVram(std::vector<uint16_t>& vram);
bool emuladorEscreverVram(const std::vector<uint16_t>& vram);

// Descarrega conteúdo e core.
void emuladorEncerrar();
