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
// moins les q points de U, donc d_U <= s_max - q. La chaine s_max - q <= s_max - 2
// ne vaut QUE pour q >= 2 : les singletons ne relevent pas de la navigation et
// sont publies a part, directement. Naviguer selon
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
// niveau zero visite tout { l <= k }. Deux consequences a ne pas oublier.
//
// La variation du niveau d'un voisin N'EST PAS bornee par un. Elle vaut
// exactement |D_-(d)| - |{ i dans A : i interieur sur l'arete }|, et chacun des
// deux termes peut etre grand des que plusieurs hyperplans coincident sur
// l'evenement. L'enonce « +-1 » vient du cas simple et n'a jamais ete vrai
// ailleurs ; le transport par lots ci-dessus ne le suppose nulle part.
//
// Et l'arete ouverte peut etre de niveau k+1 alors que ses deux extremites sont
// de niveau k : cela ne casse pas la connexite, mais interdit de confondre le
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
#include <cmath>
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
  std::vector<i32> shell;      // S(v), trie
  std::vector<i32> interior;   // B(v), trie — TRANSPORTE, jamais recense
  int level = 0;               // l(v) = |B(v)|, redondant avec interior.size()
};

// L'arete de parent telle que le juge doit pouvoir la controler : la coquille
// du parent, la FERMETURE du flat emprunte et l'orientation. Sans la fermeture,
// le juge ne peut verifier ni son rang trois, ni l'identite S(next) = C union A.
struct ParentEdge {
  std::vector<i32> shell;      // S(pi(v)), vide si v est la racine
  std::vector<i32> closure;    // C(d)
  int orientation = 0;
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
  long long grid_points_touched = 0;     // points visites par l'index, avant tout test
  long long bootstrap_rounds = 0;        // doublements de pave a l'amorce
  // DEUX evenements distincts, longtemps confondus sous un seul compteur : une
  // amorce qui a du elargir sa boite jusqu'a couvrir la grille declaree, et un
  // BALAYAGE effectif des n points. Seul le second est un obstacle device — un
  // balayage O(n) par thread y serait inadmissible — et il faut donc le voir seul.
  long long full_grid_sweeps = 0;        // amorces ayant du couvrir toute la grille
  long long exhaustive_scans = 0;        // balayages effectifs des n points
  long long disagreement_sweeps = 0;     // balayages de certification par desaccord de signe
  long long harvest_prefiltered = 0;     // supports ecartes par le test de propriete
  long long harvest_censused = 0;        // supports ayant paye un census complet
  // COMPOSITION « support canonique puis proprietaire », qui remplace `emitted`.
  long long owner_rejected_support = 0;  // U different du support canonique
  long long owner_rejected_vertex = 0;   // support canonique, mais un autre sommet possede
  long long owner_emitted = 0;
  // HIGH-WATER de la table, releve a chaque INSERTION. La taille finale ne prouve
  // rien : une mutation qui viderait la table en fin de calcul tromperait la porte.
  long long dedup_table_high_water = 0;
  // HIGH-WATERS VRAIS, releves a chaque ecriture. `shells_multiple` et
  // `batches_multiple` comptent des EVENEMENTS au-dessus d'un seuil et
  // `grid_points_touched` est une SOMME : aucun des trois ne dit quelle capacite
  // un noyau borne devrait declarer. Ceux-ci le disent.
  long long shell_high_water = 0;
  long long closure_high_water = 0;
  long long touched_high_water = 0;
  long long batch_high_water = 0;
  long long interior_high_water = 0;
  long long reverse_depth_max = 0;        // profondeur maximale de la pile
  long long reverse_children_tested = 0;  // voisins soumis au test de parent
  long long reverse_backtracks = 0;
  long long reverse_flats_enumerated = 0;   // flats CANONIQUES livres au callback
  // Travail TOTAL de l'enumeration, et non les seuls flats livres : un triplet
  // ecarte parce qu'il n'est pas la base canonique de sa fermeture a quand meme
  // paye sa fermeture. Sans ces deux compteurs la queue multiplicitaire reste
  // sous-instrumentee et tout ratio publie est optimiste.
  long long reverse_triplets_scanned = 0;
  long long reverse_closures_built = 0;
  long long reverse_live_high_water = 0;   // slots vifs du chemin, sortie exclue
  // POURQUOI un fils candidat est refuse. Le couple (G,-delta) — le meme plan vu
  // depuis w — est testable en O(m) ; s'il n'est pas admissible, pi(w) != v et le
  // refus ne coute aucune enumeration. Le second compteur est le refus qui exige
  // quand meme le parent : couple admissible mais non minimal.
  long long reverse_reject_backward = 0;
  long long reverse_reject_by_parent = 0;
  long long reverse_decisions = 0;         // decisions de filiation, sans requete de retour

  void absorb(const FlatStatistics& o) {
    static_assert(sizeof(FlatStatistics) == 48 * sizeof(long long),
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
    grid_points_touched += o.grid_points_touched;
    bootstrap_rounds += o.bootstrap_rounds;
    full_grid_sweeps += o.full_grid_sweeps;
    exhaustive_scans += o.exhaustive_scans;
    disagreement_sweeps += o.disagreement_sweeps;
    harvest_prefiltered += o.harvest_prefiltered;
    harvest_censused += o.harvest_censused;
    owner_rejected_support += o.owner_rejected_support;
    owner_rejected_vertex += o.owner_rejected_vertex;
    owner_emitted += o.owner_emitted;
    dedup_table_high_water = std::max(dedup_table_high_water, o.dedup_table_high_water);
    shell_high_water = std::max(shell_high_water, o.shell_high_water);
    closure_high_water = std::max(closure_high_water, o.closure_high_water);
    touched_high_water = std::max(touched_high_water, o.touched_high_water);
    batch_high_water = std::max(batch_high_water, o.batch_high_water);
    interior_high_water = std::max(interior_high_water, o.interior_high_water);
    reverse_depth_max = std::max(reverse_depth_max, o.reverse_depth_max);
    reverse_children_tested += o.reverse_children_tested;
    reverse_backtracks += o.reverse_backtracks;
    reverse_flats_enumerated += o.reverse_flats_enumerated;
    reverse_triplets_scanned += o.reverse_triplets_scanned;
    reverse_closures_built += o.reverse_closures_built;
    reverse_live_high_water = std::max(reverse_live_high_water, o.reverse_live_high_water);
    reverse_reject_backward += o.reverse_reject_backward;
    reverse_reject_by_parent += o.reverse_reject_by_parent;
    reverse_decisions += o.reverse_decisions;
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
  kDuplicateCoordinates,        // deux observations confondues : hors contrat
  kOutsideDeclaredGrid,         // coordonnee hors de la grille u16 declaree
  kOrderOutsideContract,        // s_max hors contrat, refuse avant toute soustraction
  // ARRET VOLONTAIRE du consommateur. Ce n'est PAS une contradiction interne : le
  // sink a demande l'arret, et l'appelant sait donc que sa sortie est un prefixe
  // deliberement tronque. Les confondre avec `kInvariantViolated` rendrait
  // indistinguables une troncature choisie et un invariant rompu.
  kSinkStopped,
};

inline const char* cloud_status_name(CloudStatus s) {
  switch (s) {
    case CloudStatus::kOk: return "ok";
    case CloudStatus::kAffineDimensionBelowThree: return "dimension_affine_inferieure_a_trois";
    case CloudStatus::kTooFewPoints: return "moins_de_quatre_points";
    case CloudStatus::kSeedFailed: return "germe_non_certifie";
    case CloudStatus::kInvariantViolated: return "invariant_de_transport_viole";
    case CloudStatus::kDuplicateCoordinates: return "coordonnees_dupliquees";
    case CloudStatus::kOutsideDeclaredGrid: return "hors_grille_u16_declaree";
    case CloudStatus::kOrderOutsideContract: return "ordre_hors_contrat";
    case CloudStatus::kSinkStopped: return "sink_arrete";
  }
  return "inconnu";
}

// ---------------------------------------------------------------------------
// Dimension affine. Les normales relevees a_i = (-2 p_i, 1) engendrent R^4 si
// et seulement si le nuage est de dimension affine trois ; c'est l'hypothese
// du theoreme de proprietaire et la condition pour que P_vide soit pointe.
// ---------------------------------------------------------------------------
// DOUBLONS DE COORDONNEES. Le profil quantifie autorise deux observations a
// tomber sur le meme point de grille. La convention de support canonique par
// ordre des coordonnees ne les separe alors plus, et echanger les deux
// identifiants change quatre supports publies sans rien changer a la geometrie
// — mesure de l'audit `AUDIT_ORDER_K_FLATS_9C587E6.md` §3.4 sur la fixture
// (0,0,0) (0,0,0) (2,0,0) (0,2,0) (0,0,2). Tant qu'une semantique quotientee
// n'est pas definie, le prototype REFUSE explicitement, il ne publie pas `ok`.
// GRILLE DECLAREE. Toutes les bornes de largeur de ce fichier — 2^91,2 pour
// `in_sphere_side`, 2^108,8 pour `in_circle_coplanar` — supposent des
// coordonnees dans [0, 65535]. La garde ne peut donc pas vivre dans le CLI d'un
// juge : un autre appelant passerait des coordonnees de l'ordre de 10^9 et
// obtiendrait un depassement signe `__int128` avant tout predicat. Elle est ici,
// a la frontiere des deux entrees publiques.
inline constexpr mhgp::i32 kDeclaredGridMaximum = 65535;

inline bool inside_declared_grid(const std::vector<mhgp::P3>& points) {
  for (const mhgp::P3& p : points)
    if (p.x < 0 || p.y < 0 || p.z < 0 || p.x > kDeclaredGridMaximum ||
        p.y > kDeclaredGridMaximum || p.z > kDeclaredGridMaximum) return false;
  return true;
}

inline bool has_duplicate_coordinates(const std::vector<mhgp::P3>& points) {
  std::vector<const mhgp::P3*> sorted;
  sorted.reserve(points.size());
  for (const mhgp::P3& p : points) sorted.push_back(&p);
  std::sort(sorted.begin(), sorted.end(), [](const mhgp::P3* a, const mhgp::P3* b) {
    if (a->x != b->x) return a->x < b->x;
    if (a->y != b->y) return a->y < b->y;
    return a->z < b->z;
  });
  for (std::size_t i = 1; i < sorted.size(); ++i)
    if (sorted[i]->x == sorted[i - 1]->x && sorted[i]->y == sorted[i - 1]->y &&
        sorted[i]->z == sorted[i - 1]->z) return true;
  return false;
}

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

  // (4) TRIANGLE DE DELAUNAY dans le plan de la face support, EN DEUX PASSES
  //     TOTALES, sans boucle et donc sans potentiel de terminaison a prouver.
  //
  // La version precedente faisait DECROITRE LE RAYON : si un point est
  // strictement interieur au cercle de (a,b,c), l'un des trois triangles
  // obtenus en remplacant un sommet aurait un cercle strictement plus petit.
  // C'EST FAUX, et l'audit `AUDIT_ORDER_K_FLATS_9C587E6.md` §2 le montre sur
  // cinq points u16 :
  //
  //     A=(0,0,0)  B=(0,3,0)  C=(2,1,0)  P=(1,1,0)  Q=(1,1,2)
  //
  // `P` est strictement interieur au cercle de `ABC` — le predicat entier rend
  // -72 — et pourtant les quatre rayons carres valent exactement 5/2. Aucune
  // descente n'existe, le germe rendait `germe_non_certifie` etape 6, et le
  // catalogue sortait vide. Pire, la reussite dependait de la numerotation :
  // 90 permutations sur 120 passaient. Le bon potentiel de DELAUNAY n'a jamais
  // ete le rayon, c'est le vecteur des angles.
  //
  // On n'a de toute facon pas besoin d'un potentiel. Sur une ARETE DE
  // L'ENVELOPPE du sous-nuage coplanaire, le troisieme point de DELAUNAY est
  // celui qui MAXIMISE l'angle inscrit, et « d est strictement interieur au
  // cercle de (a,b,c) » equivaut a « l'angle en d depasse l'angle en c ». C'est
  // un ordre TOTAL sur les points d'un meme cote de la droite : une passe
  // suffit, les ex aequo cocirculaires sont tous des choix valides. Et le
  // cercle obtenu est vide : un intrus du meme cote contredirait la maximalite,
  // il n'y a personne de l'autre cote puisque l'arete est sur l'enveloppe, et un
  // point de la droite hors du segment est exterieur a tout cercle par a et b.
  //
  // Reste a produire cette arete d'enveloppe, exactement et en une passe. Le
  // point lex-min du sous-nuage coplanaire est extreme pour la forme lineaire
  // « x puis y puis z », donc sommet de son enveloppe. Depuis lui, toutes les
  // directions verifient w_x >= 0, et w_x = 0 force w_y >= 0 puis w_z >= 0 : il
  // n'existe aucune paire antipodale, l'ordre angulaire est total, et une passe
  // d'emballage donne l'arete. A egalite d'angle on prend le point le PLUS
  // PROCHE, sans quoi un point du segment resterait entre les deux extremites et
  // serait interieur a tout cercle par elles.
  std::vector<mhgp::i32> plane_points;
  for (mhgp::i32 z = 0; z < n; ++z)
    if (flats::orient3d_exact(points[(std::size_t)p0], points[(std::size_t)p1],
                        points[(std::size_t)p2], points[(std::size_t)z]) == 0)
      plane_points.push_back(z);
  ++st->seed_scans;
  if (plane_points.size() < 3) { st->seed_failure_stage = 4; return CloudStatus::kSeedFailed; }

  // Normale du plan support, orientee comme (p0,p1,p2).
  const mhgp::P3 face_u = mhgp::p3_cross(mhgp::p3_sub(points[(std::size_t)p1],
                                                      points[(std::size_t)p0]),
                                         mhgp::p3_sub(points[(std::size_t)p2],
                                                      points[(std::size_t)p0]));

  mhgp::i32 ha = plane_points[0];
  for (mhgp::i32 z : plane_points) {
    const mhgp::P3& u = points[(std::size_t)z];
    const mhgp::P3& w = points[(std::size_t)ha];
    if (u.x < w.x || (u.x == w.x && (u.y < w.y || (u.y == w.y && u.z < w.z)))) ha = z;
  }
  // Cote, dans le plan, de z par rapport a la droite (ha,hb).
  auto plane_side = [&](mhgp::i32 hb, mhgp::i32 z) {
    const mhgp::P3 d = mhgp::p3_sub(points[(std::size_t)hb], points[(std::size_t)ha]);
    const mhgp::P3 w = mhgp::p3_sub(points[(std::size_t)z], points[(std::size_t)ha]);
    const mhgp::P3 c = mhgp::p3_cross(d, w);
    const mhgp::i128 v = (mhgp::i128)c.x * face_u.x + (mhgp::i128)c.y * face_u.y
                       + (mhgp::i128)c.z * face_u.z;
    return flats::sign_of(v);
  };
  auto squared_from_ha = [&](mhgp::i32 z) {
    const mhgp::P3 w = mhgp::p3_sub(points[(std::size_t)z], points[(std::size_t)ha]);
    return (mhgp::i128)w.x * w.x + (mhgp::i128)w.y * w.y + (mhgp::i128)w.z * w.z;
  };
  mhgp::i32 hb = -1;
  for (mhgp::i32 z : plane_points) {
    if (z == ha) continue;
    if (hb < 0) { hb = z; continue; }
    const int side = plane_side(hb, z);
    if (side < 0) hb = z;
    else if (side == 0 && squared_from_ha(z) < squared_from_ha(hb)) hb = z;
  }
  ++st->seed_scans;
  if (hb < 0) { st->seed_failure_stage = 4; return CloudStatus::kSeedFailed; }
  for (mhgp::i32 z : plane_points)
    if (plane_side(hb, z) < 0) { st->seed_failure_stage = 5; return CloudStatus::kSeedFailed; }
  ++st->seed_scans;

  mhgp::i32 apex = -1;
  for (mhgp::i32 z : plane_points) {
    if (z == ha || z == hb || plane_side(hb, z) == 0) continue;
    if (apex < 0) { apex = z; continue; }
    if (flats::in_circle_coplanar(points[(std::size_t)ha], points[(std::size_t)hb],
                                  points[(std::size_t)apex], points[(std::size_t)z]) < 0) apex = z;
  }
  ++st->seed_scans;
  if (apex < 0) { st->seed_failure_stage = 6; return CloudStatus::kSeedFailed; }
  mhgp::i32 tri[3] = {ha, hb, apex};
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
  seed_out->interior.clear();      // certifie vide par la construction ci-dessus
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
                   std::vector<i32>* shell_out, std::vector<i32>* interior_out) {
  mhgp::Sphere s{};
  if (!shell_sphere(points, v.shell, &s)) return false;
  shell_out->clear();
  interior_out->clear();
  for (i32 z = 0; z < (i32)points.size(); ++z) {
    const int side = mhgp::sphere_side(s, points[(std::size_t)z]);
    if (side < 0) interior_out->push_back(z);
    else if (side == 0) shell_out->push_back(z);
  }
  return true;
}

}  // namespace flats

