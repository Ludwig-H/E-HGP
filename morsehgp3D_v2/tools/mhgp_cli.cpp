// MorseHGP3D v2 — outil en ligne de commande : genere ou lit un nuage, calcule
// le catalogue et les forets, publie un rapport JSON de faits (jamais de
// revendication de statut).

#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"

using namespace mhgp;

namespace {

std::vector<double> generate(const std::string& family, int n, unsigned seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> u(0.0, 1.0);
  std::vector<double> out;
  out.reserve(static_cast<size_t>(n) * 3);
  if (family == "uniform") {
    for (int i = 0; i < n; ++i)
      for (int d = 0; d < 3; ++d) out.push_back(u(rng));
  } else if (family == "clusters") {
    const int nc = 8;
    std::vector<double> c;
    for (int i = 0; i < nc; ++i) for (int d = 0; d < 3; ++d) c.push_back(u(rng));
    std::normal_distribution<double> g(0.0, 0.04);
    for (int i = 0; i < n; ++i) {
      const int j = i % nc;
      for (int d = 0; d < 3; ++d) out.push_back(c[static_cast<size_t>(3 * j + d)] + g(rng));
    }
  } else if (family == "plane") {
    std::normal_distribution<double> g(0.0, 0.01);
    for (int i = 0; i < n; ++i) {
      out.push_back(u(rng));
      out.push_back(u(rng));
      out.push_back(g(rng));
    }
  } else {
    std::fprintf(stderr, "famille inconnue: %s\n", family.c_str());
    std::exit(2);
  }
  return out;
}

}  // namespace

int main(int argc, char** argv) {
  std::string family = "uniform";
  int n = 1000, k = 4;
  unsigned seed = 1;
  Options opt;
  std::string out_path;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto next = [&]() { return std::string(argv[++i]); };
    if (a == "--family") family = next();
    else if (a == "--n") n = std::stoi(next());
    else if (a == "--k") k = std::stoi(next());
    else if (a == "--seed") seed = static_cast<unsigned>(std::stoul(next()));
    else if (a == "--threads") opt.threads = std::stoi(next());
    else if (a == "--single-order") opt.all_orders = false;
    else if (a == "--cones") opt.cone_directions = std::stoi(next());
    else if (a == "--json") out_path = next();
    else if (a == "--verbose") opt.verbose = true;
    else { std::fprintf(stderr, "option inconnue: %s\n", a.c_str()); return 2; }
  }
  opt.k_max = k;

  const std::vector<double> xyz = generate(family, n, seed);
  double scale = 0.0, origin[3] = {0, 0, 0};
  const std::vector<P3> pts = quantize(xyz, &scale, origin);

  const Result r = run(pts, opt);

  // statistiques de voisinage
  i64 sum_nb = 0, max_nb = 0, sum_rounds = 0, max_rounds = 0;
  for (i32 v : r.catalogue.neighbourhood_size) { sum_nb += v; max_nb = std::max<i64>(max_nb, v); }
  for (i32 v : r.catalogue.growth_rounds) { sum_rounds += v; max_rounds = std::max<i64>(max_rounds, v); }

  i64 by_rank[kMaxRank + 2] = {0};
  i64 by_support[5] = {0};
  for (const CriticalSphere& s : r.catalogue.spheres) {
    if (s.rank >= 0 && s.rank <= kMaxRank + 1) ++by_rank[s.rank];
    if (s.n_support >= 1 && s.n_support <= 4) ++by_support[s.n_support];
  }

  std::string js = "{\n";
  js += "  \"tool\": \"mhgp_cli\",\n";
  js += "  \"domain\": \"quantized_grid\",\n";
  js += "  \"coord_bits\": " + std::to_string(kCoordBits) + ",\n";
  js += "  \"family\": \"" + family + "\",\n";
  js += "  \"n\": " + std::to_string(n) + ",\n";
  js += "  \"k_max\": " + std::to_string(k) + ",\n";
  js += "  \"seed\": " + std::to_string(seed) + ",\n";
  js += "  \"critical_spheres\": " + std::to_string(r.catalogue.spheres.size()) + ",\n";
  js += "  \"uncertified_points\": " + std::to_string(r.uncertified_points) + ",\n";
  js += "  \"neighbourhood_mean\": "
      + std::to_string(n ? static_cast<double>(sum_nb) / n : 0.0) + ",\n";
  js += "  \"neighbourhood_max\": " + std::to_string(max_nb) + ",\n";
  js += "  \"growth_rounds_mean\": "
      + std::to_string(n ? static_cast<double>(sum_rounds) / n : 0.0) + ",\n";
  js += "  \"growth_rounds_max\": " + std::to_string(max_rounds) + ",\n";
  js += "  \"candidate_pairs\": " + std::to_string(r.catalogue.candidate_pairs) + ",\n";
  js += "  \"candidate_triples\": " + std::to_string(r.catalogue.candidate_triples) + ",\n";
  js += "  \"candidate_quads\": " + std::to_string(r.catalogue.candidate_quads) + ",\n";
  js += "  \"by_support\": [";
  for (int i = 1; i <= 4; ++i) js += (i > 1 ? ", " : "") + std::to_string(by_support[i]);
  js += "],\n";
  js += "  \"by_rank\": {";
  bool first = true;
  for (int i = 0; i <= kMaxRank + 1; ++i) {
    if (!by_rank[i]) continue;
    if (!first) js += ", ";
    first = false;
    js += "\"" + std::to_string(i) + "\": " + std::to_string(by_rank[i]);
  }
  js += "},\n";
  js += "  \"forests\": [\n";
  for (size_t i = 0; i < r.forests.size(); ++i) {
    const Forest& f = r.forests[i];
    js += "    {\"order\": " + std::to_string(f.order)
        + ", \"births\": " + std::to_string(f.births)
        + ", \"merge_events\": " + std::to_string(f.merge_events)
        + ", \"killed\": " + std::to_string(f.killed)
        + ", \"nodes\": " + std::to_string(f.nodes.size())
        + ", \"roots\": " + std::to_string(f.roots.size()) + "}";
    if (i + 1 < r.forests.size()) js += ",";
    js += "\n";
  }
  js += "  ],\n";
  js += "  \"timings_ms\": {\"catalogue\": " + std::to_string(r.timings.catalogue_ms)
      + ", \"forest\": " + std::to_string(r.timings.forest_ms)
      + ", \"total\": " + std::to_string(r.timings.total_ms) + "}\n";
  js += "}\n";

  if (out_path.empty()) {
    std::fputs(js.c_str(), stdout);
  } else {
    FILE* f = std::fopen(out_path.c_str(), "w");
    if (!f) { std::perror("fopen"); return 1; }
    std::fputs(js.c_str(), f);
    std::fclose(f);
    std::fputs(js.c_str(), stdout);
  }
  return 0;
}
