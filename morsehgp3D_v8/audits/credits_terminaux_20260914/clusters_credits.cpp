// Auditeur B : crédits locaux P0 (Tubes/Pool/DualBlocks) sur les produits inter-amas
// de la fixture 'clusters' du constructeur (bench/front_fixtures.hpp), Kmax 10, s 8.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "pipeline/local_credits.hpp"
#include "front_fixtures.hpp"
using namespace mhgp8;
using Clock = std::chrono::steady_clock;
static double ms(Clock::time_point a, Clock::time_point b){ return std::chrono::duration<double,std::milli>(b-a).count(); }
static unsigned corner_of(const Point3& p) { return (p.x >= 35000 ? 1U : 0U) | (p.y >= 35000 ? 2U : 0U) | (p.z >= 35000 ? 4U : 0U); }
int main(int argc, char** argv) {
  const std::size_t n = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 32000;
  auto fx = bench::make_front_fixture(n, "clusters", 3);
  std::vector<std::vector<Point3>> cl(8);
  for (const auto& p : fx.points) cl[corner_of(p)].push_back(p);
  std::printf("n=%zu cluster sizes:", n); for (auto& c : cl) std::printf(" %zu", c.size()); std::printf("\n");
  const unsigned pairs[3][2] = {{0,1},{0,3},{0,7}};  // arête, face-diagonale, grande diagonale du cube
  for (auto& pr : pairs) {
    RectangleInput in; in.points = cl[pr[0]]; for (auto p : cl[pr[1]]) in.points.push_back(p);
    in.a = {0, cl[pr[0]].size()}; in.b = {cl[pr[0]].size(), in.points.size()};
    const auto t0 = Clock::now(); auto r = prepare_rectangle(in, 10, 8); const auto t1 = Clock::now();
    std::printf("product %u x %u : |A|=%zu |B|=%zu pairs=%zu prepare_ms=%.1f\n", pr[0], pr[1], in.a.size(), in.b.size(), in.a.size()*in.b.size(), ms(t0,t1));
    for (auto strat : {Strategy::Tubes, Strategy::Pool, Strategy::DualBlocks}) {
      const char* nm = strat == Strategy::Tubes ? "Tubes" : strat == Strategy::Pool ? "Pool" : "DualBlocks";
      const auto s0 = Clock::now(); auto batch = make_credit_batch(r, strat); const auto s1 = Clock::now();
      std::printf("  %-10s batch_ms=%.1f residual q2=%llu q3=%llu q4=%llu\n", nm, ms(s0,s1),
        (unsigned long long)batch.plan(Lane::Q2).candidate_pairs(), (unsigned long long)batch.plan(Lane::Q3).candidate_pairs(), (unsigned long long)batch.plan(Lane::Q4).candidate_pairs());
    }
  }
  return 0;
}
