// Partage du lot de feuilles (select_host_leaves) contre une recomputation independante : feuilles triees par poids
// m^3 decroissant puis par rang dans le lot, prefixe le plus court dont le poids atteint la cible floor(W*p/1000).
// La fusion des deux cotes est jugee de bout en bout par mhgp11_tower_full_leaf_lanes (memes dump et registre).
#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

#include "catalogue/leaf_batch.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::catalogue_detail;

namespace {
std::vector<u8> reference(const std::vector<LeafJob>& jobs, u32 permille) {
  std::vector<u64> order(jobs.size());
  std::iota(order.begin(), order.end(), u64{0});
  std::stable_sort(order.begin(), order.end(), [&](u64 a, u64 b) { return jobs[a].m > jobs[b].m; });
  u64 total = 0;
  for (const auto& job : jobs) total += u64{job.m} * job.m * job.m;
  const u64 target = total * permille / 1000;  // W < 2^40 ici : produit exact
  std::vector<u8> host(jobs.size(), 0);
  u64 acquired = 0;
  for (u64 j : order) {
    if (acquired >= target) break;
    host[j] = 1;
    acquired += u64{jobs[j].m} * jobs[j].m * jobs[j].m;
  }
  return host;
}

std::vector<LeafJob> draw(std::mt19937_64& rng, u64 count, bool skewed) {
  std::vector<LeafJob> jobs(count);
  for (auto& job : jobs) {
    const u64 r = rng();
    job.m = skewed ? static_cast<u32>(1 + (r % 1000 < 900 ? r % 12 : r % 32)) : static_cast<u32>(1 + r % 32);
  }
  return jobs;
}
}  // namespace

MHGP11_TEST(select, 2000) {
  std::mt19937_64 rng(20261006);
  u64 partial = 0, cases = 0;
  for (u64 count : {u64{1}, u64{2}, u64{7}, u64{64}, u64{1000}, u64{25000}}) {
    for (int trial = 0; trial < 20; ++trial) {
      const bool skewed = trial % 2 == 1;
      const auto jobs = draw(rng, count, skewed);
      for (u32 permille : {0u, 1u, 250u, 400u, 500u, 999u, 1000u}) {
        std::vector<u8> to_host(count, 2);
        const auto made = select_host_leaves(jobs, permille, to_host);
        REQUIRE(made.ok());
        const auto expected = reference(jobs, permille);
        CHECK(to_host == expected);
        const u64 host = static_cast<u64>(std::count(to_host.begin(), to_host.end(), u8{1}));
        CHECK(std::all_of(to_host.begin(), to_host.end(), [](u8 v) { return v <= 1; }));
        if (permille == 0) CHECK_EQ(host, 0u);
        if (permille == 1000) CHECK_EQ(host, count);
        partial += host != 0 && host != count;
        ++cases;
      }
    }
  }
  CHECK(cases == 840 && partial >= 400);
}

MHGP11_TEST(refusals, 4) {
  std::vector<LeafJob> jobs(3);
  jobs[0].m = 4; jobs[1].m = 32; jobs[2].m = 33;  // au-dela des 32 sites d'une feuille du lot
  std::vector<u8> to_host(3);
  CHECK_EQ(select_host_leaves(jobs, 400, to_host).reason, Reason::catalogue_invariant);
  jobs[2].m = 0;
  CHECK_EQ(select_host_leaves(jobs, 400, to_host).reason, Reason::catalogue_invariant);
  jobs[2].m = 5;
  CHECK_EQ(select_host_leaves(jobs, 1001, to_host).reason, Reason::parameter_out_of_range);
  std::vector<u8> short_view(2);
  CHECK_EQ(select_host_leaves(jobs, 400, short_view).reason, Reason::parameter_out_of_range);
}

MHGP11_TEST_MAIN()
