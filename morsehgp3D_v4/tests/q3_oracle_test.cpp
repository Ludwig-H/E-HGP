// MorseHGP3D v4 — ORACLE RATIONNEL INDEPENDANT DE LA LANE q3.
//
// Exige par l'audit du 17 aout (Q11), durci par l'audit oracle ebc8236.
// Pour de petits nuages et TOUS les triangles {p<q<r}, l'oracle recalcule de
// facon INDEPENDANTE (obigint signe-magnitude 384 bits, primitives dot/
// cross/norm2 REECRITES LOCALEMENT, points recus en InputPoint{id,pos} sans
// dependre du renommage de CloudIndex) :
//   - acuite par les TROIS produits scalaires d'angles (pas V² > D²) ;
//   - circumcentre par Cramer 3×3 direct sur le systeme
//       2(b-a)·c = |b|²-|a|²,  2(x-a)·c = |x|²-|a|²,  n·c = n·a,
//     avec n = (b-a)×(x-a) ;
//   - interieur/coquille par comparaison |z·det - N|² vs |a·det - N|² ;
//   - niveau exact par l'egalite croisee |a·det - N|²·(4G) == D·E·X·det²,
//     PUIS le niveau public du sujet : num·det² == den·|a·det - N|².
// Largeurs prouvees : det < 2^74, N < 2^109, z·det-N < 2^127, carre < 2^254,
// ×4G < 2^323 < 2^384. Debordement = drapeau collant fail-closed (jamais un
// signal) ; drapeau leve => refus (code 3), jamais un verdict.
//
// Fixtures u16 EXTREMES (audit § 3.1) : triangle equilateral entier a
// l'echelle maximale (cinq points cospheriques a M=65535), triangle presque
// rectangle mais aigu (marge de Thales 40001 sur 1,6·10^9), grande cosphere
// de rayon 20000 centree (32768,32768,32768), et rejeu translate au bord de
// la grille. Les limbes hauts traverses sont MESURES (plus haut limbe non
// nul de det, num, |z·det-N|², produit du niveau) et un plancher exige que
// le produit du niveau atteigne le limbe 3 (bits >= 192).
//
// Codes : 0 accord total ; 1 desaccord ; 3 invariant/plancher/refus
// numerique ; 4 mutant tue. Mutants : --inject=sign-p, --inject=prune-ge,
// --inject=cramer-swap (deux numerateurs du centre echanges),
// --inject=level-4g (facteur 4 omis dans le niveau),
// --inject=mul-carry-lost (retenue du produit long OBig jetee, limbes hauts).
#include <cstdio>
#include <cstring>
#include <set>

#include "../oracle/obigint.hpp"
#include "../src/cloud/families.hpp"
#include "../src/events/q3_event.hpp"
#include "../src/events/spindle.hpp"

