// MorseHGP3D v5 — ORACLE INDEPENDANT DE LA LANE q4 (src/lanes/q4.hpp).
//
// Pour TOUS les tetraedres {p<q<r<s} de petits nuages (familles a n ~ 50,
// grille serree 14³, nuages graves aux coordonnees exactes), l'oracle
// recalcule en arithmetique OBig<12> (oracle/obig.hpp, limbes 32 bits,
// jamais l'arithmetique de production) avec des primitives LOCALES
// (o_len2 / ov_dot / ov_cross / det3 — jamais p3_dot, p3_cross, q4_power ni
// BallKey::power dans le chemin de decision) :
//
//   (1) CIRCUMCENTRE par Cramer DIRECT sur les trois plans mediateurs :
//       |c-b|² = |c-a|²  <=>  2(b-a)·c = |b|² - |a|², idem pour x et y ;
//       M_o c = rhs, M_o = 2·[b-a ; x-a ; y-a], rhs_i = |p_i|² - |a|² ;
//       det_o = det M_o (= 8·(b-a)·((x-a)×(y-a))), num_j = det de M_o dont la
//       colonne j est remplacee par rhs (Cramer), c = num/det_o. Orientation
//       canonique det_o > 0 (negation simultanee de det_o et num : le centre
//       est inchange). Le SUJET (q4_form) forme le systeme RELATIF
//       M(c-a) = r, r_i = |p_i-a|² ; comme rhs_i = r_i + 2(p_i-a)·a = r_i +
//       (M a)_i, on a num = det·a + N' : l'oracle exige det_s = det_o et
//       N'_i = num_i - det_o·a_i EXACTEMENT (pas seulement le meme centre).
//   (2) BIEN-CENTRAGE STRICT par les QUATRE PUISSANCES EQUATORIALES en
//       seules longueurs carrees. Re-derivation. Face (p,q,r), sommet oppose
//       s ; u = q-p, v = r-p, w = s-p ; A = |u|², B = |v|², C = u·v, D = u·w,
//       E = v·w, F = |w|². Le centre circonscrit de la face dans son plan est
//       o_F = p + αu + βv avec 2(o_F-p)·u = A, 2(o_F-p)·v = B, soit
//       [A C ; C B][α ; β] = [A/2 ; B/2], Δ = AB - C² > 0 (face non degeneree),
//       α = B(A-C)/(2Δ), β = A(B-C)/(2Δ). Puissance de s par rapport a la
//       boule equatoriale (grand cercle = cercle circonscrit de la face) :
//       Pow_F(s) = |s-o_F|² - R_F² = |w-(o_F-p)|² - |o_F-p|² = F - 2w·(o_F-p)
//                = F - (B(A-C)D + A(B-C)E)/Δ.
//       Signe = signe de ΔF - B(A-C)D - A(B-C)E. Loi des cosinus :
//       C2 = 2C = A + B - l_qr, D2 = 2D = A + F - l_qs, E2 = 2E = B + F - l_rs,
//       et 4·(ΔF - B(A-C)D - A(B-C)E) = (4AB - C2²)F - B(2A - C2)D2 - A(2B - C2)E2.
//       Lemme equatorial (MATHEMATIQUES § 6.6) : le centre du tetraedre est
//       o = o_F + t n, s = s_0 + h n (h ≠ 0), |o-s|² = |o-p|² = R_F² + t² donne
//       Pow_F(s) = 2th ; « o et s du meme cote » <=> th > 0 <=> Pow_F(s) > 0.
//       Bien centre strict <=> les quatre puissances > 0 ; une puissance NULLE
//       = centre dans le plan d'une face = FRONTIERE, refusee par le contrat
//       strict. L'oracle calcule AUSSI les quatre orientations de Cramer
//       (sign det3(q-p, r-p, num - det·p) contre sign det3(q-p, r-p, s-p)) et
//       exige l'egalite des deux caracterisations (theoreme) ; le sujet
//       (q4_center_strictly_inside : UN volume, parite (-V,+V,-V,+V)) doit
//       coincider avec elles — mutant q4-center-parity.
//   (3) OWNER 6 ARETES : longueur carree maximale, ex aequo par plus petite
//       EdgeKey (sur les PointId, pas les index) ; tetra_owned_by doit rendre
//       true pour cette arete et false pour les cinq autres.
//   (4) INTERIEUR / COQUILLE de chaque autre point z : |z·det - num|² contre
//       |a·det - num|² (= det²·|z-c|² contre det²·R²) ; le signe de
//       q4_power(z) et celui de BallKey::power(z) du sujet doivent suivre.
//   (5) NIVEAU R² = |N'|²/det² du sujet (q4_level_raw, U192/i128 non reduits)
//       contre l'oracle : num_s · det_o² == |a·det_o - num_o|² · den_s
//       (produits croises < 2^260 < 2^384). BallKey du sujet PROJECTIVEMENT :
//       A_o = det_o², B_o = -2·det_o·num_o, C_o = |num_o|² - |a·det_o - num_o|².
//   (6) PREFILTRES NECESSAIRES du bien-centrage : q4_i64_prefilter ne rejette
//       JAMAIS un tetraedre bien centre (les deux affectations (x,y) et (y,x)
//       des sommets non-owner) ; q4_face_power_prefilter(face, s) == (Pow > 0)
//       pour les quatre faces de tout tetraedre non degenere, et le signe de
//       q3_power(face, s) est celui de Pow_F(s) (q3_power = Δ·Pow_F, Δ > 0) —
//       la frontiere Pow = 0 est REJETEE. Mutants q4-eq-sign (signe inverse :
//       faux rejets), q4-eq-nonstrict (>= 0 : frontiere manquee — il faut un
//       nuage ou Pow = 0 apparait : cosphere et grille 14³), q4-i64-drop-factor
//       et q4-i64-pair-min (gardes trop agressives : faux rejets).
//
// FIXTURES GRAVEES (coordonnees exactes) :
//   - tetraedre aux SIX ARETES EGALES (0,0,0),(M,M,0),(M,0,M),(0,M,M),
//     M = 65535, ids PERMUTES {7,3,5,1} : owner EdgeKey(1,3) par depart
//     EdgeKey ; R² = 3M²/4 ;
//   - « bien centre » et « faces aigues » se refutent mutuellement
//     (tests/fixtures/regressions/tetrahedron_face_filter_counterexamples.json,
//     translates de +10 pour le profil u16) : (10,10,10),(8,8,9),(8,11,10),
//     (10,9,9) bien centre avec DEUX faces obtuses, R² = 11/4 ;
//     (10,10,10),(8,8,9),(8,10,12),(8,12,9) quatre faces aigues, barycentrique
//     -1/12, NON bien centre (miniboule d'arite 3, R² = 169/36) ;
//   - corner8 (v3) : regulier unite (R² = 3/4), plat de poids 1/4 (bien
//     centre), coquille (0,0,0),(4,0,0),(2,2,0),(2,0,2) (ab diametre :
//     puissances NULLES, frontiere refusee), tres obtus (centre hors) ;
//   - tetraedre de la fixture 13 points (100,300,300),(300,300,300),
//     (200,160,400),(200,160,200) : R² = 14900, owner (0,1) par depart
//     (ab et xy toutes deux de longueur 40000) ;
//   - arrondi plancher du rayon : a = (0,0,0), b = (8,0,0), z = (4,1,2) :
//     H = 11, Ξ = 320, 2H² = 242 < Ξ < 363 = 3H² : z ∈ W_3 ∖ W_4 exactement ;
//     in_spindle et la boule-cœur ponctuelle q4 doivent l'exclure (un
//     plafond du rayon (κ_4 D)² = 4,29 → 5 l'inclurait : |z-m|² = 5).
// Planchers (--min-*) : tetraedres bien centres juges, frontieres Pow = 0,
// supports a coquille, rejets i64 reels, « aigus non centres » et « obtus
// centres ». Codes : 0 accord ; 1 desaccord du juge ; 2 refus avant calcul ;
// 3 plancher / invariant / debordement OBig (fail-closed) ; 4 mutant tue.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "../oracle/obig.hpp"
#include "../src/cloud/families.hpp"
#include "../src/core/mutants.hpp"
#include "../src/lanes/q4.hpp"
#include "../src/spindle/spindle.hpp"

