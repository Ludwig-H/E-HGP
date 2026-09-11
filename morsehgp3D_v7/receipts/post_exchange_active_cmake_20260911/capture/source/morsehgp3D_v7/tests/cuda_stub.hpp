// Stub CUDA HOTE (MHGP7_FAKE_DEVICE) — infrastructure de test locale.
//
// But : compiler les .cu de src/gpu/ comme du C++ pur (via un wrapper .cpp
// qui definit MHGP7_FAKE_DEVICE puis inclut ce stub et le .cu) et EXECUTER
// les kernels en boucles hote sequentielles, pour prouver localement la
// syntaxe ET la logique (mutants tues, oracle hote) avant de payer une
// session G4. Un run stub n'est jamais un recu device : le nom publie est
// `stub-hote`, sm=0.0.
//
// Semantique simulee : grille 1D uniquement (les kernels v6 n'utilisent que
// blockIdx.x / blockDim.x / threadIdx.x), execution strictement sequentielle
// thread par thread — aucune concurrence, donc aucun test de course ici ;
// la bit-identite lots x executeurs se prouve par les portes G2', pas par ce
// stub. `cudaMalloc` = malloc, `cudaMemcpy` = memcpy, erreurs impossibles
// (le chemin REFUS cuda se teste sur device reel seulement).
#pragma once
#if !defined(MHGP7_FAKE_DEVICE)
#error "cuda_stub.hpp ne se compile que sous MHGP7_FAKE_DEVICE"
#endif
#if defined(__CUDACC__)
#error "cuda_stub.hpp ne doit jamais etre vu par nvcc"
#endif

#include <cstdlib>
#include <cstring>

#define __global__

namespace mhgp7_stub {

struct Dim3 {
  unsigned x = 1, y = 1, z = 1;
};
inline Dim3 grid_dim, block_dim, block_idx, thread_idx;

}  // namespace mhgp7_stub

// Les kernels referencent blockIdx/blockDim/threadIdx sans qualification.
#define blockIdx mhgp7_stub::block_idx
#define blockDim mhgp7_stub::block_dim
#define threadIdx mhgp7_stub::thread_idx
#define gridDim mhgp7_stub::grid_dim

using cudaError_t = int;
inline constexpr cudaError_t cudaSuccess = 0;
enum cudaMemcpyKind { cudaMemcpyHostToDevice = 1, cudaMemcpyDeviceToHost = 2, cudaMemcpyDeviceToDevice = 3 };

struct cudaDeviceProp {
  char name[256];
  int major, minor;
};

inline const char* cudaGetErrorString(cudaError_t) { return "stub-hote"; }
inline cudaError_t cudaGetLastError() { return cudaSuccess; }
inline cudaError_t cudaDeviceSynchronize() { return cudaSuccess; }

inline cudaError_t cudaGetDeviceCount(int* n) {
  *n = 1;
  return cudaSuccess;
}

inline cudaError_t cudaGetDeviceProperties(cudaDeviceProp* p, int) {
  std::memset(p, 0, sizeof(*p));
  std::strncpy(p->name, "stub-hote", sizeof(p->name) - 1);
  p->major = 0;
  p->minor = 0;
  return cudaSuccess;
}

template <class T>
inline cudaError_t cudaMalloc(T** p, size_t bytes) {
  *p = static_cast<T*>(std::malloc(bytes));
  return *p != nullptr ? cudaSuccess : 2;
}

inline cudaError_t cudaFree(void* p) {
  std::free(p);
  return cudaSuccess;
}

inline cudaError_t cudaMemcpy(void* dst, const void* src, size_t bytes, cudaMemcpyKind) {
  std::memcpy(dst, src, bytes);
  return cudaSuccess;
}

// Lancement simule : boucle sequentielle sur tous les indices 1D. Le kernel
// DOIT filtrer lui-meme les indices >= n (meme contrat que sur device).
#define MHGP7_LAUNCH(kernel, blocks, threads, ...)                                             \
  do {                                                                                         \
    mhgp7_stub::grid_dim = mhgp7_stub::Dim3{(unsigned)(blocks), 1, 1};                         \
    mhgp7_stub::block_dim = mhgp7_stub::Dim3{(unsigned)(threads), 1, 1};                       \
    for (mhgp7_stub::block_idx.x = 0; mhgp7_stub::block_idx.x < (unsigned)(blocks);            \
         ++mhgp7_stub::block_idx.x)                                                            \
      for (mhgp7_stub::thread_idx.x = 0; mhgp7_stub::thread_idx.x < (unsigned)(threads);       \
           ++mhgp7_stub::thread_idx.x)                                                         \
        kernel(__VA_ARGS__);                                                                   \
  } while (0)

