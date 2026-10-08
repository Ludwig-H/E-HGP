// Porte de la Session recouverte (build_tower) sous penurie de memoire injectee, hors produit : les operateurs new sont
// remplaces dans ce seul executable, toutes formes appariees (tests/sched/fault.cpp).
//   allocation : sans injection, build_tower appelle exactement TROIS fois l'operateur new qui leve (SessionRun dans
//                build_tower, puis BuildState et Pipeline dans open_session, qui n'est donc pas noexcept : relecture de
//                l'auditeur Codex, receipts/audit_reponses_20261008/prelecture_t2d_corps) ; la k-ieme leve
//                std::bad_alloc tour a tour : refus memory_budget, budget rendu, aucune terminaison ; puis le temoin
//                reussit avec la meme empreinte FUL1 ;
//   penurie    : la k-ieme allocation sans exception (tampons du budget, espaces de census) echoue tour a tour pendant
//                build_tower, a un fil (ordre des allocations fixe : toutes) et a trois fils (un echantillon) : refus
//                memory_budget ou succes au-dela de la derniere, budget rendu, aucune terminaison ni attente sans fin.
#include <atomic>
#include <cstdlib>
#include <new>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "catalogue/catalogue.hpp"
#include "index/index.hpp"
#include "io/io.hpp"
#include "sched/sched.hpp"
#include "test.hpp"
#include "tower/tower.hpp"

namespace {

std::atomic<long long> throwing_left{-1}, nothrow_left{-1};  // >= 0 : allocations encore admises avant un echec
std::atomic<unsigned long long> throwing_calls{0}, nothrow_calls{0};

bool refuse(std::atomic<long long>& left) noexcept {
  return left.load() >= 0 && left.fetch_sub(1) == 0;
}

}  // namespace

[[gnu::noinline]] void* operator new(std::size_t size) {
  throwing_calls.fetch_add(1);
  if (refuse(throwing_left)) throw std::bad_alloc();
  if (void* block = std::malloc(size == 0 ? 1 : size)) return block;
  throw std::bad_alloc();
}
[[gnu::noinline]] void* operator new[](std::size_t size) { return ::operator new(size); }
[[gnu::noinline]] void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
  nothrow_calls.fetch_add(1);
  if (refuse(nothrow_left)) return nullptr;
  return std::malloc(size == 0 ? 1 : size);
}
[[gnu::noinline]] void* operator new[](std::size_t size, const std::nothrow_t& tag) noexcept {
  return ::operator new(size, tag);
}
[[gnu::noinline]] void operator delete(void* block) noexcept { std::free(block); }
[[gnu::noinline]] void operator delete(void* block, std::size_t) noexcept { std::free(block); }
[[gnu::noinline]] void operator delete(void* block, const std::nothrow_t&) noexcept { std::free(block); }
[[gnu::noinline]] void operator delete[](void* block) noexcept { std::free(block); }
[[gnu::noinline]] void operator delete[](void* block, std::size_t) noexcept { std::free(block); }
[[gnu::noinline]] void operator delete[](void* block, const std::nothrow_t&) noexcept { std::free(block); }

