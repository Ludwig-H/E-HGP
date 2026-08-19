// MorseHGP3D v4 — ORACLE RATIONNEL INDEPENDANT DE LA LANE q4.
//
// Memes standards que l'oracle q3 durci (audit ebc8236) : arithmetique
// obigint 384 bits, primitives dot/cross/norm2/sub REECRITES localement,
// points recus en OraclePoint{id,pos}. Pour TOUS les 4-sous-ensembles des
// petits nuages et des fixtures u16 extremes, l'oracle recalcule :
//   - l'owner par les six longueurs (EdgeKey minimale aux ex aequo) ;
//   - le circumcentre par Cramer 3×3 direct sur
//       2(b-a)·c = |b|²-|a|²,  2(x-a)·c = |x|²-|a|²,  2(y-a)·c = |y|²-|a|²
//     (systeme plein : quatre points non coplanaires <=> det != 0) ;
//   - le centre STRICTEMENT interieur par quatre orientations en OBig
//     (sign(det3(q-p, r-p, num-det·p)) contre sign(det)·sign(cote de s)) ;
//   - interieur/coquille par |z·det - num|² vs |a·det - num|² ;
//   - la BallKey du sujet PROJECTIVEMENT (A_o = det², B_o = -2·det·num,
//     C_o = |num|² - Rnum) ;
//   - le niveau du sujet par produits croises |N'|²·det_o² == Rnum·det_s²
//     (largeurs < 2^262 < 2^384).
// et le confronte a q4_form / q4_center_strictly_inside / q4_power.
// Fixtures extremes : tetraedre REGULIER entier a M = 65535 (les quatre
// premiers points d'equilateral_max), grande cosphere rayon 20000 (des
// tetraedres a ~20 points de coquille), presque-rectangle translate au
// bord. Compteurs de limbes publies, plancher niveau >= limbe 3.
//
// Codes : 0 accord ; 1 desaccord ; 3 invariant/plancher/refus numerique ;
// 4 mutant tue (--inject=cramer-swap | mul-carry-lost | sign-p4 |
//   q4-center-parity).
#include <cstdio>
#include <cstring>
#include <set>
#include <vector>

#include "../oracle/obigint.hpp"
#include "../src/cloud/families.hpp"
#include "../src/events/q4_event.hpp"
#include "../src/events/spindle.hpp"

