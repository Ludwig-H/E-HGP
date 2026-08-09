// MorseHGP3D v3 — JUGE DIFFERENTIEL du parcours multiplicitaire `order_k_flats`.
//
// Ce binaire n'est pas un test unitaire : c'est la porte. Il compare le sujet a
// une verite ecrite ici, qui n'appelle ni le germe, ni les predicats de pinceau,
// ni le transport du sujet.
//
// PORTEE EXACTE DE SON AUTORITE, et elle est plus etroite que ce que j'avais
// ecrit. La verite partage avec le sujet trois primitives de `morsehgp3D_v2` :
// `mhgp::sphere_side`, `mhgp::sphere4` pour construire les sommets exhaustifs,
// et `mhgp::miniball_of` pour decider les candidats et relire le support
// canonique. Une faute commune de miniboule, de bon centrage ou de convention
// de support serait donc INVISIBLE ici. Ce juge etablit « portee de navigation
// et catalogue concordants RELATIVEMENT a ces primitives », pas « catalogue
// critique exact ». L'autorite independante manquante est une reference
// rationnelle multiplicitaire dans l'oracle M1, qui n'existe pas.
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
#include <array>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <random>
#include <set>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "mhgp/miniball.hpp"
#include "prototype/order_k_device_core.hpp"
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


// ---------------------------------------------------------------------------
// Puissance rationnelle exacte, pour le SEUL usage du juge.
//
// Pour une sphere `base + num/den`, la puissance d'un point z vaut
// L(z) = |w|^2 - 2 <w, num>/den avec w = z - base, donc son NUMERATEUR sur den
// est |w|^2 * den - 2 <w, num> : c'est exactement la quantite dont
// `mhgp::sphere_side` rend le signe. Le parent n'en a pas besoin — il se decide
// sur des signes tangents — mais le juge doit comparer L_h entre DEUX spheres
// differentes, ce qu'un signe ne permet pas. On recalcule donc le numerateur
// ici, sans elargir l'API de production.
//
// Largeurs : |w|^2 sous 2^35,6 et den sous 2^73 donnent 2^108,6 ; le second
// terme est sous 2^108. Le numerateur tient dans `i128`, et le produit croise
// de deux numerateurs par les denominateurs opposes tient dans `BigInt<4>`.
static mhgp::i128 power_numerator(const mhgp::Sphere& sphere, const P3& z) {
  const P3 w = mhgp::p3_sub(z, sphere.base);
  return mhgp::p3_norm2(w) * sphere.den
       - 2 * ((mhgp::i128)w.x * sphere.nx + (mhgp::i128)w.y * sphere.ny
              + (mhgp::i128)w.z * sphere.nz);
}

static bool shell_sphere_of(const std::vector<P3>& pts, const std::vector<i32>& shell,
                            mhgp::Sphere* out) {
  const int m = (int)shell.size();
  for (int a = 0; a < m; ++a)
    for (int b = a + 1; b < m; ++b)
      for (int c = b + 1; c < m; ++c)
        for (int d = c + 1; d < m; ++d)
          if (mhgp::sphere4(pts[(size_t)shell[(size_t)a]], pts[(size_t)shell[(size_t)b]],
                            pts[(size_t)shell[(size_t)c]], pts[(size_t)shell[(size_t)d]], out))
            return true;
  return false;
}

// Signe de L_h(child) - L_h(parent), par produit croise sur les denominateurs.
static int compare_power(const std::vector<P3>& pts, const std::vector<i32>& child_shell,
                         const std::vector<i32>& parent_shell, i32 site) {
  mhgp::Sphere a{}, b{};
  if (!shell_sphere_of(pts, child_shell, &a) || !shell_sphere_of(pts, parent_shell, &b)) return 0;
  const mhgp::BigInt<4> left = mhgp::mul128(power_numerator(a, pts[(size_t)site]), b.den);
  const mhgp::BigInt<4> right = mhgp::mul128(power_numerator(b, pts[(size_t)site]), a.den);
  return mhgp::big_cmp(left, right);
}

// Signe de Q_r(child) - Q_r(parent) sur la base independante du germe.
static int compare_potential(const std::vector<P3>& pts, const std::vector<i32>& child_shell,
                             const std::vector<i32>& parent_shell,
                             const std::vector<i32>& base) {
  mhgp::Sphere a{}, b{};
  if (!shell_sphere_of(pts, child_shell, &a) || !shell_sphere_of(pts, parent_shell, &b)) return 0;
  mhgp::i128 qa = 0, qb = 0;
  for (i32 z : base) {
    qa += power_numerator(a, pts[(size_t)z]);
    qb += power_numerator(b, pts[(size_t)z]);
  }
  return mhgp::big_cmp(mhgp::mul128(qa, b.den), mhgp::mul128(qb, a.den));
}

// Meme extraction gloutonne que le sujet, rejouee ici par le juge.
static void independent_base(const std::vector<P3>& pts, const std::vector<i32>& shell,
                             std::vector<i32>* out) {
  out->clear();
  for (i32 z : shell) {
    if (out->size() >= 4) break;
    const size_t have = out->size();
    if (have == 1) {
      const P3& u = pts[(size_t)(*out)[0]];
      const P3& w = pts[(size_t)z];
      if (u.x == w.x && u.y == w.y && u.z == w.z) continue;
    } else if (have == 2) {
      const P3 u = mhgp::p3_sub(pts[(size_t)(*out)[1]], pts[(size_t)(*out)[0]]);
      const P3 w = mhgp::p3_sub(pts[(size_t)z], pts[(size_t)(*out)[0]]);
      const P3 c = mhgp::p3_cross(u, w);
      if (c.x == 0 && c.y == 0 && c.z == 0) continue;
    } else if (have == 3) {
      if (mhgp3v::flats::orient3d_exact(pts[(size_t)(*out)[0]], pts[(size_t)(*out)[1]],
                                        pts[(size_t)(*out)[2]], pts[(size_t)z]) == 0) continue;
    }
    out->push_back(z);
  }
}

static void dump(const std::vector<P3>& pts) {
  for (const P3& q : pts) printf(" (%d,%d,%d)", (int)q.x, (int)q.y, (int)q.z);
  printf("\n");
}

// Couverture reellement exercee. Sans plancher, une regression qui classerait
// tous les nuages en dimension affine inferieure ferait comparer l'exhaustif du
// sujet a l'exhaustif de la verite et garderait toute la porte verte SANS
// jamais exercer la navigation. Chaque campagne doit donc exiger un minimum de
// nuages navigues, de sommets, de coquilles multiples et de triplets quotientes.
struct Coverage {
  long long navigated_clouds = 0;
  long long direct_clouds = 0;
  long long refused_clouds = 0;
  long long vertices = 0;
  long long census = 0;
  long long multiple_shells = 0;
  long long quotiented = 0;
  long long multiple_batches = 0;
  long long equivariance_runs = 0;
  long long grid_touched = 0;
  long long bootstrap = 0;
  long long full_sweeps = 0;
  long long indexed_runs = 0;
  long long parent_vertices = 0;
  long long parent_roots = 0;
  long long reverse_vertices = 0;
  long long reverse_depth = 0;
  long long reverse_children = 0;
  long long reverse_flats = 0;
  long long reverse_decisions = 0;
  long long reverse_skipped = 0;
  long long reverse_indexed_vertices = 0;
  long long reverse_triplets = 0;
  long long reverse_closures = 0;
  long long index_internal_nodes = 0;
  long long index_nodes_visited = 0;
  long long index_leaves_visited = 0;
  long long sink_vertices = 0;
  long long sink_high_water = 0;
  long long sink_interruptions = 0;
  long long parent_early_closures = 0;
  long long parent_full_closures = 0;
  long long reject_backward = 0;
  long long reject_by_parent = 0;
  long long pairs_judged = 0;
  long long pairs_accepted = 0;
  long long pairs_prefiltered = 0;
  long long owner_rejected_support = 0;
  long long owner_rejected_vertex = 0;
  long long owner_emitted = 0;
  long long dedup_table_max = 0;
  long long device_admitted = 0;
  long long device_refused = 0;
  long long device_pairs = 0;
  long long shell_high_water = 0;
  long long closure_high_water = 0;
  long long touched_high_water = 0;
  long long batch_high_water = 0;
  long long interior_high_water = 0;
  long long exhaustive_scans = 0;
  long long admission_samples = 0;
  long long admission_overflow = 0;
};
static Coverage coverage;

static bool has_duplicates(const std::vector<P3>& pts) {
  std::vector<std::array<long long, 3>> v;
  for (const P3& p : pts) v.push_back({(long long)p.x, (long long)p.y, (long long)p.z});
  std::sort(v.begin(), v.end());
  return std::adjacent_find(v.begin(), v.end()) != v.end();
}

// ADMISSIBILITE RECALCULEE PAR LE JUGE, sans emprunter `pair_admissible` au sujet.
// Le determinant est ecrit ici, ligne par ligne, et les deux potentiels sont
// reformules depuis la definition : partager la primitive faisait juger la
// suppression de la seconde requete, jamais les SIGNES eux-memes.
static __int128 judge_orient3d(const P3& a, const P3& b, const P3& c, const P3& d) {
  const __int128 ux = (__int128)b.x - a.x, uy = (__int128)b.y - a.y, uz = (__int128)b.z - a.z;
  const __int128 vx = (__int128)c.x - a.x, vy = (__int128)c.y - a.y, vz = (__int128)c.z - a.z;
  const __int128 wx = (__int128)d.x - a.x, wy = (__int128)d.y - a.y, wz = (__int128)d.z - a.z;
  return ux * (vy * wz - vz * wy) - uy * (vx * wz - vz * wx) + uz * (vx * wy - vy * wx);
}

static bool judge_admissible(const std::vector<P3>& pts, const mhgp3v::flats::Vertex& w,
                             const i32 base[3], int direction,
                             const std::vector<i32>& root_base) {
  if (direction != -1 && direction != 1) return false;
  const P3& a = pts[(size_t)base[0]];
  const P3& b = pts[(size_t)base[1]];
  const P3& c = pts[(size_t)base[2]];
  // Rester dans la chambre : le signe tangent de chaque point de coquille est >= 0.
  for (i32 z : w.shell) {
    const __int128 o = judge_orient3d(a, b, c, pts[(size_t)z]);
    const __int128 tangent = (direction > 0) ? -o : o;
    if (tangent < 0) return false;
  }
  if (!w.interior.empty()) {
    // Niveau positif : L_h doit croitre STRICTEMENT pour h = min B(w).
    i32 h = w.interior.front();
    for (i32 z : w.interior) if (z < h) h = z;
    const __int128 o = judge_orient3d(a, b, c, pts[(size_t)h]);
    const __int128 tangent = (direction > 0) ? -o : o;
    return tangent > 0;
  }
  // Niveau zero : Q_r doit decroitre STRICTEMENT sur la base independante. Ce sont
  // les VALEURS de `orient3d` qui se somment, pas leurs signes : la derivee de Q_r
  // est une somme de formes affines, et sommer des signes est faux. J'avais deja
  // commis cette faute dans le sujet et corrigee ; je l'ai reintroduite ici, et
  // c'est ce juge independant qui l'a rendue visible sur cinq couples.
  __int128 total = 0;
  for (i32 z : root_base) total += judge_orient3d(a, b, c, pts[(size_t)z]);
  const __int128 derivative = (direction > 0) ? -total : total;
  return derivative < 0;
}

