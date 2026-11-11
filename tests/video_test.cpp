// Callback real + pixels sintéticos: não é uma captura de jogo.
// Incluir o .cpp permite chamar os callbacks internos sem expor API de teste.
#include "../src/emulador.cpp"
#include "../src/efeitos.hpp"
#include "../src/sessao.hpp"
#include "../src/tela.hpp"
#include "../src/moldes.hpp"

#include <filesystem>
#include <stdexcept>

void exigir(bool ok, const char* mensagem) {
  if (!ok) throw std::runtime_error(mensagem);
}

Imagem enviarPadrao(int largura, int altura, retro_pixel_format tipo) {
  formato = tipo;
  const int bytes = tipo == RETRO_PIXEL_FORMAT_XRGB8888 ? 4 : 2;
  const size_t pitch = largura * bytes + 13;  // padding e linhas desalinhadas
  std::vector<uint8_t> entrada(pitch * altura + 1, 0xCD);
  Imagem esperado = criarImagem(largura, altura);
  for (int y = 0; y < altura; ++y) {
    for (int x = 0; x < largura; ++x) {
      uint32_t p = ruido(x, y);
      Cor c;
      if (bytes == 4) {
        c = {uint8_t(p >> 16), uint8_t(p >> 8), uint8_t(p)};
      } else if (tipo == RETRO_PIXEL_FORMAT_RGB565) {
        c = {uint8_t(((p >> 11) & 31) * 255 / 31),
             uint8_t(((p >> 5) & 63) * 255 / 63), uint8_t((p & 31) * 255 / 31)};
      } else {
        c = {uint8_t(((p >> 10) & 31) * 255 / 31),
             uint8_t(((p >> 5) & 31) * 255 / 31), uint8_t((p & 31) * 255 / 31)};
      }
      if (bytes == 4) std::memcpy(entrada.data() + 1 + y * pitch + x * bytes, &p, 4);
      else {
        uint16_t curto = uint16_t(p);
        std::memcpy(entrada.data() + 1 + y * pitch + x * bytes, &curto, 2);
      }
      pintar(esperado, x, y, c);
    }
  }
  quadroDeVideo(entrada.data() + 1, largura, altura, pitch);
  Imagem recebido;
  emuladorCopiarQuadro(recebido);
  exigir(recebido == esperado, "callback perdeu dimensões/pixels ou ignorou pitch/formato");
  quadroDeVideo(nullptr, 1, 1, 0);
  emuladorCopiarQuadro(recebido);
  exigir(recebido == esperado, "quadro duplicado alterou imagem");
  return recebido;
}

void verificarPng(const Imagem& imagem) {
  const auto pasta = std::filesystem::temp_directory_path() /
      ("video-test-" + std::to_string(SDL_GetTicksNS()));
  Sessao s;
  s.destino = imagem;
  s.pastaCapturas = pasta.string();
  SDL_KeyboardEvent tecla{};
  tecla.key = SDLK_F12;
  tratarTecla(s, tecla);
  auto caminho = *std::filesystem::directory_iterator(pasta);
  SDL_Surface* png = SDL_LoadPNG(caminho.path().c_str());
  exigir(png && png->w == imagem.largura && png->h == imagem.altura,
         "F12 não preserva tamanho nativo");
  SDL_Surface* rgba = SDL_ConvertSurface(png, SDL_PIXELFORMAT_RGBA32);
  exigir(rgba != nullptr, SDL_GetError());
  for (int y = 0; y < imagem.altura; ++y) {
    exigir(std::memcmp(static_cast<uint8_t*>(rgba->pixels) + y * rgba->pitch,
                       imagem.pixels.data() + y * imagem.largura * 4,
                       imagem.largura * 4) == 0, "PNG alterou pixels");
  }
  SDL_DestroySurface(rgba);
  SDL_DestroySurface(png);
  std::filesystem::remove_all(pasta);
}

const std::pair<int, int> tamanhos[] = {
    {320,240}, {640,480}, {368,240}, {256,240}, {512,240},
    {640,256}, {320,512}, {321,239}, {9,5}, {1,1}, {320,240}};

