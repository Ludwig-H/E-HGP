// Passes de resolution de l'etage G, ordre par ordre (CONTRAT_TOUR.md, paragraphes 4.1, 4.3 et 4.4) : ordre 1 (les
// sites), puis, pour chaque ordre k >= 2, index des naissances (populations.cpp, parallele), premieres sondes, passe de
// resolution par tranches de cellules et controles globaux. Les tampons de l'index et de la jointure sont gardes d'un
// ordre a l'autre (une seule allocation a la taille du plus grand ordre). Chaque representant a sa case, ecrite par un
// seul fil ; compteurs par fil fusionnes dans l'ordre des fils (sommes et maxima : independants du decoupage). Les
// memes corps (tranche de cellules, cloture d'un ordre) servent la Session recouverte (pipeline.cpp), qui joue les
// tranches de tous les ordres dans une seule region, avec un index par ordre.
//
// Premieres sondes (contrat, paragraphe 4.3) : voie produit G-L7, une file de kLag representants par tranche (trace
// formee et case du repertoire de l'index prechargee a l'entree, fiches du seau prechargees a mi-file, recherche exacte
// a la sortie) ; aucune memoire en plus, ordre des representants inchange. G-L5 (jointure triee de first_probes.cpp)
// fournit des candidats verifies a la place de la recherche quand resolve_order les calcule (bras de mesure du pilote
// microbancs/mes_t2c_g ; jugee plus lente sur l'hote en local, voie de l'appareil).
#include <algorithm>

#include "tower/profile.hpp"
#include "tower/stage.hpp"

namespace mhgp12::tower_detail {
namespace {

// Ordre 1 : les naissances sont les sites, et la cible d'une trace (un site) est sa naissance.
struct FirstOrder {
  const Catalogue& catalogue;
  ResolvedOrder& order;
  static Outcome body(void* raw, u64 begin, u64 end, u32) noexcept {
    auto& self = *static_cast<FirstOrder*>(raw);
    const auto balls = StageAccess::cell_balls(self.order).span();
    const auto offsets = StageAccess::cell_offsets(self.order).span();
    const auto masks = StageAccess::trace_masks(self.order).span();
    auto targets = StageAccess::targets(self.order).span();
    for (u64 c = begin; c < end; ++c) {
      const auto shell = self.catalogue.shell(balls[c]);
      for (u64 r = offsets[c]; r < offsets[c + 1]; ++r) {
        if (masks[r] == 0 || (masks[r] & (masks[r] - 1)) != 0) return fail(Reason::tower_invariant, 1);
        const u32 j = static_cast<u32>(__builtin_ctzll(masks[r]));
        if (j >= shell.size()) return fail(Reason::tower_invariant, 1);
        targets[r] = birth_target(idx(shell[j]));
      }
    }
    return {};
  }
};

// Trace stricte I u A du representant de masque mask (bit j = j-ieme site de U), SiteIdx croissants.
bool build_trace(std::span<const SiteIdx> inner, std::span<const SiteIdx> shell, u64 mask, Part& f) noexcept {
  std::size_t i = 0;
  for (u64 rest = mask; i < inner.size() || rest != 0;) {
    const u32 j = rest != 0 ? static_cast<u32>(__builtin_ctzll(rest)) : 0;
    const bool from_inner = rest == 0 || (i < inner.size() && idx(inner[i]) < idx(shell[j]));
    if (f.k == kMaxPart) return false;
    f.id[f.k++] = idx(from_inner ? inner[i++] : shell[j]);
    if (!from_inner) rest &= rest - 1;
  }
  return true;
}

struct OrderPass {
  const ResolveContext& context;
  ResolvedOrder& order;
  Workers& workers;                    // espaces de census par fil
  std::span<OrderCounters> counters;   // une case par fil, propre a l'ordre
  std::span<SectionCycles> profiles;   // idem (construction MHGP12_TOWER_PROFILE)
  const PopulationTable& table;
  std::span<const u32> candidates;  // G-L5 ; vide : file de sondes prechargees (G-L7)
  static constexpr u64 kLag = 16, kHalf = kLag / 2;

