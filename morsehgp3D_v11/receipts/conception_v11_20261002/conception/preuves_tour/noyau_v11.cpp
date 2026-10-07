// Controle de conception de la tour v11 (2 octobre 2026) : noyau SANS LOTS sur les entrees reelles du Kruskal de la
// v10 (vidages kruskal_k<K>.bin de l'audit L06 : naissances, jonctions, rangs, representants resolus).
//   A  reference : Kruskal par lots de la v10 (copie de preuves_l06_code_tour/krbench/krbench.cpp, fonction kruskal_lots)
//   B  noyau v11 : union-find arete par arete, union par TAILLE, cellule emballee, evenements (rang, operandes,
//      plus petite feuille, survivant), historique d'attache (perdant -> survivant, rang), sommet apres chaque jonction
//   C  materialisation : contraction des evenements de meme rang relies -> noeuds N-aires, numerotation (rang, plus
//      petite feuille)
//   D  historique : requete component_at(feuille, rang) ; image d'une jonction par jtop + une remontee conditionnelle
// Sorties : egalite des forets A et C (signature canonique), egalite des deux calculs d'image, compteurs deterministes.
// Les temps sont des minima de passes entrelacees sur une machine chargee : seuls les RAPPORTS sont a lire.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <vector>
using u32 = std::uint32_t;
using u64 = std::uint64_t;
static const u32 kNone = 0xFFFFFFFFu;
static const u32 kEv = 0x80000000u;  // operande : bit de poids fort = evenement, sinon feuille
using Clock = std::chrono::steady_clock;
static double since(Clock::time_point t) { return std::chrono::duration<double>(Clock::now() - t).count(); }

struct In {
  u32 nb = 0, nj = 0, nr = 0;
  std::vector<u32> jrank, joff, rep, brank;
};
static In load(const char* path) {
  In in;
  FILE* f = std::fopen(path, "rb");
  if (!f) std::exit(2);
  u32 hdr[4];
  if (std::fread(hdr, 4, 4, f) != 4) std::exit(2);
  in.nb = hdr[0], in.nj = hdr[1], in.nr = hdr[2];
  in.jrank.resize(in.nj), in.joff.resize(u64(in.nj) + 1), in.rep.resize(in.nr), in.brank.resize(in.nb);
  if (std::fread(in.jrank.data(), 4, in.nj, f) != in.nj) std::exit(2);
  if (std::fread(in.joff.data(), 4, u64(in.nj) + 1, f) != u64(in.nj) + 1) std::exit(2);
  if (std::fread(in.rep.data(), 4, in.nr, f) != in.nr) std::exit(2);
  if (std::fread(in.brank.data(), 4, in.nb, f) != in.nb) std::exit(2);
  std::fclose(f);
  return in;
}

// ---- A : reference par lots (semantique de la v10)
struct Forest {
  std::vector<u32> rank, parent;
};
static void kruskal_lots(const In& in, Forest& out) {
  const u32 nb = in.nb, nj = in.nj;
  out.rank.assign(in.brank.begin(), in.brank.end());
  out.parent.assign(nb, kNone);
  std::vector<u32> dsu(nb), top(nb);
  std::iota(dsu.begin(), dsu.end(), 0u);
  std::iota(top.begin(), top.end(), 0u);
  auto find = [&](u32 x) {
    while (dsu[x] != x) x = dsu[x] = dsu[dsu[x]];
    return x;
  };
  std::vector<u32> pre, kids;
  std::vector<std::pair<u32, u32>> members;
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
        for (size_t t = a; t < b; ++t) out.parent[top[members[t].second]] = node;
        top[members[a].first] = node;
      }
      a = b;
    }
    i = j;
  }
}

