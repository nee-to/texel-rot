// cena.cpp — as camadas da composição e a geometria do portal.

#include "cena.hpp"

#include <cmath>    // std::sqrt
#include <cstdlib>  // std::abs

#include "paleta.hpp"

// "namespace { ... }" sem nome = tudo aqui dentro só existe neste arquivo.
// Outros .cpp não enxergam essas funções e constantes, o que evita
// conflito de nomes e deixa claro o que é detalhe interno.
namespace {

// Geometria do portal (em pixels da resolução interna).
constexpr int PORTAL_CX = 150;       // centro horizontal do portal
constexpr int PORTAL_NASCENTE = 104; // linha onde o arco começa a curvar
constexpr int PORTAL_EXTERNO = 34;   // meia-largura da pedra (borda de fora)
constexpr int PORTAL_INTERNO = 22;   // meia-largura da abertura
constexpr int CHAO_Y = 150;          // linha do chão

// ---------------------------------------------------------------------------
// Geometria do arco ogival (gótico)
// ---------------------------------------------------------------------------

// Um arco ogival é a interseção de dois círculos.
// Cada círculo tem centro numa das "nascentes" do arco e raio = 2 * w.
// Abaixo da nascente, o arco vira duas paredes retas (|x - cx| <= w).
// Altura da ponta acima da nascente = sqrt(3) * w ≈ 1,73 * w.
// Exemplo: w = 22 -> ponta 38 px acima da nascente.
bool dentroDoArco(int x, int y, int cx, int nascente, int w) {
  if (y >= nascente) {
    return std::abs(x - cx) <= w;
  }
  int dy = y - nascente;
  int dxDir = x - (cx + w); // distância até o centro do círculo da direita
  int dxEsq = x - (cx - w); // distância até o centro do círculo da esquerda
  int raio2 = (2 * w) * (2 * w);
  return dxDir * dxDir + dy * dy <= raio2 && dxEsq * dxEsq + dy * dy <= raio2;
}

// Pixel pertence à abertura do portal (onde aparece o amanhecer)?
bool naAbertura(int x, int y) {
  return y < CHAO_Y &&
         dentroDoArco(x, y, PORTAL_CX, PORTAL_NASCENTE, PORTAL_INTERNO);
}

// A ruína: tijolos do alto à direita do arco desapareceram.
// Quanto mais alto, maior a chance de o tijolo faltar.
bool tijoloQuebrado(int x, int y) {
  if (y >= 80 || x <= PORTAL_CX + 2) {
    return false;
  }
  int linha = y / 4;
  int desloc = (linha % 2) * 4;
  uint32_t h = ruido((x + desloc) / 8, linha + 1000);
  int chance = (80 - y) * 3; // em porcentagem: y=70 -> 30%, y=50 -> 90%
  return static_cast<int>(h % 100) < chance;
}

// Pixel pertence à pedra do portal?
bool naPedra(int x, int y) {
  return y < CHAO_Y &&
         dentroDoArco(x, y, PORTAL_CX, PORTAL_NASCENTE, PORTAL_EXTERNO) &&
         !dentroDoArco(x, y, PORTAL_CX, PORTAL_NASCENTE, PORTAL_INTERNO) &&
         !tijoloQuebrado(x, y);
}

// ---------------------------------------------------------------------------
// Camadas da composição (desenhadas de trás para frente)
// ---------------------------------------------------------------------------

// 1. Céu em faixas horizontais. A troca de faixa é seca, sem degradê suave.
//    (Suavizar isso com dithering pode ser o estudo 2.)
void desenharCeu(Imagem& buf) {
  for (int y = 0; y < ALTURA; y++) {
    int faixa = y / 28; // divisão inteira: 0..27 -> 0, 28..55 -> 1 ...
    if (faixa > 4) {
      faixa = 4;
    }
    for (int x = 0; x < LARGURA; x++) {
      pintar(buf, x, y, CEU[faixa]);
    }
  }
}

// 2. Estrelas espalhadas pelo ruído, raras, sem invadir a lua.
void desenharEstrelas(Imagem& buf) {
  for (int y = 0; y < 130; y++) {
    for (int x = 0; x < LARGURA; x++) {
      int dx = x - 252;
      int dy = y - 40;
      if (dx * dx + dy * dy < 24 * 24) {
        continue; // perto da lua, o brilho dela apaga as estrelas
      }
      uint32_t h = ruido(x, y);
      if (h % 2003 == 0) {
        pintar(buf, x, y, ESTRELA_FORTE);
      } else if (h % 397 == 0) {
        pintar(buf, x, y, ESTRELA_FRACA);
      }
    }
  }
}

// 3. Lua com halo em padrão de xadrez (pixel sim, pixel não).
void desenharLua(Imagem& buf) {
  const int cx = 252, cy = 40, raio = 15;
  for (int y = cy - 22; y <= cy + 22; y++) {
    for (int x = cx - 22; x <= cx + 22; x++) {
      int dx = x - cx;
      int dy = y - cy;
      int d2 = dx * dx + dy * dy;
      if (d2 <= raio * raio) {
        pintar(buf, x, y, LUA);
      } else if (d2 <= 20 * 20 && (x + y) % 2 == 0) {
        pintar(buf, x, y, HALO);
      }
    }
  }
  // Crateras: pequenos retângulos fixos.
  // (Posições assimétricas de propósito: duas crateras alinhadas
  // viram "olhos" e a lua passa a parecer um rosto.)
  retangulo(buf, 243, 39, 3, 2, LUA_CRATERA);
  retangulo(buf, 255, 44, 4, 3, LUA_CRATERA);
  retangulo(buf, 249, 50, 2, 2, LUA_CRATERA);
  retangulo(buf, 258, 30, 2, 2, LUA_CRATERA);
}

// 4. Castelo distante: só silhueta, sem detalhe, para dar profundidade.
void desenharCasteloDistante(Imagem& buf) {
  // Terra ao longe, entre o horizonte e o chão do primeiro plano.
  retangulo(buf, 0, 140, LARGURA, CHAO_Y - 140, SILHUETA);

  // Cada torre: x, largura, y do topo, e se tem telhado pontudo.
  struct Torre {
    int x, l, topo;
    bool telhado;
  };
  const Torre torres[] = {
      {14, 10, 92, true},  {24, 26, 108, false}, {50, 8, 84, true},
      {58, 30, 112, false}, {88, 10, 98, true},  {98, 20, 118, false},
  };

  for (const Torre& t : torres) {
    retangulo(buf, t.x, t.topo, t.l, 140 - t.topo, SILHUETA);

    if (t.telhado) {
      // Triângulo: a cada linha subindo, a largura diminui.
      int altura = t.l + 4;
      for (int i = 0; i < altura; i++) {
        int meia = (t.l / 2) * (altura - i) / altura;
        int y = t.topo - i - 1;
        retangulo(buf, t.x + t.l / 2 - meia, y, 2 * meia + 1, 1, SILHUETA);
      }
    } else {
      // Ameias: um dente de 2 px a cada 4 px no topo da muralha.
      for (int x = t.x; x < t.x + t.l; x += 4) {
        retangulo(buf, x, t.topo - 2, 2, 2, SILHUETA);
      }
    }

    // Poucas janelas acesas: alguém ainda vive ali?
    for (int y = t.topo + 4; y < 136; y += 6) {
      for (int x = t.x + 2; x < t.x + t.l - 2; x += 4) {
        if (ruido(x, y) % 9 == 0) {
          retangulo(buf, x, y, 1, 2, JANELA_ACESA);
        }
      }
    }
  }
}

// 5. Chão do primeiro plano com lajes, e a luz do portal vazando nele.
void desenharChao(Imagem& buf) {
  for (int y = CHAO_Y; y < ALTURA; y++) {
    int fileira = (y - CHAO_Y) / 6;
    for (int x = 0; x < LARGURA; x++) {
      bool junta = (y - CHAO_Y) % 6 == 0 || (x + fileira * 7) % 16 == 0;
      Cor c = junta ? CHAO_JUNTA : CHAO;

      // A luz se abre como um trapézio a partir da abertura.
      int abertura = PORTAL_INTERNO + (y - CHAO_Y) * 3 / 2;
      int dist = std::abs(x - PORTAL_CX);
      if (dist < abertura) {
        // Mais forte no centro e perto da porta, mais fraca nas bordas.
        float forca = 0.45f * (1.0f - static_cast<float>(dist) / abertura) *
                      (1.0f - static_cast<float>(y - CHAO_Y) / 40.0f);
        c = mistura(c, LUZ_PORTAL, forca);
      }
      pintar(buf, x, y, c);
    }
  }
}

// 6. Dentro do portal: um amanhecer que não deveria estar ali.
void desenharAmanhecer(Imagem& buf) {
  const int topo = PORTAL_NASCENTE - 38; // ponta do arco interno
  for (int y = topo; y < CHAO_Y; y++) {
    int faixa = (y - topo) * 5 / (CHAO_Y - topo);
    for (int x = PORTAL_CX - PORTAL_INTERNO; x <= PORTAL_CX + PORTAL_INTERNO;
         x++) {
      if (!naAbertura(x, y)) {
        continue;
      }
      Cor c = AMANHECER[faixa];

      // Sol baixo, parcialmente escondido pelo horizonte.
      int dx = x - PORTAL_CX;
      int dy = y - 132;
      if (dx * dx + dy * dy <= 8 * 8) {
        c = SOL;
      }
      // Horizonte de outro lugar, visto pela porta.
      if (y >= 138) {
        c = HORIZONTE_DENTRO;
      }
      pintar(buf, x, y, c);
    }
  }
}

// 7. A pedra do portal: tijolos, luz da lua, sombra e o brilho do amanhecer.
void desenharPortal(Imagem& buf) {
  for (int y = 0; y < CHAO_Y; y++) {
    for (int x = PORTAL_CX - PORTAL_EXTERNO; x <= PORTAL_CX + PORTAL_EXTERNO;
         x++) {
      if (!naPedra(x, y)) {
        continue;
      }

      // Tijolos de 8 x 4. Linhas alternadas deslocadas em 4 px,
      // como uma parede de verdade. É o mesmo truque de / e % do Canvas.
      int linha = y / 4;
      int desloc = (linha % 2) * 4;
      bool argamassa = (y % 4 == 0) || ((x + desloc) % 8 == 0);

      Cor c;
      if (argamassa) {
        c = ARGAMASSA;
      } else {
        // Cada tijolo sorteia um de três tons, sempre o mesmo.
        c = PEDRA[ruido((x + desloc) / 8, linha) % 3];
      }

      // A lua está à direita: bordas viradas para ela ficam claras,
      // bordas do outro lado ficam escuras.
      if (!naPedra(x + 1, y) && !naAbertura(x + 1, y)) {
        c = PEDRA_LUZ_LUA;
      } else if (!naPedra(x - 1, y) && !naAbertura(x - 1, y)) {
        c = PEDRA_SOMBRA;
      }

      // Brilho do amanhecer na pedra a até 4 px da abertura.
      for (int d = 1; d <= 4; d++) {
        if (naAbertura(x - d, y) || naAbertura(x + d, y) ||
            naAbertura(x, y + d)) {
          float forca = 0.6f * (5 - d) / 4.0f;
          c = mistura(c, LUZ_PORTAL, forca);
          break;
        }
      }
      pintar(buf, x, y, c);
    }
  }
}

// 8. Degraus diante do portal e entulho que caiu da parte quebrada.
void desenharDegrausEEntulho(Imagem& buf) {
  for (int i = 0; i < 3; i++) {
    int y = CHAO_Y + i * 4;
    int meia = 30 + i * 8;
    retangulo(buf, PORTAL_CX - meia, y, 2 * meia, 4, PEDRA[2]);
    // Aresta de cima de cada degrau recebe a luz do portal.
    retangulo(buf, PORTAL_CX - meia, y, 2 * meia, 1,
              mistura(PEDRA[1], LUZ_PORTAL, 0.5f - i * 0.12f));
  }
  // Entulho à direita, abaixo da parte destruída.
  retangulo(buf, 188, 145, 6, 5, PEDRA[0]);
  retangulo(buf, 188, 145, 6, 1, PEDRA_LUZ_LUA);
  retangulo(buf, 196, 147, 4, 3, PEDRA[1]);
  retangulo(buf, 182, 147, 3, 3, PEDRA[2]);
  retangulo(buf, 203, 148, 2, 2, PEDRA[0]);
}

// 9. Pilar quebrado no primeiro plano, à direita, com estandarte rasgado.
void desenharPrimeiroPlano(Imagem& buf) {
  const int x0 = 270, l = 14;
  for (int x = x0; x < x0 + l; x++) {
    int topo = 96 + static_cast<int>(ruido(x, 7) % 7); // topo irregular
    Cor c = (x >= x0 + l - 2) ? PRIMEIRO_PLANO_LUZ : PRIMEIRO_PLANO;
    retangulo(buf, x, topo, 1, ALTURA - topo, c);
  }
  // Estandarte: tecido vermelho com dobras e a ponta de baixo rasgada.
  for (int x = x0 + 2; x < x0 + l - 3; x++) {
    int comprimento = 34 + static_cast<int>(ruido(x, 3) % 6);
    Cor c = (x % 3 == 0) ? ESTANDARTE_DOBRA : ESTANDARTE;
    retangulo(buf, x, 108, 1, comprimento, c);
  }
}

// 10. Morcegos: sprites pequenos definidos por texto ('X' = pixel).
void desenharMorcego(Imagem& buf, int x0, int y0) {
  const char* sprite[] = {
      "X.....X",
      "XX...XX",
      ".XXXXX.",
      "...X...",
  };
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 7; x++) {
      if (sprite[y][x] == 'X') {
        pintar(buf, x0 + x, y0 + y, MORCEGO);
      }
    }
  }
}


}  // namespace

