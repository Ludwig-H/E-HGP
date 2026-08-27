// MorseHGP3D v5 — FIXTURE BLOQUANTE : la source q4 est INDEPENDANTE de q3.
//
// Coordonnees EXACTES reprises de la v4 (audit bc5b05d, renforce apres
// f6b29e1), rejugees ici par des primitives LOCALES (oracle/obig.hpp) puis
// par le PIPELINE v5 complet (src/pipeline/run.hpp) :
//   tetraedre  a = (100,300,300), b = (300,300,300), x = (200,160,400),
//              y = (200,160,200), ids 0..3 ; circumcentre c = (200,230,300),
//              R² = 14900 ; ab et xy de longueur carree 40000, les quatre
//              autres aretes 39600 : owner ab par depart EdgeKey(0,1) < (2,3) ;
//   z_i (ids 4..12) = (200,355,300),(200,354,310),(200,353,315),(200,352,320),
//              (200,351,323),(200,350,325),(200,356,305),(200,355,312),
//              (200,354,317) : neuf temoins de W_3(a,b) ∖ W_4(a,b) — l'ancre
//              ab est q3-MORTE (n3 = 9 >= h_3 = 9) et q4-VIVANTE (n4 = 0 <
//              h_4 = 8) ; |z_i - c|² graves (15625, 15476, 15354, 15284,
//              15170, 15025, 15901, 15769, 15665), tous > R² : profondeur 0 ;
//   w_j (ids 13..21) = (196+j, 105, 300), j = 0..8 (version 22 points) :
//              rendent les faces axy et bxy q3-profondes (depth >= h_3) et
//              l'ancre xy q3-morte / q4-vivante ; |w_j - c|² = 15625 + t²,
//              t = j - 4 : toujours exterieurs ;
//   point 22 (version 23 = « 13+1 » de q4_events_probe v4) = (200,109,300) :
//              |z - c|² = 14641 < 14900 (interieur du tetraedre q4) mais
//              4|z - m|² = 145924 > 3D² = 120000 (hors du cover coefficient 3
//              de ab, dans le coefficient 4 : 145924 <= 160000).
//   version « 13+8 » : les huit points (200,109,300), (200,110,300),
//              (200,111,300), (200,109,301), (200,109,299), (200,110,301),
//              (200,110,299), (200,111,301) (ids 13..20), tous interieurs a
//              la boule q4 et hors du cover coefficient 3 : |I_B| = 8 = h_4,
//              la boule {0,1,2,3} est MORTE ; seul le coefficient 4 la voit a
//              la generation (mecanisme, voir ci-dessous).
//
// Re-derivations locales (jamais les primitives de production dans la
// decision) : W_q(a,b) par H = (z-a)·(b-z), Ξ = |(b-a)×(z-a)|², W_3 : H > 0 et
// 3H² > Ξ, W_4 : H > 0 et 2H² > Ξ (MATHEMATIQUES § 4.1) ; profondeur q3 d'une
// face aigue par sa puissance equatoriale en six longueurs (l'equatoriale
// d'une face aigue EST sa circumboule) : Pow = (4AB - C2²)F - B(2A - C2)D2 -
// A(2B - C2)E2 < 0 (derivation dans tests/q4_oracle.cpp) ; interieur du
// tetraedre par |z - c|² contre 14900, le centre entier c etant PROUVE par
// l'equidistance exacte des quatre sommets.
//
// Ce que le pipeline doit produire (on_forest) : l'evenement q4 du tetraedre
// {0,1,2,3}, q = 4, d = 0, niveau ≡ 14900, a K = 3 (versions 13 et 22) ; a
// K = 4 avec l'interieur {22} (version 23) ; AUCUN evenement de support
// {0,1,2,3} (version 13+8, boule morte). Bit-identique a 1 et a --threads fils.
//
// MUTANTS :
//   q4-seeds-from-q3-live : la lane q4 branchee sur les rectangles vivants
//     de q3 ; l'ancre ab est q3-morte => le tetraedre est PERDU (code 4) ;
//   q4-cover-coef3 : cover coefficient 3 au lieu de 4. En v5 le census est
//     une descente EXACTE de l'arbre (census.hpp), independante du cover :
//     l'interieur 22 de la version 23 est retrouve quand meme et l'OBJET ne
//     change pas — la fixture 13+1 de la v4 ne le discrimine PLUS (constat
//     grave ici, jamais cache : sur --fixture=23 ce mutant rend 3, « porte
//     inefficace », par construction). Ce que le coefficient 4 garantit en
//     v5 est un MECANISME : par Jung, tout interieur d'une boule q4 d'ancre
//     ab verifie |z - m| <= D(1/√8 + √(3/8)) < D, donc le filtre de
//     profondeur A LA GENERATION voit tous les interieurs et tue la boule
//     {0,1,2,3} de la version 13+8 avant emission (gen.depth_killed[2] >= 1,
//     et l'evenement n'existe pas) ; avec le coefficient 3 les huit
//     interieurs sont invisibles a la generation, la boule est EMISE et ne
//     meurt qu'au prefiltre : depth_killed[2] tombe a 0 — c'est la porte
//     appariee (code 4 sur --fixture=13+8), l'objet restant identique.
// Codes : 0 conforme ; 2 refus (arguments, mutant inconnu) ; 3 fixture
// violee / porte inefficace / debordement OBig ; 4 mutant tue.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../oracle/obig.hpp"
#include "../src/core/mutants.hpp"
#include "../src/lanes/q4.hpp"
#include "../src/pipeline/run.hpp"
#include "../src/spindle/spindle.hpp"

