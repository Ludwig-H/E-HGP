#include "head/head.hpp"

#include <algorithm>
#include <atomic>
#include <functional>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>

namespace mhgp10 {

namespace {

double lambda_of(double level, double z) {
  if (level <= 0) return std::numeric_limits<double>::infinity();
  return std::pow(level, -0.5 * z);
}

// Implementation de reference (parcours en profondeur depuis la racine) : sert hors des preconditions du
// balayage (poids non unitaires, CSR qui n'est pas l'inverse exact des parents).
CondensedTree condense_reference(const PointDendrogram& d, const ClusterParams& p) {
  const u32 n = d.nodes();
  // masse de chaque sous-arbre ; points directement attaches a chaque noeud (CSR)
  std::vector<u64> mass(n, 0);
  std::vector<u32> att_off(n + 1, 0), att(d.points());
  for (u32 x = 0; x < d.points(); ++x) ++att_off[d.point_node[x] + 1];
  for (u32 v = 0; v < n; ++v) att_off[v + 1] += att_off[v];
  {
    std::vector<u32> fill(att_off.begin(), att_off.end() - 1);
    for (u32 x = 0; x < d.points(); ++x) att[fill[d.point_node[x]]++] = x;
  }
  for (u32 v = 0; v < n; ++v) {  // enfants crees avant les parents
    for (u32 j = att_off[v]; j < att_off[v + 1]; ++j) mass[v] += d.point_weight[att[j]];
    for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j) mass[v] += mass[d.child_val[j]];
  }
  u32 root = kNone;
  for (u32 v = 0; v < n; ++v)
    if (d.parent[v] == kNone) root = v;

  CondensedTree t;
  t.point_cluster.assign(d.points(), kNone);
  t.node_cluster.assign(n, kNone);
  t.point_lambda.assign(d.points(), 0.0);
  auto new_cluster = [&](u32 parent, double birth, u64 m) {
    t.parent.push_back(parent);
    t.birth.push_back(birth);
    t.stability.push_back(0.0);
    t.mass.push_back(m);
    return static_cast<u32>(t.parent.size() - 1);
  };
  // sortie de tous les points d'un sous-arbre au lambda donne
  std::vector<u32> stack;
  auto drop_subtree = [&](u32 v, u32 c, double lam) {
    stack.assign(1, v);
    while (!stack.empty()) {
      const u32 u = stack.back();
      stack.pop_back();
      t.node_cluster[u] = c;
      for (u32 j = att_off[u]; j < att_off[u + 1]; ++j) {
        const u32 x = att[j];
        t.point_cluster[x] = c;
        t.point_lambda[x] = lam;
        t.stability[c] += d.point_weight[x] * (lam - t.birth[c]);
      }
      for (u32 j = d.child_off[u]; j < d.child_off[u + 1]; ++j) stack.push_back(d.child_val[j]);
    }
  };
  // travail : (noeud, cluster courant)
  std::vector<std::pair<u32, u32>> work;
  work.push_back({root, new_cluster(kNone, 0.0, mass[root])});
  std::vector<u32> big;
  while (!work.empty()) {
    auto [v, c] = work.back();
    work.pop_back();
    t.node_cluster[v] = c;
    // points attaches directement : sortent a leur niveau d'entree
    for (u32 j = att_off[v]; j < att_off[v + 1]; ++j) {
      const u32 x = att[j];
      const double lam = lambda_of(d.level[d.point_rank[x]], p.z);
      t.point_cluster[x] = c;
      t.point_lambda[x] = lam;
      t.stability[c] += d.point_weight[x] * (lam - t.birth[c]);
    }
    if (d.child_off[v] == d.child_off[v + 1]) continue;
    const double lam = lambda_of(d.level[d.node_rank[v]], p.z);
    big.clear();
    for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j)
      if (mass[d.child_val[j]] >= p.min_cluster_size) big.push_back(d.child_val[j]);
    if (big.size() >= 2) {
      for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j) {
        const u32 u = d.child_val[j];
        if (mass[u] >= p.min_cluster_size) {
          t.stability[c] += double(mass[u]) * (lam - t.birth[c]);
          work.push_back({u, new_cluster(c, lam, mass[u])});
        } else {
          drop_subtree(u, c, lam);
        }
      }
    } else {
      for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j) {
        const u32 u = d.child_val[j];
        if (big.size() == 1 && u == big[0]) work.push_back({u, c});
        else drop_subtree(u, c, lam);
      }
    }
  }
  return t;
}

}  // namespace

