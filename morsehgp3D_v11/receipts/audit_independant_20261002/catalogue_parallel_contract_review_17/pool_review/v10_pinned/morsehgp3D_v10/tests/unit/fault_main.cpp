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
//   mhgp10_fault thread_creation   : creation d'un fil refusee par le systeme (pthread_create interpose, EAGAIN),
//                                    puis sched::make_pool -> session_overhead (raccord R2, verification adverse R2-10)
//   mhgp10_fault output_set        : chaque allocation d'une sequence de sorties des sondes (cli::OutputSet) echoue a
//                                    son tour (raccord R2, prealable P2 ; auditeur continu, outputset_exception)
//   mhgp10_fault read_configs      : chaque allocation du lecteur unique de --configs (read_configs, head.hpp) echoue
//                                    a son tour : memory_budget, liste vide, aucun descripteur perdu (raccord R2,
//                                    etape tete ; auditeur continu, input_allocation_boundaries : le lecteur
//                                    parse_head_configs qu'il remplace laissait echapper std::bad_alloc)
//   mhgp10_fault cli_helpers       : aides de preparation des sondes (reparation du raccord R2 ; auditeur continu,
//                                    input_allocation_boundaries : std::bad_alloc echappait de read_u32le_cloud et de
//                                    parse_integer_list) : chaque allocation echoue a son tour, refus memory_budget,
//                                    aucune exception, aucun descripteur perdu ; parse_z et parse_integer n'allouent
//                                    pas ; cli::guarded convertit std::bad_alloc et elle seule
//   mhgp10_fault output_commit     : publication des sorties sous echec de rename ou de link, interposes dans ce seul
//                                    executable (reparation du raccord R2 ; auditeur continu,
//                                    outputset_transaction_20260930 : un echec du second renommage laissait NEW_A /
//                                    OLD_B) : originaux restaures par leur sauvegarde (meme inode), sorties nouvelles
//                                    retirees, etat de recuperation quand la restauration echoue a son tour
// Codes : 0 conforme, 1 desaccord, 2 groupe inconnu, 3 plancher non atteint.
#include <dirent.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#include "catalogue/catalogue.hpp"
#include "cloud/site_tree.hpp"
#include "cloud/u32le_input.hpp"
#include "core/cli_options.hpp"
#include "core/cli_output.hpp"
#include "head/head.hpp"
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
// La k-ieme allocation du fil courant echoue. volatile : chaque ecriture qui arme ou desarme le compte autour d'une
// expression new est un effet observable. Sans cela, un compilateur qui traite operator new comme une fonction connue
// (sans lecture des variables du programme) peut retirer l'ecriture « t_countdown = 0 » qui precede l'allocation :
// clang 18 a -O3 avec -fsanitize=address,undefined le faisait avant « new (std::nothrow) Wide64[3] », et le harnais
// n'injectait que 4 formes alignees sur 5 (reparation du raccord R2, constat B1).
thread_local volatile long long t_countdown = -1;

// Decision commune a toutes les formes : vrai si cette allocation doit echouer.
bool must_fail(bool aligned) {
  const long long left = t_countdown;  // une lecture, une ecriture : pas d'operateur compose sur un volatile
  if (left >= 0) {
    t_countdown = left - 1;
    if (left == 0) return true;
  }
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

// Toutes les formes sont remplacees et appariees : new par malloc ou aligned_alloc, delete par free. Un sanitizer
// fournit aussi les siennes ; celles de l'executable l'emportent quand le runtime est une bibliotheque partagee (GCC par
// defaut ; Clang avec -shared-libsan, que CMakeLists.txt impose a ses builds a sanitizer) : l'injection passe alors par
// ces relais, et malloc reste intercepte par le sanitizer. Avec le runtime statique de Clang, l'edition de liens echoue
// sous TSan (definitions multiples de operator new) et le runtime lie dans l'executable masque la bibliotheque de
// prechargement (reparation du raccord R2, constat B1 ; porte mhgp10_regression_clang_sanitizer_harness).
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
// Jamais developpees en ligne : GCC, qui verrait « free » applique a un pointeur rendu par operator new dans une aide
// d'en-tete compilee ici (read_u32le_cloud), le signalerait a tort (-Wmismatched-new-delete) ; les formes sont bien
// appariees, new par malloc et delete par free.
#define MHGP10_FAULT_DELETE [[gnu::noinline]]
MHGP10_FAULT_DELETE void operator delete(void* p) noexcept { std::free(p); }
MHGP10_FAULT_DELETE void operator delete(void* p, std::size_t) noexcept { std::free(p); }
MHGP10_FAULT_DELETE void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }
MHGP10_FAULT_DELETE void operator delete[](void* p) noexcept { std::free(p); }
MHGP10_FAULT_DELETE void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
MHGP10_FAULT_DELETE void operator delete[](void* p, const std::nothrow_t&) noexcept { std::free(p); }
MHGP10_FAULT_DELETE void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
MHGP10_FAULT_DELETE void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
MHGP10_FAULT_DELETE void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { std::free(p); }
MHGP10_FAULT_DELETE void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
MHGP10_FAULT_DELETE void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
MHGP10_FAULT_DELETE void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { std::free(p); }