using namespace mhgp5;

namespace {

using OB = mhgp5_oracle::OBig384;
OB ob(i64 v) { return OB::from_i64(v); }

// ---- primitives locales -----------------------------------------------------

i64 o_len2(const P3& p, const P3& q) {
  const i64 dx = p.x - q.x, dy = p.y - q.y, dz = p.z - q.z;
  return dx * dx + dy * dy + dz * dz;
}
struct OV3 {
  OB c[3];
};
OV3 ov_diff(const P3& p, const P3& q) { return OV3{{ob(p.x - q.x), ob(p.y - q.y), ob(p.z - q.z)}}; }
OB ov_dot(const OV3& a, const OV3& b) { return a.c[0] * b.c[0] + a.c[1] * b.c[1] + a.c[2] * b.c[2]; }
OV3 ov_cross(const OV3& a, const OV3& b) {
  return OV3{{a.c[1] * b.c[2] - a.c[2] * b.c[1], a.c[2] * b.c[0] - a.c[0] * b.c[2],
              a.c[0] * b.c[1] - a.c[1] * b.c[0]}};
}

// z ∈ W_q(a,b), q ∈ {3,4}.
bool o_in_w(int q, const P3& a, const P3& b, const P3& z) {
  const OV3 w = ov_diff(z, a), d = ov_diff(b, a), bz = ov_diff(b, z);
  const OB H = ov_dot(w, bz);
  if (H.sign() <= 0) return false;
  const OV3 cr = ov_cross(d, w);
  const OB Xi = ov_dot(cr, cr);
  return cmp(ob(q == 3 ? 3 : 2) * H * H, Xi) > 0;
}

// Puissance equatoriale de la face (p,q,r) sur s (six longueurs carrees).
OB o_eq_power(const P3& p, const P3& q, const P3& r, const P3& s) {
  const OB A = ob(o_len2(p, q)), B = ob(o_len2(p, r)), F = ob(o_len2(p, s));
  const OB C2 = A + B - ob(o_len2(q, r));
  const OB D2 = A + F - ob(o_len2(q, s));
  const OB E2 = B + F - ob(o_len2(r, s));
  const OB H = ob(4) * A * B - C2 * C2;
  return H * F - B * (ob(2) * A - C2) * D2 - A * (ob(2) * B - C2) * E2;
}

int g_viol = 0;
void viol(const char* what) {
  std::fprintf(stderr, "FIXTURE : %s\n", what);
  ++g_viol;
}

struct Variant {
  std::string name;  // 13, 22, 23, 13+8
  std::vector<P3> pts;
  bool xy_anchor_alive = true;  // exige xy q3-morte / q4-vivante
  bool all_faces_deep = false;  // exige les quatre faces q3-profondes
  int expect_k = 3;             // K de l'evenement attendu (0 = aucun)
  int expect_d = 0;
  PointId expect_interior = 0;
  bool mechanism = false;       // porte appariee du coefficient (13+8)
};

bool build_variant(const std::string& name, Variant* v) {
  v->name = name;
  v->pts = {{100, 300, 300}, {300, 300, 300}, {200, 160, 400}, {200, 160, 200},
            {200, 355, 300}, {200, 354, 310}, {200, 353, 315}, {200, 352, 320}, {200, 351, 323},
            {200, 350, 325}, {200, 356, 305}, {200, 355, 312}, {200, 354, 317}};
  if (name == "13") return true;
  if (name == "13+8") {
    v->pts.insert(v->pts.end(), {{200, 109, 300}, {200, 110, 300}, {200, 111, 300}, {200, 109, 301},
                                 {200, 109, 299}, {200, 110, 301}, {200, 110, 299}, {200, 111, 301}});
    v->xy_anchor_alive = false;  // les huit points sont dans W_4(x,y) : xy devient q4-morte, sans effet (owner ab)
    v->expect_k = 0;
    v->mechanism = true;
    return true;
  }
  for (i64 j = 0; j < 9; ++j) v->pts.push_back(P3{196 + j, 105, 300});
  v->all_faces_deep = true;
  if (name == "22") return true;
  if (name == "23") {
    v->pts.push_back(P3{200, 109, 300});
    v->expect_k = 4;
    v->expect_d = 1;
    v->expect_interior = 22;
    return true;
  }
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  std::string fixture = "22", inject;
  int threads = 1;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--fixture=", 0) == 0) fixture = arg.substr(10);
    else if (arg.rfind("--threads=", 0) == 0) threads = std::atoi(arg.c_str() + 10);
    else if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else {
      std::fprintf(stderr, "REFUS : argument inconnu %s\n", arg.c_str());
      return 2;
    }
  }
  Variant v;
  if (!build_variant(fixture, &v) || threads < 1) {
    std::fprintf(stderr, "REFUS : --fixture=13|22|23|13+8, --threads>=1\n");
    return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) {
    std::fprintf(stderr, "REFUS : mutant inconnu %s\n", inject.c_str());
    return 2;
  }
  const bool mutant = !inject.empty();
  mhgp5_oracle::clear_overflow();

