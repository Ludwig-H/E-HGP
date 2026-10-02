// Portes unitaires des fondations : entiers larges (juge a arithmetique decimale independante),
// ordonnanceur (couverture exacte, determinisme, imbrication serialisee, exceptions), statuts et tampons, frontieres
// des sondes (lecteur u32le, parseur strict, parametres du catalogue, garde des boules, budget de noeuds, sorties).
//   mhgp10_unit            : toutes les portes
//   mhgp10_unit GROUPE     : un seul groupe du pool, chacun une porte CTest separee : exceptions
//                            (pool_caller_exception, pool_worker_exception, pool_flag_restored, pool_cancel_and_reuse),
//                            tranches pres de 2^64 (pool_claim_wrap), travaux courts enchaines (pool_short_jobs),
//                            participation des fils apres un echange perdu (pool_participation)
// Codes : 0 conforme, 1 desaccord d'un juge, 2 groupe inconnu, 3 plancher non atteint.
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <new>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "arith/wide.hpp"
#include "catalogue/catalogue.hpp"
#include "cloud/cloud.hpp"
#include "cloud/site_tree.hpp"
#include "cloud/u32le_input.hpp"
#include "core/buffer.hpp"
#include "core/cli_options.hpp"
#include "core/cli_output.hpp"
#include "core/status.hpp"
#include "head/head.hpp"
#include "sched/pool.hpp"

using namespace mhgp10;

