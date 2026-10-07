#!/usr/bin/env python3
"""Sonde de conception v11 (hors produit) : variantes du generateur v10 pilotees par l'environnement.

MHGP_EXP_ALLPAIRS=N : un noeud dont la liste parente compte au plus N sites filtre par dominance toutes paires
                      (Y = toute la liste parente) au lieu du reservoir de 3K sites ; 0 = filtre de la v10.
MHGP_EXP_FLAT=1     : enumeration plate : pas de test de la droite des equidistants (un triplet non aligne de poids
                      admissible est vivant) ; 0 = feuille de la v10.
Les deux variantes doivent rendre le meme catalogue (theoreme G : ni Y ni l'ordre des tests n'interviennent).
Compteurs supplementaires ecrits sur la sortie d'erreur (une ligne JSON).
"""
import sys

src = open(sys.argv[1]).read()


def rep(old, new, count=1):
    global src
    if src.count(old) != count:
        raise SystemExit(f"occurrence inattendue ({src.count(old)}) : {old[:70]!r}")
    src = src.replace(old, new)


rep('#include <numeric>\n', '#include <numeric>\n#include <cstdio>\n#include <cstdlib>\n')
rep('constexpr int kT = 6;\n', '''constexpr int kT = 6;
// SONDE v11 : configuration et compteurs
struct ExpCfg {
  int allpairs = 0, flat = 0;
};
ExpCfg g_exp;
struct ExpCnt {
  u64 r0_nodes = 0, r1_nodes = 0, r1_roots = 0, r0_visits = 0, r1_visits = 0, r1_pairs = 0, r0_tests = 0;
  u64 h_np[10] = {};      // noeuds par taille de liste parente : <=16, 24, 32, 48, 64, 128, 256, 1024, 4096, au-dela
  u64 h_visits[10] = {};  // visites par la meme classe
  u64 p2_pairs = 0, p3_pairs = 0, p4_pairs = 0;
  u64 trip_w = 0, trip_aligned = 0, trip_acute = 0, trip_live4 = 0;
  u64 rows4 = 0, rows4_nonempty = 0;
  u64 quad_w = 0, quad_env = 0, quad_inside = 0, quad_inbox = 0;
  u64 j2 = 0, j3 = 0, j4 = 0, e2 = 0, e3 = 0, e4 = 0;
};
inline int np_class(u64 np) {
  static const u64 lim[9] = {16, 24, 32, 48, 64, 128, 256, 1024, 4096};
  for (int i = 0; i < 9; ++i)
    if (np <= lim[i]) return i;
  return 9;
}
''')
rep('''struct alignas(64) Local {  // une par fil : pas de faux partage entre compteurs voisins
''', '''struct alignas(64) Local {  // une par fil : pas de faux partage entre compteurs voisins
  ExpCnt xc;
''')
# juges par arite
rep('''  ++L.led.judged;
  const P3 a = L.lp[anchor_local];''', '''  ++L.led.judged;
  if (qgen == 2) ++L.xc.j2; else if (qgen == 3) ++L.xc.j3; else ++L.xc.j4;
  const P3 a = L.lp[anchor_local];''')
rep('''  L.recs.push_back(r);
  ++L.led.emitted;''', '''  L.recs.push_back(r);
  if (q == 2) ++L.xc.e2; else if (q == 3) ++L.xc.e3; else ++L.xc.e4;
  ++L.led.emitted;''')
# paires vivantes
rep('''      if (static_cast<i64>(d) <= th3) {
        setbit(P2 + size_t(i) * nw, j);
        setbit(P2 + size_t(j) * nw, i);
      }''', '''      ++L.xc.p2_pairs;
      if (static_cast<i64>(d) <= th4) ++L.xc.p4_pairs;
      if (static_cast<i64>(d) <= th3) {
        ++L.xc.p3_pairs;
        setbit(P2 + size_t(i) * nw, j);
        setbit(P2 + size_t(j) * nw, i);
      }''')
# triplets : variante plate
rep('''            const int line = center_line_meets(lx[i], lx2[i], lx[j], lx2[j], lx[k], lx2[k], Q);
            if (line < 0) continue;  // alignes''', '''            ++L.xc.trip_w;
            int line;
            if (g_exp.flat) {
              const P3 cr = geom::cross(geom::sub(lp[j], lp[i]), geom::sub(lp[k], lp[i]));
              line = (cr.x == 0 && cr.y == 0 && cr.z == 0) ? -1 : 1;
            } else {
              line = center_line_meets(lx[i], lx2[i], lx[j], lx2[j], lx[k], lx2[k], Q);
            }
            if (line < 0) {
              ++L.xc.trip_aligned;
              continue;  // alignes
            }
            if (geom::acute(lp[i], lp[j], lp[k])) ++L.xc.trip_acute;''')
