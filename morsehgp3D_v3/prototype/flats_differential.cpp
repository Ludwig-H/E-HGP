// MorseHGP3D v3 — JUGE DIFFERENTIEL du parcours multiplicitaire `order_k_flats`.
//
// Ce binaire n'est pas un test unitaire : c'est la porte. Il compare le sujet a
// une verite ecrite ici, qui n'appelle ni le germe, ni les prédicats de pinceau,
// ni le transport du sujet — elle ne partage avec lui que `mhgp::sphere_side`.
//
// Trois comparaisons, pas une seule :
//
//   (A) LE SOMMET. Tous les sommets de l'arrangement sont enumeres en force
//       brute (quadruplets independants -> `sphere4` -> census exact), groupes
//       par coquille, filtres au niveau strict <= s_max - 2, puis compares au
//       parcours sur le couple (coquille, NIVEAU). Comparer des ensembles de
//       coquilles sans les niveaux laisserait passer un niveau faux : c'est
//       exactement ce qui s'est produit sur la fixture coplanaire.
//
//   (B) LE CATALOGUE. Tous les sous-ensembles de taille au plus quatre,
//       miniboule, census exact, deduplication par coquille, support canonique
//       relu sur la COQUILLE TRIEE PAR COORDONNEES.
//
//   (C) L'EQUIVARIANCE. Renumeroter le nuage ne doit rien changer au catalogue
//       une fois les supports ramenes aux identifiants d'origine. Sans ce
//       controle, la convention de support canonique la plus naturelle — le plus
//       petit sous-ensemble par IDENTIFIANT — passe les deux premieres portes et
//       publie pourtant un support different selon la numerotation, des qu'une
//       miniboule a plusieurs supports minimaux. Le cube en a quatre.
//
// Le census exact par sommet est actif pendant toute la campagne : le transport
// n'est jamais autorite, il est refute ou confirme a chaque sommet.
//
// Les fixtures portent les coordonnees EXACTES publiees par les audits de
// `audits/`. Chacune a refute une affirmation ; la liste est dans le README.
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

// ---------------------------------------------------------------- (A) sommets
static std::map<std::vector<i32>, int> brute_vertices(const std::vector<P3>& pts) {
  std::map<std::vector<i32>, int> out;   // coquille -> niveau strict
  const int n = (int)pts.size();
  for (i32 a = 0; a < n; ++a)
    for (i32 b = a + 1; b < n; ++b)
      for (i32 c = b + 1; c < n; ++c)
        for (i32 d = c + 1; d < n; ++d) {
          mhgp::Sphere s{};
          if (!mhgp::sphere4(pts[(size_t)a], pts[(size_t)b], pts[(size_t)c], pts[(size_t)d], &s))
            continue;                                   // coplanaires : pas un sommet
          std::vector<i32> shell;
          int level = 0;
          for (i32 z = 0; z < n; ++z) {
            const int side = mhgp::sphere_side(s, pts[(size_t)z]);
            if (side < 0) ++level;
            else if (side == 0) shell.push_back(z);
          }
          out[shell] = level;
        }
  return out;
}

// -------------------------------------------------------------- (B) catalogue
struct Truth {
  std::set<std::vector<i32>> shells;
  std::set<std::pair<std::vector<i32>, int>> spheres;
  int per_arity[5] = {};
};

static Truth brute_catalogue(const std::vector<P3>& pts, int s_max) {
  Truth t;
  const int n = (int)pts.size();
  auto consider = [&](std::vector<i32> u) {
    const mhgp::MiniballResult mb = mhgp::miniball_of(pts, u.data(), (int)u.size());
    if (!mb.ok) return;
    for (i32 z : u)
      if (mhgp::sphere_side(mb.sph, pts[(size_t)z]) != 0) return;
    std::vector<i32> shell;
    int rank = 0;
    for (i32 z = 0; z < n; ++z) {
      const int side = mhgp::sphere_side(mb.sph, pts[(size_t)z]);
      if (side > 0) continue;
      ++rank;
      if (side == 0) shell.push_back(z);
    }
    if (rank > s_max) return;
    if (!t.shells.insert(shell).second) return;
    std::vector<i32> by_coord = shell;
    std::sort(by_coord.begin(), by_coord.end(), [&](i32 x, i32 y) {
      const P3& u = pts[(size_t)x]; const P3& w = pts[(size_t)y];
      if (u.x != w.x) return u.x < w.x;
      if (u.y != w.y) return u.y < w.y;
      if (u.z != w.z) return u.z < w.z;
      return x < y;
    });
    const mhgp::MiniballResult cm = mhgp::miniball_of(pts, by_coord.data(), (int)by_coord.size());
    if (!cm.ok) return;
    std::vector<i32> sup(cm.support, cm.support + cm.n_support);
    t.spheres.insert({sup, rank});
    ++t.per_arity[cm.n_support];
  };
  for (i32 a = 0; a < n; ++a) {
    consider({a});
    for (i32 b = a + 1; b < n; ++b) {
      consider({a, b});
      for (i32 c = b + 1; c < n; ++c) {
        consider({a, b, c});
        for (i32 d = c + 1; d < n; ++d) consider({a, b, c, d});
      }
    }
  }
  return t;
}

