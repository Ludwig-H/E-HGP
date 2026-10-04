// Contexte CUDA de la voie GPU des feuilles : ouverture anticipee (un fil par processus, rejoint par l'executeur ou a
// la sortie du processus) et pool memoire du peripherique (memoire gardee entre passes, pics physiques par lot).
#include "catalogue/leaf_batch.hpp"
#include "catalogue/leaf_batch_context.hpp"

#include <cuda_runtime.h>

#include <chrono>
#include <mutex>
#include <thread>

namespace mhgp11::catalogue_detail {
namespace {

u64 now_ns() {
  return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::steady_clock::now().time_since_epoch()).count());
}

bool ok(cudaError_t e) { return e == cudaSuccess; }

// Ouverture anticipee du contexte : un fil par processus, rejoint par l'executeur ou a la sortie du processus.
struct Prefetch {
  std::mutex mutex;
  std::thread thread;
  bool started = false;
  u64 ns = 0;
  ~Prefetch() {
    if (thread.joinable()) thread.join();
  }
};

Prefetch& prefetch_state() {
  static Prefetch state;
  return state;
}

// Le pool du peripherique garde toute memoire rendue (seuil de liberation maximal) : les passes suivantes la
// reprennent. Idempotent.
// Les pics UsedMemHigh et ReservedMemHigh du pool sont remis au niveau courant a chaque lot : ils mesurent alors la
// memoire physique du lot, distincte du compte logique (budget) et de device_bytes (cumul des allocations).
Outcome device_pool(cudaMemPool_t& pool) noexcept {
  int device = 0;
  if (!ok(cudaGetDevice(&device)) || !ok(cudaDeviceGetDefaultMemPool(&pool, device)))
    return fail(Reason::parameter_out_of_range);
  return {};
}

}  // namespace

Outcome keep_pool_memory() noexcept {
  cudaMemPool_t pool;
  MHGP11_TRY(device_pool(pool));
  u64 keep = ~u64{0}, reset = 0, reset_reserved = 0;
  if (!ok(cudaMemPoolSetAttribute(pool, cudaMemPoolAttrReleaseThreshold, &keep)) ||
      !ok(cudaMemPoolSetAttribute(pool, cudaMemPoolAttrUsedMemHigh, &reset)) ||
      !ok(cudaMemPoolSetAttribute(pool, cudaMemPoolAttrReservedMemHigh, &reset_reserved)))
    return fail(Reason::parameter_out_of_range);
  return {};
}

Outcome pool_highs(u64& used_high, u64& reserved_high) noexcept {
  cudaMemPool_t pool;
  MHGP11_TRY(device_pool(pool));
  if (!ok(cudaMemPoolGetAttribute(pool, cudaMemPoolAttrUsedMemHigh, &used_high)) ||
      !ok(cudaMemPoolGetAttribute(pool, cudaMemPoolAttrReservedMemHigh, &reserved_high)))
    return fail(Reason::parameter_out_of_range);
  return {};
}

// Attend l'ouverture anticipee si elle a eu lieu ; rend sa duree (0 sinon).
u64 join_prefetch() noexcept {
  Prefetch& p = prefetch_state();
  std::lock_guard<std::mutex> lock(p.mutex);
  if (p.thread.joinable()) p.thread.join();
  return p.ns;
}


void prefetch_cuda_context() noexcept {
  Prefetch& p = prefetch_state();
  std::lock_guard<std::mutex> lock(p.mutex);
  if (p.started) return;
  p.started = true;
  try {
    p.thread = std::thread([&p] {
      const u64 begin = now_ns();
      static_cast<void>(cudaFree(nullptr));  // l'executeur refait l'appel et juge son statut
      p.ns = now_ns() - begin;
    });
  } catch (...) {
    // Sans fil disponible, le contexte s'ouvre au premier lot, comme sans ouverture anticipee.
  }
}

bool cuda_leaf_batch_available() noexcept {
  int devices = 0;
  return cudaGetDeviceCount(&devices) == cudaSuccess && devices > 0;
}

}  // namespace mhgp11::catalogue_detail
