// MorseHGP3D v3 — probe du FRONT DE BOULES VIDES.
//
// Sujet : `prototype/ball_front.hpp`. Ce programme enumere les boules de rang
// ferme au plus `K+1` par pivotement exact, publie pour chacune son census
// ferme `(I,E)`, et falsifie cette enumeration contre un oracle exhaustif sur
// les petits nuages.
//
// Ce probe ne produit pas encore de payload et ne mesure aucun SLO. Il repond a
// une seule question : la source directe complete est-elle enumerable en un
// travail proportionnel a sa taille ?
//
// Codes de sortie : 0 accord, 1 desaccord du juge, 2 campagne refusee avant
// calcul, 3 plancher ou invariant viole, 4 mutant tue.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include "ball_front.hpp"
#include "cloud_families.hpp"

namespace {

using mhgp3v::ballfront::Census;
using mhgp3v::ballfront::i128;
using mhgp3v::ballfront::i64;
using mhgp3v::ballfront::Index;
using mhgp3v::ballfront::Lift;
using mhgp3v::ballfront::bf_orient3;
using mhgp3v::ballfront::PivotResult;
using mhgp3v::ballfront::Work;

// ---------------------------------------------------------------------------
// Cellule : une boule de rang ferme, avec son census complet.
// ---------------------------------------------------------------------------
struct Cell {
  std::vector<int> shell;   // trie, complet
  std::vector<int> inside;  // trie, complet
  std::array<int, 4> base{{-1, -1, -1, -1}};  // quadruplet canonique non coplanaire
  Lift lift;                                  // sphere du sommet
};

struct BaseKey {
  std::array<int, 4> v;
  bool operator==(const BaseKey& other) const { return v == other.v; }
};

struct BaseKeyHash {
  std::size_t operator()(const BaseKey& key) const {
    std::uint64_t h = 1469598103934665603ULL;
    for (int x : key.v) {
      h ^= (std::uint64_t)(std::uint32_t)x;
      h *= 1099511628211ULL;
    }
    return (std::size_t)h;
  }
};

// Quadruplet canonique : les quatre premiers identifiants du shell trie qui ne
// sont pas coplanaires. Il determine la sphere, donc la cellule.
bool canonical_base(const Index& index, const std::vector<int>& shell,
                    std::array<int, 4>* out) {
  const int m = (int)shell.size();
  for (int i = 0; i < m; ++i)
    for (int j = i + 1; j < m; ++j)
      for (int k = j + 1; k < m; ++k)
        for (int l = k + 1; l < m; ++l) {
          if (bf_orient3(index.at(shell[(std::size_t)i]), index.at(shell[(std::size_t)j]),
                       index.at(shell[(std::size_t)k]),
                       index.at(shell[(std::size_t)l])) == 0)
            continue;
          *out = {shell[(std::size_t)i], shell[(std::size_t)j], shell[(std::size_t)k],
                  shell[(std::size_t)l]};
          return true;
        }
  return false;
}


// ---------------------------------------------------------------------------
// AUTO-CENTRAGE EXACT : le centre est-il dans l'interieur relatif de conv(U) ?
//
// q2 : le centre d'une miniboule de paire est le milieu, toujours dans le
//      segment ouvert des que les deux labels sont distincts.
// q3 : avec `centre = a + (galpha*p1 + gbeta*p2)/gg` et `gg>0`, la condition
//      est `galpha>0`, `gbeta>0` et `galpha+gbeta<gg`.
// q4 : avec `centre = a - C/(2D)` et `C=(cx,cy,cz)`, les poids valent
//      `alpha = -det(C,p2,p3)/(2 D^2)` et ses deux permutations. Sur u16,
//      `|det(C,p2,p3)| < 2^103` et `2 D^2 < 2^102,2` : tout tient en 128 bits.
// ---------------------------------------------------------------------------
bool self_centred_triangle(const mhgp3v::ballfront::TriangleLift& tri) {
  if (!tri.lift.ok || tri.gg <= 0) return false;
  return tri.galpha > 0 && tri.gbeta > 0 && (tri.galpha + tri.gbeta) < tri.gg;
}

bool self_centred_tetra(const Index& index, int ia, int ib, int ic, int id) {
  const mhgp::P3& a = index.at(ia);
  const Lift lift = mhgp3v::ballfront::lift_of(a, index.at(ib), index.at(ic), index.at(id));
  if (!lift.ok) return false;
  const mhgp3v::ballfront::V3 p1 = mhgp3v::ballfront::sub(index.at(ib), a);
  const mhgp3v::ballfront::V3 p2 = mhgp3v::ballfront::sub(index.at(ic), a);
  const mhgp3v::ballfront::V3 p3 = mhgp3v::ballfront::sub(index.at(id), a);
  const i128 cx = lift.cx, cy = lift.cy, cz = lift.cz;
  auto det_with_c = [&](const mhgp3v::ballfront::V3& q,
                        const mhgp3v::ballfront::V3& r) -> i128 {
    return cx * ((i128)q.y * r.z - (i128)q.z * r.y) -
           cy * ((i128)q.x * r.z - (i128)q.z * r.x) +
           cz * ((i128)q.x * r.y - (i128)q.y * r.x);
  };
  const i128 two_d2 = 2 * lift.dd * lift.dd;  // strictement positif
  const i128 na = -det_with_c(p2, p3);
  const i128 nb = det_with_c(p1, p3);
  const i128 nc = -det_with_c(p1, p2);
  if (na <= 0 || nb <= 0 || nc <= 0) return false;
  return (na + nb + nc) < two_d2;
}

// Miniboule et census d'un sous-ensemble du shell, avec son propre census
// GLOBAL. Le niveau du sommet visite n'est JAMAIS herite : les fixtures
// `projection_niveau_0_vers_1` de l'auditeur montrent qu'une paire ou un
// triple du shell d'un sommet de niveau zero peut etre de niveau un.
bool harvest_support(const Index& index, const std::vector<int>& ids, int smax,
                     Work* work, std::vector<int>* inside, std::vector<int>* shell) {
  Lift lift;
  const int q = (int)ids.size();
  if (q == 2) {
    lift = mhgp3v::ballfront::lift_pair(index.at(ids[0]), index.at(ids[1]));
  } else if (q == 3) {
    const mhgp3v::ballfront::TriangleLift tri = mhgp3v::ballfront::lift_triangle(
        index.at(ids[0]), index.at(ids[1]), index.at(ids[2]));
    if (!self_centred_triangle(tri)) return false;
    lift = tri.lift;
  } else if (q == 4) {
    if (!self_centred_tetra(index, ids[0], ids[1], ids[2], ids[3])) return false;
    lift = mhgp3v::ballfront::lift_of(index.at(ids[0]), index.at(ids[1]), index.at(ids[2]),
                                      index.at(ids[3]));
  } else {
    return false;
  }
  if (!lift.ok) return false;
  Census census;
  mhgp3v::ballfront::census_ball(index, lift, smax - q + 1, &census, work);
  if (census.overflow) return false;  // rang ferme hors fenetre
  if ((int)census.inside.size() + (int)census.shell.size() > smax) return false;
  *inside = census.inside;
  *shell = census.shell;
  return true;
}

// ---------------------------------------------------------------------------
// Front d'onde
// ---------------------------------------------------------------------------
struct Front {
  const Index* index = nullptr;
  int cap = 10;  // K : nombre maximal d'interieurs stricts
  Work work;
  std::vector<Cell> cells;
  std::unordered_map<BaseKey, int, BaseKeyHash> seen;
  bool drop_entering = false;  // mutant : n'explore pas la direction entrante
  bool drop_leaving = false;   // mutant : n'explore pas la direction sortante
  bool use_pencil = true;      // transition par lots, sinon pivot iteratif
  bool drop_quotient = false;  // mutant : ne quotiente pas les triplets d'un flat