// ---- B : noyau v11
struct Cell {
  u32 up, size, last, minleaf;  // 16 octets : une demi-ligne de cache
};
struct Kernel {
  std::vector<Cell> cell;
  std::vector<u32> ev_rank, ev_a, ev_b, ev_min, ev_surv;
  std::vector<u32> att_parent, att_rank, jtop;
  u64 finds = 0, find_steps = 0;
};
static void kernel(const In& in, Kernel& k, bool count) {
  const u32 nb = in.nb, nj = in.nj;
  k.cell.resize(nb);
  for (u32 i = 0; i < nb; ++i) k.cell[i] = Cell{i, 1, kNone, i};
  k.ev_rank.clear(), k.ev_a.clear(), k.ev_b.clear(), k.ev_min.clear(), k.ev_surv.clear();
  const u32 cap = nb ? nb - 1 : 0;
  k.ev_rank.reserve(cap), k.ev_a.reserve(cap), k.ev_b.reserve(cap), k.ev_min.reserve(cap), k.ev_surv.reserve(cap);
  k.att_parent.assign(nb, kNone);
  k.att_rank.assign(nb, 0);
  k.jtop.resize(nj);
  Cell* c = k.cell.data();
  u64 finds = 0, steps = 0;
  auto find = [&](u32 x) {
    while (c[x].up != x) {
      c[x].up = c[c[x].up].up;
      x = c[x].up;
      if (count) ++steps;
    }
    if (count) ++finds;
    return x;
  };
  const u32* rep = in.rep.data();
  for (u32 t = 0; t < nj; ++t) {
    const u32 b0 = in.joff[t], e0 = in.joff[t + 1];
    for (u32 r = b0 + 16; r < e0 + 16 && r < in.nr; ++r) __builtin_prefetch(&c[rep[r]]);
    const u32 rk = in.jrank[t];
    u32 x = find(rep[b0]);
    for (u32 r = b0 + 1; r < e0; ++r) {
      const u32 y = find(rep[r]);
      if (x == y) continue;
      const u32 e = static_cast<u32>(k.ev_rank.size());
      const u32 a = c[x].last == kNone ? x : (c[x].last | kEv), b = c[y].last == kNone ? y : (c[y].last | kEv);
      const u32 ml = std::min(c[x].minleaf, c[y].minleaf);
      u32 s = x, l = y;
      if (c[x].size < c[y].size) s = y, l = x;
      k.ev_rank.push_back(rk), k.ev_a.push_back(a), k.ev_b.push_back(b), k.ev_min.push_back(ml), k.ev_surv.push_back(s);
      c[l].up = s;
      c[s].size += c[l].size;
      c[s].minleaf = ml;
      c[s].last = e;
      k.att_parent[l] = s;
      k.att_rank[l] = rk;
      x = s;
    }
    k.jtop[t] = c[x].last == kNone ? x : (c[x].last | kEv);
  }
  k.finds = finds, k.find_steps = steps;
}

// ---- C : materialisation (version sequentielle de controle ; par groupes contigus de meme rang)
struct Mat {
  std::vector<u32> rank, parent, nid;  // nid : evenement -> noeud N-aire
  u64 groups = 0, groups_multi = 0, max_group = 0, merges = 0;
};
static void materialize(const In& in, const Kernel& k, Mat& m) {
  const u32 nb = in.nb, ne = static_cast<u32>(k.ev_rank.size());
  std::vector<u32> loc(ne);
  std::iota(loc.begin(), loc.end(), 0u);
  auto find = [&](u32 x) {
    while (loc[x] != x) x = loc[x] = loc[loc[x]];
    return x;
  };
  for (u32 e = 0; e < ne; ++e)
    for (u32 op : {k.ev_a[e], k.ev_b[e]})
      if ((op & kEv) && k.ev_rank[op & ~kEv] == k.ev_rank[e]) loc[find(op & ~kEv)] = find(e);
  // cles (rang, plus petite feuille) des ensembles ; les evenements d'un rang sont contigus
  std::vector<std::array<u32, 3>> keys;  // rang, minleaf, racine locale
  std::vector<u32> setmin(ne, kNone);
  for (u32 e = 0; e < ne; ++e) {
    const u32 r = find(e);
    setmin[r] = std::min(setmin[r], k.ev_min[e]);
  }
  for (u32 e = 0; e < ne; ++e)
    if (find(e) == e) keys.push_back({k.ev_rank[e], setmin[e], e});
  std::sort(keys.begin(), keys.end());
  std::vector<u32> id_of(ne, kNone);
  for (u32 i = 0; i < keys.size(); ++i) id_of[keys[i][2]] = nb + i;
  m.nid.resize(ne);
  for (u32 e = 0; e < ne; ++e) m.nid[e] = id_of[find(e)];
  m.rank.assign(in.brank.begin(), in.brank.end());
  for (const auto& key : keys) m.rank.push_back(key[0]);
  m.parent.assign(m.rank.size(), kNone);
  for (u32 e = 0; e < ne; ++e)
    for (u32 op : {k.ev_a[e], k.ev_b[e]}) {
      if (!(op & kEv)) m.parent[op] = m.nid[e];
      else if (k.ev_rank[op & ~kEv] != k.ev_rank[e]) m.parent[m.nid[op & ~kEv]] = m.nid[e];
    }
  m.merges = keys.size();
  m.groups = m.groups_multi = m.max_group = 0;
  for (u32 e = 0; e < ne;) {
    u32 f = e;
    while (f < ne && k.ev_rank[f] == k.ev_rank[e]) ++f;
    ++m.groups;
    m.groups_multi += (f - e) >= 2;
    m.max_group = std::max<u64>(m.max_group, f - e);
    e = f;
  }
}

