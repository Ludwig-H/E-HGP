// MorseHGP3D v3 — SOURCE PAR ANCRE MAXIMALE, NOYAU DEVICE.
//
// Ce noyau n'implemente RIEN : il appelle `run_anchor_point` de
// `prototype/anchor_pipeline.hpp`, exactement la fonction que l'hote compile.
// La parite hote/device n'est donc pas un accord a esperer entre deux
// implementations, c'est la meme fonction sur deux materiels. Le differentiel
// sert a detecter une divergence de compilateur, d'arrondi entier ou de
// materiel — jamais une divergence d'algorithme.
//
// Le chronometre publie ici est un diagnostic de noyau. Il ne qualifie NI le
// SLO `warm_e2e`, NI le payload officiel : ni les dix forets, ni les
// verticales, ni les lots, ni le certificat minimal ne sont produits.
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "prototype/anchor_device.hpp"

namespace mhgp3v {
namespace anchor {

// ---------------------------------------------------------------------------
// Le puits device : un compteur global atomique et un tampon de capacite
// fixe. Un debordement leve `kOverflowOutput` et la campagne entiere est
// refusee ; aucune sortie n'est tronquee.
// ---------------------------------------------------------------------------
struct DeviceSink {
  EmittedSupport* buffer;
  unsigned long long* cursor;
  unsigned long long capacity;
  bool store;
  unsigned* flags;