// Creation et jointure des fils interposees dans ce seul executable de test (raccord R2, verification adverse R2-10) :
// la N-ieme creation, tous fils createurs confondus, echoue avec EAGAIN, comme sous RLIMIT_AS, RLIMIT_NPROC ou
// pids.max ; std::thread leve alors std::system_error. Les creations et les jointures reussies sont comptees : aucun
// fil cree ne doit survivre a un refus. La definition suivante dans l'ordre de recherche (libc, ou l'intercepteur d'un
// sanitizer) fait le travail reel.
namespace {
std::atomic<long long> g_thread_countdown{-1};
std::atomic<unsigned long long> g_thread_calls{0}, g_thread_created{0}, g_thread_joined{0};

template <class Fn>
Fn next_definition(const char* name) {
  void* p = dlsym(RTLD_NEXT, name);
  Fn f = nullptr;
  static_assert(sizeof f == sizeof p, "pointeurs de fonction et d'objet de meme taille");
  std::memcpy(&f, &p, sizeof f);
  return f;
}
}  // namespace

extern "C" int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start)(void*),
                              void* arg) noexcept {
  using Fn = int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
  static const Fn real = next_definition<Fn>("pthread_create");
  g_thread_calls.fetch_add(1);
  if (g_thread_countdown.load() >= 0 && g_thread_countdown.fetch_sub(1) == 0) return EAGAIN;
  if (real == nullptr) return ENOSYS;
  const int rc = real(thread, attr, start, arg);
  if (rc == 0) g_thread_created.fetch_add(1);
  return rc;
}

extern "C" int pthread_join(pthread_t thread, void** result) {
  using Fn = int (*)(pthread_t, void**);
  static const Fn real = next_definition<Fn>("pthread_join");
  if (real == nullptr) return ENOSYS;
  const int rc = real(thread, result);
  if (rc == 0) g_thread_joined.fetch_add(1);
  return rc;
}

// rename et link interposes dans ce seul executable de test (reparation du raccord R2, publication des sorties) : le
// i-eme appel depuis la derniere remise a zero echoue si le bit i du masque est pose (EIO pour rename, EPERM pour
// link, comme un systeme de fichiers sans liens physiques), sans toucher aucun fichier. Les autres appels sont
// transmis a la libc.
namespace {
std::atomic<unsigned long long> g_rename_calls{0}, g_rename_mask{0}, g_link_calls{0}, g_link_mask{0};
}  // namespace

extern "C" int rename(const char* from, const char* to) noexcept {
  using Fn = int (*)(const char*, const char*);
  static const Fn real = next_definition<Fn>("rename");
  const unsigned long long i = g_rename_calls.fetch_add(1);
  if (i < 64 && ((g_rename_mask.load() >> i) & 1) != 0) {
    errno = EIO;
    return -1;
  }
  if (real == nullptr) {
    errno = ENOSYS;
    return -1;
  }
  return real(from, to);
}

extern "C" int link(const char* from, const char* to) noexcept {
  using Fn = int (*)(const char*, const char*);
  static const Fn real = next_definition<Fn>("link");
  const unsigned long long i = g_link_calls.fetch_add(1);
  if (i < 64 && ((g_link_mask.load() >> i) & 1) != 0) {
    errno = EPERM;
    return -1;
  }
  if (real == nullptr) {
    errno = ENOSYS;
    return -1;
  }
  return real(from, to);
}

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

