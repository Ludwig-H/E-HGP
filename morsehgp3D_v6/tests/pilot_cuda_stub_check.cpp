// Wrapper stub du PILOTE serie C : compile cli/mhgp6_cuda.cu comme du C++
// pur (tests/cuda_stub.hpp — cudaMalloc = malloc, kernels en boucles hote).
// Preuve de SYNTAXE ET LOGIQUE C++ hote du pilote avant toute session —
// jamais une preuve nvcc ni un recu device. La porte associee n'exerce que
// le REFUS de parsing (la parite fonctionnelle est deja prouvee par
// mhgp6_pilot_stub sur la meme route).
#define MHGP6_FAKE_DEVICE 1
#include "cuda_stub.hpp"

#include "../cli/mhgp6_cuda.cu"  // NOLINT
