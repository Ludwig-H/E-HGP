// MorseHGP3D v3 — navigation MULTIPLICITAIRE dans l'arrangement releve.
//
// Ce fichier remplace `order_k_bfs.hpp`. Il n'en est pas une variante : les
// trois enonces sur lesquels reposait le precedent sont faux hors position
// simple, et chacun a ete reproduit ici contre une verite exhaustive avant
// d'etre corrige.
//
// ---------------------------------------------------------------------------
// 1. Les quatre grandeurs qu'il ne faut jamais confondre
// ---------------------------------------------------------------------------
//
// Pour x = (c, t) dans R^4 et le relevement phi(p) = (p, |p|^2), posons
//
//     L_i(x) = t - 2 c . p_i + |p_i|^2.
//
// « p_i est strictement interieur a la sphere x » s'ecrit L_i(x) < 0. A un
// sommet v de l'arrangement des n hyperplans {L_i = 0} :
//
//     B(v) = { i : L_i(v) < 0 }        ensemble interieur
//     S(v) = { i : L_i(v) = 0 }        COQUILLE complete
//     l(v) = |B(v)|                    NIVEAU strict
//     rho(v) = l(v) + |S(v)|           RANG FERME
//
// Le rang ferme est un FILTRE DE PUBLICATION. Ce n'est ni un niveau de graphe,
// ni un potentiel de parcours : couper le parcours sur rho retire des sommets
// de niveau zero indispensables. Le cube cospherique le montre en une ligne —
// coquille huit, niveau zero, rang ferme huit — et l'ancien parcours le coupait
// avant meme de naviguer, perdant les douze boules diametrales de ses aretes.
//
// ---------------------------------------------------------------------------
// 2. Le vrai 1-squelette : des FLATS FERMES DE RANG TROIS, pas des triplets
// ---------------------------------------------------------------------------
//
// Une arete de l'arrangement est une droite F de R^4, intersection de trois
// hyperplans independants. Sa fermeture est C(F) = { i : F inclus dans H_i }.
// Dans la geometrie des points : toutes les spheres de F passent par un meme
// CERCLE, et C(F) est l'ensemble des points de la coquille situes sur ce
// cercle — c'est-a-dire, puisque tous sont deja sur la sphere de v, les points
// de S(v) situes dans un meme PLAN.
//
// Les aretes incidentes a v sont donc en bijection avec les PLANS distincts
// engendres par au moins trois points non alignes de S(v), et non avec les
// C(m,3) triplets. Deux triplets de meme fermeture decrivent la meme arete.
//
// La transition exacte le long de F, du sommet v vers le sommet w, est
//
//     S(w) = C(F) union A,
//
// ou A est le LOT des points atteignant simultanement le prochain parametre.
// Cette formule conserve tous les membres constants du cercle et retire tous
// les anciens membres qui ne contiennent pas la droite. Le transport du niveau
// se fait par lots, sans jamais supposer qu'un seul point change d'etat :
//
//     D_-(d) = { i dans S(v) \ C(F) : i strictement interieur sur l'arete }
//     B_e    = B(v) union D_-(d)                       (arete ouverte)
//     B(w)   = B_e \ { i dans A : i etait interieur sur l'arete }.
//
// ---------------------------------------------------------------------------
// 3. Le plafond de navigation est le NIVEAU STRICT s_max - 2
// ---------------------------------------------------------------------------
//
// THEOREME DE PROPRIETAIRE (audit `AUDIT_VOIE_MULTIPLICITES_ORDER_K.md`, §6).
// Si le nuage est de dimension affine trois et si U est un support de q points
// affinement independants, 1 <= q <= 4, alors il existe un sommet
// d'arrangement o(U) contenant U tel que B(o(U)) soit inclus dans l'ensemble
// interieur de la sphere minimale de U. En particulier l(o(U)) <= d_U.
//
// Si la sphere de U est de rang ferme au plus s_max, sa coquille contient au
// moins les q points de U, donc d_U <= s_max - q <= s_max - 2. Naviguer selon
//
//     l(v) <= k_nav,   k_nav = s_max - 2,
//
// et recolter les sous-ensembles des coquilles visitees suffit donc a publier
// TOUT le catalogue. Un proprietaire peut porter une coquille de taille huit,
// cinquante ou davantage : il doit etre traverse quel que soit son rang ferme.
//
// ---------------------------------------------------------------------------
// 4. Connexite du sous-graphe { l <= k } — acquise, et pas par ce fichier
// ---------------------------------------------------------------------------
//
// L'ancien fichier declarait explicitement ne pas demontrer cette connexite et
// s'en remettait a l'oracle nuage par nuage. Elle est DEMONTREE dans
// `audits/AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md`, pour un arrangement fini
// d'hyperplans non verticaux possedant au moins un sommet — ni le relevement
// parabolique, ni la simplicite ne sont requis. Le squelette de la preuve :
//
// (a) DESCENTE. Pour l(v) > 0, le polyedre de chambre
//     P_B = { x : L_i(x) <= 0 sur B(v), L_j(x) >= 0 ailleurs } est de dimension
//     quatre (le deplacement vertical infinitesimal au-dessus de v satisfait
//     tout strictement), v en est un sommet donc P_B est pointe, et au moins
//     une contrainte de B porte une facette. Une face non vide d'un polyedre
//     pointe contient un sommet : on obtient w avec l(w) <= l(v) - 1, par un
//     chemin dont les sommets ET LES ARETES OUVERTES restent de niveau <= l(v).
//
// (b) NIVEAU ZERO. Les sommets de niveau zero sont exactement ceux de
//     P_vide = { x : L_j(x) >= 0 pour tout j }, pointe, donc de graphe connexe.
//
// Ce que ce fichier en tire, et rien de plus : un parcours depuis UN germe de
// niveau zero visite tout { l <= k }. Deux consequences a ne pas oublier :
// le niveau d'un voisin varie de -1, 0 OU +1 — la variation nulle est reelle,
// et le transport par lots ci-dessus ne suppose jamais le contraire ; et
// l'arete ouverte peut etre de niveau k+1 alors que ses deux extremites sont de
// niveau k, ce qui ne casse pas la connexite mais interdit de confondre le
// graphe induit par les etiquettes et le sous-complexe geometrique.
//
// ---------------------------------------------------------------------------
// 5. Le germe est certifie de niveau zero, il ne l'est plus par decret
// ---------------------------------------------------------------------------
//
// L'ancien germe prenait une face de l'enveloppe convexe et forcait le niveau a
// zero. C'est faux : un point COPLANAIRE a la face et strictement interieur a
// son cercle circonscrit est interieur a TOUTES les spheres du pinceau. La
// fixture u16 a cinq points (4,1,0) (14,19,0) (4,17,0) (17,9,0) (15,8,19) donne
// un germe de niveau exact 1 stocke a zero ; le niveau transporte finissait par
// passer sous zero et le nuage entier etait declare hors domaine.
//
// La correction n'est pas de compter ces points : c'est de les rendre
// impossibles. On choisit dans le plan de la face un triangle DE DELAUNAY du
// sous-nuage coplanaire — cercle circonscrit vide dans ce plan. Toute sphere du
// pinceau coupe ce plan selon ce cercle, donc aucun point coplanaire n'est
// jamais interieur, et le premier lot rencontre depuis le demi-espace vide est
// de niveau zero PAR CONSTRUCTION. Le predicat de cocircularite coplanaire est
// un determinant 4x4 entier exact, borne par 2^108,8.
#pragma once

