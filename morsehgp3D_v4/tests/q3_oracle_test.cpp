// MorseHGP3D v4 — ORACLE RATIONNEL INDEPENDANT DE LA LANE q3.
//
// Exige par l'audit du 17 aout (Q11 : « maintenant, avant q4 »). Pour de
// petits nuages (n <= 48) et TOUS les triangles {i<j<k}, l'oracle recalcule
// de facon INDEPENDANTE (obigint signe-magnitude 384 bits, jamais les formes
// de la production) :
//   - acuite par les TROIS produits scalaires d'angles (pas V² > D²) ;
//   - circumcentre par Cramer 3×3 direct sur le systeme
//       2(b-a)·c = |b|²-|a|²,  2(x-a)·c = |x|²-|a|²,  n·c = n·a,
//     avec n = (b-a)×(x-a) ;
//   - interieur/coquille par comparaison |z·det - N|² vs |a·det - N|² ;
//   - niveau exact par l'egalite croisee |a·det - N|²·(4G) == D·E·X·det².
// et le confronte aux predicats de production (V²>D², Q3Form/q3_power,
// q3_ball_depth). Largeurs prouvees : det < 2^74, N < 2^109, z·det-N < 2^127,
// carre < 2^254, ×4G < 2^323 < 2^384. Tout debordement d'obigint ABORT.
//
// Codes : 0 accord total ; 1 desaccord ; 3 invariant du test ;
// 4 mutant tue (--inject=sign-p | --inject=prune-ge).
#include <cstdio>
#include <cstring>

#include "../oracle/obigint.hpp"
#include "../src/cloud/families.hpp"
#include "../src/events/q3_instruction.hpp"
#include "../src/events/spindle.hpp"

namespace {

using namespace mhgp4;
using OB = mhgp4_oracle::OBig<6>;

OB ob(i128 v) { return OB::from_i128(v); }

struct OracleBall {
  OB det;        // determinant du systeme (≠ 0 ssi non degenere)
  OB num[3];     // c = num / det
};

// Cramer 3×3 en obigint, colonnes remplacees une a une.
OracleBall oracle_circumball(const P3& a, const P3& b, const P3& x) {
  const P3 e1 = p3_sub(b, a);
  const P3 e2 = p3_sub(x, a);
  const P3 n = p3_cross(e1, e2);
  const i64 r1[3] = {2 * e1.x, 2 * e1.y, 2 * e1.z};
  const i64 r2[3] = {2 * e2.x, 2 * e2.y, 2 * e2.z};
  const i64 r3[3] = {n.x, n.y, n.z};
  const i128 rhs[3] = {(i128)p3_norm2(b) - p3_norm2(a),
                       (i128)p3_norm2(x) - p3_norm2(a),
                       (i128)n.x * a.x + (i128)n.y * a.y + (i128)n.z * a.z};
  const auto det3ob = [&](const OB m[3][3]) {
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
           m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
           m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
  };
  OB m[3][3];
  const i64* rows[3] = {r1, r2, r3};
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
  return res;
}

// Signe de |z - c|² - R² pour c = num/det, R la distance de c a `a` :
// sgn(|z·det - N|² - |a·det - N|²).
int oracle_power_sign(const OracleBall& bl, const P3& a, const P3& z) {
  const auto dist2 = [&](const P3& p) {
    OB s = ob(0);
    const i64 c[3] = {p.x, p.y, p.z};
    for (int i = 0; i < 3; ++i) {
      const OB d = ob(c[i]) * bl.det - bl.num[i];
      s = s + d * d;
    }
    return s;
  };
  return cmp(dist2(z), dist2(a));
}

int g_disagreements = 0;

void report(const char* what, i32 i, i32 j, i32 k) {
  std::fprintf(stderr, "DESACCORD %s sur le triangle (%d,%d,%d)\n", what, i, j, k);
  ++g_disagreements;
}

// Fixtures gravees : cosphere u16 (centre (4,4,4), rayon² 25, coquille des
// 24 permutations signees de (3,4,0)) et tetraedre regulier entier.
std::vector<P3> cosphere_cloud() {
  std::vector<P3> pts;
  const int pat[3] = {3, 4, 0};
  int perm[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
  std::set<long long> seen;
  for (auto& pr : perm)
    for (int sx = -1; sx <= 1; sx += 2)
      for (int sy = -1; sy <= 1; sy += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
          const i64 x = 4 + sx * pat[pr[0]];
          const i64 y = 4 + sy * pat[pr[1]];
          const i64 z = 4 + sz * pat[pr[2]];
          if (x < 0 || y < 0 || z < 0) continue;
          const long long key = (x << 34) | (y << 17) | z;
          if (!seen.insert(key).second) continue;
          pts.push_back(P3{x, y, z});
        }
  pts.push_back(P3{4, 4, 4});
  pts.push_back(P3{1, 1, 1});
  return pts;
}

std::vector<P3> tetra_cloud() {
  return {{0, 0, 0}, {2, 2, 0}, {2, 0, 2}, {0, 2, 2}, {9, 9, 9}, {1, 9, 3}};
}

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  bool inj_sign_p = false, inj_prune_ge = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--inject=sign-p") == 0) inj_sign_p = true;
    else if (std::strcmp(argv[i], "--inject=prune-ge") == 0) inj_prune_ge = true;
  }
  const bool mutant = inj_sign_p || inj_prune_ge;

