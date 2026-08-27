// MorseHGP3D v5 — tri STABLE parallele, sortie identique a std::stable_sort.
//
// Contrat : `parallel_stable_sort(first, last, less, threads)` produit
// EXACTEMENT la sequence de `std::stable_sort(first, last, less)` — memes
// elements, meme ordre, y compris entre elements equivalents (le tri stable est
// unique ; la stabilite est le contrat : un tri instable serait une divergence
// de digest). La sortie ne depend donc pas du nombre de fils. La fonction
// RETOURNE le nombre d'ouvriers reellement crees (1 = sequentiel, aucun fil) ;
// c'est cette valeur, jamais le budget demande, que les compteurs publient.
//
// Algorithme (trois phases, une seule equipe de T fils synchronisee par
// std::barrier, tirage dynamique des taches dans chaque phase) :
//   1. T tranches contigues triees par std::stable_sort ;
//   2. ceil(log2 T) niveaux de fusion par paires de tranches adjacentes ; la
//      tranche impaire finale est recopiee ; chaque fusion est decoupee en
//      PIECES par rangs dans la tranche de gauche, la borne correspondante dans
//      la tranche de droite etant `lower_bound` (les equivalents de droite
//      restent a droite : la gauche precede TOUJOURS la droite) ;
//   3. ping-pong entre l'entree et UN tampon de la taille de l'entree, alloue
//      une fois ; la parite du nombre de niveaux est choisie pour que le
//      dernier niveau ecrive dans l'entree (aucune recopie finale).
// Seuil : sous kParallelSortMinElems elements ou threads <= 1, std::stable_sort
// pur (retour 1). Le nombre d'ouvriers passe par `planned_workers` de pool.hpp
// (mutant `parallel-one-worker` : un seul fil quel que soit le budget).
//
// Exigences sur les elements : constructibles par defaut et copiables (le
// tampon est un std::vector<T>). Les elements sont COPIES, jamais deplaces :
// les pieces d'une meme fusion lisent la source concurremment (splitters).
// `less` est copie par ouvrier et doit etre reentrant (capturer un tableau par
// reference constante est l'usage : trier une permutation d'indices). Une
// exception levee par `less` dans un ouvrier termine le processus.
//
// Mutant `parallel-sort-unstable` : la fusion prend la droite d'abord en cas
// d'egalite (tri correct mais instable : la porte parallel_sort_gate le tue).
#pragma once

#include <algorithm>
#include <atomic>
#include <barrier>
#include <cstddef>
#include <iterator>
#include <thread>
#include <vector>

#include "../core/mutants.hpp"
#include "pool.hpp"