namespace {

int failures = 0;
void expect(bool ok, const char* what) {
  if (!ok) {
    ++failures;
    std::printf("ECHEC %s\n", what);
    std::fflush(stdout);  // conserve le diagnostic si un defaut du pool fait ensuite tomber le processus
  }
}

// ---- juge decimal : chaines de chiffres, arithmetique d'ecole (algorithme volontairement autre) ----
std::string dec_abs(const std::string& s) { return s[0] == '-' ? s.substr(1) : s; }
int dec_cmp_abs(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
  return a < b ? -1 : (a > b ? 1 : 0);
}
std::string dec_strip(std::string s) {
  size_t i = 0;
  while (i + 1 < s.size() && s[i] == '0') ++i;
  return s.substr(i);
}
std::string dec_add_abs(const std::string& a, const std::string& b) {
  std::string r;
  int carry = 0;
  for (int i = int(a.size()) - 1, j = int(b.size()) - 1; i >= 0 || j >= 0 || carry; --i, --j) {
    int d = carry + (i >= 0 ? a[i] - '0' : 0) + (j >= 0 ? b[j] - '0' : 0);
    r.push_back(char('0' + d % 10));
    carry = d / 10;
  }
  return dec_strip(std::string(r.rbegin(), r.rend()));
}
std::string dec_sub_abs(const std::string& a, const std::string& b) {  // |a| >= |b|
  std::string r;
  int borrow = 0;
  for (int i = int(a.size()) - 1, j = int(b.size()) - 1; i >= 0; --i, --j) {
    int d = (a[i] - '0') - borrow - (j >= 0 ? b[j] - '0' : 0);
    borrow = d < 0;
    if (d < 0) d += 10;
    r.push_back(char('0' + d));
  }
  return dec_strip(std::string(r.rbegin(), r.rend()));
}
std::string dec_add(const std::string& a, const std::string& b) {
  const bool na = a[0] == '-', nb = b[0] == '-';
  const std::string A = dec_abs(a), B = dec_abs(b);
  std::string m;
  bool neg;
  if (na == nb) {
    m = dec_add_abs(A, B);
    neg = na;
  } else if (dec_cmp_abs(A, B) >= 0) {
    m = dec_sub_abs(A, B);
    neg = na;
  } else {
    m = dec_sub_abs(B, A);
    neg = nb;
  }
  return (neg && m != "0" ? "-" : "") + m;
}
std::string dec_mul(const std::string& a, const std::string& b) {
  const std::string A = dec_abs(a), B = dec_abs(b);
  std::vector<int> r(A.size() + B.size(), 0);
  for (int i = int(A.size()) - 1; i >= 0; --i)
    for (int j = int(B.size()) - 1; j >= 0; --j) r[i + j + 1] += (A[i] - '0') * (B[j] - '0');
  for (int k = int(r.size()) - 1; k > 0; --k) {
    r[k - 1] += r[k] / 10;
    r[k] %= 10;
  }
  std::string s;
  for (int d : r) s.push_back(char('0' + d));
  s = dec_strip(s);
  const bool neg = (a[0] == '-') != (b[0] == '-');
  return (neg && s != "0" ? "-" : "") + s;
}

i128 random_i128(std::mt19937_64& g) {
  const int bits = int(g() % 127);
  u128 v = (u128(g()) << 64) | g();
  v = bits == 0 ? 0 : (v >> (128 - bits));
  return (g() & 1) ? -static_cast<i128>(v) : static_cast<i128>(v);
}

void test_wide() {
  using namespace mhgp10::arith;
  std::mt19937_64 g(20260928);
  u64 checks = 0;
  for (int t = 0; t < 20000; ++t) {
    const i128 x = random_i128(g), y = random_i128(g), z = random_i128(g);
    const auto X = I192::from_i128(x), Y = I192::from_i128(y);
    const auto P = mul(X, Y);                          // Wide<6>
    const auto Z = I128w::from_i128(z);
    const auto Q = mul(P, Z);                          // Wide<8>
    const std::string sx = to_string(X), sy = to_string(Y);
    expect(to_string(P) == dec_mul(sx, sy), "mul");
    expect(to_string(Q) == dec_mul(dec_mul(sx, sy), to_string(Z)), "mul triple");
    I192 S;
    if (add(X, Y, S)) expect(to_string(S) == dec_add(sx, sy), "add");
    I192 D;
    if (sub(X, Y, D)) expect(to_string(D) == dec_add(sx, (sy[0] == '-' ? sy.substr(1) : (sy == "0" ? sy : "-" + sy))), "sub");
    const int c = cmp(X, Y);
    const int cr = (x < y) ? -1 : (x > y ? 1 : 0);
    expect(c == cr, "cmp");
    // produits croises : comparaison de fractions x/1 * y vs y * x
    expect(cmp(mul(X, Y), mul(Y, X)) == 0, "commutativite");
    checks += 5;
  }
  // debordement detecte
  I192 big;
  big.w[2] = ~u64{0};
  big.w[1] = ~u64{0};
  big.w[0] = ~u64{0};
  I192 out;
  expect(!add(big, I192::from_i128(1), out), "debordement add");
  std::printf("wide_checks %llu\n", static_cast<unsigned long long>(checks));
  if (checks < 100000) {
    std::printf("PLANCHER wide_checks\n");
    failures += 1000;
  }
}

void test_pool() {
  for (unsigned threads : {1u, 2u, 5u, 8u}) {
    sched::Pool pool(threads);
    const u64 n = 1000003;
    std::vector<u64> out(n, 0);
    pool.parallel_for(n, 777, [&](u64 b, u64 e, unsigned) {
      for (u64 i = b; i < e; ++i) out[i] = i * i + 1;
    });
    bool ok = true;
    for (u64 i = 0; i < n; ++i) ok &= out[i] == i * i + 1;
    expect(ok, "pool couverture");
    // imbrication : serialisee et comptee
    std::vector<u64> acc(64, 0);
    pool.parallel_for(64, 1, [&](u64 b, u64, unsigned) {
      u64 s = 0;
      pool.parallel_for(100, 7, [&](u64 bb, u64 ee, unsigned) {
        for (u64 i = bb; i < ee; ++i) s += i;
      });
      acc[b] = s;
    });
    bool ok2 = true;
    for (u64 v : acc) ok2 &= v == 4950;
    expect(ok2, "pool imbrique");
    if (threads > 1) expect(pool.nested_calls() == 64, "pool imbrication comptee");
    // reutilisation repetee
    for (int rep = 0; rep < 200; ++rep) {
      std::vector<u32> v(1000, 0);
      pool.parallel_for(1000, 3, [&](u64 b, u64 e, unsigned) {
        for (u64 i = b; i < e; ++i) v[i] = u32(i);
      });
      bool ok3 = true;
      for (u32 i = 0; i < 1000; ++i) ok3 &= v[i] == i;
      expect(ok3, "pool reutilisation");
    }
  }
}

// Travaux courts enchaines (regression du 29 septembre 2026) : un ouvrier en retard ne doit jamais executer une
// tranche d'un travail avec la fonction d'un autre, ni lire les champs d'un travail sans synchronisation. Chaque
// travail a sa propre fonction, gardee en vie, et ses propres compteurs : chaque indice doit etre execute exactement
// une fois, par la fonction de son travail.
void test_pool_short_jobs() {
  // un travail d'un indice suivi d'un travail de 64 : le compteur perime d'un ouvrier en retard (>= 1) tombe alors
  // dans la plage du travail suivant, qui serait execute deux fois ou par la mauvaise fonction
  for (unsigned threads : {4u, 8u}) {
    sched::Pool pool(threads);
    const u32 jobs = 50000, width = 64;
    std::vector<std::atomic<u32>> hits(u64(jobs) * width);
    std::vector<std::function<void(u64, u64, unsigned)>> fns;
    fns.reserve(jobs);
    for (u32 j = 0; j < jobs; ++j)
      fns.emplace_back([&hits, j](u64 b, u64 e, unsigned) {
        for (u64 i = b; i < e; ++i) hits[u64(j) * width + i].fetch_add(1, std::memory_order_relaxed);
      });
    auto size_of = [&](u32 j) -> u32 { return j % 2 ? width : 1 + j % 3; };
    for (u32 j = 0; j < jobs; ++j) pool.parallel_for(size_of(j), 1, fns[j]);
    u64 bad = 0;
    for (u32 j = 0; j < jobs; ++j)
      for (u32 i = 0; i < width; ++i) bad += hits[u64(j) * width + i].load() != (i < size_of(j) ? 1u : 0u);
    if (bad) std::printf("pool travaux courts : %llu cases fausses a %u fils\n", (unsigned long long)bad, threads);
    expect(bad == 0, "pool travaux courts enchaines");
  }
}

// ---- Exceptions dans le pool (audit continu pool_head § 1, audit independant § 2, 29 septembre 2026) ----
// Contrat (sched/pool.hpp) : la premiere exception levee par body, dans l'appelant ou dans un ouvrier, est capturee ;
// aucune tranche reclamee apres sa capture n'est executee ; parallel_for ne rend la main qu'apres la sortie de tous les
// fils entres dans le travail, puis la relance dans l'appelant ; le drapeau de region est restaure ; le pool reste
// utilisable. Chaque groupe echoue sur le pool d'avant la correction (sortie anticipee, std::terminate, drapeau colle).

struct ChunkError {  // exception de test : tranche et fil qui l'ont levee
  u64 begin;
  unsigned worker;
};

// Attente active bornee : vrai si ready() devient vrai avant `ms` millisecondes.
template <class Ready>
bool wait_until(Ready ready, int ms) {
  const auto limit = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
  while (!ready()) {
    if (std::chrono::steady_clock::now() >= limit) return false;
    std::this_thread::yield();
  }
  return true;
}

// Etat du pool apres une exception : drapeau de region baisse dans l'appelant, aucun appel ordinaire compte comme
// imbrique, couverture exacte d'un travail ordinaire, et participation de tous les fils (P tranches qui attendent
// chacune que P fils distincts soient entres : un ouvrier mort ou bloque fait expirer l'attente).
void expect_pool_reusable(sched::Pool& pool, u64 nested_before, const char* what) {
  const bool clean = !sched::in_parallel_region() && pool.nested_calls() == nested_before;
  expect(clean, what);
  if (!clean) return;  // drapeau colle : tout parallel_for suivant passerait en serie et l'attente ci-dessous expirerait
  const u64 n = 100003;
  std::vector<u64> out(n, 0);
  pool.parallel_for(n, 7, [&](u64 b, u64 e, unsigned) {
    for (u64 i = b; i < e; ++i) out[i] = i * i + 1;
  });
  bool ok = true;
  for (u64 i = 0; i < n; ++i) ok &= out[i] == i * i + 1;
  expect(ok, what);
  const unsigned P = pool.size();
  std::vector<std::atomic<u32>> seen(P);
  std::atomic<u32> distinct{0}, timeouts{0};
  pool.parallel_for(P, 1, [&](u64, u64, unsigned id) {
    if (seen[id].fetch_add(1) == 0) distinct.fetch_add(1);
    if (!wait_until([&] { return distinct.load() == P; }, 20000)) timeouts.fetch_add(1);
  });
  expect(distinct.load() == P && timeouts.load() == 0, what);
  expect(!sched::in_parallel_region() && pool.nested_calls() == nested_before, what);
}

// (a) Exception du fil appelant pendant qu'un ouvrier execute le rappel (sonde pool_throw.cpp de l'audit continu,
// sans son interblocage : l'ouvrier ne depend plus du catch de l'appelant pour sortir). L'ouvrier reste dans le rappel
// 100 ms apres la levee en guettant le catch : parallel_for ne doit pas rendre la main avant sa sortie. Pool d'avant :
// l'appelant sortait aussitot et l'ouvrier lisait ensuite une fonction detruite (ASan stack-use-after-scope).
void test_pool_caller_exception() {
  u64 scenarios = 0;
  for (unsigned threads : {2u, 4u, 8u}) {
    sched::Pool pool(threads);
    std::atomic<u32> inside{0}, saw_catch{0};
    std::atomic<bool> thrown{false}, caught{false};
    u32 inside_at_throw = 0, inside_at_catch = ~0u;
    bool got = false;
    try {
      pool.parallel_for(threads, 1, [&](u64 b, u64, unsigned id) {
        if (id == 0) {
          wait_until([&] { return inside.load() > 0; }, 20000);
          inside_at_throw = inside.load();
          thrown.store(true);
          throw ChunkError{b, id};
        }
        inside.fetch_add(1);
        wait_until([&] { return thrown.load(); }, 20000);
        if (wait_until([&] { return caught.load(); }, 100)) saw_catch.fetch_add(1);
        inside.fetch_sub(1);
      });
    } catch (const ChunkError& e) {
      got = e.worker == 0;
      inside_at_catch = inside.load();
      caught.store(true);
    }
    expect(got, "pool exception appelant relancee");
    expect(inside_at_catch == 0 && saw_catch.load() == 0, "pool exception appelant : sortie apres tous les ouvriers");
    scenarios += inside_at_throw > 0;
    expect_pool_reusable(pool, 0, "pool reutilisable apres exception appelant");
  }
  std::printf("pool_caller_exception scenarios %llu\n", static_cast<unsigned long long>(scenarios));
  if (scenarios < 3) failures += 1000;  // un ouvrier doit etre dans le rappel au moment de la levee
}

// (b) Exception dans un ouvrier (sonde pool_failure_probe.cpp de l'audit independant : std::bad_alloc dans un
// ouvrier pendant que l'appelant est dans sa tranche ; pool d'avant : std::terminate, SIGABRT). Puis toutes les
// tranches levent, chacune avec son identite : une exception levee atteint l'appelant, et chaque fil s'arrete a sa
// premiere levee (au plus une tranche par fil).
void test_pool_worker_exception() {
  u64 scenarios = 0;
  for (unsigned threads : {2u, 4u, 8u}) {
    sched::Pool pool(threads);
    {
      std::atomic<bool> worker_threw{false}, gave_up{false};
      bool got = false;
      try {
        pool.parallel_for(64, 1, [&](u64, u64, unsigned id) {
          if (id != 0) {
            worker_threw.store(true);
            throw std::bad_alloc();
          }
          if (!gave_up.load() && !wait_until([&] { return worker_threw.load(); }, 20000)) gave_up.store(true);
        });
      } catch (const std::bad_alloc&) {
        got = true;
      }
      expect(got && worker_threw.load(), "pool bad_alloc d'un ouvrier relancee dans l'appelant");
      scenarios += got && worker_threw.load();
    }
    expect_pool_reusable(pool, 0, "pool reutilisable apres exception ouvrier");
    {
      const u64 n = 1000;
      std::vector<std::atomic<u32>> threw(n);
      std::atomic<u64> started{0};
      bool got = false, known = false;
      try {
        pool.parallel_for(n, 1, [&](u64 b, u64, unsigned id) {
          started.fetch_add(1);
          threw[b].store(1);
          throw ChunkError{b, id};
        });
      } catch (const ChunkError& e) {
        got = true;
        known = e.begin < n && threw[e.begin].load() == 1 && e.worker < threads;
      }
      expect(got && known, "pool une exception levee relancee");
      expect(started.load() >= 1 && started.load() <= threads, "pool chaque fil s'arrete a sa premiere exception");
    }
    expect_pool_reusable(pool, 0, "pool reutilisable apres exceptions multiples");
  }
  std::printf("pool_worker_exception scenarios %llu\n", static_cast<unsigned long long>(scenarios));
  if (scenarios < 3) failures += 1000;
}

// (c) Drapeau de region en sortie exceptionnelle (audit continu pool_head § 1). L'ouvrier a rendu sa tranche avant la
// levee : (c) est isole de (a). Pool d'avant : in_parallel_region() restait vrai dans l'appelant, et son parallel_for
// suivant passait en serie, compte comme imbrique. Puis une exception d'un parallel_for imbrique (execute en serie)
// traverse la tranche exterieure : exactement les tranches internes jusqu'a la levee, drapeau restaure.
void test_pool_flag_restored() {
  for (unsigned threads : {2u, 4u}) {
    sched::Pool pool(threads);
    std::atomic<u32> done{0};
    std::atomic<bool> caller_in{false};
    bool got = false;
    try {
      pool.parallel_for(threads, 1, [&](u64, u64, unsigned id) {
        if (id != 0) {  // aucune tranche d'ouvrier ne finit avant que l'appelant ait la sienne
          wait_until([&] { return caller_in.load(); }, 20000);
          done.fetch_add(1);
          return;
        }
        caller_in.store(true);
        wait_until([&] { return done.load() == threads - 1; }, 20000);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        throw std::runtime_error("drapeau");
      });
    } catch (const std::runtime_error&) {
      got = true;
    }
    expect(got && !sched::in_parallel_region(), "pool drapeau restaure apres exception de l'appelant");
    std::atomic<u32> in_region{0};
    pool.parallel_for(64, 1, [&](u64, u64, unsigned) { in_region.fetch_add(sched::in_parallel_region() ? 1 : 0); });
    expect(pool.nested_calls() == 0 && in_region.load() == 64, "pool appel suivant parallele, non compte imbrique");
    // exception dans un parallel_for imbrique
    const u64 before = pool.nested_calls();
    std::vector<std::atomic<u32>> ran(threads);
    std::atomic<u32> outer{0};
    bool got2 = false;
    try {
      pool.parallel_for(threads, 1, [&](u64 b, u64, unsigned) {
        outer.fetch_add(1);
        pool.parallel_for(100, 1, [&](u64 bb, u64, unsigned) {
          ran[b].fetch_add(1);
          if (bb == 5) throw ChunkError{bb, 0};
        });
      });
    } catch (const ChunkError& e) {
      got2 = e.begin == 5;
    }
    u32 rows = 0;
    bool exact = true;
    for (u32 i = 0; i < threads; ++i) {
      exact &= ran[i].load() == 0 || ran[i].load() == 6;
      rows += ran[i].load() == 6;
    }
    expect(got2 && exact && rows == outer.load() && pool.nested_calls() - before == outer.load(),
           "pool exception imbriquee : tranches internes jusqu'a la levee");
    expect_pool_reusable(pool, pool.nested_calls(), "pool reutilisable apres exception imbriquee");
  }
}

// (d) Plus aucune tranche apres la capture, et reutilisation. En serie (pool a un fil) : exactement les tranches
// jusqu'a la levee. A plusieurs fils, l'appelant leve quand tous les ouvriers sont entres, chaque tranche d'ouvrier
// dure 2 ms : au catch, toute tranche commencee est finie et plus rien ne s'execute ensuite ; les tranches commencees
// apres la levee restent peu nombreuses (au plus une par fil une fois la capture faite ; le seuil n / 2 ne serait
// franchi que si la capture tardait de n / 2 * 2 ms / (P - 1) au moins, alors qu'un pool sans annulation les execute
// toutes).
void test_pool_cancel_and_reuse() {
  {
    sched::Pool pool(1);
    u64 ran = 0;
    bool got = false;
    try {
      pool.parallel_for(100, 1, [&](u64 b, u64, unsigned) {
        ++ran;
        if (b == 5) throw ChunkError{b, 0};
      });
    } catch (const ChunkError& e) {
      got = e.begin == 5;
    }
    expect(got && ran == 6, "pool a un fil : aucune tranche apres la levee");
    expect_pool_reusable(pool, 0, "pool a un fil reutilisable apres exception");
  }
  for (unsigned threads : {2u, 4u}) {
    sched::Pool pool(threads);
    const u64 n = 4000;
    std::vector<std::atomic<u32>> seen(threads);
    std::atomic<u32> entered{0};
    std::atomic<bool> caller_in{false}, thrown{false};
    std::atomic<u64> started{0}, finished{0}, late{0};
    u64 started_at_catch = 0, finished_at_catch = 0;
    bool got = false;
    try {
      pool.parallel_for(n, 1, [&](u64 b, u64, unsigned id) {
        started.fetch_add(1);
        if (thrown.load()) late.fetch_add(1);
        if (id == 0) {
          caller_in.store(true);
          wait_until([&] { return entered.load() == threads - 1; }, 20000);
          thrown.store(true);
          throw ChunkError{b, id};
        }
        if (seen[id].exchange(1) == 0) entered.fetch_add(1);
        wait_until([&] { return caller_in.load(); }, 20000);  // l'appelant a toujours une tranche
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        finished.fetch_add(1);
      });
    } catch (const ChunkError& e) {
      got = e.worker == 0;
      started_at_catch = started.load();
      finished_at_catch = finished.load();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    std::printf("pool annulation %u fils : %llu tranches commencees apres la levee (n = %llu)\n", threads,
                static_cast<unsigned long long>(late.load()), static_cast<unsigned long long>(n));
    expect(got && entered.load() == threads - 1, "pool annulation : scenario");
    expect(started_at_catch == finished_at_catch + 1, "pool annulation : toute tranche commencee finie au catch");
    expect(started.load() == started_at_catch, "pool annulation : aucune tranche apres la relance");
    expect(late.load() <= n / 2, "pool annulation : distribution arretee apres l'exception");
    expect_pool_reusable(pool, 0, "pool reutilisable apres annulation");
  }
}

// ---- Reclamation des tranches pres de 2^64 (verificateur du tour 1, 29 septembre 2026) ----
// Contrat (sched/pool.hpp) : les tranches sont [k * grain, min(n, (k + 1) * grain)), chacune executee au plus une fois
// (exactement une fois sans exception), pour tous n et grain. Pool d'avant : fetch_add portait le compteur jusqu'a
// n + P * grain, qui repassait par 0 au-dela de 2^64 (une tranche executee deux fois), et la serie calculait b + grain,
// qui debordait aussi (fin avant le debut, puis boucle sans fin). Sondes du verificateur : wrap_grain.cpp (n = 10,
// grain = 2^63 : tranche 0 executee deux fois, 300 executions sur 300) et adv_pool.cpp g8 (n = 2^64 - 1, grain 1, toute
// tranche leve : une tranche executee deux fois, 1853 executions sur 2000).

struct WrapAnomaly {};  // tranche mal formee ou executee deux fois : arrete le fil (avant, le travail ne finissait pas)

// Registre des executions d'un travail : forme exacte de chaque tranche et nombre d'executions par tranche.
class ChunkLedger {
 public:
  ChunkLedger(u64 n, u64 grain) : n_(n), grain_(grain), runs_(kSlots) {}
  void record(u64 b, u64 e) {
    const bool shape = b % grain_ == 0 && b < n_ && e == b + std::min(grain_, n_ - b);
    const u64 k = b / grain_;
    if (!shape || k >= kSlots || runs_[k].fetch_add(1) != 0) {
      anomalies_.fetch_add(1);
      throw WrapAnomaly{};
    }
    executed_.fetch_add(1);
  }
  u64 anomalies() const { return anomalies_.load(); }
  u64 executed() const { return executed_.load(); }
  // toutes les tranches executees exactement une fois (travail sans exception d'au plus kSlots tranches)
  bool complete() const {
    const u64 chunks = n_ / grain_ + (n_ % grain_ != 0 ? 1 : 0);
    if (chunks > kSlots || anomalies() != 0) return false;
    for (u64 k = 0; k < kSlots; ++k)
      if (runs_[k].load() != (k < chunks ? 1u : 0u)) return false;
    return true;
  }

 private:
  static constexpr u64 kSlots = 64;
  u64 n_, grain_;
  std::vector<std::atomic<u32>> runs_;
  std::atomic<u64> anomalies_{0}, executed_{0};
};

void test_pool_claim_wrap() {
  const u64 top = ~u64{0};        // 2^64 - 1
  const u64 half = u64{1} << 63;  // 2^63
  u64 jobs = 0, multi = 0;
  auto report = [](const char* what, unsigned threads, u64 runs, u64 bad) {
    std::printf("pool_claim_wrap %s fils %u travaux %llu en_defaut %llu\n", what, threads,
                static_cast<unsigned long long>(runs), static_cast<unsigned long long>(bad));
  };
  // (1) sonde wrap_grain : n = 10, grain = 2^63, une seule tranche [0, 10). Elle dure 1 ms, le temps que les ouvriers
  // reclament ; pool d'avant : chacun la reprenait, le compteur passant de 2^63 + 2^63 a 0.
  for (unsigned threads : {2u, 4u, 8u}) {
    sched::Pool pool(threads);
    u64 bad = 0;
    for (int rep = 0; rep < 40; ++rep, ++jobs) {
      ChunkLedger L(10, half);
      try {
        pool.parallel_for(10, half, [&](u64 b, u64 e, unsigned) {
          L.record(b, e);
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        });
      } catch (const WrapAnomaly&) {
      }
      bad += L.complete() ? 0 : 1;
    }
    report("grain_2^63", threads, 40, bad);
    expect(bad == 0, "pool grain 2^63 : tranche unique executee une fois");
    expect_pool_reusable(pool, 0, "pool reutilisable apres grain 2^63");
  }
  // (2) derniere tranche au bord de 2^64, sans exception, en serie (pool a un fil) et en parallele. Pool d'avant : fin
  // b + grain debordee (fin avant le debut) et compteur repassant par 0, en serie comme en parallele.
  struct Case {
    u64 n, grain;
  };
  const Case cases[] = {{half + 10, half}, {top, u64{1} << 62}, {top, top - 1}, {top, half + 1}, {top, top}};
  for (unsigned threads : {1u, 2u, 4u, 8u}) {
    sched::Pool pool(threads);
    u64 bad = 0;
    for (const Case& c : cases)
      for (int rep = 0; rep < 10; ++rep, ++jobs) {
        ChunkLedger L(c.n, c.grain);
        try {
          pool.parallel_for(c.n, c.grain, [&](u64 b, u64 e, unsigned) {
            L.record(b, e);
            std::this_thread::sleep_for(std::chrono::microseconds(300));
          });
        } catch (const WrapAnomaly&) {
        }
        bad += L.complete() ? 0 : 1;
      }
    report("bord_2^64", threads, 10 * std::size(cases), bad);
    expect(bad == 0, "pool tranches au bord de 2^64 : chacune executee une fois");
    expect_pool_reusable(pool, 0, "pool reutilisable apres tranches au bord de 2^64");
  }
  // (3) sonde g8 : n = 2^64 - 1, grain 1, toute tranche leve, 8 fils. Chaque tranche attend au plus 20 ms qu'une
  // seconde ait commence, pour que plusieurs fils reclament. Exige : une exception de tranche recue, aucune tranche
  // executee deux fois, au plus une par fil. Pool d'avant : store(n) puis fetch_add faisait repasser le compteur par 0.
  {
    const unsigned threads = 8;
    sched::Pool pool(threads);
    u64 bad = 0;
    for (int rep = 0; rep < 200; ++rep, ++jobs) {
      ChunkLedger L(top, 1);
      std::vector<std::atomic<u32>> by_worker(threads);
      bool got = false;
      try {
        pool.parallel_for(top, 1, [&](u64 b, u64 e, unsigned id) {
          L.record(b, e);
          by_worker[id].fetch_add(1);
          wait_until([&] { return L.executed() >= 2; }, 20);
          throw ChunkError{b, id};
        });
      } catch (const ChunkError&) {
        got = true;
      } catch (const WrapAnomaly&) {
      }
      bool once = true;
      for (auto& w : by_worker) once &= w.load() <= 1;
      bad += got && L.anomalies() == 0 && L.executed() >= 1 && L.executed() <= threads && once ? 0 : 1;
      multi += L.executed() >= 2 ? 1 : 0;
    }
    report("g8_n=2^64-1", threads, 200, bad);
    expect(bad == 0, "pool n = 2^64 - 1, toute tranche leve : aucune tranche executee deux fois");
    expect_pool_reusable(pool, 0, "pool reutilisable apres n = 2^64 - 1");
  }
  std::printf("pool_claim_wrap travaux %llu dont g8 a plusieurs tranches %llu\n", static_cast<unsigned long long>(jobs),
              static_cast<unsigned long long>(multi));
  if (multi < 20) failures += 1000;  // la sonde g8 doit voir plusieurs fils reclamer, sinon rien n'y est eprouve
}

// ---- Participation des fils (verificateur du tour 2, 30 septembre 2026 ; raccord R2) ----
// Mutant MV3 du verificateur : « break » au lieu de « continue » quand l'echange compare echoue. Sur x86, l'echange
// n'echoue qu'en concurrence et son gagnant continue : toutes les tranches s'executent et les six portes du pool
// passaient, mais chaque fil qui perd une course quitte le travail (sonde du verificateur, 8 fils : 1, 1, 1, 6 et 3
// fils actifs). Sur LL/SC, un echec a tort de compare_exchange_weak chez le dernier fil actif laisserait des tranches
// non executees. Travail de R tours (grain 1) ; un tour : P tranches d'attente, puis une rafale de B tranches vides.
// Une tranche d'attente attend, bornee, que P fils distincts soient entres dans son tour ; les fils liberes ensemble
// (attente active) se disputent aussitot le compteur dans la rafale, ou les echanges echouent en concurrence. Pool
// correct : chaque tour est pris par les P fils (un fil qui attend ne reclame rien, et les P tranches d'attente
// precedent la rafale), chacun R tranches d'attente, aucune attente expiree, quel que soit l'ordonnancement. Un fil
// sorti apres un echange perdu laisse le tour suivant incomplet : son attente expire, et la premiere expiration libere
// les autres (echec en 20 s au plus par pool). Critere du plan de raccord (au moins P / 2 fils avec au moins 1 % des
// tranches), exige sur les tranches d'attente, ou il decoule du premier ; la part de chaque fil dans les rafales depend
// de l'ordonnancement : imprimee, non exigee.

// Attente active, cedee toutes les 1024 lectures : vrai des que count atteint target ; faux a l'expiration de `ms`
// millisecondes, ou aussitot si abort est leve.
bool wait_count(const std::atomic<u32>& count, u32 target, const std::atomic<bool>& abort, int ms) {
  const auto limit = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
  for (u32 spin = 1;; ++spin) {
    if (count.load() >= target) return true;
    if (abort.load()) return false;
    if (spin % 1024 == 0) {
      if (std::chrono::steady_clock::now() >= limit) return false;
      std::this_thread::yield();
    }
  }
}

void test_pool_participation() {
  for (unsigned P : {4u, 8u}) {
    sched::Pool pool(P);
    const u64 R = 64, B = 1024, span = P + B, n = R * span;
    std::vector<std::atomic<u32>> entered(R), mark(R * P);  // mark[r * P + id] : tranches d'attente du fil id, tour r
    std::vector<std::atomic<u64>> waits_by(P), burst_by(P);
    std::atomic<bool> abort{false};
    std::atomic<u32> expired{0}, strays{0};
    pool.parallel_for(n, 1, [&](u64 b, u64, unsigned id) {  // grain 1 : la tranche b, du tour b / span
      if (id >= P) {
        strays.fetch_add(1);
        return;
      }
      const u64 r = b / span;
      if (b % span >= P) {  // rafale : tranche vide
        burst_by[id].fetch_add(1, std::memory_order_relaxed);
        return;
      }
      mark[r * P + id].fetch_add(1);
      waits_by[id].fetch_add(1);
      entered[r].fetch_add(1);
      if (!wait_count(entered[r], P, abort, 20000)) {
        expired.fetch_add(1);
        abort.store(true);
      }
    });
    u64 incomplete = 0, lo = n, hi = 0, burst = 0, burst_lo = n, burst_hi = 0;
    unsigned active = 0;
    for (u64 r = 0; r < R; ++r) {
      unsigned once = 0;
      for (unsigned id = 0; id < P; ++id) once += mark[r * P + id].load() == 1 ? 1 : 0;
      incomplete += once == P ? 0 : 1;
    }
    for (unsigned id = 0; id < P; ++id) {
      const u64 c = waits_by[id].load(), f = burst_by[id].load();
      lo = std::min(lo, c);
      hi = std::max(hi, c);
      active += 100 * c >= R * P ? 1 : 0;  // au moins 1 % des tranches d'attente
      burst += f;
      burst_lo = std::min(burst_lo, f);
      burst_hi = std::max(burst_hi, f);
    }
    std::printf("pool_participation fils %u tours %llu tours_incomplets %llu attentes_expirees %u "
                "tranches_d_attente_par_fil %llu..%llu fils_a_1pc %u rafales %llu par_fil %llu..%llu\n",
                P, static_cast<unsigned long long>(R), static_cast<unsigned long long>(incomplete), expired.load(),
                static_cast<unsigned long long>(lo), static_cast<unsigned long long>(hi), active,
                static_cast<unsigned long long>(burst), static_cast<unsigned long long>(burst_lo),
                static_cast<unsigned long long>(burst_hi));
    expect(incomplete == 0 && expired.load() == 0 && strays.load() == 0 && lo == R && hi == R,
           "pool participation : chaque tour pris par les P fils, R tranches d'attente par fil");
    expect(burst == R * B, "pool participation : chaque tranche de rafale executee une fois");
    expect(2 * active >= P, "pool participation : au moins P / 2 fils avec au moins 1 % des tranches d'attente");
    expect_pool_reusable(pool, 0, "pool reutilisable apres le travail par tours");
  }
}

// ---- sorties des sondes (core/cli_output.hpp ; raccord R2, prealable P2) ----
// Dossier prive sous TMPDIR (ou /tmp), vide puis retire a la fin (fichiers, liens et sous-dossiers d'un niveau).
struct ScratchDir {
  std::string path;
  ScratchDir() {
    const char* base = std::getenv("TMPDIR");
    std::string tmpl = std::string(base != nullptr && *base != '\0' ? base : "/tmp") + "/mhgp10_unit_sorties_XXXXXX";
    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');
    if (::mkdtemp(buf.data()) != nullptr) path = buf.data();
  }
  ~ScratchDir() {
    if (path.empty()) return;
    for (const std::string& n : list(path)) {
      const std::string q = path + "/" + n;
      struct stat st {};
      if (::lstat(q.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        for (const std::string& m : list(q)) ::unlink((q + "/" + m).c_str());
        ::rmdir(q.c_str());
      } else {
        ::unlink(q.c_str());
      }
    }
    ::rmdir(path.c_str());
  }
  std::string at(const std::string& name) const { return path + "/" + name; }
  static std::vector<std::string> list(const std::string& dir) {
    std::vector<std::string> out;
    if (DIR* d = ::opendir(dir.c_str())) {
      while (const dirent* e = ::readdir(d))
        if (std::string(e->d_name) != "." && std::string(e->d_name) != "..") out.push_back(e->d_name);
      ::closedir(d);
    }
    std::sort(out.begin(), out.end());
    return out;
  }
};

bool put_file(const std::string& p, const std::string& text) {
  std::FILE* f = std::fopen(p.c_str(), "wb");
  if (f == nullptr) return false;
  const bool wrote = std::fwrite(text.data(), 1, text.size(), f) == text.size();
  return std::fclose(f) == 0 && wrote;
}

std::string get_file(const std::string& p) {  // contenu, ou « ABSENT »
  std::FILE* f = std::fopen(p.c_str(), "rb");
  if (f == nullptr) return "ABSENT";
  std::string text;
  char chunk[4096];
  size_t got;
  while ((got = std::fread(chunk, 1, sizeof chunk, f)) > 0) text.append(chunk, got);
  std::fclose(f);
  return text;
}

int open_descriptors() {
  int n = 0;
  if (DIR* d = ::opendir("/proc/self/fd")) {
    while (const dirent* e = ::readdir(d))
      if (std::string(e->d_name) != "." && std::string(e->d_name) != "..") ++n;
    ::closedir(d);
  }
  return n;
}

// Contrat de cli::OutputSet (en-tete de core/cli_output.hpp) : conflits refuses avant toute reservation (meme chemin,
// ./ et .., lien symbolique, lien physique, entree), rien n'est cree ; destinations illisibles refusees
// output_unwritable ; un fichier preexistant n'est jamais tronque ni retire sans commit ; commit tout ou rien ; lien
// symbolique suivi, lien physique remplace sous le nom resolu seulement, droits repris ; sorties speciales ecrites
// directement ; garantie d'exception (aucun descripteur perdu, destination intacte, temporaire retire) ; publication
// defaite par rollback ou par la destruction sans release (reparation du raccord R2).
void test_output_set() {
  ScratchDir tmp;
  expect(!tmp.path.empty(), "sorties : dossier prive");
  if (tmp.path.empty()) return;
  u64 checks = 0;
  auto check = [&](bool ok, const char* what) {
    ++checks;
    expect(ok, what);
  };
  const Reason conflict = Reason::output_conflict, unwritable = Reason::output_unwritable;
  auto text = [](const char* t) { return [t](std::FILE* f) { std::fputs(t, f); }; };
  check(put_file(tmp.at("in"), "entree") && put_file(tmp.at("in2"), "entree2") && put_file(tmp.at("x_old"), "ancien") &&
            put_file(tmp.at("nondir"), "fichier") && ::mkdir(tmp.at("sub").c_str(), 0700) == 0 &&
            ::symlink("n4", tmp.at("l4").c_str()) == 0 && ::link(tmp.at("x_old").c_str(), tmp.at("h5").c_str()) == 0 &&
            ::symlink("in", tmp.at("lin").c_str()) == 0 && ::link(tmp.at("in").c_str(), tmp.at("hin").c_str()) == 0,
        "sorties : fixtures");
  const std::vector<std::string> baseline = ScratchDir::list(tmp.path);
  // (A) conflits : refus output_conflict avant toute reservation, rien n'est cree
  {
    cli::OutputSet o;
    check(o.declare(tmp.at("n1")).ok() && o.declare(tmp.at("n1")).reason == conflict, "sorties : meme chemin");
    check(o.declare(tmp.at("n2")).ok() && o.declare(tmp.path + "/./n2").reason == conflict, "sorties : alias ./");
    check(o.declare(tmp.at("n3")).ok() && o.declare(tmp.path + "/sub/../n3").reason == conflict, "sorties : alias ..");
    check(o.declare(tmp.at("n4")).ok() && o.declare(tmp.at("l4")).reason == conflict,
          "sorties : alias par lien symbolique (cible absente)");
    check(o.declare(tmp.at("x_old")).ok() && o.declare(tmp.at("h5")).reason == conflict,
          "sorties : alias par lien physique");
  }
  {
    cli::OutputSet o;
    check(o.declare(tmp.at("l4")).ok() && o.declare(tmp.at("n4")).reason == conflict,
          "sorties : alias par lien symbolique, ordre inverse");
  }
  {
    cli::OutputSet o;
    check(o.add_input(tmp.at("in")).ok(), "sorties : entree enregistree");
    check(o.declare(tmp.at("in")).reason == conflict, "sorties : sortie = entree");
    check(o.declare(tmp.path + "/./in").reason == conflict, "sorties : sortie = entree par ./");
    check(o.declare(tmp.at("lin")).reason == conflict, "sorties : sortie = entree par lien symbolique");
    check(o.declare(tmp.at("hin")).reason == conflict, "sorties : sortie = entree par lien physique");
    check(o.declare(tmp.at("autre")).ok(), "sorties : sortie distincte de l'entree admise");
    check(o.declare(tmp.at("in2")).ok() && o.add_input(tmp.at("in2")).reason == conflict,
          "sorties : entree enregistree apres la sortie qui la designe");
  }
  check(ScratchDir::list(tmp.path) == baseline && get_file(tmp.at("in")) == "entree", "sorties : conflits, rien de cree");
  // (B) destinations impossibles : output_unwritable, rien de cree
  {
    cli::OutputSet o;
    check(o.declare("").reason == unwritable, "sorties : chemin vide");
    check(o.declare(tmp.at("absent/x")).reason == unwritable, "sorties : dossier absent");
    check(o.declare(tmp.at("sub")).reason == unwritable, "sorties : destination dossier");
    check(o.declare(tmp.at("nondir/x")).reason == unwritable, "sorties : composant non dossier");
    check(o.declare(tmp.path + "/").reason == unwritable, "sorties : chemin de dossier");
  }
  if (::geteuid() != 0) {  // la racine ecrit partout : cas sans objet
    check(put_file(tmp.at("ro"), "lecture seule") && ::chmod(tmp.at("ro").c_str(), 0444) == 0, "sorties : fixture ro");
    {
      cli::OutputSet o;
      check(o.declare(tmp.at("ro")).reason == unwritable, "sorties : fichier sans droit d'ecriture jamais remplace");
    }
    check(get_file(tmp.at("ro")) == "lecture seule", "sorties : fichier sans droit d'ecriture intact");
    ::unlink(tmp.at("ro").c_str());
  }
  check(ScratchDir::list(tmp.path) == baseline, "sorties : destinations impossibles, rien de cree");
  // (C) publication : jamais sans commit ; commit par renommage ; droits, liens symboliques et physiques
  {
    cli::OutputSet o;
    check(o.declare(tmp.at("x_old")).ok() && o.reserve().ok(), "sorties : reservation");
    check(ScratchDir::list(tmp.path).size() == baseline.size() + 1 && get_file(tmp.at("x_old")) == "ancien",
          "sorties : un temporaire, destination intacte apres reservation");
    check(o.write(tmp.at("x_old"), text("nouveau")).ok() && get_file(tmp.at("x_old")) == "ancien",
          "sorties : destination intacte apres ecriture");
  }
  check(get_file(tmp.at("x_old")) == "ancien" && get_file(tmp.at("h5")) == "ancien" &&
            ScratchDir::list(tmp.path) == baseline,
        "sorties : sans commit, fichier preexistant intact et temporaire retire");
  check(put_file(tmp.at("x14"), "ancien") && ::chmod(tmp.at("x14").c_str(), 0640) == 0, "sorties : fixture droits");
  {
    cli::OutputSet o;
    check(o.declare(tmp.at("x14")).ok() && o.reserve().ok() && o.write(tmp.at("x14"), text("nouveau")).ok() &&
              o.commit().ok(),
          "sorties : commit");
    o.release();
  }
  struct stat st14 {};
  check(get_file(tmp.at("x14")) == "nouveau" && ::stat(tmp.at("x14").c_str(), &st14) == 0 &&
            (st14.st_mode & 07777) == 0640,
        "sorties : publie par commit, droits repris");
  check(put_file(tmp.at("t15"), "ancien") && ::symlink("t15", tmp.at("l15").c_str()) == 0 &&
            ::symlink("t16", tmp.at("l16").c_str()) == 0 && put_file(tmp.at("x17"), "ancien") &&
            ::link(tmp.at("x17").c_str(), tmp.at("h17").c_str()) == 0,
        "sorties : fixtures liens");
  {
    cli::OutputSet o;
    check(o.declare(tmp.at("l15")).ok() && o.declare(tmp.at("l16")).ok() && o.declare(tmp.at("x17")).ok() &&
              o.reserve().ok() && o.write(tmp.at("l15"), text("nouveau15")).ok() &&
              o.write(tmp.at("l16"), text("nouveau16")).ok() && o.write(tmp.at("x17"), text("nouveau17")).ok() &&
              o.commit().ok(),
          "sorties : commit a travers des liens");
    o.release();
  }
  char target[64] = {};
  const ssize_t tn = ::readlink(tmp.at("l15").c_str(), target, sizeof target - 1);
  check(get_file(tmp.at("t15")) == "nouveau15" && tn == 3 && std::string(target) == "t15",
        "sorties : lien symbolique suivi, la cible recoit la sortie, le lien reste");
  check(get_file(tmp.at("t16")) == "nouveau16" && get_file(tmp.at("l16")) == "nouveau16",
        "sorties : lien symbolique pendant, cible creee");
  check(get_file(tmp.at("x17")) == "nouveau17" && get_file(tmp.at("h17")) == "ancien",
        "sorties : lien physique, seul le nom resolu est remplace");
  // (D) sorties speciales : ecrites directement, chaque erreur controlee
  {
    cli::OutputSet o;
    check(o.declare("/dev/null").ok() && o.reserve().ok() && o.write("/dev/null", text("x")).ok() && o.commit().ok(),
          "sorties : /dev/null");
    o.release();
  }
  {
    cli::OutputSet o;
    check(o.declare("/dev/full").ok() && o.reserve().ok() && o.write("/dev/full", text("x")).reason == unwritable,
          "sorties : /dev/full, petite sortie (fclose)");
    const std::string big(8000, 'e');
    check(o.write("/dev/full", [&](std::FILE* f) { std::fwrite(big.data(), 1, big.size(), f); }).reason == unwritable,
          "sorties : /dev/full, 8 000 octets (ferror)");
  }
  // (E) tout ou rien
  {
    cli::OutputSet o;
    check(o.declare(tmp.at("x_old")).ok() && o.declare("/dev/full").ok() && o.reserve().ok() &&
              o.write(tmp.at("x_old"), text("nouveau")).ok() && o.write("/dev/full", text("x")).reason == unwritable,
          "sorties : seconde sortie refusee");
  }
  check(get_file(tmp.at("x_old")) == "ancien", "sorties : refus a la seconde sortie, premiere destination intacte");
  {
    cli::OutputSet o;
    check(o.declare(tmp.at("a22")).ok() && o.declare(tmp.at("b22")).ok() && o.reserve().ok() &&
              o.write(tmp.at("a22"), text("a")).ok() && o.commit().reason == unwritable,
          "sorties : commit refuse si une sortie n'est pas ecrite");
  }
  check(get_file(tmp.at("a22")) == "ABSENT" && get_file(tmp.at("b22")) == "ABSENT",
        "sorties : commit refuse, rien de publie");
  {
    cli::OutputSet o;
    check(o.declare(tmp.at("a23")).ok() && o.declare(tmp.at("v23"), true).ok() && o.reserve().ok() &&
              o.write(tmp.at("a23"), text("a")).ok() && o.commit().ok(),
          "sorties : optionnelle non ecrite admise");
    check(get_file(tmp.at("a23")) == "a" && get_file(tmp.at("v23")) == "ABSENT", "sorties : optionnelle absente");
    o.release();
  }
  {
    cli::OutputSet o;
    check(o.declare(tmp.at("a24")).ok() && o.declare(tmp.at("v24"), true).ok() && o.reserve().ok() &&
              o.write(tmp.at("v24"), text("v")).ok() && o.write(tmp.at("a24"), text("a")).ok() && o.commit().ok(),
          "sorties : optionnelle ecrite");
    check(get_file(tmp.at("a24")) == "a" && get_file(tmp.at("v24")) == "v", "sorties : optionnelle publiee");
    check(o.write(tmp.at("jamais"), text("j")).reason == unwritable && get_file(tmp.at("jamais")) == "ABSENT",
          "sorties : sortie non declaree jamais ecrite");
    o.release();
  }
  // (F) garantie d'exception : aucun descripteur perdu, destination intacte, temporaire retire
  const std::vector<std::string> before_f = ScratchDir::list(tmp.path);
  const int fds = open_descriptors();
  bool caught = false;
  {
    cli::OutputSet o;
    check(o.declare(tmp.at("x_old")).ok() && o.reserve().ok(), "sorties : reservation avant exception");
    try {
      (void)o.write(tmp.at("x_old"), [](std::FILE* f) {
        std::fputs("partiel", f);
        throw std::runtime_error("ecrivain qui leve");
      });
    } catch (const std::runtime_error&) {
      caught = true;
    }
    check(caught && open_descriptors() == fds, "sorties : ecrivain qui leve, descripteur ferme");
  }
  check(get_file(tmp.at("x_old")) == "ancien" && ScratchDir::list(tmp.path) == before_f,
        "sorties : ecrivain qui leve, destination intacte, temporaire retire");
  {
    cli::OutputSet o;
    check(o.declare(tmp.at("x_old")).ok() && o.reserve().ok() &&
              o.write(tmp.at("x_old"), [](std::FILE*) { throw std::bad_alloc(); }).reason == Reason::memory_budget &&
              open_descriptors() == fds,
          "sorties : ecrivain sans memoire, memory_budget, descripteur ferme");
  }
  check(get_file(tmp.at("x_old")) == "ancien" && ScratchDir::list(tmp.path) == before_f && open_descriptors() == fds,
        "sorties : ecrivain sans memoire, destination intacte, temporaire retire");
  // (G) publication defaite (reparation du raccord R2) : commit publie et garde une sauvegarde de chaque original ;
  // release la retire ; rollback, ou la destruction sans release, restaure l'original (le meme inode) et retire une
  // sortie nouvelle. Les echecs de rename et de link sont joues par mhgp10_fault output_commit.
  check(put_file(tmp.at("g1"), "ancien") && ::unlink(tmp.at("g2").c_str()) != 0, "sorties : fixtures de publication");
  struct stat before_g {}, after_g {};
  const bool stat_g = ::stat(tmp.at("g1").c_str(), &before_g) == 0;
  const std::vector<std::string> before_list = ScratchDir::list(tmp.path);
  {
    cli::OutputSet o;
    check(o.declare(tmp.at("g1")).ok() && o.declare(tmp.at("g2")).ok() && o.reserve().ok() &&
              o.write(tmp.at("g1"), text("nouveau1")).ok() && o.write(tmp.at("g2"), text("nouveau2")).ok() &&
              o.commit().ok() && get_file(tmp.at("g1")) == "nouveau1" && get_file(tmp.at("g2")) == "nouveau2" &&
              ScratchDir::list(tmp.path).size() == before_list.size() + 2,
          "sorties : commit publie et garde une sauvegarde de l'original");
    check(o.rollback().ok() && !o.recovery_needed() && o.commit().reason == unwritable,
          "sorties : rollback, puis commit refuse");
  }
  check(stat_g && get_file(tmp.at("g1")) == "ancien" && ::stat(tmp.at("g1").c_str(), &after_g) == 0 &&
            after_g.st_ino == before_g.st_ino && get_file(tmp.at("g2")) == "ABSENT" &&
            ScratchDir::list(tmp.path) == before_list,
        "sorties : rollback restaure l'original (meme inode), retire la sortie nouvelle, ne laisse rien");
  {
    cli::OutputSet o;
    check(o.declare(tmp.at("g1")).ok() && o.declare(tmp.at("g2")).ok() && o.reserve().ok() &&
              o.write(tmp.at("g1"), text("nouveau1")).ok() && o.write(tmp.at("g2"), text("nouveau2")).ok() &&
              o.commit().ok(),
          "sorties : commit sans release");
  }
  check(get_file(tmp.at("g1")) == "ancien" && ::stat(tmp.at("g1").c_str(), &after_g) == 0 &&
            after_g.st_ino == before_g.st_ino && get_file(tmp.at("g2")) == "ABSENT" &&
            ScratchDir::list(tmp.path) == before_list,
        "sorties : sans release, le destructeur defait la publication");
  {
    cli::OutputSet o;
    check(o.declare(tmp.at("g1")).ok() && o.declare(tmp.at("g2")).ok() && o.reserve().ok() &&
              o.write(tmp.at("g1"), text("nouveau1")).ok() && o.write(tmp.at("g2"), text("nouveau2")).ok() &&
              o.commit().ok() && o.commit().ok(),
          "sorties : commit idempotent");
    o.release();
    check(o.rollback().ok() && get_file(tmp.at("g1")) == "nouveau1", "sorties : rollback sans effet apres release");
  }
  check(get_file(tmp.at("g1")) == "nouveau1" && get_file(tmp.at("g2")) == "nouveau2" &&
            ScratchDir::list(tmp.path).size() == before_list.size() + 1,
        "sorties : release rend la publication definitive, aucune sauvegarde laissee");
  std::printf("sorties controles %llu\n", static_cast<unsigned long long>(checks));
  if (checks < (::geteuid() != 0 ? 62u : 59u)) failures += 1000;  // plancher : chaque controle ci-dessus a tourne
}

void test_status_and_buffer() {
  expect(status_of(Reason::none) == Status::ok, "statut none");
  expect(status_of(Reason::coordinate_out_of_domain) == Status::invalid_input, "statut entree");
  expect(status_of(Reason::shell_quotient_budget) == Status::unsupported_degeneracy, "statut degenerescence");
  expect(status_of(Reason::multiplicity_unsupported) == Status::unsupported_degeneracy, "statut multiplicite");
  expect(fail(Reason::root_count, 2).precedes(fail(Reason::arith_guard, 3)), "priorite plus petit K");
  MemoryBudget budget(1000);
  Buffer<u64> a;
  expect(a.allocate(100, budget), "budget 800 o");
  Buffer<u64> b;
  expect(!b.allocate(100, budget), "budget refuse");
  a.reset();
  expect(budget.used() == 0 && budget.peak() == 800, "budget libere");
  Csr<u32> c;
  expect(c.off.allocate(3) && c.val.allocate(2), "csr alloc");
  c.off[0] = 0;
  c.off[1] = 1;
  c.off[2] = 2;
  expect(c.well_formed() && c.row(1).size() == 1, "csr");
}

// Nuage de controle des frontieres : 40 points aleatoires u18 (graine 20260929), coordonnees multiples de 1024 (etendue
// u18 : l'explosion d'une feuille trop petite croit avec elle).
Result<Cloud> frontier_cloud() {
  std::mt19937_64 g(20260929);
  const u32 n = 40;
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) {
    x[i] = u32(g() % 256) * 1024;
    y[i] = u32(g() % 256) * 1024;
    z[i] = u32(g() % 256) * 1024;
    pid[i] = i;
  }
  return prepare_cloud(x, y, z, pid, 18);
}

// Deux catalogues egaux champ par champ (boules, rangs, supports, poids, drapeaux, populations, niveaux exacts).
bool same_catalogue(const Catalogue& A, const Catalogue& B) {
  bool same = A.balls() == B.balls() && A.level.size() == B.level.size();
  for (u32 i = 0; same && i < A.balls(); ++i) {
    same = A.rank[i] == B.rank[i] && A.support[i] == B.support[i] && A.qmin[i] == B.qmin[i] && A.p[i] == B.p[i] &&
           A.u[i] == B.u[i] && A.flags[i] == B.flags[i] && A.n_interior[i] == B.n_interior[i] &&
           A.pop_off[i + 1] == B.pop_off[i + 1];
    for (u64 q = A.pop_off[i]; same && q < A.pop_off[i + 1]; ++q) same = A.pop[q] == B.pop[q];
  }
  for (size_t r = 0; same && r < A.level.size(); ++r) same = geom::compare(A.level[r], B.level[r]) == 0;
  return same;
}

// Frontieres d'entree (audits du 29 septembre 2026) : garde des indices de boules jugee a des cardinaux artificiels
// (aucune boule allouee), taille d'un fichier u32le, feuille M < K + 3 refusee avant calcul par build_catalogue, et
// catalogue identique a M = K + 3 et a la taille par defaut (le catalogue ne depend pas de M).
void test_input_frontiers() {
  for (u64 b : {u64(0), u64(1), u64(kNone) - 1}) expect(check_ball_count(b).ok(), "garde boules : cardinal admis");
  for (u64 b : {u64(kNone), u64(1) << 32, u64(1) << 40, ~u64(0)}) {
    const Outcome o = check_ball_count(b);
    expect(o.reason == Reason::index_overflow_u32 && o.status() == Status::resource_exhausted,
           "garde boules : refus index_overflow_u32");
  }
  for (u64 r = 1; r < 12; ++r)
    expect(check_u32le_size(48 + r).outcome().reason == Reason::size_mismatch, "u32le : fin incomplete refusee");
  expect(check_u32le_size(48).ok() && check_u32le_size(48).value() == 4, "u32le : quatre points");
  expect(check_u32le_size(0).ok() && check_u32le_size(0).value() == 0, "u32le : vide (juge par prepare_cloud)");
  expect(check_u32le_size(12 * (u64(kNone) - 1)).ok(), "u32le : kNone - 1 points admis");
  expect(check_u32le_size(12 * u64(kNone)).outcome().reason == Reason::index_overflow_u32, "u32le : kNone points");
  auto cloud = frontier_cloud();
  expect(cloud.ok(), "frontieres : nuage");
  if (!cloud.ok()) return;
  sched::Pool pool(1);
  const int K = 5;
  for (u32 leaf : {1u, 2u, u32(K) - 1, u32(K), u32(K) + kLeafMargin - 1}) {
    CatalogueParams p;
    p.kmax = K;
    p.leaf_size = leaf;
    const auto r = build_catalogue(cloud.value(), p, pool);
    expect(!r.ok() && r.outcome().reason == Reason::parameter_out_of_range, "feuille M < K + 3 refusee");
  }
  CatalogueParams small, dflt;
  small.kmax = dflt.kmax = K;
  small.leaf_size = u32(K) + kLeafMargin;
  const auto a = build_catalogue(cloud.value(), small, pool), b = build_catalogue(cloud.value(), dflt, pool);
  expect(a.ok() && b.ok(), "feuille M = K + 3 et defaut admises");
  if (!a.ok() || !b.ok()) return;
  const Catalogue &A = a.value(), &B = b.value();
  std::printf("frontieres balls %u levels %zu nodes %llu/%llu\n", A.balls(), A.level.size(),
              (unsigned long long)A.ledger.nodes, (unsigned long long)B.ledger.nodes);
  expect(A.balls() > 0 && A.ledger.nodes > B.ledger.nodes && same_catalogue(A, B),  // l'arbre differe vraiment
         "feuille M = K + 3 : meme catalogue que la taille par defaut");
}

// Parseur strict commun des sondes (core/cli_options.hpp, audits du 29 septembre 2026) : jeton entier, chiffres
// decimaux, signe seulement si permis, borne du type cible avant conversion, listes sans element vide. Puis les deux
// lecteurs uniques de la tete (head.hpp, raccord R2) : --z par parse_z et domaine de validate(ClusterParams) ; fichiers
// --configs au format exact par read_configs (raisons, liste vide sur refus, plafond de 1 Mio).
void test_cli_parser() {
  using cli::parse_integer;
  const u32 u32max = std::numeric_limits<u32>::max();
  const u64 u64max = std::numeric_limits<u64>::max();
  const int imax = std::numeric_limits<int>::max(), imin = std::numeric_limits<int>::min();
  // valeurs de bord admises
  expect(parse_integer<u32>("0", 0, u32max).value() == 0, "parse : 0");
  expect(parse_integer<u32>("4294967295", 0, u32max).value() == u32max, "parse : u32 max");
  expect(parse_integer<u64>("18446744073709551615", 0, u64max).value() == u64max, "parse : u64 max");
  expect(parse_integer<int>("2147483647", 0, imax).value() == imax, "parse : int max");
  expect(parse_integer<int>("-2147483648", imin, imax).value() == imin, "parse : int min (signe permis)");
  expect(parse_integer<int>("007", 0, imax).value() == 7, "parse : zeros de tete, base 10");
  expect(parse_integer<unsigned>("1024", 0, cli::kMaxThreads).value() == 1024, "parse : --threads max");
  // refus : suffixe, prefixe, signe interdit, base implicite, espaces, depassement du type ou de la borne
  for (const char* bad : {"2not_an_integer", "5x", "+2", "0x2", "-1", "-0", "", " 5", "5 ", "1e3", "5.0", "4294967296",
                          "4294967304", "99999999999999999999999", "\xd9\xa5"})
    expect(!parse_integer<u32>(bad, 0, u32max).ok() &&
               parse_integer<u32>(bad, 0, u32max).outcome().reason == Reason::parameter_out_of_range,
           "parse : entier u32 refuse");
  expect(!parse_integer<u64>("18446744073709551616", 0, u64max).ok(), "parse : u64 max + 1");
  expect(!parse_integer<int>("2147483648", 0, imax).ok(), "parse : int max + 1");
  expect(!parse_integer<int>("-2147483649", imin, imax).ok(), "parse : int min - 1");
  expect(!parse_integer<unsigned>("1025", 0, cli::kMaxThreads).ok(), "parse : --threads au-dela du plafond");
  expect(!parse_integer<unsigned>("4294967297", 0, cli::kMaxThreads).ok(), "parse : --threads = 2^32 + 1");
  expect(!parse_integer<int>("0", 1, imax).ok() && parse_integer<int>("1", 1, imax).ok(), "parse : borne basse");
  // listes
  const auto l = cli::parse_integer_list<int>("1,2,10", 0, imax);
  expect(l.ok() && l.value() == std::vector<int>{1, 2, 10}, "parse : liste");
  for (const char* bad : {"", ",2", "2,", "2,,3", "2,x", "2, 3", "2,-1"})
    expect(!cli::parse_integer_list<int>(bad, 0, imax).ok(), "parse : liste refusee");
  // aucune option sans effet (reparation du raccord R2) : forme d'option, nom, option repetee
  {
    char a0[] = "sonde", a1[] = "in", a2[] = "--k=2", a3[] = "--selection=eom", a4[] = "--allow-single", a5[] = "--k=3",
         a6[] = "--selection=leaf", a7[] = "--allow-single", a8[] = "--kk=1";
    char* args[] = {a0, a1, a2, a3, a4, a5, a6, a7, a8};
    expect(cli::looks_like_option("--k=5") && cli::looks_like_option("--") && !cli::looks_like_option("-k") &&
               !cli::looks_like_option("fichier") && !cli::looks_like_option(""),
           "options : forme");
    expect(cli::option_name("--k=5") == "--k" && cli::option_name("--allow-single") == "--allow-single" &&
               cli::option_name("--z=1=2") == "--z" && cli::option_name("") == "",
           "options : nom");
    expect(!cli::repeated_option(2, 2, args) && !cli::repeated_option(2, 3, args) && !cli::repeated_option(2, 4, args) &&
               cli::repeated_option(2, 5, args) && cli::repeated_option(2, 6, args) && cli::repeated_option(2, 7, args) &&
               !cli::repeated_option(2, 8, args),
           "options : repetition par nom, valeur comprise ou non");
  }
  // --z : lecteur unique parse_z de la tete (raccord R2, prealable P3), syntaxe decimale stricte ; repr Python compris
  // (1e-05, 1e+16) ; le domaine (0 < z <= 16) est juge ensuite par validate(ClusterParams)
  for (const char* good : {"1", "1.0", "2.5", "0.5", ".5", "5.", "1e-05", "1e+16", "3E2", "0", "16"}) {
    double z = -1;
    expect(parse_z(good, z), "parse_z : syntaxe admise");
  }
  double z25 = 0, z5 = 0;
  expect(parse_z("2.5", z25) && z25 == 2.5 && parse_z("1e-05", z5) && z5 == 1e-05, "parse_z : meme double que std::stod");
  for (const char* bad : {"inf", "nan", "-1", "+1", "1e", "e5", ".", "", " 1", "1 ", "1.5x", "0x1p3", "infinity", "1,5",
                          "--1", "-0"}) {
    double z = -1;
    expect(!parse_z(bad, z) && z == -1, "parse_z : syntaxe refusee, z inchange");
  }
  auto domain = [](const char* token) {  // syntaxe admise, puis domaine de validate(ClusterParams)
    ClusterParams p;
    return parse_z(token, p.z) && validate(p).ok();
  };
  expect(domain("1e-05") && domain("16") && domain("1e-320"), "z : bornes et sous-normal dans le domaine");
  for (const char* out : {"0", "0.0", "17", "16.000001", "1e400", "1e-400"})
    expect(!domain(out), "z : hors du domaine (0, 16] refuse par validate");
  ClusterParams mcs0;
  mcs0.min_cluster_size = 0;
  expect(validate(mcs0).reason == Reason::parameter_out_of_range, "mcs = 0 refuse par validate");
  // --configs : lecteur unique read_configs de la tete (raccord R2 : cli::parse_head_configs retire). Fichier entier,
  // une configuration validee par ligne ; illisible : input_unreadable ; hors format, hors domaine, vide, plus de
  // 1 Mio : parameter_out_of_range ; jamais un prefixe (liste vide sur refus).
  ScratchDir tmp;
  expect(!tmp.path.empty(), "configs : dossier prive");
  std::vector<ClusterParams> list;
  expect(put_file(tmp.at("ok"), "39 1.0 eom 0\n39 2.5 leaf 1\n\n5 1e-05 eom 0") &&
             read_configs(tmp.at("ok"), list).ok() && list.size() == 3 && list[1].selection == Selection::leaf &&
             list[1].allow_single_cluster && list[1].z == 2.5 && list[2].z == 1e-05 && list[0].min_cluster_size == 39,
         "configs : trois configurations");
  int bad_files = 0;
  for (const char* bad : {"", "\n\n", "39 1.0 eom", "39 1.0 eom 0 7", "39 1.0 xyz 0", "39 1.0 eom 2", "-5 1.0 eom 0",
                          "39 nan eom 0", "39 1.0 eom 0\n39 2.5 leaf", "0 1.0 eom 0", "39 0 eom 0", "39 17 eom 0",
                          "39 1.0 eom 0\n39 1.0 eom 0\v"}) {
    const std::string name = "bad" + std::to_string(bad_files++);
    list.assign(2, ClusterParams{});
    const Outcome o = put_file(tmp.at(name), bad) ? read_configs(tmp.at(name), list) : fail(Reason::none);
    expect(o.reason == Reason::parameter_out_of_range && list.empty(), "configs : contenu refuse, liste vide");
  }
  list.assign(2, ClusterParams{});
  expect(read_configs(tmp.at("absent"), list).reason == Reason::input_unreadable && list.empty(),
         "configs : fichier absent, input_unreadable");
  expect(read_configs(tmp.path, list).reason == Reason::input_unreadable && list.empty(),
         "configs : dossier, input_unreadable");
  // plafond de 1 Mio (R01, verificateur du second tour) : 2^20 octets admis, 2^20 + 1 refuses
  std::string ceiling = "3 1 eom 0\n";
  ceiling.resize(size_t(1) << 20, '\n');
  expect(put_file(tmp.at("mio"), ceiling) && read_configs(tmp.at("mio"), list).ok() && list.size() == 1,
         "configs : 1 Mio exactement admis");
  ceiling.push_back('\n');
  expect(put_file(tmp.at("mio1"), ceiling) && read_configs(tmp.at("mio1"), list).reason ==
                                                 Reason::parameter_out_of_range && list.empty(),
         "configs : 1 Mio + 1 octet refuse");
}

// Parametres du generateur (check_catalogue_params) : regle produit M >= K + 3 ; diagnostic allow_small_leaf admet
// K <= M < K + 3 et exige toujours un budget de noeuds ; M < K toujours refuse ; max_leaf dans [1, kMaxLeafBound] et
// feuille effective <= max_leaf ; ball_limit dans [1, kNone].
void test_catalogue_params() {
  auto with = [](int K, u32 M, bool small, u64 nodes) {
    CatalogueParams p;
    p.kmax = K;
    p.leaf_size = M;
    p.allow_small_leaf = small;
    p.max_nodes = nodes;
    return check_catalogue_params(p);
  };
  const Reason out = Reason::parameter_out_of_range;
  expect(with(5, 0, false, 0).ok() && with(5, 8, false, 0).ok(), "params : defaut et M = K + 3");
  expect(with(5, 7, false, 0).reason == out && with(5, 5, false, 0).reason == out, "params : M < K + 3 refuse");
  expect(with(5, 5, true, 0).reason == out, "params : petite feuille sans budget refusee");
  // raccord R2 : le diagnostic exige un budget a toute feuille (--allow-small-leaf --max-nodes=0 etait admis, inerte)
  expect(with(5, 0, true, 0).reason == out && with(5, 8, true, 0).reason == out,
         "params : diagnostic sans budget refuse, feuille par defaut ou M >= K + 3");
  expect(with(5, 0, true, 1000).ok() && with(5, 8, true, 1000).ok(), "params : diagnostic avec budget, grande feuille");
  expect(with(5, 5, true, 1000).ok() && with(5, 7, true, 1000).ok(), "params : K <= M < K + 3 avec budget");
  expect(with(10, 11, true, 1000).ok() && with(2, 2, true, 1000).ok(), "params : feuilles des instruments d'audit");
  expect(with(5, 4, true, 1000).reason == out && with(5, 3, true, 1000).reason == out, "params : M < K refuse");
  expect(with(5, 0, false, 5).ok(), "params : budget en mode produit");
  expect(with(0, 0, false, 0).reason == Reason::kmax_out_of_range &&
             with(13, 0, false, 0).reason == Reason::kmax_out_of_range,
         "params : kmax hors domaine");
  CatalogueParams p;
  p.kmax = 5;
  p.leaf_size = 300;
  expect(check_catalogue_params(p).reason == out, "params : feuille > max_leaf refusee avant calcul");
  p.max_leaf = kMaxLeafBound;
  p.leaf_size = kMaxLeafBound;
  expect(check_catalogue_params(p).ok(), "params : feuille = max_leaf = borne");
  p.max_leaf = kMaxLeafBound + 1;
  expect(check_catalogue_params(p).reason == out, "params : max_leaf au-dela de la borne");
  p.max_leaf = 0;
  p.leaf_size = 0;
  expect(check_catalogue_params(p).reason == out, "params : max_leaf nul");
  p.max_leaf = 10;  // table par defaut : M(5) = 16 > 10
  expect(check_catalogue_params(p).reason == out, "params : feuille par defaut > max_leaf");
  CatalogueParams q;
  q.ball_limit = 0;
  expect(check_catalogue_params(q).reason == out, "params : ball_limit nul");
  q.ball_limit = u64(kNone) + 1;
  expect(check_catalogue_params(q).reason == out, "params : ball_limit au-dela de kNone");
  q.ball_limit = 1;
  expect(check_catalogue_params(q).ok(), "params : ball_limit = 1");
}

// Garde des indices de boules rendue causale : a travers build_catalogue, ball_limit = B (nombre de boules du nuage)
// refuse index_overflow_u32 avant l'assemblage, ball_limit = B + 1 rend le meme catalogue que le defaut. Retirer
// l'appel de check_ball_count dans build_catalogue fait echouer ce controle.
void test_ball_guard() {
  auto cloud = frontier_cloud();
  expect(cloud.ok(), "garde boules : nuage");
  if (!cloud.ok()) return;
  sched::Pool pool(2);
  CatalogueParams p;
  p.kmax = 5;
  const auto ref = build_catalogue(cloud.value(), p, pool);
  expect(ref.ok() && ref.value().balls() > 1, "garde boules : catalogue de reference");
  if (!ref.ok()) return;
  const u32 B = ref.value().balls();
  p.ball_limit = B;
  const auto at = build_catalogue(cloud.value(), p, pool);
  expect(!at.ok() && at.outcome().reason == Reason::index_overflow_u32 &&
             at.outcome().status() == Status::resource_exhausted,
         "garde boules : ball_limit = B refuse index_overflow_u32");
  p.ball_limit = u64(B) + 1;
  const auto above = build_catalogue(cloud.value(), p, pool);
  expect(above.ok() && same_catalogue(above.value(), ref.value()), "garde boules : ball_limit = B + 1 admis");
  std::printf("garde_boules B %u\n", B);
}

// Budget de noeuds (diagnostic, tout mode) : refus node_budget si et seulement si l'arbre complet compte plus de
// max_nodes noeuds, au meme seuil a 1 et 4 fils (l'arbre ne depend pas du nombre de fils) ; admis, le catalogue est le
// meme que sans budget. Feuille explosive (M = K sur le nuage etendu) refusee par le budget au lieu de tourner sans fin.
void test_node_budget() {
  auto cloud = frontier_cloud();
  expect(cloud.ok(), "budget : nuage");
  if (!cloud.ok()) return;
  sched::Pool one(1), four(4);
  for (u32 M : {0u, 7u}) {  // table par defaut (mode produit) ; K + 2 (diagnostic)
    CatalogueParams p;
    p.kmax = 5;
    p.leaf_size = M;
    p.allow_small_leaf = M != 0;
    p.max_nodes = 100000000;  // budget large pour mesurer l'arbre en mode diagnostic
    const auto ref = build_catalogue(cloud.value(), p, one);
    expect(ref.ok(), "budget : arbre de reference");
    if (!ref.ok()) return;
    const u64 N = ref.value().ledger.nodes;
    for (sched::Pool* pool : {&one, &four}) {
      p.max_nodes = N;
      const auto at = build_catalogue(cloud.value(), p, *pool);
      expect(at.ok() && at.value().ledger.nodes == N && same_catalogue(at.value(), ref.value()),
             "budget : max_nodes = noeuds de l'arbre admis, meme catalogue");
      p.max_nodes = N - 1;
      const auto below = build_catalogue(cloud.value(), p, *pool);
      expect(!below.ok() && below.outcome().reason == Reason::node_budget &&
                 below.outcome().status() == Status::resource_exhausted,
             "budget : max_nodes = noeuds - 1 refuse node_budget");
    }
    std::printf("budget M %u noeuds %llu\n", M, (unsigned long long)N);
  }
  CatalogueParams boom;  // M = K : explosion avec l'etendue (audit leaf_stress)
  boom.kmax = 5;
  boom.leaf_size = 5;
  boom.allow_small_leaf = true;
  boom.max_nodes = 200000;
  for (sched::Pool* pool : {&one, &four}) {
    const auto r = build_catalogue(cloud.value(), boom, *pool);
    expect(!r.ok() && r.outcome().reason == Reason::node_budget, "budget : feuille M = K refusee par le budget");
  }
}

void test_cloud() {
  std::mt19937_64 g(7);
  const u32 n = 5000;
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) {
    x[i] = u32(g() % 64);  // grille grossiere : doublons nombreux
    y[i] = u32(g() % 64);
    z[i] = u32(g() % 8);
    pid[i] = 1000000u + 7u * i;
  }
  auto a = prepare_cloud(x, y, z, pid, 18);
  expect(a.ok(), "cloud ok");
  // permutation de l'entree : memes sites, memes poids, memes listes de PointId
  std::vector<u32> perm(n);
  for (u32 i = 0; i < n; ++i) perm[i] = i;
  std::shuffle(perm.begin(), perm.end(), g);
  std::vector<u32> x2(n), y2(n), z2(n), p2(n);
  for (u32 i = 0; i < n; ++i) {
    x2[i] = x[perm[i]];
    y2[i] = y[perm[i]];
    z2[i] = z[perm[i]];
    p2[i] = pid[perm[i]];
  }
  auto b = prepare_cloud(x2, y2, z2, p2, 18);
  expect(b.ok(), "cloud permute ok");
  const Cloud& A = a.value();
  const Cloud& B = b.value();
  bool same = A.sites() == B.sites() && A.weight == n;
  u64 wsum = 0;
  for (u32 s = 0; same && s < A.sites(); ++s) {
    same &= A.x[s] == B.x[s] && A.y[s] == B.y[s] && A.z[s] == B.z[s] && A.w[s] == B.w[s];
    same &= A.ids.row(s).size() == A.w[s];
    for (u32 t = 0; same && t < A.w[s]; ++t) same &= A.ids.row(s)[t] == B.ids.row(s)[t];
    if (s > 0) same &= morton3(A.x[s - 1], A.y[s - 1], A.z[s - 1]) < morton3(A.x[s], A.y[s], A.z[s]);
    wsum += A.w[s];
  }
  expect(same && wsum == n && A.ids.well_formed(), "cloud permutation / multiplicites");
  expect(A.sites() < n, "cloud doublons regroupes");
  // refus
  std::vector<u32> bad = x;
  bad[3] = 1u << 18;
  expect(prepare_cloud(bad, y, z, pid, 18).outcome().reason == Reason::coordinate_out_of_domain, "cloud domaine");
  std::vector<u32> dup = pid;
  dup[5] = dup[6];
  expect(prepare_cloud(x, y, z, dup, 18).outcome().reason == Reason::duplicate_point_id, "cloud pid double");
  expect(prepare_cloud({}, {}, {}, {}, 18).outcome().reason == Reason::empty_input, "cloud vide");
}

void test_site_tree() {
  std::mt19937_64 g(11);
  u64 checks = 0;
  for (int t = 0; t < 30; ++t) {
    const u32 n = 50 + u32(g() % 1500);
    const u32 span = (t % 3 == 0) ? 16 : 5000;  // grille grossiere (doublons) ou fine
    std::vector<u32> x(n), y(n), z(n), pid(n);
    for (u32 i = 0; i < n; ++i) {
      x[i] = u32(g() % span);
      y[i] = u32(g() % span);
      z[i] = u32(g() % span);
      pid[i] = i;
    }
    auto r = prepare_cloud(x, y, z, pid, 18);
    if (!r.ok()) { expect(false, "tree cloud"); continue; }
    const Cloud& c = r.value();
    SiteTree tree(c);
    std::vector<u32> got;
    for (int q = 0; q < 40; ++q) {
      const i64 qx = i64(g() % span), qy = i64(g() % span), qz = i64(g() % span);
      std::vector<std::pair<u64, u32>> all;
      for (u32 s = 0; s < c.sites(); ++s) {
        const i64 dx = i64(c.x[s]) - qx, dy = i64(c.y[s]) - qy, dz = i64(c.z[s]) - qz;
        all.push_back({u64(dx * dx + dy * dy + dz * dz), c.w[s]});
      }
      std::sort(all.begin(), all.end());
      for (u64 k : {1ull, 2ull, 5ull, 11ull}) {
        u64 acc = 0, want = ~u64{0};
        for (auto& [d, w] : all) {
          acc += w;
          if (acc >= k) { want = d; break; }
        }
        expect(tree.kth_distance(qx, qy, qz, k) == want, "kth_distance");
        ++checks;
      }
      const u64 r2 = all[std::min<size_t>(all.size() - 1, 7)].first;
      tree.within(qx, qy, qz, r2, got);
      u32 cnt = 0;
      for (auto& [d, w] : all) cnt += d <= r2;
      expect(got.size() == cnt, "within");
      ++checks;
    }
  }
  std::printf("site_tree_checks %llu\n", static_cast<unsigned long long>(checks));
  if (checks < 5000) failures += 1000;
}


// Requetes a centre rationnel de SiteTree (nearest, closed_ball) contre la force brute exacte : centres de spheres
// passant par 1 a 4 sites (centre entier, formes q2, q3, q4 ; ancre sur la sphere), sur grilles grossieres
// (cospherite, ex aequo), moyennes et u18. La cle exacte decide ; l'arbre (elagage flottant a marge, bande exacte
// de la boule fermee) ne doit rien perdre ni rien ajouter. Un centre hors du cube u18 passe par le repli exact ; les
// centres lointains (constat G1) sont juges par la porte mhgp10_regression_site_tree_far_center.
void test_site_tree_rational() {
  std::mt19937_64 g(23);
  u64 checks = 0, shells = 0;
  for (int t = 0; t < 24; ++t) {
    const u32 n = 40 + u32(g() % 1200);
    const u32 span = (t % 3 == 0) ? 12 : ((t % 3 == 1) ? 400 : 250000);
    std::vector<u32> x(n), y(n), z(n), pid(n);
    for (u32 i = 0; i < n; ++i) {
      x[i] = u32(g() % span);
      y[i] = u32(g() % span);
      z[i] = u32(g() % span);
      pid[i] = i;
    }
    auto r = prepare_cloud(x, y, z, pid, 18);
    if (!r.ok()) {
      expect(false, "tree cloud");
      continue;
    }
    const Cloud& c = r.value();
    SiteTree tree(c);
    auto P = [&](u32 s) { return geom::P3{i64(c.x[s]), i64(c.y[s]), i64(c.z[s])}; };
    std::vector<std::pair<i128, u32>> got, want;
    std::vector<u32> gi, gu, wi, wu;
    for (int q = 0; q < 60; ++q) {
      const int arity = 1 + int(g() % 4);
      u32 s[4];
      for (int a = 0; a < 4; ++a) s[a] = u32(g() % c.sites());
      const geom::P3 anchor = P(s[0]);
      geom::Center ctr{{0, 0, 0}, 1};
      bool ok = true;
      if (arity == 2) geom::center2(P(s[0]), P(s[1]), ctr);
      else if (arity == 3) ok = geom::center3(P(s[0]), P(s[1]), P(s[2]), ctr);
      else if (arity == 4) ok = geom::center4(P(s[0]), P(s[1]), P(s[2]), P(s[3]), ctr);
      if (!ok) continue;
      want.clear();
      wi.clear();
      wu.clear();
      for (u32 zz = 0; zz < c.sites(); ++zz) {
        const i128 key = geom::side_key(ctr, anchor, P(zz));
        want.push_back({key, zz});
        if (key < 0) wi.push_back(zz);
        else if (key == 0) wu.push_back(zz);
      }
      std::sort(want.begin(), want.end());
      for (u32 count : {1u, 3u, 7u, 12u}) {
        tree.nearest(anchor, ctr, count, got);
        const size_t m = std::min<size_t>(count, want.size());
        bool same = got.size() == m;
        for (size_t i = 0; same && i < m; ++i) same = got[i] == want[i];
        expect(same, "nearest rationnel");
        ++checks;
      }
      tree.closed_ball(anchor, ctr, gi, gu);
      expect(gi == wi && gu == wu, "closed_ball rationnel");
      shells += wu.size() > 1;
      ++checks;
    }
  }
  std::printf("site_tree_rational_checks %llu shells %llu\n", static_cast<unsigned long long>(checks),
              static_cast<unsigned long long>(shells));
  if (checks < 4000 || shells < 300) failures += 1000;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc > 2) return 2;
  if (argc == 2) {
    const std::string group = argv[1];
    if (group == "pool_caller_exception") test_pool_caller_exception();
    else if (group == "pool_worker_exception") test_pool_worker_exception();
    else if (group == "pool_flag_restored") test_pool_flag_restored();
    else if (group == "pool_cancel_and_reuse") test_pool_cancel_and_reuse();
    else if (group == "pool_claim_wrap") test_pool_claim_wrap();
    else if (group == "pool_short_jobs") test_pool_short_jobs();
    else if (group == "pool_participation") test_pool_participation();
    else {
      std::printf("groupe inconnu %s\n", group.c_str());
      return 2;
    }
  } else {
    test_input_frontiers();
    test_cli_parser();
    test_catalogue_params();
    test_ball_guard();
    test_node_budget();
    test_output_set();
    test_site_tree();
    test_site_tree_rational();
    test_cloud();
    test_wide();
    test_pool();
    test_pool_short_jobs();
    test_pool_caller_exception();
    test_pool_worker_exception();
    test_pool_flag_restored();
    test_pool_cancel_and_reuse();
    test_pool_claim_wrap();
    test_pool_participation();
    test_status_and_buffer();
  }
  if (failures) {
    std::printf("unit_failures %d\n", failures);
    return failures >= 1000 ? 3 : 1;
  }
  std::printf("unit_ok\n");
  return 0;
}
