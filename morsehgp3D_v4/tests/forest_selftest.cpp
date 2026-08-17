// MorseHGP3D v4 — SELFTEST DE FORET : sujet contre juge independant.
//
// SUJET (petits n, regime oracle T2) : evenements par enumeration aux
// PREDICATS DE PRODUCTION (q2 census diametral, q3 owner/acuite/Gram, q4
// owner 6 aretes/arite stricte/puissance affine — equivalents aux pipelines
// WSPD, prouve par les portes par lane), puis `build_forest` (union-find a
// macro-lots `same_exact_level`).
//
// JUGE (independant) : enumeration de TOUS les sous-ensembles σ, miniboule
// par recherche de support PROPRE (paires/triplets/quadruplets, centre
// strictement dans l'enveloppe relative, plus petite boule contenante —
// comparaisons de distances rationnelles en OBig 384 bits), Gabriel = boule
// ouverte vide ; un point (de σ ou externe) SUR la sphere hors support ⟹ σ
// ecarte (le meme refus transactionnel que la production). Puis le K-graphe
// du manuscrit (cliques COMPLETES, Def. 29) et un Kruskal PROPRE a lots
// (comparateur de niveaux OBig). Comparaison par K : sommets, nombre de
// lots, PARTITION apres chaque lot, multiensemble des nœuds (lot, absorbees),
// egalite croisee des niveaux de lot, `attach_violations == 0`.
//
// Fixtures gravees :
//  - colineaire3 : K=1, deux aretes de niveau 1 → UN nœud ternaire (le
//    macro-lot), jamais une chaine binaire ;
//  - tie q4/q2 : un tetraedre R² = 14900 (representant U192 non reduit) et
//    une arete D²/4 = 14900 (fraction canonique) dans la meme foret K=3 —
//    meme niveau semantique, representants DIFFERENTS : le mutant
//    « egalite de representation » brise le lot a tort.
// Codes : 0 accord ; 1 desaccord ; 3 invariant/plancher ; 4 mutant tue
// (--inject=binary-ties | repr-ties).
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <vector>

#include "../oracle/obigint.hpp"
#include "../src/cloud/families.hpp"
#include "../src/events/q2_event.hpp"
#include "../src/events/q4_event.hpp"
#include "../src/forest/forest.hpp"
#include "../src/forest/sphere_plateau.hpp"

