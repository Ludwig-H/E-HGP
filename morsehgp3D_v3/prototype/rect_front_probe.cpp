// MorseHGP3D v3 — PORTE DU FRONT DE RECTANGLES.
//
// Le sujet decide une lane pour un RECTANGLE de noeuds d'arbre, avec un budget
// BORNE d'evaluations par rectangle (raison de front `RESOURCE_CAP`). La
// recherche de temoins est AU MEILLEUR D'ABORD, ordonnee par `Lambda_max`
// decroissant — du noeud le plus interieur vers la frontiere : traiter la liste
// dans l'ordre de l'arbre epuise le budget sur la frontiere de la boule sans
// jamais atteindre le centre, et la fermeture est alors NULLE.
//
// Codes de sortie : 1 desaccord du juge, 2 campagne refusee AVANT calcul,
// 3 plancher ou invariant viole, 4 mutant tue.
#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <queue>
#include <string>
#include <vector>

#include "cloud_families.hpp"
#include "rect_front.hpp"

namespace {

using mhgp3v::RectBox;
using mhgp3v::RectFrontInject;
using mhgp3v::RectLane;
using mhgp3v::RectVerdict;

struct Node {
  RectBox box;
  int begin, end, left, right;
  double rad;
  double c[3];
};

struct Tree {
  std::vector<Node> nodes;
  std::vector<std::array<long long, 3>> pts;
};

int build(Tree* t, int b, int e, int leaf) {
  Node n{};
  n.begin = b; n.end = e; n.left = -1; n.right = -1;
  for (int i = 0; i < 3; ++i) { n.box.lo[i] = t->pts[b][i]; n.box.hi[i] = t->pts[b][i]; }
  for (int k = b; k < e; ++k)
    for (int i = 0; i < 3; ++i) {
      n.box.lo[i] = std::min(n.box.lo[i], t->pts[k][i]);
      n.box.hi[i] = std::max(n.box.hi[i], t->pts[k][i]);
    }
  double r2 = 0;
  for (int i = 0; i < 3; ++i) {
    n.c[i] = 0.5 * (double)(n.box.lo[i] + n.box.hi[i]);
    const double h = 0.5 * (double)(n.box.hi[i] - n.box.lo[i]);
    r2 += h * h;
  }
  n.rad = std::sqrt(r2);
  const int id = (int)t->nodes.size();
  t->nodes.push_back(n);
  if (e - b > leaf) {
    int ax = 0; long long best = -1;
    for (int i = 0; i < 3; ++i) {
      const long long w = t->nodes[id].box.hi[i] - t->nodes[id].box.lo[i];
      if (w > best) { best = w; ax = i; }
    }
    const long long mid = (t->nodes[id].box.lo[ax] + t->nodes[id].box.hi[ax]) / 2;
    auto it = std::partition(t->pts.begin() + b, t->pts.begin() + e,
                             [ax, mid](const std::array<long long, 3>& p) { return p[ax] <= mid; });
    int m = (int)(it - t->pts.begin());
    if (m == b || m == e) {
      m = (b + e) / 2;
      std::nth_element(t->pts.begin() + b, t->pts.begin() + m, t->pts.begin() + e,
                       [ax](const std::array<long long, 3>& x, const std::array<long long, 3>& y) {
                         return x[ax] < y[ax];
                       });
    }
    t->nodes[id].left = build(t, b, m, leaf);
    t->nodes[id].right = build(t, m, e, leaf);
  }
  return id;
}

struct Counters {
  long long rect_visited = 0, rect_closed = 0, rect_residual = 0, rect_capped = 0;
  long long rect_positive = 0, rect_keep_anchor = 0;
  long long core_tests = 0, core_closed = 0, core_judged = 0, core_disagree = 0;
  long long all_judged = 0, all_disagree = 0;
  long long evals = 0, mass_closed = 0, mass_residual = 0, mass_positive = 0, mass_keep_anchor = 0, w_high = 0;
  long long all_hits = 0, none_hits = 0, mixed_hits = 0;
};

// Trois issues, et non deux. La file de priorite porte aussi un MAJORANT du
// nombre de temoins : a tout instant `cred + somme des |C| encore en file`
// majore ce que le rectangle pourra jamais crediter, puisque tout point non
// encore classe est soit dans un noeud de la file, soit deja NONE.
//   cred        >= h  -> FERME   : aucune paire du rectangle n'est un support.
//   cred + pend <  h  -> POSITIF : TOUTE paire du rectangle est un support,
//                                  produite en bloc sans enumerer de temoins.
//   sinon             -> RESIDUEL.
enum class RectOutcome { kClosed, kPositive, kResidual };

const int kNeed[3] = {10, 9, 8};

// L'ISSUE POSITIVE N'EXISTE QU'EN q2, et l'audit `96be8e0` a raison de le
// refuter ailleurs : `cred + pend < h` prouve seulement que la paire n'est pas
// ELIMINEE par le certificat universel. En q3/q4 elle ne fabrique ni troisieme
// site affine independant, ni support bien centre, ni owner — un nuage
// colineaire en est le contre-exemple decisif. On la nomme donc `kKeepAnchor`
// hors q2, et elle n'annonce aucun support.
enum class RectOutcomeKind { kClosed, kPositiveQ2, kKeepAnchor, kResidual };

// FAST PATH DU COEUR COMMUN : une seule sphere entiere, un comptage borne, et
// la lane se ferme sans qu'aucun noeud ne soit classe. Le comptage s'arrete des
// que le seuil est atteint ; l'echec ne conclut RIEN et retombe sur la
// descente. Aucun faux temoin possible : le coeur est un MINORANT, tout point
// qu'il contient est interieur a toute boule admissible du rectangle.
bool g_verify_core = false;
bool g_verify_all = false;
unsigned long long g_rng = 0x243F6A8885A308D3ull;

bool core_closes(const Tree& t, int ia, int ib, RectLane lane, Counters* c,
                 RectFrontInject inject) {
  // LES TROIS LANES. La restriction a q2 reposait sur un FAUX JUGE — appeler
  // le classifieur AABB pour juger le coeur n'est pas un oracle independant,
  // puisque `MIXED` n'y refute rien. Coeur large `(d-2S)/2` en q2, coeur
  // etroit `(d-3S)/4` en q3/q4.
  const int mult = (lane == RectLane::kQ2) ? 2 : 3;
  const mhgp3v::RectCore core =
      mhgp3v::rect_common_core(t.nodes[ia].box, t.nodes[ib].box, mult, inject);
  if (!core.valid) return false;
  long long cnt = 0;
  int stack[128];
  int sp = 0;
  stack[sp++] = 0;
  while (sp > 0) {
    const Node& C = t.nodes[stack[--sp]];
    ++c->core_tests;
    if (mhgp3v::rect_core_misses_box(core, C.box)) continue;
    if (C.left >= 0) {
      if (sp + 2 <= 128) { stack[sp++] = C.left; stack[sp++] = C.right; }
      else return false;
      continue;
    }
    for (int k = C.begin; k < C.end; ++k) {
      const long long z[3] = {t.pts[k][0], t.pts[k][1], t.pts[k][2]};
      if (mhgp3v::rect_core_contains(core, z)) {
        // JUGE DU COEUR. Tout point du coeur doit satisfaire le predicat EXACT
        // du rectangle : le coeur est un MINORANT de la region ALL, jamais un
        // majorant. Le juge est une autre ecriture — boite degeneree {z} et
        // intervalle exact — et non la sphere qui vient de decider.
        if (g_verify_core) {
          // JUGE PAR TRIPLES REELS. Rappeler le classifieur AABB ne serait PAS
          // un oracle independant : son `MIXED` ne refute rien, `Hmin`,
          // `E2max` et `X2max` pouvant provenir de paires differentes. On
          // enumere donc des couples de points EXISTANTS de `A` et `B` et on
          // evalue le predicat ponctuel exact.
          const Node& AA = t.nodes[ia];
          const Node& BB = t.nodes[ib];
          for (int ai = AA.begin; ai < AA.end; ++ai)
            for (int bi = BB.begin; bi < BB.end; ++bi) {
              __int128 h = 0, e2 = 0, x2 = 0;
              for (int d = 0; d < 3; ++d) {
                const long long ea = z[d] - t.pts[ai][d];
                const long long tb = t.pts[bi][d] - z[d];
                h += (__int128)ea * tb; e2 += (__int128)ea * ea; x2 += (__int128)tb * tb;
              }
              bool ok = (h > 0);
              if (ok && lane == RectLane::kQ3) ok = (4 * h * h > e2 * x2);
              if (ok && lane == RectLane::kQ4) ok = (3 * h * h > e2 * x2);
              ++c->core_judged;
              if (!ok) ++c->core_disagree;
            }
        }
        if (++cnt >= kNeed[(int)lane]) { ++c->core_closed; return true; }
      }
    }
  }
  return false;
}

RectOutcome witness_outcome(const Tree& t, int ia, int ib, RectLane lane, long long budget,
                            RectFrontInject inject, Counters* c) {
  const Node& A = t.nodes[ia];
  const Node& B = t.nodes[ib];
  long long cred = 0;     // points certifies temoins (noeuds ALL)
  long long queued = 0;   // points des noeuds MIXED encore en file
  long long stuck = 0;    // points des feuilles MIXED, indecidables sans descendre
  std::priority_queue<std::pair<long long, int>> pq;

  // La classification se fait A L'INSERTION, jamais au depilage : la file est
  // ordonnee du plus interieur au plus exterieur, donc les noeuds NONE en
  // seraient depiles en DERNIER et le majorant ne baisserait jamais avant que
  // le budget ne soit epuise.
  auto admit = [&](int ic) -> bool {
    const Node& C = t.nodes[ic];
    const long long k = C.end - C.begin;
    ++c->evals;
    long long mx = 0;
    const RectVerdict v = mhgp3v::rect_classify(A.box, B.box, C.box, lane, &mx, inject);
    // JUGE DES VERDICTS `ALL`, par ECHANTILLONNAGE DE TRIPLES REELS. Un noeud
    // declare `ALL` affirme que TOUT `z` de `C` est temoin universel de TOUTE
    // paire de `A x B`. On tire donc des triples de points EXISTANTS et on
    // evalue le predicat ponctuel exact — H > 0 et c H^2 > E2 X2 — dans une
    // ecriture qui n'emprunte ni `Lambda`, ni le coeur central, ni les bornes
    // de boites.
    if (g_verify_all && v == RectVerdict::kAll) {
      for (int s = 0; s < 4; ++s) {
        const int ai = A.begin + (int)((g_rng = g_rng * 6364136223846793005ull + 1) >> 33) % (A.end - A.begin);
        const int bi = B.begin + (int)((g_rng = g_rng * 6364136223846793005ull + 1) >> 33) % (B.end - B.begin);
        const int zi = C.begin + (int)((g_rng = g_rng * 6364136223846793005ull + 1) >> 33) % (C.end - C.begin);
        __int128 h = 0, e2 = 0, x2 = 0;
        for (int d = 0; d < 3; ++d) {
          const long long ea = t.pts[zi][d] - t.pts[ai][d];
          const long long tb = t.pts[bi][d] - t.pts[zi][d];
          h += (__int128)ea * tb; e2 += (__int128)ea * ea; x2 += (__int128)tb * tb;
        }
        bool ok = (h > 0);
        if (ok && lane == RectLane::kQ3) ok = (4 * h * h > e2 * x2);
        if (ok && lane == RectLane::kQ4) ok = (3 * h * h > e2 * x2);
        ++c->all_judged;
        if (!ok) ++c->all_disagree;
      }
    }
    if (v == RectVerdict::kNone) { ++c->none_hits; return false; }
    if (v == RectVerdict::kAll) { ++c->all_hits; cred += k; return true; }
    ++c->mixed_hits;
    if (C.left < 0) { stuck += k; return false; }
    queued += k;
    pq.push({mx, ic});
    if ((long long)pq.size() > c->w_high) c->w_high = (long long)pq.size();
    return false;
  };

  // BUDGET EXACT. L'audit releve un off-by-one : une iteration pouvait demarrer
  // avec une unite restante puis classer DEUX enfants, si bien que `budget=24`
  // autorisait vingt-cinq classifications. Le budget compte desormais des
  // CLASSIFICATIONS, et la porte verifie `evals <= budget * rect_visites`.
  if (budget <= 0) return RectOutcome::kResidual;
  --budget;
  admit(0);
  if (cred >= kNeed[(int)lane]) return RectOutcome::kClosed;
  if (cred + queued + stuck < kNeed[(int)lane]) return RectOutcome::kPositive;
  while (!pq.empty() && budget >= 2) {
    const int ic = pq.top().second;
    pq.pop();
    const Node& C = t.nodes[ic];
    queued -= C.end - C.begin;
    budget -= 2;
    admit(C.left);
    admit(C.right);
    if (cred >= kNeed[(int)lane]) return RectOutcome::kClosed;
    if (cred + queued + stuck < kNeed[(int)lane]) return RectOutcome::kPositive;
  }
  if (!pq.empty()) ++c->rect_capped;
  return RectOutcome::kResidual;
}

// ARRET A « BIEN SEPARE », ET NON A « FERME ». C'est ce qui rend le cardinal du
// front `O(n)` par Callahan-Kosaraju : une recursion qui scinde jusqu'a ce que
// le certificat ferme descend jusqu'aux feuilles sur le residuel, et son front
// croit alors en `n^1,4`. Un rectangle bien separe qui n'a pas ferme sort
// TERMINAL et part a la source generative.
bool well_separated(const Tree& t, int ia, int ib, double s) {
  const Node& A = t.nodes[ia];
  const Node& B = t.nodes[ib];
  double d2 = 0;
  for (int i = 0; i < 3; ++i) { const double u = A.c[i] - B.c[i]; d2 += u * u; }
  return std::sqrt(d2) - A.rad - B.rad >= s * std::max(A.rad, B.rad);
}

void solve(const Tree& t, int ia, int ib, RectLane lane, long long budget,
           RectFrontInject inject, Counters* c, double stop_wsp, bool use_core = false) {
  const Node& A = t.nodes[ia];
  const Node& B = t.nodes[ib];
  const long long ka = A.end - A.begin, kb = B.end - B.begin;
  const long long mass = (ia == ib) ? ka * (ka - 1) / 2 : ka * kb;
  if (mass <= 0) return;
  ++c->rect_visited;
  if (ia == ib) {
    if (A.left < 0) { ++c->rect_residual; c->mass_residual += mass; return; }
    solve(t, A.left, A.left, lane, budget, inject, c, stop_wsp, use_core);
    solve(t, A.right, A.right, lane, budget, inject, c, stop_wsp, use_core);
    solve(t, A.left, A.right, lane, budget, inject, c, stop_wsp, use_core);
    return;
  }
  if (use_core && core_closes(t, ia, ib, lane, c, inject)) {
    ++c->rect_closed; c->mass_closed += mass; return;
  }
  const RectOutcome oc = witness_outcome(t, ia, ib, lane, budget, inject, c);
  if (oc == RectOutcome::kClosed) { ++c->rect_closed; c->mass_closed += mass; return; }
  if (oc == RectOutcome::kPositive) {
    // En q2 c'est un support certifie ; hors q2 c'est un KEEP_ANCHOR qui ne
    // conclut rien et part a la source. Les deux comptent separement.
    if (lane == RectLane::kQ2) { ++c->rect_positive; c->mass_positive += mass; }
    else { ++c->rect_keep_anchor; c->mass_keep_anchor += mass; }
    return;
  }
  if ((A.left < 0 && B.left < 0) ||
      (stop_wsp > 0.0 && well_separated(t, ia, ib, stop_wsp))) {
    ++c->rect_residual; c->mass_residual += mass; return;
  }
  if (A.left >= 0 && (B.left < 0 || A.rad >= B.rad)) {
    solve(t, A.left, ib, lane, budget, inject, c, stop_wsp, use_core);
    solve(t, A.right, ib, lane, budget, inject, c, stop_wsp, use_core);
  } else {
    solve(t, ia, B.left, lane, budget, inject, c, stop_wsp, use_core);
    solve(t, ia, B.right, lane, budget, inject, c, stop_wsp, use_core);
  }
}

// ---- Juge independant : min et max exhaustifs sur les points ENTIERS des trois
// boites. Representation deliberement differente : aucune separation par
// coordonnee, une seule somme brute.
void brute_interval(const RectBox& a, const RectBox& b, const RectBox& cbx,
                    long long* mn, long long* mx) {
  bool first = true;
  for (long long ax = a.lo[0]; ax <= a.hi[0]; ++ax)
  for (long long ay = a.lo[1]; ay <= a.hi[1]; ++ay)
  for (long long az = a.lo[2]; az <= a.hi[2]; ++az)
  for (long long bx = b.lo[0]; bx <= b.hi[0]; ++bx)
  for (long long by = b.lo[1]; by <= b.hi[1]; ++by)
  for (long long bz = b.lo[2]; bz <= b.hi[2]; ++bz)
  for (long long zx = cbx.lo[0]; zx <= cbx.hi[0]; ++zx)
  for (long long zy = cbx.lo[1]; zy <= cbx.hi[1]; ++zy)
  for (long long zz = cbx.lo[2]; zz <= cbx.hi[2]; ++zz) {
    const long long v = (zx - ax) * (bx - zx) + (zy - ay) * (by - zy) + (zz - az) * (bz - zz);
    if (first) { *mn = *mx = v; first = false; }
    else { *mn = std::min(*mn, v); *mx = std::max(*mx, v); }
  }
}

struct SelfTest { long long tests = 0, disagree_min = 0, disagree_max = 0; };

SelfTest run_selftest(int iters, RectFrontInject inject) {
  SelfTest st{};
  unsigned long long r = 0x9E3779B97F4A7C15ull;
  auto rnd = [&r](long long hi) {
    r ^= r << 13; r ^= r >> 7; r ^= r << 17;
    return (long long)(r % (unsigned long long)hi);
  };
  for (int it = 0; it < iters; ++it) {
    RectBox bx[3];
    for (int q = 0; q < 3; ++q)
      for (int i = 0; i < 3; ++i) {
        const long long lo = rnd(11) - 5;
        bx[q].lo[i] = lo; bx[q].hi[i] = lo + rnd(3);
      }
    long long mn = 0, mx = 0, emn = 0, emx = 0;
    mhgp3v::rect_h_interval(bx[0], bx[1], bx[2], &mn, &mx, inject);
    brute_interval(bx[0], bx[1], bx[2], &emn, &emx);
    ++st.tests;
    if (mn != emn) ++st.disagree_min;
    if (mx != emx) ++st.disagree_max;
  }
  return st;
}

// ---- FIXTURES GRAVEES, coordonnees exactes, exigees par l'audit `96be8e0`.
struct Fixture { const char* nom; RectBox a, b, c; long long mn, mx; };

int run_fixtures() {
  const Fixture fx[] = {
    // Le sommet est INTERIEUR : les extremites donnent zero, `z=1` donne un.
    {"sommet_interieur", {{0,0,0},{0,0,0}}, {{2,0,0},{2,0,0}}, {{0,0,0},{2,0,0}}, 0, 1},
    // Le sommet est HORS de C : l'ecretage est obligatoire, sinon surestimation.
    {"sommet_hors_boite", {{0,0,0},{0,0,0}}, {{2,0,0},{2,0,0}}, {{10,0,0},{10,0,0}}, -80, -80},
    // RESEAU ENTIER contre CONTINU : maximum entier nul, maximum continu 1/4.
    {"reseau_contre_continu", {{0,0,0},{0,0,0}}, {{1,0,0},{1,0,0}}, {{0,0,0},{1,0,0}}, 0, 0},
    // Milieu IMPAIR : (a+b)/2 = 3/2, les deux entiers voisins valent 2.
    {"milieu_impair", {{0,0,0},{0,0,0}}, {{3,0,0},{3,0,0}}, {{0,0,0},{3,0,0}}, 0, 2},
    // Extremes u16 : H tient dans i64 (3 x 65535^2 < 2^34). Le sommet continu
    // est en 32767,5 ; les deux entiers voisins donnent la MEME valeur, et le
    // maximum ENTIER 3 221 127 168 est strictement sous le maximum CONTINU
    // 3 221 127 168,75. C'est la fixture de l'ecart reseau/continu a l'extreme.
    {"extremes_u16", {{0,0,0},{0,0,0}}, {{65535,65535,65535},{65535,65535,65535}},
     {{32767,32767,32767},{32768,32768,32768}}, 3221127168LL, 3221127168LL},
  };
  int bad = 0;
  for (const Fixture& f : fx) {
    long long mn = 0, mx = 0;
    mhgp3v::rect_h_interval(f.a, f.b, f.c, &mn, &mx);
    const bool ok = (mn == f.mn && mx == f.mx);
    std::printf("fixture %-24s min=%lld/%lld max=%lld/%lld %s\n", f.nom, mn, f.mn, mx, f.mx,
                ok ? "OK" : "DESACCORD");
    if (!ok) ++bad;
  }
  // FIXTURE DU FAUX JUGE (audit `a7f061b`, section 3). Sur une droite
  // embarquee, `A=[0,2]x{0}x{0}`, `B=[10,12]x{0}x{0}`, `z=(6,0,0)` : les NEUF
  // paires entieres placent `z` strictement entre `a` et `b`, donc
  // `E2 X2 = H^2` et q4 est VRAIE partout. Pourtant l'enveloppe AABB donne
  // `Hmin=16`, `E2max=X2max=36`, et `3 x 16^2 = 768 < 1296`. Le classifieur
  // rend donc legitimement `MIXED`, et s'en servir comme juge du coeur etait
  // une faute de methode — la mienne.
  {
    const RectBox fa{{0, 0, 0}, {2, 0, 0}};
    const RectBox fb{{10, 0, 0}, {12, 0, 0}};
    const RectBox fz{{6, 0, 0}, {6, 0, 0}};
    long long mn = 0, mx = 0;
    mhgp3v::rect_h_interval(fa, fb, fz, &mn, &mx);
    const long long e2m = mhgp3v::rect_maxsq(fz, fa), x2m = mhgp3v::rect_maxsq(fb, fz);
    long long faux = 0, vrais = 0;
    for (long long ax = 0; ax <= 2; ++ax)
      for (long long bx = 10; bx <= 12; ++bx) {
        const long long h = (6 - ax) * (bx - 6);
        const long long e2 = (6 - ax) * (6 - ax), x2 = (bx - 6) * (bx - 6);
        if (h > 0 && 3 * h * h > e2 * x2) ++vrais; else ++faux;
      }
    long long dummy = 0;
    const bool aabb_all =
        mhgp3v::rect_classify(fa, fb, fz, RectLane::kQ4, &dummy) == RectVerdict::kAll;
    std::printf("faux_juge Hmin=%lld E2max=%lld X2max=%lld 3Hmin2=%lld E2X2=%lld"
                " triplets_vrais=%lld/%lld aabb_all=%d\n",
                mn, e2m, x2m, 3 * mn * mn, e2m * x2m, vrais, vrais + faux, aabb_all ? 1 : 0);
    if (vrais != 9 || faux != 0) {
      std::fprintf(stderr, "FIXTURE: les neuf triplets devaient etre vrais en q4\n");
      ++bad;
    }
  }

  // JUGE EXHAUSTIF DE L'INTERVALLE DU SCORE `S`. Le `NONE` du certificat
  // central en depend entierement : un `Smin` surestime prononcerait un faux
  // `NONE`. On compare donc aux minimum et maximum EXHAUSTIFS sur les points
  // entiers de la boite temoin, en representation brute.
  {
    unsigned long long r = 0xDEADBEEF12345678ull;
    auto rnd = [&r](long long hi) { r ^= r << 13; r ^= r >> 7; r ^= r << 17;
                                    return (long long)(r % (unsigned long long)hi); };
    long long tests = 0, dis = 0;
    for (int it = 0; it < 40000; ++it) {
      RectBox bx[3];
      for (int qq = 0; qq < 3; ++qq)
        for (int i = 0; i < 3; ++i) { const long long lo = rnd(21) - 10;
          bx[qq].lo[i] = lo; bx[qq].hi[i] = lo + rnd(6); }
      long long smn = 0, smx = 0;
      mhgp3v::rect_s_interval(bx[0], bx[1], bx[2], &smn, &smx);
      long long emn = 0, emx = 0; bool first = true;
      for (long long zx = bx[2].lo[0]; zx <= bx[2].hi[0]; ++zx)
      for (long long zy = bx[2].lo[1]; zy <= bx[2].hi[1]; ++zy)
      for (long long zz = bx[2].lo[2]; zz <= bx[2].hi[2]; ++zz) {
        const long long z[3] = {zx, zy, zz};
        long long s = 0;
        for (int i = 0; i < 3; ++i) {
          const long long pp = 2 * z[i] - bx[0].lo[i] - bx[1].lo[i];
          const long long qq2 = 2 * z[i] - bx[0].hi[i] - bx[1].hi[i];
          const long long m = std::max(pp < 0 ? -pp : pp, qq2 < 0 ? -qq2 : qq2);
          s += m * m;
        }
        if (first) { emn = emx = s; first = false; }
        else { emn = std::min(emn, s); emx = std::max(emx, s); }
      }
      ++tests;
      if (emn != smn || emx != smx) ++dis;
    }
    std::printf("juge_score tests=%lld desaccords=%lld\n", tests, dis);
    if (dis) { std::fprintf(stderr, "DESACCORD: l'intervalle du score n'est pas exact\n"); ++bad; }
  }

  // FIXTURE DES PORTEURS (audit `AUDIT_DEBLOCAGE`, § 7). `a=(0,0,0)`,
  // `b=(4,0,0)`, `x=(2,3,0)` : `D2=16`, `E2=X2=13`, `H=-5`. Le triangle `abx`
  // est AIGU et porte par l'arete maximale, alors que `x` est strictement HORS
  // de la boule diametrale — donc invisible a tout certificat de temoin. C'est
  // la fixture qui montre que temoins et porteurs sont complementaires.
  {
    const RectBox ca{{0, 0, 0}, {0, 0, 0}};
    const RectBox cb{{4, 0, 0}, {4, 0, 0}};
    const RectBox cx{{2, 3, 0}, {2, 3, 0}};
    long long mn = 0, mx = 0;
    mhgp3v::rect_h_interval(ca, cb, cx, &mn, &mx);
    const mhgp3v::RectCarrier v = mhgp3v::rect_carrier_verdict(ca, cb, cx);
    const long long d2 = mhgp3v::rect_minsq(ca, cb);
    const long long e2 = mhgp3v::rect_minsq(cx, ca), x2 = mhgp3v::rect_minsq(cb, cx);
    std::printf("porteur D2=%lld E2=%lld X2=%lld H=%lld verdict=%s\n", d2, e2, x2, mn,
                v == mhgp3v::RectCarrier::kAll ? "ALL"
                    : (v == mhgp3v::RectCarrier::kNone ? "NONE" : "MIXED"));
    if (!(d2 == 16 && e2 == 13 && x2 == 13 && mn == -5)) {
      std::fprintf(stderr, "FIXTURE: les valeurs du porteur ne sont pas celles de l'audit\n");
      ++bad;
    }
    if (v != mhgp3v::RectCarrier::kAll) {
      std::fprintf(stderr, "FIXTURE: (2,3,0) doit etre un porteur q3 de [(0,0,0),(4,0,0)]\n");
      ++bad;
    }
    if (mn >= 0) {
      std::fprintf(stderr, "FIXTURE: un porteur doit etre HORS de la boule diametrale\n");
      ++bad;
    }
  }

  // JUGE EXHAUSTIF DES MARGES. `ALL` doit etre COMPLET sur le produit AABB —
  // tout triple y est alors porteur — et `NONE` doit etre SUR : aucun triple
  // porteur ne doit s'y cacher. `NONE` a le droit d'etre incomplet, jamais
  // faux.
  {
    unsigned long long r = 0x5DEECE66Dull;
    auto rnd = [&r](long long hi) { r ^= r << 13; r ^= r >> 7; r ^= r << 17;
                                    return (long long)(r % (unsigned long long)hi); };
    long long tests = 0, faux_all = 0, faux_none = 0;
    for (int it = 0; it < 30000; ++it) {
      RectBox bx[3];
      for (int qq = 0; qq < 3; ++qq)
        for (int i = 0; i < 3; ++i) { const long long lo = rnd(13) - 6;
          bx[qq].lo[i] = lo; bx[qq].hi[i] = lo + rnd(3); }
      const mhgp3v::RectCarrier v =
          mhgp3v::rect_carrier_by_margins(bx[0], bx[1], bx[2]);
      long long porteurs = 0, total = 0;
      for (long long ax = bx[0].lo[0]; ax <= bx[0].hi[0]; ++ax)
      for (long long ay = bx[0].lo[1]; ay <= bx[0].hi[1]; ++ay)
      for (long long az = bx[0].lo[2]; az <= bx[0].hi[2]; ++az)
      for (long long bxx = bx[1].lo[0]; bxx <= bx[1].hi[0]; ++bxx)
      for (long long by = bx[1].lo[1]; by <= bx[1].hi[1]; ++by)
      for (long long bz = bx[1].lo[2]; bz <= bx[1].hi[2]; ++bz)
      for (long long cx = bx[2].lo[0]; cx <= bx[2].hi[0]; ++cx)
      for (long long cy = bx[2].lo[1]; cy <= bx[2].hi[1]; ++cy)
      for (long long cz = bx[2].lo[2]; cz <= bx[2].hi[2]; ++cz) {
        const long long A[3] = {ax, ay, az}, B[3] = {bxx, by, bz}, C[3] = {cx, cy, cz};
        long long D = 0, E = 0, X = 0;
        for (int i = 0; i < 3; ++i) {
          D += (B[i] - A[i]) * (B[i] - A[i]);
          E += (C[i] - A[i]) * (C[i] - A[i]);
          X += (B[i] - C[i]) * (B[i] - C[i]);
        }
        ++total;
        if (E + X - D > 0 && D - E >= 0 && D - X >= 0) ++porteurs;
      }
      ++tests;
      if (v == mhgp3v::RectCarrier::kAll && porteurs != total) ++faux_all;
      if (v == mhgp3v::RectCarrier::kNone && porteurs != 0) ++faux_none;
    }
    std::printf("juge_marges tests=%lld ALL_faux=%lld NONE_faux=%lld\n",
                tests, faux_all, faux_none);
    if (faux_all || faux_none) {
      std::fprintf(stderr, "DESACCORD: les marges rendent un verdict faux\n"); ++bad;
    }
  }

  // FIXTURE PERMANENTE DES MARGES (audit `dba8961`, § 5). `a=(0,0,0)`,
  // `b=(0,1,0)`, `C = {(1,0,0),(1,1,0)}` : les deux points donnent
  // `(M0,M1,M2) = (2,0,-1)` et `(2,-1,0)`. AUCUN n'est porteur, mais aucune
  // marge n'est impossible partout — le verdict doit donc etre `MIXED`, et
  // surtout PAS `NONE`. C'est la fixture qui distingue « incomplet » de
  // « faux ».
  {
    const RectBox ma{{0, 0, 0}, {0, 0, 0}};
    const RectBox mb{{0, 1, 0}, {0, 1, 0}};
    const RectBox mc{{1, 0, 0}, {1, 1, 0}};
    const mhgp3v::RectMargins mm = mhgp3v::rect_carrier_margins(ma, mb, mc);
    const mhgp3v::RectCarrier mv = mhgp3v::rect_carrier_by_margins(ma, mb, mc);
    std::printf("marges M0=[%lld,%lld] M1=[%lld,%lld] M2=[%lld,%lld] verdict=%s\n",
                mm.m0lo, mm.m0hi, mm.m1lo, mm.m1hi, mm.m2lo, mm.m2hi,
                mv == mhgp3v::RectCarrier::kAll ? "ALL"
                    : (mv == mhgp3v::RectCarrier::kNone ? "NONE" : "MIXED"));
    if (mv != mhgp3v::RectCarrier::kMixed) {
      std::fprintf(stderr, "FIXTURE: les marges doivent rendre MIXED, jamais NONE\n");
      ++bad;
    }
    // Les deux points, verifies un a un par le predicat ponctuel exact.
    for (int k = 0; k < 2; ++k) {
      const long long x[3] = {1, (long long)k, 0};
      const long long D = 1, E = 1 + (long long)k * k, X = 1 + (1 - (long long)k) * (1 - (long long)k);
      const long long M0 = E + X - D, M1 = D - E, M2 = D - X;
      const bool porteur = (M0 > 0 && M1 >= 0 && M2 >= 0);
      std::printf("  point (%lld,%lld,%lld) M0=%lld M1=%lld M2=%lld porteur=%d\n",
                  x[0], x[1], x[2], M0, M1, M2, porteur ? 1 : 0);
      if (porteur) { std::fprintf(stderr, "FIXTURE: aucun des deux points n'est porteur\n"); ++bad; }
    }
  }

  // MUTANT COLINEAIRE (audit `96be8e0`, section 4) : un nuage porte par une
  // droite ne contient AUCUN triangle ni tetraedre propre, donc aucun support
  // q3/q4 ne peut exister. Toute issue etiquetee POSITIVE hors q2 serait un
  // faux support ; on exige `positifs_q2 == 0` sur les lanes q3 et q4.
  Tree t;
  for (int i = 0; i < 256; ++i) t.pts.push_back({(long long)(7 * i), 0, 0});
  t.nodes.reserve(4 * t.pts.size());
  build(&t, 0, (int)t.pts.size(), 1);
  for (int ln = 1; ln <= 2; ++ln) {
    Counters c{};
    solve(t, 0, 0, (RectLane)ln, 64, RectFrontInject::kNone, &c, 0.0, false);
    std::printf("colineaire lane=q%d positifs_q2=%lld keep_anchor=%lld fermes=%lld\n",
                ln + 2, c.rect_positive, c.rect_keep_anchor, c.rect_closed);
    if (c.rect_positive != 0) {
      std::fprintf(stderr, "FAUX SUPPORT: lane q%d annonce %lld positifs sur un nuage colineaire\n",
                   ln + 2, c.rect_positive);
      ++bad;
    }
  }
  if (bad) { std::fprintf(stderr, "FIXTURES: %d desaccords\n", bad); return 3; }
  std::printf("fixtures accord=OUI\n");
  return 0;
}

// ---- ORACLE EXHAUSTIF DES VERDICTS `ALL` (audit `a7f061b`, section 4).
// Un echantillonnage a quatre triples par nœud peut FALSIFIER une faute
// frequente ; il ne prouve JAMAIS un universel. Sur un petit nuage, on enumere
// donc la TOTALITE des triples `(a,b,z)` de chaque nœud declare `ALL`, dans une
// ecriture qui n'emprunte ni `Lambda`, ni le coeur central, ni les bornes de
// boites — seulement le predicat ponctuel exact.
struct AllOracle { long long verdicts = 0, triples = 0, disagree = 0; };

void oracle_all_rect(const Tree& t, int ia, int ib, RectLane lane, AllOracle* o) {
  const Node& A = t.nodes[ia];
  const Node& B = t.nodes[ib];
  std::vector<int> st{0};
  while (!st.empty()) {
    const int ic = st.back(); st.pop_back();
    const Node& C = t.nodes[ic];
    long long mx = 0;
    const RectVerdict v = mhgp3v::rect_classify(A.box, B.box, C.box, lane, &mx);
    if (v == RectVerdict::kAll) {
      ++o->verdicts;
      for (int ai = A.begin; ai < A.end; ++ai)
        for (int bi = B.begin; bi < B.end; ++bi)
          for (int zi = C.begin; zi < C.end; ++zi) {
            __int128 h = 0, e2 = 0, x2 = 0;
            for (int d = 0; d < 3; ++d) {
              const long long ea = t.pts[zi][d] - t.pts[ai][d];
              const long long tb = t.pts[bi][d] - t.pts[zi][d];
              h += (__int128)ea * tb; e2 += (__int128)ea * ea; x2 += (__int128)tb * tb;
            }
            bool ok = (h > 0);
            if (ok && lane == RectLane::kQ3) ok = (4 * h * h > e2 * x2);
            if (ok && lane == RectLane::kQ4) ok = (3 * h * h > e2 * x2);
            ++o->triples;
            if (!ok) ++o->disagree;
          }
      continue;                      // un nœud ALL n'a pas besoin d'etre raffine
    }
    if (v == RectVerdict::kMixed && C.left >= 0) { st.push_back(C.left); st.push_back(C.right); }
  }
}

int run_oracle_all(int n, int coord, long long seed, long long min_triples) {
  // PLANCHERS PAR LANE. Les verdicts `ALL` q3/q4 sont intrinsequement plus
  // rares que q2 — le spindle q4 est strictement inclus dans le q3, lui-meme
  // dans la boule diametrale —, donc un plancher unique serait soit vide de
  // sens en q2, soit inatteignable en q4.
  const long long floors[3] = {min_triples, min_triples / 100, min_triples / 200};
  const std::vector<mhgp::P3> cloud =
      mhgp3v::make_family_cloud(mhgp3v::CloudFamily::kUniform, n, coord, seed);
  Tree t;
  for (const mhgp::P3& p : cloud) t.pts.push_back({p.x, p.y, p.z});
  t.nodes.reserve(4 * t.pts.size());
  build(&t, 0, (int)t.pts.size(), 4);
  int bad = 0;
  for (int ln = 0; ln < 3; ++ln) {
    AllOracle o{};
    // TOUS les couples de nœuds disjoints de l'arbre, pas un echantillon.
    for (size_t ia = 0; ia < t.nodes.size(); ++ia)
      for (size_t ib = ia + 1; ib < t.nodes.size(); ++ib) {
        if (!(t.nodes[ib].begin >= t.nodes[ia].end || t.nodes[ia].begin >= t.nodes[ib].end))
          continue;
        oracle_all_rect(t, (int)ia, (int)ib, (RectLane)ln, &o);
      }
    std::printf("oracle_all lane=q%d verdicts=%lld triples=%lld desaccords=%lld\n",
                ln + 2, o.verdicts, o.triples, o.disagree);
    if (o.triples < floors[ln]) {
      std::fprintf(stderr, "PLANCHER ORACLE: lane q%d seulement %lld triples (< %lld)\n",
                   ln + 2, o.triples, floors[ln]);
      return 3;
    }
    if (o.disagree != 0) ++bad;
  }
  if (bad) { std::fprintf(stderr, "DESACCORD DU JUGE: %d lanes refutees exhaustivement\n", bad); return 1; }
  std::printf("oracle_all accord=OUI\n");
  return 0;
}

[[noreturn]] void refuse(const char* why) {
  std::fprintf(stderr, "REFUS: %s\n", why);
  std::exit(2);
}

long long arg_ll(const char* s, long long lo, long long hi, const char* name) {
  errno = 0;
  char* end = nullptr;
  const long long v = std::strtoll(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0') refuse(name);
  if (v < lo || v > hi) refuse(name);
  return v;
}

}  // namespace

int main(int argc, char** argv) {
  std::string family = "uniform";
  std::vector<long long> ns;
  long long coord = 65535, budget = 24, leaf = 8, seed = 12345, selftest = 20000;
  long long budget_depth = 0;   // si > 0 : budget = budget_depth * ceil(log2(n/leaf))
  int lane_raw = 0;
  double max_slope = 1.35;
  long long min_closed_pct = 0, min_all = 0, min_none = 0, min_mixed = 0;
  double stop_wsp = 0.0;
  bool use_core = false;
  RectFrontInject inject = RectFrontInject::kNone;
  bool expect_mutant = false;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--fixtures") return run_fixtures();
    if (a.rfind("--oracle-all=", 0) == 0)
      return run_oracle_all((int)arg_ll(a.substr(13).c_str(), 16, 400, "oracle-all"), 4096, 7777, 100000);
    auto val = [&](const char* p) { return a.substr(std::strlen(p)); };
    if (a.rfind("--family=", 0) == 0) family = val("--family=");
    else if (a.rfind("--points=", 0) == 0) {
      std::string s = val("--points="); size_t p = 0;
      while (p < s.size()) { size_t q = s.find(',', p);
        if (q == std::string::npos) q = s.size();
        ns.push_back(arg_ll(s.substr(p, q - p).c_str(), 64, 100000000LL, "points")); p = q + 1; }
    }
    else if (a.rfind("--coord=", 0) == 0) coord = arg_ll(val("--coord=").c_str(), 16, 65535, "coord");
    else if (a.rfind("--budget=", 0) == 0) budget = arg_ll(val("--budget=").c_str(), 1, 1000000, "budget");
    // BUDGET PROPORTIONNEL A LA PROFONDEUR. Atteindre un noeud temoin au niveau
    // des feuilles coute environ `2 log2(n/leaf)` classifications ; un budget
    // CONSTANT est donc sous le seuil des que l'arbre grandit, et fabrique une
    // pente superieure a un qui ne doit rien a la geometrie (reçu G4 du
    // 13 aout : budget 24 refuse partout, budget 48 passe).
    else if (a.rfind("--budget-depth=", 0) == 0)
      budget_depth = arg_ll(val("--budget-depth=").c_str(), 1, 64, "budget-depth");
    else if (a.rfind("--leaf=", 0) == 0) leaf = arg_ll(val("--leaf=").c_str(), 1, 4096, "leaf");
    else if (a.rfind("--lane=", 0) == 0) lane_raw = (int)arg_ll(val("--lane=").c_str(), 0, 2, "lane");
    else if (a.rfind("--seed=", 0) == 0) seed = arg_ll(val("--seed=").c_str(), 0, (1LL << 40), "seed");
    else if (a.rfind("--selftest=", 0) == 0) selftest = arg_ll(val("--selftest=").c_str(), 0, 1000000, "selftest");
    else if (a.rfind("--stop-wsp=", 0) == 0) {
      stop_wsp = std::atof(val("--stop-wsp=").c_str());
      if (!(stop_wsp >= 0.0 && stop_wsp <= 64.0)) refuse("stop-wsp");
    }
    else if (a == "--core") use_core = true;
    else if (a == "--verify-core") { use_core = true; g_verify_core = true; }
    else if (a == "--verify-all") g_verify_all = true;
    else if (a.rfind("--max-slope=", 0) == 0) max_slope = std::atof(val("--max-slope=").c_str());
    else if (a.rfind("--min-closed-pct=", 0) == 0) min_closed_pct = arg_ll(val("--min-closed-pct=").c_str(), 0, 100, "min-closed-pct");
    else if (a.rfind("--min-all=", 0) == 0) min_all = arg_ll(val("--min-all=").c_str(), 0, (1LL << 50), "min-all");
    else if (a.rfind("--min-none=", 0) == 0) min_none = arg_ll(val("--min-none=").c_str(), 0, (1LL << 50), "min-none");
    else if (a.rfind("--min-mixed=", 0) == 0) min_mixed = arg_ll(val("--min-mixed=").c_str(), 0, (1LL << 50), "min-mixed");
    else if (a == "--inject=max-par-coins") { inject = RectFrontInject::kMaxParCoins; expect_mutant = true; }
    else if (a == "--inject=sommet-non-ecrete") { inject = RectFrontInject::kSommetNonEcrete; expect_mutant = true; }
    else if (a == "--inject=coeur-trop-grand") { inject = RectFrontInject::kCoeurTropGrand; use_core = true; g_verify_core = true; }
    else refuse("option inconnue");
  }
  if (ns.empty()) ns = {2000, 8000, 32000};

