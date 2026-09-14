// Auditeur B (14 sept. 2026) — balayage de la largeur de cellule des tubes sur un
// produit inter-amas de la fixture clusters du constructeur (copie tube_audit).
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "pipeline/local_credits.hpp"
#include "tube_credits.hpp"
#include "front_fixtures.hpp"
using namespace mhgp8;
static unsigned corner_of(const Point3& p) { return (p.x >= 35000 ? 1U : 0U) | (p.y >= 35000 ? 2U : 0U) | (p.z >= 35000 ? 4U : 0U); }
int main(int argc, char** argv) {
  const std::size_t n = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 32000;
  const unsigned ca = argc > 2 ? std::atoi(argv[2]) : 0, cb = argc > 3 ? std::atoi(argv[3]) : 1;
  auto fx = bench::make_front_fixture(n, "clusters", 3);
  std::vector<std::vector<Point3>> cl(8); for (const auto& p : fx.points) cl[corner_of(p)].push_back(p);
  RectangleInput in; in.points = cl[ca]; for (auto p : cl[cb]) in.points.push_back(p);
  in.a = {0, cl[ca].size()}; in.b = {cl[ca].size(), in.points.size()};
  auto r = prepare_rectangle(in, 10, 8);
  for (Lane lane : {Lane::Q2, Lane::Q3, Lane::Q4}) {
    Work w{}; const auto need = static_cast<std::uint8_t>(r->threshold(lane) - r->core_credit(lane));
    const auto t0 = std::chrono::steady_clock::now();
    auto ca_ = tube_audit::credits(*r, r->a_range(), r->a_box(), r->b_box(), lane, need, w);
    auto cb_ = tube_audit::credits(*r, r->b_range(), r->b_box(), r->a_box(), lane, need, w);
    const auto t1 = std::chrono::steady_clock::now();
    unsigned long long residual = 0; std::vector<unsigned long long> hb(need + 1, 0); for (auto c : cb_) ++hb[c];
    for (auto c : ca_) for (unsigned k = 0; k + c < need; ++k) residual += hb[k];
    std::printf("q%u width_mul=%s records=%llu cells=%llu fallbacks=%llu residual=%llu ms=%.1f\n", (unsigned)lane, std::getenv("MHGP8_TUBE_WIDTH_MUL") ? std::getenv("MHGP8_TUBE_WIDTH_MUL") : "1",
      (unsigned long long)w.tube_records, (unsigned long long)w.tube_cells, (unsigned long long)w.tube_separation_fallbacks, residual,
      std::chrono::duration<double,std::milli>(t1-t0).count());
  }
}
