// MorseHGP3D v3 — PREMIÈRES INCIDENCES DU CŒUR : la dichotomie, mesurée et jugée.
//
// `audits/NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md` retire la source
// silencieuse de la liste des inconnues mathématiques. Pour une facette F du
// cœur, de miniboule fermée B_F et de niveau b_F, en posant
//
//     E_F = (B_F inter X) \ F,
//
// la première incidence se décide sans aucune recherche de voisinage :
//
//   * BRANCHE FERMÉE, E_F non vide. Alors lambda(F) = b_F et
//     M(F) = { F union {x} : x dans E_F }. La preuve tient en deux lignes : B_F
//     contient déjà F union {x}, donc le niveau ne bouge pas ; et réciproquement
//     une coface de niveau b_F a une miniboule qui est aussi miniboule de F,
//     donc B_F par unicité euclidienne. Ce lemme n'exige AUCUNE régularité.
//
//   * BRANCHE VIDE, E_F vide. Alors lambda(F) est le minimum des niveaux des
//     cofaces DIRECTES contenant F, et M(F) en est le groupe d'ex æquo. La
//     preuve montre que tout minimiseur est de Gabriel au sens ouvert : un
//     intrus strict dans sa miniboule fournirait une incidence strictement
//     moins chère. La complétude de la source directe suffit donc.
//
// Ce binaire ne remplace pas l'oracle général : il MESURE la dichotomie et la
// JUGE contre une vérité exhaustive écrite ici — pour chaque facette, le minimum
// de beta sur tous les points extérieurs, et l'ensemble complet de ses ex æquo.
//
// Ce qu'il n'est pas : ni un pipeline, ni un tri externe réel, ni une autorité
// de régularité. Le regroupement est en mémoire ; ce sont les VOLUMES qu'il
// publie — facettes, records de suppression, branches, co-minimiseurs, doublons
// — qui disent si le tri externe est viable.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <random>
#include <set>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "mhgp/miniball.hpp"
#include "prototype/order_k_flats.hpp"

using mhgp::P3;
using mhgp::i32;

static P3 pt(int x, int y, int z) {
  P3 p{};
  p.x = (i32)x; p.y = (i32)y; p.z = (i32)z;
  return p;
}

// Miniboule exacte d'un ensemble, et le test « la miniboule passe par tout
// l'ensemble » qui distingue une coface d'un sous-ensemble non critique.
static bool miniball_through_all(const std::vector<P3>& pts, const std::vector<i32>& set,
                                 mhgp::Sphere* out) {
  const mhgp::MiniballResult mb = mhgp::miniball_of(pts, set.data(), (int)set.size());
  if (!mb.ok) return false;
  *out = mb.sph;
  return true;
}

struct Dichotomy {
  long long direct_cofaces = 0;
  long long deletion_records = 0;
  long long core_facets = 0;
  long long closed_nonempty = 0;
  long long closed_empty = 0;
  long long cominimisers = 0;
  long long cominimisers_max = 0;
  long long duplicate_cofaces = 0;
  long long closed_ball_touched = 0;
  long long deletion_bytes = 0;
  long long disagreements = 0;
};

// Vérité exhaustive : lambda(F) et M(F) par balayage de tous les points
// extérieurs. Elle n'emprunte à la dichotomie ni la branche, ni la source
// directe — seulement la miniboule et le comparateur de niveau.
static void brute_first_incidence(const std::vector<P3>& pts, const std::vector<i32>& facet,
                                  mhgp::Sphere* level_out, std::set<std::vector<i32>>* set_out) {
  const int n = (int)pts.size();
  bool have = false;
  mhgp::Sphere best{};
  set_out->clear();
  for (i32 x = 0; x < n; ++x) {
    if (std::binary_search(facet.begin(), facet.end(), x)) continue;
    std::vector<i32> coface = facet;
    coface.push_back(x);
    std::sort(coface.begin(), coface.end());
    mhgp::Sphere sphere{};
    if (!miniball_through_all(pts, coface, &sphere)) continue;
    if (!have) { best = sphere; have = true; set_out->clear(); set_out->insert(coface); continue; }
    const int cmp = mhgp::sphere_cmp_beta(sphere, best);
    if (cmp < 0) { best = sphere; set_out->clear(); set_out->insert(coface); }
    else if (cmp == 0) set_out->insert(coface);
  }
  *level_out = best;
}

