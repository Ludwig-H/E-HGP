// Porte de l'ordre des gardes du parcours (microbanc MES-M5, hors produit) : apres la lecture des totaux d'un niveau,
// les gardes wide_leaf et capacite passent AVANT toute reservation de tampon, tout noyau et toute allocation de l'hote,
// diagnostic par niveau (stats) compris (observation de l'auditeur, recu audit_juges_emst_20261007/m5).
//
// Le vrai Driver::run est joue sur un executeur factice, sur le modele de la sonde de l'auditeur (m5/probe.cpp) : les
// totaux de chaque niveau sont fabriques ; les niveaux avant la profondeur d sont admis et petits ; au niveau d, les
// totaux depassent la capacite (parents ou taches) ou la feuille maximale. Ce programme remplace l'operateur new global
// pour compter les allocations de l'hote faites apres la lecture des totaux du niveau d. Attendus : statut capacite ou
// wide_leaf, aucun prefixe publie, zero reservation, zero noyau et zero allocation apres ces totaux, pour d = 0, 1, 2,
// 4, 8, 16, 32 (profondeurs ou un vecteur qui double sa capacite realloue). Temoins vivants : un niveau admis atteint
// bien la reservation suivante, et le compteur voit une allocation faite expres.
//
// Usage : mhgp12_traversal_driver_selftest
// Sortie : une ligne JSON par cas, puis un bilan. Codes : 0 conforme, 1 un cas differe de l'attendu.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <string>
#include <vector>

#include "mhgp12/traversal/driver.hpp"

namespace {

bool g_count = false;
unsigned long long g_allocations = 0;

}  // namespace

// Remplacements globaux hors ligne (noinline) : sinon GCC rapproche new et free une fois inlines et signale un faux
// desaccord (-Wmismatched-new-delete).
[[gnu::noinline]] void* operator new(std::size_t n) {
  if (g_count) ++g_allocations;
  if (void* p = std::malloc(n == 0 ? 1 : n)) return p;
  throw std::bad_alloc();
}
[[gnu::noinline]] void* operator new[](std::size_t n) { return ::operator new(n); }
[[gnu::noinline]] void operator delete(void* p) noexcept { std::free(p); }
[[gnu::noinline]] void operator delete[](void* p) noexcept { std::free(p); }
[[gnu::noinline]] void operator delete(void* p, std::size_t) noexcept { std::free(p); }
[[gnu::noinline]] void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {

namespace t = mhgp12::traversal;
namespace b = t::bfs;
using b::u32;
using b::u64;

struct ReservationReached {};

// Executeur factice : tampons de l'hote, noyaux sans effet, totaux fabriques niveau par niveau.
struct Backend {
  template <class T>
  struct Array {
    std::vector<T> v;
    T* data() { return v.data(); }
  };
  std::vector<b::LevelTotals> script;
  u64 level = 0, target = 0;
  bool armed = false;
  unsigned reservations_after = 0, launches_after = 0;

  template <class T>
  u64 ensure(Array<T>& a, u64 n) {
    if (armed) {
      ++reservations_after;
      throw ReservationReached{};
    }
    if (a.v.size() < n) a.v.resize(n);
    return 1;
  }
  template <class T>
  u64 ensure_keep(Array<T>& a, u64 n, u64) {
    return ensure(a, n);
  }
  template <class T>
  u64 upload(Array<T>& a, const T* src, u64 n) {
    const u64 made = ensure(a, n);
    if (n != 0) std::memcpy(a.data(), src, n * sizeof(T));
    return made;
  }
  template <class K>
  void launch(const K&, u64) {
    if (armed) ++launches_after;
  }
  void mark(int, u32) {}
  b::LevelTotals read_totals(Array<b::LevelTotals>&) {
    const b::LevelTotals totals = script[level];
    if (level == target) {
      armed = true;
      g_count = true;
    }
    ++level;
    return totals;
  }
};

b::LevelTotals small_level() {
  b::LevelTotals s{};
  s.f[0] = 1;  // un parent suivant
  s.f[1] = 3;  // sites de sa liste
  s.f[2] = 1;  // une tache
  return s;
}

struct Outcome {
  u64 status = 0;
  bool reached = false, empty = false;
  unsigned reservations = 0, launches = 0;
  unsigned long long allocations = 0;
};

Outcome play(u64 depth, const b::LevelTotals& last) {
  Backend backend;
  backend.script.assign(depth, small_level());
  backend.script.push_back(last);
  backend.target = depth;
  t::Driver<Backend, b::kNone> driver(backend, b::Params{});
  driver.n_sites = 1;
  const u32 zero = 0;
  t::NoHook hook;
  Outcome o;
  g_allocations = 0;
  try {
    const t::RunResult r = driver.run(&zero, &zero, &zero, hook);
    g_count = false;
    o.status = r.status;
    o.empty = r.ledger == t::Ledger{} && r.n_leaves == 0 && r.n_leaf_sites == 0;
  } catch (const ReservationReached&) {
    g_count = false;
    o.reached = true;
  }
  o.reservations = backend.reservations_after;
  o.launches = backend.launches_after;
  o.allocations = g_allocations;
  return o;
}

}  // namespace

int main() {
  int cases = 0, failures = 0;
  auto report = [&](const std::string& name, bool ok, const Outcome& o) {
    ++cases;
    if (!ok) ++failures;
    std::cout << "{\"cas\":\"" << name << "\",\"conforme\":" << (ok ? "true" : "false") << ",\"statut\":" << o.status
              << ",\"reservations_apres_totaux\":" << o.reservations << ",\"noyaux_apres_totaux\":" << o.launches
              << ",\"allocations_hote_apres_totaux\":" << o.allocations << "}\n";
  };
  // Temoin du compteur : une allocation faite expres est vue (appel direct, qu'aucun compilateur n'elide).
  g_allocations = 0;
  g_count = true;
  void* volatile probe = ::operator new(64);
  g_count = false;
  ::operator delete(probe);
  Outcome alive;
  alive.allocations = g_allocations;
  report("compteur_d_allocations_vivant", g_allocations == 1, alive);
  const u64 depths[] = {0, 1, 2, 4, 8, 16, 32};
  for (u64 d : depths) {
    b::LevelTotals parents = small_level(), tasks = small_level(), wide = small_level();
    parents.f[0] = 0x80000000ull;  // 2^31 parents suivants : au-dela des indices de 31 bits
    tasks.f[2] = 0x100000000ull;   // 2^32 taches suivantes : au-dela de u32
    wide.max_leaf = 257;           // feuille au-dela de max_leaf = 256
    const struct {
      const char* name;
      b::LevelTotals totals;
      u64 status;
    } refusals[] = {{"parents", parents, t::kStatusCapacity},
                    {"taches", tasks, t::kStatusCapacity},
                    {"feuille_large", wide, t::kStatusWideLeaf}};
    for (const auto& r : refusals) {
      const Outcome o = play(d, r.totals);
      const bool ok = !o.reached && o.status == r.status && o.empty && o.reservations == 0 && o.launches == 0 &&
                      o.allocations == 0;
      report(std::string("refus_") + r.name + "_profondeur_" + std::to_string(d), ok, o);
    }
    // Temoin admis : apres des totaux admissibles, la reservation suivante est bien atteinte (et interceptee).
    const Outcome admitted = play(d, small_level());
    report("admis_profondeur_" + std::to_string(d), admitted.reached && admitted.reservations == 1, admitted);
  }
  std::cout << "{\"porte\":\"gardes_du_parcours_m5\",\"cas\":" << cases << ",\"ecarts\":" << failures << "}\n";
  return failures == 0 ? 0 : 1;
}