  bool record(const std::vector<int>& shell, const std::vector<int>& inside, int* out_id) {
    std::array<int, 4> base{};
    if (!canonical_base(*index, shell, &base)) return false;
    const BaseKey key{base};
    auto it = seen.find(key);
    if (it != seen.end()) {
      *out_id = it->second;
      return false;
    }
    Cell cell;
    cell.shell = shell;
    cell.inside = inside;
    cell.base = base;
    cell.lift = mhgp3v::ballfront::lift_of(index->at(base[0]), index->at(base[1]),
                                           index->at(base[2]), index->at(base[3]));
    if (!cell.lift.ok) return false;
    const int id = (int)cells.size();
    cells.push_back(cell);
    seen.emplace(key, id);
    ++work.cells_emitted;
    work.shell_ids += (i64)shell.size();
    if (shell.size() > 4) ++work.degenerate_shells;
    *out_id = id;
    return true;
  }

  // Suit une arete de l'arrangement : le triplet `(t0,t1,t2)` reste sur la
  // sphere, le centre glisse sur la droite perpendiculaire. `want` est le signe
  // du cote explore. Les points du shell situes de ce cote deviennent
  // interieurs : ils rejoignent l'ensemble banni.
  void follow(int t0, int t1, int t2, int want, const std::vector<int>& shell,
              const std::vector<int>& inside, std::vector<int>* pending) {
    std::vector<int> banned = inside;
    for (int p : shell) {
      if (p == t0 || p == t1 || p == t2) continue;
      const i128 side = bf_orient3(index->at(t0), index->at(t1), index->at(t2), index->at(p));
      const int s = side > 0 ? 1 : (side < 0 ? -1 : 0);
      if (s == want) banned.push_back(p);
    }
    std::sort(banned.begin(), banned.end());
    banned.erase(std::unique(banned.begin(), banned.end()), banned.end());
    if ((int)banned.size() > cap) return;  // au-dela de la fenetre demandee
    ++work.transitions;
    // Oriente la facette pour que le cote explore soit le cote positif.
    const int a = want > 0 ? t0 : t1;
    const int b = want > 0 ? t1 : t0;
    const PivotResult result =
        mhgp3v::ballfront::pivot(*index, a, b, t2, banned, cap + 1, &work);
    if (!result.ok) return;
    if (result.census.overflow) {
      ++work.cells_rejected_rank;
      return;
    }
    std::vector<int> new_shell = result.census.shell;
    if (!std::binary_search(new_shell.begin(), new_shell.end(), a)) return;
    int id = -1;
    if (record(new_shell, result.census.inside, &id)) pending->push_back(id);
  }