#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "mhgp/miniball.hpp"

namespace mhgp3v {
namespace flats {

using mhgp::i128;
using mhgp::i32;
using mhgp::P3;

inline int sign_of(i128 v) { return v > 0 ? 1 : (v < 0 ? -1 : 0); }

// ---------------------------------------------------------------------------
// Predicats entiers exacts. Coordonnees sur la grille declaree u16, donc les
// differences sont sous 2^17 et les normes carrees sous 3 * 2^34 < 2^35,6.
// ---------------------------------------------------------------------------

// det[b-a, c-a, d-a]. Borne 6 * 2^51 < 2^53,6.
inline i128 orient3d_exact(const P3& a, const P3& b, const P3& c, const P3& d) {
  const i128 bx = b.x - a.x, by = b.y - a.y, bz = b.z - a.z;
  const i128 cx = c.x - a.x, cy = c.y - a.y, cz = c.z - a.z;
  const i128 dx = d.x - a.x, dy = d.y - a.y, dz = d.z - a.z;
  return bx * (cy * dz - cz * dy) - by * (cx * dz - cz * dx) + bz * (cx * dy - cy * dx);
}

// Signe du determinant 4x4 releve, calibre : -1 interieur strict, 0 sur la
// sphere, +1 exterieur strict. `orient_sign` est le signe de orient3d_exact(a,b,c,d).
// Borne 4 * 2^35,6 * 2^53,6 = 2^91,2.
inline int in_sphere_side(const P3& a, const P3& b, const P3& c, const P3& d,
                          const P3& e, int orient_sign) {
  auto row = [&a](const P3& p, i128* x, i128* y, i128* z, i128* w) {
    *x = p.x - a.x; *y = p.y - a.y; *z = p.z - a.z;
    *w = (*x) * (*x) + (*y) * (*y) + (*z) * (*z);
  };
  i128 bx, by, bz, bw, cx, cy, cz, cw, dx, dy, dz, dw, ex, ey, ez, ew;
  row(b, &bx, &by, &bz, &bw);
  row(c, &cx, &cy, &cz, &cw);
  row(d, &dx, &dy, &dz, &dw);
  row(e, &ex, &ey, &ez, &ew);
  auto minor = [](i128 x1, i128 y1, i128 z1, i128 x2, i128 y2, i128 z2,
                  i128 x3, i128 y3, i128 z3) {
    return x1 * (y2 * z3 - z2 * y3) - y1 * (x2 * z3 - z2 * x3) + z1 * (x2 * y3 - y2 * x3);
  };
  const i128 det = -bw * minor(cx, cy, cz, dx, dy, dz, ex, ey, ez)
                 + cw * minor(bx, by, bz, dx, dy, dz, ex, ey, ez)
                 - dw * minor(bx, by, bz, cx, cy, cz, ex, ey, ez)
                 + ew * minor(bx, by, bz, cx, cy, cz, dx, dy, dz);
  const int s = sign_of(det);
  if (s == 0) return 0;
  return (s * orient_sign < 0) ? -1 : 1;
}

// COCIRCULARITE COPLANAIRE EXACTE.
//
// Pour a, b, c, d COPLANAIRES, le determinant 4x4 dont les lignes sont
//
//     (b-a, |b-a|^2), (c-a, |c-a|^2), (u, 0), (d-a, |d-a|^2),   u = (b-a)x(c-a)
//
// s'annule sur une sphere passant par a, b, c dont le centre verifie
// (o - a) . u = 0 : c'est le cercle circonscrit du triangle DANS SON PLAN. En
// effet, en x = a + s u le determinant vaut s^2 |u|^2 det[b-a, c-a, u], sans
// terme lineaire en s, donc le centre est bien dans le plan.
//
// Le signe est invariant par echange de b et c : l'echange transpose deux
// lignes et retourne u, soit deux changements de signe. Calibre sur
// a=(0,0,0), b=(1,0,0), c=(0,1,0) : le centre (1/2,1/2,0) donne -1/2 < 0 et le
// point (2,2,0) donne +4 > 0. Donc NEGATIF = strictement interieur au cercle.
//
// Borne : mineurs 3x3 melant deux lignes de differences (2^17) et la ligne u
// (2^35), soit 6 * 2^69 < 2^71,6 ; multiplies par une norme carree 2^35,6 et
// sommes sur trois termes : < 2^108,8. Tient dans i128.
inline i128 in_circle_coplanar(const P3& a, const P3& b, const P3& c, const P3& d) {
  const i128 bx = b.x - a.x, by = b.y - a.y, bz = b.z - a.z;
  const i128 cx = c.x - a.x, cy = c.y - a.y, cz = c.z - a.z;
  const i128 dx = d.x - a.x, dy = d.y - a.y, dz = d.z - a.z;
  const i128 bw = bx * bx + by * by + bz * bz;
  const i128 cw = cx * cx + cy * cy + cz * cz;
  const i128 dw = dx * dx + dy * dy + dz * dz;
  const i128 ux = by * cz - bz * cy, uy = bz * cx - bx * cz, uz = bx * cy - by * cx;
  auto minor = [](i128 x1, i128 y1, i128 z1, i128 x2, i128 y2, i128 z2,
                  i128 x3, i128 y3, i128 z3) {
    return x1 * (y2 * z3 - z2 * y3) - y1 * (x2 * z3 - z2 * x3) + z1 * (x2 * y3 - y2 * x3);
  };
  // Developpement le long de la quatrieme colonne (w), dont la ligne u est nulle.
  // det = -bw*M(c,u,d) + cw*M(b,u,d) - 0 + dw*M(b,c,u).
  return -bw * minor(cx, cy, cz, ux, uy, uz, dx, dy, dz)
         + cw * minor(bx, by, bz, ux, uy, uz, dx, dy, dz)
         + dw * minor(bx, by, bz, cx, cy, cz, ux, uy, uz);
}

// Le pinceau des spheres passant par un triangle. Tout l'ordre se lit en
// predicats entiers : signe(t_z - t_w) = o(w) * [w interieur a (a,b,c,z)]^{+-}.
struct Pencil {
  const std::vector<P3>* points;
  i32 a, b, c;

