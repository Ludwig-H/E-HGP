// Refus sous budget serre de la voie appareil (tranche T2-d, prelecture de l'auditeur Codex du 8 octobre ; budget de
// l'appareil revu par la tranche T1-d : sous son pic, la voie appareil passe en flux et par tranches) : les
// sorties hote anticipees vivent des le dernier lot, la memoire de transit suit les demandes du flux ; le pic de chaque
// budget change, et un petit budget peut refuser a un autre endroit qu'avant T2-d. Ce qui ne change pas : un refus est
// entier (memory_budget, rien de publie), aucune reservation ne depasse la limite, tout est rendu a la destruction, et
// le contexte sert encore apres un refus. Budgets separes comme CatalogueDevice::open(budget, device) : tableaux de
// l'appareil d'un cote, memoire de transit, sorties et reprises de l'hote de l'autre.
//   pipeline_budget    : transit simule (StagedExecutor, comptes de l'executeur CUDA), trois fils (les reservations
//                        sont faites par le fil pilote, tailles fixees par le nombre de fils : sequence deterministe) ;
//                        pic de chaque budget mesure sur un appel reussi, puis limites
//                        serrees : au pic, catalogue identique a la voie CPU ; en dessous, refus memory_budget ;
//   device_open_budget : la vraie voie appareil (session G4) : memes limites ; sans GPU, appareil indisponible.
#include "device_support.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::catalogue_detail;
using namespace mhgp12::device_test;

namespace {

struct BudgetCase {
  const char* name;
  Points points;
  int k;
  u32 leaf;
};

// Feuilles rejouees sur l'hote (coquille de 48 sites, K5) et nuage de 2 500 sites (plusieurs tranches de flux).
std::vector<BudgetCase> budget_cases() {
  return {{"coquille48", shell({1, 2, 3}, 1u << 18, 3u << 18), 5, 24}, {"nuage2500", scatter(2500, 65535, 7), 5, 24}};
}

// Issue d'un appel sous deux limites : succes identique a la reference, ou refus memory_budget ; pics ; limites
// tenues ; budgets entierement rendus une fois l'etat, l'executeur et le catalogue detruits.
struct Trial {
  bool ok = false, refused = false, same = false, within = false, released = false;
  u64 host_peak = 0, device_peak = 0, slices = 0, streamed = 0;
};

// Limite de l'un des deux budgets (l'autre illimite) : side 0 = hote, 1 = appareil.
struct Limits {
  u64 host, device;
};
Limits limit_on(int side, u64 limit) {
  return side == 0 ? Limits{limit, MemoryBudget::kUnlimited} : Limits{MemoryBudget::kUnlimited, limit};
}

Trial staged_trial(const Cloud& cloud, const CatalogueParams& params, sched::Pool& pool, const Catalogue& ref,
                   Limits limits) {
  Trial t;
  MemoryBudget host(limits.host), device(limits.device);
  {
    CatalogueDiagnostics diag;
    auto got = staged_device(cloud, params, pool, host, device, 4096, 2, &diag);
    t.ok = got.ok();
    t.slices = diag.finish_slices;
    t.streamed = diag.arena_streamed;
    t.refused = !got.ok() && got.outcome().reason == Reason::memory_budget;
    t.same = got.ok() && same(cloud, ref, got.value());
    t.host_peak = host.peak();
    t.device_peak = device.peak();
    t.within = host.peak() <= limits.host && device.peak() <= limits.device;
  }
  t.released = host.released().ok() && device.released().ok();
  return t;
}

// Limites essayees sous un pic : un octet de moins, trois quarts, moitie, un huitieme, 4 Kio, zero.
std::vector<u64> below(u64 peak) {
  return {peak - 1, peak - peak / 4, peak / 2, peak / 8, u64{4096}, u64{0}};
}

}  // namespace

