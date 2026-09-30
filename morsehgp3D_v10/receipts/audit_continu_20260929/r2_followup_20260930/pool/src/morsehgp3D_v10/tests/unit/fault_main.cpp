// Portes d'injection d'echecs d'allocation (hors produit). Les fonctions globales d'allocation sont remplacees
// (remplacement permis par la norme), toutes formes : scalaire, tableau, nothrow, et alignees (operator new(size_t,
// align_val_t), que prend std::vector d'un type alignas(64), comme le std::vector<Local> de generator.cpp). Un
// relais vers malloc ou aligned_alloc peut faire echouer une allocation choisie : la k-ieme du fil courant, la N-ieme
// tous fils confondus, ou la N-ieme allocation alignee. Le new nothrow de libstdc++ passe par ce new : un Buffer voit
// l'echec comme un nullptr, std::stable_sort se replie sans tampon.
//   mhgp10_fault                   : tous les groupes
//   mhgp10_fault pool_construction : creation partielle du pool (audit independant § 2)
//   mhgp10_fault entry_points      : std::bad_alloc dans build_catalogue et build_tower, dans l'appelant ou un ouvrier
//   mhgp10_fault aligned_forms     : chaque allocation alignee de build_catalogue et build_tower (verificateur du
//                                    tour 1 : le harnais d'avant ne remplacait pas les formes alignees)
// Codes : 0 conforme, 1 desaccord, 2 groupe inconnu, 3 plancher non atteint.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "catalogue/catalogue.hpp"
#include "cloud/site_tree.hpp"
#include "sched/pool.hpp"
#include "tower/tower.hpp"

namespace {

std::atomic<bool> g_armed{false};           // la N-ieme allocation (tous fils, toutes formes) echoue
std::atomic<long long> g_countdown{-1};
std::atomic<bool> g_armed_aligned{false};   // la N-ieme allocation alignee (tous fils) echoue
std::atomic<long long> g_countdown_aligned{-1};
std::atomic<bool> g_counting{false};        // compte les allocations (tous fils)
std::atomic<unsigned long long> g_count{0}, g_count_aligned{0};
std::atomic<unsigned long long> g_injected{0}, g_injected_off_main{0}, g_injected_aligned{0};
std::thread::id g_main;                     // ecrit avant la creation de tout pool
thread_local long long t_countdown = -1;    // la k-ieme allocation du fil courant echoue

// Decision commune a toutes les formes : vrai si cette allocation doit echouer.
bool must_fail(bool aligned) {
  if (t_countdown >= 0 && t_countdown-- == 0) return true;
  if (g_counting.load(std::memory_order_relaxed)) {
    g_count.fetch_add(1, std::memory_order_relaxed);
    if (aligned) g_count_aligned.fetch_add(1, std::memory_order_relaxed);
  }
  const bool hit =
      (g_armed.load(std::memory_order_relaxed) && g_countdown.fetch_sub(1, std::memory_order_relaxed) == 0) ||
      (aligned && g_armed_aligned.load(std::memory_order_relaxed) &&
       g_countdown_aligned.fetch_sub(1, std::memory_order_relaxed) == 0);
  if (hit) {
    g_injected.fetch_add(1, std::memory_order_relaxed);
    if (aligned) g_injected_aligned.fetch_add(1, std::memory_order_relaxed);
    if (std::this_thread::get_id() != g_main) g_injected_off_main.fetch_add(1, std::memory_order_relaxed);
  }
  return hit;
}

// aligned_alloc exige une taille multiple de l'alignement (une puissance de 2, ici au moins 32).
void* aligned_block(std::size_t n, std::align_val_t al) noexcept {
  const std::size_t a = static_cast<std::size_t>(al);
  if (n > std::numeric_limits<std::size_t>::max() - a) return nullptr;
  return std::aligned_alloc(a, n == 0 ? a : (n + a - 1) / a * a);
}

}  // namespace

