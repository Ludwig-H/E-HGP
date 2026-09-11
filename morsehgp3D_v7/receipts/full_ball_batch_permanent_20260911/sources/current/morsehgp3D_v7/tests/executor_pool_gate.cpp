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
//   (9) CONFINEMENT DE PANNE COTE WORKER (la dent v5 reparee, propriete du
//       pool depuis REPONSE_AUDITEURS_MULTICPU § 5.6) : une DeviceFatalError
//       levee par un travail FERME le pool (usage unique) AVANT la
//       notification du producteur, remonte typee, les soumissions suivantes
//       recoivent la MEME erreur fatale et la comptabilite reste equilibree
//       (soumis = reussis + echoues + annules) ;
//   (10) FIXTURE PERMANENTE de la course post-fatale (§ 5.6) : executeur
//       retenu, un second travail DEJA en file, fatal declenche — AUCUN
//       travail poste-fatal execute, tous les producteurs reveilles avec le
//       bon type, premiere erreur conservee, second ticket ANNULE (jamais
//       execute), soumission ulterieure refusee. La porte d'avant testait un
//       fatal isole et restait verte par vacuite sur cette course (199
//       reutilisations sur 200 dans la sonde auditeur). Sequencement par
//       LATCH fatal_entered + attestation d'etat (flake 36/400 corrige) ;
//   (11) echec de construction d'executeur, total et partiel : exception
//       relancee au constructeur, fils joints, jamais un pool a moitie pret.
// Codes : 0 ; 1 desaccord ; 2 refus d'argument ; 3 plancher ; 4 mutant tue.
// SELECTIVITE (5e contre-lecture § 5.6) : chaque injection saute DIRECTEMENT
// a sa scene-signature en tete de main — `pool-serial` : pic manque a
// N >= 2 ; `pool-drop-exception` : exception avalee (retour normal) ;
// `pool-worker-resume-after-fatal` : travail poste-fatal execute ;
// `pool-activate-after-unlock` : ticket manque (ni actif ni en file) a la
// fermeture linearisee. Zero scene nominale incompatible, zero echec
// parasite, plus aucune clause terminale « tue par n'importe quoi ».
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

