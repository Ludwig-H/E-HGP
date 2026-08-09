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
};
static Coverage coverage;

static bool has_duplicates(const std::vector<P3>& pts) {
  std::vector<std::array<long long, 3>> v;
  for (const P3& p : pts) v.push_back({(long long)p.x, (long long)p.y, (long long)p.z});
  std::sort(v.begin(), v.end());
  return std::adjacent_find(v.begin(), v.end()) != v.end();
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
    if (index_faults) failures += index_faults;
    else printf("[index] grande sphere, noeuds internes et accord exhaustif verifies\n");
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
  printf("\n%d cas, %d desaccords\n", cases, failures);
  if (coverage.navigated_clouds < min_navigated || coverage.vertices < min_vertices
      || coverage.multiple_shells < min_multiple_shells
      || coverage.quotiented < min_quotiented) {
    printf("ECHEC : plancher de couverture non atteint — navigues %lld/%d, sommets %lld/%d, "
           "coquilles multiples %lld/%d, triplets quotientes %lld/%d\n",
           coverage.navigated_clouds, min_navigated, coverage.vertices, min_vertices,
           coverage.multiple_shells, min_multiple_shells, coverage.quotiented, min_quotiented);
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
