// emulador.cpp — implementação do frontend libretro mínimo.
//
// Referência da interface: third_party/libretro/libretro.h
// (MIT, The RetroArch team — https://github.com/libretro/RetroArch)

#include "emulador.hpp"

#include <SDL3/SDL.h>

#include <algorithm>  // std::search
#include <cstdarg>  // va_list, para a função de log do core
#include <cstdio>
#include <cstring>  // std::memcpy
#include <fstream>
#include <iostream>
#include <vector>

#include "libretro.h"

// Tudo neste namespace sem nome é estado interno deste arquivo.
// Usamos variáveis globais aqui porque o libretro chama funções nossas
// (callbacks) sem passar nenhum contexto: elas precisam achar o estado.
namespace {

// Ponteiros para as funções que o core exporta. Um ponteiro de função
// guarda o endereço de uma função; chamar "core.run()" executa a função
// que está dentro do .so.
struct Core {
  SDL_SharedObject* biblioteca = nullptr;
  void (*init)() = nullptr;
  void (*deinit)() = nullptr;
  unsigned (*apiVersion)() = nullptr;
  void (*getSystemInfo)(retro_system_info*) = nullptr;
  void (*getSystemAvInfo)(retro_system_av_info*) = nullptr;
  void (*setEnvironment)(retro_environment_t) = nullptr;
  void (*setVideoRefresh)(retro_video_refresh_t) = nullptr;
  void (*setAudioSample)(retro_audio_sample_t) = nullptr;
  void (*setAudioSampleBatch)(retro_audio_sample_batch_t) = nullptr;
  void (*setInputPoll)(retro_input_poll_t) = nullptr;
  void (*setInputState)(retro_input_state_t) = nullptr;
  void (*run)() = nullptr;
  bool (*loadGame)(const retro_game_info*) = nullptr;
  void (*unloadGame)() = nullptr;
  size_t (*serializeSize)() = nullptr;
  bool (*serialize)(void*, size_t) = nullptr;
  bool (*unserialize)(const void*, size_t) = nullptr;
};

Core core;
bool coreIniciado = false;
bool jogoCarregado = false;
std::string pastaSistema;
std::vector<uint8_t> dadosJogo;  // só usado se o core pedir o jogo na memória
// Estados salvos. Slot 0 = "desfazer" automático; 1 a 3 = teclas F5-F7.
std::vector<uint8_t> estados[NUM_SLOTS];
// Rascunho reaproveitado para ler/escrever a VRAM pelo estado salvo.
std::vector<uint8_t> rascunho;
double fps = 60.0;
SDL_AudioStream* audio = nullptr;
int taxaAudio = 0;

// Formato de pixel que o core vai usar. O padrão do libretro é 0RGB1555.
retro_pixel_format formato = RETRO_PIXEL_FORMAT_0RGB1555;

// Último quadro recebido, já convertido para RGBA, no tamanho nativo.
std::vector<uint8_t> quadroNativo;
unsigned quadroLargura = 0;
unsigned quadroAltura = 0;

// Procura uma função no .so pelo nome e guarda o endereço em "destino".
// "template" = a mesma função serve para qualquer tipo de ponteiro.
template <typename T>
bool carregarFuncao(T& destino, const char* nome) {
  destino = reinterpret_cast<T>(SDL_LoadFunction(core.biblioteca, nome));
  if (destino == nullptr) {
    std::cerr << "core sem a função " << nome << '\n';
    return false;
  }
  return true;
}

// --- Callbacks: funções NOSSAS que o core chama -------------------------

// Mensagens de log do core. "..." = número variável de argumentos,
// como o printf. Mostramos só avisos e erros.
void logDoCore(retro_log_level nivel, const char* formatoTexto, ...) {
  if (nivel < RETRO_LOG_WARN) {
    return;
  }
  va_list args;
  va_start(args, formatoTexto);
  std::fprintf(stderr, "[core] ");
  std::vfprintf(stderr, formatoTexto, args);
  va_end(args);
}

// O core pergunta coisas ao frontend por aqui ("onde fica a BIOS?",
// "posso usar o formato XRGB8888?"). Devolver false = "não sei/não aceito".
bool ambiente(unsigned comando, void* dados) {
  switch (comando) {
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
      // Aceitamos que o core repita o quadro anterior mandando nullptr.
      *static_cast<bool*>(dados) = true;
      return true;

    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
      auto pedido = *static_cast<const retro_pixel_format*>(dados);
      if (pedido == RETRO_PIXEL_FORMAT_XRGB8888 ||
          pedido == RETRO_PIXEL_FORMAT_RGB565 ||
          pedido == RETRO_PIXEL_FORMAT_0RGB1555) {
        formato = pedido;
        return true;
      }
      return false;
    }

    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
      *static_cast<const char**>(dados) = pastaSistema.c_str();
      return true;

    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
      static_cast<retro_log_callback*>(dados)->log = logDoCore;
      return true;

    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
      // Nenhuma opção do core mudou.
      *static_cast<bool*>(dados) = false;
      return true;

    default:
      // Opções do core, renderização por GPU etc.: não suportamos.
      // O core usa os valores padrão.
      return false;
  }
}

