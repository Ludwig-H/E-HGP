// MorseHGP3D v6 — candidats de boules et RLE.
//
// Un candidat = (BallKey primitive, representant de niveau, arite du
// generateur). Ordre canonique : cle, puis ARITE MINIMALE, puis plus petite
// REPRESENTATION du niveau (depart deterministe). Le RLE garde, par cle
// unique, le premier candidat de cet ordre : son arite est la cardinalite
// minimale d'un support de la boule (regle de mort par arite minimale en
// aval), son niveau est le representant canonique. Sujet et juge appliquent
// la meme regle : les representants coincident.
#pragma once

#include <algorithm>
#include <vector>

#include "../core/mutants.hpp"
#include "../lanes/keys.hpp"
#include "../lanes/level.hpp"
#include "../parallel/sort.hpp"

namespace mhgp7 {

struct BallCandidate {
  BallKey key;
  ExactLevel level;
  u8 arity;  // 2, 3 ou 4
};

inline bool ball_candidate_less(const BallCandidate& x, const BallCandidate& y) {
  if (x.key != y.key) return x.key < y.key;
  if (x.arity != y.arity) return x.arity < y.arity;
  return x.level < y.level;
}

// Les deux etapes sont exposees separement pour signer, sans copie, le
// multiensemble canonique AVANT RLE. `rle_candidates` conserve l'API historique.
inline size_t sort_candidates(std::vector<BallCandidate>* cands, int threads = 1) {
  return parallel_stable_sort(cands->begin(), cands->end(), ball_candidate_less, threads);
}

inline void deduplicate_candidates(std::vector<BallCandidate>* cands) {
  if (MHGP7_MUTANT("rle-drop")) return;
  cands->erase(std::unique(cands->begin(), cands->end(),
                           [](const BallCandidate& x, const BallCandidate& y) { return x.key == y.key; }),
               cands->end());
}

// Tri stable (parallele, identique a std::stable_sort par contrat) +
// dedoublonnage par cle. Mutant `rle-drop` : le dedoublonnage est saute
// (boules re-censusees, cardinalites doublees — le juge le voit). Retourne
// le nombre d'ouvriers crees par le tri.
inline size_t rle_candidates(std::vector<BallCandidate>* cands, int threads = 1) {
  const size_t workers = sort_candidates(cands, threads);
  deduplicate_candidates(cands);
  return workers;
}

}  // namespace mhgp7

