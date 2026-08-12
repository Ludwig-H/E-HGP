// MorseHGP3D v3 — SOURCE PAR LISTES IMBRIQUEES DE CELLULES DE CENTRES.
//
// Sujet : l'architecture de
// `audits/NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md`, corrigee
// par `audits/AUDIT_REPONSES_CELLULES_CENTRES_20260812.md`.
//
// Le renversement par rapport au plein arrangement : on ne parcourt plus les
// sommets — dont la mesure montre qu'ils sont majoritairement des TRANSITS — on
// subdivise l'espace des CENTRES et on herite des listes de candidats.
//
// ---------------------------------------------------------------------------
// LEMME PROFONDEUR--CELLULE (auditeur, section 3 de l'audit du 12 aout).
//
// Pour une cellule de centres `C` de fermeture compacte `K_C`, poser
//   l_C(x) = min_{c dans K_C} ||x-c||^2,   u_C(x) = max_{c dans K_C} ||x-c||^2,
//   R_p(C) = (p+1)-ieme plus petite valeur de u_C,
//   A_p(C) = { x : l_C(x) <= R_p(C) }.
// Si une boule positive possedee par `C` a EXACTEMENT `p` sites strictement
// interieurs, alors `beta<=R_p(C)` et `I_B union U_B` est inclus dans `A_p(C)`.
// PREUVE. Si `beta>R_p(C)`, les `p+1` temoins de `u_C<=R_p(C)` verifient
// `||x-c||^2<=u_C<=R_p(C)<beta` : ils sont `p+1` interieurs stricts, ce qui
// contredit l'hypothese. Ensuite tout `x` de la boule fermee verifie
// `l_C(x)<=||x-c||^2<=beta<=R_p(C)`.
//
// Ce lemme est STRICTEMENT plus fin que la version par arite : il indexe les
// listes par la profondeur exacte et non par `smax-q+1`.
//
// MONOTONIE. `K_D` inclus dans `K_C` donne `l_D>=l_C`, `u_D<=u_C`, donc
// `R_p(D)<=R_p(C)` et `A_p(D)` inclus dans `A_p(C)`. Les `p+1` plus petits `u_D`
// etant tous dans `A_p(C)`, `R_p(D)` se calcule EXACTEMENT sur la seule liste
// parente. Aucun rescan global apres la racine. Dans une cellule,
// `A_0` inclus dans `A_1` inclus dans ... : les listes sont imbriquees.
//
// EXACT-ONCE ENTRE PROFONDEURS. Poser `tau_C(x)=min{p : x dans A_p(C)}` et,
// pour un support propose `U`, `e(U)=max_{x dans U} tau_C(x)`. On scanne les
// buckets jusqu'a `A_e`. Si le compte interieur partiel `r` verifie `r<=e`,
// alors le census est global; et comme un membre au moins n'est pas dans
// `A_{e-1}`, on a `beta>R_{e-1}`, donc les `e` temoins de `R_{e-1}` sont
// interieurs stricts : `r>=e`, donc `r=e` EXACTEMENT. C'est un invariant
// verifiable, pas une convention. Si `r>e`, on promeut `e` vers `r` et on ne
// scanne QUE les nouveaux buckets. L'indice croit strictement; chaque `PointId`
// est teste au plus une fois par boule. Si `r>smax-q`, le support est
// `above_support_window` et AUCUN shell partiel n'est publie.
//
// ---------------------------------------------------------------------------
// LANES D'ARITE INDEPENDANTES (auditeur, section 4).
//
// Il est FAUX d'engendrer q3 depuis des aretes q2 retenues, ou q4 depuis des
// facettes q3 retenues : les lanes de profondeur ne sont pas emboitees entre
// arites. Deux contre-fixtures gravees le prouvent et sont executables par
// `--fixtures=arite3` et `--fixtures=arite4`. Les boucles ci-dessous partent
// donc de cliques GEOMETRIQUES et ne consultent jamais le verdict de l'arite
// inferieure. Cet invariant est le coeur de la completude; le mutant
// `arity-cascade` le casse et doit etre tue par ces deux fixtures.
//
// ---------------------------------------------------------------------------
// ENUMERATION. Tout membre `x` d'un support de rayon carre `beta` possede par
// `C` verifie `l_C(x)<=beta<=u_C(x)` : les intervalles des membres ont une
// intersection non vide. Le plan bissecteur de deux membres rencontre `K_C`.
// Ces deux conditions sont des FILTRES EXACTS, pas une reduction sparse : le
// pire cas reste `C(m,4)`. Le graphe est materialise une fois par cellule en
// bitset, ce qui supprime les retests, non les cliques.
//
// EXACT-ONCE SPATIAL. Une cellule possede les centres de son pave HALF-OPEN; la
// face haute de la racine appartient au dernier enfant. Aucun dictionnaire
// global de supports n'est necessaire.
//
// ARITHMETIQUE. Cellules dyadiques, bornes entieres a l'echelle `2^depth`,
// sites mis a la meme echelle. `l` et `u` sont des entiers exacts. Aucun
// flottant n'est une autorite.
//
// Codes de sortie : 0 accord, 1 desaccord du juge, 2 refus avant calcul,
// 3 plancher ou invariant viole, 4 mutant tue.
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "ball_front.hpp"
#include "cloud_families.hpp"