// O core entrega um quadro de vídeo. "pitch" = bytes por linha, que pode
// ser MAIOR que largura * bytes_por_pixel (sobra no fim de cada linha).
void quadroDeVideo(const void* dados, unsigned largura, unsigned altura,
                   size_t pitch) {
  if (dados == nullptr) {
    return;  // quadro repetido: mantemos o anterior
  }
  quadroLargura = largura;
  quadroAltura = altura;
  quadroNativo.resize(static_cast<size_t>(largura) * altura * 4);

  const auto* bytes = static_cast<const uint8_t*>(dados);

  for (unsigned y = 0; y < altura; y++) {
    // Início da linha y: usamos o pitch do core, não a largura.
    const uint8_t* linha = bytes + y * pitch;

    for (unsigned x = 0; x < largura; x++) {
      uint8_t r = 0, g = 0, b = 0;

      if (formato == RETRO_PIXEL_FORMAT_XRGB8888) {
        // 32 bits por pixel: 0x00RRGGBB. memcpy lê os 4 bytes com
        // segurança, sem depender de alinhamento de memória.
        uint32_t p;
        std::memcpy(&p, linha + x * 4, 4);
        r = (p >> 16) & 0xFF;
        g = (p >> 8) & 0xFF;
        b = p & 0xFF;
      } else {
        // 16 bits por pixel.
        uint16_t p;
        std::memcpy(&p, linha + x * 2, 2);
        if (formato == RETRO_PIXEL_FORMAT_RGB565) {
          // RRRRRGGGGGGBBBBB: 5 bits de R, 6 de G, 5 de B.
          // Esticamos para 0..255. Exemplo: 31 de 5 bits -> 31*255/31 = 255.
          r = static_cast<uint8_t>(((p >> 11) & 0x1F) * 255 / 31);
          g = static_cast<uint8_t>(((p >> 5) & 0x3F) * 255 / 63);
          b = static_cast<uint8_t>((p & 0x1F) * 255 / 31);
        } else {
          // 0RRRRRGGGGGBBBBB: 5 bits para cada canal.
          r = static_cast<uint8_t>(((p >> 10) & 0x1F) * 255 / 31);
          g = static_cast<uint8_t>(((p >> 5) & 0x1F) * 255 / 31);
          b = static_cast<uint8_t>((p & 0x1F) * 255 / 31);
        }
      }

      size_t i = (static_cast<size_t>(y) * largura + x) * 4;
      quadroNativo[i + 0] = r;
      quadroNativo[i + 1] = g;
      quadroNativo[i + 2] = b;
      quadroNativo[i + 3] = 255;
    }
  }
}

// libretro entrega pares estéreo de inteiros de 16 bits. O stream copia
// esses bytes e o SDL converte para o formato da saída padrão do sistema.
size_t loteDeAudio(const int16_t* dados, size_t quadros) {
  if (audio == nullptr || dados == nullptr || quadros == 0) {
    return quadros;
  }
  // Impede que uma travada acumule segundos de som atrasado.
  // Estéreo S16 = 4 bytes/quadro; 250 ms = taxaAudio bytes.
  if (SDL_GetAudioStreamQueued(audio) > taxaAudio) {
    SDL_ClearAudioStream(audio);
  }
  // Enviamos em blocos para manter o tamanho dentro do int aceito pelo SDL.
  size_t enviados = 0;
  while (enviados < quadros) {
    size_t bloco = std::min(quadros - enviados, size_t{4096});
    if (!SDL_PutAudioStreamData(audio, dados + enviados * 2,
                                static_cast<int>(bloco * 2 * sizeof(int16_t)))) {
      std::cerr << "falha ao enviar áudio: " << SDL_GetError() << '\n';
      return enviados;
    }
    enviados += bloco;
  }
  return enviados;
}

