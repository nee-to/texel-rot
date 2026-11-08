// cena.hpp — a composição "Memória de um lugar impossível".
//
// Este header expõe uma única função. Todo o resto (camadas, geometria
// do portal) é detalhe interno de cena.cpp.

#pragma once

#include "imagem.hpp"

// Desenha a composição inteira no buffer, camada por camada.
void desenharComposicao(Imagem& buf);

// Mapa de instabilidade da cena: quanto cada pixel pode se corromper.
// 0 = sempre estável (a abertura do portal), 1 = o mais frágil possível.
Mapa criarMapaInstabilidade();