  // ---- Le juge d'abord : aucune mesure n'est publiee si l'intervalle est faux.
  if (selftest > 0) {
    const SelfTest st = run_selftest((int)selftest, inject);
    std::printf("selftest tests=%lld desaccords_min=%lld desaccords_max=%lld\n",
                st.tests, st.disagree_min, st.disagree_max);
    if (expect_mutant && inject != RectFrontInject::kCoeurTropGrand) {
      if (st.disagree_min == 0 && st.disagree_max == 0) {
        std::fprintf(stderr, "MUTANT SURVIVANT: l'injection ne contredit pas le juge\n");
        return 3;
      }
      std::printf("mutant_killed=1 raison=juge_exhaustif\n");
      return 4;
    }
    if (st.disagree_min != 0 || st.disagree_max != 0) {
      std::fprintf(stderr, "DESACCORD DU JUGE sur l'intervalle exact\n");
      return 1;
    }
  }

  mhgp3v::CloudFamily fam;
  if (family == "uniform") fam = mhgp3v::CloudFamily::kUniform;
  else if (family == "terrain") fam = mhgp3v::CloudFamily::kTerrain;
  else if (family == "eight_clusters") fam = mhgp3v::CloudFamily::kEightClusters;
  else if (family == "scanline_single_pass") fam = mhgp3v::CloudFamily::kScanlineSinglePass;
  else if (family == "scanline_overlap_multiecho") fam = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
  else refuse("famille inconnue");

