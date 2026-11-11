// imagem.cpp — implementação das operações declaradas em imagem.hpp.

#include "imagem.hpp"

#include <cstddef>

Imagem criarImagem(int largura, int altura) {
  if (largura <= 0 || altura <= 0) return {};
  return {largura, altura,
          std::vector<uint8_t>(static_cast<size_t>(largura) * altura * BYTES_POR_PIXEL, 0)};
}

Cor ler(const Imagem& buf, int x, int y) {
  if (buf.pixels.empty()) return {0, 0, 0};
  // "Prender" (clamp) a coordenada: -3 vira 0, 325 vira 319.
  if (x < 0) x = 0;
  if (x >= buf.largura) x = buf.largura - 1;
  if (y < 0) y = 0;
  if (y >= buf.altura) y = buf.altura - 1;
  int i = BYTES_POR_PIXEL * (y * buf.largura + x);
  return {buf.pixels[i + 0], buf.pixels[i + 1], buf.pixels[i + 2]};
}

// Escreve uma cor no buffer. O "&" em "Imagem& buf" é uma
// REFERÊNCIA: a função recebe o próprio buffer, não uma cópia dele.
// Sem o "&", cada chamada copiaria 230.400 bytes e a escrita se perderia.
void pintar(Imagem& buf, int x, int y, Cor c) {
  // Proteção de limites: ignoramos pixels fora da imagem.
  // Escrever fora do vector seria comportamento indefinido em C++.
  if (x < 0 || x >= buf.largura || y < 0 || y >= buf.altura) {
    return;
  }
  // Mesmo índice que você usava no Canvas: 4 * (y * largura + x)
  int i = BYTES_POR_PIXEL * (y * buf.largura + x);
  buf.pixels[i + 0] = c.r;
  buf.pixels[i + 1] = c.g;
  buf.pixels[i + 2] = c.b;
  buf.pixels[i + 3] = 255; // alfa: totalmente opaco
}

// Retângulo preenchido. (x, y) = canto superior esquerdo.
void retangulo(Imagem& buf, int x, int y, int l, int a, Cor c) {
  for (int py = y; py < y + a; py++) {
    for (int px = x; px < x + l; px++) {
      pintar(buf, px, py, c);
    }
  }
}

// Mistura duas cores. t = 0 devolve a, t = 1 devolve b.
// Exemplo: mistura({0,0,0}, {200,100,50}, 0.5) = {100, 50, 25}.
// Fazemos a conta em float e convertemos de volta para uint8_t
// com static_cast, que é a forma explícita de conversão em C++.
Cor mistura(Cor a, Cor b, float t) {
  return {
      static_cast<uint8_t>(a.r + (b.r - a.r) * t),
      static_cast<uint8_t>(a.g + (b.g - a.g) * t),
      static_cast<uint8_t>(a.b + (b.b - a.b) * t),
  };
}

// "Ruído" determinístico: transforma (x, y) num número que parece
// aleatório, mas é sempre o mesmo para a mesma coordenada.
// Assim a imagem é idêntica em toda execução. Usamos uint32_t
// (sem sinal) porque nele o estouro da multiplicação é bem definido.
uint32_t ruido(int x, int y) {
  uint32_t h = static_cast<uint32_t>(x) * 374761393u +
               static_cast<uint32_t>(y) * 668265263u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}