  __device__ void emit(const EmittedSupport& e) {
    if (!store) {
      atomicAdd(cursor, 1ULL);
      return;
    }
    const unsigned long long slot = atomicAdd(cursor, 1ULL);
    if (slot >= capacity) {
      atomicOr(flags, kOverflowOutput);
      return;
    }
    buffer[slot] = e;
  }
};

// Reduction des compteurs : chaque fil garde les siens en registres puis les
// verse une seule fois. Aucun compteur n'est estime ni echantillonne.
__device__ void accumulate(unsigned long long* dst, const WorkCounters& w) {
  const long long* src = reinterpret_cast<const long long*>(&w);
  const int fields = (int)(sizeof(WorkCounters) / sizeof(long long));
  // Les quatre derniers champs sont des high-water : maximum, pas somme.
  for (int i = 0; i < fields - 4; ++i)
    if (src[i] != 0) atomicAdd(dst + i, (unsigned long long)src[i]);
  for (int i = fields - 4; i < fields; ++i)
    if (src[i] != 0) atomicMax(dst + i, (unsigned long long)src[i]);
}

__global__ void anchor_kernel(TreeView tree, int smax, Scratch base, int slots,
                              unsigned long long* counters, DeviceSink sink, unsigned* flags) {
  const int slot = blockIdx.x * blockDim.x + threadIdx.x;
  if (slot >= slots) return;
  Scratch sc;
  sc.partners = base.partners + (long long)slot * kPartnerCap;
  sc.site_d2 = base.site_d2 + (long long)slot * kSiteCap;
  sc.site_id = base.site_id + (long long)slot * kSiteCap;
  sc.margin_g = base.margin_g + (long long)slot * kSiteCap;
  sc.margin_llow = base.margin_llow + (long long)slot * kSiteCap;
  sc.margin_uhigh = base.margin_uhigh + (long long)slot * kSiteCap;
  sc.kept = base.kept + (long long)slot * kKeptCap;
  sc.lens = base.lens + (long long)slot * kLensCap;
  sc.acute = base.acute + (long long)slot * kLensCap;

  WorkCounters w{};
  unsigned local_flags = 0;
  for (int a = slot; a < tree.n; a += slots)
    run_anchor_point(tree, a, smax, sc, sink, &w, &local_flags);
  if (local_flags != 0) atomicOr(flags, local_flags);
  accumulate(counters, w);
}

// ---------------------------------------------------------------------------
// Enveloppe hote. Elle preflighte la memoire AVANT toute allocation et refuse
// atomiquement si la capacite manque.
// ---------------------------------------------------------------------------
#define MHGP3V_CUDA_CHECK(expr)                                                       \
  do {                                                                                \
    const cudaError_t rc_ = (expr);                                                   \
    if (rc_ != cudaSuccess) {                                                         \
      std::fprintf(stderr, "REFUS : CUDA %s a rendu %s\n", #expr,                      \
                   cudaGetErrorString(rc_));                                          \
      std::exit(3);                                                                   \
    }                                                                                 \
  } while (0)

DeviceRun run_device(const std::vector<int>& lo, const std::vector<int>& hi,
                     const std::vector<int>& begin_, const std::vector<int>& end_,
                     const std::vector<int>& left, const std::vector<int>& right,
                     const std::vector<int>& order, const std::vector<int>& px,
                     const std::vector<int>& py, const std::vector<int>& pz, int smax,
                     bool store, unsigned long long output_capacity, int slots) {
  DeviceRun run{};
  run.slots = slots;
  const int nodes = (int)begin_.size();
  const int n = (int)px.size();

  const unsigned long long per_slot =
      (unsigned long long)kPartnerCap * sizeof(int) +
      (unsigned long long)kSiteCap * (sizeof(i64) + sizeof(int) + 3 * sizeof(i64)) +
      (unsigned long long)kKeptCap * sizeof(int) +
      (unsigned long long)kLensCap * (sizeof(int) + 1);
  run.scratch_bytes = per_slot * (unsigned long long)slots;
  run.output_bytes = store ? output_capacity * sizeof(EmittedSupport) : 0ULL;

  std::size_t free_bytes = 0, total_bytes = 0;
  MHGP3V_CUDA_CHECK(cudaMemGetInfo(&free_bytes, &total_bytes));
  const unsigned long long needed =
      run.scratch_bytes + run.output_bytes +
      (unsigned long long)(nodes * 8 + n * 4) * sizeof(int) + (1ULL << 20);
  if (needed > (unsigned long long)free_bytes) {
    std::fprintf(stderr,
                 "REFUS : preflight memoire device %llu octets demandes, %zu libres\n",
                 needed, free_bytes);
    std::exit(3);
  }

  int *d_lo, *d_hi, *d_begin, *d_end, *d_left, *d_right, *d_order, *d_px, *d_py, *d_pz;
  auto upload = [](const std::vector<int>& v, int** out) {
    MHGP3V_CUDA_CHECK(cudaMalloc(out, v.size() * sizeof(int)));
    MHGP3V_CUDA_CHECK(
        cudaMemcpy(*out, v.data(), v.size() * sizeof(int), cudaMemcpyHostToDevice));
  };
  upload(lo, &d_lo);
  upload(hi, &d_hi);
  upload(begin_, &d_begin);
  upload(end_, &d_end);
  upload(left, &d_left);
  upload(right, &d_right);
  upload(order, &d_order);
  upload(px, &d_px);
  upload(py, &d_py);
  upload(pz, &d_pz);

  TreeView tree{};
  tree.lo = d_lo;
  tree.hi = d_hi;
  tree.begin = d_begin;
  tree.end = d_end;
  tree.left = d_left;
  tree.right = d_right;
  tree.order = d_order;
  tree.px = d_px;
  tree.py = d_py;
  tree.pz = d_pz;
  tree.nodes = nodes;
  tree.n = n;

  Scratch base{};
  MHGP3V_CUDA_CHECK(cudaMalloc(&base.partners, (std::size_t)slots * kPartnerCap * sizeof(int)));
  MHGP3V_CUDA_CHECK(cudaMalloc(&base.site_d2, (std::size_t)slots * kSiteCap * sizeof(i64)));
  MHGP3V_CUDA_CHECK(cudaMalloc(&base.site_id, (std::size_t)slots * kSiteCap * sizeof(int)));
  MHGP3V_CUDA_CHECK(cudaMalloc(&base.margin_g, (std::size_t)slots * kSiteCap * sizeof(i64)));
  MHGP3V_CUDA_CHECK(cudaMalloc(&base.margin_llow, (std::size_t)slots * kSiteCap * sizeof(i64)));
  MHGP3V_CUDA_CHECK(cudaMalloc(&base.margin_uhigh, (std::size_t)slots * kSiteCap * sizeof(i64)));
  MHGP3V_CUDA_CHECK(cudaMalloc(&base.kept, (std::size_t)slots * kKeptCap * sizeof(int)));
  MHGP3V_CUDA_CHECK(cudaMalloc(&base.lens, (std::size_t)slots * kLensCap * sizeof(int)));
  MHGP3V_CUDA_CHECK(cudaMalloc(&base.acute, (std::size_t)slots * kLensCap));

  const int fields = (int)(sizeof(WorkCounters) / sizeof(long long));
  unsigned long long* d_counters = nullptr;
  MHGP3V_CUDA_CHECK(cudaMalloc(&d_counters, (std::size_t)fields * sizeof(unsigned long long)));
  MHGP3V_CUDA_CHECK(cudaMemset(d_counters, 0, (std::size_t)fields * sizeof(unsigned long long)));
  unsigned* d_flags = nullptr;
  MHGP3V_CUDA_CHECK(cudaMalloc(&d_flags, sizeof(unsigned)));
  MHGP3V_CUDA_CHECK(cudaMemset(d_flags, 0, sizeof(unsigned)));
  unsigned long long* d_cursor = nullptr;
  MHGP3V_CUDA_CHECK(cudaMalloc(&d_cursor, sizeof(unsigned long long)));
  MHGP3V_CUDA_CHECK(cudaMemset(d_cursor, 0, sizeof(unsigned long long)));
  EmittedSupport* d_out = nullptr;
  if (store)
    MHGP3V_CUDA_CHECK(cudaMalloc(&d_out, (std::size_t)output_capacity * sizeof(EmittedSupport)));

  DeviceSink sink{};
  sink.buffer = d_out;
  sink.cursor = d_cursor;
  sink.capacity = output_capacity;
  sink.store = store;
  sink.flags = d_flags;

  const int block = 128;
  const int grid = (slots + block - 1) / block;
  cudaEvent_t ev0, ev1;
  MHGP3V_CUDA_CHECK(cudaEventCreate(&ev0));
  MHGP3V_CUDA_CHECK(cudaEventCreate(&ev1));
  MHGP3V_CUDA_CHECK(cudaEventRecord(ev0));
  anchor_kernel<<<grid, block>>>(tree, smax, base, slots, d_counters, sink, d_flags);
  MHGP3V_CUDA_CHECK(cudaEventRecord(ev1));
  MHGP3V_CUDA_CHECK(cudaEventSynchronize(ev1));
  MHGP3V_CUDA_CHECK(cudaGetLastError());
  float ms = 0.0f;
  MHGP3V_CUDA_CHECK(cudaEventElapsedTime(&ms, ev0, ev1));
  run.kernel_ms = (double)ms;

  std::vector<unsigned long long> host_counters((std::size_t)fields, 0ULL);
  MHGP3V_CUDA_CHECK(cudaMemcpy(host_counters.data(), d_counters,
                               (std::size_t)fields * sizeof(unsigned long long),
                               cudaMemcpyDeviceToHost));
  long long* dst = reinterpret_cast<long long*>(&run.counters);
  for (int i = 0; i < fields; ++i) dst[i] = (long long)host_counters[(std::size_t)i];
  MHGP3V_CUDA_CHECK(cudaMemcpy(&run.flags, d_flags, sizeof(unsigned), cudaMemcpyDeviceToHost));
  MHGP3V_CUDA_CHECK(
      cudaMemcpy(&run.emitted, d_cursor, sizeof(unsigned long long), cudaMemcpyDeviceToHost));
  if (store) {
    if (run.emitted > output_capacity) {
      std::fprintf(stderr, "REFUS : %llu supports pour une capacite de %llu\n", run.emitted,
                   output_capacity);
      std::exit(3);
    }
    run.supports.resize((std::size_t)run.emitted);
    MHGP3V_CUDA_CHECK(cudaMemcpy(run.supports.data(), d_out,
                                 (std::size_t)run.emitted * sizeof(EmittedSupport),
                                 cudaMemcpyDeviceToHost));
  }

  cudaFree(base.partners);
  cudaFree(base.site_d2);
  cudaFree(base.site_id);
  cudaFree(base.margin_g);
  cudaFree(base.margin_llow);
  cudaFree(base.margin_uhigh);
  cudaFree(base.kept);
  cudaFree(base.lens);
  cudaFree(base.acute);
  cudaFree(d_counters);
  cudaFree(d_flags);
  cudaFree(d_cursor);
  if (d_out != nullptr) cudaFree(d_out);
  cudaFree(d_lo);
  cudaFree(d_hi);
  cudaFree(d_begin);
  cudaFree(d_end);
  cudaFree(d_left);
  cudaFree(d_right);
  cudaFree(d_order);
  cudaFree(d_px);
  cudaFree(d_py);
  cudaFree(d_pz);
  cudaEventDestroy(ev0);
  cudaEventDestroy(ev1);
  return run;
}

}  // namespace anchor
}  // namespace mhgp3v