static void run_cloud(const std::vector<P3>& pts, int k, Dichotomy* out, bool judge) {
  const int n = (int)pts.size();
  if (k + 1 > n) return;

  // ---- source directe : les cofaces de Gabriel de cardinal k+1 --------------
  mhgp3v::FlatStatistics st{};
  mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
  const mhgp::Catalogue catalogue = mhgp3v::flat_catalogue(pts, k + 1, &st, &status, false, true);
  if (status != mhgp3v::CloudStatus::kOk) return;

  mhgp3v::CertifiedIndex index;
  index.build(pts, 16);

  // Une coface directe est une sphère critique dont la boule fermée compte
  // exactement k+1 points : c'est le K-simplexe de Gabriel du manuscrit.
  std::map<std::vector<i32>, mhgp::Sphere> direct;
  for (const mhgp::CriticalSphere& sphere : catalogue.spheres) {
    if (sphere.rank != k + 1) continue;
    std::vector<i32> coface;
    for (int i = 0; i < sphere.rank; ++i)
      coface.push_back(catalogue.members[(std::size_t)(sphere.members_begin + i)]);
    std::sort(coface.begin(), coface.end());
    if (!direct.emplace(coface, sphere.sph).second) ++out->duplicate_cofaces;
  }
  out->direct_cofaces += (long long)direct.size();

  // ---- flux de suppressions : k+1 records par coface directe ---------------
  struct Record { std::vector<i32> facet; mhgp::Sphere level; const std::vector<i32>* coface; };
  std::vector<Record> stream;
  for (const auto& entry : direct)
    for (std::size_t drop = 0; drop < entry.first.size(); ++drop) {
      std::vector<i32> facet;
      for (std::size_t i = 0; i < entry.first.size(); ++i)
        if (i != drop) facet.push_back(entry.first[i]);
      stream.push_back(Record{facet, entry.second, &entry.first});
    }
  out->deletion_records += (long long)stream.size();
  // Identité de masse : exactement k+1 records par coface directe.
  if ((long long)stream.size() != (long long)direct.size() * (k + 1)) ++out->disagreements;
  out->deletion_bytes += (long long)stream.size() * (long long)(k * 4 + 8 + 8);

  // ---- regroupement par facette --------------------------------------------
  std::map<std::vector<i32>, std::vector<std::size_t>> grouped;
  for (std::size_t i = 0; i < stream.size(); ++i)
    grouped[stream[i].facet].push_back(i);
  out->core_facets += (long long)grouped.size();

  for (const auto& group : grouped) {
    const std::vector<i32>& facet = group.first;
    mhgp::Sphere facet_ball{};
    if (!miniball_through_all(pts, facet, &facet_ball)) { ++out->disagreements; continue; }

    // E_F par requête de boule FERMÉE certifiée. Trouver un témoin ne suffit
    // pas : tous les co-minimiseurs du niveau exact sont dans le même lot.
    std::vector<i32> outside_in_ball;
    index.closed_ball(facet_ball, &out->closed_ball_touched, [&](i32 z) {
      if (!std::binary_search(facet.begin(), facet.end(), z)) outside_in_ball.push_back(z);
    });
    std::sort(outside_in_ball.begin(), outside_in_ball.end());

    std::set<std::vector<i32>> decided;
    if (!outside_in_ball.empty()) {
      ++out->closed_nonempty;
      for (i32 x : outside_in_ball) {
        std::vector<i32> coface = facet;
        coface.push_back(x);
        std::sort(coface.begin(), coface.end());
        decided.insert(coface);
      }
    } else {
      ++out->closed_empty;
      const mhgp::Sphere* best = nullptr;
      for (std::size_t i : group.second) {
        if (best == nullptr || mhgp::sphere_cmp_beta(stream[i].level, *best) < 0)
          best = &stream[i].level;
      }
      for (std::size_t i : group.second)
        if (best != nullptr && mhgp::sphere_cmp_beta(stream[i].level, *best) == 0)
          decided.insert(*stream[i].coface);
    }
    out->cominimisers += (long long)decided.size();
    out->cominimisers_max = std::max(out->cominimisers_max, (long long)decided.size());

    if (judge) {
      mhgp::Sphere truth_level{};
      std::set<std::vector<i32>> truth_set;
      brute_first_incidence(pts, facet, &truth_level, &truth_set);
      if (decided != truth_set) {
        ++out->disagreements;
        printf("  DESACCORD facette {");
        for (std::size_t i = 0; i < facet.size(); ++i) printf("%s%d", i ? "," : "", facet[i]);
        printf("} : dichotomie %zu cofaces, verite %zu (branche %s)\n", decided.size(),
               truth_set.size(), outside_in_ball.empty() ? "vide" : "fermee");
      }
    }
  }
}

