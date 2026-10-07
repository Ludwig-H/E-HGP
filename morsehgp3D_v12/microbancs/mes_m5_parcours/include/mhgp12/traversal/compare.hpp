// Comparaison d'un parcours au vidage de reference (microbanc MES-M5, hors produit, hote seulement).
//
// Identite = meme statut, meme grand livre (noeuds, feuilles, tests G1, profondeur et feuille maximales) et MEME
// ENSEMBLE de feuilles, independamment de leur ordre de production : les deux ensembles sont ranges par boite (lo, hi),
// boites deux a deux distinctes (elles sont disjointes), puis compares feuille a feuille : boite, liste (suite exacte
// des SiteIdx, croissante dans la v11), profondeur, chemin. L'empreinte canonique (FNV-1a 64 de la suite rangee des
// boites et listes) ne depend ni de l'ordre ni des chemins.
#pragma once

#include <algorithm>
#include <cstring>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "mhgp12/traversal/bfs.hpp"
#include "mhgp12/traversal/driver.hpp"
#include "mhgp12/traversal/format.hpp"

namespace mhgp12::traversal {

static_assert(sizeof(bfs::Leaf) == sizeof(format::Leaf), "feuille du parcours = feuille du format");
static_assert(sizeof(Ledger) == 5 * sizeof(u64));

struct LeafSet {
  const format::Leaf* leaves = nullptr;
  u64 n = 0;
  const u32* sites = nullptr;
  u64 n_sites = 0;
};

inline LeafSet leaf_set(const format::Dump& d) {
  return LeafSet{d.leaves.data(), d.leaves.size(), d.sites.data(), d.sites.size()};
}

inline bool box_less(const format::Leaf& a, const format::Leaf& b) {
  for (int k = 0; k < 3; ++k)
    if (a.lo[k] != b.lo[k]) return a.lo[k] < b.lo[k];
  for (int k = 0; k < 3; ++k)
    if (a.hi[k] != b.hi[k]) return a.hi[k] < b.hi[k];
  return false;
}

inline bool same_box(const format::Leaf& a, const format::Leaf& b) {
  return std::memcmp(a.lo, b.lo, sizeof(a.lo)) == 0 && std::memcmp(a.hi, b.hi, sizeof(a.hi)) == 0;
}

inline std::vector<u64> canonical_order(const LeafSet& s) {
  std::vector<u64> order(s.n);
  std::iota(order.begin(), order.end(), u64{0});
  std::sort(order.begin(), order.end(), [&](u64 a, u64 b) { return box_less(s.leaves[a], s.leaves[b]); });
  return order;
}

// Empreinte canonique : (lo, hi, m, sites) dans l'ordre des boites ; rend aussi faux si deux boites sont egales.
inline u64 canonical_digest(const LeafSet& s, bool& distinct) {
  const auto order = canonical_order(s);
  u64 h = dump::kFnvBasis;
  distinct = true;
  for (u64 i = 0; i < order.size(); ++i) {
    const format::Leaf& l = s.leaves[order[i]];
    if (i > 0 && same_box(l, s.leaves[order[i - 1]])) distinct = false;
    h = dump::fnv1a(h, l.lo, sizeof(l.lo));
    h = dump::fnv1a(h, l.hi, sizeof(l.hi));
    h = dump::fnv1a(h, &l.m, sizeof(l.m));
    if (l.begin <= s.n_sites && l.m <= s.n_sites - l.begin) h = dump::fnv1a(h, s.sites + l.begin, u64{l.m} * 4);
  }
  return h;
}

struct Comparison {
  bool status_equal = false, ledger_equal = false, leaves_equal = false;
  u64 reference_leaves = 0, leaves = 0;
  u64 missing = 0, extra = 0, list_mismatch = 0, list_permuted = 0, meta_mismatch = 0;  // meta : profondeur, chemin
  u64 reference_digest = 0, digest = 0;
  std::string first;  // premiere difference, lisible
  bool identity() const { return status_equal && ledger_equal && leaves_equal; }
};

inline std::string describe(const format::Leaf& l) {
  std::ostringstream o;
  o << "[" << l.lo[0] << "," << l.lo[1] << "," << l.lo[2] << ")-[" << l.hi[0] << "," << l.hi[1] << "," << l.hi[2]
    << ") m=" << l.m << " d=" << l.depth;
  return o.str();
}

inline Comparison compare(const format::Dump& ref, u64 status, const Ledger& ledger, const LeafSet& mine) {
  Comparison c;
  const format::Header& h = ref.header;
  c.status_equal = h.status == status;
  const Ledger expected{h.nodes, h.leaves, h.filter_tests, h.max_depth, h.max_leaf_seen};
  c.ledger_equal = expected == ledger;
  if (!c.ledger_equal && c.first.empty()) {
    std::ostringstream o;
    o << "grand livre : noeuds " << ledger.nodes << "/" << expected.nodes << ", feuilles " << ledger.leaves << "/"
      << expected.leaves << ", tests " << ledger.filter_tests << "/" << expected.filter_tests << ", profondeur "
      << ledger.max_depth << "/" << expected.max_depth << ", feuille max " << ledger.max_leaf << "/"
      << expected.max_leaf;
    c.first = o.str();
  }
  const LeafSet r = leaf_set(ref);
  c.reference_leaves = r.n;
  c.leaves = mine.n;
  bool distinct_r = true, distinct_m = true;
  c.reference_digest = canonical_digest(r, distinct_r);
  c.digest = canonical_digest(mine, distinct_m);
  const auto ro = canonical_order(r), mo = canonical_order(mine);
  u64 i = 0, j = 0;
  while (i < ro.size() || j < mo.size()) {
    if (j == mo.size() || (i < ro.size() && box_less(r.leaves[ro[i]], mine.leaves[mo[j]]))) {
      ++c.missing;
      if (c.first.empty()) c.first = "feuille absente : " + describe(r.leaves[ro[i]]);
      ++i;
      continue;
    }
    if (i == ro.size() || box_less(mine.leaves[mo[j]], r.leaves[ro[i]])) {
      ++c.extra;
      if (c.first.empty()) c.first = "feuille en trop : " + describe(mine.leaves[mo[j]]);
      ++j;
      continue;
    }
    const format::Leaf& a = r.leaves[ro[i]];
    const format::Leaf& b = mine.leaves[mo[j]];
    bool lists = a.m == b.m && b.begin <= mine.n_sites && b.m <= mine.n_sites - b.begin &&
                 std::memcmp(r.sites + a.begin, mine.sites + b.begin, u64{a.m} * 4) == 0;
    if (!lists) {
      ++c.list_mismatch;
      if (a.m == b.m && b.begin <= mine.n_sites && b.m <= mine.n_sites - b.begin) {
        std::vector<u32> x(r.sites + a.begin, r.sites + a.begin + a.m), y(mine.sites + b.begin, mine.sites + b.begin + b.m);
        std::sort(x.begin(), x.end());
        std::sort(y.begin(), y.end());
        if (x == y) ++c.list_permuted;
      }
      if (c.first.empty()) c.first = "liste differente : " + describe(a) + " contre m=" + std::to_string(b.m);
    }
    if (a.depth != b.depth || a.path[0] != b.path[0] || a.path[1] != b.path[1]) {
      ++c.meta_mismatch;
      if (c.first.empty()) c.first = "profondeur ou chemin : " + describe(a);
    }
    ++i;
    ++j;
  }
  c.leaves_equal = distinct_r && distinct_m && c.missing == 0 && c.extra == 0 && c.list_mismatch == 0 &&
                   c.meta_mismatch == 0 && c.reference_digest == c.digest && r.n == mine.n;
  if (!c.status_equal && c.first.empty()) c.first = "statut " + std::to_string(status) + " / " + std::to_string(h.status);
  return c;
}

}  // namespace mhgp12::traversal