  const std::vector<P3>& P = v.pts;
  const size_t n = P.size();
  const P3 &a = P[0], &b = P[1], &x = P[2], &y = P[3];
  const u64 smax = 11;
  const u64 h3 = lane_h(Lane::kQ3, smax), h4 = lane_h(Lane::kQ4, smax);
  if (h3 != 9 || h4 != 8) viol("seuils h3/h4 (attendus 9/8)");

  // ---- 1. Ancres ab (et xy) : q3-mortes, q4-vivantes — oracle local ET in_spindle.
  const P3* anchors[2][2] = {{&a, &b}, {&x, &y}};
  u64 n3[2] = {0, 0}, n4[2] = {0, 0};
  for (int an = 0; an < 2; ++an)
    for (size_t u = 0; u < n; ++u) {
      if (&P[u] == anchors[an][0] || &P[u] == anchors[an][1]) continue;
      const bool w3 = o_in_w(3, *anchors[an][0], *anchors[an][1], P[u]);
      const bool w4 = o_in_w(4, *anchors[an][0], *anchors[an][1], P[u]);
      if (w3 != in_spindle(Lane::kQ3, *anchors[an][0], *anchors[an][1], P[u])) viol("in_spindle q3 vs oracle");
      if (w4 != in_spindle(Lane::kQ4, *anchors[an][0], *anchors[an][1], P[u])) viol("in_spindle q4 vs oracle");
      if (w4 && !w3) viol("W_4 ⊄ W_3");
      n3[an] += w3;
      n4[an] += w4;
    }
  if (n3[0] < h3 || n4[0] >= h4) viol("ancre ab : attendue q3-morte et q4-vivante");
  if (n3[0] != 9 || n4[0] != 0) viol("ancre ab : n3 = 9 et n4 = 0 graves");
  if (v.xy_anchor_alive && (n3[1] < h3 || n4[1] >= h4)) viol("ancre xy : attendue q3-morte et q4-vivante");
  if (v.name == "13" && n3[1] >= h3) viol("version 13 : l'ancre xy n'est pas encore q3-morte (c'est le trou que 22 ferme)");

