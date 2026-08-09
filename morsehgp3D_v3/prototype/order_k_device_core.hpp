// MorseHGP3D v3 — NOYAU DEVICE-PORTABLE de la décision de parcours.
//
// `order_k_flats.hpp` décide correctement, mais rien de son chemin de décision
// n'est exécutable sur un GPU : `std::vector` partout, allocations dynamiques par
// sommet, `std::sort`, tailles non bornées. Ce fichier écrit le MÊME chemin de
// décision sous les contraintes d'un front d'onde device :
//
//   * aucune allocation, aucun conteneur de la bibliothèque standard ;
//   * capacité FIXE, déclarée, avec refus explicite au-delà ;
//   * annotations `MHGP_HD`, donc compilable par `nvcc` pour l'hôte et le device ;
//   * arithmétique exacte, les mêmes `orient3d` entiers que le chemin CPU.
//
// ---------------------------------------------------------------------------
// LE REFUS EST UNE PIÈCE DU CONTRAT, PAS UN ÉCHEC
// ---------------------------------------------------------------------------
//
// Une coquille cosphérique peut avoir $\Theta(n)$ points : AUCUNE capacité fixe
// n'est universellement suffisante, et prétendre le contraire serait faux. Le
// noyau déclare donc sa capacité et REFUSE ce qui la dépasse, avec un statut
// distinct. C'est exactement la discipline du projet `morsehgp3d`, dont toutes
// les requêtes device sont des PROPOSITIONS rejouées exactement sur CPU : ici
// aussi, un sommet refusé est renvoyé à l'hôte, qui le décide avec le chemin
// non borné. Le différentiel exige alors deux choses — que les sommets acceptés
// soient décidés à l'identique, et que les refus soient comptés, jamais
// silencieux.
//
// ---------------------------------------------------------------------------
// D'OÙ VIENNENT LES CAPACITÉS
// ---------------------------------------------------------------------------
//
// L'ensemble INTÉRIEUR est borné par le contrat : la coupe du parcours est le
// niveau strict $\ell\leq s_{\max}-2$, donc $|B(v)|\leq s_{\max}-2$. À
// $s_{\max}=11$ cela fait neuf, et `kMaxInterior = 16` couvre le contrat avec
// marge. La COQUILLE, elle, n'a pas de borne de contrat : quatre points en
// position générale, mais $n$ en cosphéricité totale. `kMaxShell = 32` est un
// choix de capacité, pas un théorème, et c'est pourquoi le dépassement est un
// refus explicite et compté.
#pragma once

#include "mhgp/mhgp.hpp"
#include "prototype/order_k_flats.hpp"

