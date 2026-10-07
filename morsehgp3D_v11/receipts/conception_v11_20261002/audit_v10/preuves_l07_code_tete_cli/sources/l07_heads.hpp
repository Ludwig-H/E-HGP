// Audit L07 (2 octobre 2026) : tetes de reference de l'auditeur, hors produit.
//   condense_x(d, p, cohort=false) : port ligne a ligne de mhgp10::condense (afb081774) ; doit rendre les memes
//                                    stabilites, bit pour bit.
//   condense_x(d, p, cohort=true)  : meme parcours, meme ordre de sommation, mais le seuil min_cluster_size est aussi
//                                    teste aux departs des points attaches : des qu'une cohorte (points de meme rang)
//                                    laisse une masse < mcs dans un noeud, tout ce qui reste sort a ce lambda.
//   normalize(d)                   : contracte les noeuds de meme rang que leur parent et remonte au parent les points
//                                    dont l'entree egale le rang du parent (coupe stricte : plateaux atomiques).
//   select_x                       : copie de la selection de mhgp10::cluster, avec compteurs de marche.
#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "head/head.hpp"

namespace l07 {

using namespace mhgp10;

inline double lambda_of(double level, double z) {
  if (level <= 0) return std::numeric_limits<double>::infinity();
  return std::pow(level, -0.5 * z);
}

struct Trig {
  u64 nodes = 0;      // noeuds ou une cohorte fait passer la masse sous mcs en laissant un reste non vide
  u64 rest_mass = 0;  // masse du reste (points et sous-arbres qui sortent plus tot que dans la tete v10)
  u64 heavy = 0;      // points de poids >= mcs rencontres (hors du domaine de cette reference)
};

inline CondensedTree condense_x(const PointDendrogram& d, const ClusterParams& p, bool cohort, Trig* trig = nullptr) {
  const u32 n = d.nodes();
  std::vector<u64> mass(n, 0);
  std::vector<u32> att_off(n + 1, 0), att(d.points());
  for (u32 x = 0; x < d.points(); ++x) ++att_off[d.point_node[x] + 1];
  for (u32 v = 0; v < n; ++v) att_off[v + 1] += att_off[v];
  {
    std::vector<u32> fill(att_off.begin(), att_off.end() - 1);
    for (u32 x = 0; x < d.points(); ++x) att[fill[d.point_node[x]]++] = x;
  }
  for (u32 v = 0; v < n; ++v) {
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
  std::vector<std::pair<u32, u32>> work;
  work.push_back({root, new_cluster(kNone, 0.0, mass[root])});
  std::vector<u32> big;
  std::vector<std::pair<u32, u32>> coh;  // (rang, poids) des points attaches du noeud courant
  while (!work.empty()) {
    auto [v, c] = work.back();
    work.pop_back();
    t.node_cluster[v] = c;
    const bool has_children = d.child_off[v] != d.child_off[v + 1];
    u32 rstar = kNone;  // rang de la cohorte qui fait passer la masse du noeud sous mcs
    if (cohort && att_off[v] != att_off[v + 1]) {
      coh.clear();
      for (u32 j = att_off[v]; j < att_off[v + 1]; ++j) {
        coh.push_back({d.point_rank[att[j]], d.point_weight[att[j]]});
        if (trig && d.point_weight[att[j]] >= p.min_cluster_size) ++trig->heavy;
      }
      std::sort(coh.begin(), coh.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
      u64 remaining = mass[v];
      for (size_t i = 0; i < coh.size();) {
        const u32 r = coh[i].first;
        u64 w = 0;
        size_t e = i;
        while (e < coh.size() && coh[e].first == r) w += coh[e++].second;
        if (has_children && r == d.node_rank[v]) break;  // evenement final : c'est la division geometrique
        const u64 rest = remaining - w;
        if (rest < p.min_cluster_size) {
          rstar = r;
          if (rest > 0 && trig) {
            ++trig->nodes;
            trig->rest_mass += rest;
          }
          break;
        }
        remaining = rest;
        i = e;
      }
    }
    for (u32 j = att_off[v]; j < att_off[v + 1]; ++j) {
      const u32 x = att[j];
      u32 r = d.point_rank[x];
      if (rstar != kNone && r < rstar) r = rstar;
      const double lam = lambda_of(d.level[r], p.z);
      t.point_cluster[x] = c;
      t.point_lambda[x] = lam;
      t.stability[c] += d.point_weight[x] * (lam - t.birth[c]);
    }
    if (rstar != kNone) {
      const double lam = lambda_of(d.level[rstar], p.z);
      for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j) drop_subtree(d.child_val[j], c, lam);
      continue;
    }
    if (!has_children) continue;
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

struct Walk {
  u64 deactivate = 0, cluster_label = 0, point_label = 0;  // pas de remontee des trois boucles de cluster()
  u32 max_depth = 0;                                       // profondeur maximale de l'arbre condense
};

inline Clustering select_x(CondensedTree tree, u32 points, const ClusterParams& p, Walk* walk = nullptr) {
  Clustering out;
  out.tree = std::move(tree);
  const CondensedTree& t = out.tree;
  const u32 m = static_cast<u32>(t.parent.size());
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
    for (u32 c = 0; c < m; ++c) {
      if (!chosen[c]) continue;
      for (u32 a = t.parent[c]; a != kNone; a = t.parent[a]) {
        if (walk) ++walk->deactivate;
        if (chosen[a]) {
          chosen[c] = 0;
          break;
        }
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
    for (u32 a = c; a != kNone; a = t.parent[a]) {
      if (walk) ++walk->cluster_label;
      if (chosen[a]) {
        out.cluster_label[c] = id[a];
        break;
      }
    }
  out.label.assign(points, -1);
  for (u32 x = 0; x < points; ++x) {
    for (u32 c = t.point_cluster[x]; c != kNone; c = t.parent[c]) {
      if (walk) ++walk->point_label;
      if (chosen[c]) {
        out.label[x] = id[c];
        break;
      }
    }
  }
  if (walk) {
    std::vector<u32> depth(m, 0);
    for (u32 c = 1; c < m; ++c) {
      depth[c] = depth[t.parent[c]] + 1;
      walk->max_depth = std::max(walk->max_depth, depth[c]);
    }
  }
  return out;
}

struct NormStats {
  u64 contracted = 0, relocated = 0;
};

inline PointDendrogram normalize(const PointDendrogram& d, NormStats* st = nullptr) {
  const u32 n = d.nodes();
  std::vector<u32> top(n);
  for (u32 v = n; v-- > 0;) {
    const u32 par = d.parent[v];
    top[v] = (par != kNone && d.node_rank[par] == d.node_rank[v]) ? top[par] : v;
  }
  std::vector<u32> newid(n, kNone);
  u32 m = 0;
  for (u32 v = 0; v < n; ++v)
    if (top[v] == v) newid[v] = m++;
  PointDendrogram o;
  o.level = d.level;
  o.node_rank.resize(m);
  o.parent.assign(m, kNone);
  for (u32 v = 0; v < n; ++v)
    if (top[v] == v) {
      o.node_rank[newid[v]] = d.node_rank[v];
      if (d.parent[v] != kNone) o.parent[newid[v]] = newid[top[d.parent[v]]];
    }
  o.child_off.assign(m + 1, 0);
  for (u32 v = 0; v < m; ++v)
    if (o.parent[v] != kNone) ++o.child_off[o.parent[v] + 1];
  for (u32 v = 0; v < m; ++v) o.child_off[v + 1] += o.child_off[v];
  o.child_val.resize(o.child_off[m]);
  {
    std::vector<u32> fill(o.child_off.begin(), o.child_off.end() - 1);
    for (u32 v = 0; v < m; ++v)
      if (o.parent[v] != kNone) o.child_val[fill[o.parent[v]]++] = v;
  }
  o.point_node.resize(d.points());
  o.point_rank = d.point_rank;
  o.point_weight = d.point_weight;
  u64 moved = 0;
  for (u32 x = 0; x < d.points(); ++x) {
    u32 v = top[d.point_node[x]];
    if (d.parent[v] != kNone) {
      const u32 par = top[d.parent[v]];
      if (d.point_rank[x] == d.node_rank[par]) {
        v = par;
        ++moved;
      }
    }
    o.point_node[x] = newid[v];
  }
  if (st) {
    st->contracted = n - m;
    st->relocated = moved;
  }
  return o;
}

}  // namespace l07
