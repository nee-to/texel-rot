// roteiro.cpp — a curva de intensidade ao longo do ciclo.

#include "roteiro.hpp"

#include <cmath>  // std::fmod

#include "imagem.hpp"  // ruido

namespace {

// Curva "suave": começa e termina devagar.
// suave(0) = 0, suave(0.5) = 0.5, suave(1) = 1.
// Exemplo: suave(0.25) = 0.25 * 0.25 * (3 - 0.5) = 0.156
double suave(double p) { return p * p * (3.0 - 2.0 * p); }

}  // namespace

float intensidade(double t) {
  // Posição dentro do ciclo. fmod é o "%" para números com vírgula.
  // Exemplo: t = 37.5 -> 37.5 - 2 * 16 = 5.5 s (estamos na tensão).
  double c = std::fmod(t, DURACAO_CICLO);

  if (c < 4.0) {
    return 0.0f; // calma
  }

  if (c < 8.0) {
    // Tensão: cresce devagar até 0.3...
    double p = (c - 4.0) / 4.0;
    double valor = 0.3 * suave(p);
    // ...com tremores: a cada 1/4 s, 20% de chance de um pico rápido.
    int quarto = static_cast<int>(t * 4.0);
    if (ruido(quarto, 4242) % 100 < 20) {
      valor += 0.35;
    }
    return static_cast<float>(valor);
  }

  if (c < 10.0) {
    return 1.0f; // ruptura
  }

  if (c < 14.0) {
    // Retorno: a estrutura se reconstrói aos poucos.
    double p = (c - 10.0) / 4.0;
    return static_cast<float>(1.0 - suave(p));
  }

  return 0.0f; // calma final
}
