// MorseHGP3D v7 — tri STABLE parallele, sortie identique a std::stable_sort.
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
//   1. T tranches contigues triees par std::stable_sort (route generique) ou
//      std::sort (indices totalement ordonnes, departage par rang original) ;
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
// tampon est un std::vector<T>). Sur la ROUTE DIRECTE les elements sont
// COPIES, jamais deplaces : les pieces d'une meme fusion lisent la source
// concurremment (splitters). Sur la route permutation ci-dessous ils sont
// DEPLACES, mais par un seul fil et une position a la fois.
// `less` est copie par ouvrier et doit etre reentrant (capturer un tableau par
// reference constante est l'usage : trier une permutation d'indices). Une
// exception levee par `less` dans un ouvrier est capturee puis propagee
// apres drainage des barrieres et jonction de tous les fils.
//
// Mutant `parallel-sort-unstable` : la fusion prend la droite d'abord en cas
// d'egalite (tri correct mais instable : la porte parallel_sort_gate le tue).
//
// ===========================================================================
// ROUTE PERMUTATION (palier P4 d'echelle, docs/ECHELLE.md § 6.4)
// ===========================================================================
//
// LE POSTE SUPPRIME. La route directe ci-dessus materialise UN DOUBLE EXACT du
// tableau (`std::vector<T> buf(n)`), et `std::stable_sort` en fait autant dans
// son `_Temporary_buffer` quand la route directe est sequentielle : sur les
// candidats de boules (`BallCandidate`, 144 octets compiles) c'est 144 n
// octets de residence de plus, nes et morts ENTRE deux jalons `rss_mb` — donc
// invisibles aux instantanes et visibles seulement dans `residence_hwm_mb`.
// La route permutation trie des INDICES u32 (4 n octets, plus 4 n de tampon
// interne) puis applique la permutation SUR PLACE par suivi de cycles : le
// payload supplementaire est au plus 8 n octets d'indices, plus UN T pour les
// cycles (4 n d'indices au sequentiel). Ce n'est PAS une borne RSS : il faut
// ajouter les plans de fusion, equipes, barrieres et piles O(log n). La porte
// perm_residence verifie que std::sort de la bibliotheque utilisee n'alloue
// aucun tampon proportionnel a n dans les tris locaux des indices. La route
// generique conserve std::stable_sort et ses temporaires de bibliotheque.
// La route permutation est prise pour tout T dont la taille depasse
// `kPermutationSortMinElemBytes`, a 1 fil comme a N fils.
//
// ---------------------------------------------------------------------------
// THEOREME (stabilite de la route permutation)
// ---------------------------------------------------------------------------
// Soient `a[0..n)` une suite finie, `less` un ordre faible strict sur ses
// elements, et n <= 2^32 - 1. Soit `lt` la relation sur les indices
//
//     lt(x, y)  :=  less(a[x], a[y])                              (pullback)
//                   OU  ( !less(a[x],a[y]) ET !less(a[y],a[x]) ET x < y ).
//
// Soit `idx` la suite (0, 1, ..., n-1) triee par un tri QUELCONQUE correct
// sous `lt`, et soit `b` definie par le RAMASSAGE `b[r] = a[idx[r]]`.
// ALORS `b == std::stable_sort(a, less)`, element pour element.
//
// Preuve. (1) `lt` est un ordre TOTAL strict : le tire-en-arriere d'un ordre
// faible strict par une application est un ordre faible strict, et le
// departage par `x < y` en brise toutes les classes d'equivalence (deux
// indices distincts sont toujours comparables). Un ordre total strict n'admet
// donc QU'UNE suite triee : `idx` est unique, independamment du tri employe et
// de sa stabilite — c'est le point exact ou la naivete se paie (trier les
// indices sous le seul `less(a[x],a[y])` avec un tri INSTABLE rend une
// permutation correcte mais arbitraire sur les ex aequo, et donc une sortie
// qui n'est plus la sortie stable ; la porte `mhgp7_perm_sort_mutant_ex_aequo`
// tue exactement ce defaut par le mutant `perm-tie-desc`).
// (2) `idx` est croissante pour `lt`, donc pour tout r : `!less(b[r+1], b[r])`
// (sinon `lt(idx[r+1], idx[r])` par la premiere branche) : `b` est triee.
// (3) Si `b[r]` et `b[r']` sont equivalentes pour `less` avec r < r', alors
// `lt(idx[r], idx[r'])` ne peut tenir que par la SECONDE branche, donc
// `idx[r] < idx[r']` : les ex aequo de `b` apparaissent dans l'ordre de leurs
// positions d'origine — `b` est un rearrangement STABLE de `a`.
// (4) Le rearrangement trie et stable d'une suite est unique : si `b` et `c`
// le sont toutes deux, la r-ieme occurrence de chaque classe d'equivalence
// occupe la meme position dans les deux. Donc `b == std::stable_sort(a,less)`.
// (5) `apply_permutation_cycles` realise exactement le ramassage `b[r] =
// a[idx[r]]` sur place : la boucle externe visite chaque cycle de la
// permutation une seule fois (une position visitee est marquee point fixe,
// `p[j] = j`), et la boucle interne, partant de r, ecrit `a[j] <- a[idx[j]]`
// en descendant le cycle AVANT que `idx[j]` n'ait ete lu par une autre
// ecriture — la valeur d'origine de `a[r]` est mise de cote et reposee a la
// fermeture du cycle. Aucune position n'est ecrite deux fois, aucune n'est
// omise. Corollaire : le nombre de deplacements vaut n - f + c (f points fixes,
// c cycles non triviaux), jamais plus de n + c. ∎
//
// PORTEE. La route permutation ne change ni l'objet, ni l'ordre, ni le nombre
// de fils vu par l'appelant ; elle rend le MEME resultat que la route directe
// (porte `mhgp7_perm_sort_routes`, qui les oppose sur la meme entree).
//
// Mutants de la route (registre `kMutants`) :
//   `perm-apply-scatter`  : l'application prend le sens INVERSE (dispersion
//       `a[idx[r]] <- a[r]` au lieu du ramassage) — permutation inverse, donc
//       tableau non trie des que la permutation n'est pas une involution ;
//   `perm-apply-partial`  : la FERMETURE de chaque cycle est sautee (l'element
//       mis de cote n'est jamais repose) — l'application est partielle et le
//       tableau perd un element par cycle ;
//   `perm-tie-desc`       : le departage des ex aequo devient `y < x` — le
//       resultat reste TRIE mais cesse d'etre STABLE. INVISIBLE aux digests du
//       pipeline (deux `BallCandidate` equivalents pour `ball_candidate_less`
//       sont identiques champ par champ : l'instabilite n'y a rien a echanger),
//       il n'est tue que par une scene a CHARGE UTILE distincte des cles.
#pragma once

