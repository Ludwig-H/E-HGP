// MorseHGP3D v9 — porte du pool persistant de la tour (v9 E2, parallel/pool.hpp).
//
// --unit : le TaskPool seul, sous PoolScope.
//   - run_threads appelle fn(0) sur le fil proprietaire et fn(t) au plus une
//     fois par t dans [1, count), chaque t sur un seul fil (une fois chacun
//     quand le proprietaire les attend) ; parallel_items, _ranges et
//     parallel_sort servis par le pool (compteur de travaux) avec les memes
//     sorties que sans pool, pour 2, 3, 4 et 8 participants ;
//   - retour seulement apres la sortie de tous les fils entres (travail lent
//     sur un ouvrier, jamais un retour anticipe) ; un fil reveille apres la
//     fermeture d'un travail ne l'execute jamais (crochet de retard) ;
//   - ouvriers engages MESURES : run_threads rend 1 + les fils entres dans le
//     travail (exactement `count` quand le proprietaire les attend, 1 + les
//     fils entres quand un fil en retard saute le travail), jamais le nombre
//     de participants declare ;
//   - premiere exception capturee, relancee sur le proprietaire, pool
//     reutilisable ensuite ; une exception qui echappe a fn sur le
//     proprietaire ET sur un fil du pool (run_threads direct, sans
//     enveloppe) : la premiere capturee est relancee dans les deux ordres
//     forces (crochet de compte des captures), et le travail suivant ne
//     releve aucune exception perimee ;
//   - appels imbriques (part du proprietaire, ouvriers du pool) et appels
//     d'un fil tiers : jamais sur le pool, jamais d'interblocage (chien de
//     garde) ;
//   - environnement flottant du proprietaire installe a chaque travail ;
//   - echec de lancement (MHGP9_TESTING) : le pool partiellement cree n'est
//     jamais publie, tous ses fils sont joints ; meme contrat sur la voie par
//     appel (sans pool), dont le mutant d'execution
//     `parallel-admit-partial-launch` (--inject=) est tue.
// --fixtures (cible avec la chaine) : sur le nuage de trois grappes des
//   portes de chaine (1 500 sites), K5 et K8, la tour construite depuis le
//   meme catalogue est identique (condense, tower_work et compteurs
//   deterministes) pool actif et coupe, a 1, 2, 3, 4 et 8 fils, phase A
//   recouvrante ou non ; option de chaine (defaut actif) ; echec de
//   lancement de la tour = refus de ressource, pool actif ou coupe ; plus
//   les cas de --priority.
// --priority (cible avec la tour seule) : une entree refusee par les
//   controles seriels du domaine (K = 0, K = 11, index invalide), avec
//   l'echec de lancement du premier fil du pool, reste une entree invalide
//   (full_ball_input_domain) pool actif ou coupe, comme avant E2 ; temoin :
//   un K valide avec le meme echec est le refus de ressource, pool actif.
// --frame=CHEMIN --k=K --tower=HEX --catalogue=HEX : meme identite sur une
//   trame LiDAR (chaine a 8 fils, condenses epingles), temps entrelaces
//   pool actif / coupe publies a titre indicatif ; nombre EXACT de travaux
//   du pool a chaque W > 1 et dans la chaine (08/000000 : 45 a K5, 80 a
//   K10, sous le regroupement hache de la phase 0 ; 58 et 108 avec le tri).
//
// Planchers (code 3) : travaux servis par le pool (le meme nombre a chaque
// W > 1 d'un meme catalogue), fils du pool = W - 1, moins de fils crees par
// la tour avec le pool que sans, requetes statiques.
// Code 0 conforme, 1 desaccord (ligne `cause=`), 2 argument, 3 plancher.
#include <algorithm>
#include <atomic>
#include <cfenv>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "../../src/tower/parallel/pool.hpp"
#if defined(MHGP9_POOL_GATE_CHAIN)
#include "../../src/chain/tower_chain.hpp"
#endif
#if defined(MHGP9_POOL_GATE_CHAIN) || defined(MHGP9_POOL_GATE_TOWER)
#include "../../src/tower/forest/full_ball_tower.hpp"
#include "../../src/tower/tree/cloud_index.hpp"
#endif

#if !defined(MHGP9_TESTING)
#error This gate needs the MHGP9_TESTING launch hooks
#endif

