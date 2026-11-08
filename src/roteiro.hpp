// roteiro.hpp — a duração da obra: quando a imagem se corrompe.
//
// O estado visual depende só do tempo t (em segundos), nunca da
// quantidade de quadros. Em 60 Hz ou 120 Hz, o mesmo t dá o mesmo estado.

#pragma once

// Duração de um ciclo completo, em segundos.
inline constexpr double DURACAO_CICLO = 16.0;

// Intensidade global da corrupção no tempo t: 0 = intacto, 1 = ruptura.
//
//   0 s  ─ 4 s   calma       0
//   4 s  ─ 8 s   tensão      sobe até 0.3, com tremores curtos
//   8 s  ─ 10 s  ruptura     1
//  10 s  ─ 14 s  retorno     desce de 1 até 0
//  14 s  ─ 16 s  calma       0  (pausa antes de recomeçar)
float intensidade(double t);