using namespace mhgp7;

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
// (11) Executeurs inconstructibles : echec TOTAL et echec PARTIEL (le
// premier construit, le deuxieme leve) — le constructeur du pool doit
// joindre tous les fils deja lances puis relancer, jamais un pool a moitie
// pret ni un fil fuite.
std::atomic<int> g_flaky{0};
struct ThrowingExecutor {
  ThrowingExecutor() { throw std::runtime_error("executeur inconstructible"); }
};
struct FlakyExecutor {
  FlakyExecutor() {
    if (g_flaky.fetch_add(1) == 1) throw std::runtime_error("second executeur inconstructible");
  }
};
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
  const bool m_serial = MHGP7_MUTANT("pool-serial");
  const bool m_drop = MHGP7_MUTANT("pool-drop-exception");
  const bool m_resume = MHGP7_MUTANT("pool-worker-resume-after-fatal");
  const bool m_late = MHGP7_MUTANT("pool-activate-after-unlock");

  // ---- SELECTIVITE DES PORTES MUTANTES (5e contre-lecture § 5.6) : sous
  // chaque injection, la porte saute DIRECTEMENT a sa scene-signature —
  // aucune scene nominale incompatible, aucun echec parasite, aucun vert par
  // combinaison (l'ancienne clause terminale acceptait n'importe quelle
  // divergence). Signatures : pool-serial = pic manque a N >= 2 ;
  // pool-drop-exception = exception avalee ; pool-worker-resume-after-fatal
  // = travail poste-fatal execute ; pool-activate-after-unlock = ticket
  // manque (ni actif ni en file) a la fermeture linearisee.
  if (m_serial) {
    // N = 2 demande, executeur unique force : DEUX travaux retenus ne
    // peuvent etre actifs qu'UN a la fois. Latch adaptee au mutant (verdict
    // des que l'etat actif==1/file==1 est atteste — aucune attente de 5 s) ;
    // les DEUX observations (etat serialise ET g_built==1 ET pic==1) sont
    // accumulees AVANT le verdict (6e contre-lecture).
    g_built = 0;
    gpu::ExecutorPool<FakeExecutor> pool(2, 4);
    std::mutex m;
    std::condition_variable cv;
    bool release = false;
    const auto held = [&](FakeExecutor&) {
      std::unique_lock<std::mutex> lk(m);
      cv.wait_for(lk, std::chrono::seconds(10), [&] { return release; });
    };
    std::thread pa([&] {
      try {
        pool.submit_and_wait(held);
      } catch (...) {
      }
    });
    std::thread pb([&] {
      try {
        pool.submit_and_wait(held);
      } catch (...) {
      }
    });
    bool serialized = false;
    {
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      while (std::chrono::steady_clock::now() < deadline) {
        const auto c = pool.counters();
        if (c.active == 1 && c.queued == 1) {
          serialized = true;
          break;
        }
        if (c.active >= 2) break;  // deux actifs simultanes : PAS serialise
        std::this_thread::yield();
      }
    }
    {
      std::lock_guard<std::mutex> lk(m);
      release = true;
    }
    cv.notify_all();
    pa.join();
    pb.join();
    // Pic lu APRES les joins (7e contre-lecture : counters() peut voir
    // l'activation avant que le worker n'ait pousse la mise a jour du pic,
    // placee hors section critique — flake 1/9 observe). Compteur monotone :
    // la lecture post-join est complete sans sleep ni signature affaiblie.
    const u32 pk = pool.peak_active();
    if (serialized && g_built.load() == 1 && pk == 1) {
      std::printf("mutant pool-serial TUE : N=2 demande, actif=1 file=1 g_built=1 pic=1 attestes\n");
      return 4;
    }
    std::printf("MUTANT NON TUE (serialise=%d g_built=%d pic=%u)\n", serialized ? 1 : 0, g_built.load(),
                (unsigned)pk);
    return 1;
  }
  if (m_drop) {
    gpu::ExecutorPool<FakeExecutor> pool(2);
    bool received = false;
    try {
      pool.submit_and_wait([](FakeExecutor&) { throw std::runtime_error("erreur ordinaire"); });
    } catch (const std::exception&) {
      received = true;
    }
    if (!received) {
      std::printf("mutant pool-drop-exception TUE : exception d'un travail avalee (retour normal)\n");
      return 4;
    }
    std::printf("MUTANT NON TUE\n");
    return 1;
  }
  if (m_resume) {
    // Scene-signature = la fixture § 5.6 : executeur unique retenu, second
    // travail EN FILE, fatal — le worker mutant reboucle et l'execute.
    gpu::ExecutorPool<FakeExecutor> pool(1);
    std::mutex m;
    std::condition_variable cv;
    bool release = false, fatal_entered = false;
    std::atomic<bool> job2_ran{false};
    std::thread p1([&] {
      try {
        pool.submit_and_wait([&](FakeExecutor&) {
          std::unique_lock<std::mutex> lk(m);
          fatal_entered = true;
          cv.notify_all();
          cv.wait_for(lk, std::chrono::seconds(10), [&] { return release; });
          throw gpu::DeviceFatalError("device perdu signature");
        });
      } catch (...) {
      }
    });
    {
      std::unique_lock<std::mutex> lk(m);
      cv.wait_for(lk, std::chrono::seconds(10), [&] { return fatal_entered; });
      if (!fatal_entered) {  // PRECONDITION de fixture (6e contre-lecture) :
        // jamais un 4 par echeance — un etat non atteste est un REFUS (3).
        std::printf("REFUS FIXTURE : fatal_entered non atteste\n");
        release = true;  // m est deja tenu par lk
        lk.unlock();
        cv.notify_all();
        p1.join();
        return 3;
      }
    }
    std::thread p2([&] {
      try {
        pool.submit_and_wait([&](FakeExecutor&) { job2_ran.store(true); });
      } catch (...) {
      }
    });
    {
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      while (pool.counters().queued < 1 && std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
      if (pool.counters().queued < 1) {  // PRECONDITION : second travail EN FILE
        std::printf("REFUS FIXTURE : second travail jamais atteste en file\n");
        {
          std::lock_guard<std::mutex> lg(m);
          release = true;
        }
        cv.notify_all();
        p1.join();
        p2.join();
        return 3;
      }
    }
    {
      std::lock_guard<std::mutex> lk(m);
      release = true;
    }
    cv.notify_all();
    p1.join();
    p2.join();
    if (job2_ran.load()) {
      std::printf("mutant pool-worker-resume-after-fatal TUE : travail poste-fatal execute\n");
      return 4;
    }
    std::printf("MUTANT NON TUE\n");
    return 1;
  }
  if (m_late) {
    // Scene-signature (6e contre-lecture) : le hook pre_activate est appele
    // HORS verrou sous ce mutant (l'activation l'a suivi apres l'unlock) —
    // un fil CLOSER ferme pendant le hook, se termine (mu_ libre), et son
    // etat capture montre le ticket MANQUE : ni actif, ni en file, ni
    // annule. Le hook n'est libere qu'APRES la capture ; le corps du job est
    // retenu pour ecarter toute course avec sa decrementation.
    gpu::ExecutorPool<FakeExecutor> pool(1);
    std::mutex m;
    std::condition_variable cv;
    bool in_hook = false, release_hook = false, release_job = false;
    pool.test_hook_pre_activate = [&] {
      std::unique_lock<std::mutex> lk(m);
      in_hook = true;
      cv.notify_all();
      cv.wait_for(lk, std::chrono::seconds(10), [&] { return release_hook; });
    };
    std::thread p([&] {
      try {
        pool.submit_and_wait([&](FakeExecutor&) {
          std::unique_lock<std::mutex> lk(m);
          cv.wait_for(lk, std::chrono::seconds(10), [&] { return release_job; });
        });
      } catch (...) {
      }
    });
    {
      std::unique_lock<std::mutex> lk(m);
      cv.wait_for(lk, std::chrono::seconds(10), [&] { return in_hook; });
      if (!in_hook) {
        std::printf("REFUS FIXTURE : hook pre_activate jamais atteint\n");
        release_hook = release_job = true;
        lk.unlock();
        cv.notify_all();
        p.join();
        return 3;
      }
    }
    std::thread closer([&] {
      pool.close_fatal(std::make_exception_ptr(std::runtime_error("fermeture linearisee dans la fenetre")));
    });
    closer.join();  // mu_ est LIBRE sous le mutant : la fermeture s'intercale
    const auto snap = pool.counters();
    const bool missed = snap.active == 0 && snap.queued == 0 && snap.cancelled == 0 &&
                        snap.submitted == 1 && snap.succeeded == 0 && snap.failed == 0;
    {
      std::lock_guard<std::mutex> lk(m);
      release_hook = true;
      release_job = true;
    }
    cv.notify_all();
    p.join();
    if (missed) {
      std::printf("mutant pool-activate-after-unlock TUE : ticket manque a la fermeture (ni actif ni en file)\n");
      return 4;
    }
    std::printf("MUTANT NON TUE\n");
    return 1;
  }

  // ---- (3) pic deterministe par latch, a N = 1, 2, 4, 8.
  bool peak_exact = true;
  for (const int N : {1, 2, 4, 8}) {
    g_built = 0;
    const u32 pk = latch_peak(N, &g_jobs);
    char what[128];
    std::snprintf(what, sizeof(what), "N=%d : pic d'activite %u == N (latch)", N, (unsigned)pk);
    if (pk != (u32)N) peak_exact = false;
    expect(pk == (u32)N, what);
    std::snprintf(what, sizeof(what), "N=%d : %d executeurs construits", N, g_built.load());
    expect(g_built.load() == N, what);
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
    int held_started = 0;
    std::vector<std::thread> prod;
    // Les DEUX executeurs sont occupes par des travaux retenus : la file se
    // remplit vraiment, et l'annulation n'est pas vide (sans cela le test
    // serait vert par vacuite, `annules = 0`). Sequencement par LATCH, plus
    // aucune temporisation (audit § 5.6 : les sleeps refermaient seulement le
    // cas mesure).
    for (int p = 0; p < 2; ++p)
      prod.emplace_back([&] {
        try {
          pool.submit_and_wait([&](FakeExecutor&) {
            std::unique_lock<std::mutex> lk(m);
            ++held_started;
            cv.notify_all();
            cv.wait_for(lk, std::chrono::seconds(5), [&] { return !hold; });
          });
          ++ok_seen;
        } catch (const std::exception&) {
          ++fatal_seen;
        }
      });
    {  // latch : les deux travaux retenus ONT demarre (executeurs occupes) —
       // resultat ATTESTE (6e contre-lecture : un timeout de fixture est un
       // echec explicite, jamais une poursuite sur un etat non atteste)
      std::unique_lock<std::mutex> lk(m);
      cv.wait_for(lk, std::chrono::seconds(5), [&] { return held_started >= 2; });
      expect(held_started >= 2, "erreur fatale : les deux travaux retenus attestes demarres");
    }
    for (int p = 0; p < 6; ++p)
      prod.emplace_back([&] {
        try {
          pool.submit_and_wait([&](FakeExecutor& ex) { (void)ex.scan(3); });
          ++ok_seen;
        } catch (const std::exception&) {
          ++fatal_seen;
        }
      });
    // Attestation d'ETAT bornee (defaut de fixture => echec rapide, jamais un
    // timeout CTest) : la file atteint sa capacite 4 — garanti par
    // construction (executeurs retenus, 6 producteurs).
    {
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      while (pool.counters().queued < 4 && std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
      expect(pool.counters().queued >= 4, "erreur fatale : file remplie (4) attestee avant fermeture");
    }
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

  // (9) CONFINEMENT DE PANNE FATALE COTE WORKER : le pool est ferme quand
  // l'erreur remonte au producteur (aucun wrapper cote producteur).
  bool fatal_ok = true;
  {
    gpu::ExecutorPool<FakeExecutor> pool(2);
    bool rethrown = false;
    try {
      pool.submit_and_wait([](FakeExecutor&) {
        throw gpu::DeviceFatalError("cudaErrorIllegalAddress simule");
      });
    } catch (const gpu::DeviceFatalError&) {
      rethrown = true;
    }
    fatal_ok = fatal_ok && rethrown;
    fatal_ok = fatal_ok && pool.closed();
    bool refused_with_fatal = false;
    try {
      pool.submit_and_wait([](FakeExecutor&) {});
    } catch (const gpu::DeviceFatalError&) {
      refused_with_fatal = true;  // l'erreur FATALE prime sur l'arret ordinaire
    } catch (...) {
    }
    fatal_ok = fatal_ok && refused_with_fatal;
    const auto c = pool.counters();
    fatal_ok = fatal_ok && c.balanced();
    expect(fatal_ok, "confinement worker : ferme AVANT le reveil, remonte type, refus type, comptes equilibres");
  }

  // (10) FIXTURE PERMANENTE de la course post-fatale (§ 5.6) : N = 1 ; le
  // travail fatal RETIENT l'executeur jusqu'a ce qu'un second travail soit
  // observe EN FILE, puis leve. Personne d'autre que le worker ne peut fermer
  // (aucun wrapper producteur) : si le worker reboucle, le second travail
  // s'execute sur l'executeur empoisonne — c'est exactement la course.
  bool race_ok = true;
  {
    gpu::ExecutorPool<FakeExecutor> pool(1);
    std::mutex m;
    std::condition_variable cv;
    bool release = false;
    bool fatal_entered = false;  // latch : le travail fatal RETIENT l'executeur
    std::atomic<bool> job2_ran{false};
    bool p1_fatal_typed = false, p2_fatal_typed = false;
    std::string p1_msg;
    std::thread p1([&] {
      try {
        pool.submit_and_wait([&](FakeExecutor&) {
          std::unique_lock<std::mutex> lk(m);
          fatal_entered = true;
          cv.notify_all();
          cv.wait_for(lk, std::chrono::seconds(10), [&] { return release; });
          throw gpu::DeviceFatalError("device perdu scenario10");
        });
      } catch (const gpu::DeviceFatalError& e) {
        p1_fatal_typed = true;
        p1_msg = e.what();
      } catch (...) {
      }
    });
    // SEQUENCEMENT (flake 36/400 releve par l'auditeur : p2 pouvait gagner
    // l'ordonnancement et executer AVANT que le fatal ne retienne l'unique
    // executeur) : latch attendue AVANT de lancer le second producteur.
    {
      std::unique_lock<std::mutex> lk(m);
      cv.wait_for(lk, std::chrono::seconds(10), [&] { return fatal_entered; });
      race_ok = race_ok && fatal_entered;
    }
    std::thread p2([&] {
      try {
        pool.submit_and_wait([&](FakeExecutor&) { job2_ran.store(true); });
      } catch (const gpu::DeviceFatalError&) {
        p2_fatal_typed = true;  // ticket ANNULE avec l'erreur fatale, jamais execute
      } catch (...) {
      }
    });
    // Attestation d'ETAT bornee (defaut de fixture => echec rapide, jamais un
    // timeout CTest) : le second travail est en file — garanti par
    // construction, l'executeur unique etant retenu.
    {
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      while (pool.counters().queued < 1 && std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
      race_ok = race_ok && pool.counters().queued >= 1;
    }
    {
      std::lock_guard<std::mutex> lk(m);
      release = true;
    }
    cv.notify_all();
    p1.join();  // tous les producteurs reveilles (le join borne par le wait_for)
    p2.join();
    race_ok = race_ok && !job2_ran.load();                        // aucun travail poste-fatal
    race_ok = race_ok && p1_fatal_typed && p2_fatal_typed;        // reveils avec le bon type
    race_ok = race_ok && p1_msg == "device perdu scenario10";     // premiere erreur conservee
    bool refused_typed = false;
    try {
      pool.submit_and_wait([](FakeExecutor&) {});
    } catch (const gpu::DeviceFatalError& e) {
      refused_typed = std::string(e.what()) == "device perdu scenario10";
    } catch (...) {
    }
    race_ok = race_ok && refused_typed;
    const auto c = pool.counters();
    race_ok = race_ok && c.balanced() && c.submitted == 2 && c.failed == 1 && c.cancelled == 1 && c.succeeded == 0;
  }
  expect(race_ok, "course post-fatale : second ticket annule jamais execute, reveils types, comptes soldes");

  // (11) ECHEC DE CONSTRUCTION D'EXECUTEUR (durcissement § 5.6) :
  // transactionnel — exception relancee au constructeur, fils joints (la
  // destruction du pool partiel ne bloque ni ne fuit).
  bool ctor_ok = true;
  {
    bool threw = false;
    try {
      gpu::ExecutorPool<ThrowingExecutor> pool(2);
    } catch (const std::exception&) {
      threw = true;
    }
    ctor_ok = ctor_ok && threw;
  }
  {
    g_flaky = 0;
    bool threw = false;
    try {
      gpu::ExecutorPool<FlakyExecutor> pool(2);
    } catch (const std::exception&) {
      threw = true;
    }
    ctor_ok = ctor_ok && threw;
  }
  expect(ctor_ok, "echec de construction (total et partiel) : relance, fils joints");

  // (12) FENETRE FILE->ACTIF a N = 2 (3e contre-lecture § 5.6) : l'instant
  // d'activation est le RETRAIT sous mu_ — a la fermeture fatale, chaque
  // ticket est soit en file (ANNULE) soit actif (il VA AU BOUT et son
  // executeur se retire). Pair actif + file pleine + producteur bloque sur
  // la contre-pression : reception complete du contrat declare.
  bool pair_ok = true;
  {
    gpu::ExecutorPool<FakeExecutor> pool(2, 2);
    std::mutex m;
    std::condition_variable cv;
    int held_started = 0;
    bool release_fatal = false, release_pair = false;
    std::atomic<bool> j3_ran{false}, j4_ran{false};
    bool p1_typed = false, p2_done = false, p5_typed = false;
    std::atomic<int> p34_typed{0};  // incremente par DEUX producteurs (4e contre-lecture : data race si int nu)
    std::thread p1([&] {
      try {
        pool.submit_and_wait([&](FakeExecutor&) {
          std::unique_lock<std::mutex> lk(m);
          ++held_started;
          cv.notify_all();
          cv.wait_for(lk, std::chrono::seconds(10), [&] { return release_fatal; });
          throw gpu::DeviceFatalError("device perdu scenario12");
        });
      } catch (const gpu::DeviceFatalError&) {
        p1_typed = true;
      } catch (...) {
      }
    });
    std::thread p2([&] {
      try {
        pool.submit_and_wait([&](FakeExecutor&) {
          std::unique_lock<std::mutex> lk(m);
          ++held_started;
          cv.notify_all();
          cv.wait_for(lk, std::chrono::seconds(10), [&] { return release_pair; });
        });
        p2_done = true;  // le pair ACTIF au fatal va au bout (contrat declare)
      } catch (...) {
      }
    });
    {  // latch : les deux executeurs sont retenus
      std::unique_lock<std::mutex> lk(m);
      cv.wait_for(lk, std::chrono::seconds(10), [&] { return held_started >= 2; });
      pair_ok = pair_ok && held_started >= 2;
    }
    std::thread p3([&] {
      try {
        pool.submit_and_wait([&](FakeExecutor&) { j3_ran.store(true); });
      } catch (const gpu::DeviceFatalError&) {
        ++p34_typed;
      } catch (...) {
      }
    });
    std::thread p4([&] {
      try {
        pool.submit_and_wait([&](FakeExecutor&) { j4_ran.store(true); });
      } catch (const gpu::DeviceFatalError&) {
        ++p34_typed;
      } catch (...) {
      }
    });
    {  // file pleine (cap 2) attestee, borne rapide
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      while (pool.counters().queued < 2 && std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
      pair_ok = pair_ok && pool.counters().queued >= 2;
    }
    std::thread p5([&] {  // bloque sur la contre-pression (ou refuse si deja ferme)
      try {
        pool.submit_and_wait([&](FakeExecutor& ex) { (void)ex.scan(5); });
      } catch (const gpu::DeviceFatalError&) {
        p5_typed = true;
      } catch (...) {
      }
    });
    {
      std::lock_guard<std::mutex> lk(m);
      release_fatal = true;
    }
    cv.notify_all();
    p1.join();
    p3.join();
    p4.join();
    p5.join();
    {
      std::lock_guard<std::mutex> lk(m);
      release_pair = true;
    }
    cv.notify_all();
    p2.join();
    pair_ok = pair_ok && p1_typed && p2_done && p5_typed && p34_typed.load() == 2;
    pair_ok = pair_ok && !j3_ran.load() && !j4_ran.load();  // aucun travail en file execute
    const auto c = pool.counters();
    pair_ok = pair_ok && c.balanced() && c.submitted == 4 && c.succeeded == 1 && c.failed == 1 && c.cancelled == 2;
    g_jobs += (u64)c.submitted;
  }
  // NB : la porte ne revendique PAS que p5 fut observe DANS l'attente de
  // place — il est refuse type qu'il ait ete bloque puis reveille ou qu'il
  // soit arrive apres la fermeture (les deux chemins sont conformes).
  expect(pair_ok, "fenetre file->actif (N=2) : pair actif au bout, file annulee, p5 refuse type, comptes soldes");

  // (13) CONTRE-PREUVE PERMANENTE de l'instant d'activation (6e
  // contre-lecture § 5.6) : le hook pre_activate est appele SOUS mu_,
  // immediatement avant `active++`. Un fil CLOSER qui appelle close_fatal
  // pendant le hook reste DERRIERE le verrou — il ne peut se lineariser
  // qu'apres l'activation, et voit donc le ticket ACTIF. Le corps du job est
  // retenu jusqu'au snapshot (aucune course avec sa decrementation). Sous le
  // mutant, la scene-signature du dispatch (en tete de main) montre le
  // ticket MANQUE.
  bool hook_ok = true;
  {
    gpu::ExecutorPool<FakeExecutor> pool(1);
    std::mutex m;
    std::condition_variable cv;
    bool in_hook = false, release_hook = false, release_job = false;
    pool.test_hook_pre_activate = [&] {
      std::unique_lock<std::mutex> lk(m);
      in_hook = true;
      cv.notify_all();
      cv.wait_for(lk, std::chrono::seconds(10), [&] { return release_hook; });
    };
    bool p_ok = false;
    std::thread p([&] {
      try {
        pool.submit_and_wait([&](FakeExecutor&) {
          std::unique_lock<std::mutex> lk(m);
          cv.wait_for(lk, std::chrono::seconds(10), [&] { return release_job; });
        });
        p_ok = true;  // le travail actif au fatal VA AU BOUT (contrat declare)
      } catch (...) {
      }
    });
    {
      std::unique_lock<std::mutex> lk(m);
      cv.wait_for(lk, std::chrono::seconds(10), [&] { return in_hook; });
      hook_ok = hook_ok && in_hook;
    }
    // Le closer CONTESTE mu_ pendant le hook : il ne peut fermer qu'apres la
    // fin de la section critique (activation comprise).
    std::thread closer([&] {
      pool.close_fatal(std::make_exception_ptr(std::runtime_error("fermeture linearisee dans la fenetre")));
    });
    {
      std::lock_guard<std::mutex> lk(m);
      release_hook = true;
    }
    cv.notify_all();
    closer.join();
    const auto snap = pool.counters();
    hook_ok = hook_ok && snap.active == 1 && snap.queued == 0 && snap.cancelled == 0;  // ticket VU actif
    {
      std::lock_guard<std::mutex> lk(m);
      release_job = true;
    }
    cv.notify_all();
    p.join();
    hook_ok = hook_ok && p_ok;
    const auto c = pool.counters();
    hook_ok = hook_ok && c.balanced() && c.submitted == 1 && c.succeeded == 1 && c.cancelled == 0;
    g_jobs += 1;
  }
  expect(hook_ok, "instant d'activation : fermeture linearisee pendant le hook pre_activate voit le ticket ACTIF, travail au bout");

  std::printf("executor_pool_gate travaux=%llu resultats=%llu echecs=%d pic_exact=%d\n", (unsigned long long)g_jobs, (unsigned long long)total, failures,
              peak_exact ? 1 : 0);
  // Plus AUCUNE clause terminale mutante : chaque injection a rendu son
  // verdict dans sa scene-signature dediee, en tete de main (§ 5.6).
  if (g_jobs < 1000) {
    std::printf("PLANCHER\n");
    return 3;
  }
  return failures ? 1 : 0;
}