// ---------------------------------------------------------------------------
// INDEX SPATIAL CERTIFIE — le flottant ne peut qu'ajouter du travail
// ---------------------------------------------------------------------------
//
// L'audit numerique exige d'un accelerateur qu'il sature avant conversion,
// n'exclue jamais en `double` et propage un statut de couverture. La grille
// precedente violait les trois. Une grille a maille fixe a de toute facon un
// defaut plus profond : une boule VIDE mais grande — et le pinceau en produit,
// puisque entre deux evenements consecutifs la region balayee ne contient par
// definition aucun point — lui coute $(R/\text{maille})^3$ cellules pour zero
// resultat.
//
// On prend donc un ARBRE k-d sur les identifiants, avec la boite entiere de
// chaque noeud. Une region vide coute la profondeur, pas son volume.
//
// LE CONTRAT DE CORRECTION tient en une phrase : le flottant ne sert qu'a
// ELAGUER un noeud, l'appartenance finale est decidee par `mhgp::sphere_side`.
// Un elagage errone serait une omission, donc il est rendu impossible par une
// marge. Le centre exact est `base + num/den` et le rayon `sqrt(N)/den` avec
// `N = |num|^2`. Les conversions `big_to_double` et la racine ont une erreur
// relative de l'ordre de $2^{-52}$ ; comme coordonnees et rayons restent sous
// $2^{17}$ sur la grille declaree, l'erreur absolue sur le centre et sur le
// rayon reste sous $2^{-35}$. On elargit de **un demi**, soit plus de $2^{34}$
// fois cette borne, alors que deux points entiers distincts sont a distance au
// moins un. Aucun point de la boule fermee ne peut donc etre elague, et un
// noeud garde a tort ne coute qu'un test exact de plus.
struct CertifiedIndex {
  struct Node {
    int lo[3] = {0, 0, 0};
    int hi[3] = {0, 0, 0};
    int begin = 0, end = 0;
    int left = -1, right = -1;         // left < 0 : feuille
  };
  std::vector<Node> nodes;
  std::vector<mhgp::i32> order;
  const std::vector<mhgp::P3>* points = nullptr;
  int leaf_size = 16;
  // NOEUDS REELLEMENT VISITES. « Points touches » ne dit pas si l'arbre elague :
  // un arbre a feuille unique touche tout le nuage en une descente et affiche le
  // meme compteur qu'un arbre profond qui coupe. Celui-ci mesure la descente.
  mutable long long nodes_visited = 0;
  mutable long long leaves_visited = 0;

