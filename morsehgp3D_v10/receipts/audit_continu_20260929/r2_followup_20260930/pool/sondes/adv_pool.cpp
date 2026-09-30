// Sonde adverse du verificateur (29 septembre 2026) : cas limites du pool corrige (cle pool).
// Compile contre src/sched/pool.cpp seul. Groupes :
//   g1 immediate   : l'appelant leve des sa premiere tranche (ouvriers en cours de reveil ou de capture)
//   g2 random      : levees aleatoires (types Tracked/int/bad_alloc/runtime_error), P, n, grain aleatoires
//   g3 nested      : levees dans des parallel_for imbriques (serie), depuis l'appelant et les ouvriers
//   g4 unwind      : pool detruit pendant le deroulement de l'exception relancee
//   g5 late        : un ouvrier leve tard, l'appelant a fini ses tranches et attend
//   g6 alternate   : alternance rapide levee / travail ordinaire (ouvriers en retard)
//   g7 sort        : comparateur de parallel_sort qui leve apres K comparaisons
//   g8 overflow    : (informatif) n proche de 2^64 : une tranche peut-elle etre executee deux fois apres la capture ?
// Codes : 0 conforme, 1 desaccord. Argument : groupe (defaut : g1..g7), puis facteur d'iterations (defaut 1.0).
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "sched/pool.hpp"
#include "sched/sort.hpp"

using namespace mhgp10;