namespace mhgp12 {
namespace {

std::unique_ptr<sched::Pool> pool_of(u32 workers) {
  auto made = sched::make_pool(sched::PoolParams{workers});
  if (!made.ok()) std::terminate();
  return std::move(made).take();
}

// Nuage aleatoire de n sites distincts, index et catalogue a K (memoire du budget donne).
struct Chain {
  std::optional<GlobalIndex> index;
  std::optional<Catalogue> catalogue;
};
Chain make_chain(u32 n, int kmax, u64 seed, MemoryBudget& budget, sched::Pool& pool) {
  std::mt19937_64 rng(seed);
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  while (x.size() < n) {
    const u32 a = static_cast<u32>(rng() % 4096), b = static_cast<u32>(rng() % 4096), c = static_cast<u32>(rng() % 4096);
    bool seen = false;
    for (std::size_t i = 0; i < x.size() && !seen; ++i) seen = x[i] == a && y[i] == b && z[i] == c;
    if (seen) continue;
    x.push_back(a);
    y.push_back(b);
    z.push_back(c);
    ids.push_back(make_id<PointId>(static_cast<u32>(ids.size())));
  }
  Chain out;
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth(), budget);
  if (!cloud.ok()) return out;
  auto index = build_index(std::move(cloud).take(), IndexParams{}, budget);
  if (!index.ok()) return out;
  out.index.emplace(std::move(index).take());
  CatalogueParams params;
  params.kmax = kmax;
  auto catalogue = build_catalogue(out.index->cloud(), params, budget, pool);
  if (catalogue.ok()) out.catalogue.emplace(std::move(catalogue).take());
  return out;
}

std::string digest_of(const Chain& c, const Tower& tower) {
  const tower::FullSource source{&c.index->cloud(), c.catalogue->levels(), tower::catalogue_balls(*c.catalogue),
                                 &tower.forests};
  auto d = tower::full_digest(source);
  if (!d.ok()) return "refus";
  const auto hex = io::to_hex(d.value());
  return std::string(hex.data(), hex.size());
}

// Une Session sous injection ; rend son issue (et l'empreinte au succes) et compte les new qui levent et sans exception
// appeles par build_tower SEUL (l'empreinte, calculee ensuite hors injection, alloue aussi). Le budget doit revenir a
// base.
struct Calls {
  unsigned long long throwing = 0, nothrow = 0;
};
Outcome attempt(const Chain& c, MemoryBudget& budget, sched::Pool& pool, std::string* digest, Calls* calls = nullptr) {
  Outcome out;
  {
    const Calls before{throwing_calls.load(), nothrow_calls.load()};
    auto tower = build_tower(*c.index, *c.catalogue, budget, pool);
    throwing_left.store(-1);
    nothrow_left.store(-1);
    if (calls != nullptr) *calls = Calls{throwing_calls.load() - before.throwing, nothrow_calls.load() - before.nothrow};
    if (tower.ok() && digest != nullptr) *digest = digest_of(c, tower.value());
    out = tower.ok() ? Outcome{} : tower.outcome();
  }
  return out;
}

MHGP12_TEST(allocation, 12) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(3);
  const Chain c = make_chain(70, 4, 5, budget, *pool);
  REQUIRE(c.catalogue.has_value());
  const u64 base = budget.used();
  std::string expected;
  Calls calls;
  REQUIRE(attempt(c, budget, *pool, &expected, &calls).ok());
  const unsigned long long throwing = calls.throwing;
  CHECK_EQ(throwing, 3ull);  // SessionRun, BuildState, Pipeline
  CHECK_EQ(budget.used(), base);
  for (unsigned long long k = 0; k < throwing; ++k) {
    throwing_left.store(static_cast<long long>(k));
    const Outcome o = attempt(c, budget, *pool, nullptr);
    CHECK_EQ(o.reason, Reason::memory_budget);
    CHECK_EQ(budget.used(), base);  // tampons de l'etage G et de la foret rendus en deroulant
  }
  std::string again;
  CHECK(attempt(c, budget, *pool, &again).ok());
  CHECK(again == expected);
}

// Balayage des allocations sans exception de build_tower (toutes, ou un echantillon regulier) ; rend les refus.
u64 sweep(const Chain& c, MemoryBudget& budget, sched::Pool& pool, unsigned long long total, u64 step) {
  const u64 base = budget.used();
  u64 refusals = 0;
  for (unsigned long long k = 0; k < total; k += step) {
    nothrow_left.store(static_cast<long long>(k));
    const Outcome o = attempt(c, budget, pool, nullptr);
    if (!o.ok()) {
      CHECK_EQ(o.reason, Reason::memory_budget);
      ++refusals;
    }
    CHECK_EQ(budget.used(), base);
  }
  return refusals;
}

MHGP12_TEST(penurie, 200) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto single = pool_of(1);
  auto three = pool_of(3);
  const Chain c = make_chain(60, 4, 9, budget, *three);
  REQUIRE(c.catalogue.has_value());
  std::string expected;
  Calls calls;
  REQUIRE(attempt(c, budget, *single, &expected, &calls).ok());
  const unsigned long long total = calls.nothrow;
  CHECK(total >= 100);
  // Un fil : l'ordre des allocations est fixe ; chacune, tour a tour, rend un refus memory_budget.
  CHECK_EQ(sweep(c, budget, *single, total, 1), total);
  // Trois fils : un echantillon (l'allocation visee depend de l'entrelacement) ; refus ou succes, jamais de reste.
  sweep(c, budget, *three, total, std::max<u64>(1, total / 40));
  std::string again;
  CHECK(attempt(c, budget, *three, &again).ok());
  CHECK(again == expected);
  std::printf("penurie allocations=%llu\n", total);
}

}  // namespace
}  // namespace mhgp12

MHGP12_TEST_MAIN()