namespace {

using namespace mhgp4;
using OB = mhgp4_oracle::OBig<6>;

OB ob(i128 v) { return OB::from_i128(v); }

// ---- Primitives locales de l'oracle (audit § 4.1) : reecrites ici, jamais
// p3_dot/p3_cross/p3_norm2 de la production dans le chemin oracle.
P3 osub(const P3& a, const P3& b) { return P3{a.x - b.x, a.y - b.y, a.z - b.z}; }
i64 odot(const P3& a, const P3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
i64 onorm2(const P3& a) { return odot(a, a); }
P3 ocross(const P3& a, const P3& b) {
  return P3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// ---- Compteurs de limbes (audit § 3.2) : plus haut limbe non nul observe.
int high_limb(const OB& v) {
  for (int i = 5; i >= 0; --i)
    if (v.w[i]) return i;
  return -1;
}
int g_limb_det = -1, g_limb_num = -1, g_limb_dist2 = -1, g_limb_niveau = -1;
void bump(int& g, const OB& v) {
  const int h = high_limb(v);
  if (h > g) g = h;
}

bool g_inj_cramer_swap = false;

struct OracleBall {
  OB det;        // determinant du systeme (≠ 0 ssi non degenere)
  OB num[3];     // c = num / det
};

// Cramer 3×3 en obigint, colonnes remplacees une a une.
OracleBall oracle_circumball(const P3& a, const P3& b, const P3& x) {
  const P3 e1 = osub(b, a);
  const P3 e2 = osub(x, a);
  const P3 n = ocross(e1, e2);
  const i64 r1[3] = {2 * e1.x, 2 * e1.y, 2 * e1.z};
  const i64 r2[3] = {2 * e2.x, 2 * e2.y, 2 * e2.z};
  const i64 r3[3] = {n.x, n.y, n.z};
  const i128 rhs[3] = {(i128)onorm2(b) - onorm2(a),
                       (i128)onorm2(x) - onorm2(a),
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
  if (g_inj_cramer_swap) {  // MUTANT : deux numerateurs du centre echanges
    const OB t = res.num[0];
    res.num[0] = res.num[1];
    res.num[1] = t;
  }
  bump(g_limb_det, res.det);
  for (int col = 0; col < 3; ++col) bump(g_limb_num, res.num[col]);
  return res;
}

// |p·det - N|², le carre de distance homogene au centre rationnel.
OB oracle_dist2(const OracleBall& bl, const P3& p) {
  OB s = ob(0);
  const i64 c[3] = {p.x, p.y, p.z};
  for (int i = 0; i < 3; ++i) {
    const OB d = ob(c[i]) * bl.det - bl.num[i];
    s = s + d * d;
  }
  bump(g_limb_dist2, s);
  return s;
}

// Signe de |z - c|² - R² pour c = num/det, R la distance de c a `a`.
int oracle_power_sign(const OracleBall& bl, const P3& a, const P3& z) {
  return cmp(oracle_dist2(bl, z), oracle_dist2(bl, a));
}

int g_disagreements = 0;

void report(const char* what, PointId i, PointId j, PointId k) {
  std::fprintf(stderr, "DESACCORD %s sur le triangle (%u,%u,%u)\n", what,
               (unsigned)i, (unsigned)j, (unsigned)k);
  ++g_disagreements;
}

// ---- Fixtures gravees.
// Cosphere generique : coquille = permutations signees du motif autour du
// centre, plus le centre et un point interieur excentre.
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

std::vector<P3> tetra_cloud() {
  return {{0, 0, 0}, {2, 2, 0}, {2, 0, 2}, {0, 2, 2}, {9, 9, 9}, {1, 9, 3}};
}

// Triangle equilateral entier a l'echelle maximale u16 (audit § 3.1) : les
// cinq premiers points sont cospheriques (centre (M/2,M/2,M/2), R²=3M²/4),
// toutes les aretes du triangle de base valent 2M² — egalites d'owner
// departagees par EdgeKey sous les plus grands coefficients de la grille.
std::vector<P3> equilateral_max_cloud() {
  const i64 M = 65535;
  return {{0, 0, 0}, {M, M, 0}, {M, 0, M}, {0, M, M}, {M, M, M},
          {30000, 20000, 10000}};
}

// Triangle presque rectangle mais strictement aigu (audit § 3.1) : l'angle a
// l'apex a pour produit scalaire 40001 sur des aretes de 1,6·10^9 — juste du
// cote aigu de la frontiere de Thales. Un point profond et un point externe
// completent. `t` translate le nuage (rejeu au bord maximal de la grille).
std::vector<P3> near_right_cloud(const P3& t) {
  return {{t.x + 0, t.y + 0, t.z + 0},
          {t.x + 40000, t.y + 0, t.z + 0},
          {t.x + 20000, t.y + 20001, t.z + 0},
          {t.x + 20000, t.y + 10000, t.z + 1000},
          {t.x + 1000, t.y + 19000, t.z + 2000}};
}

// InputPoint : l'oracle recoit des points deja formes (audit § 4.2) — l'id
// est le rang dans le nuage d'origine, jamais le renommage de CloudIndex.
struct InputPoint {
  PointId id;
  P3 pos;
};

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  bool inj_sign_p = false, inj_prune_ge = false, inj_level4g = false,
       inj_carry = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--inject=sign-p") == 0) inj_sign_p = true;
    else if (std::strcmp(argv[i], "--inject=prune-ge") == 0) inj_prune_ge = true;
    else if (std::strcmp(argv[i], "--inject=cramer-swap") == 0)
      g_inj_cramer_swap = true;
    else if (std::strcmp(argv[i], "--inject=level-4g") == 0) inj_level4g = true;
    else if (std::strcmp(argv[i], "--inject=mul-carry-lost") == 0)
      inj_carry = true;
  }
  const bool mutant = inj_sign_p || inj_prune_ge || g_inj_cramer_swap ||
                      inj_level4g || inj_carry;
  mhgp4_oracle::inject_mul_carry_lost() = inj_carry;

  // Fixture owner AU-DESSUS DU BIT 31 (audit cible 5964214, § 3) : triangle
  // equilateral entier (0,0,0), (1,1,0), (1,0,1) — trois aretes de longueur
  // carree 2, l'owner est donc choisi PAR la seule EdgeKey minimale sur les
  // vrais PointId. Plusieurs affectations d'IDs, dont au-dessus du bit 31 :
  // un PointId signe i32 y inverserait l'ordre et deplacerait l'owner.
  {
    const PointId H = (PointId)1u << 31;
    struct OwnerCase {
      PointId ia, ib, ix2;
      int owner;  // 0 = arete ab, 1 = ax, 2 = bx
    };
    const OwnerCase cases[] = {
        {7, 3, 5, 2},               // (3,5) minimale : bx
        {H + 7, H + 3, 5, 2},       // (5, 2^31+3) minimale : bx
        {5, H + 3, H + 7, 0},       // (5, 2^31+3) minimale : ab
        {0xFFFFFFFEu, 1, 0xFFFFFFFFu, 0},  // (1, 2^32-2) minimale : ab
    };
    for (const OwnerCase& c : cases) {
      const bool owns[3] = {anchor_owns_q3(2, 2, 2, c.ia, c.ib, c.ix2),
                            anchor_owns_q3(2, 2, 2, c.ia, c.ix2, c.ib),
                            anchor_owns_q3(2, 2, 2, c.ib, c.ix2, c.ia)};
      int owner = -1, count = 0;
      for (int t = 0; t < 3; ++t)
        if (owns[t]) { owner = t; ++count; }
      if (count != 1 || owner != c.owner) {
        std::fprintf(stderr,
                     "FIXTURE owner bit31 : ids (%u,%u,%u) -> owner %d "
                     "(attendu %d, %d gagnants)\n",
                     (unsigned)c.ia, (unsigned)c.ib, (unsigned)c.ix2, owner,
                     c.owner, count);
        return 3;
      }
    }
  }

  const int small_pat[3] = {3, 4, 0};
  const int big_pat[3] = {12000, 16000, 0};
  std::vector<std::vector<P3>> clouds;
  clouds.push_back(make_family_cloud(CloudFamily::kUniform, 48,
                                     cloud_family_default_coord(CloudFamily::kUniform, 48), 3));
  clouds.push_back(make_family_cloud(CloudFamily::kEightClusters, 48,
                                     cloud_family_default_coord(CloudFamily::kEightClusters, 48), 3));
  clouds.push_back(cosphere_cloud(4, small_pat, P3{1, 1, 1}));
  clouds.push_back(tetra_cloud());
  clouds.push_back(equilateral_max_cloud());
  clouds.push_back(cosphere_cloud(32768, big_pat, P3{30000, 30000, 30000}));
  clouds.push_back(near_right_cloud(P3{0, 0, 0}));
  clouds.push_back(near_right_cloud(P3{25535, 45534, 63535}));

  const u64 smax = 11;
  u64 triangles = 0, events_seen = 0, supports_with_extra_shell = 0;
  std::set<Q3BallKey> degenerate_ballkeys;
  for (const std::vector<P3>& pts : clouds) {
    // Sujet : index de production. Oracle : liste InputPoint independante.
    const CloudIndex ix = build_cloud_index(pts);
    const int m = ix.unique_count();
    if ((size_t)m != pts.size()) return 3;  // fixtures a sites distincts
    std::vector<InputPoint> ipts(pts.size());
    std::vector<i32> uidx(pts.size());  // rang d'origine -> index unique sujet
    for (size_t p = 0; p < pts.size(); ++p) {
      ipts[p] = InputPoint{(PointId)p, pts[p]};
      i32 u = -1;
      for (i32 v = 0; v < m; ++v)
        if (ix.upos[(size_t)v].x == pts[p].x && ix.upos[(size_t)v].y == pts[p].y &&
            ix.upos[(size_t)v].z == pts[p].z) {
          u = v;
          break;
        }
      if (u < 0) return 3;
      uidx[p] = u;
    }
    const u64 h3 = lane_h(Lane::kQ3, std::min<u64>(smax, (u64)m));
    for (size_t p = 0; p < ipts.size(); ++p)
      for (size_t q = p + 1; q < ipts.size(); ++q)
        for (size_t r = q + 1; r < ipts.size(); ++r) {
          ++triangles;
          const P3 &pa = ipts[p].pos, &pb = ipts[q].pos, &px = ipts[r].pos;
          const PointId da = ipts[p].id, db = ipts[q].id, dx = ipts[r].id;
          // ORACLE : acuite stricte par les trois angles, primitives locales.
          const bool o_acute = odot(osub(pb, pa), osub(px, pa)) > 0 &&
                               odot(osub(pa, pb), osub(px, pb)) > 0 &&
                               odot(osub(pa, px), osub(pb, px)) > 0;
          // SUJET : owner (longueur max / EdgeKey) puis V² > D² a l'apex.
          const i64 lab = p3_norm2(p3_sub(pb, pa));
          const i64 lax = p3_norm2(p3_sub(px, pa));
          const i64 lbx = p3_norm2(p3_sub(px, pb));
          const P3* v0 = &pa;
          const P3* v1 = &pb;
          const P3* apex = &px;
          i64 lmax = lab;
          PointId ia = da, ib2 = db;
          size_t oapex = r, o0 = p, o1 = q;
          const auto better = [&](i64 l, PointId x, PointId y) {
            return l > lmax ||
                   (l == lmax && edge_key_less(edge_key(x, y), edge_key(ia, ib2)));
          };
          if (better(lax, da, dx)) {
            lmax = lax; ia = da; ib2 = dx;
            v0 = &pa; v1 = &px; apex = &pb; oapex = q; o0 = p; o1 = r;
          }
          if (better(lbx, db, dx)) {
            lmax = lbx; ia = db; ib2 = dx;
            v0 = &pb; v1 = &px; apex = &pa; oapex = p; o0 = q; o1 = r;
          }
          const P3 vv{2 * apex->x - v0->x - v1->x, 2 * apex->y - v0->y - v1->y,
                      2 * apex->z - v0->z - v1->z};
          const bool s_acute = p3_norm2(vv) > lmax;
          if (o_acute != s_acute) {
            report("acuite", da, db, dx);
            continue;
          }
          if (!o_acute) continue;

          // Profondeur et coquille : sujet contre oracle. L'oracle parcourt
          // SA liste InputPoint, jamais l'ordre interne du sujet.
          const OracleBall bl = oracle_circumball(*v0, *v1, *apex);
          if (bl.det.is_zero()) {
            report("degenerescence inattendue", da, db, dx);
            continue;
          }
          u64 o_depth = 0, o_shell = 0;
          std::vector<PointId> o_interior;  // tries : boucle croissante en id
          for (size_t u = 0; u < ipts.size(); ++u) {
            if (u == p || u == q || u == r) continue;
            const int sg = oracle_power_sign(bl, *v0, ipts[u].pos);
            if (sg < 0) {
              ++o_depth;
              if (o_depth <= 8) o_interior.push_back(ipts[u].id);
            } else if (sg == 0) {
              ++o_shell;
            }
          }
          const Q3Form f = q3_form(*v0, *v1, *apex);
          u64 s_shell = 0;
          u64 s_depth;
          i32 s_interior_u[8];
          u8 s_ni = 0;
          if (inj_sign_p) {
            // MUTANT : P <= 0 compte la coquille comme interieur.
            s_depth = 0;
            for (size_t u = 0; u < ipts.size(); ++u) {
              if (u == p || u == q || u == r) continue;
              if (q3_power(f, ipts[u].pos) <= 0) ++s_depth;
            }
            s_shell = o_shell;  // le mutant ne les distingue plus
          } else {
            s_depth = q3_ball_depth(ix, f, uidx[o0], uidx[o1], uidx[oapex],
                                    (u64)m, &s_shell, inj_prune_ge, s_interior_u,
                                    &s_ni);
          }
          if (s_depth != o_depth) {
            report("profondeur", da, db, dx);
            continue;
          }
          if (s_shell != o_shell) {
            report("coquille", da, db, dx);
            continue;
          }
          // Liste des interieurs (audit cible 5964214, § 1) : les IDs
          // EXTERNES du collecteur sujet, tries, contre la liste oracle —
          // seulement quand elle tient dans le contrat (profondeur <= 8).
          if (!inj_sign_p && o_depth <= 8) {
            std::vector<PointId> s_int;
            for (u8 t = 0; t < s_ni; ++t) {
              const i32 u = s_interior_u[t];
              s_int.push_back(ix.bucket_ids[ix.bucket_start[(size_t)u]]);
            }
            std::sort(s_int.begin(), s_int.end());
            if (s_int != o_interior) {
              report("interieurs", da, db, dx);
              continue;
            }
          }
          if (o_depth < h3 && o_shell == 0) {
            ++events_seen;
            // Niveau exact, cote oracle : |a·det - N|²·4G == D·E·X·det²,
            // grandeurs D,E,F,X recalculees par les primitives LOCALES.
            const i64 D = onorm2(osub(*v1, *v0));
            const i64 E = onorm2(osub(*apex, *v0));
            const i64 F = odot(osub(*v1, *v0), osub(*apex, *v0));
            const i64 X = D + E - 2 * F;
            const i128 G = (i128)D * E - (i128)F * F;
            // MUTANT level-4g : facteur 4 omis dans le niveau.
            const i128 gfac = inj_level4g ? G : 4 * G;
            const OB dist2_a = oracle_dist2(bl, *v0);
            const OB lhs = dist2_a * ob(gfac);
            const OB rhs = ob((i128)D * E) * ob(X) * (bl.det * bl.det);
            bump(g_limb_niveau, lhs);
            bump(g_limb_niveau, rhs);
            if (cmp(lhs, rhs) != 0) report("niveau exact", da, db, dx);
            // Niveau public du sujet (fraction canonique, rayon au carre) :
            // num·det² == den·|a·det - N|².
            const Q3Level lv = q3_exact_level(*v0, *v1, *apex);
            const OB s_lhs = ob(lv.num) * (bl.det * bl.det);
            const OB s_rhs = ob(lv.den) * dist2_a;
            if (cmp(s_lhs, s_rhs) != 0) report("niveau public", da, db, dx);
            // BallKey primitive du sujet contre la forme oracle (audit cible
            // 5964214, § 1) : A_o = det², B_o = -2·det·num, C_o = |num|² -
            // |a·det - N|², comparees PROJECTIVEMENT (produits croises ; les
            // deux formes ont un coefficient de tete > 0, la primitivite du
            // sujet fait le reste). Largeurs : A_o·b < 2^234, a·B_o < 2^252,
            // A_o·c < 2^252, a·C_o < 2^286 — tout < 2^384.
            const Q3BallKey bk = q3_ball_key(f);
            const OB A_o = bl.det * bl.det;
            bool ball_ok = true;
            OB nn = ob(0);
            for (int t = 0; t < 3; ++t) {
              const OB B_ot = ob(-2) * (bl.det * bl.num[t]);
              if (cmp(A_o * ob(bk.b[t]), ob(bk.a) * B_ot) != 0) ball_ok = false;
              nn = nn + bl.num[t] * bl.num[t];
            }
            const OB C_o = nn - dist2_a;
            if (cmp(A_o * ob(bk.c), ob(bk.a) * C_o) != 0) ball_ok = false;
            if (!ball_ok) report("ballkey", da, db, dx);
          }
          if (o_shell > 0) {
            ++supports_with_extra_shell;
            degenerate_ballkeys.insert(q3_ball_key(f));
          }
        }
  }

  std::printf(
      "q3_oracle : %llu triangles, %llu evenements, "
      "supports_with_extra_shell=%llu, ballkeys_degenerees_uniques=%zu, "
      "desaccords=%d\n",
      (unsigned long long)triangles, (unsigned long long)events_seen,
      (unsigned long long)supports_with_extra_shell, degenerate_ballkeys.size(),
      g_disagreements);
  std::printf("limbes max (0..5) : det=%d num=%d dist2=%d niveau=%d\n",
              g_limb_det, g_limb_num, g_limb_dist2, g_limb_niveau);
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
  if (events_seen == 0 || supports_with_extra_shell == 0) {
    std::fprintf(stderr, "PLANCHER : evenements=%llu, supports a coquille=%llu\n",
                 (unsigned long long)events_seen,
                 (unsigned long long)supports_with_extra_shell);
    return 3;
  }
  // Plancher de largeur (audit § 3.2) : les fixtures extremes doivent faire
  // traverser au produit du niveau le limbe 3 (bits >= 192) — sans quoi la
  // largeur 6 limbes resterait « prouvee mais jamais mordue » sans mesure.
  if (g_limb_niveau < 3) {
    std::fprintf(stderr, "PLANCHER : limbe max du niveau %d < 3\n",
                 g_limb_niveau);
    return 3;
  }
  return 0;
}
