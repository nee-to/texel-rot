// sessao.hpp — o estado da obra enquanto o programa roda, e as teclas.

#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>

#include "imagem.hpp"
#include "vram.hpp"

struct Sessao {
  // Imagem
  Imagem origem = criarImagem();   // quadro do emulador, limpo
  Imagem destino = criarImagem();  // o que vai para a tela
  Mapa mapa;
  bool mapaUniforme = false;
  float intensidade = 0.0f;  // faixas e blocos na imagem final
  bool automatico = false;   // intensidade segue o roteiro de 16 s

  // VRAM
  Vram vram;
  bool verVram = false;
  bool vramInteira = false;
  int pagina = 20;
  int molde = 0;
  int quadrosDesdeLeitura = 0;

  // Emulação
  bool pausado = false;
  double passo = 1.0 / 60.0;  // duração de um quadro do core, em segundos
  double acumulado = 0.0;

  uint32_t semente = 0;  // muda a cada corrupção: cada aperto é diferente
  std::string pastaCapturas;
  bool rodando = true;
};

// Prepara a sessão depois que o conteúdo foi carregado.
void iniciarSessao(Sessao& s, const std::string& pastaCapturas);

// Reage a uma tecla apertada.
void tratarTecla(Sessao& s, const SDL_KeyboardEvent& tecla);

// Emula os quadros que o tempo real pede (dt = segundos desde a última volta).
void avancarEmulacao(Sessao& s, double dt);

// Monta s.destino para o instante t (segundos).
void montarQuadro(Sessao& s, double t);
