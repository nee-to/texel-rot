// tela.hpp — janela, apresentação e capturas (SDL).
//
// Os recursos do SDL ficam em std::unique_ptr com um "destruidor" próprio.
// Isso é RAII: quando a Tela deixa de existir, janela, renderer e textura
// são destruídos automaticamente, na ordem certa, sem chamadas manuais.

#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <string>

#include "imagem.hpp"

// Diz ao unique_ptr como destruir cada tipo do SDL.
struct DestruidorSdl {
  void operator()(SDL_Window* p) const { SDL_DestroyWindow(p); }
  void operator()(SDL_Renderer* p) const { SDL_DestroyRenderer(p); }
  void operator()(SDL_Texture* p) const { SDL_DestroyTexture(p); }
};

struct Tela {
  // Membros são destruídos na ordem INVERSA da declaração:
  // textura, depois renderer, depois janela. Exatamente o necessário.
  std::unique_ptr<SDL_Window, DestruidorSdl> janela;
  std::unique_ptr<SDL_Renderer, DestruidorSdl> renderer;
  std::unique_ptr<SDL_Texture, DestruidorSdl> textura;
};

// Cria janela (LARGURA x ALTURA vezes "escala"), renderer com VSync e
// textura ampliada sem suavização. Devolve false se algo falhar.
bool criarTela(Tela& tela, const char* titulo, int escala);

// Envia a imagem para a textura e mostra na janela.
void apresentar(Tela& tela, const Imagem& imagem);

// Salva a imagem ampliada 4x em PNG na pasta (criada se não existir).
// Nome: AAAA-MM-DD_HH-MM-SS_mmm.png
void salvarCaptura(const Imagem& imagem, const std::string& pasta);
