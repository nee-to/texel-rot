// efeitos.cpp — implementação das corrupções na imagem final.

#include "efeitos.hpp"

#include <cmath>  // std::sqrt

namespace {

// Converte um número do ruído em um valor entre 0 e 1 (com 3 casas).
// Exemplo: h = 123456 -> 123456 % 1000 = 456 -> 0.456
float fracao(uint32_t h) { return static_cast<float>(h % 1000) / 1000.0f; }

}  // namespace

Mapa criarMapa(bool uniforme, int largura, int altura) {
  Mapa mapa(largura * altura, 1.0f);
  if (uniforme) {
    return mapa;
  }
  // Distância até o centro, normalizada: 0 no centro, 1 no canto.
  // Até 0.3: estável. De 0.3 a 1: sobe de 0 a 1.
  // Exemplo: d = 0.65 -> (0.65 - 0.3) / 0.7 = 0.5
  const float cx = largura / 2.0f, cy = altura / 2.0f;
  const float maxD = std::sqrt(cx * cx + cy * cy);
  for (int y = 0; y < altura; y++) {
    for (int x = 0; x < largura; x++) {
      float dx = x - cx, dy = y - cy;
      float d = std::sqrt(dx * dx + dy * dy) / maxD;
      float v = (d - 0.3f) / 0.7f;
      mapa[y * largura + x] = v < 0.0f ? 0.0f : v;
    }
  }
  return mapa;
}

void deslocarFaixas(const Imagem& origem, Imagem& destino, const Mapa& mapa,
                    float intensidade, int amplitude, uint32_t momento) {
  const int largura = origem.largura, altura = origem.altura;
  if (intensidade <= 0.0f) {
    return;
  }
  // A amplitude cresce com a intensidade, mas nunca é menor que 1.
  int a = static_cast<int>(amplitude * intensidade);
  if (a < 1) {
    a = 1;
  }
  int m = static_cast<int>(momento);

  int inicio = 0;
  while (inicio < altura) {
    // Cada faixa sorteia altura (2 a 7 linhas), limiar e deslocamento.
    int alturaFaixa = 2 + static_cast<int>(ruido(inicio, m + 7) % 6);
    uint32_t h = ruido(inicio, m);
    // Limiar: quão instável o pixel precisa estar para a faixa pegá-lo.
    float limiar = fracao(h);
    // Deslocamento entre -a e +a. Ex.: a = 6 -> % 13 dá 0..12 -> -6..+6.
    int desloc = static_cast<int>((h >> 10) % (2 * a + 1)) - a;

    if (desloc != 0) {
      for (int y = inicio; y < inicio + alturaFaixa && y < altura; y++) {
        for (int x = 0; x < largura; x++) {
          if (mapa[y * largura + x] * intensidade > limiar) {
            // Volta na largura. O "+ largura" evita resto negativo:
            // em C++, -3 % 320 = -3.
            int xOrigem = ((x - desloc) % largura + largura) % largura;
            pintar(destino, x, y, ler(origem, xOrigem, y));
          }
        }
      }
    }
    inicio += alturaFaixa;
  }
}

void trocarBlocos(const Imagem& origem, Imagem& destino, const Mapa& mapa,
                  float intensidade, uint32_t momento) {
  const int largura = origem.largura, altura = origem.altura;
  if (intensidade <= 0.0f) {
    return;
  }
  const int BL = 8;  // largura do bloco
  const int BA = 4;  // altura do bloco
  int m = static_cast<int>(momento);

  for (int by = 0; by < altura / BA; by++) {
    for (int bx = 0; bx < largura / BL; bx++) {
      // A instabilidade do bloco é a do pixel do centro dele.
      float nivel =
          mapa[(by * BA + BA / 2) * largura + bx * BL + BL / 2] * intensidade;
      uint32_t h = ruido(bx + m * 97, by + m * 31);
      // "+ 0.15": blocos são mais raros que faixas.
      if (nivel <= fracao(h) + 0.15f) {
        continue;
      }
      // Origem: até 6 blocos para os lados e 4 para cima/baixo.
      int origemBx = bx + static_cast<int>((h >> 10) % 13) - 6;
      int origemBy = by + static_cast<int>((h >> 14) % 9) - 4;
      for (int j = 0; j < BA; j++) {
        for (int i = 0; i < BL; i++) {
          // ler() prende a coordenada nas bordas: nunca lê fora.
          pintar(destino, bx * BL + i, by * BA + j,
                 ler(origem, origemBx * BL + i, origemBy * BA + j));
        }
      }
    }
  }
}