  std::vector<std::vector<P3>> clouds;
  clouds.push_back(make_family_cloud(CloudFamily::kUniform, 48,
                                     cloud_family_default_coord(CloudFamily::kUniform, 48), 3));
  clouds.push_back(make_family_cloud(CloudFamily::kEightClusters, 48,
                                     cloud_family_default_coord(CloudFamily::kEightClusters, 48), 3));
  clouds.push_back(cosphere_cloud());
  clouds.push_back(tetra_cloud());

  const u64 smax = 11;
  u64 triangles = 0, events_seen = 0, shells_seen = 0;
  for (const std::vector<P3>& pts : clouds) {
    const CloudIndex ix = build_cloud_index(pts);
    const int m = ix.unique_count();
    if ((size_t)m != pts.size()) return 3;  // fixtures a sites distincts
    const u64 h3 = lane_h(Lane::kQ3, std::min<u64>(smax, (u64)m));
    for (i32 i = 0; i < m; ++i)
      for (i32 j = i + 1; j < m; ++j)
        for (i32 k = j + 1; k < m; ++k) {
          ++triangles;
          const P3 &pa = ix.upos[(size_t)i], &pb = ix.upos[(size_t)j],
                   &px = ix.upos[(size_t)k];
          // ORACLE : acuite stricte par les trois angles.
          const bool o_acute = p3_dot(p3_sub(pb, pa), p3_sub(px, pa)) > 0 &&
                               p3_dot(p3_sub(pa, pb), p3_sub(px, pb)) > 0 &&
                               p3_dot(p3_sub(pa, px), p3_sub(pb, px)) > 0;
          // SUJET : owner (longueur max / EdgeKey) puis V² > D² a l'apex.
          const i64 lab = p3_norm2(p3_sub(pb, pa));
          const i64 lax = p3_norm2(p3_sub(px, pa));
          const i64 lbx = p3_norm2(p3_sub(px, pb));
          const P3* v0 = &pa;
          const P3* v1 = &pb;
          const P3* apex = &px;
          i64 lmax = lab;
          PointId ia = (PointId)i, ib2 = (PointId)j;
          i32 uapex = k, u0 = i, u1 = j;
          const auto better = [&](i64 l, PointId x, PointId y) {
            return l > lmax ||
                   (l == lmax && edge_key_less(edge_key(x, y), edge_key(ia, ib2)));
          };
          if (better(lax, (PointId)i, (PointId)k)) {
            lmax = lax; ia = (PointId)i; ib2 = (PointId)k;
            v0 = &pa; v1 = &px; apex = &pb; uapex = j; u0 = i; u1 = k;
          }
          if (better(lbx, (PointId)j, (PointId)k)) {
            lmax = lbx; ia = (PointId)j; ib2 = (PointId)k;
            v0 = &pb; v1 = &px; apex = &pa; uapex = i; u0 = j; u1 = k;
          }
          const P3 vv{2 * apex->x - v0->x - v1->x, 2 * apex->y - v0->y - v1->y,
                      2 * apex->z - v0->z - v1->z};
          const bool s_acute = p3_norm2(vv) > lmax;
          if (o_acute != s_acute) {
            report("acuite", i, j, k);
            continue;
          }
          if (!o_acute) continue;

          // Profondeur et coquille : sujet contre oracle.
          const OracleBall bl = oracle_circumball(*v0, *v1, *apex);
          if (bl.det.is_zero()) {
            report("degenerescence inattendue", i, j, k);
            continue;
          }
          u64 o_depth = 0, o_shell = 0;
          for (i32 u = 0; u < m; ++u) {
            if (u == i || u == j || u == k) continue;
            const int sg = oracle_power_sign(bl, *v0, ix.upos[(size_t)u]);
            if (sg < 0) ++o_depth;
            else if (sg == 0) ++o_shell;
          }
          const Q3Form f = q3_form(*v0, *v1, *apex);
          u64 s_shell = 0;
          u64 s_depth;
          if (inj_sign_p) {
            // MUTANT : P <= 0 compte la coquille comme interieur.
            s_depth = 0;
            for (i32 u = 0; u < m; ++u) {
              if (u == i || u == j || u == k) continue;
              if (q3_power(f, ix.upos[(size_t)u]) <= 0) ++s_depth;
            }
            s_shell = o_shell;  // le mutant ne les distingue plus
          } else {
            s_depth = q3_ball_depth(ix, f, u0, u1, uapex, (u64)m, &s_shell,
                                    inj_prune_ge);
          }
          if (s_depth != o_depth) {
            report("profondeur", i, j, k);
            continue;
          }
          if (s_shell != o_shell) {
            report("coquille", i, j, k);
            continue;
          }
          if (o_depth < h3 && o_shell == 0) {
            ++events_seen;
            // Niveau exact : |a·det - N|²·4G == D·E·X·det².
            const i64 D = p3_norm2(p3_sub(*v1, *v0));
            const i64 E = p3_norm2(p3_sub(*apex, *v0));
            const i64 F = p3_dot(p3_sub(*v1, *v0), p3_sub(*apex, *v0));
            const i64 X = D + E - 2 * F;
            const i128 G4 = 4 * ((i128)D * E - (i128)F * F);
            OB lhs = ob(0);
            const i64 c0[3] = {v0->x, v0->y, v0->z};
            for (int t = 0; t < 3; ++t) {
              const OB d = ob(c0[t]) * bl.det - bl.num[t];
              lhs = lhs + d * d;
            }
            lhs = lhs * ob(G4);
            const OB rhs = ob((i128)D * E) * ob(X) * (bl.det * bl.det);
            if (cmp(lhs, rhs) != 0) report("niveau exact", i, j, k);
          }
          shells_seen += o_shell > 0 ? 1 : 0;
        }
  }

  std::printf(
      "q3_oracle : %llu triangles, %llu evenements, %llu boules a coquille, "
      "desaccords=%d\n",
      (unsigned long long)triangles, (unsigned long long)events_seen,
      (unsigned long long)shells_seen, g_disagreements);
  if (mutant) {
    if (g_disagreements > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (g_disagreements > 0) return 1;
  if (events_seen == 0 || shells_seen == 0) {
    std::fprintf(stderr, "PLANCHER : evenements=%llu, coquilles=%llu\n",
                 (unsigned long long)events_seen, (unsigned long long)shells_seen);
    return 3;
  }
  return 0;
}