namespace {

int failures = 0;
void expect(bool ok, const char* what, long long a = 0, long long b = 0) {
  if (!ok) {
    ++failures;
    std::printf("ECHEC %s (%lld, %lld)\n", what, a, b);
    std::fflush(stdout);
  }
}

double scale = 1.0;
int iters(int base) { return std::max(1, int(base * scale)); }

std::atomic<long long> tracked_live{0};
std::atomic<long long> tracked_negative{0};
struct Tracked {
  u64 tag;
  std::vector<u64> payload;  // tas : ASan verrait un objet d'exception libere trop tot
  explicit Tracked(u64 t) : tag(t), payload(64, t * 7 + 1) { tracked_live.fetch_add(1); }
  Tracked(const Tracked& o) : tag(o.tag), payload(o.payload) { tracked_live.fetch_add(1); }
  ~Tracked() {
    if (tracked_live.fetch_sub(1) - 1 < 0) tracked_negative.fetch_add(1);
  }
  bool intact() const {
    if (payload.size() != 64) return false;
    for (u64 x : payload)
      if (x != tag * 7 + 1) return false;
    return true;
  }
};

bool pool_ok(sched::Pool& pool) {
  if (sched::in_parallel_region()) return false;
  const u64 n = 5003;
  std::vector<u32> out(n, 0);
  pool.parallel_for(n, 7, [&](u64 b, u64 e, unsigned) {
    for (u64 i = b; i < e; ++i) out[i] += 1;
  });
  for (u64 i = 0; i < n; ++i)
    if (out[i] != 1) return false;
  return !sched::in_parallel_region();
}

// g1 : l'appelant leve a sa premiere tranche.
void g1_immediate() {
  for (unsigned P : {2u, 3u, 4u, 8u}) {
    sched::Pool pool(P);
    long long got_count = 0, normal = 0;
    for (int it = 0; it < iters(1500); ++it) {
      const u64 n = 1 + (it % 97) * 11, grain = 1 + it % 5;
      const u64 chunks = (n + grain - 1) / grain;
      std::vector<std::atomic<u32>> hits(chunks);
      std::atomic<int> active{0};
      std::atomic<u64> started{0};
      bool got = false;
      int active_at_catch = -1;
      u64 started_at_catch = 0;
      try {
        pool.parallel_for(n, grain, [&](u64 b, u64, unsigned id) {
          active.fetch_add(1);
          started.fetch_add(1);
          hits[b / grain].fetch_add(1);
          if (id == 0) {
            active.fetch_sub(1);
            throw Tracked(u64(it));
          }
          std::atomic<int> spin{0};
          for (int k = 0; k < 200; ++k) spin.fetch_add(1, std::memory_order_relaxed);
          active.fetch_sub(1);
        });
      } catch (const Tracked& t) {
        got = true;
        active_at_catch = active.load();
        started_at_catch = started.load();
        expect(t.tag == u64(it) && t.intact(), "g1 exception intacte");
      }
      bool once = true, all = true;
      for (auto& h : hits) {
        once &= h.load() <= 1;
        all &= h.load() == 1;
      }
      expect(once, "g1 tranche executee au plus une fois", it, P);
      if (got) {
        ++got_count;
        expect(active_at_catch == 0, "g1 quiescence au catch", active_at_catch, P);
        if (it % 100 == 0) std::this_thread::sleep_for(std::chrono::microseconds(300));
        expect(started.load() == started_at_catch, "g1 aucune tranche apres la relance", it, P);
      } else {
        ++normal;
        expect(all, "g1 sans levee (appelant sans tranche) : couverture exacte", it, P);
      }
      expect(!sched::in_parallel_region() && pool.nested_calls() == 0, "g1 drapeau et imbrication", it, P);
    }
    expect(tracked_live.load() == 0 && tracked_negative.load() == 0, "g1 objets d'exception liberes une fois",
           tracked_live.load(), tracked_negative.load());
    expect(pool_ok(pool), "g1 pool reutilisable", P);
    std::printf("g1 P=%u levees=%lld sans_tranche_appelant=%lld\n", P, got_count, normal);
  }
}

// g2 : levees aleatoires.
void g2_random() {
  std::mt19937_64 rng(20260929);
  std::vector<std::unique_ptr<sched::Pool>> pools;
  for (unsigned P : {1u, 2u, 3u, 4u, 6u, 8u}) pools.emplace_back(new sched::Pool(P));
  const double probs[] = {0.0, 0.0005, 0.005, 0.05, 0.5, 1.0};
  long long thrown_runs = 0, clean_runs = 0, by_type[4] = {0, 0, 0, 0};
  for (int it = 0; it < iters(3000); ++it) {
    sched::Pool& pool = *pools[rng() % pools.size()];
    const u64 n = 1 + rng() % 3000, grain = 1 + rng() % 64;
    const double p = probs[rng() % 6];
    const int type = int(rng() % 4);
    const u64 chunks = (n + grain - 1) / grain;
    const u64 seed = rng();
    std::vector<std::atomic<u32>> hits(chunks), threw(chunks);
    std::atomic<int> active{0};
    int kind = -1;
    u64 tag = ~0ull;
    bool intact = true;
    try {
      pool.parallel_for(n, grain, [&](u64 b, u64 e, unsigned) {
        active.fetch_add(1);
        const u64 c = b / grain;
        hits[c].fetch_add(1);
        std::mt19937_64 local(seed ^ (c * 0x9E3779B97F4A7C15ull));
        const bool do_throw = double(local() % 1000000) / 1e6 < p;
        u64 acc = 0;
        for (u64 i = b; i < e; ++i) acc += i * i;
        if (acc == 42) std::printf("-");
        if (do_throw) {
          threw[c].store(1);
          active.fetch_sub(1);
          switch (type) {
            case 0: throw Tracked(c);
            case 1: throw int(c);
            case 2: throw std::bad_alloc();
            default: throw std::runtime_error("chunk " + std::to_string(c));
          }
        }
        active.fetch_sub(1);
      });
    } catch (const Tracked& t) {
      kind = 0;
      tag = t.tag;
      intact = t.intact();
    } catch (int c) {
      kind = 1;
      tag = u64(c);
    } catch (const std::bad_alloc&) {
      kind = 2;
    } catch (const std::runtime_error& e) {
      kind = 3;
      tag = std::strtoull(e.what() + 6, nullptr, 10);
    }
    const int active_after = active.load();
    u64 nthrew = 0;
    bool once = true, all = true;
    for (u64 c = 0; c < chunks; ++c) {
      nthrew += threw[c].load();
      once &= hits[c].load() <= 1;
      all &= hits[c].load() == 1;
    }
    expect(once, "g2 tranche executee au plus une fois", it, pool.size());
    expect(active_after == 0, "g2 quiescence", active_after, pool.size());
    if (nthrew > 0) {
      ++thrown_runs;
      expect(kind == type, "g2 exception du type leve relancee", kind, type);
      if (kind >= 0) ++by_type[kind];
      if (kind == 0 || kind == 1 || kind == 3)
        expect(tag < chunks && threw[tag].load() == 1, "g2 exception relancee = une exception levee", (long long)tag,
               (long long)chunks);
      expect(intact, "g2 objet d'exception intact");
    } else {
      ++clean_runs;
      expect(kind == -1 && all, "g2 sans levee : couverture exacte", kind, pool.size());
    }
    expect(tracked_live.load() == 0 && tracked_negative.load() == 0, "g2 objets d'exception liberes une fois",
           tracked_live.load(), tracked_negative.load());
    expect(!sched::in_parallel_region(), "g2 drapeau", it);
  }
  for (auto& pl : pools) {
    expect(pool_ok(*pl) && pl->nested_calls() == 0, "g2 pool reutilisable", pl->size());
  }
  std::printf("g2 avec_levee=%lld sans_levee=%lld types=%lld/%lld/%lld/%lld\n", thrown_runs, clean_runs, by_type[0],
              by_type[1], by_type[2], by_type[3]);
}

// g3 : levees dans des parallel_for imbriques (executes en serie), depuis l'appelant et les ouvriers.
void g3_nested() {
  std::mt19937_64 rng(7);
  for (unsigned P : {2u, 4u, 8u}) {
    sched::Pool pool(P);
    long long thrown_runs = 0;
    for (int it = 0; it < iters(600); ++it) {
      const u64 no = 1 + rng() % 24, ni = 1 + rng() % 40;
      const u64 seed = rng();
      const bool always = rng() % 4 == 0;
      std::vector<std::atomic<u32>> hits(no * ni), threw(no * ni);
      std::vector<std::atomic<u32>> outer_started(no);
      const u64 nested0 = pool.nested_calls();
      bool got = false;
      u64 tag = ~0ull;
      try {
        pool.parallel_for(no, 1, [&](u64 bo, u64, unsigned) {
          outer_started[bo].fetch_add(1);
          pool.parallel_for(ni, 1, [&](u64 bi, u64, unsigned wi) {
            if (wi != 0) throw std::logic_error("imbrique hors du fil courant");
            hits[bo * ni + bi].fetch_add(1);
            std::mt19937_64 local(seed ^ (bo * 1000003 + bi));
            if (always ? bi == ni - 1 : local() % 23 == 0) {
              threw[bo * ni + bi].store(1);
              throw Tracked(bo * ni + bi);
            }
          });
        });
      } catch (const Tracked& t) {
        got = true;
        tag = t.tag;
      }
      u64 started_outer = 0, nthrew = 0;
      bool prefix_ok = true;
      for (u64 o = 0; o < no; ++o) {
        const u32 s = outer_started[o].load();
        prefix_ok &= s <= 1;
        started_outer += s;
        if (s == 0) {
          for (u64 i = 0; i < ni; ++i) prefix_ok &= hits[o * ni + i].load() == 0;
          continue;
        }
        // en serie : les tranches internes forment un prefixe qui s'arrete a la premiere levee
        bool stopped = false;
        for (u64 i = 0; i < ni; ++i) {
          const u32 h = hits[o * ni + i].load();
          if (stopped) prefix_ok &= h == 0;
          else prefix_ok &= h == 1;
          if (threw[o * ni + i].load()) {
            stopped = true;
            ++nthrew;
          }
        }
      }
      expect(prefix_ok, "g3 prefixes internes exacts", it, P);
      expect(pool.nested_calls() - nested0 == started_outer, "g3 appels imbriques comptes",
             (long long)(pool.nested_calls() - nested0), (long long)started_outer);
      if (nthrew > 0) {
        ++thrown_runs;
        expect(got && tag < no * ni && threw[tag].load() == 1, "g3 exception relancee = une levee", it, P);
      } else {
        expect(!got && started_outer == no, "g3 sans levee : toutes les tranches externes", it, P);
      }
      expect(tracked_live.load() == 0 && !sched::in_parallel_region(), "g3 objets liberes, drapeau baisse", it, P);
    }
    expect(pool_ok(pool), "g3 pool reutilisable", P);
    std::printf("g3 P=%u avec_levee=%lld\n", P, thrown_runs);
  }
}

// g4 : pool detruit pendant le deroulement de l'exception relancee.
void g4_unwind() {
  std::mt19937_64 rng(11);
  long long got_count = 0;
  for (int it = 0; it < iters(300); ++it) {
    const unsigned P = 2 + unsigned(rng() % 7);
    const u64 n = 1 + rng() % 500, bad = rng() % n;
    std::atomic<int> active{0};
    bool got = false;
    try {
      sched::Pool pool(P);
      pool.parallel_for(n, 1, [&](u64 b, u64, unsigned) {
        active.fetch_add(1);
        if (b == bad) {
          active.fetch_sub(1);
          throw Tracked(b);
        }
        std::this_thread::yield();
        active.fetch_sub(1);
      });
    } catch (const Tracked& t) {
      got = t.tag == bad && t.intact();
    }
    got_count += got;
    expect(got && active.load() == 0 && tracked_live.load() == 0, "g4 exception relancee, pool detruit", it, P);
  }
  std::printf("g4 levees=%lld\n", got_count);
}

// g5 : un ouvrier leve tard, apres que l'appelant a fini ses tranches (l'appelant attend users == 0).
void g5_late() {
  long long got_count = 0, caller_waited = 0;
  for (unsigned P : {2u, 4u}) {
    sched::Pool pool(P);
    for (int it = 0; it < iters(40); ++it) {
      std::atomic<bool> caller_done{false};
      std::atomic<int> worker_throws{0}, worker_in{0};
      bool got = false;
      try {
        pool.parallel_for(P, 1, [&](u64, u64, unsigned id) {
          if (id == 0) {  // l'appelant attend (borne) qu'un ouvrier soit entre, puis rend aussitot ses tranches
            const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(20);
            while (worker_in.load() == 0 && std::chrono::steady_clock::now() < limit) std::this_thread::yield();
            return;
          }
          worker_in.fetch_add(1);
          // l'ouvrier reste 5 ms (l'appelant a fini ses tranches et attend users == 0), puis leve
          const auto limit = std::chrono::steady_clock::now() + std::chrono::milliseconds(5);
          while (std::chrono::steady_clock::now() < limit) std::this_thread::yield();
          worker_throws.fetch_add(1);
          throw std::out_of_range("tard");
        });
        caller_done.store(true);
      } catch (const std::out_of_range&) {
        got = true;
      }
      got_count += got;
      caller_waited += worker_throws.load() > 0;
      expect(got == (worker_throws.load() > 0), "g5 levee tardive d'un ouvrier relancee", it, P);
      expect(!caller_done.load() || worker_throws.load() == 0, "g5 pas de retour normal apres une levee", it, P);
    }
    expect(pool_ok(pool), "g5 pool reutilisable", P);
  }
  std::printf("g5 levees=%lld\n", got_count);
}

// g6 : alternance rapide levee / travail ordinaire.
void g6_alternate() {
  sched::Pool pool(8);
  long long got_count = 0;
  for (int it = 0; it < iters(20000); ++it) {
    const u64 n = 64;
    if (it % 2 == 0) {
      try {
        pool.parallel_for(n, 1, [&](u64 b, u64, unsigned) {
          if (b % 3 == 0) throw int(b);
        });
      } catch (int) {
        ++got_count;
      }
    } else {
      std::vector<u32> out(n, 0);
      pool.parallel_for(n, 1, [&](u64 b, u64 e, unsigned) {
        for (u64 i = b; i < e; ++i) out[i] += 1;
      });
      bool ok = true;
      for (u32 v : out) ok &= v == 1;
      expect(ok, "g6 couverture exacte apres une levee", it);
    }
  }
  expect(got_count == iters(20000) / 2 + iters(20000) % 2, "g6 toutes les levees relancees", got_count);
  expect(pool_ok(pool) && pool.nested_calls() == 0, "g6 pool reutilisable");
  std::printf("g6 levees=%lld\n", got_count);
}

// g7 : comparateur de parallel_sort qui leve apres K comparaisons.
void g7_sort() {
  std::mt19937_64 rng(5);
  sched::Pool pool(4);
  long long got_count = 0, sorted_count = 0;
  for (int it = 0; it < iters(60); ++it) {
    std::vector<u64> v(20000 + rng() % 20000);
    for (u64& x : v) x = rng();
    std::vector<u64> ref = v;
    std::sort(ref.begin(), ref.end());
    std::atomic<long long> budget{(long long)(rng() % 3000000)};
    bool got = false;
    try {
      sched::parallel_sort(pool, v, [&](u64 a, u64 b) {
        if (budget.fetch_sub(1, std::memory_order_relaxed) <= 0) throw Tracked(7);
        return a < b;
      });
    } catch (const Tracked& t) {
      got = t.intact();
    }
    if (got) ++got_count;
    else {
      ++sorted_count;
      expect(v == ref, "g7 tri complet identique a std::sort", it);
    }
    expect(tracked_live.load() == 0 && !sched::in_parallel_region(), "g7 objets liberes, drapeau", it);
  }
  expect(pool_ok(pool) && pool.nested_calls() == 0, "g7 pool reutilisable");
  std::printf("g7 levees=%lld tris_complets=%lld\n", got_count, sorted_count);
}

// g8 (informatif) : n proche de 2^64, grain 1, toute tranche leve. Compte les tranches executees deux fois.
void g8_overflow() {
  long long dup_runs = 0, runs = 0;
  sched::Pool pool(8);
  for (int it = 0; it < iters(2000); ++it) {
    std::vector<std::atomic<u32>> low(64);
    std::atomic<u32> other{0};
    try {
      pool.parallel_for(~u64{0}, 1, [&](u64 b, u64, unsigned) {
        if (b < 64) low[b].fetch_add(1);
        else other.fetch_add(1);
        throw int(1);
      });
    } catch (int) {
    }
    bool dup = false;
    for (auto& x : low) dup |= x.load() > 1;
    dup_runs += dup;
    ++runs;
  }
  std::printf("g8 informatif : executions avec une tranche executee deux fois = %lld / %lld\n", dup_runs, runs);
}

}  // namespace

int main(int argc, char** argv) {
  const std::string group = argc >= 2 ? argv[1] : "";
  if (argc >= 3) scale = std::atof(argv[2]);
  if (group.empty() || group == "g1") g1_immediate();
  if (group.empty() || group == "g2") g2_random();
  if (group.empty() || group == "g3") g3_nested();
  if (group.empty() || group == "g4") g4_unwind();
  if (group.empty() || group == "g5") g5_late();
  if (group.empty() || group == "g6") g6_alternate();
  if (group.empty() || group == "g7") g7_sort();
  if (group == "g8") g8_overflow();
  if (failures) {
    std::printf("adv_failures %d\n", failures);
    return 1;
  }
  std::printf("adv_ok\n");
  return 0;
}
