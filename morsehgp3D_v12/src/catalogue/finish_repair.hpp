// Fin d'etage, repli exact des chaines mal ordonnees (rare), ecrit une fois pour les deux executeurs. Une chaine est
// une suite maximale de voisins non certainement ordonnes par F4 (cles F3 egales ou voisines) qui contient une paire
// mal ordonnee ; elle est retriee en exact sur l'hote par (niveau exact, positions de S*), memes comparaisons que
// ChainKernel, puis l'ordre et les cles sont reecrits chaine par chaine et tout est reverifie (finish_driver.hpp).
//
// Rapatriement compact (tranche T2-d) : les positions des M paires mal ordonnees sont compactees sur l'executeur
// (drapeaux, somme prefixe, ecriture), puis chaque chaine est bornee en lisant ses cles par fenetres de kChainWindow
// cles de part et d'autre de la paire, doublees tant qu'une borne touche le bord de la fenetre, puis l'ordre et les
// cles des R elements des chaines passent sur l'hote : 4 M + 8 S + 12 R octets lus (S : somme des fenetres lues ;
// bounds reserve 2 M cases), contre verdicts, ordre et cles des n boules avant (16 octets par boule). Beaucoup de
// petites chaines peuvent couter plus : aucun gain n'est garanti.
#pragma once

#include <algorithm>

#include "catalogue/finish_kernels.hpp"

namespace mhgp12::catalogue_detail::fin {

inline constexpr u64 kChainWindow = 64;  // cles lues de part et d'autre d'une paire mal ordonnee (fenetre initiale)

// Drapeau de chaque paire mal ordonnee (entree de la somme prefixe des positions).
struct MisorderKernel {
  const u32* verdict;
  u64 n;
  u64* flags;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    for (u32 c = 0; c < kChunks; ++c) {
      MHGP12_LANES(kWarp, l) {
        const u64 i = item(tile, c, l);
        if (i < n) flags[i] = verdict[i] == kPairMisordered ? 1u : 0u;
      }
    }
  }
};

// Positions des paires mal ordonnees, dans l'ordre (rang de chaque position : prefixe exclusif des drapeaux).
struct PositionKernel {
  const u32* verdict;
  const u64* at;
  u64 n;
  u32* positions;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    for (u32 c = 0; c < kChunks; ++c) {
      MHGP12_LANES(kWarp, l) {
        const u64 i = item(tile, c, l);
        if (i < n && verdict[i] == kPairMisordered) positions[at[i]] = static_cast<u32>(i);
      }
    }
  }
};

// Chaines du repli exact : bornes [s, e) (2 par chaine) et elements (boule et cle F3 d'origine), chaine apres chaine.
struct Chains {
  Buffer<u64> bounds;
  Buffer<u32> balls;
  Buffer<u64> keys;
  u64 count = 0, elements = 0;
};

// Chaine de voisins incertains autour de la paire (i - 1, i), 1 <= i < n : bornes [s, e), cles lues par fenetres
// doublees tant qu'une borne touche un bord de fenetre qui n'est pas un bord du tableau (la fenetre entiere ne l'est
// jamais : la boucle s'arrete).
template <class B, class K>
Outcome chain_bounds(B& b, K& keys_array, u64 n, u64 i, u64& s, u64& e, MemoryBudget& budget) noexcept {
  for (u64 w = kChainWindow < n ? kChainWindow : n;; w = 2 * w < n ? 2 * w : n) {
    const u64 lo = i > w ? i - w : 0, hi = n - i > w ? i + w : n;
    Buffer<u64> keys;
    MHGP12_TRY(keys.allocate(hi - lo, budget));
    MHGP12_TRY(b.download(keys.data(), keys_array, hi - lo, lo));
    s = i - 1;
    while (s > lo && key_order(keys[s - 1 - lo], keys[s - lo]) == 0) --s;
    e = i + 1;
    while (e < hi && key_order(keys[e - 1 - lo], keys[e - lo]) == 0) ++e;
    if ((s > lo || lo == 0) && (e < hi || hi == n)) return {};
  }
}

