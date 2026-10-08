// Catalogue en flux (tranche T1-d) : fin d'etage par tranches de cles et arene en flux, contre la voie complete.
//   slices_identity      : arene de la voie CPU, voie par tranches sur l'executeur Pool avec une memoire de travail qui
//                          force au moins 1, 2, 7 et beaucoup de tranches : le meme Catalogue (empreinte MHGP12DP,
//                          grand livre, niveaux non reduits, table) ;
//   slices_plan          : plan des tranches : coupes seulement entre cles d'ordre F4 certain (grappes de cles
//                          incertaines a cheval sur les cases jamais coupees), chaque tranche sous la
//                          memoire de travail, repartition stable ; plateau de cles egales incoupable : memory_budget ;
//   slices_cpu_budget    : voie CPU au pic exact de la voie complete : identique (reseau a niveaux egaux : repli sur
//                          la voie complete) ; sous ce pic : par tranches et meme Catalogue, ou refus memory_budget ; au
//                          moins une limite par tranches ; tres serre : refus, budget rendu ;
//   slices_device_budget : voie appareil (transit simule, budgets de l'hote et de l'appareil separes) sous des budgets
//                          de l'appareil qui forcent l'arene en flux et les tranches : meme Catalogue, lots rapatries ;
//   slices_device_reuse  : meme contexte apres la voie en flux (etat du parcours rendu) : nuage, temoin, nuage.
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "device_support.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::catalogue_detail;
using namespace mhgp12::device_test;

namespace {

struct SliceCase {
  const char* name;
  Points points;
  int k;
  u32 leaf;
  u64 most;  // plus grand nombre de tranches demande (cles assez diverses)
};

std::vector<SliceCase> slice_cases() {
  Points cube{{8, 8, 8}};
  for (u32 x : {0u, 16u})
    for (u32 y : {0u, 16u})
      for (u32 z : {0u, 16u}) cube.push_back({x, y, z});
  return {{"nuage2500", scatter(2500, 65535, 7), 5, 24, 200},
          {"nuage1200_k3", scatter(1200, 4095, 11), 3, 8, 60},
          {"nuage1500", scatter(1500, 32767, 5), 5, 24, 100},
          {"coquille48", shell({1, 2, 3}, 1u << 18, 3u << 18), 5, 24, 2},
          {"cube_et_centre", cube, 5, 8, 1}};
}

}  // namespace

MHGP12_TEST(slices_identity, 20) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = sched::make_pool({3});
  REQUIRE(pool.ok());
  for (const SliceCase& c : slice_cases()) {
    auto cloud = make_cloud(c.points, budget);
    REQUIRE(cloud.ok());
    const CatalogueParams params = params_of(c.k, c.leaf);
    auto ref = cpu(cloud.value(), params, 3, budget);
    REQUIRE(ref.ok());
    for (const u64 parts : {u64{1}, u64{2}, u64{7}, c.most}) {
      if (parts > c.most) continue;
      LeafArena arena;
      REQUIRE(leaf_arena(cloud.value(), params, budget, *pool.value(), arena).ok());
      const u64 cap = arena_slice_bytes(arena) / parts + 1;
      u64 slices = 0;
      auto got = sliced_catalogue(cloud.value(), arena, c.k, cap, budget, *pool.value(), &slices);
      if (!CHECK(got.ok() && same(cloud.value(), ref.value(), got.value())))
        std::fprintf(stderr, "tranches : %s, %llu parts\n", c.name, static_cast<unsigned long long>(parts));
      CHECK(slices >= parts && (parts > 1 || slices == 1));
      CHECK(arena.chunks.empty());  // arene de l'hote rendue avant les niveaux et la table
    }
  }
}