static bool compare(const char* tag, const std::vector<P3>& pts, int s_max, bool verbose) {
  mhgp3v::FlatStatistics st{};
  mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
  const mhgp::Catalogue cat = mhgp3v::flat_catalogue(pts, s_max, &st, &status, true);

  // GARDE DE DOMAINE SYMETRIQUE. Deux observations confondues sont hors contrat :
  // le sujet doit REFUSER, et refuser exactement dans ce cas. Un refus sur un
  // nuage sain, ou une publication sur un nuage a doublon, est un echec.
  const bool duplicated = has_duplicates(pts);
  if (duplicated != (status == mhgp3v::CloudStatus::kDuplicateCoordinates)) {
    printf("[%s] s_max=%2d DOMAINE desaccord : doublons=%d statut=%s\n", tag, s_max,
           (int)duplicated, mhgp3v::cloud_status_name(status));
    return false;
  }
  if (duplicated) {
    if (!cat.spheres.empty() || !cat.members.empty()) {
      printf("[%s] s_max=%2d REFUS NON TRANSACTIONNEL : %zu spheres publiees\n", tag, s_max,
             cat.spheres.size());
      return false;
    }
    ++coverage.refused_clouds;
    return true;
  }

  bool ok = st.census_mismatch_shell == 0 && st.census_mismatch_level == 0 &&
            status != mhgp3v::CloudStatus::kSeedFailed &&
            status != mhgp3v::CloudStatus::kInvariantViolated;
  if (status == mhgp3v::CloudStatus::kOk && (int)pts.size() >= 4) ++coverage.navigated_clouds;
  else if (status == mhgp3v::CloudStatus::kSeedFailed ||
           status == mhgp3v::CloudStatus::kInvariantViolated ||
           status == mhgp3v::CloudStatus::kDuplicateCoordinates) ++coverage.refused_clouds;
  else ++coverage.direct_clouds;
  coverage.vertices += st.vertices_visited;
  coverage.census += st.census_checks;
  coverage.multiple_shells += st.shells_multiple;
  coverage.quotiented += st.triples_quotiented;
  coverage.multiple_batches += st.batches_multiple;

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

  // (D) INDEX CONTRE REFERENCE. Le chemin indexe doit rendre EXACTEMENT le meme
  // catalogue que le balayage complet — mêmes supports, mêmes rangs, mêmes
  // membres, même ordre, même statut. Un index qui rate un point produirait un
  // catalogue plus petit, et sans cette porte seul un désaccord avec la force
  // brute le révélerait, c'est-à-dire beaucoup plus tard et beaucoup plus mal.
  {
    mhgp3v::FlatStatistics ist{};
    mhgp3v::CloudStatus istatus = mhgp3v::CloudStatus::kOk;
    const mhgp::Catalogue icat = mhgp3v::flat_catalogue(pts, s_max, &ist, &istatus, true, true);
    bool identical = (istatus == status) && icat.spheres.size() == cat.spheres.size() &&
                     icat.members == cat.members;
    for (size_t i = 0; identical && i < icat.spheres.size(); ++i) {
      const mhgp::CriticalSphere& a = icat.spheres[i];
      const mhgp::CriticalSphere& b = cat.spheres[i];
      if (a.n_support != b.n_support || a.rank != b.rank ||
          a.members_begin != b.members_begin) identical = false;
      for (int q = 0; q < mhgp::kMaxSupport && identical; ++q)
        if (a.support[q] != b.support[q]) identical = false;
      if (identical && mhgp::sphere_cmp_beta(a.sph, b.sph) != 0) identical = false;
    }
    if (!identical) {
      printf("[%s] s_max=%2d INDEX != REFERENCE : statut %s contre %s, %zu contre %zu spheres\n",
             tag, s_max, mhgp3v::cloud_status_name(istatus),
             mhgp3v::cloud_status_name(status), icat.spheres.size(), cat.spheres.size());
      ok = false;
    }
    coverage.grid_touched += ist.grid_points_touched;
    coverage.bootstrap += ist.bootstrap_rounds;
    coverage.full_sweeps += ist.full_grid_sweeps;
    coverage.indexed_runs += 1;
  }

  // (E) GATE D — LE PARENT LOCAL, avec les quatre assertions que la note exige
  // avant tout remplacement du parcours : rang trois de la fermeture C(d),
  // identite S(next) = C union A, finitude de l'extremite, et STRICTE variation
  // du potentiel — hausse de L_h a ensemble interieur egal, ou baisse de Q_r au
  // niveau zero. Les trois premieres se lisent sur les ensembles ; la quatrieme
  // demande de comparer deux puissances rationnelles portees par des spheres
  // DIFFERENTES, donc le numerateur brut de `sphere_side` et un produit croise.
  if (status == mhgp3v::CloudStatus::kOk && (int)pts.size() >= 4) {
    mhgp3v::FlatStatistics pst{};
    mhgp3v::CloudStatus pstatus = mhgp3v::CloudStatus::kOk;
    std::vector<mhgp3v::flats::ParentEdge> parents;
    const auto seen_vertices =
        mhgp3v::navigate_shallow(pts, s_max - 2, &pst, &pstatus, false, nullptr, &parents);
    // FAIL-CLOSED. Sauter le bloc quand le second parcours echoue ou quand le
    // vecteur de parents n'a pas la bonne taille rendrait la porte muette
    // exactement dans les cas ou elle doit rougir.
    if (pstatus != mhgp3v::CloudStatus::kOk || parents.size() != seen_vertices.size()) {
      printf("[%s] s_max=%2d GATE D INEXPLOITABLE : statut=%s parents=%zu sommets=%zu\n", tag,
             s_max, mhgp3v::cloud_status_name(pstatus), parents.size(), seen_vertices.size());
      ok = false;
    } else {
      std::map<std::vector<i32>, size_t> position;
      for (size_t i = 0; i < seen_vertices.size(); ++i) position[seen_vertices[i].shell] = i;
      int roots = 0, missing_parent = 0, unknown_parent = 0, not_included = 0, cyclic = 0;
      int bad_closure = 0, bad_transition = 0, bad_potential = 0;
      for (size_t i = 0; i < seen_vertices.size(); ++i) {
        if (parents[i].shell.empty()) { ++roots; continue; }
        const auto it = position.find(parents[i].shell);
        if (it == position.end()) { ++unknown_parent; continue; }
        const auto& child = seen_vertices[i];
        const auto& mother = seen_vertices[it->second];

        // (1) rang trois de C(d) : la fermeture est incluse dans la coquille et
        // porte un triple non aligne, donc son enveloppe affine est un plan.
        const auto& closure = parents[i].closure;
        bool inside_shell = true;
        for (i32 z : closure)
          if (!std::binary_search(child.shell.begin(), child.shell.end(), z)) inside_shell = false;
        bool has_triangle = false;
        for (size_t a = 0; a + 2 < closure.size() + 1 && !has_triangle; ++a)
          for (size_t b = a + 1; b + 1 < closure.size() + 1 && !has_triangle; ++b)
            for (size_t c = b + 1; c < closure.size() && !has_triangle; ++c) {
              const mhgp::P3 u = mhgp::p3_sub(pts[(size_t)closure[b]], pts[(size_t)closure[a]]);
              const mhgp::P3 w = mhgp::p3_sub(pts[(size_t)closure[c]], pts[(size_t)closure[a]]);
              const mhgp::P3 x = mhgp::p3_cross(u, w);
              if (x.x != 0 || x.y != 0 || x.z != 0) has_triangle = true;
            }
        if (!inside_shell || !has_triangle) ++bad_closure;

        // (2) S(next) = C union A, et C = S(v) inter S(w) : deux spheres
        // distinctes d'un meme pinceau se coupent exactement selon le cercle du
        // flat, donc l'intersection des coquilles EST la fermeture.
        std::vector<i32> common;
        std::set_intersection(child.shell.begin(), child.shell.end(), mother.shell.begin(),
                              mother.shell.end(), std::back_inserter(common));
        if (common != closure) ++bad_transition;
        for (i32 z : closure)
          if (!std::binary_search(mother.shell.begin(), mother.shell.end(), z)) ++bad_transition;

        // (3) inclusion des ensembles interieurs
        for (i32 z : mother.interior)
          if (!std::binary_search(child.interior.begin(), child.interior.end(), z)) {
            ++not_included;
            break;
          }

        // (4) STRICTE variation du potentiel.
        // Le parent a un niveau AU PLUS egal a celui du fils : c'est
        // |B(parent)| < |B(fils)| qui est le bon cas, pas l'inverse.
        if (mother.interior.size() < child.interior.size()) {
          // niveau strictement decroissant : rien de plus a prouver
        } else if (mother.interior.size() == child.interior.size()) {
          if (!child.interior.empty()) {
            const i32 h = child.interior.front();     // h = min B(v)
            if (compare_power(pts, child.shell, mother.shell, h) >= 0) ++bad_potential;
          } else {
            // niveau zero : Q_r doit STRICTEMENT decroitre.
            std::vector<i32> base;
            independent_base(pts, seen_vertices[0].shell, &base);
            if (base.size() != 4 || compare_potential(pts, child.shell, mother.shell, base) <= 0)
              ++bad_potential;
          }
        } else {
          ++bad_potential;                            // le niveau a augmente
        }

        // (5) absence de cycle
        size_t cursor = i;
        int steps = 0;
        while (!parents[cursor].shell.empty() && steps <= (int)seen_vertices.size()) {
          const auto next = position.find(parents[cursor].shell);
          if (next == position.end()) break;
          cursor = next->second;
          ++steps;
        }
        if (steps > (int)seen_vertices.size()) ++cyclic;
      }
      for (size_t i = 0; i < seen_vertices.size(); ++i)
        if (parents[i].shell.empty() && seen_vertices[i].shell != seen_vertices[0].shell)
          ++missing_parent;
      coverage.parent_vertices += (long long)seen_vertices.size();
      coverage.parent_roots += roots;
      if (roots != 1 || missing_parent || unknown_parent || not_included || cyclic ||
          bad_closure || bad_transition || bad_potential) {
        printf("[%s] s_max=%2d GATE D : racines=%d sans_parent=%d parent_inconnu=%d"
               " inclusion=%d cycles=%d fermeture=%d transition=%d potentiel=%d (%zu sommets)\n",
               tag, s_max, roots, missing_parent, unknown_parent, not_included, cyclic,
               bad_closure, bad_transition, bad_potential, seen_vertices.size());
        ok = false;
      }
    }
  }

  // (H) LE NOYAU DEVICE-PORTABLE CONTRE LE CHEMIN CPU.
  //
  // Rien du chemin de decision de `order_k_flats.hpp` n'est executable sur un GPU :
  // `std::vector` partout, allocations par sommet, tailles non bornees. Le noyau de
  // `order_k_device_core.hpp` ecrit le MEME chemin sans allocation, a capacite
  // FIXE, et REFUSE ce qui la depasse — une coquille cospherique peut avoir
  // Theta(n) points, donc aucune capacite fixe n'est universellement suffisante, et
  // pretendre le contraire serait faux.
  //
  // La porte exige deux choses, et rien de plus : que tout sommet ADMIS soit decide
  // a l'identique — couple par couple, verdict par verdict — et que les refus
  // soient COMPTES, jamais silencieux.
  if (status == mhgp3v::CloudStatus::kOk && (int)pts.size() >= 4) {
    mhgp3v::FlatStatistics dst{};
    mhgp3v::CloudStatus dstatus = mhgp3v::CloudStatus::kOk;
    const auto seen = mhgp3v::navigate_shallow(pts, s_max - 2, &dst, &dstatus, false);
    if (dstatus == mhgp3v::CloudStatus::kOk && !seen.empty()) {
      std::vector<i32> root_base;
      for (const auto& v : seen) {
        if (v.level != 0) continue;
        for (i32 z : v.shell) {
          if (root_base.size() >= 4) break;
          const std::size_t have = root_base.size();
          if (have == 1) {
            const P3& a = pts[(size_t)root_base[0]];
            const P3& b = pts[(size_t)z];
            if (a.x == b.x && a.y == b.y && a.z == b.z) continue;
          } else if (have == 2) {
            const P3 u = mhgp::p3_sub(pts[(size_t)root_base[1]], pts[(size_t)root_base[0]]);
            const P3 w = mhgp::p3_sub(pts[(size_t)z], pts[(size_t)root_base[0]]);
            const P3 c = mhgp::p3_cross(u, w);
            if (c.x == 0 && c.y == 0 && c.z == 0) continue;
          } else if (have == 3) {
            if (mhgp3v::flats::orient3d_exact(pts[(size_t)root_base[0]], pts[(size_t)root_base[1]],
                                              pts[(size_t)root_base[2]], pts[(size_t)z]) == 0)
              continue;
          }
          root_base.push_back(z);
        }
        break;
      }
      int device_differs = 0, admitted = 0, refused = 0, capacity_refused = 0;
      long long pairs = 0;
      mhgp3v::device::AdmissionStats astats;
      if (root_base.size() == 4) {
        for (const auto& v : seen) {
          mhgp3v::device::BoundedVertex bv;
          if (mhgp3v::device::admit(v, &bv, &astats) != mhgp3v::device::Admission::kOk) {
            // REFUS = CONTRAT. Le sommet est rejoue sur le chemin non borne, et c'est
            // ce rejeu qui doit etre exige, non l'absence de depassement.
            ++refused;
            continue;
          }
          ++admitted;
          // Les flats livres doivent etre les MEMES, dans le MEME ordre.
          std::vector<std::vector<i32>> cpu_flats;
          // Le high-water de FERMETURE se releve ici, la ou les fermetures sont
          // reellement construites : le BFS n'appelle pas `for_each_flat`, donc le
          // lire dans ses statistiques donnait un compteur MORT — zero en permanence.
          mhgp3v::flats::for_each_flat(pts, v, [&](const mhgp3v::flats::FlatAtVertex& f) {
            cpu_flats.push_back(f.closure);
            return true;
          }, nullptr, nullptr, &dst.closure_high_water);
          size_t seen_flats = 0;
          bool order_ok = true;
          mhgp3v::device::for_each_flat_from(pts.data(), bv, 0, 1, 2,
                                             [&](const mhgp3v::device::BoundedFlat& g, int, int,
                                                 int) {
            if (seen_flats >= cpu_flats.size()) { order_ok = false; return false; }
            const std::vector<i32>& want = cpu_flats[seen_flats];
            if ((int)want.size() != g.closure_size) { order_ok = false; return false; }
            for (int i = 0; i < g.closure_size; ++i)
              if (want[(size_t)i] != g.closure[i]) order_ok = false;
            ++seen_flats;
            return order_ok;
          });
          if (!order_ok || seen_flats != cpu_flats.size()) { ++device_differs; continue; }
          // Puis les VERDICTS, couple par couple.
          mhgp3v::flats::for_each_flat(pts, v, [&](const mhgp3v::flats::FlatAtVertex& f) {
            mhgp3v::device::BoundedFlat g;
            g.base[0] = f.base[0]; g.base[1] = f.base[1]; g.base[2] = f.base[2];
            g.closure_size = (int)f.closure.size();
            for (int i = 0; i < g.closure_size; ++i) g.closure[i] = f.closure[(size_t)i];
            for (int direction = -1; direction <= 1; direction += 2) {
              ++pairs;
              const bool cpu_pair =
                  mhgp3v::flats::pair_admissible(pts, v, f.base, direction, root_base);
              const bool dev_pair = mhgp3v::device::pair_admissible(
                  pts.data(), bv, g.base, direction, root_base.data(), (int)root_base.size());
              if (cpu_pair != dev_pair) { ++device_differs; return false; }
              mhgp3v::flats::Vertex w;
              if (!mhgp3v::flats::neighbour_along(pts, v, f, direction, nullptr, &dst, &w))
                continue;
              if (w.level > s_max - 2) continue;
              mhgp3v::device::BoundedVertex bw;
              if (mhgp3v::device::admit(w, &bw, &astats) != mhgp3v::device::Admission::kOk) {
                ++capacity_refused;
                continue;
              }
              if (!mhgp3v::flats::backward_pair_admissible(pts, w, f.base, direction, root_base))
                continue;
              const mhgp3v::flats::ChildOutcome cpu_out =
                  mhgp3v::flats::decide_child(pts, w, f, direction, root_base);
              bool capacity_ok = true;
              const mhgp3v::flats::ChildOutcome dev_out = mhgp3v::device::decide_child(
                  pts.data(), bw, g, direction, root_base.data(), (int)root_base.size(),
                  &capacity_ok);
              if (!capacity_ok) { ++capacity_refused; continue; }
              if (cpu_out != dev_out) { ++device_differs; return false; }
            }
            return true;
          });
        }
      }
      if (root_base.size() != 4 || device_differs) {
        printf("[%s] s_max=%2d NOYAU DEVICE != CPU : base=%zu divergences=%d (admis=%d refuses=%d"
               " couples=%lld)\n", tag, s_max, root_base.size(), device_differs, admitted,
               refused, pairs);
        ok = false;
      }
      coverage.device_admitted += admitted;
      coverage.device_refused += refused + capacity_refused;
      coverage.device_pairs += pairs;
      // LES CAPACITES SONT MESUREES, PAS DEVINEES. J'avais fige kMaxShell = 32 et
      // kMaxInterior = 16 sans jamais relever les high-waters correspondants : la
      // porte ne disait donc rien sur la marge reelle.
      // Les high-waters de coquille et d'interieur se relevent A L'ADMISSION, la ou
      // le noyau borne recoit reellement le sommet.
      coverage.shell_high_water = std::max(coverage.shell_high_water, astats.shell_high_water);
      coverage.interior_high_water =
          std::max(coverage.interior_high_water, astats.interior_high_water);
      coverage.admission_samples += astats.samples;
      coverage.admission_overflow += astats.shell_overflow + astats.interior_overflow;
      coverage.closure_high_water = std::max(coverage.closure_high_water, dst.closure_high_water);
      coverage.touched_high_water = std::max(coverage.touched_high_water, dst.touched_high_water);
      coverage.batch_high_water = std::max(coverage.batch_high_water, dst.batch_high_water);
      // Les balayages complets se comptent sur le chemin INDEXE : c'est le seul ou
      // ils sont un defaut. Un balayage O(n) par thread serait inadmissible sur
      // device, et il faut donc savoir s'il survient.
      // NE PAS lire ce compteur ici : `dst` vient d'un parcours SANS index, ou le
      // balayage exhaustif n'existe pas comme repli mais comme chemin normal. Il est
      // accumule dans le bloc indexe, seul endroit ou il signifie quelque chose.
    }
  }

  // (G) « SUPPORT CANONIQUE PUIS PROPRIETAIRE » CONTRE LA TABLE `emitted`.
  //
  // `emitted` est la derniere table globale en Theta(sortie) du catalogue : tant
  // qu'elle est la, streamer les sommets ne sert a rien des qu'on veut le
  // catalogue. La composition prouvee par l'auditeur la remplace exactement — le
  // premier filtre supprime les doublons d'un meme sommet, le second ceux entre
  // sommets. La porte exige l'egalite des DEUX catalogues, sphere par sphere :
  // support, arite, rang et membres.
  if (status == mhgp3v::CloudStatus::kOk) {
    mhgp3v::FlatStatistics ost{};
    mhgp3v::CloudStatus ostatus = mhgp3v::CloudStatus::kOk;
    const mhgp::Catalogue owned =
        mhgp3v::flat_catalogue(pts, s_max, &ost, &ostatus, false, true, true);
    bool same = (ostatus == status && owned.spheres.size() == cat.spheres.size());
    if (same)
      for (size_t i = 0; i < owned.spheres.size(); ++i) {
        const mhgp::CriticalSphere& a = owned.spheres[i];
        const mhgp::CriticalSphere& b = cat.spheres[i];
        if (a.n_support != b.n_support || a.rank != b.rank) { same = false; break; }
        for (int j = 0; j < 4; ++j) if (a.support[j] != b.support[j]) same = false;
        if (!same) break;
        for (int j = 0; j < a.rank; ++j)
          if (owned.members[(size_t)(a.members_begin + j)] !=
              cat.members[(size_t)(b.members_begin + j)]) { same = false; break; }
        if (!same) break;
      }
    if (!same) {
      printf("[%s] s_max=%2d OWNER != EMITTED : %zu spheres contre %zu, statut %s contre %s"
             " (refus support=%lld refus sommet=%lld emises=%lld)\n", tag, s_max,
             owned.spheres.size(), cat.spheres.size(), mhgp3v::cloud_status_name(ostatus),
             mhgp3v::cloud_status_name(status), ost.owner_rejected_support,
             ost.owner_rejected_vertex, ost.owner_emitted);
      ok = false;
    }
    // LA TABLE DOIT ETRE VIDE. Dans le quadrant navigue et indexe, plus rien n'y
    // entre : ni les non-singletons, qui passent par le proprietaire, ni les
    // singletons, qui ne peuvent entrer en collision avec rien. C'est le HIGH-WATER
    // releve a chaque insertion qui est exige nul, et non la taille finale : une
    // mutation qui viderait la table a la fin tromperait cette seconde.
    if (ost.dedup_table_high_water != 0 && ostatus == mhgp3v::CloudStatus::kOk &&
        (int)pts.size() >= 4) {
      printf("[%s] s_max=%2d TABLE NON VIDE sous proprietaire : %lld entrees\n", tag, s_max,
             ost.dedup_table_high_water);
      ok = false;
    }
    coverage.dedup_table_max = std::max(coverage.dedup_table_max, ost.dedup_table_high_water);
    coverage.owner_rejected_support += ost.owner_rejected_support;
    coverage.owner_rejected_vertex += ost.owner_rejected_vertex;
    coverage.owner_emitted += ost.owner_emitted;
  }

  // (E bis) L'ENUMERATION EST-ELLE MONOTONE, ET LA SORTIE PRECOCE FIDELE ?
  //
  // Le parent s'arrete desormais au PREMIER couple admissible. Sa validite ne
  // depend que du potentiel — n'importe quel choix admissible est un parent
  // acyclique a racine unique —, mais l'en-tete affirme davantage : que ce premier
  // admissible est l'ancien MINIMUM, parce que `for_each_flat` livre les flats
  // dans l'ordre croissant de leur clef. Les deux affirmations sont jugees ici :
  // la monotonie clef par clef, et l'egalite du parent precoce avec le balayage
  // complet, sur CHAQUE sommet visite.
  if (status == mhgp3v::CloudStatus::kOk && (int)pts.size() >= 4) {
    mhgp3v::FlatStatistics est{};
    mhgp3v::CloudStatus estatus = mhgp3v::CloudStatus::kOk;
    const auto seen = mhgp3v::navigate_shallow(pts, s_max - 2, &est, &estatus, false);
    if (estatus == mhgp3v::CloudStatus::kOk && !seen.empty()) {
      // La base independante est reconstruite exactement comme le parcours la
      // construit : depuis la coquille du sommet de niveau zero.
      std::vector<i32> root_base;
      for (const auto& v : seen) {
        if (v.level != 0) continue;
        for (i32 z : v.shell) {
          if (root_base.size() >= 4) break;
          const std::size_t have = root_base.size();
          if (have == 1) {
            const P3& a = pts[(size_t)root_base[0]];
            const P3& b = pts[(size_t)z];
            if (a.x == b.x && a.y == b.y && a.z == b.z) continue;
          } else if (have == 2) {
            const P3 u = mhgp::p3_sub(pts[(size_t)root_base[1]], pts[(size_t)root_base[0]]);
            const P3 w = mhgp::p3_sub(pts[(size_t)z], pts[(size_t)root_base[0]]);
            const P3 c = mhgp::p3_cross(u, w);
            if (c.x == 0 && c.y == 0 && c.z == 0) continue;
          } else if (have == 3) {
            if (mhgp3v::flats::orient3d_exact(pts[(size_t)root_base[0]], pts[(size_t)root_base[1]],
                                       pts[(size_t)root_base[2]], pts[(size_t)z]) == 0) continue;
          }
          root_base.push_back(z);
        }
        break;
      }
      int not_monotone = 0, parent_differs = 0;
      long long early = 0, full = 0;
      // (E ter) LA DECISION DE FILIATION, REJOUEE CONTRE L'ANCIEN CHEMIN.
      //
      // `decide_child` n'appelle plus aucune requete de voisin de retour, et le
      // prefiltre en O(m) refuse 61 % des candidats. Sans rejeu direct, un mutant
      // « prefiltre toujours vrai » laisserait l'optimisation morte et la porte
      // verte, et un mutant « decision toujours accepter » ne serait vu
      // qu'indirectement. Chaque couple est donc juge deux fois : par la decision,
      // et par le chemin complet parent canonique + requete de retour + comparaison
      // de coquille. Les deux verdicts doivent coincider, et un `false` du
      // prefiltre doit impliquer un refus de l'ancien chemin.
      int decision_differs = 0, prefilter_wrong = 0, decision_broken = 0, signs_differ = 0;
      long long pairs_judged = 0, accepted = 0, prefiltered = 0;
      if (root_base.size() == 4) {
        for (const auto& v : seen) {
          mhgp3v::FlatStatistics qst{};
          mhgp3v::flats::for_each_flat(pts, v, [&](const mhgp3v::flats::FlatAtVertex& flat) {
            for (int direction = -1; direction <= 1; direction += 2) {
              mhgp3v::flats::Vertex w;
              if (!mhgp3v::flats::neighbour_along(pts, v, flat, direction, nullptr, &qst, &w))
                continue;
              if (w.level > s_max - 2) continue;
              ++pairs_judged;
              // Ancien chemin, integralement : parent canonique par BALAYAGE
              // COMPLET, requete de retour, comparaison de coquille.
              mhgp3v::flats::FlatAtVertex g;
              int orientation = 0;
              bool old_accept = false;
              if (mhgp3v::flats::canonical_parent(pts, w, root_base, &g, &orientation, nullptr,
                                                  nullptr, true)) {
                mhgp3v::flats::Vertex mother;
                if (mhgp3v::flats::neighbour_along(pts, w, g, orientation, nullptr, &qst, &mother))
                  old_accept = (mother.shell == v.shell);
              }
              const bool prefilter = mhgp3v::flats::backward_pair_admissible(
                  pts, w, flat.base, direction, root_base);
              // Les signes eux-memes, recalcules par le juge et non empruntes.
              if (prefilter != judge_admissible(pts, w, flat.base, -direction, root_base))
                ++signs_differ;
              if (!prefilter) {
                ++prefiltered;
                if (old_accept) ++prefilter_wrong;    // refus certifie et pourtant fils
                continue;
              }
              const mhgp3v::flats::ChildOutcome outcome =
                  mhgp3v::flats::decide_child(pts, w, flat, direction, root_base);
              if (outcome == mhgp3v::flats::ChildOutcome::kBroken) { ++decision_broken; continue; }
              const bool now_accept = (outcome == mhgp3v::flats::ChildOutcome::kAccept);
              if (now_accept) ++accepted;
              if (now_accept != old_accept) ++decision_differs;
            }
            return true;
          });
        }
        for (const auto& v : seen) {
          std::vector<i32> previous;
          bool first = true;
          mhgp3v::flats::for_each_flat(pts, v, [&](const mhgp3v::flats::FlatAtVertex& flat) {
            if (!first && !(previous < flat.closure)) ++not_monotone;
            previous = flat.closure;
            first = false;
            return true;
          });
          mhgp3v::flats::FlatAtVertex fa, fb;
          int da = 0, db = 0;
          long long ta = 0, ca = 0, tb = 0, cb = 0;
          const bool ea = mhgp3v::flats::canonical_parent(pts, v, root_base, &fa, &da, &ta, &ca,
                                                          false);
          const bool fb_ok = mhgp3v::flats::canonical_parent(pts, v, root_base, &fb, &db, &tb, &cb,
                                                             true);
          early += ca;
          full += cb;
          if (ea != fb_ok) ++parent_differs;
          else if (ea && (da != db || fa.closure != fb.closure)) ++parent_differs;
        }
      }
      if (root_base.size() != 4 || not_monotone || parent_differs || decision_differs ||
          prefilter_wrong || decision_broken || signs_differ) {
        printf("[%s] s_max=%2d PARENT PRECOCE : base=%zu monotonie violee=%d parents"
               " differents=%d | decisions differentes=%d prefiltre faux=%d decision cassee=%d"
               " signes differents=%d (%lld couples juges, %lld acceptes, %lld prefiltres)\n",
               tag, s_max, root_base.size(), not_monotone, parent_differs, decision_differs,
               prefilter_wrong, decision_broken, signs_differ, pairs_judged, accepted,
               prefiltered);
        ok = false;
      }
      coverage.pairs_judged += pairs_judged;
      coverage.pairs_accepted += accepted;
      coverage.pairs_prefiltered += prefiltered;
      coverage.parent_early_closures += early;
      coverage.parent_full_closures += full;
    }
  }

  // (F) REVERSE SEARCH contre le BFS. Le parcours sans `seen` doit rendre
  // EXACTEMENT le meme ensemble de sommets, avec les memes coquilles et les
  // memes ensembles interieurs. C'est la porte qui autorise a retirer les tables
  // globales, et rien d'autre ne la remplace : un parent legerement faux
  // produirait un sous-arbre tronque que seule cette comparaison verrait.
  if (status == mhgp3v::CloudStatus::kOk && (int)pts.size() >= 4) {
    mhgp3v::FlatStatistics bst{}, rst{};
    mhgp3v::CloudStatus bstatus = mhgp3v::CloudStatus::kOk, rstatus = mhgp3v::CloudStatus::kOk;
    const auto by_bfs = mhgp3v::navigate_shallow(pts, s_max - 2, &bst, &bstatus, false);
    const auto by_reverse = mhgp3v::reverse_search_shallow(pts, s_max - 2, &rst, &rstatus);
    // Le chemin INDEXE doit etre juge lui aussi : sans cela la porte ne qualifie
    // que les balayages exhaustifs du pinceau, pas celui qui servira au produit.
    {
      // FEUILLE DE QUATRE, et non seize : tous les nuages permanents ont au plus
      // treize points, donc un arbre a feuilles de seize n'a qu'une feuille et le
      // rejeu indexe ne qualifiait aucun elagage interne.
      mhgp3v::CertifiedIndex tree;
      tree.build(pts, 4);
      for (const auto& node : tree.nodes) if (node.left >= 0) ++coverage.index_internal_nodes;
      mhgp3v::FlatStatistics ist{};
      mhgp3v::CloudStatus istatus = mhgp3v::CloudStatus::kOk;
      const auto by_indexed =
          mhgp3v::reverse_search_shallow(pts, s_max - 2, &ist, &istatus, &tree);
      std::map<std::vector<i32>, std::vector<i32>> a, b;
      for (const auto& v : by_reverse) a[v.shell] = v.interior;
      for (const auto& v : by_indexed) b[v.shell] = v.interior;
      // La projection dans une `map` masquerait un doublon indexe : la taille est
      // donc comparee au nombre de RECORDS, des deux cotes.
      if (b.size() != by_indexed.size()) {
        printf("[%s] s_max=%2d REVERSE INDEXE : %zu sommets pour %zu coquilles distinctes"
               " — un doublon indexe etait masque par la projection\n", tag, s_max,
               by_indexed.size(), b.size());
        ok = false;
      }
      if (istatus != rstatus || a != b) {
        printf("[%s] s_max=%2d REVERSE INDEXE != REVERSE : statut %s contre %s, %zu contre %zu\n",
               tag, s_max, mhgp3v::cloud_status_name(istatus),
               mhgp3v::cloud_status_name(rstatus), b.size(), a.size());
        ok = false;
      }
      coverage.reverse_flats += ist.reverse_flats_enumerated;
      coverage.reverse_decisions += ist.reverse_decisions;
      // LES DEUX IDENTITES DE COMPTAGE, exigees par nuage et non observees en fin
      // de campagne : tout candidat est refuse en O(m) ou decide, et toute decision
      // refuse ou accepte — un accept par sommet non racine, puisque l'arbre en a
      // exactement un par sommet visite sauf la racine.
      if (ist.reverse_children_tested !=
              ist.reverse_reject_backward + ist.reverse_decisions ||
          ist.reverse_decisions !=
              ist.reverse_reject_by_parent + (long long)by_indexed.size() - 1) {
        printf("[%s] s_max=%2d IDENTITE DE COMPTAGE : candidats=%lld refus O(m)=%lld"
               " decisions=%lld refus decision=%lld sommets=%zu\n", tag, s_max,
               ist.reverse_children_tested, ist.reverse_reject_backward, ist.reverse_decisions,
               ist.reverse_reject_by_parent, by_indexed.size());
        ok = false;
      }
      coverage.reject_backward += ist.reverse_reject_backward;
      coverage.reject_by_parent += ist.reverse_reject_by_parent;
      coverage.reverse_triplets += ist.reverse_triplets_scanned;
      coverage.reverse_closures += ist.reverse_closures_built;
      coverage.exhaustive_scans += ist.exhaustive_scans;
      coverage.index_nodes_visited += tree.nodes_visited;
      coverage.index_leaves_visited += tree.leaves_visited;
      // LE SINK BORNE, juge par un consommateur qui ne tient RIEN : il compte et
      // replie un hachage independant de l'ordre. Si le compte et le repli egalent
      // ceux de la sortie materialisee, le parcours a bien rendu la meme chose
      // sans qu'aucun consommateur ne l'accumule. Le high-water des slots vifs est
      // publie par le parcours lui-meme.
      {
        mhgp3v::FlatStatistics sst{};
        mhgp3v::CloudStatus sstatus = mhgp3v::CloudStatus::kOk;
        long long count = 0;
        unsigned long long fold = 0;
        mhgp3v::reverse_search_stream(pts, s_max - 2, &sst, &sstatus,
                                      [&](const mhgp3v::flats::Vertex& v) {
          ++count;
          unsigned long long h = 1469598103934665603ULL;
          for (i32 z : v.shell) h = (h ^ (unsigned long long)(z + 1)) * 1099511628211ULL;
          h = (h ^ 0x9e37ULL) * 1099511628211ULL;
          for (i32 z : v.interior) h = (h ^ (unsigned long long)(z + 1)) * 1099511628211ULL;
          fold += h;                            // somme : independante de l'ordre
          return true;
        });
        long long want_count = 0;
        unsigned long long want_fold = 0;
        for (const auto& v : by_reverse) {
          ++want_count;
          unsigned long long h = 1469598103934665603ULL;
          for (i32 z : v.shell) h = (h ^ (unsigned long long)(z + 1)) * 1099511628211ULL;
          h = (h ^ 0x9e37ULL) * 1099511628211ULL;
          for (i32 z : v.interior) h = (h ^ (unsigned long long)(z + 1)) * 1099511628211ULL;
          want_fold += h;
        }
        if (sstatus != rstatus || count != want_count || fold != want_fold) {
          printf("[%s] s_max=%2d SINK != MATERIALISE : statut %s contre %s, %lld contre %lld\n",
                 tag, s_max, mhgp3v::cloud_status_name(sstatus),
                 mhgp3v::cloud_status_name(rstatus), count, want_count);
          ok = false;
        }
        coverage.sink_vertices += count;
        coverage.sink_high_water = std::max(coverage.sink_high_water, sst.reverse_live_high_water);
        // L'INTERRUPTION est une porte, pas une politesse : un sink qui s'arrete
        // doit stopper le parcours et rendre un statut non `kOk`, sans quoi une
        // sortie tronquee passerait pour complete.
        if (want_count > 1) {
          mhgp3v::FlatStatistics ist2{};
          mhgp3v::CloudStatus istatus2 = mhgp3v::CloudStatus::kOk;
          long long seen = 0;
          mhgp3v::reverse_search_stream(pts, s_max - 2, &ist2, &istatus2,
                                        [&](const mhgp3v::flats::Vertex&) {
            return ++seen < 1;
          });
          // ARRET APRES UN PREFIXE, et pas seulement au germe : la branche issue
          // d'un enfant doit elle aussi remonter `kSinkStopped`.
          if (want_count > 3) {
            mhgp3v::FlatStatistics ist3{};
            mhgp3v::CloudStatus istatus3 = mhgp3v::CloudStatus::kOk;
            long long prefix = 0;
            mhgp3v::reverse_search_stream(pts, s_max - 2, &ist3, &istatus3,
                                          [&](const mhgp3v::flats::Vertex&) {
              return ++prefix < 3;
            });
            if (prefix != 3 || istatus3 != mhgp3v::CloudStatus::kSinkStopped) {
              printf("[%s] s_max=%2d SINK PREFIXE : %lld sommets, statut %s\n", tag, s_max,
                     prefix, mhgp3v::cloud_status_name(istatus3));
              ok = false;
            }
            ++coverage.sink_interruptions;
          }
          if (seen != 1 || istatus2 != mhgp3v::CloudStatus::kSinkStopped) {
            printf("[%s] s_max=%2d SINK INTERROMPU : %lld sommets, statut %s\n", tag, s_max,
                   seen, mhgp3v::cloud_status_name(istatus2));
            ok = false;
          }
          ++coverage.sink_interruptions;
        }
      }
      // Trois points DISTINCTS d'une meme sphere ne sont jamais alignes : une
      // droite coupe une sphere en au plus deux points. Le garde de colinearite de
      // `for_each_flat` est donc inactif sur une coquille, et les deux compteurs
      // doivent etre EGAUX. Une divergence signifierait une coquille non
      // cospherique ou un point double, c'est-a-dire un invariant rompu en amont.
      if (ist.reverse_triplets_scanned != ist.reverse_closures_built) {
        printf("[%s] s_max=%2d COQUILLE : %lld triplets pour %lld fermetures — trois points"
               " alignes sur une sphere\n", tag, s_max, ist.reverse_triplets_scanned,
               ist.reverse_closures_built);
        ok = false;
      }
      if (istatus == mhgp3v::CloudStatus::kOk)
        coverage.reverse_indexed_vertices += (long long)by_indexed.size();
    }
    if (bstatus != rstatus) {
      printf("[%s] s_max=%2d REVERSE : statut %s contre %s\n", tag, s_max,
             mhgp3v::cloud_status_name(rstatus), mhgp3v::cloud_status_name(bstatus));
      ok = false;
    } else if (bstatus != mhgp3v::CloudStatus::kOk) {
      // Deux statuts egaux mais non `kOk` ne doivent pas faire sauter la porte en
      // silence : on compte ce cas pour qu'un plancher puisse l'interdire.
      ++coverage.reverse_skipped;
    } else {
      std::map<std::vector<i32>, std::vector<i32>> from_bfs, from_reverse;
      for (const auto& v : by_bfs) from_bfs[v.shell] = v.interior;
      for (const auto& v : by_reverse) from_reverse[v.shell] = v.interior;
      if (by_reverse.size() != from_reverse.size()) {
        printf("[%s] s_max=%2d REVERSE : %zu sommets pour %zu coquilles distinctes"
               " — un sommet a ete visite deux fois\n", tag, s_max, by_reverse.size(),
               from_reverse.size());
        ok = false;
      }
      if (from_bfs != from_reverse) {
        printf("[%s] s_max=%2d REVERSE != BFS : %zu contre %zu sommets\n", tag, s_max,
               from_reverse.size(), from_bfs.size());
        ok = false;
      }
      coverage.reverse_vertices += (long long)by_reverse.size();
      coverage.reverse_depth = std::max(coverage.reverse_depth, rst.reverse_depth_max);
      coverage.reverse_children += rst.reverse_children_tested;
    }
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

  // PAYLOAD, et pas seulement l'ensemble (support, rang) : doublons, tranche de
  // membres, contiguite de `members_begin`, geometrie exacte de la sphere, et
  // ordre de serialisation. Sans cela une mutation de ces champs laisserait la
  // porte verte, et `ForestNode::source` est un indice dans cet ordre.
  int payload_faults = 0;
  {
    std::set<std::vector<i32>> seen_support;
    std::size_t cursor = 0;
    std::vector<mhgp::CriticalSphere> previous;
    for (const mhgp::CriticalSphere& sph : cat.spheres) {
      std::vector<i32> sup(sph.support, sph.support + sph.n_support);
      if (!seen_support.insert(sup).second) ++payload_faults;       // doublon publie
      for (int i = sph.n_support; i < mhgp::kMaxSupport; ++i)
        if (sph.support[i] != -1) ++payload_faults;                 // queue non remplie de -1
      if ((std::size_t)sph.members_begin != cursor) ++payload_faults;   // tranche non contigue
      cursor += (std::size_t)sph.rank;
      std::vector<i32> members;
      for (int i = 0; i < sph.rank; ++i)
        members.push_back(cat.members[(size_t)(sph.members_begin + i)]);
      if (!std::is_sorted(members.begin(), members.end())) ++payload_faults;
      // les membres sont exactement la boule fermee de la sphere publiee
      std::vector<i32> expected;
      for (i32 z = 0; z < (i32)pts.size(); ++z)
        if (mhgp::sphere_side(sph.sph, pts[(size_t)z]) <= 0) expected.push_back(z);
      if (members != expected) ++payload_faults;
      // la sphere publiee passe par tout son support et est la miniboule de sa coquille
      for (int i = 0; i < sph.n_support; ++i)
        if (mhgp::sphere_side(sph.sph, pts[(size_t)sph.support[i]]) != 0) ++payload_faults;
      if (!previous.empty()) {
        const mhgp::CriticalSphere& q = previous.back();
        bool ordered = false, equal = true;
        for (int i = 0; i < mhgp::kMaxSupport && equal; ++i) {
          if (q.support[i] < sph.support[i]) { ordered = true; equal = false; }
          else if (q.support[i] > sph.support[i]) { equal = false; }
        }
        if (!ordered) ++payload_faults;             // ordre lexicographique strict
      }
      previous.push_back(sph);
    }
    if (cursor != cat.members.size()) ++payload_faults;   // membres orphelins
  }
  if (payload_faults) ok = false;

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
    if (payload_faults) printf("        FAUTES DE PAYLOAD = %d\n", payload_faults);
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
// EGALITE COMPLETE de deux catalogues : statut a part, puis support, arite, rang,
// beta ET membres. Comparer un couple (nombre, histogramme d'arites) laisse passer
// deux catalogues differents de meme profil — et mon message annonçait « meme
// catalogue » pour ce seul profil.
static bool same_catalogue(const mhgp::Catalogue& a, const mhgp::Catalogue& b) {
  if (a.spheres.size() != b.spheres.size()) return false;
  for (size_t i = 0; i < a.spheres.size(); ++i) {
    const mhgp::CriticalSphere& x = a.spheres[i];
    const mhgp::CriticalSphere& y = b.spheres[i];
    if (x.n_support != y.n_support || x.rank != y.rank) return false;
    for (int j = 0; j < 4; ++j) if (x.support[j] != y.support[j]) return false;
    if (mhgp::sphere_cmp_beta(x.sph, y.sph) != 0) return false;
    for (int j = 0; j < x.rank; ++j)
      if (a.members[(size_t)(x.members_begin + j)] != b.members[(size_t)(y.members_begin + j)])
        return false;
  }
  return true;
}

static bool permutation_equivariant(const char* tag, const std::vector<P3>& pts, int s_max,
                                    int trials, std::mt19937& rng) {
  auto signature = [&](const std::vector<P3>& q, const std::vector<int>& back) {
    mhgp3v::FlatStatistics st{};
    mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
    const mhgp::Catalogue cat = mhgp3v::flat_catalogue(q, s_max, &st, &status, true);
    std::set<std::pair<std::vector<i32>, int>> out;
    // Le STATUT fait partie de la signature : un germe qui reussit sur une
    // numerotation et echoue sur une autre est une non-equivariance, meme si les
    // deux catalogues publies sont vides.
    out.insert({std::vector<i32>{(i32)(-1000 - (int)status)}, 0});
    if (st.census_mismatch_shell || st.census_mismatch_level)
      out.insert({std::vector<i32>{-2000}, 0});
    for (const mhgp::CriticalSphere& s : cat.spheres) {
      std::vector<i32> sup;
      for (int i = 0; i < s.n_support; ++i) sup.push_back((i32)back[(size_t)s.support[i]]);
      std::sort(sup.begin(), sup.end());
      out.insert({sup, s.rank});
    }
    // L'EQUIVARIANCE DU PROPRIETAIRE. Le choix de o(U) doit etre geometrique : une
    // renumerotation ne peut pas changer quel sommet possede un support.
    {
      mhgp3v::FlatStatistics owner_st{};
      mhgp3v::CloudStatus owner_status = mhgp3v::CloudStatus::kOk;
      const mhgp::Catalogue owned =
          mhgp3v::flat_catalogue(q, s_max, &owner_st, &owner_status, false, true, true);
      out.insert({std::vector<i32>{(i32)(-5000 - (int)owner_status)}, 0});
      // La signature transporte les MEMBRES et un indice d'occurrence : un `set` de
      // (support, rang) masquerait les multiplicites et ne verrait pas un membre
      // perdu.
      std::map<std::vector<i32>, int> seen_support;
      for (const mhgp::CriticalSphere& sphere : owned.spheres) {
        std::vector<i32> key{-6000};
        for (int i = 0; i < sphere.n_support; ++i)
          key.push_back((i32)back[(size_t)sphere.support[i]]);
        std::sort(key.begin() + 1, key.end());
        std::vector<i32> members;
        for (int i = 0; i < sphere.rank; ++i)
          members.push_back((i32)back[(size_t)owned.members[(size_t)(sphere.members_begin + i)]]);
        std::sort(members.begin(), members.end());
        const int occurrence = seen_support[key]++;
        key.push_back(-7000);
        key.push_back((i32)occurrence);
        key.insert(key.end(), members.begin(), members.end());
        out.insert({key, sphere.rank});
      }
    }

    // L'EQUIVARIANCE DE LA REVERSE SEARCH, et pas seulement celle du catalogue.
    // Ce qui est geometrique est l'ENSEMBLE visite : le germe, l'arbre et l'ordre
    // des fils dependent de la numerotation, puisque la direction canonique du
    // parent compare des clefs d'indices. Ce que la signature exige est donc que
    // le meme ensemble de sommets soit atteint depuis une AUTRE racine.
    if ((int)q.size() >= 4) {
      mhgp3v::FlatStatistics rst{};
      mhgp3v::CloudStatus rstatus = mhgp3v::CloudStatus::kOk;
      const auto by_reverse = mhgp3v::reverse_search_shallow(q, s_max - 2, &rst, &rstatus);
      out.insert({std::vector<i32>{(i32)(-3000 - (int)rstatus)}, 0});
      for (const auto& v : by_reverse) {
        std::vector<i32> shell;
        for (i32 z : v.shell) shell.push_back((i32)back[(size_t)z]);
        std::sort(shell.begin(), shell.end());
        shell.insert(shell.begin(), -4000);
        out.insert({shell, v.level});
      }
    }
    return out;
  };
  std::vector<int> identity((size_t)pts.size());
  for (size_t i = 0; i < identity.size(); ++i) identity[i] = (int)i;
  const auto reference = signature(pts, identity);
  ++coverage.equivariance_runs;
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
  int min_navigated = 0, min_vertices = 0, min_multiple_shells = 0, min_quotiented = 0;
  int min_reverse = 0, min_reverse_depth = 0, min_internal_nodes = 0, min_owner = 0;
  long long seed = 4242;
  // `atoi` acceptait « 0junk » et rendait zero : une campagne vide passait pour
  // verte. Lecture INTEGRALE stricte, sinon code 2 avant tout calcul.
  auto integer = [&](const char* text, long long* out) {
    const char* first = text;
    const char* last = text + strlen(text);
    if (first == last) return false;
    const bool negative = (*first == '-');
    if (negative) ++first;
    if (first == last) return false;
    unsigned long long magnitude = 0;
    const auto result = std::from_chars(first, last, magnitude);
    if (result.ec != std::errc{} || result.ptr != last) return false;
    if (magnitude > 4611686018427387903ULL) return false;
    *out = negative ? -(long long)magnitude : (long long)magnitude;
    return true;
  };
  for (int i = 1; i < argc; ++i) {
    const bool has_value = (i + 1 < argc);
    long long value = 0;
    // Le token doit etre lu en ENTIER, puis tenir dans la plage semantique de
    // l'option AVANT le cast. Sans ce controle, `--clouds 4294967296` rendait
    // zero et `--coord 4295032832` rendait 65536 : la campagne devenait vide ou
    // hors grille, et le binaire annoncait OK.
    auto take = [&](int* target, long long low, long long high) {
      if (!has_value || !integer(argv[i + 1], &value)) return false;
      if (value < low || value > high) return false;
      ++i;
      *target = (int)value;
      return true;
    };
    bool ok_argument = false;
    if (!strcmp(argv[i], "--clouds")) ok_argument = take(&clouds, 0, 1000000);
    else if (!strcmp(argv[i], "--points")) ok_argument = take(&npoints, 1, 4096);
    else if (!strcmp(argv[i], "--coord")) ok_argument = take(&coord, 2, 65536);
    else if (!strcmp(argv[i], "--smax")) ok_argument = take(&smax_hi, 2, 4096);
    else if (!strcmp(argv[i], "--min-cases")) ok_argument = take(&min_cases, 1, 1000000000);
    else if (!strcmp(argv[i], "--min-navigated")) ok_argument = take(&min_navigated, 0, 1000000000);
    else if (!strcmp(argv[i], "--min-vertices")) ok_argument = take(&min_vertices, 0, 2000000000);
    else if (!strcmp(argv[i], "--min-multiple-shells")) ok_argument = take(&min_multiple_shells, 0, 1000000000);
    else if (!strcmp(argv[i], "--min-quotiented")) ok_argument = take(&min_quotiented, 0, 1000000000);
    else if (!strcmp(argv[i], "--min-reverse")) ok_argument = take(&min_reverse, 0, 2000000000);
    else if (!strcmp(argv[i], "--min-reverse-depth")) ok_argument = take(&min_reverse_depth, 0, 100000);
    else if (!strcmp(argv[i], "--min-internal-nodes")) ok_argument = take(&min_internal_nodes, 0, 2000000000);
    else if (!strcmp(argv[i], "--min-owner")) ok_argument = take(&min_owner, 0, 2000000000);
    else if (!strcmp(argv[i], "--seed")) {
      if (has_value && integer(argv[i + 1], &value) && value >= 0 && value <= 4294967295LL) {
        ++i; seed = value; ok_argument = true;
      }
    } else {
      printf("ECHEC : argument inconnu %s\n", argv[i]);
      return 2;
    }
    if (!ok_argument) {
      printf("ECHEC : valeur entiere invalide ou manquante pour %s\n", argv[i]);
      return 2;
    }
  }
  // La GRILLE DECLAREE est u16 : les bornes des predicats entiers en dependent,
  // et un `--coord` hors grille produit un depassement signe dans `in_sphere_side`.
  // Une demande impossible ne doit pas etre censuree en silence : neuf points
  // distincts dans une boite de cote deux n'existent pas, et le generateur
  // rendait alors zero nuage tandis que le plancher global etait satisfait par
  // les seules fixtures. Le produit est calcule en `long long`, sans debordement.
  {
    const long long capacity = (long long)coord * (long long)coord * (long long)coord;
    if ((long long)npoints > capacity) {
      printf("ECHEC : campagne impossible, %d points distincts demandes dans %lld positions\n",
             npoints, capacity);
      return 2;
    }
  }

  int failures = 0, cases = 0, generated_generic = 0, generated_cospherical = 0;
  std::mt19937 rng((unsigned)seed);

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
  // Refutation de la « descente stricte du rayon » : P est strictement interieur
  // au cercle de ABC et pourtant les quatre rayons carres valent 5/2.
  // Sphere u16 a centre tres eloigne : la marge flottante de l'index elaguait
  // la RACINE et rendait zero point au lieu des quatre supports.
  // Témoins de Gate D, coordonnées exactes de la note.
  // La coquille du germe est {0,2,3,4,5} et ses quatre premiers membres sont
  // COPLANAIRES dans x+z=1 : prendre les quatre premiers sans vérifier leur
  // indépendance donnait deux racines.
  fixtures.push_back({"germe_base_non_independante",
                      {pt(0, 0, 1), pt(0, 1, 0), pt(0, 1, 1), pt(1, 0, 0), pt(1, 1, 0),
                       pt(2, 0, 0)}});
  // Choisir seulement le voisin admissible de coquille minimale crée un cycle
  // de longueur deux ; c'est le signe STRICT de L_h qui le coupe.
  fixtures.push_back({"lex_admissible_cycle",
                      {pt(14, 6, 1), pt(7, 10, 8), pt(3, 5, 11), pt(3, 8, 5), pt(7, 7, 3),
                       pt(14, 3, 14)}});
  fixtures.push_back({"lp_optimum_tie",
                      {pt(1, 1, 7), pt(7, 9, 4), pt(1, 2, 6), pt(9, 2, 10), pt(0, 3, 5),
                       pt(0, 2, 6)}});
  // Un choix lexicographique sans Q_r crée un cycle de longueur deux au niveau
  // zéro.
  fixtures.push_back({"level_zero_lex_cycle",
                      {pt(0, 4, 0), pt(1, 4, 15), pt(10, 7, 2), pt(11, 15, 0), pt(6, 14, 14),
                       pt(9, 6, 5), pt(14, 11, 0)}});
  fixtures.push_back({"centre_lointain_elagage_refute",
                      {pt(32767, 32767, 0), pt(57863, 57862, 0), pt(7672, 7673, 0),
                       pt(60104, 30135, 1)}});
  fixtures.push_back({"descente_rayon_refutee",
                      {pt(0, 0, 0), pt(0, 3, 0), pt(2, 1, 0), pt(1, 1, 0), pt(1, 1, 2)}});
  // Deux observations confondues : le sujet doit REFUSER, pas publier `ok`.
  fixtures.push_back({"coordonnees_dupliquees",
                      {pt(0, 0, 0), pt(0, 0, 0), pt(2, 0, 0), pt(0, 2, 0), pt(0, 0, 2)}});
  fixtures.push_back({"germe_arete_traversee",
                      {pt(0, 0, 0), pt(2, 0, 0), pt(4, 0, 0), pt(2, 4, 0), pt(2, 2, 6), pt(1, 3, 2)}});

  // Les 120 permutations de la fixture qui a refute la descente : c'est la seule
  // maniere de voir une non-equivariance qui ne touche que 30 numerotations.
  {
    const std::vector<P3> witness{pt(0, 0, 0), pt(0, 3, 0), pt(2, 1, 0), pt(1, 1, 0),
                                  pt(1, 1, 2)};
    std::vector<int> perm{0, 1, 2, 3, 4};
    std::sort(perm.begin(), perm.end());
    std::set<std::set<std::pair<std::vector<i32>, int>>> signatures;
    int refused = 0;
    do {
      std::vector<P3> q(5);
      std::vector<int> back(5);
      for (int i = 0; i < 5; ++i) { q[(size_t)i] = witness[(size_t)perm[(size_t)i]]; back[(size_t)i] = perm[(size_t)i]; }
      mhgp3v::FlatStatistics st{};
      mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
      const mhgp::Catalogue cat = mhgp3v::flat_catalogue(q, 5, &st, &status, true);
      if (status != mhgp3v::CloudStatus::kOk) ++refused;
      std::set<std::pair<std::vector<i32>, int>> sig;
      for (const mhgp::CriticalSphere& sp : cat.spheres) {
        std::vector<i32> sup;
        for (int i = 0; i < sp.n_support; ++i) sup.push_back((i32)back[(size_t)sp.support[i]]);
        std::sort(sup.begin(), sup.end());
        sig.insert({sup, sp.rank});
      }
      signatures.insert(sig);
    } while (std::next_permutation(perm.begin(), perm.end()));
    ++cases;
    if (refused != 0 || signatures.size() != 1) {
      printf("[descente_rayon_refutee] 120 PERMUTATIONS : %d refus, %zu signatures distinctes"
             " (attendu 0 et 1)\n", refused, signatures.size());
      ++failures;
    } else {
      printf("[descente_rayon_refutee] 120 permutations : 0 refus, signature unique\n");
    }
  }

  // PORTE DE L'INDEX, testee sur le predicat lui-meme et pas seulement a
  // travers le catalogue. Trois exigences de la note d'audit : la sphere a
  // centre tres eloigne dont chaque support doit atteindre sa feuille, des
  // noeuds internes reellement exerces, et l'accord du predicat exact avec une
  // enumeration de toutes les coordonnees d'une petite boite.
  {
    ++cases;
    int index_faults = 0;
    {   // (a) grande sphere : les quatre supports doivent etre rendus
      const std::vector<P3> far{pt(32767, 32767, 0), pt(57863, 57862, 0), pt(7672, 7673, 0),
                                pt(60104, 30135, 1)};
      mhgp::Sphere sphere{};
      if (!mhgp::sphere4(far[0], far[1], far[2], far[3], &sphere)) ++index_faults;
      else {
        mhgp3v::CertifiedIndex tree;
        tree.build(far, 16);
        long long touched = 0;
        int returned = 0;
        tree.closed_ball(sphere, &touched, [&](i32) { ++returned; });
        if (returned != 4) {
          printf("[index] grande sphere : %d points rendus au lieu de 4\n", returned);
          ++index_faults;
        }
      }
    }
    {   // (b) noeuds internes reellement exerces, et accord avec l'exhaustif
      std::vector<P3> cube;
      for (int x = 0; x < 4; ++x) for (int y = 0; y < 4; ++y) for (int z = 0; z < 3; ++z)
        cube.push_back(pt(x * 7, y * 5, z * 11));
      mhgp3v::CertifiedIndex tree;
      tree.build(cube, 4);
      if (tree.nodes.size() < 3) { printf("[index] arbre sans noeud interne\n"); ++index_faults; }
      std::mt19937 local(12345);
      std::uniform_int_distribution<int> pick(0, (int)cube.size() - 1);
      for (int trial = 0; trial < 200; ++trial) {
        const int a = pick(local), b = pick(local), c = pick(local), d = pick(local);
        mhgp::Sphere sphere{};
        if (!mhgp::sphere4(cube[(size_t)a], cube[(size_t)b], cube[(size_t)c], cube[(size_t)d],
                           &sphere)) continue;
        std::set<i32> by_index, by_scan;
        long long touched = 0;
        tree.closed_ball(sphere, &touched, [&](i32 id) { by_index.insert(id); });
        for (i32 z = 0; z < (i32)cube.size(); ++z)
          if (mhgp::sphere_side(sphere, cube[(size_t)z]) <= 0) by_scan.insert(z);
        if (by_index != by_scan) {
          printf("[index] boule fermee : %zu contre %zu par balayage\n", by_index.size(),
                 by_scan.size());
          ++index_faults;
          break;
        }
      }
    }
    {   // (c) `box` ET `sign_disagreement`, les deux requetes du pinceau, jugees
        // elles aussi. L'auto-test ne portait que sur `closed_ball` : les deux
        // primitives qui elaguent reellement le balayage du pinceau n'etaient
        // qualifiees par rien.
      std::vector<P3> cube;
      for (int x = 0; x < 5; ++x) for (int y = 0; y < 4; ++y) for (int z = 0; z < 3; ++z)
        cube.push_back(pt(x * 6, y * 9, z * 4));
      mhgp3v::CertifiedIndex tree;
      tree.build(cube, 4);
      std::mt19937 local(777);
      std::uniform_int_distribution<int> coordinate(-4, 32);
      std::uniform_int_distribution<int> pick(0, (int)cube.size() - 1);
      for (int trial = 0; trial < 200 && index_faults == 0; ++trial) {
        long long lo[3], hi[3];
        for (int d = 0; d < 3; ++d) {
          const int u = coordinate(local), v = coordinate(local);
          lo[d] = std::min(u, v);
          hi[d] = std::max(u, v);
        }
        std::set<i32> by_index, by_scan;
        long long touched = 0;
        tree.box(lo, hi, &touched, [&](i32 id) { by_index.insert(id); });
        for (i32 z = 0; z < (i32)cube.size(); ++z) {
          const P3& p = cube[(size_t)z];
          const long long c[3] = {(long long)p.x, (long long)p.y, (long long)p.z};
          bool in = true;
          for (int d = 0; d < 3; ++d) if (c[d] < lo[d] || c[d] > hi[d]) in = false;
          if (in) by_scan.insert(z);
        }
        if (by_index != by_scan) {
          printf("[index] boite : %zu contre %zu par balayage\n", by_index.size(),
                 by_scan.size());
          ++index_faults;
        }
      }
      for (int trial = 0; trial < 300 && index_faults == 0; ++trial) {
        mhgp::Sphere a{}, b{};
        const int i0 = pick(local), i1 = pick(local), i2 = pick(local), i3 = pick(local);
        const int j0 = pick(local), j1 = pick(local), j2 = pick(local), j3 = pick(local);
        if (!mhgp::sphere4(cube[(size_t)i0], cube[(size_t)i1], cube[(size_t)i2],
                           cube[(size_t)i3], &a)) continue;
        if (!mhgp::sphere4(cube[(size_t)j0], cube[(size_t)j1], cube[(size_t)j2],
                           cube[(size_t)j3], &b)) continue;
        std::set<i32> by_index, by_scan;
        long long touched = 0;
        tree.sign_disagreement(a, b, &touched, [&](i32 id) { by_index.insert(id); });
        for (i32 z = 0; z < (i32)cube.size(); ++z) {
          const int sa = mhgp::sphere_side(a, cube[(size_t)z]);
          const int sb = mhgp::sphere_side(b, cube[(size_t)z]);
          const int na = sa < 0 ? -1 : (sa > 0 ? 1 : 0);
          const int nb = sb < 0 ? -1 : (sb > 0 ? 1 : 0);
          if (na != nb) by_scan.insert(z);
        }
        // La requete est SURE, pas exacte : elle peut rendre des points d'accord,
        // jamais en omettre un de desaccord. C'est cette inclusion qui est jugee.
        for (i32 z : by_scan)
          if (by_index.find(z) == by_index.end()) {
            printf("[index] desaccord ternaire : le point %d manque\n", z);
            ++index_faults;
          }
      }
      // ELAGAGE REELLEMENT EXERCE. Un arbre a feuille unique donnerait les memes
      // resultats en visitant TOUT : la porte exige donc qu'au moins une requete
      // visite strictement moins de noeuds que l'arbre n'en contient.
      {
        long long best = (long long)tree.nodes.size();
        bool pruned = false;
        for (int trial = 0; trial < 100 && !pruned; ++trial) {
          long long lo[3], hi[3];
          for (int d = 0; d < 3; ++d) { lo[d] = trial; hi[d] = trial + 2; }
          tree.nodes_visited = 0;
          long long touched = 0;
          tree.box(lo, hi, &touched, [](i32) {});
          if (tree.nodes_visited < best) { best = tree.nodes_visited; pruned = true; }
        }
        if (!pruned || (int)tree.nodes.size() < 3) {
          printf("[index] aucun elagage exerce : %lld noeuds visites pour %zu noeuds\n", best,
                 tree.nodes.size());
          ++index_faults;
        }
      }
    }
    if (index_faults) failures += index_faults;
    else
      printf("[index] grande sphere, noeuds internes, boite, desaccord ternaire et accord"
             " exhaustif verifies\n");
  }

  // LES DEUX FIXTURES QUE LA NOTE EXIGE POUR LE PROPRIETAIRE.
  //
  // `owner_signed_cone` : support U = {(0,0,2),(4,0,2),(1,3,2)} et deux points
  // additionnels (2,1,1),(2,1,3). B_U contient ces deux points, P_U est le segment
  // des centres (2,1,z) pour 0 <= z <= 4, et G_U y est CONSTANT. Le tie-break
  // lexicographique choisit z = 0. Le cone de chambre NON SIGNE rejetterait a tort
  // cette orientation ; le cone SIGNE l'accepte. C'est la seule fixture qui separe
  // les deux cones, donc la seule qui protege le choix de eps_s = -1 sur B_U.
  //
  // `owner_multiple_supports` : le cube u16, dont les quatre paires antipodales
  // portent la meme boule et le meme proprietaire. Sans le rejet U != U_can,
  // l'owner seul emettrait quatre fois.
  {
    ++cases;
    int cone_faults = 0;
    const std::vector<P3> cone{pt(0, 0, 2), pt(4, 0, 2), pt(1, 3, 2), pt(2, 1, 1), pt(2, 1, 3)};
    {
      mhgp3v::FlatStatistics a{}, b{};
      mhgp3v::CloudStatus sa = mhgp3v::CloudStatus::kOk, sb = mhgp3v::CloudStatus::kOk;
      const mhgp::Catalogue plain = mhgp3v::flat_catalogue(cone, 5, &a, &sa, false, true, false);
      const mhgp::Catalogue owned = mhgp3v::flat_catalogue(cone, 5, &b, &sb, false, true, true);
      if (sa != sb || !same_catalogue(plain, owned)) ++cone_faults;
      if (a.dedup_table_high_water == 0 || b.dedup_table_high_water != 0) ++cone_faults;
    }
    {
      // Le cube : quatre supports de CARDINALITE minimale pour une seule boule, et
      // six minimaux pour l'INCLUSION. Le catalogue owner doit etre identique.
      std::vector<P3> cube;
      for (int x = 0; x < 2; ++x) for (int y = 0; y < 2; ++y) for (int z = 0; z < 2; ++z)
        cube.push_back(pt(2 * x, 2 * y, 2 * z));
      mhgp3v::FlatStatistics a{}, b{};
      mhgp3v::CloudStatus sa = mhgp3v::CloudStatus::kOk, sb = mhgp3v::CloudStatus::kOk;
      const mhgp::Catalogue plain = mhgp3v::flat_catalogue(cube, 8, &a, &sa, false, true, false);
      const mhgp::Catalogue owned = mhgp3v::flat_catalogue(cube, 8, &b, &sb, false, true, true);
      if (sa != sb || !same_catalogue(plain, owned)) ++cone_faults;
      if (b.dedup_table_high_water != 0) ++cone_faults;
    }
    if (cone_faults) {
      printf("[owner cone signe] %d faute(s)\n", cone_faults);
      failures += cone_faults;
    } else {
      printf("[owner cone signe] segment des centres et cube u16 : catalogues identiques,"
             " table vide sous proprietaire\n");
    }
  }

  // LA FRONTIERE u16 DU SIGNE OWNER, GRAVEE.
  //
  // `owner_rays_ok` passait la valeur BRUTE de `orient3d_exact`, un `i128`, a
  // `tangent_sign(int, int)`. La conversion implicite ne conservait que les
  // trente-deux bits bas. Ce n'etait pas une perte de precision : c'etait une
  // perte de SIGNE, donc un cone tangent faux, donc un proprietaire faux, donc
  // des spheres supprimees du catalogue.
  //
  // Les deux echelles ne sont pas choisies : elles sont les deux frontieres
  // arithmetiques du predicat sur la grille u16.
  //
  //   tetraedre axial d'arete s        det = s^3       franchit 2^31 entre 1290 et 1291
  //   tetraedre alterne d'arete s      det = -2 s^3    vaut EXACTEMENT INT_MIN a s = 1024
  //
  // A s = 1024 le mal n'etait meme pas un signe faux : `tangent_sign` niait
  // `INT_MIN`, ce qui est un comportement indefini, et Release rendait le bon
  // catalogue par accident pendant qu'UBSan criait.
  //
  // La porte est double, et le second temps est le plus important. Exiger
  // l'egalite des catalogues ne prouverait rien si les echelles cessaient
  // d'atteindre la frontiere — une fixture peut devenir muette sans jamais
  // rougir. On exige donc AUSSI un temoin : sur ces nuages, au moins un
  // determinant reellement evalue doit changer de signe par troncature. La
  // fixture certifie ainsi qu'elle mord encore.
  {
    ++cases;
    int u16_faults = 0;
    long long truncation_witnesses = 0;
    struct ScaledTetra { const char* name; int scale; bool alternating; };
    const ScaledTetra tetras[] = {
        {"axial", 1290, false},   {"axial", 1291, false},   {"axial", 2048, false},
        {"alterne", 1023, true},  {"alterne", 1024, true},  {"alterne", 1025, true},
        {"alterne", 1626, true},
    };
    for (const ScaledTetra& t : tetras) {
      const int s = t.scale;
      std::vector<P3> cloud;
      if (t.alternating)
        cloud = {pt(0, 0, 0), pt(s, s, 0), pt(s, 0, s), pt(0, s, s)};
      else
        cloud = {pt(0, 0, 0), pt(s, 0, 0), pt(0, s, 0), pt(0, 0, s)};
      // Le temoin de troncature, mesure sur les memes quadruplets que le cone
      // tangent parcourt : tous les triplets ordonnes du nuage, contre le
      // quatrieme point.
      for (std::size_t a = 0; a < cloud.size(); ++a)
        for (std::size_t b = 0; b < cloud.size(); ++b)
          for (std::size_t c = 0; c < cloud.size(); ++c)
            for (std::size_t d = 0; d < cloud.size(); ++d) {
              if (a == b || a == c || b == c || d == a || d == b || d == c) continue;
              const mhgp::i128 det =
                  mhgp3v::flats::orient3d_exact(cloud[a], cloud[b], cloud[c], cloud[d]);
              const int exact = mhgp3v::flats::sign_of(det);
              const int truncated = (int)det > 0 ? 1 : ((int)det < 0 ? -1 : 0);
              if (exact != truncated) ++truncation_witnesses;
            }
      for (int s_max = 2; s_max <= 4; ++s_max) {
        for (int index = 0; index < 2; ++index) {
          mhgp3v::FlatStatistics a{}, b{};
          mhgp3v::CloudStatus sa = mhgp3v::CloudStatus::kOk, sb = mhgp3v::CloudStatus::kOk;
          const mhgp::Catalogue plain =
              mhgp3v::flat_catalogue(cloud, s_max, &a, &sa, false, index != 0, false);
          const mhgp::Catalogue owned =
              mhgp3v::flat_catalogue(cloud, s_max, &b, &sb, false, index != 0, true);
          if (sa != sb || !same_catalogue(plain, owned)) {
            printf("[frontiere u16 owner] %s echelle %d s_max=%d index=%d : %zu spheres"
                   " normales contre %zu sous proprietaire\n",
                   t.name, s, s_max, index, plain.spheres.size(), owned.spheres.size());
            ++u16_faults;
          }
        }
      }
    }
    // Sans temoin, la porte serait verte sans rien exercer.
    if (truncation_witnesses == 0) {
      printf("[frontiere u16 owner] ECHEC : aucune troncature 32 bits ne change de signe sur"
             " ces echelles — la fixture ne mord plus\n");
      ++u16_faults;
    }
    if (u16_faults) failures += u16_faults;
    else
      printf("[frontiere u16 owner] echelles 1290/1291/2048 et 1023/1024/1025/1626 :"
             " catalogues identiques sous proprietaire, %lld signes retournes par troncature\n",
             truncation_witnesses);
  }

  // DOMAINE COMPLET DU PROPRIETAIRE, contre-exemple exact de l'audit.
  //
  // Le proprietaire ne couvre que la recolte NAVIGUEE. La voie directe exhaustive
  // et les singletons non indexes n'ont aucun sommet, donc aucune notion de
  // propriete. Ma premiere version rejetait tout candidat nul et les supprimait en
  // SILENCE : quatre singletons perdus sur un tetraedre sans index, et la totalite
  // du catalogue perdue sur un triangle de dimension affine deux. Le differentiel
  // ne le voyait pas parce qu'il n'active le proprietaire que sur `kOk` et
  // toujours avec index.
  //
  // La porte exige donc que les QUATRE combinaisons index x proprietaire rendent
  // le meme catalogue, sur les deux nuages exacts de l'audit.
  {
    ++cases;
    struct Probe {
      const char* name;
      std::vector<P3> pts;
      int expected_spheres;
      mhgp3v::CloudStatus expected_status;
    };
    // Le triangle a trois points exerce `kTooFewPoints`, PAS la dimension affine :
    // il faut un nuage d'au moins quatre points COPLANAIRES pour l'autre branche.
    const std::vector<Probe> probes{
        {"tetraedre", {pt(0, 0, 0), pt(2, 0, 0), pt(0, 2, 0), pt(0, 0, 2)}, 11,
         mhgp3v::CloudStatus::kOk},
        {"triangle", {pt(0, 0, 0), pt(4, 0, 0), pt(1, 3, 0)}, 7,
         mhgp3v::CloudStatus::kTooFewPoints},
        {"coplanaire", {pt(0, 0, 0), pt(4, 0, 0), pt(1, 3, 0), pt(5, 3, 0), pt(2, 1, 0)}, -1,
         mhgp3v::CloudStatus::kAffineDimensionBelowThree}};
    int domain_faults = 0;
    for (const auto& probe : probes) {
      mhgp::Catalogue reference;
      mhgp3v::CloudStatus reference_status = mhgp3v::CloudStatus::kOk;
      for (int indexed = 0; indexed < 2; ++indexed)
        for (int owner = 0; owner < 2; ++owner) {
          mhgp3v::FlatStatistics pst{};
          mhgp3v::CloudStatus pstatus = mhgp3v::CloudStatus::kOk;
          const mhgp::Catalogue got = mhgp3v::flat_catalogue(
              probe.pts, 6, &pst, &pstatus, false, indexed != 0, owner != 0);
          if (indexed == 0 && owner == 0) {
            reference = got;
            reference_status = pstatus;
            // VERITE ATTENDUE, et non seulement coherence interne : deux catalogues
            // faux mais identiques passeraient sinon.
            if ((probe.expected_spheres >= 0 &&
                 (int)got.spheres.size() != probe.expected_spheres) ||
                pstatus != probe.expected_status) {
              printf("[domaine owner] %s : reference %zu spheres statut %s, attendu %d et %s\n",
                     probe.name, got.spheres.size(), mhgp3v::cloud_status_name(pstatus),
                     probe.expected_spheres, mhgp3v::cloud_status_name(probe.expected_status));
              ++domain_faults;
            }
            continue;
          }
          // Le STATUT fait partie de la comparaison, et les catalogues sont
          // compares en ENTIER — support, arite, rang, beta et membres.
          if (pstatus != reference_status || !same_catalogue(got, reference)) {
            printf("[domaine owner] %s : index=%d owner=%d rend %zu spheres statut %s, contre"
                   " %zu et %s\n", probe.name, indexed, owner, got.spheres.size(),
                   mhgp3v::cloud_status_name(pstatus), reference.spheres.size(),
                   mhgp3v::cloud_status_name(reference_status));
            ++domain_faults;
          }
        }
    }
    if (domain_faults) failures += domain_faults;
    else
      printf("[domaine owner] tetraedre navigable, triangle a trois points (kTooFewPoints) et"
             " nuage coplanaire a cinq points (dimension affine deux) : les quatre combinaisons"
             " index x proprietaire rendent le meme catalogue ENTIER, statut compris\n");
  }

  // FIXTURE ENTIERE DES DEUX POTENTIELS, coordonnees de l'audit.
  //
  //   A=(0,0,1) B=(1,0,1) C=(0,1,1)   E=(0,0,0) D=(0,0,2) F=(0,0,3)
  //
  // Avec base=[A,B,C] la normale est (0,0,1) et orient3d(A,B,C,X) = z_X - 1, donc
  // orient(E)=-1, orient(D)=1, orient(F)=2. Le sommet ABCE est de niveau zero et
  // ABCF a D interieur : la fixture exerce donc les DEUX potentiels — la decroissance
  // de Q_r au niveau zero, et la croissance de L_h au niveau positif — sur les memes
  // six points, avec des verdicts entiers exacts.
  //
  // Elle grave surtout la mutation que l'audit signale : la base ordonnee et la
  // direction se transportent ENSEMBLE. Permuter deux points de la base sans
  // inverser la direction change le signe de `orient3d` et inverse les quatre
  // verdicts — c'est verifie, pas commente.
  {
    ++cases;
    const std::vector<P3> six{pt(0, 0, 1), pt(1, 0, 1), pt(0, 1, 1),
                              pt(0, 0, 0), pt(0, 0, 2), pt(0, 0, 3)};
    const i32 base[3] = {0, 1, 2};
    const i32 swapped[3] = {1, 0, 2};              // permutation IMPAIRE de la base
    const std::vector<i32> root_base{0, 1, 2, 4};
    int faults = 0;
    if (mhgp3v::flats::orient3d_exact(six[0], six[1], six[2], six[3]) != -1) ++faults;
    if (mhgp3v::flats::orient3d_exact(six[0], six[1], six[2], six[4]) != 1) ++faults;
    if (mhgp3v::flats::orient3d_exact(six[0], six[1], six[2], six[5]) != 2) ++faults;

    mhgp3v::flats::Vertex abce;                    // niveau zero : Q_r doit decroitre
    abce.shell = {0, 1, 2, 3};
    abce.level = 0;
    mhgp3v::flats::Vertex abcf;                    // niveau un : L_h doit croitre
    abcf.shell = {0, 1, 2, 5};
    abcf.interior = {4};
    abcf.level = 1;

    if (!mhgp3v::flats::pair_admissible(six, abce, base, 1, root_base)) ++faults;
    if (mhgp3v::flats::pair_admissible(six, abce, base, -1, root_base)) ++faults;
    if (!mhgp3v::flats::pair_admissible(six, abcf, base, -1, root_base)) ++faults;
    if (mhgp3v::flats::pair_admissible(six, abcf, base, 1, root_base)) ++faults;
    // Base permutee SEULE : les quatre verdicts s'inversent.
    if (mhgp3v::flats::pair_admissible(six, abce, swapped, 1, root_base)) ++faults;
    if (!mhgp3v::flats::pair_admissible(six, abce, swapped, -1, root_base)) ++faults;
    if (mhgp3v::flats::pair_admissible(six, abcf, swapped, -1, root_base)) ++faults;
    if (!mhgp3v::flats::pair_admissible(six, abcf, swapped, 1, root_base)) ++faults;
    // MEMBRE DE COQUILLE OMIS. Il faut un sommet ou la coquille est le SEUL
    // blocage, sinon la mutation reste invisible : en ABCF la direction +1 est
    // refusee deux fois, par la chambre et par L_h, et omettre F ne change pas le
    // booleen. En ABCD au contraire les deux directions sont refusees chacune par un
    // seul filtre — +1 par la coquille via D, -1 par Q_r — et omettre D de la
    // coquille rend +1 admissible. C'est cette configuration qui grave la mutation.
    mhgp3v::flats::Vertex abcd;
    abcd.shell = {0, 1, 2, 4};
    abcd.level = 0;
    if (mhgp3v::flats::pair_admissible(six, abcd, base, 1, root_base)) ++faults;
    if (mhgp3v::flats::pair_admissible(six, abcd, base, -1, root_base)) ++faults;
    {
      mhgp3v::flats::Vertex truncated = abcd;
      truncated.shell = {0, 1, 2};
      if (!mhgp3v::flats::pair_admissible(six, truncated, base, 1, root_base)) ++faults;
    }
    // TROIS APPELS DIRECTS a la decision, que la fixture n'exercait pas : un couple
    // admissible ANTERIEUR, une cible ABSENTE, et une direction HOSTILE. Sans eux,
    // la fixture gravait les signes mais pas le decideur.
    {
      mhgp3v::flats::FlatAtVertex late;          // clef {1,2,3}, la derniere de ABCE
      late.base[0] = 1; late.base[1] = 2; late.base[2] = 3;
      late.closure = {1, 2, 3};
      // En ABCE le flat {0,1,2} precede et sa direction +1 est admissible : la
      // decision doit REFUSER, sans jamais atteindre la cible.
      if (mhgp3v::flats::decide_child(six, abce, late, -1, root_base) !=
          mhgp3v::flats::ChildOutcome::kReject) ++faults;
      // CIBLE ABSENTE, deux formes distinctes — et la premiere version de cette
      // fixture se trompait. Une cible de clef POSTERIEURE ne donne pas `kBroken` :
      // le premier couple admissible rencontre avant elle donne `kReject`, ce qui
      // est correct. Pour exercer l'echec ferme il faut soit une clef que
      // l'enumeration DEPASSE, soit une base divergente sur la bonne clef.
      mhgp3v::flats::FlatAtVertex passed;        // clef {0,1} : depassee des le premier flat
      passed.base[0] = 0; passed.base[1] = 1; passed.base[2] = 2;
      passed.closure = {0, 1};
      if (mhgp3v::flats::decide_child(six, abcf, passed, -1, root_base) !=
          mhgp3v::flats::ChildOutcome::kBroken) ++faults;
      mhgp3v::flats::FlatAtVertex diverging;     // bonne clef, base qui n'est pas la sienne
      diverging.base[0] = 0; diverging.base[1] = 1; diverging.base[2] = 5;
      diverging.closure = {0, 1, 2};
      if (mhgp3v::flats::decide_child(six, abcf, diverging, -1, root_base) !=
          mhgp3v::flats::ChildOutcome::kBroken) ++faults;
      mhgp3v::flats::FlatAtVertex plane;
      plane.base[0] = 0; plane.base[1] = 1; plane.base[2] = 2;
      plane.closure = {0, 1, 2};
      if (mhgp3v::flats::decide_child(six, abcf, plane, 0, root_base) !=
          mhgp3v::flats::ChildOutcome::kBroken) ++faults;
      if (mhgp3v::flats::pair_admissible(six, abce, base, 0, root_base)) ++faults;
      if (mhgp3v::flats::pair_admissible(six, abce, base, 2, root_base)) ++faults;
    }
    if (faults) {
      printf("[deux potentiels] %d faute(s) sur les verdicts entiers exacts\n", faults);
      ++failures;
    } else {
      printf("[deux potentiels] Q_r au niveau zero, L_h au niveau un, base permutee et"
             " coquille tronquee : quatorze verdicts entiers exacts, plus six"
             " appels directs a la decision\n");
    }
  }

  // FRONTIERE DE DOMAINE u16, verifiee a l'API et non au CLI. Les bornes de
  // largeur des predicats en dependent : un appelant qui n'est pas ce juge
  // obtiendrait sinon un depassement signe `__int128` avant tout predicat.
  {
    ++cases;
    struct Domain { const char* name; std::vector<P3> pts; bool must_refuse; };
    const std::vector<Domain> domains{
        {"hors_grille_1e9",
         {pt(0, 0, 0), pt(1000000000, 0, 0), pt(0, 1000000000, 0), pt(0, 0, 1000000000),
          pt(999999999, 1, 2)}, true},
        {"frontiere_65535",
         {pt(0, 0, 0), pt(65535, 0, 0), pt(0, 65535, 0), pt(0, 0, 65535), pt(1, 2, 3)}, false},
        {"frontiere_65536",
         {pt(0, 0, 0), pt(65536, 0, 0), pt(0, 65535, 0), pt(0, 0, 65535), pt(1, 2, 3)}, true},
        {"coordonnee_negative",
         {pt(0, 0, 0), pt(-1, 0, 0), pt(0, 4, 0), pt(0, 0, 4), pt(1, 2, 3)}, true},
    };
    for (const Domain& d : domains) {
      mhgp3v::FlatStatistics st{};
      mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
      const mhgp::Catalogue cat = mhgp3v::flat_catalogue(d.pts, 5, &st, &status, true);
      const bool refused = (status == mhgp3v::CloudStatus::kOutsideDeclaredGrid);
      if (refused != d.must_refuse || (refused && (!cat.spheres.empty() || !cat.members.empty()))) {
        printf("[domaine %s] statut=%s spheres=%zu (refus attendu=%d)\n", d.name,
               mhgp3v::cloud_status_name(status), cat.spheres.size(), (int)d.must_refuse);
        ++failures;
      }
    }
    printf("[domaine u16] quatre frontieres verifiees a l'API\n");
  }

  printf("=== fixtures ===\n");
  for (const Fix& f : fixtures) {
    for (int s = 2; s <= 8; ++s) { ++cases; if (!compare(f.name, f.pts, s, false)) ++failures; }
    ++cases;
    if (!permutation_equivariant(f.name, f.pts, 5, 24, rng)) ++failures;
  }

  printf("=== nuages generiques (%d nuages, %d points, coord < %d) ===\n", clouds, npoints, coord);
  std::uniform_int_distribution<int> dist(0, coord - 1);
  for (int c = 0; c < clouds; ++c) {
    // Points DISTINCTS : les doublons sont hors contrat et ont leur fixture
    // dediee. Les tirer ici ferait refuser la moitie d'une campagne saturee et
    // n'exercerait plus la navigation qu'elle est censee couvrir.
    std::vector<P3> pts;
    for (int guard = 0; (int)pts.size() < npoints && guard < 100 * npoints; ++guard) {
      const P3 q = pt(dist(rng), dist(rng), dist(rng));
      bool seen_point = false;
      for (const P3& r : pts) if (r.x == q.x && r.y == q.y && r.z == q.z) seen_point = true;
      if (!seen_point) pts.push_back(q);
    }
    if ((int)pts.size() < npoints) {
      printf("ECHEC : nuage generique %d non genere apres saturation du tirage\n", c);
      return 3;
    }
    ++generated_generic;
    for (int s = 2; s <= smax_hi; ++s) {
      ++cases;
      char tag[64];
      snprintf(tag, sizeof tag, "alea#%d", c);
      if (!compare(tag, pts, s, false)) ++failures;
    }
    if (c % 5 == 0) {                       // l'equivariance n'est plus reservee aux fixtures
      ++cases;
      char tag[64];
      snprintf(tag, sizeof tag, "alea#%d", c);
      if (!permutation_equivariant(tag, pts, smax_hi, 8, rng)) ++failures;
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
    ++generated_cospherical;
    for (int s = 2; s <= smax_hi; ++s) {
      ++cases;
      char tag[64];
      snprintf(tag, sizeof tag, "cospherique#%d", c);
      if (!compare(tag, pts, s, false)) ++failures;
    }
    if (c % 5 == 0) {
      ++cases;
      char tag[64];
      snprintf(tag, sizeof tag, "cospherique#%d", c);
      if (!permutation_equivariant(tag, pts, smax_hi, 8, rng)) ++failures;
    }
  }

  printf("\nfamilles : generiques demandes=%d generes=%d | cospheriques demandes=%d generes=%d\n",
         clouds, generated_generic, clouds / 4, generated_cospherical);
  if (generated_generic != clouds || generated_cospherical != clouds / 4) {
    printf("ECHEC : accord demande/genere viole par famille\n");
    return 3;
  }
  printf("\ncouverture : nuages navigues=%lld directs=%lld refuses=%lld | sommets=%lld"
         " census=%lld coquilles>4=%lld triplets quotientes=%lld lots>1=%lld"
         " equivariances=%lld\n",
         coverage.navigated_clouds, coverage.direct_clouds, coverage.refused_clouds,
         coverage.vertices, coverage.census, coverage.multiple_shells,
         coverage.quotiented, coverage.multiple_batches, coverage.equivariance_runs);
  printf("index : executions=%lld points touches=%lld amorces=%lld balayages complets=%lld\n",
         coverage.indexed_runs, coverage.grid_touched, coverage.bootstrap,
         coverage.full_sweeps);
  printf("gate D : sommets avec parent teste=%lld  racines=%lld\n",
         coverage.parent_vertices, coverage.parent_roots);
  printf("reverse : sommets=%lld  indexes=%lld  profondeur max=%lld  fils testes=%lld"
         "  flats livres=%lld  decisions=%lld  portes sautees=%lld\n",
         coverage.reverse_vertices, coverage.reverse_indexed_vertices, coverage.reverse_depth,
         coverage.reverse_children, coverage.reverse_flats, coverage.reverse_decisions,
         coverage.reverse_skipped);
  // TRAVAIL TOTAL, et non les seuls flats livres. Un triplet ecarte a paye sa
  // fermeture : publier « flats par sommet » sans ces deux colonnes flatte le
  // ratio d'un facteur qui est ici mesure, pas suppose.
  printf("        : triplets balayes=%lld  fermetures reconstruites=%lld"
         "  noeuds de l'index construits=%lld visites=%lld dont feuilles=%lld\n",
         coverage.reverse_triplets, coverage.reverse_closures, coverage.index_internal_nodes,
         coverage.index_nodes_visited, coverage.index_leaves_visited);
  // Le HIGH-WATER est la borne revendiquee : slots vifs du chemin, sortie exclue.
  // Le rapporter a cote du nombre de sommets rendus est tout l'interet du sink.
  printf("        : sink sommets rendus=%lld  slots vifs maximum=%lld  interruptions=%lld\n",
         coverage.sink_vertices, coverage.sink_high_water, coverage.sink_interruptions);
  // Le GAIN de la sortie precoce du parent, mesure sur les memes sommets : ce
  // rapport est la seule chose qui autorise a parler de gain.
  printf("        : refus par le couple de retour=%lld (O(m), sans fermeture)"
         "  refus par la decision=%lld\n", coverage.reject_backward, coverage.reject_by_parent);
  printf("        : couples juges deux fois=%lld  acceptes=%lld  prefiltres=%lld\n",
         coverage.pairs_judged, coverage.pairs_accepted, coverage.pairs_prefiltered);
  printf("device : sommets admis=%lld  refuses par capacite=%lld  couples juges=%lld\n",
         coverage.device_admitted, coverage.device_refused, coverage.device_pairs);
  printf("       : high-water coquille=%lld fermeture=%lld touches=%lld lot=%lld interieur=%lld"
         "  (capacites %d / %d)  balayages exhaustifs=%lld\n", coverage.shell_high_water,
         coverage.closure_high_water, coverage.touched_high_water, coverage.batch_high_water,
         coverage.interior_high_water, mhgp3v::device::kMaxShell, mhgp3v::device::kMaxInterior,
         coverage.exhaustive_scans);
  printf("       : tentatives d'admission=%lld  dont refusees pour capacite=%lld\n",
         coverage.admission_samples, coverage.admission_overflow);
  printf("owner  : emises=%lld  refus support non canonique=%lld  refus autre proprietaire=%lld"
         "  table residuelle maximum=%lld\n", coverage.owner_emitted,
         coverage.owner_rejected_support, coverage.owner_rejected_vertex,
         coverage.dedup_table_max);
  printf("        : parent precoce=%lld fermetures  balayage complet=%lld  rapport %.2f\n",
         coverage.parent_early_closures, coverage.parent_full_closures,
         coverage.parent_early_closures
             ? (double)coverage.parent_full_closures / (double)coverage.parent_early_closures
             : 0.0);
  printf("\n%d cas, %d desaccords\n", cases, failures);
  if (coverage.navigated_clouds < min_navigated || coverage.vertices < min_vertices
      || coverage.multiple_shells < min_multiple_shells
      || coverage.quotiented < min_quotiented || coverage.reverse_vertices < min_reverse
      || coverage.reverse_depth < min_reverse_depth
      || coverage.reverse_indexed_vertices < min_reverse
      || coverage.index_internal_nodes < min_internal_nodes
      || coverage.index_nodes_visited < min_internal_nodes
      || coverage.sink_vertices < min_reverse || coverage.sink_interruptions < min_reverse_depth
      || coverage.reverse_decisions < min_reverse_depth
      || coverage.reject_backward < min_reverse_depth
      || coverage.pairs_judged < min_reverse || coverage.pairs_accepted < min_reverse_depth
      || coverage.pairs_prefiltered < min_reverse_depth
      || coverage.owner_emitted < min_owner || coverage.owner_rejected_vertex < min_owner
      || coverage.owner_rejected_support < min_owner / 32) {
    printf("ECHEC : plancher de couverture non atteint — navigues %lld/%d, sommets %lld/%d, "
           "coquilles multiples %lld/%d, triplets quotientes %lld/%d\n",
           coverage.navigated_clouds, min_navigated, coverage.vertices, min_vertices,
           coverage.multiple_shells, min_multiple_shells, coverage.quotiented, min_quotiented);
    printf("        owner emises %lld/%d, refus sommet %lld/%d, refus support %lld/%d\n",
           coverage.owner_emitted, min_owner, coverage.owner_rejected_vertex, min_owner,
           coverage.owner_rejected_support, min_owner / 32);
    printf("        reverse %lld/%d, indexes %lld/%d, decisions %lld/%d, refus O(m) %lld/%d,"
           " profondeur %lld/%d, noeuds internes %lld/%d\n", coverage.reverse_vertices,
           min_reverse, coverage.reverse_indexed_vertices, min_reverse,
           coverage.reverse_decisions, min_reverse_depth, coverage.reject_backward,
           min_reverse_depth, coverage.reverse_depth, min_reverse_depth,
           coverage.index_internal_nodes, min_internal_nodes);
    return 3;
  }
  // LES CAPACITES DU NOYAU DEVICE SONT JUGEES, pas declarees. Un high-water au-dela
  // de la capacite est un refus non compte ; un high-water nul alors que des sommets
  // ont ete admis est un compteur MORT, ce qui est pire qu'une capacite fausse.
  //
  // Ma premiere version traitait un depassement de capacite comme un ECHEC. C'est
  // faux : un depassement correctement REFUSE puis rejoue est precisement le
  // contrat d'une capacite bornee, et l'audit l'a montre sur une campagne a
  // `points=21 smax=19` ou un interieur de 17 etait refuse 94 fois sans aucun
  // desaccord — et ou ma porte echouait quand meme. Ce qui doit etre exige est
  // l'ACCORD des sommets admis, deja verifie couple par couple, et la coherence du
  // refus : tout depassement observe doit etre compte comme refus.
  //
  // Et un maximum NUL n'est pas un compteur mort : un sommet de niveau zero a un
  // interieur vide, c'est une valeur geometrique legitime. Seul un nombre
  // d'ECHANTILLONS nul denonce l'instrumentation.
  if (coverage.device_admitted > 0 && coverage.admission_samples == 0) {
    printf("ECHEC : le noyau device a admis %lld sommets mais n'a enregistre aucune"
           " tentative d'admission — compteur mort\n", coverage.device_admitted);
    return 3;
  }
  if (coverage.shell_high_water > mhgp3v::device::kMaxShell &&
      coverage.admission_overflow == 0) {
    printf("ECHEC : coquille %lld au-dela de la capacite %d sans aucun refus compte\n",
           coverage.shell_high_water, mhgp3v::device::kMaxShell);
    return 3;
  }
  if (coverage.interior_high_water > mhgp3v::device::kMaxInterior &&
      coverage.admission_overflow == 0) {
    printf("ECHEC : interieur %lld au-dela de la capacite %d sans aucun refus compte\n",
           coverage.interior_high_water, mhgp3v::device::kMaxInterior);
    return 3;
  }

  // Une porte SAUTEE n'est pas une porte passee : le plancher l'interdit.
  if (min_reverse > 0 && coverage.reverse_skipped > 0) {
    printf("ECHEC : %lld portes reverse sautees pour statut non kOk — un plancher reverse"
           " exige zero saut\n", coverage.reverse_skipped);
    return 3;
  }
  if (cases < min_cases) {
    printf("ECHEC : plancher non atteint, %d cas pour %d exiges — une campagne vide "
           "ou censuree ne peut pas rendre OK\n", cases, min_cases);
    return 3;
  }
  if (failures == 0) printf("OK : sommets, catalogue, census et equivariance concordants\n");
  return failures == 0 ? 0 : 1;
}
