// MorseHGP3D v5 — JUGE INDEPENDANT DE LA FORET (regime oracle T2, n <= 14).
//
// SUJET : `run_pipeline` (index → WSPD → lanes → RLE → prefiltre → census →
// plateaux → fold par K), observe par `on_forest` : evenements du K, puis
// `facet_keys`, `final_canon_fid`, `deltas`, `batch_levels`, compteurs.
//
// JUGE (code neuf, primitives LOCALES — jamais p3_*, BallKey::power ni
// same_exact_level de la production dans une decision ; arithmetique de
// decision en mhgp5_oracle::OBig<12>, debordement = drapeau collant => code 3) :
//
//  1. CANDIDATES : toutes les paires, tous les triplets non colineaires et
//     tous les quadruplets non coplanaires du nuage, avec centre rationnel
//     cnum/cden et test « centre dans l'enveloppe FERMEE » :
//       paire (p,q)          : c = (p+q)/2                        cden = 2 ;
//       triangle (p,q,r)     : e1 = q−p, e2 = r−p, n = e1×e2 ≠ 0,
//                              w = (|e1|² e2 − |e2|² e1) × n,
//                              c = p + w / (2|n|²)   (re-derivation : w·e1 =
//                              |e1|²|n|², w·e2 = |e2|²|n|², w ⊥ n, donc
//                              (c−p)·e_i = |e_i|²/2 — les deux equidistances
//                              dans le plan) ; barycentriques
//                              α·2|n|⁴ = (w×e2)·n, β·2|n|⁴ = (e1×w)·n,
//                              ferme ssi α, β >= 0 et α+β <= 1 ;
//       tetraedre (p,q,r,s)  : e1,e2,e3, det = e1·(e2×e3) ≠ 0,
//                              v = |e1|²(e2×e3) + |e2|²(e3×e1) + |e3|²(e1×e2),
//                              c = p + v / (2 det)   ((c−p)·e_i = |e_i|²/2
//                              par orthogonalite des produits mixtes) ;
//                              λ1·2det² = v·(e2×e3), λ2·2det² = e1·(v×e3),
//                              λ3·2det² = e1·(e2×v), ferme ssi λ_i >= 0 et
//                              λ1+λ2+λ3 <= 1.
//     Pour chaque candidate : R² = |s·cden − cnum|² / cden² (s = premier point
//     de support), cote de chaque point du nuage (|x·cden − cnum|² <=> R²·1),
//     masques « dans-ou-sur » et « strictement dedans ». Les candidates sont
//     TRIEES par R² (produits croises OBig) et regroupees en CLASSES d'egalite.
//  2. MINIBOULE d'un sous-ensemble σ (Fait 12 du manuscrit : la miniboule est
//     la boule circonscrite d'un support de 2 a 4 points dont le centre est
//     dans l'enveloppe ; toute candidate contenant σ a un rayon >= ρ(σ)) : la
//     PREMIERE candidate de l'ordre trie dont le support ⊆ σ et σ ⊆
//     dans-ou-sur. Memoisee par masque.
//  3. GABRIEL (Def. 28 pure, plateaux compris) : aucun point de X∖σ
//     STRICTEMENT dans la miniboule ; un point externe SUR la sphere est
//     permis — c'est exactement σ = I_B ∪ T avec c ∈ conv(T), traite comme le
//     sujet (§ 5.3bis de la v4), sans jamais enumerer les T.
//  4. ROLES par rayons de naissance INDEPENDANTS : la facette σ∖{u} est
//     ACTIVE ssi classe(miniboule(σ∖{u})) < classe(σ) (facette-point : ρ = 0,
//     active) ; sinon elle nait AU niveau (attachement). Le juge n'utilise ni
//     active_mask ni « c ∉ conv(T∖{v}) ».
//  5. K-GRAPHE du manuscrit (Def. 29, cliques COMPLETES sur les facettes de
//     chaque K-simplexe de Gabriel) et KRUSKAL A LOTS (une classe de R² = un
//     lot ; racines gelees avant le lot, unions ensemble, deltas par racine
//     post-lot : parents = canoniques pre-lot des facettes actives ∨
//     preexistantes, nees = attachements non preexistants ; canonique = plus
//     petite FacetKey).
//  6. K = 1 ≡ SINGLE-LINKAGE : Kruskal sur TOUTES les paires (D² en i64, sans
//     miniboule), partition par niveau, comparee a la partition du sujet
//     REJOUEE depuis ses deltas (parents → output, nees), a chaque lot.
//
// COMPARAISONS par K : multiensemble des evenements (σ tries en PointId),
// niveau de chaque evenement (croise OBig), active_mask contre les roles du
// juge, ensemble des facettes, nombre de lots et niveaux de lot (croise OBig),
// partition finale (blocs) et canonique = min du bloc, deltas (batch, output,
// parents, nees — SANS le champ level), fusions = facettes − blocs, noeuds et
// attachements nes, coherence du rejeu des deltas avec la partition finale.
//
// FIXTURES gravees (coordonnees exactes, PointId NON monotones) :
//   colineaire3, tie q4/q2 a representants differents (v4), carre cocyclique
//   (110,100,100),(100,110,100),(90,100,100),(100,90,100) [K=2 : 4 parents +
//   2 nees ; K=3 : naissance 0 parent + 4 nees], q2_one_interior_attachment
//   {(0,0,0),(4,0,0),(2,1,0)} [2 parents, jamais 3], croissance unaire
//   {(8,10,10),(12,10,10),(10,11,10),(10,13,10)} [lot 13/4 : 3+2 ; lot 4 : 1+1],
//   sphere_q4_seule (n=12, smax=11 : tetraedre regulier + 5e point de coquille
//   non antipodal + 7 interieurs — la boule n'est produite QUE par la lane q4,
//   ses evenements K=10 meurent sous genfilter-nonstrict), pentagone
//   cocirculaire (cinq points sur le cercle R²=25 + deux interieurs hors plan :
//   plusieurs triangles aigus emettent la MEME boule, le RLE est exerce).
// Largeurs (u16, |Δ| < 2^16) : formation en i128 (cnum < 2^88, cden < 2^71),
// decisions en OBig<12> (cotes < 2^180, enveloppes < 2^141, croises < 2^334).
// Codes : 0 accord ; 1 desaccord du juge ; 2 refus avant calcul (argument,
// famille, mutant inconnu, n > 14, statut du pipeline) ; 3 plancher /
// invariant / debordement ; 4 mutant tue (desaccord, fixture ou refus du
// pipeline sous --inject).
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../oracle/obig.hpp"
#include "../src/cloud/families.hpp"
#include "../src/pipeline/run.hpp"

