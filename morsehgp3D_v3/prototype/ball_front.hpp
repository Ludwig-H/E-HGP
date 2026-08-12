// MorseHGP3D v3 — FRONT DE BOULES VIDES : second temoin de la source directe.
//
// Objet enumere : les boules `B` dont le bord porte au moins quatre points non
// coplanaires — le shell ferme `U_B` — et dont l'interieur strict verifie
// `|I_B| <= K`. Ce sont les SOMMETS de l'arrangement des plans bissecteurs.
// `|I_B| <= K` n'est PAS le rang ferme : le rang ferme contractuel est
// `s(B)=|I_B|+|U_B|`, et une boule peut avoir `|I_B|=0` avec un shell de
// taille `Theta(n)`. Le rang ferme filtre une publication, jamais la
// navigation (theoreme de proprietaire, `AUDIT_VOIE_MULTIPLICITES_ORDER_K.md`).
//
// PORTEE. Ce fichier n'enumere que l'univers a shell d'au moins quatre points.
// Les supports q2 et q3 dont le shell a exactement deux ou trois elements ne
// sont pas des sommets de l'arrangement : ils vivent sur ses faces et ses
// aretes, et sont recoltes par projection auto-centree depuis les sommets.
// Tant que cette recolte n'est pas jugee, la branche correspondante est
// `incomplete`, jamais absente.
//
// STATUT. `prototype/order_k_flats.hpp` implemente la meme mathematique de
// facon plus complete — transitions par LOTS `S(w)=C(F) union A`, reverse
// search sans table, germe certifie, theoreme de proprietaire. Ce fichier ne
// le remplace pas : il en est un TEMOIN INDEPENDANT, ecrit avec une autre
// representation (forme liftee separable, elagage de boite exact) et d'autres
// transitions, plus une sonde de volume a l'echelle que le differentiel
// exhaustif ne peut pas produire.
//
// TRANSITIONS. Un triplet du shell reste sur la sphere et le centre glisse sur
// la droite perpendiculaire, dans ses deux sens. La variation de niveau n'est
// PAS supposee valoir un : le census de la boule atteinte est recalcule
// entierement, et l'auditeur a exhibe une transition ou le niveau passe de 1 a
// 3. Ce fichier ne suppose donc aucune monotonie; il ne suppose pas non plus la
// connexite, qu'il FALSIFIE par `--judge` contre un oracle exhaustif.
//
// ARITHMETIQUE. Aucun flottant n'est une autorite. Pour une origine `a` et
// trois vecteurs `p_i=P_i-a` de carres `s_i`, la forme liftee vaut
//   f(u) = D*|u|^2 - u_x*Mx + u_y*My - u_z*Mz,
// avec `D=det[p1;p2;p3]`. Le predicat est `power(y) = -sign(D)*f(y-a)`, positif
// strictement a l'interieur, nul sur la sphere. Sur `u16`, `|f|<2^90`; la
// variante triangle reste sous `2^104`. Tout tient en entiers signes 128 bits.
//
// La forme est SEPARABLE en `u_x,u_y,u_z` : sur une boite entiere, ses extrema
// se somment axe par axe, chaque axe etant une parabole `D*t^2+c*t` dont
// l'extremum entier est aux bords ou aux deux entiers voisins de `-c/(2D)`.
// C'est l'elagage exact du LBVH, sans enveloppe flottante.
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "morton_lbvh.hpp"

