// MorseHGP3D v3 — SONDE D'ECHELLE de l'arrangement shallow.
//
// Sujet : `prototype/order_k_flats.hpp`, parcours `reverse_search_stream`.
//
// Ce que cette sonde mesure, et que le differentiel exhaustif ne peut pas
// mesurer : le NOMBRE DE SOMMETS de l'arrangement de niveau au plus `k_nav` et
// le TRAVAIL par sommet, sur des nuages de taille realiste. `flats_differential`
// juge la correction sur au plus vingt points; il ne dit rien du volume a
// 12 500, 25 000 ou 50 000. Sans ce volume, aucune discussion de seconde n'a de
// sens : c'est la taille de la sortie que toute route doit payer.
//
// La sonde n'emet aucun payload, ne revendique aucun statut public et ne mesure
// aucun SLO. Son chronometre est indicatif : la machine est partagee. Les
// compteurs deterministes, eux, sont des faits.
//
// Codes de sortie : 0 mesure produite, 2 campagne refusee avant calcul,
// 3 plancher ou invariant viole.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "cloud_families.hpp"
#include "order_k_flats.hpp"

namespace {

int parse_int(const char* text, bool* ok) {
  char* end = nullptr;
  const long long value = std::strtoll(text, &end, 10);
  *ok = end != nullptr && *end == '\0' && value >= 0 && value <= 100000000LL;
  return (int)value;
}

}  // namespace