// Toutes les formes sont remplacees et appariees : new par malloc ou aligned_alloc, delete par free. Sous ASan et
// TSan, le runtime fournit les siennes, qui ne passeraient pas par ces relais.
void* operator new(std::size_t n) {
  if (must_fail(false)) throw std::bad_alloc();
  if (void* p = std::malloc(n == 0 ? 1 : n)) return p;
  throw std::bad_alloc();
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  try {
    return ::operator new(n);
  } catch (const std::bad_alloc&) {
    return nullptr;
  }
}
void* operator new[](std::size_t n, const std::nothrow_t& tag) noexcept { return ::operator new(n, tag); }
void* operator new(std::size_t n, std::align_val_t al) {
  if (must_fail(true)) throw std::bad_alloc();
  if (void* p = aligned_block(n, al)) return p;
  throw std::bad_alloc();
}
void* operator new[](std::size_t n, std::align_val_t al) { return ::operator new(n, al); }
void* operator new(std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept {
  try {
    return ::operator new(n, al);
  } catch (const std::bad_alloc&) {
    return nullptr;
  }
}
void* operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t& tag) noexcept {
  return ::operator new(n, al, tag);
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { std::free(p); }

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

struct alignas(64) Wide64 {  // comme Local (generator.cpp) : passe par les formes alignees de new
  unsigned char bytes[64];
};
static_assert(alignof(Wide64) > __STDCPP_DEFAULT_NEW_ALIGNMENT__, "Wide64 doit passer par les formes alignees");

// Juge du juge : l'injection atteint une expression new et le new nothrow des Buffer (refus, budget rendu), puis chaque
// forme alignee (scalaire, tableau, nothrow scalaire et tableau, std::vector d'un type alignas(64)), comptee comme
// alignee et servie alignee sans injection. Harnais d'avant : aucune forme alignee n'etait injectee.
void test_harness() {
  static u64* volatile sink = nullptr;
  bool threw = false;
  t_countdown = 0;
  try {
    sink = new u64[4];
  } catch (const std::bad_alloc&) {
    threw = true;
  }
  t_countdown = -1;
  delete[] sink;
  sink = nullptr;
  MemoryBudget budget;
  Buffer<u64> buf;
  t_countdown = 0;
  const bool refused = !buf.allocate(8, budget);
  t_countdown = -1;
  expect(threw && refused && budget.used() == 0 && buf.allocate(8, budget), "harnais : injection sur new et nothrow");

  static Wide64* volatile wide = nullptr;  // le pointeur s'echappe : aucune allocation ne peut etre elidee
  u32 injected = 0;
  t_countdown = 0;
  try {
    wide = new Wide64;
  } catch (const std::bad_alloc&) {
    ++injected;
  }
  t_countdown = -1;
  delete wide;
  wide = nullptr;
  t_countdown = 0;
  try {
    wide = new Wide64[3];
  } catch (const std::bad_alloc&) {
    ++injected;
  }
  t_countdown = -1;
  delete[] wide;
  wide = nullptr;
  t_countdown = 0;
  wide = new (std::nothrow) Wide64;
  t_countdown = -1;
  injected += wide == nullptr ? 1 : 0;
  delete wide;
  wide = nullptr;
  t_countdown = 0;
  wide = new (std::nothrow) Wide64[3];
  t_countdown = -1;
  injected += wide == nullptr ? 1 : 0;
  delete[] wide;
  wide = nullptr;
  t_countdown = 0;
  try {
    std::vector<Wide64> v(5);
    wide = v.data();
  } catch (const std::bad_alloc&) {
    ++injected;
  }
  t_countdown = -1;
  wide = nullptr;
  g_count = 0;
  g_count_aligned = 0;
  g_counting = true;
  bool served = false;
  {
    std::vector<Wide64> v(5);
    wide = v.data();
    served = reinterpret_cast<std::uintptr_t>(v.data()) % alignof(Wide64) == 0;
  }
  g_counting = false;
  wide = nullptr;
  const bool counted = g_count_aligned.load() == 1 && g_count.load() == 1;
  g_count = 0;
  g_count_aligned = 0;
  std::printf("harnais formes_alignees injectees %u sur 5\n", injected);
  expect(injected == 5 && served && counted, "harnais : injection et comptage sur les formes alignees");
}

// Pool utilisable : couverture exacte, et participation de tous les fils (P tranches qui attendent P fils distincts).
bool pool_serves(sched::Pool& pool) {
  if (sched::in_parallel_region()) return false;  // drapeau colle : la suite passerait en serie et l'attente expirerait
  const u64 n = 20011;
  std::vector<u64> out(n, 0);
  pool.parallel_for(n, 13, [&](u64 b, u64 e, unsigned) {
    for (u64 i = b; i < e; ++i) out[i] = 3 * i + 1;
  });
  bool ok = true;
  for (u64 i = 0; i < n; ++i) ok &= out[i] == 3 * i + 1;
  const unsigned P = pool.size();
  std::vector<std::atomic<u32>> seen(P);
  std::atomic<u32> distinct{0};
  pool.parallel_for(P, 1, [&](u64, u64, unsigned id) {
    if (seen[id].fetch_add(1) == 0) distinct.fetch_add(1);
    const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    while (distinct.load() < P && std::chrono::steady_clock::now() < limit) std::this_thread::yield();
  });
  return ok && distinct.load() == P && !sched::in_parallel_region();
}

// Creation partielle du pool : la k-ieme allocation du fil qui construit echoue (tableau des fils, puis etat de
// chaque std::thread, les fils precedents etant lances). Pool d'avant : un std::thread joignable detruit pendant le
// deroulement du constructeur appelle std::terminate (SIGABRT). Attendu : std::bad_alloc sort du constructeur apres
// l'arret et la jointure des fils crees ; un pool construit ensuite fonctionne.
void test_pool_construction() {
  const unsigned P = 4;
  t_countdown = 1000000;
  { sched::Pool probe(P); }
  const long long allocs = 1000000 - t_countdown;
  t_countdown = -1;
  long long threw = 0;
  for (long long k = 0; k < allocs; ++k) {
    bool got = false;
    t_countdown = k;
    try {
      sched::Pool pool(P);
    } catch (const std::bad_alloc&) {
      got = true;
    }
    t_countdown = -1;
    expect(got, "construction partielle du pool : std::bad_alloc sort du constructeur");
    threw += got;
  }
  sched::Pool pool(P);
  expect(pool_serves(pool), "pool construit apres les echecs de construction");
  std::printf("pool_construction allocations %lld echecs_injectes %lld\n", allocs, threw);
  if (allocs < P) failures += 1000;  // tableau des fils + un etat par ouvrier
}

Cloud make_cloud(u32 n, u64 seed) {
  std::mt19937_64 g(seed);
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) {
    x[i] = u32(g() % 4096);
    y[i] = u32(g() % 4096);
    z[i] = u32(g() % 4096);
    pid[i] = 1000u + 3u * i;
  }
  auto r = prepare_cloud(x, y, z, pid, kCoordinateBits);
  expect(r.ok() && r.value().sites() == n, "nuage de la fixture");
  return r.take();
}