  void build(const std::vector<mhgp::P3>& cloud, int leaf = 16) {
    points = &cloud;
    leaf_size = std::max(1, leaf);
    order.resize(cloud.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = (mhgp::i32)i;
    nodes.clear();
    if (order.empty()) return;
    nodes.reserve(2 * (order.size() / (std::size_t)leaf_size + 2));
    build_range(0, (int)order.size());
  }

  int build_range(int begin, int end) {
    const int self = (int)nodes.size();
    nodes.push_back(Node{});
    Node node{};
    node.begin = begin;
    node.end = end;
    for (int d = 0; d < 3; ++d) { node.lo[d] = kDeclaredGridMaximum; node.hi[d] = 0; }
    for (int t = begin; t < end; ++t) {
      const mhgp::P3& p = (*points)[(std::size_t)order[(std::size_t)t]];
      const int c[3] = {(int)p.x, (int)p.y, (int)p.z};
      for (int d = 0; d < 3; ++d) {
        node.lo[d] = std::min(node.lo[d], c[d]);
        node.hi[d] = std::max(node.hi[d], c[d]);
      }
    }
    if (end - begin > leaf_size) {
      int axis = 0;
      for (int d = 1; d < 3; ++d)
        if (node.hi[d] - node.lo[d] > node.hi[axis] - node.lo[axis]) axis = d;
      const int mid = begin + (end - begin) / 2;
      std::nth_element(order.begin() + begin, order.begin() + mid, order.begin() + end,
                       [&](mhgp::i32 a, mhgp::i32 b) {
                         const mhgp::P3& u = (*points)[(std::size_t)a];
                         const mhgp::P3& w = (*points)[(std::size_t)b];
                         const mhgp::i32 ua = axis == 0 ? u.x : (axis == 1 ? u.y : u.z);
                         const mhgp::i32 wa = axis == 0 ? w.x : (axis == 1 ? w.y : w.z);
                         if (ua != wa) return ua < wa;
                         return a < b;
                       });
      node.left = build_range(begin, mid);
      node.right = build_range(mid, end);
    }
    nodes[(std::size_t)self] = node;
    return self;
  }

  // Boule flottante ELARGIE d'un demi : elle contient strictement la boule
  // exacte, donc elaguer sur elle ne peut rien omettre.
  struct LooseBall {
    double centre[3] = {0, 0, 0};
    double radius = 0;                 // rayon DEJA elargi de la marge
    bool usable = false;               // le chemin rapide est-il autorise ?
    mhgp::i128 centre_num[3] = {0, 0, 0};   // base_j * den + n_j
    mhgp::i128 den = 0;
    mhgp::BigInt<4> radius2_num{};     // N = |num|^2
  };

  // LA MARGE FLOTTANTE A ETE REFUTEE, et il faut le dire nettement.
  //
  // La version precedente elargissait la boule d'un demi et pretendait que
  // l'erreur restait sous 2^-35 « puisque coordonnees et rayons restent sous
  // 2^17 sur la grille declaree ». C'est faux : le CENTRE d'une sphere portee
  // par un quadruplet presque coplanaire sort arbitrairement loin de la grille.
  // La note `NOTE_POSITIVE_INDEX_KD_EXACT_ET_CERTIFICAT_PINCEAU.md` §1.3 donne
  // quatre points u16 distincts
  //
  //     (32767,32767,0) (57863,57862,0) (7672,7673,0) (60104,30135,1)
  //
  // dont la sphere exacte a `den = 2` et un rayon de l'ordre de 10^18. A cette
  // echelle la marge d'un demi ne signifie plus rien : l'elagage supprimait LA
  // RACINE, et la requete rendait zero point au lieu des quatre supports.
  //
  // Le chemin rapide est donc GARDE. Il n'est autorise que si centre et rayon
  // tiennent sous 2^20, ou l'erreur absolue des conversions reste sous 2^-30 et
  // la marge d'un demi la domine de plus de 2^28. Hors de cette garde, le
  // prédicat exact ci-dessous decide, en entiers.
  static constexpr double kFastPathBound = 1048576.0;   // 2^20

  static LooseBall loosen(const mhgp::Sphere& sphere) {
    LooseBall ball;
    if (sphere.den <= 0) return ball;
    ball.den = sphere.den;
    ball.centre_num[0] = (mhgp::i128)sphere.base.x * sphere.den + sphere.nx;
    ball.centre_num[1] = (mhgp::i128)sphere.base.y * sphere.den + sphere.ny;
    ball.centre_num[2] = (mhgp::i128)sphere.base.z * sphere.den + sphere.nz;
    ball.radius2_num = mhgp::sphere_num2(sphere);
    const double den = (double)sphere.den;
    if (!(den > 0) || !std::isfinite(den)) return ball;
    const double rx = (double)sphere.nx / den, ry = (double)sphere.ny / den,
                 rz = (double)sphere.nz / den;
    if (!std::isfinite(rx) || !std::isfinite(ry) || !std::isfinite(rz)) return ball;
    ball.centre[0] = (double)sphere.base.x + rx;
    ball.centre[1] = (double)sphere.base.y + ry;
    ball.centre[2] = (double)sphere.base.z + rz;
    const double r = std::sqrt(rx * rx + ry * ry + rz * rz);
    if (!std::isfinite(r) || r > kFastPathBound) return ball;
    for (int d = 0; d < 3; ++d)
      if (std::abs(ball.centre[d]) > kFastPathBound) return ball;
    ball.radius = r + 0.5;
    ball.usable = true;
    return ball;
  }

  // PREDICAT EXACT boite--boule, note §1 theoreme 1. Avec
  // C_j = base_j*den + n_j et g_j = max(l_j*den - C_j, 0, C_j - h_j*den),
  // on a den^2 * dist(c, Q)^2 = somme des g_j^2, et le noeud est elaguable si et
  // seulement si cette somme depasse STRICTEMENT N. L'egalite est conservee : la
  // boule est fermee et les points de coquille sont contractuels. De meme le
  // noeud est certainement STRICTEMENT interieur si la somme des f_j^2, avec
  // f_j = max(|l_j*den - C_j|, |h_j*den - C_j|), est strictement sous N.
  // Largeurs : C_j et g_j sous 2^91, leurs carres sous 2^182, la somme sous
  // 2^184 — tres en dessous du bit de signe de `BigInt<4>`.
  static bool exact_outside(const Node& node, const LooseBall& ball) {
    if (ball.den <= 0) return false;
    mhgp::BigInt<4> total{};
    for (int d = 0; d < 3; ++d) {
      const mhgp::i128 low = (mhgp::i128)node.lo[d] * ball.den - ball.centre_num[d];
      const mhgp::i128 high = ball.centre_num[d] - (mhgp::i128)node.hi[d] * ball.den;
      mhgp::i128 gap = 0;
      if (low > gap) gap = low;
      if (high > gap) gap = high;
      total = mhgp::big_add(total, mhgp::mul128(gap, gap));
    }
    return mhgp::big_cmp(total, ball.radius2_num) > 0;
  }

  static bool exact_strictly_inside(const Node& node, const LooseBall& ball) {
    if (ball.den <= 0) return false;
    mhgp::BigInt<4> total{};
    for (int d = 0; d < 3; ++d) {
      const mhgp::i128 a = (mhgp::i128)node.lo[d] * ball.den - ball.centre_num[d];
      const mhgp::i128 b = (mhgp::i128)node.hi[d] * ball.den - ball.centre_num[d];
      const mhgp::i128 fa = a < 0 ? -a : a;
      const mhgp::i128 fb = b < 0 ? -b : b;
      const mhgp::i128 reach = fa > fb ? fa : fb;
      total = mhgp::big_add(total, mhgp::mul128(reach, reach));
    }
    return mhgp::big_cmp(total, ball.radius2_num) < 0;
  }

  static bool node_may_touch(const Node& node, const LooseBall& ball) {
    if (!ball.usable) return !exact_outside(node, ball);
    double d2 = 0;
    for (int d = 0; d < 3; ++d) {
      const double c = ball.centre[d];
      const double lo = (double)node.lo[d], hi = (double)node.hi[d];
      const double gap = c < lo ? (lo - c) : (c > hi ? (c - hi) : 0.0);
      d2 += gap * gap;
    }
    return d2 <= ball.radius * ball.radius;
  }

  // Visite tous les points du nuage dans la boule FERMEE, et rien d'autre. La
  // decision d'appartenance est exacte et garde l'egalite, donc la coquille.
  template <class Fn>
  void closed_ball(const mhgp::Sphere& sphere, long long* touched, Fn&& visit) const {
    if (nodes.empty()) return;
    const LooseBall ball = loosen(sphere);
    int stack[128];
    int top = 0;
    stack[top++] = 0;
    while (top > 0) {
      const Node& node = nodes[(std::size_t)stack[--top]];
      ++nodes_visited;
      if (!node_may_touch(node, ball)) continue;
      if (node.left < 0) {
        ++leaves_visited;
        for (int t = node.begin; t < node.end; ++t) {
          const mhgp::i32 id = order[(std::size_t)t];
          ++(*touched);
          if (mhgp::sphere_side(sphere, (*points)[(std::size_t)id]) <= 0) visit(id);
        }
      } else {
        if (top + 2 <= 128) { stack[top++] = node.left; stack[top++] = node.right; }
        else {                                  // pile saturee : ne rien omettre
          descend_all(node.left, sphere, ball, touched, visit);
          descend_all(node.right, sphere, ball, touched, visit);
        }
      }
    }
  }

  template <class Fn>
  void descend_all(int index, const mhgp::Sphere& sphere, const LooseBall& ball,
                   long long* touched, Fn&& visit) const {
    const Node& node = nodes[(std::size_t)index];
    ++nodes_visited;
    if (!node_may_touch(node, ball)) return;
    if (node.left < 0) {
      ++leaves_visited;
      for (int t = node.begin; t < node.end; ++t) {
        const mhgp::i32 id = order[(std::size_t)t];
        ++(*touched);
        if (mhgp::sphere_side(sphere, (*points)[(std::size_t)id]) <= 0) visit(id);
      }
      return;
    }
    descend_all(node.left, sphere, ball, touched, visit);
    descend_all(node.right, sphere, ball, touched, visit);
  }

  // DESACCORD DE SIGNE entre deux spheres du meme pinceau.
  //
  // Le nom importe. Ce n'est PAS la difference symetrique des deux boules
  // fermees : c'est le desaccord TERNAIRE de `sphere_side`, qui vaut -1, 0 ou
  // +1. La distinction n'est pas cosmetique — elle conserve precisement le cas
  // contractuel « sur la coquille a une extremite, strictement interieur a
  // l'autre », que la difference des boules fermees perdrait puisque le point
  // appartient alors aux deux. Un point du CERCLE du flat, lui, a le meme signe
  // nul aux deux extremites : il est deja dans la fermeture et ne doit pas etre
  // redecouvert comme evenement.
  //
  // C'est la requete que le lemme reclame, et pas le balayage d'une boule. Entre
  // deux parametres la puissance d'un point est affine ; un point dont la
  // puissance s'annule strictement entre les deux a donc des signes terminaux
  // strictement opposes, et un point dont elle s'annule a une extremite a un
  // signe nul d'un cote et non nul de l'autre. Balayer une boule entiere serait
  // correct mais ruineux : la sphere d'un sliver de surface est geometriquement
  // enorme bien qu'elle ne contienne qu'une poignee de points, et sa FRONTIERE
  // traverse une grande partie du nuage. Le desaccord, lui, est mince des que
  // l'evenement est proche — c'est le cas courant — et un noeud strictement
  // interieur aux DEUX boules, ou strictement exterieur aux DEUX, est coupe.
  //
  // Les deux coupes sont conservatives : on n'elague que si le noeud est
  // certainement du meme cote des deux spheres, marge d'un demi comprise. Le
  // verdict final reste `mhgp::sphere_side`, exact, sur chaque point atteint.
  struct Straddle { bool certainly_inside = false, certainly_outside = false; };

  static Straddle classify(const Node& node, const LooseBall& ball) {
    Straddle out;
    if (!ball.usable) {
      out.certainly_outside = exact_outside(node, ball);
      out.certainly_inside = exact_strictly_inside(node, ball);
      return out;
    }
    double near2 = 0, far2 = 0;
    for (int d = 0; d < 3; ++d) {
      const double c = ball.centre[d];
      const double lo = (double)node.lo[d], hi = (double)node.hi[d];
      const double gap = c < lo ? (lo - c) : (c > hi ? (c - hi) : 0.0);
      const double reach = std::max(std::abs(c - lo), std::abs(c - hi));
      near2 += gap * gap;
      far2 += reach * reach;
    }
    // `ball.radius` porte deja la marge d'un demi ; l'interieur certain se juge
    // donc contre le rayon DIMINUE de la meme marge.
    const double inner = ball.radius - 1.0;
    if (inner > 0 && far2 <= inner * inner) out.certainly_inside = true;
    if (near2 > ball.radius * ball.radius) out.certainly_outside = true;
    return out;
  }

  template <class Fn>
  void sign_disagreement(const mhgp::Sphere& a, const mhgp::Sphere& b,
                            long long* touched, Fn&& visit) const {
    if (nodes.empty()) return;
    const LooseBall la = loosen(a), lb = loosen(b);
    int stack[128];
    int top = 0;
    stack[top++] = 0;
    while (top > 0) {
      const int self = stack[--top];
      const Node& node = nodes[(std::size_t)self];
      ++nodes_visited;
      const Straddle sa = classify(node, la), sb = classify(node, lb);
      if ((sa.certainly_inside && sb.certainly_inside) ||
          (sa.certainly_outside && sb.certainly_outside)) continue;
      if (node.left < 0) {
        ++leaves_visited;
        for (int t = node.begin; t < node.end; ++t) {
          const mhgp::i32 id = order[(std::size_t)t];
          ++(*touched);
          const mhgp::P3& p = (*points)[(std::size_t)id];
          const int side_a = mhgp::sphere_side(a, p);
          const int side_b = mhgp::sphere_side(b, p);
          if (side_a != side_b) visit(id);
        }
      } else if (top + 2 <= 128) {
        stack[top++] = node.left;
        stack[top++] = node.right;
      } else {
        sign_disagreement_at(node.left, a, b, la, lb, touched, visit);
        sign_disagreement_at(node.right, a, b, la, lb, touched, visit);
      }
    }
  }

  template <class Fn>
  void sign_disagreement_at(int index, const mhgp::Sphere& a, const mhgp::Sphere& b,
                               const LooseBall& la, const LooseBall& lb,
                               long long* touched, Fn&& visit) const {
    const Node& node = nodes[(std::size_t)index];
    ++nodes_visited;
    const Straddle sa = classify(node, la), sb = classify(node, lb);
    if ((sa.certainly_inside && sb.certainly_inside) ||
        (sa.certainly_outside && sb.certainly_outside)) return;
    if (node.left < 0) {
      for (int t = node.begin; t < node.end; ++t) {
        const mhgp::i32 id = order[(std::size_t)t];
        ++(*touched);
        const mhgp::P3& p = (*points)[(std::size_t)id];
        if (mhgp::sphere_side(a, p) != mhgp::sphere_side(b, p)) visit(id);
      }
      return;
    }
    sign_disagreement_at(node.left, a, b, la, lb, touched, visit);
    sign_disagreement_at(node.right, a, b, la, lb, touched, visit);
  }

  // Pave ENTIER, teste exactement. Sert d'amorce : aucune exactitude n'en depend.
  template <class Fn>
  void box(const long long* lo, const long long* hi, long long* touched, Fn&& visit) const {
    if (nodes.empty()) return;
    int stack[128];
    int top = 0;
    stack[top++] = 0;
    while (top > 0) {
      const Node& node = nodes[(std::size_t)stack[--top]];
      ++nodes_visited;
      bool disjoint = false;
      for (int d = 0; d < 3; ++d)
        if ((long long)node.hi[d] < lo[d] || (long long)node.lo[d] > hi[d]) disjoint = true;
      if (disjoint) continue;
      if (node.left < 0) {
        ++leaves_visited;
        for (int t = node.begin; t < node.end; ++t) {
          const mhgp::i32 id = order[(std::size_t)t];
          const mhgp::P3& p = (*points)[(std::size_t)id];
          if (p.x < lo[0] || p.x > hi[0] || p.y < lo[1] || p.y > hi[1] ||
              p.z < lo[2] || p.z > hi[2]) continue;
          ++(*touched);
          visit(id);
        }
      } else if (top + 2 <= 128) {
        stack[top++] = node.left;
        stack[top++] = node.right;
      } else {
        // PILE SATUREE : les deux autres requetes retombaient sur une descente
        // recursive, celle-ci OMETTAIT le sous-arbre en silence. Une requete
        // d'index n'a pas le droit de perdre un point.
        box_at(node.left, lo, hi, touched, visit);
        box_at(node.right, lo, hi, touched, visit);
      }
    }
  }

  template <class Fn>
  void box_at(int index, const long long* lo, const long long* hi, long long* touched,
              Fn&& visit) const {
    const Node& node = nodes[(std::size_t)index];
    ++nodes_visited;
    for (int d = 0; d < 3; ++d)
      if ((long long)node.hi[d] < lo[d] || (long long)node.lo[d] > hi[d]) return;
    if (node.left < 0) {
      ++leaves_visited;
      for (int t = node.begin; t < node.end; ++t) {
        const mhgp::i32 id = order[(std::size_t)t];
        const mhgp::P3& p = (*points)[(std::size_t)id];
        if (p.x < lo[0] || p.x > hi[0] || p.y < lo[1] || p.y > hi[1] ||
            p.z < lo[2] || p.z > hi[2]) continue;
        ++(*touched);
        visit(id);
      }
      return;
    }
    box_at(node.left, lo, hi, touched, visit);
    box_at(node.right, lo, hi, touched, visit);
  }
};


// ---------------------------------------------------------------------------
// GATE D — PARENT LOCAL POUR LA REVERSE SEARCH
// ---------------------------------------------------------------------------
//
// `NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md` demontre qu'un parent unique se
// choisit AU SOMMET, sans `seen`, sans mosaique globale et sans enumerer tous
// les voisins pour decider lequel est le parent. Ce bloc calcule la direction du
// parent ; il ne remplace pas encore le parcours, il le juge.
//
// Le cone tangent de la chambre en v est K_v = { d : a_s . d >= 0 pour tout s
// dans S(v) }, et ses rayons extremes sont exactement les orientations des flats
// fermes de rang trois incidents. Il n'y a donc pas de programme lineaire a
// resoudre dans le prototype : les rayons candidats sont deja enumeres par le
// parcours, et il suffit de les filtrer puis d'en prendre un canonique.
//
// LA DIRECTION D'UN FLAT, ET SON SIGNE TANGENT, SANS AUCUN GRAND ENTIER.
//
// J'avais derive la direction depuis le circumcentre `sphere3`, ce qui donnait
// des produits frolant 2^127 et m'obligeait a passer en `BigInt<4>`. La note
// §6 donne bien plus simple. Pour une base planaire (a,b,c) et
// u = (b-a) x (c-a), un rayon ENTIER du pinceau est directement
//
//     d = (u, 2 u . a),
//
// et comme a_i = (-2 p_i, 1),
//
//     a_i . d = -2 p_i . u + 2 u . a = 2 u . (a - p_i) = -2 orient3d(a,b,c,p_i).
//
// Les deux formes coincident : le circumcentre dans le plan verifie
// (c_0 - a) . u = 0, donc 2 c_0 . u = 2 a . u. Le signe tangent se lit donc avec
// le predicat entier que le pinceau evalue deja, et rien d'autre.
inline int tangent_sign(int orient_of_site, int orientation) {
  const int raw = -orient_of_site;          // signe de a_i . d
  return orientation > 0 ? raw : -raw;
}

// LA TRONCATURE NE PEUT PLUS REVENIR PAR INADVERTANCE.
//
// L'argument attendu est un SIGNE dans {-1,0,+1}, pas un determinant. Le chemin
// proprietaire passait la valeur brute de `orient3d_exact`, de type `i128` : la
// conversion implicite gardait les 32 bits bas, ce qui rendait un signe faux
// des l'echelle u16 1291 et laissait `-INT_MIN` indefini a l'echelle 1025.
// La surcharge supprimee transforme cette faute en erreur de compilation ; les
// appelants doivent ecrire `tangent_sign(sign_of(det), direction)`.
int tangent_sign(mhgp::i128 orient_of_site, int orientation) = delete;
int tangent_sign(long long orient_of_site, int orientation) = delete;

// ---------------------------------------------------------------------------
// LE PARCOURS.
//
// Coupe UNIQUEMENT sur le niveau strict. Aretes = flats fermes de rang trois,
// enumeres par leurs PLANS distincts et non par les C(m,3) triplets. Transition
// par lots. Le census exact peut etre active : il ne corrige rien, il refute.
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// REVERSE SEARCH — le parcours SANS `seen`, `frontier` ni `visited`
// ---------------------------------------------------------------------------
//
// Le theoreme de parent local dit qu'un sommet non racine a un parent unique
// calculable depuis lui seul. Il rend donc l'enumeration STATELESS : on descend
// de v vers w si et seulement si pi(w) = v. Aucune deduplication n'est
// necessaire, donc aucune table partagee, donc aucune ecriture concurrente — et
// c'est cela, et non les quarante-huit coeurs, qui rend un front d'onde GPU
// possible.
//
// La memoire de navigation devient la PILE : par niveau, l'indice du fils
// courant et la coquille du parent. Le BFS reste en place comme oracle borne, et
// le differentiel exige que les deux parcours visitent exactement le meme
// ensemble de sommets.
//
// Les fonctions ci-dessous factorisent ce que le BFS faisait en ligne : parcourir
// les flats d'un sommet dans un ordre DETERMINISTE — sans quoi les indices de
// fils ne seraient pas reproductibles au retour —, puis suivre un flat dans une
// direction.
namespace flats {

struct FlatAtVertex {
  i32 base[3] = {-1, -1, -1};
  std::vector<i32> closure;
};

// Enumere les flats fermes de rang trois incidents a v, chacun UNE fois, dans
// l'ordre des triplets de la coquille. Le quotient est le meme que celui du
// BFS : un triplet qui n'est pas la base canonique de sa fermeture est ecarte.
//
// Le callback rend `false` pour ARRETER. Sans cela, une reverse search qui vient
// de trouver son fils continuait d'enumerer les triplets restants et de
// reconstruire leurs fermetures : le `return` du lambda ne sortait que du
// callback. C'etait a la fois du travail jete et un compteur trompeur.
// REPRISE A UN CURSEUR.
//
// La reverse search revient sur un sommet autant de fois qu'il a de fils, et elle
// RECONSTRUISAIT a chaque retour les fermetures de tous les triplets deja consommes
// pour retrouver sa place. Le test de base canonique ne depend que du triplet et de
// la coquille, jamais des triplets precedents, donc reprendre directement au curseur
// est licite — et les triplets sautes ne sont plus touches du tout.
//
// J'ai d'abord publie que ce changement ne gagnait RIEN, sur deux mesures identiques
// au triplet pres. Elles l'etaient parce que le curseur n'etait pas branche : l'appel
// qui devait le relier avait echoue et je n'avais rebuilde que le binaire. Une
// identite exacte de compteurs est le signe qu'un code neuf ne s'execute pas, pas
// celui d'une optimisation inutile.
//
// Branche et remesure : 192 570 -> 151 708 fermetures sur la campagne cospherique,
// 108 856 -> 89 130 sur la generique. Environ 1,2x.
//
// Le callback recoit la position (i,j,k) du triplet, qui EST l'adresse de reprise.
template <class Fn>
inline void for_each_flat_from(const std::vector<P3>& points, const Vertex& v, int i0, int j0,
                               int k0, Fn&& visit, long long* triplets = nullptr,
                               long long* closures = nullptr, long long* closure_high = nullptr) {
  const int m = (int)v.shell.size();
  if (m < 3) return;
  int i = i0, j = j0, k = k0;
  if (i < 0 || j <= i || k <= j || k >= m || j >= m - 1 || i >= m - 2) return;
  std::vector<i32> closure;
  for (;;) {
    bool deliver = false;
    FlatAtVertex flat;
    do {
      const i32 ta = v.shell[(std::size_t)i], tb = v.shell[(std::size_t)j],
                tc = v.shell[(std::size_t)k];
      if (triplets != nullptr) ++*triplets;
      {
        const mhgp::P3 u = mhgp::p3_sub(points[(std::size_t)tb], points[(std::size_t)ta]);
        const mhgp::P3 w = mhgp::p3_sub(points[(std::size_t)tc], points[(std::size_t)ta]);
        const mhgp::P3 x = mhgp::p3_cross(u, w);
        if (x.x == 0 && x.y == 0 && x.z == 0) break;
      }
      closure.clear();
      if (closures != nullptr) ++*closures;
      for (int t = 0; t < m; ++t) {
        const i32 z = v.shell[(std::size_t)t];
        if (orient3d_exact(points[(std::size_t)ta], points[(std::size_t)tb],
                           points[(std::size_t)tc], points[(std::size_t)z]) == 0)
          closure.push_back(z);
      }
      if (closure_high != nullptr && (long long)closure.size() > *closure_high)
        *closure_high = (long long)closure.size();
      i32 canonical[3] = {-1, -1, -1};
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
          canonical[0] = closure[(std::size_t)x];
          canonical[1] = closure[(std::size_t)y];
          canonical[2] = closure[(std::size_t)z];
          found = true;
        }
        if (!found) break;
      }
      if (!(canonical[0] == ta && canonical[1] == tb && canonical[2] == tc)) break;
      flat.base[0] = ta; flat.base[1] = tb; flat.base[2] = tc;
      flat.closure = closure;
      deliver = true;
    } while (false);
    if (deliver && !visit(flat, i, j, k)) return;
    if (++k >= m) {
      if (++j >= m - 1) { if (++i >= m - 2) return; j = i + 1; }
      k = j + 1;
    }
  }
}

