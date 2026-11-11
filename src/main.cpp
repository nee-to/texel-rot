// texel-rot — corrupção visual em tempo real sobre um core libretro.
//
// Uso: ./build/app <conteúdo> [core.so] [--escala N] [--pgxp] [--sem-dither]
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

const char* USO =
    "uso: ./build/app <conteúdo> [core.so] [--escala N] [--pgxp] [--sem-dither]\n"
    "  --escala N     resolução interna do Beetle PSX: 1, 2, 4, 8 ou 16\n"
    "  --pgxp         geometria mais precisa (menos tremido nos polígonos)\n"
    "  --sem-dither   desliga o pontilhado e usa cores de 32 bits\n";

int main(int argc, char** argv) {
  std::string conteudo;
  std::string core = CORE_PADRAO;
  int posicionais = 0;

  // Opções com "--" podem vir em qualquer posição; o resto é conteúdo e core.
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--escala" && i + 1 < argc) {
      std::string n = argv[++i];
      if (n != "1" && n != "2" && n != "4" && n != "8" && n != "16") {
        std::cerr << "escala inválida: " << n << '\n' << USO;
        return 1;
      }
      // "1x(native)" é o nome que o core usa para a escala original.
      emuladorDefinirOpcao("beetle_psx_internal_resolution",
                           n == "1" ? "1x(native)" : n + "x");
    } else if (arg == "--pgxp") {
      emuladorDefinirOpcao("beetle_psx_pgxp_mode", "memory only");
    } else if (arg == "--sem-dither") {
      emuladorDefinirOpcao("beetle_psx_dither_mode", "disabled");
      emuladorDefinirOpcao("beetle_psx_depth", "32bpp");
    } else if (arg.rfind("--", 0) == 0) {
      std::cerr << "opção desconhecida: " << arg << '\n' << USO;
      return 1;
    } else if (posicionais == 0) {
      conteudo = arg;
      posicionais++;
    } else if (posicionais == 1) {
      core = arg;
      posicionais++;
    }
  }
  if (conteudo.empty()) {
    std::cout << USO;
    return 1;
  }
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    std::cerr << SDL_GetError() << '\n';
    return 1;
  }
  int resultado = rodar(conteudo, core);
  SDL_Quit();
  return resultado;
}
