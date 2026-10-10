// Fin d'etage par tranches de cles, parties de l'hote (tranche T1-d, slices.hpp) : bases de l'arene, plan des tranches
// (histogrammes par cases de bits, coupes certaines, raffinement), repartition, rassemblement, table S* -> boule et
// niveaux finaux. Resultats independants du nombre de fils : sommes, minima et maxima par ouvrier fusionnes, positions
// fixees par des prefixes.
#include <algorithm>
#include <cstring>

#include "catalogue/finish_kernels.hpp"
#include "catalogue/finish_outputs.hpp"
#include "catalogue/slices.hpp"

namespace mhgp12::catalogue_detail::fin {
namespace {

constexpr u32 kBinBits = 16;
constexpr u64 kGrain = u64{1} << 16;

struct Bin {
  u64 balls = 0, incidences = 0, bytes = 0, lo = ~u64{0}, hi = 0;
};

void add_to(Bin& into, const Bin& b) noexcept {
  into.balls += b.balls;
  into.incidences += b.incidences;
  into.bytes += b.bytes;
  into.lo = b.lo < into.lo ? b.lo : into.lo;
  into.hi = b.hi > into.hi ? b.hi : into.hi;
}

int bit_length64(u64 x) noexcept { return x == 0 ? 0 : 64 - __builtin_clzll(x); }

// Lot de chaque boule globale (dichotomie sur les bases) ; i < balls.
u64 chunk_of(const ArenaView& arena, u64 i) noexcept {
  u64 lo = 0, hi = arena.chunks.size() - 1;
  while (lo < hi) {
    const u64 mid = lo + (hi - lo + 1) / 2;
    if (arena.ball_at[mid] <= i) lo = mid;
    else hi = mid - 1;
  }
  return lo;
}

const BallRecord& record_of(const ArenaView& arena, u64 c, u64 i) noexcept {
  return arena.chunks[c].records[i - arena.ball_at[c]];
}

// Histogramme des cles de [lo, hi] en cases de 2^shift cles : une serie de cases par ouvrier, puis fusion.
struct Histogram {
  const ArenaView* arena;
  const u64* keys;
  u64 lo, hi, bins;
  u32 shift;
  Bin* per_worker;  // ouvriers x bins
  static Outcome body(void* context, u64 begin, u64 end, u32 worker) noexcept {
    const auto& h = *static_cast<const Histogram*>(context);
    Bin* bins = h.per_worker + u64{worker} * h.bins;
    u64 c = chunk_of(*h.arena, begin);
    for (u64 i = begin; i < end; ++i) {
      while (h.arena->ball_at[c + 1] <= i) ++c;
      const u64 key = h.keys[i];
      if (key < h.lo || key > h.hi) continue;
      const BallRecord& r = record_of(*h.arena, c, i);
      const u64 inc = u64{r.p} + r.m;
      Bin& b = bins[(key - h.lo) >> h.shift];
      b.balls += 1;
      b.incidences += inc;
      b.bytes += kSliceBytesPerBall + kSliceBytesPerIncidence * inc;
      b.lo = key < b.lo ? key : b.lo;
      b.hi = key > b.hi ? key : b.hi;
    }
    return {};
  }
};

Outcome histogram(const ArenaView& arena, std::span<const u64> keys, u64 lo, u64 hi, u32 shift, Buffer<Bin>& out,
                  MemoryBudget& budget, sched::Pool& pool) noexcept {
  const u64 bins = ((hi - lo) >> shift) + 1;
  Buffer<Bin> per_worker;
  MHGP12_TRY(per_worker.allocate(bins * pool.size(), budget));
  std::fill(per_worker.begin(), per_worker.end(), Bin{});
  Histogram h{&arena, keys.data(), lo, hi, bins, shift, per_worker.data()};
  MHGP12_TRY(pool.parallel_for(keys.size(), kGrain, &h, &Histogram::body));
  MHGP12_TRY(out.allocate(bins, budget));
  std::fill(out.begin(), out.end(), Bin{});
  for (u64 w = 0; w < pool.size(); ++w)
    for (u64 b = 0; b < bins; ++b) add_to(out[b], per_worker[w * bins + b]);
  return {};
}

// Boule d'un intervalle de cles pour la decoupe exacte : cle, incidences.
struct KeyItem {
  u64 key, incidences;
};

// Decoupe en cours : tranches emises dans l'ordre des cles. Les cases voisines d'ordre F4 incertain forment des unites
// insecables ; les unites sont rangees dans le groupe courant tant qu'il ne deborde pas cap (une coupe ne tombe donc
// qu'entre deux cases d'ordre certain, et jamais apres le debordement) ; un groupe trop lourd (une seule unite) est
// redecoupe sur son propre intervalle (cases plus fines), ou, si cela ne le reduirait pas de moitie, cle par cle
// (decoupe exacte) ; sans coupe certaine possible, refus memory_budget. L'intervalle se reduit a chaque appel : la
// decoupe termine.
struct Planner {
  const ArenaView& arena;
  std::span<const u64> keys;
  u64 cap;
  MemoryBudget& budget;
  sched::Pool& pool;
  std::vector<Bin> slices{};