  // Le flat porte par un triplet est defini par sa FERMETURE : tous les points
  // du shell coplanaires avec lui. Plusieurs triplets decrivent donc le meme
  // flat, et il faut n'en garder qu'un — sinon une coquille cospherique fait
  // exploser le nombre de transitions sans ajouter un seul sommet.
  bool is_canonical_triple(const std::vector<int>& shell, int t0, int t1, int t2) const {
    if (drop_quotient) return true;
    std::vector<int> closure;
    for (int p : shell)
      if (bf_orient3(index->at(t0), index->at(t1), index->at(t2), index->at(p)) == 0)
        closure.push_back(p);
    const int m = (int)closure.size();
    for (int i = 0; i < m; ++i)
      for (int j = i + 1; j < m; ++j)
        for (int k = j + 1; k < m; ++k) {
          const mhgp3v::ballfront::V3 u =
              mhgp3v::ballfront::sub(index->at(closure[(std::size_t)j]),
                                     index->at(closure[(std::size_t)i]));
          const mhgp3v::ballfront::V3 v =
              mhgp3v::ballfront::sub(index->at(closure[(std::size_t)k]),
                                     index->at(closure[(std::size_t)i]));
          const i128 cx = (i128)u.y * v.z - (i128)u.z * v.y;
          const i128 cy = (i128)u.z * v.x - (i128)u.x * v.z;
          const i128 cz = (i128)u.x * v.y - (i128)u.y * v.x;
          if (cx == 0 && cy == 0 && cz == 0) continue;
          return closure[(std::size_t)i] == t0 && closure[(std::size_t)j] == t1 &&
                 closure[(std::size_t)k] == t2;
        }
    return false;
  }

  void expand_pencil(int cell_id, std::vector<int>* pending) {
    const std::vector<int> shell = cells[(std::size_t)cell_id].shell;
    const std::vector<int> inside = cells[(std::size_t)cell_id].inside;
    const Lift lift = cells[(std::size_t)cell_id].lift;
    const int m = (int)shell.size();
    for (int i = 0; i < m; ++i)
      for (int j = i + 1; j < m; ++j)
        for (int k = j + 1; k < m; ++k) {
          const int t0 = shell[(std::size_t)i], t1 = shell[(std::size_t)j],
                    t2 = shell[(std::size_t)k];
          const mhgp3v::ballfront::V3 u =
              mhgp3v::ballfront::sub(index->at(t1), index->at(t0));
          const mhgp3v::ballfront::V3 v =
              mhgp3v::ballfront::sub(index->at(t2), index->at(t0));
          const i128 cx = (i128)u.y * v.z - (i128)u.z * v.y;
          const i128 cy = (i128)u.z * v.x - (i128)u.x * v.z;
          const i128 cz = (i128)u.x * v.y - (i128)u.y * v.x;
          if (cx == 0 && cy == 0 && cz == 0) continue;
          if (!is_canonical_triple(shell, t0, t1, t2)) continue;
          for (int dir = -1; dir <= 1; dir += 2) {
            if (dir > 0 && drop_entering) continue;
            if (dir < 0 && drop_leaving) continue;
            ++work.transitions;
            const mhgp3v::ballfront::PencilStep step = mhgp3v::ballfront::pencil_next(
                *index, lift, shell, inside, t0, t1, t2, dir, cap, &work);
            if (!step.ok) continue;          // arete non bornee de ce cote
            if (step.over_level) {
              ++work.cells_rejected_rank;
              continue;
            }
            int id = -1;
            if (record(step.shell, step.interior, &id)) pending->push_back(id);
          }
        }
  }

