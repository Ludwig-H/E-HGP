// Mesure MES-S du contrat numerique de la v12 (CONTRAT_NUMERIQUE.md, § 2 NUM-REPERE et § 8), sur les vidages MHGP12LF
// des feuilles de la v11. Hors produit ; n'ecrit que des comptes (aucune coordonnee).
//
// Etendue en bits d'un ensemble fini E de points entiers : s = plus petit s tel que max_axe max_{x dans E} (x_a - m_a) <
// 2^s, m le coin minimal de E (s = nombre de bits de la plus grande difference ; 0 pour un point).
//   feuille : E = fermeture [lo, hi] de la boite de centres (ses deux coins) et TOUS les sites de la liste ;
//   boite seule : E = {lo, hi} ;
//   sans le site le plus lointain : E prive du site de plus grande distance L-infini a la fermeture de la boite
//   (0 pour un site dedans ; premier site en cas d'egalite) ;
//   meilleur retrait d'un site : minimum de s sur les retraits d'un seul site ;
//   support d'une boule emise : E = S* (2, 3 ou 4 sites).
// Le critere de la voie etroite de la v11 (etendue <= 2^20 sur chaque axe, kNarrowSpan) est aussi compte.
//
// Usage : mhgp12_mes_s <vidage.bin> [...]   Sortie : une ligne JSON par vidage.
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "mhgp12/leaf/dump_format.hpp"

