// Etage des feuilles : lots de feuilles d'un niveau, comptes exactement puis ecrits, repartis sur le Pool. Chaque
// feuille est jouee par la source J3 (leaf_j3.hpp) : warp de 32 voies si m <= 32, warp virtuel de 256 voies sinon ;
// arithmetique native si l'etendue s de son repere (fermeture de la boite et sites de la liste, num::Frame) est au
// plus 16, exacte plus large sinon (repli exact, CONTRAT_CATALOGUE.md, paragraphe 2). Le choix est fait avant de jouer
// la feuille, uniforme sur la feuille : aucune tentative n'est perdue sur l'hote.
//
// Comptage puis reservation (CST-0211) : le comptage joue chaque feuille une fois et garde au plus kCase emissions par
// feuille ; les decalages exacts sont des prefixes controles ; le lot reserve exactement ses boules et incidences,
// puis l'ecriture copie les cases et rejoue les seules feuilles qui en debordent. Les compteurs logiques d'une
// feuille sont ceux du comptage, apres succes ; le rejeu doit rendre les memes comptes (invariant).
#include "catalogue/internal.hpp"
#include "sched/sched.hpp"

namespace mhgp12::catalogue_detail {
namespace {

constexpr u32 kCase = 64;              // emissions gardees par feuille au comptage (64 x 24 octets)
constexpr u64 kBatchLeaves = 16384;    // feuilles par lot : cases du lot <= 25 Mo
constexpr u64 kLeafGrain = 4;          // feuilles par tranche du Pool (couts tres inegaux)

struct LeafWork {
  u64 balls, incidences;
  u32 status, width;
};

// Sommes d'un ouvrier, lues apres la jointure.
struct Tally {
  LeafCounts counts;
  u64 narrow, medium, wide, exact, virtual_warp, rewritten, max_span;
  bool overflow;
};

struct Batch {
  const Cloud& cloud;
  int kmax;
  std::span<const bfs::Leaf> leaves;
  std::span<const u32> sites;
  LeafShared<32>* shared32;
  LeafShared<256>* shared256;
  Tally* tallies;
  LeafWork* work;
  Emission<32>* cases;
  const u64* record_at;      // ecriture : premiere boule de chaque feuille dans le lot
  const u64* population_at;  // ecriture : premiere incidence de chaque feuille dans le lot
  BallRecord* records;
  SiteIdx* population;
  u32 chunk;
};

struct CaseSink {  // comptage, warp de 32 voies : compte et garde les kCase premieres emissions
  Emission<32>* slots;
  u64 balls = 0, incidences = 0;
  void emit(const Emission<32>& e) noexcept {
    if (balls < kCase) slots[balls] = e;
    ++balls;
    incidences += u64{e.p} + e.m;
  }
};

template <u32 N>
struct CountSink {
  u64 balls = 0, incidences = 0;
  void emit(const Emission<N>& e) noexcept {
    ++balls;
    incidences += u64{e.p} + e.m;
  }
};

// Emission en rangs locaux -> boule en SiteIdx globaux ; les rangs locaux suivent les SiteIdx croissants de la feuille.
template <u32 N>
void convert(const Emission<N>& e, const u32* leaf_sites, BallRecord& r, SiteIdx* population, u64 at, u32 chunk) {
  for (u32 k = 0; k < 4; ++k) r.support[k] = e.support[k] == kNoLocal ? kNone : leaf_sites[e.support[k]];
  r.population = at;
  r.p = e.p;
  r.m = static_cast<u8>(e.m);  // e.m <= 64 : coquille plafonnee avant emission
  r.qmin = e.qmin;
  r.pad = 0;
  r.chunk = chunk;
  for (auto rest = e.interior; simt::any(rest); rest = simt::clear_lowest(rest))
    population[at++] = make_id<SiteIdx>(leaf_sites[simt::ctz(rest)]);
  for (auto rest = e.shell; simt::any(rest); rest = simt::clear_lowest(rest))
    population[at++] = make_id<SiteIdx>(leaf_sites[simt::ctz(rest)]);
}

template <u32 N>
struct WriteSink {  // ecriture directe d'une feuille rejouee, aux places de son comptage
  const u32* leaf_sites;
  BallRecord* records;
  SiteIdx* population;
  u64 population_at, balls_cap, incidences_cap;
  u32 chunk;
  u64 balls = 0, incidences = 0;
  bool overflow = false;
  void emit(const Emission<N>& e) noexcept {
    if (balls >= balls_cap || incidences + e.p + e.m > incidences_cap) {
      overflow = true;
      return;
    }
    convert<N>(e, leaf_sites, records[balls], population, population_at + incidences, chunk);
    ++balls;
    incidences += u64{e.p} + e.m;
  }
};

// Repere de la feuille (NUM-REPERE, NUM-COUVERTURE) : fermeture [lo, hi] de sa boite et tous les sites de sa liste.
num::Frame leaf_frame(const Cloud& cloud, const bfs::Leaf& leaf, const u32* sites) noexcept {
  num::Frame frame;
  // 0 <= lo < hi <= 2^32 (boites du parcours, CST-0204) : add_box ne refuse pas ; un refus rend un repere vide, que
  // l'appelant traite en invariant.
  if (!frame.add_box({static_cast<u64>(leaf.lo[0]), static_cast<u64>(leaf.lo[1]), static_cast<u64>(leaf.lo[2])},
                     {static_cast<u64>(leaf.hi[0]), static_cast<u64>(leaf.hi[1]), static_cast<u64>(leaf.hi[2])})
           .ok())
    return num::Frame{};
  for (u32 i = 0; i < leaf.m; ++i) {
    const u32 s = sites[i];
    frame.add_site(cloud.x()[s], cloud.y()[s], cloud.z()[s]);
  }
  return frame;
}

LeafInput input_of(const Batch& b, const bfs::Leaf& leaf, const u32* sites, const num::Frame& frame) noexcept {
  LeafInput in;
  in.x = b.cloud.x().data();
  in.y = b.cloud.y().data();
  in.z = b.cloud.z().data();
  in.sites = sites;
  in.m = leaf.m;
  for (int a = 0; a < 3; ++a) {
    in.lo[a] = leaf.lo[a];
    in.hi[a] = leaf.hi[a];
    in.origin[a] = static_cast<i64>(frame.origin()[a]);
  }
  in.kmax = b.kmax;
  return in;
}

// Joue une feuille sur le warp de sa largeur, dans l'arithmetique de son palier.
template <class Sink32, class Sink256>
u32 play(const LeafInput& in, int span, const Batch& b, u32 worker, LeafCounts& counts, Sink32& s32,
         Sink256& s256) noexcept {
  const bool narrow = span <= kLeafNarrowSpan;
  if (in.m <= kGraphSites) {
    LeafShared<32>& shared = b.shared32[worker];
    return narrow ? run_leaf<32, Narrow>(in, shared, counts, s32) : run_leaf<32, Exact>(in, shared, counts, s32);
  }
  LeafShared<256>& shared = b.shared256[worker];
  return narrow ? run_leaf<256, Narrow>(in, shared, counts, s256) : run_leaf<256, Exact>(in, shared, counts, s256);
}

void tally_tier(Tally& t, int span, u32 m) noexcept {
  ++(span <= num::kNarrowSpan ? t.narrow : span <= num::kMediumSpan ? t.medium : t.wide);
  if (span > kLeafNarrowSpan) ++t.exact;
  if (m > kGraphSites) ++t.virtual_warp;
  t.max_span = static_cast<u64>(span) > t.max_span ? static_cast<u64>(span) : t.max_span;
}

Outcome count_body(void* context, u64 begin, u64 end, u32 worker) noexcept {
  Batch& b = *static_cast<Batch*>(context);
  Tally& tally = b.tallies[worker];
  for (u64 i = begin; i < end; ++i) {
    const bfs::Leaf& leaf = b.leaves[i];
    const u32* sites = b.sites.data() + leaf.begin;
    const num::Frame frame = leaf_frame(b.cloud, leaf, sites);
    const int span = frame.span();
    const LeafInput in = input_of(b, leaf, sites, frame);
    LeafCounts counts;
    CaseSink cases{b.cases + i * kCase};
    CountSink<256> wide;
    LeafWork& w = b.work[i];
    w.width = leaf.m <= kGraphSites ? 32 : 256;
    w.status = frame.empty() ? u32{kLeafInvariant} : play(in, span, b, worker, counts, cases, wide);
    w.balls = w.width == 32 ? cases.balls : wide.balls;
    w.incidences = w.width == 32 ? cases.incidences : wide.incidences;
    if (w.status == kLeafOk && !add_leaf_counts(tally.counts, counts)) tally.overflow = true;
    tally_tier(tally, span, leaf.m);
  }
  return {};
}

Outcome fill_body(void* context, u64 begin, u64 end, u32 worker) noexcept {
  Batch& b = *static_cast<Batch*>(context);
  for (u64 i = begin; i < end; ++i) {
    const LeafWork& w = b.work[i];
    if (w.balls == 0) continue;
    const bfs::Leaf& leaf = b.leaves[i];
    const u32* sites = b.sites.data() + leaf.begin;
    const u64 at = b.record_at[i], pat = b.population_at[i];
    if (w.width == 32 && w.balls <= kCase) {
      u64 offset = pat;
      for (u64 j = 0; j < w.balls; ++j) {
        const Emission<32>& e = b.cases[i * kCase + j];
        convert<32>(e, sites, b.records[at + j], b.population, offset, b.chunk);
        offset += u64{e.p} + e.m;
      }
      continue;
    }
    const num::Frame frame = leaf_frame(b.cloud, leaf, sites);
    const LeafInput in = input_of(b, leaf, sites, frame);
    LeafCounts counts;
    WriteSink<32> s32{sites, b.records + at, b.population, pat, w.balls, w.incidences, b.chunk};
    WriteSink<256> s256{sites, b.records + at, b.population, pat, w.balls, w.incidences, b.chunk};
    const u32 status = play(in, frame.span(), b, worker, counts, s32, s256);
    const auto& sink_balls = w.width == 32 ? s32.balls : s256.balls;
    const auto& sink_incidences = w.width == 32 ? s32.incidences : s256.incidences;
    if (status != kLeafOk || s32.overflow || s256.overflow || sink_balls != w.balls || sink_incidences != w.incidences)
      return fail(Reason::catalogue_invariant);
    ++b.tallies[worker].rewritten;
  }
  return {};
}

Outcome leaf_outcome(u32 status) noexcept {
  if (status == kLeafOk) return {};
  return fail(status == kLeafShellCapacity ? Reason::shell_capacity : Reason::catalogue_invariant);
}

// Decalages exacts des boules et incidences de chaque feuille du lot (sommes controlees).
Outcome offsets(const Buffer<LeafWork>& work, Buffer<u64>& record_at, Buffer<u64>& population_at, u64& balls,
                u64& incidences) noexcept {
  balls = 0;
  incidences = 0;
  for (u64 i = 0; i < work.size(); ++i) {
    record_at[i] = balls;
    population_at[i] = incidences;
    if (__builtin_add_overflow(balls, work[i].balls, &balls) ||
        __builtin_add_overflow(incidences, work[i].incidences, &incidences))
      return fail(Reason::memory_budget);  // taille non representable
  }
  return {};
}

// Ajout d'un lot a la liste des lots ; une penurie du conteneur des poignees est un refus memoire.
Outcome append(std::vector<Chunk>& chunks, Chunk&& chunk) noexcept {
  try {
    chunks.push_back(std::move(chunk));
  } catch (const std::bad_alloc&) {
    return fail(Reason::memory_budget);
  }
  return {};
}

}  // namespace

bool add_leaf_counts(LeafCounts& into, const LeafCounts& c) noexcept {
  bool ok = true;
  for (u32 f = 0; f < kLeafCounters; ++f) ok = !__builtin_add_overflow(into.c[f], c.c[f], &into.c[f]) && ok;
  return ok;
}

// Un lot : comptage, controle des issues, decalages, reservation exacte, ecriture.
Outcome LeafStage::batch(std::span<const bfs::Leaf> leaves, std::span<const u32> sites) noexcept {
  const u64 n = leaves.size();
  const u32 workers = pool_.size();
  bool virtual_warp = false;
  for (const auto& leaf : leaves) virtual_warp = virtual_warp || leaf.m > kGraphSites;
  Buffer<LeafShared<32>> shared32;
  Buffer<LeafShared<256>> shared256;
  Buffer<Tally> tallies;
  Buffer<LeafWork> work;
  Buffer<Emission<32>> cases;
  MHGP12_TRY(shared32.allocate(workers, budget_));
  if (virtual_warp) MHGP12_TRY(shared256.allocate(workers, budget_));
  MHGP12_TRY(tallies.allocate(workers, budget_));
  MHGP12_TRY(work.allocate(n, budget_));
  MHGP12_TRY(cases.allocate(n * kCase, budget_));
  for (auto& t : tallies) t = Tally{};
  Batch b{cloud_,         params_.kmax, leaves,      sites,   shared32.data(), shared256.data(), tallies.data(),
          work.data(),    cases.data(), nullptr,     nullptr, nullptr,         nullptr,          0};
  Stopwatch count_watch;
  MHGP12_TRY(pool_.parallel_for(n, kLeafGrain, &b, &count_body));
  totals_.count_ns += count_watch.nanoseconds();
  Outcome status{};
  for (u64 i = 0; i < n; ++i) status = merge(status, leaf_outcome(work[i].status));
  MHGP12_TRY(status);
  for (const Tally& t : tallies) {
    if (t.overflow || !add_leaf_counts(totals_.counts, t.counts)) return fail(Reason::catalogue_counter_overflow);
    totals_.narrow += t.narrow;
    totals_.medium += t.medium;
    totals_.wide += t.wide;
    totals_.exact += t.exact;
    totals_.virtual_warp += t.virtual_warp;
    totals_.max_span = t.max_span > totals_.max_span ? t.max_span : totals_.max_span;
  }
  Buffer<u64> record_at, population_at;
  MHGP12_TRY(record_at.allocate(n, budget_));
  MHGP12_TRY(population_at.allocate(n, budget_));
  u64 balls = 0, incidences = 0;
  MHGP12_TRY(offsets(work, record_at, population_at, balls, incidences));
  if (balls >= u64{kNone} - balls_) return fail(Reason::index_overflow_u32);  // BallIdx sur 32 bits, kNone exclu
  Chunk chunk;
  MHGP12_TRY(chunk.records.allocate(balls, budget_));
  MHGP12_TRY(chunk.population.allocate(incidences, budget_));
  b.record_at = record_at.data();
  b.population_at = population_at.data();
  b.records = chunk.records.data();
  b.population = chunk.population.data();
  b.chunk = static_cast<u32>(chunks_.size());
  Stopwatch fill_watch;
  MHGP12_TRY(pool_.parallel_for(n, kLeafGrain, &b, &fill_body));
  totals_.fill_ns += fill_watch.nanoseconds();
  for (const Tally& t : tallies) totals_.rewritten += t.rewritten;
  balls_ += balls;
  return append(chunks_, std::move(chunk));
}

Outcome LeafStage::consume(std::span<const bfs::Leaf> leaves, std::span<const u32> sites, u32) noexcept {
  for (u64 first = 0; first < leaves.size(); first += kBatchLeaves) {
    const u64 count = leaves.size() - first < kBatchLeaves ? leaves.size() - first : kBatchLeaves;
    MHGP12_TRY(batch(leaves.subspan(first, count), sites));
  }
  return {};
}

}  // namespace mhgp12::catalogue_detail
