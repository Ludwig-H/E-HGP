// MorseHGP3D v6 — fixtures de FRONTIERE du sweep de corde q4 (audit du 31 aout).
//
// La passe 2 de process_anchor_q4 enumere les racines μ_z = P(z)/B(z) de la
// corde de chaque seed survivant : normalisation par le signe de B (den > 0),
// clip FERME 2P² <= J·B² (egalite = borne de Jung, admissible), tri exact
// cmp_chord_roots, regle de bloc (sorties retirees, incidents a zero, entrees
// apres). Chaque fixture realise une frontiere PRECISE de ce chemin sur un
// petit nuage entier GRAVE ; la verite est l'OBJET (ensemble des BallKey
// survivantes au prefiltre exact, arite minimale — la methode de l'oracle du
// selftest) PLUS des planchers de compteurs qui prouvent la non-vacuite de la
// frontiere visee, PLUS les quantites exactes (P, B, J) gravees en litteraux
// et re-derivees par les formes de production (q3_form, q3_power,
// cmp_2p2_jb2).
//
//   F1 — racines EGALES : sept points sur la sphere entiere |z−c|² = 50 ;
//        pour le seed (a,b,x), les quatre autres sites partagent EXACTEMENT
//        μ = −100 (cospheriques ⟹ meme racine) : un bloc de taille 4.
//   F2 — EXTREMITE de Jung : le tetraedre regulier ENTIER a+{(0,0,0),(4,4,0),
//        (4,0,4),(0,4,4)} (sommets alternes du cube, arete² = 32) realise
//        2P² = J·B² EXACTEMENT (P = 8192, J = 8192, B = −128) : la racine de
//        la completion est SUR l'extremite de la corde fermee, et J/2 = 4096
//        est un carre parfait (μ* = 64 entier). L'hypothese « un tetraedre
//        regulier entier n'existe pas » de la commande d'audit est refutee.
//   F3 — B = 0 : nuage entierement COPLANAIRE ; un site strictement interieur
//        (P < 0) au centre μ = 0 est temoin constant (sweep_const_interior),
//        jamais une completion ; aucune racine n'existe (onchord == 0).
//   F4 — la completion d vit dans le FACTEUR du rectangle WSPD qui contient
//        a (trou de completude n°1 de la contre-lecture) : l'ancre (a,b) est
//        possedee par un rectangle {b}×{a,d} et la cle (a,b,x,d) survit.
//   F5 — mutants : sweep-nonstrict-depth diverge sur un nuage grave a
//        profondeur 4 = h4−1 (les incidents comptes interieurs poussent la
//        profondeur a h4 : fausse mort, cle perdue) ; sweep-drop-exit-root
//        diverge sur F2 (la completion reguliere est une racine de SORTIE,
//        B = −128 < 0 : la retirer perd la cle — divergence d'OBJET, pas
//        seulement de multiensemble pre-RLE).
//
// Codes : 0 conforme ; 1 desaccord ; 2 refus (argument ou mutant inconnu) ;
// 3 mutant injecte non tue ; 4 mutant injecte tue.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "../src/pipeline/expand.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp6;

