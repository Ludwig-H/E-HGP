// Portes unitaires des fondations : entiers larges (juge a arithmetique decimale independante),
// ordonnanceur (couverture exacte, determinisme, imbrication serialisee, exceptions), statuts et tampons.
//   mhgp10_unit            : toutes les portes
//   mhgp10_unit GROUPE     : un seul groupe du pool, chacun une porte CTest separee : exceptions
//                            (pool_caller_exception, pool_worker_exception, pool_flag_restored, pool_cancel_and_reuse),
//                            tranches pres de 2^64 (pool_claim_wrap), travaux courts enchaines (pool_short_jobs)
// Codes : 0 conforme, 1 desaccord d'un juge, 2 groupe inconnu, 3 plancher non atteint.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <functional>
#include <new>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "arith/wide.hpp"
#include "cloud/cloud.hpp"
#include "cloud/site_tree.hpp"
#include "core/buffer.hpp"
#include "core/status.hpp"
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
// de la boule fermee) ne doit rien perdre ni rien ajouter.
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
    else {
      std::printf("groupe inconnu %s\n", group.c_str());
      return 2;
    }
  } else {
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
    test_status_and_buffer();
  }
  if (failures) {
    std::printf("unit_failures %d\n", failures);
    return failures >= 1000 ? 3 : 1;
  }
  std::printf("unit_ok\n");
  return 0;
}