namespace {

using mhgp9::tower::parallel_detail::PoolScope;
using mhgp9::tower::parallel_detail::TaskPool;
namespace pd = mhgp9::tower::parallel_detail;

struct Fail {
  int code;
  std::string cause;
};
[[noreturn]] void fail(const std::string& cause, int code = 1) { throw Fail{code, cause}; }
// Some mutants leave pool threads inside a job that borrows this frame:
// report and leave at once, never unwind under them.
[[noreturn]] void fail_now(const std::string& cause) {
  std::printf("cause=%s\n", cause.c_str());
  std::fflush(stdout);
  std::_Exit(1);
}

// Runs `body` on its own thread; a body still running after `seconds` is a
// deadlock (reported, then _Exit: the stuck threads are never joined).
template <typename Body>
void with_watchdog(const char* cause, int seconds, Body&& body) {
  std::mutex mu;
  std::condition_variable done_cv;
  bool done = false;
  std::exception_ptr error;
  std::thread runner([&] {
    try { body(); } catch (...) { error = std::current_exception(); }
    { std::lock_guard<std::mutex> lock(mu); done = true; }
    done_cv.notify_all();
  });
  {
    std::unique_lock<std::mutex> lock(mu);
    if (!done_cv.wait_for(lock, std::chrono::seconds(seconds), [&] { return done; })) fail_now(cause);
  }
  runner.join();
  if (error) std::rethrow_exception(error);
}

struct UnitCounts {
  std::uint64_t checks = 0, pool_jobs = 0, worker_items = 0, nested_calls = 0, late_skips = 0, error_jobs = 0;
};

void unit_dispatch(UnitCounts& counts) {
  for (const std::size_t participants : {std::size_t{2}, std::size_t{3}, std::size_t{4}, std::size_t{8}}) {
    TaskPool pool(participants);
    PoolScope scope(&pool);
    if (pool.participants() != participants || pool.threads() != participants - 1)
      fail("pool.participants=" + std::to_string(pool.participants()));
    for (std::size_t count = 2; count <= participants; ++count) {
      // run_threads: every t exactly once, t = 0 on the owner.
      // The owner waits for every other index: each then runs exactly once.
      std::vector<std::thread::id> who(count);
      std::vector<std::atomic<int>> hits(count);
      std::atomic<std::size_t> finished{0}, started{0};
      const std::uint64_t before = pool.jobs();
      const std::size_t engaged = pd::run_threads(count, [&](std::size_t t) {
        if (t < count) { who[t] = std::this_thread::get_id(); hits[t].fetch_add(1); }
        if (t == 0) {
          const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(20);
          while (started.load() + 1 < count && std::chrono::steady_clock::now() < limit) std::this_thread::yield();
        } else started.fetch_add(1);
        finished.fetch_add(1);
      });
      ++counts.checks;
      if (finished.load() != count) fail_now("pool.returned_before_join finished=" + std::to_string(finished.load()));
      if (pool.jobs() != before + 1) fail("pool.bypassed count=" + std::to_string(count));
      if (who[0] != std::this_thread::get_id()) fail("pool.owner_not_worker_zero");
      for (std::size_t t = 0; t < count; ++t) {
        if (hits[t].load() != 1) fail("pool.index_hits t=" + std::to_string(t));
        for (std::size_t u = 0; u < t; ++u)
          if (who[u] == who[t]) fail("pool.index_shared_thread");
      }
      // Every index ran once: exactly `count` workers engaged.
      if (engaged != count)
        fail("pool.engaged_count count=" + std::to_string(count) + " engaged=" + std::to_string(engaged));
    }
    // The helpers on the pool: same results as without it.
    const int threads = static_cast<int>(participants);
    const std::size_t n = 20000;
    std::vector<std::atomic<std::uint32_t>> seen(n);
    std::vector<std::atomic<bool>> worked(participants);
    std::atomic<std::uint64_t> by_workers{0};
    std::uint64_t before = pool.jobs();
    const std::size_t engaged = mhgp9::tower::parallel_items(n, threads, [&](std::size_t i, std::size_t t) {
      if (t >= participants) fail_now("pool.worker_index_range");
      seen[i].fetch_add(1);
      worked[t].store(true);
      if (t != 0) by_workers.fetch_add(1);
    });
    ++counts.checks;
    // Measured: at least every worker that took an item, at most the pool.
    std::size_t working = 0;
    for (const auto& w : worked) working += w.load() ? 1 : 0;
    if (engaged < std::max<std::size_t>(working, 1) || engaged > participants || pool.jobs() != before + 1)
      fail("pool.items_not_served engaged=" + std::to_string(engaged) + " working=" + std::to_string(working));
    for (std::size_t i = 0; i < n; ++i)
      if (seen[i].load() != 1) fail("pool.items_coverage i=" + std::to_string(i));
    counts.worker_items += by_workers.load();
    std::vector<std::atomic<std::uint32_t>> covered(n);
    before = pool.jobs();
    mhgp9::tower::parallel_ranges(n, threads, [&](std::size_t b, std::size_t e, std::size_t) {
      for (std::size_t i = b; i < e; ++i) covered[i].fetch_add(1);
    });
    ++counts.checks;
    if (pool.jobs() != before + 1) fail("pool.ranges_not_served");
    for (std::size_t i = 0; i < n; ++i)
      if (covered[i].load() != 1) fail("pool.ranges_coverage");
    std::vector<std::uint64_t> values(200000), expected;
    std::uint64_t state = 0x2545f4914f6cdd1dull ^ participants;
    for (auto& v : values) { state = state * 6364136223846793005ull + 1442695040888963407ull; v = state >> 11; }
    for (std::size_t i = 0; i < values.size(); ++i) values[i] = (values[i] << 20) | i;  // strict total order
    expected = values;
    std::sort(expected.begin(), expected.end());
    before = pool.jobs();
    mhgp9::tower::parallel_sort(values, threads, std::less<std::uint64_t>());
    ++counts.checks;
    if (values != expected) fail("pool.sort_differs");
    if (pool.jobs() != before + 3) fail("pool.sort_not_served jobs=" + std::to_string(pool.jobs() - before));
    counts.pool_jobs += pool.jobs();
  }
}

void unit_join(UnitCounts& counts) {
  // Worker 0 (the owner) waits until a pool thread holds an item; the pool
  // threads' items are slow. The call may return only once they are done.
  TaskPool pool(4);
  PoolScope scope(&pool);
  for (int round = 0; round < 3; ++round) {
    std::atomic<int> started{0}, done{0};
    const std::size_t n = 8;
    mhgp9::tower::parallel_items(n, 4, [&](std::size_t, std::size_t t) {
      if (t == 0) {
        const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(20);
        while (started.load() == 0 && std::chrono::steady_clock::now() < limit) std::this_thread::yield();
      } else {
        started.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
      }
      done.fetch_add(1);
    });
    ++counts.checks;
    if (started.load() == 0) fail("pool.no_worker_started");
    if (done.load() != static_cast<int>(n)) fail_now("pool.returned_before_join done=" + std::to_string(done.load()));
    counts.worker_items += static_cast<std::uint64_t>(started.load());
  }
}

void unit_exceptions(UnitCounts& counts) {
  TaskPool pool(4);
  PoolScope scope(&pool);
  // First exception, from any worker, rethrown on the owner after the join.
  for (const bool on_worker : {false, true}) {
    std::atomic<int> inside{0}, started{0};
    bool caught = false;
    try {
      mhgp9::tower::parallel_items(1000, 4, [&](std::size_t i, std::size_t t) {
        inside.fetch_add(1);
        struct Out { std::atomic<int>& v; ~Out() { v.fetch_sub(1); } } out{inside};
        if (t != 0) started.fetch_add(1);
        if (on_worker) {
          if (t == 0) {
            const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(20);
            while (started.load() == 0 && std::chrono::steady_clock::now() < limit) std::this_thread::yield();
          } else throw std::runtime_error("pool_gate_worker_throw");
        } else if (i == 500) throw std::runtime_error("pool_gate_worker_throw");
      });
    } catch (const std::runtime_error& e) {
      caught = std::string_view(e.what()) == "pool_gate_worker_throw";
    }
    ++counts.checks;
    if (!caught) fail("pool.exception_lost");
    if (inside.load() != 0) fail_now("pool.exception_before_join");
  }
  // The pool stays usable.
  std::atomic<int> sum{0};
  mhgp9::tower::parallel_items(100, 4, [&](std::size_t, std::size_t) { sum.fetch_add(1); });
  ++counts.checks;
  if (sum.load() != 100) fail("pool.unusable_after_exception");
}

// An exception escapes fn on the owner AND on a pool thread in the same job
// (run_threads called directly, no capturing wrapper). Both orders are
// forced with the capture counter: the first captured is rethrown, and the
// next job, whose fn never throws, must not rethrow the other one.
void unit_error_order(UnitCounts& counts) {
  TaskPool pool(2);
  PoolScope scope(&pool);
  const auto wait_for = [](const auto& ready) {
    const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    while (!ready() && std::chrono::steady_clock::now() < limit) std::this_thread::yield();
  };
  for (const bool worker_first : {true, false}) {
    const std::size_t base = pd::job_error_captures.load();
    const auto captured = [&] { return pd::job_error_captures.load() > base; };
    std::atomic<bool> entered{false};
    std::string thrown = "none";
    try {
      pd::run_threads(2, [&](std::size_t t) {
        if (t == 1) {
          entered.store(true);
          if (!worker_first) wait_for(captured);  // after the owner's capture
          throw std::runtime_error("pool_gate_worker");
        }
        if (worker_first) wait_for(captured);  // after the thread's capture
        else wait_for([&] { return entered.load(); });  // the thread is inside the job
        throw std::runtime_error("pool_gate_owner");
      });
    } catch (const std::runtime_error& e) {
      thrown = e.what();
    }
    std::string next = "none";
    try {
      pd::run_threads(2, [](std::size_t) {});
    } catch (const std::runtime_error& e) {
      next = e.what();
    }
    ++counts.checks;
    if (next != "none") fail("pool.stale_error thrown=" + thrown + " next=" + next);
    if (!entered.load() || pd::job_error_captures.load() != base + 2)
      fail("floor.error_order_not_exercised captures=" + std::to_string(pd::job_error_captures.load() - base), 3);
    const std::string expected = worker_first ? "pool_gate_worker" : "pool_gate_owner";
    if (thrown != expected) fail("pool.error_order thrown=" + thrown + " expected=" + expected);
    ++counts.error_jobs;
  }
}

void unit_nested(UnitCounts& counts) {
  with_watchdog("pool.nested_deadlock", 30, [&] {
    TaskPool pool(4);
    PoolScope scope(&pool);
    std::atomic<std::uint64_t> inner{0}, sorted_ok{0}, started{0};
    const std::uint64_t before = pool.jobs();
    mhgp9::tower::parallel_items(8, 4, [&](std::size_t i, std::size_t t) {
      if (t == 0) {  // make sure a pool thread nests too
        const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(20);
        while (started.load() == 0 && std::chrono::steady_clock::now() < limit) std::this_thread::yield();
      } else started.fetch_add(1);
      mhgp9::tower::parallel_items(64, 4, [&](std::size_t, std::size_t) { inner.fetch_add(1); });
      std::vector<std::uint32_t> v(50000);
      for (std::size_t j = 0; j < v.size(); ++j) v[j] = static_cast<std::uint32_t>((j * 2654435761u) ^ i);
      auto w = v;
      std::sort(w.begin(), w.end());
      w.erase(std::unique(w.begin(), w.end()), w.end());
      v = w;
      std::reverse(v.begin(), v.end());
      mhgp9::tower::parallel_sort(v, 4, std::less<std::uint32_t>());
      if (v == w) sorted_ok.fetch_add(1);
    });
    ++counts.checks;
    if (inner.load() != 8 * 64 || sorted_ok.load() != 8) fail("pool.nested_results");
    if (pool.jobs() != before + 1) fail("pool.nested_reached_pool jobs=" + std::to_string(pool.jobs() - before));
    counts.nested_calls += 16;
    // A foreign thread (a phase-A runner) never uses the owner's pool.
    std::atomic<int> foreign{0};
    std::thread other([&] {
      mhgp9::tower::parallel_items(64, 4, [&](std::size_t, std::size_t) { foreign.fetch_add(1); });
    });
    other.join();
    ++counts.checks;
    if (foreign.load() != 64 || pool.jobs() != before + 1) fail("pool.foreign_thread_used_pool");
  });
}

void unit_fenv(UnitCounts& counts) {
  const int initial = std::fegetround();
  TaskPool pool(4);  // created under the initial rounding mode
  PoolScope scope(&pool);
  for (const int mode : {FE_UPWARD, FE_TONEAREST, FE_DOWNWARD}) {
    if (std::fesetround(mode) != 0) fail("fenv.unsupported_mode", 2);
    std::vector<int> seen(4, -1);
    std::atomic<int> recorded{0};
    pd::run_threads(4, [&](std::size_t t) {
      seen[t] = std::fegetround();
      if (t != 0) { recorded.fetch_add(1); return; }
      const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(20);  // every index runs
      while (recorded.load() < 3 && std::chrono::steady_clock::now() < limit) std::this_thread::yield();
    });
    std::fesetround(initial);
    ++counts.checks;
    for (std::size_t t = 0; t < seen.size(); ++t)
      if (seen[t] != mode) fail("pool.stale_fenv t=" + std::to_string(t));
  }
}

// A pool thread woken late (join delay hook) finds the job closed and skips
// it. The job borrows only static storage: a thread that ran it after the
// owner returned would be counted, never read a dead frame.
std::atomic<int> late_calls{0}, late_after_return{0};
std::atomic<bool> late_returned{false};
struct LateJob {
  void operator()(std::size_t t) const {
    if (t == 0) return;  // the owner's share: nothing to drain
    late_calls.fetch_add(1);
    if (late_returned.load()) late_after_return.fetch_add(1);
  }
};
LateJob late_job;

void unit_late_join(UnitCounts& counts) {
  TaskPool pool(4);
  PoolScope scope(&pool);
  pd::join_delay_ms = 200;
  pd::join_delay_index = 3;
  int skipped = 0;
  for (int round = 0; round < 3; ++round) {
    late_calls = 0; late_after_return = 0; late_returned = false;
    const std::size_t engaged = pd::run_threads(4, late_job);
    const int joined = late_calls.load();  // final: every joined thread has left
    late_returned = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(400));  // the late thread has woken and decided
    ++counts.checks;
    if (late_after_return.load() != 0) fail_now("pool.joined_closed_job calls=" + std::to_string(late_calls.load()));
    // Engaged workers are measured: the owner and the threads that joined.
    if (engaged != 1 + static_cast<std::size_t>(joined))
      fail("pool.declared_workers engaged=" + std::to_string(engaged) + " joined=" + std::to_string(joined));
    if (late_calls.load() < 3) ++skipped;
  }
  pd::join_delay_index = 0;
  pd::join_delay_ms = 0;
  if (skipped == 0) fail("floor.late_join_not_exercised", 3);
  counts.late_skips += static_cast<std::uint64_t>(skipped);
}