MHGP12_TEST(pipeline_budget, 40) {
  MemoryBudget budget(MemoryBudget::kUnlimited);  // nuages et references
  auto pool = sched::make_pool({3});
  REQUIRE(pool.ok());
  for (const BudgetCase& c : budget_cases()) {
    auto cloud = make_cloud(c.points, budget);
    REQUIRE(cloud.ok());
    const CatalogueParams params = params_of(c.k, c.leaf);
    auto ref = cpu(cloud.value(), params, 1, budget);
    REQUIRE(ref.ok());
    const Limits none{MemoryBudget::kUnlimited, MemoryBudget::kUnlimited};
    const Trial free = staged_trial(cloud.value(), params, *pool.value(), ref.value(), none);
    REQUIRE(free.ok && free.same && free.released);
    REQUIRE(free.host_peak > 0 && free.device_peak > 0);
    // Hote : au pic exact, identique ; en dessous, refus (sequence des reservations deterministe).
    const Trial at = staged_trial(cloud.value(), params, *pool.value(), ref.value(), limit_on(0, free.host_peak));
    CHECK(at.ok && at.same && at.within && at.released);
    CHECK_EQ(at.host_peak, free.host_peak);
    for (const u64 limit : below(free.host_peak)) {
      const Trial t = staged_trial(cloud.value(), params, *pool.value(), ref.value(), limit_on(0, limit));
      if (!CHECK(t.refused && t.within && t.released))
        std::fprintf(stderr, "budget serre de l'hote : %s, limite %llu\n", c.name,
                     static_cast<unsigned long long>(limit));
    }
    // Appareil (tranche T1-d) : sous le pic, etat du parcours rendu avant la fin d'etage, arene en flux, fin d'etage
    // par tranches : le meme catalogue tant que la memoire de travail minimale tient, sinon refus memory_budget ;
    // jamais au-dela de la limite, tout rendu. Un succes sous le pic est une adaptation (l'une des trois).
    u64 adapted = 0;
    for (const u64 limit : below(free.device_peak)) {
      const Trial t = staged_trial(cloud.value(), params, *pool.value(), ref.value(), limit_on(1, limit));
      if (!CHECK(((t.ok && t.same) || t.refused) && t.within && t.released))
        std::fprintf(stderr, "budget de l'appareil : %s, limite %llu\n", c.name,
                     static_cast<unsigned long long>(limit));
      if (t.ok) ++adapted;
      if (limit <= 4096) CHECK(t.refused);
    }
    CHECK(adapted >= 1);
  }
}

// Contexte reutilisable apres un refus : sur le meme executeur et le meme etat, un refus de l'hote au pic moins un
// octet, puis le petit temoin du carre, identique a la voie CPU ; tout est rendu a la destruction.
MHGP12_TEST(pipeline_budget_reuse, 6) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = sched::make_pool({1});
  REQUIRE(pool.ok());
  auto big = make_cloud(scatter(2500, 65535, 7), budget);
  auto small = make_cloud({{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}}, budget);
  REQUIRE(big.ok() && small.ok());
  const CatalogueParams big_params = params_of(5, 24), small_params = params_of(2, 5);
  auto big_ref = cpu(big.value(), big_params, 1, budget);
  auto small_ref = cpu(small.value(), small_params, 1, budget);
  REQUIRE(big_ref.ok() && small_ref.ok());
  const Limits none{MemoryBudget::kUnlimited, MemoryBudget::kUnlimited};
  const Trial free = staged_trial(big.value(), big_params, *pool.value(), big_ref.value(), none);
  REQUIRE(free.ok && free.host_peak > 0);
  MemoryBudget host(free.host_peak - 1), device(MemoryBudget::kUnlimited);
  {
    StagedExecutor executor{{*pool.value(), device}};
    executor.host = &host;
    executor.slot_size = 4096;
    dev::DeviceState<StagedExecutor> state;
    CatalogueDiagnostics diag;
    auto refused = guarded([&]() {
      return dev::device_catalogue(executor, state, big.value(), big_params, host, *pool.value(), diag);
    });
    CHECK(!refused.ok() && refused.outcome().reason == Reason::memory_budget);
    auto again = guarded([&]() {
      return dev::device_catalogue(executor, state, small.value(), small_params, host, *pool.value(), diag);
    });
    REQUIRE(again.ok());
    CHECK(same(small.value(), small_ref.value(), again.value()));
    CHECK(host.peak() <= host.limit());
  }
  CHECK(host.released().ok() && device.released().ok());
}

