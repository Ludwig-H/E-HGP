// Banc independant L06 : cout du Kruskal par lots de la v10 (copie fidele) contre un noyau union-find arete par arete
// (sans lots, sans noeuds), et contre un noyau qui materialise les multifusions a la volee. Entrees : vidages
// kruskal_k<K>.bin produits par la copie instrumentee (naissances, jonctions, rangs, representants resolus).
// Sorties comparees : nombre de fusions, multiensemble (rang, nombre d'enfants) et partition finale.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <vector>
using u32 = std::uint32_t;
using u64 = std::uint64_t;
static const u32 kNone = 0xFFFFFFFFu;
using Clock = std::chrono::steady_clock;
static double since(Clock::time_point t) { return std::chrono::duration<double>(Clock::now() - t).count(); }

struct In {
  u32 nb = 0, nj = 0, nr = 0, site_births = 0;
  std::vector<u32> jrank, joff, rep, brank;
};
struct Forest {
  std::vector<u32> rank, parent, child_off, child_val;
  u64 merges = 0;
};

// (A) copie fidele de tower.cpp:1041-1116 (meme structure de donnees, memes tris).
static void kruskal_lots(const In& in, Forest& out) {
  const u32 nb = in.nb, nj = in.nj;
  out.rank.clear();
  out.rank.reserve(nb + nj);
  for (u32 i = 0; i < nb; ++i) out.rank.push_back(in.brank[i]);
  out.parent.assign(nb, kNone);
  out.parent.reserve(nb + nj);
  out.child_off.assign(nb + 1, 0);
  out.child_off.reserve(nb + nj + 1);
  out.child_val.clear();
  out.merges = 0;
  std::vector<u32> dsu(nb), top(nb);
  for (u32 i = 0; i < nb; ++i) dsu[i] = top[i] = i;
  auto find = [&](u32 x) {
    while (dsu[x] != x) x = dsu[x] = dsu[dsu[x]];
    return x;
  };
  std::vector<u32> pre;
  std::vector<std::pair<u32, u32>> members;
  std::vector<u32> kids;
  for (u32 i = 0; i < nj;) {
    const u32 rk = in.jrank[i];
    u32 j = i;
    while (j < nj && in.jrank[j] == rk) ++j;
    pre.clear();
    for (u32 r = in.joff[i]; r < in.joff[j]; ++r) pre.push_back(find(in.rep[r]));
    for (u32 t = i; t < j; ++t) {
      const u32 r0 = in.joff[t] - in.joff[i];
      for (u32 r = r0 + 1; r < in.joff[t + 1] - in.joff[i]; ++r) {
        const u32 x = find(pre[r0]), y = find(pre[r]);
        if (x != y) dsu[std::max(x, y)] = std::min(x, y);
      }
    }
    members.clear();
    for (u32 r : pre) members.push_back({find(r), r});
    std::sort(members.begin(), members.end());
    members.erase(std::unique(members.begin(), members.end()), members.end());
    for (size_t a = 0; a < members.size();) {
      size_t b = a;
      while (b < members.size() && members[b].first == members[a].first) ++b;
      if (b - a >= 2) {
        const u32 node = static_cast<u32>(out.rank.size());
        out.rank.push_back(rk);
        out.parent.push_back(kNone);
        kids.clear();
        for (size_t t = a; t < b; ++t) kids.push_back(top[members[t].second]);
        std::sort(kids.begin(), kids.end());
        for (u32 kid : kids) {
          out.child_val.push_back(kid);
          out.parent[kid] = node;
        }
        out.child_off.push_back(static_cast<u32>(out.child_val.size()));
        top[members[a].first] = node;
        ++out.merges;
      }
      a = b;
    }
    i = j;
  }
}