void unit_launch(UnitCounts& counts) {
  // Persistent pool: creation fails at its second thread (one thread exists).
  for (const std::size_t participants : {std::size_t{2}, std::size_t{4}, std::size_t{8}}) {
    pd::launch_fail_after = participants == 2 ? 0 : 1;
    bool refused = false;
    try {
      TaskPool pool(participants);
      pd::launch_fail_after = static_cast<std::size_t>(-1);
      fail_now("pool.partial_launch_admitted participants=" + std::to_string(participants) +
               " threads=" + std::to_string(pool.threads()));
    } catch (const std::system_error&) {
      refused = true;
    }
    pd::launch_fail_after = static_cast<std::size_t>(-1);
    ++counts.checks;
    if (!refused) fail("pool.launch_failure_not_refused");
    if (pd::launch_active.load() != 0) fail("pool.launch_threads_not_joined");
  }
  // Per-call path (no pool): the partially launched threads never run fn.
  {
    std::atomic<int> calls{0};
    bool refused = false;
    pd::launch_fail_after = 1;
    try {
      mhgp9::tower::parallel_items(64, 4, [&](std::size_t, std::size_t) { calls.fetch_add(1); });
    } catch (const std::system_error&) {
      refused = true;
    }
    pd::launch_fail_after = static_cast<std::size_t>(-1);
    ++counts.checks;
    if (!refused) fail("legacy.launch_failure_not_refused");
    if (calls.load() != 0) fail("legacy.partial_launch_admitted calls=" + std::to_string(calls.load()));
    if (pd::launch_active.load() != 0) fail("legacy.launch_threads_not_joined");
  }
  if (pd::launch_active.load() != 0) fail("pool.threads_alive_after_destruction");
}

