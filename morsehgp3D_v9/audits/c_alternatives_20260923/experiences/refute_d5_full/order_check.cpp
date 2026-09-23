// Auditeur C (jure D5) : ordre reel des contributions datees du produit a un niveau donne.
// Usage : order_check cloud.u32le Kmax catalogue.bin K ballA ballB
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>
#include "src/chain/tower_chain.hpp"
using namespace mhgp9;
using namespace mhgp9::tower;
int main(int argc, char** argv) {
  if (argc < 7) return 2;
  std::ifstream in(argv[1], std::ios::binary);
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::vector<InputPoint> input(bytes.size() / 12);
  for (size_t i = 0; i < input.size(); ++i) {
    std::uint32_t c[3];
    for (int a = 0; a < 3; ++a) { c[a] = 0; for (int b = 0; b < 4; ++b) c[a] |= std::uint32_t(bytes[(3 * i + a) * 4 + b]) << (8 * b); }
    input[i] = InputPoint{(PointId)i, P3{(i32)c[0], (i32)c[1], (i32)c[2]}};
  }
  const unsigned kmax = std::stoul(argv[2]);
  std::FILE* f = std::fopen(argv[3], "rb");
  std::uint64_t count = 0;
  if (std::fread(&count, 8, 1, f) != 1) return 3;
  std::vector<BallData> balls(count);
  if (std::fread(balls.data(), sizeof(BallData), count, f) != count) return 3;
  std::fclose(f);
  std::vector<BallData> kept;
  for (const auto& b : balls) if ((unsigned)b.n_interior + b.arity <= kmax + 1) kept.push_back(b);
  balls.swap(kept);
  const CloudIndex ix = build_cloud_index(input);
  auto tw = build_full_ball_tower(ix, balls, kmax, 1, {}, true);
  if (tw.status != FullBallStatus::kCompleteRelative) { std::printf("tower %s\n", tw.reason); return 1; }
  const unsigned K = std::stoul(argv[4]);
  const std::uint32_t A = std::stoul(argv[5]), Bb = std::stoul(argv[6]);
  // population -> boule (ensembles de PointId egaux)
  std::map<std::pair<std::vector<PointId>, std::vector<PointId>>, std::uint32_t> by_pop;
  for (std::uint32_t j = 0; j < balls.size(); ++j) {
    std::vector<PointId> I, U;
    for (i32 s : balls[j].interior()) I.push_back(ix.point_id(s));
    for (i32 s : balls[j].shell()) U.push_back(ix.point_id(s));
    std::sort(I.begin(), I.end()); std::sort(U.begin(), U.end());
    by_pop[{I, U}] = j;
  }
  const auto& forest = tw.orders[K - 1].forest;
  const auto& rows = forest.populations()->rows();
  const ExactLevel lvl = balls[A].level;
  std::printf("same_level(A,B)=%d  A: p=%u u=%u q=%u  B: p=%u u=%u q=%u\n", (int)same_exact_level(balls[A].level, balls[Bb].level),
    balls[A].n_interior, balls[A].n_shell, balls[A].arity, balls[Bb].n_interior, balls[Bb].n_shell, balls[Bb].arity);
  size_t idx = 0;
  for (const auto& c : forest.contributions()) {
    if (same_exact_level(c.level, lvl)) {
      const auto& row = rows[c.ref.population];
      auto it = by_pop.find({row.interior, row.shell});
      const bool fresh = same_exact_level(forest.nodes()[c.segment].level, lvl);
      std::printf("contrib[%zu] segment=%llu (%s) population=%llu ball=%d mask=%u interior=%d\n", idx,
        (unsigned long long)c.segment, fresh ? "noeud cree a ce niveau" : "continuation d'un noeud anterieur",
        (unsigned long long)c.ref.population, it == by_pop.end() ? -1 : (int)it->second, c.ref.shell_mask, (int)c.ref.include_interior);
    }
    ++idx;
  }
  return 0;
}