  Outcome emit(const Bin& g) noexcept {
    try {
      slices.push_back(g);
    } catch (const std::bad_alloc&) {
      return fail(Reason::memory_budget);
    }
    return {};
  }

  Outcome cut(u64 lo, u64 hi) noexcept {
    const int width = bit_length64(hi - lo);
    const u32 shift = width > static_cast<int>(kBinBits) ? static_cast<u32>(width) - kBinBits : 0;
    Buffer<Bin> bins;
    MHGP12_TRY(histogram(arena, keys, lo, hi, shift, bins, budget, pool));
    return walk(bins.span(), lo, hi, shift == 0);
  }

  Outcome walk(std::span<const Bin> bins, u64 lo, u64 hi, bool exact) noexcept {
    Bin group, unit;
    for (const Bin& b : bins) {
      if (b.balls == 0) continue;
      if (unit.balls != 0 && key_order(unit.hi, b.lo) < 0) {  // coupe certaine : l'unite est close
        MHGP12_TRY(pack(group, unit, lo, hi, exact));
        unit = Bin{};
      }
      add_to(unit, b);
    }
    if (unit.balls != 0) MHGP12_TRY(pack(group, unit, lo, hi, exact));
    return group.balls == 0 ? Outcome{} : close(group, lo, hi, exact);
  }

  // L'unite rejoint le groupe courant s'il ne deborde pas ; sinon le groupe est clos et l'unite en ouvre un autre.
  Outcome pack(Bin& group, const Bin& unit, u64 lo, u64 hi, bool exact) noexcept {
    if (group.balls != 0 && group.bytes + unit.bytes > cap) {
      MHGP12_TRY(close(group, lo, hi, exact));
      group = Bin{};
    }
    add_to(group, unit);
    return {};
  }

  Outcome close(const Bin& g, u64 lo, u64 hi, bool exact) noexcept {
    if (g.bytes <= cap) return emit(g);
    if (exact || g.lo == g.hi) return fail(Reason::memory_budget);  // aucune coupe certaine : plateau incoupable
    if (g.hi - g.lo >= (hi - lo) / 2) return exact_cut(g.lo, g.hi);
    return cut(g.lo, g.hi);
  }

