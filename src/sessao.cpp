// sessao.cpp — teclas, ritmo da emulação e montagem do quadro.

#include "sessao.hpp"

#include <iostream>

#include "efeitos.hpp"
#include "emulador.hpp"
#include "moldes.hpp"
#include "roteiro.hpp"
#include "tela.hpp"

namespace {

RegiaoVram regiaoAtual(const Sessao& s) {
  return s.vramInteira ? regiaoTotal() : regiaoDaPagina(s.pagina);
}

std::string nomeRegiao(const Sessao& s) {
  return s.vramInteira ? "VRAM inteira" : "página " + std::to_string(s.pagina);
}

// Aplica o molde atual: guarda o desfazer, lê, altera, escreve de volta.
void aplicarMolde(Sessao& s) {
  emuladorSalvarEstado(0);
  if (!emuladorLerVram(s.vram)) {
    std::cout << "este core não expõe a VRAM no estado salvo\n";
    return;
  }
  const Molde& m = MOLDES[s.molde];
  m.aplicar(s.vram, regiaoAtual(s), s.semente++);
  emuladorEscreverVram(s.vram);
  s.quadrosDesdeLeitura = 999;  // a visualização relê na próxima volta
  std::cout << m.nome << " em " << nomeRegiao(s) << '\n';
}

void trocarMolde(Sessao& s, int passo) {
  s.molde = (s.molde + passo + NUM_MOLDES) % NUM_MOLDES;
  std::cout << "molde: " << MOLDES[s.molde].nome << " — "
            << MOLDES[s.molde].descricao << '\n';
}

}  // namespace

void iniciarSessao(Sessao& s, const std::string& pastaCapturas) {
  s.mapa = criarMapa(s.mapaUniforme);
  s.passo = 1.0 / emuladorFps();
  s.pastaCapturas = pastaCapturas;
}

void tratarTecla(Sessao& s, const SDL_KeyboardEvent& tecla) {
  if (tecla.repeat) {
    return;  // ignora a repetição automática de tecla segurada
  }
  SDL_Keycode k = tecla.key;
  // Scancode = posição física da tecla; usado para , e . (iguais no ABNT2).
  SDL_Scancode sc = tecla.scancode;

  if (k == SDLK_ESCAPE) {
    s.rodando = false;

    // --- VRAM ---
  } else if (k == SDLK_C) {
    aplicarMolde(s);
  } else if (k == SDLK_K || k == SDLK_L) {
    trocarMolde(s, k == SDLK_L ? 1 : -1);
  } else if (k == SDLK_U) {
    std::cout << (emuladorCarregarEstado(0) ? "desfeito\n"
                                            : "nada para desfazer\n");
    s.quadrosDesdeLeitura = 999;
  } else if (k == SDLK_F1) {
    s.verVram = !s.verVram;
    s.quadrosDesdeLeitura = 999;
  } else if (sc == SDL_SCANCODE_COMMA || sc == SDL_SCANCODE_PERIOD) {
    int passo = (sc == SDL_SCANCODE_PERIOD) ? 1 : -1;
    s.pagina = (s.pagina + passo + NUM_PAGINAS) % NUM_PAGINAS;
    s.vramInteira = false;
    std::cout << nomeRegiao(s) << '\n';
  } else if (k == SDLK_B) {
    s.vramInteira = !s.vramInteira;
    std::cout << "região: " << nomeRegiao(s) << '\n';

    // --- Imagem final ---
  } else if (k >= SDLK_0 && k <= SDLK_9) {
    // Os códigos de 0..9 são seguidos: k - SDLK_0 dá 0..9.
    s.intensidade = static_cast<float>(k - SDLK_0) / 9.0f;
    s.automatico = false;
    std::cout << "intensidade: " << s.intensidade << '\n';
  } else if (k == SDLK_TAB) {
    s.automatico = !s.automatico;
    std::cout << "roteiro: " << (s.automatico ? "ligado" : "desligado") << '\n';
  } else if (k == SDLK_M) {
    s.mapaUniforme = !s.mapaUniforme;
    s.mapa = criarMapa(s.mapaUniforme);
    std::cout << (s.mapaUniforme ? "mapa uniforme\n" : "mapa radial\n");

    // --- Estados, pausa, captura ---
  } else if (k == SDLK_F5 || k == SDLK_F6 || k == SDLK_F7) {
    int slot = 1 + static_cast<int>(k - SDLK_F5);
    std::cout << (emuladorSalvarEstado(slot) ? "salvo no slot "
                                             : "falha no slot ")
              << slot << '\n';
  } else if (k == SDLK_F9 || k == SDLK_F10 || k == SDLK_F11) {
    int slot = 1 + static_cast<int>(k - SDLK_F9);
    std::cout << (emuladorCarregarEstado(slot) ? "carregado slot "
                                               : "slot vazio: ")
              << slot << '\n';
    s.quadrosDesdeLeitura = 999;
  } else if (k == SDLK_P) {
    s.pausado = !s.pausado;
  } else if (k == SDLK_F12) {
    salvarCaptura(s.destino, s.pastaCapturas);
  }
}

void avancarEmulacao(Sessao& s, double dt) {
  if (s.pausado) {
    return;
  }
  // Acumula o tempo real e emula um quadro a cada "passo" segundos.
  // Assim o conteúdo roda na velocidade certa em monitor de 60 ou 120 Hz.
  // No máximo 4 por volta, para não correr atrás para sempre se travar.
  s.acumulado += dt;
  int rodados = 0;
  while (s.acumulado >= s.passo && rodados < 4) {
    emuladorRodarQuadro();
    s.acumulado -= s.passo;
    rodados++;
  }
  if (rodados == 4) {
    s.acumulado = 0.0;
  }
}

void montarQuadro(Sessao& s, double t) {
  if (s.verVram) {
    // Ler a VRAM copia o estado inteiro: só a cada 6 voltas.
    if (++s.quadrosDesdeLeitura >= 6) {
      emuladorLerVram(s.vram);
      s.quadrosDesdeLeitura = 0;
    }
    if (!s.vram.empty()) {
      desenharVram(s.vram, regiaoAtual(s), s.destino);
    }
    return;
  }

  // ORIGEM = quadro do emulador. DESTINO = origem + faixas + blocos.
  emuladorCopiarQuadro(s.origem);
  s.destino = s.origem;
  float forca = s.automatico ? intensidade(t) : s.intensidade;
  deslocarFaixas(s.origem, s.destino, s.mapa, forca, 24,
                 static_cast<uint32_t>(t * 8.0));
  trocarBlocos(s.origem, s.destino, s.mapa, forca,
               static_cast<uint32_t>(t * 4.0));
}