// Creation d'un fil refusee par le systeme (raccord R2 ; verification adverse R2-10 : std::system_error de std::thread
// n'etait converti par aucune sonde, et 256 fils sous RLIMIT_AS de 1 Gio donnaient SIGABRT). La k-ieme creation echoue
// (EAGAIN), k = 0 .. P - 2 : le constructeur du pool leve std::system_error (resource_unavailable_try_again) apres
// l'arret et la jointure des k fils crees ; sched::make_pool rend resource_exhausted/session_overhead, sans exception
// ni fil survivant. Puis chaque allocation de make_pool echoue a son tour (std::bad_alloc) : meme refus. Enfin
// make_pool sans injection rend un pool qui sert, et chaque fil cree depuis le debut du processus a ete joint.
void test_thread_creation() {
  const unsigned P = 4;
  u64 ctor_refusals = 0, make_refusals = 0, alloc_refusals = 0;
  for (long long k = 0; k + 1 < P; ++k) {
    for (const bool through_make_pool : {false, true}) {
      const unsigned long long calls0 = g_thread_calls.load(), created0 = g_thread_created.load(),
                               joined0 = g_thread_joined.load();
      bool refused = false, escaped = false;
      g_thread_countdown = k;
      try {
        if (through_make_pool) {
          auto r = sched::make_pool(P);
          refused = !r.ok() && r.outcome().reason == Reason::session_overhead &&
                    status_of(r.outcome().reason) == Status::resource_exhausted && r.value() == nullptr;
        } else {
          sched::Pool pool(P);
        }
      } catch (const std::system_error& e) {
        refused = !through_make_pool && e.code() == std::errc::resource_unavailable_try_again;
        escaped = through_make_pool;
      } catch (...) {
        escaped = true;
      }
      g_thread_countdown = -1;
      const unsigned long long calls = g_thread_calls.load() - calls0, created = g_thread_created.load() - created0,
                               joined = g_thread_joined.load() - joined0;
      const bool ok = refused && !escaped && calls == static_cast<unsigned long long>(k) + 1 &&
                      created == static_cast<unsigned long long>(k) && joined == created;
      expect(ok, through_make_pool ? "creation de fil refusee : make_pool rend session_overhead, fils joints"
                                   : "creation de fil refusee : std::system_error apres la jointure des fils crees");
      (through_make_pool ? make_refusals : ctor_refusals) += ok ? 1 : 0;
    }
  }
  // chaque allocation de make_pool (pool, tableau des fils, etat de chaque std::thread) echoue a son tour
  t_countdown = 1000000;
  { auto probe = sched::make_pool(P); }
  const long long allocs = 1000000 - t_countdown;
  t_countdown = -1;
  for (long long k = 0; k < allocs; ++k) {
    bool refused = false;
    t_countdown = k;
    try {
      auto r = sched::make_pool(P);
      refused = !r.ok() && r.outcome().reason == Reason::session_overhead && r.value() == nullptr;
    } catch (...) {
      refused = false;
    }
    t_countdown = -1;
    expect(refused, "allocation de make_pool refusee : session_overhead");
    alloc_refusals += refused ? 1 : 0;
  }
  bool served = false;
  {
    auto r = sched::make_pool(P);
    served = r.ok() && r.value() != nullptr && r.value()->size() == P && pool_serves(*r.value());
  }
  expect(served, "make_pool sans injection : pool qui sert");
  expect(g_thread_created.load() == g_thread_joined.load(), "chaque fil cree a ete joint");
  std::printf("thread_creation positions %u refus_constructeur %llu refus_make_pool %llu allocations_make_pool %lld "
              "refus_allocation %llu fils_crees %llu fils_joints %llu\n",
              P - 1, (unsigned long long)ctor_refusals, (unsigned long long)make_refusals, allocs,
              (unsigned long long)alloc_refusals, g_thread_created.load(), g_thread_joined.load());
  if (ctor_refusals < P - 1 || make_refusals < P - 1 || allocs < P || g_thread_created.load() < P - 1)
    failures += 1000;  // chaque position de creation, et au moins le pool, son tableau et un etat par ouvrier
}

// Sorties des sondes sous echec d'allocation (raccord R2, prealable P2 ; auditeur continu, outputset_exception_20260930 :
// une allocation qui levait apres fopen laissait fuir le descripteur et la sentinelle tronquee ; un ecrivain qui levait
// laissait fuir le sien). La k-ieme allocation du fil courant echoue, k = 0, 1, ... jusqu'a la premiere sequence
// complete sans injection (au plus 4096 essais) : entree enregistree, trois sorties declarees (une preexistante, une
// nouvelle, une optionnelle), reservation, trois ecritures dont l'ecrivain alloue, commit. Pour chaque k : aucune
// exception ne sort ; chaque etape rend ok ou memory_budget ; apres la destruction, les descripteurs sont ceux d'avant et
// le dossier ne contient que les fichiers attendus (aucun temporaire) ; ou bien le commit a reussi et les trois sorties
// sont publiees, ou bien aucune ne l'est et la sortie preexistante est intacte. Plancher : une injection au moins dans
// chaque etape qui alloue (entree, declaration, reservation, ecriture).
std::vector<std::string> dir_entries(const std::string& dir) {
  std::vector<std::string> out;
  if (DIR* d = ::opendir(dir.c_str())) {
    while (const dirent* e = ::readdir(d))
      if (std::strcmp(e->d_name, ".") != 0 && std::strcmp(e->d_name, "..") != 0) out.push_back(e->d_name);
    ::closedir(d);
  }
  std::sort(out.begin(), out.end());
  return out;
}