  // Decoupe cle par cle de [lo, hi] : boules de l'intervalle triees par cle, une case par cle distincte.
  Outcome exact_cut(u64 lo, u64 hi) noexcept {
    u64 count = 0;
    for (const u64 k : keys) count += k >= lo && k <= hi ? 1u : 0u;
    Buffer<KeyItem> items;
    MHGP12_TRY(items.allocate(count, budget));
    u64 at = 0, c = 0;
    for (u64 i = 0; i < keys.size(); ++i) {
      while (arena.ball_at[c + 1] <= i) ++c;
      if (keys[i] < lo || keys[i] > hi) continue;
      const BallRecord& r = record_of(arena, c, i);
      items[at++] = KeyItem{keys[i], u64{r.p} + r.m};
    }
    std::sort(items.begin(), items.end(), [](const KeyItem& a, const KeyItem& b) { return a.key < b.key; });
    Buffer<Bin> bins;
    u64 distinct = 0;
    for (u64 i = 0; i < count; ++i) distinct += i == 0 || items[i].key != items[i - 1].key ? 1u : 0u;
    MHGP12_TRY(bins.allocate(distinct, budget));
    for (u64 i = 0, b = 0; i < count; ++i) {
      if (i != 0 && items[i].key != items[i - 1].key) ++b;
      if (i == 0 || items[i].key != items[i - 1].key) bins[b] = Bin{};
      const Bin one{1, items[i].incidences, kSliceBytesPerBall + kSliceBytesPerIncidence * items[i].incidences,
                    items[i].key, items[i].key};
      add_to(bins[b], one);
    }
    return walk(bins.span(), lo, hi, true);
  }
};

}  // namespace

Outcome arena_bases(std::span<const Chunk> chunks, Buffer<u64>& ball_at, Buffer<u64>& incidence_at, ArenaView& out,
                    MemoryBudget& budget) noexcept {
  MHGP12_TRY(ball_at.allocate(chunks.size() + 1, budget));
  MHGP12_TRY(incidence_at.allocate(chunks.size() + 1, budget));
  u64 balls = 0, incidences = 0;
  for (u64 c = 0; c < chunks.size(); ++c) {
    ball_at[c] = balls;
    incidence_at[c] = incidences;
    if (__builtin_add_overflow(balls, chunks[c].records.size(), &balls) ||
        __builtin_add_overflow(incidences, chunks[c].population.size(), &incidences))
      return fail(Reason::catalogue_invariant);
  }
  ball_at[chunks.size()] = balls;
  incidence_at[chunks.size()] = incidences;
  if (balls >= kNone) return fail(Reason::index_overflow_u32);
  out = ArenaView{chunks, ball_at.data(), incidence_at.data(), balls, incidences};
  return {};
}

Outcome plan_slices(const ArenaView& arena, std::span<const u64> keys, u64 slice_bytes, SlicePlan& plan,
                    MemoryBudget& budget, sched::Pool& pool) noexcept {
  plan.count = 0;
  if (keys.size() != arena.balls) return fail(Reason::catalogue_invariant);
  if (keys.empty()) return {};
  u64 lo = ~u64{0}, hi = 0;
  for (const u64 k : keys) {
    lo = k < lo ? k : lo;
    hi = k > hi ? k : hi;
  }
  Planner p{arena, keys, slice_bytes, budget, pool};
  MHGP12_TRY(p.cut(lo, hi));
  const u64 count = p.slices.size();
  MHGP12_TRY(plan.upper.allocate(count, budget));
  MHGP12_TRY(plan.balls.allocate(count, budget));
  MHGP12_TRY(plan.incidences.allocate(count, budget));
  u64 balls = 0;
  for (u64 j = 0; j < count; ++j) {
    plan.upper[j] = j + 1 < count ? p.slices[j + 1].lo : ~u64{0};
    plan.balls[j] = p.slices[j].balls;
    plan.incidences[j] = p.slices[j].incidences;
    balls += p.slices[j].balls;
  }
  plan.count = count;
  return balls == arena.balls ? Outcome{} : fail(Reason::catalogue_invariant);
}

namespace {

// Repartition : pour chaque bloc de kGrain boules, nombre de boules par tranche (passe 1), puis ecriture aux places
// fixees par les prefixes (passe 2) ; l'ordre des indices dans une tranche est croissant.
struct Assign {
  const u64* keys;
  const SlicePlan* plan;
  u64 n, slices;
  u64* counts;  // blocs x tranches : comptes, puis places
  u32* ids;
  bool write;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    const auto& a = *static_cast<const Assign*>(context);
    for (u64 block = begin; block < end; ++block) {
      u64* row = a.counts + block * a.slices;
      const u64 last = (block + 1) * kGrain < a.n ? (block + 1) * kGrain : a.n;
      for (u64 i = block * kGrain; i < last; ++i) {
        const u64 j = slice_of(*a.plan, a.keys[i]);
        if (a.write) a.ids[row[j]++] = static_cast<u32>(i);
        else ++row[j];
      }
    }
    return {};
  }
};

// Rassemblement : longueurs des populations (passe 1, dans length), puis copie des enregistrements rebases et des
// populations (passe 2).
struct Gather {
  const ArenaView* arena;
  const u32* ids;
  u64 n;
  u64* offset;  // decalage de population de chaque boule de la tranche
  BallRecord* records;
  SiteIdx* population;
  bool write;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    const auto& g = *static_cast<const Gather*>(context);
    u64 c = chunk_of(*g.arena, g.ids[begin]);
    for (u64 i = begin; i < end; ++i) {
      while (g.arena->ball_at[c + 1] <= g.ids[i]) ++c;
      const Chunk& chunk = g.arena->chunks[c];
      BallRecord r = chunk.records[g.ids[i] - g.arena->ball_at[c]];
      const u64 length = u64{r.p} + r.m;
      if (!g.write) {
        g.offset[i] = length;
        continue;
      }
      if (r.population > chunk.population.size() || length > chunk.population.size() - r.population)
        return fail(Reason::catalogue_invariant);
      if (length != 0)
        std::memcpy(g.population + g.offset[i], chunk.population.data() + r.population, length * sizeof(SiteIdx));
      r.population = g.offset[i];
      r.chunk = 0;
      g.records[i] = r;
    }
    return {};
  }
};

}  // namespace

