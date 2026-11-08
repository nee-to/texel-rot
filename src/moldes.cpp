// moldes.cpp — os moldes de corrupção da VRAM.
//
// Cada valor de 16 bits pode ser uma cor direta (5 bits de R, G e B) ou
// índices de paleta de 4/8 bits. Os moldes não sabem qual é qual: operam
// nos dados como eles são, e o resultado visual depende do que a região
// guarda. É isso que faz o mesmo molde dar resultados diferentes em
// páginas diferentes.

#include "moldes.hpp"

#include <vector>

namespace {

// Percorre a região em faixas horizontais de altura sorteada (1 a 8
// linhas). Para cada faixa chama fn(inicio, fim, h), onde h é um número
// do ruído próprio daquela faixa.
template <typename F>
void porFaixas(RegiaoVram r, uint32_t semente, F fn) {
  int s = static_cast<int>(semente);
  int inicio = 0;
  while (inicio < r.a) {
    uint32_t h = ruido(inicio, s * 13 + 5);
    int altura = 1 + static_cast<int>(h % 8);
    int fim = inicio + altura < r.a ? inicio + altura : r.a;
    fn(inicio, fim, h);
    inicio = fim;
  }
}

// Desloca a linha y da região em "desloc" posições, dando a volta.
// "linha" é um rascunho: copiar antes evita ler valores já movidos.
void deslocarLinha(Vram& v, RegiaoVram r, int y, int desloc,
                   std::vector<uint16_t>& linha) {
  linha.resize(static_cast<size_t>(r.l));
  for (int x = 0; x < r.l; x++) {
    linha[x] = valorEm(v, r, x, y);
  }
  for (int x = 0; x < r.l; x++) {
    int origem = ((x - desloc) % r.l + r.l) % r.l;  // sempre 0..l-1
    valorEm(v, r, x, y) = linha[origem];
  }
}

// 1. POEIRA — bits isolados invertidos, espalhados.
//    Pontos e riscos finos; a textura continua reconhecível.
void poeira(Vram& v, RegiaoVram r, uint32_t semente) {
  int quantidade = r.l * r.a / 64;  // ~1 a cada 64 valores
  for (int k = 0; k < quantidade; k++) {
    uint32_t h = ruido(k, static_cast<int>(semente) + 77);
    int x = static_cast<int>(h % static_cast<uint32_t>(r.l));
    int y = static_cast<int>((h >> 12) % static_cast<uint32_t>(r.a));
    // Inverte 1 dos 16 bits (XOR). Ex.: bit 3 -> 0b0000000000001000
    valorEm(v, r, x, y) ^= static_cast<uint16_t>(1u << ((h >> 24) % 16));
  }
}

// 2. DERIVA — metade das faixas escorrega para os lados.
//    O deslocamento "cola" nas superfícies 3D que usam a textura.
void deriva(Vram& v, RegiaoVram r, uint32_t semente) {
  std::vector<uint16_t> linha;
  int amp = r.l / 4;
  porFaixas(r, semente, [&](int ini, int fim, uint32_t h) {
    if ((h >> 8) % 2 != 0) {
      return;  // esta faixa fica parada
    }
    int desloc = static_cast<int>((h >> 10) % (2 * amp + 1)) - amp;
    for (int y = ini; y < fim; y++) {
      deslocarLinha(v, r, y, desloc, linha);
    }
  });
}

// 3. ECO — blocos de 16 x 16 copiados de outro ponto da região.
//    Pedaços da textura se repetem fora do lugar.
void eco(Vram& v, RegiaoVram r, uint32_t semente) {
  const int bl = r.l < 16 ? r.l : 16;
  const int ba = r.a < 16 ? r.a : 16;
  const int colunas = r.l / bl, linhas = r.a / ba;
  int quantidade = colunas * linhas / 6 + 1;  // ~1/6 dos blocos
  std::vector<uint16_t> bloco(static_cast<size_t>(bl * ba));

  for (int k = 0; k < quantidade; k++) {
    uint32_t h = ruido(k, static_cast<int>(semente) * 7 + 3);
    uint32_t g = ruido(k + 1000, static_cast<int>(semente) * 7 + 3);
    int ox = static_cast<int>(h % colunas) * bl;
    int oy = static_cast<int>((h >> 12) % linhas) * ba;
    int dx = static_cast<int>(g % colunas) * bl;
    int dy = static_cast<int>((g >> 12) % linhas) * ba;
    // Copia para o rascunho e depois escreve: origem e destino podem
    // ser o mesmo bloco.
    for (int j = 0; j < ba; j++)
      for (int i = 0; i < bl; i++) bloco[j * bl + i] = valorEm(v, r, ox + i, oy + j);
    for (int j = 0; j < ba; j++)
      for (int i = 0; i < bl; i++) valorEm(v, r, dx + i, dy + j) = bloco[j * bl + i];
  }
}

// 4. CISALHAMENTO — num trecho da região, cada linha desloca um pouco
//    mais que a anterior. Gera diagonais, como ler a memória com a
//    largura de linha (pitch) errada.
void cisalhamento(Vram& v, RegiaoVram r, uint32_t semente) {
  if (r.a < 3) {
    return;  // região baixa demais para um trecho de 1/3
  }
  std::vector<uint16_t> linha;
  uint32_t h = ruido(static_cast<int>(semente), 404);
  int inclinacao = 1 + static_cast<int>(h % 4);         // 1..4 a cada 4 linhas
  int altura = r.a / 3 + static_cast<int>((h >> 8) % (r.a - r.a / 3));
  int inicio = static_cast<int>((h >> 16) % (r.a - altura + 1));
  for (int y = 0; y < altura; y++) {
    deslocarLinha(v, r, inicio + y, y * inclinacao / 4, linha);
  }
}

// 5. FEBRE — em algumas faixas, os campos de cor giram: R vai para G,
//    G para B, B para R. A forma fica; as cores (ou os índices da
//    paleta) trocam de lugar.
void febre(Vram& v, RegiaoVram r, uint32_t semente) {
  porFaixas(r, semente, [&](int ini, int fim, uint32_t h) {
    if ((h >> 8) % 3 != 0) {
      return;  // só ~1/3 das faixas
    }
    for (int y = ini; y < fim; y++) {
      for (int x = 0; x < r.l; x++) {
        uint16_t p = valorEm(v, r, x, y);
        uint16_t rr = p & 0x1F, gg = (p >> 5) & 0x1F, bb = (p >> 10) & 0x1F;
        valorEm(v, r, x, y) =
            static_cast<uint16_t>((p & 0x8000) | (rr << 5) | (gg << 10) | bb);
      }
    }
  });
}

// 6. EROSÃO — zera os bits baixos de cada campo de 5 bits.
//    Cores diretas viram poucos tons; texturas com paleta perdem
//    variação. Exemplo com 2 bits: 0b10111 -> 0b10100.
void erosao(Vram& v, RegiaoVram r, uint32_t semente) {
  int bits = 1 + static_cast<int>(ruido(static_cast<int>(semente), 909) % 3);
  uint16_t campo = static_cast<uint16_t>((0x1F >> bits) << bits);
  uint16_t mascara =
      static_cast<uint16_t>(0x8000 | campo | (campo << 5) | (campo << 10));
  for (int y = 0; y < r.a; y++) {
    for (int x = 0; x < r.l; x++) {
      valorEm(v, r, x, y) &= mascara;
    }
  }
}

// 7. ESPELHO — algumas faixas são invertidas da esquerda para a direita.
void espelho(Vram& v, RegiaoVram r, uint32_t semente) {
  porFaixas(r, semente, [&](int ini, int fim, uint32_t h) {
    if ((h >> 8) % 3 != 0) {
      return;
    }
    for (int y = ini; y < fim; y++) {
      for (int x = 0; x < r.l / 2; x++) {
        uint16_t& a = valorEm(v, r, x, y);
        uint16_t& b = valorEm(v, r, r.l - 1 - x, y);
        uint16_t t = a;
        a = b;
        b = t;
      }
    }
  });
}

}  // namespace

const Molde MOLDES[NUM_MOLDES] = {
    {"poeira", "bits isolados invertidos", poeira},
    {"deriva", "faixas escorregam para os lados", deriva},
    {"eco", "blocos repetidos fora do lugar", eco},
    {"cisalhamento", "diagonais de pitch errado", cisalhamento},
    {"febre", "campos de cor trocam de lugar", febre},
    {"erosão", "bits baixos zerados, menos tons", erosao},
    {"espelho", "faixas invertidas", espelho},
};
