// texel-rot — corrupção visual em tempo real sobre um core libretro.
//
// Uso: ./build/app <conteúdo> [core.so]
// Core padrão: Beetle PSX (/usr/lib/libretro/mednafen_psx_libretro.so).
// BIOS: pasta sistema/ do projeto. Capturas: pasta capturas/.
//
// Arquivos:
//   main.cpp            argumentos, inicialização e laço principal
//   sessao.hpp/.cpp     estado da obra, teclas, ritmo e montagem do quadro
//   emulador.hpp/.cpp   frontend libretro (core, quadros, estados, VRAM)
//   vram.hpp/.cpp       regiões e visualização da VRAM
//   moldes.hpp/.cpp     moldes de corrupção da VRAM
//   efeitos.hpp/.cpp    faixas e blocos na imagem final
//   roteiro.hpp/.cpp    intensidade automática em ciclo de 16 s
//   tela.hpp/.cpp       janela, apresentação e capturas (SDL)
//   imagem.hpp/.cpp     buffer RGBA e operações de pixel

#include <SDL3/SDL.h>

#include <iostream>
#include <string>

#include "emulador.hpp"
#include "sessao.hpp"
#include "tela.hpp"

namespace {

const char* CORE_PADRAO = "/usr/lib/libretro/mednafen_psx_libretro.so";

// O laço fica numa função separada para que a Tela (RAII) seja destruída
// ao sair dela, ANTES do SDL_Quit no main.
int rodar(const std::string& conteudo, const std::string& core) {
  // PASTA_PROJETO vem do CMakeLists.txt.
  const std::string projeto = PASTA_PROJETO;

  if (!emuladorCarregarCore(core, projeto + "/sistema") ||
      !emuladorCarregarJogo(conteudo)) {
    emuladorEncerrar();
    return 1;
  }

  Tela tela;
  if (!criarTela(tela, "texel-rot", 3)) {
    emuladorEncerrar();
    return 1;
  }

  Sessao sessao;
  iniciarSessao(sessao, projeto + "/capturas");

  Uint64 anterior = SDL_GetTicksNS();
  while (sessao.rodando) {
    Uint64 agora = SDL_GetTicksNS();
    double dt = static_cast<double>(agora - anterior) / 1e9;
    double t = static_cast<double>(agora) / 1e9;
    anterior = agora;

    SDL_Event evento;
    while (SDL_PollEvent(&evento)) {
      if (evento.type == SDL_EVENT_QUIT) {
        sessao.rodando = false;
      } else if (evento.type == SDL_EVENT_KEY_DOWN) {
        tratarTecla(sessao, evento.key);
      }
    }

    avancarEmulacao(sessao, dt);
    montarQuadro(sessao, t);
    apresentar(tela, sessao.destino);
  }

  emuladorEncerrar();
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "uso: ./build/app <conteúdo> [core.so]\n";
    return 1;
  }
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::cerr << SDL_GetError() << '\n';
    return 1;
  }
  int resultado = rodar(argv[1], argc >= 3 ? argv[2] : CORE_PADRAO);
  SDL_Quit();
  return resultado;
}
