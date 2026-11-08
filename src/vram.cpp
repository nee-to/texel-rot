// vram.cpp — regiões e visualização da VRAM.

#include "vram.hpp"

#include "emulador.hpp"  // VRAM_LARGURA, VRAM_ALTURA

RegiaoVram regiaoDaPagina(int pagina) {
  // % 16 = coluna, / 16 = linha.
  return {(pagina % 16) * 64, (pagina / 16) * 256, 64, 256};
}

RegiaoVram regiaoTotal() { return {0, 0, VRAM_LARGURA, VRAM_ALTURA}; }

uint16_t& valorEm(Vram& vram, RegiaoVram r, int x, int y) {
  return vram[(r.y + y) * VRAM_LARGURA + (r.x + x)];
}

void desenharVram(const Vram& vram, RegiaoVram selecao, Imagem& destino) {
  // Contorno da seleção na escala da tela.
  const int sx0 = selecao.x * LARGURA / VRAM_LARGURA;
  const int sx1 = (selecao.x + selecao.l) * LARGURA / VRAM_LARGURA - 1;
  const int sy0 = selecao.y * ALTURA / VRAM_ALTURA;
  const int sy1 = (selecao.y + selecao.a) * ALTURA / VRAM_ALTURA - 1;

  for (int y = 0; y < ALTURA; y++) {
    int vy = y * VRAM_ALTURA / ALTURA;
    for (int x = 0; x < LARGURA; x++) {
      int vx = x * VRAM_LARGURA / LARGURA;
      uint16_t p = vram[vy * VRAM_LARGURA + vx];
      // Formato do PS1: bits 0-4 = R, 5-9 = G, 10-14 = B.
      Cor c = {static_cast<uint8_t>((p & 0x1F) * 255 / 31),
               static_cast<uint8_t>(((p >> 5) & 0x1F) * 255 / 31),
               static_cast<uint8_t>(((p >> 10) & 0x1F) * 255 / 31)};

      bool dentroX = x >= sx0 && x <= sx1;
      bool dentroY = y >= sy0 && y <= sy1;
      if ((dentroY && (x == sx0 || x == sx1)) ||
          (dentroX && (y == sy0 || y == sy1))) {
        c = {255, 220, 0};
      }
      pintar(destino, x, y, c);
    }
  }
}
