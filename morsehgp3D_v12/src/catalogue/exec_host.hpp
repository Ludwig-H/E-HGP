// Executeur hote des noyaux de warps du catalogue (voie CPU de reference) : un noyau de warps = une boucle sur ses
// warps, repartis sur le Pool par tranches fixes ; warp simule (simt.hpp), memoire partagee du warp sur la pile de la
// tranche. Chaque warp n'ecrit que ses sorties (les seuls atomiques sont les OU des mots de fautes), donc le resultat
// ne depend ni du nombre de fils ni de leur ordonnancement. Tableaux : Buffer du budget de l'appel (FrontArray).
// Meme interface que l'executeur CUDA de la voie appareil (device_cuda.cu) : ensure, ensure_keep, put, read, upload,
// download, adopt, stream_out, launch, meter ; le pilote du parcours (traversal_driver.hpp), la fin d'etage
// (finish_driver.hpp) et les lots de feuilles de la voie appareil (device_driver.hpp) sont ecrits une fois pour
// les deux.
#pragma once

#include <cstring>

#include "catalogue/transfer_meter.hpp"
#include "catalogue/traversal.hpp"

namespace mhgp12::catalogue_detail {

template <class K>
struct HostLaunch {
  const K* kernel;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    const K& k = *static_cast<const HostLaunch*>(context)->kernel;
    typename K::Shared shared;
    for (u64 w = begin; w < end; ++w) k(w, shared);
    return {};
  }
};

// Copie parallele par blocs de 2^16 elements.
template <class T>
struct HostCopy {
  T* to;
  const T* from;
  static constexpr u64 kBlock = u64{1} << 16;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    const auto& c = *static_cast<const HostCopy*>(context);
    for (u64 block = begin; block < end; ++block)
      std::memcpy(c.to + block * kBlock, c.from + block * kBlock, kBlock * sizeof(T));
    return {};
  }
};

struct PoolExecutor {
  sched::Pool& pool;
  MemoryBudget& budget;
  TransferMeter meter{};
  template <class T>
  using Array = FrontArray<T>;

  template <class T>
  Outcome ensure(FrontArray<T>& a, u64 n) noexcept {
    return a.ensure(n, budget);
  }
  template <class T>
  Outcome ensure_keep(FrontArray<T>& a, u64 n, u64 keep) noexcept {
    return a.ensure_keep(n, keep, budget);
  }
  template <class T>
  Outcome put(FrontArray<T>& a, const T& value, u64 at) noexcept {
    const Stopwatch watch;
    a.data()[at] = value;
    meter.note(watch, sizeof(T), 0);
    return {};
  }
  template <class T>
  Result<T> read(FrontArray<T>& a, u64 at) noexcept {
    const Stopwatch watch;
    const T value = a.data()[at];
    meter.note(watch, 0, sizeof(T));
    return value;
  }
  Result<bfs::LevelTotals> read_totals(FrontArray<bfs::LevelTotals>& a) noexcept { return read(a, 0); }
  template <class T>
  Outcome copy(T* to, const T* from, u64 n) noexcept {
    if (n == 0 || to == from) return {};
    const u64 blocks = n / HostCopy<T>::kBlock, done = blocks * HostCopy<T>::kBlock;
    HostCopy<T> context{to, from};
    if (blocks > 1 && pool.size() > 1) MHGP12_TRY(pool.parallel_for(blocks, 1, &context, &HostCopy<T>::body));
    else if (blocks != 0) MHGP12_TRY(HostCopy<T>::body(&context, 0, blocks, 0));
    std::memcpy(to + done, from + done, (n - done) * sizeof(T));
    return {};
  }
  template <class T>
  Outcome upload(FrontArray<T>& a, const T* from, u64 n, u64 at) noexcept {
    const Stopwatch watch;
    MHGP12_TRY(copy(a.data() + at, from, n));
    meter.note(watch, n * sizeof(T), 0);
    return {};
  }
  template <class T>
  Outcome download(T* to, FrontArray<T>& a, u64 n, u64 at) noexcept {
    const Stopwatch watch;
    MHGP12_TRY(copy(to, a.data() + at, n));
    meter.note(watch, 0, n * sizeof(T));
    return {};
  }
  // Sortie adoptee sans copie : le tableau lui-meme s'il a exactement n elements du type de la sortie (le tableau
  // devient vide) ; sinon la sortie passe par le flux.
  template <class T>
  bool adopt(FrontArray<T>& a, Buffer<T>& out, u64 n) noexcept {
    if (!out.empty() || a.buffer.size() != n || n == 0) return false;
    out.swap(a.buffer);
    a.buffer.reset();
    return true;
  }
  template <class A, class T>
  bool adopt(A&, Buffer<T>&, u64) noexcept {
    return false;
  }
  // Flux de sortie (transfer_meter.hpp) : la memoire de l'executeur est celle de l'hote, chaque segment est remis
  // entier a son consommateur ; une operation et une tranche par segment, duree en transfert ou en publication.
  Outcome stream_out(std::span<const StreamSegment> segments) noexcept {
    for (const StreamSegment& s : segments) {
      if (s.n == 0) continue;
      const Stopwatch watch;
      MHGP12_TRY(s.consume(s.context, s.source, 0, s.n));
      (s.publication ? meter.publish_ns : meter.ns) += watch.nanoseconds();
      meter.d2h_bytes += s.n * s.elem;
      ++meter.ops;
      ++meter.chunks;
    }
    return {};
  }
  template <class K>
  Outcome launch(const K& kernel, u64 warps) noexcept {
    HostLaunch<K> context{&kernel};
    if (warps <= 8 || pool.size() == 1) return HostLaunch<K>::body(&context, 0, warps, 0);
    const u64 grain = warps / (8 * u64{pool.size()}) + 1;
    return pool.parallel_for(warps, grain, &context, &HostLaunch<K>::body);
  }
  Outcome sync() noexcept { return {}; }
};

}  // namespace mhgp12::catalogue_detail
