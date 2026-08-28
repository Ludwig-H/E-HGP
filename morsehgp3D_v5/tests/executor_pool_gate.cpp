// MorseHGP3D v5 — PORTE DU POOL D'EXECUTEURS (src/gpu/executor_pool.hpp, G0) :
// executeur factice (compteur de constructions, travail = somme deterministe),
// P producteurs x J travaux chacun ; exige : (1) resultats de chaque producteur
// dans SON ordre de soumission (l'attente par travail rend l'emission
// producteur-locale) ; (2) exactement N executeurs construits (persistants) ;
// (3) pic d'activite <= N et >= 2 quand N >= 2 (concurrence reelle) ;
// (4) exception d'un travail relancee dans le producteur soumettant, les
// autres producteurs poursuivent ; (5) planchers : >= 1000 travaux.
// Codes : 0 ; 1 desaccord ; 3 plancher ; 4 mutant tue (`pool-serial` : pic < 2
// a N = 4 ; `pool-drop-exception` : exception avalee).
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "../src/gpu/executor_pool.hpp"

using namespace mhgp5;

namespace {
std::atomic<int> g_built{0};
struct FakeExecutor {
  FakeExecutor() { ++g_built; }
  u64 scan(u64 x) {
    u64 h = x * 1000003ull + 12345ull;
    for (int i = 0; i < 2000; ++i) h ^= (h << 13) ^ (h >> 7) ^ (u64)i;  // travail deterministe
    return h;
  }
};
int failures = 0;
void expect(bool ok, const char* what) { if (!ok) { std::printf("ECHEC : %s\n", what); ++failures; } else std::printf("ok : %s\n", what); }
}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) { const std::string a = argv[i]; if (a.rfind("--inject=", 0) == 0) inject = a.substr(9); else return 2; }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool mutant = MHGP5_MUTANT("pool-serial") || MHGP5_MUTANT("pool-drop-exception");
  const int P = 12, J = 100;
  u64 total = 0;
  bool order_ok = true, exc_ok = true;
  u32 peak = 0;
  for (const int N : {1, 4}) {
    g_built = 0;
    std::atomic<u64> done{0};
    std::vector<std::vector<u64>> res((size_t)P);
    {
      gpu::ExecutorPool<FakeExecutor> pool(N);
      std::vector<std::thread> prod;
      for (int p = 0; p < P; ++p)
        prod.emplace_back([&, p] {
          std::vector<u64>& r = res[(size_t)p];
          for (int j = 0; j < J; ++j) {
            const u64 x = (u64)p * 100000ull + (u64)j;
            u64 y = 0;
            bool got_exc = false;
            try {
              pool.submit_and_wait([&](FakeExecutor& ex) {
                if (j == 37 && p == 5) throw std::runtime_error("travail 37 du producteur 5");
                y = ex.scan(x);
              });
            } catch (const std::runtime_error& e) {
              got_exc = std::string(e.what()) == "travail 37 du producteur 5";
            }
            if (j == 37 && p == 5) { if (!got_exc) exc_ok = false; continue; }
            r.push_back(y);
            ++done;
          }
        });
      for (std::thread& t : prod) t.join();
      peak = std::max(peak, pool.peak_active());
      char what[96];
      std::snprintf(what, sizeof(what), "N=%d : pic d'activite %u <= N", N, pool.peak_active());
      expect(pool.peak_active() <= (u32)N, what);
      if (N >= 2) { std::snprintf(what, sizeof(what), "N=%d : pic d'activite %u >= 2 (concurrence reelle)", N, pool.peak_active()); expect(pool.peak_active() >= 2, what); }
      std::snprintf(what, sizeof(what), "N=%d : travaux acheves %llu == soumis %d", N, (unsigned long long)pool.jobs_done(), P * J);
      expect(pool.jobs_done() == (u64)(P * J), what);
    }
    char what[96];
    std::snprintf(what, sizeof(what), "N=%d : exactement N executeurs construits (%d)", N, g_built.load());
    expect(g_built == N, what);
    for (int p = 0; p < P; ++p) {
      FakeExecutor ref;
      size_t k = 0;
      for (int j = 0; j < J; ++j) {
        if (j == 37 && p == 5) continue;
        if (k >= res[(size_t)p].size() || res[(size_t)p][k] != ref.scan((u64)p * 100000ull + (u64)j)) order_ok = false;
        ++k;
      }
    }
    total += done;
  }
  expect(order_ok, "resultats de chaque producteur dans son ordre de soumission, tous presents");
  expect(exc_ok, "exception du travail relancee dans le producteur soumettant, les autres poursuivent");
  std::printf("executor_pool_gate travaux=%llu pic=%u\n", (unsigned long long)total, peak);
  if (total < 1000) { std::printf("PLANCHER\n"); return 3; }
  if (mutant) { if (failures) return 4; std::printf("MUTANT NON TUE\n"); return 1; }
  if (failures) return 1;
  std::printf("executor_pool_gate OK\n");
  return 0;
}