rep('''            if (static_cast<i64>(d) <= th4) {
              setbit(hrow(i, j), k);''', '''            if (static_cast<i64>(d) <= th4) {
              ++L.xc.trip_live4;
              setbit(hrow(i, j), k);''')
# quadruplets : compteurs de selectivite (enveloppe, interieur, boite), sans changer la decision
rep('''            const u64* Dk = Dm + size_t(k) * nw;
            for (u32 tl = k >> 6; tl < nw; ++tl)
              for (u64 ls = Hij[tl] & Hik[tl] & Hjk[tl] & bits_above(k, tl); ls; ls &= ls - 1) {''', '''            const u64* Dk = Dm + size_t(k) * nw;
            ++L.xc.rows4;
            {
              bool any = false;
              for (u32 tl = k >> 6; tl < nw; ++tl) any = any || (Hij[tl] & Hik[tl] & Hjk[tl] & bits_above(k, tl)) != 0;
              L.xc.rows4_nonempty += any;
            }
            for (u32 tl = k >> 6; tl < nw; ++tl)
              for (u64 ls = Hij[tl] & Hik[tl] & Hjk[tl] & bits_above(k, tl); ls; ls &= ls - 1) {''')
rep('''                if (!geom::center4(a, b, e, f, ctr)) continue;
                if (!center_in_box(a, ctr, Q)) continue;
                const P3* t4[4] = {&a, &b, &e, &f};
                if (!geom::strictly_inside_tetra(t4, a, ctr)) continue;''', '''                ++L.xc.quad_w;
                {
                  bool env = true;
                  const i64 cx[4] = {a.x, b.x, e.x, f.x}, cy[4] = {a.y, b.y, e.y, f.y}, cz[4] = {a.z, b.z, e.z, f.z};
                  const i64* cc[3] = {cx, cy, cz};
                  for (int ax = 0; ax < 3; ++ax) {
                    const i64 mn = std::min(std::min(cc[ax][0], cc[ax][1]), std::min(cc[ax][2], cc[ax][3]));
                    const i64 mx = std::max(std::max(cc[ax][0], cc[ax][1]), std::max(cc[ax][2], cc[ax][3]));
                    env = env && (mx << kT) >= Q.lo[ax] && (mn << kT) < Q.hi[ax];
                  }
                  L.xc.quad_env += env;
                }
                if (!geom::center4(a, b, e, f, ctr)) continue;
                const P3* t4[4] = {&a, &b, &e, &f};
                const bool ins = geom::strictly_inside_tetra(t4, a, ctr);
                L.xc.quad_inside += ins;
                if (!center_in_box(a, ctr, Q)) continue;
                ++L.xc.quad_inbox;
                if (!ins) continue;''')
# filtre des noeuds : variante toutes paires
rep('''  const u32 np = static_cast<u32>(parent.size());
  const i64 hx = Q.hi[0] - Q.lo[0], hy = Q.hi[1] - Q.lo[1], hz = Q.hi[2] - Q.lo[2];
  // reservoir par insertion, cle (dd, rang)''', '''  const u32 np = static_cast<u32>(parent.size());
  const i64 hx = Q.hi[0] - Q.lo[0], hy = Q.hi[1] - Q.lo[1], hz = Q.hi[2] - Q.lo[2];
  {
    const int c = np_class(np);
    ++L.xc.h_np[c];
    L.xc.h_visits[c] += np;
  }
  if (g_exp.allpairs > 0 && np <= u32(g_exp.allpairs)) {
    // SONDE : Y = toute la liste parente (proposition L : la liste reste certifiee quel que soit Y)
    static thread_local std::vector<i64> ax, ay, az, aa;
    ax.resize(np);
    ay.resize(np);
    az.resize(np);
    aa.resize(np);
    for (u32 t = 0; t < np; ++t) {
      const P3& X = C.X[parent[t]];
      const i64 a = X.x - Q.lo[0], b = X.y - Q.lo[1], c = X.z - Q.lo[2];
      ax[t] = 2 * hx * a;
      ay[t] = 2 * hy * b;
      az[t] = 2 * hz * c;
      aa[t] = a * a + b * b + c * c;
    }
    cand.resize(np);
    u32 m = 0;
    for (u32 t = 0; t < np; ++t) {
      i64 w = 0;
      for (u32 r = 0; r < np; ++r) {
        const i64 d = std::max<i64>(ax[t] - ax[r], 0) + std::max<i64>(ay[t] - ay[r], 0) + std::max<i64>(az[t] - az[r], 0);
        w += -i64(aa[t] - aa[r] > d) & i64(C.w[parent[r]]);
      }
      cand[m] = parent[t];
      m += w < C.K;
    }
    cand.resize(m);
    ++L.xc.r1_nodes;
    L.xc.r1_visits += np;
    L.xc.r1_pairs += u64(np) * np;
    return;
  }
  ++L.xc.r0_nodes;
  L.xc.r0_visits += np;
  // reservoir par insertion, cle (dd, rang)''')