namespace {

int g_failures = 0;
void check(bool ok, const char* what) {
  if (!ok) {
    ++g_failures;
    std::fprintf(stderr, "ECHEC : %s\n", what);
  }
}

using ObjectMap = std::map<BallKey, u8>;

// ---- Cote v6 : generation -> tri -> RLE -> prefiltre exact (méthode du
// selftest --sweep-oracle, sequentielle).
ObjectMap object_v6(const CloudIndex& ix, u64 smax_eff, GenerateStats* gs) {
  GenerateOptions go;
  go.smax = smax_eff;
  go.threads = 1;
  std::vector<BallCandidate> cands;
  generate_candidates(ix, go, &cands, gs);
  sort_candidates(&cands, 1);
  deduplicate_candidates(&cands);
  std::vector<Survivor> surv;
  ExpandStats es;
  prefilter_balls(ix, cands, smax_eff, 1, &surv, &es);
  ObjectMap got;
  for (const Survivor& s : surv) got[cands[s.idx].key] = cands[s.idx].arity;
  return got;
}

// ---- Cote oracle : enumeration exhaustive des supports (paires, triangles
// strictement aigus, tetraedres strictement bien centres) + profondeur par
// BallKey::power, seuil h = smax − arite + 1. Copie de la methode du selftest.
ObjectMap object_oracle(const CloudIndex& ix, u64 smax_eff) {
  std::map<BallKey, u8> want_supports;
  const auto note = [&](const BallKey& k, u8 arity) {
    auto [it, fresh] = want_supports.try_emplace(k, arity);
    if (!fresh && arity < it->second) it->second = arity;
  };
  const size_t m = ix.upos.size();
  for (size_t i = 0; i < m; ++i)
    for (size_t j = i + 1; j < m; ++j) {
      const P3 &pa = ix.upos[i], &pb = ix.upos[j];
      if (p3_norm2(p3_sub(pb, pa)) == 0) continue;
      note(q2_ball_key(pa, pb), 2);
    }
  for (size_t i = 0; i < m; ++i)
    for (size_t j = i + 1; j < m; ++j)
      for (size_t k = j + 1; k < m; ++k) {
        const P3 *pa = &ix.upos[i], *pb = &ix.upos[j], *px = &ix.upos[k];
        i64 lab = p3_norm2(p3_sub(*pb, *pa)), lax = p3_norm2(p3_sub(*px, *pa)), lbx = p3_norm2(p3_sub(*px, *pb));
        if (lax >= lab && lax >= lbx) std::swap(pb, px);
        else if (lbx >= lab && lbx >= lax) std::swap(pa, px);
        lab = p3_norm2(p3_sub(*pb, *pa));
        const P3 v{2 * px->x - pa->x - pb->x, 2 * px->y - pa->y - pb->y, 2 * px->z - pa->z - pb->z};
        if (!(p3_norm2(v) > lab)) continue;  // rectangle ou obtus : support d'arite 2
        const Q3Form f3 = q3_form(*pa, *pb, *px);
        if (f3.g <= 0) continue;  // colineaires
        note(q3_ball_key(f3), 3);
      }
  for (size_t i = 0; i < m; ++i)
    for (size_t j = i + 1; j < m; ++j)
      for (size_t k = j + 1; k < m; ++k)
        for (size_t l = k + 1; l < m; ++l) {
          const Q4Form f4 = q4_form(ix.upos[i], ix.upos[j], ix.upos[k], ix.upos[l]);
          if (f4.det == 0) continue;
          if (!q4_center_strictly_inside(f4, ix.upos[i], ix.upos[j], ix.upos[k], ix.upos[l])) continue;
          note(ball_key_reduce(q4_ball_form(f4)), 4);
        }
  ObjectMap want;
  for (const auto& [k, arity] : want_supports) {
    u64 depth = 0;
    for (size_t z = 0; z < m; ++z)
      if (k.power(ix.upos[z]) < 0) ++depth;
    const u64 h = smax_eff >= arity ? smax_eff - arity + 1 : 0;
    if (depth < h) want[k] = arity;
  }
  return want;
}

// Compare l'objet v6 a l'oracle ; rend true si CONFORME.
bool object_matches(const ObjectMap& got, const ObjectMap& want, const char* name) {
  if (got == want) return true;
  std::fprintf(stderr, "%s : objet v6 (%zu boules) != oracle (%zu boules)\n", name, got.size(), want.size());
  for (const auto& [k, a] : want)
    if (!got.count(k)) {
      std::fprintf(stderr, "  MANQUANTE arite=%d (perte de completude)\n", (int)a);
      break;
    }
  for (const auto& [k, a] : got)
    if (!want.count(k) || want.at(k) != a) {
      std::fprintf(stderr, "  EXCEDENTAIRE/arite fausse arite=%d\n", (int)a);
      break;
    }
  return false;
}

i32 find_upos(const CloudIndex& ix, const P3& p) {
  for (size_t u = 0; u < ix.upos.size(); ++u)
    if (ix.upos[u] == p) return (i32)u;
  return -1;
}

// P(z) et B(z) du sweep pour un seed (a,b,x) : P = q3_power de la forme du
// seed (identite affine L = 4P du th. 10.4), B = n·(z−a), n = (b−a)×(x−a).
i128 sweep_p(const P3& a, const P3& b, const P3& x, const P3& z) { return q3_power(q3_form(a, b, x), z); }
i64 sweep_b(const P3& a, const P3& b, const P3& x, const P3& z) {
  return p3_dot(p3_cross(p3_sub(b, a), p3_sub(x, a)), p3_sub(z, a));
}
i128 sweep_j(const P3& a, const P3& b, const P3& x) {
  const i64 d2 = p3_norm2(p3_sub(b, a));
  const i64 l_ax = p3_norm2(p3_sub(x, a));
  const i64 l_bx = p3_norm2(p3_sub(x, b));
  return (i128)d2 * (3 * q3_form(a, b, x).g - 2 * (i128)l_ax * l_bx);
}

// ---- Nuages graves. build_cloud_index(vector<P3>) donne PointId = index :
// l'ordre des litteraux fixe les identites (l'ancre canonique et la regle
// exact-once du seed en dependent).

// F1 / racines egales — sept points sur la sphere |z − (50,50,50)|² = 50 plus
// un temoin lointain. Pour le seed (a,b,x) (ancre ab, D² = 170, arete maximale
// a egalite ab = ax departagee par EdgeKey(0,1) < (0,2)), les quatre sites
// d,e,f,g sont cospheriques avec a,b,x : leurs racines valent EXACTEMENT
// μ = −100 (P = −100·B, grave ci-dessous), un bloc de taille 4. Aucune paire
// diametrale sur la sphere (l'arite minimale de la boule commune reste > 2).
std::vector<P3> cloud_f1() {
  return {
      {57, 50, 51},     // a  (id 0) : centre + (7,0,1)
      {45, 55, 50},     // b  (id 1) : centre + (−5,5,0)
      {45, 45, 50},     // x  (id 2) : centre + (−5,−5,0), seed aigu (|2x−a−b|² = 370 > 170)
      {57, 50, 49},     // d  (id 3) : centre + (7,0,−1)  — P = 24000,  B = −240 (sortie)
      {51, 57, 50},     // e  (id 4) : centre + (1,7,0)   — P = 6000,   B = −60  (sortie)
      {53, 54, 55},     // f  (id 5) : centre + (3,4,5)   — P = −52000, B = 520  (entree)
      {50, 55, 55},     // g  (id 6) : centre + (0,5,5)   — P = −55000, B = 550  (entree)
      {200, 200, 200},  // temoin lointain (id 7) : hors de tout cover d'interet
  };
}

// F2 / extremite de Jung — tetraedre regulier entier (sommets alternes du cube
// de cote 4), offset (20,20,20) : aretes² toutes egales a 32, circonsphere de
// centre (22,22,22) et R² = 12 = (3/8)·32, la borne de Jung EXACTE. Pour le
// seed canonique (a,b,x) et la completion y : P = 8192, B = −128, J = 8192,
// 2P² = 134217728 = J·B². Racine num/den = −8192/128 = −64 = −μ*, μ* = 64
// entier (J/2 = 4096 = 64²). Quatre temoins hors sphere completent n = 8.
std::vector<P3> cloud_f2() {
  return {
      {20, 20, 20},  // a (id 0)
      {24, 24, 20},  // b (id 1)
      {24, 20, 24},  // x (id 2) : seed canonique (plus petit PointId aigu)
      {20, 24, 24},  // y (id 3) : completion, racine de SORTIE a l'extremite
      {40, 20, 20},  // temoins hors circonsphere (id 4..7)
      {20, 40, 20},
      {40, 40, 40},
      {10, 10, 10},
  };
}

// F3 / B = 0 — nuage entierement coplanaire (plan z = 10). Pour le seed
// (a,b,x) (D² = 100 > 61, 61 ; |2x−a−b|² = 144 > 100 aigu), le site z1 est
// coplanaire (B = 0) et strictement interieur au centre μ = 0 : P = −93000.
// Il compte comme temoin constant et n'est jamais une completion (det = 0
// pour tout tetraedre coplanaire — aucune cle q4 dans l'objet). Les quatre
// autres sites du plan sont exterieurs a la circonboule de abx (P > 0).
std::vector<P3> cloud_f3() {
  return {
      {10, 10, 10},  // a  (id 0)
      {20, 10, 10},  // b  (id 1)
      {15, 16, 10},  // x  (id 2)
      {15, 11, 10},  // z1 (id 3) : B = 0, P = −93000 < 0 — temoin constant
      {10, 17, 10},  // exterieurs coplanaires (id 4..7)
      {20, 17, 10},
      {13, 3, 10},
      {17, 3, 10},
  };
}

// F4 / completion dans le facteur du rectangle — meme quadruple spherique que
// F1 (a,b,x,d sur la sphere R² = 50, ancre ab intra-amas, D² = 170, aretes
// maximales a egalite ab = ax = bd = xd departagees par EdgeKey(0,1)), plus un
// second amas a ~2000 (>> 8× le diametre) pour que la WSPD soit non triviale.
// a et d ne different que de z ± 1 : l'arbre radix les groupe, et le rectangle
// terminal qui possede la paire (a,b) est {b}×{a,d} — la completion d vit
// dans le FACTEUR qui contient a. La cle (a,b,x,d) doit survivre dans l'objet
// (trou de completude n°1 : le sweep tire ses completions du cover, jamais
// d'une structure C×D qui exclurait les facteurs).
std::vector<P3> cloud_f4() {
  return {
      {107, 100, 101},   // a (id 0)
      {95, 105, 100},    // b (id 1)
      {95, 95, 100},     // x (id 2) : seed canonique
      {107, 100, 99},    // d (id 3) : completion, racine de sortie (B = −240)
      {2100, 100, 100},  // amas lointain (id 4..7)
      {2104, 100, 100},
      {2100, 104, 100},
      {2100, 100, 104},
  };
}

// F5 / profondeur au seuil — le tetraedre regulier de F2 plus QUATRE points
// strictement interieurs a sa circonsphere (R² = 12 autour de (22,22,22)).
// n = 8 ⟹ smax = 8, h4 = 5 : la profondeur de la boule du tetraedre vaut
// EXACTEMENT 4 = h4 − 1 (les quatre interieurs sont des temoins constants c0,
// P < 0 et racine strictement hors corde). Nominal : emise. Sous
// sweep-nonstrict-depth le bloc incident (taille 1) est compte interieur :
// profondeur 5 >= h4, fausse mort, cle perdue — divergence d'objet.
std::vector<P3> cloud_f5() {
  return {
      {20, 20, 20},  // a (id 0)
      {24, 24, 20},  // b (id 1)
      {24, 20, 24},  // x (id 2)
      {20, 24, 24},  // y (id 3)
      {22, 22, 22},  // interieurs stricts (id 4..7) : d² au centre = 0, 1, 1, 1
      {23, 22, 22},
      {22, 23, 22},
      {22, 22, 23},
  };
}

// ---- F1 : racines egales (bloc cospherique de taille 4).
int fixture_f1() {
  const int before = g_failures;
  const std::vector<P3> pts = cloud_f1();
  const CloudIndex ix = build_cloud_index(pts);
  check(ix.valid && !ix.has_duplicate_positions(), "F1 : entree valide");
  const P3 a = pts[0], b = pts[1], x = pts[2];
  const P3 cospheriques[4] = {pts[3], pts[4], pts[5], pts[6]};
  // Geometrie gravee : sphere commune, seed aigu canonique, racines μ = −100.
  const P3 centre{50, 50, 50};
  for (const P3& p : {a, b, x, pts[3], pts[4], pts[5], pts[6]})
    check(p3_norm2(p3_sub(p, centre)) == 50, "F1 : point sur la sphere R² = 50");
  check(is_acute_seed(a, b, x, 170, 0, 1, 2), "F1 : x est le seed aigu canonique de (a,b)");
  const i128 J = sweep_j(a, b, x);
  check(J == 1615000, "F1 : J = 1615000 grave");
  const i128 p_ref[4] = {24000, 6000, -52000, -55000};
  const i64 b_ref[4] = {-240, -60, 520, 550};
  for (int i = 0; i < 4; ++i) {
    const i128 P = sweep_p(a, b, x, cospheriques[i]);
    const i64 B = sweep_b(a, b, x, cospheriques[i]);
    check(P == p_ref[i] && B == b_ref[i], "F1 : (P, B) graves du site cospherique");
    check(B != 0 && P == (i128)-100 * B, "F1 : racine EXACTEMENT μ = −100 (P = −100·B)");
    check(cmp_2p2_jb2(P > 0 ? -P : P, J, B) < 0, "F1 : racine strictement sur la corde (2P² < J·B²)");
  }
  const u64 smax_eff = std::min<u64>(11, pts.size());
  GenerateStats gs;
  const ObjectMap got = object_v6(ix, smax_eff, &gs);
  const ObjectMap want = object_oracle(ix, smax_eff);
  check(object_matches(got, want, "F1"), "F1 : objet == oracle exhaustif");
  // Planchers : le bloc de taille 4 existe ⟹ strictement moins de blocs que
  // de racines (non-vacuite de la regle de bloc sur racines egales).
  check(gs.sweep_pass2_seeds >= 1, "F1 : au moins un seed en passe 2");
  check(gs.sweep_roots_onchord >= 4, "F1 : au moins quatre racines sur corde");
  check(gs.sweep_root_groups < gs.sweep_roots_onchord, "F1 : un bloc de racines egales de taille >= 2");
  return g_failures - before;
}

// ---- F2 : extremite de Jung (egalite exacte du clip 2P² = J·B²).
int fixture_f2() {
  const int before = g_failures;
  const std::vector<P3> pts = cloud_f2();
  const CloudIndex ix = build_cloud_index(pts);
  check(ix.valid && !ix.has_duplicate_positions(), "F2 : entree valide");
  const P3 a = pts[0], b = pts[1], x = pts[2], y = pts[3];
  // Tetraedre regulier entier : les six aretes carrees valent 32.
  const P3* v[4] = {&a, &b, &x, &y};
  for (int i = 0; i < 4; ++i)
    for (int j = i + 1; j < 4; ++j)
      check(p3_norm2(p3_sub(*v[j], *v[i])) == 32, "F2 : tetraedre regulier (arete² = 32)");
  check(is_acute_seed(a, b, x, 32, 0, 1, 2), "F2 : x est le seed aigu canonique de (a,b)");
  const i128 P = sweep_p(a, b, x, y);
  const i64 B = sweep_b(a, b, x, y);
  const i128 J = sweep_j(a, b, x);
  check(P == 8192 && B == -128 && J == 8192, "F2 : (P, B, J) = (8192, −128, 8192) graves");
  check(B < 0, "F2 : la completion est une racine de SORTIE (B < 0)");
  check(cmp_2p2_jb2(P > 0 ? -P : P, J, B) == 0, "F2 : EGALITE exacte 2P² = J·B² (extremite de la corde fermee)");
  check(isqrt128_floor(J / 2) * isqrt128_floor(J / 2) == J / 2, "F2 : J/2 = 4096 carre parfait (μ* = 64 entier)");
  const u64 smax_eff = std::min<u64>(11, pts.size());
  GenerateStats gs;
  const ObjectMap got = object_v6(ix, smax_eff, &gs);
  const ObjectMap want = object_oracle(ix, smax_eff);
  check(object_matches(got, want, "F2"), "F2 : objet == oracle exhaustif");
  const BallKey key = ball_key_reduce(q4_ball_form(q4_form(a, b, x, y)));
  const auto it = got.find(key);
  check(it != got.end() && it->second == 4, "F2 : la cle du tetraedre regulier survit (racine a l'extremite admise)");
  check(gs.sweep_pass2_seeds >= 1, "F2 : au moins un seed en passe 2");
  check(gs.sweep_roots_onchord >= 1, "F2 : la racine d'extremite est sur la corde fermee");
  return g_failures - before;
}

// ---- F3 : B = 0 (site coplanaire, temoin constant, jamais une completion).
int fixture_f3() {
  const int before = g_failures;
  const std::vector<P3> pts = cloud_f3();
  const CloudIndex ix = build_cloud_index(pts);
  check(ix.valid && !ix.has_duplicate_positions(), "F3 : entree valide");
  const P3 a = pts[0], b = pts[1], x = pts[2], z1 = pts[3];
  check(is_acute_seed(a, b, x, 100, 0, 1, 2), "F3 : x est le seed aigu canonique de (a,b)");
  check(sweep_b(a, b, x, z1) == 0, "F3 : z1 coplanaire au seed (B = 0)");
  check(sweep_p(a, b, x, z1) == -93000, "F3 : z1 strictement interieur au centre (P = −93000)");
  const u64 smax_eff = std::min<u64>(11, pts.size());
  GenerateStats gs;
  const ObjectMap got = object_v6(ix, smax_eff, &gs);
  const ObjectMap want = object_oracle(ix, smax_eff);
  check(object_matches(got, want, "F3"), "F3 : objet == oracle exhaustif");
  for (const auto& kv : got)
    check(kv.second <= 3, "F3 : aucune cle d'arite 4 (coplanaire : jamais une completion)");
  check(gs.sweep_pass2_seeds >= 1, "F3 : au moins un seed en passe 2");
  check(gs.sweep_const_interior >= 1, "F3 : temoin constant interieur compte (branche B = 0, P < 0)");
  check(gs.sweep_roots_onchord == 0 && gs.sweep_roots_offchord == 0 && gs.sweep_root_groups == 0,
        "F3 : aucune racine (tous les sites sont coplanaires)");
  return g_failures - before;
}

// ---- F4 : la completion d vit dans le facteur du rectangle WSPD de l'ancre.
int fixture_f4() {
  const int before = g_failures;
  const std::vector<P3> pts = cloud_f4();
  const CloudIndex ix = build_cloud_index(pts);
  check(ix.valid && !ix.has_duplicate_positions(), "F4 : entree valide");
  const P3 a = pts[0], b = pts[1], x = pts[2], d = pts[3];
  check(is_acute_seed(a, b, x, 170, 0, 1, 2), "F4 : x est le seed aigu canonique de (a,b)");
  const i128 P = sweep_p(a, b, x, d);
  const i64 B = sweep_b(a, b, x, d);
  const i128 J = sweep_j(a, b, x);
  check(P == 24000 && B == -240 && J == 1615000, "F4 : (P, B, J) = (24000, −240, 1615000) graves");
  check(cmp_2p2_jb2(P > 0 ? -P : P, J, B) < 0, "F4 : racine strictement sur la corde");
  const u64 smax_eff = std::min<u64>(11, pts.size());
  // Localisation du rectangle terminal vivant qui possede la paire (a,b).
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax_eff), lane_h(Lane::kQ3, smax_eff), lane_h(Lane::kQ4, smax_eff)};
  std::vector<MultiAliveRect> alive;
  GenerateStats wst;
  alive_rectangles_fused(ix, 8, h_of, 0b111, 1, &alive, &wst);
  const i32 ua = find_upos(ix, a), ub = find_upos(ix, b), ud = find_upos(ix, d);
  check(ua >= 0 && ub >= 0 && ud >= 0, "F4 : positions uniques localisees");
  const auto in_node = [&](NodeRef v, i32 u) {
    const NodeRange r = ix.range_of(v);
    return u >= r.first && u <= r.last;
  };
  int found = -1;
  bool a_cote_a = false;
  for (size_t i = 0; i < alive.size(); ++i) {
    const MultiAliveRect& r = alive[i];
    if (!(r.mask & 0b100)) continue;  // lane q4 encore ouverte
    const bool o1 = in_node(r.r.a, ua) && in_node(r.r.b, ub);
    const bool o2 = in_node(r.r.a, ub) && in_node(r.r.b, ua);
    if (o1 || o2) {
      found = (int)i;
      a_cote_a = o1;
      break;
    }
  }
  check(found >= 0, "F4 : un rectangle vivant (lane q4) possede la paire (a,b)");
  if (found >= 0) {
    // Le facteur qui contient a doit contenir AUSSI la completion d (a et d
    // ne different que de z ± 1 : l'arbre radix les groupe en sous-arbre).
    const NodeRef fac_a = a_cote_a ? alive[(size_t)found].r.a : alive[(size_t)found].r.b;
    const NodeRange ra = ix.range_of(fac_a);
    check(in_node(fac_a, ud), "F4 : la completion d vit dans le facteur du rectangle qui contient a");
    check(ra.last - ra.first + 1 >= 2, "F4 : ce facteur n'est pas un singleton");
  }
  GenerateStats gs;
  const ObjectMap got = object_v6(ix, smax_eff, &gs);
  const ObjectMap want = object_oracle(ix, smax_eff);
  check(object_matches(got, want, "F4"), "F4 : objet == oracle exhaustif");
  const BallKey key = ball_key_reduce(q4_ball_form(q4_form(a, b, x, d)));
  const auto it = got.find(key);
  check(it != got.end() && it->second == 4, "F4 : la cle (a,b,x,d) survit — le facteur du rectangle n'est pas exclu");
  check(gs.sweep_pass2_seeds >= 1, "F4 : au moins un seed en passe 2");
  return g_failures - before;
}