  std::vector<double> resid_mass, visited_per_n;
  Counters agg{};
  for (size_t k = 0; k < ns.size(); ++k) {
    const long long n = ns[k];
    const std::vector<mhgp::P3> cloud = mhgp3v::make_family_cloud(fam, (int)n, (int)coord, seed);
    if ((long long)cloud.size() * 100 < n * 90)
      refuse("cardinalite non garantie par la famille");
    Tree t;
    t.pts.reserve(cloud.size());
    for (const mhgp::P3& p : cloud) t.pts.push_back({p.x, p.y, p.z});
    // condensat du nuage : la mesure est liee au nuage exact, pas a un nom
    unsigned long long digest = 1469598103934665603ull;
    for (const auto& p : t.pts)
      for (int i = 0; i < 3; ++i) { digest ^= (unsigned long long)p[i]; digest *= 1099511628211ull; }
    t.nodes.reserve(4 * t.pts.size());
    build(&t, 0, (int)t.pts.size(), (int)leaf);
    Counters c{};
    long long eff_budget = budget;
    if (budget_depth > 0) {
      long long depth = 0;
      for (long long q = (long long)t.pts.size() / std::max<long long>(leaf, 1); q > 1; q >>= 1) ++depth;
      ++depth;
      eff_budget = budget_depth * depth;
    }
    const RectLane lane = (RectLane)lane_raw;
    solve(t, 0, 0, lane, eff_budget, inject, &c, stop_wsp, use_core);
    const long long m = (long long)t.pts.size();
    const long long tot = m * (m - 1) / 2;
    if (c.mass_closed + c.mass_positive + c.mass_keep_anchor + c.mass_residual != tot) {
      std::fprintf(stderr, "INVARIANT VIOLE: partition des paires %lld + %lld + %lld + %lld != %lld\n",
                   c.mass_closed, c.mass_positive, c.mass_keep_anchor, c.mass_residual, tot);
      return 3;
    }
    std::printf("n=%lld famille=%s lane=q%d digest=%016llx"
                " | visites=%lld fermes=%lld positifs_q2=%lld keep_anchor=%lld residuels=%lld capes=%lld"
                " | masse_fermee=%lld masse_positive=%lld masse_keep_anchor=%lld masse_residuelle=%lld"
                " budget=%lld pct_ferme=%.2f pct_decide=%.2f"
                " | eval=%lld ALL=%lld NONE=%lld MIXED=%lld Wmax=%lld coeur=%lld/%lld front/pt=%.3f\n",
                m, family.c_str(), lane_raw + 2, digest,
                c.rect_visited, c.rect_closed, c.rect_positive, c.rect_keep_anchor, c.rect_residual, c.rect_capped,
                c.mass_closed, c.mass_positive, c.mass_keep_anchor, c.mass_residual, eff_budget,
                100.0 * (double)c.mass_closed / (double)tot,
                100.0 * (double)(c.mass_closed + c.mass_positive) / (double)tot,
                c.evals, c.all_hits, c.none_hits, c.mixed_hits, c.w_high, c.core_closed, c.core_tests,
                (double)(c.rect_closed + c.rect_positive + c.rect_keep_anchor + c.rect_residual) / (double)m);
    resid_mass.push_back((double)c.mass_residual);
    visited_per_n.push_back((double)c.rect_visited);
    agg.all_hits += c.all_hits; agg.none_hits += c.none_hits; agg.mixed_hits += c.mixed_hits;
    agg.mass_closed += c.mass_closed;
    if (g_verify_all) {
      std::printf("juge_all triples=%lld desaccords=%lld\n", c.all_judged, c.all_disagree);
      if (c.all_judged < 1000) {
        std::fprintf(stderr, "PLANCHER JUGE ALL: %lld triples juges\n", c.all_judged);
        return 3;
      }
      if (c.all_disagree != 0) {
        std::fprintf(stderr, "DESACCORD DU JUGE: %lld triples ALL refutes par le predicat ponctuel\n",
                     c.all_disagree);
        return 1;
      }
    }
    if (g_verify_all) {
      std::printf("juge_all triples=%lld desaccords=%lld\n", c.all_judged, c.all_disagree);
      if (c.all_judged < 1000) {
        std::fprintf(stderr, "PLANCHER JUGE ALL: %lld triples juges\n", c.all_judged);
        return 3;
      }
      if (c.all_disagree != 0) {
        std::fprintf(stderr, "DESACCORD DU JUGE: %lld triples ALL refutes par le predicat ponctuel\n",
                     c.all_disagree);
        return 1;
      }
    }
    if (g_verify_core) {
      std::printf("juge_coeur points=%lld desaccords=%lld\n", c.core_judged, c.core_disagree);
      if (c.core_judged < 1000) {
        std::fprintf(stderr, "PLANCHER JUGE COEUR: %lld points juges\n", c.core_judged);
        return 3;
      }
      if (c.core_disagree != 0) {
        std::fprintf(stderr, "DESACCORD DU JUGE: %lld points du coeur hors de la region ALL exacte\n",
                     c.core_disagree);
        return 1;
      }
    }
    if (c.evals > eff_budget * c.rect_visited) {
      std::fprintf(stderr, "INVARIANT VIOLE: %lld classifications pour %lld rectangles a budget %lld\n",
                   c.evals, c.rect_visited, eff_budget);
      return 3;
    }
    if (k + 1 == ns.size() && min_closed_pct > 0 &&
        100.0 * (double)c.mass_closed / (double)tot < (double)min_closed_pct) {
      std::fprintf(stderr, "PLANCHER: fermeture %.2f%% < %lld%%\n",
                   100.0 * (double)c.mass_closed / (double)tot, min_closed_pct);
      return 3;
    }
  }

