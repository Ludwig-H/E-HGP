// Micro-banc hors depot : passe 1 de l'etage des rangs de la base (a10605a06, generator.cpp:825-834), telle quelle
// (accumulateurs inc[c+1], pops[c+1], bad[c] dans des std::vector<u64> partages, relus/ecrits a chaque boule), contre
// la meme passe a accumulateurs locaux (forme A1). Memes donnees : enregistrements de 104 octets en lecture aleatoire.
//   faux_partage BALLS THREADS REPS
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "catalogue/catalogue.hpp"
#include "sched/pool.hpp"

using namespace mhgp10;
using clk = std::chrono::steady_clock;

struct Rec {  // 104 octets comme l'enregistrement du generateur (niveau 56 + support 16 + champs)
  unsigned char level[56];
  std::array<u32, 4> sup;
  u32 q, p, u, flags, n_i, pop_begin, pop_len;
  u32 pad;
};
static_assert(sizeof(Rec) == 104);
struct Ref {
  double approx;
  std::array<u32, 4> sup;
  u32 local, rec;
};

int main(int argc, char** argv) {
  const u64 nballs = argc > 1 ? std::stoull(argv[1]) : 1407885;
  const unsigned threads = argc > 2 ? unsigned(std::stoul(argv[2])) : 4;
  const int reps = argc > 3 ? std::stoi(argv[3]) : 15;
  sched::Pool pool(threads);
  const unsigned L = pool.size();
  std::vector<std::vector<Rec>> locals(L);
  std::mt19937_64 g(12345);
  for (u64 i = 0; i < nballs; ++i) {
    Rec r{};
    r.pop_len = 3 + g() % 5;
    locals[i % L].push_back(r);
  }
  UninitVector<Ref> refs(nballs);
  std::vector<u64> perm(nballs);
  std::iota(perm.begin(), perm.end(), 0);
  std::shuffle(perm.begin(), perm.end(), g);
  for (u64 i = 0; i < nballs; ++i) refs[i] = Ref{0, {}, u32(perm[i] % L), u32(perm[i] / L)};
  UninitVector<signed char> cmp(nballs);
  for (u64 i = 0; i < nballs; ++i) cmp[i] = (g() % 5 == 0) ? 0 : -1;
  auto rec = [&](const Ref& x) -> const Rec& { return locals[x.local][x.rec]; };
  const u32 nb = u32(nballs);
  const u64 chunks = std::min<u64>(u64(pool.size()) * 8, std::max<u64>(1, nb / 4096));
  auto chunk_lo = [&](u64 c) { return u64(nb) * c / chunks; };
  std::vector<double> tb, tl;
  u64 check_b = 0, check_l = 0;
  for (int rep = 0; rep < reps; ++rep) {
    for (int variant = 0; variant < 2; ++variant) {
      std::vector<u64> bad(chunks, u64(nb)), inc(chunks + 1, 0), pops(chunks + 1, 0);
      const auto t0 = clk::now();
      if (variant == 0) {  // base, telle quelle
        pool.parallel_for(chunks, 1, [&](u64 c0, u64 c1, unsigned) {
          for (u64 c = c0; c < c1; ++c)
            for (u64 b = chunk_lo(c); b < chunk_lo(c + 1); ++b) {
              if (b > 0 && bad[c] == u64(nb) && (cmp[b] > 0 || (cmp[b] == 0 && refs[b - 1].sup == refs[b].sup))) bad[c] = b;
              if (b > 0 && cmp[b] < 0) ++inc[c + 1];
              pops[c + 1] += rec(refs[b]).pop_len;
            }
        });
      } else {  // accumulateurs locaux
        pool.parallel_for(chunks, 1, [&](u64 c0, u64 c1, unsigned) {
          for (u64 c = c0; c < c1; ++c) {
            u64 ninc = 0, npop = 0, first_bad = u64(nb);
            const u64 lo = chunk_lo(c), hi = chunk_lo(c + 1);
            for (u64 b = lo; b < hi; ++b) {
              if (b > 0 && first_bad == u64(nb) && (cmp[b] > 0 || (cmp[b] == 0 && refs[b - 1].sup == refs[b].sup)))
                first_bad = b;
              ninc += b > 0 && cmp[b] < 0;
              npop += rec(refs[b]).pop_len;
            }
            inc[c + 1] = ninc;
            pops[c + 1] = npop;
            bad[c] = first_bad;
          }
        });
      }
      const double s = std::chrono::duration<double>(clk::now() - t0).count();
      u64 sum = 0;
      for (u64 v : pops) sum += v;
      for (u64 v : inc) sum += v;
      (variant == 0 ? tb : tl).push_back(s);
      (variant == 0 ? check_b : check_l) = sum;
    }
  }
  std::sort(tb.begin(), tb.end());
  std::sort(tl.begin(), tl.end());
  std::printf("{\"boules\":%llu,\"fils\":%u,\"tranches\":%llu,\"reps\":%d,\"base_med_ms\":%.3f,\"local_med_ms\":%.3f,"
              "\"base_min_ms\":%.3f,\"local_min_ms\":%.3f,\"rapport_med\":%.3f,\"sommes_egales\":%s}\n",
              (unsigned long long)nballs, pool.size(), (unsigned long long)chunks, reps, 1e3 * tb[tb.size() / 2],
              1e3 * tl[tl.size() / 2], 1e3 * tb[0], 1e3 * tl[0], tb[tb.size() / 2] / tl[tl.size() / 2],
              check_b == check_l ? "true" : "false");
  return 0;
}