int run_unit() {
  UnitCounts counts;
  unit_join(counts);  // first: a pool that returns early would corrupt every later check
  unit_dispatch(counts);
  unit_exceptions(counts);
  unit_error_order(counts);
  unit_nested(counts);
  unit_fenv(counts);
  unit_late_join(counts);
  unit_launch(counts);
  std::printf("task_pool_gate unit checks=%llu pool_jobs=%llu worker_items=%llu nested_calls=%llu late_skips=%llu "
              "error_jobs=%llu\n",
              static_cast<unsigned long long>(counts.checks), static_cast<unsigned long long>(counts.pool_jobs),
              static_cast<unsigned long long>(counts.worker_items),
              static_cast<unsigned long long>(counts.nested_calls),
              static_cast<unsigned long long>(counts.late_skips), static_cast<unsigned long long>(counts.error_jobs));
  // 2, 3, 4 and 8 participants: 1 + 2 + 3 + 7 run_threads jobs and five
  // helper jobs each (items, ranges, the three steps of the sort).
  if (counts.pool_jobs < 33 || counts.worker_items == 0 || counts.nested_calls < 16 || counts.error_jobs != 2) {
    std::printf("cause=floor.unit\n");
    return 3;
  }
  return 0;
}

#if defined(MHGP9_POOL_GATE_CHAIN) || defined(MHGP9_POOL_GATE_TOWER)
// Failure priority across the pool's creation: an input refused by the
// serial domain checks is invalid input whatever the thread resources, pool
// on and off, since these checks run before any thread exists (as before
// E2). Control: a valid order with the same launch failure reaches the pool
// and is the resource refusal. Returns the number of towers judged.
std::uint64_t priority_fixtures() {
  using mhgp9::tower::FullBallStatus;
  std::vector<mhgp9::tower::InputPoint> input{{7, {100, 200, 300}}, {3, {4000, 50, 60}}, {9, {70, 8000, 90}}};
  const auto valid = mhgp9::tower::build_cloud_index(input);
  input[1].position.x = mhgp9::tower::kCoordMax + 1;  // outside the u18 profile: the index is invalid
  const auto outside = mhgp9::tower::build_cloud_index(input);
  if (!valid.valid || outside.valid) fail("priority.fixture_index");
  struct Case {
    const char* name;
    const mhgp9::tower::CloudIndex* ix;
    unsigned kmax;
  };
  std::uint64_t towers = 0;
  for (const Case& c : {Case{"k0", &valid, 0u}, Case{"k11", &valid, 11u}, Case{"invalid_index", &outside, 2u}})
    for (const bool pool : {true, false}) {
      pd::launch_fail_after = 1;
      const auto tw = mhgp9::tower::build_full_ball_tower(*c.ix, {}, c.kmax, 4, {}, true,
          mhgp9::tower::FullBallTowerOptions{.overlap_static = true, .persistent_pool = pool});
      pd::launch_fail_after = static_cast<std::size_t>(-1);
      ++towers;
      const std::string where = std::string(c.name) + " pool=" + (pool ? "1" : "0") + " status=" +
                                std::to_string(static_cast<int>(tw.status)) + " reason=" + tw.reason;
      if (tw.status != FullBallStatus::kInvalidInput || std::string_view(tw.reason) != "full_ball_input_domain")
        fail("tower.domain_after_launch " + where);
      if (!tw.orders.empty() || tw.stats.pool_threads != 0 || pd::launch_active.load() != 0)
        fail("tower.domain_refusal_state " + where);
    }
  {
    pd::launch_fail_after = 1;
    const auto tw = mhgp9::tower::build_full_ball_tower(valid, {}, 2, 4, {}, true,
        mhgp9::tower::FullBallTowerOptions{.overlap_static = true, .persistent_pool = true});
    pd::launch_fail_after = static_cast<std::size_t>(-1);
    ++towers;
    if (tw.status != FullBallStatus::kResourceExhausted ||
        std::string_view(tw.reason) != "full_ball_thread_launch_failed" || pd::launch_active.load() != 0)
      fail(std::string("floor.priority_control reason=") + tw.reason, 3);
  }
  return towers;
}

