// Catalogue critique : boules bien centrees (centre dans l'interieur relatif de conv(S), S support de 2 a
// 4 sites affinement independants), interieur strict I, coquille U complete, admises pour les ordres <= K :
//   coquille sans site pondere : p + q_min <= K + 1 ;  coquille ponderee : p <= K - 1 (sur-ensemble sur).
// (p = poids de I, q_min = plus petit support en positions.)
//
// Generation par BOITES DE CENTRES (lentille L13 de l'audit v9, conception GEN_v1) : chaque boule est
// enumeree dans l'unique feuille demi-ouverte qui contient son centre ; la liste de la feuille contient
// la boule K-NN fermee de tout point de la feuille (lemmes des gardes et des dominateurs), donc le
// recensement local est exact (theoreme C). Aucun flottant dans les decisions.
//
// Ordre canonique publie : (niveau exact, S*) ; rang = rang dense des niveaux exacts distincts.
#pragma once

#include <array>
#include <cstring>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "arith/geometry.hpp"
#include "cloud/cloud.hpp"
#include "core/status.hpp"
#include "sched/pool.hpp"

namespace mhgp10 {

// Allocateur des grands tableaux du catalogue : aucune initialisation par valeur (types triviaux). Chaque case est
// ecrite par la boucle parallele qui remplit le tableau : le premier contact des pages se repartit sur les fils au
// lieu d'un remplissage en serie. En build de test (MHGP10_POISON), les octets sont empoisonnes 0xA5.
template <class T>
struct UninitAlloc : std::allocator<T> {
  static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>, "types triviaux seulement");
  template <class U>
  struct rebind {
    using other = UninitAlloc<U>;
  };
  UninitAlloc() = default;
  template <class U>
  UninitAlloc(const UninitAlloc<U>&) noexcept {}
  T* allocate(std::size_t n) {
    T* p = std::allocator<T>::allocate(n);
#ifdef MHGP10_POISON
    std::memset(static_cast<void*>(p), 0xA5, n * sizeof(T));
#endif
    return p;
  }
  template <class U, class... A>
  void construct(U* p, A&&... a) {
    if constexpr (sizeof...(A) > 0) ::new (static_cast<void*>(p)) U(std::forward<A>(a)...);
  }
};
template <class T>
using UninitVector = std::vector<T, UninitAlloc<T>>;

// filter_tests : tests de dominance du filtre des noeuds (S0 pour chaque site de la liste parente, puis Y \ S0 pour
// ceux que S0 n'exclut pas) ; preskipped_bbox : noeuds ignores par l'enveloppe de la liste parente, sans filtrage
// (compris dans skipped_bbox).
struct CatalogueLedger {
  u64 nodes = 0, leaves = 0, skipped_bbox = 0, preskipped_bbox = 0, sum_m = 0, max_m = 0;
  u64 filter_tests = 0, leaf_dominance_tests = 0;
  u64 pair_tests = 0, triple_tests = 0, line_hits = 0, quad_tests = 0;
  u64 judged = 0, emitted = 0, extended = 0, weighted = 0, max_shell = 0, stalled_leaves = 0;
};

struct Catalogue {
  int kmax = 0;
  // par boule, ordre canonique
  UninitVector<u32> rank;                    // rang du niveau
  UninitVector<std::array<u32, 4>> support;  // S* trie (kNone au-dela de q_min)
  UninitVector<u8> qmin;                     // 2..4
  UninitVector<u32> p;                       // poids interieur
  UninitVector<u32> u;                       // poids de la coquille
  UninitVector<u8> flags;                    // bit0 coquille etendue, bit1 coquille ponderee
  UninitVector<u64> pop_off;                 // CSR : I trie puis U trie (indices de sites)
  UninitVector<u32> pop;
  UninitVector<u32> n_interior;              // |I| en positions
  UninitVector<geom::Level> level;           // niveau exact par rang
  CatalogueLedger ledger;
  // temps muraux (s) : frontiere en largeur, taches paralleles des boites, ordre canonique, assemblage
  double t_frontier = 0, t_boxes = 0, t_order = 0, t_assemble = 0;
  double t_collect = 0, t_sort = 0, t_bands = 0, t_compare = 0, t_ranks = 0, t_copy = 0;  // detail des deux derniers
  u64 bands = 0, band_members = 0;  // bandes flottantes reparees en exact, et leurs boules
  u64 tasks = 0, max_task_sites = 0;  // taches paralleles de l'arbre, et sites de la plus chargee

  u32 balls() const { return static_cast<u32>(rank.size()); }
  std::span<const u32> interior(u32 b) const { return {pop.data() + pop_off[b], n_interior[b]}; }
  std::span<const u32> shell(u32 b) const {
    return {pop.data() + pop_off[b] + n_interior[b], static_cast<std::size_t>(pop_off[b + 1] - pop_off[b] - n_interior[b])};
  }
};

enum : u8 { kExtendedShell = 1, kWeightedShell = 2 };

struct CatalogueParams {
  int kmax = 5;           // ordre maximal servi (catalogue interne jusqu'a kMaxCatalogueOrder)
  u32 leaf_size = 0;      // 0 : M(K) par defaut
  u32 max_leaf = 256;     // au-dela : refus resource_exhausted
};

Result<Catalogue> build_catalogue(const Cloud& cloud, const CatalogueParams& params, sched::Pool& pool);

// Centre exact d'une boule du catalogue, relatif a son premier site de support.
geom::Center ball_center(const Cloud& cloud, const Catalogue& cat, u32 b);

}  // namespace mhgp10
