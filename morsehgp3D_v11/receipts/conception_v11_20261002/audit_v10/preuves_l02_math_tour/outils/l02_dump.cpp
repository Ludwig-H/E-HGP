// Sonde d'audit L02 (hors depot) : catalogue (kcat) -> tour FULL 1..K -> dump enrichi + invariant d'Euler.
//   l02_dump IN.u32le --k=K [--kcat=KC] [--threads=W] [--dump=FILE] [--no-points] [--no-euler] [--only-euler]
// Dump : sites, puis par ordre les noeuds (parent, niveau exact, image verticale, boule de naissance et population
// I u U en indices de sites) et les attaches core. Sortie standard : une ligne JSON (statistiques, Euler).
// Lecture seule de l'API publique de mhgp10_core ; aucun acces aux internes de tower.cpp.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "catalogue/support.hpp"
#include "tower/tower.hpp"

using namespace mhgp10;

static long long binom_ll(int n, int k) {
  if (k < 0 || k > n) return 0;
  __int128 r = 1;
  for (int i = 1; i <= k; ++i) r = r * (n - k + i) / i;
  return (long long)r;
}

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  int kmax = 5, kcat = 0;
  unsigned threads = 2;
  std::string dump;
  bool points = true, euler = true, only_euler = false, coherence = false;
  long drop_ball = -1;  // audit : retirer cette boule du catalogue avant Euler et la tour (catalogue ampute)
  bool list_balls = false;
  for (int i = 2; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--k=", 0) == 0) kmax = std::stoi(a.substr(4));
    else if (a.rfind("--kcat=", 0) == 0) kcat = std::stoi(a.substr(7));
    else if (a.rfind("--threads=", 0) == 0) threads = unsigned(std::stoul(a.substr(10)));
    else if (a.rfind("--dump=", 0) == 0) dump = a.substr(7);
    else if (a == "--no-points") points = false;
    else if (a == "--no-euler") euler = false;
    else if (a == "--only-euler") only_euler = true;
    else if (a == "--coherence") coherence = true;
    else if (a.rfind("--drop-ball=", 0) == 0) drop_ball = std::stol(a.substr(12));
    else if (a == "--list-balls") list_balls = true;
    else return 2;
  }
  if (kcat == 0) kcat = kmax;
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::vector<u32> raw;
  u32 buf[3];
  while (std::fread(buf, 4, 3, f) == 3) raw.insert(raw.end(), buf, buf + 3);
  std::fclose(f);
  const u32 n = static_cast<u32>(raw.size() / 3);
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) {
    x[i] = raw[3 * i];
    y[i] = raw[3 * i + 1];
    z[i] = raw[3 * i + 2];
    pid[i] = i;
  }
  using clk = std::chrono::steady_clock;
  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  if (!prepared.ok()) {
    std::printf("{\"status\":\"refus\",\"stage\":\"cloud\",\"reason\":\"%s\"}\n",
                std::string(reason_name(prepared.outcome().reason)).c_str());
    return 2;
  }
  const Cloud& cloud = prepared.value();
  sched::Pool pool(threads);
  SiteTree tree(cloud);
  CatalogueParams cp;
  cp.kmax = kcat;
  const auto t0 = clk::now();
  auto built = build_catalogue(cloud, cp, pool);
  const auto t1 = clk::now();
  if (!built.ok()) {
    std::printf("{\"status\":\"refus\",\"stage\":\"catalogue\",\"reason\":\"%s\"}\n",
                std::string(reason_name(built.outcome().reason)).c_str());
    return 2;
  }
  Catalogue amputated;
  if (drop_ball >= 0) {  // copie du catalogue sans la boule drop_ball (ordre, rangs et niveaux conserves)
    const Catalogue& c = built.value();
    if (drop_ball >= (long)c.balls()) return 2;
    amputated.kmax = c.kmax;
    amputated.level = c.level;
    amputated.pop_off.push_back(0);
    for (u32 b = 0; b < c.balls(); ++b) {
      if ((long)b == drop_ball) continue;
      amputated.rank.push_back(c.rank[b]);
      amputated.support.push_back(c.support[b]);
      amputated.qmin.push_back(c.qmin[b]);
      amputated.p.push_back(c.p[b]);
      amputated.u.push_back(c.u[b]);
      amputated.flags.push_back(c.flags[b]);
      amputated.n_interior.push_back(c.n_interior[b]);
      for (u64 q = c.pop_off[b]; q < c.pop_off[b + 1]; ++q) amputated.pop.push_back(c.pop[q]);
      amputated.pop_off.push_back(amputated.pop.size());
    }
  }
  const Catalogue& cat = drop_ball >= 0 ? amputated : built.value();
  const u32 nb = cat.balls();
  const u32 ns = cloud.sites();
  // ---- Euler : pour 1 <= K <= min(ns, kcat - 2) : [K = 1] ns + somme_b e_K(b) = 1
  //      e_K(b) = somme_s N_s(b) C(s - 1, K - 1 - p) (-1)^(s - 1 - (K - 1 - p)), N_s = nombre de parties T de U,
  //      |T| = s, dont l'enveloppe convexe fermee contient le centre (boule reguliere : N_m = 1 seul).
  std::vector<__int128> esum(kcat + 3, 0);
  u64 ext_balls = 0, ext_skipped = 0, ext_maxm = 0, weighted = 0;
  std::map<u32, u64> ext_m_hist;
  if (euler) {
    std::vector<geom::P3> PV(ns);
    for (u32 s = 0; s < ns; ++s) PV[s] = geom::P3{i64(cloud.x[s]), i64(cloud.y[s]), i64(cloud.z[s])};
    auto P = [&](u32 s) -> const geom::P3& { return PV[s]; };
    for (u32 b = 0; b < nb; ++b) {
      const auto U = cat.shell(b);
      const int m = (int)U.size();
      const int p = (int)cat.p[b];
      if (cat.flags[b] & kWeightedShell) ++weighted;
      if (!(cat.flags[b] & kExtendedShell)) {
        for (int K = 1; K <= kcat; ++K) {
          const int j = K - 1 - p;
          if (j < 0 || j > m - 1) continue;
          esum[K] += ((m - 1 - j) % 2 ? -1 : 1) * (__int128)binom_ll(m - 1, j);
        }
        continue;
      }
      ++ext_balls;
      ++ext_m_hist[(u32)m];
      ext_maxm = std::max<u64>(ext_maxm, m);
      if (m > 22) {
        ++ext_skipped;
        continue;
      }
      const geom::Center c = ball_center(cloud, cat, b);
      const geom::P3& anchor = P(cat.support[b][0]);
      const u32 full = 1u << m;
      std::vector<u8> nonsep(full, 0);
      std::vector<long long> N(m + 1, 0);
      std::vector<u32> A;
      for (u32 mask = 1; mask < full; ++mask) {
        const int s = __builtin_popcount(mask);
        bool ns_ = false;
        if (s <= 4) {
          A.clear();
          for (int i = 0; i < m; ++i)
            if (mask >> i & 1) A.push_back(U[i]);
          ns_ = center_in_closed_hull(P, std::span<const u32>(A), anchor, c);
        } else {
          for (int i = 0; i < m && !ns_; ++i)
            if (mask >> i & 1) ns_ = nonsep[mask & ~(1u << i)];
        }
        nonsep[mask] = ns_;
        if (ns_) ++N[s];
      }
      for (int K = 1; K <= kcat; ++K) {
        const int j = K - 1 - p;
        if (j < 0) continue;
        __int128 e = 0;
        for (int s = 1; s <= m; ++s) {
          if (!N[s] || j > s - 1) continue;
          e += ((s - 1 - j) % 2 ? -1 : 1) * (__int128)N[s] * binom_ll(s - 1, j);
        }
        esum[K] += e;
      }
    }
  }
  const auto t2 = clk::now();
  std::printf("{\"status\":\"ok\",\"n\":%u,\"sites\":%u,\"K\":%d,\"kcat\":%d,\"threads\":%u,\"balls\":%u,\"levels\":%zu,"
              "\"catalogue_s\":%.3f,\"euler_s\":%.3f,\"ext_balls\":%llu,\"ext_skipped\":%llu,\"ext_max_m\":%llu,"
              "\"weighted_balls\":%llu",
              n, ns, kmax, kcat, pool.size(), nb, cat.level.size(), std::chrono::duration<double>(t1 - t0).count(),
              std::chrono::duration<double>(t2 - t1).count(), (unsigned long long)ext_balls,
              (unsigned long long)ext_skipped, (unsigned long long)ext_maxm, (unsigned long long)weighted);
  if (list_balls) {  // par boule du catalogue COMPLET : [p, q_min, m, etendue]
    const Catalogue& c = built.value();
    std::printf(",\"ball_list\":[");
    for (u32 b = 0; b < c.balls(); ++b)
      std::printf("%s[%u,%u,%zu,%d]", b ? "," : "", c.p[b], (unsigned)c.qmin[b], c.shell(b).size(), (c.flags[b] & kExtendedShell) ? 1 : 0);
    std::printf("]");
  }
  std::printf(",\"ext_m_hist\":{");
  {
    bool first = true;
    for (auto& [mm, c] : ext_m_hist) {
      std::printf("%s\"%u\":%llu", first ? "" : ",", mm, (unsigned long long)c);
      first = false;
    }
  }
  std::printf("}");
  if (euler) {
    std::printf(",\"euler\":[");
    for (int K = 1; K <= kcat; ++K) {
      const __int128 v = esum[K] + (K == 1 ? (__int128)ns : 0);
      std::printf("%s{\"K\":%d,\"chi\":%lld,\"in_range\":%s}", K > 1 ? "," : "", K, (long long)v,
                  (K <= kcat - 2 && (u32)K <= ns) ? "true" : "false");
    }
    std::printf("]");
  }
  if (only_euler) {
    std::printf("}\n");
    return 0;
  }
  TowerParams tp;
  tp.kmax = kmax;
  tp.points = points;
  tp.entry = PointEntry::core;
  const auto t3 = clk::now();
  auto tw = build_tower(cloud, tree, cat, tp, pool);
  const auto t4 = clk::now();
  if (!tw.ok()) {
    std::printf(",\"tower\":\"refus\",\"reason\":\"%s\",\"order\":%d}\n", std::string(reason_name(tw.outcome().reason)).c_str(),
                tw.outcome().order);
    return tw.outcome().status() == Status::invariant_violated ? 3 : 2;
  }
  const Tower& tower = tw.value();
  std::printf(",\"tower_s\":%.3f,\"meb_fallbacks\":%llu,\"orders\":[", std::chrono::duration<double>(t4 - t3).count(),
              (unsigned long long)tower.stats.meb_fallbacks);
  for (const OrderForest& o : tower.orders) {
    const u32 nn = (u32)o.rank.size();
    u64 births = 0, merges = 0, ext_births = 0, big_births = 0, links = 0, ar3 = 0, armax = 0;
    std::map<u32, u64> arity;
    // plateaux : rangs portant plusieurs fusions
    std::map<u32, u32> merges_per_rank;
    for (u32 v = 0; v < nn; ++v) {
      if (o.birth[v] != kNone) {
        ++births;
        if (o.k > 1) {
          const u32 b = o.birth[v];
          if (cat.flags[b] & kExtendedShell) ++ext_births;
          if (cat.pop_off[b + 1] - cat.pop_off[b] > (u64)o.k) ++big_births;
        }
      } else {
        ++merges;
        const u32 a = o.child_off[v + 1] - o.child_off[v];
        ++arity[a];
        links += a - 1;
        if (a >= 3) ++ar3;
        armax = std::max<u64>(armax, a);
        ++merges_per_rank[o.rank[v]];
      }
    }
    u64 plateau_ranks = 0, plateau_merges = 0;
    for (auto& [r, c] : merges_per_rank)
      if (c >= 2) {
        ++plateau_ranks;
        plateau_merges += c;
      }
    std::printf("%s{\"k\":%d,\"nodes\":%u,\"births\":%llu,\"merges\":%llu,\"joins\":%llu,\"links\":%llu,\"arity_ge3\":%llu,"
                "\"arity_max\":%llu,\"ext_births\":%llu,\"births_pop_gt_k\":%llu,\"ranks_with_2plus_merges\":%llu,"
                "\"merges_on_shared_ranks\":%llu,\"knn_jumps_join\":%llu,\"steps_join\":%llu,\"resolves_join\":%llu,"
                "\"seed_hits_join\":%llu,\"memo_hits_join\":%llu,\"birth_hits_join\":%llu,\"local_calls_join\":%llu}",
                o.k > 1 ? "," : "", o.k, nn, (unsigned long long)births, (unsigned long long)merges,
                (unsigned long long)o.joins, (unsigned long long)links, (unsigned long long)ar3, (unsigned long long)armax,
                (unsigned long long)ext_births, (unsigned long long)big_births, (unsigned long long)plateau_ranks,
                (unsigned long long)plateau_merges, (unsigned long long)o.stats.join.knn_jumps,
                (unsigned long long)o.stats.join.steps, (unsigned long long)o.stats.join.resolves,
                (unsigned long long)o.stats.join.seed_hits, (unsigned long long)o.stats.join.memo_hits,
                (unsigned long long)o.stats.join.birth_hits, (unsigned long long)o.stats.join.local_calls);
  }
  std::printf("]");
  // ---- Coherence points-verticales et attaches vivantes (tous les sites, tous les ordres), a l'echelle :
  //   (A) l'attache core v de x a l'ordre K est vivante a la coupe fermee D_K(x) : niveau(v) <= D_K(x) < niveau(parent) ;
  //   (B) pour K >= 2 : l'image verticale de v, remontee a la coupe fermee D_K(x) dans l'ordre K-1, est le noeud de
  //       x a l'ordre K-1 remonte a la meme coupe (L_K(a) est inclus dans L_{K-1}(a), x y est entre).
  //   (C) toute image verticale lower[v] est vivante a la coupe fermee du niveau de v.
  if (coherence && points) {
    auto count_le = [&](u64 e) -> u32 {  // nombre de niveaux du catalogue <= e (comparaison exacte)
      geom::Level L;
      L.num = arith::I192::from_u128(e);
      L.den = arith::I128w::from_u128(1);
      u32 lo = 0, hi = (u32)cat.level.size();
      while (lo < hi) {
        const u32 mid = lo + (hi - lo) / 2;
        if (geom::compare(cat.level[mid], L) <= 0) lo = mid + 1;
        else hi = mid;
      }
      return lo;
    };
    auto climb = [&](const OrderForest& f, u32 v, u32 r) {
      while (f.parent[v] != kNone && f.rank[f.parent[v]] <= r) v = f.parent[v];
      return v;
    };
    u64 chkA = 0, badA = 0, chkB = 0, badB = 0, chkC = 0, badC = 0;
    for (size_t i = 0; i < tower.orders.size(); ++i) {
      const OrderForest& up = tower.orders[i];
      for (u32 s = 0; s < ns; ++s) {
        const u32 v = up.point_node[s];
        const u32 r = count_le(up.point_level[s]);
        ++chkA;
        if (v == kNone || up.rank[v] > r || (up.parent[v] != kNone && up.rank[up.parent[v]] <= r)) {
          ++badA;
          continue;
        }
        if (i == 0) continue;
        const OrderForest& down = tower.orders[i - 1];
        const u32 img = climb(down, up.lower[v], r);
        const u32 w = climb(down, down.point_node[s], r);
        ++chkB;
        if (img != w) ++badB;
      }
      if (i > 0) {
        const OrderForest& down = tower.orders[i - 1];
        for (u32 v = 0; v < up.rank.size(); ++v) {
          const u32 w = up.lower[v];
          ++chkC;
          if (w == kNone || down.rank[w] > up.rank[v] || (down.parent[w] != kNone && down.rank[down.parent[w]] <= up.rank[v]))
            ++badC;
        }
      }
    }
    // (D) plateaux atomiques : aucune fusion n'a pour parent une fusion de meme rang ; toute fusion a >= 2 enfants ;
    //     aucune naissance n'a pour parent une fusion de meme rang (une naissance est isolee a la coupe fermee).
    u64 chkD = 0, badD = 0;
    for (const OrderForest& f : tower.orders)
      for (u32 v = 0; v < f.rank.size(); ++v) {
        ++chkD;
        const bool merge = f.birth[v] == kNone;
        if (merge && f.child_off[v + 1] - f.child_off[v] < 2) ++badD;
        else if (f.parent[v] != kNone && f.rank[f.parent[v]] <= f.rank[v]) ++badD;
      }
    std::printf(",\"coherence\":{\"attach_alive_checks\":%llu,\"attach_alive_bad\":%llu,\"point_vertical_checks\":%llu,"
                "\"point_vertical_bad\":%llu,\"lower_alive_checks\":%llu,\"lower_alive_bad\":%llu,"
                "\"plateau_checks\":%llu,\"plateau_bad\":%llu}",
                (unsigned long long)chkA, (unsigned long long)badA, (unsigned long long)chkB, (unsigned long long)badB,
                (unsigned long long)chkC, (unsigned long long)badC, (unsigned long long)chkD, (unsigned long long)badD);
  }
  std::printf("}\n");
  if (!dump.empty()) {
    FILE* out = std::fopen(dump.c_str(), "w");
    if (!out) return 2;
    std::fprintf(out, "sites %u\n", ns);
    for (u32 s = 0; s < ns; ++s) std::fprintf(out, "site %u %u %u %u\n", s, cloud.x[s], cloud.y[s], cloud.z[s]);
    for (const OrderForest& ord : tower.orders) {
      std::fprintf(out, "order %d %zu\n", ord.k, ord.rank.size());
      for (u32 v = 0; v < ord.rank.size(); ++v) {
        std::string num = "0", den = "1";
        if (ord.rank[v] > 0) {
          num = arith::to_string(cat.level[ord.rank[v] - 1].num);
          den = arith::to_string(cat.level[ord.rank[v] - 1].den);
        }
        const long long low = ord.lower.empty() ? -1LL : (long long)ord.lower[v];
        std::fprintf(out, "node %u %lld %s %s %lld", v, ord.parent[v] == kNone ? -1LL : (long long)ord.parent[v], num.c_str(),
                     den.c_str(), low);
        if (ord.birth[v] != kNone) {
          std::fprintf(out, " B");
          if (ord.k == 1) std::fprintf(out, " %u", ord.birth[v]);
          else {
            const u32 b = ord.birth[v];
            for (u32 s : cat.interior(b)) std::fprintf(out, " %u", s);
            for (u32 s : cat.shell(b)) std::fprintf(out, " %u", s);
          }
        } else {
          std::fprintf(out, " M");
        }
        std::fprintf(out, "\n");
      }
      if (points)
        for (u32 s = 0; s < ns; ++s)
          std::fprintf(out, "point %u %u %llu\n", s, ord.point_node[s], (unsigned long long)ord.point_level[s]);
    }
    std::fclose(out);
  }
  return 0;
}