// (B) noyau minimal : union-find arete par arete (etoile rep0 - rep_r), union par taille, demi-compression ; n'ecrit
// que la liste des unions reussies (rang, racine a, racine b). Aucune structure de lot, aucun noeud.
struct Ev {
  u32 rank, a, b;
};
static u64 kernel_edges(const In& in, std::vector<Ev>& ev) {
  const u32 nb = in.nb, nj = in.nj;
  std::vector<u32> dsu(nb), sz(nb, 1);
  std::iota(dsu.begin(), dsu.end(), 0u);
  auto find = [&](u32 x) {
    while (dsu[x] != x) x = dsu[x] = dsu[dsu[x]];
    return x;
  };
  ev.clear();
  ev.reserve(nb);
  for (u32 t = 0; t < nj; ++t) {
    const u32 b0 = in.joff[t], e0 = in.joff[t + 1];
    u32 x = find(in.rep[b0]);
    for (u32 r = b0 + 1; r < e0; ++r) {
      u32 y = find(in.rep[r]);
      if (x == y) continue;
      ev.push_back({in.jrank[t], x, y});
      if (sz[x] < sz[y]) std::swap(x, y);
      dsu[y] = x;
      sz[x] += sz[y];
    }
  }
  return ev.size();
}

// (C) noyau + materialisation a la volee des multifusions (noeud du plateau absorbe les suivants ; deux noeuds du
// meme rang qui se rencontrent sont fusionnes par alias). Sortie : parents et rangs, enfants en CSR par une passe.
static void kernel_nary(const In& in, Forest& out) {
  const u32 nb = in.nb, nj = in.nj;
  std::vector<u32> dsu(nb), sz(nb, 1), top(nb);
  std::iota(dsu.begin(), dsu.end(), 0u);
  std::iota(top.begin(), top.end(), 0u);
  auto find = [&](u32 x) {
    while (dsu[x] != x) x = dsu[x] = dsu[dsu[x]];
    return x;
  };
  std::vector<u32>& rank = out.rank;
  std::vector<u32>& parent = out.parent;
  rank.assign(in.brank.begin(), in.brank.end());
  rank.reserve(2 * nb);
  parent.assign(nb, kNone);
  parent.reserve(2 * nb);
  std::vector<u32> alias;  // noeud de fusion -> representant (kNone : lui-meme)
  alias.reserve(nb);
  auto canon = [&](u32 v) {
    while (v >= nb && alias[v - nb] != kNone) v = alias[v - nb];
    return v;
  };
  for (u32 t = 0; t < nj; ++t) {
    const u32 b0 = in.joff[t], e0 = in.joff[t + 1];
    const u32 rk = in.jrank[t];
    u32 x = find(in.rep[b0]);
    for (u32 r = b0 + 1; r < e0; ++r) {
      u32 y = find(in.rep[r]);
      if (x == y) continue;
      u32 tx = canon(top[x]), ty = canon(top[y]);
      u32 node;
      const bool mx = tx >= nb && rank[tx] == rk, my = ty >= nb && rank[ty] == rk;
      if (mx && my) {
        alias[ty - nb] = tx;  // deux noeuds du meme plateau : un seul noeud
        node = tx;
      } else if (mx) {
        parent[ty] = tx;
        node = tx;
      } else if (my) {
        parent[tx] = ty;
        node = ty;
      } else {
        node = static_cast<u32>(rank.size());
        rank.push_back(rk);
        parent.push_back(kNone);
        alias.push_back(kNone);
        parent[tx] = node;
        parent[ty] = node;
      }
      if (sz[x] < sz[y]) std::swap(x, y);
      dsu[y] = x;
      sz[x] += sz[y];
      top[x] = node;
    }
  }
  // resolution des alias dans les parents (une passe)
  u64 merges = 0;
  for (u32 v = 0; v < rank.size(); ++v) {
    if (v >= nb && alias[v - nb] != kNone) continue;
    if (v >= nb) ++merges;
    if (parent[v] != kNone) parent[v] = canon(parent[v]);
  }
  out.merges = merges;
  // enfants d'un noeud alias : leur parent pointe encore sur l'alias, resolu ci-dessus par canon
}