namespace {

using namespace mhgp4;
using OB = mhgp4_oracle::OBig<6>;

OB ob(i128 v) { return OB::from_i128(v); }

// Primitives locales de l'oracle — jamais celles de la production.
P3 osub(const P3& a, const P3& b) { return P3{a.x - b.x, a.y - b.y, a.z - b.z}; }
i64 odot(const P3& a, const P3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
i64 onorm2(const P3& a) { return odot(a, a); }

int high_limb(const OB& v) {
  for (int i = 5; i >= 0; --i)
    if (v.w[i]) return i;
  return -1;
}
int g_limb_det = -1, g_limb_num = -1, g_limb_niveau = -1;
void bump(int& g, const OB& v) {
  const int h = high_limb(v);
  if (h > g) g = h;
}

bool g_inj_cramer_swap = false;

struct OracleBall {
  OB det;
  OB num[3];
};

OracleBall oracle_circumball4(const P3& a, const P3& b, const P3& x, const P3& y) {
  const P3 es[3] = {osub(b, a), osub(x, a), osub(y, a)};
  i64 rows[3][3];
  i128 rhs[3];
  const P3* pts[3] = {&b, &x, &y};
  for (int i = 0; i < 3; ++i) {
    rows[i][0] = 2 * es[i].x;
    rows[i][1] = 2 * es[i].y;
    rows[i][2] = 2 * es[i].z;
    rhs[i] = (i128)onorm2(*pts[i]) - onorm2(a);
  }
  const auto det3ob = [&](const OB m[3][3]) {
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
           m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
           m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
  };
  OB m[3][3];
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) m[i][j] = ob(rows[i][j]);
  OracleBall res;
  res.det = det3ob(m);
  for (int col = 0; col < 3; ++col) {
    OB mc[3][3];
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
        mc[i][j] = (j == col) ? ob(rhs[i]) : ob(rows[i][j]);
    res.num[col] = det3ob(mc);
  }
  if (g_inj_cramer_swap) {  // MUTANT
    const OB t = res.num[0];
    res.num[0] = res.num[1];
    res.num[1] = t;
  }
  bump(g_limb_det, res.det);
  for (int c = 0; c < 3; ++c) bump(g_limb_num, res.num[c]);
  return res;
}

OB oracle_dist2(const OracleBall& bl, const P3& p) {
  OB s = ob(0);
  const i64 c[3] = {p.x, p.y, p.z};
  for (int i = 0; i < 3; ++i) {
    const OB d = ob(c[i]) * bl.det - bl.num[i];
    s = s + d * d;
  }
  return s;
}

// Centre strictement interieur, cote oracle : pour chaque face (p,q,r) de
// sommet oppose s, sign(det3(q-p, r-p, num - det·p)) doit etre non nul et
// egal a sign(det)·sign(det3(q-p, r-p, s-p)).
bool oracle_center_inside(const OracleBall& bl, const P3& a, const P3& b,
                          const P3& x, const P3& y) {
  const P3* v[4] = {&a, &b, &x, &y};
  for (int s = 0; s < 4; ++s) {
    const P3* fp[3];
    int t = 0;
    for (int i = 0; i < 4; ++i)
      if (i != s) fp[t++] = v[i];
    const P3 e1 = osub(*fp[1], *fp[0]);
    const P3 e2 = osub(*fp[2], *fp[0]);
    const P3 e3 = osub(*v[s], *fp[0]);
    const auto det3v = [&](const OB c0[3]) {
      const OB r0[3] = {ob(e1.x), ob(e1.y), ob(e1.z)};
      const OB r1[3] = {ob(e2.x), ob(e2.y), ob(e2.z)};
      return r0[0] * (r1[1] * c0[2] - r1[2] * c0[1]) -
             r0[1] * (r1[0] * c0[2] - r1[2] * c0[0]) +
             r0[2] * (r1[0] * c0[1] - r1[1] * c0[0]);
    };
    const i64 p0[3] = {fp[0]->x, fp[0]->y, fp[0]->z};
    OB cc[3];
    for (int i = 0; i < 3; ++i) cc[i] = bl.num[i] - ob(p0[i]) * bl.det;
    const OB sc[3] = {ob(e3.x), ob(e3.y), ob(e3.z)};
    const OB side_c = det3v(cc);
    const OB side_s = det3v(sc);
    const int want = bl.det.sign() * side_s.sign();
    if (side_c.sign() == 0 || side_c.sign() != want) return false;
  }
  return true;
}

int g_disagreements = 0;
void report(const char* what, PointId i, PointId j, PointId k, PointId l) {
  std::fprintf(stderr, "DESACCORD %s sur le tetraedre (%u,%u,%u,%u)\n", what,
               (unsigned)i, (unsigned)j, (unsigned)k, (unsigned)l);
  ++g_disagreements;
}

std::vector<P3> cosphere_cloud(i64 cx, const int pat[3], const P3& inner) {
  std::vector<P3> pts;
  int perm[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
  std::set<long long> seen;
  for (auto& pr : perm)
    for (int sx = -1; sx <= 1; sx += 2)
      for (int sy = -1; sy <= 1; sy += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
          const i64 x = cx + sx * pat[pr[0]];
          const i64 y = cx + sy * pat[pr[1]];
          const i64 z = cx + sz * pat[pr[2]];
          if (x < 0 || y < 0 || z < 0) continue;
          const long long key = (x << 34) | (y << 17) | z;
          if (!seen.insert(key).second) continue;
          pts.push_back(P3{x, y, z});
        }
  pts.push_back(P3{cx, cx, cx});
  pts.push_back(inner);
  return pts;
}

std::vector<P3> equilateral_max_cloud() {
  const i64 M = 65535;
  return {{0, 0, 0}, {M, M, 0}, {M, 0, M}, {0, M, M}, {M, M, M},
          {30000, 20000, 10000}};
}

std::vector<P3> near_right_cloud(const P3& t) {
  return {{t.x + 0, t.y + 0, t.z + 0},
          {t.x + 40000, t.y + 0, t.z + 0},
          {t.x + 20000, t.y + 20001, t.z + 0},
          {t.x + 20000, t.y + 10000, t.z + 1000},
          {t.x + 1000, t.y + 19000, t.z + 2000}};
}

struct OraclePoint {
  PointId id;
  P3 pos;
};

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  bool inj_sign_p4 = false, inj_carry = false, inj_center_parity = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--inject=cramer-swap") == 0) g_inj_cramer_swap = true;
    else if (std::strcmp(argv[i], "--inject=mul-carry-lost") == 0) inj_carry = true;
    else if (std::strcmp(argv[i], "--inject=sign-p4") == 0) inj_sign_p4 = true;
    // MUTANT q4-center-parity : le test d'arite 4 stricte ne recalcule
    // plus quatre fois le volume du tetraedre — il derive les quatre
    // orientations d'UN determinant par la parite (-V, +V, -V, +V,
    // prouvee en tete de q4_instruction.hpp). Inverser cette parite doit
    // faire diverger l'oracle rationnel independant.
    else if (std::strcmp(argv[i], "--inject=q4-center-parity") == 0)
      inj_center_parity = true;
  }
  const bool mutant =
      g_inj_cramer_swap || inj_carry || inj_sign_p4 || inj_center_parity;
  mhgp4_oracle::inject_mul_carry_lost() = inj_carry;