  int bad = 0;
  for (size_t k = 1; k < resid_mass.size(); ++k) {
    const double s = std::log2(resid_mass[k] / resid_mass[k - 1]) /
                     std::log2((double)ns[k] / (double)ns[k - 1]);
    const double sv = std::log2(visited_per_n[k] / visited_per_n[k - 1]) /
                      std::log2((double)ns[k] / (double)ns[k - 1]);
    std::printf("pente masse_residuelle=%.3f pente rect_visites=%.3f (%lld->%lld)\n",
                s, sv, ns[k - 1], ns[k]);
    if (s >= max_slope) ++bad; else bad = 0;
    if (bad >= 2) { std::fprintf(stderr, "REFUS DE PENTE: deux pentes consecutives >= %.2f\n", max_slope); return 3; }
  }
  if (agg.all_hits < min_all) { std::fprintf(stderr, "PLANCHER ALL: %lld < %lld\n", agg.all_hits, min_all); return 3; }
  if (agg.none_hits < min_none) { std::fprintf(stderr, "PLANCHER NONE: %lld < %lld\n", agg.none_hits, min_none); return 3; }
  if (agg.mixed_hits < min_mixed) { std::fprintf(stderr, "PLANCHER MIXED: %lld < %lld\n", agg.mixed_hits, min_mixed); return 3; }
  std::printf("OK famille=%s lane=q%d budget=%lld leaf=%lld ALL=%lld NONE=%lld MIXED=%lld\n",
              family.c_str(), lane_raw + 2, budget, leaf, agg.all_hits, agg.none_hits, agg.mixed_hits);
  return 0;
}