Outcome assign_slices(std::span<const u64> keys, const SlicePlan& plan, Buffer<u32>& ids, Buffer<u64>& start,
                      MemoryBudget& budget, sched::Pool& pool) noexcept {
  const u64 n = keys.size(), blocks = (n + kGrain - 1) / kGrain, s = plan.count;
  MHGP12_TRY(ids.allocate(n, budget));
  MHGP12_TRY(start.allocate(s + 1, budget));
  Buffer<u64> counts;
  MHGP12_TRY(counts.allocate(blocks * s, budget));
  std::fill(counts.begin(), counts.end(), u64{0});
  Assign a{keys.data(), &plan, n, s, counts.data(), ids.data(), false};
  MHGP12_TRY(pool.parallel_for(blocks, 1, &a, &Assign::body));
  u64 at = 0;
  for (u64 j = 0; j < s; ++j) {
    start[j] = at;
    for (u64 block = 0; block < blocks; ++block) {
      const u64 count = counts[block * s + j];
      counts[block * s + j] = at;
      at += count;
    }
    if (at - start[j] != plan.balls[j]) return fail(Reason::catalogue_invariant);
  }
  start[s] = at;
  if (at != n) return fail(Reason::catalogue_invariant);
  a.write = true;
  return pool.parallel_for(blocks, 1, &a, &Assign::body);
}