namespace mhgp3v {
namespace ballfront {

using i128 = __int128;
using i64 = long long;

// ---------------------------------------------------------------------------
// Predicats entiers
// ---------------------------------------------------------------------------

struct V3 {
  i64 x = 0, y = 0, z = 0;
};

inline V3 sub(const mhgp::P3& a, const mhgp::P3& b) {
  return V3{a.x - b.x, a.y - b.y, a.z - b.z};
}

inline i128 det3(const V3& p, const V3& q, const V3& r) {
  const i128 a = (i128)q.y * r.z - (i128)q.z * r.y;
  const i128 b = (i128)q.x * r.z - (i128)q.z * r.x;
  const i128 c = (i128)q.x * r.y - (i128)q.y * r.x;
  return (i128)p.x * a - (i128)p.y * b + (i128)p.z * c;
}

// bf_orient3(a,b,c,d) = det[b-a; c-a; d-a].
inline i128 bf_orient3(const mhgp::P3& a, const mhgp::P3& b, const mhgp::P3& c,
                     const mhgp::P3& d) {
  return det3(sub(b, a), sub(c, a), sub(d, a));
}

inline int sign_of(i128 v) { return v > 0 ? 1 : (v < 0 ? -1 : 0); }

// Forme liftee d'une sphere passant par quatre points, ramenee a l'origine `a`.
struct Lift {
  mhgp::P3 origin{};
  i128 dd = 0;  // D
  i128 cx = 0, cy = 0, cz = 0;
  int sgn = 0;  // signe de D ; power = -sgn * f
  bool ok = false;
};

// Mineur 3x3 des colonnes (i,j) et de la colonne des carres.
inline i128 minor_with_squares(const V3& p1, i128 s1, const V3& p2, i128 s2, const V3& p3,
                               i128 s3, int col_a, int col_b) {
  const i64 a1 = col_a == 0 ? p1.x : (col_a == 1 ? p1.y : p1.z);
  const i64 a2 = col_a == 0 ? p2.x : (col_a == 1 ? p2.y : p2.z);
  const i64 a3 = col_a == 0 ? p3.x : (col_a == 1 ? p3.y : p3.z);
  const i64 b1 = col_b == 0 ? p1.x : (col_b == 1 ? p1.y : p1.z);
  const i64 b2 = col_b == 0 ? p2.x : (col_b == 1 ? p2.y : p2.z);
  const i64 b3 = col_b == 0 ? p3.x : (col_b == 1 ? p3.y : p3.z);
  // det [[a1,b1,s1],[a2,b2,s2],[a3,b3,s3]]
  return (i128)a1 * ((i128)b2 * s3 - (i128)b3 * s2) -
         (i128)b1 * ((i128)a2 * s3 - (i128)a3 * s2) +
         s1 * ((i128)a2 * b3 - (i128)a3 * b2);
}

inline i128 norm2(const V3& v) {
  return (i128)v.x * v.x + (i128)v.y * v.y + (i128)v.z * v.z;
}

inline Lift lift_of(const mhgp::P3& a, const mhgp::P3& b, const mhgp::P3& c,
                    const mhgp::P3& d) {
  Lift out;
  out.origin = a;
  const V3 p1 = sub(b, a), p2 = sub(c, a), p3 = sub(d, a);
  const i128 s1 = norm2(p1), s2 = norm2(p2), s3 = norm2(p3);
  out.dd = det3(p1, p2, p3);
  out.sgn = sign_of(out.dd);
  if (out.sgn == 0) return out;
  const i128 mx = minor_with_squares(p1, s1, p2, s2, p3, s3, 1, 2);  // (y,z,s)
  const i128 my = minor_with_squares(p1, s1, p2, s2, p3, s3, 0, 2);  // (x,z,s)
  const i128 mz = minor_with_squares(p1, s1, p2, s2, p3, s3, 0, 1);  // (x,y,s)
  out.cx = -mx;
  out.cy = my;
  out.cz = -mz;
  out.ok = true;
  return out;
}

// Miniboule d'une PAIRE : sphere de diametre [a,b]. Avec `u=y-a` et
// `e=b-a`, l'equation est `|u|^2 - e.u = 0`, donc `D=1`.
inline Lift lift_pair(const mhgp::P3& a, const mhgp::P3& b) {
  Lift out;
  out.origin = a;
  const V3 e = sub(b, a);
  if (e.x == 0 && e.y == 0 && e.z == 0) return out;  // colocalises : hors domaine
  out.dd = 1;
  out.sgn = 1;
  out.cx = -(i128)e.x;
  out.cy = -(i128)e.y;
  out.cz = -(i128)e.z;
  out.ok = true;
  return out;
}

// Miniboule d'un TRIANGLE : sphere passant par `a,b,c` dont le centre est dans
// leur plan affine. En posant `p1=b-a`, `p2=c-a` et
// `G=|p1|^2|p2|^2-(p1.p2)^2>0`, le vecteur `G*c` est entier et
//   f_G(u) = G|u|^2 + (G c).u
// reste sous `2^104` sur le profil u16. Le centre vaut `a - (G c)/(2G)`.
struct TriangleLift {
  Lift lift;
  i128 galpha = 0, gbeta = 0, gg = 0;  // centre = a + (galpha*p1 + gbeta*p2)/gg
};

inline TriangleLift lift_triangle(const mhgp::P3& a, const mhgp::P3& b,
                                  const mhgp::P3& c) {
  TriangleLift out;
  const V3 p1 = sub(b, a), p2 = sub(c, a);
  const i128 n11 = norm2(p1), n22 = norm2(p2);
  const i128 n12 = (i128)p1.x * p2.x + (i128)p1.y * p2.y + (i128)p1.z * p2.z;
  const i128 gg = n11 * n22 - n12 * n12;
  if (gg <= 0) return out;  // colineaires
  // lambda et mu resolvent le systeme 2x2 ; on garde les numerateurs sur `gg`.
  const i128 glambda = -n11 * n22 + n22 * n12;
  const i128 gmu = -n22 * n11 + n11 * n12;
  Lift lift;
  lift.origin = a;
  lift.dd = gg;
  lift.sgn = 1;
  lift.cx = glambda * p1.x + gmu * p2.x;
  lift.cy = glambda * p1.y + gmu * p2.y;
  lift.cz = glambda * p1.z + gmu * p2.z;
  lift.ok = true;
  out.lift = lift;
  // centre = a - (lambda p1 + mu p2)/2, donc les coefficients barycentriques
  // relatifs a (b,c) valent -glambda/(2gg) et -gmu/(2gg).
  out.galpha = -glambda;
  out.gbeta = -gmu;
  out.gg = 2 * gg;
  return out;
}

// power > 0 : strictement interieur ; == 0 : sur la sphere ; < 0 : exterieur.
inline i128 power_of(const Lift& lift, const mhgp::P3& y) {
  const V3 u = sub(y, lift.origin);
  const i128 f = lift.dd * norm2(u) + lift.cx * u.x + lift.cy * u.y + lift.cz * u.z;
  return lift.sgn > 0 ? -f : f;
}

// ---------------------------------------------------------------------------
// Extrema exacts de la forme separable sur une boite entiere
// ---------------------------------------------------------------------------

inline i128 floor_div(i128 a, i128 b) {
  i128 q = a / b;
  if ((a % b != 0) && ((a < 0) != (b < 0))) --q;
  return q;
}

// Maximum entier de D*t^2 + c*t sur [lo,hi].
inline i128 axis_max(i128 dd, i128 c, i64 lo, i64 hi) {
  const i128 at_lo = dd * (i128)lo * lo + c * lo;
  const i128 at_hi = dd * (i128)hi * hi + c * hi;
  i128 best = at_lo > at_hi ? at_lo : at_hi;
  if (dd < 0) {  // concave : sommet interieur candidat
    const i128 t0 = floor_div(-c, 2 * dd);
    for (int k = 0; k < 2; ++k) {
      const i128 t = t0 + k;
      if (t < lo || t > hi) continue;
      const i128 v = dd * t * t + c * t;
      if (v > best) best = v;
    }
  }
  return best;
}

inline i128 axis_min(i128 dd, i128 c, i64 lo, i64 hi) {
  return -axis_max(-dd, -c, lo, hi);
}

// Minorant exact de `power` sur une boite entiere.
inline i128 power_lower_bound(const Lift& lift, const i64 lo[3], const i64 hi[3]);

// Majorant exact de `power` sur une boite entiere en coordonnees absolues.
inline i128 power_upper_bound(const Lift& lift, const i64 lo[3], const i64 hi[3]) {
  const i64 olo[3] = {lo[0] - lift.origin.x, lo[1] - lift.origin.y, lo[2] - lift.origin.z};
  const i64 ohi[3] = {hi[0] - lift.origin.x, hi[1] - lift.origin.y, hi[2] - lift.origin.z};
  const i128 coef[3] = {lift.cx, lift.cy, lift.cz};
  i128 acc = 0;
  if (lift.sgn > 0) {  // power = -f : maximiser -f revient a minimiser f
    for (int d = 0; d < 3; ++d) acc += axis_min(lift.dd, coef[d], olo[d], ohi[d]);
    return -acc;
  }
  for (int d = 0; d < 3; ++d) acc += axis_max(lift.dd, coef[d], olo[d], ohi[d]);
  return acc;
}

inline i128 power_lower_bound(const Lift& lift, const i64 lo[3], const i64 hi[3]) {
  const i64 olo[3] = {lo[0] - lift.origin.x, lo[1] - lift.origin.y, lo[2] - lift.origin.z};
  const i64 ohi[3] = {hi[0] - lift.origin.x, hi[1] - lift.origin.y, hi[2] - lift.origin.z};
  const i128 coef[3] = {lift.cx, lift.cy, lift.cz};
  i128 acc = 0;
  if (lift.sgn > 0) {  // power = -f : minimiser -f revient a maximiser f
    for (int d = 0; d < 3; ++d) acc += axis_max(lift.dd, coef[d], olo[d], ohi[d]);
    return -acc;
  }
  for (int d = 0; d < 3; ++d) acc += axis_min(lift.dd, coef[d], olo[d], ohi[d]);
  return acc;
}

// Distance carree minimale exacte d'un point entier a une boite entiere.
inline i128 box_distance2(const mhgp::P3& q, const i64 lo[3], const i64 hi[3]) {
  const i64 c[3] = {q.x, q.y, q.z};
  i128 acc = 0;
  for (int d = 0; d < 3; ++d) {
    const i64 gap = c[d] < lo[d] ? lo[d] - c[d] : (c[d] > hi[d] ? c[d] - hi[d] : 0);
    acc += (i128)gap * gap;
  }
  return acc;
}

// ---------------------------------------------------------------------------
// Compteurs de travail
// ---------------------------------------------------------------------------

struct Work {
  i64 pivots = 0;              // pivots demarres
  i64 pivot_rounds = 0;        // iterations de raffinement
  i64 ball_queries = 0;        // requetes de census de boule
  i64 ball_node_visits = 0;    // noeuds LBVH visites par un census
  i64 ball_leaf_tests = 0;     // points testes en feuille par un census
  i64 pivot_node_visits = 0;   // noeuds LBVH visites par un pivot
  i64 pivot_leaf_tests = 0;    // points testes en feuille par un pivot
  i64 seed_node_visits = 0;    // noeuds LBVH visites par une recherche de germe
  i64 shell_ids = 0;           // somme des tailles de shell publiees
  i64 point_tests = 0;         // evaluations de `power`
  i64 cells_emitted = 0;       // cellules distinctes emises
  i64 cells_rejected_rank = 0; // cellules hors fenetre `|I|<=K`
  i64 transitions = 0;         // aretes de l'arrangement suivies
  i64 seed_scans = 0;          // points balayes pour les germes
  i64 degenerate_shells = 0;   // shells de plus de quatre points
  // Ledger separe de la requete de pinceau, exige par l'audit Q4.
  i64 pencil_queries = 0;
  i64 pencil_node_visits = 0;
  i64 pencil_leaf_tests = 0;
  i64 pencil_ambiguous_nodes = 0;  // noeuds non elagues par le desaccord
  i64 exact_ratio_tests = 0;       // comparaisons exactes de deux parametres
  i64 best_updates = 0;
  i64 tie_mass = 0;                // somme des tailles de lot
  i64 shell_queries = 0;
  i64 shell_node_visits = 0;
};

// ---------------------------------------------------------------------------
// Index resident
// ---------------------------------------------------------------------------

struct Index {
  const std::vector<mhgp::P3>* cloud = nullptr;
  MortonLbvh tree;

