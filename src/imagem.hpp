// imagem.hpp — o buffer de pixels e as operações básicas sobre ele.
//
// Um arquivo .hpp (header) só DECLARA o que existe: nomes, tipos e
// assinaturas. É parecido com um arquivo .d.ts em TypeScript.
// As implementações ficam em imagem.cpp.

#pragma once  // evita que este header seja incluído duas vezes no mesmo .cpp

#include <cstdint>  // uint8_t, uint32_t
#include <vector>

// Resolução interna. "constexpr" = valor conhecido em tempo de compilação.
// "inline" permite definir a constante num header incluído por vários .cpp.
// 320 x 240 (4:3) é a resolução mais comum dos jogos de PS1. Quadros de
// outros tamanhos (256, 368, 512, 640 de largura...) são reamostrados.
inline constexpr int LARGURA = 320;
inline constexpr int ALTURA = 240;

// Cada pixel ocupa 4 bytes: R, G, B, A (nessa ordem na memória).
inline constexpr int BYTES_POR_PIXEL = 4;

// Uma cor é só um agrupamento de três bytes. "struct" agrupa dados,
// parecido com um objeto literal { r, g, b } em JS.
struct Cor {
  uint8_t r, g, b;
};

// Apelido de tipo: "Imagem" é o mesmo que std::vector<uint8_t>.
// Só deixa as assinaturas mais legíveis; não cria um tipo novo.
using Imagem = std::vector<uint8_t>;

// Um valor float por pixel (0 a 1), mesma ordem de linhas que a Imagem,
// mas sem o "* 4": índice = y * LARGURA + x.
// Usado como mapa de instabilidade (onde a imagem pode se corromper).
using Mapa = std::vector<float>;

// Cria um buffer LARGURA x ALTURA com todos os bytes em 0.
Imagem criarImagem();

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