#include <algorithm>
#include <atomic>
#include <barrier>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "../core/mutants.hpp"
#include "../core/types.hpp"
#include "pool.hpp"

namespace mhgp7 {

// Sous ce cardinal, sequentiel pur.
inline constexpr size_t kParallelSortMinElems = (size_t)1 << 15;
// Au moins ce nombre d'elements par tranche initiale (borne le nombre d'ouvriers).
inline constexpr size_t kParallelSortMinSlice = (size_t)1 << 14;
// Grain minimal d'une piece de fusion.
inline constexpr size_t kParallelSortMinGrain = (size_t)1 << 12;
// Au-dela de cette taille d'element, la route PERMUTATION (indices u32 + suivi
// de cycles) remplace le double exact du tableau. Le seuil est le point ou le
// tampon d'indices (8 octets par element : la permutation plus le tampon du
// tri interne) cesse d'etre plus cher que le double exact ; il laisse la route
// directe aux petits elements (u32 des evenements du fold : 4 octets).
inline constexpr size_t kPermutationSortMinElemBytes = 32;

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

// Le seul appel TotalIndexOrder=true est le tri de la permutation sous
// IndexLess : les indices distincts ne sont JAMAIS equivalents. Un tri local
// instable y donne donc exactement la meme suite et n'a pas besoin du tampon
// temporaire proportionnel a n de std::stable_sort (libstdc++ qualifiee).
// Les petits elements ordinaires conservent leur stabilite historique.
template <bool TotalIndexOrder, typename It, typename Less>
inline void sort_slice(It first, It last, Less less) {
  if constexpr (TotalIndexOrder) std::sort(first, last, less);
  else std::stable_sort(first, last, less);
}

// ROUTE DIRECTE : tranches + fusions + UN double exact du tableau. Reste la
// route stable des petits elements et le socle de la route permutation.
template <bool TotalIndexOrder = false, typename It, typename Less>
inline size_t sort_direct(It first, It last, Less less, int threads) {
  using T = typename std::iterator_traits<It>::value_type;
  static_assert(!TotalIndexOrder || std::is_same_v<T, u32>);
  const size_t n = (size_t)(last - first);
  if (n < kParallelSortMinElems || threads <= 1) {
    sort_slice<TotalIndexOrder>(first, last, less);
    return 1;
  }
  const size_t W = planned_workers(n / kParallelSortMinSlice, threads);
  if (W <= 1) {
    sort_slice<TotalIndexOrder>(first, last, less);
    return 1;
  }
  const bool right_first = MHGP7_MUTANT("parallel-sort-unstable");

  // Plan : tranches, puis niveaux de fusion (calcule avant les fils). La route
  // generique stable peut encore allouer des temporaires dans les tris locaux.
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

  parallel_detail::FirstException fx;
  parallel_detail::run_threads(W, [&](size_t) {
    try {
      Less lt = less;
      // Phase 1 : tranches.
      for (;;) {
        if (fx.stop.load()) break;
        const size_t s = next[0].fetch_add(1);
        if (s >= S) break;
        const std::ptrdiff_t b = (std::ptrdiff_t)bounds[s], e = (std::ptrdiff_t)bounds[s + 1];
        if (slices_in_buf) {
          std::copy(first + b, first + e, bfirst + b);
          sort_slice<TotalIndexOrder>(bfirst + b, bfirst + e, lt);
        } else {
          sort_slice<TotalIndexOrder>(first + b, first + e, lt);
        }
      }
      // Phase 2 : niveaux de fusion, barriere entre deux niveaux.
      for (size_t l = 1; l <= L; ++l) {
        sync.arrive_and_wait();
        if (fx.stop.load()) {
          sync.arrive_and_drop();
          return;
        }
        const std::vector<MergePiece>& pieces = levels[l - 1];
        const bool dst_is_input = ((L - l) % 2 == 0);
        for (;;) {
          if (fx.stop.load()) break;
          const size_t i = next[l].fetch_add(1);
          if (i >= pieces.size()) break;
          if (dst_is_input) merge_piece(bfirst, first, pieces[i], lt, right_first);
          else merge_piece(first, bfirst, pieces[i], lt, right_first);
        }
      }
    } catch (...) {
      fx.capture();
      sync.arrive_and_drop();
    }
  });
  fx.rethrow_if_any();
  return W;
}

// Comparateur d'INDICES : le tire-en-arriere de `less` par `base`, DEPARTAGE
// par l'indice lui-meme (point (1) du theoreme d'en-tete). Objet-fonction et
// non lambda : il est copie par ouvrier et appele en const, comme l'exige le
// contrat de la route directe.
template <typename It, typename Less>
struct IndexLess {
  It base;
  Less less;
  bool tie_desc = false;
  bool operator()(u32 x, u32 y) const {
    const auto& ax = base[(std::ptrdiff_t)x];
    const auto& ay = base[(std::ptrdiff_t)y];
    if (less(ax, ay)) return true;
    if (less(ay, ax)) return false;
    // Mutant `perm-tie-desc` : les ex aequo repartent en ordre inverse — trie
    // mais plus stable.
    return tie_desc ? (y < x) : (x < y);
  }
};

// Applique SUR PLACE le ramassage `a[r] <- a_origine[p[r]]` par suivi de
// cycles. `p` est DETRUITE : chaque position visitee y devient un point fixe,
// ce qui sert de marque de visite sans aucun tableau annexe (point (5) du
// theoreme). Un seul element temporaire de type T existe a la fois.
template <typename It>
inline void apply_permutation_cycles(It first, std::vector<u32>* perm) {
  using T = typename std::iterator_traits<It>::value_type;
  const size_t n = perm->size();
  if (MHGP7_MUTANT("perm-apply-scatter")) {
    // Sens INVERSE : on applique la permutation inverse (dispersion). Le
    // tableau n'est trie que si la permutation est une involution.
    std::vector<u32> inv(n);
    for (size_t r = 0; r < n; ++r) inv[(size_t)(*perm)[r]] = (u32)r;
    perm->swap(inv);
  }
  // Mutant `perm-apply-partial` : la FERMETURE de chaque cycle est sautee — la
  // derniere position du cycle n'est jamais reecrite et l'element mis de cote
  // est perdu. C'est l'oubli classique du suivi de cycles, et c'est bien une
  // application PARTIELLE : tout le reste du cycle a bien ete deplace.
  // (Arreter la boucle EXTERNE a n/2 n'en serait pas une : une permutation de
  // tri a O(log n) cycles tres longs, chacun rencontrant presque surement un
  // indice < n/2 — mesure : zero divergence sur les douze scenes.)
  const bool skip_close = MHGP7_MUTANT("perm-apply-partial");
  u32* p = perm->data();
  for (size_t r = 0; r < n; ++r) {
    if ((size_t)p[r] == r) continue;
    T tmp = std::move(first[(std::ptrdiff_t)r]);
    size_t j = r;
    for (;;) {
      const size_t s = (size_t)p[j];
      p[j] = (u32)j;
      if (s == r) {
        if (!skip_close) first[(std::ptrdiff_t)j] = std::move(tmp);
        break;
      }
      first[(std::ptrdiff_t)j] = std::move(first[(std::ptrdiff_t)s]);
      j = s;
    }
  }
}

// ROUTE PERMUTATION : tri d'indices u32 sous `IndexLess`, puis application par
// cycles. Payload supplementaire : au plus 8 n octets d'indices, plus UN
// element. Ajouter les metadonnees/equipes et piles; ce n'est pas un RSS.
template <typename It, typename Less>
inline size_t sort_by_permutation(It first, It last, Less less, int threads) {
  const size_t n = (size_t)(last - first);
  std::vector<u32> perm(n);
  for (size_t i = 0; i < n; ++i) perm[i] = (u32)i;
  const IndexLess<It, Less> lt{first, less, MHGP7_MUTANT("perm-tie-desc")};
  const size_t w = sort_direct<true>(perm.begin(), perm.end(), lt, threads);
  apply_permutation_cycles(first, &perm);
  return w;
}

}  // namespace parallel_sort_detail

