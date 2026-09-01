// MorseHGP3D v6 — PORTE DU POOL D'EXECUTEURS (src/gpu/executor_pool.hpp, G0).
// Executeur factice (compteur de constructions, travail = somme
// deterministe). Ce qui est exige :
//   (1) resultats de chaque producteur dans SON ordre de soumission
//       (l'attente par travail rend l'emission producteur-locale) ;
//   (2) exactement N executeurs construits (persistants) ;
//   (3) PIC D'ACTIVITE DETERMINISTE : N travaux sont retenus dans une LATCH
//       jusqu'a ce que les N soient actifs, donc le pic vaut EXACTEMENT N —
//       il ne depend plus de l'ordonnanceur. Teste a N = 1, 2, 4, 8. Si les N
//       n'arrivent pas avant l'echeance, la latch est liberee et le pic
//       observe est < N : c'est la signature d'un pool serialise ;
//   (4) exception d'un travail relancee dans le producteur soumettant, les
//       autres producteurs poursuivent ;
//   (5) REENTRANCE refusee : un travail qui soumet au meme pool obtient une
//       exception immediate, jamais un blocage ;
//   (6) DOMAINE : 0 et 9 sont REFUSES (jamais clampes), 3 est accepte ;
//   (7) CONTRE-PRESSION : `queue_cap = 1` borne le pic de file a 1 ;
//   (8) plancher : >= 1000 travaux.
//   (9) CONFINEMENT DE PANNE (la dent v5 reparee) : une DeviceFatalError
//       via submit_and_wait_contained FERME le pool (usage unique), remonte
//       au producteur, les soumissions suivantes recoivent la MEME erreur
//       fatale, les tickets en file sont ANNULES et la comptabilite reste
//       equilibree (soumis = reussis + echoues + annules).
// Codes : 0 ; 1 desaccord ; 2 refus d'argument ; 3 plancher ; 4 mutant tue
// (`pool-serial` : pic < N a N >= 2 ; `pool-drop-exception` : exception
// avalee ; `pool-close-fatal-missing` : pool encore OUVERT apres une panne
// fatale confinee).
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "../src/gpu/executor_pool.hpp"

using namespace mhgp6;

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
u64 g_jobs = 0;
void expect(bool ok, const char* what) {
  if (!ok) {
    std::printf("ECHEC : %s\n", what);
    ++failures;
  } else {
    std::printf("ok : %s\n", what);
  }
}