  void build(const std::vector<mhgp::P3>& points, int leaf_size = 8) {
    cloud = &points;
    tree.build(points, leaf_size);
  }

  const mhgp::P3& at(int id) const { return (*cloud)[(std::size_t)id]; }
  int size() const { return (int)cloud->size(); }
};

// Resultat d'un census ferme sur une boule.
struct Census {
  std::vector<int> inside;  // strictement interieurs, trie
  std::vector<int> shell;   // exactement sur la sphere, trie
  bool overflow = false;    // plus de `cap` interieurs : hors fenetre
};

// Enumere `inside` et `shell` par descente LBVH avec elagage exact.
// `cap` borne le nombre d'interieurs collectes : au-dela, la boule est hors
// fenetre et la descente s'arrete (`overflow`).
inline void census_ball(const Index& index, const Lift& lift, int cap, Census* out,
                        Work* work) {
  out->inside.clear();
  out->shell.clear();
  out->overflow = false;
  if (!lift.ok) return;
  ++work->ball_queries;
  std::vector<int> stack;
  stack.push_back(0);
  while (!stack.empty()) {
    const int node_id = stack.back();
    stack.pop_back();
    const LbvhNode& node = index.tree.nodes[(std::size_t)node_id];
    ++work->ball_node_visits;
    const i128 upper = power_upper_bound(lift, node.lo, node.hi);
    if (upper < 0) continue;  // toute la boite est strictement exterieure
    if (node.left < 0) {
      for (int t = node.begin; t < node.end; ++t) {
        const int id = index.tree.order[(std::size_t)t];
        ++work->ball_leaf_tests;
        ++work->point_tests;
        const i128 p = power_of(lift, index.at(id));
        if (p > 0) {
          if ((int)out->inside.size() >= cap) {
            out->overflow = true;
            return;
          }
          out->inside.push_back(id);
        } else if (p == 0) {
          out->shell.push_back(id);
        }
      }
      continue;
    }
    stack.push_back(node.left);
    stack.push_back(node.right);
  }
  std::sort(out->inside.begin(), out->inside.end());
  std::sort(out->shell.begin(), out->shell.end());
}

// ---------------------------------------------------------------------------
// Pivot exact
// ---------------------------------------------------------------------------

// Cherche le point `y` du cote strictement positif de la facette orientee
// `(a,b,c)` qui minimise le parametre du pinceau, en excluant `banned`.
// Retourne -1 si le cote est vide de tout point admissible.
//
// Invariant : a chaque tour, la sphere de `(a,b,c,y)` est testee; tout point
// admissible strictement interieur du bon cote donne un `y` strictement
// meilleur. La suite des parametres est strictement decroissante, donc finie.
struct PivotResult {
  int apex = -1;
  Lift lift;
  Census census;
  bool ok = false;
};

inline bool is_banned(const std::vector<int>& banned, int id) {
  return std::find(banned.begin(), banned.end(), id) != banned.end();
}

// Distance carree minimale exacte d'un point entier a une boite entiere.

// Germe du pivot : point admissible du cote strictement positif le plus proche
// de `a`, par descente LBVH bornee. Ce n'est qu'un point de depart : sa qualite
// change le nombre de tours, jamais le point fixe atteint.
inline int nearest_positive(const Index& index, int fa, int fb, int fc,
                            const std::vector<int>& banned, Work* work) {
  const mhgp::P3& a = index.at(fa);
  const mhgp::P3& b = index.at(fb);
  const mhgp::P3& c = index.at(fc);
  int best = -1;
  i128 best_d2 = 0;
  std::vector<int> stack;
  stack.push_back(0);
  while (!stack.empty()) {
    const int node_id = stack.back();
    stack.pop_back();
    const LbvhNode& node = index.tree.nodes[(std::size_t)node_id];
    ++work->seed_node_visits;
    if (best >= 0 && box_distance2(a, node.lo, node.hi) > best_d2) continue;
    if (node.left < 0) {
      for (int t = node.begin; t < node.end; ++t) {
        const int id = index.tree.order[(std::size_t)t];
        ++work->pivot_leaf_tests;
        if (id == fa || id == fb || id == fc) continue;
        if (is_banned(banned, id)) continue;
        const V3 delta = sub(index.at(id), a);
        const i128 d2 = norm2(delta);
        if (best >= 0 && d2 >= best_d2) continue;
        if (bf_orient3(a, b, c, index.at(id)) <= 0) continue;
        best = id;
        best_d2 = d2;
      }
      continue;
    }
    // Le fils le plus proche est empile en dernier : il est depile en premier et
    // fixe tot un `best_d2` qui elague le frere. Sans cet ordre, la premiere
    // descente ne coupe rien et le parcours degenere en balayage.
    const LbvhNode& left = index.tree.nodes[(std::size_t)node.left];
    const LbvhNode& right = index.tree.nodes[(std::size_t)node.right];
    if (box_distance2(a, left.lo, left.hi) <= box_distance2(a, right.lo, right.hi)) {
      stack.push_back(node.right);
      stack.push_back(node.left);
    } else {
      stack.push_back(node.left);
      stack.push_back(node.right);
    }
  }
  ++work->seed_scans;
  return best;
}

// Cherche un point admissible STRICTEMENT interieur a la sphere courante et du
// cote positif : son existence prouve que le pivot courant n'est pas le point
// fixe. La descente s'arrete au premier temoin, sans cap : un cap ici ferait
// conclure a tort la convergence.
inline int improving_witness(const Index& index, const Lift& lift, int fa, int fb, int fc,
                             const std::vector<int>& banned, Work* work) {
  const mhgp::P3& a = index.at(fa);
  const mhgp::P3& b = index.at(fb);
  const mhgp::P3& c = index.at(fc);
  std::vector<int> stack;
  stack.push_back(0);
  while (!stack.empty()) {
    const int node_id = stack.back();
    stack.pop_back();
    const LbvhNode& node = index.tree.nodes[(std::size_t)node_id];
    ++work->pivot_node_visits;
    if (power_upper_bound(lift, node.lo, node.hi) <= 0) continue;
    if (node.left < 0) {
      for (int t = node.begin; t < node.end; ++t) {
        const int id = index.tree.order[(std::size_t)t];
        ++work->pivot_leaf_tests;
        if (id == fa || id == fb || id == fc) continue;
        if (is_banned(banned, id)) continue;
        ++work->point_tests;
        if (power_of(lift, index.at(id)) <= 0) continue;
        if (bf_orient3(a, b, c, index.at(id)) <= 0) continue;
        return id;
      }
      continue;
    }
    stack.push_back(node.left);
    stack.push_back(node.right);
  }
  return -1;
}

// ---------------------------------------------------------------------------
// TRANSITION PAR LOTS : le croisement CONSECUTIF du pinceau
// ---------------------------------------------------------------------------
//
// Le voisin d'arrangement n'est pas `argmin beta` : c'est le premier croisement
// du pinceau a partir du parametre courant, dans un sens donne, tous ex aequo
// groupes. Pour un triplet `(a,b,c)` du shell, posons
//   lambda(y) = orient3(a,b,c,y)      (affine, nulle sur le plan du flat)
//   p_v(y)    = power de la sphere du sommet courant  (nulle sur son shell)
// Les spheres du pinceau sont `p_t = p_v + t*lambda` : elles passent toutes par
// le cercle du flat. Un point `y` est sur la sphere de parametre
//   t_y = -p_v(y)/lambda(y).
//
// TROIS CONSEQUENCES, ET AUCUNE N'A BESOIN DE 256 BITS.
//
// 1. Comparer deux parametres ne demande aucun quotient. On a
//    sign(t_1 - t_2) = sign(p_{t_1}(y_2)) * sign(lambda(y_2)),
//    donc il suffit d'evaluer la sphere `(a,b,c,y_1)` en `y_2` : un predicat de
//    sphere ordinaire, borne par `2^90` sur u16. C'est exactement le
//    `Pencil::compare_t` du sujet `order_k_flats.hpp`.
//
// 2. L'ELAGAGE est le lemme du pinceau applique a une BOITE, et non a un point.
//    La puissance etant AFFINE en `t`, un point possede un evenement dans
//    l'intervalle ouvert entre le parametre courant et le meilleur parametre
//    connu si et seulement si il change de signe entre les DEUX spheres
//    terminales. Une boite entiere ne peut donc contenir aucun candidat des que
//    tous ses points sont du meme cote des deux spheres, ce qui se decide par
//    les extrema exacts des deux formes separables. Aucune enveloppe flottante,
//    aucune amorce par doublement de pave, aucun balayage de grille.
//
// 3. Le census DISPARAIT. L'interieur se transporte :
//      S(w) = C(F) union A,
//      B(w) = (B(v) union D_dir) prive de A,
//    ou `C(F)` est la fermeture du flat — les points du shell courant dans le
//    plan de `(a,b,c)` —, `D_dir` les points du shell qui basculent a
//    l'interieur dans le sens choisi, et `A` le lot du croisement. Le niveau
//    peut donc varier de plus d'une unite, comme l'audit l'a exhibe.

struct PencilStep {
  bool ok = false;
  bool over_level = false;   // |B(w)| depasse la coupe de navigation
  Lift lift;                 // sphere du sommet atteint
  std::vector<int> shell;    // S(w), trie
  std::vector<int> interior; // B(w), trie
};

// Collecte exacte du shell d'une sphere : tous les points de puissance nulle.
inline void collect_shell(const Index& index, const Lift& lift, std::vector<int>* out,
                          Work* work) {
  out->clear();
  ++work->shell_queries;
  std::vector<int> stack;
  stack.push_back(0);
  while (!stack.empty()) {
    const int node_id = stack.back();
    stack.pop_back();
    const LbvhNode& node = index.tree.nodes[(std::size_t)node_id];
    ++work->shell_node_visits;
    if (power_lower_bound(lift, node.lo, node.hi) > 0) continue;
    if (power_upper_bound(lift, node.lo, node.hi) < 0) continue;
    if (node.left < 0) {
      for (int t = node.begin; t < node.end; ++t) {
        const int id = index.tree.order[(std::size_t)t];
        if (power_of(lift, index.at(id)) == 0) out->push_back(id);
      }
      continue;
    }
    stack.push_back(node.left);
    stack.push_back(node.right);
  }
  std::sort(out->begin(), out->end());
}

inline PencilStep pencil_next(const Index& index, const Lift& current,
                              const std::vector<int>& shell_v,
                              const std::vector<int>& interior_v, int fa, int fb, int fc,
                              int dir, int cap, Work* work) {
  PencilStep out;
  ++work->pencil_queries;
  const mhgp::P3& a = index.at(fa);
  const mhgp::P3& b = index.at(fb);
  const mhgp::P3& c = index.at(fc);

  int best = -1;
  Lift best_lift;
  // Point de reference HEURISTIQUE : le barycentre entier du flat. Il ne sert
  // qu'a ordonner la descente pour fixer tot un incumbent — sans lui, la
  // premiere descente n'elague rien et degenere en balayage. L'arrondi n'a
  // aucune autorite : il ne change ni le resultat, ni les predicats.
  const mhgp::P3 pivot_hint{(a.x + b.x + c.x) / 3, (a.y + b.y + c.y) / 3,
                            (a.z + b.z + c.z) / 3};
  std::vector<int> stack;
  stack.push_back(0);
  while (!stack.empty()) {
    const int node_id = stack.back();
    stack.pop_back();
    const LbvhNode& node = index.tree.nodes[(std::size_t)node_id];
    ++work->pencil_node_visits;
    if (best >= 0) {
      // Desaccord ternaire entre les deux spheres terminales.
      const i128 lo_v = power_lower_bound(current, node.lo, node.hi);
      const i128 lo_b = power_lower_bound(best_lift, node.lo, node.hi);
      if (lo_v > 0 && lo_b > 0) continue;
      const i128 hi_v = power_upper_bound(current, node.lo, node.hi);
      const i128 hi_b = power_upper_bound(best_lift, node.lo, node.hi);
      if (hi_v < 0 && hi_b < 0) continue;
      ++work->pencil_ambiguous_nodes;
    }
    if (node.left < 0) {
      for (int t = node.begin; t < node.end; ++t) {
        const int id = index.tree.order[(std::size_t)t];
        ++work->pencil_leaf_tests;
        const i128 pv = power_of(current, index.at(id));
        if (pv == 0) continue;  // deja sur le shell courant : parametre nul
        const i128 lam = bf_orient3(a, b, c, index.at(id));
        if (lam == 0) continue;  // dans le plan du flat : jamais atteint
        // `t_y` a le signe de `dir` si et seulement si `p_v * lambda * dir < 0`.
        const int sgn_pv = sign_of(pv), sgn_lam = sign_of(lam);
        if (sgn_pv * sgn_lam * dir >= 0) continue;
        if (best < 0) {
          const Lift candidate = lift_of(a, b, c, index.at(id));
          if (!candidate.ok) continue;
          best = id;
          best_lift = candidate;
          ++work->best_updates;
          continue;
        }
        // `id` est-il AVANT `best` dans le sens `dir` ? Avec
        // `sign(t_best - t_id) = sign(p_best(id)) * sign(lambda(id))`, la
        // condition `dir*(t_id - t_best) < 0` s'ecrit
        // `dir * sign(p_best(id)) * sign(lambda(id)) > 0`.
        ++work->exact_ratio_tests;
        const i128 pb = power_of(best_lift, index.at(id));
        if (pb == 0) continue;  // ex aequo : le lot est collecte a la fin
        if (dir * sign_of(pb) * sgn_lam <= 0) continue;
        const Lift candidate = lift_of(a, b, c, index.at(id));
        if (!candidate.ok) continue;
        best = id;
        best_lift = candidate;
        ++work->best_updates;
      }
      continue;
    }
    const LbvhNode& left = index.tree.nodes[(std::size_t)node.left];
    const LbvhNode& right = index.tree.nodes[(std::size_t)node.right];
    if (box_distance2(pivot_hint, left.lo, left.hi) <=
        box_distance2(pivot_hint, right.lo, right.hi)) {
      stack.push_back(node.right);
      stack.push_back(node.left);
    } else {
      stack.push_back(node.left);
      stack.push_back(node.right);
    }
  }
  if (best < 0) return out;  // aucun croisement de ce cote : arete non bornee

  // `S(w)` est exactement le shell ferme de la sphere atteinte.
  collect_shell(index, best_lift, &out.shell, work);
  if (out.shell.size() < 4) return out;
  work->tie_mass += (i64)out.shell.size();

  // `B(w) = (B(v) union D_dir) prive de S(w)`.
  std::vector<int> interior = interior_v;
  for (int p : shell_v) {
    const i128 lam = bf_orient3(a, b, c, index.at(p));
    if (sign_of(lam) == dir) interior.push_back(p);
  }
  std::sort(interior.begin(), interior.end());
  interior.erase(std::unique(interior.begin(), interior.end()), interior.end());
  std::vector<int> kept;
  kept.reserve(interior.size());
  for (int p : interior)
    if (!std::binary_search(out.shell.begin(), out.shell.end(), p)) kept.push_back(p);
  out.interior = kept;
  out.lift = best_lift;
  out.over_level = (int)kept.size() > cap;
  out.ok = true;
  return out;
}

inline PivotResult pivot(const Index& index, int fa, int fb, int fc,
                         const std::vector<int>& banned, int cap, Work* work) {
  PivotResult out;
  ++work->pivots;
  int apex = nearest_positive(index, fa, fb, fc, banned, work);
  if (apex < 0) return out;  // cote positif vide : facette du bord de l'enveloppe

  for (;;) {
    ++work->pivot_rounds;
    const Lift lift =
        lift_of(index.at(fa), index.at(fb), index.at(fc), index.at(apex));
    if (!lift.ok) return out;  // quatre points coplanaires : facette degeneree
    const int better = improving_witness(index, lift, fa, fb, fc, banned, work);
    if (better < 0) {
      out.apex = apex;
      out.lift = lift;
      census_ball(index, lift, cap, &out.census, work);
      out.ok = true;
      return out;
    }
    apex = better;
  }
}

}  // namespace ballfront
}  // namespace mhgp3v