// Vraie voie appareil (session G4) sous budget serre : hote de 512 Kio (sous la memoire de transit minimale de 1 Mio),
// appareil de 1 Kio (sous le premier tableau) : refus memory_budget ; puis, par budget, pic d'un appel reussi sur un
// contexte neuf, et deux contextes neufs : au pic, catalogue identique a la voie CPU ; un octet de moins, refus. Les
// budgets sont rendus a la destruction de chaque contexte. Sans GPU : appareil indisponible, rien n'est joue.
namespace {

Trial device_trial(const Cloud& cloud, const CatalogueParams& params, sched::Pool& pool, const Catalogue& ref,
                   Limits limits) {
  Trial t;
  MemoryBudget host(limits.host), device_budget(limits.device);
  {
    auto device = CatalogueDevice::open(host, device_budget);
    if (!device.ok()) return t;
    auto got = build_catalogue_device(cloud, params, device.value(), pool);
    t.ok = got.ok();
    t.refused = !got.ok() && got.outcome().reason == Reason::memory_budget;
    t.same = got.ok() && same(cloud, ref, got.value());
    t.host_peak = host.peak();
    t.device_peak = device_budget.peak();
    t.within = host.peak() <= limits.host && device_budget.peak() <= limits.device;
  }
  t.released = host.released().ok() && device_budget.released().ok();
  return t;
}

}  // namespace

MHGP12_TEST(device_open_budget, 1) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  {
    auto probe = CatalogueDevice::open(budget);
    if (!probe.ok()) {
      CHECK(probe.outcome().reason == Reason::device_unavailable);
      std::printf("device_open_budget : appareil indisponible (device_unavailable), refus non joues\n");
      return;
    }
  }
  auto pool = sched::make_pool({4});
  REQUIRE(pool.ok());
  u64 trials = 0;
  for (const BudgetCase& c : budget_cases()) {
    auto cloud = make_cloud(c.points, budget);
    REQUIRE(cloud.ok());
    const CatalogueParams params = params_of(c.k, c.leaf);
    auto ref = cpu(cloud.value(), params, 4, budget);
    REQUIRE(ref.ok());
    const Trial host_small = device_trial(cloud.value(), params, *pool.value(), ref.value(), limit_on(0, 512 << 10));
    const Trial device_small = device_trial(cloud.value(), params, *pool.value(), ref.value(), limit_on(1, 1024));
    CHECK(host_small.refused && host_small.within && host_small.released);
    CHECK(device_small.refused && device_small.within && device_small.released);
    const Limits none{MemoryBudget::kUnlimited, MemoryBudget::kUnlimited};
    const Trial free = device_trial(cloud.value(), params, *pool.value(), ref.value(), none);
    REQUIRE(free.ok && free.same && free.released);
    for (int side = 0; side < 2; ++side) {
      const u64 peak = side == 0 ? free.host_peak : free.device_peak;
      const Trial at = device_trial(cloud.value(), params, *pool.value(), ref.value(), limit_on(side, peak));
      const Trial under = device_trial(cloud.value(), params, *pool.value(), ref.value(), limit_on(side, peak - 1));
      // hote : un octet de moins refuse ; appareil (T1-d) : arene en flux et tranches, ou refus memory_budget
      const bool below_ok = side == 0 ? under.refused : (under.ok && under.same) || under.refused;
      if (!CHECK(at.ok && at.same && at.within && at.released && below_ok && under.within && under.released))
        std::fprintf(stderr, "budget appareil : %s, cote %d, pic %llu\n", c.name, side,
                     static_cast<unsigned long long>(peak));
      trials += 2;
    }
    trials += 3;
  }
  std::printf("device_open_budget : refus sous budget serre joues sur l'appareil (%llu appels)\n",
              static_cast<unsigned long long>(trials));
}