Outcome gather_slice(const ArenaView& arena, std::span<const u32> ids, Buffer<BallRecord>& records,
                     Buffer<SiteIdx>& population, MemoryBudget& budget, sched::Pool& pool) noexcept {
  const u64 n = ids.size();
  Buffer<u64> offset;
  MHGP12_TRY(offset.allocate(n, budget));
  MHGP12_TRY(records.allocate(n, budget));
  Gather g{&arena, ids.data(), n, offset.data(), records.data(), nullptr, false};
  if (n != 0) MHGP12_TRY(pool.parallel_for(n, kGrain, &g, &Gather::body));
  u64 total = 0;
  for (u64 i = 0; i < n; ++i) {
    const u64 length = offset[i];
    offset[i] = total;
    total += length;
  }
  MHGP12_TRY(population.allocate(total, budget));
  g.population = population.data();
  g.write = true;
  return n == 0 ? Outcome{} : pool.parallel_for(n, kGrain, &g, &Gather::body);
}

namespace {

// Cle de la table d'une boule : celle de TableKeyKernel (S*[0..3], case absente = nombre de sites).
Key2 table_key(const CatalogueBall& b, u32 sites, u32 bits) noexcept {
  u32 parts[4];
  for (u32 k = 0; k < 4; ++k) parts[k] = k < b.qmin ? static_cast<u32>(idx(b.support[k])) : sites;
  return pack4(parts, bits);
}

// Lignes de la table triees par la cle (une ligne par site ; cles distinctes, ordre total).
struct TableRows {
  const CatalogueBall* balls;
  u32 sites, bits;
  const u64* offsets;
  BallIdx* values;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    const auto& t = *static_cast<const TableRows*>(context);
    for (u64 s = begin; s < end; ++s)
      std::sort(t.values + t.offsets[s], t.values + t.offsets[s + 1], [&](BallIdx a, BallIdx b) {
        return key2_cmp(table_key(t.balls[idx(a)], t.sites, t.bits), table_key(t.balls[idx(b)], t.sites, t.bits)) < 0;
      });
    return {};
  }
};

}  // namespace

Outcome host_table(std::span<const CatalogueBall> balls, u32 sites, Buffer<u64>& offsets, Buffer<BallIdx>& values,
                   MemoryBudget& budget, sched::Pool& pool) noexcept {
  const u64 n = balls.size();
  MHGP12_TRY(offsets.allocate(u64{sites} + 1, budget));
  std::fill(offsets.begin(), offsets.end(), u64{0});
  for (const CatalogueBall& b : balls) {
    const u32 s = static_cast<u32>(idx(b.support[0]));
    if (s >= sites) return fail(Reason::catalogue_invariant);
    ++offsets[s + 1];
  }
  for (u64 s = 0; s < sites; ++s) offsets[s + 1] += offsets[s];
  MHGP12_TRY(values.allocate(n, budget));
  Buffer<u64> cursor;
  MHGP12_TRY(cursor.allocate(sites, budget));
  std::copy(offsets.begin(), offsets.begin() + sites, cursor.begin());
  for (u64 i = 0; i < n; ++i) values[cursor[idx(balls[i].support[0])]++] = make_id<BallIdx>(static_cast<u32>(i));
  TableRows rows{balls.data(), sites, width_of(sites), offsets.data(), values.data()};
  return sites == 0 ? Outcome{} : pool.parallel_for(sites, 4096, &rows, &TableRows::body);
}

Outcome materialize_levels(std::vector<Buffer<LevelWords>>& words, u64 distinct, Buffer<num::Level>& levels,
                           MemoryBudget& budget, sched::Pool& pool) noexcept {
  if (distinct + 1 > kNone) return fail(Reason::index_overflow_u32);
  MHGP12_TRY(levels.allocate(distinct + 1, budget));
  levels[0] = num::Level{};
  u64 at = 1;
  for (Buffer<LevelWords>& w : words) {
    if (w.size() > distinct + 1 - at) return fail(Reason::catalogue_invariant);
    Materialize m{w.data(), levels.data() + at};
    if (!w.empty()) MHGP12_TRY(pool.parallel_for(w.size(), 4096, &m, &Materialize::body));
    at += w.size();
    w.reset();
  }
  return at == distinct + 1 ? Outcome{} : fail(Reason::catalogue_invariant);
}

}  // namespace mhgp12::catalogue_detail::fin
