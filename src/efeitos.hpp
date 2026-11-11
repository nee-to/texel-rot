// efeitos.hpp — corrupções na imagem final, quadro a quadro.
//
// Regras:
// - leem da ORIGEM (const: nunca é alterada) e escrevem no DESTINO;
// - o MAPA diz quanto cada pixel pode se corromper (0 = estável);
// - intensidade 0 não muda nada;
// - "momento" vem do tempo: o mesmo momento dá sempre o mesmo resultado.

#pragma once

#include <cstdint>

#include "imagem.hpp"

// Mapa radial (centro estável, bordas instáveis) ou uniforme (tudo 1).
Mapa criarMapa(bool uniforme, int largura = LARGURA, int altura = ALTURA);

// Faixas horizontais lidas deslocadas para os lados.
// amplitude = deslocamento máximo em px com intensidade 1.
void deslocarFaixas(const Imagem& origem, Imagem& destino, const Mapa& mapa,
                    float intensidade, int amplitude, uint32_t momento);

// Blocos de 8 x 4 substituídos por blocos de um lugar próximo.
void trocarBlocos(const Imagem& origem, Imagem& destino, const Mapa& mapa,
                  float intensidade, uint32_t momento);
