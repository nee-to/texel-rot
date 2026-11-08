// paleta.hpp — as cores da obra.
//
// Ficam todas juntas para que a paleta seja uma decisão visível:
// mudar o clima da imagem é editar este arquivo.

#pragma once

#include "imagem.hpp"  // struct Cor

// Paleta pequena e fixa. Cada cor tem um papel na composição.
inline constexpr Cor CEU[5] = {
    {8, 6, 14},  {12, 10, 22}, {18, 14, 32}, // noite profunda (topo)
    {26, 19, 44}, {36, 25, 56},              // perto do horizonte
};
inline constexpr Cor ESTRELA_FRACA = {120, 110, 160};
inline constexpr Cor ESTRELA_FORTE = {220, 210, 240};
inline constexpr Cor LUA = {232, 220, 192};
inline constexpr Cor LUA_CRATERA = {196, 182, 152};
inline constexpr Cor HALO = {52, 38, 74};
inline constexpr Cor SILHUETA = {14, 10, 24};    // castelo distante e terra ao longe
inline constexpr Cor JANELA_ACESA = {230, 150, 80};
inline constexpr Cor CHAO = {22, 17, 32};
inline constexpr Cor CHAO_JUNTA = {14, 10, 22};
inline constexpr Cor ARGAMASSA = {28, 22, 40};
inline constexpr Cor PEDRA[3] = {{62, 52, 82}, {72, 60, 92}, {56, 46, 74}};
inline constexpr Cor PEDRA_LUZ_LUA = {120, 108, 140};
inline constexpr Cor PEDRA_SOMBRA = {36, 30, 50};
inline constexpr Cor LUZ_PORTAL = {214, 128, 92}; // luz "impossível" que vaza do portal
inline constexpr Cor AMANHECER[5] = {
    {255, 232, 176}, {248, 200, 128}, {232, 152, 96},
    {196, 96, 80},   {120, 48, 72},
};
inline constexpr Cor SOL = {255, 248, 224};
inline constexpr Cor HORIZONTE_DENTRO = {80, 32, 60};
inline constexpr Cor PRIMEIRO_PLANO = {24, 18, 36};
inline constexpr Cor PRIMEIRO_PLANO_LUZ = {70, 58, 92};
inline constexpr Cor ESTANDARTE = {110, 24, 40};
inline constexpr Cor ESTANDARTE_DOBRA = {72, 14, 30};
inline constexpr Cor MORCEGO = {10, 8, 16};