static void dump(const std::vector<P3>& pts) {
  for (const P3& q : pts) printf(" (%d,%d,%d)", (int)q.x, (int)q.y, (int)q.z);
  printf("\n");
}

static bool compare(const char* tag, const std::vector<P3>& pts, int s_max, bool verbose) {
  mhgp3v::FlatStatistics st{};
  mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
  const mhgp::Catalogue cat = mhgp3v::flat_catalogue(pts, s_max, &st, &status, true);

  bool ok = st.census_mismatch_shell == 0 && st.census_mismatch_level == 0 &&
            status != mhgp3v::CloudStatus::kSeedFailed &&
            status != mhgp3v::CloudStatus::kInvariantViolated;

  int missing_vertices = 0, extra_vertices = 0, wrong_level = 0;
  if (status == mhgp3v::CloudStatus::kOk && (int)pts.size() >= 4) {
    mhgp3v::FlatStatistics nav_st{};
    mhgp3v::CloudStatus nav_status = mhgp3v::CloudStatus::kOk;
    const auto visited = mhgp3v::navigate_shallow(pts, s_max - 2, &nav_st, &nav_status, true);
    std::map<std::vector<i32>, int> got_v;
    for (const auto& v : visited) got_v[v.shell] = v.level;
    const auto truth_v = brute_vertices(pts);
    for (const auto& kv : truth_v) {
      if (kv.second > s_max - 2) continue;
      auto it = got_v.find(kv.first);
      if (it == got_v.end()) ++missing_vertices;
      else if (it->second != kv.second) ++wrong_level;
    }
    for (const auto& kv : got_v)
      if (truth_v.find(kv.first) == truth_v.end()) ++extra_vertices;
    if (missing_vertices || extra_vertices || wrong_level) ok = false;
  }

  const Truth t = brute_catalogue(pts, s_max);
  std::set<std::pair<std::vector<i32>, int>> got;
  int per[5] = {};
  for (const mhgp::CriticalSphere& s : cat.spheres) {
    std::vector<i32> sup(s.support, s.support + s.n_support);
    got.insert({sup, s.rank});
    ++per[s.n_support];
  }
  if (got != t.spheres) ok = false;

  if (!ok || verbose) {
    printf("[%s] s_max=%2d statut=%-34s  catalogue %zu/%zu  arites %d/%d/%d/%d contre %d/%d/%d/%d\n",
           tag, s_max, mhgp3v::cloud_status_name(status), got.size(), t.spheres.size(),
           per[1], per[2], per[3], per[4],
           t.per_arity[1], t.per_arity[2], t.per_arity[3], t.per_arity[4]);
    printf("        sommets: manquants=%d surnumeraires=%d niveau_faux=%d | visites=%lld"
           " coquilles>4=%lld flats=%lld quotientes=%lld lots>1=%lld"
           " census=%lld/%lld/%lld emis=%lld doublons=%lld sur_niveau=%lld germe=%lld\n",
           missing_vertices, extra_vertices, wrong_level, st.vertices_visited,
           st.shells_multiple, st.flats_enumerated, st.triples_quotiented,
           st.batches_multiple, st.census_checks, st.census_mismatch_shell,
           st.census_mismatch_level, st.emit_attempts, st.emit_duplicate_shell,
           st.vertices_over_level, st.seed_failure_stage);
  }
  if (!ok) {
    printf("        points :");
    dump(pts);
    for (const auto& s : t.spheres)
      if (!got.count(s)) {
        printf("        MANQUANT support={");
        for (size_t i = 0; i < s.first.size(); ++i) printf("%s%d", i ? "," : "", s.first[i]);
        printf("} rang=%d\n", s.second);
      }
    for (const auto& s : got)
      if (!t.spheres.count(s)) {
        printf("        SURNUMERAIRE support={");
        for (size_t i = 0; i < s.first.size(); ++i) printf("%s%d", i ? "," : "", s.first[i]);
        printf("} rang=%d\n", s.second);
      }
  }
  return ok;
}