namespace mhgp3v {
namespace device {

using mhgp::P3;
using mhgp::i32;

inline constexpr int kMaxShell = 32;
inline constexpr int kMaxInterior = 16;
inline constexpr int kMaxClosure = kMaxShell;

// Statut d'admission d'un sommet dans le noyau borné. Un refus n'est pas une
// erreur : c'est une demande de rejeu hôte.
enum class Admission {
  kOk = 0,
  kShellOverflow,
  kInteriorOverflow,
  kShellTooSmall,
};

struct BoundedVertex {
  i32 shell[kMaxShell];
  i32 interior[kMaxInterior];
  int shell_size = 0;
  int interior_size = 0;
  int level = 0;
};

struct BoundedFlat {
  i32 base[3] = {-1, -1, -1};
  i32 closure[kMaxClosure];
  int closure_size = 0;
};

// Conversion depuis le sommet non borné. Elle ne modifie rien et ne tronque
// JAMAIS : au-delà de la capacité elle refuse.
inline Admission admit(const flats::Vertex& v, BoundedVertex* out) {
  if ((int)v.shell.size() > kMaxShell) return Admission::kShellOverflow;
  if ((int)v.interior.size() > kMaxInterior) return Admission::kInteriorOverflow;
  if (v.shell.size() < 3) return Admission::kShellTooSmall;
  out->shell_size = (int)v.shell.size();
  out->interior_size = (int)v.interior.size();
  out->level = v.level;
  for (int i = 0; i < out->shell_size; ++i) out->shell[i] = v.shell[(std::size_t)i];
  for (int i = 0; i < out->interior_size; ++i) out->interior[i] = v.interior[(std::size_t)i];
  return Admission::kOk;
}

// Recherche dans un tableau TRIÉ, sans `std::binary_search`.
MHGP_HD inline bool contains(const i32* sorted, int size, i32 z) {
  int lo = 0, hi = size - 1;
  while (lo <= hi) {
    const int mid = lo + ((hi - lo) >> 1);
    if (sorted[mid] == z) return true;
    if (sorted[mid] < z) lo = mid + 1;
    else hi = mid - 1;
  }
  return false;
}

// `orient3d` exact, identique au chemin CPU : c'est le MÊME entier, pas une
// reformulation. Les coordonnées vivent dans la grille déclarée u16, donc les
// produits restent sous $2^{53}$ et `i128` les porte largement.
MHGP_HD inline mhgp::i128 orient3d_bounded(const P3& a, const P3& b, const P3& c, const P3& d) {
  const mhgp::i128 ux = (mhgp::i128)b.x - a.x, uy = (mhgp::i128)b.y - a.y,
                   uz = (mhgp::i128)b.z - a.z;
  const mhgp::i128 vx = (mhgp::i128)c.x - a.x, vy = (mhgp::i128)c.y - a.y,
                   vz = (mhgp::i128)c.z - a.z;
  const mhgp::i128 wx = (mhgp::i128)d.x - a.x, wy = (mhgp::i128)d.y - a.y,
                   wz = (mhgp::i128)d.z - a.z;
  return ux * (vy * wz - vz * wy) - uy * (vx * wz - vz * wx) + uz * (vx * wy - vy * wx);
}

MHGP_HD inline int tangent_sign_of(mhgp::i128 orient_value, int direction) {
  const mhgp::i128 raw = -orient_value;
  const mhgp::i128 signed_value = direction > 0 ? raw : -raw;
  return signed_value < 0 ? -1 : (signed_value > 0 ? 1 : 0);
}

// L'admissibilité d'UN couple (base ordonnée, direction), à l'identique du chemin
// CPU : rester dans la chambre, puis faire croître L_h au niveau positif ou
// décroître Q_r au niveau zéro. La base ordonnée et la direction se transportent
// ENSEMBLE — une permutation impaire sans inversion changerait le signe.
MHGP_HD inline bool pair_admissible(const P3* points, const BoundedVertex& w, const i32 base[3],
                                    int direction, const i32* root_base, int root_size) {
  if (direction != -1 && direction != 1) return false;
  const P3& a = points[base[0]];
  const P3& b = points[base[1]];
  const P3& c = points[base[2]];
  for (int i = 0; i < w.shell_size; ++i)
    if (tangent_sign_of(orient3d_bounded(a, b, c, points[w.shell[i]]), direction) < 0) return false;
  if (w.interior_size > 0) {
    // h = min B(v) : l'ensemble intérieur est trié, donc c'est sa tête.
    const mhgp::i128 o = orient3d_bounded(a, b, c, points[w.interior[0]]);
    return tangent_sign_of(o, direction) > 0;
  }
  // Les VALEURS de `orient3d` se somment, pas leurs signes : la dérivée de Q_r
  // est une somme de formes affines.
  mhgp::i128 total = 0;
  for (int i = 0; i < root_size; ++i) total += orient3d_bounded(a, b, c, points[root_base[i]]);
  const mhgp::i128 derivative = (direction > 0) ? -total : total;
  return derivative < 0;
}

MHGP_HD inline bool backward_pair_admissible(const P3* points, const BoundedVertex& w,
                                             const i32 base[3], int forward, const i32* root_base,
                                             int root_size) {
  return pair_admissible(points, w, base, -forward, root_base, root_size);
}

// Comparaison lexicographique de deux fermetures, sans conteneur.
MHGP_HD inline int compare_closure(const i32* a, int na, const i32* b, int nb) {
  const int m = na < nb ? na : nb;
  for (int i = 0; i < m; ++i) {
    if (a[i] < b[i]) return -1;
    if (a[i] > b[i]) return 1;
  }
  if (na < nb) return -1;
  if (na > nb) return 1;
  return 0;
}

// Énumération des flats fermés de rang trois, à capacité bornée et sans
// allocation. Même quotient et MÊME ORDRE que le chemin CPU : les triplets de la
// coquille triée en ordre lexicographique, et seules les bases canoniques sont
// livrées. Rend `false` si une fermeture dépasse la capacité — refus, pas
// troncature.
template <class Fn>
MHGP_HD inline bool for_each_flat_from(const P3* points, const BoundedVertex& v, int i0, int j0,
                                       int k0, Fn&& visit) {
  const int m = v.shell_size;
  if (m < 3) return true;
  int i = i0, j = j0, k = k0;
  if (i < 0 || j <= i || k <= j || k >= m || j >= m - 1 || i >= m - 2) return true;
  BoundedFlat flat;
  for (;;) {
    bool deliver = false;
    const i32 ta = v.shell[i], tb = v.shell[j], tc = v.shell[k];
    do {
      const P3& a = points[ta];
      const P3& b = points[tb];
      const P3& c = points[tc];
      {
        const mhgp::P3 u = mhgp::p3_sub(b, a);
        const mhgp::P3 w = mhgp::p3_sub(c, a);
        const mhgp::P3 x = mhgp::p3_cross(u, w);
        if (x.x == 0 && x.y == 0 && x.z == 0) break;
      }
      flat.closure_size = 0;
      for (int t = 0; t < m; ++t) {
        const i32 z = v.shell[t];
        if (orient3d_bounded(a, b, c, points[z]) == 0) {
          if (flat.closure_size >= kMaxClosure) return false;   // refus, jamais troncature
          flat.closure[flat.closure_size++] = z;
        }
      }
      i32 canonical[3] = {-1, -1, -1};
      {
        bool found = false;
        const int q = flat.closure_size;
        for (int x = 0; x < q && !found; ++x)
        for (int y = x + 1; y < q && !found; ++y)
        for (int z = y + 1; z < q && !found; ++z) {
          const mhgp::P3 e1 = mhgp::p3_sub(points[flat.closure[y]], points[flat.closure[x]]);
          const mhgp::P3 e2 = mhgp::p3_sub(points[flat.closure[z]], points[flat.closure[x]]);
          const mhgp::P3 cr = mhgp::p3_cross(e1, e2);
          if (cr.x == 0 && cr.y == 0 && cr.z == 0) continue;
          canonical[0] = flat.closure[x];
          canonical[1] = flat.closure[y];
          canonical[2] = flat.closure[z];
          found = true;
        }
        if (!found) break;
      }
      if (!(canonical[0] == ta && canonical[1] == tb && canonical[2] == tc)) break;
      flat.base[0] = ta; flat.base[1] = tb; flat.base[2] = tc;
      deliver = true;
    } while (false);
    if (deliver && !visit(flat, i, j, k)) return true;
    if (++k >= m) {
      if (++j >= m - 1) { if (++i >= m - 2) return true; j = i + 1; }
      k = j + 1;
    }
  }
}

// La décision de filiation, sans aucune requête de voisin de retour : on énumère
// les couples du candidat JUSQU'À la clef de retour.
MHGP_HD inline flats::ChildOutcome decide_child(const P3* points, const BoundedVertex& w,
                                                const BoundedFlat& flat_at_v, int direction,
                                                const i32* root_base, int root_size,
                                                bool* capacity_ok) {
  *capacity_ok = true;
  if (direction != -1 && direction != 1) return flats::ChildOutcome::kBroken;
  const int back_slot = (-direction > 0) ? 1 : 0;
  flats::ChildOutcome verdict = flats::ChildOutcome::kBroken;
  i32 previous[kMaxClosure];
  int previous_size = 0;
  bool have_previous = false;
  const bool complete = for_each_flat_from(points, w, 0, 1, 2,
                                           [&](const BoundedFlat& g, int, int, int) {
    if (have_previous &&
        compare_closure(previous, previous_size, g.closure, g.closure_size) >= 0) return false;
    for (int i = 0; i < g.closure_size; ++i) previous[i] = g.closure[i];
    previous_size = g.closure_size;
    have_previous = true;
    const int cmp = compare_closure(g.closure, g.closure_size, flat_at_v.closure,
                                    flat_at_v.closure_size);
    if (cmp > 0) return false;
    const bool is_return = (cmp == 0);
    if (is_return && !(g.base[0] == flat_at_v.base[0] && g.base[1] == flat_at_v.base[1] &&
                       g.base[2] == flat_at_v.base[2]))
      return false;
    for (int slot = 0; slot < 2; ++slot) {
      const int dir = slot == 0 ? -1 : 1;
      const bool admissible = pair_admissible(points, w, g.base, dir, root_base, root_size);
      if (is_return && slot == back_slot) {
        verdict = admissible ? flats::ChildOutcome::kAccept : flats::ChildOutcome::kBroken;
        return false;
      }
      if (admissible) { verdict = flats::ChildOutcome::kReject; return false; }
    }
    return true;
  });
  if (!complete) { *capacity_ok = false; return flats::ChildOutcome::kBroken; }
  return verdict;
}

}  // namespace device
}  // namespace mhgp3v
