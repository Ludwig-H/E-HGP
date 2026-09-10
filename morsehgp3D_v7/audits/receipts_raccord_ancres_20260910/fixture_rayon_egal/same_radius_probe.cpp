// Recherche d'une fixture entiere forcant un pas de descente a RAYON EGAL dans
// full_ball_detail::Builder::resolve. Catalogue = MEB de TOUS les sous-ensembles.
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>
#include "../src/forest/full_ball_tower.hpp"
using namespace mhgp7;
struct Run { FullBallTowerResult r; size_t balls; };
Run run(const std::vector<P3>& pts, unsigned kmax) {
  std::vector<InputPoint> in;
  for (size_t j = 0; j < pts.size(); ++j) in.push_back({static_cast<PointId>(j), pts[j]});
  const auto ix = build_cloud_index(in);
  std::map<BallKey, BallData> catalogue;
  AnchorMebWork work;
  const u32 domain = (u32{1} << pts.size()) - 1;
  for (u32 mask = 1; mask <= domain; ++mask) {
    std::vector<P3> sub;
    for (size_t j = 0; j < pts.size(); ++j) if (mask & (u32{1} << j)) sub.push_back(pts[j]);
    if (sub.size() < 2) continue;
    const auto m = anchor_meb(sub, work);
    if (m.status != AnchorMebStatus::kOk) { std::printf("meb failure %s\n", m.reason); std::exit(3); }
    if (catalogue.count(m.key)) continue;
    BallData row{};
    row.key = m.key; row.level = m.level; row.arity = m.support_size;
    for (size_t j = 0; j < pts.size(); ++j) {
      const i32 idx = static_cast<i32>(std::find(ix.upos.begin(), ix.upos.end(), pts[j]) - ix.upos.begin());
      const i128 power = m.key.power(pts[j]);
      if (power == 0) row.shell_ids[row.n_shell++] = idx;
      else if (power < 0) row.interior_ids[row.n_interior++] = idx;
    }
    std::sort(row.interior_ids, row.interior_ids + row.n_interior);
    std::sort(row.shell_ids, row.shell_ids + row.n_shell);
    // L'arite du census est le MINIMUM sur la coquille COMPLETE, pas celle du
    // sous-ensemble generateur : recalcul sur toute la population fermee.
    std::vector<P3> population;
    for (size_t j = 0; j < pts.size(); ++j) if (m.key.power(pts[j]) <= 0) population.push_back(pts[j]);
    const auto full = anchor_meb(population, work);
    if (full.status != AnchorMebStatus::kOk || !(full.key == m.key)) { std::printf("population meb mismatch\n"); std::exit(3); }
    row.arity = full.support_size;
    if (row.n_interior + row.arity > std::min<size_t>(kmax + 1, pts.size())) continue;
    catalogue.emplace(m.key, row);
  }
  std::vector<BallData> balls;
  for (const auto& kv : catalogue) balls.push_back(kv.second);
  return {build_full_ball_tower(ix, balls, kmax), balls.size()};
}
void print(const char* tag, const Run& x, unsigned kmax, i64 xx, i64 xy) {
  const auto& r = x.r;
  std::printf("{\"tag\":\"%s\",\"x\":[%lld,%lld],\"kmax\":%u,\"balls\":%zu,\"status\":%d,\"reason\":\"%s\","
              "\"same_radius_steps\":%llu,\"descending_steps\":%llu,\"max_chain_steps\":%llu,"
              "\"intruder_queries\":%llu,\"anchor_hits\":%llu,\"representatives\":%llu,\"births\":%llu,\"merges\":%llu}\n",
              tag, (long long)xx, (long long)xy, kmax, x.balls, static_cast<int>(r.status), r.reason,
              (unsigned long long)r.stats.same_radius_steps, (unsigned long long)r.stats.descending_steps,
              (unsigned long long)r.stats.max_chain_steps, (unsigned long long)r.stats.intruder_queries,
              (unsigned long long)r.stats.anchor_hits, (unsigned long long)r.stats.representatives,
              (unsigned long long)r.stats.births, (unsigned long long)r.stats.merges);
}
int main(int argc, char** argv) {
  // Quatre points du cercle centre (50,50) rayon 5 sans diametre, deux triangles
  // positifs {0,143,270} et {53,143,270} degres partageant deux sommets ;
  // trois interieurs ; un point x parcouru sur une grille.
  const std::vector<std::vector<P3>> bases{
    {{55,50,0},{53,54,0},{46,53,0},{50,45,0},{52,48,0},{53,47,0},{54,48,0}},
    {{55,50,0},{53,54,0},{46,53,0},{50,45,0},{48,52,0},{47,53,0},{48,54,0}},
    {{55,50,0},{53,54,0},{46,53,0},{50,45,0},{51,51,0},{52,52,0},{51,52,0}},
  };
  const unsigned kmax = argc > 1 ? static_cast<unsigned>(std::atoi(argv[1])) : 7;
  for (size_t b = 0; b < bases.size(); ++b)
  for (i64 xx = 0; xx <= 120; xx += 5) for (i64 xy = 0; xy <= 120; xy += 5) {
    auto pts = bases[b]; pts.push_back({xx, xy, 0});
    bool dup = false; for (size_t j = 0; j + 1 < pts.size(); ++j) dup = dup || (pts[j] == pts.back());
    if (dup) continue;
    const auto x = run(pts, kmax);
    if (x.r.stats.same_radius_steps > 0) { std::printf("base=%zu ", b); print("search8", x, kmax, xx, xy); return 0; }
    if (x.r.status != FullBallStatus::kCompleteRelative) { std::printf("base=%zu ", b); print("refus", x, kmax, xx, xy); }
  }
  return 1;
}