template <class Fn>
inline void for_each_flat(const std::vector<P3>& points, const Vertex& v, Fn&& visit,
                          long long* triplets = nullptr, long long* closures = nullptr,
                          long long* closure_high = nullptr) {
  for_each_flat_from(points, v, 0, 1, 2,
                     [&](const FlatAtVertex& flat, int, int, int) { return visit(flat); },
                     triplets, closures, closure_high);
}

// LE COUPLE DE RETOUR, teste en O(m) et sans aucune fermeture.
//
// On descend de v vers w le long du plan de `base`, dans la direction `forward`.
// Si w est notre fils, alors pi(w) emprunte le MEME plan en direction opposee.
// Tester l'admissibilite de ce seul couple coute un `orient3d` par point de la
// coquille de w ; s'il echoue, pi(w) != v est certifie et le refus n'a coute
// aucune enumeration de flats. C'est une condition NECESSAIRE, pas suffisante : un
// couple admissible peut ne pas etre le premier dans l'ordre.
// L'admissibilite d'UN couple (base ordonnee, direction), ecrite UNE fois et
// partagee par le choix du parent, le prefiltre de retour et la decision de
// filiation. La base ordonnee et la direction se transportent ENSEMBLE : une
// permutation impaire de la base sans inversion de la direction changerait le
// signe de `orient3d` et rendrait le test faux.
inline bool pair_admissible(const std::vector<P3>& points, const Vertex& w, const i32 base[3],
                            int direction, const std::vector<i32>& root_base) {
  // DOMAINE. `direction` n'a que deux valeurs licites. Les tests s'ecrivent
  // `direction > 0`, si bien que 0 et 2 aliasaient silencieusement +1 : un appelant
  // fautif obtenait un verdict au lieu d'un refus.
  if (direction != -1 && direction != 1) return false;
  Pencil pencil{&points, base[0], base[1], base[2]};
  for (i32 z : w.shell)
    if (tangent_sign(pencil.orient_of(z), direction) < 0) return false;
  const i32 site = w.interior.empty() ? -1 : w.interior.front();
  if (site >= 0) return tangent_sign(pencil.orient_of(site), direction) > 0;
  mhgp::i128 total = 0;
  for (i32 z : root_base)
    total += orient3d_exact(points[(std::size_t)base[0]], points[(std::size_t)base[1]],
                            points[(std::size_t)base[2]], points[(std::size_t)z]);
  const mhgp::i128 derivative = (direction > 0) ? -total : total;
  return derivative < 0;
}

inline bool backward_pair_admissible(const std::vector<P3>& points, const Vertex& w,
                                     const i32 base[3], int forward,
                                     const std::vector<i32>& root_base) {
  return pair_admissible(points, w, base, -forward, root_base);
}

// ---------------------------------------------------------------------------
// DECIDER LA FILIATION SANS AUCUNE REQUETE DE VOISIN DE RETOUR
// ---------------------------------------------------------------------------
//
// Corollaire de l'auditeur, plus fort que le prefiltre. L'adjacence du pinceau est
// SYMETRIQUE : puisque w est le prochain evenement depuis v le long de (C,d), le
// prochain evenement depuis w le long de (C,-d) est deja connu — c'est v. Apres un
// prefiltre positif, rappeler `neighbour_along` depuis w ne fait que recalculer v.
//
// Il suffit donc d'enumerer les couples de w JUSQU'A la clef canonique (C,-d) :
//   * un couple admissible strictement anterieur refute la filiation ;
//   * la clef de retour atteinte et admissible l'accepte ;
//   * fermeture manquante, ordre qui regresse ou retour inadmissible : ECHEC FERME.
//
// La fermeture est la MEME aux deux extremites : les nouveaux membres du lot ont un
// `orient3d` non nul et n'agrandissent pas le plan, et les partants disparaissent
// du plan sans le changer. Comme la base canonique est une fonction de la seule
// fermeture, les deux bases coincident — ce qui est verifie, non suppose, et rend
// inutile tout transport de signe entre les deux reperes.
enum class ChildOutcome { kAccept, kReject, kBroken };

inline ChildOutcome decide_child(const std::vector<P3>& points, const Vertex& w,
                                 const FlatAtVertex& flat_at_v, int direction,
                                 const std::vector<i32>& root_base, long long* triplets = nullptr,
                                 long long* closures = nullptr) {
  if (direction != -1 && direction != 1) return ChildOutcome::kBroken;
  const int back_slot = (-direction > 0) ? 1 : 0;
  ChildOutcome verdict = ChildOutcome::kBroken;   // jamais atteint = echec ferme
  // La clef PRECEDENTE est memorisee : le commentaire promettait un echec sur
  // regression, mais comparer seulement a la cible ne detecte pas un desordre entre
  // deux clefs anterieures. L'ordre strict de l'enumeration est une hypothese de la
  // decision, donc il est verifie a chaque pas.
  std::vector<i32> previous;
  bool have_previous = false;
  for_each_flat(points, w, [&](const FlatAtVertex& g) {
    if (have_previous && !(previous < g.closure)) return false;   // regression : ferme
    previous = g.closure;
    have_previous = true;
    if (g.closure > flat_at_v.closure) return false;      // depasse : echec ferme
    const bool is_return = (g.closure == flat_at_v.closure);
    if (is_return && !(g.base[0] == flat_at_v.base[0] && g.base[1] == flat_at_v.base[1] &&
                       g.base[2] == flat_at_v.base[2]))
      return false;                                       // bases disjointes : echec ferme
    for (int slot = 0; slot < 2; ++slot) {
      const int dir = slot == 0 ? -1 : 1;
      const bool admissible = pair_admissible(points, w, g.base, dir, root_base);
      if (is_return && slot == back_slot) {
        verdict = admissible ? ChildOutcome::kAccept : ChildOutcome::kBroken;
        return false;
      }
      if (admissible) { verdict = ChildOutcome::kReject; return false; }
    }
    return true;
  }, triplets, closures);
  return verdict;
}