int main(int argc, char** argv) {
  int clouds = 200, npoints = 12, coord = 24, k = 3;
  long long seed = 4242;
  bool judge = true;
  for (int i = 1; i < argc; ++i) {
    const bool has = (i + 1 < argc);
    if (!strcmp(argv[i], "--clouds") && has) clouds = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--points") && has) npoints = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--coord") && has) coord = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--k") && has) k = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--seed") && has) seed = atoll(argv[++i]);
    else if (!strcmp(argv[i], "--no-judge")) judge = false;
    else { printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
  }
  if (clouds < 1 || npoints < 2 || coord < 2 || coord > 65536 || k < 1 || k >= npoints) {
    printf("ECHEC : campagne absurde\n");
    return 2;
  }

  Dichotomy total;
  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick(0, coord - 1);
  for (int c = 0; c < clouds; ++c) {
    std::vector<P3> pts;
    for (int guard = 0; (int)pts.size() < npoints && guard < 100 * npoints; ++guard) {
      const P3 q = pt(pick(rng), pick(rng), pick(rng));
      bool seen = false;
      for (const P3& r : pts) if (r.x == q.x && r.y == q.y && r.z == q.z) seen = true;
      if (!seen) pts.push_back(q);
    }
    if ((int)pts.size() < npoints) { printf("ECHEC : nuage %d non genere\n", c); return 3; }
    run_cloud(pts, k, &total, judge);
  }

  printf("k=%d  nuages=%d  points=%d  grille=[0,%d)\n", k, clouds, npoints, coord);
  printf("source directe : cofaces=%lld  doublons=%lld  records de suppression=%lld"
         "  (%.2f par coface)\n", total.direct_cofaces, total.duplicate_cofaces,
         total.deletion_records,
         total.direct_cofaces ? (double)total.deletion_records / (double)total.direct_cofaces : 0.0);
  printf("dichotomie     : facettes du coeur=%lld  branche fermee=%lld (%.1f%%)"
         "  branche vide=%lld\n", total.core_facets, total.closed_nonempty,
         total.core_facets ? 100.0 * (double)total.closed_nonempty / (double)total.core_facets : 0.0,
         total.closed_empty);
  printf("co-minimiseurs : total=%lld  moyenne=%.2f  maximum=%lld\n", total.cominimisers,
         total.core_facets ? (double)total.cominimisers / (double)total.core_facets : 0.0,
         total.cominimisers_max);
  printf("index          : points touches par closed_ball=%lld (%.1f par facette)\n",
         total.closed_ball_touched,
         total.core_facets ? (double)total.closed_ball_touched / (double)total.core_facets : 0.0);
  printf("volume         : octets du flux de suppressions=%lld\n", total.deletion_bytes);
  printf("\n%lld desaccords contre la verite exhaustive\n", total.disagreements);
  if (total.core_facets == 0) {
    printf("ECHEC : aucune facette du coeur, la campagne ne mesure rien\n");
    return 3;
  }
  if (total.disagreements == 0)
    printf("OK : la dichotomie reproduit lambda(F) et M(F) exactement\n");
  return total.disagreements == 0 ? 0 : 1;
}