namespace {

using namespace mhgp4;
using OB = mhgp4_oracle::OBig<6>;

OB ob(i128 v) { return OB::from_i128(v); }

// ---- Boule rationnelle du juge : centre = cnum/cden, rayon par un point
// de support de reference. Comparaisons homogenes en OBig.
struct JBall {
  i128 cnum[3];
  i128 cden;  // > 0
  P3 ref;     // un point du support (sur la sphere)
};

OB jdist2(const JBall& b, const P3& p) {
  OB s = ob(0);
  const i64 c[3] = {p.x, p.y, p.z};
  for (int i = 0; i < 3; ++i) {
    const OB d = ob(c[i]) * ob(b.cden) - ob(b.cnum[i]);
    s = s + d * d;
  }
  return s;
}

// -1/0/+1 : position de p par rapport a la sphere de b.
int jside(const JBall& b, const P3& p) { return cmp(jdist2(b, p), jdist2(b, b.ref)); }

// R²(b1) <=> R²(b2) par produits croises (denominateurs cden² > 0).
int jcmp_r2(const JBall& b1, const JBall& b2) {
  const OB l = jdist2(b1, b1.ref) * (ob(b2.cden) * ob(b2.cden));
  const OB r = jdist2(b2, b2.ref) * (ob(b1.cden) * ob(b1.cden));
  return cmp(l, r);
}

i64 jdot(const P3& a, const P3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
P3 jsub(const P3& a, const P3& b) { return P3{a.x - b.x, a.y - b.y, a.z - b.z}; }
P3 jcross(const P3& a, const P3& b) {
  return P3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
i64 jn2(const P3& a) { return jdot(a, a); }

// Boule de support d'une paire : centre (p+q)/2, toujours valide.
bool jball2(const P3& p, const P3& q, JBall* out) {
  out->cnum[0] = p.x + q.x;
  out->cnum[1] = p.y + q.y;
  out->cnum[2] = p.z + q.z;
  out->cden = 2;
  out->ref = p;
  return true;
}

// Boule d'un triplet : Cramer 3×3 (deux equidistances + plan), validite =
// centre STRICTEMENT interieur au triangle (barycentriques homogenes).
bool jball3(const P3& p, const P3& q, const P3& r, JBall* out) {
  const P3 e1 = jsub(q, p), e2 = jsub(r, p);
  const P3 n = jcross(e1, e2);
  if (n.x == 0 && n.y == 0 && n.z == 0) return false;  // colineaires
  const i64 rows[3][3] = {{2 * e1.x, 2 * e1.y, 2 * e1.z},
                          {2 * e2.x, 2 * e2.y, 2 * e2.z},
                          {n.x, n.y, n.z}};
  const i128 rhs[3] = {(i128)jn2(q) - jn2(p), (i128)jn2(r) - jn2(p),
                       (i128)n.x * p.x + (i128)n.y * p.y + (i128)n.z * p.z};
  const auto det3 = [&](int col, bool use_rhs) {
    i128 m[3][3];
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
        m[i][j] = (use_rhs && j == col) ? rhs[i] : (i128)rows[i][j];
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
           m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
           m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
  };
  const i128 det = det3(0, false);
  if (det == 0) return false;
  for (int c = 0; c < 3; ++c) out->cnum[c] = det3(c, true);
  out->cden = det;
  if (out->cden < 0) {
    out->cden = -out->cden;
    for (int c = 0; c < 3; ++c) out->cnum[c] = -out->cnum[c];
  }
  out->ref = p;
  // Centre strictement interieur au triangle : c = p + α e1 + β e2 avec
  // α, β > 0 et α + β < 1 — coordonnees via Gram (E1 = e1·e1, ...).
  const i128 v[3] = {out->cnum[0] - out->cden * p.x, out->cnum[1] - out->cden * p.y,
                     out->cnum[2] - out->cden * p.z};  // (c - p)·cden
  const i128 d1 = v[0] * e1.x + v[1] * e1.y + v[2] * e1.z;  // (c-p)·e1·cden
  const i128 d2 = v[0] * e2.x + v[1] * e2.y + v[2] * e2.z;
  const i64 E1 = jn2(e1), E2 = jn2(e2), F = jdot(e1, e2);
  const i128 G = (i128)E1 * E2 - (i128)F * F;  // > 0 (non colineaires)
  // α·G·cden = d1·E2 − d2·F ; β·G·cden = d2·E1 − d1·F (cden > 0, G > 0).
  const OB alpha = ob(d1) * ob(E2) - ob(d2) * ob(F);
  const OB beta = ob(d2) * ob(E1) - ob(d1) * ob(F);
  const OB gc = ob(G) * ob(out->cden);
  if (alpha.sign() <= 0 || beta.sign() <= 0) return false;
  if (cmp(alpha + beta, gc) >= 0) return false;  // α + β < 1 strict
  return true;
}

// Boule d'un quadruplet : Cramer plein, validite = centre strictement
// interieur au tetraedre (quatre orientations, arithmetique du juge).
bool jball4(const P3& p, const P3& q, const P3& r, const P3& s, JBall* out) {
  const P3 es[3] = {jsub(q, p), jsub(r, p), jsub(s, p)};
  const i64 rows[3][3] = {{2 * es[0].x, 2 * es[0].y, 2 * es[0].z},
                          {2 * es[1].x, 2 * es[1].y, 2 * es[1].z},
                          {2 * es[2].x, 2 * es[2].y, 2 * es[2].z}};
  const i128 rhs[3] = {(i128)jn2(q) - jn2(p), (i128)jn2(r) - jn2(p),
                       (i128)jn2(s) - jn2(p)};
  const auto det3 = [&](int col, bool use_rhs) {
    i128 m[3][3];
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
        m[i][j] = (use_rhs && j == col) ? rhs[i] : (i128)rows[i][j];
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
           m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
           m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
  };
  const i128 det = det3(0, false);
  if (det == 0) return false;  // coplanaires
  for (int c = 0; c < 3; ++c) out->cnum[c] = det3(c, true);
  out->cden = det;
  if (out->cden < 0) {
    out->cden = -out->cden;
    for (int c = 0; c < 3; ++c) out->cnum[c] = -out->cnum[c];
  }
  out->ref = p;
  const P3* v[4] = {&p, &q, &r, &s};
  for (int f = 0; f < 4; ++f) {
    const P3* fp[3];
    int t = 0;
    for (int i = 0; i < 4; ++i)
      if (i != f) fp[t++] = v[i];
    const P3 a1 = jsub(*fp[1], *fp[0]), a2 = jsub(*fp[2], *fp[0]);
    const P3 a3 = jsub(*v[f], *fp[0]);
    const auto d3 = [&](const OB c0[3]) {
      const OB r0[3] = {ob(a1.x), ob(a1.y), ob(a1.z)};
      const OB r1[3] = {ob(a2.x), ob(a2.y), ob(a2.z)};
      return r0[0] * (r1[1] * c0[2] - r1[2] * c0[1]) -
             r0[1] * (r1[0] * c0[2] - r1[2] * c0[0]) +
             r0[2] * (r1[0] * c0[1] - r1[1] * c0[0]);
    };
    OB cc[3];
    const i64 p0[3] = {fp[0]->x, fp[0]->y, fp[0]->z};
    for (int i = 0; i < 3; ++i)
      cc[i] = ob(out->cnum[i]) - ob(p0[i]) * ob(out->cden);
    const OB sv[3] = {ob(a3.x), ob(a3.y), ob(a3.z)};
    const OB side_c = d3(cc);
    const OB side_s = d3(sv);
    if (side_c.sign() == 0 || side_c.sign() != side_s.sign()) return false;
  }
  return true;
}

int g_fail = 0;
void fail(const char* what, int K) {
  std::fprintf(stderr, "DESACCORD foret K=%d : %s\n", K, what);
  ++g_fail;
}

// ---- Le juge : simplexes de Gabriel par sous-ensembles, K-graphe a
// cliques completes, Kruskal a lots.
struct JSimplex {
  std::vector<i32> pts;  // indices dans le nuage, tries
  std::vector<i32> support;
  JBall ball;
};

struct JForest {
  u64 batches = 0;
  std::vector<std::map<FacetKey, FacetKey>> parts;  // partition apres chaque lot
  std::vector<std::pair<u64, u64>> nodes;           // (lot, absorbees)
  std::vector<const JSimplex*> batch_first;         // niveau de chaque lot
};

FacetKey jfacet(const std::vector<i32>& pts, size_t drop) {
  FacetKey f;
  for (size_t t = 0; t < pts.size(); ++t)
    if (t != drop) f.p[f.k++] = (PointId)pts[t];
  return f;  // pts tries => facette triee
}

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  bool inj_binary = false, inj_repr = false, inj_drop_shell = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--inject=binary-ties") == 0) inj_binary = true;
    else if (std::strcmp(argv[i], "--inject=repr-ties") == 0) inj_repr = true;
    else if (std::strcmp(argv[i], "--inject=drop-shell-plateau") == 0)
      inj_drop_shell = true;
  }
  const bool mutant = inj_binary || inj_repr || inj_drop_shell;

  std::vector<std::vector<P3>> clouds;
  // Fixture du plateau ternaire (K=1) : deux aretes de niveau 1 en un lot.
  clouds.push_back({{0, 0, 0}, {2, 0, 0}, {4, 0, 0}});
  // Fixture du tie q4/q2 : tetraedre R² = 14900 et arete D² = 59600
  // (= (244,8,0)) avec deux interieurs — meme niveau 14900 dans la foret
  // K=3, representants differents (U192 non reduit contre fraction
  // canonique).
  clouds.push_back({{100, 300, 300},
                    {300, 300, 300},
                    {200, 160, 400},
                    {200, 160, 200},
                    {2000, 2000, 2000},
                    {2244, 2008, 2000},
                    {2122, 2004, 2010},
                    {2122, 2004, 1990}});
  // Fixture bloquante square_cospherical_K2_plateau (audit § 1) : quatre
  // points cocycliques, centre (100,100,100), R² = 100. La filtration
  // fusionne les quatre cotes de Gamma_2 au niveau 100 via les quatre
  // triangles RECTANGLES (Gabriel au sens pur : boule ouverte vide) ; le
  // sous-flux regulier n'emet rien — le mutant drop-shell-plateau doit
  // mourir ici.
  clouds.push_back({{110, 100, 100}, {100, 110, 100}, {90, 100, 100},
                    {100, 90, 100}});
  clouds.push_back(make_family_cloud(CloudFamily::kUniform, 12,
                                     cloud_family_default_coord(CloudFamily::kUniform, 12), 3));
  clouds.push_back(make_family_cloud(CloudFamily::kEightClusters, 12,
                                     cloud_family_default_coord(CloudFamily::kEightClusters, 12), 3));

  u64 total_events = 0, total_fusions = 0, plateaus_multi = 0;
  bool collinear_checked = false, tie_checked = false;
  bool square_checked = false;
  for (size_t ci = 0; ci < clouds.size(); ++ci) {
    const std::vector<P3>& pts = clouds[ci];
    const int m = (int)pts.size();
    if (m > 14) return 3;  // regime oracle T2

    // ---- JUGE : tous les sous-ensembles, miniboule propre, Gabriel.
    std::vector<JSimplex> gabriel;
    for (u32 mask = 0; mask < (1u << m); ++mask) {
      const int k = __builtin_popcount(mask);
      if (k < 2 || k > 11) continue;
      std::vector<i32> sub;
      for (int t = 0; t < m; ++t)
        if (mask & (1u << t)) sub.push_back(t);
      // Miniboule par recherche de support : plus petite boule valide
      // contenant le sous-ensemble.
      JBall best{};
      std::vector<i32> best_sup;
      bool have = false;
      const auto consider = [&](JBall& b, std::vector<i32> sup) {
        for (const i32 u : sub) {
          bool in_sup = false;
          for (const i32 su : sup)
            if (su == u) in_sup = true;
          if (in_sup) continue;
          if (jside(b, pts[(size_t)u]) > 0) return;  // hors boule : invalide
        }
        if (!have || jcmp_r2(b, best) < 0) {
          best = b;
          best_sup = sup;
          have = true;
        }
      };
      for (size_t s1 = 0; s1 < sub.size(); ++s1)
        for (size_t s2 = s1 + 1; s2 < sub.size(); ++s2) {
          JBall b;
          if (jball2(pts[(size_t)sub[s1]], pts[(size_t)sub[s2]], &b))
            consider(b, {sub[s1], sub[s2]});
          for (size_t s3 = s2 + 1; s3 < sub.size(); ++s3) {
            if (jball3(pts[(size_t)sub[s1]], pts[(size_t)sub[s2]],
                       pts[(size_t)sub[s3]], &b))
              consider(b, {sub[s1], sub[s2], sub[s3]});
            for (size_t s4 = s3 + 1; s4 < sub.size(); ++s4)
              if (jball4(pts[(size_t)sub[s1]], pts[(size_t)sub[s2]],
                         pts[(size_t)sub[s3]], pts[(size_t)sub[s4]], &b))
                consider(b, {sub[s1], sub[s2], sub[s3], sub[s4]});
          }
        }
      if (!have) continue;  // sous-ensemble degenere (colineaire...)
      // Def. 28 PURE (§ 5.3bis) : seule la boule OUVERTE doit etre vide de
      // X∖σ — un point SUR la sphere (membre de σ au-dela du support, ou
      // externe) est PERMIS : c'est le regime des plateaux.
      bool is_gabriel = true;
      for (i32 u = 0; u < m; ++u) {
        bool in_sup = false, in_sub = (mask >> u) & 1u;
        for (const i32 su : best_sup)
          if (su == u) in_sup = true;
        if (in_sup) continue;
        const int sg = jside(best, pts[(size_t)u]);
        if (sg < 0 && !in_sub) is_gabriel = false;
        else if (sg > 0 && in_sub) return 3;  // impossible : miniboule
      }
      if (!is_gabriel) continue;
      JSimplex js;
      js.pts = sub;
      js.support = best_sup;
      js.ball = best;
      gabriel.push_back(js);
    }

    // ---- SUJET : boules candidates depuis les GENERATEURS de lanes
    // (paire ; triangle strictement aigu d'arete maximale ; tetraedre
    // strictement centre), dedupe par BallKey primitive (commune aux trois
    // lanes : une boule est une boule), census I_B / U_B COMPLET par la
    // forme du generateur, puis EXPANSION du plateau (§ 5.3bis) : tous les
    // σ = I_B ∪ T, T ⊆ U_B, c ∈ conv(T) ferme. Oracle borne : plafond de
    // coquille explicite (resource_exhausted au-dela, jamais de
    // troncature). Le mutant drop-shell-plateau retablit l'ancien refus
    // des boules a extra-coquille : il perd les fusions du carre.
    struct SubjEvent {
      std::vector<i32> tpart, ipart;
      Q4Level level;
    };
    std::vector<SubjEvent> subj;
    std::set<Q3BallKey> seen_balls;
    int subj_rc = 0;
    const auto expand_ball = [&](const Q3BallKey& key, const Q4Level& lvl,
                                 const std::vector<i32>& interior,
                                 const std::vector<i32>& extra_shell,
                                 const std::vector<i32>& gen_support) {
      if (subj_rc) return;
      if (inj_drop_shell && !extra_shell.empty()) return;  // MUTANT
      if (!seen_balls.insert(key).second) return;
      std::vector<i32> shell_all = extra_shell;
      for (const i32 su : gen_support) shell_all.push_back(su);
      if (shell_all.size() > 12) {
        std::fprintf(stderr, "REFUS resource_exhausted : coquille %zu\n",
                     shell_all.size());
        subj_rc = 3;
        return;
      }
      const BallRat c = ball_center(key);
      const u32 nu = (u32)shell_all.size();
      for (u32 tm = 1; tm < (1u << nu); ++tm) {
        const int nt = __builtin_popcount(tm);
        if (nt < 2 || interior.size() + (size_t)nt > 11) continue;
        std::vector<i32> T;
        for (u32 b = 0; b < nu; ++b)
          if (tm & (1u << b)) T.push_back(shell_all[(size_t)b]);
        if (!center_in_conv(c, pts, T)) continue;
        std::sort(T.begin(), T.end());
        subj.push_back(SubjEvent{T, interior, lvl});
      }
    };
    // q2 : toutes les paires (le milieu est toujours un support valide).
    for (i32 i = 0; i < m; ++i)
      for (i32 j = i + 1; j < m; ++j) {
        const P3 &pa = pts[(size_t)i], &pb = pts[(size_t)j];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) return 3;
        const i64 s2[3] = {pa.x + pb.x, pa.y + pb.y, pa.z + pb.z};
        std::vector<i32> interior, extra_shell;
        for (i32 u = 0; u < m; ++u) {
          if (u == i || u == j) continue;
          const i64 t0 = 2 * pts[(size_t)u].x - s2[0];
          const i64 t1 = 2 * pts[(size_t)u].y - s2[1];
          const i64 t2 = 2 * pts[(size_t)u].z - s2[2];
          const i64 d2 = t0 * t0 + t1 * t1 + t2 * t2;
          if (d2 < D2) interior.push_back(u);
          else if (d2 == D2) extra_shell.push_back(u);
        }
        expand_ball(q2_ball_key(pa, pb), promote_q3_level(q2_exact_level(D2)),
                    interior, extra_shell, {i, j});
      }
    // q3 : triangles strictement aigus (etiquetage arete maximale).
    for (i32 i = 0; i < m; ++i)
      for (i32 j = i + 1; j < m; ++j)
        for (i32 k = j + 1; k < m; ++k) {
          const i32 vs[3] = {i, j, k};
          int bu = 0, bv = 1;
          i64 bl2 = -1;
          for (int s0 = 0; s0 < 3; ++s0)
            for (int s1 = s0 + 1; s1 < 3; ++s1) {
              const i64 l2 = p3_norm2(
                  p3_sub(pts[(size_t)vs[s1]], pts[(size_t)vs[s0]]));
              if (l2 > bl2 ||
                  (l2 == bl2 &&
                   edge_key_less(edge_key((PointId)vs[s0], (PointId)vs[s1]),
                                 edge_key((PointId)vs[bu], (PointId)vs[bv]))))
                bl2 = l2, bu = s0, bv = s1;
            }
          const i32 ia = vs[bu], ib = vs[bv];
          i32 ix = -1;
          for (const i32 u : vs)
            if (u != ia && u != ib) ix = u;
          const P3 &pa = pts[(size_t)ia], &pb = pts[(size_t)ib],
                   &px = pts[(size_t)ix];
          const P3 vv{2 * px.x - pa.x - pb.x, 2 * px.y - pa.y - pb.y,
                      2 * px.z - pa.z - pb.z};
          if (p3_norm2(vv) <= bl2) continue;  // pas strictement aigu
          const Q3Form f3 = q3_form(pa, pb, px);
          std::vector<i32> interior, extra_shell;
          for (i32 u = 0; u < m; ++u) {
            if (u == i || u == j || u == k) continue;
            const i128 pw = q3_power(f3, pts[(size_t)u]);
            if (pw < 0) interior.push_back(u);
            else if (pw == 0) extra_shell.push_back(u);
          }
          expand_ball(q3_ball_key(f3),
                      promote_q3_level(q3_exact_level(pa, pb, px)), interior,
                      extra_shell, {ia, ib, ix});
        }
    // q4 : tetraedres strictement centres (etiquetage arete maximale).
    for (i32 i = 0; i < m; ++i)
      for (i32 j = i + 1; j < m; ++j)
        for (i32 k = j + 1; k < m; ++k)
          for (i32 l = k + 1; l < m; ++l) {
            const i32 vs[4] = {i, j, k, l};
            i32 bu = -1, bv = -1;
            i64 bl2 = -1;
            for (int s0 = 0; s0 < 4; ++s0)
              for (int s1 = s0 + 1; s1 < 4; ++s1) {
                const i64 l2 = p3_norm2(
                    p3_sub(pts[(size_t)vs[s1]], pts[(size_t)vs[s0]]));
                if (l2 > bl2 ||
                    (l2 == bl2 &&
                     edge_key_less(edge_key((PointId)vs[s0], (PointId)vs[s1]),
                                   edge_key((PointId)bu, (PointId)bv))))
                  bl2 = l2, bu = vs[s0], bv = vs[s1];
              }
            i32 ox = -1, oy = -1;
            for (const i32 u : vs)
              if (u != bu && u != bv) (ox < 0 ? ox : oy) = u;
            const Q4Form f4 = q4_form(pts[(size_t)bu], pts[(size_t)bv],
                                      pts[(size_t)ox], pts[(size_t)oy]);
            if (f4.det == 0) continue;
            if (!q4_center_strictly_inside(f4, pts[(size_t)bu],
                                           pts[(size_t)bv], pts[(size_t)ox],
                                           pts[(size_t)oy]))
              continue;
            std::vector<i32> interior, extra_shell;
            for (i32 u = 0; u < m; ++u) {
              if (u == i || u == j || u == k || u == l) continue;
              const i128 pw = q4_power(f4, pts[(size_t)u]);
              if (pw < 0) interior.push_back(u);
              else if (pw == 0) extra_shell.push_back(u);
            }
            expand_ball(q3_ball_key_reduce(q4_ball_form(f4)), q4_level_raw(f4),
                        interior, extra_shell, {bu, bv, ox, oy});
          }
    if (subj_rc) return subj_rc;

    // ---- Par K : sujet (plateaux inclus) contre juge (cliques Def. 28).
    for (int K = 1; K <= 10; ++K) {
      std::vector<ForestEvent> sevents;
      for (const SubjEvent& se : subj) {
        if ((int)(se.tpart.size() + se.ipart.size()) != K + 1) continue;
        ForestEvent ev;
        ev.q = (u8)se.tpart.size();
        ev.d = (u8)se.ipart.size();
        for (size_t t = 0; t < se.tpart.size(); ++t)
          ev.support[t] = (PointId)se.tpart[t];
        for (size_t t = 0; t < se.ipart.size(); ++t)
          ev.interior[t] = (PointId)se.ipart[t];
        ev.level = se.level;
        sevents.push_back(ev);
      }
      total_events += sevents.size();

      // SUJET : foret a macro-lots, avec instantanes de partition.
      std::vector<std::map<FacetKey, FacetKey>> sparts;
      const ForestResult sr =
          build_forest(sevents, inj_binary, inj_repr, &sparts);
      total_fusions += sr.fusions;
      if (sr.attach_violations != 0) {
        std::fprintf(stderr, "INVARIANT : attach_violations=%llu (K=%d)\n",
                     (unsigned long long)sr.attach_violations, K);
        return 3;
      }

      // JUGE : cliques completes, Kruskal a lots (arithmetique OBig).
      std::vector<const JSimplex*> jev;
      for (const JSimplex& js : gabriel)
        if ((int)js.pts.size() == K + 1) jev.push_back(&js);
      std::stable_sort(jev.begin(), jev.end(),
                       [](const JSimplex* x, const JSimplex* y) {
                         return jcmp_r2(x->ball, y->ball) < 0;
                       });
      detail_forest::UnionFind juf;
      std::map<FacetKey, i32> jid;
      const auto jfid = [&](const FacetKey& f) {
        const auto it = jid.find(f);
        if (it != jid.end()) return it->second;
        const i32 v = juf.add();
        jid.emplace(f, v);
        return v;
      };
      JForest jf;
      size_t e0 = 0;
      while (e0 < jev.size()) {
        size_t e1 = e0 + 1;
        while (e1 < jev.size() &&
               jcmp_r2(jev[e1]->ball, jev[e0]->ball) == 0)
          ++e1;
        std::map<i32, u64> pre;
        std::vector<std::vector<i32>> ids(e1 - e0);
        for (size_t e = e0; e < e1; ++e)
          for (size_t drop = 0; drop < jev[e]->pts.size(); ++drop) {
            const i32 v = jfid(jfacet(jev[e]->pts, drop));
            ids[e - e0].push_back(v);
            pre[juf.find(v)] = 0;
          }
        for (auto& evi : ids)  // clique = chemin depuis la premiere facette
          for (size_t w = 1; w < evi.size(); ++w) juf.unite(evi[0], evi[w]);
        std::map<i32, u64> absorbed;
        for (const auto& pr : pre) ++absorbed[juf.find(pr.first)];
        for (const auto& ab : absorbed)
          if (ab.second >= 2) jf.nodes.push_back({jf.batches, ab.second});
        jf.batch_first.push_back(jev[e0]);
        // Instantane de partition (canonique : plus petite facette du bloc).
        std::map<FacetKey, FacetKey> part;
        std::map<i32, FacetKey> canon;
        for (const auto& kv : jid) {
          const i32 root = juf.find(kv.second);
          if (canon.find(root) == canon.end()) canon.emplace(root, kv.first);
        }
        for (const auto& kv : jid) part[kv.first] = canon[juf.find(kv.second)];
        jf.parts.push_back(part);
        ++jf.batches;
        e0 = e1;
      }

      // ---- Comparaison.
      if (sr.batches != jf.batches) {
        fail("nombre de lots", K);
        continue;
      }
      if (sparts.size() != jf.parts.size() || sparts != jf.parts) {
        fail("partition apres lot", K);
        continue;
      }
      std::vector<std::pair<u64, u64>> snodes;
      for (const ForestNode& nd : sr.nodes) snodes.push_back({nd.batch, nd.absorbed});
      std::sort(snodes.begin(), snodes.end());
      std::vector<std::pair<u64, u64>> jnodes = jf.nodes;
      std::sort(jnodes.begin(), jnodes.end());
      if (snodes != jnodes) {
        fail("nœuds (lot, absorbees)", K);
        continue;
      }
      for (const ForestNode& nd : sr.nodes)
        if (nd.absorbed >= 3) ++plateaus_multi;

      // Fixtures gravees.
      if (ci == 0 && K == 1 && !mutant) {
        if (sr.batches < 1 || sr.nodes.size() != 1 || sr.nodes[0].absorbed != 3) {
          std::fprintf(stderr, "FIXTURE colineaire3 : nœud ternaire attendu\n");
          return 3;
        }
        collinear_checked = true;
      }
      if (ci == 1 && K == 3 && !mutant) {
        // Le tetraedre (q4) et l'arete (q2, deux interieurs) partagent le
        // niveau 14900 : ils doivent vivre dans le MEME lot.
        bool same_batch = false;
        for (size_t e = 0; e + 1 < sevents.size(); ++e)
          for (size_t e2 = e + 1; e2 < sevents.size(); ++e2)
            if (sevents[e].q != sevents[e2].q &&
                same_exact_level(sevents[e].level, sevents[e2].level))
              same_batch = true;
        if (!same_batch) {
          std::fprintf(stderr, "FIXTURE tie q4/q2 : niveau partage attendu\n");
          return 3;
        }
        tie_checked = true;
      }
      if (ci == 2 && !mutant) {
        // square_cospherical_K2_plateau (audit bloquant § 1) : K=1, un
        // nœud quaternaire au niveau 50 (les quatre cotes) puis les
        // diagonales sans fusion ; K=2, UN nœud a SIX facettes-aretes au
        // niveau 100 (les quatre triangles rectangles du plateau) ; K=3,
        // un nœud a quatre facettes-triangles.
        const u64 want_nodes = 1;
        const u64 want_absorbed = (K == 1) ? 4 : (K == 2) ? 6 : (K == 3) ? 4 : 0;
        if (K >= 1 && K <= 3) {
          if (sr.nodes.size() != want_nodes ||
              sr.nodes[0].absorbed != want_absorbed) {
            std::fprintf(stderr,
                         "FIXTURE carre : K=%d nœuds=%zu absorbees=%llu "
                         "(attendus 1/%llu)\n",
                         K, sr.nodes.size(),
                         sr.nodes.empty()
                             ? 0ull
                             : (unsigned long long)sr.nodes[0].absorbed,
                         (unsigned long long)want_absorbed);
            return 3;
          }
          if (K == 2) square_checked = true;
        }
      }
    }
  }

  if (mhgp4_oracle::overflow_seen()) {
    std::fprintf(stderr, "REFUS numeric_failure : debordement obigint\n");
    return 3;
  }
  std::printf(
      "forest_selftest : %llu evenements, %llu fusions, "
      "plateaux_multi=%llu, desaccords=%d\n",
      (unsigned long long)total_events, (unsigned long long)total_fusions,
      (unsigned long long)plateaus_multi, g_fail);
  if (mutant) {
    if (g_fail > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (g_fail > 0) return 1;
  if (!collinear_checked || !tie_checked || !square_checked ||
      plateaus_multi == 0 || total_fusions == 0) {
    std::fprintf(stderr, "PLANCHER : fixtures ou fusions manquantes\n");
    return 3;
  }
  return 0;
}
