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
// Un mutant n'est JAMAIS une option de production : le registre est vide
// dans tout chemin nominal.
#pragma once

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace mhgp5 {

// La liste EXHAUSTIVE des mutants declares (source unique). Toute entree a
// exactement un point d'injection dans src/ et au moins une porte CTest a
// code 4. `mhgp5_mutants_gate` verifie la premiere moitie par grep.
inline constexpr const char* kMutants[] = {
    // familles
    "family-scanline-overshoot",
    // wspd
    "wspd-drop-rect", "wspd-cap-terminal", "wspd-split-heaviest",
    // fuseaux / temoins
    "core-ball-ceil-distance", "witness-no-lane-mask",
    // lanes
    "q2-radius-ceil", "q3-prune-ge", "q3-sign-p", "q3-cramer-swap", "q3-level-4g",
    "q4-seeds-from-q3-live", "q4-cover-coef3", "q4-no-canonical", "q4-center-parity",
    "q4-seed-core-nonstrict", "q4-eq-nonstrict", "q4-eq-sign", "q4-i64-drop-factor",
    "q4-i64-pair-min", "jung-swap-bounds",
    // entiers larges
    "level-trunc-hi", "mul-carry-lost",
    // flux / census / plateaux
    "rle-drop", "census-nonstrict", "genfilter-nonstrict", "depth-threshold-minus-one",
    "range-add-max-le-zero", "skip-full-census", "shell-cap-before-depth",
    "drop-shell-plateau",
    // foret
    "binary-ties", "repr-ties", "attach-prebatch", "drop-nonmerge", "dense-pointid",
    "canonical-is-uf-root", "fold-hardcodes-kmax10", "attach-detector-disabled",
    // rendu
    "render-active-only", "render-collapse-mult", "birth-from-events",
    // parallelisme
    "par-drop-shard", "par-drop-ball-chunk", "parallel-one-worker",
    // sortie / budget
    "budget-events-only", "birth-dup-tau",
    // oracle
    "obig-carry-lost",
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
// inconnu : l'appelant refuse avec le code 2.
inline bool mutants_enable(std::string_view csv) {
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
}

inline bool mutant_enabled(std::string_view name) {
  for (const std::string& m : mutant_registry())
    if (m == name) return true;
  return false;
}

inline bool mutants_any() { return !mutant_registry().empty(); }

}  // namespace mhgp5

// Point d'injection : lecture memorisee par site (le registre est fige avant
// tout calcul). En code device, aucun mutant n'existe : constante false.
#if defined(__CUDA_ARCH__)
#define MHGP5_MUTANT(name) (false)
#else
#define MHGP5_MUTANT(name)                                    \
  ([]() -> bool {                                             \
    static const bool on = ::mhgp5::mutant_enabled(name);     \
    return on;                                                \
  }())
#endif