  struct Pending {
    Part f;
    u64 key = 0, rep = 0, cell = 0;
  };
  struct Lane {
    OrderPass& pass;
    OrderCounters& counters;
    CensusWorkspace& workspace;
    SectionClock& clock;
    std::span<const LevelRank> ranks;
    std::span<u32> targets;
    Outcome finish(const Pending& p) noexcept {
      FirstProbe first;
      first.done = true;
      if (pass.candidates.empty()) first.hit = pass.table.find(p.f, p.key);
      else if (const u32 candidate = pass.candidates[p.rep]; candidate != kNone)
        first.hit = pass.table.verify(candidate, p.f);
      auto target = resolve_part(pass.context, p.f, ranks[p.cell], workspace, counters, clock, first);
      if (!target.ok()) return target.outcome();
      targets[p.rep] = target.value();
      return {};
    }
  };

  static Outcome body(void* raw, u64 begin, u64 end, u32 worker) noexcept {
    auto& self = *static_cast<OrderPass*>(raw);
    const Catalogue& cat = self.context.domain.catalogue;
    const auto balls = StageAccess::cell_balls(self.order).span();
    const auto offsets = StageAccess::cell_offsets(self.order).span();
    const auto masks = StageAccess::trace_masks(self.order).span();
    const Order k = self.context.order.k;
    SectionCycles* sink = kProfile ? &self.profiles[worker] : nullptr;
    const u64 start = profile_tick();
    SectionClock clock(sink);
    Lane lane{self, self.counters[worker], *self.workers.census[worker], clock,
              StageAccess::cell_ranks(self.order).span(), StageAccess::targets(self.order).span()};
    std::array<Pending, kLag> file;
    std::array<u64, kMaxShell> shell_keys;  // empreintes des sites de U, une fois par cellule
    u64 in = 0, out = 0;
    for (u64 c = begin; c < end; ++c) {
      const auto inner = cat.interior(balls[c]), shell = cat.shell(balls[c]);
      if (shell.size() > kMaxShell) return fail(Reason::tower_invariant, k);
      u64 base = 0;  // empreinte de I ; celle de la trace I u A = base + empreintes des sites de A (key_of)
      if (self.candidates.empty()) {
        for (const SiteIdx site : inner) base += site_key(idx(site));
        for (std::size_t j = 0; j < shell.size(); ++j) shell_keys[j] = site_key(idx(shell[j]));
      }
      for (u64 r = offsets[c]; r < offsets[c + 1]; ++r) {
        Pending& p = file[in % kLag];
        p.f = Part{};
        if (!build_trace(inner, shell, masks[r], p.f) || p.f.k != k) return fail(Reason::tower_invariant, k);
        p.rep = r;
        p.cell = c;
        if (self.candidates.empty()) {
          u64 key = base;
          for (u64 rest = masks[r]; rest != 0; rest &= rest - 1) key += shell_keys[__builtin_ctzll(rest)];
          p.key = key & self.table.key_mask();
          self.table.prefetch_directory(p.key);
          if (in >= kHalf) self.table.prefetch_bucket(file[(in - kHalf) % kLag].key);
        } else {
          self.table.prefetch(self.candidates[r]);
        }
        ++in;
        clock.lap(kProfileTrace);
        if (in - out == kLag) MHGP12_TRY(lane.finish(file[out++ % kLag]));
      }
    }
    while (out < in) MHGP12_TRY(lane.finish(file[out++ % kLag]));
    clock.charge(kProfileTotal, start);
    return {};
  }
};

// Controles globaux d'un ordre : un controle par plus petite boule et par succes de sonde ; une chaine par
// representant, terminee par une sonde reussie ou un arret.
Outcome check_order(const ResolvedOrder& order, const OrderCounters& total) noexcept {
  u64 chains = 0;
  for (const u64 bin : total.chain_histogram) chains += bin;
  if (total.controls != total.smallest_balls() + total.first_probe_hits + total.probe_hits_after_steps ||
      chains != order.representatives() ||
      chains != total.first_probe_hits + total.probe_hits_after_steps + total.cell_stops + total.birth_stops)
    return fail(Reason::tower_invariant, order.order());
  return {};
}

// Ordre k >= 2 : index, premieres sondes, passe de resolution, fusion des compteurs, controles ; chronos table_ns,
// join_ns et pass_ns de l'ordre (frontieres disjointes).
Outcome resolve_order(const Domain& d, Resolution& out, Order k, OrderBuffers& buffers, Workers& workers,
                      MemoryBudget& budget, sched::Pool& pool, ResolutionDiagnostics& diag) noexcept {
  ResolvedOrder& order = StageAccess::order(out, k);
  PopulationTable& table = buffers.table;
  const Stopwatch tabling;
  MHGP12_TRY(table.build(d.catalogue, order.birth_keys(), order.birth_ranks(), k, budget, pool));
  diag.table_ns[k] = tabling.nanoseconds();
  const Stopwatch joining;
  const std::span<const u32> candidates{};  // G-L7 ; G-L5 : first_probe_candidates (bras du pilote)
  diag.join_ns[k] = joining.nanoseconds();
  const Stopwatch passing;
  for (u32 w = 0; w < pool.size(); ++w) workers.counters[w] = OrderCounters{};
  if constexpr (kProfile)
    for (u32 w = 0; w < pool.size(); ++w) workers.profiles[w] = SectionCycles{};
  const ResolveContext context{d, out, OrderView{k, order.birth_keys(), &table}};
  OrderPass pass{context, order, workers, workers.counters.span(), workers.profiles.span(), table, candidates};
  MHGP12_TRY(pool.parallel_for(order.cells(), kCellGrain, &pass, &OrderPass::body));
  OrderCounters& total = StageAccess::counters(order);
  for (u32 w = 0; w < pool.size(); ++w) add_counters(total, workers.counters[w]);
  if constexpr (kProfile)
    for (u32 w = 0; w < pool.size(); ++w)
      for (int s = 0; s < kProfileSections; ++s) {
        diag.profile[k].cycles[s] += workers.profiles[w].cycles[s];
        diag.profile[k].count[s] += workers.profiles[w].count[s];
      }
  MHGP12_TRY(check_order(order, total));
  diag.pass_ns[k] = passing.nanoseconds();
  return {};
}

}  // namespace

Outcome first_order_cells(const Catalogue& catalogue, ResolvedOrder& order, u64 begin, u64 end) noexcept {
  FirstOrder first{catalogue, order};
  return FirstOrder::body(&first, begin, end, 0);
}

Outcome resolve_cells(const ResolveContext& context, ResolvedOrder& order, Workers& workers,
                      std::span<OrderCounters> counters, std::span<SectionCycles> profiles,
                      const PopulationTable& table, u64 begin, u64 end, u32 worker) noexcept {
  OrderPass pass{context, order, workers, counters, profiles, table, std::span<const u32>{}};
  return OrderPass::body(&pass, begin, end, worker);
}

Outcome close_order(ResolvedOrder& order, std::span<const OrderCounters> counters) noexcept {
  OrderCounters& total = StageAccess::counters(order);
  for (const OrderCounters& part : counters) add_counters(total, part);
  return check_order(order, total);
}

Outcome resolve_orders(const Domain& d, Resolution& out, OrderBuffers& buffers, Workers& workers,
                       MemoryBudget& budget, sched::Pool& pool, ResolutionDiagnostics& diag) noexcept {
  for (Order k = 1; k <= out.orders(); ++k) {
    const Stopwatch watch;
    ResolvedOrder& order = StageAccess::order(out, k);
    if (k == 1) {
      FirstOrder first{d.catalogue, order};
      MHGP12_TRY(pool.parallel_for(order.cells(), kCellGrain, &first, &FirstOrder::body));
    } else {
      MHGP12_TRY(resolve_order(d, out, k, buffers, workers, budget, pool, diag));
    }
    diag.order_ns[k] = watch.nanoseconds();
    if (k == 1) diag.pass_ns[k] = diag.order_ns[k];
    diag.orders_ns += diag.order_ns[k];
    diag.tables_ns += diag.table_ns[k];
    diag.joins_ns += diag.join_ns[k];
    diag.resolve_ns += diag.pass_ns[k];
  }
  diag.table_bytes = buffers.table.bytes() + buffers.join.bytes();
  return {};
}

}  // namespace mhgp12::tower_detail
