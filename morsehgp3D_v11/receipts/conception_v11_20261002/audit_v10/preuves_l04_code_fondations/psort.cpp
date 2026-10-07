// L04 audit : sched::parallel_sort contre std::sort (ordres totaux), et compte deterministe des comparaisons.
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <random>
#include <vector>

#include "sched/sort.hpp"

using namespace mhgp10;

struct Rec {
  double key;
  u32 a, b;
  bool operator==(const Rec&) const = default;
};

int main() {
  int failures = 0;
  u64 checks = 0;
  std::mt19937_64 g(99);
  for (unsigned P : {2u, 3u, 4u, 7u, 8u}) {
    sched::Pool pool(P);
    for (u64 n : {u64(8191), u64(8192), u64(8193), u64(20000), u64(100003), u64(400000)}) {
      for (int pattern = 0; pattern < 6; ++pattern) {
        std::vector<Rec> v(n);
        for (u64 i = 0; i < n; ++i) {
          double k = 0;
          switch (pattern) {
            case 0: k = double(g() % 1000000007ull); break;           // aleatoire
            case 1: k = double(i); break;                             // deja trie
            case 2: k = double(n - i); break;                         // inverse
            case 3: k = double(g() % 7); break;                       // sept cles (ordre total par (a, b))
            case 4: k = 1.0; break;                                   // une seule cle : un seul seau
            default: k = double((i * 2654435761ull) % 97); break;     // peu de cles, periodique
          }
          v[i] = Rec{k, u32(i), u32(g())};
        }
        std::shuffle(v.begin(), v.end(), g);
        if (pattern == 1) std::sort(v.begin(), v.end(), [](const Rec& x, const Rec& y) { return x.a < y.a; });
        std::vector<Rec> ref = v;
        std::atomic<u64> cmp_par{0}, cmp_seq{0};
        auto less_seq = [&](const Rec& x, const Rec& y) {
          cmp_seq.fetch_add(1, std::memory_order_relaxed);
          return x.key != y.key ? x.key < y.key : x.a < y.a;
        };
        auto less_par = [&](const Rec& x, const Rec& y) {
          cmp_par.fetch_add(1, std::memory_order_relaxed);
          return x.key != y.key ? x.key < y.key : x.a < y.a;
        };
        std::sort(ref.begin(), ref.end(), less_seq);
        sched::parallel_sort(pool, v, less_par);
        ++checks;
        if (!(v == ref)) {
          ++failures;
          std::printf("ECHEC P=%u n=%llu motif=%d\n", P, (unsigned long long)n, pattern);
        }
        if (n == 400000 && (P == 8 || P == 2))
          std::printf("P=%u n=%llu motif=%d : comparaisons parallele/sequentiel = %.2f\n", P, (unsigned long long)n, pattern,
                      double(cmp_par.load()) / double(cmp_seq.load()));
      }
    }
    if (pool.nested_calls() != 0) ++failures;
  }
  // ordre NON total (ex aequo) : le resultat depend-il du nombre de fils ?
  {
    std::vector<Rec> base(200000);
    for (u64 i = 0; i < base.size(); ++i) base[i] = Rec{double(g() % 50), u32(i), 0};
    auto weak = [](const Rec& x, const Rec& y) { return x.key < y.key; };
    std::vector<Rec> s1 = base, s2 = base, s8 = base;
    std::sort(s1.begin(), s1.end(), weak);
    sched::Pool p2(2), p8(8);
    sched::parallel_sort(p2, s2, weak);
    sched::parallel_sort(p8, s8, weak);
    std::printf("ordre non total : P=2 identique a std::sort ? %d ; P=8 identique a P=2 ? %d (contrat : non garanti)\n",
                int(s2 == s1), int(s8 == s2));
  }
  std::printf("psort_checks %llu failures %d\n", (unsigned long long)checks, failures);
  return failures ? 1 : 0;
}