MHGP12_TEST(slices_plan, 13) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = sched::make_pool({3});
  REQUIRE(pool.ok());
  auto cloud = make_cloud(scatter(1200, 4095, 11), budget);
  REQUIRE(cloud.ok());
  LeafArena arena;
  REQUIRE(leaf_arena(cloud.value(), params_of(3, 8), budget, *pool.value(), arena).ok());
  Buffer<u64> ball_at, incidence_at;
  fin::ArenaView view;
  REQUIRE(fin::arena_bases(arena.chunks, ball_at, incidence_at, view, budget).ok());
  // cles synthetiques : paires voisines a 2^-45 pres (ordre incertain) et cles egales par triplets
  Buffer<u64> keys;
  REQUIRE(keys.allocate(view.balls, budget).ok());
  for (u64 i = 0; i < view.balls; ++i) {
    const double base = 1.0 + static_cast<double>(i / 6) / 64.0, key = (i % 2) ? base * (1 + 0x1p-45) : base;
    std::memcpy(&keys[i], &key, sizeof key);
  }
  std::vector<u64> weight;  // memoire de travail de chaque boule globale
  for (const Chunk& chunk : arena.chunks)
    for (const BallRecord& r : chunk.records)
      weight.push_back(fin::kSliceBytesPerBall + fin::kSliceBytesPerIncidence * (u64{r.p} + r.m));
  const u64 cap = arena_slice_bytes(arena) / 40 + 1;
  fin::SlicePlan plan;
  REQUIRE(fin::plan_slices(view, keys.span(), cap, plan, budget, *pool.value()).ok());
  CHECK(plan.count >= 40);
  Buffer<u32> ids;
  Buffer<u64> start;
  REQUIRE(fin::assign_slices(keys.span(), plan, ids, start, budget, *pool.value()).ok());
  bool sorted = true, certain = true, bounded = true;
  for (u64 j = 0; j < plan.count; ++j) {
    u64 lo = ~u64{0}, hi = 0, bytes = 0;
    for (u64 i = start[j]; i < start[j + 1]; ++i) {
      sorted = sorted && (i == start[j] || ids[i - 1] < ids[i]);
      lo = std::min(lo, keys[ids[i]]);
      hi = std::max(hi, keys[ids[i]]);
      bytes += weight[ids[i]];
    }
    bounded = bounded && bytes <= cap;
    if (j + 1 < plan.count) {
      u64 next = ~u64{0};
      for (u64 i = start[j + 1]; i < start[j + 2]; ++i) next = std::min(next, keys[ids[i]]);
      certain = certain && fin::key_order(hi, next) < 0;
    }
  }
  CHECK(start[plan.count] == view.balls && weight.size() == view.balls);
  CHECK(sorted && certain && bounded);
  // grappes : trois cles a 600 unites de derniere place l'une de l'autre (ordre F4 incertain), grappes espacees de
  // 6 000 unites (ecart de 4 800 : ordre certain) ; les cases du plan sont plus fines qu'une grappe, qui chevauche donc
  // une frontiere de cases : coupes seulement entre grappes
  {
    const double one = 1.0;
    u64 origin = 0;
    std::memcpy(&origin, &one, sizeof one);
    for (u64 i = 0; i < view.balls; ++i) keys[i] = origin + (i / 3) * 6000 + (i % 3) * 600;
    fin::SlicePlan grouped;
    REQUIRE(fin::plan_slices(view, keys.span(), cap, grouped, budget, *pool.value()).ok());
    Buffer<u32> gids;
    Buffer<u64> gstart;
    REQUIRE(fin::assign_slices(keys.span(), grouped, gids, gstart, budget, *pool.value()).ok());
    bool between = true;
    for (u64 j = 0; j + 1 < grouped.count; ++j) {
      u64 hi = 0, next = ~u64{0};
      for (u64 i = gstart[j]; i < gstart[j + 1]; ++i) hi = std::max(hi, keys[gids[i]]);
      for (u64 i = gstart[j + 1]; i < gstart[j + 2]; ++i) next = std::min(next, keys[gids[i]]);
      between = between && fin::key_order(hi, next) < 0 && (next - origin) % 6000 == 0;
    }
    CHECK(grouped.count >= 20 && between);
  }
  // plateau : toutes les cles egales, plus lourd que la memoire de travail : incoupable
  std::fill(keys.begin(), keys.end(), keys[0]);
  fin::SlicePlan flat;
  const Outcome refused = fin::plan_slices(view, keys.span(), cap, flat, budget, *pool.value());
  CHECK(!refused.ok() && refused.reason == Reason::memory_budget);
}

