// Sonde CUDA de la VM G4 pour le plan GPU de la v10 (hors produit). Mesure :
//   1. exactitude de l'arithmetique __int128 sur le device (produits i64 x i64 et comparaisons, contre l'hote) ;
//   2. debit de chaines de produits-accumulations MODULAIRES (non signees, arithmetique definie) sur 64 et 128 bits :
//      ordre de grandeur de cout, pas un debit de predicats exacts (correction du 29 septembre 2026 : la version
//      precedente debordait en entiers signes, comportement indefini) ;
//   3. bande passante hote <-> device en memoire epinglee (transfert seul) ;
//   4. latence d'un lancement de noyau vide suivi de sa synchronisation, et debit de lancements asynchrones.
// Chaque appel CUDA est verifie ; la premiere erreur arrete la sonde (code 3).
// Sortie : une seule ligne JSON sur stdout, dont le champ status correspond au code de sortie :
//   0 ok, 1 i128_mismatch (ecart d'exactitude), 2 no_cuda (aucun peripherique), 3 cuda_error (appel CUDA en echec, y
//   compris la lecture des proprietes d'un peripherique present), 5 timing_invalid (duree mesuree nulle, negative ou
//   non finie : aucun debit n'est publie).
#include <cuda_runtime.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

using i128 = __int128;
using u128 = unsigned __int128;
using clk = std::chrono::steady_clock;