bool same_catalogue(const Catalogue& a, const Catalogue& b) {
  bool same = a.kmax == b.kmax && a.rank == b.rank && a.support == b.support && a.qmin == b.qmin && a.p == b.p &&
              a.u == b.u && a.flags == b.flags && a.pop_off == b.pop_off && a.pop == b.pop &&
              a.n_interior == b.n_interior && a.level.size() == b.level.size();
  for (size_t i = 0; same && i < a.level.size(); ++i) same = geom::compare(a.level[i], b.level[i]) == 0;
  return same;
}

bool empty_catalogue(const Catalogue& c) {
  return c.kmax == 0 && c.rank.empty() && c.pop_off.empty() && c.pop.empty() && c.level.empty();
}

bool same_tower(const Tower& a, const Tower& b) {
  bool same = a.kmax == b.kmax && a.orders.size() == b.orders.size();
  for (size_t k = 0; same && k < a.orders.size(); ++k) {
    const OrderForest& x = a.orders[k];
    const OrderForest& y = b.orders[k];
    same = x.k == y.k && x.rank == y.rank && x.parent == y.parent && x.child_off == y.child_off &&
           x.child_val == y.child_val && x.birth == y.birth && x.lower == y.lower && x.point_node == y.point_node &&
           x.point_level == y.point_level && x.point_cat_rank == y.point_cat_rank && x.ball_node == y.ball_node;
  }
  return same;
}