int run_priority() {
  const std::uint64_t towers = priority_fixtures();
  std::printf("task_pool_gate priority towers=%llu\n", static_cast<unsigned long long>(towers));
  return 0;
}
#endif

#if defined(MHGP9_POOL_GATE_CHAIN)
using mhgp9::tower::FullBallStats;
using mhgp9::tower::FullBallTowerResult;

// tower_work as the probe publishes it (same fields, same order).
std::vector<std::uint64_t> tower_work(const FullBallStats& s) {
  const auto& w = s.resolve_work;
  return {s.records, s.extra_records, s.representatives, s.anchor_hits, s.key_lookups, s.intruder_queries,
          s.intruder_nodes, w.calls, w.power_tests, s.births, s.merges, s.contributions, s.grouped_lots,
          s.resolver_cache_hits, w.pair_distances, w.materializations, w.supports_by_size[1], w.supports_by_size[2],
          w.supports_by_size[3], w.supports_by_size[4], w.proposals, w.verified_proposals,
          w.boundary_canonicalizations, w.proposal_fallbacks};
}
// Every other deterministic counter of the static path (never timings,
// worker counts or thread metadata).
std::vector<std::uint64_t> static_counters(const FullBallStats& s) {
  std::vector<std::uint64_t> v{s.anchor_blocks, s.regular_blocks, s.extra_blocks, s.same_radius_steps,
      s.descending_steps, s.max_chain_steps, s.inert_blocks, s.declared_support_checks, s.singleton_lots,
      s.lot_dsu_slots, s.lower_edges_indexed, s.lower_nodes_activated, s.lower_edges_activated, s.lower_queries,
      s.lower_find_steps, s.lower_path_writes, s.presorted_catalogues, s.validation_work.calls,
      s.validation_work.power_tests, s.validation_work.pair_distances, s.validation_work.materializations};
  for (const auto* a : {&s.static_requests, &s.static_unique, &s.static_seeded, &s.static_post_seed_queries,
                        &s.static_post_seed_hits, &s.static_post_seed_terminals})
    v.insert(v.end(), a->begin(), a->end());
  return v;
}