namespace {

using namespace mhgp5;
using OB = mhgp5_oracle::OBig<12>;

OB ob(i128 v) { return OB::from_i128(v); }

// ---- primitives locales du juge -------------------------------------------

struct JV {
  i128 x, y, z;
};
JV jv(const P3& p) { return JV{(i128)p.x, (i128)p.y, (i128)p.z}; }
JV jsub(const JV& a, const JV& b) { return JV{a.x - b.x, a.y - b.y, a.z - b.z}; }
JV jadd(const JV& a, const JV& b) { return JV{a.x + b.x, a.y + b.y, a.z + b.z}; }
JV jscale(i128 s, const JV& a) { return JV{s * a.x, s * a.y, s * a.z}; }
JV jneg(const JV& a) { return JV{-a.x, -a.y, -a.z}; }
i128 jdot(const JV& a, const JV& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
JV jcross(const JV& a, const JV& b) {
  return JV{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
i128 jn2(const JV& a) { return jdot(a, a); }
bool jzero(const JV& a) { return a.x == 0 && a.y == 0 && a.z == 0; }

struct OV {
  OB x, y, z;
};
OV ov(const JV& a) { return OV{ob(a.x), ob(a.y), ob(a.z)}; }
OB odot(const OV& a, const OV& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
OV ocross(const OV& a, const OV& b) {
  return OV{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// ---- candidates -------------------------------------------------------------

struct Cand {
  i128 cnum[3] = {0, 0, 0};
  i128 cden = 0;  // > 0
  u32 sup_mask = 0;
  int ref = 0;  // premier point de support (sur la sphere)
  u32 in_or_on = 0, strict_in = 0;
  OB rnum, rden;  // R² = rnum / rden
  int cls = -1;   // classe d'egalite de R² (croissante)
};

void set_center(Cand* c, const JV& p, const JV& num, i128 den) {
  c->cden = den;
  const JV pc = jadd(jscale(den, p), num);
  c->cnum[0] = pc.x;
  c->cnum[1] = pc.y;
  c->cnum[2] = pc.z;
}

bool form_pair(const P3& p, const P3& q, Cand* c) {
  const JV s = jadd(jv(p), jv(q));
  c->cden = 2;
  c->cnum[0] = s.x;
  c->cnum[1] = s.y;
  c->cnum[2] = s.z;
  return true;
}

// Triangle : centre circonscrit dans le plan, valide ssi c ∈ triangle FERME.
bool form_triangle(const P3& p, const P3& q, const P3& r, Cand* c) {
  const JV P = jv(p), e1 = jsub(jv(q), P), e2 = jsub(jv(r), P);
  const JV n = jcross(e1, e2);
  if (jzero(n)) return false;
  const i128 nn = jn2(n);
  const JV w = jcross(jsub(jscale(jn2(e1), e2), jscale(jn2(e2), e1)), n);
  const OV W = ov(w), E1 = ov(e1), E2 = ov(e2), N = ov(n);
  const OB alpha = odot(ocross(W, E2), N);
  const OB beta = odot(ocross(E1, W), N);
  if (alpha.sign() < 0 || beta.sign() < 0) return false;
  const OB two_nn2 = ob(2) * ob(nn) * ob(nn);
  if (cmp(alpha + beta, two_nn2) > 0) return false;
  set_center(c, P, w, 2 * nn);
  return true;
}

// Tetraedre : centre circonscrit, valide ssi c ∈ tetraedre FERME.
bool form_tetra(const P3& p, const P3& q, const P3& r, const P3& s, Cand* c) {
  const JV P = jv(p), e1 = jsub(jv(q), P), e2 = jsub(jv(r), P), e3 = jsub(jv(s), P);
  const i128 det = jdot(e1, jcross(e2, e3));
  if (det == 0) return false;
  const JV v = jadd(jadd(jscale(jn2(e1), jcross(e2, e3)), jscale(jn2(e2), jcross(e3, e1))),
                    jscale(jn2(e3), jcross(e1, e2)));
  const OV V = ov(v), E1 = ov(e1), E2 = ov(e2), E3 = ov(e3);
  const OB l1 = odot(V, ocross(E2, E3));
  const OB l2 = odot(E1, ocross(V, E3));
  const OB l3 = odot(E1, ocross(E2, V));
  if (l1.sign() < 0 || l2.sign() < 0 || l3.sign() < 0) return false;
  const OB two_det2 = ob(2) * ob(det) * ob(det);
  if (cmp(l1 + l2 + l3, two_det2) > 0) return false;
  if (det > 0) set_center(c, P, v, 2 * det);
  else set_center(c, P, jneg(v), -2 * det);
  return true;
}

OB dist2_scaled(const Cand& c, const P3& x) {
  const i128 xs[3] = {x.x, x.y, x.z};
  OB acc = ob(0);
  for (int i = 0; i < 3; ++i) {
    const OB d = ob(xs[i]) * ob(c.cden) - ob(c.cnum[i]);
    acc += d * d;
  }
  return acc;
}

// Ordre exact des R² : rnum_a·rden_b <=> rnum_b·rden_a.
int cmp_r2(const Cand& a, const Cand& b) { return cmp(a.rnum * b.rden, b.rnum * a.rden); }

// ---- le juge d'un nuage ------------------------------------------------------

struct JDelta {
  u64 batch = 0;
  FacetKey output;
  std::vector<FacetKey> parents, born;
};
bool jdelta_less(const JDelta& a, const JDelta& b) {
  if (a.batch != b.batch) return a.batch < b.batch;
  if (!(a.output == b.output)) return a.output < b.output;
  if (a.parents != b.parents) return a.parents < b.parents;
  return a.born < b.born;
}
bool jdelta_eq(const JDelta& a, const JDelta& b) {
  return a.batch == b.batch && a.output == b.output && a.parents == b.parents && a.born == b.born;
}

struct JudgeK {
  std::vector<std::vector<PointId>> events;                  // σ tries, tries
  std::map<std::vector<PointId>, std::pair<u32, int>> info;  // σ -> (masque, classe)
  std::vector<FacetKey> facets;                              // triees
  std::vector<std::vector<FacetKey>> blocks;                 // triees
  std::vector<int> batch_cls;                                // classe de chaque lot
  std::vector<JDelta> deltas;
  u64 born = 0, nodes = 0;
};

struct JudgeCloud {
  int m = 0;
  std::vector<P3> pts;
  std::vector<PointId> ids;
  std::map<PointId, int> index_of_id;
  std::vector<Cand> cands;
  std::vector<int> order;  // indices de candidates, R² croissant
  std::vector<int> memo;   // masque -> candidate de la miniboule (-2 = inconnu)
  std::vector<int> cls_rep;  // classe -> une candidate representante

  bool build() {
    index_of_id.clear();
    for (int i = 0; i < m; ++i) {
      if (!p3_in_profile(pts[(size_t)i])) return false;
      index_of_id[ids[(size_t)i]] = i;
      for (int j = 0; j < i; ++j)
        if (pts[(size_t)i] == pts[(size_t)j]) return false;
    }
    if ((int)index_of_id.size() != m) return false;
    cands.clear();
    for (int a = 0; a < m; ++a)
      for (int b = a + 1; b < m; ++b) {
        Cand c;
        if (form_pair(pts[(size_t)a], pts[(size_t)b], &c)) {
          c.sup_mask = (1u << a) | (1u << b);
          c.ref = a;
          cands.push_back(c);
        }
        for (int d = b + 1; d < m; ++d) {
          Cand t;
          if (form_triangle(pts[(size_t)a], pts[(size_t)b], pts[(size_t)d], &t)) {
            t.sup_mask = (1u << a) | (1u << b) | (1u << d);
            t.ref = a;
            cands.push_back(t);
          }
          for (int e = d + 1; e < m; ++e) {
            Cand q;
            if (form_tetra(pts[(size_t)a], pts[(size_t)b], pts[(size_t)d], pts[(size_t)e], &q)) {
              q.sup_mask = (1u << a) | (1u << b) | (1u << d) | (1u << e);
              q.ref = a;
              cands.push_back(q);
            }
          }
        }
      }
    for (Cand& c : cands) {
      c.rnum = dist2_scaled(c, pts[(size_t)c.ref]);
      c.rden = ob(c.cden) * ob(c.cden);
      c.in_or_on = 0;
      c.strict_in = 0;
      for (int u = 0; u < m; ++u) {
        const int side = cmp(dist2_scaled(c, pts[(size_t)u]), c.rnum);
        if (side <= 0) c.in_or_on |= 1u << u;
        if (side < 0) c.strict_in |= 1u << u;
      }
      // Le support est sur la sphere par construction : verifie (invariant du juge).
      for (int u = 0; u < m; ++u)
        if ((c.sup_mask >> u) & 1u) {
          if (cmp(dist2_scaled(c, pts[(size_t)u]), c.rnum) != 0) return false;
        }
    }
    order.resize(cands.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = (int)i;
    std::stable_sort(order.begin(), order.end(),
                     [&](int x, int y) { return cmp_r2(cands[(size_t)x], cands[(size_t)y]) < 0; });
    cls_rep.clear();
    for (size_t i = 0; i < order.size(); ++i) {
      if (i == 0 || cmp_r2(cands[(size_t)order[i - 1]], cands[(size_t)order[i]]) != 0) cls_rep.push_back(order[i]);
      cands[(size_t)order[i]].cls = (int)cls_rep.size() - 1;
    }
    memo.assign((size_t)1 << m, -2);
    return true;
  }

  // Candidate de la miniboule de σ ; -1 pour |σ| < 2 ; -3 = aucune (impossible).
  int miniball(u32 mask) {
    if (__builtin_popcount(mask) < 2) return -1;
    int& r = memo[(size_t)mask];
    if (r != -2) return r;
    r = -3;
    for (const int ci : order) {
      const Cand& c = cands[(size_t)ci];
      if ((c.sup_mask & ~mask) != 0) continue;
      if ((mask & ~c.in_or_on) != 0) continue;
      r = ci;
      break;
    }
    return r;
  }
  int cls_of(u32 mask) {
    const int c = miniball(mask);
    return c < 0 ? -1 : cands[(size_t)c].cls;
  }
  bool gabriel(u32 mask, int* cand_out) {
    const int c = miniball(mask);
    *cand_out = c;
    if (c < 0) return false;
    return (cands[(size_t)c].strict_in & ~mask) == 0;
  }
  FacetKey key_of(u32 mask) const {
    FacetKey f;
    for (int u = 0; u < m; ++u)
      if ((mask >> u) & 1u) f.p[f.k++] = ids[(size_t)u];
    // Tri par insertion sur le prefixe (k <= 10) : evite le faux positif
    // -Warray-bounds de GCC 13 sur std::sort d'un sous-intervalle de tableau fixe.
    for (u8 t = 1; t < f.k; ++t) {
      const PointId v = f.p[t];
      u8 w = t;
      for (; w > 0 && f.p[w - 1] > v; --w) f.p[w] = f.p[w - 1];
      f.p[w] = v;
    }
    return f;
  }
  std::vector<PointId> ids_of(u32 mask) const {
    std::vector<PointId> v;
    for (int u = 0; u < m; ++u)
      if ((mask >> u) & 1u) v.push_back(ids[(size_t)u]);
    std::sort(v.begin(), v.end());
    return v;
  }
  u32 mask_of_ids(const PointId* p, int k, bool* ok) const {
    u32 mask = 0;
    for (int t = 0; t < k; ++t) {
      const auto it = index_of_id.find(p[t]);
      if (it == index_of_id.end()) { *ok = false; return 0; }
      mask |= 1u << it->second;
    }
    *ok = true;
    return mask;
  }
};

struct JUnionFind {
  std::vector<int> parent;
  std::vector<FacetKey> canon;
  int add(const FacetKey& f) {
    parent.push_back((int)parent.size());
    canon.push_back(f);
    return (int)parent.size() - 1;
  }
  int find(int v) {
    while (parent[(size_t)v] != v) v = parent[(size_t)v];
    return v;
  }
  bool unite(int a, int b) {
    const int ra = find(a), rb = find(b);
    if (ra == rb) return false;
    const FacetKey mn = std::min(canon[(size_t)ra], canon[(size_t)rb]);
    parent[(size_t)rb] = ra;
    canon[(size_t)ra] = mn;
    return true;
  }
};

// Le fold du juge pour un K : K-graphe a cliques completes, Kruskal a lots.
// Rend false sur une contradiction interne (invariant du juge).
bool judge_fold(JudgeCloud& jc, int K, const std::vector<std::pair<u32, int>>& gab, JudgeK* out) {
  (void)K;
  // gab : (masque, candidate) des σ de Gabriel de taille K+1, tries par classe.
  std::vector<std::pair<int, u32>> ev;  // (classe, masque)
  for (const auto& g : gab) ev.push_back({jc.cands[(size_t)g.second].cls, g.first});
  std::stable_sort(ev.begin(), ev.end());
  for (const auto& e : ev) {
    out->events.push_back(jc.ids_of(e.second));
    out->info[out->events.back()] = {e.second, e.first};
  }
  std::sort(out->events.begin(), out->events.end());
  JUnionFind uf;
  std::map<FacetKey, int> fid;
  size_t e0 = 0;
  u64 batch = 0;
  while (e0 < ev.size()) {
    size_t e1 = e0 + 1;
    while (e1 < ev.size() && ev[e1].first == ev[e0].first) ++e1;
    const int cls = ev[e0].first;
    struct Role {
      bool active = false, attach = false, existed = false;
      int id = -1;
    };
    std::map<FacetKey, Role> roles;
    std::vector<std::vector<FacetKey>> ev_facets;
    for (size_t e = e0; e < e1; ++e) {
      const u32 mask = ev[e].second;
      std::vector<FacetKey> fs;
      for (int u = 0; u < jc.m; ++u) {
        if (!((mask >> u) & 1u)) continue;
        const u32 fm = mask & ~(1u << u);
        const FacetKey f = jc.key_of(fm);
        fs.push_back(f);
        Role& ro = roles[f];
        const int fcls = jc.cls_of(fm);  // -1 : facette-point, rayon 0
        const bool active = fcls < cls;
        if (active) ro.active = true;
        else ro.attach = true;
      }
      ev_facets.push_back(fs);
    }
    for (auto& kv : roles) {
      Role& ro = kv.second;
      if (ro.active && ro.attach) return false;  // role incoherent au meme niveau
      const auto it = fid.find(kv.first);
      ro.existed = it != fid.end();
      if (ro.attach && ro.existed) return false;  // attachement deja vu : contradiction
      ro.id = ro.existed ? it->second : uf.add(kv.first);
      if (!ro.existed) fid.emplace(kv.first, ro.id);
      if (ro.attach && !ro.existed) ++out->born;
    }
    std::map<int, FacetKey> pre;  // racine pre-lot -> canonique pre-lot
    for (const auto& kv : roles)
      if (kv.second.active || kv.second.existed) {
        const int root = uf.find(kv.second.id);
        pre.emplace(root, uf.canon[(size_t)root]);
      }
    for (const auto& fs : ev_facets)
      for (size_t w = 1; w < fs.size(); ++w) uf.unite(roles[fs[0]].id, roles[fs[w]].id);
    std::map<int, JDelta> touched;
    for (const auto& pr : pre) touched[uf.find(pr.first)].parents.push_back(pr.second);
    for (const auto& kv : roles)
      if (kv.second.attach && !kv.second.existed) touched[uf.find(kv.second.id)].born.push_back(kv.first);
    for (auto& tc : touched) {
      JDelta& d = tc.second;
      std::sort(d.parents.begin(), d.parents.end());
      std::sort(d.born.begin(), d.born.end());
      if (d.parents.size() >= 2) ++out->nodes;
      if (d.parents.size() == 1 && d.born.empty()) continue;
      d.batch = batch;
      d.output = uf.canon[(size_t)uf.find(tc.first)];
      out->deltas.push_back(d);
    }
    out->batch_cls.push_back(cls);
    ++batch;
    e0 = e1;
  }
  for (const auto& kv : fid) out->facets.push_back(kv.first);
  std::map<int, std::vector<FacetKey>> blocks;
  for (const auto& kv : fid) blocks[uf.find(kv.second)].push_back(kv.first);
  for (auto& kv : blocks) {
    std::sort(kv.second.begin(), kv.second.end());
    if (!(uf.canon[(size_t)kv.first] == kv.second.front())) return false;  // canonique = min
    out->blocks.push_back(kv.second);
  }
  std::sort(out->blocks.begin(), out->blocks.end());
  std::sort(out->deltas.begin(), out->deltas.end(), jdelta_less);
  return true;
}

// ---- observation du sujet ------------------------------------------------------

struct SubjectK {
  std::vector<ForestEvent> events;
  ForestResult r;
};

OB level_num(const ExactLevel& l) { return OB::from_u64_words(l.num, 3); }
OB level_den(const ExactLevel& l) { return ob(l.den); }

// niveau du sujet == rnum/rden du juge ?
bool level_equals(const ExactLevel& l, const Cand& c) { return cmp(level_num(l) * c.rden, c.rnum * level_den(l)) == 0; }

std::vector<PointId> sigma_of(const ForestEvent& e) {
  std::vector<PointId> v;
  for (int t = 0; t < (int)e.q; ++t) v.push_back(e.support[t]);
  for (int t = 0; t < (int)e.d; ++t) v.push_back(e.interior[t]);
  std::sort(v.begin(), v.end());
  return v;
}

// Rejeu des deltas du sujet : partition apres chaque lot (canonique -> bloc).
// Rend false si un output n'est pas le minimum de son bloc.
bool replay_deltas(const ForestResult& r, std::vector<std::map<FacetKey, std::vector<FacetKey>>>* after) {
  std::map<FacetKey, std::vector<FacetKey>> blocks;
  size_t di = 0;
  for (u64 b = 0; b < r.batches; ++b) {
    for (; di < r.deltas.size() && r.deltas[di].batch == b; ++di) {
      const ComponentDelta& d = r.deltas[di];
      std::vector<FacetKey> members = d.born;
      for (const FacetKey& p : d.parents) {
        const auto it = blocks.find(p);
        if (it == blocks.end()) members.push_back(p);
        else {
          members.insert(members.end(), it->second.begin(), it->second.end());
          blocks.erase(it);
        }
      }
      std::sort(members.begin(), members.end());
      if (members.empty() || !(members.front() == d.output)) return false;
      blocks[d.output] = members;
    }
    after->push_back(blocks);
  }
  return di == r.deltas.size();
}

int g_fail = 0;      // desaccords du juge (code 1)
int g_fixture = 0;   // fixtures gravees / planchers locaux (code 3)
void fail(const char* cloud, int K, const char* what) {
  std::fprintf(stderr, "DESACCORD %s K=%d : %s\n", cloud, K, what);
  ++g_fail;
}
void fixture_fail(const char* cloud, const char* what) {
  std::fprintf(stderr, "FIXTURE %s : %s\n", cloud, what);
  ++g_fixture;
}

// K = 1 ≡ single-linkage : Kruskal sur toutes les paires (D² i64), compare a
// la partition rejouee du sujet a chaque lot.
void judge_single_linkage(const JudgeCloud& jc, const SubjectK& s, const char* name) {
  const int m = jc.m;
  struct Pair {
    i64 d2;
    int a, b;
  };
  std::vector<Pair> pairs;
  for (int a = 0; a < m; ++a)
    for (int b = a + 1; b < m; ++b) {
      const JV d = jsub(jv(jc.pts[(size_t)a]), jv(jc.pts[(size_t)b]));
      pairs.push_back({(i64)jn2(d), a, b});
    }
  std::sort(pairs.begin(), pairs.end(), [](const Pair& x, const Pair& y) {
    if (x.d2 != y.d2) return x.d2 < y.d2;
    return x.a != y.a ? x.a < y.a : x.b < y.b;
  });
  std::vector<int> parent(m);
  for (int i = 0; i < m; ++i) parent[(size_t)i] = i;
  const auto find = [&](int v) {
    while (parent[(size_t)v] != v) v = parent[(size_t)v];
    return v;
  };
  // Niveaux de fusion SL (D²) et partition apres chaque groupe de D² egal.
  std::vector<i64> sl_d2;
  std::vector<std::vector<std::vector<FacetKey>>> sl_parts;
  std::vector<i64> merge_d2;
  const auto snapshot = [&]() {
    std::map<int, std::vector<FacetKey>> bl;
    for (int u = 0; u < m; ++u) {
      FacetKey f;
      f.k = 1;
      f.p[0] = jc.ids[(size_t)u];
      bl[find(u)].push_back(f);
    }
    std::vector<std::vector<FacetKey>> out;
    for (auto& kv : bl) {
      std::sort(kv.second.begin(), kv.second.end());
      out.push_back(kv.second);
    }
    std::sort(out.begin(), out.end());
    return out;
  };
  for (size_t i = 0; i < pairs.size();) {
    size_t j = i;
    bool merged = false;
    while (j < pairs.size() && pairs[j].d2 == pairs[i].d2) {
      const int ra = find(pairs[j].a), rb = find(pairs[j].b);
      if (ra != rb) {
        parent[(size_t)rb] = ra;
        merged = true;
      }
      ++j;
    }
    sl_d2.push_back(pairs[i].d2);
    sl_parts.push_back(snapshot());
    if (merged) merge_d2.push_back(pairs[i].d2);
    i = j;
  }
  std::vector<std::map<FacetKey, std::vector<FacetKey>>> after;
  if (!replay_deltas(s.r, &after)) {
    fail(name, 1, "rejeu des deltas impossible (K=1)");
    return;
  }
  // Chaque niveau de fusion SL est un lot du sujet (num·4 == D²·den).
  for (const i64 d2 : merge_d2) {
    bool found = false;
    for (const ExactLevel& l : s.r.batch_levels)
      if (cmp(level_num(l) * ob(4), ob(d2) * level_den(l)) == 0) found = true;
    if (!found) {
      fail(name, 1, "niveau de fusion single-linkage absent des lots");
      return;
    }
  }
  // A chaque lot du sujet, la partition rejouee (+ singletons non touches)
  // egale la partition SL au niveau du lot (toutes les paires D² <= 4·niveau).
  for (size_t b = 0; b < s.r.batch_levels.size() && b < after.size(); ++b) {
    const ExactLevel& l = s.r.batch_levels[b];
    int last = -1;
    for (size_t g = 0; g < sl_d2.size(); ++g)
      if (cmp(ob(sl_d2[g]) * level_den(l), level_num(l) * ob(4)) <= 0) last = (int)g;
    std::vector<std::vector<FacetKey>> sl;
    if (last < 0) {
      for (int u = 0; u < m; ++u) {
        FacetKey f;
        f.k = 1;
        f.p[0] = jc.ids[(size_t)u];
        sl.push_back({f});
      }
      std::sort(sl.begin(), sl.end());
    } else {
      sl = sl_parts[(size_t)last];
    }
    std::vector<std::vector<FacetKey>> subj;
    std::set<FacetKey> covered;
    for (const auto& kv : after[b]) {
      subj.push_back(kv.second);
      for (const FacetKey& f : kv.second) covered.insert(f);
    }
    for (int u = 0; u < m; ++u) {
      FacetKey f;
      f.k = 1;
      f.p[0] = jc.ids[(size_t)u];
      if (!covered.count(f)) subj.push_back({f});
    }
    std::sort(subj.begin(), subj.end());
    if (subj != sl) {
      fail(name, 1, "partition K=1 differente du single-linkage au niveau du lot");
      return;
    }
  }
}

// ---- comparaison sujet / juge pour un K ------------------------------------------

void compare_k(JudgeCloud& jc, int K, const JudgeK& j, const SubjectK& s, const char* name) {
  const ForestResult& r = s.r;
  // Evenements : multiensemble des σ.
  std::vector<std::vector<PointId>> se;
  for (const ForestEvent& e : s.events) se.push_back(sigma_of(e));
  std::sort(se.begin(), se.end());
  if (se != j.events) {
    fail(name, K, "ensemble des evenements (simplexes de Gabriel)");
    return;
  }
  // Niveau et roles de chaque evenement.
  for (const ForestEvent& e : s.events) {
    const auto it = j.info.find(sigma_of(e));
    if (it == j.info.end()) { fail(name, K, "evenement inconnu du juge"); return; }
    const u32 mask = it->second.first;
    const int cls = it->second.second;
    const int cand = jc.miniball(mask);
    if (cand < 0 || !level_equals(e.level, jc.cands[(size_t)cand])) { fail(name, K, "niveau d'un evenement"); return; }
    bool ok = true;
    for (int t = 0; t < (int)e.q && ok; ++t) {
      const auto iu = jc.index_of_id.find(e.support[t]);
      ok = iu != jc.index_of_id.end();
      if (!ok) break;
      const bool active_j = jc.cls_of(mask & ~(1u << iu->second)) < cls;
      const bool active_s = ((e.active_mask >> t) & 1u) != 0;
      ok = active_j == active_s;
    }
    for (int t = 0; t < (int)e.d && ok; ++t) {
      const auto iu = jc.index_of_id.find(e.interior[t]);
      ok = iu != jc.index_of_id.end() && jc.cls_of(mask & ~(1u << iu->second)) == cls;
    }
    if (!ok) { fail(name, K, "roles des facettes (active_mask / interieurs)"); return; }
  }
  // Facettes.
  if (r.facet_keys != j.facets) { fail(name, K, "ensemble des facettes"); return; }
  if (r.facets != j.facets.size()) { fail(name, K, "compteur de facettes"); return; }
  // Lots et niveaux de lots.
  if (r.batches != j.batch_cls.size() || r.batch_levels.size() != j.batch_cls.size()) {
    fail(name, K, "nombre de lots");
    return;
  }
  for (size_t b = 0; b < j.batch_cls.size(); ++b)
    if (!level_equals(r.batch_levels[b], jc.cands[(size_t)jc.cls_rep[(size_t)j.batch_cls[b]]])) {
      fail(name, K, "niveau de lot");
      return;
    }
  // Partition finale : blocs et canonique = plus petit fid du bloc.
  if (r.final_canon_fid.size() != r.facet_keys.size()) { fail(name, K, "taille de final_canon_fid"); return; }
  std::map<u32, std::vector<FacetKey>> sb;
  for (size_t fid = 0; fid < r.final_canon_fid.size(); ++fid) {
    const u32 c = r.final_canon_fid[fid];
    if (c > fid) { fail(name, K, "canonique superieur au fid"); return; }
    sb[c].push_back(r.facet_keys[fid]);
  }
  std::vector<std::vector<FacetKey>> sblocks;
  for (auto& kv : sb) {
    std::sort(kv.second.begin(), kv.second.end());
    if (!(kv.second.front() == r.facet_keys[kv.first])) { fail(name, K, "canonique n'est pas le min du bloc"); return; }
    sblocks.push_back(kv.second);
  }
  std::sort(sblocks.begin(), sblocks.end());
  if (sblocks != j.blocks) { fail(name, K, "partition finale"); return; }
  if (r.fusions != r.facets - sblocks.size()) { fail(name, K, "fusions != facettes - blocs"); return; }
  // Deltas sans le niveau.
  std::vector<JDelta> sd;
  for (const ComponentDelta& d : r.deltas) sd.push_back(JDelta{d.batch, d.output, d.parents, d.born});
  std::sort(sd.begin(), sd.end(), jdelta_less);
  bool same = sd.size() == j.deltas.size();
  for (size_t i = 0; same && i < sd.size(); ++i) same = jdelta_eq(sd[i], j.deltas[i]);
  if (!same) { fail(name, K, "deltas (naissances / croissances / fusions)"); return; }
  if (r.nodes != j.nodes) { fail(name, K, "noeuds (deltas a >= 2 parents)"); return; }
  if (r.new_attachments != j.born) { fail(name, K, "attachements nes"); return; }
  // Rejeu des deltas -> partition finale.
  std::vector<std::map<FacetKey, std::vector<FacetKey>>> after;
  if (!replay_deltas(r, &after) || after.empty()) { fail(name, K, "rejeu des deltas"); return; }
  std::vector<std::vector<FacetKey>> rb;
  for (const auto& kv : after.back()) rb.push_back(kv.second);
  std::sort(rb.begin(), rb.end());
  if (rb != j.blocks) { fail(name, K, "rejeu des deltas != partition finale"); return; }
}

// ---- nuages ----------------------------------------------------------------------

enum class Fix { kNone, kCollinear3, kTie, kSquare, kAttach, kGrowth, kQ4Only, kCocircular };

struct Cloud {
  std::string name;
  std::vector<InputPoint> in;
  Fix fix = Fix::kNone;
  u64 smax = 11;
};

Cloud fixture(const char* name, Fix fix, std::initializer_list<std::pair<PointId, P3>> pts) {
  Cloud c;
  c.name = name;
  c.fix = fix;
  for (const auto& p : pts) c.in.push_back(InputPoint{p.first, p.second});
  return c;
}

u32 scrambled_id(u32 i) { return (i + 1u) * 0x9E3779B9u ^ 0x5A5A5A5Au; }

FacetKey key2(PointId a, PointId b) {
  FacetKey f;
  f.k = 2;
  f.p[0] = std::min(a, b);
  f.p[1] = std::max(a, b);
  return f;
}

bool has_delta(const std::vector<ComponentDelta>& ds, size_t parents, size_t born) {
  for (const ComponentDelta& d : ds)
    if (d.parents.size() == parents && d.born.size() == born) return true;
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  std::string families = "uniform,eight_clusters,terrain";
  int n = 12;
  long long seed = 3;
  std::string inject;
  u64 min_events = 1, min_fusions = 1, min_batches = 1;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--families=", 0) == 0) families = arg.substr(11);
    else if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--seed=", 0) == 0) seed = std::atoll(arg.c_str() + 7);
    else if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else if (arg.rfind("--min-events=", 0) == 0) min_events = (u64)std::atoll(arg.c_str() + 13);
    else if (arg.rfind("--min-fusions=", 0) == 0) min_fusions = (u64)std::atoll(arg.c_str() + 14);
    else if (arg.rfind("--min-batches=", 0) == 0) min_batches = (u64)std::atoll(arg.c_str() + 14);
    else return 2;
  }
  if (n < 2 || n > 14) {
    std::fprintf(stderr, "REFUS : n=%d hors du regime oracle (2..14)\n", n);
    return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) {
    std::fprintf(stderr, "REFUS : mutant inconnu %s\n", inject.c_str());
    return 2;
  }
  const bool mutant = !inject.empty();

  std::vector<Cloud> clouds;
  clouds.push_back(fixture("colineaire3", Fix::kCollinear3, {{5, {0, 0, 0}}, {3, {2, 0, 0}}, {9, {4, 0, 0}}}));
  clouds.push_back(fixture("tie_q4_q2", Fix::kTie,
                           {{21, {100, 300, 300}}, {13, {300, 300, 300}}, {34, {200, 160, 400}}, {8, {200, 160, 200}},
                            {55, {2000, 2000, 2000}}, {5, {2244, 2008, 2000}}, {89, {2122, 2004, 2010}}, {3, {2122, 2004, 1990}}}));
  clouds.push_back(fixture("carre_cocyclique", Fix::kSquare,
                           {{12, {110, 100, 100}}, {7, {100, 110, 100}}, {30, {90, 100, 100}}, {4, {100, 90, 100}}}));
  clouds.push_back(fixture("q2_one_interior_attachment", Fix::kAttach, {{17, {0, 0, 0}}, {2, {4, 0, 0}}, {40, {2, 1, 0}}}));
  clouds.push_back(fixture("croissance_unaire", Fix::kGrowth,
                           {{9, {8, 10, 10}}, {6, {12, 10, 10}}, {25, {10, 11, 10}}, {1, {10, 13, 10}}}));
  clouds.push_back(fixture("sphere_q4_seule", Fix::kQ4Only,
                           {{31, {110, 110, 110}}, {14, {110, 90, 90}}, {27, {90, 110, 90}}, {2, {90, 90, 110}},
                            {19, {114, 110, 102}}, {8, {100, 100, 100}}, {23, {101, 100, 100}}, {11, {100, 101, 100}},
                            {35, {100, 100, 101}}, {6, {99, 100, 100}}, {29, {100, 99, 100}}, {16, {100, 100, 99}}}));
  clouds.push_back(fixture("pentagone_cocirculaire", Fix::kCocircular,
                           {{44, {105, 100, 100}}, {3, {103, 104, 100}}, {28, {96, 103, 100}}, {15, {97, 96, 100}},
                            {60, {100, 95, 100}}, {9, {100, 100, 103}}, {37, {100, 100, 97}}}));
  {
    size_t start = 0;
    while (start <= families.size()) {
      const size_t comma = families.find(',', start);
      const std::string item = families.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
      if (!item.empty()) {
        CloudFamily fam;
        if (!parse_cloud_family(item.c_str(), &fam)) {
          std::fprintf(stderr, "REFUS : famille inconnue %s\n", item.c_str());
          return 2;
        }
        const int coord = cloud_family_default_coord(fam, n);
        const std::vector<P3> pts = make_family_cloud(fam, n, coord, seed);
        Cloud c;
        c.name = item + "_n" + std::to_string(n);
        for (size_t i = 0; i < pts.size(); ++i) c.in.push_back(InputPoint{scrambled_id((u32)i), pts[i]});
        clouds.push_back(c);
      }
      if (comma == std::string::npos) break;
      start = comma + 1;
    }
  }

  mhgp5_oracle::clear_overflow();
  u64 total_events = 0, total_fusions = 0, total_batches = 0, total_multi = 0;
  bool fx_collinear = false, fx_tie = false, fx_square = false, fx_attach = false, fx_growth = false,
       fx_q4only = false, fx_cocircular = false;
  for (const Cloud& cl : clouds) {
    const char* name = cl.name.c_str();
    if (cl.in.size() > 14) {
      std::fprintf(stderr, "REFUS : %s a %zu points (> 14)\n", name, cl.in.size());
      return 2;
    }
    // ---- sujet.
    std::vector<SubjectK> subj(11);
    RunOptions o;
    o.threads = 1;
    o.smax = cl.smax;
    o.on_forest = [&](u64 K, const std::vector<ForestEvent>& events, const ForestResult& r) {
      subj[K].events = events;
      subj[K].r = r;
    };
    const RunResult rr = run_pipeline(cl.in, o);
    if (rr.status != PipelineStatus::kCompleteRegular) {
      std::fprintf(stderr, "REFUS du pipeline sur %s : %s\n", name, rr.message.c_str());
      if (mutant) {
        std::fprintf(stderr, "MUTANT TUE : %s change le statut\n", inject.c_str());
        return 4;
      }
      return status_exit_code(rr.status);
    }
    // ---- juge.
    JudgeCloud jc;
    jc.m = (int)cl.in.size();
    for (const InputPoint& p : cl.in) {
      jc.pts.push_back(p.position);
      jc.ids.push_back(p.id);
    }
    if (!jc.build()) {
      std::fprintf(stderr, "INVARIANT (juge) : construction des candidates sur %s\n", name);
      return 3;
    }
    const int kmax = std::min<int>(10, jc.m - 1);
    if ((int)rr.kmax_eff != std::min<int>(kmax, (int)cl.smax - 1)) {
      std::fprintf(stderr, "INVARIANT : kmax_eff=%llu inattendu sur %s\n", (unsigned long long)rr.kmax_eff, name);
      return 3;
    }
    std::vector<std::vector<std::pair<u32, int>>> gab(11);
    for (u32 mask = 1; mask < (1u << jc.m); ++mask) {
      const int k = __builtin_popcount(mask) - 1;
      if (k < 1 || k > (int)rr.kmax_eff) continue;
      int cand = -1;
      if (jc.gabriel(mask, &cand)) gab[(size_t)k].push_back({mask, cand});
      else if (cand == -3) {
        std::fprintf(stderr, "INVARIANT (juge) : aucune candidate ne contient un sous-ensemble\n");
        return 3;
      }
    }
    for (int K = 1; K <= (int)rr.kmax_eff; ++K) {
      JudgeK j;
      if (!judge_fold(jc, K, gab[(size_t)K], &j)) {
        std::fprintf(stderr, "INVARIANT (juge) : fold incoherent sur %s K=%d\n", name, K);
        return 3;
      }
      const SubjectK& s = subj[(size_t)K];
      compare_k(jc, K, j, s, name);
      if (K == 1) judge_single_linkage(jc, s, name);
      total_events += s.events.size();
      total_fusions += s.r.fusions;
      total_batches += s.r.batches;
      for (const ComponentDelta& d : s.r.deltas)
        if (d.parents.size() >= 3) ++total_multi;
      std::printf("%s K=%d : evenements=%zu facettes=%llu lots=%llu fusions=%llu deltas=%zu noeuds=%llu (juge : %zu evenements, %zu lots)\n",
                  name, K, s.events.size(), (unsigned long long)s.r.facets, (unsigned long long)s.r.batches,
                  (unsigned long long)s.r.fusions, s.r.deltas.size(), (unsigned long long)s.r.nodes, j.events.size(),
                  j.batch_cls.size());
    }
    // ---- fixtures gravees (attentes mathematiques explicites).
    const auto ids = [&](size_t i) { return cl.in[i].id; };
    switch (cl.fix) {
      case Fix::kCollinear3: {
        // K=1 : UN lot, UN delta a trois parents (jamais une chaine binaire).
        const ForestResult& r1 = subj[1].r;
        if (r1.batches != 1 || r1.deltas.size() != 1 || r1.deltas[0].parents.size() != 3 || !r1.deltas[0].born.empty())
          fixture_fail(name, "K=1 : un lot et un noeud ternaire attendus");
        else fx_collinear = true;
        break;
      }
      case Fix::kTie: {
        // K=3 : le tetraedre (q=4) et l'arete a deux interieurs (q=2) dans le
        // MEME lot, avec des representants de niveau DIFFERENTS.
        const SubjectK& s3 = subj[3];
        bool same_batch = false, repr_differ = false;
        for (const ForestEvent& e : s3.events)
          for (const ForestEvent& f : s3.events) {
            if (e.q != 4 || f.q != 2) continue;
            bool oke = false, okf = false;
            const u32 me = jc.mask_of_ids(e.support, (int)e.q, &oke);
            const u32 mf = jc.mask_of_ids(f.support, (int)f.q, &okf);
            if (!oke || !okf) continue;
            u32 mfi = mf;
            bool oki = false;
            mfi |= jc.mask_of_ids(f.interior, (int)f.d, &oki);
            if (!oki) continue;
            if (jc.cls_of(me) == jc.cls_of(mfi)) {
              same_batch = true;
              if (!(e.level == f.level)) repr_differ = true;  // inegalite de REPRESENTATION (le fait grave)
            }
          }
        if (!same_batch || !repr_differ || s3.r.batches == 0) fixture_fail(name, "K=3 : tie q4/q2 a representants differents attendu");
        else fx_tie = true;
        break;
      }
      case Fix::kSquare: {
        // K=1 : lot 50 -> un delta a 4 parents ; lot 100 (diagonales) : continuation.
        // K=2 : un delta, 4 parents (cotes), 2 nees (diagonales).
        // K=3 : NAISSANCE : un delta, 0 parent, 4 nees.
        const ForestResult &r1 = subj[1].r, &r2 = subj[2].r, &r3 = subj[3].r;
        bool ok = r1.batches == 2 && r1.deltas.size() == 1 && r1.deltas[0].parents.size() == 4 && r1.deltas[0].born.empty();
        ok = ok && r2.batches == 1 && r2.deltas.size() == 1 && r2.deltas[0].parents.size() == 4 && r2.deltas[0].born.size() == 2;
        if (ok) {
          const std::vector<FacetKey> want = {key2(ids(0), ids(2)), key2(ids(1), ids(3))};
          std::vector<FacetKey> got = r2.deltas[0].born;
          std::vector<FacetKey> w = want;
          std::sort(w.begin(), w.end());
          ok = got == w && r2.facets == 6 && subj[2].events.size() == 4;
        }
        ok = ok && r3.batches == 1 && r3.deltas.size() == 1 && r3.deltas[0].parents.empty() && r3.deltas[0].born.size() == 4 &&
             r3.facets == 4;
        if (!ok) fixture_fail(name, "carre : K=1 4 parents / K=2 4 parents + 2 diagonales nees / K=3 naissance 0+4");
        else fx_square = true;
        break;
      }
      case Fix::kAttach: {
        // K=2 : un delta a EXACTEMENT 2 parents ({a,z},{b,z}) et 1 nee ({a,b}).
        const ForestResult& r2 = subj[2].r;
        const bool ok = r2.deltas.size() == 1 && r2.deltas[0].parents.size() == 2 && r2.deltas[0].born.size() == 1 &&
                        r2.deltas[0].born[0] == key2(ids(0), ids(1)) && r2.new_attachments == 1 && r2.nodes == 1;
        if (!ok) fixture_fail(name, "K=2 : noeud a 2 enfants + facette {a,b} nee attendus (jamais 3)");
        else fx_attach = true;
        break;
      }
      case Fix::kGrowth: {
        // K=2 : lot 13/4 = multifusion (3 parents, 2 nees) ; lot 4 = croissance (1 parent, 1 nee {a,b}).
        const ForestResult& r2 = subj[2].r;
        bool ok = r2.deltas.size() == 2 && has_delta(r2.deltas, 3, 2) && has_delta(r2.deltas, 1, 1) && r2.nodes == 1;
        for (const ComponentDelta& d : r2.deltas)
          if (d.parents.size() == 1) ok = ok && d.born.size() == 1 && d.born[0] == key2(ids(0), ids(1));
        if (!ok) fixture_fail(name, "K=2 : fusion 3+2 puis croissance 1+1 ({a,b}) attendues");
        else fx_growth = true;
        break;
      }
      case Fix::kQ4Only: {
        // K=10 : la boule R²=300 (tetraedre regulier + 5e point de coquille +
        // 7 interieurs) porte au moins un evenement ; elle n'est produite que
        // par la lane q4 (aucune paire antipodale, aucun plan de triangle par
        // le centre) : sous genfilter-nonstrict les evenements K=10 disparaissent.
        const SubjectK& s10 = subj[10];
        bool has300 = false;
        for (const ForestEvent& e : s10.events)
          if (e.q == 4 && cmp(level_num(e.level), ob(300) * level_den(e.level)) == 0) has300 = true;
        if (!has300 || rr.kmax_eff != 10) fixture_fail(name, "K=10 : evenement q4 de niveau 300 attendu");
        else fx_q4only = true;
        break;
      }
      case Fix::kCocircular: {
        // Le RLE est exerce : plusieurs generateurs emettent la meme boule.
        if (!(rr.emitted > rr.expand.unique_balls)) fixture_fail(name, "RLE non exerce (aucun doublon de boule)");
        else fx_cocircular = true;
        break;
      }
      case Fix::kNone: break;
    }
  }

  if (mhgp5_oracle::overflow_seen()) {
    std::fprintf(stderr, "REFUS numeric_failure : debordement de l'arithmetique de l'oracle\n");
    return 3;
  }
  std::printf("forest_judge : nuages=%zu evenements=%llu fusions=%llu lots=%llu multifusions=%llu desaccords=%d fixtures_ko=%d\n",
              clouds.size(), (unsigned long long)total_events, (unsigned long long)total_fusions,
              (unsigned long long)total_batches, (unsigned long long)total_multi, g_fail, g_fixture);
  if (mutant) {
    if (g_fail > 0 || g_fixture > 0) {
      std::printf("MUTANT TUE : %s\n", inject.c_str());
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s non discrimine\n", inject.c_str());
    return 3;
  }
  if (g_fail > 0) return 1;
  if (g_fixture > 0) return 3;
  if (!fx_collinear || !fx_tie || !fx_square || !fx_attach || !fx_growth || !fx_q4only || !fx_cocircular) {
    std::fprintf(stderr, "PLANCHER : une fixture gravee n'a pas ete verifiee\n");
    return 3;
  }
  if (total_events < min_events || total_fusions < min_fusions || total_batches < min_batches || total_multi == 0) {
    std::fprintf(stderr, "PLANCHER : evenements=%llu (min %llu) fusions=%llu (min %llu) lots=%llu (min %llu) multifusions=%llu\n",
                 (unsigned long long)total_events, (unsigned long long)min_events, (unsigned long long)total_fusions,
                 (unsigned long long)min_fusions, (unsigned long long)total_batches, (unsigned long long)min_batches,
                 (unsigned long long)total_multi);
    return 3;
  }
  return 0;
}