// ---------------------------------------------------------------------------
// LE PROPRIETAIRE D'UN SUPPORT — la piece qui remplace la table `emitted`
// ---------------------------------------------------------------------------
//
// Pour un support independant U de sphere minimale x_U, posons
// B_U = {i : L_i(x_U) < 0}. Le polyedre de propriete est F_U intersecte avec
// {L_i <= 0 pour i dans B_U} et {L_j >= 0 pour j hors B_U union U}. Un sommet v
// qui contient U appartient a ce polyedre si et seulement si
//
//     U inclus dans S(v),   B(v) inclus dans B_U,   B_U inclus dans B(v) union S(v).
//
// La premiere inclusion seule est le prefiltre vivant ; la seconde interdit qu'un
// ancien interieur soit devenu exterieur. En minimisant G_U = somme des formes
// positives sur ce polyedre, avec depart lexicographique exact des coordonnees, on
// obtient un unique sommet o(U) — sans aucune table.
//
// CRITERE LOCAL. Avec eps_s = -1 pour s dans B_U inter S(v) et +1 sinon, le cone
// tangent SIGNE est {d : a_u . d = 0 pour u dans U, eps_s a_s . d >= 0 sinon}.
// Employer a sa place le cone non signe de la chambre rejetterait le vrai
// proprietaire : un membre de B_U actif en coquille doit pouvoir devenir
// strictement interieur. Alors v = o(U) si et seulement si aucun rayon extreme
// admissible n'a (g_U . d, d_0, d_1, d_2, d_3) lexicographiquement negatif.
//
// Le gradient ne coute pas O(n) : avec A_X = somme des a_i precalculee une fois,
// g_U = A_X - somme_{u dans U} a_u - 2 somme_{i dans B_U} a_i, et les termes de U
// s'annulent sur le cone. Comme a_i . d = -2 orient3d(base, p_i) pour
// d = (u, 2 u . a), tout se lit avec le meme predicat entier que le pinceau.
//
// Arite quatre : aucun rayon, la sphere EST le sommet. Arite trois : la droite du
// pinceau, deux orientations. Arite deux : le cone vit dans un plan, ses rayons
// extremes portent les droites de bord, c'est-a-dire les plans U union {s}.
struct OwnerContext {
  mhgp::i128 sum_x = 0, sum_y = 0, sum_z = 0;   // A_X = (-2 somme p, n)
  mhgp::i128 count = 0;
};

inline OwnerContext owner_context(const std::vector<P3>& points) {
  OwnerContext ctx;
  for (const P3& p : points) {
    ctx.sum_x += p.x;
    ctx.sum_y += p.y;
    ctx.sum_z += p.z;
  }
  ctx.count = (mhgp::i128)points.size();
  return ctx;
}

// Rend `false` des qu'un rayon extreme admissible ameliore strictement : v n'est
// alors pas le proprietaire.
inline bool owner_rays_ok(const std::vector<P3>& points, const OwnerContext& ctx,
                          const Vertex& v, const mhgp::i32* support, int q,
                          const mhgp::i32 base[3], const std::vector<i32>& b_u) {
  const P3& a = points[(std::size_t)base[0]];
  const mhgp::P3 e1 = mhgp::p3_sub(points[(std::size_t)base[1]], a);
  const mhgp::P3 e2 = mhgp::p3_sub(points[(std::size_t)base[2]], a);
  const mhgp::P3 u = mhgp::p3_cross(e1, e2);
  if (u.x == 0 && u.y == 0 && u.z == 0) return true;      // triplet aligne : pas un rayon
  // A_X . d = 2 u . (n a - somme p), pour l'orientation +1.
  const mhgp::i128 ax = ctx.count * (mhgp::i128)a.x - ctx.sum_x;
  const mhgp::i128 ay = ctx.count * (mhgp::i128)a.y - ctx.sum_y;
  const mhgp::i128 az = ctx.count * (mhgp::i128)a.z - ctx.sum_z;
  const mhgp::i128 axd = 2 * ((mhgp::i128)u.x * ax + (mhgp::i128)u.y * ay + (mhgp::i128)u.z * az);
  mhgp::i128 inside = 0;
  for (i32 z : b_u)
    inside += orient3d_exact(a, points[(std::size_t)base[1]], points[(std::size_t)base[2]],
                             points[(std::size_t)z]);
  const mhgp::i128 gd_plus = axd + 4 * inside;
  const mhgp::i128 ua = (mhgp::i128)u.x * a.x + (mhgp::i128)u.y * a.y + (mhgp::i128)u.z * a.z;
  for (int delta = -1; delta <= 1; delta += 2) {
    bool admissible = true;
    for (i32 s : v.shell) {
      bool in_support = false;
      for (int i = 0; i < q; ++i) if (support[i] == s) in_support = true;
      if (in_support) continue;
      const int eps = std::binary_search(b_u.begin(), b_u.end(), s) ? -1 : 1;
      // Le predicat rend un `i128`. Le passer directement a `tangent_sign(int,...)`
      // tronquait les bits hauts : sur u16 le determinant depasse `INT_MAX` des
      // l'echelle 1291, le signe devenait arbitraire et `-INT_MIN` etait un
      // comportement indefini. On reduit AVANT l'appel.
      const int t = tangent_sign(sign_of(orient3d_exact(a, points[(std::size_t)base[1]],
                                                        points[(std::size_t)base[2]],
                                                        points[(std::size_t)s])), delta);
      if (eps * t < 0) { admissible = false; break; }
    }
    if (!admissible) continue;
    const mhgp::i128 gd = (delta > 0) ? gd_plus : -gd_plus;
    if (gd < 0) return false;
    if (gd > 0) continue;
    const mhgp::i128 d[4] = {(mhgp::i128)delta * u.x, (mhgp::i128)delta * u.y,
                             (mhgp::i128)delta * u.z, (mhgp::i128)delta * 2 * ua};
    for (int i = 0; i < 4; ++i) {
      if (d[i] < 0) return false;
      if (d[i] > 0) break;
    }
  }
  return true;
}

inline bool is_owner(const std::vector<P3>& points, const OwnerContext& ctx, const Vertex& v,
                     const mhgp::i32* support, int q, const std::vector<i32>& b_u) {
  if (q < 2 || q > 4) return false;
  for (int i = 0; i < q; ++i)
    if (!std::binary_search(v.shell.begin(), v.shell.end(), support[i])) return false;
  for (i32 z : v.interior)
    if (!std::binary_search(b_u.begin(), b_u.end(), z)) return false;
  for (i32 z : b_u)
    if (!std::binary_search(v.interior.begin(), v.interior.end(), z) &&
        !std::binary_search(v.shell.begin(), v.shell.end(), z)) return false;
  if (q == 4) return true;                                 // aucun rayon
  if (q == 3) {
    const mhgp::i32 base[3] = {support[0], support[1], support[2]};
    return owner_rays_ok(points, ctx, v, support, q, base, b_u);
  }
  for (i32 s : v.shell) {
    if (s == support[0] || s == support[1]) continue;
    const mhgp::i32 base[3] = {support[0], support[1], s};
    if (!owner_rays_ok(points, ctx, v, support, q, base, b_u)) return false;
  }
  return true;
}

// La direction canonique du parent, ou `false` si aucune orientation n'est
// admissible — ce qui n'arrive qu'a la racine. Deux filtres exacts, comme au
// BFS : rester dans la chambre, et faire croitre L_h ou decroitre Q_r.
//
// ---------------------------------------------------------------------------
// LE PREMIER ADMISSIBLE SUFFIT, ET LE MINIMUM N'ETAIT PAS L'HYPOTHESE UTILE
// ---------------------------------------------------------------------------
//
// Cette requete dominait le cout. Le parcours n'enumere qu'une fois les flats du
// sommet courant, mais CHAQUE fils candidat payait ensuite une enumeration
// COMPLETE des flats de son propre sommet — environ 5,7 par sommet visite. C'est
// la, et pas dans la descente, que passait le travail.
//
// Ce qu'Avis--Fukuda demande de pi n'est pas d'etre un minimum : c'est d'etre une
// fonction DETERMINISTE du sommet seul, acyclique, a racine unique. Les deux
// dernieres proprietes viennent du POTENTIEL et valent pour n'importe quel choix
// admissible — toute direction admissible fait strictement croitre L_h a ensemble
// interieur egal, ou strictement decroitre Q_r au niveau zero, donc pi decroit
// strictement un potentiel et ne peut pas cycler ; et le seul sommet sans
// direction admissible est le germe. Prendre le PREMIER couple admissible dans un
// ordre d'enumeration fixe est donc un parent legitime, et l'arret est immediat.
//
// Il se trouve en outre que ce premier admissible EST l'ancien minimum. Trois
// points distincts d'une meme sphere ne sont jamais alignes — une droite coupe
// une sphere en au plus deux points —, donc la base canonique d'un flat de
// coquille est exactement le triplet des TROIS PLUS PETITS elements de sa
// fermeture ; deux flats distincts ont des bases distinctes, leurs fermetures se
// separent des les trois premiers elements, et comparer les fermetures revient a
// comparer les bases. Comme `for_each_flat` balaye les triplets de la coquille
// TRIEE dans l'ordre lexicographique et ne livre que des bases canoniques, il
// livre les flats dans l'ordre strictement croissant de leur clef, et la direction
// -1 precede +1. La semantique du parent est donc inchangee, et non seulement
// valide.
//
// Cette derniere affirmation n'est pas une evidence : le differentiel verifie la
// monotonie clef par clef sur chaque sommet, et compare le parent a sortie
// precoce au balayage complet.
inline bool canonical_parent(const std::vector<P3>& points, const Vertex& v,
                             const std::vector<i32>& root_base, FlatAtVertex* flat_out,
                             int* orientation_out, long long* triplets = nullptr,
                             long long* closures = nullptr, bool full_scan = false) {
  const i32 site = v.interior.empty() ? -1 : v.interior.front();
  std::vector<i32> best_key;
  bool found = false;
  (void)site;
  for_each_flat(points, v, [&](const FlatAtVertex& flat) {
    for (int direction = -1; direction <= 1; direction += 2) {
      if (!pair_admissible(points, v, flat.base, direction, root_base)) continue;
      std::vector<i32> key = flat.closure;
      key.push_back(direction > 0 ? 1 : 0);
      if (!found || key < best_key) {
        best_key = key;
        *flat_out = flat;
        *orientation_out = direction;
        found = true;
      }
      if (!full_scan) return false;  // premier admissible = minimum
    }
    return true;
  }, triplets, closures);
  return found;
}

// Suit un flat dans une direction et rend le sommet suivant, ou `false` si le
// pinceau est non borne de ce cote. Meme certification qu'au BFS : amorce par
// l'ensemble interieur puis pave croissant, puis desaccord de signe entre les
// deux spheres terminales, puis repli exhaustif.
inline bool neighbour_along(const std::vector<P3>& points, const Vertex& v,
                            const FlatAtVertex& flat, int direction,
                            const CertifiedIndex* index, FlatStatistics* st, Vertex* out) {
  const int n = (int)points.size();
  const int m = (int)v.shell.size();
  Pencil pencil{&points, flat.base[0], flat.base[1], flat.base[2]};
  i32 apex = -1;
  int orient_apex = 0;
  for (int t = 0; t < m; ++t) {
    const i32 z = v.shell[(std::size_t)t];
    const int oz = pencil.orient_of(z);
    if (oz != 0) { apex = z; orient_apex = oz; break; }
  }
  if (apex < 0) return false;

  ++st->pencil_queries;
  i32 best = -1;
  int best_orient = 0;
  std::vector<char> seen_candidate((std::size_t)n, 0);
  std::vector<i32> touched;
  auto absorb = [&](i32 z) {
    if (seen_candidate[(std::size_t)z]) return;
    seen_candidate[(std::size_t)z] = 1;
    touched.push_back(z);
    if (std::binary_search(v.shell.begin(), v.shell.end(), z)) return;
    const int oz = pencil.orient_of(z);
    if (oz == 0) return;
    ++st->pencil_candidates;
    if (pencil.compare_t(z, oz, apex, orient_apex) != direction) return;
    if (best < 0) { best = z; best_orient = oz; return; }
    if (pencil.compare_t(z, oz, best, best_orient) == -direction) { best = z; best_orient = oz; }
  };

  mhgp::Sphere apex_sphere{};
  mhgp::i32 apex_support[4] = {flat.base[0], flat.base[1], flat.base[2], apex};
  std::sort(apex_support, apex_support + 4);
  const bool apex_ok = mhgp::sphere4(points[(std::size_t)apex_support[0]],
                                     points[(std::size_t)apex_support[1]],
                                     points[(std::size_t)apex_support[2]],
                                     points[(std::size_t)apex_support[3]], &apex_sphere);
  if (index == nullptr) {
    for (i32 z = 0; z < n; ++z) absorb(z);
  } else {
    for (i32 z : v.interior) absorb(z);
    const mhgp::P3& anchor = points[(std::size_t)flat.base[0]];
    long long half = 4;
    bool covered = false;
    while (best < 0 && !covered) {
      ++st->bootstrap_rounds;
      const long long lo[3] = {(long long)anchor.x - half, (long long)anchor.y - half,
                               (long long)anchor.z - half};
      const long long hi[3] = {(long long)anchor.x + half, (long long)anchor.y + half,
                               (long long)anchor.z + half};
      index->box(lo, hi, &st->grid_points_touched, absorb);
      covered = (half >= (long long)kDeclaredGridMaximum);
      half *= 2;
    }
    if (covered && best < 0) ++st->full_grid_sweeps;
    for (int round = 0; apex_ok && round < 8 && best >= 0; ++round) {
      const i32 previous = best;
      mhgp::i32 bs[4] = {flat.base[0], flat.base[1], flat.base[2], best};
      std::sort(bs, bs + 4);
      mhgp::Sphere best_sphere{};
      if (!mhgp::sphere4(points[(std::size_t)bs[0]], points[(std::size_t)bs[1]],
                         points[(std::size_t)bs[2]], points[(std::size_t)bs[3]],
                         &best_sphere)) break;
      ++st->disagreement_sweeps;
      index->sign_disagreement(apex_sphere, best_sphere, &st->grid_points_touched, absorb);
      if (best == previous) break;
    }
    if (best < 0) {
      ++st->exhaustive_scans;
      for (i32 z = 0; z < n; ++z) absorb(z);
    }
  }
  if (best < 0) { ++st->unbounded_stops; return false; }

  std::vector<i32> batch;
  for (i32 z : touched) {
    if (std::binary_search(v.shell.begin(), v.shell.end(), z)) continue;
    const int oz = pencil.orient_of(z);
    if (oz == 0) continue;
    if (pencil.compare_t(z, oz, best, best_orient) == 0) batch.push_back(z);
  }
  std::sort(batch.begin(), batch.end());
  st->batch_high_water = std::max(st->batch_high_water, (long long)batch.size());
  st->touched_high_water = std::max(st->touched_high_water, (long long)touched.size());
  if (batch.size() > 1) ++st->batches_multiple;

  std::vector<i32> shell = flat.closure;
  shell.insert(shell.end(), batch.begin(), batch.end());
  std::sort(shell.begin(), shell.end());

  std::vector<i32> interior = v.interior;
  for (int t = 0; t < m; ++t) {
    const i32 z = v.shell[(std::size_t)t];
    if (std::binary_search(flat.closure.begin(), flat.closure.end(), z)) continue;
    if (pencil.side(best, z, best_orient) < 0) interior.push_back(z);
  }
  for (i32 z : batch)
    if (pencil.side(apex, z, orient_apex) < 0) {
      const auto it = std::find(interior.begin(), interior.end(), z);
      if (it != interior.end()) interior.erase(it);
    }
  std::sort(interior.begin(), interior.end());
  interior.erase(std::unique(interior.begin(), interior.end()), interior.end());

  st->shell_high_water = std::max(st->shell_high_water, (long long)shell.size());
  st->interior_high_water = std::max(st->interior_high_water, (long long)interior.size());
  out->shell = shell;
  out->interior = interior;
  out->level = (int)interior.size();
  return true;
}

}  // namespace flats