namespace {

using mhgp3v::ballfront::i128;
using mhgp3v::ballfront::i64;
using mhgp3v::ballfront::Lift;
using mhgp3v::ballfront::sub;
using mhgp3v::ballfront::TriangleLift;
using mhgp3v::ballfront::V3;

// ---------------------------------------------------------------------------
// Mutants. Chacun casse UNE decision et doit etre tue par le juge d'identites.
// ---------------------------------------------------------------------------
enum class Mutant {
  kNone,
  kDropTies,        // filtre `l < R` au lieu de `l <= R`
  kOwnerClosed,     // pave ferme : le meme support est emis par plusieurs cellules
  kRankClosed,      // rejet sur le rang ferme au lieu de `p+q`
  kTightThreshold,  // seuil de la lane la plus etroite employe partout
  kBisectorStrict,  // bissecteur tangent refuse (`<` au lieu de `<=`)
  kShrinkList,      // un candidat retire de chaque liste
  kArityCascade,    // q3 exige une arete q2 retenue, q4 une facette q3 retenue
  kStrataStop,      // census arrete au premier bucket, sans promotion
  kIncidenceOffByOne  // un K4 compte une fois par face : la borne doit le voir
};

const char* mutant_name(Mutant m) {
  switch (m) {
    case Mutant::kNone: return "aucun";
    case Mutant::kDropTies: return "drop-ties";
    case Mutant::kOwnerClosed: return "owner-closed";
    case Mutant::kRankClosed: return "rank-closed";
    case Mutant::kTightThreshold: return "tight-threshold";
    case Mutant::kBisectorStrict: return "bisector-strict";
    case Mutant::kShrinkList: return "shrink-list";
    case Mutant::kArityCascade: return "arity-cascade";
    case Mutant::kStrataStop: return "strata-stop";
    case Mutant::kIncidenceOffByOne: return "incidence-off-by-one";
  }
  return "?";
}

// Un pave dyadique de centres : [lo, hi] a l'echelle 2^depth.
struct CentreCell {
  i64 lo[3] = {0, 0, 0};
  i64 hi[3] = {0, 0, 0};
  int depth = 0;
};

struct Stats {
  i64 cells_created = 0, cells_terminal = 0, cells_pruned = 0, cells_split = 0;
  i64 max_depth_reached = 0;
  i64 parent_candidate_reads = 0, bound_evals = 0;
  i64 candidate_ids = 0, candidate_high_water = 0;
  i64 separation_tests = 0, separation_pruned = 0;
  i64 child_separation_tests = 0, child_separation_pruned = 0;
  i64 overlap_pairs = 0, terminal_overlaps = 0, max_terminal_overlaps = 0;
  i64 potential_triples = 0, potential_quads = 0;
  i64 clique_pairs = 0, clique_triples = 0, clique_quads = 0;
  i64 bisector_tests = 0, bisector_pruned = 0;
  i64 hull_tests = 0, hull_pruned = 0;
  i64 diameter_tests = 0, diameter_pruned = 0;
  i64 occurrences_recorded = 0, occurrences_unowned = 0, occurrences_nonpositive = 0;
  i64 occurrences_degenerate = 0, owner_tests = 0, owner_multiple = 0, batches_flushed = 0;
  i64 probe_tests = 0, probe_accepted = 0, probe_edges = 0, probe_triangles = 0, probe_quads = 0;
  i64 incidence_checks = 0;
  i64 axis_tests = 0, axis_pruned = 0, axis_unusable = 0;
  i64 lifts_built = 0, degenerate_lifts = 0;
  // LEDGER DES CAUSES, PAR ARITE. Sans lui, « les petites cellules causent le
  // ratio » reste une hypothese et non une mesure.
  i64 lifts_q[5] = {0, 0, 0, 0, 0};
  i64 owner_rejected_q[5] = {0, 0, 0, 0, 0};
  i64 positive_rejected_q[5] = {0, 0, 0, 0, 0};
  i64 degenerate_q[5] = {0, 0, 0, 0, 0};
  i64 rank_rejected_q[5] = {0, 0, 0, 0, 0};
  i64 hull_pruned_q[5] = {0, 0, 0, 0, 0};
  i64 early_rank_supports_q[5] = {0, 0, 0, 0, 0};
  i64 early_rank_groups = 0;
  i64 owner_rejected = 0, self_centre_rejected = 0, rank_rejected = 0;
  i64 ball_groups = 0, group_saved_scans = 0;
  i64 census_scans = 0, census_point_tests = 0, census_promotions = 0;
  i64 supports[5] = {0, 0, 0, 0, 0};
  i64 accepted_closed_rank = 0, extra_shell = 0;
  i64 balls = 0, balls_multi_support = 0;
  i64 shell_high_water = 0;
};

// `l` et `u` a l'echelle 2^(2*depth) : le site est multiplie par 2^depth.
inline void bounds_of(const mhgp::P3& p, const CentreCell& cell, i128* lower, i128* upper) {
  const i64 scale = (i64)1 << cell.depth;
  const i64 coord[3] = {p.x * scale, p.y * scale, p.z * scale};
  i128 lo_acc = 0, hi_acc = 0;
  for (int d = 0; d < 3; ++d) {
    const i64 c = coord[d];
    const i64 gap =
        c < cell.lo[d] ? cell.lo[d] - c : (c > cell.hi[d] ? c - cell.hi[d] : 0);
    lo_acc += (i128)gap * gap;
    const i64 far_lo = c > cell.lo[d] ? c - cell.lo[d] : cell.lo[d] - c;
    const i64 far_hi = c > cell.hi[d] ? c - cell.hi[d] : cell.hi[d] - c;
    const i64 far = far_lo > far_hi ? far_lo : far_hi;
    hi_acc += (i128)far * far;
  }
  *lower = lo_acc;
  *upper = hi_acc;
}

// Le centre rationnel `num/den` (den>0) appartient-il au pave HALF-OPEN de la
// cellule, la face haute de la racine restant fermee ?
inline bool owns_centre(const CentreCell& cell, const i128 num[3], i128 den,
                        const i64 root_hi[3], bool closed_mutant) {
  const i64 scale = (i64)1 << cell.depth;
  for (int d = 0; d < 3; ++d) {
    const i128 left = num[d] * (i128)scale;
    if (left < (i128)cell.lo[d] * den) return false;
    const bool root_face = cell.hi[d] == root_hi[d] * scale;
    if (root_face || closed_mutant) {
      if (left > (i128)cell.hi[d] * den) return false;
    } else {
      if (left >= (i128)cell.hi[d] * den) return false;
    }
  }
  return true;
}

// FILTRE DE BISSECTEUR : le centre d'un support contenant `a` et `b` est sur
// leur plan bissecteur. `g(c)=||c-a||^2-||c-b||^2` est AFFINE en `c`, donc ses
// extrema sur un pave sont aux coins, coordonnee par coordonnee.
inline bool bisector_meets_cell(const mhgp::P3& a, const mhgp::P3& b,
                                const CentreCell& cell, bool strict_mutant) {
  const i64 scale = (i64)1 << cell.depth;
  const i64 diff[3] = {a.x - b.x, a.y - b.y, a.z - b.z};
  const i128 constant =
      (i128)scale * ((i128)a.x * a.x + (i128)a.y * a.y + (i128)a.z * a.z -
                     ((i128)b.x * b.x + (i128)b.y * b.y + (i128)b.z * b.z));
  i128 lo = constant, hi = constant;
  for (int d = 0; d < 3; ++d) {
    const i128 coef = -2 * (i128)diff[d];
    const i128 at_lo = coef * cell.lo[d];
    const i128 at_hi = coef * cell.hi[d];
    lo += at_lo < at_hi ? at_lo : at_hi;
    hi += at_lo < at_hi ? at_hi : at_lo;
  }
  if (strict_mutant) return lo < 0 && 0 < hi;
  return lo <= 0 && 0 <= hi;
}

constexpr int kDopDirs = 4;
constexpr int kDop[kDopDirs][3] = {{1, 1, 1}, {1, 1, -1}, {1, -1, 1}, {-1, 1, 1}};

inline i128 abs128(i128 v) { return v < 0 ? -v : v; }

inline i128 gcd128(i128 a, i128 b) {
  a = abs128(a);
  b = abs128(b);
  while (b != 0) {
    const i128 t = a % b;
    a = b;
    b = t;
  }
  return a;
}

// CLE GEOMETRIQUE DE BOULE. Le centre rationnel reduit est de taille FIXE et
// existe AVANT tout census, contrairement a la coquille `U_B` : c'est la cle
// chaude exigee par l'audit. Deux candidats de meme centre peuvent encore avoir
// deux rayons; ils sont separes ensuite par un test de puissance exact.
struct CentreKey {
  i128 n0 = 0, n1 = 0, n2 = 0, den = 1;
  bool operator<(const CentreKey& o) const {
    if (den != o.den) return den < o.den;
    if (n0 != o.n0) return n0 < o.n0;
    if (n1 != o.n1) return n1 < o.n1;
    return n2 < o.n2;
  }
  bool operator==(const CentreKey& o) const {
    return den == o.den && n0 == o.n0 && n1 == o.n1 && n2 == o.n2;
  }
};

inline CentreKey reduce_centre(const i128 num[3], i128 den) {
  i128 g = den;
  for (int d = 0; d < 3; ++d) g = gcd128(g, num[d]);
  if (g == 0) g = 1;
  return CentreKey{num[0] / g, num[1] / g, num[2] / g, den / g};
}

// FILTRE D'ENVELOPPE. Un support minimal POSITIF a son centre dans
// `relint conv(U)`, donc dans la bbox de `U`. Si cette bbox ne rencontre pas la
// cellule, aucun support positif ne peut etre possede par elle. Le test est
// exact comme REFUS, incremental le long de l'enumeration, et coute six
// comparaisons — contre un lift complet. Il ne se propage PAS au sous-arbre :
// ajouter un point ELARGIT la bbox, donc un sur-ensemble peut redevenir
// admissible; on ne coupe donc que le candidat courant.
struct HullBox {
  i64 mn[3];
  i64 mx[3];
};

inline void hull_seed(const mhgp::P3& p, HullBox* h) {
  h->mn[0] = h->mx[0] = p.x;
  h->mn[1] = h->mx[1] = p.y;
  h->mn[2] = h->mx[2] = p.z;
}

inline void hull_add(const mhgp::P3& p, const HullBox& in, HullBox* out) {
  const i64 v[3] = {p.x, p.y, p.z};
  for (int d = 0; d < 3; ++d) {
    out->mn[d] = in.mn[d] < v[d] ? in.mn[d] : v[d];
    out->mx[d] = in.mx[d] > v[d] ? in.mx[d] : v[d];
  }
}

inline bool hull_meets_cell(const HullBox& h, const CentreCell& cell) {
  const i64 scale = (i64)1 << cell.depth;
  for (int d = 0; d < 3; ++d) {
    if (h.mx[d] * scale < cell.lo[d]) return false;
    if (h.mn[d] * scale > cell.hi[d]) return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// TEST DROITE--CELLULE POUR LA LANE q4.
//
// Legitimation (auditeur, section 5) : tout tetraedre propre POSITIF possede au
// moins une face aigue, donc positive — au moins deux, en fait. Le lieu des
// centres equidistants des trois sommets d'un triangle NON degenere est la
// droite rationnelle normale a son plan passant par son circumcentre. Un q4
// possede par la cellule impose donc que cette droite rencontre la cellule.
// Le triangle n'a PAS besoin d'etre un support q3 pertinent, ni meme positif :
// la condition est purement geometrique, ce qui preserve l'independance des
// lanes.
//
// Le test employe est le theoreme de l'axe separateur droite--pave, sans
// division : la droite `c0 + t n` manque le pave de centre `m` et de demi-cote
// `h` s'il existe `i` tel que, pour `A = n x e_i`,
//   |<c0 - m, A>| > sum_j h_j |A_j|,
// car `<n,A>=0` rend la projection de la droite constante. Le test est exact et
// FAIL-OPEN : en cas de risque de debordement il ne coupe rien.
//
// ARITHMETIQUE. Les coordonnees sont ramenees a une origine entiere `o` proche
// de la cellule. En notant `L` le rayon local et `W` la largeur de la cellule a
// l'echelle `S=2^depth`, la plus grande quantite formee est bornee par
// `41472 S L^7 + 20736 W L^6`. Le garde `L<=4096, depth<=26` la maintient sous
// `2^127`. En pratique `L` vaut quelques dizaines.
struct LocalFrame {
  i64 origin[3] = {0, 0, 0};
  i64 lo[3] = {0, 0, 0};  // cellule a l'echelle S, translatee
  i64 hi[3] = {0, 0, 0};
  i64 scale = 1;
  bool usable = false;
};

inline i64 floor_div_i64(i64 a, i64 b) {
  i64 q = a / b;
  if ((a % b != 0) && ((a < 0) != (b < 0))) --q;
  return q;
}

// La droite normale au triangle, passant par son circumcentre, rencontre-t-elle
// la cellule ? Renvoie `true` si elle la rencontre OU si le test est indecis.
inline bool triangle_axis_meets_cell(const mhgp::P3& a, const V3& p1, const V3& p2,
                                     const TriangleLift& tri, const LocalFrame& frame) {
  if (!frame.usable) return true;
  const i64 nx = p1.y * p2.z - p1.z * p2.y;
  const i64 ny = p1.z * p2.x - p1.x * p2.z;
  const i64 nz = p1.x * p2.y - p1.y * p2.x;
  if (nx == 0 && ny == 0 && nz == 0) return true;  // colineaires : indecis
  if (!tri.lift.ok || tri.gg <= 0) return true;
  const i128 den = tri.gg;  // > 0
  // Numerateur du circumcentre dans le repere local, a l'echelle `den`.
  i128 num[3];
  num[0] = (i128)(a.x - frame.origin[0]) * den + tri.galpha * p1.x + tri.gbeta * p2.x;
  num[1] = (i128)(a.y - frame.origin[1]) * den + tri.galpha * p1.y + tri.gbeta * p2.y;
  num[2] = (i128)(a.z - frame.origin[2]) * den + tri.galpha * p1.z + tri.gbeta * p2.z;
  const i64 nvec[3] = {nx, ny, nz};
  for (int i = 0; i < 3; ++i) {
    // A = n x e_i
    i64 A[3] = {0, 0, 0};
    const int u = (i + 1) % 3, v = (i + 2) % 3;
    A[u] = nvec[v];
    A[v] = -nvec[u];
    if (A[0] == 0 && A[1] == 0 && A[2] == 0) continue;
    // 2*S*den*<c0-m,A> = <2*S*num - (lo+hi)*den, A>
    i128 dot = 0, radius = 0;
    for (int d = 0; d < 3; ++d) {
      const i128 term = 2 * (i128)frame.scale * num[d] -
                        (i128)(frame.lo[d] + frame.hi[d]) * den;
      dot += term * A[d];
      const i128 width = (i128)(frame.hi[d] - frame.lo[d]) * den;
      radius += width * (A[d] < 0 ? -(i128)A[d] : (i128)A[d]);
    }
    const i128 abs_dot = dot < 0 ? -dot : dot;
    if (abs_dot > radius) return false;  // axe separateur exact
  }
  return true;
}

// ---------------------------------------------------------------------------
// FILTRE DE DIAMETRE — un prune de SOUS-ARBRE, pas seulement de candidat.
//
// Tous les membres d'un support sont sur la sphere de centre `c` et de rayon
// carre `beta`. Donc pour tous `x,y` du support, `||x-y||^2 <= 4 beta`. Or
// `beta <= u_C(x)` pour tout membre, donc `beta <= Umin`, l'intersection
// courante des bornes hautes. La condition necessaire exacte est
//
//     max_{x,y in U} ||x-y||^2  <=  4 * Umin(U).
//
// Elle est MONOTONE dans le bon sens : ajouter un point ne peut qu'augmenter le
// diametre et diminuer `Umin`. Un sous-ensemble qui la viole ne peut donc etre
// etendu en aucun support, et tout le sous-arbre serait coupe.
//
// MESURE : REFUTE COMME FILTRE UTILE. Sur `terrain`, `n=1 500`, il ne coupe que
// `126 610` des `19 771 459` tests, soit `0,64 %`. La raison est structurelle :
// la liste `A_p(C)` ne retient deja que des sites dont les distances a la
// cellule sont comparables, donc `max||x-y||^2 <= 4 Umin` y est presque toujours
// vrai. Le prune est donc DESACTIVE et seuls ses compteurs restent, pour que
// cette refutation reste mesurable et ne soit pas reproposee.
//
// ARITHMETIQUE. `||x-y||^2 <= 3*65535^2 < 2^34` et `Umin` est a l'echelle
// `4^depth`. Le produit reste sous `2^34 * 4^26 = 2^86`, donc dans `i128` sans
// repere local.
inline i128 dist2_of(const mhgp::P3& a, const mhgp::P3& b) {
  const i64 dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
  return (i128)dx * dx + (i128)dy * dy + (i128)dz * dz;
}

// Un candidat de la liste d'une cellule, trie par `l` croissant.
struct Cand {
  i128 l = 0, u = 0;
  int id = 0;
  int tau = 0;  // premier bucket `A_p` qui contient ce site
};

// Un support accepte, avec son census GLOBAL exact et sa coquille NON BORNEE.
struct Record {
  std::vector<int> support;
  std::vector<int> interior;
  std::vector<int> shell;  // identite semantique de la boule (lemme L2)
  bool closed_rank_ok = false;
};

// Un candidat ayant passe proprietaire et positivite, en attente de census.
struct Pending {
  CentreKey key;
  Lift lift;
  int ids[4] = {0, 0, 0, 0};
  int q = 0;
  int e = 0;  // stratum initial `max tau`
};

struct Engine {
  const mhgp::P3* pts = nullptr;
  int n = 0;
  int smax = 11;
  int leaf = 4;
  int work_cap = 20000;
  // SONDAGE DU VRAI GRAPHE : desactive par defaut. Il divise les cellules par
  // trois mais multiplie les lifts par un facteur et demi sur ce backend CPU.
  // Le bon reglage est une propriete du BACKEND, pas de la geometrie : un A/B
  // device doit le recalibrer avant tout defaut different.
  int probe_factor = 1;
  int probe_top_cap = 96;
  int max_depth = 22;
  bool axis_filter = false;
  Mutant mutant = Mutant::kNone;
  bool collect = false;
  i64 root_hi[3] = {0, 0, 0};
  Stats stats;
  std::vector<Record> records;
  int invariant_broken = 0;

  std::vector<std::vector<Cand>> level;
  std::vector<i128> scratch_u;
  std::vector<i128> thresholds;  // R_0 .. R_{tmax-1}
  std::vector<Pending> pending;

  // ---------------------------------------------------------------------------
  // LIFT DIFFERE PAR LOT — le RLE `SupportKey` AVANT lift.
  //
  // Un meme tuple geometrique est propose par plusieurs cellules voisines, mais
  // un seul point de l'espace le possede. La mesure de multiplicite montre que
  // ces cellules sont sous un ancetre PROCHE : un lot borne en octets, vide
  // quand il deborde, capte donc l'essentiel de la deduplication sans aucune
  // table globale d'occurrences. Le parcours en profondeur rend un lot a taille
  // plafonnee spatialement coherent par construction.
  //
  // Le lot ne peut etre vide qu'entre deux cellules terminales : si les
  // enregistrements d'une meme cellule etaient coupes, son census tournerait
  // deux fois et publierait deux fois ses supports.
  struct BatchCell {
    CentreCell cell;
    std::vector<Cand> cands;
    std::vector<int> bucket_end;
    bool have_thresholds = false;
    std::vector<Pending> pending;
  };
  struct BatchRec {
    int ids[4] = {0, 0, 0, 0};
    int q = 0;
    int e = 0;
    int slot = 0;
  };
  std::vector<BatchCell> batch_cells;
  std::vector<BatchRec> batch_recs;
  int batch_rec_cap = 1 << 20;
  int cur_slot = -1;
  bool deferred_lift = false;
  // ABLATION A FLOT IDENTIQUE. Mode de MESURE uniquement : chaque niveau coupe
  // une phase en laissant intactes toutes celles qui la precedent. La sortie
  // devient fausse par construction, donc le mode est incompatible avec le juge
  // et avec les mutants. C'est la seule facon d'attribuer le cout a une
  // primitive sans compteur materiel.
  // 0 aucune, 1 sans census, 2 sans lift, 3 sans cliques, 4 sans adjacence,
  // 5 arbre seul.
  int ablate = 0;
  std::vector<Record> cell_records;
  std::vector<int> bucket_end;  // borne de prefixe de chaque `A_p` dans la liste
  std::vector<int> group;
  std::vector<int> interior_ids, shell_ids;
  std::vector<unsigned long long> row_ij, row_ijk;
  // COORDONNEES CONTIGUES DE LA CELLULE. L'enumeration lisait `pts[cands[i].id]`,
  // soit une indirection et un acces aleatoire au nuage entier a chaque visite
  // de clique. Recopier les coordonnees de la liste — quelques dizaines de
  // triplets — les rend sequentielles pour toute la cellule.
  std::vector<mhgp::P3> cell_pts;
  std::vector<char> group_done;
  // HISTOGRAMME DE MULTIPLICITE, exige par le contre-audit. Un meme tuple
  // geometrique est propose dans plusieurs cellules; seul son circumcentre en
  // designe une. On compte les occurrences par `SupportKey` et on separe les
  // issues, pour ne pas confondre multiplicite intercellules et candidats
  // jamais possedes nulle part. Reserve aux petits nuages : la table est un
  // instrument de mesure, jamais une structure du chemin produit.
  bool track_multiplicity = false;
  // issue : 0 degenere, 1 sans owner, 2 non positif, 3 rang, 4 pertinent
  std::map<std::array<int, 4>, std::pair<i64, int>> occurrences;
  std::map<std::array<int, 5>, i64> occurrences_batched;

  void note_occurrence(const int* ids, int q, int issue) {
    if (!track_multiplicity) return;
    std::array<int, 4> key{-1, -1, -1, -1};
    for (int i = 0; i < q; ++i) key[(std::size_t)i] = ids[i];
    auto& slot = occurrences[key];
    ++slot.first;
    if (issue > slot.second) slot.second = issue;
    std::array<int, 5> bkey{key[0], key[1], key[2], key[3], current_batch};
    ++occurrences_batched[bkey];
  }

  // `p` va de 0 a smax-2 pour l'arite deux : il faut donc `smax-1` seuils.
  int tmax() const { return smax - 1; }

  void run(const CentreCell& root, const std::vector<Cand>& seed) {
    level.assign((std::size_t)max_depth + 2, {});
    current_batch = 0;
    batch_counter = 0;
    descend(root, seed);
    if (deferred_lift) flush_batch();
  }

  int batch_depth = 3;
  int batch_counter = 0;
  int current_batch = 0;

  void descend(const CentreCell& cell, const std::vector<Cand>& parent) {
    stats.max_depth_reached = std::max(stats.max_depth_reached, (i64)cell.depth);
    std::vector<Cand>& mine = level[(std::size_t)cell.depth];
    mine.clear();
    mine.reserve(parent.size());
    scratch_u.clear();
    scratch_u.reserve(parent.size());
    for (const Cand& c : parent) {
      ++stats.parent_candidate_reads;
      ++stats.bound_evals;
      i128 lo = 0, hi = 0;
      bounds_of(pts[c.id], cell, &lo, &hi);
      mine.push_back(Cand{lo, hi, c.id, 0});
      scratch_u.push_back(hi);
    }
    const int tm = tmax();
    const bool have_thresholds = (n >= tm) && ((int)mine.size() >= tm);
    // PREMIERE PASSE sur la cellule dyadique. Elle doit preceder le
    // resserrement : la bbox se calcule sur la liste DEJA filtree, sinon elle
    // est inutilement large et tous les filtres avals s'affaiblissent.
    if (have_thresholds) {
      std::nth_element(scratch_u.begin(), scratch_u.begin() + (tm - 1), scratch_u.end());
      const i128 rtop0 = scratch_u[(std::size_t)(tm - 1)];
      std::size_t keep0 = 0;
      for (std::size_t i = 0; i < mine.size(); ++i)
        if (mine[i].l <= rtop0) mine[keep0++] = mine[i];
      mine.resize(keep0);
      if (mine.size() < 2) {
        ++stats.cells_pruned;
        return;
      }
    }

    // RESSERREMENT ET SEPARATION CONVEXE. Le centre de tout support positif
    // possede par `C` est dans `conv(A(C))`, donc dans la bbox de la liste. On
    // remplace `K_C` par `K_C inter bbox` : convexe plus petit contenant encore
    // tous les centres possedes, donc seuils plus serres. La cellule DYADIQUE
    // reste seule autorite pour la propriete et pour les enfants.
    CentreCell tight = cell;
    {
      const i64 scale = (i64)1 << cell.depth;
      i64 smin[3] = {0, 0, 0}, smx[3] = {0, 0, 0};
      bool first = true;
      for (const Cand& c : mine) {
        const mhgp::P3& p = pts[c.id];
        const i64 v[3] = {p.x, p.y, p.z};
        for (int d = 0; d < 3; ++d) {
          if (first) { smin[d] = v[d]; smx[d] = v[d]; }
          else { smin[d] = std::min(smin[d], v[d]); smx[d] = std::max(smx[d], v[d]); }
        }
        first = false;
      }
      ++stats.separation_tests;
      for (int d = 0; d < 3; ++d) {
        tight.lo[d] = std::max(tight.lo[d], smin[d] * scale);
        tight.hi[d] = std::min(tight.hi[d], smx[d] * scale);
        if (tight.lo[d] > tight.hi[d]) {
          ++stats.separation_pruned;
          ++stats.cells_pruned;
          return;
        }
      }
      for (int d = 0; d < kDopDirs; ++d) {
        i64 pmin = 0, pmax = 0;
        bool one = true;
        for (const Cand& c : mine) {
          const mhgp::P3& p = pts[c.id];
          const i64 proj = (i64)kDop[d][0] * p.x + (i64)kDop[d][1] * p.y +
                           (i64)kDop[d][2] * p.z;
          if (one) { pmin = pmax = proj; one = false; }
          else { pmin = std::min(pmin, proj); pmax = std::max(pmax, proj); }
        }
        i64 bmin = 0, bmax = 0;
        for (int a = 0; a < 3; ++a) {
          const i64 at_lo = (i64)kDop[d][a] * tight.lo[a];
          const i64 at_hi = (i64)kDop[d][a] * tight.hi[a];
          bmin += std::min(at_lo, at_hi);
          bmax += std::max(at_lo, at_hi);
        }
        ++stats.separation_tests;
        if (bmax < pmin * scale || bmin > pmax * scale) {
          ++stats.separation_pruned;
          ++stats.cells_pruned;
          return;
        }
      }
      // Deuxieme passe de bornes sur la boite resserree.
      scratch_u.clear();
      for (Cand& c : mine) {
        i128 lo = 0, hi = 0;
        bounds_of(pts[c.id], tight, &lo, &hi);
        ++stats.bound_evals;
        c.l = lo;
        c.u = hi;
        scratch_u.push_back(hi);
      }
    }

    // LES SEUILS PAR PROFONDEUR : les `tmax` plus petites valeurs de `u`.
    thresholds.clear();
    if (have_thresholds) {
      std::partial_sort(scratch_u.begin(), scratch_u.begin() + tm, scratch_u.end());
      thresholds.assign(scratch_u.begin(), scratch_u.begin() + tm);
      if (mutant == Mutant::kTightThreshold)
        for (int p = 0; p < tm; ++p) thresholds[(std::size_t)p] = thresholds[0];
      const i128 rtop = thresholds[(std::size_t)(tm - 1)];
      std::size_t keep = 0;
      for (std::size_t i = 0; i < mine.size(); ++i) {
        const bool ok =
            mutant == Mutant::kDropTies ? (mine[i].l < rtop) : (mine[i].l <= rtop);
        if (ok) mine[keep++] = mine[i];
      }
      mine.resize(keep);
      if (mutant == Mutant::kShrinkList && mine.size() > 4) mine.pop_back();
    }

    stats.candidate_ids += (i64)mine.size();
    stats.candidate_high_water = std::max(stats.candidate_high_water, (i64)mine.size());
    if (mine.size() < 2) {
      ++stats.cells_pruned;
      return;
    }

    std::sort(mine.begin(), mine.end(),
              [](const Cand& a, const Cand& b) { return a.l < b.l; });
    // `tau` et les bornes de prefixe des buckets `A_p`, `l` etant trie.
    bucket_end.assign((std::size_t)std::max(tm, 1), (int)mine.size());
    if (have_thresholds) {
      int p = 0;
      for (std::size_t i = 0; i < mine.size(); ++i) {
        while (p < tm && thresholds[(std::size_t)p] < mine[i].l) {
          bucket_end[(std::size_t)p] = (int)i;
          ++p;
        }
        mine[i].tau = p;
      }
      for (; p < tm; ++p) bucket_end[(std::size_t)p] = (int)mine.size();
    } else {
      for (Cand& c : mine) c.tau = 0;
    }

    // POTENTIEL EXACT DE CLIQUES (auditeur). En triant par borne gauche, soit
    // `a_i` le nombre d'intervalles anterieurs dont la borne droite atteint la
    // borne gauche de `i`. Le nombre EXACT de q-cliques d'intervalles vaut
    // `sum_i C(a_i,q-1)`, calculable par un seul sweep. Il distingue une liste
    // longue mais ORDONNEE — beaucoup de paires, peu de triangles — d'une liste
    // reellement ambigue, ce qu'un simple compte de paires ne fait pas.
    //
    // Un fait utile : `u_j < l_i` entraine `l_j <= u_j < l_i`, donc `j < i`
    // puisque `l` est trie. Le compte global des `u` strictement inferieurs a
    // `l_i` est donc exactement le compte des anterieurs.
    i64 pot_e = 0, pot_t = 0, pot_q = 0;
    {
      const int m = (int)mine.size();
      scratch_u.clear();
      scratch_u.reserve((std::size_t)m);
      for (const Cand& c : mine) scratch_u.push_back(c.u);
      std::sort(scratch_u.begin(), scratch_u.end());
      int j = 0;
      for (int i = 0; i < m; ++i) {
        while (j < m && scratch_u[(std::size_t)j] < mine[(std::size_t)i].l) ++j;
        const i64 a = (i64)i - (i64)j;  // >= 0
        if (a <= 0) continue;
        pot_e += a;
        pot_t += a * (a - 1) / 2;
        pot_q += a * (a - 1) * (a - 2) / 6;
      }
    }
    stats.overlap_pairs += pot_e;
    stats.potential_triples += pot_t;
    stats.potential_quads += pot_q;
    // Poids de travail : un triplet paie un `lift_triangle`, un quadruplet un
    // `lift_of` et son auto-centrage, donc environ trois et six fois une paire.
    const i64 work = pot_e + 3 * pot_t + 6 * pot_q;
    // CRITERE A DEUX ETAGES (auditeur). Le potentiel d'intervalles est un
    // MAJORANT grossier : le graphe d'intervalles est un surgraphe du graphe de
    // bissecteurs 3D. Pres de la racine il explose et force a decouper la ou le
    // vrai graphe est creux — c'est ce qui rendait la pente des cellules rouge.
    // On sonde donc le VRAI graphe dans une bande d'indecision, une seule fois,
    // et on reutilise l'adjacence si la cellule devient terminale.
    adj_ready = false;
    bool terminal = work <= (i64)work_cap || (int)mine.size() <= leaf ||
                    cell.depth >= max_depth;
    if (!terminal && work <= (i64)work_cap * (i64)probe_factor &&
        (int)mine.size() <= probe_top_cap) {
      int m3p = (int)mine.size(), m4p = (int)mine.size();
      if (have_thresholds) {
        const int p3 = smax - 3, p4 = smax - 4;
        if (p3 >= 0 && p3 < (int)bucket_end.size()) m3p = bucket_end[(std::size_t)p3];
        if (p4 >= 0 && p4 < (int)bucket_end.size()) m4p = bucket_end[(std::size_t)p4];
      }
      const int c2p = (int)mine.size();
      const int topp = std::max(c2p, std::max(m3p, m4p));
      build_adjacency(tight, mine, topp);
      i64 real_e = 0, real_t3 = 0, real_t4 = 0, real_q4 = 0;
      real_counts(topp, c2p, m3p, m4p, &real_e, &real_t3, &real_t4, &real_q4);
      ++stats.probe_tests;
      stats.probe_edges += real_e;
      stats.probe_triangles += real_t3;
      stats.probe_quads += real_q4;
      // CONTROLE DE LA BORNE D'INCIDENCE, sous sa forme EXACTE et sans division.
      // Chaque `K4` fournit quatre triangles et un triangle possede au plus
      // `m_4-3` apex, donc `4 Q_4 <= (m_4-3) T_4`. Aucune tolerance : une
      // inegalite stricte est une faute de comptage.
      // Le mutant emule la faute de comptage realiste : un `K4` compte une fois
      // PAR FACE au lieu d'une seule, faute d'orientation des bitsets. Un mutant
      // d'une seule unite ne testerait rien, la borne n'etant pas serree.
      if (mutant == Mutant::kIncidenceOffByOne) real_q4 *= 4;
      ++stats.incidence_checks;
      if ((i128)4 * real_q4 > (i128)std::max(m4p - 3, 0) * real_t4) {
        std::fprintf(stderr,
                     "INVARIANT viole : 4*Q4=%lld > (m4-3)*T4=%lld (m4=%d, T4=%lld)\n",
                     (long long)(4 * real_q4), (long long)((i64)(m4p - 3) * real_t4), m4p,
                     (long long)real_t4);
        invariant_broken = 1;
      }
      if (real_e + 3 * real_t3 + 6 * real_q4 <= (i64)work_cap) {
        terminal = true;
        ++stats.probe_accepted;
      } else {
        adj_ready = false;
      }
    }
    const i64 overlaps = pot_e;
    if (terminal) {
      ++stats.cells_terminal;
      stats.terminal_overlaps += overlaps;
      stats.max_terminal_overlaps = std::max(stats.max_terminal_overlaps, overlaps);
      if (ablate < 5) generate(cell, tight, mine, have_thresholds);
      return;
    }

    ++stats.cells_split;
    for (int octant = 0; octant < 8; ++octant) {
      CentreCell child;
      child.depth = cell.depth + 1;
      bool skip = false;
      for (int d = 0; d < 3 && !skip; ++d) {
        const i64 lo2 = cell.lo[d] * 2, hi2 = cell.hi[d] * 2;
        const i64 mid = (lo2 + hi2) / 2;
        const bool high = ((octant >> d) & 1) != 0;
        if (high) {
          if (lo2 == hi2 || mid == hi2) { skip = true; break; }
          child.lo[d] = mid;
          child.hi[d] = hi2;
        } else {
          child.lo[d] = lo2;
          child.hi[d] = mid;
        }
      }
      if (skip) continue;
      bool owns_nothing = false;
      const i64 scale_child = (i64)1 << child.depth;
      for (int d = 0; d < 3; ++d)
        if (child.lo[d] == child.hi[d] && child.hi[d] != root_hi[d] * scale_child)
          owns_nothing = true;
      if (owns_nothing) continue;
      // SEPARATION AVANT LA LISTE. Le centre de tout support positif possede par
      // l'enfant est dans `conv(A(parent))`, donc dans la boite resserree du
      // parent. Un enfant disjoint de cette boite est vide de supports, et il
      // est inutile de payer ses `|A(parent)|` evaluations de bornes pour le
      // decouvrir ensuite. Le test coute six comparaisons.
      {
        bool disjoint = false;
        for (int d = 0; d < 3 && !disjoint; ++d) {
          if (child.hi[d] < tight.lo[d] * 2) disjoint = true;
          if (child.lo[d] > tight.hi[d] * 2) disjoint = true;
        }
        ++stats.child_separation_tests;
        if (disjoint) {
          ++stats.child_separation_pruned;
          continue;
        }
      }
      ++stats.cells_created;
      const int saved = current_batch;
      if (child.depth == batch_depth) current_batch = ++batch_counter;
      descend(child, mine);
      current_batch = saved;
    }
  }

  // LE GRAPHE DES PAIRES ADMISSIBLES : intervalles compatibles ET bissecteur
  // rencontrant la cellule. Construit une seule fois, puis reutilise soit pour
  // decider le split, soit pour enumerer.
  std::vector<unsigned long long> adj;
  bool adj_ready = false;
  int adj_top = 0;

  void build_adjacency(const CentreCell& tight, const std::vector<Cand>& cands, int top) {
    const int words = (top + 63) / 64;
    adj.assign((std::size_t)top * (std::size_t)words, 0);
    for (int i = 0; i < top; ++i) {
      const i128 u_i = cands[(std::size_t)i].u;
      const mhgp::P3& pi = pts[cands[(std::size_t)i].id];
      for (int j = i + 1; j < top; ++j) {
        if (cands[(std::size_t)j].l > u_i) break;
        ++stats.bisector_tests;
        if (!bisector_meets_cell(pi, pts[cands[(std::size_t)j].id], tight,
                                 mutant == Mutant::kBisectorStrict)) {
          ++stats.bisector_pruned;
          continue;
        }
        adj[(std::size_t)i * (std::size_t)words + (std::size_t)(j >> 6)] |=
            (unsigned long long)1 << (j & 63);
      }
    }
    adj_top = top;
    adj_ready = true;
  }

  // LES VRAIS `E`, `T` ET `Q` DU GRAPHE DE BISSECTEURS, PAR POPCOUNT, ET SANS
  // AUCUN LIFT (auditeur).
  //
  // Chaque compteur emploie SA propre coupe de lane : `E_2` sur `D_9`, `T_3` sur
  // `D_8`, `T_4` et `Q_4` sur `D_7`. Les bitsets etant orientes par identifiant,
  // `popcount(N+(i) inter N+(j))` compte chaque triangle une fois et
  // `popcount(N+(i) inter N+(j) inter N+(k))` compte chaque K4 une fois. Comme
  // `D_7` est la plus courte des trois listes, le comptage exact des K4 y est
  // bon marche : il remplace tout coefficient empirique.
  //
  // La borne d'incidence de l'auditeur, `4 Q_4 <= (m_4-3) T_4`, reste publiee
  // comme controle : chaque K4 fournit quatre triangles et un triangle a au plus
  // `m_4-3` apex.
  void real_counts(int top, int c2, int c3, int c4, i64* edges, i64* tri3, i64* tri4,
                   i64* quads) {
    const int words = (top + 63) / 64;
    i64 e = 0, t3 = 0, t4 = 0, q4 = 0;
    std::vector<unsigned long long> inter((std::size_t)words);
    for (int i = 0; i < c2; ++i) {
      const unsigned long long* ri = &adj[(std::size_t)i * (std::size_t)words];
      for (int wj = 0; wj < words; ++wj) {
        unsigned long long bits = ri[wj];
        while (bits) {
          const int j = (wj << 6) + __builtin_ctzll(bits);
          bits &= bits - 1;
          if (j >= c2) continue;
          ++e;
          if (j >= c3) continue;
          const unsigned long long* rj = &adj[(std::size_t)j * (std::size_t)words];
          for (int w = 0; w < words; ++w) inter[(std::size_t)w] = ri[w] & rj[w];
          for (int w = 0; w < words; ++w) {
            unsigned long long bk = inter[(std::size_t)w];
            while (bk) {
              const int k = (w << 6) + __builtin_ctzll(bk);
              bk &= bk - 1;
              if (k >= c3) continue;
              ++t3;
              if (k >= c4 || j >= c4 || i >= c4) continue;
              ++t4;
              const unsigned long long* rk = &adj[(std::size_t)k * (std::size_t)words];
              for (int w2 = 0; w2 < words; ++w2) {
                unsigned long long bt = inter[(std::size_t)w2] & rk[w2];
                while (bt) {
                  const int t = (w2 << 6) + __builtin_ctzll(bt);
                  bt &= bt - 1;
                  if (t < c4) ++q4;
                }
              }
            }
          }
        }
      }
    }
    *edges = e;
    *tri3 = t3;
    *tri4 = t4;
    *quads = q4;
  }

  void generate(const CentreCell& cell, const CentreCell& tight,
                const std::vector<Cand>& cands, bool have_thresholds) {
    const int m = (int)cands.size();
    if (m < 2) return;
    // BORNES DE LANE : un support d'arite `q` accepte a `p<=smax-q` interieurs,
    // donc tous ses membres ont `tau<=smax-q`. `tau` croit avec `l` trie.
    auto lane_cut = [&](int q) {
      if (!have_thresholds) return m;
      const int p = smax - q;
      if (p < 0) return 0;
      if (p >= (int)bucket_end.size()) return m;
      return bucket_end[(std::size_t)p];
    };
    const int c2 = lane_cut(2), c3 = lane_cut(3), c4 = lane_cut(4);
    const int top = std::max(c2, std::max(c3, c4));
    if (top < 2) return;

    // REPERE LOCAL pour le test droite--cellule. Origine entiere proche de la
    // cellule; le garde `L<=4096` maintient toutes les quantites sous 2^127.
    LocalFrame frame;
    {
      frame.scale = (i64)1 << tight.depth;
      for (int d = 0; d < 3; ++d) {
        frame.origin[d] = floor_div_i64(tight.lo[d], frame.scale);
        frame.lo[d] = tight.lo[d] - frame.origin[d] * frame.scale;
        frame.hi[d] = tight.hi[d] - frame.origin[d] * frame.scale;
      }
      i64 local_max = 0;
      for (int i = 0; i < top; ++i) {
        const mhgp::P3& p = pts[cands[(std::size_t)i].id];
        const i64 v[3] = {p.x - frame.origin[0], p.y - frame.origin[1],
                          p.z - frame.origin[2]};
        for (int d = 0; d < 3; ++d)
          local_max = std::max(local_max, v[d] < 0 ? -v[d] : v[d]);
      }
      i64 width = 0;
      for (int d = 0; d < 3; ++d) width = std::max(width, frame.hi[d] - frame.lo[d]);
      frame.usable = local_max <= 4096 && tight.depth <= 26 && width <= (i64)1 << 40;
      if (!frame.usable) ++stats.axis_unusable;
    }

    const i128 scale4 = (i128)1 << (2 * tight.depth);
    const int words = (top + 63) / 64;
    if (ablate >= 4) return;
    cell_pts.resize((std::size_t)top);
    for (int i = 0; i < top; ++i) cell_pts[(std::size_t)i] = pts[cands[(std::size_t)i].id];
    if (!adj_ready || adj_top != top) build_adjacency(tight, cands, top);
    adj_ready = false;

    // Ouvrir le slot de cette cellule terminale dans le lot courant.
    if (deferred_lift) {
      BatchCell bc;
      bc.cell = cell;
      bc.cands = cands;
      bc.bucket_end = bucket_end;
      bc.have_thresholds = have_thresholds;
      batch_cells.push_back(std::move(bc));
      cur_slot = (int)batch_cells.size() - 1;
    }
    if (ablate >= 3) return;
    pending.clear();
    cell_records.clear();
    // TAMPONS HOISTES. Ces deux lignes de bits etaient allouees a CHAQUE cellule
    // terminale, soit deux allocations tas par cellule; a cent mille cellules
    // c'est un cout comparable a celui des lifts qu'elles servent.
    row_ij.assign((std::size_t)words, 0);
    row_ijk.assign((std::size_t)words, 0);
    int ids[4];
    for (int i = 0; i < top; ++i) {
      const unsigned long long* ri = &adj[(std::size_t)i * (std::size_t)words];
      const i128 u_i = cands[(std::size_t)i].u;
      for (int wj = 0; wj < words; ++wj) {
        unsigned long long bits = ri[wj];
        while (bits) {
          const int j = (wj << 6) + __builtin_ctzll(bits);
          bits &= bits - 1;
          HullBox hi_box, hij, hijk, hijkt;
          hull_seed(cell_pts[(std::size_t)i], &hi_box);
          hull_add(cell_pts[(std::size_t)j], hi_box, &hij);
          const i128 u_ij = std::min(u_i, cands[(std::size_t)j].u);
          const i128 d2_ij = dist2_of(cell_pts[(std::size_t)i], cell_pts[(std::size_t)j]);
          ++stats.diameter_tests;
          if (d2_ij * scale4 > 4 * u_ij) ++stats.diameter_pruned;
          ++stats.hull_tests;
          const bool hull_ij = hull_meets_cell(hij, tight);
          if (!hull_ij) { ++stats.hull_pruned; ++stats.hull_pruned_q[2]; }
          bool pair_kept = false;
          if (j < c2 && hull_ij) {
            ++stats.clique_pairs;
            ids[0] = cands[(std::size_t)i].id;
            ids[1] = cands[(std::size_t)j].id;
            const int e2 = std::max(cands[(std::size_t)i].tau, cands[(std::size_t)j].tau);
            pair_kept = deferred_lift ? record_tuple(ids, 2, e2)
                                      : propose(cell, ids, 2, e2);
          }
          // LANE INDEPENDANTE : le passage a q3 ne consulte JAMAIS `pair_kept`.
          if (mutant == Mutant::kArityCascade && !pair_kept) continue;
          if (j >= c3) continue;
          const unsigned long long* rj = &adj[(std::size_t)j * (std::size_t)words];
          for (int w = 0; w < words; ++w) row_ij[(std::size_t)w] = ri[w] & rj[w];
          bool stop_k = false;
          for (int wk = 0; wk < words && !stop_k; ++wk) {
            unsigned long long bk = row_ij[(std::size_t)wk];
            while (bk) {
              const int k = (wk << 6) + __builtin_ctzll(bk);
              bk &= bk - 1;
              if (k >= c3) { stop_k = true; break; }
              ++stats.clique_triples;
              const i128 u_ijk = std::min(u_ij, cands[(std::size_t)k].u);
              const i128 d2_ijk =
                  std::max(d2_ij,
                           std::max(dist2_of(cell_pts[(std::size_t)i], cell_pts[(std::size_t)k]),
                                    dist2_of(cell_pts[(std::size_t)j], cell_pts[(std::size_t)k])));
              ++stats.diameter_tests;
              if (d2_ijk * scale4 > 4 * u_ijk) ++stats.diameter_pruned;
              hull_add(cell_pts[(std::size_t)k], hij, &hijk);
              ++stats.hull_tests;
              const bool hull_ijk = hull_meets_cell(hijk, tight);
              if (!hull_ijk) { ++stats.hull_pruned; ++stats.hull_pruned_q[3]; }
              ids[0] = cands[(std::size_t)i].id;
              ids[1] = cands[(std::size_t)j].id;
              ids[2] = cands[(std::size_t)k].id;
              const int e3 = std::max(cands[(std::size_t)k].tau,
                                      std::max(cands[(std::size_t)i].tau,
                                               cands[(std::size_t)j].tau));
              const bool tri_kept =
                  hull_ijk ? (deferred_lift ? record_tuple(ids, 3, e3)
                                            : propose(cell, ids, 3, e3))
                           : false;
              if (mutant == Mutant::kArityCascade && !tri_kept) continue;
              if (k >= c4) continue;
              const unsigned long long* rk = &adj[(std::size_t)k * (std::size_t)words];
              bool any_apex = false;
              for (int w = 0; w < words; ++w) {
                row_ijk[(std::size_t)w] = row_ij[(std::size_t)w] & rk[w];
                if (row_ijk[(std::size_t)w]) any_apex = true;
              }
              if (!any_apex) continue;
              // LE FILTRE DE LA LANE q4, applique SEULEMENT quand un apex existe :
              // la droite des centres equidistants du triangle GEOMETRIQUE doit
              // rencontrer la cellule. Aucun verdict de l'arite trois n'est
              // consulte, conformement a l'independance des lanes.
              // VARIANTE A/B, DESACTIVEE PAR DEFAUT sur ce backend CPU. Le
              // filtre est une condition necessaire exacte et fail-open, mais
              // son cout annule son gain ici; « sans division » ne prouve rien
              // sur un device tant qu'aucun kernel n'est mesure.
              if (axis_filter) {
                int sorted3[3] = {ids[0], ids[1], ids[2]};
                std::sort(sorted3, sorted3 + 3);
                const mhgp::P3& ta = pts[sorted3[0]];
                const V3 tp1 = sub(pts[sorted3[1]], ta), tp2 = sub(pts[sorted3[2]], ta);
                const TriangleLift tri =
                    mhgp3v::ballfront::lift_triangle(ta, pts[sorted3[1]], pts[sorted3[2]]);
                ++stats.axis_tests;
                if (!triangle_axis_meets_cell(ta, tp1, tp2, tri, frame)) {
                  ++stats.axis_pruned;
                  continue;
                }
              }
              bool stop_t = false;
              for (int wt = 0; wt < words && !stop_t; ++wt) {
                unsigned long long bt = row_ijk[(std::size_t)wt];
                while (bt) {
                  const int t = (wt << 6) + __builtin_ctzll(bt);
                  bt &= bt - 1;
                  if (t >= c4) { stop_t = true; break; }
                  ++stats.clique_quads;
                  const i128 u_all = std::min(u_ijk, cands[(std::size_t)t].u);
                  const mhgp::P3& pt_t = cell_pts[(std::size_t)t];
                  const i128 d2_all =
                      std::max(d2_ijk,
                               std::max(dist2_of(cell_pts[(std::size_t)i], pt_t),
                                        std::max(dist2_of(cell_pts[(std::size_t)j], pt_t),
                                                 dist2_of(cell_pts[(std::size_t)k], pt_t))));
                  ++stats.diameter_tests;
                  if (d2_all * scale4 > 4 * u_all) ++stats.diameter_pruned;
                  hull_add(cell_pts[(std::size_t)t], hijk, &hijkt);
                  ++stats.hull_tests;
                  if (!hull_meets_cell(hijkt, tight)) {
                    ++stats.hull_pruned;
                    ++stats.hull_pruned_q[4];
                    continue;
                  }
                  ids[0] = cands[(std::size_t)i].id;
                  ids[1] = cands[(std::size_t)j].id;
                  ids[2] = cands[(std::size_t)k].id;
                  ids[3] = cands[(std::size_t)t].id;
                  const int e4 = std::max(e3, cands[(std::size_t)t].tau);
                  if (deferred_lift) record_tuple(ids, 4, e4);
                  else propose(cell, ids, 4, e4);
                }
              }
            }
          }
        }
      }
    }
    if (!deferred_lift) {
      census_pending(cands, have_thresholds);
      close_cell();
      return;
    }
    // La cellule terminale est copiee dans le lot; son census attendra la
    // vidange. Le lot ne se vide qu'ICI, entre deux cellules terminales.
    if (batch_recs.size() >= (std::size_t)batch_rec_cap) flush_batch();
  }

  // Vidange du lot : un lift par tuple DISTINCT, puis choix du proprietaire
  // parmi les cellules qui l'ont propose, puis census par cellule.
  void flush_batch() {
    if (!batch_recs.empty()) {
      std::sort(batch_recs.begin(), batch_recs.end(),
                [](const BatchRec& a, const BatchRec& b) {
                  for (int i = 0; i < 4; ++i)
                    if (a.ids[i] != b.ids[i]) return a.ids[i] < b.ids[i];
                  return a.q < b.q;
                });
      std::size_t i = 0;
      while (i < batch_recs.size()) {
        std::size_t j = i;
        while (j < batch_recs.size() && std::equal(batch_recs[i].ids, batch_recs[i].ids + 4,
                                                   batch_recs[j].ids))
          ++j;
        solve_tuple(i, j);
        i = j;
      }
      batch_recs.clear();
    }
    for (BatchCell& bc : batch_cells) {
      if (bc.pending.empty()) continue;
      pending.swap(bc.pending);
      bucket_end = bc.bucket_end;
      census_pending(bc.cands, bc.have_thresholds);
      close_cell();
      pending.clear();
    }
    batch_cells.clear();
    ++stats.batches_flushed;
  }

  // Un tuple, un lift. Le proprietaire est cherche dans TOUT le run, jamais
  // choisi comme premier enregistrement.
  void solve_tuple(std::size_t begin, std::size_t end) {
    const BatchRec& lead = batch_recs[begin];
    const int q = lead.q;
    int ids[4] = {0, 0, 0, 0};
    for (int i = 0; i < q; ++i) ids[i] = lead.ids[i];
    i128 num[3] = {0, 0, 0};
    i128 den = 0;
    Lift lift;
    bool positive = false;
    ++stats.lifts_built;
    ++stats.lifts_q[q];
    if (!solve_geometry(ids, q, num, &den, &lift, &positive)) {
      ++stats.degenerate_lifts;
      ++stats.degenerate_q[q];
      stats.occurrences_degenerate += (i64)(end - begin);
      return;
    }
    if (!positive) {
      ++stats.self_centre_rejected;
      ++stats.positive_rejected_q[q];
      stats.occurrences_nonpositive += (i64)(end - begin);
      return;
    }
    int winner = -1;
    for (std::size_t r = begin; r < end; ++r) {
      const BatchRec& rec = batch_recs[r];
      BatchCell& bc = batch_cells[(std::size_t)rec.slot];
      ++stats.owner_tests;
      if (owns_centre(bc.cell, num, den, root_hi, mutant == Mutant::kOwnerClosed)) {
        Pending p;
        p.key = reduce_centre(num, den);
        p.lift = lift;
        std::memcpy(p.ids, ids, sizeof(int) * (std::size_t)q);
        p.q = q;
        p.e = rec.e;
        bc.pending.push_back(p);
        if (winner < 0) winner = (int)r; else ++stats.owner_multiple;
      }
    }
    if (winner < 0) {
      ++stats.owner_rejected;
      ++stats.owner_rejected_q[q];
      stats.occurrences_unowned += (i64)(end - begin);
    }
  }

  // Geometrie exacte d'un tuple : centre rationnel, positivite et forme liftee.
  bool solve_geometry(const int* ids, int q, i128 num[3], i128* den_out, Lift* lift_out,
                      bool* positive) {
    const mhgp::P3& a = pts[ids[0]];
    i128 den = 0;
    if (q == 2) {
      const mhgp::P3& b = pts[ids[1]];
      *lift_out = mhgp3v::ballfront::lift_pair(a, b);
      if (!lift_out->ok) return false;
      num[0] = (i128)(a.x + b.x);
      num[1] = (i128)(a.y + b.y);
      num[2] = (i128)(a.z + b.z);
      den = 2;
      *positive = true;
    } else if (q == 3) {
      const TriangleLift tri = mhgp3v::ballfront::lift_triangle(a, pts[ids[1]], pts[ids[2]]);
      if (!tri.lift.ok || tri.gg <= 0) return false;
      *lift_out = tri.lift;
      const V3 p1 = sub(pts[ids[1]], a);
      const V3 p2 = sub(pts[ids[2]], a);
      num[0] = (i128)a.x * tri.gg + tri.galpha * p1.x + tri.gbeta * p2.x;
      num[1] = (i128)a.y * tri.gg + tri.galpha * p1.y + tri.gbeta * p2.y;
      num[2] = (i128)a.z * tri.gg + tri.galpha * p1.z + tri.gbeta * p2.z;
      den = tri.gg;
      *positive = tri.galpha > 0 && tri.gbeta > 0 && (tri.galpha + tri.gbeta) < tri.gg;
    } else {
      const Lift lift =
          mhgp3v::ballfront::lift_of(a, pts[ids[1]], pts[ids[2]], pts[ids[3]]);
      if (!lift.ok) return false;
      *lift_out = lift;
      den = 2 * lift.dd;
      i128 cx = -lift.cx, cy = -lift.cy, cz = -lift.cz;
      if (den < 0) { den = -den; cx = -cx; cy = -cy; cz = -cz; }
      num[0] = (i128)a.x * den + cx;
      num[1] = (i128)a.y * den + cy;
      num[2] = (i128)a.z * den + cz;
      const V3 p1 = sub(pts[ids[1]], a), p2 = sub(pts[ids[2]], a), p3 = sub(pts[ids[3]], a);
      const i128 lx = lift.cx, ly = lift.cy, lz = lift.cz;
      auto det_c = [&](const V3& u, const V3& v) -> i128 {
        return lx * ((i128)u.y * v.z - (i128)u.z * v.y) -
               ly * ((i128)u.x * v.z - (i128)u.z * v.x) +
               lz * ((i128)u.x * v.y - (i128)u.y * v.x);
      };
      const i128 na = -det_c(p2, p3), nb = det_c(p1, p3), nc = -det_c(p1, p2);
      *positive = na > 0 && nb > 0 && nc > 0 && (na + nb + nc) < 2 * lift.dd * lift.dd;
    }
    *den_out = den;
    return true;
  }

  // ENREGISTREMENT SANS LIFT. Le tuple est seulement note avec sa cellule; la
  // geometrie est calculee une fois par tuple distinct a la vidange du lot.
  bool record_tuple(const int* raw, int q, int e) {
    BatchRec r;
    std::memcpy(r.ids, raw, sizeof(int) * (std::size_t)q);
    std::sort(r.ids, r.ids + q);
    for (int i = q; i < 4; ++i) r.ids[i] = -1;
    r.q = q;
    r.e = e;
    r.slot = cur_slot;
    batch_recs.push_back(r);
    ++stats.occurrences_recorded;
    return true;
  }

  // UN SEUL LIFT PAR TUPLE DISTINCT DU LOT : centre rationnel, auto-centrage et
  // forme de puissance proviennent du meme calcul.
  bool propose(const CentreCell& cell, const int* raw, int q, int e,
               const TriangleLift* tri_in = nullptr) {
    int ids[4];
    std::memcpy(ids, raw, sizeof(int) * (std::size_t)q);
    std::sort(ids, ids + q);
    i128 num[3] = {0, 0, 0};
    i128 den = 0;
    Lift lift;
    bool positive = false;
    ++stats.lifts_built;
    ++stats.lifts_q[q];
    ++stats.occurrences_recorded;
    if (ablate >= 2) return false;
    const mhgp::P3& a = pts[ids[0]];
    if (q == 2) {
      const mhgp::P3& b = pts[ids[1]];
      lift = mhgp3v::ballfront::lift_pair(a, b);
      if (!lift.ok) { ++stats.degenerate_lifts; ++stats.degenerate_q[q]; return false; }
      num[0] = (i128)(a.x + b.x);
      num[1] = (i128)(a.y + b.y);
      num[2] = (i128)(a.z + b.z);
      den = 2;
      positive = true;
    } else if (q == 3) {
      const TriangleLift tri =
          tri_in ? *tri_in : mhgp3v::ballfront::lift_triangle(a, pts[ids[1]], pts[ids[2]]);
      if (!tri.lift.ok || tri.gg <= 0) { ++stats.degenerate_lifts; ++stats.degenerate_q[q]; return false; }
      lift = tri.lift;
      const V3 p1 = sub(pts[ids[1]], a);
      const V3 p2 = sub(pts[ids[2]], a);
      num[0] = (i128)a.x * tri.gg + tri.galpha * p1.x + tri.gbeta * p2.x;
      num[1] = (i128)a.y * tri.gg + tri.galpha * p1.y + tri.gbeta * p2.y;
      num[2] = (i128)a.z * tri.gg + tri.galpha * p1.z + tri.gbeta * p2.z;
      den = tri.gg;
      positive = tri.galpha > 0 && tri.gbeta > 0 && (tri.galpha + tri.gbeta) < tri.gg;
    } else {
      lift = mhgp3v::ballfront::lift_of(a, pts[ids[1]], pts[ids[2]], pts[ids[3]]);
      if (!lift.ok) { ++stats.degenerate_lifts; ++stats.degenerate_q[q]; return false; }
      den = 2 * lift.dd;
      i128 cx = -lift.cx, cy = -lift.cy, cz = -lift.cz;
      if (den < 0) { den = -den; cx = -cx; cy = -cy; cz = -cz; }
      num[0] = (i128)a.x * den + cx;
      num[1] = (i128)a.y * den + cy;
      num[2] = (i128)a.z * den + cz;
      const V3 p1 = sub(pts[ids[1]], a), p2 = sub(pts[ids[2]], a), p3 = sub(pts[ids[3]], a);
      const i128 lx = lift.cx, ly = lift.cy, lz = lift.cz;
      auto det_c = [&](const V3& u, const V3& v) -> i128 {
        return lx * ((i128)u.y * v.z - (i128)u.z * v.y) -
               ly * ((i128)u.x * v.z - (i128)u.z * v.x) +
               lz * ((i128)u.x * v.y - (i128)u.y * v.x);
      };
      const i128 na = -det_c(p2, p3), nb = det_c(p1, p3), nc = -det_c(p1, p2);
      positive = na > 0 && nb > 0 && nc > 0 && (na + nb + nc) < 2 * lift.dd * lift.dd;
    }
    if (!owns_centre(cell, num, den, root_hi, mutant == Mutant::kOwnerClosed)) {
      ++stats.owner_rejected;
      ++stats.owner_rejected_q[q];
      note_occurrence(ids, q, 1);
      return false;
    }
    if (!positive) {
      ++stats.self_centre_rejected;
      ++stats.positive_rejected_q[q];
      note_occurrence(ids, q, 2);
      return false;
    }
    Pending p;
    p.key = reduce_centre(num, den);
    p.lift = lift;
    std::memcpy(p.ids, ids, sizeof(int) * (std::size_t)q);
    p.q = q;
    p.e = e;
    note_occurrence(ids, q, 4);
    pending.push_back(p);
    return true;
  }

  // CENSUS UNE FOIS PAR BOULE, STRATIFIE PAR PROFONDEUR.
  // Les candidats sont d'abord groupes par cle geometrique de taille fixe, puis
  // separes par un test de puissance exact (meme centre, rayons differents).
  void census_pending(const std::vector<Cand>& cands, bool have_thresholds) {
    if (pending.empty()) return;
    std::sort(pending.begin(), pending.end(), [](const Pending& a, const Pending& b) {
      if (!(a.key == b.key)) return a.key < b.key;
      return a.q < b.q;
    });
    std::size_t i = 0;
    while (i < pending.size()) {
      std::size_t j = i;
      while (j < pending.size() && pending[j].key == pending[i].key) ++j;
      group_done.assign(j - i, 0);
      std::vector<char>& done = group_done;
      for (std::size_t a = i; a < j; ++a) {
        if (done[a - i]) continue;
        group.clear();
        group.push_back((int)a);
        for (std::size_t b = a + 1; b < j; ++b) {
          if (done[b - i]) continue;
          if (mhgp3v::ballfront::power_of(pending[a].lift, pts[pending[b].ids[0]]) == 0) {
            done[b - i] = 1;
            group.push_back((int)b);
          }
        }
        ++stats.ball_groups;
        stats.group_saved_scans += (i64)group.size() - 1;
        census_group(cands, have_thresholds);
      }
      i = j;
    }
    pending.clear();
  }

  void census_group(const std::vector<Cand>& cands, bool have_thresholds) {
    const Pending& lead = pending[(std::size_t)group[0]];
    int qmin = 5;
    int e = 0;
    for (int gi : group) {
      qmin = std::min(qmin, pending[(std::size_t)gi].q);
      e = std::max(e, pending[(std::size_t)gi].e);
    }
    const int budget = smax - qmin;
    const int m = (int)cands.size();
    auto prefix_of = [&](int p) {
      if (!have_thresholds) return m;
      if (p >= (int)bucket_end.size()) return m;
      return bucket_end[(std::size_t)p];
    };
    if (ablate >= 1) return;
    ++stats.census_scans;
    interior_ids.clear();
    shell_ids.clear();
    int scanned = 0;
    int interior = 0;
    const int e_start = e;
    for (;;) {
      const int limit = prefix_of(e);
      for (int idx = scanned; idx < limit; ++idx) {
        ++stats.census_point_tests;
        const i128 pw =
            mhgp3v::ballfront::power_of(lead.lift, pts[cands[(std::size_t)idx].id]);
        if (pw > 0) {
          interior_ids.push_back(cands[(std::size_t)idx].id);
          ++interior;
          // ABANDON ANTICIPE : au-dela du budget, aucun support du groupe ne
          // peut etre accepte. Aucun shell partiel n'est publie. Le ledger doit
          // ATTRIBUER chaque occurrence du groupe, sinon la partition ne ferme
          // pas — c'est le defaut exact releve par le contre-audit.
          if (interior > budget) {
            ++stats.rank_rejected;
            ++stats.early_rank_groups;
            for (int gi : group) ++stats.early_rank_supports_q[pending[(std::size_t)gi].q];
            return;
          }
        } else if (pw == 0) {
          shell_ids.push_back(cands[(std::size_t)idx].id);
        }
      }
      scanned = limit;
      if (interior <= e) break;
      if (mutant == Mutant::kStrataStop) break;
      ++stats.census_promotions;
      e = interior;
      if (e > budget) {
        ++stats.rank_rejected;
        ++stats.early_rank_groups;
        for (int gi : group) ++stats.early_rank_supports_q[pending[(std::size_t)gi].q];
        return;
      }
    }
    // INVARIANT DU LEMME, VERIFIE A LA FERMETURE. Le curseur `h` s'arrete quand
    // `r_h<=h`; le lemme impose alors l'EGALITE `r_h==h`, car un membre au moins
    // du support n'est pas dans `A_{h-1}`, donc `beta>R_{h-1}` et les `h`
    // temoins de `R_{h-1}` sont interieurs stricts. Une inegalite stricte
    // signalerait un seuil ou un bucket faux.
    if (have_thresholds && mutant == Mutant::kNone && interior != e) {
      std::fprintf(stderr, "INVARIANT viole : r=%d != h=%d (entree e0=%d)\n", interior, e,
                   e_start);
      invariant_broken = 1;
    }
    const int shell = (int)shell_ids.size();
    stats.shell_high_water = std::max(stats.shell_high_water, (i64)shell);
    std::sort(interior_ids.begin(), interior_ids.end());
    std::sort(shell_ids.begin(), shell_ids.end());
    for (int gi : group) {
      const Pending& p = pending[(std::size_t)gi];
      if (interior + p.q > smax) {
        ++stats.rank_rejected;
        ++stats.rank_rejected_q[p.q];
        continue;
      }
      if (mutant == Mutant::kRankClosed && interior + shell > smax) {
        ++stats.rank_rejected;
        continue;
      }
      ++stats.supports[p.q];
      Record rec;
      rec.support.assign(p.ids, p.ids + p.q);
      rec.interior = interior_ids;
      rec.shell = shell_ids;
      rec.closed_rank_ok = (interior + shell) <= smax;
      if (rec.closed_rank_ok) ++stats.accepted_closed_rank; else ++stats.extra_shell;
      cell_records.push_back(rec);
    }
  }

  void close_cell() {
    if (cell_records.empty()) return;
    std::sort(cell_records.begin(), cell_records.end(),
              [](const Record& a, const Record& b) {
                if (a.shell != b.shell) return a.shell < b.shell;
                return a.support < b.support;
              });
    std::size_t i = 0;
    while (i < cell_records.size()) {
      std::size_t j = i;
      while (j < cell_records.size() && cell_records[j].shell == cell_records[i].shell) ++j;
      ++stats.balls;
      if (j - i > 1) ++stats.balls_multi_support;
      i = j;
    }
    if (collect) records.insert(records.end(), cell_records.begin(), cell_records.end());
    cell_records.clear();
  }
};

// ---------------------------------------------------------------------------
// Verite terrain exhaustive : tous les sous-ensembles, census sur TOUT le nuage,
// meme contrat d'acceptation `p+q<=smax`.
// ---------------------------------------------------------------------------
bool truth_positive(const mhgp::P3* pts, const int* ids, int q) {
  const mhgp::P3& a = pts[ids[0]];
  if (q == 2) {
    const mhgp::P3& b = pts[ids[1]];
    return !(a.x == b.x && a.y == b.y && a.z == b.z);
  }
  if (q == 3) {
    const TriangleLift tri = mhgp3v::ballfront::lift_triangle(a, pts[ids[1]], pts[ids[2]]);
    if (!tri.lift.ok || tri.gg <= 0) return false;
    return tri.galpha > 0 && tri.gbeta > 0 && (tri.galpha + tri.gbeta) < tri.gg;
  }
  const Lift lift = mhgp3v::ballfront::lift_of(a, pts[ids[1]], pts[ids[2]], pts[ids[3]]);
  if (!lift.ok) return false;
  const V3 p1 = sub(pts[ids[1]], a), p2 = sub(pts[ids[2]], a), p3 = sub(pts[ids[3]], a);
  const i128 cx = lift.cx, cy = lift.cy, cz = lift.cz;
  auto det_c = [&](const V3& u, const V3& v) -> i128 {
    return cx * ((i128)u.y * v.z - (i128)u.z * v.y) -
           cy * ((i128)u.x * v.z - (i128)u.z * v.x) +
           cz * ((i128)u.x * v.y - (i128)u.y * v.x);
  };
  const i128 na = -det_c(p2, p3), nb = det_c(p1, p3), nc = -det_c(p1, p2);
  return na > 0 && nb > 0 && nc > 0 && (na + nb + nc) < 2 * lift.dd * lift.dd;
}

bool truth_lift(const mhgp::P3* pts, const int* ids, int q, Lift* out) {
  if (q == 2) *out = mhgp3v::ballfront::lift_pair(pts[ids[0]], pts[ids[1]]);
  else if (q == 3)
    *out = mhgp3v::ballfront::lift_triangle(pts[ids[0]], pts[ids[1]], pts[ids[2]]).lift;
  else
    *out = mhgp3v::ballfront::lift_of(pts[ids[0]], pts[ids[1]], pts[ids[2]], pts[ids[3]]);
  return out->ok;
}

void ground_truth(const std::vector<mhgp::P3>& cloud, int smax,
                  std::map<std::vector<int>, Record>* out) {
  const int n = (int)cloud.size();
  int ids[4];
  for (int q = 2; q <= 4; ++q) {
    std::vector<int> pick((std::size_t)q);
    for (int i = 0; i < q; ++i) pick[(std::size_t)i] = i;
    for (;;) {
      for (int i = 0; i < q; ++i) ids[i] = pick[(std::size_t)i];
      do {
        if (!truth_positive(cloud.data(), ids, q)) break;
        Lift lift;
        if (!truth_lift(cloud.data(), ids, q, &lift)) break;
        int interior = 0;
        std::vector<int> in_ids, sh_ids;
        bool over = false;
        for (int id = 0; id < n; ++id) {
          const i128 p = mhgp3v::ballfront::power_of(lift, cloud[(std::size_t)id]);
          if (p > 0) {
            ++interior;
            in_ids.push_back(id);
            if (interior + q > smax) { over = true; break; }
          } else if (p == 0) {
            sh_ids.push_back(id);
          }
        }
        if (over) break;
        Record rec;
        rec.support.assign(ids, ids + q);
        rec.interior = in_ids;
        rec.shell = sh_ids;
        rec.closed_rank_ok = (interior + (int)sh_ids.size()) <= smax;
        (*out)[rec.support] = rec;
      } while (false);
      int i = q - 1;
      while (i >= 0 && pick[(std::size_t)i] == n - q + i) --i;
      if (i < 0) break;
      ++pick[(std::size_t)i];
      for (int j = i + 1; j < q; ++j) pick[(std::size_t)j] = pick[(std::size_t)(j - 1)] + 1;
    }
  }
}

struct Options {
  int smax = 11, leaf = 4, work_cap = 20000, max_depth = 22;
  bool axis_filter = false;
  bool multiplicity = false;
  int batch_depth = 3;
  int batch_records = 1 << 20;
  bool deferred_lift = false;
  int probe_factor = 1;
  int ablate = 0;
  Mutant mutant = Mutant::kNone;
  bool judge = false;
  i64 min_supports = 0, min_cells = 0, min_quads = 0, min_probes = 0;
  std::string label;
};

int run_engine(const std::vector<mhgp::P3>& cloud, const Options& opt) {
  const int points = (int)cloud.size();
  Engine engine;
  engine.pts = cloud.data();
  engine.n = points;
  engine.smax = opt.smax;
  engine.leaf = opt.leaf;
  engine.work_cap = opt.work_cap;
  engine.axis_filter = opt.axis_filter;
  engine.track_multiplicity = opt.multiplicity;
  engine.batch_depth = opt.batch_depth;
  engine.batch_rec_cap = opt.batch_records < 1024 ? 1024 : opt.batch_records;
  engine.deferred_lift = opt.deferred_lift;
  engine.probe_factor = opt.probe_factor < 1 ? 1 : opt.probe_factor;
  engine.ablate = opt.ablate;
  engine.max_depth = opt.max_depth;
  engine.mutant = opt.mutant;
  engine.collect = opt.judge;

  CentreCell root;
  root.depth = 0;
  root.lo[0] = root.hi[0] = cloud[0].x;
  root.lo[1] = root.hi[1] = cloud[0].y;
  root.lo[2] = root.hi[2] = cloud[0].z;
  for (const mhgp::P3& p : cloud) {
    const i64 c[3] = {p.x, p.y, p.z};
    for (int d = 0; d < 3; ++d) {
      root.lo[d] = std::min(root.lo[d], c[d]);
      root.hi[d] = std::max(root.hi[d], c[d]);
    }
  }
  for (int d = 0; d < 3; ++d) engine.root_hi[d] = root.hi[d];

  std::vector<Cand> seed;
  seed.reserve(cloud.size());
  for (int i = 0; i < points; ++i) seed.push_back(Cand{0, 0, i, 0});
  ++engine.stats.cells_created;
  engine.run(root, seed);

  const Stats& s = engine.stats;
  std::printf("CentreCellReceipt-v3\n");
  std::printf("cloud=%s points=%d smax=%d leaf=%d work_cap=%d max_depth=%d axis_filter=%d deferred_lift=%d inject=%s\n",
              opt.label.c_str(), points, opt.smax, opt.leaf, opt.work_cap, opt.max_depth,
              opt.axis_filter ? 1 : 0, opt.deferred_lift ? 1 : 0, mutant_name(opt.mutant));
  std::printf("cells_created=%lld split=%lld terminal=%lld pruned=%lld depth_max=%lld\n",
              s.cells_created, s.cells_split, s.cells_terminal, s.cells_pruned,
              s.max_depth_reached);
  std::printf("parent_candidate_reads=%lld bound_evals=%lld candidate_ids=%lld high_water=%lld\n",
              s.parent_candidate_reads, s.bound_evals, s.candidate_ids,
              s.candidate_high_water);
  std::printf("separation_tests=%lld separation_pruned=%lld child_sep_tests=%lld child_sep_pruned=%lld\n",
              s.separation_tests, s.separation_pruned, s.child_separation_tests,
              s.child_separation_pruned);
  std::printf("overlap_pairs=%lld potential_triples=%lld potential_quads=%lld\n",
              s.overlap_pairs, s.potential_triples, s.potential_quads);
  std::printf("terminal_overlaps=%lld max_terminal_overlaps=%lld\n",
              s.terminal_overlaps, s.max_terminal_overlaps);
  std::printf("clique_pairs=%lld clique_triples=%lld clique_quads=%lld\n", s.clique_pairs,
              s.clique_triples, s.clique_quads);
  std::printf("bisector_tests=%lld bisector_pruned=%lld hull_tests=%lld hull_pruned=%lld\n",
              s.bisector_tests, s.bisector_pruned, s.hull_tests, s.hull_pruned);
  std::printf("diameter_tests=%lld diameter_pruned=%lld\n", s.diameter_tests,
              s.diameter_pruned);
  std::printf("occurrences=%lld lifts_distincts=%lld facteur_rle=%.3f owner_tests=%lld owner_multiple=%lld batches=%lld\n",
              s.occurrences_recorded, s.lifts_built,
              s.lifts_built ? (double)s.occurrences_recorded / (double)s.lifts_built : 0.0,
              s.owner_tests, s.owner_multiple, s.batches_flushed);
  std::printf("occ_unowned=%lld occ_nonpositive=%lld occ_degenerate=%lld\n",
              s.occurrences_unowned, s.occurrences_nonpositive, s.occurrences_degenerate);
  std::printf("probe_tests=%lld probe_accepted=%lld probe_edges=%lld probe_triangles=%lld probe_quads=%lld\n",
              s.probe_tests, s.probe_accepted, s.probe_edges, s.probe_triangles,
              s.probe_quads);
  std::printf("incidence_checks=%lld\n", s.incidence_checks);
  std::printf("axis_tests=%lld axis_pruned=%lld axis_unusable=%lld\n", s.axis_tests,
              s.axis_pruned, s.axis_unusable);
  {
    // LA PARTITION DOIT FERMER : pour chaque arite,
    // lifts = degeneres + owner + positivite + acceptes + rang final + rang anticipe.
    // Les prunes d'enveloppe sont AVANT le lift et restent hors de la partition.
    for (int q = 2; q <= 4; ++q) {
      const i64 sum = s.degenerate_q[q] + s.owner_rejected_q[q] + s.positive_rejected_q[q] +
                      s.supports[q] + s.rank_rejected_q[q] + s.early_rank_supports_q[q];
      std::printf("ledger_q%d lifts=%lld degenerate=%lld owner_rejected=%lld positive_rejected=%lld accepted=%lld rank_final=%lld rank_early=%lld somme=%lld ecart=%lld hull_pruned=%lld\n",
                  q, s.lifts_q[q], s.degenerate_q[q], s.owner_rejected_q[q],
                  s.positive_rejected_q[q], s.supports[q], s.rank_rejected_q[q],
                  s.early_rank_supports_q[q], sum, s.lifts_q[q] - sum, s.hull_pruned_q[q]);
    }
    std::printf("early_rank_groups=%lld\n", s.early_rank_groups);
  }
  if (opt.multiplicity) {
    // L'IDENTITE DOIT FERMER : la somme des multiplicites vaut le nombre
    // d'occurrences. Les issues sont separees pour distinguer la multiplicite
    // intercellules d'un tuple possede quelque part, de celle d'un tuple qui
    // n'est possede nulle part.
    i64 total = 0;
    const char* names[5] = {"degenere", "sans_owner", "non_positif", "rang", "pertinent"};
    for (int issue = 0; issue <= 4; ++issue) {
      std::vector<i64> mult;
      for (const auto& e : engine.occurrences)
        if (e.second.second == issue) mult.push_back(e.second.first);
      if (mult.empty()) {
        std::printf("multiplicite issue=%s cles=0\n", names[issue]);
        continue;
      }
      std::sort(mult.begin(), mult.end());
      i64 sum = 0;
      for (i64 v : mult) sum += v;
      total += sum;
      const std::size_t p50 = mult.size() / 2;
      const std::size_t p95 = (mult.size() * 95) / 100 >= mult.size()
                                  ? mult.size() - 1
                                  : (mult.size() * 95) / 100;
      std::printf("multiplicite issue=%s cles=%zu occurrences=%lld moyenne=%.3f p50=%lld p95=%lld max=%lld\n",
                  names[issue], mult.size(), sum, (double)sum / (double)mult.size(),
                  mult[p50], mult[p95], mult.back());
    }
    std::printf("multiplicite_total_occurrences=%lld lifts=%lld ecart=%lld\n", total,
                s.lifts_built, s.lifts_built - total);
    // RENDEMENT D'UNE DEDUPLICATION PAR LOT. Un lot est le sous-arbre enracine a
    // `batch_depth`. Si les occurrences d'un meme tuple tombent majoritairement
    // sous un meme ancetre, un RLE local suffit et aucune table globale n'est
    // necessaire; sinon la voie `SupportKey-before-lift` doit etre globale.
    std::printf("dedup batch_depth=%d lots=%d cles_globales=%zu cles_par_lot=%zu facteur_global=%.3f facteur_lot=%.3f\n",
                engine.batch_depth, engine.batch_counter, engine.occurrences.size(),
                engine.occurrences_batched.size(),
                (double)total / (double)engine.occurrences.size(),
                (double)total / (double)engine.occurrences_batched.size());
  }
  std::printf("lifts_built=%lld degenerate=%lld\n", s.lifts_built, s.degenerate_lifts);
  std::printf("owner_rejected=%lld self_centre_rejected=%lld rank_rejected=%lld\n",
              s.owner_rejected, s.self_centre_rejected, s.rank_rejected);
  std::printf("ball_groups=%lld group_saved_scans=%lld census_scans=%lld census_point_tests=%lld census_promotions=%lld\n",
              s.ball_groups, s.group_saved_scans, s.census_scans, s.census_point_tests,
              s.census_promotions);
  std::printf("supports_q2=%lld supports_q3=%lld supports_q4=%lld supports_total=%lld\n",
              s.supports[2], s.supports[3], s.supports[4],
              s.supports[2] + s.supports[3] + s.supports[4]);
  std::printf("accepted_closed_rank=%lld extra_shell=%lld balls=%lld balls_multi_support=%lld shell_high_water=%lld\n",
              s.accepted_closed_rank, s.extra_shell, s.balls, s.balls_multi_support,
              s.shell_high_water);
  const long long total = s.supports[2] + s.supports[3] + s.supports[4];
  std::printf("supports_per_point=%.4f\n", (double)total / (double)points);

  if (engine.invariant_broken) {
    std::fprintf(stderr, "INVARIANT du lemme de profondeur viole\n");
    return 3;
  }
  if (total < opt.min_supports || s.cells_terminal < opt.min_cells ||
      s.clique_quads < opt.min_quads || s.incidence_checks < opt.min_probes) {
    std::fprintf(stderr,
                 "PLANCHER viole : supports=%lld<%lld cells=%lld<%lld quads=%lld<%lld sondes=%lld<%lld\n",
                 total, opt.min_supports, s.cells_terminal, opt.min_cells, s.clique_quads,
                 opt.min_quads, s.incidence_checks, opt.min_probes);
    return 3;
  }
  if (!opt.judge) {
    if (opt.mutant != Mutant::kNone) {
      std::fprintf(stderr, "REFUS : un mutant sans juge ne prouve rien\n");
      return 2;
    }
    return 0;
  }

  std::map<std::vector<int>, Record> truth;
  ground_truth(cloud, opt.smax, &truth);
  std::map<std::vector<int>, Record> mine;
  int duplicated = 0;
  for (const Record& r : engine.records) {
    if (mine.find(r.support) != mine.end()) ++duplicated;
    mine[r.support] = r;
  }
  int missing = 0, spurious = 0, mismatched = 0;
  for (const auto& entry : truth) {
    auto it = mine.find(entry.first);
    if (it == mine.end()) { ++missing; continue; }
    if (it->second.interior != entry.second.interior ||
        it->second.shell != entry.second.shell ||
        it->second.closed_rank_ok != entry.second.closed_rank_ok)
      ++mismatched;
  }
  for (const auto& entry : mine)
    if (truth.find(entry.first) == truth.end()) ++spurious;
  std::printf("judge_truth=%d judge_mine=%d missing=%d spurious=%d mismatched=%d duplicated=%d\n",
              (int)truth.size(), (int)mine.size(), missing, spurious, mismatched, duplicated);
  const bool disagree = missing || spurious || mismatched || duplicated;
  if (disagree) {
    if (opt.mutant != Mutant::kNone) {
      std::printf("MUTANT TUE: %s\n", mutant_name(opt.mutant));
      return 4;
    }
    std::fprintf(stderr, "DESACCORD juge\n");
    return 1;
  }
  if (opt.mutant != Mutant::kNone) {
    std::fprintf(stderr, "MUTANT SURVIVANT: %s\n", mutant_name(opt.mutant));
    return 3;
  }
  std::printf("ACCORD juge sur les identites (support, I_B, U_B, rang ferme)\n");
  return 0;
}

// ---------------------------------------------------------------------------
// FIXTURES GRAVEES
// ---------------------------------------------------------------------------

// `egalite_beta_egale_R` : cellule singleton en (10,10,10), support q2
// (9,10,10),(11,10,10). Les deux sites ont `u_C=1`, donc `R_1=1`, et leur `l_C`
// vaut aussi 1. Filtrer par `l<R` perd le support alors que `beta=R` est
// admissible.
int fixture_equality() {
  const std::vector<mhgp::P3> cloud = {mhgp::P3{9, 10, 10}, mhgp::P3{11, 10, 10},
                                       mhgp::P3{10, 40, 10}, mhgp::P3{10, 10, 40}};
  CentreCell cell;
  cell.depth = 0;
  for (int d = 0; d < 3; ++d) { cell.lo[d] = 10; cell.hi[d] = 10; }
  std::vector<i128> lowers, uppers;
  for (const mhgp::P3& p : cloud) {
    i128 lo = 0, hi = 0;
    bounds_of(p, cell, &lo, &hi);
    lowers.push_back(lo);
    uppers.push_back(hi);
  }
  std::vector<i128> su = uppers;
  std::sort(su.begin(), su.end());
  const i128 r = su[1];  // R_1 : deuxieme plus petite valeur de u
  int closed = 0, open = 0;
  for (std::size_t i = 0; i < cloud.size(); ++i) {
    if (lowers[i] <= r) ++closed;
    if (lowers[i] < r) ++open;
  }
  std::printf("fixture egalite_beta_egale_R R=%lld ferme=%d ouvert=%d\n", (long long)r,
              closed, open);
  if (r != 1 || closed != 2 || open != 0) {
    std::fprintf(stderr, "INVARIANT viole : attendu R=1, ferme=2, ouvert=0\n");
    return 3;
  }
  std::printf("FIXTURE egalite : le filtre ferme garde le support, l'ouvert le perd\n");
  return 0;
}

// `proprietaire_half_open` : deux cellules soeurs partagent une face; un centre
// exactement sur la face appartient a UNE seule d'entre elles.
int fixture_owner() {
  CentreCell low, high;
  low.depth = 1;
  high.depth = 1;
  const i64 rhi[3] = {8, 8, 8};
  for (int d = 0; d < 3; ++d) {
    low.lo[d] = 0; low.hi[d] = 8;
    high.lo[d] = 8; high.hi[d] = 16;
  }
  const i128 num[3] = {4, 4, 4};
  const bool in_low = owns_centre(low, num, 1, rhi, false);
  const bool in_high = owns_centre(high, num, 1, rhi, false);
  const bool cl = owns_centre(low, num, 1, rhi, true);
  const bool ch = owns_centre(high, num, 1, rhi, true);
  std::printf("fixture proprietaire_half_open bas=%d haut=%d ferme_bas=%d ferme_haut=%d\n",
              in_low, in_high, cl, ch);
  if (in_low || !in_high || !cl || !ch) {
    std::fprintf(stderr, "INVARIANT viole : la face doit appartenir au seul enfant haut\n");
    return 3;
  }
  std::printf("FIXTURE proprietaire : exact-once tenu, le pave ferme le casse\n");
  return 0;
}

bool truth_has(const std::map<std::vector<int>, Record>& truth, std::vector<int> ids) {
  std::sort(ids.begin(), ids.end());
  return truth.find(ids) != truth.end();
}

// CONTRE-FIXTURE INTER-ARITE q3 (auditeur, section 4).
// `ABC` est un support q3 pertinent alors qu'AUCUNE de ses trois aretes ne l'est.
int fixture_arity3(int smax, Mutant mutant) {
  std::vector<mhgp::P3> cloud = {mhgp::P3{10, 10, 10}, mhgp::P3{20, 10, 10},
                                 mhgp::P3{15, 18, 10}};
  for (int z = 8; z <= 12; ++z) cloud.push_back(mhgp::P3{11, 8, z});
  cloud.push_back(mhgp::P3{11, 9, 8});
  cloud.push_back(mhgp::P3{11, 9, 12});
  for (int z = 8; z <= 10; ++z) cloud.push_back(mhgp::P3{12, 7, z});
  cloud.push_back(mhgp::P3{8, 13, 10});
  for (int z = 9; z <= 11; ++z) cloud.push_back(mhgp::P3{8, 14, z});
  cloud.push_back(mhgp::P3{8, 15, 10});
  cloud.push_back(mhgp::P3{9, 11, 10});
  for (int z = 8; z <= 11; ++z) cloud.push_back(mhgp::P3{9, 12, z});
  for (int z = 9; z <= 11; ++z) cloud.push_back(mhgp::P3{16, 18, z});
  cloud.push_back(mhgp::P3{17, 16, 6});
  cloud.push_back(mhgp::P3{17, 16, 14});
  cloud.push_back(mhgp::P3{17, 17, 7});
  cloud.push_back(mhgp::P3{17, 17, 13});
  for (int z = 8; z <= 10; ++z) cloud.push_back(mhgp::P3{17, 18, z});
  std::map<std::vector<int>, Record> truth;
  ground_truth(cloud, smax, &truth);
  const bool abc = truth_has(truth, {0, 1, 2});
  const bool ab = truth_has(truth, {0, 1});
  const bool ac = truth_has(truth, {0, 2});
  const bool bc = truth_has(truth, {1, 2});
  std::printf("fixture arite3 points=%d ABC=%d AB=%d AC=%d BC=%d\n", (int)cloud.size(),
              abc ? 1 : 0, ab ? 1 : 0, ac ? 1 : 0, bc ? 1 : 0);
  if (!abc || ab || ac || bc) {
    std::fprintf(stderr,
                 "INVARIANT viole : ABC doit etre pertinent, aucune arete ne doit l'etre\n");
    return 3;
  }
  Options opt;
  opt.smax = smax;
  opt.judge = true;
  opt.mutant = mutant;
  opt.label = "fixture_arite3";
  const int rc = run_engine(cloud, opt);
  if (rc != 0) return rc;
  std::printf("FIXTURE arite3 : la lane q3 est independante de la lane q2\n");
  return 0;
}

// CONTRE-FIXTURE INTER-ARITE q4 (auditeur, section 4).
// Le tetraedre est un support q4 pertinent alors qu'AUCUNE de ses faces ne l'est.
int fixture_arity4(int smax, Mutant mutant) {
  std::vector<mhgp::P3> cloud = {mhgp::P3{20, 20, 20}, mhgp::P3{60, 60, 20},
                                 mhgp::P3{60, 20, 60}, mhgp::P3{20, 60, 60}};
  const int g01[9][3] = {{21, 55, 65}, {21, 57, 64}, {21, 58, 63}, {21, 59, 62},
                         {22, 53, 67}, {22, 55, 66}, {22, 56, 65}, {22, 57, 65},
                         {22, 58, 64}};
  const int g23[9][3] = {{21, 21, 18}, {21, 22, 17}, {21, 23, 16}, {21, 25, 15},
                         {22, 21, 17}, {22, 22, 16}, {22, 23, 15}, {22, 24, 15},
                         {22, 25, 14}};
  for (const auto& g : g01) cloud.push_back(mhgp::P3{g[0], g[1], g[2]});
  for (const auto& g : g23) cloud.push_back(mhgp::P3{g[0], g[1], g[2]});
  std::map<std::vector<int>, Record> truth;
  ground_truth(cloud, smax, &truth);
  const bool tet = truth_has(truth, {0, 1, 2, 3});
  const bool f0 = truth_has(truth, {1, 2, 3});
  const bool f1 = truth_has(truth, {0, 2, 3});
  const bool f2 = truth_has(truth, {0, 1, 3});
  const bool f3 = truth_has(truth, {0, 1, 2});
  std::printf("fixture arite4 points=%d TET=%d faces=%d%d%d%d\n", (int)cloud.size(),
              tet ? 1 : 0, f0 ? 1 : 0, f1 ? 1 : 0, f2 ? 1 : 0, f3 ? 1 : 0);
  if (!tet || f0 || f1 || f2 || f3) {
    std::fprintf(stderr,
                 "INVARIANT viole : le tetraedre doit etre pertinent, aucune face ne doit l'etre\n");
    return 3;
  }
  Options opt;
  opt.smax = smax;
  opt.judge = true;
  opt.mutant = mutant;
  opt.label = "fixture_arite4";
  const int rc = run_engine(cloud, opt);
  if (rc != 0) return rc;
  std::printf("FIXTURE arite4 : la lane q4 est independante de la lane q3\n");
  return 0;
}

// CONTRE-FIXTURE DE COQUILLE (auditeur). Autour du centre `(10,10,10)`, les
// trente points de rayon cinq : six permutations signees de `(5,0,0)` et
// vingt-quatre de `(4,3,0)`, puisque `4^2+3^2=5^2`. La paire antipodale
// `(5,10,10),(15,10,10)` a `p=0` et `q=2`, donc `p+q=2<=smax` : elle est
// PERTINENTE. Mais `|U_B|=30`. Tout cap de coquille — le tampon `[24]` de la
// version precedente — la tronque ou tue le processus. La stratification par
// profondeur ne sauve rien ici : `A_0` contient deja les trente egalites.
std::vector<mhgp::P3> grave_shell() {
  std::vector<mhgp::P3> out;
  const i64 c = 10;
  for (int axis = 0; axis < 3; ++axis)
    for (int sgn = -1; sgn <= 1; sgn += 2) {
      i64 v[3] = {c, c, c};
      v[axis] += (i64)5 * sgn;
      out.push_back(mhgp::P3{v[0], v[1], v[2]});
    }
  for (int zero = 0; zero < 3; ++zero) {
    const int a = (zero + 1) % 3, b = (zero + 2) % 3;
    for (int swap = 0; swap < 2; ++swap)
      for (int sa = -1; sa <= 1; sa += 2)
        for (int sb = -1; sb <= 1; sb += 2) {
          i64 v[3] = {c, c, c};
          v[a] += (i64)(swap ? 3 : 4) * sa;
          v[b] += (i64)(swap ? 4 : 3) * sb;
          out.push_back(mhgp::P3{v[0], v[1], v[2]});
        }
  }
  return out;
}

int fixture_shell(int smax) {
  const std::vector<mhgp::P3> cloud = grave_shell();
  if (cloud.size() != 30) {
    std::fprintf(stderr, "INVARIANT viole : trente points attendus, %zu obtenus\n",
                 cloud.size());
    return 3;
  }
  std::map<std::vector<int>, Record> truth;
  ground_truth(cloud, smax, &truth);
  // La paire antipodale sur le premier axe : indices 0 et 1 par construction.
  const auto it = truth.find(std::vector<int>{0, 1});
  if (it == truth.end()) {
    std::fprintf(stderr, "INVARIANT viole : la paire antipodale doit etre pertinente\n");
    return 3;
  }
  std::printf("fixture coquille p=%zu shell=%zu rang_ferme_ok=%d\n",
              it->second.interior.size(), it->second.shell.size(),
              it->second.closed_rank_ok ? 1 : 0);
  if (!it->second.interior.empty() || it->second.shell.size() != 30 ||
      it->second.closed_rank_ok) {
    std::fprintf(stderr,
                 "INVARIANT viole : attendu p=0, |U_B|=30, extra-shell\n");
    return 3;
  }
  Options opt;
  opt.smax = smax;
  opt.judge = true;
  opt.label = "fixture_coquille";
  const int rc = run_engine(cloud, opt);
  if (rc != 0) return rc;
  std::printf("FIXTURE coquille : |U_B|=30 publie sans cap, classe extra-shell\n");
  return 0;
}

// NUAGE GRAVE : grille cubique. Les nuages aleatoires ne produisent JAMAIS
// l'egalite entiere `l_C(x) == R_p(C)`, si bien que le mutant `drop-ties` y
// survit toujours. Sur une grille, la symetrie rend ces egalites systematiques.
std::vector<mhgp::P3> grave_grid(int side, int step) {
  std::vector<mhgp::P3> out;
  out.reserve((std::size_t)side * side * side);
  for (int i = 0; i < side; ++i)
    for (int j = 0; j < side; ++j)
      for (int k = 0; k < side; ++k)
        out.push_back(mhgp::P3{i * step, j * step, k * step});
  return out;
}

// NUAGE GRAVE A DOUBLON : les generateurs du depot dedupliquent, donc aucune
// famille n'exerce le refus `unsupported_degeneracy`. Ce nuage le fait.
std::vector<mhgp::P3> grave_doublon() {
  std::vector<mhgp::P3> out = grave_grid(4, 10);
  out.push_back(out[7]);
  return out;
}


int parse_int(const char* text, bool* ok) {
  char* end = nullptr;
  const long long value = std::strtoll(text, &end, 10);
  *ok = end != nullptr && *end == '\0' && end != text && value >= 0 && value <= 100000000LL;
  return (int)value;
}

}  // namespace

int main(int argc, char** argv) {
  int points = 200;
  int coord = 0;
  long long seed = 11;
  std::string family = "uniform";
  std::string fixtures;
  std::string inject;
  Options opt;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    bool ok = true;
    if (arg.rfind("--points=", 0) == 0) points = parse_int(arg.substr(9).c_str(), &ok);
    else if (arg.rfind("--smax=", 0) == 0) opt.smax = parse_int(arg.substr(7).c_str(), &ok);
    else if (arg.rfind("--coord=", 0) == 0) coord = parse_int(arg.substr(8).c_str(), &ok);
    else if (arg.rfind("--seed=", 0) == 0) seed = parse_int(arg.substr(7).c_str(), &ok);
    else if (arg.rfind("--leaf=", 0) == 0) opt.leaf = parse_int(arg.substr(7).c_str(), &ok);
    else if (arg.rfind("--work-cap=", 0) == 0)
      opt.work_cap = parse_int(arg.substr(11).c_str(), &ok);
    else if (arg == "--axis-filter") opt.axis_filter = true;
    else if (arg == "--multiplicity") opt.multiplicity = true;
    else if (arg.rfind("--batch-depth=", 0) == 0)
      opt.batch_depth = parse_int(arg.substr(14).c_str(), &ok);
    else if (arg.rfind("--batch-records=", 0) == 0)
      opt.batch_records = parse_int(arg.substr(16).c_str(), &ok);
    else if (arg == "--deferred-lift") opt.deferred_lift = true;
    else if (arg.rfind("--probe-factor=", 0) == 0)
      opt.probe_factor = parse_int(arg.substr(15).c_str(), &ok);
    else if (arg.rfind("--ablate=", 0) == 0) opt.ablate = parse_int(arg.substr(9).c_str(), &ok);
    else if (arg.rfind("--max-depth=", 0) == 0)
      opt.max_depth = parse_int(arg.substr(12).c_str(), &ok);
    else if (arg.rfind("--min-supports=", 0) == 0)
      opt.min_supports = parse_int(arg.substr(15).c_str(), &ok);
    else if (arg.rfind("--min-cells=", 0) == 0)
      opt.min_cells = parse_int(arg.substr(12).c_str(), &ok);
    else if (arg.rfind("--min-quads=", 0) == 0)
      opt.min_quads = parse_int(arg.substr(12).c_str(), &ok);
    else if (arg.rfind("--min-probes=", 0) == 0)
      opt.min_probes = parse_int(arg.substr(13).c_str(), &ok);
    else if (arg.rfind("--family=", 0) == 0) family = arg.substr(9);
    else if (arg == "--judge") opt.judge = true;
    else if (arg == "--fixtures") fixtures = "toutes";
    else if (arg.rfind("--fixtures=", 0) == 0) fixtures = arg.substr(11);
    else if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else {
      std::fprintf(stderr, "REFUS argument inconnu: %s\n", arg.c_str());
      return 2;
    }
    if (!ok) {
      std::fprintf(stderr, "REFUS valeur invalide: %s\n", arg.c_str());
      return 2;
    }
  }

  if (opt.smax < 4 || opt.smax > 24 || opt.leaf < 4 || opt.work_cap < 1 ||
      opt.max_depth < 1 || opt.max_depth > 26 || opt.ablate < 0 || opt.ablate > 5) {
    std::fprintf(stderr, "REFUS domaine\n");
    return 2;
  }
  // L'ABLATION PRODUIT UNE SORTIE FAUSSE PAR CONSTRUCTION. Elle est donc
  // incompatible avec le juge, avec les mutants et avec tout plancher, et elle
  // s'annonce en clair dans le recu.
  if (opt.ablate != 0) {
    if (opt.judge || !inject.empty()) {
      std::fprintf(stderr, "REFUS : l'ablation ne peut ni etre jugee ni porter un mutant\n");
      return 2;
    }
    // Le chemin differe ne respecte PAS la propriete de flot amont : a
    // `ablate=2` il passerait quand meme par les lifts de `solve_tuple`, et a
    // `ablate=3` il accumulerait des contextes sans jamais vider le lot. Les
    // deux modes sont donc exclusifs.
    if (opt.deferred_lift) {
      std::fprintf(stderr,
                   "REFUS : l'ablation et le lift differe ne partagent pas le meme flot\n");
      return 2;
    }
    // Un plancher sur une sortie volontairement fausse n'a aucun sens.
    if (opt.min_supports || opt.min_cells || opt.min_quads || opt.min_probes) {
      std::fprintf(stderr, "REFUS : aucun plancher sur une sortie ablatee\n");
      return 2;
    }
    std::printf("ABLATION=%d SORTIE FAUSSE PAR CONSTRUCTION, MESURE SEULEMENT\n",
                opt.ablate);
  }

  if (!inject.empty()) {
    if (inject == "drop-ties") opt.mutant = Mutant::kDropTies;
    else if (inject == "owner-closed") opt.mutant = Mutant::kOwnerClosed;
    else if (inject == "rank-closed") opt.mutant = Mutant::kRankClosed;
    else if (inject == "tight-threshold") opt.mutant = Mutant::kTightThreshold;
    else if (inject == "bisector-strict") opt.mutant = Mutant::kBisectorStrict;
    else if (inject == "shrink-list") opt.mutant = Mutant::kShrinkList;
    else if (inject == "arity-cascade") opt.mutant = Mutant::kArityCascade;
    else if (inject == "strata-stop") opt.mutant = Mutant::kStrataStop;
    else if (inject == "incidence-off-by-one") opt.mutant = Mutant::kIncidenceOffByOne;
    else {
      std::fprintf(stderr, "REFUS injection inconnue: %s\n", inject.c_str());
      return 2;
    }
  }

  if (!fixtures.empty()) {
    if (fixtures == "egalite") return fixture_equality();
    if (fixtures == "proprietaire") return fixture_owner();
    if (fixtures == "arite3") return fixture_arity3(opt.smax, opt.mutant);
    if (fixtures == "arite4") return fixture_arity4(opt.smax, opt.mutant);
    if (fixtures == "coquille") return fixture_shell(opt.smax);
    if (fixtures == "toutes") {
      int rc = fixture_equality();
      if (rc) return rc;
      rc = fixture_owner();
      if (rc) return rc;
      rc = fixture_arity3(opt.smax, opt.mutant);
      if (rc) return rc;
      rc = fixture_arity4(opt.smax, opt.mutant);
      if (rc) return rc;
      return fixture_shell(opt.smax);
    }
    std::fprintf(stderr, "REFUS fixture inconnue: %s\n", fixtures.c_str());
    return 2;
  }

  if (points < 4) {
    std::fprintf(stderr, "REFUS domaine\n");
    return 2;
  }
  if (opt.judge && points > 220) {
    std::fprintf(stderr, "REFUS juge exhaustif au-dela de 220 points\n");
    return 2;
  }

  std::vector<mhgp::P3> cloud;
  if (family == "grave_grid") {
    int side = 1;
    while ((side + 1) * (side + 1) * (side + 1) <= points) ++side;
    if (side < 3) {
      std::fprintf(stderr, "REFUS grille gravee trop petite\n");
      return 2;
    }
    cloud = grave_grid(side, 10);
  } else if (family == "grave_doublon") {
    cloud = grave_doublon();
  } else {
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
    if (coord > 65535) {
      std::fprintf(stderr, "REFUS coord hors du profil u16\n");
      return 2;
    }
    cloud = mhgp3v::make_family_cloud(fam, points, coord, seed);
    if ((int)cloud.size() != points) {
      std::fprintf(stderr, "REFUS nuage incomplet\n");
      return 2;
    }
  }
  // DOUBLONS EXACTS : refus explicite, sans jitter.
  {
    std::vector<std::array<i64, 3>> keys;
    keys.reserve(cloud.size());
    for (const mhgp::P3& p : cloud) keys.push_back({p.x, p.y, p.z});
    std::sort(keys.begin(), keys.end());
    const std::size_t distinct =
        (std::size_t)(std::unique(keys.begin(), keys.end()) - keys.begin());
    if (distinct != cloud.size()) {
      std::fprintf(stderr, "REFUS unsupported_degeneracy : %zu sites confondus sur %zu\n",
                   cloud.size() - distinct, cloud.size());
      return 2;
    }
  }
  opt.label = family;
  return run_engine(cloud, opt);
}