MHGP12_TEST(slices_cpu_budget, 12) {
  MemoryBudget references(MemoryBudget::kUnlimited);
  auto pool = sched::make_pool({3});
  REQUIRE(pool.ok());
  auto cloud = make_cloud(scatter(2500, 65535, 7), references);
  REQUIRE(cloud.ok());
  const CatalogueParams params = params_of(5, 24);
  auto ref = cpu(cloud.value(), params, 3, references);
  REQUIRE(ref.ok());
  u64 peak = 0;
  {
    MemoryBudget free(MemoryBudget::kUnlimited);
    CatalogueDiagnostics diag;
    auto full = build_catalogue(cloud.value(), params, free, *pool.value(), &diag);
    REQUIRE(full.ok() && diag.finish_slices == 0);
    peak = free.peak();
  }
  // Au pic exact de la voie complete : identique (une limite qui porte ce pic sert toujours, comme avant T1-d ; les
  // portes de la Session recouverte en dependent) ; un reseau de 5 x 5 x 5 sites (niveaux egaux en nombre : le pic
  // estime de la voie complete depasse son pic, et la voie par tranches ne tient pas sous ce pic) passe par le repli
  // sur la voie complete (essai du 8 octobre : sans repli, refus memory_budget).
  {
    MemoryBudget budget(peak);
    auto got = build_catalogue(cloud.value(), params, budget, *pool.value(), nullptr);
    CHECK(got.ok() && same(cloud.value(), ref.value(), got.value()));
  }
  {
    Points grid;
    for (u32 i = 0; i < 5; ++i)
      for (u32 j = 0; j < 5; ++j)
        for (u32 k = 0; k < 5; ++k) grid.push_back({2 * i, 2 * j, 2 * k});
    auto small = make_cloud(grid, references);
    REQUIRE(small.ok());
    const CatalogueParams small_params = params_of(3, 24);
    auto small_ref = cpu(small.value(), small_params, 3, references);
    REQUIRE(small_ref.ok());
    MemoryBudget free(MemoryBudget::kUnlimited);
    REQUIRE(build_catalogue(small.value(), small_params, free, *pool.value(), nullptr).ok());
    MemoryBudget budget(free.peak());
    CatalogueDiagnostics diag;
    auto got = build_catalogue(small.value(), small_params, budget, *pool.value(), &diag);
    CHECK(got.ok() && same(small.value(), small_ref.value(), got.value()) && diag.finish_slices == 0);
  }
  // Sous le pic : par tranches et identique, ou refus memory_budget (entre le minimum du pic de la voie complete et
  // son pic, la voie complete est jouee et refuse, comme avant) ; au moins une limite par tranches.
  u64 sliced = 0;
  for (const u64 limit : {peak * 3 / 4, peak * 3 / 5, peak / 2}) {
    MemoryBudget budget(limit);
    {
      CatalogueDiagnostics diag;
      auto got = build_catalogue(cloud.value(), params, budget, *pool.value(), &diag);
      const bool fine = got.ok() ? same(cloud.value(), ref.value(), got.value()) && diag.finish_slices >= 1
                                 : got.outcome().reason == Reason::memory_budget;
      if (!CHECK(fine))
        std::fprintf(stderr, "voie CPU par tranches : limite %llu sur un pic de %llu (%s, %llu tranches)\n",
                     static_cast<unsigned long long>(limit), static_cast<unsigned long long>(peak),
                     std::string(reason_name(got.outcome().reason)).c_str(),
                     static_cast<unsigned long long>(diag.finish_slices));
      sliced += got.ok() ? 1u : 0u;
      CHECK(budget.peak() <= limit);
    }
    CHECK(budget.released().ok());
  }
  CHECK(sliced >= 1);
  MemoryBudget tight(peak / 16);
  {
    auto refused = build_catalogue(cloud.value(), params, tight, *pool.value(), nullptr);
    CHECK(!refused.ok() && refused.outcome().reason == Reason::memory_budget);
  }
  CHECK(tight.released().ok());
}