using namespace mhgp5;

namespace {

using OB = mhgp5_oracle::OBig384;

OB ob(i64 v) { return OB::from_i64(v); }
OB ob128(i128 v) { return OB::from_i128((mhgp5_oracle::oi128)v); }

// ---- primitives LOCALES de l'oracle -------------------------------------------

// Longueur carree (i64 exact : < 3·2^32) et norme carree absolue.
i64 o_len2(const P3& p, const P3& q) {
  const i64 dx = p.x - q.x, dy = p.y - q.y, dz = p.z - q.z;
  return dx * dx + dy * dy + dz * dz;
}
i64 o_abs2(const P3& p) { return p.x * p.x + p.y * p.y + p.z * p.z; }

struct OV3 {
  OB c[3];
};
OV3 ov_diff(const P3& p, const P3& q) { return OV3{{ob(p.x - q.x), ob(p.y - q.y), ob(p.z - q.z)}}; }
OV3 ov_scale(const OB& s, const OV3& v) { return OV3{{s * v.c[0], s * v.c[1], s * v.c[2]}}; }
OB ov_dot(const OV3& a, const OV3& b) { return a.c[0] * b.c[0] + a.c[1] * b.c[1] + a.c[2] * b.c[2]; }
OV3 ov_cross(const OV3& a, const OV3& b) {
  return OV3{{a.c[1] * b.c[2] - a.c[2] * b.c[1], a.c[2] * b.c[0] - a.c[0] * b.c[2],
              a.c[0] * b.c[1] - a.c[1] * b.c[0]}};
}
OB det3(const OV3& r0, const OV3& r1, const OV3& r2) { return ov_dot(r0, ov_cross(r1, r2)); }

int sign128(i128 v) { return v < 0 ? -1 : (v > 0 ? 1 : 0); }

// ---- (1) Cramer direct sur les plans mediateurs ---------------------------------

struct OracleBall {
  OB det;   // > 0 apres orientation ; 0 = coplanaire
  OV3 num;  // c = num / det
};

OracleBall oracle_circumball(const P3& a, const P3& b, const P3& x, const P3& y) {
  const OB two = ob(2);
  const OV3 m[3] = {ov_scale(two, ov_diff(b, a)), ov_scale(two, ov_diff(x, a)), ov_scale(two, ov_diff(y, a))};
  const OB rhs[3] = {ob(o_abs2(b) - o_abs2(a)), ob(o_abs2(x) - o_abs2(a)), ob(o_abs2(y) - o_abs2(a))};
  OracleBall r;
  r.det = det3(m[0], m[1], m[2]);
  for (int j = 0; j < 3; ++j) {
    OV3 rows[3];
    for (int i = 0; i < 3; ++i) {
      rows[i] = m[i];
      rows[i].c[j] = rhs[i];
    }
    r.num.c[j] = det3(rows[0], rows[1], rows[2]);
  }
  if (r.det.sign() < 0) {
    r.det = -r.det;
    for (int j = 0; j < 3; ++j) r.num.c[j] = -r.num.c[j];
  }
  return r;
}

// det²·|p - c|² = |p·det - num|².
OB oracle_dist2(const OracleBall& bl, const P3& p) {
  const i64 c[3] = {p.x, p.y, p.z};
  OB s = ob(0);
  for (int i = 0; i < 3; ++i) {
    const OB d = ob(c[i]) * bl.det - bl.num.c[i];
    s = s + d * d;
  }
  return s;
}

// (2) orientations de Cramer, det > 0 canonique.
bool oracle_inside_orient(const OracleBall& bl, const P3* v[4]) {
  for (int s = 0; s < 4; ++s) {
    const P3* fp[3];
    int t = 0;
    for (int i = 0; i < 4; ++i)
      if (i != s) fp[t++] = v[i];
    const OV3 e1 = ov_diff(*fp[1], *fp[0]);
    const OV3 e2 = ov_diff(*fp[2], *fp[0]);
    const OV3 es = ov_diff(*v[s], *fp[0]);
    const i64 p0[3] = {fp[0]->x, fp[0]->y, fp[0]->z};
    OV3 ec;
    for (int i = 0; i < 3; ++i) ec.c[i] = bl.num.c[i] - ob(p0[i]) * bl.det;
    const OB side_c = det3(e1, e2, ec);
    const OB side_s = det3(e1, e2, es);
    if (side_c.sign() == 0 || side_c.sign() != side_s.sign()) return false;
  }
  return true;
}

// (2) puissance equatoriale de la face (p,q,r) sur s, en six longueurs carrees :
// (4AB - C2²)F - B(2A - C2)D2 - A(2B - C2)E2 (derivation en tete).
OB oracle_eq_power(i64 l_pq, i64 l_pr, i64 l_ps, i64 l_qr, i64 l_qs, i64 l_rs) {
  const OB A = ob(l_pq), B = ob(l_pr), F = ob(l_ps);
  const OB C2 = A + B - ob(l_qr);
  const OB D2 = A + F - ob(l_qs);
  const OB E2 = B + F - ob(l_rs);
  const OB H = ob(4) * A * B - C2 * C2;
  return H * F - B * (ob(2) * A - C2) * D2 - A * (ob(2) * B - C2) * E2;
}

// ---- comptes et rapports ----------------------------------------------------------

struct Stats {
  u64 tetras = 0, nondeg = 0, supports = 0, supports_with_shell = 0, interiors = 0, frontier = 0;
  u64 i64_rejects = 0, face_rejects = 0, acute_not_centered = 0, obtuse_but_centered = 0;
};

int g_disagreements = 0;
void report(const char* what, PointId i, PointId j, PointId k, PointId l) {
  if (g_disagreements < 40)
    std::fprintf(stderr, "DESACCORD %s sur le tetraedre (%u,%u,%u,%u)\n", what, (unsigned)i, (unsigned)j,
                 (unsigned)k, (unsigned)l);
  ++g_disagreements;
}

struct OraclePoint {
  PointId id;
  P3 pos;
};

// Resultat du jugement d'UN tetraedre (pour les fixtures gravees).
struct Verdict {
  bool degenerate = false;
  bool inside = false;       // oracle ET sujet (un desaccord est rapporte)
  int acute_faces = 0;       // faces strictement aigues
  int zero_faces = 0;        // puissances nulles (frontiere)
  EdgeKey owner{0, 0};
  OB r2_num, r2_den;         // R² oracle : |a·det - num|² / det²
};

// Juge le tetraedre (v[0..3], id[0..3]) contre le sujet ; met a jour st.
Verdict judge_tetra(const OraclePoint* pt[4], const std::vector<OraclePoint>& all, bool with_others, Stats* st) {
  Verdict vd;
  ++st->tetras;
  const P3* p[4] = {&pt[0]->pos, &pt[1]->pos, &pt[2]->pos, &pt[3]->pos};
  const PointId id[4] = {pt[0]->id, pt[1]->id, pt[2]->id, pt[3]->id};
  i64 l[4][4] = {};
  for (int i = 0; i < 4; ++i)
    for (int j = i + 1; j < 4; ++j) l[i][j] = l[j][i] = o_len2(*p[i], *p[j]);

  // (3) owner oracle : longueur maximale, ex aequo par EdgeKey minimale.
  int bu = 0, bv = 1;
  for (int i = 0; i < 4; ++i)
    for (int j = i + 1; j < 4; ++j) {
      if (i == 0 && j == 1) continue;
      const i64 lb = l[bu][bv];
      if (l[i][j] > lb || (l[i][j] == lb && edge_key(id[i], id[j]) < edge_key(id[bu], id[bv]))) {
        bu = i;
        bv = j;
      }
    }
  int ox = -1, oy = -1;
  for (int i = 0; i < 4; ++i)
    if (i != bu && i != bv) (ox < 0 ? ox : oy) = i;
  vd.owner = edge_key(id[bu], id[bv]);
  const P3 &pa = *p[bu], &pb = *p[bv], &px = *p[ox], &py = *p[oy];

  // (1) Cramer oracle contre q4_form.
  const OracleBall bl = oracle_circumball(pa, pb, px, py);
  const Q4Form f4 = q4_form(pa, pb, px, py);
  const bool o_deg = bl.det.is_zero();
  const bool s_deg = f4.det == 0;
  if (o_deg != s_deg) {
    report("degenerescence (det = 0)", id[0], id[1], id[2], id[3]);
    vd.degenerate = true;
    return vd;
  }
  if (o_deg) {
    vd.degenerate = true;
    return vd;
  }
  ++st->nondeg;
  if (cmp(ob128(f4.det), bl.det) != 0) report("det (Cramer relatif vs direct)", id[0], id[1], id[2], id[3]);
  {
    const i64 ac[3] = {pa.x, pa.y, pa.z};
    for (int i = 0; i < 3; ++i)
      if (cmp(ob128(f4.np[i]), bl.num.c[i] - bl.det * ob(ac[i])) != 0) {
        report("N' (num - det·a)", id[0], id[1], id[2], id[3]);
        break;
      }
  }

  // (2)+(6) les quatre faces : puissances oracle, q3_power et prefiltre du sujet.
  bool o_pow_inside = true;
  for (int s = 0; s < 4; ++s) {
    int f[3], t = 0;
    for (int i = 0; i < 4; ++i)
      if (i != s) f[t++] = i;
    const OB pw = oracle_eq_power(l[f[0]][f[1]], l[f[0]][f[2]], l[f[0]][s], l[f[1]][f[2]], l[f[1]][s], l[f[2]][s]);
    const int sg = pw.sign();
    if (sg == 0) ++vd.zero_faces;
    if (sg <= 0) o_pow_inside = false;
    const Q3Form f3 = q3_form(*p[f[0]], *p[f[1]], *p[f[2]]);
    if (sign128(q3_power(f3, *p[s])) != sg) report("signe de q3_power(face, s) vs puissance equatoriale", id[0], id[1], id[2], id[3]);
    const bool pf = q4_face_power_prefilter(f3, *p[s]);
    if (pf != (sg > 0)) report("prefiltre de puissance de face (frontiere ou signe)", id[0], id[1], id[2], id[3]);
    if (!pf) ++st->face_rejects;
    // Acuite stricte de la face par ses trois produits scalaires aux sommets.
    const i64 a0 = l[f[0]][f[1]] + l[f[0]][f[2]] - l[f[1]][f[2]];
    const i64 a1 = l[f[1]][f[0]] + l[f[1]][f[2]] - l[f[0]][f[2]];
    const i64 a2 = l[f[2]][f[0]] + l[f[2]][f[1]] - l[f[0]][f[1]];
    if (a0 > 0 && a1 > 0 && a2 > 0) ++vd.acute_faces;
  }
  if (vd.zero_faces > 0) ++st->frontier;
  const bool o_inside = oracle_inside_orient(bl, p);
  if (o_inside != o_pow_inside) report("ORACLE : orientations de Cramer vs puissances (theoreme)", id[0], id[1], id[2], id[3]);
  const bool s_inside = q4_center_strictly_inside(f4, pa, pb, px, py);
  if (s_inside != o_inside) report("centre strictement interieur", id[0], id[1], id[2], id[3]);
  vd.inside = o_inside;
  if (vd.acute_faces == 4 && !o_inside) ++st->acute_not_centered;
  if (vd.acute_faces < 4 && o_inside) ++st->obtuse_but_centered;

  // (3) owner du sujet : true pour l'owner oracle, false pour les cinq autres.
  for (int i = 0; i < 4; ++i)
    for (int j = i + 1; j < 4; ++j) {
      int w = -1, z = -1;
      for (int k = 0; k < 4; ++k)
        if (k != i && k != j) (w < 0 ? w : z) = k;
      const bool got = tetra_owned_by(l[i][j], l[i][w], l[i][z], l[j][w], l[j][z], l[w][z], id[i], id[j], id[w], id[z]);
      const bool want = (i == bu && j == bv);
      if (got != want) report("owner 6 aretes (tetra_owned_by)", id[0], id[1], id[2], id[3]);
    }

  // (6) etage i64 : necessaire, pour les deux affectations des non-owner.
  const i64 D2 = l[bu][bv];
  const bool pre_xy = q4_i64_prefilter(D2, l[bu][ox], l[bv][ox], l[bu][oy], l[bv][oy], l[ox][oy]);
  const bool pre_yx = q4_i64_prefilter(D2, l[bu][oy], l[bv][oy], l[bu][ox], l[bv][ox], l[ox][oy]);
  if (o_inside) {
    if (!pre_xy || !pre_yx) report("prefiltre i64 : FAUX REJET d'un tetraedre bien centre", id[0], id[1], id[2], id[3]);
  } else if (!pre_xy || !pre_yx) {
    ++st->i64_rejects;
  }

  const OB da2 = oracle_dist2(bl, pa);
  vd.r2_num = da2;
  vd.r2_den = bl.det * bl.det;
  if (!o_inside) return vd;
  ++st->supports;

  // (4) interieur / coquille de chaque autre point.
  if (with_others) {
    const BallKey bk = ball_key_reduce(q4_ball_form(f4));
    u64 shell = 0;
    for (const OraclePoint& z : all) {
      if (&z == pt[0] || &z == pt[1] || &z == pt[2] || &z == pt[3]) continue;
      const int sg = cmp(oracle_dist2(bl, z.pos), da2);
      const int ss = sign128(q4_power(f4, z.pos));
      if (ss != sg) report("interieur / coquille (q4_power)", id[0], id[1], id[2], id[3]);
      if (sign128(bk.power(z.pos)) != sg) report("interieur / coquille (BallKey::power du sujet)", id[0], id[1], id[2], id[3]);
      if (sg < 0) ++st->interiors;
      if (sg == 0) ++shell;
    }
    if (shell > 0) ++st->supports_with_shell;
    // (5) BallKey projective.
    const OB A_o = bl.det * bl.det;
    OB nn = ob(0);
    bool ball_ok = bk.a > 0;
    for (int t = 0; t < 3; ++t) {
      const OB B_ot = ob(-2) * (bl.det * bl.num.c[t]);
      if (cmp(A_o * ob128(bk.b[t]), ob128(bk.a) * B_ot) != 0) ball_ok = false;
      nn = nn + bl.num.c[t] * bl.num.c[t];
    }
    const OB C_o = nn - da2;
    if (cmp(A_o * ob128(bk.c), ob128(bk.a) * C_o) != 0) ball_ok = false;
    if (!ball_ok) report("BallKey (projectivement)", id[0], id[1], id[2], id[3]);
  }
  // (5) niveau : num_s · det_o² == da2 · den_s ; den_s = det_s².
  {
    const ExactLevel lv = q4_level_raw(f4);
    const OB num_s = OB::from_u64_words(lv.num, 3);
    const OB den_s = ob128(lv.den);
    if (cmp(num_s * vd.r2_den, da2 * den_s) != 0) report("niveau |N'|²/det² (produits croises)", id[0], id[1], id[2], id[3]);
    if (lv.den != f4.det * f4.det) report("niveau : den != det²", id[0], id[1], id[2], id[3]);
  }
  return vd;
}

void judge_cloud(const std::vector<OraclePoint>& pts, Stats* st) {
  const size_t m = pts.size();
  for (size_t i0 = 0; i0 < m; ++i0)
    for (size_t i1 = i0 + 1; i1 < m; ++i1)
      for (size_t i2 = i1 + 1; i2 < m; ++i2)
        for (size_t i3 = i2 + 1; i3 < m; ++i3) {
          const OraclePoint* pt[4] = {&pts[i0], &pts[i1], &pts[i2], &pts[i3]};
          judge_tetra(pt, pts, true, st);
        }
}

std::vector<OraclePoint> with_ids(const std::vector<P3>& pts) {
  std::vector<OraclePoint> out;
  std::set<long long> seen;
  for (const P3& p : pts) {
    const long long key = (p.x << 34) | (p.y << 17) | p.z;
    if (!seen.insert(key).second) continue;
    out.push_back(OraclePoint{(PointId)out.size(), p});
  }
  return out;
}

// ---- nuages graves ------------------------------------------------------------------

std::vector<P3> equilateral_max_cloud() {
  const i64 M = 65535;
  return {{0, 0, 0}, {M, M, 0}, {M, 0, M}, {0, M, M}, {M, M, M}, {30000, 20000, 10000}};
}

std::vector<P3> near_right_cloud(const P3& t) {
  return {{t.x, t.y, t.z}, {t.x + 40000, t.y, t.z}, {t.x + 20000, t.y + 20001, t.z},
          {t.x + 20000, t.y + 10000, t.z + 1000}, {t.x + 1000, t.y + 19000, t.z + 2000}};
}

std::vector<P3> cosphere_cloud(i64 cx, const i64 pat[3], const P3& inner) {
  const int perm[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
  std::vector<P3> pts;
  for (const auto& pr : perm)
    for (int sx = -1; sx <= 1; sx += 2)
      for (int sy = -1; sy <= 1; sy += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
          const i64 x = cx + sx * pat[pr[0]], y = cx + sy * pat[pr[1]], z = cx + sz * pat[pr[2]];
          if (x < 0 || y < 0 || z < 0) continue;
          pts.push_back(P3{x, y, z});
        }
  pts.push_back(P3{cx, cx, cx});
  pts.push_back(inner);
  return pts;
}

// Les deux contre-exemples graves (json), translates de +10.
std::vector<P3> refutation_cloud() {
  return {{10, 10, 10}, {8, 8, 9}, {8, 11, 10}, {10, 9, 9}, {8, 10, 12}, {8, 12, 9}};
}

// corner8 (v3) : regulier unite, plat, coquille, tres obtus.
std::vector<P3> corner8_cloud() {
  return {{0, 0, 0},          {0, 1, 1},          {1, 0, 1},          {1, 1, 0},
          {3000, 2000, 2001}, {2000, 3000, 1999}, {1000, 2000, 2001}, {2000, 1000, 1999},
          {4, 0, 0},          {2, 2, 0},          {2, 0, 2},
          {100, 0, 0},        {50, 1, 0},         {50, 0, 1}};
}

std::vector<P3> fixture13_cloud() {
  return {{100, 300, 300}, {300, 300, 300}, {200, 160, 400}, {200, 160, 200},
          {200, 355, 300}, {200, 354, 310}, {200, 353, 315}, {200, 352, 320}, {200, 351, 323},
          {200, 350, 325}, {200, 356, 305}, {200, 355, 312}, {200, 354, 317}};
}

// ---- fixtures gravees : attentes -----------------------------------------------------

struct Expect {
  const char* name;
  P3 p[4];
  PointId id[4];
  bool inside;
  int acute_faces;  // -1 : non exige
  int zero_faces;   // -1 : non exige
  EdgeKey owner;
  i64 r2_num, r2_den;  // 0/0 : non exige
};

void check_expect(const Expect& e, Stats* st) {
  std::vector<OraclePoint> pts;
  for (int i = 0; i < 4; ++i) pts.push_back(OraclePoint{e.id[i], e.p[i]});
  const OraclePoint* pt[4] = {&pts[0], &pts[1], &pts[2], &pts[3]};
  const int before = g_disagreements;
  const Verdict vd = judge_tetra(pt, pts, true, st);
  const auto fx = [&](const char* what) {
    std::fprintf(stderr, "FIXTURE %s : %s\n", e.name, what);
    ++g_disagreements;
  };
  if (g_disagreements != before) fx("desaccord sujet/oracle sur la fixture");
  if (vd.degenerate) { fx("degeneree"); return; }
  if (vd.inside != e.inside) fx(e.inside ? "attendu bien centre" : "attendu NON bien centre");
  if (e.acute_faces >= 0 && vd.acute_faces != e.acute_faces) fx("nombre de faces aigues");
  if (e.zero_faces >= 0 && vd.zero_faces != e.zero_faces) fx("nombre de puissances nulles (frontiere)");
  if (!(vd.owner == e.owner)) fx("owner EdgeKey");
  if (e.r2_den > 0 && cmp(vd.r2_num * ob(e.r2_den), ob(e.r2_num) * vd.r2_den) != 0) fx("R² grave");
}

// Fixture d'arrondi : z ∈ W_3 ∖ W_4 exactement (H = 11, Ξ = 320).
bool check_rounding_fixture() {
  const P3 a{0, 0, 0}, b{8, 0, 0}, z{4, 1, 2};
  // Oracle : H = (z-a)·(b-z), Ξ = |(b-a)×(z-a)|², en OB avec les primitives locales.
  const OV3 w = ov_diff(z, a), d = ov_diff(b, a), bz = ov_diff(b, z);
  const OB H = ov_dot(w, bz);
  const OB Xi = ov_dot(ov_cross(d, w), ov_cross(d, w));
  const OB H2 = H * H;
  bool ok = true;
  if (cmp(H, ob(11)) != 0 || cmp(Xi, ob(320)) != 0) ok = false;
  const bool o_w3 = H.sign() > 0 && cmp(ob(3) * H2, Xi) > 0;   // 363 > 320
  const bool o_w4 = H.sign() > 0 && cmp(ob(2) * H2, Xi) > 0;   // 242 < 320
  if (!o_w3 || o_w4) ok = false;
  if (in_spindle(Lane::kQ3, a, b, z) != o_w3) ok = false;
  if (in_spindle(Lane::kQ4, a, b, z) != o_w4) ok = false;
  // Boule-cœur ponctuelle q4 de l'ancre (boites reduites aux points) : z hors.
  AxisBox A{}, B{};
  A.lo[0] = A.hi[0] = a.x; A.lo[1] = A.hi[1] = a.y; A.lo[2] = A.hi[2] = a.z;
  B.lo[0] = B.hi[0] = b.x; B.lo[1] = B.hi[1] = b.y; B.lo[2] = B.hi[2] = b.z;
  const CoreBall cb = core_ball(Lane::kQ4, A, B);
  if (point_in_ball(z, cb)) ok = false;
  // |z-m|² = 5 en unites simples ; (κ_4 D)² = 64·(2-√3)/4 ≈ 4,29 < 5 : un plafond a 5 l'inclurait.
  const i64 m2[3] = {a.x + b.x, a.y + b.y, a.z + b.z};
  const i64 u0 = 2 * z.x - m2[0], u1 = 2 * z.y - m2[1], u2 = 2 * z.z - m2[2];
  if (u0 * u0 + u1 * u1 + u2 * u2 != 20) ok = false;  // 4·|z-m|² = 20
  if (!ok) {
    std::fprintf(stderr, "FIXTURE arrondi q4 : z=(4,1,2) doit etre dans W_3 et hors W_4 / cœur (H=11, Xi=320)\n");
    ++g_disagreements;
  }
  return ok;
}

struct Args {
  bool ok = true;
  CloudFamily family = CloudFamily::kUniform;
  int n = 50;
  int coord = 0;
  long long seed = 3;
  int grid_m = 48;
  u64 min_tetra = 1, min_frontier = 1, min_shell = 1, min_i64 = 1, min_acute_nc = 1, min_obtuse_c = 1;
  std::string inject;
};

Args parse(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return arg.compare(0, l, prefix) == 0 ? arg.c_str() + l : nullptr;
    };
    if (const char* v = val("--family=")) a.ok = parse_cloud_family(v, &a.family) && a.ok;
    else if (const char* v = val("--n=")) a.n = std::atoi(v);
    else if (const char* v = val("--coord=")) a.coord = std::atoi(v);
    else if (const char* v = val("--seed=")) a.seed = std::atoll(v);
    else if (const char* v = val("--grid-m=")) a.grid_m = std::atoi(v);
    else if (const char* v = val("--min-tetra=")) a.min_tetra = (u64)std::atoll(v);
    else if (const char* v = val("--min-frontier=")) a.min_frontier = (u64)std::atoll(v);
    else if (const char* v = val("--min-shell=")) a.min_shell = (u64)std::atoll(v);
    else if (const char* v = val("--min-i64-rejects=")) a.min_i64 = (u64)std::atoll(v);
    else if (const char* v = val("--min-acute-not-centered=")) a.min_acute_nc = (u64)std::atoll(v);
    else if (const char* v = val("--min-obtuse-centered=")) a.min_obtuse_c = (u64)std::atoll(v);
    else if (const char* v = val("--inject=")) a.inject = v;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      a.ok = false;
    }
  }
  return a;
}

}  // namespace