static std::vector<std::array<u32, 3>> signature(u32 nb, const std::vector<u32>& rank, const std::vector<u32>& parent) {
  const u32 nn = static_cast<u32>(rank.size());
  std::vector<u32> nkids(nn, 0), minleaf(nn, kNone);
  for (u32 v = 0; v < nb; ++v) minleaf[v] = v;
  for (u32 v = 0; v < nn; ++v) {  // parents apres leurs enfants dans les deux numerotations
    const u32 p = parent[v];
    if (p == kNone) continue;
    ++nkids[p];
    minleaf[p] = std::min(minleaf[p], minleaf[v]);
  }
  std::vector<std::array<u32, 3>> sig;
  for (u32 v = nb; v < nn; ++v) sig.push_back({rank[v], nkids[v], minleaf[v]});
  std::sort(sig.begin(), sig.end());
  return sig;
}

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  const int reps = argc > 2 ? std::atoi(argv[2]) : 5;
  const In in = load(argv[1]);
  Forest fa;
  Kernel k;
  double ta = 1e9, tb = 1e9;
  for (int r = 0; r < reps; ++r) {
    auto t0 = Clock::now();
    kruskal_lots(in, fa);
    ta = std::min(ta, since(t0));
    t0 = Clock::now();
    kernel(in, k, false);
    tb = std::min(tb, since(t0));
  }
  kernel(in, k, true);
  Mat m;
  auto t0 = Clock::now();
  materialize(in, k, m);
  const double tc = since(t0);
  const bool same = signature(in.nb, fa.rank, fa.parent) == signature(in.nb, m.rank, m.parent);
  bool numbering = fa.rank == m.rank && fa.parent == m.parent;  // meme numerotation (rang, plus petite feuille)
  // ---- D : historique et images
  const u32 nb = in.nb, ne = static_cast<u32>(k.ev_rank.size());
  std::vector<u32> off(u64(nb) + 1, 0);
  for (u32 e = 0; e < ne; ++e) ++off[k.ev_surv[e] + 1];
  for (u32 i = 0; i < nb; ++i) off[i + 1] += off[i];
  std::vector<u32> hr(ne), hn(ne), fill(off.begin(), off.end() - 1);
  for (u32 e = 0; e < ne; ++e) {
    const u32 p = fill[k.ev_surv[e]]++;
    hr[p] = k.ev_rank[e];
    hn[p] = m.nid[e];
  }
  u64 hops = 0, probes = 0, bad = 0, maxhops = 0, maxlist = 0;
  for (u32 i = 0; i < nb; ++i) maxlist = std::max<u64>(maxlist, off[i + 1] - off[i]);
  for (u32 t = 0; t < in.nj; ++t) {
    const u32 rk = in.jrank[t];
    u32 x = in.rep[in.joff[t]];
    u64 h = 0;
    while (k.att_parent[x] != kNone && k.att_rank[x] <= rk) x = k.att_parent[x], ++h;
    hops += h;
    maxhops = std::max(maxhops, h);
    u32 lo = off[x], hi = off[x + 1];
    while (lo < hi) {
      const u32 mid = lo + (hi - lo) / 2;
      ++probes;
      if (hr[mid] <= rk) lo = mid + 1;
      else hi = mid;
    }
    const u32 q = lo == off[x] ? x : hn[lo - 1];
    u32 v = (k.jtop[t] & kEv) ? m.nid[k.jtop[t] & ~kEv] : k.jtop[t];
    if (m.parent[v] != kNone && m.rank[m.parent[v]] <= rk) v = m.parent[v];
    bad += q != v;
  }
  std::printf("{\"file\":\"%s\",\"births\":%u,\"joins\":%u,\"reps\":%u,\"events\":%u,\"merges_lots\":%zu,\"merges_v11\":%llu,"
              "\"same_forest\":%s,\"same_numbering\":%s,\"rank_groups\":%llu,\"rank_groups_multi\":%llu,\"max_group_events\":%llu,"
              "\"find_steps_per_find\":%.3f,\"image_queries\":%u,\"image_mismatch\":%llu,\"attach_hops_mean\":%.3f,"
              "\"attach_hops_max\":%llu,\"bsearch_probes_mean\":%.2f,\"max_events_one_root\":%llu,"
              "\"t_lots_ms\":%.3f,\"t_kernel_v11_ms\":%.3f,\"ratio_lots_over_kernel\":%.2f,\"t_materialize_seq_ms\":%.3f}\n",
              argv[1], in.nb, in.nj, in.nr, ne, fa.rank.size() - in.nb, (unsigned long long)m.merges, same ? "true" : "false",
              numbering ? "true" : "false", (unsigned long long)m.groups, (unsigned long long)m.groups_multi,
              (unsigned long long)m.max_group, k.finds ? double(k.find_steps) / double(k.finds) : 0.0, in.nj,
              (unsigned long long)bad, in.nj ? double(hops) / in.nj : 0.0, (unsigned long long)maxhops,
              in.nj ? double(probes) / in.nj : 0.0, (unsigned long long)maxlist, ta * 1e3, tb * 1e3, ta / tb, tc * 1e3);
  return same && bad == 0 ? 0 : 1;
}