MHGP12_TEST(slices_device_budget, 10) {
  MemoryBudget references(MemoryBudget::kUnlimited);
  auto pool = sched::make_pool({3});
  REQUIRE(pool.ok());
  for (const SliceCase& c : slice_cases()) {
    if (c.k != 5 || c.most < 60) continue;  // nuages dont le parcours ne domine pas la memoire de l'appareil
    auto cloud = make_cloud(c.points, references);
    REQUIRE(cloud.ok());
    const CatalogueParams params = params_of(c.k, c.leaf);
    auto ref = cpu(cloud.value(), params, 3, references);
    REQUIRE(ref.ok());
    u64 peak = 0;
    {
      MemoryBudget host(MemoryBudget::kUnlimited), device(MemoryBudget::kUnlimited);
      CatalogueDiagnostics diag;
      auto full = staged_device(cloud.value(), params, *pool.value(), host, device, 4096, 2, &diag);
      REQUIRE(full.ok() && diag.finish_slices == 0 && diag.arena_streamed == 0);
      peak = device.peak();
    }
    u64 sliced = 0;
    for (const u64 limit : {peak / 2, peak / 3}) {
      MemoryBudget host(MemoryBudget::kUnlimited), device(limit);
      {
        CatalogueDiagnostics diag;
        auto got = staged_device(cloud.value(), params, *pool.value(), host, device, 4096, 2, &diag);
        if (!CHECK(got.ok() && same(cloud.value(), ref.value(), got.value()) && device.peak() <= limit))
          std::fprintf(stderr, "voie appareil en flux : %s, limite %llu (%s, pic %llu)\n", c.name,
                       static_cast<unsigned long long>(limit), std::string(reason_name(got.outcome().reason)).c_str(),
                       static_cast<unsigned long long>(device.peak()));
        sliced += diag.finish_slices >= 1 && diag.arena_streamed >= 1 ? 1u : 0u;
      }
      CHECK(host.released().ok() && device.released().ok());
    }
    CHECK(sliced >= 1);
  }
}

// Contexte resident reemploye apres la voie en flux : sur le meme executeur et le meme etat, sous un budget de
// l'appareil qui force l'etat du parcours rendu et la fin d'etage par tranches, le nuage de 2 500 sites (identique),
// puis le temoin du carre (voie complete, identique), puis de nouveau le nuage (identique) : les tableaux rendus
// recroissent ; tout est rendu a la destruction.
MHGP12_TEST(slices_device_reuse, 7) {
  MemoryBudget references(MemoryBudget::kUnlimited);
  auto pool = sched::make_pool({3});
  REQUIRE(pool.ok());
  auto big = make_cloud(scatter(2500, 65535, 7), references);
  auto small = make_cloud({{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}}, references);
  REQUIRE(big.ok() && small.ok());
  const CatalogueParams big_params = params_of(5, 24), small_params = params_of(2, 5);
  auto big_ref = cpu(big.value(), big_params, 3, references);
  auto small_ref = cpu(small.value(), small_params, 3, references);
  REQUIRE(big_ref.ok() && small_ref.ok());
  u64 peak = 0;
  {
    MemoryBudget host(MemoryBudget::kUnlimited), device(MemoryBudget::kUnlimited);
    REQUIRE(staged_device(big.value(), big_params, *pool.value(), host, device, 4096, 2).ok());
    peak = device.peak();
  }
  MemoryBudget host(MemoryBudget::kUnlimited), device(peak / 3);
  {
    StagedExecutor executor{{*pool.value(), device}};
    executor.host = &host;
    executor.slot_size = 4096;
    dev::DeviceState<StagedExecutor> state;
    const auto run = [&](const Cloud& cloud, const CatalogueParams& params, CatalogueDiagnostics& diag) {
      return guarded(
          [&]() { return dev::device_catalogue(executor, state, cloud, params, host, *pool.value(), diag); });
    };
    CatalogueDiagnostics first_diag, small_diag, again_diag;
    auto first = run(big.value(), big_params, first_diag);
    CHECK(first.ok() && same(big.value(), big_ref.value(), first.value()) && first_diag.finish_slices >= 1);
    auto second = run(small.value(), small_params, small_diag);
    CHECK(second.ok() && same(small.value(), small_ref.value(), second.value()) && small_diag.finish_slices == 0);
    auto again = run(big.value(), big_params, again_diag);
    CHECK(again.ok() && same(big.value(), big_ref.value(), again.value()) && again_diag.finish_slices >= 1);
    CHECK(device.peak() <= device.limit());
  }
  CHECK(host.released().ok() && device.released().ok());
}