int main(int argc, char** argv) {
  int points = 1000;
  int smax = 11;
  int coord = 0;
  long long seed = 20260812;
  std::string family = "uniform";
  long long min_vertices = 0;
  int leaf = 16;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    bool ok = true;
    if (arg.rfind("--points=", 0) == 0) points = parse_int(arg.substr(9).c_str(), &ok);
    else if (arg.rfind("--smax=", 0) == 0) smax = parse_int(arg.substr(7).c_str(), &ok);
    else if (arg.rfind("--coord=", 0) == 0) coord = parse_int(arg.substr(8).c_str(), &ok);
    else if (arg.rfind("--seed=", 0) == 0) seed = parse_int(arg.substr(7).c_str(), &ok);
    else if (arg.rfind("--leaf=", 0) == 0) leaf = parse_int(arg.substr(7).c_str(), &ok);
    else if (arg.rfind("--family=", 0) == 0) family = arg.substr(9);
    else if (arg.rfind("--min-vertices=", 0) == 0)
      min_vertices = parse_int(arg.substr(15).c_str(), &ok);
    else {
      std::fprintf(stderr, "REFUS argument inconnu: %s\n", arg.c_str());
      return 2;
    }
    if (!ok) {
      std::fprintf(stderr, "REFUS valeur invalide: %s\n", arg.c_str());
      return 2;
    }
  }
  if (points < 4 || smax < 2 || smax > 32) {
    std::fprintf(stderr, "REFUS domaine: points>=4 et 2<=smax<=32\n");
    return 2;
  }

  mhgp3v::CloudFamily fam;
  if (family == "uniform") fam = mhgp3v::CloudFamily::kUniform;
  else if (family == "terrain") fam = mhgp3v::CloudFamily::kTerrain;
  else if (family == "scanline_single_pass") fam = mhgp3v::CloudFamily::kScanlineSinglePass;
  else if (family == "scanline_overlap_multiecho")
    fam = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
  else {
    std::fprintf(stderr, "REFUS famille inconnue: %s\n", family.c_str());
    return 2;
  }
  if (coord <= 0) coord = mhgp3v::cloud_family_default_coord(fam, points);
  const std::vector<mhgp::P3> cloud = mhgp3v::make_family_cloud(fam, points, coord, seed);
  if ((int)cloud.size() != points) {
    std::fprintf(stderr, "REFUS nuage incomplet: %d/%d\n", (int)cloud.size(), points);
    return 2;
  }

  // `k_nav = smax - 2` : le theoreme de proprietaire garantit que tout support
  // d'arite deux a quatre possede un proprietaire sous cette coupe.
  const int level_ceiling = smax - 2;

  mhgp3v::CertifiedIndex index;
  index.build(cloud, leaf);

  mhgp3v::FlatStatistics st;
  mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
  long long vertices = 0;
  long long shell_ids = 0;
  long long interior_ids = 0;
  long long closed_rank_within = 0;  // sommets de rang ferme au plus smax
  std::vector<long long> by_level((std::size_t)level_ceiling + 2, 0);
  std::vector<long long> by_shell(16, 0);

  const auto t0 = std::chrono::steady_clock::now();
  mhgp3v::reverse_search_stream(
      cloud, level_ceiling, &st, &status,
      [&](const mhgp3v::flats::Vertex& v) {
        ++vertices;
        shell_ids += (long long)v.shell.size();
        interior_ids += (long long)v.interior.size();
        const std::size_t level = v.interior.size();
        if (level < by_level.size()) ++by_level[level];
        const std::size_t shell = v.shell.size();
        ++by_shell[shell < by_shell.size() ? shell : by_shell.size() - 1];
        if ((int)(v.shell.size() + v.interior.size()) <= smax) ++closed_rank_within;
        return true;
      },
      &index);
  const auto t1 = std::chrono::steady_clock::now();
  const double seconds = std::chrono::duration<double>(t1 - t0).count();

  if (status != mhgp3v::CloudStatus::kOk) {
    std::fprintf(stderr, "REFUS statut nuage: %s\n", mhgp3v::cloud_status_name(status));
    return 2;
  }

  std::printf("FlatsScaleReceipt-v1\n");
  std::printf("family=%s points=%d coord=%d seed=%lld smax=%d k_nav=%d leaf=%d\n",
              family.c_str(), points, coord, seed, smax, level_ceiling, leaf);
  std::printf("vertices=%lld vertices_per_point=%.4f closed_rank_within=%lld\n", vertices,
              (double)vertices / (double)points, closed_rank_within);
  std::printf("shell_ids=%lld interior_ids=%lld shells_multiple=%lld\n", shell_ids,
              interior_ids, st.shells_multiple);
  std::printf("pencil_queries=%lld pencil_candidates=%lld grid_points_touched=%lld\n",
              st.pencil_queries, st.pencil_candidates, st.grid_points_touched);
  std::printf("exhaustive_scans=%lld disagreement_sweeps=%lld flats_enumerated=%lld\n",
              st.exhaustive_scans, st.disagreement_sweeps, st.reverse_flats_enumerated);
  std::printf("reverse_depth_max=%lld reverse_children_tested=%lld reverse_decisions=%lld\n",
              st.reverse_depth_max, st.reverse_children_tested, st.reverse_decisions);
  std::printf("reverse_live_high_water=%lld interior_high_water=%lld shell_high_water=%lld\n",
              st.reverse_live_high_water, st.interior_high_water, st.shell_high_water);
  std::printf("full_grid_sweeps=%lld bootstrap_rounds=%lld unbounded_stops=%lld\n",
              st.full_grid_sweeps, st.bootstrap_rounds, st.unbounded_stops);
  std::printf("seconds=%.4f vertices_per_second=%.1f\n", seconds,
              seconds > 0 ? (double)vertices / seconds : 0.0);
  for (int level = 0; level <= level_ceiling; ++level)
    std::printf("level[%d]=%lld\n", level, by_level[(std::size_t)level]);
  for (std::size_t shell = 4; shell < by_shell.size(); ++shell)
    if (by_shell[shell] != 0)
      std::printf("shell[%zu]=%lld\n", shell, by_shell[shell]);

  if (vertices < min_vertices) {
    std::fprintf(stderr, "PLANCHER non atteint: vertices=%lld < %lld\n", vertices,
                 min_vertices);
    return 3;
  }
  return 0;
}