void testarPixels() {
  Tela tela;
  exigir(criarTela(tela, "teste nativo", 3), SDL_GetError());
  for (auto tipo : {RETRO_PIXEL_FORMAT_XRGB8888, RETRO_PIXEL_FORMAT_RGB565,
                    RETRO_PIXEL_FORMAT_0RGB1555}) {
    for (auto [l, a] : tamanhos) {
      auto imagem = enviarPadrao(l, a, tipo);
      apresentar(tela, imagem);
      exigir(tela.textura && tela.textura->w == l && tela.textura->h == a,
             "textura não acompanha resolução");
      int logL, logA;
      SDL_RendererLogicalPresentation modo;
      exigir(SDL_GetRenderLogicalPresentation(tela.renderer.get(), &logL, &logA, &modo), SDL_GetError());
      exigir(logL == l && logA == a && modo == SDL_LOGICAL_PRESENTATION_LETTERBOX,
             "apresentação distorce proporção do quadro");
      verificarPng(imagem);
      auto copia = imagem;
      pintar(copia, -1, 0, {1,2,3});
      pintar(copia, l, a - 1, {1,2,3});
      pintar(copia, 0, a, {1,2,3});
      exigir(copia == imagem, "escrita fora dos limites");
      for (bool uniforme : {false, true}) {
        copia = imagem;
        auto mapa = criarMapa(uniforme, l, a);
        exigir(mapa.size() == size_t(l) * a, "mapa com tamanho errado");
        deslocarFaixas(imagem, copia, mapa, 0, 24, 0);
        trocarBlocos(imagem, copia, mapa, 0, 0);
        exigir(copia == imagem, "intensidade zero modificou imagem");
        for (unsigned m = 0; m < 8; ++m) {
          copia = imagem;
          deslocarFaixas(imagem, copia, mapa, 1, 24, m);
          trocarBlocos(imagem, copia, mapa, 1, m);
          exigir(copia.largura == l && copia.altura == a && copia.pixels.size() == imagem.pixels.size(),
                 "efeitos mudaram dimensões");
        }
      }
    }
  }
  std::cout << "PASS: formatos/pitch, dimensões variáveis, textura, F12/PNG pixel a pixel e efeitos\n";
}

void testarSessao() {
  Sessao s;
  iniciarSessao(s, ".");
  for (auto [l, a] : tamanhos) {
    auto imagem = enviarPadrao(l, a, RETRO_PIXEL_FORMAT_XRGB8888);
    s.intensidade = 0;
    montarQuadro(s, 0);
    exigir(s.destino == imagem, "sessão perdeu quadro nativo");
    exigir(s.mapa == criarMapa(s.mapaUniforme, l, a), "sessão usa mapa antigo");
    SDL_KeyboardEvent tecla{};
    tecla.key = SDLK_M;
    tratarTecla(s, tecla);
    exigir(s.mapa == criarMapa(s.mapaUniforme, l, a), "M recriou mapa com dimensões antigas");
    s.intensidade = 1;
    montarQuadro(s, 0.5);
  }
  std::cout << "PASS: sessão, troca de tamanho, tecla M e efeitos ativos\n";
}

void testarVram() {
  Vram vram(VRAM_LARGURA * VRAM_ALTURA, 0x7FFF);
  Imagem imagem = criarImagem(256, 240);
  desenharVram(vram, regiaoDaPagina(31), imagem);
  exigir(imagem.largura == VRAM_LARGURA && imagem.altura == VRAM_ALTURA,
         "F1 descarta pixels da VRAM");
  exigir(ler(imagem, 500, 500).r == 255, "VRAM não foi desenhada inteira");
  verificarPng(imagem);
  auto antes = imagem;
  desenharVram({}, regiaoTotal(), imagem);
  exigir(imagem == antes, "VRAM vazia alterou destino");
  for (int i = 0; i < NUM_MOLDES; ++i) {
    for (auto r : {regiaoTotal(), regiaoDaPagina(0), regiaoDaPagina(31)}) {
      auto a = vram, b = vram;
      MOLDES[i].aplicar(a, r, 123);
      MOLDES[i].aplicar(b, r, 123);
      exigir(a == b, "molde não determinístico");
      for (int y = 0; y < VRAM_ALTURA; ++y)
        for (int x = 0; x < VRAM_LARGURA; ++x)
          if (x < r.x || x >= r.x+r.l || y < r.y || y >= r.y+r.a)
            exigir(a[y*VRAM_LARGURA+x] == vram[y*VRAM_LARGURA+x], "molde escapou da região");
    }
  }
  std::cout << "PASS: VRAM completa, PNG e limites dos moldes\n";
}

// O core consulta opções pelo callback de ambiente: só as definidas
// devem ser respondidas, e a última definição de uma chave vale.
void testarOpcoes() {
  retro_variable v{"beetle_psx_internal_resolution", nullptr};
  exigir(!ambiente(RETRO_ENVIRONMENT_GET_VARIABLE, &v) && v.value == nullptr,
         "opção não definida não deveria ter valor");
  emuladorDefinirOpcao("beetle_psx_internal_resolution", "2x");
  emuladorDefinirOpcao("beetle_psx_internal_resolution", "4x");
  exigir(ambiente(RETRO_ENVIRONMENT_GET_VARIABLE, &v) && v.value != nullptr &&
             std::string(v.value) == "4x",
         "opção definida não chegou ao core");
  retro_variable outra{"beetle_psx_pgxp_mode", nullptr};
  exigir(!ambiente(RETRO_ENVIRONMENT_GET_VARIABLE, &outra), "chave errada respondida");
  exigir(!ambiente(RETRO_ENVIRONMENT_GET_VARIABLE, nullptr), "nullptr aceito");
}

int main(int argc, char** argv) {
  try {
    exigir(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
    std::string modo = argc > 1 ? argv[1] : "all";
    if (modo == "pixels" || modo == "all") testarPixels();
    if (modo == "session" || modo == "all") testarSessao();
    if (modo == "vram" || modo == "all") testarVram();
    if (modo == "options" || modo == "all") testarOpcoes();
    SDL_Quit();
    return 0;
  } catch (const std::exception& erro) {
    std::cerr << "FAIL: " << erro.what() << '\n';
    SDL_Quit();
    return 1;
  }
}