int open_descriptors() {
  int n = 0;
  if (DIR* d = ::opendir("/proc/self/fd")) {
    while (const dirent* e = ::readdir(d))
      if (std::strcmp(e->d_name, ".") != 0 && std::strcmp(e->d_name, "..") != 0) ++n;
    ::closedir(d);
  }
  return n;
}

bool write_text(const std::string& p, const char* text) {
  std::FILE* f = std::fopen(p.c_str(), "wb");
  if (f == nullptr) return false;
  const bool wrote = std::fputs(text, f) >= 0;
  return std::fclose(f) == 0 && wrote;
}

std::string read_text(const std::string& p) {
  std::FILE* f = std::fopen(p.c_str(), "rb");
  if (f == nullptr) return "ABSENT";
  std::string t;
  char chunk[256];
  size_t got;
  while ((got = std::fread(chunk, 1, sizeof chunk, f)) > 0) t.append(chunk, got);
  std::fclose(f);
  return t;
}

void test_output_set() {
  const char* base = std::getenv("TMPDIR");
  std::string tmpl = std::string(base != nullptr && *base != '\0' ? base : "/tmp") + "/mhgp10_fault_sorties_XXXXXX";
  std::vector<char> buf(tmpl.begin(), tmpl.end());
  buf.push_back('\0');
  if (::mkdtemp(buf.data()) == nullptr) {
    expect(false, "sorties : dossier prive");
    return;
  }
  const std::string dir = buf.data(), in = dir + "/in", old = dir + "/old", fresh = dir + "/new", opt = dir + "/opt";
  // contenus de plus de 15 octets : la copie dans l'ecrivain alloue (pas d'optimisation des chaines courtes)
  const char* const text_old = "nouveau contenu de la sortie preexistante";
  const char* const text_new = "contenu de la sortie nouvelle, publiee au commit";
  const char* const text_opt = "contenu de la sortie optionnelle, ecrite apres coup";
  const std::vector<std::string> published = {"in", "new", "old", "opt"}, untouched = {"in", "old"};
  const char* const stage_name[] = {"entree", "declaration", "reservation", "ecriture", "commit"};
  u64 stage_refusals[5] = {0, 0, 0, 0, 0};
  u64 runs = 0, injected = 0;
  bool completed = false;
  for (long long k = 0; k < 4096 && !completed; ++k) {
    ::unlink(fresh.c_str());
    ::unlink(opt.c_str());
    expect(write_text(in, "entree") && write_text(old, "ancien"), "sorties : fixtures");
    const int fds = open_descriptors();
    bool escaped = false, committed = false, codes_ok = true;
    int failed_stage = -1;
    t_countdown = k;
    try {
      cli::OutputSet o;
      auto step = [&](int stage, const Outcome& r) {  // ok, ou memory_budget seulement
        if (r.ok()) return true;
        if (r.reason != Reason::memory_budget) codes_ok = false;
        failed_stage = stage;
        return false;
      };
      auto writer = [](const char* t) {
        return [t](std::FILE* f) {
          const std::string copy(t);  // allocation dans l'ecrivain
          std::fputs(copy.c_str(), f);
        };
      };
      committed = step(0, o.add_input(in)) && step(1, o.declare(old)) && step(1, o.declare(fresh)) &&
                  step(1, o.declare(opt, true)) && step(2, o.reserve()) && step(3, o.write(old, writer(text_old))) &&
                  step(3, o.write(fresh, writer(text_new))) && step(3, o.write(opt, writer(text_opt))) &&
                  step(4, o.commit());
      if (committed) o.release();  // publication definitive (sans release, le destructeur la defait)
    } catch (...) {
      escaped = true;
    }
    const bool hit = t_countdown == -1;  // la (k + 1)-ieme allocation a eu lieu, donc echoue
    t_countdown = -1;
    ++runs;
    injected += hit ? 1 : 0;
    if (failed_stage >= 0) ++stage_refusals[failed_stage];
    const std::vector<std::string> after = dir_entries(dir);
    const bool state_ok = committed ? after == published && read_text(old) == text_old &&
                                          read_text(fresh) == text_new && read_text(opt) == text_opt
                                    : after == untouched && read_text(old) == "ancien";
    const bool ok = !escaped && codes_ok && open_descriptors() == fds && state_ok && read_text(in) == "entree" &&
                    (hit || committed);
    if (!ok)
      std::printf("sorties k=%lld injection=%d etape=%d exception=%d codes=%d descripteurs %d/%d etat=%d\n", k, hit,
                  failed_stage, escaped, codes_ok, open_descriptors(), fds, state_ok);
    expect(ok, "sorties sous echec d'allocation : refus propre, aucun descripteur perdu, tout ou rien");
    if (!hit) completed = true;
  }
  for (const std::string& n : dir_entries(dir)) ::unlink((dir + "/" + n).c_str());
  ::rmdir(dir.c_str());
  std::printf("output_set essais %llu injections %llu sequence_complete %d refus_par_etape", (unsigned long long)runs,
              (unsigned long long)injected, completed ? 1 : 0);
  for (int st = 0; st < 5; ++st) std::printf(" %s=%llu", stage_name[st], (unsigned long long)stage_refusals[st]);
  std::printf("\n");
  expect(completed, "sorties : sequence complete sans injection atteinte");
  if (!completed || stage_refusals[0] == 0 || stage_refusals[1] == 0 || stage_refusals[2] == 0 ||
      stage_refusals[3] == 0)
    failures += 1000;  // chaque etape qui alloue a vu au moins une injection
}