// Équivariance par permutation : renuméroter les points ne doit rien changer au
// catalogue, une fois les supports ramenés aux identifiants d'origine.
static bool permutation_equivariant(const char* tag, const std::vector<P3>& pts, int s_max,
                                    int trials, std::mt19937& rng) {
  auto signature = [&](const std::vector<P3>& q, const std::vector<int>& back) {
    mhgp3v::FlatStatistics st{};
    mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
    const mhgp::Catalogue cat = mhgp3v::flat_catalogue(q, s_max, &st, &status, false);
    std::set<std::pair<std::vector<i32>, int>> out;
    for (const mhgp::CriticalSphere& s : cat.spheres) {
      std::vector<i32> sup;
      for (int i = 0; i < s.n_support; ++i) sup.push_back((i32)back[(size_t)s.support[i]]);
      std::sort(sup.begin(), sup.end());
      out.insert({sup, s.rank});
    }
    return out;
  };
  std::vector<int> identity((size_t)pts.size());
  for (size_t i = 0; i < identity.size(); ++i) identity[i] = (int)i;
  const auto reference = signature(pts, identity);
  for (int t = 0; t < trials; ++t) {
    std::vector<int> perm = identity;
    std::shuffle(perm.begin(), perm.end(), rng);
    std::vector<P3> q((size_t)pts.size());
    std::vector<int> back((size_t)pts.size());
    for (size_t i = 0; i < perm.size(); ++i) { q[i] = pts[(size_t)perm[i]]; back[i] = perm[i]; }
    if (signature(q, back) != reference) {
      printf("[%s] s_max=%d NON EQUIVARIANT a la permutation %d\n", tag, s_max, t);
      return false;
    }
  }
  return true;
}

