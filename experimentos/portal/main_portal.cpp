// Memória de um lugar impossível — estudo 2: deterioração no tempo
//
// Uma ruína gótica (portal em arco ogival) numa noite escura.
// A impossibilidade: dentro do portal é amanhecer, com um sol baixo,
// enquanto do lado de fora é noite de lua cheia.
//
// Toda a imagem é desenhada pela CPU num buffer de 320 x 180 pixels RGBA.
// Depois o buffer é enviado para uma textura e ampliado sem suavização.
//
// Arquivos:
//   imagem.hpp/.cpp  buffer de pixels e operações básicas
//   paleta.hpp       cores da obra
//   cena.hpp/.cpp    a composição
//   corrupcao.hpp/.cpp  faixas deslocadas e blocos trocados
//   roteiro.hpp/.cpp    a intensidade ao longo de um ciclo de 16 s
//   main.cpp         janela, eventos e apresentação (SDL)
//
// Teclas: S salva o quadro atual em BMP | Esc ou fechar janela sai.

#include <SDL3/SDL.h>

#include <iostream>
#include <string>

#include "cena.hpp"
#include "corrupcao.hpp"
#include "imagem.hpp"
#include "roteiro.hpp"

// Salva o buffer em BMP (formato simples, sem compressão) para registro.
// "const Imagem&": referência que só lê. Evita a cópia e garante que
// a função não altera os pixels.
void salvarBMP(const Imagem& pixels, const char* caminho) {
  // CreateSurfaceFrom pede um ponteiro não-const, mas só vamos ler;
  // const_cast remove o const para satisfazer a assinatura.
  SDL_Surface* sup = SDL_CreateSurfaceFrom(
      LARGURA, ALTURA, SDL_PIXELFORMAT_RGBA32,
      const_cast<uint8_t*>(pixels.data()), LARGURA * BYTES_POR_PIXEL);
  if (sup != nullptr && SDL_SaveBMP(sup, caminho)) {
    std::cout << "salvo: " << caminho << '\n';
  } else {
    std::cout << SDL_GetError() << '\n';
  }
  SDL_DestroySurface(sup);
}

// Monta o quadro do instante t (segundos): copia a origem para o destino
// e aplica as corrupções com a intensidade que o roteiro manda.
// Só depende de t: o mesmo t produz sempre a mesma imagem.
void montarQuadro(const Imagem& origem, Imagem& destino, const Mapa& mapa,
                  double t) {
  // Copiar um vector com "=" copia todos os bytes. A origem não muda.
  destino = origem;

  float forca = intensidade(t);

  // O tempo vira "momentos" inteiros: as faixas trocam 8 vezes por
  // segundo e os blocos 4 vezes. Saltos secos, como dado travando.
  auto momentoFaixas = static_cast<uint32_t>(t * 8.0);
  auto momentoBlocos = static_cast<uint32_t>(t * 4.0);

  deslocarFaixas(origem, destino, mapa, forca, 24, momentoFaixas);
  trocarBlocos(origem, destino, mapa, forca, momentoBlocos);
}

int main() {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::cout << SDL_GetError() << '\n';
    return 1;
  }

  // Janela de 1280 x 720 = exatamente 4x a resolução interna.
  SDL_Window* janela =
      SDL_CreateWindow("Memória", LARGURA * 4, ALTURA * 4, SDL_WINDOW_RESIZABLE);
  if (janela == nullptr) {
    std::cout << SDL_GetError() << '\n';
    return 1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(janela, nullptr);
  if (renderer == nullptr) {
    std::cout << SDL_GetError() << '\n';
    return 1;
  }
  SDL_SetRenderVSync(renderer, 1);

  // O renderer passa a "pensar" em 320 x 180 e amplia só por fatores
  // inteiros (2x, 3x, 4x...). Sobra de espaço vira tarja escura.
  SDL_SetRenderLogicalPresentation(renderer, LARGURA, ALTURA,
                                   SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

  // A textura é a imagem que fica na GPU. Formato RGBA32 = bytes R, G, B, A
  // nessa ordem na memória, igual ao ImageData do Canvas.
  // STREAMING = vamos atualizar o conteúdo dela em todo quadro.
  SDL_Texture* textura =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                        SDL_TEXTUREACCESS_STREAMING, LARGURA, ALTURA);
  if (textura == nullptr) {
    std::cout << SDL_GetError() << '\n';
    return 1;
  }
  // NEAREST = ampliar sem suavizar: cada pixel vira um bloco nítido.
  SDL_SetTextureScaleMode(textura, SDL_SCALEMODE_NEAREST);

  // ORIGEM: o portal intacto, desenhado uma vez e nunca mais alterado.
  Imagem origem = criarImagem();
  desenharComposicao(origem);

  // DESTINO: o que vai para a tela, refeito a cada quadro.
  Imagem destino = criarImagem();

  // MAPA: onde a imagem pode se corromper (também calculado uma vez).
  Mapa mapa = criarMapaInstabilidade();

  bool rodando = true;
  while (rodando) {
    // Tempo desde o SDL_Init, em segundos. SDL_GetTicks dá milissegundos.
    Uint64 ms = SDL_GetTicks();
    double t = static_cast<double>(ms) / 1000.0;

    SDL_Event evento;
    while (SDL_PollEvent(&evento)) {
      if (evento.type == SDL_EVENT_QUIT) {
        rodando = false;
      } else if (evento.type == SDL_EVENT_KEY_DOWN) {
        if (evento.key.key == SDLK_ESCAPE) {
          rodando = false;
        } else if (evento.key.key == SDLK_S) {
          // Nome com o tempo, para não sobrescrever: quadro_9350.bmp
          std::string nome = "quadro_" + std::to_string(ms) + ".bmp";
          salvarBMP(destino, nome.c_str());
        }
      }
    }

    montarQuadro(origem, destino, mapa, t);
    // O último argumento é o PITCH: bytes por linha = 320 * 4 = 1280.
    SDL_UpdateTexture(textura, nullptr, destino.data(),
                      LARGURA * BYTES_POR_PIXEL);

    // A cada quadro: limpar com a cor do vazio, desenhar a textura, mostrar.
    SDL_SetRenderDrawColor(renderer, 8, 6, 14, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, textura, nullptr, nullptr);
    SDL_RenderPresent(renderer);
  }

  // Liberar na ordem inversa da criação.
  SDL_DestroyTexture(textura);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(janela);
  SDL_Quit();
  return 0;
}