// Lecteur unique de --configs (head.hpp) : la (k + 1)-ieme allocation du fil echoue, k = 0, 1, ... jusqu'a la premiere
// lecture sans injection. Chaque refus est memory_budget, la liste de l'appelant est vide, aucune exception ne sort,
// aucun descripteur n'est perdu ; la lecture complete rend les deux configurations attendues. Contenu de plus de 15
// octets (le texte lu alloue) et jetons en vecteur (allocations des jetons et de la liste).
void test_read_configs() {
  const char* base = std::getenv("TMPDIR");
  std::string tmpl = std::string(base != nullptr && *base != '\0' ? base : "/tmp") + "/mhgp10_fault_configs_XXXXXX";
  std::vector<char> buf(tmpl.begin(), tmpl.end());
  buf.push_back('\0');
  if (::mkdtemp(buf.data()) == nullptr) {
    expect(false, "configs : dossier prive");
    return;
  }
  const std::string dir = buf.data(), cfg = dir + "/configs";
  expect(write_text(cfg, "39 1.0 eom 0\n7 2.5 leaf 1\n"), "configs : fixture");
  u64 runs = 0, injected = 0, refused = 0;
  bool completed = false;
  for (long long k = 0; k < 4096 && !completed; ++k) {
    const int fds = open_descriptors();
    std::vector<ClusterParams> list(3);  // contenu prealable : vide sur refus
    Outcome o;
    bool escaped = false;
    t_countdown = k;
    try {
      o = read_configs(cfg, list);
    } catch (...) {
      escaped = true;
    }
    const bool hit = t_countdown == -1;  // la (k + 1)-ieme allocation a eu lieu, donc echoue
    t_countdown = -1;
    ++runs;
    injected += hit ? 1 : 0;
    refused += !o.ok();
    const bool content = o.ok() ? list.size() == 2 && list[0].min_cluster_size == 39 && list[0].z == 1.0 &&
                                      list[1].min_cluster_size == 7 && list[1].z == 2.5 &&
                                      list[1].selection == Selection::leaf && list[1].allow_single_cluster
                                : o.reason == Reason::memory_budget && list.empty();
    const bool ok = !escaped && content && open_descriptors() == fds && (hit || o.ok()) && (o.ok() || hit);
    if (!ok)
      std::printf("configs k=%lld injection=%d exception=%d raison=%.*s descripteurs %d/%d\n", k, hit, escaped,
                  static_cast<int>(reason_name(o.reason).size()), reason_name(o.reason).data(), open_descriptors(),
                  fds);
    expect(ok, "configs sous echec d'allocation : memory_budget, liste vide, aucun descripteur perdu");
    if (!hit) completed = true;
  }
  ::unlink(cfg.c_str());
  ::rmdir(dir.c_str());
  std::printf("read_configs essais %llu injections %llu refus_memory_budget %llu sequence_complete %d\n",
              (unsigned long long)runs, (unsigned long long)injected, (unsigned long long)refused, completed ? 1 : 0);
  expect(completed, "configs : lecture complete sans injection atteinte");
  if (!completed || refused < 3) failures += 1000;  // texte, jetons et liste : au moins trois allocations injectees
}

// Publication des sorties (cli::OutputSet, etapes 4 et 5 de son en-tete) sous echec de rename ou de link. Trois sorties
// dans un dossier neuf : A et B preexistantes (« ancienA », « ancienB »), C nouvelle. Chaque cas pose un masque d'echecs
// (i-eme rename, i-eme link depuis le debut du commit), joue commit puis une fin (release, rollback, destruction), et
// juge : le code rendu, le contenu ET l'inode de A et B (un original restaure est le meme fichier qu'avant l'appel),
// la presence de C, les fichiers du dossier apres destruction (aucun temporaire ; une sauvegarde seulement dans l'etat
// de recuperation, et elle porte alors l'original), recovery_needed(), les descripteurs. Avant la reparation : un
// echec du second renommage rendait output_unwritable en laissant A remplace (NEW_A / OLD_B).
ino_t inode_of(const std::string& p) {
  struct stat st {};
  return ::stat(p.c_str(), &st) == 0 ? st.st_ino : 0;
}