int main(int argc, char** argv) {
  int clouds = 400, npoints = 11, coord = 24, smax_hi = 6, min_cases = 1;
  unsigned seed = 4242;
  for (int i = 1; i < argc; ++i) {
    const bool has_value = (i + 1 < argc);
    if (!strcmp(argv[i], "--clouds") && has_value) clouds = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--points") && has_value) npoints = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--coord") && has_value) coord = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--smax") && has_value) smax_hi = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--seed") && has_value) seed = (unsigned)atoi(argv[++i]);
    else if (!strcmp(argv[i], "--min-cases") && has_value) min_cases = atoi(argv[++i]);
    else {
      printf("ECHEC : argument inconnu %s\n", argv[i]);
      return 2;
    }
  }
  if (clouds < 0 || npoints < 1 || coord < 2 || smax_hi < 2 || min_cases < 1) {
    printf("ECHEC : campagne absurde (clouds<0, points<1, coord<2, smax<2 ou min-cases<1)\n");
    return 2;
  }

  int failures = 0, cases = 0;
  std::mt19937 rng(seed);

  struct Fix { const char* name; std::vector<P3> pts; };
  std::vector<Fix> fixtures;

  fixtures.push_back({"coplanar_constant_witness",
                      {pt(4, 1, 0), pt(14, 19, 0), pt(4, 17, 0), pt(17, 9, 0), pt(15, 8, 19)}});
  {
    std::vector<P3> cube;
    for (int x = 0; x <= 2; x += 2) for (int y = 0; y <= 2; y += 2) for (int z = 0; z <= 2; z += 2)
      cube.push_back(pt(x, y, z));
    fixtures.push_back({"cube", cube});
  }
  fixtures.push_back({"constant_shell_members",
                      {pt(3, 2, 2), pt(2, 3, 2), pt(1, 2, 2), pt(2, 1, 2), pt(2, 2, 3), pt(2, 2, 5)}});
  fixtures.push_back({"bridge_shell5",
                      {pt(10, 14, 13), pt(10, 10, 15), pt(6, 10, 13), pt(6, 10, 7), pt(10, 13, 6),
                       pt(12, 9, 3), pt(9, 9, 1), pt(18, 10, 2), pt(7, 11, 15)}});
  fixtures.push_back({"unreachable_extra_shell",
                      {pt(0, 0, 0), pt(2, 0, 0), pt(1, 1, 0), pt(0, 3, 4), pt(5, 2, 3)}});
  fixtures.push_back({"noncritical_shell_tie",
                      {pt(1065, 1000, 100), pt(1063, 1016, 100), pt(1060, 1025, 100),
                       pt(1056, 1033, 100)}});
  fixtures.push_back({"unit_increment_refutation",
                      {pt(0, 2, 0), pt(4, 2, 0), pt(2, 3, 0)}});
  fixtures.push_back({"non_well_centred_vertex",
                      {pt(0, 0, 0), pt(2, 0, 0), pt(0, 2, 0), pt(0, 0, 2)}});
  fixtures.push_back({"regular_tetrahedron",
                      {pt(2, 2, 2), pt(2, 0, 0), pt(0, 2, 0), pt(0, 0, 2)}});
  fixtures.push_back({"giant_centre_det1",
                      {pt(0, 0, 0), pt(1, 0, 0), pt(65535, 1, 0), pt(65535, 65535, 1)}});
  fixtures.push_back({"radius2_of_P0",
                      {pt(0, 0, 0), pt(4, 0, 0), pt(1, 1, 0), pt(1, 0, 1)}});
  fixtures.push_back({"well_centred_not_small",
                      {pt(40000, 40000, 40000), pt(40000, 0, 0), pt(0, 40000, 0), pt(0, 0, 40000)}});
  fixtures.push_back({"Q1_decisive",
                      {pt(10, 10, 1), pt(10, 10, 9), pt(13, 13, 5), pt(13, 7, 5), pt(14, 9, 6),
                       pt(11, 6, 6)}});
  fixtures.push_back({"partial_catalogue_on_reject",
                      {pt(5, 7, 6), pt(7, 3, 5), pt(5, 1, 4), pt(3, 5, 0), pt(7, 3, 2), pt(1, 0, 2),
                       pt(6, 6, 3)}});
  fixtures.push_back({"base_n2", {pt(0, 0, 0), pt(2, 0, 0)}});
  fixtures.push_back({"base_n3", {pt(0, 0, 0), pt(4, 0, 0), pt(1, 3, 0)}});
  fixtures.push_back({"coplanaire_pur",
                      {pt(0, 0, 0), pt(4, 0, 0), pt(0, 4, 0), pt(4, 4, 0), pt(2, 1, 0)}});
  fixtures.push_back({"germe_demi_tour",
                      {pt(26, 30, 33), pt(27, 30, 34), pt(27, 30, 26), pt(34, 30, 33),
                       pt(30, 33, 26), pt(25, 30, 25), pt(35, 31, 30)}});
  fixtures.push_back({"germe_arete_traversee",
                      {pt(0, 0, 0), pt(2, 0, 0), pt(4, 0, 0), pt(2, 4, 0), pt(2, 2, 6), pt(1, 3, 2)}});

  printf("=== fixtures ===\n");
  for (const Fix& f : fixtures) {
    for (int s = 2; s <= 8; ++s) { ++cases; if (!compare(f.name, f.pts, s, false)) ++failures; }
    ++cases;
    if (!permutation_equivariant(f.name, f.pts, 5, 24, rng)) ++failures;
  }

  printf("=== nuages generiques (%d nuages, %d points, coord < %d) ===\n", clouds, npoints, coord);
  std::uniform_int_distribution<int> dist(0, coord - 1);
  for (int c = 0; c < clouds; ++c) {
    std::vector<P3> pts;
    for (int i = 0; i < npoints; ++i) pts.push_back(pt(dist(rng), dist(rng), dist(rng)));
    for (int s = 2; s <= smax_hi; ++s) {
      ++cases;
      char tag[64];
      snprintf(tag, sizeof tag, "alea#%d", c);
      if (!compare(tag, pts, s, false)) ++failures;
    }
  }

  printf("=== nuages a cospheries forcees ===\n");
  std::vector<P3> sphere_pts;
  for (int x = -5; x <= 5; ++x) for (int y = -5; y <= 5; ++y) for (int z = -5; z <= 5; ++z)
    if (x * x + y * y + z * z == 25) sphere_pts.push_back(pt(x + 30, y + 30, z + 30));
  for (int c = 0; c < clouds / 4; ++c) {
    std::vector<P3> pts;
    std::shuffle(sphere_pts.begin(), sphere_pts.end(), rng);
    const int take = 4 + (int)(rng() % 5);
    for (int i = 0; i < take && i < (int)sphere_pts.size(); ++i) pts.push_back(sphere_pts[(size_t)i]);
    const int extra = 2 + (int)(rng() % 4);
    std::uniform_int_distribution<int> d2(24, 36);
    for (int i = 0; i < extra; ++i) pts.push_back(pt(d2(rng), d2(rng), d2(rng)));
    for (int s = 2; s <= smax_hi; ++s) {
      ++cases;
      char tag[64];
      snprintf(tag, sizeof tag, "cospherique#%d", c);
      if (!compare(tag, pts, s, false)) ++failures;
    }
  }

  printf("\n%d cas, %d desaccords\n", cases, failures);
  if (cases < min_cases) {
    printf("ECHEC : plancher non atteint, %d cas pour %d exiges — une campagne vide "
           "ou censuree ne peut pas rendre OK\n", cases, min_cases);
    return 3;
  }
  if (failures == 0) printf("OK : sommets, catalogue, census et equivariance concordants\n");
  return failures == 0 ? 0 : 1;
}
