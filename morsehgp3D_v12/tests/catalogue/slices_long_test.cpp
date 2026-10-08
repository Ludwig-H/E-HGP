// Arene en flux sur plusieurs lots de feuilles (tranche T1-d, porte longue) : nuage uniforme de 150 000 sites a K5
// (feuilles de 24 sites ; plus de 2^17 feuilles par niveau, donc six lots ; 12,3 millions de boules), voie appareil
// sur le transit simule (budgets de l'hote et de l'appareil separes) sous un budget de l'appareil de 18 % du pic de la
// voie complete, qui refuse l'arene residente des le deuxieme lot : arene deja ecrite rapatriee, puis chaque lot
// rapatrie un a un, etat du parcours rendu, fin d'etage par tranches ; meme Catalogue que la voie CPU, tout rendu. Le
// parcours n'est pas borne par T1-d : a K2 et feuilles de 5 sites, son front domine (essai du 8 octobre), d'ou ce nuage.
#include <algorithm>
#include <array>
#include <vector>

#include "device_support.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::device_test;

namespace {

// n points de [0, 2^bits)^3 (SplitMix64), positions distinctes par tri et elimination des doublons.
Points uniform(u64 n, u64 seed, u32 bits) {
  Mix mix{seed};
  Points out(n);
  for (auto& p : out) p = {mix.below(u64{1} << bits), mix.below(u64{1} << bits), mix.below(u64{1} << bits)};
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

}  // namespace

MHGP12_TEST(slices_streaming, 6) {
  MemoryBudget references(MemoryBudget::kUnlimited);
  auto pool = sched::make_pool({3});
  REQUIRE(pool.ok());
  auto cloud = make_cloud(uniform(150000, 29, 21), references);
  REQUIRE(cloud.ok());
  const CatalogueParams params = params_of(5, 24);
  auto ref = cpu(cloud.value(), params, 3, references);
  REQUIRE(ref.ok());
  u64 peak = 0;
  {
    MemoryBudget host(MemoryBudget::kUnlimited), device(MemoryBudget::kUnlimited);
    CatalogueDiagnostics diag;
    auto full = staged_device(cloud.value(), params, *pool.value(), host, device, u64{1} << 20, 2, &diag);
    REQUIRE(full.ok() && diag.batches >= 2 && diag.arena_streamed == 0);
    peak = device.peak();
  }
  MemoryBudget host(MemoryBudget::kUnlimited), device(peak * 18 / 100);
  {
    CatalogueDiagnostics diag;
    auto got = staged_device(cloud.value(), params, *pool.value(), host, device, u64{1} << 20, 2, &diag);
    CHECK(got.ok() && same(cloud.value(), ref.value(), got.value()));
    CHECK(diag.batches >= 2 && diag.arena_streamed >= 3 && diag.finish_slices >= 2);
    CHECK(device.peak() <= device.limit());
    std::printf("slices_streaming : %llu lots, %llu rapatries, %llu tranches, pic de l'appareil %llu sur %llu\n",
                static_cast<unsigned long long>(diag.batches), static_cast<unsigned long long>(diag.arena_streamed),
                static_cast<unsigned long long>(diag.finish_slices), static_cast<unsigned long long>(device.peak()),
                static_cast<unsigned long long>(peak));
  }
  CHECK(host.released().ok() && device.released().ok());
}

MHGP12_TEST_MAIN()