void test_output_commit() {
  const char* base = std::getenv("TMPDIR");
  const std::string tmpl = std::string(base != nullptr && *base != '\0' ? base : "/tmp") + "/mhgp10_fault_commit_XXXXXX";
  enum class End { release, rollback, destroy, release_then_rollback, commit_again };
  struct Case {
    const char* name;
    unsigned long long rename_mask, link_mask;  // echecs pendant commit (et sa defaite)
    unsigned long long end_rename_mask;         // echecs pendant la fin (rollback)
    End end;
    bool commit_ok;
    const char* a;  // contenu attendu de A apres destruction : "ancienA", "nouveauA"
    const char* b;
    bool c_present;
    bool a_same_inode, b_same_inode;  // A, B : meme inode qu'avant l'appel
    int backups_left;                 // sauvegardes laissees (etat de recuperation)
    bool recovery;
  };
  const Case cases[] = {
      {"sans echec, release", 0, 0, 0, End::release, true, "nouveauA", "nouveauB", true, false, false, 0, false},
      {"premier rename en echec", 1, 0, 0, End::destroy, false, "ancienA", "ancienB", false, true, true, 0, false},
      {"second rename en echec (auditeur : NEW_A / OLD_B)", 2, 0, 0, End::destroy, false, "ancienA", "ancienB", false,
       true, true, 0, false},
      {"troisieme rename en echec", 4, 0, 0, End::destroy, false, "ancienA", "ancienB", false, true, true, 0, false},
      {"second rename puis restauration de A en echec", 2 | 4, 0, 0, End::destroy, false, "nouveauA", "ancienB", false,
       false, true, 1, true},
      {"sauvegarde de B impossible (link)", 0, 2, 0, End::destroy, false, "ancienA", "ancienB", false, true, true, 0,
       false},
      {"commit puis rollback (sortie standard en echec)", 0, 0, 0, End::rollback, true, "ancienA", "ancienB", false,
       true, true, 0, false},
      {"commit sans release : le destructeur defait", 0, 0, 0, End::destroy, true, "ancienA", "ancienB", false, true,
       true, 0, false},
      {"release puis rollback : sans effet", 0, 0, 0, End::release_then_rollback, true, "nouveauA", "nouveauB", true,
       false, false, 0, false},
      {"rollback, restauration de B en echec", 0, 0, 1, End::rollback, true, "ancienA", "nouveauB", false, true, false,
       1, true},
      {"second commit apres un echec : refuse, rien de publie", 2, 0, 0, End::commit_again, false, "ancienA", "ancienB",
       false, true, true, 0, false},
  };
  u64 ran = 0, restored = 0, recoveries = 0;
  for (const Case& c : cases) {
    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');
    if (::mkdtemp(buf.data()) == nullptr) {
      expect(false, "commit : dossier prive");
      return;
    }
    const std::string dir = buf.data(), A = dir + "/A", B = dir + "/B", C = dir + "/C";
    expect(write_text(A, "ancienA") && write_text(B, "ancienB"), "commit : fixtures");
    const ino_t ino_a = inode_of(A), ino_b = inode_of(B);
    const int fds = open_descriptors();
    bool commit_ok = false, recovery = false, extra_ok = true;
    unsigned long long renames_in_commit = 0;
    {
      cli::OutputSet o;
      auto text = [](const char* t) { return [t](std::FILE* f) { std::fputs(t, f); }; };
      const bool prepared = o.declare(A).ok() && o.declare(B).ok() && o.declare(C).ok() && o.reserve().ok() &&
                            o.write(A, text("nouveauA")).ok() && o.write(B, text("nouveauB")).ok() &&
                            o.write(C, text("nouveauC")).ok();
      expect(prepared, "commit : preparation");
      g_rename_calls = 0;
      g_link_calls = 0;
      g_rename_mask = c.rename_mask;
      g_link_mask = c.link_mask;
      commit_ok = o.commit().ok();
      renames_in_commit = g_rename_calls.load();
      g_rename_mask = 0;
      g_link_mask = 0;
      if (c.end == End::commit_again) {
        extra_ok = o.commit().reason == Reason::output_unwritable && read_text(A) == "ancienA" &&
                   read_text(B) == "ancienB" && read_text(C) == "ABSENT";
      } else if (c.end == End::release || c.end == End::release_then_rollback) {
        o.release();
        if (c.end == End::release_then_rollback) extra_ok = o.rollback().ok();
        extra_ok = extra_ok && o.commit().ok();  // idempotent apres publication : rien a refaire
      } else if (c.end == End::rollback) {
        g_rename_calls = 0;
        g_rename_mask = c.end_rename_mask;
        const Outcome back = o.rollback();
        g_rename_mask = 0;
        extra_ok = back.ok() == (c.end_rename_mask == 0) && o.commit().reason == Reason::output_unwritable;
      }
      recovery = o.recovery_needed();
    }
    int backups = 0, temps = 0;
    std::string backup_text;
    ino_t backup_ino = 0;
    for (const std::string& n : dir_entries(dir)) {
      if (n.size() > 4 && n.compare(n.size() - 4, 4, ".bak") == 0) {
        ++backups;
        backup_text = read_text(dir + "/" + n);
        backup_ino = inode_of(dir + "/" + n);
      }
      if (n.size() > 4 && n.compare(n.size() - 4, 4, ".tmp") == 0) ++temps;
    }
    // etat de recuperation : la sauvegarde laissee porte l'original (contenu et inode) du fichier non restaure
    const bool backup_holds_original =
        backups == 0 || (backups == 1 && ((std::string(c.a) == "nouveauA" && backup_text == "ancienA" && backup_ino == ino_a) ||
                                          (std::string(c.b) == "nouveauB" && backup_text == "ancienB" && backup_ino == ino_b)));
    const bool ok = commit_ok == c.commit_ok && extra_ok && read_text(A) == c.a && read_text(B) == c.b &&
                    (read_text(C) == "nouveauC") == c.c_present && (c.c_present || read_text(C) == "ABSENT") &&
                    (inode_of(A) == ino_a) == c.a_same_inode && (inode_of(B) == ino_b) == c.b_same_inode &&
                    backups == c.backups_left && temps == 0 && backup_holds_original && recovery == c.recovery &&
                    open_descriptors() == fds && (c.link_mask == 0 || renames_in_commit == 0);
    if (!ok)
      std::printf("commit [%s] commit=%d fin=%d A=%s B=%s C=%s inodes %d %d sauvegardes %d temporaires %d "
                  "recuperation=%d descripteurs %d/%d renames %llu\n",
                  c.name, commit_ok, extra_ok, read_text(A).c_str(), read_text(B).c_str(), read_text(C).c_str(),
                  inode_of(A) == ino_a, inode_of(B) == ino_b, backups, temps, recovery, open_descriptors(), fds,
                  renames_in_commit);
    expect(ok, c.name);
    ++ran;
    restored += !c.commit_ok || c.end == End::rollback || c.end == End::destroy ? (inode_of(A) == ino_a) : 0;
    recoveries += recovery;
    for (const std::string& n : dir_entries(dir)) ::unlink((dir + "/" + n).c_str());
    ::rmdir(dir.c_str());
  }
  std::printf("output_commit cas %llu originaux_restaures %llu etats_de_recuperation %llu\n", (unsigned long long)ran,
              (unsigned long long)restored, (unsigned long long)recoveries);
  if (ran != sizeof cases / sizeof cases[0] || restored < 6 || recoveries != 2) failures += 1000;
}

