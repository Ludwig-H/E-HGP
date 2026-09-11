// Wrapper stub : compile src/gpu/device_witness.cu comme du C++ pur et
// execute les kernels en boucles hote (tests/cuda_stub.hpp) — une preuve de
// syntaxe et de logique C++ HOTE, jamais une preuve nvcc ni un recu device
// (voir l'en-tete du .cu). Portes locales : 0 (nominal), 4
// (witness-di128-lost-carry ; witness-skip-write), 1 (contre-fixture double
// injection skip+carry : les oracles tranchent, jamais un 4 aveugle).
#define MHGP7_FAKE_DEVICE 1
#include "cuda_stub.hpp"

#include "../src/gpu/device_witness.cu"  // NOLINT