std::vector<mhgp9::gen::Point3> clusters(int n) {
  std::vector<mhgp9::gen::Point3> points;
  std::uint64_t state = 3;
  for (int i = 0; i < n; ++i) {
    const std::int32_t centre = (i % 3) * 40000 + 20000;
    std::int32_t c[3];
    for (int axis = 0; axis < 3; ++axis) {
      state = state * 6364136223846793005ull + 1442695040888963407ull;
      c[axis] = centre + static_cast<std::int32_t>((state >> 40) % 9000);
    }
    points.push_back({c[0], c[1], c[2]});
  }
  return points;
}

mhgp9::tower::CloudIndex index_of(const std::vector<mhgp9::gen::Point3>& points) {
  std::vector<mhgp9::tower::InputPoint> input(points.size());
  for (std::size_t i = 0; i < points.size(); ++i)
    input[i] = mhgp9::tower::InputPoint{static_cast<mhgp9::tower::PointId>(i),
                                        mhgp9::tower::P3{points[i].x, points[i].y, points[i].z}};
  return mhgp9::tower::build_cloud_index(input);
}

struct Identity {
  std::uint64_t towers = 0, pool_towers = 0, pool_jobs_min = ~0ull, pool_jobs_max = 0, max_requests = 0;
  std::uint64_t threads_on = 0, threads_off = 0;  // summed over the W > 1 towers
};

// The towers of one catalogue for W in {1, 2, 3, 4, 8}, pool on and off,
// overlapped phase A or not: the same digest, tower_work and counters.
// The reference is the chain's own tower (same catalogue, pool on).
void identity(const char* label, const std::vector<mhgp9::gen::Point3>& points, unsigned kmax,
              const std::vector<mhgp9::tower::BallData>& catalogue, const mhgp9::ChainResult& chain, Identity& id,
              bool both_overlaps, bool print_times) {
  const auto ix = index_of(points);
  const std::uint64_t digest = chain.tower_digest;
  const std::vector<std::uint64_t> work = tower_work(chain.tower_stats);
  std::vector<std::uint64_t> counters;
  for (const bool overlap : {true, false}) {
    if (!overlap && !both_overlaps) continue;
    // Every helper call with more than one planned worker is a pool job:
    // the same count at every W > 1 of one catalogue and path.
    std::uint64_t group_jobs = 0;
    for (const int w : {1, 2, 3, 4, 8})
      for (const bool pool : {true, false}) {
        const auto t0 = std::chrono::steady_clock::now();
        const auto tw = mhgp9::tower::build_full_ball_tower(ix, catalogue, kmax, w, {}, true,
            mhgp9::tower::FullBallTowerOptions{.overlap_static = overlap, .persistent_pool = pool});
        const double wall = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        const auto& s = tw.stats;
        const std::string where = std::string(label) + " W=" + std::to_string(w) + " pool=" + (pool ? "1" : "0") +
                                  " overlap=" + (overlap ? "1" : "0");
        if (tw.status != mhgp9::tower::FullBallStatus::kCompleteRelative) fail("tower.status " + where + " " + tw.reason);
        if (mhgp9::tower_digest(tw) != digest) fail("tower.digest " + where);
        if (tower_work(s) != work) fail("tower.work " + where);
        if (counters.empty()) counters = static_counters(s);
        else if (static_counters(s) != counters) fail("tower.counters " + where);
        ++id.towers;
        for (const auto r : s.static_requests) id.max_requests = std::max<std::uint64_t>(id.max_requests, r);
        const std::uint64_t created = s.pool_threads + s.helper_threads + s.runner_threads;
        if (pool && w > 1) {
          ++id.pool_towers;
          id.pool_jobs_min = std::min<std::uint64_t>(id.pool_jobs_min, s.pool_jobs);
          id.pool_jobs_max = std::max<std::uint64_t>(id.pool_jobs_max, s.pool_jobs);
          if (group_jobs == 0) group_jobs = s.pool_jobs;
          else if (s.pool_jobs != group_jobs)
            fail("tower.pool_jobs_vary " + where + " jobs=" + std::to_string(s.pool_jobs) + " first=" +
                 std::to_string(group_jobs));
          if (s.pool_threads != static_cast<std::uint64_t>(w - 1)) fail("floor.pool_threads " + where, 3);
          id.threads_on += created;
        } else {
          if (s.pool_threads != 0 || s.pool_jobs != 0) fail("tower.pool_used_when_off " + where);
          if (w > 1) id.threads_off += created;
        }
        if (print_times)
          std::printf("time %s wall_ms=%.1f pool_ms=%.2f validate_ms=%.1f static_ms=%.1f lots_ms=%.1f "
                      "images_ms=%.1f bank_ms=%.1f encode_ms=%.1f threads_created=%llu pool_jobs=%llu\n",
                      where.c_str(), wall, tw.times.pool_ms, tw.times.validate_ms, tw.times.static_ms,
                      tw.times.lots_ms, tw.times.images_ms, tw.times.bank_ms, tw.times.encode_ms,
                      static_cast<unsigned long long>(created), static_cast<unsigned long long>(s.pool_jobs));
      }
  }
}

