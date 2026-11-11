// imagem.hpp — o buffer de pixels e as operações básicas sobre ele.
//
// Um arquivo .hpp (header) só DECLARA o que existe: nomes, tipos e
// assinaturas. É parecido com um arquivo .d.ts em TypeScript.
// As implementações ficam em imagem.cpp.

#pragma once  // evita que este header seja incluído duas vezes no mesmo .cpp

#include <cstdint>  // uint8_t, uint32_t
#include <vector>

// Tamanho inicial, antes de receber o primeiro quadro; não limita o core.
inline constexpr int LARGURA = 320;
inline constexpr int ALTURA = 240;

// Cada pixel ocupa 4 bytes: R, G, B, A (nessa ordem na memória).
inline constexpr int BYTES_POR_PIXEL = 4;

// Uma cor é só um agrupamento de três bytes. "struct" agrupa dados,
// parecido com um objeto literal { r, g, b } em JS.
struct Cor {
  uint8_t r, g, b;
};

// Dimensões viajam junto dos pixels, como width/height/data de ImageData.
struct Imagem {
  int largura = 0;
  int altura = 0;
  std::vector<uint8_t> pixels;
  bool operator==(const Imagem&) const = default;
};

// Um valor float por pixel (0 a 1), mesma ordem de linhas que a Imagem,
// mas sem o "* 4": índice = y * LARGURA + x.
// Usado como mapa de instabilidade (onde a imagem pode se corromper).
using Mapa = std::vector<float>;

// Cria um buffer nas dimensões pedidas, com todos os bytes em 0.
Imagem criarImagem(int largura = LARGURA, int altura = ALTURA);

// Escreve uma cor no pixel (x, y). Fora dos limites: não faz nada.
void pintar(Imagem& buf, int x, int y, Cor c);

// Lê a cor do pixel (x, y). Fora dos limites, a coordenada é "presa"
// na borda mais próxima, então a leitura nunca sai do vector.
Cor ler(const Imagem& buf, int x, int y);

// Retângulo preenchido. (x, y) = canto superior esquerdo.
void retangulo(Imagem& buf, int x, int y, int l, int a, Cor c);

// Mistura duas cores. t = 0 devolve a, t = 1 devolve b.
Cor mistura(Cor a, Cor b, float t);

// Número pseudoaleatório determinístico para a coordenada (x, y).
uint32_t ruido(int x, int y);
