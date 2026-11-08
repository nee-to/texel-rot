// vram.hpp — a memória de vídeo como imagem: regiões e visualização.
//
// A VRAM do PS1 é uma "imagem" de 1024 x 512 valores de 16 bits:
//   índice = y * 1024 + x
// Nela ficam as telas desenhadas e as texturas. É dividida em 32 páginas
// de textura de 64 x 256 valores (16 por linha, 2 linhas), que usamos
// para escolher ONDE corromper.

#pragma once

#include <cstdint>
#include <vector>

#include "imagem.hpp"

using Vram = std::vector<uint16_t>;

// Retângulo na VRAM, em coordenadas de 16 bits (0..1023, 0..511).
struct RegiaoVram {
  int x, y, l, a;
};

inline constexpr int NUM_PAGINAS = 32;

// Região da página p (0 a 31). Exemplo: p = 17 -> x = 64, y = 256.
RegiaoVram regiaoDaPagina(int pagina);

// A VRAM inteira.
RegiaoVram regiaoTotal();

// Acesso a um valor da região: (x, y) relativos ao canto da região.
// Quem chama garante 0 <= x < r.l e 0 <= y < r.a.
uint16_t& valorEm(Vram& vram, RegiaoVram r, int x, int y);

// Desenha a VRAM inteira reduzida na imagem, com contorno amarelo na
// região selecionada. Todo valor é lido como cor direta de 15 bits.
void desenharVram(const Vram& vram, RegiaoVram selecao, Imagem& destino);
