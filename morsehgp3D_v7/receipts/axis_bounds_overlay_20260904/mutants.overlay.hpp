// MorseHGP3D v6 — registre des MUTANTS.
//
// Doctrine du depot : chaque defaut connu ou historique a un mutant qui le
// reintroduit, et une porte qui le tue (code 4). En v4 les mutants etaient
// des booleens enfiles a travers les signatures de production, ou des copies
// locales du code dans les probes (un mutant sur une copie ne prouve rien
// sur la bibliotheque). En v6 :
//
//   - la CLI remplit le registre UNE fois (`mutants_enable("a,b")`) avant
//     tout calcul ; elle refuse (code 2) un nom absent de `kMutants` ;
//   - chaque point d'injection dans le code de PRODUCTION s'ecrit
//     `if (MHGP7_MUTANT("nom")) { ... }` — greppable, la liste des points
//     d'injection est `grep -rn MHGP7_MUTANT src/` et doit egaler `kMutants` ;
//   - la lecture est un booleen statique par site : cout nul dans les
//     boucles chaudes.
//
// Un mutant n'est JAMAIS une option de production : les points d'injection ne
// sont COMPILES que dans les cibles de test (`MHGP7_TESTING`, pose par CMake sur
// les executables de tests/) ; dans un binaire produit, `MHGP7_MUTANT` est la
// constante false et `mutants_enable` refuse tout nom — aucun run produit ne
// peut porter un mutant, et le cout dans les boucles chaudes est nul.
#pragma once

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace mhgp7 {

// La liste EXHAUSTIVE des mutants declares (source unique). Toute entree a
// au moins un point d'injection dans src/ (ou tests/ pour un mutant
// d'oracle) et au moins une porte CTest a code 4 ; un nom n'entre ici
// qu'AVEC son point d'injection (`mhgp7_mutants_gate`), jamais par avance.
inline constexpr const char* kMutants[] = {
    // REGISTRE = exactement les points d'injection presents (grep -rn MHGP7_MUTANT
    // src/ cli/ oracle/, hors exemples de commentaires). Un nom sans point
    // d'injection n'entre au registre qu'avec son code (doctrine v6 : chaque nom
    // = un point d'injection + une porte code 4).
    "family-scanline-overshoot", "wspd-drop-rect", "wspd-cap-terminal", "wspd-split-heaviest",
    "wspd-wide-drop-k2-mid", "core-ball-ceil-distance", "witness-no-lane-mask", "cover-rect-dmin",
    "cover-envelope-open", "cover-envelope-factor", "q3-prune-ge", "q3-level-4g",
    "q4-no-canonical", "q4-center-parity", "q4-cover-coef3", "q4-seed-core-nonstrict", "q4-eq-nonstrict",
    "q4-eq-sign", "q4-i64-drop-factor", "q4-i64-pair-min", "jung-swap-bounds",
    "level-trunc-hi", "dint-mulhi-dropped", "rle-drop", "census-nonstrict",
    "genfilter-nonstrict", "depth-threshold-minus-one", "range-add-max-le-zero", "skip-full-census",
    "binary-ties", "repr-ties", "attach-prebatch", "drop-nonmerge",
    "dense-pointid", "canonical-is-uf-root", "attach-detector-disabled", "par-drop-shard", "caps-drop-emission", "caps-late-wave-check", "caps-skip-prefusion-budget", "pool-serial", "pool-drop-exception", "pool-worker-resume-after-fatal", "pool-activate-after-unlock",
    "witness-di128-lost-carry", "witness-skip-write", "witness-skip-native-write",
    "gpu-index-drop-node", "wire-t1-plus-one", "gpu-range-add-le", "gpu-stack-shallow",
    "gpu-swap-push-order", "gpu-census-nonstrict", "gpu-skip-ball-write", "gpu-nshell-overdomain", "gpu-skip-count-write", "gpu-lot-base-reset",
    "par-drop-ball-chunk", "parallel-one-worker", "parallel-sort-unstable", "fold-inject-a-failure-k2",
    "fold-inject-b-exception-k3", "render-active-only", "render-collapse-mult", "obig-carry-lost",
    "sector-kill-nonstrict", "sector-credit-inbox", "sector-credit-global", "anchor-kill-h-minus-one",
    "chord-nonstrict", "chord-skip-positive", "chord-dead-skip-positive", "cell-kill-nonstrict",
    "cell-kill-h-minus-one", "prefix-tamper-event-order", "prefix-tamper-batch-levels", "cell-locate-eps-zero",
    "fused-mask-stuck", "sweep-drop-exit-root", "sweep-nonstrict-depth",
    // SONDES D'ABLATION du reduce (2 septembre, arbre § 5.10) : decomposer
    // materialisation_tri_copie / post_remplissage AVANT tout palier. Chaque
    // ablation CHANGE l'objet (tuee code 4 par la conformite) : elle ne peut
    // ni survivre dans une porte ni exister dans un binaire produit.
    "ablation-mat-sans-copie", "ablation-mat-sans-tris", "ablation-post-cle-factice",
    // CSR de FacetKey (2 septembre, GO exploratoire COMPACTDELTA_CSR) : chaque
    // site vit EXCLUSIVEMENT dans la branche csr de reduce_fold ; tue par
    // tests/fold_csr_gate.cpp (first_divergence sur le champ ANNONCE, sinon code 1)
    // et, pour ceux qui CHANGENT l'objet, par la conformite --layout=csr. Les
    // csr-offset-* (refus avant vue) et csr-guard-skip / csr-inject-bad-alloc
    // (gardes de capacite, panne d'allocation injectee) ne sont tues que par
    // leur porte dediee : sous la conformite, tout statut non complet vaut 4
    // par vacuite de refus, ce qui ne prouverait rien.
    "csr-order-by-output", "csr-keep-continuation", "csr-stale-level", "csr-stale-output",
    "csr-unsorted-born", "csr-unsorted-parents", "csr-drop-delta", "csr-dup-delta", "csr-shift-offset",
    "csr-offset-hole", "csr-offset-overlap", "csr-offset-end", "csr-offset-domain", "csr-guard-skip",
    "csr-inject-bad-alloc",
    // ETAGE NOMME D'UN bad_alloc (2 septembre, ALERTE_G4_ECHELLE_V6) : panne
    // d'allocation injectee dans src/pipeline/run.hpp a DEUX etages
    // differents — l'un sur le fil principal (census), l'autre dans un WORKER
    // de l'etage B au premier ordre (fold). Tues par tests/bad_alloc_gate.cpp :
    // le refus doit etre resource_exhausted, NOMMER l'etage, ne publier aucun
    // callback ni provisoire, et ne JAMAIS terminer par signal (code 134).
    "caps-throw-bad-alloc-provision", "caps-throw-bad-alloc-census", "caps-throw-bad-alloc-fold",
    // JALONS DE RESIDENCE (2 septembre, palier P2) : le defaut historique des
    // six `rss_mb` — un INSTANTANE la ou il faut un PIC. Point d'injection dans
    // src/pipeline/run.hpp (vm_hwm_mb_now), tue code 4 par tests/residence_gate.cpp
    // (le pic cesse d'etre non decroissant des que le fold libere sa memoire).
    "hwm-instant-rss",
    // ENCODEUR PUR A OFFSETS FIXES (2 septembre, C6 jalon 2,
    // REPONSE_AUDITEUR_CONCEPTION_C6_20260902) : points d'injection dans
    // src/gpu/wire.hpp (pack_write_offset, pack_prevalidate), tues code 4 par
    // tests/wire_pack_gate.cpp. `wire-pack-stride-short` : le pas d'ecriture
    // n'est plus 112 — les offsets cessent d'etre fixes (divergence octet
    // pour octet contre append_ball_in). `wire-pack-slack-size` : la
    // prevalidation tolere une boule au-dela du tampon — le refus de
    // capacite disparait (dent SEPAREE : aucun octet ne diverge).
    "wire-pack-stride-short", "wire-pack-slack-size",
    // ANNEAU DE LOTS ET CONTRAT DES BAUX (2 septembre, C6 jalons 1 et 3,
    // REPONSE_AUDITEUR_CONCEPTION_C6_20260902) : points d'injection dans
    // src/gpu/lot_ring.hpp, tues code 4 par tests/lot_ring_gate.cpp, chacun par
    // sa scene-signature. c6-reuse-before-lease : un emplacement est repris
    // alors que son bail court encore ; c6-merge-by-completion : la retraite
    // suit l'ordre d'achevement au lieu de base_global ; c6-wrong-epoch : au
    // bouclage, le bail garde l'epoque et la base de l'occupant precedent ;
    // c6-publish-prefix : l'echange terminal a lieu malgre le refus ;
    // c6-rebuild-before-validate : la reconstruction accepte un lot pas
    // entierement valide.
    "c6-reuse-before-lease", "c6-merge-by-completion", "c6-wrong-epoch", "c6-publish-prefix",
    "c6-rebuild-before-validate",
    // GARDES DURES DU FOLD (palier P5) : la garde de capacite est sautee, donc
    // un ordre au-dela du plafond passe. Tue code 4 par tests/fold_caps_gate.cpp
    // (le refus ATTENDU devient absent), plafonds abaisses par les crochets
    // fold_events_cap_for_tests / fold_incidences_cap_for_tests.
    "caps-fold-guard-skip",
    // CONTRE-CORRECTIONS DES PALIERS P2/P3 (2 septembre, retours des deux
    // relecteurs). Points d'injection dans src/pipeline/run.hpp et
    // src/pipeline/expand.hpp, tues code 4 par tests/selftest.cpp
    // (--failure-contract), tests/bad_alloc_gate.cpp et
    // tests/residence_gate.cpp.
    //  - `provisional-keep-sum-parents` : invalidate_provisional n'efface plus
    //    Sigma|parents| — un PREFIXE EXACT de payload (le Sigma|parents| des
    //    forets deja publiees) survivrait a un refus transactionnel.
    //  - `drop-stage-milestone` : le jalon apres_rle n'est plus releve du tout
    //    (ni rss, ni hwm) — exactement ce qu'une route future ferait par
    //    accident ; tue par le plancher de JALONS JUGES de la porte de
    //    residence, sans lequel elle serait verte par vacuite.
    //  - `keep-ball-chunks` : les trois liberations par tranche du palier P3
    //    (prefiltre, census, expansion) sont retirees — la fusion reporte
    //    DEUX FOIS la residence de l'etage ; tue par le PLAFOND d'increment de
    //    pic du census de la porte de residence.
    "provisional-keep-sum-parents", "drop-stage-milestone", "keep-ball-chunks",
    "perm-apply-scatter", "perm-apply-partial", "perm-tie-desc", "census-stack-per-ball",
    "parallel-admit-partial-launch", "census-direct-offset", "census-direct-publish-prefix",
    "silent-drop-coface",
    "reduced-latent-parent", "reduced-drop-materialization",
    "axis-argmin-floor-only", "axis-argmin-ceil-always", "axis-argmin-no-clip",
    "axis-argmin-narrow-coefficient", "axis-argmin-max-min",
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
#if !defined(MHGP7_TESTING)
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

}  // namespace mhgp7

// Point d'injection : lecture memorisee par site (le registre est fige avant
// tout calcul). Hors cible de test ou en code device : constante false.
#if defined(__CUDA_ARCH__) || !defined(MHGP7_TESTING)
#define MHGP7_MUTANT(name) (false)
#else
#define MHGP7_MUTANT(name)                                    \
  ([]() -> bool {                                             \
    static const bool on = ::mhgp7::mutant_enabled(name);     \
    return on;                                                \
  }())
#endif