inline std::vector<flats::Vertex> navigate_shallow(const std::vector<mhgp::P3>& points,
                                                   int level_ceiling,
                                                   FlatStatistics* st,
                                                   CloudStatus* status,
                                                   bool verify_census,
                                                   const CertifiedIndex* index = nullptr,
                                                   std::vector<flats::ParentEdge>* parents =
                                                       nullptr) {
  // CONTRAT DE PROPRIETE DE L'INDEX. `index` doit avoir ete construit sur CE
  // vecteur `points`, non modifie depuis : ses boites seraient sinon perimees et
  // l'elagage omettrait des points. La seule construction autorisee dans ce
  // fichier est celle de `flat_catalogue`, qui le batit juste avant de naviguer
  // sur la meme vue. Passer ici un index etranger est un usage hors contrat.
  using namespace flats;
  std::vector<Vertex> visited;
  const int n = (int)points.size();
  *status = CloudStatus::kOk;
  if (!inside_declared_grid(points)) {
    *status = CloudStatus::kOutsideDeclaredGrid;
    return visited;
  }
  if (has_duplicate_coordinates(points)) {
    *status = CloudStatus::kDuplicateCoordinates;
    return visited;
  }
  if (n < 4) { *status = CloudStatus::kTooFewPoints; return visited; }
  if (!affine_dimension_is_three(points)) {
    *status = CloudStatus::kAffineDimensionBelowThree;
    return visited;
  }
  if (level_ceiling < 0) return visited;

  Vertex seed;
  const CloudStatus seeded = seed_level_zero(points, st, &seed);
  if (seeded != CloudStatus::kOk) { *status = seeded; return visited; }

  // Base independante canonique de la coquille du germe : les quatre formes
  // dont la somme est Q_r. Le germe en est l'unique minimum sur P_vide, MAIS
  // seulement si les quatre formes a_s = (-2 p_s, 1) sont independantes,
  // c'est-a-dire si les quatre points sont affinement independants. Prendre les
  // quatre premiers de la coquille ne suffit pas : sur une grille saturee une
  // coquille de cinq points en contient quatre COPLANAIRES, Q_r cesse d'avoir
  // le germe pour unique minimum, et un sommet de niveau zero se retrouve sans
  // direction admissible. Mesure : un nuage sur six cents, mais a tous les
  // ordres, avec exactement une racine surnumeraire.
  std::vector<i32> root_potential_base;
  for (i32 z : seed.shell) {
    if ((int)root_potential_base.size() >= 4) break;
    const int have = (int)root_potential_base.size();
    if (have == 1) {
      const mhgp::P3& u = points[(std::size_t)root_potential_base[0]];
      const mhgp::P3& w = points[(std::size_t)z];
      if (u.x == w.x && u.y == w.y && u.z == w.z) continue;
    } else if (have == 2) {
      const mhgp::P3 u = mhgp::p3_sub(points[(std::size_t)root_potential_base[1]],
                                      points[(std::size_t)root_potential_base[0]]);
      const mhgp::P3 w = mhgp::p3_sub(points[(std::size_t)z],
                                      points[(std::size_t)root_potential_base[0]]);
      const mhgp::P3 c = mhgp::p3_cross(u, w);
      if (c.x == 0 && c.y == 0 && c.z == 0) continue;
    } else if (have == 3) {
      if (orient3d_exact(points[(std::size_t)root_potential_base[0]],
                         points[(std::size_t)root_potential_base[1]],
                         points[(std::size_t)root_potential_base[2]],
                         points[(std::size_t)z]) == 0) continue;
    }
    root_potential_base.push_back(z);
  }
  if ((int)root_potential_base.size() < 4) {
    // La coquille du germe engendre R^4 par construction : ne pas y trouver
    // quatre formes independantes serait une contradiction, pas un cas limite.
    *status = CloudStatus::kInvariantViolated;
    return visited;
  }

  std::unordered_set<std::vector<i32>, ShellHash> seen;
  std::vector<Vertex> frontier;
  seen.insert(seed.shell);
  frontier.push_back(seed);

  std::vector<i32> closure, batch, shell_check, interior_check, touched_ids;
  std::vector<char> seen_candidate((std::size_t)n, 0);
  while (!frontier.empty()) {
    const Vertex v = frontier.back();
    frontier.pop_back();
    const int m = (int)v.shell.size();
    visited.push_back(v);
    if (parents != nullptr) parents->push_back(flats::ParentEdge{});
    ++st->vertices_visited;
    if (m > 4) ++st->shells_multiple;

    // GATE D. Le rayon du parent se choisit ICI, parmi les orientations
    // admissibles des flats incidents — les rayons extremes du cone tangent —
    // et non parmi les voisins deja construits. Deux filtres exacts :
    // l'orientation doit rester dans la chambre, c'est-a-dire ne rendre aucun
    // membre de coquille interieur, et elle doit faire croitre L_h pour
    // h = min B(v), ou decroitre Q_r au niveau zero. Le choix canonique parmi
    // les admissibles est le plus petit couple (fermeture, orientation) ; toute
    // regle deterministe convient, la preuve n'exige que les deux filtres.
    const i32 potential_site = v.interior.empty() ? -1 : v.interior.front();
    std::vector<i32> parent_key;
    int parent_orientation = 0;
    std::vector<i32> parent_closure;

    if (verify_census) {
      // Ne pas pouvoir reconstruire la sphere d'un sommet EST une contradiction :
      // un sommet d'arrangement a par definition quatre hyperplans independants.
      if (!census(points, v, &shell_check, &interior_check)) {
        ++st->census_mismatch_shell;
        *status = CloudStatus::kInvariantViolated;
        visited.clear();
        return visited;
      }
      {
        ++st->census_checks;
        // FAIL-CLOSED. Compter la contradiction sans changer le statut laissait
        // l'API fail-open : seul le binaire du juge lisait les compteurs, tout
        // autre appelant obtenait un catalogue construit sur un transport
        // demenit par son propre census.
        // L'ENSEMBLE interieur est compare, pas seulement son cardinal : c'est
        // lui qui est transporte, et deux ensembles de meme taille mais de
        // contenu different produiraient des candidats faux sans jamais faire
        // rougir un compteur de niveau.
        const bool contradicted = (shell_check != v.shell) || (interior_check != v.interior);
        if (shell_check != v.shell) ++st->census_mismatch_shell;
        if (interior_check != v.interior) ++st->census_mismatch_level;
        if (contradicted) {
          *status = CloudStatus::kInvariantViolated;
          visited.clear();                 // ATOMIQUE : pas de prefixe partiel
          return visited;
        }
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

      // Admissibilite de cette orientation comme rayon de parent.
      for (int direction = -1; direction <= 1; direction += 2) {
        bool admissible = false;
        if (parents != nullptr) {
          admissible = true;
          for (int t = 0; t < m && admissible; ++t) {
            const i32 z = v.shell[(std::size_t)t];
            if (tangent_sign(pencil.orient_of(z), direction) < 0) admissible = false;
          }
          if (admissible) {
            if (potential_site >= 0) {
              if (tangent_sign(pencil.orient_of(potential_site), direction) <= 0)
                admissible = false;
            } else {
              // Niveau zero : Q_r doit STRICTEMENT decroitre. Sa derivee est la
              // somme des a_s . d sur la base independante du germe, soit
              // -2 fois la somme des orient3d, orientee. On somme donc les
              // VALEURS de orient3d, jamais leurs signes.
              mhgp::i128 total = 0;
              for (i32 z : root_potential_base)
                total += orient3d_exact(points[(std::size_t)base[0]],
                                        points[(std::size_t)base[1]],
                                        points[(std::size_t)base[2]],
                                        points[(std::size_t)z]);
              const mhgp::i128 derivative = (direction > 0) ? -total : total;
              if (derivative >= 0) admissible = false;
            }
          }
          if (admissible) {
            std::vector<i32> key = closure;
            key.push_back(direction > 0 ? 1 : 0);
            if (parent_key.empty() || key < parent_key) {
              parent_key = key;
              parent_orientation = direction;
              parent_closure = closure;
            }
          }
        }
        ++st->pencil_queries;
        i32 best = -1;
        int best_orient = 0;

        // Absorbe un candidat : garde le meilleur dans la direction demandee et
        // son lot d'ex aequo. `seen_candidate` evite de re-tester un point deja
        // rapporte par un balayage precedent.
        auto absorb = [&](i32 z) {
          if (seen_candidate[(std::size_t)z]) return;
          seen_candidate[(std::size_t)z] = 1;
          touched_ids.push_back(z);
          if (std::binary_search(v.shell.begin(), v.shell.end(), z)) return;
          const int oz = pencil.orient_of(z);
          if (oz == 0) return;              // constant le long du pinceau
          ++st->pencil_candidates;
          if (pencil.compare_t(z, oz, apex, orient_apex) != direction) return;
          if (best < 0) { best = z; best_orient = oz; return; }
          if (pencil.compare_t(z, oz, best, best_orient) == -direction) {
            best = z; best_orient = oz;
          }
        };

        if (index == nullptr) {
          for (i32 z = 0; z < n; ++z) absorb(z);
        } else {
          // AMORCE. Un pave entier autour d'un sommet du triangle, double
          // jusqu'a trouver un candidat ou jusqu'a couvrir toute la grille.
          // Aucune exactitude n'en depend : c'est le certificat par union des
          // deux vraies boules, plus bas, qui decide.
          // AMORCE. Les evenements « sortants » sont exactement les points de
          // B(v), qui est TRANSPORTE et donc deja connu : aucun balayage. Il
          // reste a chercher les « entrants », qui sont hors de la boule.
          for (i32 z : v.interior) absorb(z);
          mhgp::Sphere apex_sphere{};
          mhgp::i32 apex_support[4] = {base[0], base[1], base[2], apex};
          std::sort(apex_support, apex_support + 4);
          const bool apex_ok = mhgp::sphere4(points[(std::size_t)apex_support[0]],
                                             points[(std::size_t)apex_support[1]],
                                             points[(std::size_t)apex_support[2]],
                                             points[(std::size_t)apex_support[3]], &apex_sphere);

          // AMORCE PAR PAVE, dimensionne par le RAYON DE LA SPHERE COURANTE.
          //
          // Deux amorces ont ete essayees et mesurees avant celle-ci. Partir de
          // l'etendue du triangle coutait un ordre de grandeur sur les slivers
          // de surface. Sonder le pinceau par une sphere exacte de parametre
          // double etait geometriquement juste — la region reellement traversee
          // EST une lentille — mais pire en pratique : a grand pas la sonde tend
          // vers un demi-espace, et la lentille avec elle.
          //
          // Le bon ordre de grandeur est le rayon de la sphere du sommet : le
          // prochain evenement est a cette echelle des que le nuage est dense.
          // Le rayon n'est ici qu'une ESTIMATION de taille de recherche, jamais
          // une decision : le certificat reste la lentille exacte ci-dessous, et
          // le repli exhaustif ferme le cas ou rien n'est trouve.
          // Mesure : partir du rayon de la sphere courante divise par quatre le
          // nombre de tours, mais rapporte cinq fois plus de candidats, chacun
          // paye en predicats exacts — 22,9 s contre 13,7 s a n=800. Le pave
          // part donc PETIT et double.
          const mhgp::P3& anchor = points[(std::size_t)base[0]];
          long long half = 4;
          bool covered_grid = false;
          while (best < 0 && !covered_grid) {
            ++st->bootstrap_rounds;
            const long long lo[3] = {(long long)anchor.x - half, (long long)anchor.y - half,
                                     (long long)anchor.z - half};
            const long long hi[3] = {(long long)anchor.x + half, (long long)anchor.y + half,
                                     (long long)anchor.z + half};
            index->box(lo, hi, &st->grid_points_touched, absorb);
            covered_grid = (half >= (long long)kDeclaredGridMaximum);
            half *= 2;
          }
          if (covered_grid && best < 0) ++st->full_grid_sweeps;

          // CERTIFICATION PAR LE DESACCORD DE SIGNE. On ne balaie pas la boule
          // du sommet — celle d'un sliver est geometriquement enorme bien que
          // vide — mais l'ensemble des points dont `sphere_side` DIFFERE entre
          // les deux spheres terminales, qui est exactement l'ensemble des
          // evenements non constants entre les deux parametres. Elle est mince des que l'evenement est proche, et elle
          // retrecit a chaque tour puisque chaque nouveau meilleur candidat a un
          // parametre strictement compris entre les deux precedents.
          for (int round = 0; apex_ok && round < 8 && best >= 0; ++round) {
            const i32 previous = best;
            mhgp::i32 bs[4] = {base[0], base[1], base[2], best};
            std::sort(bs, bs + 4);
            mhgp::Sphere best_sphere{};
            if (!mhgp::sphere4(points[(std::size_t)bs[0]], points[(std::size_t)bs[1]],
                               points[(std::size_t)bs[2]], points[(std::size_t)bs[3]],
                               &best_sphere)) break;
            ++st->disagreement_sweeps;
            index->sign_disagreement(apex_sphere, best_sphere, &st->grid_points_touched,
                                        absorb);
            if (best == previous) break;
          }
        }

        // Le LOT est relu sur l'ensemble deja balaye : tout point de meme
        // parametre que `best` est sur la sphere de `best`, donc dans une boule
        // deja couverte.
        batch.clear();
        if (best >= 0) {
          for (i32 z : touched_ids) {
            if (std::binary_search(v.shell.begin(), v.shell.end(), z)) continue;
            const int oz = pencil.orient_of(z);
            if (oz == 0) continue;
            if (pencil.compare_t(z, oz, best, best_orient) == 0) batch.push_back(z);
          }
          std::sort(batch.begin(), batch.end());
        }
        for (i32 z : touched_ids) seen_candidate[(std::size_t)z] = 0;
        touched_ids.clear();

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
        // TRANSPORT DE L'ENSEMBLE, pas du seul cardinal. B_e = B(v) union les
        // anciens membres de coquille devenus interieurs ; puis on retire du lot
        // entrant ceux qui etaient interieurs sur l'arete ouverte.
        std::vector<i32> interior = v.interior;
        for (int t = 0; t < m; ++t) {
          const i32 z = v.shell[(std::size_t)t];
          if (std::binary_search(closure.begin(), closure.end(), z)) continue;
          if (pencil.side(best, z, best_orient) < 0) interior.push_back(z);
        }
        for (i32 z : batch)
          if (pencil.side(apex, z, orient_apex) < 0) {
            const auto it = std::find(interior.begin(), interior.end(), z);
            if (it != interior.end()) interior.erase(it);
          }
        std::sort(interior.begin(), interior.end());
        interior.erase(std::unique(interior.begin(), interior.end()), interior.end());
        const int level = (int)interior.size();

        if (level < 0) {
          *status = CloudStatus::kInvariantViolated;
          visited.clear();
          return visited;
        }
        if (parents != nullptr && admissible && direction == parent_orientation &&
            closure == parent_closure && !parent_key.empty())
          parents->back() = flats::ParentEdge{shell, closure, direction};
        if (level > level_ceiling) { ++st->vertices_over_level; continue; }
        if (seen.insert(shell).second) frontier.push_back(Vertex{shell, interior, level});
      }
    }
    if (!any_flat) ++st->degenerate_flat_vertex;
  }
  return visited;
}

// ---------------------------------------------------------------------------
// LE PARCOURS PAR REVERSE SEARCH
// ---------------------------------------------------------------------------
//
// Meme ensemble de sommets que le BFS, sans `seen`, `frontier` ni `visited`. On
// descend de v vers w si et seulement si pi(w) = v ; l'unicite du parent rend
// toute deduplication inutile. L'etat de navigation est la PILE : par niveau, la
// coquille du parent et l'indice du fils courant.
//
// Le BFS reste l'oracle : le differentiel exige que les deux parcours rendent
// exactement le meme ensemble, et publie ici la profondeur maximale, le nombre
// de voisins soumis au test de parent et le nombre de retours.
//
// Ce que cela ne rend PAS : une borne de temps. Chaque fils teste coute un calcul
// de parent, donc une enumeration de flats et une requete de voisin ; une grande
// coquille peut avoir un nombre combinatoire de flats. Ce que cela rend, c'est la
// memoire — et l'absence d'ecriture partagee.
//
// SORTIE BORNEE. Le parcours ne materialise plus ses sommets : il les REND un a un
// a un sink, et sa memoire vive est la PILE — par niveau, l'indice du fils et la
// coquille du parent — plus le scratch d'une requete. Ce n'etait pas le cas tant
// que l'endpoint rendait un `std::vector<Vertex>` : la sortie etait alors
// Omega(V), et aucun gain memoire n'etait demontre. Le high-water des slots vifs
// est PUBLIE, pas suppose.
//
// Le sink rend `false` pour interrompre. Une sortie interrompue n'est pas une
// sortie complete : le statut devient `kInvariantViolated` seulement si le sink
// n'a jamais demande l'arret, sinon l'appelant sait ce qu'il a fait.
template <class Sink>
inline void reverse_search_stream(const std::vector<mhgp::P3>& points,
                                  int level_ceiling, FlatStatistics* st, CloudStatus* status,
                                  Sink&& sink, const CertifiedIndex* index = nullptr) {
  using namespace flats;
  const int n = (int)points.size();
  *status = CloudStatus::kOk;
  if (n < 4) { *status = CloudStatus::kTooFewPoints; return; }
  if (!inside_declared_grid(points)) { *status = CloudStatus::kOutsideDeclaredGrid; return; }
  if (has_duplicate_coordinates(points)) {
    *status = CloudStatus::kDuplicateCoordinates;
    return;
  }
  if (!affine_dimension_is_three(points)) {
    *status = CloudStatus::kAffineDimensionBelowThree;
    return;
  }
  if (level_ceiling < 0) return;

  Vertex seed;
  const CloudStatus seeded = seed_level_zero(points, st, &seed);
  if (seeded != CloudStatus::kOk) { *status = seeded; return; }

  std::vector<i32> root_base;
  for (i32 z : seed.shell) {
    if (root_base.size() >= 4) break;
    const std::size_t have = root_base.size();
    if (have == 1) {
      const mhgp::P3& u = points[(std::size_t)root_base[0]];
      const mhgp::P3& w = points[(std::size_t)z];
      if (u.x == w.x && u.y == w.y && u.z == w.z) continue;
    } else if (have == 2) {
      const mhgp::P3 u = mhgp::p3_sub(points[(std::size_t)root_base[1]],
                                      points[(std::size_t)root_base[0]]);
      const mhgp::P3 w = mhgp::p3_sub(points[(std::size_t)z], points[(std::size_t)root_base[0]]);
      const mhgp::P3 c = mhgp::p3_cross(u, w);
      if (c.x == 0 && c.y == 0 && c.z == 0) continue;
    } else if (have == 3) {
      if (orient3d_exact(points[(std::size_t)root_base[0]], points[(std::size_t)root_base[1]],
                         points[(std::size_t)root_base[2]], points[(std::size_t)z]) == 0) continue;
    }
    root_base.push_back(z);
  }
  if (root_base.size() < 4) { *status = CloudStatus::kInvariantViolated; return; }

  // Aucune requete de parent dans le parcours. La decision de filiation est
  // `decide_child`, qui enumere les couples du candidat jusqu'a la clef de retour
  // et n'appelle jamais `neighbour_along`. `canonical_parent` et `neighbour_along`
  // restent publics pour le JUGE, qui rejoue la symetrie du pinceau contre eux —
  // oracle differentiel, jamais autorite partagee.
  //
  // Le germe ne peut pas etre atteint ici : n'ayant aucune direction admissible, son
  // couple de retour est refuse par le prefiltre en O(m), donc `decide_child` n'est
  // jamais appele sur lui. Toute autre issue `kBroken` est une contradiction — clef
  // de retour depassee, bases disjointes, ou retour inadmissible apres un prefiltre
  // positif — et remonte comme telle.
  // LE CURSEUR DE REPRISE, et non un compteur lineaire. Avec un compteur il fallait
  // re-enumerer — donc reconstruire les fermetures — de tous les triplets deja
  // consommes pour retrouver sa place. Le curseur est l'adresse du triplet et la
  // fente de direction, donc la reprise ne touche plus rien avant elle.
  struct Level { Vertex vertex; int i = 0, j = 1, k = 2, dir = 0; };
  std::vector<Level> stack;
  // SLOTS VIFS : la somme des tailles des sommets du chemin. C'est la borne de
  // memoire que le parcours revendique, donc celle qu'il doit publier.
  long long live = 0;
  auto push = [&](const Vertex& v) {
    live += (long long)(v.shell.size() + v.interior.size());
    st->reverse_live_high_water = std::max(st->reverse_live_high_water, live);
    stack.push_back(Level{v, 0, 1, 2, 0});
  };
  bool interrupted = false;
  // Le germe passe par le MEME contrat que les autres : un sink qui le refuse
  // interrompt le parcours, et l'interruption doit se voir dans le statut.
  if (!sink(seed)) { *status = CloudStatus::kSinkStopped; return; }
  push(seed);

  while (!stack.empty()) {
    st->reverse_depth_max = std::max(st->reverse_depth_max, (long long)stack.size());
    Level& top = stack.back();

    // Les voisins dans l'ordre deterministe (flat, direction). L'indice du fils
    // est donc reproductible au retour, ce qui est toute la mecanique
    // d'AVIS--FUKUDA.
    bool descended = false;
    bool broken = false;
    Vertex child;
    int next_i = top.i, next_j = top.j, next_k = top.k, next_dir = top.dir;
    for_each_flat_from(points, top.vertex, top.i, top.j, top.k,
                       [&](const FlatAtVertex& flat, int fi, int fj, int fk) {
      ++st->reverse_flats_enumerated;
      const bool resumed = (fi == top.i && fj == top.j && fk == top.k);
      const int first_slot = resumed ? top.dir : 0;
      if (first_slot >= 2) return true;              // triplet deja epuise
      for (int slot = first_slot; slot < 2; ++slot) {
        const int direction = slot == 0 ? -1 : 1;
        Vertex candidate;
        if (!neighbour_along(points, top.vertex, flat, direction, index, st, &candidate)) continue;
        if (candidate.level > level_ceiling) continue;
        ++st->reverse_children_tested;
        if (!backward_pair_admissible(points, candidate, flat.base, direction, root_base)) {
          ++st->reverse_reject_backward;
          continue;                          // pi(candidate) != v, certifie en O(m)
        }
        // Plus aucune requete de voisin de retour : l'adjacence du pinceau etant
        // symetrique, il suffit d'enumerer les couples du candidat JUSQU'A la clef
        // de retour. Le juge rejoue la symetrie contre `canonical_parent`.
        ++st->reverse_decisions;
        const ChildOutcome outcome =
            decide_child(points, candidate, flat, direction, root_base,
                         &st->reverse_triplets_scanned, &st->reverse_closures_built);
        if (outcome == ChildOutcome::kBroken) { broken = true; return false; }
        if (outcome == ChildOutcome::kReject) {
          ++st->reverse_reject_by_parent;
          continue;
        }
        next_i = fi; next_j = fj; next_k = fk; next_dir = slot + 1;
        child = candidate;
        descended = true;
        return false;                                     // ARRET : plus rien a enumerer
      }
      return true;
    }, &st->reverse_triplets_scanned, &st->reverse_closures_built, &st->closure_high_water);
    if (broken) { *status = CloudStatus::kInvariantViolated; return; }

    if (descended) {
      top.i = next_i; top.j = next_j; top.k = next_k; top.dir = next_dir;
      if (!sink(child)) { interrupted = true; break; }
      push(child);
      continue;
    }
    ++st->reverse_backtracks;
    live -= (long long)(stack.back().vertex.shell.size() + stack.back().vertex.interior.size());
    stack.pop_back();
  }
  if (interrupted) *status = CloudStatus::kSinkStopped;
}

// Enveloppe qui MATERIALISE la sortie. Elle reste le sujet du differentiel, parce
// que comparer deux ensembles demande de les tenir ; elle n'est pas le chemin que
// le produit doit prendre, et son cout memoire est celui qu'elle affiche.
inline std::vector<flats::Vertex> reverse_search_shallow(const std::vector<mhgp::P3>& points,
                                                        int level_ceiling,
                                                        FlatStatistics* st,
                                                        CloudStatus* status,
                                                        const CertifiedIndex* index = nullptr) {
  std::vector<flats::Vertex> visited;
  reverse_search_stream(points, level_ceiling, st, status,
                        [&](const flats::Vertex& v) { visited.push_back(v); return true; }, index);
  if (*status != CloudStatus::kOk) visited.clear();
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
                                      bool verify_census, bool use_index = false,
                                      bool use_owner = false) {
  *st = FlatStatistics{};
  mhgp::Catalogue catalogue;
  const int n = (int)points.size();
  *status = CloudStatus::kOk;

  // L'ORDRE se valide avant tout : `s_max - 2` est un calcul signe, et
  // `s_max = INT_MIN` le faisait deborder avant meme les singletons.
  //
  // LA BORNE EST `mhgp::kMaxRank`, PAS LA GRILLE. J'acceptais jusqu'a 65535 alors
  // que `mhgp.hpp` documente 32 comme « borne dure sur s_max supportee », et que
  // l'oracle, lui, la respecte deja. La contradiction n'etait pas theorique : elle
  // rend indefinie toute capacite derivee du contrat, a commencer par
  // |B(v)| <= s_max - 2, dont le noyau device a besoin pour dimensionner son
  // ensemble interieur. Sous cette borne, |B(v)| <= 30 est un THEOREME du contrat.
  if (s_max < 0 || s_max > mhgp::kMaxRank) {
    *status = CloudStatus::kOrderOutsideContract;
    return catalogue;
  }

  if (!inside_declared_grid(points)) {
    *status = CloudStatus::kOutsideDeclaredGrid;
    return catalogue;
  }
  if (has_duplicate_coordinates(points)) {
    *status = CloudStatus::kDuplicateCoordinates;
    return catalogue;
  }

  std::vector<mhgp::CriticalSphere> kept;
  std::vector<mhgp::i32> members_pool;
  // `emitted` est la derniere table globale en Theta(sortie). La composition
  // « support canonique PUIS proprietaire » la remplace exactement : le premier
  // filtre supprime les doublons d'un meme sommet — le cube u16 a six supports
  // minimaux pour une seule boule —, le second ceux entre sommets. Les deux
  // chemins coexistent tant que le differentiel n'a pas juge leur egalite.
  std::unordered_set<std::vector<mhgp::i32>, flats::ShellHash> emitted;
  // Le contexte ne balaye les points que si le proprietaire est demande.
  const flats::OwnerContext owner_ctx =
      use_owner ? flats::owner_context(points) : flats::OwnerContext{};

  CertifiedIndex grid;
  const bool indexed = use_index && n >= 1;
  if (indexed) grid.build(points, 16);

  std::vector<mhgp::i32> members, shell;
  auto try_emit_with = [&](const mhgp::MiniballResult& mb, const mhgp::i32* candidate, int cq,
                           const flats::Vertex* owner_vertex, bool from_shell) {
    ++st->emit_attempts;
    members.clear();
    shell.clear();
    if (indexed) {
      // CENSUS LOCAL. La boule fermee exacte remplace le balayage du nuage
      // entier : c'est le second $O(n)$ par tentative que l'audit compte, et il
      // valait a lui seul un terme en $nV$.
      grid.closed_ball(mb.sph, &st->grid_points_touched, [&](mhgp::i32 z) {
        members.push_back(z);
        if (mhgp::sphere_side(mb.sph, points[(std::size_t)z]) == 0) shell.push_back(z);
      });
      std::sort(members.begin(), members.end());
      std::sort(shell.begin(), shell.end());
    } else {
      for (mhgp::i32 z = 0; z < n; ++z) {
        const int side = mhgp::sphere_side(mb.sph, points[(std::size_t)z]);
        if (side > 0) continue;
        if (side == 0) shell.push_back(z);
        members.push_back(z);
      }
    }
    if ((int)members.size() > s_max) return;
    // LE PROPRIETAIRE NE COUVRE QUE LA RECOLTE NAVIGUEE. La voie directe
    // exhaustive et les singletons non indexes n'ont aucun sommet, donc aucune
    // notion de propriete : ils gardent `emitted`. Ma premiere version rejetait
    // tout candidat nul, ce qui les SUPPRIMAIT en silence — l'audit le montre sur
    // un tetraedre, dont les quatre singletons disparaissaient sans index, et sur
    // un triangle de dimension affine deux, dont tout le catalogue disparaissait.
    const bool owned_path = (use_owner && owner_vertex != nullptr);
    if (!owned_path) {
      const bool fresh = emitted.insert(shell).second;
      st->dedup_table_high_water =
          std::max(st->dedup_table_high_water, (long long)emitted.size());
      if (!fresh) { ++st->emit_duplicate_shell; return; }
    }

    // SUPPORT CANONIQUE.
    //
    // Une miniboule peut avoir PLUSIEURS supports minimaux, et les deux notions
    // different : le cube cospherique a QUATRE supports de cardinalite minimale —
    // ses quatre diagonales — mais SIX supports minimaux pour l'INCLUSION, les
    // quatre diagonales et les deux tetraedres de parite. La convention publique
    // porte sur la cardinalite minimale puis l'ordre des coordonnees. Lire le support
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

    if (owned_path) {
      // (3) REJET DE TOUT SUPPORT NON CANONIQUE. Sans lui, l'owner seul emettrait
      // une fois par support minimal : six fois sur le cube u16.
      //
      // L'ARITE QUATRE passe par une autre voie, et ma premiere version la perdait.
      // Elle est recoltee depuis la COQUILLE ENTIERE du sommet, donc le candidat a
      // |S(v)| points et non quatre : des qu'une coquille est cospherique, comparer
      // le candidat au support canonique rejetait la sphere partout, et une sphere
      // par nuage disparaissait. La note traite ce cas a part : on canonise la
      // coquille une fois, et on n'emprunte cette voie QUE si le support canonique a
      // bien arite quatre. La sphere est alors le sommet lui-meme.
      if (from_shell) {
        if (canonical_mb.n_support != 4) { ++st->owner_rejected_support; return; }
      } else {
        if (cq != canonical_mb.n_support) { ++st->owner_rejected_support; return; }
        for (int i = 0; i < cq; ++i) {
          bool found = false;
          for (int j = 0; j < canonical_mb.n_support; ++j)
            if (canonical_mb.support[j] == candidate[i]) found = true;
          if (!found) { ++st->owner_rejected_support; return; }
        }
      }
      // (4) LE SOMMET COURANT EST-IL o(U_can) ? B_U est deja calcule : c'est
      // l'interieur strict de la boule censee, membres moins coquille.
      std::vector<mhgp::i32> b_u;
      for (mhgp::i32 z : members)
        if (!std::binary_search(shell.begin(), shell.end(), z)) b_u.push_back(z);
      if (owner_vertex == nullptr ||
          !flats::is_owner(points, owner_ctx, *owner_vertex, canonical_mb.support,
                           canonical_mb.n_support, b_u)) {
        ++st->owner_rejected_vertex;
        return;
      }
      ++st->owner_emitted;
    }

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

  // Voie directe exhaustive : aucun sommet ne la porte, donc aucune notion de
  // propriete. Elle garde `emitted`, et la fixture de domaine la confronte
  // desormais au mode owner dans les quatre quadrants index x proprietaire.
  auto try_emit = [&](const mhgp::i32* candidate, int m) {
    const mhgp::MiniballResult mb = mhgp::miniball_of(points, candidate, m);
    if (!mb.ok) return;
    for (int i = 0; i < m; ++i)
      if (mhgp::sphere_side(mb.sph, points[(std::size_t)candidate[i]]) != 0) return;
    try_emit_with(mb, nullptr, 0, nullptr, false);
  };

  // SINGLETONS. Sous refus des doublons, la boule fermee de rayon nul centree en
  // p ne contient que p : rang un, coquille {p}. Les faire passer par un census
  // global coutait n^2, soit 2,5 milliards de classifications a 50 000 points
  // AVANT le germe. Le resultat est ici en temps constant, et le chemin de
  // reference garde le census pour que le differentiel puisse les confronter.
  if (indexed) {
    if (s_max >= 1) {
      for (mhgp::i32 p = 0; p < n; ++p) {
        const mhgp::Sphere sphere = mhgp::sphere1(points[(std::size_t)p]);
        std::vector<mhgp::i32> one{p};
        // Un singleton ne peut entrer en collision avec rien : la boule de rayon
        // nul centree en p a pour coquille {p}, et toute recolte part d'un support
        // d'au moins deux points, donc d'une coquille d'au moins deux points. Sous
        // proprietaire, la table n'a donc plus aucun role ici non plus.
        if (!use_owner) {
          const bool fresh = emitted.insert(one).second;
          st->dedup_table_high_water =
              std::max(st->dedup_table_high_water, (long long)emitted.size());
          if (!fresh) { ++st->emit_duplicate_shell; continue; }
        }
        ++st->emit_attempts;
        mhgp::CriticalSphere critical{};
        critical.support[0] = p;
        for (int i = 1; i < mhgp::kMaxSupport; ++i) critical.support[i] = -1;
        critical.n_support = 1;
        critical.rank = 1;
        critical.sph = sphere;
        critical.beta = mhgp::sphere_beta(sphere);
        critical.members_begin = (mhgp::i32)members_pool.size();
        members_pool.push_back(p);
        kept.push_back(critical);
        ++st->emitted_arity[1];
      }
    }
  } else {
    for (mhgp::i32 p = 0; p < n; ++p) try_emit(&p, 1);
  }

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
    const auto vertices = navigate_shallow(points, level_ceiling, st, &nav, verify_census,
                                           indexed ? &grid : nullptr);
    if (nav != CloudStatus::kOk) { *status = nav; return catalogue; }
    // ---------------------------------------------------------------------
    // RECOLTE, avec le TEST DE PROPRIETE local.
    //
    // Un sommet v ne peut posseder le support U que s'il appartient au polyedre
    // de signes P_U — sinon un AUTRE sommet le possede, et le theoreme de
    // proprietaire garantit que celui-la est de niveau au plus s_max - q, donc
    // visite. Or l'appartenance a P_U se lit presque entierement sur l'ensemble
    // INTERIEUR de v : un membre de la coquille de v satisfait L = 0 et ne
    // contraint rien, tandis que tout point strictement interieur a la sphere de
    // v doit etre strictement interieur a la boule de U.
    //
    // Le test coute donc |B(v)| comparaisons exactes par support candidat, et il
    // remplace un census complet. B(v) se lit une seule fois par sommet.
    for (const flats::Vertex& v : vertices) {
      const int m = (int)v.shell.size();
      const std::vector<mhgp::i32>& interior = v.interior;
      // La miniboule est calculee UNE fois et traverse le filtre : la recalculer
      // dans le filtre puis dans l'emission coutait plus cher que le census
      // qu'elle evitait.
      auto guarded_emit = [&](const mhgp::i32* candidate, int q, bool from_shell = false) {
        const mhgp::MiniballResult mb = mhgp::miniball_of(points, candidate, q);
        if (!mb.ok) return;
        for (int i = 0; i < q; ++i)
          if (mhgp::sphere_side(mb.sph, points[(std::size_t)candidate[i]]) != 0) return;
        if (indexed) {
          for (mhgp::i32 z : interior)
            if (mhgp::sphere_side(mb.sph, points[(std::size_t)z]) >= 0) {
              ++st->harvest_prefiltered;
              return;
            }
        }
        ++st->harvest_censused;
        try_emit_with(mb, candidate, q, &v, from_shell);
      };

      guarded_emit(v.shell.data(), m, true);
      for (int i = 0; i < m; ++i)
        for (int j = i + 1; j < m; ++j) {
          const mhgp::i32 e[2] = {v.shell[(std::size_t)i], v.shell[(std::size_t)j]};
          guarded_emit(e, 2);
          // Les 4-sous-ensembles sont REDONDANTS et ne sont pas recoltes. Si le
          // support canonique d'une sphere critique a quatre points, il est
          // affinement independant, sa sphere est le sommet lui-meme et sa
          // coquille est S(v) : la recolte de la coquille la publie. Si quatre
          // points de la coquille sont coplanaires, leur miniboule a un support
          // d'au plus trois points et la recolte d'arite trois la publie.
          for (int k = j + 1; k < m; ++k) {
            const mhgp::i32 f[3] = {e[0], e[1], v.shell[(std::size_t)k]};
            guarded_emit(f, 3);
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