rep('''  cand.resize(m);
  L.led.filter_tests += tests;
}''', '''  cand.resize(m);
  L.led.filter_tests += tests;
  L.xc.r0_tests += tests;
  if (g_exp.allpairs > 0 && m <= u32(g_exp.allpairs)) ++L.xc.r1_roots;
}''')
# lecture de l'environnement et impression
rep('''  Catalogue cat;
  cat.kmax = params.kmax;
  if (n < 2) {''', '''  if (const char* s = std::getenv("MHGP_EXP_ALLPAIRS")) g_exp.allpairs = std::atoi(s);
  if (const char* s = std::getenv("MHGP_EXP_FLAT")) g_exp.flat = std::atoi(s);
  Catalogue cat;
  cat.kmax = params.kmax;
  if (n < 2) {''')
rep('''  for (const Local& L : locals)
    if (!L.fail.ok()) return L.fail;''', '''  for (const Local& L : locals)
    if (!L.fail.ok()) return L.fail;
  {
    ExpCnt t;
    for (const Local& L : locals) {
      const u64* a = reinterpret_cast<const u64*>(&L.xc);
      u64* b = reinterpret_cast<u64*>(&t);
      for (size_t i = 0; i < sizeof(ExpCnt) / sizeof(u64); ++i) b[i] += a[i];
    }
    std::fprintf(stderr, "{\\"exp_allpairs\\":%d,\\"exp_flat\\":%d,\\"r0_nodes\\":%llu,\\"r1_nodes\\":%llu,\\"r1_roots\\":%llu,"
                 "\\"r0_visits\\":%llu,\\"r1_visits\\":%llu,\\"r1_pairs\\":%llu,\\"r0_tests\\":%llu,",
                 g_exp.allpairs, g_exp.flat, (unsigned long long)t.r0_nodes, (unsigned long long)t.r1_nodes,
                 (unsigned long long)t.r1_roots, (unsigned long long)t.r0_visits, (unsigned long long)t.r1_visits,
                 (unsigned long long)t.r1_pairs, (unsigned long long)t.r0_tests);
    std::fprintf(stderr, "\\"h_np\\":[");
    for (int i = 0; i < 10; ++i) std::fprintf(stderr, "%s%llu", i ? "," : "", (unsigned long long)t.h_np[i]);
    std::fprintf(stderr, "],\\"h_visits\\":[");
    for (int i = 0; i < 10; ++i) std::fprintf(stderr, "%s%llu", i ? "," : "", (unsigned long long)t.h_visits[i]);
    std::fprintf(stderr, "],\\"p2_pairs\\":%llu,\\"p3_pairs\\":%llu,\\"p4_pairs\\":%llu,\\"trip_w\\":%llu,\\"trip_aligned\\":%llu,"
                 "\\"trip_acute\\":%llu,\\"trip_live4\\":%llu,\\"rows4\\":%llu,\\"rows4_nonempty\\":%llu,\\"quad_w\\":%llu,"
                 "\\"quad_env\\":%llu,\\"quad_inside\\":%llu,\\"quad_inbox\\":%llu,\\"j2\\":%llu,\\"j3\\":%llu,\\"j4\\":%llu,"
                 "\\"e2\\":%llu,\\"e3\\":%llu,\\"e4\\":%llu}\\n",
                 (unsigned long long)t.p2_pairs, (unsigned long long)t.p3_pairs, (unsigned long long)t.p4_pairs,
                 (unsigned long long)t.trip_w, (unsigned long long)t.trip_aligned, (unsigned long long)t.trip_acute,
                 (unsigned long long)t.trip_live4, (unsigned long long)t.rows4, (unsigned long long)t.rows4_nonempty,
                 (unsigned long long)t.quad_w, (unsigned long long)t.quad_env, (unsigned long long)t.quad_inside,
                 (unsigned long long)t.quad_inbox, (unsigned long long)t.j2, (unsigned long long)t.j3,
                 (unsigned long long)t.j4, (unsigned long long)t.e2, (unsigned long long)t.e3, (unsigned long long)t.e4);
  }''')
open(sys.argv[2], 'w').write(src)
print("ok")