  int orient_of(i32 z) const {
    return sign_of(orient3d_exact((*points)[(std::size_t)a], (*points)[(std::size_t)b],
                            (*points)[(std::size_t)c], (*points)[(std::size_t)z]));
  }
  int side(i32 z, i32 w, int orient_z) const {
    return in_sphere_side((*points)[(std::size_t)a], (*points)[(std::size_t)b],
                          (*points)[(std::size_t)c], (*points)[(std::size_t)z],
                          (*points)[(std::size_t)w], orient_z);
  }
  int compare_t(i32 z, int orient_z, i32 w, int orient_w) const {
    const int inside = side(z, w, orient_z);
    if (inside == 0) return 0;
    return orient_w * (inside < 0 ? 1 : -1);
  }
};

struct Vertex {
  std::vector<i32> shell;   // S(v), trie
  int level = 0;            // l(v) = |B(v)|
};

struct ShellHash {
  std::size_t operator()(const std::vector<i32>& v) const {
    std::size_t h = 1469598103934665603ULL;
    for (i32 x : v) { h ^= (std::size_t)x + 0x9e3779b9; h *= 1099511628211ULL; }
    return h;
  }
};

}  // namespace flats

// ---------------------------------------------------------------------------
// Diagnostics. Aucun n'est optionnel : un compteur muet est une branche morte.
// ---------------------------------------------------------------------------
struct FlatStatistics {
  long long seed_scans = 0;
  long long vertices_visited = 0;
  long long vertices_over_level = 0;     // voisins refuses par l <= k_nav
  long long flats_enumerated = 0;        // aretes distinctes, apres quotient
  long long triples_quotiented = 0;      // triplets ecartes comme base non canonique
  long long pencil_queries = 0;
  long long pencil_candidates = 0;
  long long unbounded_stops = 0;
  long long batches_multiple = 0;        // lots entrants de taille > 1
  long long shells_multiple = 0;         // sommets de coquille > 4
  long long census_checks = 0;           // verifications exactes effectuees
  long long census_mismatch_shell = 0;   // DOIT rester nul
  long long census_mismatch_level = 0;   // DOIT rester nul
  long long emitted_arity[5] = {};
  long long emit_attempts = 0;
  long long emit_duplicate_shell = 0;    // recolte redondante mesuree
  long long degenerate_flat_vertex = 0;  // coquille entierement coplanaire
  long long seed_failure_stage = 0;      // etape exacte d'un refus de germe