  // ---- 2. Les quatre faces : longueurs gravees, acuite stricte, profondeur q3.
  struct Face {
    const P3 *p, *q, *apex;
  };
  const Face faces[4] = {{&a, &b, &x}, {&a, &b, &y}, {&x, &y, &a}, {&x, &y, &b}};
  int deep_faces = 0;
  for (int fc = 0; fc < 4; ++fc) {
    const Face& F = faces[fc];
    if (o_len2(*F.p, *F.q) != 40000 || o_len2(*F.p, *F.apex) != 39600 || o_len2(*F.q, *F.apex) != 39600)
      viol("longueurs d'une face (40000 / 39600 / 39600)");
    // Acuite stricte : les trois produits scalaires aux sommets > 0.
    const i64 lpq = 40000, lpa = 39600, lqa = 39600;
    if (!(lpq + lpa > lqa && lpq + lqa > lpa && lpa + lqa > lpq)) viol("face non strictement aigue");
    u64 depth = 0;
    for (size_t u = 0; u < n; ++u) {
      if (&P[u] == F.p || &P[u] == F.q || &P[u] == F.apex) continue;
      const OB pw = o_eq_power(*F.p, *F.q, *F.apex, P[u]);
      const Q3Form f3 = q3_form(*F.p, *F.q, *F.apex);
      const i128 sp = q3_power(f3, P[u]);
      if ((sp < 0) != (pw.sign() < 0) || (sp == 0) != (pw.sign() == 0)) viol("q3_power vs puissance equatoriale locale");
      if (pw.sign() < 0) ++depth;
    }
    if (depth >= h3) ++deep_faces;
    if (fc < 2 && depth < h3) viol("faces abx / aby : attendues q3-profondes (les neuf z_i)");
  }
  if (v.all_faces_deep && deep_faces != 4) viol("version 22/23 : les quatre faces doivent etre q3-profondes");
  if (v.name == "13" && deep_faces != 2) viol("version 13 : exactement deux faces q3-profondes (axy, bxy restent des Q3Event)");

