// Sonde CUDA de la VM G4 pour le plan GPU de la v10 (hors produit). Mesure :
//   1. exactitude de l'arithmetique __int128 sur le device (produits i64 x i64 et comparaisons, contre l'hote) ;
//   2. debit d'une boucle de produits-comparaisons en i64 et en i128 (forme des predicats D-loc et des centres) ;
//   3. bande passante hote <-> device en memoire epinglee ;
//   4. latence de lancement d'un noyau vide.
// Sortie : une ligne JSON sur stdout. Code 0 si l'exactitude tient, 1 sinon, 2 si CUDA est indisponible.
#include <cuda_runtime.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

using i128 = __int128;
using clk = std::chrono::steady_clock;

__global__ void cmp128(const int64_t* a, const int64_t* b, const int64_t* c, const int64_t* d, int* out, size_t n) {
  const size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x;
  if (i >= n) return;
  const i128 x = i128(a[i]) * b[i], y = i128(c[i]) * d[i];
  out[i] = x < y ? -1 : (x > y ? 1 : 0);
}

// Boucle de debit : chaque fil enchaine `iters` produits et accumulations dependants (pas d'elimination possible).
__global__ void loop64(const int64_t* a, int64_t* out, size_t n, int iters) {
  const size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x;
  if (i >= n) return;
  int64_t x = a[i], acc = 0;
  for (int t = 0; t < iters; ++t) {
    acc += x * (x ^ t);
    x = (x >> 1) ^ acc;
  }
  out[i] = acc;
}

__global__ void loop128(const int64_t* a, int64_t* out, size_t n, int iters) {
  const size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x;
  if (i >= n) return;
  int64_t x = a[i];
  i128 acc = 0;
  for (int t = 0; t < iters; ++t) {
    acc += i128(x) * (x ^ t);
    x = int64_t(acc >> 7) ^ x;
  }
  out[i] = int64_t(acc) ^ int64_t(acc >> 64);
}

__global__ void empty_kernel() {}

static double secs(clk::time_point a, clk::time_point b) { return std::chrono::duration<double>(b - a).count(); }

int main() {
  int dev = 0;
  cudaDeviceProp p;
  if (cudaGetDeviceCount(&dev) != cudaSuccess || dev == 0 || cudaGetDeviceProperties(&p, 0) != cudaSuccess) {
    std::printf("{\"status\":\"no_cuda\"}\n");
    return 2;
  }
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
  cudaMalloc(&da, n * 8);
  cudaMalloc(&db, n * 8);
  cudaMalloc(&dc, n * 8);
  cudaMalloc(&dd, n * 8);
  cudaMalloc(&dout, n * 4);
  cudaMalloc(&dout64, n * 8);
  cudaMemcpy(da, a.data(), n * 8, cudaMemcpyHostToDevice);
  cudaMemcpy(db, b.data(), n * 8, cudaMemcpyHostToDevice);
  cudaMemcpy(dc, c.data(), n * 8, cudaMemcpyHostToDevice);
  cudaMemcpy(dd, d.data(), n * 8, cudaMemcpyHostToDevice);
  cmp128<<<unsigned((n + 255) / 256), 256>>>(da, db, dc, dd, dout, n);
  std::vector<int> got(n);
  cudaMemcpy(got.data(), dout, n * 4, cudaMemcpyDeviceToHost);
  size_t bad = 0, equal = 0;
  for (size_t i = 0; i < n; ++i) {
    const i128 x = i128(a[i]) * b[i], y = i128(c[i]) * d[i];
    const int want = x < y ? -1 : (x > y ? 1 : 0);
    bad += got[i] != want;
    equal += want == 0;
  }
  // 2. debit i64 et i128
  const int iters = 4096;
  const size_t m = size_t(1) << 22;
  cudaEvent_t e0, e1;
  cudaEventCreate(&e0);
  cudaEventCreate(&e1);
  float ms64 = 0, ms128 = 0;
  loop64<<<unsigned((m + 255) / 256), 256>>>(da, dout64, m, 16);  // chauffe
  cudaEventRecord(e0);
  loop64<<<unsigned((m + 255) / 256), 256>>>(da, dout64, m, iters);
  cudaEventRecord(e1);
  cudaEventSynchronize(e1);
  cudaEventElapsedTime(&ms64, e0, e1);
  cudaEventRecord(e0);
  loop128<<<unsigned((m + 255) / 256), 256>>>(da, dout64, m, iters);
  cudaEventRecord(e1);
  cudaEventSynchronize(e1);
  cudaEventElapsedTime(&ms128, e0, e1);
  // 3. bande passante en memoire epinglee (256 Mio)
  const size_t bytes = size_t(256) << 20;
  void* host = nullptr;
  cudaMallocHost(&host, bytes);
  void* devbuf = nullptr;
  cudaMalloc(&devbuf, bytes);
  cudaMemcpy(devbuf, host, bytes, cudaMemcpyHostToDevice);
  auto t0 = clk::now();
  for (int r = 0; r < 4; ++r) cudaMemcpy(devbuf, host, bytes, cudaMemcpyHostToDevice);
  auto t1 = clk::now();
  for (int r = 0; r < 4; ++r) cudaMemcpy(host, devbuf, bytes, cudaMemcpyDeviceToHost);
  auto t2 = clk::now();
  // 4. latence de lancement
  empty_kernel<<<1, 1>>>();
  cudaDeviceSynchronize();
  auto t3 = clk::now();
  for (int r = 0; r < 1000; ++r) empty_kernel<<<1, 1>>>();
  cudaDeviceSynchronize();
  auto t4 = clk::now();
  const cudaError_t err = cudaGetLastError();
  const double ops = double(m) * iters;
  std::printf("{\"status\":\"%s\",\"device\":\"%s\",\"cc\":\"%d.%d\",\"sms\":%d,\"global_mem_gib\":%.1f,"
              "\"cuda_error\":\"%s\",\"i128_pairs\":%zu,\"i128_equal_cases\":%zu,\"i128_mismatches\":%zu,"
              "\"loop64_gops\":%.1f,\"loop128_gops\":%.1f,\"i128_over_i64\":%.2f,\"h2d_gbps\":%.1f,\"d2h_gbps\":%.1f,"
              "\"launch_us\":%.2f}\n",
              bad == 0 ? "ok" : "i128_mismatch", p.name, p.major, p.minor, p.multiProcessorCount,
              double(p.totalGlobalMem) / double(1ull << 30), cudaGetErrorString(err), n, equal, bad,
              ops / (ms64 * 1e-3) * 1e-9, ops / (ms128 * 1e-3) * 1e-9, double(ms128) / double(ms64),
              4.0 * bytes / secs(t0, t1) * 1e-9, 4.0 * bytes / secs(t1, t2) * 1e-9, secs(t3, t4) * 1e6 / 1000.0);
  return bad == 0 && err == cudaSuccess ? 0 : 1;
}
