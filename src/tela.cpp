// tela.cpp — implementação da janela e das capturas.

#include "tela.hpp"

#include <cstdio>  // std::snprintf
#include <iostream>

bool criarTela(Tela& tela, const char* titulo, int escala) {
  tela.janela.reset(SDL_CreateWindow(titulo, LARGURA * escala,
                                     ALTURA * escala, SDL_WINDOW_RESIZABLE));
  if (!tela.janela) {
    std::cerr << SDL_GetError() << '\n';
    return false;
  }
  tela.renderer.reset(SDL_CreateRenderer(tela.janela.get(), nullptr));
  if (!tela.renderer) {
    std::cerr << SDL_GetError() << '\n';
    return false;
  }
  SDL_SetRenderVSync(tela.renderer.get(), 1);
  // Pensa em LARGURA x ALTURA e amplia só por fatores inteiros.
  SDL_SetRenderLogicalPresentation(tela.renderer.get(), LARGURA, ALTURA,
                                   SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

  // RGBA32 = bytes R, G, B, A na memória. STREAMING = muda todo quadro.
  tela.textura.reset(SDL_CreateTexture(tela.renderer.get(),
                                       SDL_PIXELFORMAT_RGBA32,
                                       SDL_TEXTUREACCESS_STREAMING, LARGURA,
                                       ALTURA));
  if (!tela.textura) {
    std::cerr << SDL_GetError() << '\n';
    return false;
  }
  SDL_SetTextureScaleMode(tela.textura.get(), SDL_SCALEMODE_NEAREST);
  return true;
}

void apresentar(Tela& tela, const Imagem& imagem) {
  // Pitch = bytes por linha = 320 * 4 = 1280.
  SDL_UpdateTexture(tela.textura.get(), nullptr, imagem.data(),
                    LARGURA * BYTES_POR_PIXEL);
  SDL_SetRenderDrawColor(tela.renderer.get(), 0, 0, 0, 255);
  SDL_RenderClear(tela.renderer.get());
  SDL_RenderTexture(tela.renderer.get(), tela.textura.get(), nullptr, nullptr);
  SDL_RenderPresent(tela.renderer.get());
}

void salvarCaptura(const Imagem& imagem, const std::string& pasta) {
  if (!SDL_CreateDirectory(pasta.c_str())) {
    std::cerr << SDL_GetError() << '\n';
    return;
  }

  // Data e hora locais para o nome do arquivo.
  SDL_Time agora = 0;
  SDL_DateTime dt{};
  SDL_GetCurrentTime(&agora);
  SDL_TimeToDateTime(agora, &dt, true);
  char nome[64];
  std::snprintf(nome, sizeof(nome), "/%04d-%02d-%02d_%02d-%02d-%02d_%03d.png",
                dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second,
                dt.nanosecond / 1000000);
  std::string caminho = pasta + nome;

  // A superfície só "empresta" os pixels: o SDL pede ponteiro não-const,
  // mas só vamos ler.
  SDL_Surface* original = SDL_CreateSurfaceFrom(
      LARGURA, ALTURA, SDL_PIXELFORMAT_RGBA32,
      const_cast<uint8_t*>(imagem.data()), LARGURA * BYTES_POR_PIXEL);
  SDL_Surface* ampliada =
      original ? SDL_ScaleSurface(original, LARGURA * 4, ALTURA * 4,
                                  SDL_SCALEMODE_NEAREST)
               : nullptr;
  if (ampliada && SDL_SavePNG(ampliada, caminho.c_str())) {
    std::cout << "captura: " << caminho << '\n';
  } else {
    std::cerr << SDL_GetError() << '\n';
  }
  SDL_DestroySurface(ampliada);  // aceitam nullptr
  SDL_DestroySurface(original);
}