void amostraDeAudio(int16_t esquerda, int16_t direita) {
  const int16_t par[] = {esquerda, direita};
  loteDeAudio(par, 1);
}

// O SDL já lê o teclado no laço de eventos do main.
void lerEntrada() {}

// O core pergunta "o botão X do controle 1 está apertado?".
// Mapeamento do teclado para o RetroPad (controle genérico do libretro;
// os nomes entre parênteses são os equivalentes no PlayStation):
//   setas = direcional | Z = B (✕) | X = A (◯) | A = Y (▢) | S = X (△)
//   Enter = Start | Backspace = Select | Q / W = L / R | E / R = L2 / R2
int16_t estadoDaEntrada(unsigned porta, unsigned dispositivo, unsigned,
                        unsigned id) {
  if (porta != 0 || dispositivo != RETRO_DEVICE_JOYPAD) {
    return 0;
  }
  const bool* teclas = SDL_GetKeyboardState(nullptr);
  switch (id) {
    case RETRO_DEVICE_ID_JOYPAD_UP: return teclas[SDL_SCANCODE_UP];
    case RETRO_DEVICE_ID_JOYPAD_DOWN: return teclas[SDL_SCANCODE_DOWN];
    case RETRO_DEVICE_ID_JOYPAD_LEFT: return teclas[SDL_SCANCODE_LEFT];
    case RETRO_DEVICE_ID_JOYPAD_RIGHT: return teclas[SDL_SCANCODE_RIGHT];
    case RETRO_DEVICE_ID_JOYPAD_B: return teclas[SDL_SCANCODE_Z];
    case RETRO_DEVICE_ID_JOYPAD_A: return teclas[SDL_SCANCODE_X];
    case RETRO_DEVICE_ID_JOYPAD_Y: return teclas[SDL_SCANCODE_A];
    case RETRO_DEVICE_ID_JOYPAD_X: return teclas[SDL_SCANCODE_S];
    case RETRO_DEVICE_ID_JOYPAD_START: return teclas[SDL_SCANCODE_RETURN];
    case RETRO_DEVICE_ID_JOYPAD_SELECT: return teclas[SDL_SCANCODE_BACKSPACE];
    case RETRO_DEVICE_ID_JOYPAD_L: return teclas[SDL_SCANCODE_Q];
    case RETRO_DEVICE_ID_JOYPAD_R: return teclas[SDL_SCANCODE_W];
    case RETRO_DEVICE_ID_JOYPAD_L2: return teclas[SDL_SCANCODE_E];
    case RETRO_DEVICE_ID_JOYPAD_R2: return teclas[SDL_SCANCODE_R];
    default: return 0;
  }
}

}  // namespace

// --- Funções públicas (declaradas em emulador.hpp) ------------------------

