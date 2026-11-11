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
  if (vram.size() != static_cast<std::size_t>(VRAM_LARGURA * VRAM_ALTURA)) return;
  if (destino.largura != VRAM_LARGURA || destino.altura != VRAM_ALTURA) {
    destino = criarImagem(VRAM_LARGURA, VRAM_ALTURA);
  }
  // Uma palavra de VRAM por pixel; o contorno é apenas uma sobreposição.
  const int sx0 = selecao.x;
  const int sx1 = selecao.x + selecao.l - 1;
  const int sy0 = selecao.y;
  const int sy1 = selecao.y + selecao.a - 1;

  for (int y = 0; y < VRAM_ALTURA; y++) {
    for (int x = 0; x < VRAM_LARGURA; x++) {
      uint16_t p = vram[y * VRAM_LARGURA + x];
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
