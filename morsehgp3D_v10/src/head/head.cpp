#include "head/head.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace mhgp10 {

namespace {

double lambda_of(double level, double z) {
  if (level <= 0) return std::numeric_limits<double>::infinity();
  return std::pow(level, -0.5 * z);
}

}  // namespace

CondensedTree condense(const PointDendrogram& d, const ClusterParams& p) {
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

Clustering cluster(const PointDendrogram& d, const ClusterParams& p) {
  Clustering out;
  out.tree = condense(d, p);
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