namespace mhgp5 {

// Sous ce cardinal, sequentiel pur.
inline constexpr size_t kParallelSortMinElems = (size_t)1 << 15;
// Au moins ce nombre d'elements par tranche initiale (borne le nombre d'ouvriers).
inline constexpr size_t kParallelSortMinSlice = (size_t)1 << 14;
// Grain minimal d'une piece de fusion.
inline constexpr size_t kParallelSortMinGrain = (size_t)1 << 12;

namespace parallel_sort_detail {

// Piece d'une fusion de A=[a0,a1) et B=[b0,b1) (b0 == a1) : la tranche
// [ai, ai1) de A, la tranche de B etant deduite par lower_bound (sauf aux deux
// extremites, fixees a b0 et b1 pour couvrir B entierement).
struct MergePiece {
  size_t a0 = 0, a1 = 0, b0 = 0, b1 = 0;
  size_t ai = 0, ai1 = 0;
  bool first = true, last = true;
};

template <typename SrcIt, typename DstIt, typename Less>
inline void merge_piece(SrcIt src, DstIt dst, const MergePiece& t, Less& less, bool right_first) {
  const SrcIt bfirst = src + (std::ptrdiff_t)t.b0;
  const SrcIt blast = src + (std::ptrdiff_t)t.b1;
  SrcIt a = src + (std::ptrdiff_t)t.ai;
  const SrcIt ae = src + (std::ptrdiff_t)t.ai1;
  // Rang dans B : les elements de B STRICTEMENT inferieurs a A[ai] precedent
  // la piece ; ceux equivalents a A[ai] restent a droite (stabilite).
  SrcIt b = t.first ? bfirst : std::lower_bound(bfirst, blast, *a, less);
  const SrcIt be = t.last ? blast : std::lower_bound(bfirst, blast, *ae, less);
  DstIt out = dst + (std::ptrdiff_t)(t.a0 + (t.ai - t.a0) + (size_t)(b - bfirst));
  if (right_first) {
    // Mutant : la droite passe devant en cas d'egalite.
    while (a != ae && b != be) {
      if (less(*a, *b)) *out++ = *a++;
      else *out++ = *b++;
    }
  } else {
    while (a != ae && b != be) {
      if (less(*b, *a)) *out++ = *b++;
      else *out++ = *a++;
    }
  }
  out = std::copy(a, ae, out);
  std::copy(b, be, out);
}

// Decoupe un niveau de fusion en pieces a partir des bornes de runs courantes
// (`bounds`, R+1 entrees) ; ecrit les bornes du niveau suivant dans `next`.
inline void plan_level(const std::vector<size_t>& bounds, size_t grain, std::vector<MergePiece>* pieces,
                       std::vector<size_t>* next) {
  const size_t runs = bounds.size() - 1;
  pieces->clear();
  next->clear();
  next->push_back(bounds[0]);
  for (size_t r = 0; r < runs; r += 2) {
    const size_t a0 = bounds[r], a1 = bounds[r + 1];
    const size_t b0 = a1, b1 = (r + 1 < runs) ? bounds[r + 2] : a1;
    const size_t len_a = a1 - a0, len = b1 - a0;
    const size_t P = (len_a == 0) ? 1 : std::max<size_t>(1, (len + grain - 1) / grain);
    for (size_t p = 0; p < P; ++p) {
      MergePiece t;
      t.a0 = a0;
      t.a1 = a1;
      t.b0 = b0;
      t.b1 = b1;
      t.ai = a0 + (len_a * p) / P;
      t.ai1 = a0 + (len_a * (p + 1)) / P;
      t.first = (p == 0);
      t.last = (p + 1 == P);
      pieces->push_back(t);
    }
    next->push_back(b1);
  }
}

}  // namespace parallel_sort_detail

template <typename It, typename Less>
inline size_t parallel_stable_sort(It first, It last, Less less, int threads) {
  using namespace parallel_sort_detail;
  using T = typename std::iterator_traits<It>::value_type;
  const size_t n = (size_t)(last - first);
  if (n < kParallelSortMinElems || threads <= 1) {
    std::stable_sort(first, last, less);
    return 1;
  }
  const size_t W = planned_workers(n / kParallelSortMinSlice, threads);
  if (W <= 1) {
    std::stable_sort(first, last, less);
    return 1;
  }
  const bool right_first = MHGP5_MUTANT("parallel-sort-unstable");

  // Plan : tranches, puis niveaux de fusion (tout est calcule AVANT de lancer
  // les fils ; les ouvriers n'allouent rien).
  const size_t S = W;
  std::vector<size_t> bounds(S + 1);
  for (size_t s = 0; s <= S; ++s) bounds[s] = (n / S) * s + std::min(s, n % S);
  size_t L = 0;
  for (size_t runs = S; runs > 1; runs = (runs + 1) / 2) ++L;
  const size_t grain = std::max(kParallelSortMinGrain, (n + 4 * W - 1) / (4 * W));
  std::vector<std::vector<MergePiece>> levels(L);
  {
    std::vector<size_t> cur = bounds, nxt;
    for (size_t l = 0; l < L; ++l) {
      plan_level(cur, grain, &levels[l], &nxt);
      cur.swap(nxt);
    }
  }
  // Le niveau l (1..L) lit X_{l-1} et ecrit X_l ; X_L = entree, donc
  // X_l = entree ssi (L - l) est pair, et X_0 = tampon ssi L est impair.
  std::vector<T> buf(n);
  const auto bfirst = buf.begin();
  const bool slices_in_buf = (L % 2 == 1);
  std::vector<std::atomic<size_t>> next(L + 1);
  for (auto& c : next) c.store(0);
  std::barrier<> sync((std::ptrdiff_t)W);

  std::vector<std::thread> pool;
  pool.reserve(W);
  for (size_t w = 0; w < W; ++w)
    pool.emplace_back([&] {
      Less lt = less;
      // Phase 1 : tranches.
      for (;;) {
        const size_t s = next[0].fetch_add(1);
        if (s >= S) break;
        const std::ptrdiff_t b = (std::ptrdiff_t)bounds[s], e = (std::ptrdiff_t)bounds[s + 1];
        if (slices_in_buf) {
          std::copy(first + b, first + e, bfirst + b);
          std::stable_sort(bfirst + b, bfirst + e, lt);
        } else {
          std::stable_sort(first + b, first + e, lt);
        }
      }
      // Phase 2 : niveaux de fusion, barriere entre deux niveaux.
      for (size_t l = 1; l <= L; ++l) {
        sync.arrive_and_wait();
        const std::vector<MergePiece>& pieces = levels[l - 1];
        const bool dst_is_input = ((L - l) % 2 == 0);
        for (;;) {
          const size_t i = next[l].fetch_add(1);
          if (i >= pieces.size()) break;
          if (dst_is_input) merge_piece(bfirst, first, pieces[i], lt, right_first);
          else merge_piece(first, bfirst, pieces[i], lt, right_first);
        }
      }
    });
  for (auto& th : pool) th.join();
  return W;
}

template <typename T, typename Less>
inline size_t parallel_stable_sort_vector(std::vector<T>* v, Less less, int threads) {
  return parallel_stable_sort(v->begin(), v->end(), less, threads);
}

}  // namespace mhgp5