  // ---- 3. Le tetraedre : centre entier prouve, R² = 14900, owner, interieurs.
  const P3 c{200, 230, 300};
  const i64 R2 = 14900;
  if (o_len2(a, c) != R2 || o_len2(b, c) != R2 || o_len2(x, c) != R2 || o_len2(y, c) != R2)
    viol("c = (200,230,300) n'est pas equidistant des quatre sommets");
  const Q4Form f4 = q4_form(a, b, x, y);
  if (f4.det <= 0) viol("q4_form : det <= 0");
  if (!q4_center_strictly_inside(f4, a, b, x, y)) viol("q4_center_strictly_inside");
  for (int s = 0; s < 4; ++s) {
    const P3* vv[4] = {&a, &b, &x, &y};
    const P3* fp[3];
    int t = 0;
    for (int i = 0; i < 4; ++i)
      if (i != s) fp[t++] = vv[i];
    if (o_eq_power(*fp[0], *fp[1], *fp[2], *vv[s]).sign() <= 0) viol("puissance equatoriale <= 0 : pas strictement bien centre");
  }
  {
    const ExactLevel lv = q4_level_raw(f4);
    const ExactLevel want{{(u64)R2, 0, 0}, 1};
    if (!same_exact_level(lv, want)) viol("niveau q4 != 14900");
    if (!tetra_owned_by(40000, 39600, 39600, 39600, 39600, 40000, 0, 1, 2, 3)) viol("owner ab (EdgeKey(0,1) < (2,3))");
    if (tetra_owned_by(40000, 39600, 39600, 39600, 39600, 40000, 2, 3, 0, 1)) viol("xy ne doit pas etre owner");
  }
  const i64 zc2[9] = {15625, 15476, 15354, 15284, 15170, 15025, 15901, 15769, 15665};
  u64 inside = 0, shell = 0;
  std::vector<PointId> interior_ids;
  for (size_t u = 4; u < n; ++u) {
    const i64 du = o_len2(P[u], c);
    if (u < 13 && du != zc2[u - 4]) viol("distance gravee |z_i - c|²");
    if ((v.name == "22" || v.name == "23") && u >= 13 && u < 22) {
      const i64 t = (i64)(u - 13) - 4;
      if (du != 15625 + t * t) viol("distance gravee |w_j - c|² = 15625 + t²");
    }
    const i128 pw = q4_power(f4, P[u]);
    if ((pw < 0) != (du < R2) || (pw == 0) != (du == R2)) viol("q4_power vs |z - c|² local");
    if (du < R2) { ++inside; interior_ids.push_back((PointId)u); }
    if (du == R2) ++shell;
    if (u >= 13 && v.name != "22") {
      // Points « 13+1 » / « 13+8 » : interieurs, hors cover coefficient 3, dans le coefficient 4.
      const i64 m4 = 4 * o_len2(P[u], P3{200, 300, 300});  // |2z - (a+b)|²
      if (du >= R2) viol("point additionnel : attendu strictement interieur");
      if (m4 <= 120000 || m4 > 160000) viol("point additionnel : attendu hors coef 3 (> 3D²) et dans coef 4 (<= 4D²)");
      if (o_in_w(3, a, b, P[u]) || o_in_w(4, a, b, P[u])) viol("point additionnel : ne doit pas etre temoin de l'ancre ab");
    }
  }
  if (shell != 0) viol("coquille attendue vide");
  if (v.name == "13+8") {
    if (inside != 8) viol("version 13+8 : exactement huit interieurs (|I_B| = h_4 : boule morte)");
  } else if (inside != (u64)v.expect_d) {
    viol("profondeur q4 gravee");
  }
  if (v.name == "23" && (interior_ids.size() != 1 || interior_ids[0] != 22)) viol("version 23 : interieur {22}");

  if (mhgp5_oracle::overflow_seen()) {
    std::fprintf(stderr, "REFUS numeric_failure : debordement de l'oracle OBig (fail-closed)\n");
    return 3;
  }
  if (g_viol > 0) {
    std::fprintf(stderr, "FIXTURE %s violee (%d) avant le pipeline\n", v.name.c_str(), g_viol);
    return 3;
  }

