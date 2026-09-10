// Auditeur : vidage de l'ordre K=1 de la tour (nuage uniforme genere, census
// WSPD reel) pour comparaison a un single-linkage exact independant en Python.
#include <cstdio>
#include <cstring>
#include <string>
#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/forest/full_ball_tower.hpp"
#include "../src/pipeline/generate.hpp"
using namespace mhgp7;
static void print_i128(i128 v) {
  if (v < 0) { std::printf("-"); v = -v; }
  char buf[64]; int k = 0; u128 u = static_cast<u128>(v);
  do { buf[k++] = static_cast<char>('0' + static_cast<int>(u % 10)); u /= 10; } while (u);
  while (k) std::putchar(buf[--k]);
}
int main(int argc, char** argv) {
  if (argc != 4) return 2;
  i64 n = 0, kmax = 0, s = 0;
  if (!parse_i64_exact(argv[1], &n) || !parse_i64_exact(argv[2], &kmax) || !parse_i64_exact(argv[3], &s)) return 2;
  auto input = make_family_input(CloudFamily::kUniform, static_cast<int>(n), 65536, 3);
  const auto ix = build_cloud_index(input);
  if (!ix.valid || ix.has_duplicate_positions()) { std::fprintf(stderr, "index invalide\n"); return 3; }
  const unsigned k = static_cast<unsigned>(std::min<i64>(kmax, n));
  const u64 smax = std::min<u64>(k + 1, ix.input_count);
  std::vector<BallCandidate> candidates; GenerateOptions go; go.s = s; go.smax = smax; go.threads = 1; GenerateStats gen;
  generate_candidates(ix, go, &candidates, &gen);
  if (gen.cap_refus != kCapRefusNone || gen.invariant_jneg) { std::fprintf(stderr, "generation refusee\n"); return 3; }
  rle_candidates(&candidates, 1);
  std::vector<Survivor> survivors; std::vector<BallData> balls; ExpandStats es;
  prefilter_balls(ix, candidates, smax, 1, &survivors, &es);
  if (census_balls(ix, candidates, survivors, smax, kBallShellMax, 1, &balls, &es) != PipelineStatus::kCompleteRegular) { std::fprintf(stderr, "census refuse\n"); return 3; }
  const auto tower = build_full_ball_tower(ix, balls, k);
  if (tower.status != FullBallStatus::kCompleteRelative) { std::fprintf(stderr, "tour refusee : %s\n", tower.reason); return 1; }
  std::printf("{\"points\":[");
  for (size_t j = 0; j < input.size(); ++j)
    std::printf("%s[%u,%lld,%lld,%lld]", j ? "," : "", input[j].id, (long long)input[j].position.x, (long long)input[j].position.y, (long long)input[j].position.z);
  std::printf("],\"balls\":%zu,\"extra_records\":%llu,\"orders\":[", balls.size(), (unsigned long long)tower.stats.extra_records);
  for (size_t o = 0; o < tower.orders.size(); ++o) {
    const auto& f = tower.orders[o].forest;
    std::printf("%s{\"K\":%u,\"nodes\":[", o ? "," : "", f.order());
    for (size_t i = 0; i < f.nodes().size(); ++i) {
      const auto& nd = f.nodes()[i];
      std::printf("%s{\"num\":[%llu,%llu,%llu],\"den\":\"", i ? "," : "", (unsigned long long)nd.level.num[0], (unsigned long long)nd.level.num[1], (unsigned long long)nd.level.num[2]);
      print_i128(nd.level.den); std::printf("\",\"parents\":[");
      for (u64 j = 0; j < nd.parent_count; ++j) std::printf("%s%llu", j ? "," : "", (unsigned long long)f.parents()[nd.first + j]);
      std::printf("]}");
    }
    std::printf("],\"contributions\":[");
    for (size_t i = 0; i < f.contributions().size(); ++i) {
      const auto& c = f.contributions()[i]; const auto& row = f.populations()->rows()[c.ref.population];
      std::printf("%s{\"num\":[%llu,%llu,%llu],\"den\":\"", i ? "," : "", (unsigned long long)c.level.num[0], (unsigned long long)c.level.num[1], (unsigned long long)c.level.num[2]);
      print_i128(c.level.den); std::printf("\",\"segment\":%llu,\"points\":[", (unsigned long long)c.segment);
      bool first = true;
      if (c.ref.include_interior) for (PointId id : row.interior) { std::printf("%s%u", first ? "" : ",", id); first = false; }
      for (size_t b = 0; b < row.shell.size(); ++b) if (c.ref.shell_mask & (u16{1} << b)) { std::printf("%s%u", first ? "" : ",", row.shell[b]); first = false; }
      std::printf("]}");
    }
    std::printf("]}");
  }
  std::printf("]}\n");
  return 0;
}