// exact_jobs (0: none): the pool job count every W > 1 pool tower must have.
int check_floors(const Identity& id, std::uint64_t min_requests, std::uint64_t min_jobs,
                 std::uint64_t exact_jobs = 0) {
  std::printf("identity towers=%llu pool_towers=%llu min_pool_jobs=%llu max_pool_jobs=%llu "
              "max_static_requests=%llu threads_created_pool_on=%llu threads_created_pool_off=%llu\n",
              static_cast<unsigned long long>(id.towers), static_cast<unsigned long long>(id.pool_towers),
              static_cast<unsigned long long>(id.pool_jobs_min), static_cast<unsigned long long>(id.pool_jobs_max),
              static_cast<unsigned long long>(id.max_requests), static_cast<unsigned long long>(id.threads_on),
              static_cast<unsigned long long>(id.threads_off));
  if (exact_jobs != 0 && (id.pool_jobs_min != exact_jobs || id.pool_jobs_max != exact_jobs)) {
    std::printf("cause=tower.pool_jobs_count expected=%llu\n", static_cast<unsigned long long>(exact_jobs));
    return 1;
  }
  if (id.pool_towers == 0 || id.pool_jobs_min < min_jobs || id.max_requests < min_requests ||
      !(id.threads_on < id.threads_off)) {
    std::printf("cause=floor.identity\n");
    return 3;
  }
  return 0;
}

int run_fixtures() {
  Identity id;
  for (const unsigned kmax : {5u, 8u}) {
    const auto points = clusters(1500);
    mhgp9::ChainOptions options;
    options.kmax = kmax;
    options.workers = 4;
    options.tower_static_threads = 4;
    options.keep_catalogue = true;
    const auto r = mhgp9::run_tower_chain(points, options);
    if (r.status != mhgp9::ChainStatus::kComplete) fail("chain.status K=" + std::to_string(kmax) + " " + r.reason);
    identity(kmax == 5 ? "fixture_k5" : "fixture_k8", points, kmax, r.catalogue_balls, r, id, true, false);
    // Through the chain: the option (default on) and its off side.
    if (!mhgp9::ChainOptions{}.tower_persistent_pool) fail("chain.pool_default_off");
    options.keep_catalogue = false;
    options.tower_persistent_pool = false;
    const auto off = mhgp9::run_tower_chain(points, options);
    if (off.status != mhgp9::ChainStatus::kComplete || off.tower_digest != r.tower_digest ||
        tower_work(off.tower_stats) != tower_work(r.tower_stats) || off.tower_stats.pool_jobs != 0 ||
        r.tower_stats.pool_threads != 3 || r.tower_stats.pool_jobs == 0)
      fail("chain.pool_option K=" + std::to_string(kmax));
  }
  // A tower thread that cannot be launched is a resource refusal, with the
  // pool (its creation fails before any work) and without it.
  {
    const auto points = clusters(1500);
    mhgp9::ChainOptions options;
    options.kmax = 5;
    options.workers = 4;
    options.keep_catalogue = true;
    const auto r = mhgp9::run_tower_chain(points, options);
    const auto ix = index_of(points);
    for (const bool pool : {true, false}) {
      pd::launch_fail_after = 1;
      const auto tw = mhgp9::tower::build_full_ball_tower(ix, r.catalogue_balls, 5, 4, {}, true,
          mhgp9::tower::FullBallTowerOptions{.overlap_static = true, .persistent_pool = pool});
      pd::launch_fail_after = static_cast<std::size_t>(-1);
      if (tw.status != mhgp9::tower::FullBallStatus::kResourceExhausted ||
          std::string_view(tw.reason) != "full_ball_thread_launch_failed" || !tw.orders.empty() ||
          tw.stats.pool_threads != 0 || pd::launch_active.load() != 0)
        fail(std::string("tower.launch_failure pool=") + (pool ? "1" : "0") + " reason=" + tw.reason);
    }
  }
  priority_fixtures();
  const int floors = check_floors(id, 8192, 40);  // hashed grouping: 45 or more per fixture tower
  if (floors) return floors;
  std::printf("task_pool_gate fixtures towers=%llu\n", static_cast<unsigned long long>(id.towers));
  return 0;
}