// Condensation par balayages des identifiants (enfants avant parents, racine = n - 1), memes sorties bit a bit que
// condense_reference quand les poids sont unitaires et que le CSR est l'inverse exact des parents (controle) :
//  1. montee : masse et enfants gros ; les feuilles (sans enfant) en parallele, les noeuds internes en serie par
//     identifiants croissants (controle du CSR : enfants strictement croissants, inferieurs au parent, parent[c] == v,
//     n - 1 entrees) ;
//  2. lambdas precalcules en parallele : noeuds de travail internes (racine ou masse >= mcs) et points qui leur sont
//     attaches (meme pow, memes niveaux que la reference) ;
//  3. descente tiree des noeuds internes, par identifiants decroissants : chaque noeud lit l'etat de son parent ; les
//     noeuds de travail ajoutent leurs termes de stabilite dans l'ordre de leur chemin (parents d'abord), les termes
//     d'un sous-arbre abandonne valant tous 1 (lam - naissance) et ajoutes un par un (meme suite d'additions) ; les
//     scissions creent les clusters de leurs enfants gros dans l'ordre des enfants ;
//  4. feuilles en parallele : une feuille de travail termine le chemin de son cluster (un seul par cluster) et y ajoute
//     ses termes en dernier ; aucune autre tache n'ecrit ce cluster ;
//  5. numeros definitifs : pile LIFO de la reference rejouee sur l'arbre des clusters ; renumerotation en parallele.
// pool nul, ou appel depuis un ouvrier : meme calcul en serie.
CondensedTree condense(const PointDendrogram& d, const ClusterParams& p, sched::Pool* pool) {
  const u32 n = d.nodes();
  const u32 np = d.points();
  if (n == 0 || d.parent.size() != n || d.child_off.size() != u64(n) + 1 || d.child_off[0] != 0 ||
      d.child_off[n] != u64(n) - 1 || d.child_val.size() != u64(n) - 1 || d.parent[n - 1] != kNone ||
      d.point_node.size() != np || d.point_rank.size() != np || d.point_weight.size() != np)
    return condense_reference(d, p);
  auto pfor = [&](u64 count, u64 grain, const std::function<void(u64, u64)>& body) {
    if (count == 0) return;
    if (pool == nullptr) {
      body(0, count);
      return;
    }
    pool->parallel_for(count, grain, [&](u64 b, u64 e, unsigned) { body(b, e); });
  };
  const u64 chunks = pool ? std::min<u64>(u64(pool->size()) * 8, std::max<u64>(1, n / 4096)) : 1;
  auto chunk_lo = [&](u64 c, u64 total) { return total * c / chunks; };
  // points : poids unitaires et noeuds valides (sinon reference) ; CSR des points attaches (x croissant)
  std::atomic<u32> bad{0};
  pfor(np, 8192, [&](u64 b, u64 e) {
    for (u64 x = b; x < e; ++x)
      if (d.point_weight[x] != 1 || d.point_node[x] >= n) bad.store(1, std::memory_order_relaxed);
  });
  if (bad.load()) return condense_reference(d, p);
  std::vector<u32> att_off(n + 1, 0), att(np);
  for (u32 x = 0; x < np; ++x) ++att_off[d.point_node[x] + 1];
  {
    std::vector<u64> part(chunks + 1, 0);
    pfor(chunks, 1, [&](u64 c0, u64 c1) {
      for (u64 c = c0; c < c1; ++c) {
        u64 s = 0;
        for (u64 v = chunk_lo(c, n); v < chunk_lo(c + 1, n); ++v) s += att_off[v + 1];
        part[c + 1] = s;
      }
    });
    for (u64 c = 0; c < chunks; ++c) part[c + 1] += part[c];
    pfor(chunks, 1, [&](u64 c0, u64 c1) {
      for (u64 c = c0; c < c1; ++c) {
        u64 s = part[c];
        for (u64 v = chunk_lo(c, n); v < chunk_lo(c + 1, n); ++v) {
          s += att_off[v + 1];
          att_off[v + 1] = static_cast<u32>(s);
        }
      }
    });
    std::vector<u32> fill(att_off.begin(), att_off.end() - 1);
    for (u32 x = 0; x < np; ++x) att[fill[d.point_node[x]]++] = x;
  }
  struct Node {
    u32 mass;
    u32 parent;
    u32 cl;    // montee : enfants gros (sature a 2) ; descente : cluster | kWork | kSplit
    u32 from;  // montee : masse des enfants gros ; descente (abandon) : noeud de travail dont le lambda s'applique
  };
  constexpr u32 kWork = 0x80000000u, kSplit = 0x40000000u, kMask = 0x3FFFFFFFu;
  // sans initialisation : chaque champ est ecrit avant d'etre lu (voir les etapes)
  std::unique_ptr<Node[]> N(new Node[n]);
#ifdef MHGP10_POISON
  std::memset(static_cast<void*>(N.get()), 0xA5, sizeof(Node) * n);
#endif
  const u64 mcs = p.min_cluster_size;
  // noeuds internes (avec enfants), dans l'ordre des identifiants ; feuilles initialisees en parallele
  std::vector<u32> inner;
  {
    std::vector<u64> cnt(chunks + 1, 0);
    pfor(chunks, 1, [&](u64 c0, u64 c1) {
      for (u64 c = c0; c < c1; ++c) {
        u64 k = 0;
        for (u64 v = chunk_lo(c, n); v < chunk_lo(c + 1, n); ++v) {
          const u32 j0 = d.child_off[v], j1 = d.child_off[v + 1];
          if (j0 > j1 || j1 > n - 1) bad.store(1, std::memory_order_relaxed);
          if (j0 < j1) {
            ++k;
          } else {
            N[v].mass = att_off[v + 1] - att_off[v];
            N[v].parent = d.parent[v];
            N[v].cl = 0;
            N[v].from = 0;
          }
        }
        cnt[c + 1] = k;
      }
    });
    if (bad.load()) return condense_reference(d, p);
    for (u64 c = 0; c < chunks; ++c) cnt[c + 1] += cnt[c];
    inner.resize(cnt[chunks]);
    pfor(chunks, 1, [&](u64 c0, u64 c1) {
      for (u64 c = c0; c < c1; ++c) {
        u64 k = cnt[c];
        for (u64 v = chunk_lo(c, n); v < chunk_lo(c + 1, n); ++v)
          if (d.child_off[v] < d.child_off[v + 1]) inner[k++] = static_cast<u32>(v);
      }
    });
  }
  for (const u32 v : inner) {
    u64 m = att_off[v + 1] - att_off[v];
    u32 nbig = 0;
    u64 bigm = 0;
    const u32 j0 = d.child_off[v], j1 = d.child_off[v + 1];
    for (u32 j = j0; j < j1; ++j) {
      const u32 c = d.child_val[j];
      if (c >= v || (j > j0 && d.child_val[j - 1] >= c) || N[c].parent != v) return condense_reference(d, p);
      const u32 mc = N[c].mass;
      m += mc;
      if (mc >= mcs) {
        ++nbig;
        bigm += mc;
      }
    }
    N[v].mass = static_cast<u32>(m);
    N[v].parent = d.parent[v];
    N[v].cl = std::min<u32>(nbig, 2);
    N[v].from = static_cast<u32>(bigm);
  }
  // lambdas des noeuds de travail internes et des points qui leur sont attaches (travail : racine ou masse >= mcs)
  CondensedTree t;
  t.point_cluster.resize(np);
  t.node_cluster.resize(n);
  t.point_lambda.resize(np);
  std::unique_ptr<double[]> lamw(new double[n]);  // ecrit pour les noeuds de travail internes, lu ensuite
#ifdef MHGP10_POISON
  std::memset(static_cast<void*>(lamw.get()), 0xA5, sizeof(double) * n);
#endif
  auto is_work = [&](u32 v) { return v == n - 1 || N[v].mass >= mcs; };
  pfor(inner.size(), 1024, [&](u64 b, u64 e) {
    for (u64 i = b; i < e; ++i)
      if (is_work(inner[i])) lamw[inner[i]] = lambda_of(d.level[d.node_rank[inner[i]]], p.z);
  });
  pfor(np, 2048, [&](u64 b, u64 e) {
    for (u64 x = b; x < e; ++x)
      if (is_work(d.point_node[x])) t.point_lambda[x] = lambda_of(d.level[d.point_rank[x]], p.z);
  });
  std::vector<u32> tparent;
  std::vector<double> tbirth, tstab;
  std::vector<u64> tmass;
  std::vector<std::vector<u32>> tkids;
  auto new_cluster = [&](u32 parent, double birth, u64 m) {
    tparent.push_back(parent);
    tbirth.push_back(birth);
    tstab.push_back(0.0);
    tmass.push_back(m);
    tkids.emplace_back();
    if (parent != kNone) tkids[parent].push_back(static_cast<u32>(tparent.size() - 1));
    return static_cast<u32>(tparent.size() - 1);
  };
  std::unique_ptr<u32[]> pushed(new u32[n]);  // cluster d'un enfant gros de scission, pose par le parent
#ifdef MHGP10_POISON
  std::memset(static_cast<void*>(pushed.get()), 0xA5, sizeof(u32) * n);
#endif
  // etat d'un noeud tire de son parent (deja fixe) ; rend le cluster et ecrit N[v].cl, N[v].from
  auto pull = [&](u32 v) {
    Node& a = N[v];
    if (v == n - 1) {
      a.cl = kWork;  // cluster 0, cree avant la descente
      return;
    }
    const Node& b = N[a.parent];
    const u32 c = b.cl & kMask;
    if (!(b.cl & kWork)) {
      a.cl = c;
      a.from = b.from;
    } else if (a.mass >= mcs) {
      a.cl = ((b.cl & kSplit) ? pushed[v] : c) | kWork;
    } else {
      a.cl = c;
      a.from = a.parent;
    }
  };
  new_cluster(kNone, 0.0, N[n - 1].mass);
  for (u64 k = inner.size(); k-- > 0;) {
    const u32 v = inner[k];
    Node& a = N[v];
    const u32 nbig = a.cl;
    const u64 bigm = a.from;
    pull(v);
    if (!(a.cl & kWork)) continue;
    const u32 c = a.cl & kMask;
    const u32 i0 = att_off[v], i1 = att_off[v + 1];
    for (u32 j = i0; j < i1; ++j) {
      const u32 x = att[j];
      t.point_cluster[x] = c;
      tstab[c] += d.point_weight[x] * (t.point_lambda[x] - tbirth[c]);
    }
    const double lam = lamw[v];
    const double drop = 1u * (lam - tbirth[c]);  // terme de chaque point abandonne (poids 1)
    const u32 j0 = d.child_off[v], j1 = d.child_off[v + 1];
    if (nbig >= 2) {
      for (u32 j = j0; j < j1; ++j) {
        const u32 u = d.child_val[j];
        const u32 mu = N[u].mass;
        if (mu >= mcs) {
          tstab[c] += double(u64(mu)) * (lam - tbirth[c]);
          pushed[u] = new_cluster(c, lam, mu);
        } else {
          for (u32 q = 0; q < mu; ++q) tstab[c] += drop;
        }
      }
      a.cl |= kSplit;
    } else {
      const u64 dropped = u64(a.mass) - (i1 - i0) - bigm;
      for (u64 q = 0; q < dropped; ++q) tstab[c] += drop;
    }
  }
  // feuilles : etat tire du parent (interne, fixe), termes d'une feuille de travail en dernier dans son cluster
  pfor(n, 4096, [&](u64 b, u64 e) {
    for (u64 v = b; v < e; ++v) {
      if (d.child_off[v] < d.child_off[v + 1]) continue;
      pull(static_cast<u32>(v));
      const Node& a = N[v];
      if (!(a.cl & kWork)) continue;
      const u32 c = a.cl & kMask;
      for (u32 j = att_off[v]; j < att_off[v + 1]; ++j) {
        const u32 x = att[j];
        t.point_cluster[x] = c;
        tstab[c] += d.point_weight[x] * (t.point_lambda[x] - tbirth[c]);
      }
    }
  });
  // numeros definitifs : pile LIFO de la reference rejouee sur l'arbre des clusters
  const u32 m = static_cast<u32>(tparent.size());
  std::vector<u32> fin(m, kNone), order;
  order.reserve(m);
  fin[0] = 0;
  order.push_back(0);
  std::vector<u32> stack(1, 0);
  while (!stack.empty()) {
    const u32 c = stack.back();
    stack.pop_back();
    for (u32 k : tkids[c]) {
      fin[k] = static_cast<u32>(order.size());
      order.push_back(k);
    }
    for (u32 k : tkids[c]) stack.push_back(k);
  }
  t.parent.resize(m);
  t.birth.resize(m);
  t.stability.resize(m);
  t.mass.resize(m);
  for (u32 k = 0; k < m; ++k) {
    const u32 o = order[k];
    t.parent[k] = tparent[o] == kNone ? kNone : fin[tparent[o]];
    t.birth[k] = tbirth[o];
    t.stability[k] = tstab[o];
    t.mass[k] = tmass[o];
  }
  pfor(n, 8192, [&](u64 b, u64 e) {
    for (u64 v = b; v < e; ++v) t.node_cluster[v] = fin[N[v].cl & kMask];
  });
  pfor(np, 8192, [&](u64 b, u64 e) {
    for (u64 x = b; x < e; ++x) {
      const Node& a = N[d.point_node[x]];
      if (a.cl & kWork) {
        t.point_cluster[x] = fin[t.point_cluster[x]];
      } else {
        t.point_cluster[x] = fin[a.cl & kMask];
        t.point_lambda[x] = lamw[a.from];
      }
    }
  });
  return t;
}