  // ---- 4. Le pipeline v5 complet, a 1 fil puis a `threads` fils.
  std::vector<InputPoint> in(n);
  for (size_t i = 0; i < n; ++i) in[i] = InputPoint{(PointId)i, P[i]};
  struct Found {
    u64 count = 0, k = 0, q = 0, d = 0;
    PointId interior0 = 0;
    ExactLevel level{};
    bool level_ok = false;
  };
  const auto run = [&](int th, Found* fd, RunResult* rr) {
    RunOptions opt;
    opt.s = 8;
    opt.smax = smax;
    opt.threads = th;
    opt.digest = true;
    opt.on_forest = [&](u64 K, const std::vector<ForestEvent>& events, const ForestResult&) {
      for (const ForestEvent& e : events) {
        bool has[4] = {false, false, false, false};
        for (int t = 0; t < (int)e.q; ++t)
          for (int s = 0; s < 4; ++s)
            if (e.support[t] == (PointId)s) has[s] = true;
        if (!(has[0] && has[1] && has[2] && has[3])) continue;
        ++fd->count;
        fd->k = K;
        fd->q = e.q;
        fd->d = e.d;
        fd->interior0 = e.d ? e.interior[0] : 0;
        fd->level = e.level;
        fd->level_ok = same_exact_level(e.level, ExactLevel{{(u64)R2, 0, 0}, 1});
      }
    };
    *rr = run_pipeline(in, opt);
  };
  Found f1, ft;
  RunResult r1, rt;
  run(1, &f1, &r1);
  if (r1.status != PipelineStatus::kCompleteRegular) {
    std::fprintf(stderr, "PIPELINE : %s\n", r1.message.c_str());
    if (mutant) {
      std::printf("MUTANT TUE : %s change le statut du pipeline\n", inject.c_str());
      return 4;
    }
    return 3;
  }
  if (threads > 1) {
    run(threads, &ft, &rt);
    if (rt.status != r1.status || rt.digest_all != r1.digest_all || rt.digest_balls != r1.digest_balls ||
        ft.count != f1.count || ft.k != f1.k) {
      std::fprintf(stderr, "PARALLELISME : sortie differente a %d fils\n", threads);
      return 3;
    }
  }
  const GenerateStats& gs = r1.gen;
  std::printf(
      "q4_source_%s : n=%zu ab n3=%llu n4=%llu xy n3=%llu n4=%llu faces_profondes=%d interieurs=%llu | "
      "evenements_support_0123=%llu K=%llu q=%llu d=%llu niveau_14900=%d | candidats_q4=%llu "
      "tues_profondeur_q4=%llu seeds_core_tues=%llu boules_uniques=%llu mortes_prefiltre=%llu evenements=%llu\n",
      v.name.c_str(), n, (unsigned long long)n3[0], (unsigned long long)n4[0], (unsigned long long)n3[1],
      (unsigned long long)n4[1], deep_faces, (unsigned long long)inside, (unsigned long long)f1.count,
      (unsigned long long)f1.k, (unsigned long long)f1.q, (unsigned long long)f1.d, f1.level_ok ? 1 : 0,
      (unsigned long long)gs.candidates[2], (unsigned long long)gs.depth_killed[2],
      (unsigned long long)gs.seeds_killed_core, (unsigned long long)r1.expand.unique_balls,
      (unsigned long long)r1.expand.dead_depth, (unsigned long long)r1.total_events);

  // ---- 5. Verdict sur l'objet.
  bool object_ok;
  if (v.expect_k == 0) {
    object_ok = f1.count == 0;
  } else {
    object_ok = f1.count == 1 && f1.k == (u64)v.expect_k && f1.q == 4 && f1.d == (u64)v.expect_d && f1.level_ok &&
                (v.expect_d == 0 || f1.interior0 == v.expect_interior);
  }
  // Mecanisme (13+8) : le coefficient 4 tue la boule a la generation.
  const bool mechanism_ok = !v.mechanism || gs.depth_killed[2] >= 1;

  if (mutant) {
    if (!object_ok) {
      std::printf("MUTANT TUE : %s perd ou altere l'evenement q4 grave\n", inject.c_str());
      return 4;
    }
    if (v.mechanism && !mechanism_ok) {
      std::printf("MUTANT TUE : %s — la boule {0,1,2,3} (|I_B| = 8) n'est plus tuee a la generation "
                  "(depth_killed_q4 = 0) ; objet identique\n",
                  inject.c_str());
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s non discrimine sur --fixture=%s (objet identique)\n",
                 inject.c_str(), v.name.c_str());
    return 3;
  }
  if (!object_ok) {
    std::fprintf(stderr, "FIXTURE : evenement q4 {0,1,2,3} absent, altere ou en trop\n");
    return 3;
  }
  if (!mechanism_ok) {
    std::fprintf(stderr, "FIXTURE 13+8 : la boule morte doit etre tuee A LA GENERATION (coefficient 4)\n");
    return 3;
  }
  return 0;
}