// (3) PIC DETERMINISTE : N travaux bloques dans une latch, liberes seulement
// quand les N sont actifs (ou a l'echeance). Retourne le pic observe.
u32 latch_peak(int N, u64* jobs) {
  gpu::ExecutorPool<FakeExecutor> pool(N, (size_t)N * 2);
  std::mutex m;
  std::condition_variable cv;
  int arrived = 0;
  bool go = false;
  std::vector<std::thread> prod;
  for (int i = 0; i < N; ++i)
    prod.emplace_back([&] {
      pool.submit_and_wait([&](FakeExecutor&) {
        std::unique_lock<std::mutex> lk(m);
        ++arrived;
        cv.notify_all();
        cv.wait_for(lk, std::chrono::seconds(5), [&] { return go; });
      });
    });
  {
    std::unique_lock<std::mutex> lk(m);
    cv.wait_for(lk, std::chrono::seconds(5), [&] { return arrived >= N; });
    go = true;
  }
  cv.notify_all();
  for (std::thread& t : prod) t.join();
  *jobs += (u64)N;
  return pool.peak_active();
}
}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--inject=", 0) == 0) inject = a.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool m_serial = MHGP6_MUTANT("pool-serial");
  const bool mutant = m_serial || MHGP6_MUTANT("pool-drop-exception") ||
                      MHGP6_MUTANT("pool-close-fatal-missing");

  // ---- (3) pic deterministe par latch, a N = 1, 2, 4, 8.
  bool peak_exact = true;
  for (const int N : {1, 2, 4, 8}) {
    g_built = 0;
    const u32 pk = latch_peak(N, &g_jobs);
    char what[128];
    std::snprintf(what, sizeof(what), "N=%d : pic d'activite %u == N (latch)", N, (unsigned)pk);
    if (pk != (u32)N) peak_exact = false;
    if (!mutant) expect(pk == (u32)N, what);
    else std::printf("mutant : %s -> %s\n", what, pk == (u32)N ? "atteint" : "PIC MANQUE");
    std::snprintf(what, sizeof(what), "N=%d : %d executeurs construits", N, g_built.load());
    if (!m_serial) expect(g_built.load() == N, what);
  }

  // ---- (6) domaine refuse hors de [1, 8], 3 accepte.
  {
    bool refused0 = false, refused9 = false, ok3 = false;
    try {
      gpu::ExecutorPool<FakeExecutor> p(0);
    } catch (const std::invalid_argument&) {
      refused0 = true;
    }
    try {
      gpu::ExecutorPool<FakeExecutor> p(9);
    } catch (const std::invalid_argument&) {
      refused9 = true;
    }
    {
      gpu::ExecutorPool<FakeExecutor> p(3);
      ok3 = p.executors() == (m_serial ? 1 : 3);
    }
    expect(refused0, "0 executeur : REFUS (jamais un clamp)");
    expect(refused9, "9 executeurs : REFUS (jamais un clamp)");
    expect(ok3, "3 executeurs : accepte (le domaine est tout entier de 1 a 8)");
  }

  // ---- (5) reentrance refusee.
  {
    gpu::ExecutorPool<FakeExecutor> pool(2);
    bool refused = false;
    pool.submit_and_wait([&](FakeExecutor&) {
      try {
        pool.submit_and_wait([](FakeExecutor&) {});
      } catch (const std::runtime_error&) {
        refused = true;
      }
    });
    ++g_jobs;
    expect(refused, "soumission reentrante depuis un travail du meme pool : REFUS immediat");
    // Aucun EMBOITEMENT n'est admis, meme vers un AUTRE pool.
    gpu::ExecutorPool<FakeExecutor> other(2);
    bool refused_other = false;
    pool.submit_and_wait([&](FakeExecutor&) {
      try {
        other.submit_and_wait([](FakeExecutor&) {});
      } catch (const std::runtime_error&) {
        refused_other = true;
      }
    });
    ++g_jobs;
    expect(refused_other, "soumission emboitee vers un AUTRE pool : REFUS immediat");
  }

  // ---- (7) contre-pression : file de capacite 1.
  {
    gpu::ExecutorPool<FakeExecutor> pool(2, 1);
    std::vector<std::thread> prod;
    for (int p = 0; p < 6; ++p)
      prod.emplace_back([&] {
        for (int j = 0; j < 20; ++j) pool.submit_and_wait([&](FakeExecutor& ex) { (void)ex.scan(7); });
      });
    for (std::thread& t : prod) t.join();
    g_jobs += 120;
    char what[128];
    std::snprintf(what, sizeof(what), "queue_cap=1 : pic de file %u <= 1, capacite annoncee %zu", (unsigned)pool.peak_queued(), pool.queue_cap());
    expect(pool.peak_queued() <= 1 && pool.queue_cap() == 1, what);
  }

  // ---- (9) ERREUR FATALE (P1) : le pool ferme l'admission, annule la file
  // avec l'erreur, reveille toutes les attentes, et la comptabilite se solde.
  {
    gpu::ExecutorPool<FakeExecutor> pool(2, 4);
    std::atomic<int> fatal_seen{0}, ok_seen{0};
    std::mutex m;
    std::condition_variable cv;
    bool hold = true;
    std::vector<std::thread> prod;
    // Les DEUX executeurs sont occupes par des travaux retenus : la file se
    // remplit vraiment, et l'annulation n'est pas vide (sans cela le test
    // serait vert par vacuite, `annules = 0`).
    for (int p = 0; p < 2; ++p)
      prod.emplace_back([&] {
        try {
          pool.submit_and_wait([&](FakeExecutor&) {
            std::unique_lock<std::mutex> lk(m);
            cv.wait_for(lk, std::chrono::seconds(5), [&] { return !hold; });
          });
          ++ok_seen;
        } catch (const std::exception&) {
          ++fatal_seen;
        }
      });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // laisser les deux travaux retenus demarrer
    for (int p = 0; p < 6; ++p)
      prod.emplace_back([&] {
        try {
          pool.submit_and_wait([&](FakeExecutor& ex) { (void)ex.scan(3); });
          ++ok_seen;
        } catch (const std::exception&) {
          ++fatal_seen;
        }
      });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    pool.close_fatal(std::make_exception_ptr(std::runtime_error("device perdu")));
    {
      std::lock_guard<std::mutex> lk(m);
      hold = false;
    }
    cv.notify_all();
    for (std::thread& t : prod) t.join();
    bool refused_after = false;
    try {
      pool.submit_and_wait([](FakeExecutor&) {});
    } catch (const std::exception&) {
      refused_after = true;
    }
    const auto c = pool.counters();
    g_jobs += (u64)c.submitted;
    char what[192];
    std::snprintf(what, sizeof(what), "erreur fatale : soumis=%llu reussis=%llu echoues=%llu annules=%llu actifs=%u en_file=%u (solde)",
                  (unsigned long long)c.submitted, (unsigned long long)c.succeeded, (unsigned long long)c.failed, (unsigned long long)c.cancelled,
                  (unsigned)c.active, (unsigned)c.queued);
    expect(c.balanced(), what);
    expect(pool.closed(), "erreur fatale : admission fermee");
    expect(refused_after, "erreur fatale : toute soumission ulterieure est refusee");
    expect(fatal_seen.load() + ok_seen.load() == 8, "erreur fatale : aucun producteur laisse en attente");
    expect(c.cancelled > 0, "erreur fatale : des tickets en file ont bien ete ANNULES (jamais vert par vacuite)");
    std::snprintf(what, sizeof(what), "erreur fatale : %d producteurs recoivent l'erreur, %d avaient deja abouti", fatal_seen.load(), ok_seen.load());
    std::printf("ok : %s\n", what);
  }

  // ---- (1), (2), (4) : P producteurs x J travaux, ordre, persistance, exceptions.
  const int P = 12, J = 100;
  u64 total = 0;
  bool order_ok = true, exc_ok = true;
  for (const int N : {1, 4}) {
    g_built = 0;
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
            if (j == 37 && p == 5) {
              if (!got_exc) exc_ok = false;
              continue;
            }
            r.push_back(y);
          }
        });
      for (std::thread& t : prod) t.join();
      g_jobs += (u64)(P * J);
      char what[128];
      std::snprintf(what, sizeof(what), "N=%d : travaux acheves %llu == soumis %d", N, (unsigned long long)pool.jobs_done(), P * J);
      expect(pool.jobs_done() == (u64)(P * J), what);
      if (!m_serial) {
        std::snprintf(what, sizeof(what), "N=%d : %d executeurs construits (persistants)", N, g_built.load());
        expect(g_built.load() == N, what);
      }
    }
    // (1) chaque producteur retrouve SES resultats dans SON ordre.
    for (int p = 0; p < P; ++p) {
      FakeExecutor ref;
      const size_t expect_n = (size_t)(p == 5 ? J - 1 : J);
      if (res[(size_t)p].size() != expect_n) {
        order_ok = false;
        continue;
      }
      size_t k = 0;
      for (int j = 0; j < J; ++j) {
        if (j == 37 && p == 5) continue;
        if (res[(size_t)p][k++] != ref.scan((u64)p * 100000ull + (u64)j)) order_ok = false;
      }
      total += res[(size_t)p].size();
    }
  }
  expect(order_ok, "ordre de soumission conserve par producteur");
  expect(exc_ok, "exception d'un travail relancee dans le producteur soumettant");

  // (9) CONFINEMENT DE PANNE FATALE.
  bool fatal_ok = true;
  {
    gpu::ExecutorPool<FakeExecutor> pool(2);
    bool rethrown = false;
    try {
      pool.submit_and_wait_contained([](FakeExecutor&) {
        throw gpu::DeviceFatalError("cudaErrorIllegalAddress simule");
      });
    } catch (const gpu::DeviceFatalError&) {
      rethrown = true;
    }
    fatal_ok = fatal_ok && rethrown;
    const bool closed_after = pool.closed();
    if (mutant && inject == "pool-close-fatal-missing") {
      // Mutant : la fermeture est sautee — un pool encore ouvert apres une
      // panne fatale est detecte et TUE.
      if (rethrown && !closed_after) {
        std::printf("mutant pool-close-fatal-missing TUE : pool ouvert apres panne fatale\n");
        return 4;
      }
      std::printf("MUTANT NON TUE\n");
      return 1;
    }
    fatal_ok = fatal_ok && closed_after;
    bool refused_with_fatal = false;
    try {
      pool.submit_and_wait_contained([](FakeExecutor&) {});
    } catch (const gpu::DeviceFatalError&) {
      refused_with_fatal = true;  // l'erreur FATALE prime sur l'arret ordinaire
    } catch (...) {
    }
    fatal_ok = fatal_ok && refused_with_fatal;
    const auto c = pool.counters();
    fatal_ok = fatal_ok && c.balanced();
  }
  expect(fatal_ok, "confinement de panne fatale : ferme, remonte, refus type, comptes equilibres");

  std::printf("executor_pool_gate travaux=%llu resultats=%llu echecs=%d pic_exact=%d\n", (unsigned long long)g_jobs, (unsigned long long)total, failures,
              peak_exact ? 1 : 0);
  if (mutant) {
    if (failures || !peak_exact || !exc_ok) return 4;
    std::printf("MUTANT NON TUE\n");
    return 1;
  }
  if (g_jobs < 1000) {
    std::printf("PLANCHER\n");
    return 3;
  }
  return failures ? 1 : 0;
}