// Indices d'allocation balayes : les premieres, puis une suite geometrique jusqu'a 95 % du travail compte.
std::vector<long long> sweep(unsigned long long total) {
  std::vector<long long> v;
  for (long long i = 0; i < 64 && (unsigned long long)i < total; ++i) v.push_back(i);
  for (double x = 64; x < 0.95 * double(total); x *= 1.08) v.push_back(static_cast<long long>(x));
  return v;
}

struct SweepStats {
  u64 runs = 0, injected = 0, refused = 0, ok_after_injection = 0, off_main = 0;
  u64 aligned_allocations = 0, aligned_injected = 0;
};

// Cible de l'injection : la N-ieme allocation de toutes formes (indices balayes par sweep), ou la N-ieme allocation
// alignee (chacune a son tour, plus une au-dela de la derniere, sans injection).
enum class Target { any, aligned };

// Un point d'entree sous injection : `build` rend (ok, identique a la reference, vide, raison).
template <class Build>
SweepStats run_sweep(const char* name, Target target, Build build) {
  SweepStats s;
  std::atomic<long long>& countdown = target == Target::aligned ? g_countdown_aligned : g_countdown;
  std::atomic<bool>& armed = target == Target::aligned ? g_armed_aligned : g_armed;
  g_count = 0;
  g_count_aligned = 0;
  g_counting = true;
  unsigned long long total = ~0ull, aligned = ~0ull, aligned_most = 0;
  for (int rep = 0; rep < 3; ++rep) {  // le nombre d'allocations depend un peu de la repartition entre fils
    const unsigned long long before = g_count.load(), before_aligned = g_count_aligned.load();
    build();
    total = std::min(total, g_count.load() - before);
    aligned = std::min(aligned, g_count_aligned.load() - before_aligned);
    aligned_most = std::max(aligned_most, g_count_aligned.load() - before_aligned);
  }
  g_counting = false;
  s.aligned_allocations = aligned;
  std::vector<long long> at_list;
  if (target == Target::aligned) {
    for (unsigned long long i = 0; i <= aligned_most; ++i) at_list.push_back(static_cast<long long>(i));
  } else {
    at_list = sweep(total);
  }
  for (long long at : at_list) {
    const unsigned long long inj0 = g_injected.load(), off0 = g_injected_off_main.load(),
                             al0 = g_injected_aligned.load();
    const u64 used0 = default_budget().used();
    countdown = at;
    armed = true;
    const auto [ok, same, empty, reason] = build();
    armed = false;
    const bool injected = g_injected.load() != inj0;
    ++s.runs;
    s.injected += injected;
    s.off_main += g_injected_off_main.load() != off0;
    s.aligned_injected += g_injected_aligned.load() != al0;
    // sans injection : resultat identique ; avec : refus memory_budget sans valeur, ou resultat identique (echec
    // absorbe par un repli sans tampon, comme std::stable_sort) ; jamais autre chose
    const bool refused = !ok && reason == Reason::memory_budget && status_of(reason) == Status::resource_exhausted &&
                         empty;
    expect(injected ? (refused || (ok && same)) : (ok && same), name);
    expect(!sched::in_parallel_region() && default_budget().used() == used0, name);
    s.refused += refused;
    s.ok_after_injection += injected && ok;
  }
  std::printf("%s allocations %llu essais %llu injections %llu (hors appelant %llu) refus_memory_budget %llu "
              "absorbees %llu\n",
              name, total, (unsigned long long)s.runs, (unsigned long long)s.injected, (unsigned long long)s.off_main,
              (unsigned long long)s.refused, (unsigned long long)s.ok_after_injection);
  std::printf("%s formes_alignees allocations %llu injections %llu\n", name, (unsigned long long)s.aligned_allocations,
              (unsigned long long)s.aligned_injected);
  return s;
}

struct EntrySweeps {
  SweepStats catalogue, tower;
};

