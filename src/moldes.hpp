// moldes.hpp — moldes de corrupção da VRAM.
//
// Um molde é uma operação deliberada sobre os valores de uma região da
// VRAM. Todos são determinísticos (mesma semente = mesmo resultado) e só
// leem/escrevem dentro da região.

#pragma once

#include <cstdint>

#include "vram.hpp"

struct Molde {
  const char* nome;
  const char* descricao;
  void (*aplicar)(Vram& vram, RegiaoVram r, uint32_t semente);
};

inline constexpr int NUM_MOLDES = 7;

// A tabela de moldes, na ordem em que as teclas K / L percorrem.
extern const Molde MOLDES[NUM_MOLDES];