// Monta a composição inteira, camada por camada.
void desenharComposicao(Imagem& buf) {
  desenharCeu(buf);
  desenharEstrelas(buf);
  desenharLua(buf);
  desenharCasteloDistante(buf);
  desenharChao(buf);
  desenharAmanhecer(buf);
  desenharPortal(buf);
  desenharDegrausEEntulho(buf);
  desenharPrimeiroPlano(buf);
  // Um morcego só, na frente da lua: contra o céu escuro
  // ele sumiria, então só a lua o revela.
  desenharMorcego(buf, 254, 33);
}

Mapa criarMapaInstabilidade() {
  Mapa mapa(LARGURA * ALTURA, 0.0f);

  for (int y = 0; y < ALTURA; y++) {
    for (int x = 0; x < LARGURA; x++) {
      // 1. A abertura do portal é o núcleo: nunca se corrompe.
      //    É ela que mantém a imagem reconhecível durante a ruptura.
      if (naAbertura(x, y)) {
        continue; // fica 0
      }

      // 2. Quanto mais longe do portal, mais frágil a memória.
      //    d = distância até o centro do portal.
      //    Até 45 px: estável. De 45 a 185 px: sobe de 0 até 0.8.
      //    Exemplo: d = 115 -> (115 - 45) / 140 = 0.5 -> 0.5 * 0.8 = 0.4
      float dx = static_cast<float>(x - PORTAL_CX);
      float dy = static_cast<float>(y - PORTAL_NASCENTE);
      float d = std::sqrt(dx * dx + dy * dy);
      float valor = (d - 45.0f) / 140.0f;
      if (valor < 0.0f) valor = 0.0f;
      if (valor > 1.0f) valor = 1.0f;
      valor *= 0.8f;

      // 3. A parte que já está desmoronando (alto à direita do arco)
      //    é a mais frágil de todas, mesmo estando perto do centro.
      bool zonaDaRuina = x > PORTAL_CX && x < PORTAL_CX + PORTAL_EXTERNO + 10 &&
                         y > 35 && y < 86;
      if (zonaDaRuina) {
        valor = 1.0f;
      }

      mapa[y * LARGURA + x] = valor;
    }
  }
  return mapa;
}