// ---- F5 nominal : profondeur gravee 4 = h4 − 1 (le nuage discriminant du
// mutant sweep-nonstrict-depth doit etre conforme SANS mutant).
int fixture_f5_nominal() {
  const int before = g_failures;
  const std::vector<P3> pts = cloud_f5();
  const CloudIndex ix = build_cloud_index(pts);
  check(ix.valid && !ix.has_duplicate_positions(), "F5 : entree valide");
  const P3 a = pts[0], b = pts[1], x = pts[2], y = pts[3];
  const u64 smax_eff = std::min<u64>(11, pts.size());
  const u64 h4 = lane_h(Lane::kQ4, smax_eff);
  check(h4 == 5, "F5 : h4 = 5 (n = 8, smax = 8)");
  const i128 J = sweep_j(a, b, x);
  // Les quatre interieurs sont des temoins CONSTANTS (P < 0, racine
  // strictement hors corde) : la profondeur au point de racine de y vaut 4.
  for (int i = 4; i < 8; ++i) {
    const i128 P = sweep_p(a, b, x, pts[(size_t)i]);
    const i64 B = sweep_b(a, b, x, pts[(size_t)i]);
    check(P < 0, "F5 : site interieur strict (P < 0)");
    check(B != 0 && cmp_2p2_jb2(P, J, B) > 0, "F5 : racine strictement hors corde (temoin constant c0)");
  }
  const BallKey key = ball_key_reduce(q4_ball_form(q4_form(a, b, x, y)));
  u64 depth = 0;
  for (const P3& p : ix.upos)
    if (key.power(p) < 0) ++depth;
  check(depth == h4 - 1, "F5 : profondeur de la boule EXACTEMENT h4 − 1 = 4 (frontiere du seuil)");
  GenerateStats gs;
  const ObjectMap got = object_v6(ix, smax_eff, &gs);
  const ObjectMap want = object_oracle(ix, smax_eff);
  check(object_matches(got, want, "F5"), "F5 : objet == oracle exhaustif (nominal)");
  const auto it = got.find(key);
  check(it != got.end() && it->second == 4, "F5 : la cle a profondeur h4 − 1 survit en nominal");
  check(gs.sweep_const_interior >= 4, "F5 : les quatre temoins constants sont comptes");
  check(gs.sweep_roots_onchord >= 1, "F5 : la racine de la completion est sur la corde");
  return g_failures - before;
}