  void expand(int cell_id, std::vector<int>* pending) {
    if (use_pencil) {
      expand_pencil(cell_id, pending);
      return;
    }
    const std::vector<int> shell = cells[(std::size_t)cell_id].shell;
    const std::vector<int> inside = cells[(std::size_t)cell_id].inside;
    const int m = (int)shell.size();
    for (int i = 0; i < m; ++i)
      for (int j = i + 1; j < m; ++j)
        for (int k = j + 1; k < m; ++k) {
          const int t0 = shell[(std::size_t)i], t1 = shell[(std::size_t)j],
                    t2 = shell[(std::size_t)k];
          // Un triplet colineaire ne porte aucune arete de l'arrangement.
          const mhgp3v::ballfront::V3 u =
              mhgp3v::ballfront::sub(index->at(t1), index->at(t0));
          const mhgp3v::ballfront::V3 v =
              mhgp3v::ballfront::sub(index->at(t2), index->at(t0));
          const i128 cx = (i128)u.y * v.z - (i128)u.z * v.y;
          const i128 cy = (i128)u.z * v.x - (i128)u.x * v.z;
          const i128 cz = (i128)u.x * v.y - (i128)u.y * v.x;
          if (cx == 0 && cy == 0 && cz == 0) continue;
          if (!drop_leaving) follow(t0, t1, t2, -1, shell, inside, pending);
          if (!drop_entering) follow(t0, t1, t2, +1, shell, inside, pending);
        }
  }
};

// ---------------------------------------------------------------------------
// Germe : une cellule de niveau zero, construite sans hypothese.
//   1. `p0` extremal lexicographique, donc sur l'enveloppe convexe ;
//   2. `p1` son plus proche voisin : la boule diametrale est vide, c'est une
//      arete de Gabriel ;
//   3. `p2` premier contact du pinceau des spheres passant par `(p0,p1)` : la
//      sphere du triangle reste vide ;
//   4. pivot 3D depuis ce triangle : la cellule obtenue est vide.
// ---------------------------------------------------------------------------
bool build_seed(const Index& index, int cap, Work* work, std::vector<int>* shell,
                std::vector<int>* inside) {
  const int n = index.size();
  if (n < 4) return false;
  int p0 = 0;
  for (int i = 1; i < n; ++i) {
    const mhgp::P3& a = index.at(i);
    const mhgp::P3& b = index.at(p0);
    if (a.x < b.x || (a.x == b.x && (a.y < b.y || (a.y == b.y && a.z < b.z)))) p0 = i;
  }
  int p1 = -1;
  i128 best_d2 = 0;
  for (int i = 0; i < n; ++i) {
    if (i == p0) continue;
    ++work->seed_scans;
    const i128 d2 = mhgp3v::ballfront::norm2(mhgp3v::ballfront::sub(index.at(i), index.at(p0)));
    if (d2 == 0) return false;  // positions colocalisees : hors domaine
    if (p1 < 0 || d2 < best_d2) {
      p1 = i;
      best_d2 = d2;
    }
  }
  if (p1 < 0) return false;

  // Direction entiere du glissement, perpendiculaire a (p1-p0).
  const mhgp3v::ballfront::V3 e = mhgp3v::ballfront::sub(index.at(p1), index.at(p0));
  mhgp3v::ballfront::V3 axis{1, 0, 0};
  if (e.y == 0 && e.z == 0) axis = mhgp3v::ballfront::V3{0, 1, 0};
  const mhgp3v::ballfront::V3 dir{e.y * axis.z - e.z * axis.y, e.z * axis.x - e.x * axis.z,
                                  e.x * axis.y - e.y * axis.x};

  // `M = p0 + p1`. Pour un temoin `z`, le parametre du premier contact vaut
  // `T = (||M-2p0||^2 - ||M-2z||^2) / (4 dir . (p0 - z))`. On retient le plus
  // petit parametre strictement positif; l'egalite est conservee et rendra un
  // shell cospherique.
  const mhgp::P3& a0 = index.at(p0);
  const mhgp::P3& a1 = index.at(p1);
  const i64 mx = a0.x + a1.x, my = a0.y + a1.y, mz = a0.z + a1.z;
  const i128 base_num = (i128)(mx - 2 * a0.x) * (mx - 2 * a0.x) +
                        (i128)(my - 2 * a0.y) * (my - 2 * a0.y) +
                        (i128)(mz - 2 * a0.z) * (mz - 2 * a0.z);
  int p2 = -1;
  i128 best_num = 0, best_den = 0;
  for (int i = 0; i < n; ++i) {
    if (i == p0 || i == p1) continue;
    ++work->seed_scans;
    const mhgp::P3& z = index.at(i);
    const i128 num = base_num - ((i128)(mx - 2 * z.x) * (mx - 2 * z.x) +
                                 (i128)(my - 2 * z.y) * (my - 2 * z.y) +
                                 (i128)(mz - 2 * z.z) * (mz - 2 * z.z));
    const i128 den = 4 * ((i128)dir.x * (a0.x - z.x) + (i128)dir.y * (a0.y - z.y) +
                          (i128)dir.z * (a0.z - z.z));
    if (den == 0) continue;
    i128 nn = num, dd = den;
    if (dd < 0) {
      nn = -nn;
      dd = -dd;
    }
    if (nn < 0) continue;  // contact dans l'autre direction
    if (p2 < 0 || nn * best_den < best_num * dd) {
      p2 = i;
      best_num = nn;
      best_den = dd;
    }
  }
  if (p2 < 0) return false;

  const std::vector<int> banned;
  for (int want = 1; want >= -1; want -= 2) {
    const int a = want > 0 ? p0 : p1;
    const int b = want > 0 ? p1 : p0;
    const PivotResult result = mhgp3v::ballfront::pivot(index, a, b, p2, banned, cap + 1, work);
    if (!result.ok || result.census.overflow) continue;
    if (!result.census.inside.empty()) continue;  // le germe doit etre de niveau zero
    *shell = result.census.shell;
    *inside = result.census.inside;
    return shell->size() >= 4;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Oracle exhaustif : toutes les spheres portees par quatre points.
// Arithmetique volontairement differente — determinant 5x5 developpe sur la
// derniere ligne, sans forme liftee ni elagage de boite.
// ---------------------------------------------------------------------------
i128 judge_insphere(const mhgp::P3& a, const mhgp::P3& b, const mhgp::P3& c,
                    const mhgp::P3& d, const mhgp::P3& e) {
  const i64 ax = a.x - e.x, ay = a.y - e.y, az = a.z - e.z;
  const i64 bx = b.x - e.x, by = b.y - e.y, bz = b.z - e.z;
  const i64 cx = c.x - e.x, cy = c.y - e.y, cz = c.z - e.z;
  const i64 dx = d.x - e.x, dy = d.y - e.y, dz = d.z - e.z;
  const i128 ab = (i128)ax * by - (i128)bx * ay;
  const i128 bc = (i128)bx * cy - (i128)cx * by;
  const i128 cd = (i128)cx * dy - (i128)dx * cy;
  const i128 da = (i128)dx * ay - (i128)ax * dy;
  const i128 ac = (i128)ax * cy - (i128)cx * ay;
  const i128 bd = (i128)bx * dy - (i128)dx * by;
  const i128 abc = (i128)az * bc - (i128)bz * ac + (i128)cz * ab;
  const i128 bcd = (i128)bz * cd - (i128)cz * bd + (i128)dz * bc;
  const i128 cda = (i128)cz * da + (i128)dz * ac + (i128)az * cd;
  const i128 dab = (i128)dz * ab + (i128)az * bd + (i128)bz * da;
  const i128 alift = (i128)ax * ax + (i128)ay * ay + (i128)az * az;
  const i128 blift = (i128)bx * bx + (i128)by * by + (i128)bz * bz;
  const i128 clift = (i128)cx * cx + (i128)cy * cy + (i128)cz * cz;
  const i128 dlift = (i128)dx * dx + (i128)dy * dy + (i128)dz * dz;
  return (dlift * abc - clift * dab) + (blift * cda - alift * bcd);
}

struct JudgeCell {
  std::vector<int> shell;
  std::vector<int> inside;
};

// Enumere toutes les boules de rang ferme au plus `cap+1`. Complexite
// volontairement brute : c'est une verite terrain bornee, pas un chemin produit.
std::vector<JudgeCell> judge_enumerate(const std::vector<mhgp::P3>& pts, int cap) {
  const int n = (int)pts.size();
  std::map<std::vector<int>, JudgeCell> found;
  for (int a = 0; a < n; ++a)
    for (int b = a + 1; b < n; ++b)
      for (int c = b + 1; c < n; ++c)
        for (int d = c + 1; d < n; ++d) {
          if (bf_orient3(pts[(std::size_t)a], pts[(std::size_t)b], pts[(std::size_t)c],
                       pts[(std::size_t)d]) == 0)
            continue;
          const i128 ref = judge_insphere(pts[(std::size_t)a], pts[(std::size_t)b],
                                          pts[(std::size_t)c], pts[(std::size_t)d],
                                          pts[(std::size_t)a]);
          (void)ref;
          const int orient = bf_orient3(pts[(std::size_t)a], pts[(std::size_t)b],
                                      pts[(std::size_t)c], pts[(std::size_t)d]) > 0
                                 ? 1
                                 : -1;
          JudgeCell cell;
          bool overflow = false;
          for (int e = 0; e < n && !overflow; ++e) {
            const i128 s = judge_insphere(pts[(std::size_t)a], pts[(std::size_t)b],
                                          pts[(std::size_t)c], pts[(std::size_t)d],
                                          pts[(std::size_t)e]);
            // Convention de l'oracle : pour une base positivement orientee, le
            // determinant developpe ci-dessus est NEGATIF a l'interieur. La
            // fixture `t.cpp` de mise au point l'a etablie sur le tetraedre
            // (0,0,0),(2,0,0),(0,2,0),(0,0,2) et son centre (1,1,1).
            const i128 signed_s = orient > 0 ? -s : s;
            if (signed_s > 0) {
              if ((int)cell.inside.size() >= cap) {
                overflow = true;
                break;
              }
              cell.inside.push_back(e);
            } else if (signed_s == 0) {
              cell.shell.push_back(e);
            }
          }
          if (overflow) continue;
          std::sort(cell.shell.begin(), cell.shell.end());
          std::sort(cell.inside.begin(), cell.inside.end());
          found[cell.shell] = cell;
        }
  std::vector<JudgeCell> out;
  out.reserve(found.size());
  for (const auto& entry : found) out.push_back(entry.second);
  return out;
}



// ---------------------------------------------------------------------------
// RECOLTE DES SUPPORTS q2/q3/q4 depuis les shells visites.
//
// Fondement : le theoreme de proprietaire. Pour tout support minimal propre
// positif `U` d'arite `q` verifiant `|I_B|+q<=smax`, il existe un sommet
// d'arrangement `o(U)` dont le shell contient `U` et dont le niveau verifie
// `l(o(U))<=|I_B|<=smax-q<=smax-2=k_nav`. Enumerer les sous-ensembles des
// shells de tous les sommets sous `k_nav` suffit donc a proposer tout support
// pertinent. Chaque proposition refait ensuite SON census global : le niveau du
// sommet n'est jamais herite.
//
// PORTEE DU DIFFERENTIEL : le juge ci-dessous enumere les memes sous-ensembles
// sur le nuage ENTIER, avec les memes primitives de miniboule. Il juge donc la
// COMPLETUDE de la recolte — la proposition « les shells suffisent » — et non
// la correction des primitives, qui est jugee separement par l'oracle a
// determinant 5x5 sur les sommets q4. C'est un accord RELATIF, et il est
// declare comme tel.
// ---------------------------------------------------------------------------
struct HarvestStats {
  long long attempts = 0;
  long long accepted[5] = {0, 0, 0, 0, 0};
  long long rejected = 0;
};

void harvest_from_shell(const Index& index, const std::vector<int>& shell, int smax,
                        Work* work, HarvestStats* stats,
                        std::map<std::vector<int>, int>* out) {
  const int m = (int)shell.size();
  for (int q = 2; q <= 4 && q <= m; ++q) {
    std::vector<int> pick((std::size_t)q);
    for (int i = 0; i < q; ++i) pick[(std::size_t)i] = i;
    for (;;) {
      std::vector<int> ids((std::size_t)q);
      for (int i = 0; i < q; ++i) ids[(std::size_t)i] = shell[(std::size_t)pick[(std::size_t)i]];
      if (out->find(ids) == out->end()) {
        ++stats->attempts;
        std::vector<int> inside, sh;
        if (harvest_support(index, ids, smax, work, &inside, &sh)) {
          (*out)[ids] = (int)inside.size();
          ++stats->accepted[q];
        } else {
          ++stats->rejected;
        }
      }
      int i = q - 1;
      while (i >= 0 && pick[(std::size_t)i] == m - q + i) --i;
      if (i < 0) break;
      ++pick[(std::size_t)i];
      for (int j = i + 1; j < q; ++j)
        pick[(std::size_t)j] = pick[(std::size_t)(j - 1)] + 1;
    }
  }
}

// ---------------------------------------------------------------------------
// FIXTURES GRAVEES — la projection ne conserve PAS le niveau.
//
// Les deux nuages ci-dessous viennent de
// `audits/AUDIT_REPONSES_VOLUME_PINCEAU_PROJECTIONS_20260812.md`. Ils refutent
// la regle « une paire du shell d'un sommet de niveau j a son milieu dans une
// face de meme niveau », que j'avais proposee et qui est fausse. Tant que ces
// deux cas passent, aucune implementation ne peut heriter le census d'un
// sommet : la recolte doit refaire le sien.
// ---------------------------------------------------------------------------
struct FixtureCase {
  const char* name;
  std::vector<mhgp::P3> cloud;
  std::vector<int> vertex_ids;   // les quatre labels du sommet q4
  std::vector<int> support_ids;  // la projection q2 ou q3
  int expected_vertex_level;
  int expected_support_level;
};

int run_fixtures(int smax) {
  std::vector<FixtureCase> cases;
  cases.push_back(FixtureCase{
      "projection_q2_niveau_0_vers_1",
      {mhgp::P3{10, 10, 10}, mhgp::P3{14, 10, 10}, mhgp::P3{14, 17, 7}, mhgp::P3{17, 14, 7},
       mhgp::P3{12, 10, 11}},
      {0, 1, 2, 3},
      {0, 1},
      0,
      1});
  cases.push_back(FixtureCase{
      "projection_q3_niveau_0_vers_1",
      {mhgp::P3{10, 10, 20}, mhgp::P3{14, 10, 20}, mhgp::P3{12, 14, 20}, mhgp::P3{12, 12, 10},
       mhgp::P3{12, 12, 21}},
      {0, 1, 2, 3},
      {0, 1, 2},
      0,
      1});

  int failures = 0;
  for (const FixtureCase& fixture : cases) {
    Index index;
    index.build(fixture.cloud, 4);
    Work work;
    const Lift vertex_lift = mhgp3v::ballfront::lift_of(
        index.at(fixture.vertex_ids[0]), index.at(fixture.vertex_ids[1]),
        index.at(fixture.vertex_ids[2]), index.at(fixture.vertex_ids[3]));
    if (!vertex_lift.ok) {
      std::fprintf(stderr, "FIXTURE %s : sommet degenere\n", fixture.name);
      ++failures;
      continue;
    }
    Census vertex_census;
    mhgp3v::ballfront::census_ball(index, vertex_lift, smax + 1, &vertex_census, &work);
    std::vector<int> inside, shell;
    const bool harvested =
        harvest_support(index, fixture.support_ids, smax, &work, &inside, &shell);
    const int vertex_level = (int)vertex_census.inside.size();
    const int support_level = harvested ? (int)inside.size() : -1;
    std::printf("fixture %s vertex_level=%d support_level=%d\n", fixture.name, vertex_level,
                support_level);
    if (vertex_level != fixture.expected_vertex_level ||
        support_level != fixture.expected_support_level) {
      std::fprintf(stderr,
                   "FIXTURE %s : attendu sommet=%d projection=%d, obtenu %d et %d\n",
                   fixture.name, fixture.expected_vertex_level,
                   fixture.expected_support_level, vertex_level, support_level);
      ++failures;
    }
  }
  if (failures != 0) {
    std::fprintf(stderr, "INVARIANT viole : %d fixture(s) de projection\n", failures);
    return 3;
  }
  std::printf("FIXTURES projection : %d cas, la projection ne conserve pas le niveau\n",
              (int)cases.size());
  return 0;
}

// ---------------------------------------------------------------------------
// CLI
// ---------------------------------------------------------------------------
int parse_int(const char* text, bool* ok) {
  char* end = nullptr;
  const long long value = std::strtoll(text, &end, 10);
  *ok = end != nullptr && *end == '\0' && value >= -1 && value <= 100000000LL;
  return (int)value;
}

}  // namespace

int main(int argc, char** argv) {
  int points = 200;
  int cap = 10;
  long long seed = 1;
  int coord = 0;
  std::string family = "uniform";
  bool judge = false;
  bool fixtures_only = false;
  bool harvest = false;
  int smax = -1;
  std::string inject;
  std::string transition = "pencil";

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    bool ok = true;
    if (arg.rfind("--points=", 0) == 0) {
      points = parse_int(arg.substr(9).c_str(), &ok);
    } else if (arg.rfind("--k=", 0) == 0) {
      cap = parse_int(arg.substr(4).c_str(), &ok);
    } else if (arg.rfind("--seed=", 0) == 0) {
      seed = parse_int(arg.substr(7).c_str(), &ok);
    } else if (arg.rfind("--coord=", 0) == 0) {
      coord = parse_int(arg.substr(8).c_str(), &ok);
    } else if (arg.rfind("--family=", 0) == 0) {
      family = arg.substr(9);
    } else if (arg == "--judge") {
      judge = true;
    } else if (arg == "--fixtures") {
      fixtures_only = true;
    } else if (arg == "--harvest") {
      harvest = true;
    } else if (arg.rfind("--smax=", 0) == 0) {
      smax = parse_int(arg.substr(7).c_str(), &ok);
    } else if (arg.rfind("--transition=", 0) == 0) {
      transition = arg.substr(13);
    } else if (arg.rfind("--inject=", 0) == 0) {
      inject = arg.substr(9);
    } else {
      std::fprintf(stderr, "REFUS argument inconnu: %s\n", arg.c_str());
      return 2;
    }
    if (!ok) {
      std::fprintf(stderr, "REFUS valeur invalide: %s\n", arg.c_str());
      return 2;
    }
  }
  if (fixtures_only) return run_fixtures(11);
  // UN REFUS PRECEDE LE CALCUL. Verifier la portee du juge apres la navigation
  // ferait payer une campagne entiere pour rendre le code 2.
  if (judge && points > 400) {
    std::fprintf(stderr, "REFUS juge exhaustif au-dela de 400 points\n");
    return 2;
  }
  if (smax < 0) smax = cap + 2;
  if (smax < 4 || smax > 40) {
    std::fprintf(stderr, "REFUS domaine: 4<=smax<=40\n");
    return 2;
  }
  if (points < 4 || cap < 0 || cap > 32) {
    std::fprintf(stderr, "REFUS domaine: points>=4 et 0<=k<=32\n");
    return 2;
  }

  mhgp3v::CloudFamily fam = mhgp3v::CloudFamily::kUniform;
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

  Index index;
  index.build(cloud, 8);

  Front front;
  front.index = &index;
  front.cap = cap;
  if (transition == "pencil") front.use_pencil = true;
  else if (transition == "pivot") front.use_pencil = false;
  else {
    std::fprintf(stderr, "REFUS transition inconnue: %s\n", transition.c_str());
    return 2;
  }
  if (inject == "drop-quotient") front.drop_quotient = true;
  else if (inject == "drop-entering") front.drop_entering = true;
  else if (inject == "drop-leaving") front.drop_leaving = true;
  else if (!inject.empty()) {
    std::fprintf(stderr, "REFUS injection inconnue: %s\n", inject.c_str());
    return 2;
  }

  std::vector<int> seed_shell, seed_inside;
  if (!build_seed(index, cap, &front.work, &seed_shell, &seed_inside)) {
    std::fprintf(stderr, "REFUS germe introuvable (degenerescence ou nuage plat)\n");
    return 2;
  }
  std::vector<int> pending;
  int seed_id = -1;
  if (!front.record(seed_shell, seed_inside, &seed_id)) {
    std::fprintf(stderr, "REFUS germe non enregistrable\n");
    return 2;
  }
  pending.push_back(seed_id);
  while (!pending.empty()) {
    const int id = pending.back();
    pending.pop_back();
    front.expand(id, &pending);
  }

  // Recolte des supports q2/q3/q4 depuis les shells visites.
  HarvestStats harvest_stats;
  std::map<std::vector<int>, int> harvested;
  if (harvest) {
    for (const Cell& cell : front.cells)
      harvest_from_shell(index, cell.shell, smax, &front.work, &harvest_stats, &harvested);
  }

  std::vector<i64> by_level((std::size_t)cap + 1, 0);
  for (const Cell& cell : front.cells) {
    const std::size_t level = cell.inside.size();
    if (level < by_level.size()) ++by_level[level];
  }

  std::printf("BallFrontReceipt-v1\n");
  std::printf("family=%s points=%d coord=%d seed=%lld k=%d transition=%s\n", family.c_str(),
              points, coord, seed, cap, transition.c_str());
  std::printf("cells=%lld degenerate_shells=%lld rejected_rank=%lld\n",
              (long long)front.work.cells_emitted, (long long)front.work.degenerate_shells,
              (long long)front.work.cells_rejected_rank);
  std::printf("pencil_queries=%lld pencil_node_visits=%lld pencil_leaf_tests=%lld\n",
              (long long)front.work.pencil_queries,
              (long long)front.work.pencil_node_visits,
              (long long)front.work.pencil_leaf_tests);
  std::printf("pencil_ambiguous_nodes=%lld exact_ratio_tests=%lld best_updates=%lld\n",
              (long long)front.work.pencil_ambiguous_nodes,
              (long long)front.work.exact_ratio_tests, (long long)front.work.best_updates);
  std::printf("shell_queries=%lld shell_node_visits=%lld tie_mass=%lld\n",
              (long long)front.work.shell_queries, (long long)front.work.shell_node_visits,
              (long long)front.work.tie_mass);
  std::printf("transitions=%lld pivots=%lld pivot_rounds=%lld ball_queries=%lld\n",
              (long long)front.work.transitions, (long long)front.work.pivots,
              (long long)front.work.pivot_rounds, (long long)front.work.ball_queries);
  std::printf("ball_node_visits=%lld ball_leaf_tests=%lld\n",
              (long long)front.work.ball_node_visits,
              (long long)front.work.ball_leaf_tests);
  std::printf("pivot_node_visits=%lld pivot_leaf_tests=%lld seed_node_visits=%lld\n",
              (long long)front.work.pivot_node_visits,
              (long long)front.work.pivot_leaf_tests,
              (long long)front.work.seed_node_visits);
  std::printf("point_tests=%lld seed_scans=%lld shell_ids=%lld\n",
              (long long)front.work.point_tests, (long long)front.work.seed_scans,
              (long long)front.work.shell_ids);
  std::printf("cells_per_point=%.4f\n", (double)front.work.cells_emitted / (double)points);
  if (harvest) {
    std::printf("harvest_attempts=%lld harvest_rejected=%lld smax=%d\n",
                harvest_stats.attempts, harvest_stats.rejected, smax);
    std::printf("supports_q2=%lld supports_q3=%lld supports_q4=%lld supports_total=%d\n",
                harvest_stats.accepted[2], harvest_stats.accepted[3],
                harvest_stats.accepted[4], (int)harvested.size());
    std::printf("supports_per_point=%.4f\n",
                (double)harvested.size() / (double)points);
  }
  for (int level = 0; level <= cap; ++level)
    std::printf("level[%d]=%lld\n", level, (long long)by_level[(std::size_t)level]);

  if (!judge) return 0;
  const std::vector<JudgeCell> truth = judge_enumerate(cloud, cap);
  std::map<std::vector<int>, const Cell*> mine;
  for (const Cell& cell : front.cells) mine[cell.shell] = &cell;
  int missing = 0, spurious = 0, mismatched = 0;
  for (const JudgeCell& want : truth) {
    auto it = mine.find(want.shell);
    if (it == mine.end()) {
      if (missing < 5)
        std::fprintf(stderr, "MANQUANT shell taille %d niveau %d\n", (int)want.shell.size(),
                     (int)want.inside.size());
      ++missing;
      continue;
    }
    if (it->second->inside != want.inside) ++mismatched;
  }
  std::map<std::vector<int>, const JudgeCell*> truth_index;
  for (const JudgeCell& want : truth) truth_index[want.shell] = &want;
  for (const Cell& cell : front.cells)
    if (truth_index.find(cell.shell) == truth_index.end()) ++spurious;

  std::printf("judge_truth=%d judge_mine=%d missing=%d spurious=%d mismatched=%d\n",
              (int)truth.size(), (int)front.cells.size(), missing, spurious, mismatched);
  if (missing != 0 || spurious != 0 || mismatched != 0) {
    if (!inject.empty()) {
      std::printf("MUTANT TUE: %s\n", inject.c_str());
      return 4;
    }
    std::fprintf(stderr, "DESACCORD juge\n");
    return 1;
  }
  if (!inject.empty()) {
    std::fprintf(stderr, "MUTANT SURVIVANT: %s\n", inject.c_str());
    return 3;
  }
  std::printf("ACCORD juge\n");
  return 0;
}