std::vector<mhgp9::gen::Point3> read_u32le(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) fail("frame.unreadable", 2);
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (bytes.empty() || bytes.size() % 12 != 0) fail("frame.length", 2);
  std::vector<mhgp9::gen::Point3> points(bytes.size() / 12);
  for (std::size_t i = 0; i < points.size(); ++i) {
    std::uint32_t c[3];
    for (int a = 0; a < 3; ++a) {
      std::uint32_t v = 0;
      for (int b = 0; b < 4; ++b) v |= static_cast<std::uint32_t>(bytes[12 * i + 4 * a + b]) << (8 * b);
      if (v > 262143u) fail("frame.coordinate", 2);
      c[a] = v;
    }
    points[i] = {static_cast<mhgp9::gen::Coordinate>(c[0]), static_cast<mhgp9::gen::Coordinate>(c[1]),
                 static_cast<mhgp9::gen::Coordinate>(c[2])};
  }
  return points;
}

int run_frame(const std::string& path, unsigned kmax, std::uint64_t tower_pin, std::uint64_t catalogue_pin) {
  const auto points = read_u32le(path);
  mhgp9::ChainOptions options;
  options.kmax = kmax;
  options.workers = 8;
  options.keep_catalogue = true;
  options.catalogue_digest = true;
  const auto r = mhgp9::run_tower_chain(points, options);
  if (r.status != mhgp9::ChainStatus::kComplete) fail("frame.chain_status " + r.reason);
  std::printf("frame n=%zu K=%u tower_digest=%016llx catalogue_digest=%016llx chain_pool_jobs=%llu\n", points.size(),
              kmax, static_cast<unsigned long long>(r.tower_digest),
              static_cast<unsigned long long>(r.catalogue_digest),
              static_cast<unsigned long long>(r.tower_stats.pool_jobs));
  if (r.tower_digest != tower_pin || r.catalogue_digest != catalogue_pin) fail("frame.pinned_digests");
  // Every helper call with more than one planned worker is a pool job: the
  // same count for every W >= 2, EXACTLY 45 on 08/000000 at K5 and 80 at
  // K10 (a job added, removed or split changes it), in the chain as in
  // every tower below.
  // Integration before R21: the hashed phase-0 grouping (default) makes
  // fewer helper calls than the sorted witness the counts were first pinned
  // on (58 and 108).
  const std::uint64_t exact_jobs = kmax == 5 ? 45 : 80;
  if (r.tower_stats.pool_jobs != exact_jobs) fail("tower.pool_jobs_count chain");
  Identity id;
  identity("frame", points, kmax, r.catalogue_balls, r, id, false, true);
  const int floors = check_floors(id, 100000, exact_jobs, exact_jobs);
  if (floors) return floors;
  std::printf("task_pool_gate frame towers=%llu\n", static_cast<unsigned long long>(id.towers));
  return 0;
}

bool parse_hex(std::string_view s, std::uint64_t& out) {
  if (s.empty() || s.size() > 16) return false;
  out = 0;
  for (const char ch : s) {
    const int d = ch >= '0' && ch <= '9' ? ch - '0' : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10 : -1;
    if (d < 0) return false;
    out = out * 16 + static_cast<std::uint64_t>(d);
  }
  return true;
}
#endif

}  // namespace

int main(int argc, char** argv) {
  const auto usage = [] {
    std::fprintf(stderr, "usage: mhgp9_tower_task_pool_gate --unit [--inject=MUTANT] | --priority | --fixtures | "
                         "--frame=PATH --k=K --tower=HEX --catalogue=HEX\n");
    return 2;
  };
  if (argc < 2) return usage();
  const std::string_view mode(argv[1]);
  try {
    if (mode == "--unit") {
      for (int i = 2; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (!arg.starts_with("--inject=") || !mhgp9::tower::mutants_enable(arg.substr(9))) return usage();
      }
      return run_unit();
    }
#if defined(MHGP9_POOL_GATE_CHAIN) || defined(MHGP9_POOL_GATE_TOWER)
    if (mode == "--priority" && argc == 2) return run_priority();
#endif
#if defined(MHGP9_POOL_GATE_CHAIN)
    if (mode == "--fixtures" && argc == 2) return run_fixtures();
    if (mode.starts_with("--frame=")) {
      unsigned kmax = 0;
      std::uint64_t tower_pin = 0, catalogue_pin = 0;
      bool has_tower = false, has_catalogue = false;
      for (int i = 2; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "--k=5") kmax = 5;
        else if (arg == "--k=10") kmax = 10;
        else if (arg.starts_with("--tower=")) has_tower = parse_hex(arg.substr(8), tower_pin);
        else if (arg.starts_with("--catalogue=")) has_catalogue = parse_hex(arg.substr(12), catalogue_pin);
        else return usage();
      }
      if (!kmax || !has_tower || !has_catalogue) return usage();
      return run_frame(std::string(mode.substr(8)), kmax, tower_pin, catalogue_pin);
    }
#endif
    return usage();
  } catch (const Fail& f) {
    std::printf("cause=%s\n", f.cause.c_str());
    return f.code;
  }
}