// std::bad_alloc dans build_catalogue et build_tower (appelant ou ouvrier, relancee par le pool) : statut
// resource_exhausted, raison memory_budget, aucune valeur publiee, budget des Buffer rendu ; puis, sur le meme pool,
// un calcul sans injection identique a la reference. Pool d'avant : std::terminate (ouvrier), comportement indefini
// (appelant dans un parallel_for), ou exception sortie du point d'entree (appelant hors parallel_for).
EntrySweeps sweep_entry_points(Target target) {
  const bool al = target == Target::aligned;
  sched::Pool pool(4);
  const Cloud cloud = make_cloud(400, 20260929);
  const SiteTree tree(cloud);
  CatalogueParams cp;
  cp.kmax = 4;
  auto ref_cat = build_catalogue(cloud, cp, pool);
  expect(ref_cat.ok() && ref_cat.value().balls() > 0, "catalogue de reference");
  TowerParams tp;
  tp.kmax = 4;
  auto ref_tower = build_tower(cloud, tree, ref_cat.value(), tp, pool);
  expect(ref_tower.ok() && ref_tower.value().orders.size() == 4, "tour de reference");
  struct Outcome4 {
    bool ok, same, empty;
    Reason reason;
  };
  auto guarded = [](auto&& call) -> Outcome4 {
    try {
      return call();
    } catch (...) {
      expect(false, "exception sortie du point d'entree");
      return Outcome4{false, false, false, Reason::none};
    }
  };
  const SweepStats c = run_sweep(al ? "fault_catalogue_alignees" : "fault_catalogue", target, [&] {
    return guarded([&] {
      auto r = build_catalogue(cloud, cp, pool);
      return Outcome4{r.ok(), r.ok() && same_catalogue(r.value(), ref_cat.value()), empty_catalogue(r.value()),
                      r.outcome().reason};
    });
  });
  const SweepStats t = run_sweep(al ? "fault_tower_alignees" : "fault_tower", target, [&] {
    return guarded([&] {
      auto r = build_tower(cloud, tree, ref_cat.value(), tp, pool);
      return Outcome4{r.ok(), r.ok() && same_tower(r.value(), ref_tower.value()),
                      r.value().kmax == 0 && r.value().orders.empty(), r.outcome().reason};
    });
  });
  auto again_cat = build_catalogue(cloud, cp, pool);
  auto again_tower = build_tower(cloud, tree, ref_cat.value(), tp, pool);
  expect(again_cat.ok() && same_catalogue(again_cat.value(), ref_cat.value()) && again_tower.ok() &&
             same_tower(again_tower.value(), ref_tower.value()) && pool_serves(pool),
         "pool reutilise apres les injections : sorties identiques");
  return EntrySweeps{c, t};
}

void test_entry_points() {
  const EntrySweeps s = sweep_entry_points(Target::any);
  if (s.catalogue.runs < 60 || s.catalogue.refused < 40 || s.tower.runs < 60 || s.tower.refused < 40) failures += 1000;
}

// Formes alignees (verificateur du tour 1) : le std::vector<Local> alignas(64) de build_catalogue passe par
// operator new(size_t, align_val_t), que le harnais d'avant ne remplacait pas, si bien qu'aucune porte ne l'injectait.
// Chaque allocation alignee de build_catalogue et de build_tower echoue a son tour : refus memory_budget sans valeur,
// ou resultat identique ; puis le meme pool sert encore. Plancher : build_catalogue atteint au moins une allocation
// alignee et elle est injectee (build_tower n'en fait aucune aujourd'hui : compte imprime, sans plancher).
void test_aligned_forms() {
  const EntrySweeps s = sweep_entry_points(Target::aligned);
  if (s.catalogue.aligned_allocations < 1 || s.catalogue.aligned_injected < 1) failures += 1000;
}

}  // namespace

int main(int argc, char** argv) {
  g_main = std::this_thread::get_id();
  if (argc > 2) return 2;
  const std::string group = argc == 2 ? argv[1] : "";
  test_harness();
  if (group.empty() || group == "pool_construction") test_pool_construction();
  if (group.empty() || group == "entry_points") test_entry_points();
  if (group.empty() || group == "aligned_forms") test_aligned_forms();
  if (!group.empty() && group != "pool_construction" && group != "entry_points" && group != "aligned_forms") {
    std::printf("groupe inconnu %s\n", group.c_str());
    return 2;
  }
  if (failures) {
    std::printf("fault_failures %d\n", failures);
    return failures >= 1000 ? 3 : 1;
  }
  std::printf("fault_ok\n");
  return 0;
}