bool emuladorCarregarCore(const std::string& caminhoCore,
                          const std::string& pastaSistemaCore) {
  pastaSistema = pastaSistemaCore;

  core.biblioteca = SDL_LoadObject(caminhoCore.c_str());
  if (core.biblioteca == nullptr) {
    std::cerr << "não abriu o core: " << SDL_GetError() << '\n';
    return false;
  }

  // "&&" para no primeiro false: se faltar uma função, desistimos.
  bool ok = carregarFuncao(core.init, "retro_init") &&
            carregarFuncao(core.deinit, "retro_deinit") &&
            carregarFuncao(core.apiVersion, "retro_api_version") &&
            carregarFuncao(core.getSystemInfo, "retro_get_system_info") &&
            carregarFuncao(core.getSystemAvInfo, "retro_get_system_av_info") &&
            carregarFuncao(core.setEnvironment, "retro_set_environment") &&
            carregarFuncao(core.setVideoRefresh, "retro_set_video_refresh") &&
            carregarFuncao(core.setAudioSample, "retro_set_audio_sample") &&
            carregarFuncao(core.setAudioSampleBatch,
                           "retro_set_audio_sample_batch") &&
            carregarFuncao(core.setInputPoll, "retro_set_input_poll") &&
            carregarFuncao(core.setInputState, "retro_set_input_state") &&
            carregarFuncao(core.run, "retro_run") &&
            carregarFuncao(core.loadGame, "retro_load_game") &&
            carregarFuncao(core.unloadGame, "retro_unload_game") &&
            carregarFuncao(core.serializeSize, "retro_serialize_size") &&
            carregarFuncao(core.serialize, "retro_serialize") &&
            carregarFuncao(core.unserialize, "retro_unserialize");
  if (!ok) {
    return false;
  }

  if (core.apiVersion() != RETRO_API_VERSION) {
    std::cerr << "versão da API libretro incompatível\n";
    return false;
  }

  // Ordem exigida pelo libretro: ambiente ANTES do init.
  core.setEnvironment(ambiente);
  core.init();
  coreIniciado = true;
  core.setVideoRefresh(quadroDeVideo);
  core.setAudioSample(amostraDeAudio);
  core.setAudioSampleBatch(loteDeAudio);
  core.setInputPoll(lerEntrada);
  core.setInputState(estadoDaEntrada);

  retro_system_info info{};
  core.getSystemInfo(&info);
  std::cout << "core: " << info.library_name << ' ' << info.library_version
            << '\n';
  return true;
}

bool emuladorCarregarJogo(const std::string& caminhoJogo) {
  retro_system_info sistema{};
  core.getSystemInfo(&sistema);

  retro_game_info jogo{};
  jogo.path = caminhoJogo.c_str();

  // Alguns cores leem o arquivo sozinhos (need_fullpath); outros querem
  // o conteúdo já carregado na memória.
  if (!sistema.need_fullpath) {
    std::ifstream arquivo(caminhoJogo, std::ios::binary);
    if (!arquivo) {
      std::cerr << "não abriu o jogo: " << caminhoJogo << '\n';
      return false;
    }
    dadosJogo.assign(std::istreambuf_iterator<char>(arquivo),
                     std::istreambuf_iterator<char>());
    jogo.data = dadosJogo.data();
    jogo.size = dadosJogo.size();
  }

  if (!core.loadGame(&jogo)) {
    std::cerr << "o core recusou o conteúdo (BIOS ausente? formato?)\n";
    return false;
  }
  jogoCarregado = true;

  retro_system_av_info av{};
  core.getSystemAvInfo(&av);
  fps = av.timing.fps;
  if (!(av.timing.sample_rate >= 8000 && av.timing.sample_rate <= 384000)) {
    std::cerr << "taxa de áudio inválida recebida do core\n";
    return false;
  }
  taxaAudio = static_cast<int>(av.timing.sample_rate);
  SDL_AudioSpec formatoAudio{};
  formatoAudio.format = SDL_AUDIO_S16;
  formatoAudio.channels = 2;
  formatoAudio.freq = taxaAudio;
  audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                    &formatoAudio, nullptr, nullptr);
  if (audio == nullptr || !SDL_ResumeAudioStreamDevice(audio)) {
    std::cerr << "não abriu a saída de áudio: " << SDL_GetError() << '\n';
    return false;
  }
  std::cout << "áudio: " << taxaAudio << " Hz, estéreo\n";
  std::cout << "conteúdo carregado: " << av.geometry.base_width << 'x'
            << av.geometry.base_height << " a " << fps << " fps\n";
  return true;
}

double emuladorFps() { return fps; }

void emuladorRodarQuadro() { core.run(); }

void emuladorCopiarQuadro(Imagem& destino) {
  if (quadroNativo.empty()) {
    return;
  }
  // Reamostragem "vizinho mais próximo": para cada pixel do destino,
  // pegamos o pixel correspondente do quadro nativo.
  // Exemplo: nativo 640 de largura, destino 320 -> x = 10 lê x = 20.
  for (int y = 0; y < ALTURA; y++) {
    unsigned sy = static_cast<unsigned>(y) * quadroAltura / ALTURA;
    for (int x = 0; x < LARGURA; x++) {
      unsigned sx = static_cast<unsigned>(x) * quadroLargura / LARGURA;
      size_t i = (static_cast<size_t>(sy) * quadroLargura + sx) * 4;
      pintar(destino, x, y,
             {quadroNativo[i], quadroNativo[i + 1], quadroNativo[i + 2]});
    }
  }
}