#define CK(call)                                                                                          \
  do {                                                                                                    \
    const cudaError_t e_ = (call);                                                                        \
    if (e_ != cudaSuccess) {                                                                              \
      std::printf("{\"status\":\"cuda_error\",\"call\":\"%s\",\"error\":\"%s\"}\n", #call, cudaGetErrorString(e_)); \
      return 3;                                                                                           \
    }                                                                                                     \
  } while (0)

__global__ void cmp128(const int64_t* a, const int64_t* b, const int64_t* c, const int64_t* d, int* out, size_t n) {
  const size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x;
  if (i >= n) return;
  const i128 x = i128(a[i]) * b[i], y = i128(c[i]) * d[i];
  out[i] = x < y ? -1 : (x > y ? 1 : 0);
}

// Boucles de debit MODULAIRES : chaque fil enchaine `iters` produits et accumulations dependants en arithmetique non
// signee (debordement defini, modulo 2^64 ou 2^128). Ordre de grandeur du cout d'une multiplication-accumulation
// 64 x 64 -> 64 et 64 x 64 -> 128, pas un debit de predicats exacts.
__global__ void loop64(const int64_t* a, int64_t* out, size_t n, int iters) {
  const size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x;
  if (i >= n) return;
  uint64_t x = uint64_t(a[i]), acc = 0;
  for (int t = 0; t < iters; ++t) {
    acc += x * (x ^ uint64_t(t));
    x = (x >> 1) ^ acc;
  }
  out[i] = int64_t(acc);
}

__global__ void loop128(const int64_t* a, int64_t* out, size_t n, int iters) {
  const size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x;
  if (i >= n) return;
  uint64_t x = uint64_t(a[i]);
  u128 acc = 0;
  for (int t = 0; t < iters; ++t) {
    acc += u128(x) * (x ^ uint64_t(t));
    x = uint64_t(acc >> 7) ^ x;
  }
  out[i] = int64_t(uint64_t(acc) ^ uint64_t(acc >> 64));
}

__global__ void empty_kernel() {}

static double secs(clk::time_point a, clk::time_point b) { return std::chrono::duration<double>(b - a).count(); }

int main() {
  int dev = 0;
  if (cudaGetDeviceCount(&dev) != cudaSuccess || dev == 0) {
    std::printf("{\"status\":\"no_cuda\"}\n");
    return 2;
  }
  cudaDeviceProp p;
  CK(cudaGetDeviceProperties(&p, 0));  // peripherique present mais illisible : cuda_error, code 3
  // 1. exactitude i128 sur 16 M paires aleatoires, bornes pleines i64 comprises
  const size_t n = size_t(1) << 24;
  std::mt19937_64 rng(20260929);
  std::vector<int64_t> a(n), b(n), c(n), d(n);
  for (size_t i = 0; i < n; ++i) {
    a[i] = int64_t(rng());
    b[i] = int64_t(rng());
    c[i] = (i % 7 == 0) ? b[i] : int64_t(rng());  // egalites forcees : a*b == c*d quand c = b et d = a
    d[i] = (i % 7 == 0) ? a[i] : int64_t(rng());
    if (i % 11 == 0) a[i] = INT64_MIN;
    if (i % 13 == 0) b[i] = INT64_MAX;
  }
  int64_t *da, *db, *dc, *dd, *dout64;
  int* dout;
  CK(cudaMalloc(&da, n * 8));
  CK(cudaMalloc(&db, n * 8));
  CK(cudaMalloc(&dc, n * 8));
  CK(cudaMalloc(&dd, n * 8));
  CK(cudaMalloc(&dout, n * 4));
  CK(cudaMalloc(&dout64, n * 8));
  CK(cudaMemcpy(da, a.data(), n * 8, cudaMemcpyHostToDevice));
  CK(cudaMemcpy(db, b.data(), n * 8, cudaMemcpyHostToDevice));
  CK(cudaMemcpy(dc, c.data(), n * 8, cudaMemcpyHostToDevice));
  CK(cudaMemcpy(dd, d.data(), n * 8, cudaMemcpyHostToDevice));
  cmp128<<<unsigned((n + 255) / 256), 256>>>(da, db, dc, dd, dout, n);
  CK(cudaGetLastError());
  CK(cudaDeviceSynchronize());
  std::vector<int> got(n);
  CK(cudaMemcpy(got.data(), dout, n * 4, cudaMemcpyDeviceToHost));
  size_t bad = 0, equal = 0;
  for (size_t i = 0; i < n; ++i) {
    const i128 x = i128(a[i]) * b[i], y = i128(c[i]) * d[i];
    const int want = x < y ? -1 : (x > y ? 1 : 0);
    bad += got[i] != want;
    equal += want == 0;
  }
  // 2. debit modulaire 64 et 128 bits (non signe : pas de debordement indefini)
  const int iters = 4096;
  const size_t m = size_t(1) << 22;
  cudaEvent_t e0, e1;
  CK(cudaEventCreate(&e0));
  CK(cudaEventCreate(&e1));
  float ms64 = 0, ms128 = 0;
  loop64<<<unsigned((m + 255) / 256), 256>>>(da, dout64, m, 16);  // chauffe
  CK(cudaGetLastError());
  CK(cudaEventRecord(e0));
  loop64<<<unsigned((m + 255) / 256), 256>>>(da, dout64, m, iters);
  CK(cudaGetLastError());
  CK(cudaEventRecord(e1));
  CK(cudaEventSynchronize(e1));
  CK(cudaEventElapsedTime(&ms64, e0, e1));
  CK(cudaEventRecord(e0));
  loop128<<<unsigned((m + 255) / 256), 256>>>(da, dout64, m, iters);
  CK(cudaGetLastError());
  CK(cudaEventRecord(e1));
  CK(cudaEventSynchronize(e1));
  CK(cudaEventElapsedTime(&ms128, e0, e1));
  // 3. bande passante en memoire epinglee (256 Mio)
  const size_t bytes = size_t(256) << 20;
  void* host = nullptr;
  CK(cudaMallocHost(&host, bytes));
  void* devbuf = nullptr;
  CK(cudaMalloc(&devbuf, bytes));
  CK(cudaMemcpy(devbuf, host, bytes, cudaMemcpyHostToDevice));
  auto t0 = clk::now();
  for (int r = 0; r < 4; ++r) CK(cudaMemcpy(devbuf, host, bytes, cudaMemcpyHostToDevice));
  auto t1 = clk::now();
  for (int r = 0; r < 4; ++r) CK(cudaMemcpy(host, devbuf, bytes, cudaMemcpyDeviceToHost));
  auto t2 = clk::now();
  // 4. latence : lancement + synchronisation, un par un ; puis debit de lancements asynchrones
  empty_kernel<<<1, 1>>>();
  CK(cudaDeviceSynchronize());
  auto t3 = clk::now();
  for (int r = 0; r < 1000; ++r) {
    empty_kernel<<<1, 1>>>();
    CK(cudaDeviceSynchronize());
  }
  auto t4 = clk::now();
  for (int r = 0; r < 1000; ++r) empty_kernel<<<1, 1>>>();
  CK(cudaDeviceSynchronize());
  auto t5 = clk::now();
  const cudaError_t err = cudaGetLastError();
  // Durees : toutes strictement positives et finies, sinon aucun debit n'est publie (status timing_invalid, code 5).
  const double w_h2d = secs(t0, t1), w_d2h = secs(t1, t2), w_sync = secs(t3, t4), w_async = secs(t4, t5);
  const bool timing_ok = ms64 > 0 && ms128 > 0 && std::isfinite(ms64) && std::isfinite(ms128) && w_h2d > 0 &&
                         w_d2h > 0 && w_sync > 0 && w_async > 0 && std::isfinite(w_h2d) && std::isfinite(w_d2h) &&
                         std::isfinite(w_sync) && std::isfinite(w_async);
  const int code = bad != 0 ? 1 : (err != cudaSuccess ? 3 : (timing_ok ? 0 : 5));
  const char* status = code == 1 ? "i128_mismatch" : (code == 3 ? "cuda_error" : (code == 5 ? "timing_invalid" : "ok"));
  const double ops = double(m) * iters;
  const double z = 0.0;  // debits non publies (0) si une duree est invalide
  std::printf("{\"status\":\"%s\",\"device\":\"%s\",\"cc\":\"%d.%d\",\"sms\":%d,\"global_mem_gib\":%.1f,"
              "\"cuda_error\":\"%s\",\"i128_pairs\":%zu,\"i128_equal_cases\":%zu,\"i128_mismatches\":%zu,"
              "\"timing_ok\":%s,\"loop64_gops\":%.1f,\"loop128_gops\":%.1f,\"i128_over_i64\":%.2f,\"h2d_gbps\":%.1f,"
              "\"d2h_gbps\":%.1f,\"launch_sync_us\":%.2f,\"launch_async_us\":%.2f,\"loops\":\"modular_unsigned\"}\n",
              status, p.name, p.major, p.minor, p.multiProcessorCount, double(p.totalGlobalMem) / double(1ull << 30),
              cudaGetErrorString(err), n, equal, bad, timing_ok ? "true" : "false",
              timing_ok ? ops / (ms64 * 1e-3) * 1e-9 : z, timing_ok ? ops / (ms128 * 1e-3) * 1e-9 : z,
              timing_ok ? double(ms128) / double(ms64) : z, timing_ok ? 4.0 * bytes / w_h2d * 1e-9 : z,
              timing_ok ? 4.0 * bytes / w_d2h * 1e-9 : z, timing_ok ? w_sync * 1e6 / 1000.0 : z,
              timing_ok ? w_async * 1e6 / 1000.0 : z);
  return code;
}