// Aides de preparation des sondes (cloud/u32le_input.hpp, core/cli_options.hpp, head.hpp) sous echec d'allocation.
// La (k + 1)-ieme allocation du fil echoue, k = 0, 1, ... jusqu'au premier appel sans injection : chaque refus est
// memory_budget, aucune exception ne sort, aucun descripteur n'est perdu ; l'appel complet rend la valeur attendue.
// Avant : read_u32le_cloud convertissait son chemin en std::filesystem::path avant son premier try, et
// parse_integer_list allouait sa liste sans interception (quatre exceptions echappees de la sonde de l'auditeur).
// parse_z (chaine C) et parse_integer ne font aucune allocation : comptees, zero. cli::guarded : std::bad_alloc
// devient le refus memory_budget, toute autre exception sort inchangee.
void test_cli_helpers() {
  const char* base = std::getenv("TMPDIR");
  std::string tmpl = std::string(base != nullptr && *base != '\0' ? base : "/tmp") + "/mhgp10_fault_aides_XXXXXX";
  std::vector<char> buf(tmpl.begin(), tmpl.end());
  buf.push_back('\0');
  if (::mkdtemp(buf.data()) == nullptr) {
    expect(false, "aides : dossier prive");
    return;
  }
  const std::string dir = buf.data(), cloud = dir + "/nuage.u32le";
  const u32 points = 6000;  // 72 000 octets : plusieurs tranches de lecture
  {
    std::string bytes(12 * size_t(points), '\0');
    for (u32 i = 0; i < points; ++i) {
      const u32 xyz[3] = {i, 2 * i + 1, 7};
      std::memcpy(&bytes[12 * size_t(i)], xyz, 12);
    }
    std::FILE* f = std::fopen(cloud.c_str(), "wb");
    const bool wrote = f != nullptr && std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size();
    expect(f != nullptr && std::fclose(f) == 0 && wrote, "aides : fixture");
  }
  u64 reader_runs = 0, reader_refused = 0, list_runs = 0, list_refused = 0;
  bool completed = false;
  for (long long k = 0; k < 4096 && !completed; ++k) {
    const int fds = open_descriptors();
    bool escaped = false, good = false;
    Reason reason = Reason::none;
    t_countdown = k;
    try {
      auto r = read_u32le_cloud(cloud.c_str());
      reason = r.outcome().reason;
      good = r.ok() && r.value().x.size() == points && r.value().y[5] == 11 && r.value().pid[points - 1] == points - 1;
    } catch (...) {
      escaped = true;
    }
    const bool hit = t_countdown == -1;
    t_countdown = -1;
    ++reader_runs;
    reader_refused += reason == Reason::memory_budget;
    const bool ok = !escaped && open_descriptors() == fds && (hit ? reason == Reason::memory_budget : good);
    if (!ok) std::printf("aides lecteur k=%lld injection=%d exception=%d descripteurs %d/%d\n", k, hit, escaped,
                         open_descriptors(), fds);
    expect(ok, "lecteur u32le sous echec d'allocation : memory_budget, aucune exception, aucun descripteur perdu");
    if (!hit) completed = true;
  }
  expect(completed, "lecteur u32le : lecture complete sans injection atteinte");
  std::string text;
  for (int i = 1; i <= 300; ++i) text += (i > 1 ? "," : "") + std::to_string(i);
  bool list_completed = false;
  for (long long k = 0; k < 4096 && !list_completed; ++k) {
    bool escaped = false, good = false;
    Reason reason = Reason::none;
    t_countdown = k;
    try {
      auto r = cli::parse_integer_list<int>(text, 0, 1000);
      reason = r.outcome().reason;
      good = r.ok() && r.value().size() == 300 && r.value().front() == 1 && r.value().back() == 300;
    } catch (...) {
      escaped = true;
    }
    const bool hit = t_countdown == -1;
    t_countdown = -1;
    ++list_runs;
    list_refused += reason == Reason::memory_budget;
    expect(!escaped && (hit ? reason == Reason::memory_budget : good),
           "parse_integer_list sous echec d'allocation : memory_budget, aucune exception");
    if (!hit) list_completed = true;
  }
  expect(list_completed, "parse_integer_list : lecture complete sans injection atteinte");
  // aides sans allocation : comptees
  g_count = 0;
  g_counting = true;
  double z = -1;
  const bool z_ok = parse_z("2.5e-1", z) && z == 0.25 && !parse_z("abc", z) && !parse_z("", z) && z == 0.25;
  const auto one = cli::parse_integer<u64>("18446744073709551615", 0, ~u64{0});
  g_counting = false;
  const unsigned long long counted = g_count.load();
  g_count = 0;
  expect(z_ok && one.ok() && one.value() == ~u64{0} && counted == 0, "parse_z et parse_integer : aucune allocation");
  // garde de haut niveau : std::bad_alloc et elle seule
  int refused_with = -1;
  const int code = cli::guarded([]() -> int { throw std::bad_alloc(); }, [&](const Outcome& o) {
    refused_with = o.reason == Reason::memory_budget && o.status() == Status::resource_exhausted ? 1 : 0;
    return 2;
  });
  bool other_escaped = false;
  try {
    (void)cli::guarded([]() -> int { throw std::runtime_error("autre"); }, [](const Outcome&) { return 2; });
  } catch (const std::runtime_error&) {
    other_escaped = true;
  }
  const int passed = cli::guarded([] { return 7; }, [](const Outcome&) { return 2; });
  expect(code == 2 && refused_with == 1 && other_escaped && passed == 7,
         "guarded : bad_alloc -> memory_budget, autre exception inchangee, code rendu tel quel");
  ::unlink(cloud.c_str());
  ::rmdir(dir.c_str());
  std::printf("cli_helpers lecteur essais %llu refus_memory_budget %llu liste essais %llu refus_memory_budget %llu "
              "allocations_parse_z_et_parse_integer %llu\n",
              (unsigned long long)reader_runs, (unsigned long long)reader_refused, (unsigned long long)list_runs,
              (unsigned long long)list_refused, counted);
  // lecteur : octets lus (reserve), tranche, quatre tableaux ; liste : croissances du vecteur de 300 entiers
  if (!completed || !list_completed || reader_refused < 6 || list_refused < 8) failures += 1000;
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
  if (group.empty() || group == "thread_creation") test_thread_creation();
  if (group.empty() || group == "output_set") test_output_set();
  if (group.empty() || group == "read_configs") test_read_configs();
  if (group.empty() || group == "cli_helpers") test_cli_helpers();
  if (group.empty() || group == "output_commit") test_output_commit();
  if (!group.empty() && group != "pool_construction" && group != "entry_points" && group != "aligned_forms" &&
      group != "thread_creation" && group != "output_set" && group != "read_configs" && group != "cli_helpers" &&
      group != "output_commit") {
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
