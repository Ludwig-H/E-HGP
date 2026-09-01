// MorseHGP3D v6 — PLAFONDS STRUCTURELS DECLARES du moteur in-memory.
//
// Doctrine (dette d'echelle, audit + balayage du 1er septembre 2026) : toute
// limite du moteur se DECLARE ici et se refuse par un statut transactionnel
// `resource_exhausted` — jamais un wrap d'entier, jamais un OOM silencieux
// la ou un refus type est possible. La verification precede l'ALLOCATION
// GLOBALE ou le tampon effectivement garde (fusion, tri, census, fold) ;
// les shards locaux, eux, materialisent jusqu'a l'observation cooperative
// (cap de cardinalite a overshoot borne). Ces plafonds decrivent le moteur EN MEMOIRE ; le chemin vers les
// dizaines de millions de points est le design streame (ECHELLE.md § 3), pas
// un relachement de ces constantes.
//
// Le budget memoire optionnel est un budget PARTIEL DE TAMPONS DECLARES
// (tri, prefiltre/census conservatif, evenements du fold x (inflight+2)) —
// c'est un PROXY DE PAYLOAD LOGIQUE NOMME (gardes sur les cardinalites
// exactes), il ne promet PAS l'absence d'OOM globale
// (ordres de tri, tables et pools du fold non comptes).
// Residences dominantes mesurees (balayage verifie par sondes compilees,
// reçus campagne_decision_20260831 : ~425 candidats bruts/point a n=32000,
// croissant avec n) :
//   BallCandidate ~144 o  (flux brut, PUIS x2 transitoire au tri fusion)
//   MultiAliveRect ~40 o  (rectangles terminaux du front fusionne)
//   Task           ~12 o  (files de vagues de la descente)
//   Survivor        16 o, BallData ~224 o, ForestEvent ~136 o x (inflight+2)
// => mur in-memory en ORDRE DE GRANDEUR EXTRAPOLE (~1,6-3,2 M points sur
// 180 GiB), TRES en dessous du
// plafond d'indices u32 : sans refus amont, l'OOM precede le plafond.
#pragma once

#include "types.hpp"

namespace mhgp6 {

// Positions d'entree (donc positions uniques m <= n) : l'arbre radix code
// les feuilles en NodeRef i32 (-1-u) et la recherche de Karras travaille en
// int (lmax double jusqu'a ~2m) — 2^30-1 garde toute l'arithmetique dans
// i32 avec marge, et couvre largement la cible 5e7 du design streame.
inline constexpr u64 kMaxTreePositions = (1ull << 30) - 1;

// Candidats BRUTS emis par la generation : le prefiltre indexe en u32 ;
// le plafond est REMONTE a l'emission (cap de CARDINALITE a overshoot
// borne, arret cooperatif AVANT la fusion globale et le tri — les shards
// locaux materialisent jusqu'a l'observation du drapeau) : avant ce
// remontage, l'OOM arrivait avant que le plafond aval ne tire.
inline constexpr u64 kMaxRawCandidates = 0xFFFFFFFFull;

// Taches de vague et rectangles terminaux du front fusionne : bornes
// verifiees a CHAQUE fusion de vague (comptes exacts, cout nul).
inline constexpr u64 kMaxWaveTasks = 0xFFFFFFFFull;
inline constexpr u64 kMaxAliveRects = 0xFFFFFFFFull;

// Codes de refus de la generation (GenerateStats::cap_refus) — mappes vers
// des messages `resource_exhausted : ...` par run_pipeline.
// Arithmetique de garde CONTROLEE (jamais un wrap silencieux dans un calcul
// d'octets) : multiplication testee par division, produit triple borne.
inline constexpr bool mul_would_overflow_u64(u64 a, u64 b) {
  return b != 0 && a > ~0ull / b;
}
// cout(count x bytes_each x factor) <= budget ? — un overflow du produit
// vaut refus (fail-closed), l'egalite exacte passe.
inline constexpr bool fits_budget(u64 count, u64 bytes_each, u64 factor, u64 budget) {
  if (mul_would_overflow_u64(count, bytes_each)) return false;
  const u64 cb = count * bytes_each;
  if (mul_would_overflow_u64(cb, factor)) return false;
  return cb * factor <= budget;
}

inline constexpr u64 kCapRefusNone = 0;
inline constexpr u64 kCapRefusRawCandidates = 1;
inline constexpr u64 kCapRefusWaveTasks = 2;
inline constexpr u64 kCapRefusAliveRects = 3;

}  // namespace mhgp6
