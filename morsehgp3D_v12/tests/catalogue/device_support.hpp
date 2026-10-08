// Outils des portes de la voie appareil et de la fin d'etage partagee (tranche T1-b) : nuages temoins, voie CPU,
// voie appareil jouee sur l'hote par l'executeur Pool (meme code que l'appareil), comparaison de deux catalogues ;
// executeur a transit simule (tranche T2-d) : les sorties en flux suivent la sequence de l'executeur CUDA.
#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <vector>

#include "catalogue/device_pipeline.hpp"
#include "catalogue/exec_host.hpp"
#include "catalogue/finish_slices.hpp"
#include "sched/sched.hpp"

namespace mhgp12::device_test {

using catalogue_detail::PoolExecutor;
using Points = std::vector<std::array<u32, 3>>;

inline Result<Cloud> make_cloud(const Points& pts, MemoryBudget& budget) {
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  for (std::size_t i = 0; i < pts.size(); ++i) {
    x.push_back(pts[i][0]);
    y.push_back(pts[i][1]);
    z.push_back(pts[i][2]);
    ids.push_back(make_id<PointId>(static_cast<u32>(i)));
  }
  return prepare_cloud(x, y, z, ids, CoordWidth(), budget);
}

// Generateur deterministe (SplitMix64).
struct Mix {
  u64 state;
  u64 next() {
    state += 0x9E3779B97F4A7C15ull;
    u64 z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
  }
  u32 below(u64 bound) { return static_cast<u32>(next() % bound); }
};

// n points distincts de [0, side]^3.
inline Points scatter(u32 n, u32 side, u64 seed) {
  Mix mix{seed};
  Points out;
  while (out.size() < n) {
    const std::array<u32, 3> p{mix.below(u64{side} + 1), mix.below(u64{side} + 1), mix.below(u64{side} + 1)};
    if (std::find(out.begin(), out.end(), p) == out.end()) out.push_back(p);
  }
  return out;
}

// Coquille : permutations signees de base, a l'echelle, autour de offset (fixture de MES-M5).
inline Points shell(std::array<u32, 3> base, u32 scale, u32 offset) {
  Points out;
  std::array<u32, 3> p = base;
  std::sort(p.begin(), p.end());
  do {
    for (int signs = 0; signs < 8; ++signs) {
      std::array<u32, 3> q{};
      for (int a = 0; a < 3; ++a) q[a] = ((signs >> a) & 1) ? offset + p[a] * scale : offset - p[a] * scale;
      out.push_back(q);
    }
  } while (std::next_permutation(p.begin(), p.end()));
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

inline CatalogueParams params_of(int k, u32 leaf) {
  CatalogueParams p;
  p.kmax = k;
  p.leaf_size = leaf;
  return p;
}

inline Result<Catalogue> cpu(const Cloud& cloud, const CatalogueParams& params, u32 threads, MemoryBudget& budget,
                             CatalogueDiagnostics* diagnostics = nullptr) {
  auto pool = sched::make_pool({threads});
  if (!pool.ok()) return pool.outcome();
  return build_catalogue(cloud, params, budget, *pool.value(), diagnostics);
}

// Voie appareil jouee par l'executeur Pool ; etat garde par l'appelant (regime resident) ou neuf.
inline Result<Catalogue> host_device(const Cloud& cloud, const CatalogueParams& params, sched::Pool& pool,
                                     MemoryBudget& budget, catalogue_detail::dev::DeviceState<PoolExecutor>& state,
                                     CatalogueDiagnostics* diagnostics = nullptr) {
  PoolExecutor executor{pool, budget};
  CatalogueDiagnostics diag;
  auto out = guarded([&]() {
    return catalogue_detail::dev::device_catalogue(executor, state, cloud, params, budget, pool, diag);
  });
  if (out.ok() && diagnostics != nullptr) *diagnostics = diag;
  return out;
}

// Executeur Pool a transit simule (tranche T2-d), comptes de l'executeur CUDA : les sorties en flux passent par une
// memoire de transit de l'hote en petites cases, par la MEME sequence que l'executeur CUDA (stream_staged) ; une copie
// lancee n'est faite qu'a l'attente de sa case, et la case est d'abord empoisonnee (0xA5) : une tranche consommee
// avant son attente, ou une case relancee avant la consommation de sa tranche, rend un catalogue faux. Budgets comme
// CatalogueDevice::open(budget, device) : tableaux de l'executeur dans le budget de l'executeur (PoolExecutor::budget,
// l'appareil), memoire de transit dans `host` avec les demandes de CudaExecutor::stage (chaque copie : min(64 Mio,
// copie), au moins 1 Mio ; chaque flux : stream_staging_bytes ; maximum garde jusqu'a la destruction) ; aucune
// sortie adoptee sans copie, comme sur l'appareil.
struct StagedExecutor : PoolExecutor {
  MemoryBudget* host = nullptr;
  BudgetReservation staging_reservation{};
  u64 staging_bytes = 0;
  std::vector<u8> staging{};
  u64 slot_size = 256, slot_count = 2;
  struct Pending {
    const u8* from = nullptr;
    u64 bytes = 0;
  };
  Pending pending[catalogue_detail::kMaxSlots] = {};

  // Comptes de CudaExecutor::stage : l'ancienne memoire est rendue avant la nouvelle reservation.
  Outcome stage(u64 bytes) noexcept {
    constexpr u64 kMin = u64{1} << 20, kMax = u64{64} << 20;
    bytes = bytes < kMin ? kMin : bytes < kMax ? bytes : kMax;
    if (host == nullptr || staging_bytes >= bytes) return {};
    staging_reservation.reset();
    staging_bytes = 0;
    MHGP12_TRY(staging_reservation.reserve(bytes, *host));
    staging_bytes = bytes;
    return {};
  }
  template <class A, class T>
  bool adopt(A&, Buffer<T>&, u64) noexcept {
    return false;
  }
  template <class T>
  Outcome upload(catalogue_detail::FrontArray<T>& a, const T* from, u64 n, u64 at) noexcept {
    MHGP12_TRY(stage(n * sizeof(T)));
    return PoolExecutor::upload(a, from, n, at);
  }
  template <class T>
  Outcome download(T* to, catalogue_detail::FrontArray<T>& a, u64 n, u64 at) noexcept {
    MHGP12_TRY(stage(n * sizeof(T)));
    return PoolExecutor::download(to, a, n, at);
  }
  template <class T>
  Outcome put(catalogue_detail::FrontArray<T>& a, const T& value, u64 at) noexcept {
    return upload(a, &value, 1, at);
  }
  template <class T>
  Result<T> read(catalogue_detail::FrontArray<T>& a, u64 at) noexcept {
    T value{};
    MHGP12_TRY(download(&value, a, 1, at));
    return value;
  }
  Result<catalogue_detail::bfs::LevelTotals> read_totals(
      catalogue_detail::FrontArray<catalogue_detail::bfs::LevelTotals>& a) noexcept {
    return read(a, 0);
  }
  u64 slots() const noexcept { return slot_count; }
  u64 slot_bytes() const noexcept { return slot_size; }
  u8* slot(u64 c) noexcept { return staging.data() + c * slot_size; }
  Outcome issue(u64 c, const u8* from, u64 bytes) noexcept {
    std::fill(slot(c), slot(c) + slot_size, u8{0xA5});
    pending[c] = Pending{from, bytes};
    return {};
  }
  Outcome wait(u64 c) noexcept {
    if (pending[c].from != nullptr) std::memcpy(slot(c), pending[c].from, pending[c].bytes);
    pending[c] = Pending{};
    return {};
  }
  Outcome stream_out(std::span<const catalogue_detail::StreamSegment> segments) noexcept {
    u64 bytes = 0;
    for (const auto& s : segments) bytes += s.n * s.elem;
    if (bytes == 0) return {};
    MHGP12_TRY(stage(catalogue_detail::stream_staging_bytes(segments, slot_count)));
    staging.assign(slot_size * slot_count, u8{0});
    u64 chunks = 0;
    const auto published = catalogue_detail::stream_staged(*this, segments, chunks);
    if (!published.ok()) return published.outcome();
    meter.publish_ns += published.value();
    meter.chunks += chunks;
    ++meter.ops;
    return {};
  }
};

// Voie appareil jouee sur l'executeur a transit simule (cases de slot_size octets, slot_count cases) : tableaux de
// l'executeur dans `device`, memoire de transit, sorties et reprises de l'hote dans `host` ; etat et executeur neufs,
// detruits au retour : il ne reste alors dans `host` que le catalogue rendu, et rien dans `device`.
inline Result<Catalogue> staged_device(const Cloud& cloud, const CatalogueParams& params, sched::Pool& pool,
                                       MemoryBudget& host, MemoryBudget& device, u64 slot_size, u64 slot_count,
                                       CatalogueDiagnostics* diagnostics = nullptr) {
  StagedExecutor executor{{pool, device}};
  executor.host = &host;
  executor.slot_size = slot_size;
  executor.slot_count = slot_count;
  catalogue_detail::dev::DeviceState<StagedExecutor> state;
  CatalogueDiagnostics diag;
  auto out = guarded([&]() {
    return catalogue_detail::dev::device_catalogue(executor, state, cloud, params, host, pool, diag);
  });
  if (out.ok() && diagnostics != nullptr) *diagnostics = diag;
  return out;
}

inline Result<Catalogue> staged_device(const Cloud& cloud, const CatalogueParams& params, sched::Pool& pool,
                                       MemoryBudget& budget, u64 slot_size, u64 slot_count,
                                       CatalogueDiagnostics* diagnostics = nullptr) {
  return staged_device(cloud, params, pool, budget, budget, slot_size, slot_count, diagnostics);
}

// Arene de la voie CPU (lots de LeafStage) et grand livre, sans fin d'etage (tranche T1-d).
struct LeafArena {
  std::vector<catalogue_detail::Chunk> chunks;
  u64 balls = 0;
  CatalogueLedger ledger;
};

inline Outcome leaf_arena(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget, sched::Pool& pool,
                          LeafArena& out) {
  catalogue_detail::LeafStage stage(cloud, params, budget, pool);
  const catalogue_detail::bfs::Params traversal{static_cast<u32>(params.kmax), params.leaf_size, params.max_leaf,
                                                static_cast<u32>(kCoordBits)};
  catalogue_detail::TraversalLedger walked;
  catalogue_detail::TraversalDiagnostics front;
  MHGP12_TRY(catalogue_detail::traverse(cloud, traversal, budget, pool, stage, walked, front));
  out.chunks = std::move(stage.chunks());
  out.balls = stage.balls();
  out.ledger = catalogue_detail::make_ledger(walked, stage.totals().counts);
  return {};
}

// Memoire de travail de toutes les boules de l'arene, au tarif d'une tranche (kSliceBytesPerBall, par incidence).
inline u64 arena_slice_bytes(const LeafArena& a) {
  u64 total = 0;
  for (const auto& c : a.chunks)
    for (const auto& r : c.records)
      total += catalogue_detail::fin::kSliceBytesPerBall +
               catalogue_detail::fin::kSliceBytesPerIncidence * (u64{r.p} + r.m);
  return total;
}

// Voie par tranches de cles sur l'executeur Pool (memoire de travail slice_bytes), publiee comme la voie complete ;
// slices : nombre de tranches.
inline Result<Catalogue> sliced_catalogue(const Cloud& cloud, LeafArena& arena, int kmax, u64 slice_bytes,
                                          MemoryBudget& budget, sched::Pool& pool, u64* slices = nullptr) {
  namespace fin = catalogue_detail::fin;
  PoolExecutor executor{pool, budget};
  fin::FinishArrays<PoolExecutor> arrays;
  fin::SliceArrays<PoolExecutor> sa;
  Buffer<u64> ball_at, incidence_at;
  fin::ArenaView view;
  MHGP12_TRY(fin::arena_bases(arena.chunks, ball_at, incidence_at, view, budget));
  const fin::SliceInput in{cloud.x().data(), cloud.y().data(), cloud.z().data(), cloud.sites(), view, &arena.chunks};
  fin::FinishOutput out;
  fin::FinishStats stats;
  MHGP12_TRY(fin::finish_sliced(executor, arrays, sa, in, out, slice_bytes, budget, pool, stats));
  if (slices != nullptr) *slices = stats.slices;
  return catalogue_detail::Assembly::adopt(out, static_cast<Order>(kmax), arena.ledger);
}

// Deux catalogues identiques : meme export (empreinte), meme grand livre, memes niveaux (numerateurs et
// denominateurs non reduits) et meme table (recherche de chaque S*).
inline bool same(const Cloud& cloud, const Catalogue& a, const Catalogue& b) {
  const auto da = catalogue_digest(cloud, a, "identite"), db = catalogue_digest(cloud, b, "identite");
  if (!da.ok() || !db.ok() || !(da.value() == db.value()) || !(a.ledger() == b.ledger())) return false;
  if (a.levels().size() != b.levels().size()) return false;
  for (u64 r = 0; r < a.levels().size(); ++r) {
    const auto& la = a.levels()[r];
    const auto& lb = b.levels()[r];
    if (!(num::to_wide(la.numerator()).words == num::to_wide(lb.numerator()).words) ||
        !(num::to_wide(la.denominator()).words == num::to_wide(lb.denominator()).words))
      return false;
  }
  for (u32 i = 0; i < a.balls(); ++i) {
    const auto& ball = a.balls_data()[i];
    const auto found = b.find_support(std::span<const SiteIdx>(ball.support.data(), ball.qmin));
    if (!found.has_value() || idx(*found) != i) return false;
  }
  return true;
}

}  // namespace mhgp12::device_test
