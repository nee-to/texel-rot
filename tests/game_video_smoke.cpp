// Smoke opcional: usa somente core, BIOS e conteúdo já instalados pelo usuário.
// Uso: game_video_smoke jogo.cue pasta-bios pasta-saida [quadros=3600]
#include "../src/emulador.cpp"
#include "../src/sessao.hpp"
#include "../src/tela.hpp"

#include <filesystem>
#include <stdexcept>

void conferir(bool ok, const char* mensagem) {
  if (!ok) throw std::runtime_error(mensagem);
}

void capturar(const Imagem& imagem, const std::filesystem::path& pasta) {
  salvarCaptura(imagem, pasta.string());
  auto arquivo = *std::filesystem::directory_iterator(pasta);
  SDL_Surface* png = SDL_LoadPNG(arquivo.path().c_str());
  conferir(png && png->w == imagem.largura && png->h == imagem.altura,
           "PNG não tem dimensões do core");
  SDL_Surface* rgba = SDL_ConvertSurface(png, SDL_PIXELFORMAT_RGBA32);
  conferir(rgba, SDL_GetError());
  for (int y = 0; y < imagem.altura; ++y) {
    conferir(std::memcmp(static_cast<uint8_t*>(rgba->pixels) + y * rgba->pitch,
                          imagem.pixels.data() + y * imagem.largura * 4,
                          imagem.largura * 4) == 0, "PNG diferente do frame recebido");
  }
  SDL_DestroySurface(rgba);
  SDL_DestroySurface(png);
  std::cout << "PNG verificado byte a byte: " << arquivo.path() << '\n';
}

int main(int argc, char** argv) {
  if (argc < 4) return 2;
  try {
    conferir(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO), SDL_GetError());
    conferir(emuladorCarregarCore("/usr/lib/libretro/mednafen_psx_libretro.so", argv[2]), "core");
    conferir(emuladorCarregarJogo(argv[1]), "conteúdo");
    {
      Tela tela;
      conferir(criarTela(tela, "smoke isolado", 3), SDL_GetError());
      Sessao s;
      iniciarSessao(s, argv[3]);
      int anteriorL = 0, anteriorA = 0;
      int quadros = argc > 4 ? std::stoi(argv[4]) : 3600;
      for (int i = 1; i <= quadros; ++i) {
        SDL_PumpEvents();
        emuladorRodarQuadro();
        montarQuadro(s, i / emuladorFps());
        conferir(s.destino == quadroNativo, "sessão perdeu pixels recebidos");
        if (s.destino.largura != anteriorL || s.destino.altura != anteriorA) {
          anteriorL = s.destino.largura; anteriorA = s.destino.altura;
          std::cout << "frame " << i << ": " << anteriorL << 'x' << anteriorA << '\n';
        }
        if (i % 600 == 0) {
          apresentar(tela, s.destino);
          capturar(s.destino, std::filesystem::path(argv[3]) / std::to_string(i));
        }
      }
      conferir(emuladorLerVram(s.vram), "VRAM indisponível");
      auto original = s.vram;
      conferir(emuladorSalvarEstado(0), "salvar estado");
      s.verVram = true;
      s.quadrosDesdeLeitura = 0;
      montarQuadro(s, 0);
      conferir(s.destino.largura == 1024 && s.destino.altura == 512, "VRAM incompleta");
      apresentar(tela, s.destino);
      capturar(s.destino, std::filesystem::path(argv[3]) / "vram");
      conferir(emuladorEscreverVram(original), "escrever VRAM");
      conferir(emuladorCarregarEstado(0), "restaurar estado");
      s.verVram = false;
      emuladorRodarQuadro();
      montarQuadro(s, 0);
      conferir(s.destino == quadroNativo, "retorno da VRAM perdeu dimensões");
    }
    emuladorEncerrar();
    SDL_Quit();
    std::cout << "PASS: core real, sessão sem efeitos, PNGs, VRAM e estados\n";
    return 0;
  } catch (const std::exception& erro) {
    std::cerr << "FAIL: " << erro.what() << '\n';
    emuladorEncerrar();
    SDL_Quit();
    return 1;
  }
}