namespace {

using mhgp12::dump::i64;
using mhgp12::dump::u32;
using mhgp12::dump::u64;

u32 bits_of(i64 span) {  // plus petit s tel que span < 2^s (span >= 0)
  u32 s = 0;
  while (s < 63 && (i64(1) << s) <= span) ++s;
  return s;
}

struct Envelope {
  std::array<i64, 3> low{}, high{};
  bool empty = true;
  void add(i64 x, i64 y, i64 z) {
    const i64 v[3] = {x, y, z};
    for (int a = 0; a < 3; ++a) {
      if (empty || v[a] < low[a]) low[a] = v[a];
      if (empty || v[a] > high[a]) high[a] = v[a];
    }
    empty = false;
  }
  u32 bits() const {
    i64 span = 0;
    for (int a = 0; a < 3; ++a) span = std::max(span, high[a] - low[a]);
    return bits_of(span);
  }
  i64 span() const {
    i64 s = 0;
    for (int a = 0; a < 3; ++a) s = std::max(s, high[a] - low[a]);
    return s;
  }
};

struct Histogram {
  std::array<u64, 34> bins{};
  u64 total = 0;
  void add(u32 s) {
    ++bins[std::min<u32>(s, 33)];
    ++total;
  }
  u64 above(u32 s) const {
    u64 n = 0;
    for (u32 b = s + 1; b < bins.size(); ++b) n += bins[b];
    return n;
  }
  void json(std::ostream& out) const {
    out << "{\"total\":" << total << ",\"bins\":{";
    bool first = true;
    for (u32 b = 0; b < bins.size(); ++b)
      if (bins[b] != 0) {
        out << (first ? "" : ",") << '"' << b << "\":" << bins[b];
        first = false;
      }
    out << "},\"above\":{";
    const u32 marks[] = {16, 17, 19, 20, 24};
    for (u32 i = 0; i < 5; ++i) out << (i ? "," : "") << '"' << marks[i] << "\":" << above(marks[i]);
    out << "}}";
  }
};

int run(const std::string& path) {
  mhgp12::dump::LeafDump d;
  std::string error;
  if (!mhgp12::dump::read(path, d, error)) {
    std::cerr << error << '\n';
    return 2;
  }
  Histogram leaf, box, without_far, best_removal, delta_far, delta_box, delta_best;
  std::array<Histogram, 5> support;  // par qmin
  u64 narrow_v11 = 0, minority_far = 0, minority_best = 0, outside_sites = 0, leaf_sites = 0;
  u64 leaves_with_outside = 0, extended_shells = 0, max_shell = 0;
  for (u64 j = 0; j < d.header.n_leaves; ++j) {
    const auto& job = d.jobs[j];
    const u32* sites = d.leaf_sites(j);
    Envelope env, env_box;
    env.add(job.lo[0], job.lo[1], job.lo[2]);
    env.add(job.hi[0], job.hi[1], job.hi[2]);
    env_box = env;
    i64 far_distance = -1;
    u32 far_index = 0;
    u32 outside_here = 0;
    for (u32 i = 0; i < job.m; ++i) {
      const u32 s = sites[i];
      const i64 v[3] = {d.x[s], d.y[s], d.z[s]};
      env.add(v[0], v[1], v[2]);
      i64 dist = 0;
      for (int a = 0; a < 3; ++a) {
        const i64 out = v[a] < job.lo[a] ? job.lo[a] - v[a] : (v[a] > job.hi[a] ? v[a] - job.hi[a] : 0);
        dist = std::max(dist, out);
      }
      if (dist > 0) ++outside_here;
      if (dist > far_distance) {
        far_distance = dist;
        far_index = i;
      }
    }
    outside_sites += outside_here;
    leaf_sites += job.m;
    leaves_with_outside += outside_here != 0;
    const u32 s_leaf = env.bits(), s_box = env_box.bits();
    if (env.span() <= (i64(1) << 20)) ++narrow_v11;
    // Sans le site le plus lointain, puis meilleur retrait d'un site.
    u32 s_far = s_leaf, s_best = s_leaf;
    for (u32 r = 0; r < job.m; ++r) {
      Envelope e = env_box;
      for (u32 i = 0; i < job.m; ++i)
        if (i != r) e.add(d.x[sites[i]], d.y[sites[i]], d.z[sites[i]]);
      const u32 s = e.bits();
      if (r == far_index) s_far = s;
      s_best = std::min(s_best, s);
    }
    leaf.add(s_leaf);
    box.add(s_box);
    without_far.add(s_far);
    best_removal.add(s_best);
    delta_far.add(s_leaf - s_far);
    delta_box.add(s_leaf - s_box);
    delta_best.add(s_leaf - s_best);
    minority_far += s_leaf - s_far >= 2;
    minority_best += s_leaf - s_best >= 2;
    for (u64 b = d.record_begin[j]; b < d.record_begin[j + 1]; ++b) {
      const auto& r = d.records[b];
      Envelope e;
      for (u32 k = 0; k < r.qmin; ++k) {
        const u32 s = sites[r.support[k]];
        e.add(d.x[s], d.y[s], d.z[s]);
      }
      support[std::min<u32>(r.qmin, 4)].add(e.bits());
      if (r.m > r.qmin) ++extended_shells;  // coquille etendue : la canonisation a ete jouee
      max_shell = std::max<u64>(max_shell, r.m);
    }
  }
  // Etendue globale du nuage.
  Envelope cloud;
  for (u64 s = 0; s < d.header.n_sites; ++s) cloud.add(d.x[s], d.y[s], d.z[s]);
  std::cout << "{\"dump\":\"" << path << "\",\"kmax\":" << d.header.kmax << ",\"leaf_size\":" << d.header.leaf_size
            << ",\"leaves\":" << d.header.n_leaves << ",\"sites\":" << d.header.n_sites
            << ",\"cloud_bits\":" << cloud.bits() << ",\"cloud_span\":[" << cloud.high[0] - cloud.low[0] << ','
            << cloud.high[1] - cloud.low[1] << ',' << cloud.high[2] - cloud.low[2] << "]"
            << ",\"v11_narrow_leaves\":" << narrow_v11 << ",\"leaves_with_outside_sites\":" << leaves_with_outside
            << ",\"outside_sites\":" << outside_sites << ",\"leaf_sites\":" << leaf_sites
            << ",\"minority_far_ge2\":" << minority_far << ",\"minority_best_ge2\":" << minority_best
            << ",\"extended_shells\":" << extended_shells << ",\"max_shell\":" << max_shell;
  std::cout << ",\"leaf\":";
  leaf.json(std::cout);
  std::cout << ",\"box_only\":";
  box.json(std::cout);
  std::cout << ",\"without_farthest\":";
  without_far.json(std::cout);
  std::cout << ",\"best_single_removal\":";
  best_removal.json(std::cout);
  std::cout << ",\"delta_farthest\":";
  delta_far.json(std::cout);
  std::cout << ",\"delta_box\":";
  delta_box.json(std::cout);
  std::cout << ",\"delta_best\":";
  delta_best.json(std::cout);
  std::cout << ",\"support\":{";
  for (u32 q = 2; q <= 4; ++q) {
    std::cout << (q > 2 ? "," : "") << "\"q" << q << "\":";
    support[q].json(std::cout);
  }
  std::cout << "}}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  int code = 0;
  for (int i = 1; i < argc; ++i) code = std::max(code, run(argv[i]));
  return code;
}