Clustering cluster(const PointDendrogram& d, const ClusterParams& p, sched::Pool* pool) {
  Clustering out;
  out.tree = condense(d, p, pool);
  const CondensedTree& t = out.tree;
  const u32 m = static_cast<u32>(t.parent.size());
  // enfants de chaque cluster (les enfants ont un indice superieur au parent)
  std::vector<std::vector<u32>> kids(m);
  for (u32 c = 1; c < m; ++c) kids[t.parent[c]].push_back(c);
  std::vector<unsigned char> chosen(m, 0);
  if (p.selection == Selection::eom) {
    std::vector<double> best(m, 0.0);
    for (u32 c = m; c-- > 0;) {
      double sub = 0;
      for (u32 k : kids[c]) sub += best[k];
      const bool is_root = t.parent[c] == kNone;
      if (kids[c].empty()) {
        best[c] = t.stability[c];
        chosen[c] = 1;
      } else if (is_root && !p.allow_single_cluster) {
        best[c] = sub;
      } else if (sub > t.stability[c]) {
        best[c] = sub;
      } else {
        best[c] = t.stability[c];
        chosen[c] = 1;
      }
    }
    // un cluster retenu desactive ses descendants
    for (u32 c = 0; c < m; ++c) {
      if (!chosen[c]) continue;
      for (u32 a = t.parent[c]; a != kNone; a = t.parent[a])
        if (chosen[a]) {
          chosen[c] = 0;
          break;
        }
    }
  } else {
    for (u32 c = 0; c < m; ++c) chosen[c] = kids[c].empty();
  }
  if (!p.allow_single_cluster && m > 0 && m > 1) chosen[0] = 0;
  if (!p.allow_single_cluster && m == 1) chosen[0] = 0;
  std::vector<i32> id(m, -1);
  for (u32 c = 0; c < m; ++c)
    if (chosen[c]) {
      id[c] = static_cast<i32>(out.selected.size());
      out.selected.push_back(c);
    }
  out.cluster_label.assign(m, -1);
  for (u32 c = 0; c < m; ++c)
    for (u32 a = c; a != kNone; a = t.parent[a])
      if (chosen[a]) {
        out.cluster_label[c] = id[a];
        break;
      }
  out.label.assign(d.points(), -1);
  for (u32 x = 0; x < d.points(); ++x) {
    for (u32 c = t.point_cluster[x]; c != kNone; c = t.parent[c])
      if (chosen[c]) {
        out.label[x] = id[c];
        break;
      }
  }
  return out;
}

}  // namespace mhgp10