static In load(const char* path) {
  In in;
  FILE* f = std::fopen(path, "rb");
  if (!f) {
    std::perror(path);
    std::exit(2);
  }
  u32 hdr[4];
  if (std::fread(hdr, 4, 4, f) != 4) std::exit(2);
  in.nb = hdr[0];
  in.nj = hdr[1];
  in.nr = hdr[2];
  in.site_births = hdr[3];
  in.jrank.resize(in.nj);
  in.joff.resize(u64(in.nj) + 1);
  in.rep.resize(in.nr);
  in.brank.resize(in.nb);
  if (std::fread(in.jrank.data(), 4, in.nj, f) != in.nj) std::exit(2);
  if (std::fread(in.joff.data(), 4, u64(in.nj) + 1, f) != u64(in.nj) + 1) std::exit(2);
  if (std::fread(in.rep.data(), 4, in.nr, f) != in.nr) std::exit(2);
  if (std::fread(in.brank.data(), 4, in.nb, f) != in.nb) std::exit(2);
  std::fclose(f);
  return in;
}

// signature canonique d'une foret : multiensemble trie des (rang, nombre d'enfants, plus petite feuille du sous-arbre)
static std::vector<std::array<u32, 3>> signature(const In& in, const std::vector<u32>& rank, const std::vector<u32>& parent,
                                                  const std::vector<u32>* alias_free_mask) {
  (void)alias_free_mask;
  const u32 nn = static_cast<u32>(rank.size());
  std::vector<u32> nkids(nn, 0), minleaf(nn, kNone);
  for (u32 v = 0; v < in.nb; ++v) minleaf[v] = v;
  // les parents ont un indice plus grand que leurs enfants dans (A) ; dans (C) aussi, sauf alias : iterer jusqu'au point fixe
  bool changed = true;
  while (changed) {
    changed = false;
    for (u32 v = 0; v < nn; ++v) {
      const u32 p = parent[v];
      if (p == kNone || minleaf[v] == kNone) continue;
      if (minleaf[v] < minleaf[p]) {
        minleaf[p] = minleaf[v];
        changed = true;
      }
    }
  }
  for (u32 v = 0; v < nn; ++v)
    if (parent[v] != kNone) ++nkids[parent[v]];
  std::vector<std::array<u32, 3>> sig;
  for (u32 v = in.nb; v < nn; ++v)
    if (nkids[v] > 0) sig.push_back({rank[v], nkids[v], minleaf[v]});
  std::sort(sig.begin(), sig.end());
  return sig;
}

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  const int reps = argc > 2 ? std::atoi(argv[2]) : 15;
  const In in = load(argv[1]);
  double ta = 1e9, tb = 1e9, tc = 1e9;
  Forest fa, fc;
  std::vector<Ev> ev;
  u64 unions = 0;
  for (int r = 0; r < reps; ++r) {  // passes entrelacees : le minimum de chaque variante
    auto t0 = Clock::now();
    kruskal_lots(in, fa);
    ta = std::min(ta, since(t0));
    t0 = Clock::now();
    unions = kernel_edges(in, ev);
    tb = std::min(tb, since(t0));
    t0 = Clock::now();
    kernel_nary(in, fc);
    tc = std::min(tc, since(t0));
  }
  const auto sa = signature(in, fa.rank, fa.parent, nullptr);
  const auto sc = signature(in, fc.rank, fc.parent, nullptr);
  u64 kids_a = fa.child_val.size();
  std::printf("{\"file\":\"%s\",\"births\":%u,\"joins\":%u,\"reps\":%u,\"merges_lots\":%llu,\"children_lots\":%llu,"
              "\"unions_kernel\":%llu,\"merges_nary\":%llu,\"same_forest\":%s,\"t_lots_ms\":%.3f,\"t_kernel_ms\":%.3f,"
              "\"t_nary_ms\":%.3f,\"ratio_lots_over_kernel\":%.2f,\"ratio_lots_over_nary\":%.2f}\n",
              argv[1], in.nb, in.nj, in.nr, (unsigned long long)fa.merges, (unsigned long long)kids_a,
              (unsigned long long)unions, (unsigned long long)fc.merges, sa == sc ? "true" : "false", ta * 1e3, tb * 1e3,
              tc * 1e3, ta / tb, ta / tc);
  return sa == sc ? 0 : 1;
}