  const int small_pat[3] = {3, 4, 0};
  const int big_pat[3] = {12000, 16000, 0};
  std::vector<std::vector<P3>> clouds;
  clouds.push_back(make_family_cloud(CloudFamily::kUniform, 26,
                                     cloud_family_default_coord(CloudFamily::kUniform, 26), 3));
  clouds.push_back(make_family_cloud(CloudFamily::kEightClusters, 26,
                                     cloud_family_default_coord(CloudFamily::kEightClusters, 26), 3));
  clouds.push_back(cosphere_cloud(4, small_pat, P3{1, 1, 1}));
  clouds.push_back(equilateral_max_cloud());
  clouds.push_back(cosphere_cloud(32768, big_pat, P3{30000, 30000, 30000}));
  clouds.push_back(near_right_cloud(P3{0, 0, 0}));
  clouds.push_back(near_right_cloud(P3{25535, 45534, 63535}));

  u64 tetras = 0, supports = 0, supports_with_extra_shell = 0;
  for (const std::vector<P3>& pts : clouds) {
    std::vector<OraclePoint> ipts(pts.size());
    for (size_t p = 0; p < pts.size(); ++p) ipts[p] = OraclePoint{(PointId)p, pts[p]};
    const size_t m = ipts.size();
    for (size_t i0 = 0; i0 < m; ++i0)
      for (size_t i1 = i0 + 1; i1 < m; ++i1)
        for (size_t i2 = i1 + 1; i2 < m; ++i2)
          for (size_t i3 = i2 + 1; i3 < m; ++i3) {
            ++tetras;
            const size_t vs[4] = {i0, i1, i2, i3};
            // Owner par les six longueurs (oracle, primitives locales).
            size_t bu = i0, bv = i1;
            i64 bl2 = -1;
            for (int s0 = 0; s0 < 4; ++s0)
              for (int s1 = s0 + 1; s1 < 4; ++s1) {
                const i64 l2 = onorm2(osub(ipts[vs[s1]].pos, ipts[vs[s0]].pos));
                if (l2 > bl2 ||
                    (l2 == bl2 &&
                     edge_key_less(edge_key(ipts[vs[s0]].id, ipts[vs[s1]].id),
                                   edge_key(ipts[bu].id, ipts[bv].id)))) {
                  bl2 = l2;
                  bu = vs[s0];
                  bv = vs[s1];
                }
              }
            size_t ox = 0, oy = 0;
            bool first = true;
            for (const size_t u : vs)
              if (u != bu && u != bv) {
                (first ? ox : oy) = u;
                first = false;
              }
            const P3 &pa = ipts[bu].pos, &pb = ipts[bv].pos;
            const P3 &px = ipts[ox].pos, &py = ipts[oy].pos;
            const PointId da = ipts[i0].id, db = ipts[i1].id, dc = ipts[i2].id,
                          dd = ipts[i3].id;
            // ORACLE : Cramer OBig ; SUJET : q4_form (adjugate i128).
            const OracleBall bl = oracle_circumball4(pa, pb, px, py);
            const Q4Form f4 = q4_form(pa, pb, px, py);
            const bool o_degenerate = bl.det.is_zero();
            const bool s_degenerate = f4.det == 0;
            if (o_degenerate != s_degenerate) {
              report("degenerescence", da, db, dc, dd);
              continue;
            }
            if (o_degenerate) continue;
            const bool o_inside = oracle_center_inside(bl, pa, pb, px, py);
            const bool s_inside =
                q4_center_strictly_inside(f4, pa, pb, px, py, inj_center_parity);
            if (o_inside != s_inside) {
              report("centre interieur", da, db, dc, dd);
              continue;
            }
            if (!o_inside) continue;
            ++supports;
            // Profondeur, coquille, interieurs.
            const OB da2 = oracle_dist2(bl, pa);
            u64 o_depth = 0, o_shell = 0;
            std::vector<PointId> o_int;
            u64 s_depth = 0, s_shell = 0;
            std::vector<PointId> s_int;
            for (size_t u = 0; u < m; ++u) {
              if (u == i0 || u == i1 || u == i2 || u == i3) continue;
              const int sg = cmp(oracle_dist2(bl, ipts[u].pos), da2);
              if (sg < 0) {
                ++o_depth;
                if (o_depth <= 8) o_int.push_back(ipts[u].id);
              } else if (sg == 0) {
                ++o_shell;
              }
              const i128 pw = q4_power(f4, ipts[u].pos);
              // MUTANT sign-p4 : P <= 0 compte la coquille comme interieur.
              if (inj_sign_p4 ? (pw <= 0) : (pw < 0)) {
                ++s_depth;
                if (s_depth <= 8) s_int.push_back(ipts[u].id);
              } else if (pw == 0) {
                ++s_shell;
              }
            }
            if (inj_sign_p4) s_shell = o_shell;
            if (s_depth != o_depth) {
              report("profondeur", da, db, dc, dd);
              continue;
            }
            if (s_shell != o_shell) {
              report("coquille", da, db, dc, dd);
              continue;
            }
            if (o_depth <= 8 && s_int != o_int) {
              report("interieurs", da, db, dc, dd);
              continue;
            }
            if (o_shell > 0) ++supports_with_extra_shell;
            // BallKey du sujet, projectivement contre la forme oracle.
            const Q3BallKey bk = q3_ball_key_reduce(q4_ball_form(f4));
            const OB A_o = bl.det * bl.det;
            bool ball_ok = true;
            OB nn = ob(0);
            for (int t = 0; t < 3; ++t) {
              const OB B_ot = ob(-2) * (bl.det * bl.num[t]);
              if (cmp(A_o * ob(bk.b[t]), ob(bk.a) * B_ot) != 0) ball_ok = false;
              nn = nn + bl.num[t] * bl.num[t];
            }
            const OB C_o = nn - da2;
            if (cmp(A_o * ob(bk.c), ob(bk.a) * C_o) != 0) ball_ok = false;
            if (!ball_ok) report("ballkey", da, db, dc, dd);
            // Niveau du sujet (U192, det_s²) : |N'|²·det_o² == Rnum·det_s².
            // Les deux membres sont ici les MEMES valeurs entieres
            // (|N'|² = Rnum, det_o² = den) : une corruption structurelle de
            // la multiplication frapperait les deux cotes a l'identique.
            // L'associativite (num·det)·det casse deliberement la symetrie
            // des paires d'operandes — c'est elle qui rend le mutant
            // mul-carry-lost visible (largeur intermediaire < 2^203).
            const Q4Level lv = q4_level_raw(f4);
            OB numo;
            numo.w[0] = lv.num[0];
            numo.w[1] = lv.num[1];
            numo.w[2] = lv.num[2];
            const OB lhs = (numo * bl.det) * bl.det;
            const OB rhs = da2 * ob(lv.den);
            bump(g_limb_niveau, lhs);
            bump(g_limb_niveau, rhs);
            if (cmp(lhs, rhs) != 0) report("niveau", da, db, dc, dd);
          }
  }

  std::printf(
      "q4_oracle : %llu tetraedres, %llu supports, "
      "supports_with_extra_shell=%llu, desaccords=%d\n",
      (unsigned long long)tetras, (unsigned long long)supports,
      (unsigned long long)supports_with_extra_shell, g_disagreements);
  std::printf("limbes max (0..5) : det=%d num=%d niveau=%d\n", g_limb_det,
              g_limb_num, g_limb_niveau);
  if (mutant) {
    if (g_disagreements > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (mhgp4_oracle::overflow_seen()) {
    std::fprintf(stderr, "REFUS numeric_failure : debordement obigint\n");
    return 3;
  }
  if (g_disagreements > 0) return 1;
  if (supports == 0 || supports_with_extra_shell == 0) {
    std::fprintf(stderr, "PLANCHER : supports=%llu, coquilles=%llu\n",
                 (unsigned long long)supports,
                 (unsigned long long)supports_with_extra_shell);
    return 3;
  }
  if (g_limb_niveau < 3) {
    std::fprintf(stderr, "PLANCHER : limbe max du niveau %d < 3\n", g_limb_niveau);
    return 3;
  }
  return 0;
}