  void absorb(const FlatStatistics& o) {
    static_assert(sizeof(FlatStatistics) == 22 * sizeof(long long),
                  "champ ajoute a FlatStatistics : le sommer dans absorb()");
    seed_scans += o.seed_scans;
    vertices_visited += o.vertices_visited;
    vertices_over_level += o.vertices_over_level;
    flats_enumerated += o.flats_enumerated;
    triples_quotiented += o.triples_quotiented;
    pencil_queries += o.pencil_queries;
    pencil_candidates += o.pencil_candidates;
    unbounded_stops += o.unbounded_stops;
    batches_multiple += o.batches_multiple;
    shells_multiple += o.shells_multiple;
    census_checks += o.census_checks;
    census_mismatch_shell += o.census_mismatch_shell;
    census_mismatch_level += o.census_mismatch_level;
    for (int i = 0; i < 5; ++i) emitted_arity[i] += o.emitted_arity[i];
    emit_attempts += o.emit_attempts;
    emit_duplicate_shell += o.emit_duplicate_shell;
    degenerate_flat_vertex += o.degenerate_flat_vertex;
    if (o.seed_failure_stage != 0) seed_failure_stage = o.seed_failure_stage;
  }
};

// Statut du nuage. `out_of_domain` a disparu comme fourre-tout : chaque refus
// nomme sa raison, et « dimension affine < 3 » n'est pas une erreur mais une
// voie dediee.
enum class CloudStatus {
  kOk = 0,
  kAffineDimensionBelowThree,   // voie directe exhaustive, pas de navigation
  kTooFewPoints,                // n < 4 : voie directe exhaustive
  kSeedFailed,                  // germe non certifiable : refus explicite
  kInvariantViolated,           // un census a contredit le transport
};

inline const char* cloud_status_name(CloudStatus s) {
  switch (s) {
    case CloudStatus::kOk: return "ok";
    case CloudStatus::kAffineDimensionBelowThree: return "dimension_affine_inferieure_a_trois";
    case CloudStatus::kTooFewPoints: return "moins_de_quatre_points";
    case CloudStatus::kSeedFailed: return "germe_non_certifie";
    case CloudStatus::kInvariantViolated: return "invariant_de_transport_viole";
  }
  return "inconnu";
}

// ---------------------------------------------------------------------------
// Dimension affine. Les normales relevees a_i = (-2 p_i, 1) engendrent R^4 si
// et seulement si le nuage est de dimension affine trois ; c'est l'hypothese
// du theoreme de proprietaire et la condition pour que P_vide soit pointe.
// ---------------------------------------------------------------------------
inline bool affine_dimension_is_three(const std::vector<mhgp::P3>& points) {
  const int n = (int)points.size();
  if (n < 4) return false;
  int b = -1;
  for (int i = 1; i < n; ++i)
    if (points[(std::size_t)i].x != points[0].x || points[(std::size_t)i].y != points[0].y ||
        points[(std::size_t)i].z != points[0].z) { b = i; break; }
  if (b < 0) return false;
  int c = -1;
  for (int i = 1; i < n; ++i) {
    const mhgp::P3 u = mhgp::p3_sub(points[(std::size_t)b], points[0]);
    const mhgp::P3 v = mhgp::p3_sub(points[(std::size_t)i], points[0]);
    const mhgp::P3 x = mhgp::p3_cross(u, v);
    if (x.x != 0 || x.y != 0 || x.z != 0) { c = i; break; }
  }
  if (c < 0) return false;
  for (int i = 1; i < n; ++i)
    if (flats::orient3d_exact(points[0], points[(std::size_t)b], points[(std::size_t)c],
                        points[(std::size_t)i]) != 0) return true;
  return false;
}

// ---------------------------------------------------------------------------
// GERME CERTIFIE DE NIVEAU ZERO.
//
//  1. p0 = lex-min : sur l'enveloppe convexe, et sur celle de la projection xy.
//  2. p1 : emballage dans la projection xy. Le plan vertical par (p0,p1)
//     supporte tout le nuage.
//  3. p2 : rotation autour de la droite (p0,p1) jusqu'a ce que tout le nuage
//     soit d'un meme cote. Le plan (p0,p1,p2) est une face support.
//  4. Dans ce plan, (p0,p1) est une arete de l'enveloppe du sous-nuage
//     coplanaire ; on remplace p2 par le troisieme point de DELAUNAY, obtenu
//     par une passe de `in_circle_coplanar`. Son cercle circonscrit est alors
//     vide dans le plan.
//  5. Depuis le demi-espace VIDE, le premier lot rencontre donne un sommet dont
//     l'ensemble interieur est vide.
//
// Chaque etape est VERIFIEE, pas supposee : un echec rend kSeedFailed plutot
// qu'un germe faux.
// ---------------------------------------------------------------------------
inline CloudStatus seed_level_zero(const std::vector<mhgp::P3>& points,
                                   FlatStatistics* st,
                                   flats::Vertex* seed_out) {
  const int n = (int)points.size();
  if (n < 4) return CloudStatus::kTooFewPoints;

  mhgp::i32 p0 = 0;
  for (mhgp::i32 i = 1; i < n; ++i) {
    const mhgp::P3& u = points[(std::size_t)i];
    const mhgp::P3& v = points[(std::size_t)p0];
    if (u.x < v.x || (u.x == v.x && (u.y < v.y || (u.y == v.y && u.z < v.z)))) p0 = i;
  }

  // (2) emballage dans la projection xy : (p0,p1) est une arete de l'enveloppe
  // de la projection, donc le plan vertical qui la contient est support.
  mhgp::i32 p1 = -1;
  for (mhgp::i32 z = 0; z < n; ++z) {
    if (z == p0) continue;
    if (p1 < 0) { p1 = z; continue; }
    const mhgp::i128 ax = points[(std::size_t)p1].x - points[(std::size_t)p0].x;
    const mhgp::i128 ay = points[(std::size_t)p1].y - points[(std::size_t)p0].y;
    const mhgp::i128 bx = points[(std::size_t)z].x - points[(std::size_t)p0].x;
    const mhgp::i128 by = points[(std::size_t)z].y - points[(std::size_t)p0].y;
    const mhgp::i128 cross = ax * by - ay * bx;
    if (cross < 0) p1 = z;
    else if (cross == 0) {
      const mhgp::i128 la = ax * ax + ay * ay, lb = bx * bx + by * by;
      // A projection egale, preferer le plus eloigne ; a projections nulles des
      // deux cotes, preferer un point qui n'est pas verticalement aligne.
      if (lb > la) p1 = z;
    }
  }
  st->seed_scans += 2;
  if (p1 < 0) { st->seed_failure_stage = 1; return CloudStatus::kSeedFailed; }

  // (3) ROTATION AUTOUR DE (p0,p1), avec l'ambiguite de demi-tour traitee.
  //
  // Comparer deux candidats par le seul signe de `orient3d` est FAUX, et le
  // nuage cospherique a sept points
  //   (26,30,33) (27,30,34) (27,30,26) (34,30,33) (30,33,26) (25,30,25) (35,31,30)
  // le montre : `orient3d` ne voit un plan qu'a pi pres, donc deux candidats
  // situes dans le plan vertical support mais de part et d'autre de l'axe sont
  // declares a egalite alors que leurs angles sont 0 et pi. La passe unique
  // partait alors du mauvais cote et le controle final rougissait.
  //
  // On classe donc explicitement l'angle. Soit e = p1 - p0 ; le plan vertical
  // support a pour normale interieure g = (-e_y, e_x, 0) — c'est exactement le
  // produit vectoriel plan que l'emballage vient de rendre positif — et on pose
  // f = g x e. Pour w = z - p0, b = w.g est positif ou nul par support, et
  // l'angle vaut 0 si (b = 0, a > 0), pi si (b = 0, a < 0), et il est dans
  // l'intervalle ouvert sinon. Le minimum de cet angle est la face cherchee ;
  // dans la classe ouverte, `orient3d` est un ordre total et suffit.
  auto collinear_with_axis = [&](mhgp::i32 z) {
    const mhgp::P3 u = mhgp::p3_sub(points[(std::size_t)p1], points[(std::size_t)p0]);
    const mhgp::P3 w = mhgp::p3_sub(points[(std::size_t)z], points[(std::size_t)p0]);
    const mhgp::P3 x = mhgp::p3_cross(u, w);
    return x.x == 0 && x.y == 0 && x.z == 0;
  };
  const mhgp::i128 ex = points[(std::size_t)p1].x - points[(std::size_t)p0].x;
  const mhgp::i128 ey = points[(std::size_t)p1].y - points[(std::size_t)p0].y;
  const mhgp::i128 ez = points[(std::size_t)p1].z - points[(std::size_t)p0].z;
  const mhgp::i128 gx = -ey, gy = ex, gz = 0;
  const mhgp::i128 fx = gy * ez - gz * ey, fy = gz * ex - gx * ez, fz = gx * ey - gy * ex;
  auto angle_class = [&](mhgp::i32 z, int* cls) {
    const mhgp::i128 wx = points[(std::size_t)z].x - points[(std::size_t)p0].x;
    const mhgp::i128 wy = points[(std::size_t)z].y - points[(std::size_t)p0].y;
    const mhgp::i128 wz = points[(std::size_t)z].z - points[(std::size_t)p0].z;
    const mhgp::i128 b = wx * gx + wy * gy + wz * gz;
    if (b < 0) return false;                       // support viole : emballage faux
    const mhgp::i128 a = wx * fx + wy * fy + wz * fz;
    *cls = (b > 0) ? 1 : (a > 0 ? 0 : 2);
    return true;
  };
  mhgp::i32 p2 = -1;
  int p2_class = 3;
  for (mhgp::i32 z = 0; z < n; ++z) {
    if (z == p0 || z == p1) continue;
    int cls = 3;
    if (!angle_class(z, &cls)) { st->seed_failure_stage = 11; return CloudStatus::kSeedFailed; }
    if (collinear_with_axis(z)) continue;          // aucun plan : jamais un p2
    if (p2 < 0 || cls < p2_class) { p2 = z; p2_class = cls; continue; }
    if (cls == p2_class && cls == 1 &&
        flats::orient3d_exact(points[(std::size_t)p0], points[(std::size_t)p1],
                              points[(std::size_t)p2], points[(std::size_t)z]) < 0) p2 = z;
  }
  ++st->seed_scans;
  if (p2 < 0) { st->seed_failure_stage = 2; return CloudStatus::kSeedFailed; }
  for (mhgp::i32 z = 0; z < n; ++z)
    if (flats::orient3d_exact(points[(std::size_t)p0], points[(std::size_t)p1],
                        points[(std::size_t)p2], points[(std::size_t)z]) < 0)
      { st->seed_failure_stage = 3; return CloudStatus::kSeedFailed; }   // rotation
  ++st->seed_scans;

  // (4) TRIANGLE DE DELAUNAY dans le plan de la face support.
  //
  // On n'exige plus que (p0,p1) soit une arete de l'enveloppe du sous-nuage
  // coplanaire — c'est faux des qu'un point est aligne entre p0 et p1, et cette
  // hypothese cachait le vrai argument. On descend a la place le RAYON :
  // si d est strictement interieur au cercle de (a,b,c), alors l'un des trois
  // triangles (a,b,d), (b,c,d), (c,a,d) a un cercle strictement plus petit.
  // Prendre a chaque tour le minimum exact de ces rayons fait donc decroitre
  // strictement une quantite prise dans un ensemble fini : la boucle termine, et
  // s'arrete precisement quand le cercle est vide.
  std::vector<mhgp::i32> plane_points;
  for (mhgp::i32 z = 0; z < n; ++z)
    if (flats::orient3d_exact(points[(std::size_t)p0], points[(std::size_t)p1],
                        points[(std::size_t)p2], points[(std::size_t)z]) == 0)
      plane_points.push_back(z);
  ++st->seed_scans;

  auto non_degenerate = [&](mhgp::i32 a, mhgp::i32 b, mhgp::i32 c) {
    const mhgp::P3 u = mhgp::p3_sub(points[(std::size_t)b], points[(std::size_t)a]);
    const mhgp::P3 w = mhgp::p3_sub(points[(std::size_t)c], points[(std::size_t)a]);
    const mhgp::P3 x = mhgp::p3_cross(u, w);
    return x.x != 0 || x.y != 0 || x.z != 0;
  };
  auto circum = [&](mhgp::i32 a, mhgp::i32 b, mhgp::i32 c, mhgp::Sphere* s) {
    return mhgp::sphere3(points[(std::size_t)a], points[(std::size_t)b],
                         points[(std::size_t)c], s);
  };

  mhgp::i32 tri[3] = {-1, -1, -1};
  {
    const int q = (int)plane_points.size();
    bool found = false;
    for (int x = 0; x < q && !found; ++x)
      for (int y = x + 1; y < q && !found; ++y)
        for (int z = y + 1; z < q && !found; ++z)
          if (non_degenerate(plane_points[(std::size_t)x], plane_points[(std::size_t)y],
                             plane_points[(std::size_t)z])) {
            tri[0] = plane_points[(std::size_t)x];
            tri[1] = plane_points[(std::size_t)y];
            tri[2] = plane_points[(std::size_t)z];
            found = true;
          }
    if (!found) { st->seed_failure_stage = 4; return CloudStatus::kSeedFailed; }
  }
  for (int guard = 0; guard <= (int)plane_points.size() * (int)plane_points.size() + 8; ++guard) {
    mhgp::i32 intruder = -1;
    for (mhgp::i32 z : plane_points) {
      if (z == tri[0] || z == tri[1] || z == tri[2]) continue;
      if (flats::in_circle_coplanar(points[(std::size_t)tri[0]], points[(std::size_t)tri[1]],
                                    points[(std::size_t)tri[2]], points[(std::size_t)z]) < 0) {
        intruder = z;
        break;
      }
    }
    ++st->seed_scans;
    if (intruder < 0) break;
    mhgp::Sphere current{};
    if (!circum(tri[0], tri[1], tri[2], &current)) { st->seed_failure_stage = 5; return CloudStatus::kSeedFailed; }
    mhgp::i32 best_tri[3] = {-1, -1, -1};
    mhgp::Sphere best_sphere{};
    for (int drop = 0; drop < 3; ++drop) {
      mhgp::i32 cand[3];
      int w = 0;
      for (int t = 0; t < 3; ++t) if (t != drop) cand[w++] = tri[t];
      cand[2] = intruder;
      if (!non_degenerate(cand[0], cand[1], cand[2])) continue;
      mhgp::Sphere s{};
      if (!circum(cand[0], cand[1], cand[2], &s)) continue;
      if (mhgp::sphere_cmp_beta(s, current) >= 0) continue;      // pas de descente
      if (best_tri[0] >= 0 && mhgp::sphere_cmp_beta(s, best_sphere) >= 0) continue;
      best_tri[0] = cand[0]; best_tri[1] = cand[1]; best_tri[2] = cand[2];
      best_sphere = s;
    }
    if (best_tri[0] < 0) { st->seed_failure_stage = 6; return CloudStatus::kSeedFailed; }   // descente impossible
    tri[0] = best_tri[0]; tri[1] = best_tri[1]; tri[2] = best_tri[2];
  }
  for (mhgp::i32 z : plane_points) {
    if (z == tri[0] || z == tri[1] || z == tri[2]) continue;
    if (flats::in_circle_coplanar(points[(std::size_t)tri[0]], points[(std::size_t)tri[1]],
                                  points[(std::size_t)tri[2]], points[(std::size_t)z]) < 0)
      { st->seed_failure_stage = 7; return CloudStatus::kSeedFailed; }   // cercle non vide
  }
  ++st->seed_scans;

  // (5) premier lot depuis le demi-espace VIDE. Le cote support est celui ou se
  // trouvent les points hors du plan ; le demi-espace vide est l'autre.
  const mhgp::i32 p0t = tri[0], p1t = tri[1], apex_plane = tri[2];
  flats::Pencil face{&points, p0t, p1t, apex_plane};
  int supporting = 0;
  for (mhgp::i32 z = 0; z < n; ++z) {
    const int o = face.orient_of(z);
    if (o == 0) continue;
    if (supporting == 0) supporting = o;
    else if (supporting != o) { st->seed_failure_stage = 8; return CloudStatus::kSeedFailed; }
  }
  ++st->seed_scans;
  if (supporting == 0) { st->seed_failure_stage = 9; return CloudStatus::kSeedFailed; }   // tout coplanaire

  // La boule tend vers { u.(x-a) > 0 } quand t -> +oo. Le demi-espace vide est
  // donc a t = -oo si les points sont du cote +, a t = +oo sinon ; on cherche
  // en consequence le parametre MINIMAL ou MAXIMAL.
  const int wanted = supporting;   // signe de (t_best - t_z) exige pour garder z
  mhgp::i32 best = -1;
  int best_orient = 0;
  for (mhgp::i32 z = 0; z < n; ++z) {
    const int oz = face.orient_of(z);
    if (oz == 0) continue;
    if (best < 0) { best = z; best_orient = oz; continue; }
    if (face.compare_t(best, best_orient, z, oz) == wanted) { best = z; best_orient = oz; }
  }
  ++st->seed_scans;
  if (best < 0) { st->seed_failure_stage = 10; return CloudStatus::kSeedFailed; }

  std::vector<mhgp::i32> shell{p0t, p1t, apex_plane, best};
  for (mhgp::i32 z = 0; z < n; ++z) {
    if (z == p0t || z == p1t || z == apex_plane || z == best) continue;
    const int oz = face.orient_of(z);
    if (oz == 0) {
      // Coplanaire : etat constant le long du pinceau. Sur le cercle il
      // appartient a toutes les coquilles ; ailleurs il est strictement
      // exterieur, puisque le cercle est vide.
      if (flats::in_circle_coplanar(points[(std::size_t)p0t], points[(std::size_t)p1t],
                                    points[(std::size_t)apex_plane],
                                    points[(std::size_t)z]) == 0) shell.push_back(z);
      continue;
    }
    if (face.compare_t(best, best_orient, z, oz) == 0) shell.push_back(z);
  }
  ++st->seed_scans;
  std::sort(shell.begin(), shell.end());
  seed_out->shell = shell;
  seed_out->level = 0;
  return CloudStatus::kOk;
}

namespace flats {

// Sphere exacte portee par la coquille : n'importe quel quadruplet independant
// de la coquille la determine. Utilisee UNIQUEMENT par le census de controle.
inline bool shell_sphere(const std::vector<P3>& points, const std::vector<i32>& shell,
                         mhgp::Sphere* out) {
  const int m = (int)shell.size();
  if (m < 4) return false;
  for (int i = 0; i < m; ++i)
    for (int j = i + 1; j < m; ++j)
      for (int k = j + 1; k < m; ++k)
        for (int l = k + 1; l < m; ++l) {
          mhgp::Sphere s{};
          if (!mhgp::sphere4(points[(std::size_t)shell[(std::size_t)i]],
                             points[(std::size_t)shell[(std::size_t)j]],
                             points[(std::size_t)shell[(std::size_t)k]],
                             points[(std::size_t)shell[(std::size_t)l]], &s)) continue;
          *out = s;
          return true;
        }
  return false;
}

// Census exact d'un sommet : recalcule la coquille et le niveau depuis la
// sphere, sans rien emprunter au transport. C'est le juge local du prototype.
inline bool census(const std::vector<P3>& points, const Vertex& v,
                   std::vector<i32>* shell_out, int* level_out) {
  mhgp::Sphere s{};
  if (!shell_sphere(points, v.shell, &s)) return false;
  shell_out->clear();
  *level_out = 0;
  for (i32 z = 0; z < (i32)points.size(); ++z) {
    const int side = mhgp::sphere_side(s, points[(std::size_t)z]);
    if (side < 0) ++(*level_out);
    else if (side == 0) shell_out->push_back(z);
  }
  return true;
}

}  // namespace flats

// ---------------------------------------------------------------------------
// LE PARCOURS.
//
// Coupe UNIQUEMENT sur le niveau strict. Aretes = flats fermes de rang trois,
// enumeres par leurs PLANS distincts et non par les C(m,3) triplets. Transition
// par lots. Le census exact peut etre active : il ne corrige rien, il refute.
// ---------------------------------------------------------------------------
inline std::vector<flats::Vertex> navigate_shallow(const std::vector<mhgp::P3>& points,
                                                   int level_ceiling,
                                                   FlatStatistics* st,
                                                   CloudStatus* status,
                                                   bool verify_census) {
  using namespace flats;
  std::vector<Vertex> visited;
  const int n = (int)points.size();
  *status = CloudStatus::kOk;
  if (n < 4) { *status = CloudStatus::kTooFewPoints; return visited; }
  if (!affine_dimension_is_three(points)) {
    *status = CloudStatus::kAffineDimensionBelowThree;
    return visited;
  }
  if (level_ceiling < 0) return visited;

  Vertex seed;
  const CloudStatus seeded = seed_level_zero(points, st, &seed);
  if (seeded != CloudStatus::kOk) { *status = seeded; return visited; }

  std::unordered_set<std::vector<i32>, ShellHash> seen;
  std::vector<Vertex> frontier;
  seen.insert(seed.shell);
  frontier.push_back(seed);

  std::vector<i32> closure, batch, shell_check;
  while (!frontier.empty()) {
    const Vertex v = frontier.back();
    frontier.pop_back();
    const int m = (int)v.shell.size();
    visited.push_back(v);
    ++st->vertices_visited;
    if (m > 4) ++st->shells_multiple;

    if (verify_census) {
      int level_exact = 0;
      if (census(points, v, &shell_check, &level_exact)) {
        ++st->census_checks;
        if (shell_check != v.shell) ++st->census_mismatch_shell;
        if (level_exact != v.level) ++st->census_mismatch_level;
      }
    }

    bool any_flat = false;
    for (int i = 0; i < m; ++i)
    for (int j = i + 1; j < m; ++j)
    for (int k = j + 1; k < m; ++k) {
      const i32 ta = v.shell[(std::size_t)i], tb = v.shell[(std::size_t)j],
                tc = v.shell[(std::size_t)k];
      {   // triplet aligne : ne definit aucun plan
        const mhgp::P3 u = mhgp::p3_sub(points[(std::size_t)tb], points[(std::size_t)ta]);
        const mhgp::P3 w = mhgp::p3_sub(points[(std::size_t)tc], points[(std::size_t)ta]);
        const mhgp::P3 x = mhgp::p3_cross(u, w);
        if (x.x == 0 && x.y == 0 && x.z == 0) continue;
      }

      // FERMETURE du flat : les membres de la coquille situes dans ce plan.
      closure.clear();
      for (int t = 0; t < m; ++t) {
        const i32 z = v.shell[(std::size_t)t];
        if (orient3d_exact(points[(std::size_t)ta], points[(std::size_t)tb],
                     points[(std::size_t)tc], points[(std::size_t)z]) == 0) closure.push_back(z);
      }
      // BASE CANONIQUE : le premier triplet independant de la fermeture, dans
      // l'ordre des identifiants. Un triplet qui n'est pas cette base decrit la
      // MEME arete et doit etre ecarte, sinon le meme voisin est recalcule
      // C(|C|,3) fois et le degre du sommet est artificiellement combinatoire.
      i32 base[3] = {-1, -1, -1};
      {
        const int q = (int)closure.size();
        bool found = false;
        for (int x = 0; x < q && !found; ++x)
        for (int y = x + 1; y < q && !found; ++y)
        for (int z = y + 1; z < q && !found; ++z) {
          const mhgp::P3 u = mhgp::p3_sub(points[(std::size_t)closure[(std::size_t)y]],
                                          points[(std::size_t)closure[(std::size_t)x]]);
          const mhgp::P3 w = mhgp::p3_sub(points[(std::size_t)closure[(std::size_t)z]],
                                          points[(std::size_t)closure[(std::size_t)x]]);
          const mhgp::P3 cr = mhgp::p3_cross(u, w);
          if (cr.x == 0 && cr.y == 0 && cr.z == 0) continue;
          base[0] = closure[(std::size_t)x];
          base[1] = closure[(std::size_t)y];
          base[2] = closure[(std::size_t)z];
          found = true;
        }
        if (!found) continue;
      }
      if (!(base[0] == ta && base[1] == tb && base[2] == tc)) {
        ++st->triples_quotiented;
        continue;
      }
      ++st->flats_enumerated;
      any_flat = true;

      Pencil pencil{&points, base[0], base[1], base[2]};
      // Sommet du repere : un membre de la coquille HORS du plan. Il fixe le
      // parametre courant. Il existe des que la coquille n'est pas coplanaire.
      i32 apex = -1;
      int orient_apex = 0;
      for (int t = 0; t < m; ++t) {
        const i32 z = v.shell[(std::size_t)t];
        const int oz = pencil.orient_of(z);
        if (oz != 0) { apex = z; orient_apex = oz; break; }
      }
      if (apex < 0) continue;

      for (int direction = -1; direction <= 1; direction += 2) {
        ++st->pencil_queries;
        i32 best = -1;
        int best_orient = 0;
        batch.clear();
        for (i32 z = 0; z < n; ++z) {
          if (std::binary_search(v.shell.begin(), v.shell.end(), z)) continue;
          const int oz = pencil.orient_of(z);
          if (oz == 0) continue;            // constant le long du pinceau
          ++st->pencil_candidates;
          if (pencil.compare_t(z, oz, apex, orient_apex) != direction) continue;
          if (best < 0) { best = z; best_orient = oz; batch.assign(1, z); continue; }
          const int cmp = pencil.compare_t(z, oz, best, best_orient);
          if (cmp == 0) batch.push_back(z);
          else if (cmp == -direction) { best = z; best_orient = oz; batch.assign(1, z); }
        }
        if (best < 0) { ++st->unbounded_stops; continue; }
        if (batch.size() > 1) ++st->batches_multiple;

        // S(w) = C(F) union A. Rien d'autre : les membres de S(v) \ C(F) ne
        // contiennent pas la droite et quittent la coquille ; les points hors de
        // S(v) coplanaires au plan sans etre sur le cercle sont constants
        // strictement d'un cote et ne rejoignent jamais aucune coquille.
        std::vector<i32> shell = closure;
        shell.insert(shell.end(), batch.begin(), batch.end());
        std::sort(shell.begin(), shell.end());

        // Transport par lots. Sur l'arete ouverte, un membre de S(v)\C est
        // strictement d'un cote constant : on le lit sur la sphere terminale.
        // Un membre du lot entrant etait interieur sur l'arete ouverte si et
        // seulement s'il etait interieur a la sphere de v.
        int level = v.level;
        for (int t = 0; t < m; ++t) {
          const i32 z = v.shell[(std::size_t)t];
          if (std::binary_search(closure.begin(), closure.end(), z)) continue;
          if (pencil.side(best, z, best_orient) < 0) ++level;
        }
        for (i32 z : batch)
          if (pencil.side(apex, z, orient_apex) < 0) --level;

        if (level < 0) { *status = CloudStatus::kInvariantViolated; return visited; }
        if (level > level_ceiling) { ++st->vertices_over_level; continue; }
        if (seen.insert(shell).second) frontier.push_back(Vertex{shell, level});
      }
    }
    if (!any_flat) ++st->degenerate_flat_vertex;
  }
  return visited;
}

// ---------------------------------------------------------------------------
// DU PARCOURS AU CATALOGUE.
//
// Critere unifie : une boule est CRITIQUE si et seulement si elle est la
// MINIBOULE de sa coquille. Le support HGP publie est le support canonique de
// cette miniboule — plus petite cardinalite, puis plus petit sous-ensemble
// lexicographique dont le centre est dans l'interieur relatif de l'enveloppe.
//
// Complétude : par le theoreme de proprietaire, tout support d'arite q entre 2
// et 4 dont la sphere a un rang ferme au plus s_max est CONTENU dans la
// coquille d'un sommet de niveau au plus s_max - q <= s_max - 2. Recolter les
// sous-ensembles de taille 2, 3 et 4 des coquilles visitees suffit donc.
// Les singletons sont publies directement.
// ---------------------------------------------------------------------------
inline mhgp::Catalogue flat_catalogue(const std::vector<mhgp::P3>& points, int s_max,
                                      FlatStatistics* st, CloudStatus* status,
                                      bool verify_census) {
  *st = FlatStatistics{};
  mhgp::Catalogue catalogue;
  const int n = (int)points.size();
  *status = CloudStatus::kOk;

  std::vector<mhgp::CriticalSphere> kept;
  std::vector<mhgp::i32> members_pool;
  std::unordered_set<std::vector<mhgp::i32>, flats::ShellHash> emitted;

  auto try_emit = [&](const mhgp::i32* candidate, int m) {
    ++st->emit_attempts;
    const mhgp::MiniballResult mb = mhgp::miniball_of(points, candidate, m);
    if (!mb.ok) return;
    // La miniboule doit passer par TOUT le candidat, sinon la sphere de ce
    // support n'est pas minimale : sa sous-sphere sera recoltee ailleurs.
    for (int i = 0; i < m; ++i)
      if (mhgp::sphere_side(mb.sph, points[(std::size_t)candidate[i]]) != 0) return;
    std::vector<mhgp::i32> members, shell;
    for (mhgp::i32 z = 0; z < n; ++z) {
      const int side = mhgp::sphere_side(mb.sph, points[(std::size_t)z]);
      if (side > 0) continue;
      if (side == 0) shell.push_back(z);
      members.push_back(z);
    }
    if ((int)members.size() > s_max) return;
    if (!emitted.insert(shell).second) { ++st->emit_duplicate_shell; return; }

    // SUPPORT CANONIQUE.
    //
    // Une miniboule peut avoir PLUSIEURS supports minimaux : le cube
    // cospherique en a quatre, ses quatre paires antipodales. Lire le support
    // sur le candidat qui a servi a la decouvrir rendrait l'arite publique
    // dependante du chemin — la force brute annonce {2,5}, la navigation {0,7}
    // pour la meme sphere.
    //
    // Le lire sur la coquille triee par IDENTIFIANT ne suffit pas non plus :
    // c'est equivariant mais pas invariant, et renumeroter le nuage change le
    // support publie. Mesure : sur `cube`, `constant_shell_members`,
    // `coplanaire_pur` et le nuage a demi-tour, une seule permutation suffit a
    // changer la sortie.
    //
    // La convention est donc GEOMETRIQUE : plus petite cardinalite, puis plus
    // petit sous-ensemble pour l'ordre des COORDONNEES. Trier la coquille par
    // (x,y,z) avant d'appeler `miniball_of` la realise sans enumerer les
    // supports, puisque l'enumeration interne suit l'ordre du tableau recu. Le
    // resultat ne depend plus que de l'ensemble de points, jamais de sa
    // numerotation. Reste hors contrat : deux points de coordonnees identiques,
    // degenerescence declaree a part.
    std::vector<mhgp::i32> by_coordinate = shell;
    std::sort(by_coordinate.begin(), by_coordinate.end(),
              [&](mhgp::i32 x, mhgp::i32 y) {
                const mhgp::P3& u = points[(std::size_t)x];
                const mhgp::P3& w = points[(std::size_t)y];
                if (u.x != w.x) return u.x < w.x;
                if (u.y != w.y) return u.y < w.y;
                if (u.z != w.z) return u.z < w.z;
                return x < y;
              });
    const mhgp::MiniballResult canonical_mb = mhgp::miniball_of(points, by_coordinate.data(),
                                                                (int)by_coordinate.size());
    if (!canonical_mb.ok) return;

    mhgp::CriticalSphere critical{};
    for (int i = 0; i < mhgp::kMaxSupport; ++i)
      critical.support[i] = i < canonical_mb.n_support ? canonical_mb.support[i] : -1;
    critical.n_support = canonical_mb.n_support;
    critical.rank = (int)members.size();
    critical.sph = canonical_mb.sph;
    critical.beta = mhgp::sphere_beta(canonical_mb.sph);
    critical.members_begin = (mhgp::i32)members_pool.size();
    members_pool.insert(members_pool.end(), members.begin(), members.end());
    kept.push_back(critical);
    ++st->emitted_arity[canonical_mb.n_support];
  };

  for (mhgp::i32 p = 0; p < n; ++p) try_emit(&p, 1);

  const bool navigable = n >= 4 && affine_dimension_is_three(points);
  if (!navigable) {
    // VOIE DIRECTE. Un nuage de moins de quatre points, ou de dimension affine
    // inferieure a trois, n'a pas de sommet d'arrangement : le theoreme de
    // proprietaire ne s'y applique pas. On ne censure pas le nuage, on le
    // resout exhaustivement — c'est exact, et le domaine est declare.
    *status = n < 4 ? CloudStatus::kTooFewPoints : CloudStatus::kAffineDimensionBelowThree;
    for (mhgp::i32 a = 0; a < n; ++a)
      for (mhgp::i32 b = a + 1; b < n; ++b) {
        const mhgp::i32 e[2] = {a, b};
        try_emit(e, 2);
        for (mhgp::i32 c = b + 1; c < n; ++c) {
          const mhgp::i32 f[3] = {a, b, c};
          try_emit(f, 3);
          for (mhgp::i32 d = c + 1; d < n; ++d) {
            const mhgp::i32 g[4] = {a, b, c, d};
            try_emit(g, 4);
          }
        }
      }
  } else {
    const int level_ceiling = s_max - 2;
    CloudStatus nav = CloudStatus::kOk;
    const auto vertices = navigate_shallow(points, level_ceiling, st, &nav, verify_census);
    if (nav != CloudStatus::kOk) { *status = nav; return catalogue; }
    for (const flats::Vertex& v : vertices) {
      const int m = (int)v.shell.size();
      try_emit(v.shell.data(), m);
      for (int i = 0; i < m; ++i)
        for (int j = i + 1; j < m; ++j) {
          const mhgp::i32 e[2] = {v.shell[(std::size_t)i], v.shell[(std::size_t)j]};
          try_emit(e, 2);
          // Les 4-sous-ensembles sont REDONDANTS et ne sont pas recoltes. Si le
          // support canonique d'une sphere critique a quatre points, il est
          // affinement independant, sa sphere est le sommet lui-meme et sa
          // coquille est S(v) : `try_emit(v.shell)` la publie. Si quatre points
          // de la coquille sont coplanaires, leur miniboule a un support d'au
          // plus trois points et la recolte d'arite trois la publie.
          for (int k = j + 1; k < m; ++k) {
            const mhgp::i32 f[3] = {e[0], e[1], v.shell[(std::size_t)k]};
            try_emit(f, 3);
          }
        }
    }
  }

  // ORDRE CANONIQUE DE SERIALISATION. Lexicographique sur les QUATRE cases de
  // `support`, remplies de -1 en queue — jamais par arite d'abord. Deux
  // generateurs qui trient l'un par arite et l'autre lexicographiquement
  // produisent des catalogues semantiquement egaux mais d'indices differents,
  // et `ForestNode::source` est un indice : la divergence devient publique.
  std::vector<int> order((std::size_t)kept.size());
  for (std::size_t i = 0; i < order.size(); ++i) order[i] = (int)i;
  std::sort(order.begin(), order.end(), [&](int x, int y) {
    const mhgp::CriticalSphere& u = kept[(std::size_t)x];
    const mhgp::CriticalSphere& w = kept[(std::size_t)y];
    for (int i = 0; i < mhgp::kMaxSupport; ++i)
      if (u.support[i] != w.support[i]) return u.support[i] < w.support[i];
    return false;
  });
  for (int idx : order) {
    mhgp::CriticalSphere critical = kept[(std::size_t)idx];
    const int begin = critical.members_begin;
    critical.members_begin = (mhgp::i32)catalogue.members.size();
    for (int i = 0; i < critical.rank; ++i)
      catalogue.members.push_back(members_pool[(std::size_t)(begin + i)]);
    catalogue.spheres.push_back(critical);
  }
  return catalogue;
}

}  // namespace mhgp3v
