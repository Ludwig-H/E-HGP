// MorseHGP3D v5 — registre des MUTANTS.
//
// Doctrine du depot : chaque defaut connu ou historique a un mutant qui le
// reintroduit, et une porte qui le tue (code 4). En v4 les mutants etaient
// des booleens enfiles a travers les signatures de production, ou des copies
// locales du code dans les probes (un mutant sur une copie ne prouve rien
// sur la bibliotheque). En v5 :
//
//   - la CLI remplit le registre UNE fois (`mutants_enable("a,b")`) avant
//     tout calcul ; elle refuse (code 2) un nom absent de `kMutants` ;
//   - chaque point d'injection dans le code de PRODUCTION s'ecrit
//     `if (MHGP5_MUTANT("nom")) { ... }` — greppable, la liste des points
//     d'injection est `grep -rn MHGP5_MUTANT src/` et doit egaler `kMutants` ;
//   - la lecture est un booleen statique par site : cout nul dans les
//     boucles chaudes.
//
// Un mutant n'est JAMAIS une option de production : les points d'injection ne
// sont COMPILES que dans les cibles de test (`MHGP5_TESTING`, pose par CMake sur
// les executables de tests/) ; dans un binaire produit, `MHGP5_MUTANT` est la
// constante false et `mutants_enable` refuse tout nom — aucun run produit ne
// peut porter un mutant, et le cout dans les boucles chaudes est nul.
#pragma once

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace mhgp5 {

// La liste EXHAUSTIVE des mutants declares (source unique). Toute entree a
// au moins un point d'injection dans src/ (ou tests/ pour un mutant
// d'oracle) et au moins une porte CTest a code 4 ; un nom n'entre ici
// qu'AVEC son point d'injection (`mhgp5_mutants_gate`), jamais par avance.
inline constexpr const char* kMutants[] = {
    // familles
    "family-scanline-overshoot",
    // wspd
    "wspd-drop-rect", "wspd-cap-terminal", "wspd-split-heaviest", "wspd-wide-drop-k2-mid",
    // fuseaux / temoins / cover
    "core-ball-ceil-distance", "witness-no-lane-mask", "cover-rect-dmin", "cover-envelope-open",
    "cover-envelope-factor",
    // lanes
    "q3-prune-ge", "q3-level-4g",
    "q4-seeds-from-q3-live", "q4-cover-coef4", "q4-no-canonical", "q4-center-parity",
    "q4-seed-core-nonstrict", "q4-eq-nonstrict", "q4-eq-sign", "q4-i64-drop-factor",
    "q4-i64-pair-min", "jung-swap-bounds",
    // entiers larges
    "level-trunc-hi", "dint-mulhi-dropped",
    // flux / census / plateaux
    "rle-drop", "census-nonstrict", "genfilter-nonstrict", "depth-threshold-minus-one",
    "range-add-max-le-zero", "skip-full-census",
    // foret
    "binary-ties", "repr-ties", "attach-prebatch", "drop-nonmerge", "dense-pointid",
    "canonical-is-uf-root", "attach-detector-disabled",
    // parallelisme
    "par-drop-shard", "par-drop-ball-chunk", "parallel-one-worker", "parallel-sort-unstable",
    // fold concurrent (src/pipeline/run.hpp) : defaut d'etage A injecte a K=2, exception du fil B a K=3
    "fold-inject-a-failure-k2", "fold-inject-b-exception-k3",
    // instrument device, partie hote (src/gpu/device_stats.hpp, src/gpu/q3_lane_batched.hpp)
    "gauge-no-peak", "log2hist-class-shift",
    // rendu
    "render-active-only", "render-collapse-mult", "birth-from-events",
    // oracle et portes (points d'injection dans oracle/ et tests/)
    "obig-carry-lost", "float-small-threshold", "q3-sign-p", "q3-cramer-swap", "q3-shaped-strict-flip",
    "q3-batched-emit-dead",
    "witness-no-warp-correction",
    "q4-shaped-jung-skip-kills",
    "q4-shaped-once-flip",
    "q4-batched-emit-deep",
    "route-ignore-threshold",
    "sector-kill-nonstrict",
    "anchor-kill-h-minus-one",
    "chord-nonstrict", "chord-skip-positive",
    "cell-kill-nonstrict",
    "cell-kill-h-minus-one",
    "prefix-tamper-event-order",
    "prefix-tamper-batch-levels",
    "pool-serial",
    "pool-drop-exception",
    "cell-locate-eps-zero",
    // reducteur vivant (src/forest/fold_live.hpp, ECHELLE L2 / theoreme T6)
    "physical-root-is-logical-root", "free-on-absorb", "root-key-mutable", "canon-not-min-on-union",
    "last-mark-shifted", "slot-cap-minus-one",
    // wire G1 : retombee silencieuse du wire index sur SoA (src/gpu/q3_lane_device.cuh, q4_lane_device.cuh)
    "wire-index-force-soa",
    // raffinement post-separation (src/pipeline/generate.hpp)
    "postsep-drop-child", "postsep-duplicate-child", "postsep-kill-h-minus-one", "postsep-refine-q2",
    "postsep-core-without-corners",
};

inline std::vector<std::string>& mutant_registry() {
  static std::vector<std::string> reg;
  return reg;
}

inline bool mutant_known(std::string_view name) {
  for (const char* m : kMutants)
    if (name == m) return true;
  return false;
}

// Active une liste `a,b,c`. Rend false (sans rien activer) si un nom est
// inconnu — ou, hors cible de test, TOUJOURS : l'appelant refuse avec le code 2.
inline bool mutants_enable(std::string_view csv) {
#if !defined(MHGP5_TESTING)
  (void)csv;
  return false;
#else
  std::vector<std::string> names;
  size_t start = 0;
  while (start <= csv.size()) {
    const size_t comma = csv.find(',', start);
    const std::string_view item = csv.substr(start, comma == std::string_view::npos ? std::string_view::npos : comma - start);
    if (!item.empty()) {
      if (!mutant_known(item)) return false;
      names.emplace_back(item);
    }
    if (comma == std::string_view::npos) break;
    start = comma + 1;
  }
  for (std::string& n : names) mutant_registry().push_back(std::move(n));
  return true;
#endif
}

inline bool mutant_enabled(std::string_view name) {
  for (const std::string& m : mutant_registry())
    if (m == name) return true;
  return false;
}

inline bool mutants_any() { return !mutant_registry().empty(); }

}  // namespace mhgp5

// Point d'injection : lecture memorisee par site (le registre est fige avant
// tout calcul). Hors cible de test ou en code device : constante false.
#if defined(__CUDA_ARCH__) || !defined(MHGP5_TESTING)
#define MHGP5_MUTANT(name) (false)
#else
#define MHGP5_MUTANT(name)                                    \
  ([]() -> bool {                                             \
    static const bool on = ::mhgp5::mutant_enabled(name);     \
    return on;                                                \
  }())
#endif