int main(int argc, char** argv) {
  const Args a = parse(argc, argv);
  if (!a.ok || a.n < 4 || a.grid_m < 4) {
    std::fprintf(stderr, "REFUS : arguments invalides\n");
    return 2;
  }
  if (!a.inject.empty() && !mutants_enable(a.inject)) {
    std::fprintf(stderr, "REFUS : mutant inconnu %s\n", a.inject.c_str());
    return 2;
  }
  const bool mutant = !a.inject.empty();
  const int coord = a.coord > 0 ? a.coord : cloud_family_default_coord(a.family, a.n);
  const std::vector<P3> fam = make_family_cloud(a.family, a.n, coord, a.seed);
  if ((int)fam.size() < a.n) {
    std::fprintf(stderr, "REFUS : la famille n'a produit que %zu points\n", fam.size());
    return 2;
  }
  // Grille serree 14³ : la frontiere Pow = 0 y est frequente (quadruples cospheriques).
  std::vector<P3> grid = make_family_cloud(CloudFamily::kUniform, 200, 14, 3);
  if ((int)grid.size() < a.grid_m) {
    std::fprintf(stderr, "REFUS : grille 14³ trop petite (%zu)\n", grid.size());
    return 2;
  }
  grid.resize((size_t)a.grid_m);
  mhgp5_oracle::clear_overflow();

  Stats st;
  // ---- nuages juges exhaustivement ----
  const i64 small_pat[3] = {3, 4, 0};
  const i64 big_pat[3] = {12000, 16000, 0};
  const std::vector<std::vector<P3>> clouds = {
      fam,
      grid,
      cosphere_cloud(4, small_pat, P3{1, 1, 1}),
      cosphere_cloud(32768, big_pat, P3{30000, 30000, 30000}),
      equilateral_max_cloud(),
      near_right_cloud(P3{0, 0, 0}),
      near_right_cloud(P3{25535, 45534, 63535}),
      refutation_cloud(),
      corner8_cloud(),
      fixture13_cloud(),
  };
  u64 frontier_grid = 0, frontier_cosphere = 0;
  for (size_t c = 0; c < clouds.size(); ++c) {
    const u64 f0 = st.frontier;
    judge_cloud(with_ids(clouds[c]), &st);
    if (c == 1) frontier_grid = st.frontier - f0;
    if (c == 2) frontier_cosphere = st.frontier - f0;
  }

  // ---- fixtures gravees (attentes exactes) ----
  const i64 M = 65535;
  const Expect expects[] = {
      {"six aretes egales (ids permutes)", {{0, 0, 0}, {M, M, 0}, {M, 0, M}, {0, M, M}}, {7, 3, 5, 1},
       true, 4, 0, EdgeKey{1, 3}, 3 * M * M, 4},
      {"bien centre a deux faces obtuses", {{10, 10, 10}, {8, 8, 9}, {8, 11, 10}, {10, 9, 9}}, {0, 1, 2, 3},
       true, 2, 0, EdgeKey{1, 2}, 11, 4},
      {"quatre faces aigues non bien centre", {{10, 10, 10}, {8, 8, 9}, {8, 10, 12}, {8, 12, 9}}, {0, 1, 2, 3},
       false, 4, 0, EdgeKey{1, 3}, 0, 0},
      {"corner8 regulier unite", {{0, 0, 0}, {0, 1, 1}, {1, 0, 1}, {1, 1, 0}}, {0, 1, 2, 3},
       true, 4, 0, EdgeKey{0, 1}, 3, 4},
      {"corner8 plat de poids 1/4", {{3000, 2000, 2001}, {2000, 3000, 1999}, {1000, 2000, 2001}, {2000, 1000, 1999}},
       {0, 1, 2, 3}, true, -1, 0, EdgeKey{0, 2}, 0, 0},
      {"corner8 coquille (ab diametre : frontiere)", {{0, 0, 0}, {4, 0, 0}, {2, 2, 0}, {2, 0, 2}}, {0, 1, 2, 3},
       false, -1, 2, EdgeKey{0, 1}, 4, 1},
      {"corner8 tres obtus", {{0, 0, 0}, {100, 0, 0}, {50, 1, 0}, {50, 0, 1}}, {0, 1, 2, 3},
       false, -1, 0, EdgeKey{0, 1}, 0, 0},
      {"fixture 13 points : tetraedre {0,1,2,3}", {{100, 300, 300}, {300, 300, 300}, {200, 160, 400}, {200, 160, 200}},
       {0, 1, 2, 3}, true, 4, 0, EdgeKey{0, 1}, 14900, 1},
  };
  for (const Expect& e : expects) check_expect(e, &st);
  check_rounding_fixture();

  if (mhgp5_oracle::overflow_seen()) {
    std::fprintf(stderr, "REFUS numeric_failure : debordement de l'oracle OBig (fail-closed)\n");
    return 3;
  }

  std::printf(
      "q4_oracle : famille=%s n=%d coord=%d grille14=%d tetraedres=%llu non_degeneres=%llu bien_centres=%llu "
      "coquilles=%llu interieurs=%llu frontieres=%llu (grille=%llu cosphere=%llu) rejets_i64=%llu rejets_face=%llu "
      "aigus_non_centres=%llu obtus_centres=%llu desaccords=%d\n",
      cloud_family_name(a.family), a.n, coord, a.grid_m, (unsigned long long)st.tetras,
      (unsigned long long)st.nondeg, (unsigned long long)st.supports, (unsigned long long)st.supports_with_shell,
      (unsigned long long)st.interiors, (unsigned long long)st.frontier, (unsigned long long)frontier_grid,
      (unsigned long long)frontier_cosphere, (unsigned long long)st.i64_rejects, (unsigned long long)st.face_rejects,
      (unsigned long long)st.acute_not_centered, (unsigned long long)st.obtuse_but_centered, g_disagreements);

  if (mutant) {
    if (g_disagreements > 0) {
      std::printf("MUTANT TUE : %s\n", a.inject.c_str());
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s non discrimine\n", a.inject.c_str());
    return 3;
  }
  if (g_disagreements > 0) return 1;
  if (st.supports < a.min_tetra || st.frontier < a.min_frontier || st.supports_with_shell < a.min_shell ||
      st.i64_rejects < a.min_i64 || st.acute_not_centered < a.min_acute_nc || st.obtuse_but_centered < a.min_obtuse_c) {
    std::fprintf(stderr,
                 "PLANCHER : bien_centres=%llu (>= %llu), frontieres=%llu (>= %llu), coquilles=%llu (>= %llu), "
                 "rejets_i64=%llu (>= %llu), aigus_non_centres=%llu (>= %llu), obtus_centres=%llu (>= %llu)\n",
                 (unsigned long long)st.supports, (unsigned long long)a.min_tetra, (unsigned long long)st.frontier,
                 (unsigned long long)a.min_frontier, (unsigned long long)st.supports_with_shell,
                 (unsigned long long)a.min_shell, (unsigned long long)st.i64_rejects, (unsigned long long)a.min_i64,
                 (unsigned long long)st.acute_not_centered, (unsigned long long)a.min_acute_nc,
                 (unsigned long long)st.obtuse_but_centered, (unsigned long long)a.min_obtuse_c);
    return 3;
  }
  std::printf("q4_oracle OK\n");
  return 0;
}