// ---- Divergence d'objet d'un nuage sous le mutant actif (le registre des
// mutants est global : object_v6 subit le mutant, l'oracle jamais — les
// mutants sweep-* n'ont de point d'injection que dans generate.hpp).
bool cloud_diverges(const std::vector<P3>& pts) {
  const CloudIndex ix = build_cloud_index(pts);
  if (!ix.valid || ix.has_duplicate_positions()) return false;
  const u64 smax_eff = std::min<u64>(11, pts.size());
  GenerateStats gs;
  const ObjectMap got = object_v6(ix, smax_eff, &gs);
  const ObjectMap want = object_oracle(ix, smax_eff);
  return got != want;
}

}  // namespace

int main(int argc, char** argv) {
  const char* inject = nullptr;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--inject=", 0) == 0 && !inject) inject = argv[i] + 9;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      return 2;
    }
  }
  if (inject && !mutants_enable(inject)) {
    std::fprintf(stderr, "mutant inconnu : %s\n", inject);
    return 2;
  }
  if (!inject) {
    fixture_f1();
    fixture_f2();
    fixture_f3();
    fixture_f4();
    fixture_f5_nominal();
    if (g_failures == 0) std::printf("sweep_fixtures : F1-F5 conformes\n");
    return g_failures ? 1 : 0;
  }
  const std::string m = inject;
  if (m == "sweep-nonstrict-depth") {
    // La cle a profondeur h4 − 1 de F5 meurt d'un bloc incident compte
    // interieur : divergence d'objet attendue.
    return cloud_diverges(cloud_f5()) ? 4 : 3;
  }
  if (m == "sweep-drop-exit-root") {
    // La completion du tetraedre regulier de F2 est une racine de SORTIE
    // (B = −128 < 0) et son unique chemin d'emission : la retirer perd la
    // cle — divergence d'OBJET, pas seulement de multiensemble pre-RLE.
    return cloud_diverges(cloud_f2()) ? 4 : 3;
  }
  // Autre mutant connu : porte non specialisee, on joue les cinq nuages.
  const bool diverged = cloud_diverges(cloud_f1()) || cloud_diverges(cloud_f2()) || cloud_diverges(cloud_f3()) ||
                        cloud_diverges(cloud_f4()) || cloud_diverges(cloud_f5());
  return diverged ? 4 : 3;
}
