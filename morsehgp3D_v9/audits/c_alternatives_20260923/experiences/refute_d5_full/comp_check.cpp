// Auditeur C (jure D5) : composantes strictes d'une coquille etendue, et arite des fusions produit.
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "src/chain/tower_chain.hpp"
using namespace mhgp9;
using namespace mhgp9::tower;
int main(int argc, char** argv) {
  std::ifstream in(argv[1], std::ios::binary);
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::vector<gen::Point3> gp(bytes.size() / 12);
  std::vector<InputPoint> input(bytes.size() / 12);
  for (size_t i = 0; i < input.size(); ++i) {
    std::uint32_t c[3];
    for (int a = 0; a < 3; ++a) { c[a] = 0; for (int b = 0; b < 4; ++b) c[a] |= std::uint32_t(bytes[(3 * i + a) * 4 + b]) << (8 * b); }
    input[i] = InputPoint{(PointId)i, P3{(i32)c[0], (i32)c[1], (i32)c[2]}};
    gp[i] = gen::Point3{(gen::Coordinate)c[0], (gen::Coordinate)c[1], (gen::Coordinate)c[2]};
  }
  const unsigned kmax = std::stoul(argv[2]);
  ChainOptions opt; opt.kmax = kmax; opt.workers = 1; opt.run_tower = false; opt.keep_catalogue = true;
  auto res = run_tower_chain(gp, opt);
  if (res.status != ChainStatus::kComplete) { std::printf("chain %s\n", res.reason.c_str()); return 1; }
  auto& balls = res.catalogue_balls;
  const CloudIndex ix = build_cloud_index(input);
  for (std::uint32_t j = 0; j < balls.size(); ++j) {
    const auto& b = balls[j];
    if (b.n_shell < 8) continue;
    local_plateau::LocalCensus local{b.key, {}, {}};
    for (i32 s : b.interior()) local.interior.push_back({ix.point_id(s), ix.upos[s]});
    for (i32 s : b.shell()) local.shell.push_back({ix.point_id(s), ix.upos[s]});
    auto table = local_plateau::ShellTable::prepare(std::move(local));
    std::printf("ball %u p=%u u=%u q=%u :", j, b.n_interior, b.n_shell, b.arity);
    for (unsigned k = 1; k <= kmax; ++k) { auto r = table.rank(k); std::printf(" K%u:%zu", k, r.present ? r.strict_components.size() : 0); }
    std::printf("\n");
  }
  auto tw = build_full_ball_tower(ix, balls, kmax, 1, {}, true);
  std::printf("tower status=%d reason=%s\n", (int)tw.status, tw.reason);
  if (tw.status == FullBallStatus::kCompleteRelative)
    for (unsigned k = 1; k <= tw.orders.size(); ++k) {
      u64 maxp = 0;
      for (const auto& nd : tw.orders[k - 1].forest.nodes()) maxp = std::max<u64>(maxp, nd.parent_count);
      std::printf("K%u nodes=%zu max_parents=%llu\n", k, tw.orders[k - 1].forest.nodes().size(), (unsigned long long)maxp);
    }
  return 0;
}