// Aiguillage : au-dela de `kPermutationSortMinElemBytes` par element, la route
// permutation ; sinon la route directe. Le `if constexpr` interdit toute
// recursion (les indices sont des u32 de 4 octets). Le domaine de la route
// permutation est n <= 2^32 - 1 (indices u32) ; au-dela, la route directe
// reprend — le pipeline refuse de toute facon plus de 2^32-1 candidats.
template <typename It, typename Less>
inline size_t parallel_stable_sort(It first, It last, Less less, int threads) {
  using T = typename std::iterator_traits<It>::value_type;
  if constexpr (sizeof(T) > kPermutationSortMinElemBytes) {
    const size_t n = (size_t)(last - first);
    if (n > 1 && n <= (size_t)UINT32_MAX)
      return parallel_sort_detail::sort_by_permutation(first, last, less, threads);
  }
  return parallel_sort_detail::sort_direct(first, last, less, threads);
}

template <typename T, typename Less>
inline size_t parallel_stable_sort_vector(std::vector<T>* v, Less less, int threads) {
  return parallel_stable_sort(v->begin(), v->end(), less, threads);
}

// Route DIRECTE exposee : la porte de tri oppose les deux routes sur la meme
// entree (elles doivent rendre la meme suite, celle de std::stable_sort).
template <typename It, typename Less>
inline size_t stable_sort_direct_route(It first, It last, Less less, int threads) {
  return parallel_sort_detail::sort_direct(first, last, less, threads);
}

}  // namespace mhgp7
