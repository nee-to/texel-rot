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
  // A textura só existe depois do primeiro quadro e acompanha seu tamanho.
  return true;
}

void apresentar(Tela& tela, const Imagem& imagem) {
  if (imagem.pixels.empty()) return;
  if (!tela.textura || tela.textura->w != imagem.largura ||
      tela.textura->h != imagem.altura) {
    tela.textura.reset(SDL_CreateTexture(tela.renderer.get(),
        SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
        imagem.largura, imagem.altura));
    if (!tela.textura) {
      std::cerr << SDL_GetError() << '\n';
      return;
    }
    SDL_SetTextureScaleMode(tela.textura.get(), SDL_SCALEMODE_NEAREST);
    // Pixels quadrados, sem esticar os eixos separadamente. A janela pode
    // reduzir a apresentação, mas nunca o buffer nem a captura PNG.
    SDL_SetRenderLogicalPresentation(tela.renderer.get(), imagem.largura,
        imagem.altura, SDL_LOGICAL_PRESENTATION_LETTERBOX);
  }
  // O pitch da nossa cópia RGBA é compacto; o do core foi lido no callback.
  if (!SDL_UpdateTexture(tela.textura.get(), nullptr, imagem.pixels.data(),
                         imagem.largura * BYTES_POR_PIXEL)) {
    std::cerr << SDL_GetError() << '\n';
    return;
  }
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
      imagem.largura, imagem.altura, SDL_PIXELFORMAT_RGBA32,
      const_cast<uint8_t*>(imagem.pixels.data()), imagem.largura * BYTES_POR_PIXEL);
  if (original && SDL_SavePNG(original, caminho.c_str())) {
    std::cout << "captura: " << caminho << '\n';
  } else {
    std::cerr << SDL_GetError() << '\n';
  }
  SDL_DestroySurface(original);
}
