// Auditeur C (v9), contre-expertise de D5 : cout reel des etages (a)+(b) avec
// structures plates, et multiplicite des facettes non hachees (sans dedoublonnage).
// Usage : hash_probe <catalogue.bin> K n_sites
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "src/tower/forest/ball_data.hpp"

using namespace mhgp9;
using namespace mhgp9::tower;

static double cpu_ms() {
  timespec ts{};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
  return ts.tv_sec * 1e3 + ts.tv_nsec * 1e-6;
}
static std::uint64_t splitmix(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ull;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
  return x ^ (x >> 31);
}
using KSet = std::array<i32, 10>;
struct KSetHash {
  std::size_t operator()(const KSet& k) const {
    std::uint64_t h = 0x9e3779b97f4a7c15ull;
    for (i32 s : k) { h ^= (std::uint32_t)s; h *= 0x100000001b3ull; h ^= h >> 29; }
    return (std::size_t)h;
  }
};
struct Entry { std::uint64_t fp; std::uint32_t ball; std::uint32_t z; };  // z = slot omis, 0xff = graine

int main(int argc, char** argv) {
  if (argc < 4) return 2;
  const unsigned kmax = (unsigned)std::stoul(argv[2]);
  const std::size_t nsites = std::stoul(argv[3]);
  std::vector<BallData> balls;
  {
    std::FILE* f = std::fopen(argv[1], "rb");
    std::uint64_t count = 0;
    if (!f || std::fread(&count, 8, 1, f) != 1) return 3;
    balls.resize(count);
    if (std::fread(balls.data(), sizeof(BallData), count, f) != count) return 3;
    std::fclose(f);
    std::vector<BallData> kept;
    for (const auto& b : balls) if ((unsigned)b.n_interior + b.arity <= kmax + 1) kept.push_back(b);
    balls.swap(kept);
  }
  // Ordre de niveau (approche flottante : sert seulement a reproduire le motif d'acces).
  std::vector<std::uint32_t> order(balls.size());
  for (std::size_t j = 0; j < order.size(); ++j) order[j] = (std::uint32_t)j;
  {
    std::vector<double> a(balls.size());
    for (std::size_t j = 0; j < a.size(); ++j) a[j] = level_approximation(balls[j].level);
    std::stable_sort(order.begin(), order.end(), [&](auto x, auto y) { return a[x] < a[y]; });
  }
  std::vector<std::vector<std::uint32_t>> programs(kmax + 1);
  std::uint64_t extra_balls = 0;
  for (std::uint32_t b : order) {
    const auto& x = balls[b];
    if (x.n_shell != x.arity) ++extra_balls;
    const unsigned lo = x.n_interior + x.arity - 1;
    const unsigned hi = std::min<unsigned>(kmax, x.n_interior + x.n_shell);
    for (unsigned k = lo; k <= hi; ++k) programs[k].push_back(b);
  }
  std::vector<std::uint64_t> h(nsites);
  for (std::size_t u = 0; u < nsites; ++u) h[u] = splitmix(u * 0x2545f4914f6cdd1dull + 7);

  std::printf("{\"balls\":%zu,\"kmax\":%u,\"extra_balls\":%llu,\"sizeof_ball\":%zu,\"orders\":[", balls.size(), kmax,
              (unsigned long long)extra_balls, sizeof(BallData));
  double tot_build = 0, tot_look = 0;
  std::uint64_t tot_entries = 0, tot_facets = 0, tot_nonhash_occ = 0, tot_nonhash_distinct = 0, tot_distinct = 0;
  for (unsigned k = 2; k <= kmax; ++k) {
    const auto& program = programs[k];
    // ---------- E2 : index plat + jointure, chronometres ----------
    std::vector<std::uint32_t> posk(balls.size(), 0xffffffffu);
    for (std::size_t j = 0; j < program.size(); ++j) posk[program[j]] = (std::uint32_t)j;
    const double c0 = cpu_ms();
    std::uint64_t n_entries = 0;
    for (std::uint32_t id : program) {
      const auto& b = balls[id];
      const unsigned pu = (unsigned)b.n_interior + b.n_shell;
      if (k == pu) ++n_entries;
      else if (b.n_shell == b.arity && k + 1 == pu) n_entries += b.n_interior;
    }
    std::size_t cap = 16;
    while (cap < 2 * n_entries) cap <<= 1;
    std::vector<Entry> table(cap, Entry{0, 0xffffffffu, 0});
    auto insert = [&](std::uint64_t fp, std::uint32_t ball, std::uint32_t z) {
      std::size_t s = (std::size_t)(fp * 0x9e3779b97f4a7c15ull >> 20) & (cap - 1);
      while (table[s].ball != 0xffffffffu) s = (s + 1) & (cap - 1);
      table[s] = Entry{fp, ball, z};
    };
    for (std::uint32_t id : program) {
      const auto& b = balls[id];
      const unsigned pu = (unsigned)b.n_interior + b.n_shell;
      std::uint64_t fp = 0;
      for (i32 x : b.interior()) fp ^= h[x];
      for (i32 x : b.shell()) fp ^= h[x];
      if (k == pu) insert(fp, id, 0xff);
      else if (b.n_shell == b.arity && k + 1 == pu)
        for (std::uint32_t z = 0; z < b.n_interior; ++z) insert(fp ^ h[b.interior_ids[z]], id, z);
    }
    const double c1 = cpu_ms();
    // Jointure : chaque facette reguliere I_B u S_B - s, verification exacte des listes.
    std::uint64_t facets = 0, found = 0, missed = 0, verify_fail = 0, probes = 0, skipped_extra = 0;
    std::vector<std::uint32_t> tpos;
    tpos.reserve(program.size() * 4);
    auto cand_set = [&](const Entry& e, KSet& out) {
      const auto& c = balls[e.ball];
      std::size_t n = 0;
      for (std::uint32_t j = 0; j < c.n_interior; ++j) if (j != e.z) out[n++] = c.interior_ids[j];
      for (i32 x : c.shell()) out[n++] = x;
      std::sort(out.begin(), out.begin() + n);
      return n;
    };
    for (std::uint32_t id : program) {
      const auto& b = balls[id];
      const unsigned pu = (unsigned)b.n_interior + b.n_shell;
      if (k == pu) continue;
      if (b.n_shell != b.arity) { ++skipped_extra; continue; }
      std::uint64_t fpb = 0;
      for (i32 x : b.interior()) fpb ^= h[x];
      for (i32 x : b.shell()) fpb ^= h[x];
      for (std::size_t omit = 0; omit < b.n_shell; ++omit) {
        ++facets;
        const std::uint64_t fp = fpb ^ h[b.shell_ids[omit]];
        std::size_t s = (std::size_t)(fp * 0x9e3779b97f4a7c15ull >> 20) & (cap - 1);
        std::uint32_t target = 0xffffffffu;
        KSet f{};
        bool fbuilt = false;
        while (table[s].ball != 0xffffffffu) {
          ++probes;
          if (table[s].fp == fp) {
            if (!fbuilt) {
              std::size_t n = 0;
              for (i32 x : b.interior()) f[n++] = x;
              for (std::size_t j = 0; j < b.n_shell; ++j) if (j != omit) f[n++] = b.shell_ids[j];
              std::sort(f.begin(), f.begin() + n);
              fbuilt = true;
            }
            KSet c{};
            const std::size_t n = cand_set(table[s], c);
            if (n == k && std::equal(f.begin(), f.begin() + k, c.begin())) { target = table[s].ball; break; }
            ++verify_fail;
          }
          s = (s + 1) & (cap - 1);
        }
        if (target != 0xffffffffu) { ++found; tpos.push_back(posk[target]); }
        else { ++missed; tpos.push_back(0xffffffffu); }
      }
    }
    const double c2 = cpu_ms();
    volatile std::uint32_t sink = tpos.empty() ? 0 : tpos.back();
    (void)sink;
    // ---------- E1 : multiplicite (non chronometre) ----------
    std::unordered_set<KSet, KSetHash> seeds, hits;
    for (std::uint32_t id : program) {
      const auto& b = balls[id];
      const unsigned pu = (unsigned)b.n_interior + b.n_shell;
      if (k == pu) {
        KSet s{}; std::size_t n = 0;
        for (i32 x : b.interior()) s[n++] = x;
        for (i32 x : b.shell()) s[n++] = x;
        std::sort(s.begin(), s.begin() + n); seeds.insert(s);
      } else if (b.n_shell == b.arity && k + 1 == pu) {
        for (std::size_t z = 0; z < b.n_interior; ++z) {
          KSet s{}; std::size_t n = 0;
          for (std::size_t j = 0; j < b.n_interior; ++j) if (j != z) s[n++] = b.interior_ids[j];
          for (i32 x : b.shell()) s[n++] = x;
          std::sort(s.begin(), s.begin() + n); hits.insert(s);
        }
      }
    }
    std::unordered_map<KSet, std::uint32_t, KSetHash> occ;
    std::uint64_t nonhash_occ = 0;
    for (std::uint32_t id : program) {
      const auto& b = balls[id];
      const unsigned pu = (unsigned)b.n_interior + b.n_shell;
      if (k == pu || b.n_shell != b.arity) continue;
      for (std::size_t omit = 0; omit < b.n_shell; ++omit) {
        KSet s{}; std::size_t n = 0;
        for (i32 x : b.interior()) s[n++] = x;
        for (std::size_t j = 0; j < b.n_shell; ++j) if (j != omit) s[n++] = b.shell_ids[j];
        std::sort(s.begin(), s.begin() + n);
        ++occ[s];
        if (!seeds.count(s) && !hits.count(s)) ++nonhash_occ;
      }
    }
    std::uint64_t nonhash_distinct = 0, maxmult = 0;
    std::vector<KSet> distinct_keys;
    distinct_keys.reserve(occ.size());
    for (const auto& [s, c] : occ) distinct_keys.push_back(s);
    const double c3 = cpu_ms();
    std::uint64_t dh = 0;
    for (const auto& s : distinct_keys) { if (seeds.count(s)) ++dh; else if (hits.count(s)) ++dh; }
    const double harness_lookup_ms = cpu_ms() - c3;
    for (const auto& [s, c] : occ) {
      if (!seeds.count(s) && !hits.count(s)) { ++nonhash_distinct; maxmult = std::max<std::uint64_t>(maxmult, c); }
    }
    tot_build += c1 - c0; tot_look += c2 - c1; tot_entries += n_entries; tot_facets += facets;
    tot_nonhash_occ += nonhash_occ; tot_nonhash_distinct += nonhash_distinct; tot_distinct += occ.size();
    std::printf("%s{\"K\":%u,\"blocks\":%zu,\"entries\":%llu,\"table_bytes\":%zu,\"build_ms\":%.2f,\"ns_per_entry\":%.1f,"
                "\"facets\":%llu,\"found\":%llu,\"missed\":%llu,\"verify_fail\":%llu,\"probes\":%llu,\"skipped_extra\":%llu,"
                "\"lookup_ms\":%.2f,\"ns_per_facet\":%.1f,\"distinct\":%zu,\"nonhash_occ\":%llu,\"nonhash_distinct\":%llu,"
                "\"nonhash_multiplicity\":%.3f,\"nonhash_max_mult\":%llu,\"harness_style_distinct_lookup_ms\":%.2f,\"distinct_hashed\":%llu}",
                k > 2 ? "," : "", k, program.size(), (unsigned long long)n_entries, cap * sizeof(Entry), c1 - c0,
                1e6 * (c1 - c0) / std::max<std::uint64_t>(1, n_entries), (unsigned long long)facets,
                (unsigned long long)found, (unsigned long long)missed, (unsigned long long)verify_fail,
                (unsigned long long)probes, (unsigned long long)skipped_extra, c2 - c1,
                1e6 * (c2 - c1) / std::max<std::uint64_t>(1, facets), occ.size(), (unsigned long long)nonhash_occ,
                (unsigned long long)nonhash_distinct, nonhash_distinct ? (double)nonhash_occ / nonhash_distinct : 0.0,
                (unsigned long long)maxmult, harness_lookup_ms, (unsigned long long)dh);
  }
  std::printf("],\"total\":{\"entries\":%llu,\"build_ms\":%.1f,\"facets\":%llu,\"lookup_ms\":%.1f,\"distinct\":%llu,"
              "\"nonhash_occ\":%llu,\"nonhash_distinct\":%llu,\"nonhash_multiplicity\":%.3f}}\n",
              (unsigned long long)tot_entries, tot_build, (unsigned long long)tot_facets, tot_look,
              (unsigned long long)tot_distinct, (unsigned long long)tot_nonhash_occ,
              (unsigned long long)tot_nonhash_distinct,
              tot_nonhash_distinct ? (double)tot_nonhash_occ / tot_nonhash_distinct : 0.0);
  return 0;
}