// Chaines autour des paires mal ordonnees : positions compactees sur l'executeur, bornes par fenetres, puis ordre et
// cles de chaque chaine (deux lectures par chaine).
template <class B, class A>
Outcome collect_chains(B& b, A& a, int ck, u64 n, Chains& out, MemoryBudget& budget) noexcept {
  MHGP12_TRY(b.launch(MisorderKernel{a.verdict.data(), n, a.before.data()}, tiles_of(n)));
  const auto m = exclusive_scan(b, a.scan, a.before.data(), a.offset.data(), n);
  if (!m.ok()) return m.outcome();
  MHGP12_TRY(b.ensure(a.pick, m.value()));
  MHGP12_TRY(b.launch(PositionKernel{a.verdict.data(), a.offset.data(), n, a.pick.data()}, tiles_of(n)));
  Buffer<u32> positions;
  MHGP12_TRY(positions.allocate(m.value(), budget));
  if (m.value() != 0) MHGP12_TRY(b.download(positions.data(), a.pick, m.value(), 0));
  MHGP12_TRY(out.bounds.allocate(2 * m.value(), budget));
  u64 covered = 0, chains = 0, count = 0;
  for (u64 k = 0; k < m.value(); ++k) {
    const u64 i = positions[k];
    if (i < covered) continue;
    if (i == 0) return fail(Reason::catalogue_invariant);  // la position 0 n'a pas de voisin a gauche
    u64 s = 0, e = 0;
    MHGP12_TRY(chain_bounds(b, a.keys.keys[ck], n, i, s, e, budget));
    out.bounds[2 * chains] = s;
    out.bounds[2 * chains + 1] = e;
    covered = e;
    ++chains;
    count += e - s;
  }
  MHGP12_TRY(out.balls.allocate(count, budget));
  MHGP12_TRY(out.keys.allocate(count, budget));
  for (u64 c = 0, at = 0; c < chains; ++c) {
    const u64 s = out.bounds[2 * c], e = out.bounds[2 * c + 1];
    MHGP12_TRY(b.download(out.balls.data() + at, a.keys.vals[ck], e - s, s));
    MHGP12_TRY(b.download(out.keys.data() + at, a.keys.keys[ck], e - s, s));
    at += e - s;
  }
  out.count = chains;
  out.elements = count;
  return {};
}

// Repli exact (rare) : les chaines de voisins incertains mal ordonnees sont retriees sur l'hote par (niveau exact,
// positions de S*), memes comparaisons que ChainKernel ; leurs niveaux et cles de positions sont rassembles par
// l'executeur (PickKernel), puis ordre et cles reecrits chaine par chaine. A : FinishArrays<B> (finish_driver.hpp) ;
// chaines et elements retries ajoutes a repaired et elements.
template <class B, class A>
Outcome finish_repair(B& b, A& a, int ck, u64 n, MemoryBudget& budget, u64& repaired, u64& elements) noexcept {
  Chains chains;
  MHGP12_TRY(collect_chains(b, a, ck, n, chains, budget));
  const u64 count = chains.elements;
  MHGP12_TRY(b.ensure(a.pick, count));
  MHGP12_TRY(b.ensure(a.pick_levels, count));
  MHGP12_TRY(b.ensure(a.pick_pkey, count));
  MHGP12_TRY(b.upload(a.pick, chains.balls.data(), count, 0));
  MHGP12_TRY(b.launch(PickKernel{a.pick.data(), count, a.levels.data(), a.pkey.data(), a.pick_levels.data(),
                                 a.pick_pkey.data()},
                      tiles_of(count)));
  Buffer<LevelWords> levels;
  Buffer<Key2> pkeys;
  Buffer<u32> rank, order;
  Buffer<u64> keys;
  MHGP12_TRY(levels.allocate(count, budget));
  MHGP12_TRY(pkeys.allocate(count, budget));
  MHGP12_TRY(rank.allocate(count, budget));
  MHGP12_TRY(order.allocate(count, budget));
  MHGP12_TRY(keys.allocate(count, budget));
  MHGP12_TRY(b.download(levels.data(), a.pick_levels, count, 0));
  MHGP12_TRY(b.download(pkeys.data(), a.pick_pkey, count, 0));
  for (u64 c = 0, at = 0; c < chains.count; ++c) {
    const u64 s = chains.bounds[2 * c], e = chains.bounds[2 * c + 1];
    for (u64 j = 0; j < e - s; ++j) rank[at + j] = static_cast<u32>(at + j);
    std::sort(rank.data() + at, rank.data() + at + (e - s), [&](u32 x, u32 y) {
      const int level = compare_levels(levels[x], levels[y]);
      return level != 0 ? level < 0 : key2_cmp(pkeys[x], pkeys[y]) < 0;
    });
    for (u64 j = at; j < at + (e - s); ++j) {
      order[j] = chains.balls[rank[j]];
      keys[j] = chains.keys[rank[j]];
    }
    MHGP12_TRY(b.upload(a.keys.vals[ck], order.data() + at, e - s, s));
    MHGP12_TRY(b.upload(a.keys.keys[ck], keys.data() + at, e - s, s));
    at += e - s;
  }
  repaired += chains.count;
  elements += count;
  return {};
}

}  // namespace mhgp12::catalogue_detail::fin
