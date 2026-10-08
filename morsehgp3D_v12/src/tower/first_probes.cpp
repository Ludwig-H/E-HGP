// Premieres sondes en masse de l'ordre k (levier G-L5, CONTRAT_TOUR.md, paragraphe 4.3) : 71 % des representants
// s'arretent a leur premiere sonde ; au lieu d'une sonde par representant, une jointure triee des premieres traces et
// de l'index des naissances (populations.cpp) par leur empreinte additive. (1) Empreintes des traces, une passe sur
// les cellules dans l'ordre (I une fois par cellule, puis les sites de A par leur masque), sans former les parties ;
// (2) tri par base stable des (empreinte, representant) sur les bits du seau de l'index ; (3) fusion seau par seau :
// chaque trace recoit la premiere place de son seau de meme empreinte, trouvee par dichotomie dans le seau (borne
// O(log E) meme si toutes les empreintes sont egales : jamais de produit de deux groupes egaux), ou kNone. Voie de
// l'appareil : jugee plus lente sur l'hote en local que la file de sondes prechargees (G-L7, passes.cpp), elle n'est
// jouee que par un bras de mesure du pilote (microbancs/mes_t2c_g). Rien n'est decide ici :
// la resolution verifie le candidat sur les SiteIdx (PopulationTable::verify) ; un candidat faux (collision
// d'empreintes) ou absent laisse le representant a la resolution pas a pas, premiere sonde jouee. Ecritures disjointes
// (une case par representant), places du tri fixees par les donnees : memes candidats a tout nombre de fils. Ecrit pour
// l'appareil (empreintes, tri par base, fusion).
#include <algorithm>

#include "tower/internal.hpp"

namespace mhgp12::tower_detail {
namespace {

constexpr u64 kKeyGrain = 256;      // cellules par tranche des empreintes
constexpr u64 kMergeGrain = 16384;  // traces triees par tranche de fusion

struct TraceKeys {
  const Catalogue& catalogue;
  const ResolvedOrder& order;
  u64 mask;
  std::span<KeyedEntry> entries;
  static Outcome body(void* raw, u64 begin, u64 end, u32) noexcept {
    auto& s = *static_cast<TraceKeys*>(raw);
    const auto balls = s.order.cell_balls();
    const auto offsets = s.order.cell_offsets();
    const auto masks = s.order.trace_masks();
    std::array<u64, kMaxShell> shell_keys;
    for (u64 c = begin; c < end; ++c) {
      const auto inner = s.catalogue.interior(balls[c]), shell = s.catalogue.shell(balls[c]);
      if (shell.size() > kMaxShell) return fail(Reason::tower_invariant, s.order.order());
      u64 base = 0;
      for (const SiteIdx site : inner) base += site_key(idx(site));
      for (std::size_t j = 0; j < shell.size(); ++j) shell_keys[j] = site_key(idx(shell[j]));
      const u64 outside = shell.size() == kMaxShell ? 0 : ~u64{0} << shell.size();
      for (u64 r = offsets[c]; r < offsets[c + 1]; ++r) {
        if ((masks[r] & outside) != 0) return fail(Reason::tower_invariant, s.order.order());
        u64 key = base;
        for (u64 rest = masks[r]; rest != 0; rest &= rest - 1) key += shell_keys[__builtin_ctzll(rest)];
        s.entries[r] = KeyedEntry{key & s.mask, static_cast<u32>(r), 0};
      }
    }
    return {};
  }
};

struct Merge {
  const PopulationTable& table;
  std::span<const KeyedEntry> sorted;
  std::span<u32> candidates;
  static Outcome body(void* raw, u64 begin, u64 end, u32) noexcept {
    auto& s = *static_cast<Merge*>(raw);
    const u64 hi = std::min<u64>(s.sorted.size(), end * kMergeGrain);
    for (u64 t = begin * kMergeGrain; t < hi; ++t) s.candidates[s.sorted[t].index] = s.table.candidate(s.sorted[t].key);
    return {};
  }
};

}  // namespace

u64 first_probe_bytes(u64 representatives) noexcept {
  return 2 * representatives * sizeof(KeyedEntry) + representatives * sizeof(u32) + radix_bytes(representatives);
}

Outcome first_probe_candidates(const Domain& d, const ResolvedOrder& order, const PopulationTable& table,
                               JoinBuffers& buffers, MemoryBudget& budget, sched::Pool& pool) noexcept {
  const u64 reps = order.representatives();
  if (reps >= kNone) return fail(Reason::tower_invariant, order.order());
  if (buffers.entries.size() < reps) MHGP12_TRY(buffers.entries.allocate(reps, budget));
  if (buffers.scratch.size() < reps) MHGP12_TRY(buffers.scratch.allocate(reps, budget));
  if (buffers.candidates.size() < reps) MHGP12_TRY(buffers.candidates.allocate(reps, budget));
  const auto entries = buffers.entries.span().first(reps);
  TraceKeys keys{d.catalogue, order, table.key_mask(), entries};
  MHGP12_TRY(pool.parallel_for(order.cells(), kKeyGrain, &keys, &TraceKeys::body));
  auto sorted = radix_sort(entries, buffers.scratch.span().first(reps), table.shift(), buffers.places, budget, pool);
  if (!sorted.ok()) return sorted.outcome();
  Merge merge{table, sorted.value(), buffers.candidates.span().first(reps)};
  return pool.parallel_for((reps + kMergeGrain - 1) / kMergeGrain, 1, &merge, &Merge::body);
}

}  // namespace mhgp12::tower_detail