bool emuladorSalvarEstado(int slot) {
  if (slot < 0 || slot >= NUM_SLOTS) {
    return false;
  }
  estados[slot].resize(core.serializeSize());
  return core.serialize(estados[slot].data(), estados[slot].size());
}

bool emuladorCarregarEstado(int slot) {
  if (slot < 0 || slot >= NUM_SLOTS || estados[slot].empty()) {
    return false;
  }
  return core.unserialize(estados[slot].data(), estados[slot].size());
}

// --- VRAM pelo estado salvo -----------------------------------------------
//
// O Beetle PSX não entrega a VRAM pela interface libretro, mas o estado
// salvo dele (formato do Mednafen) guarda a VRAM inteira numa entrada:
//
//   [1 byte: tamanho do nome = 13] ["&GPURAM[0][0]"] [4 bytes: tamanho]
//   [1.048.576 bytes: 512 linhas x 1024 valores de 16 bits]
//
// Achamos essa entrada no estado e copiamos os bytes de/para ela.
// Isso depende do formato interno do Beetle: se uma versão nova mudar o
// nome, a busca falha e as funções devolvem false (sem estragar nada).
namespace {

// Devolve a posição onde começam os dados da VRAM no estado, ou -1.
long long acharVram(const std::vector<uint8_t>& estado) {
  static const char marca[] = "\x0D&GPURAM[0][0]"; // 1 + 13 bytes
  const size_t tamMarca = sizeof(marca) - 1;        // sem o '\0' final
  const size_t bytesVram = VRAM_LARGURA * VRAM_ALTURA * 2;

  auto it = std::search(estado.begin(), estado.end(), marca, marca + tamMarca);
  if (it == estado.end()) {
    return -1;
  }
  size_t posTamanho = static_cast<size_t>(it - estado.begin()) + tamMarca;
  if (posTamanho + 4 + bytesVram > estado.size()) {
    return -1; // os dados não caberiam no estado: formato inesperado
  }
  uint32_t tamanho;
  std::memcpy(&tamanho, estado.data() + posTamanho, 4);
  if (tamanho != bytesVram) {
    return -1;
  }
  return static_cast<long long>(posTamanho + 4);
}

}  // namespace

bool emuladorLerVram(std::vector<uint16_t>& vram) {
  rascunho.resize(core.serializeSize());
  if (!core.serialize(rascunho.data(), rascunho.size())) {
    return false;
  }
  long long pos = acharVram(rascunho);
  if (pos < 0) {
    return false;
  }
  vram.resize(VRAM_LARGURA * VRAM_ALTURA);
  std::memcpy(vram.data(), rascunho.data() + pos, vram.size() * 2);
  return true;
}

bool emuladorEscreverVram(const std::vector<uint16_t>& vram) {
  if (vram.size() != static_cast<size_t>(VRAM_LARGURA * VRAM_ALTURA)) {
    return false;
  }
  // Pegamos o estado ATUAL, trocamos só a VRAM e carregamos de volta.
  rascunho.resize(core.serializeSize());
  if (!core.serialize(rascunho.data(), rascunho.size())) {
    return false;
  }
  long long pos = acharVram(rascunho);
  if (pos < 0) {
    return false;
  }
  std::memcpy(rascunho.data() + pos, vram.data(), vram.size() * 2);
  return core.unserialize(rascunho.data(), rascunho.size());
}

void emuladorEncerrar() {
  if (audio != nullptr) {
    // Também fecha o dispositivo aberto junto com o stream.
    SDL_DestroyAudioStream(audio);
    audio = nullptr;
  }
  taxaAudio = 0;
  if (jogoCarregado) {
    core.unloadGame();
    jogoCarregado = false;
  }
  if (coreIniciado) {
    core.deinit();
    coreIniciado = false;
  }
  if (core.biblioteca != nullptr) {
    SDL_UnloadObject(core.biblioteca);
    core.biblioteca = nullptr;
  }
}
