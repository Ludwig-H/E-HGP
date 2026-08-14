// MorseHGP3D v3 — J0 : LA TAILLE DE L'OBJET, AUX TROIS ECHELLES DU CONTRAT.
//
// Specification : jalon J0 de
// audits/NOTE_CLAUDE_PLAN_50K_PUIS_TRENTE_MILLIONS_20260814.md.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=diagnostic_exact_borne,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// CE QU'IL MESURE, ET POURQUOI C'EST LE PREMIER JALON
//
// Un seul chiffre gouverne la faisabilite du contrat : combien de supports
// l'objet contient-il ? La chaine historique a publie `21 413 140` `SupportKey`
// a `50 000` points sur `uniform`, soit `428` par point. Selon que ce nombre
// est lineaire ou non, et selon ce qu'il devient a `K=5`, la cible `100 ms` est
// atteignable ou arithmetiquement morte. Aucune optimisation ne repond a cette
// question ; seule une mesure le fait.
//
// Le brute force `C(n,4)` du depot plafonne a `n ~ 400`. Cette sonde emploie
// donc l'enumeration par ANCRE D'ARETE DIAMETRALE, et elle est validee EXACTE
// contre ce brute force par son propre mode `--verifie`.
//
// ---------------------------------------------------------------------------
// POURQUOI L'ENUMERATION PAR ANCRE EST EXACTE
//
// Soit `S` un support positif d'arite `q`, `(a,b)` son arete diametrale,
// `D=|ab|`, `m` son milieu, `B(c,R)` sa circumboule. Alors :
//
//   (J) Jung en dimension trois : `D/2 <= R <= D sqrt(3/8)` ;
//   (M) `c` est dans le plan mediateur de `(a,b)`, a distance
//       `t = sqrt(R^2-D^2/4) <= D/sqrt(8)` de `m` ;
//   (L) tout sommet est dans la lentille `B(a,D) inter B(b,D)`, donc a distance
//       au plus `D sqrt(3)/2 = 0,866 D` de `m` ;
//   (I) tout interieur est a distance `< R+t <= 0,966 D` de `m`.
//
// Une seule requete `B(m,D)` fournit donc SOMMETS ET INTERIEURS. Il n'y a
// aucune approximation dans cette etape.
//
// ---------------------------------------------------------------------------
// LE PREFILTRE D'ANCRE : LE FUSEAU
//
// `W_q(a,b)` est l'intersection de TOUTES les boules admissibles de l'ancre a
// l'arite `q`. Il est donc inclus dans la circumboule de tout support d'arete
// diametrale `(a,b)` : si `|P inter W_q| >= mort_q`, l'ancre ne porte aucun
// support pertinent. Necessaire, jamais suffisant.
//
// Forme entiere, avec `U = 2z-(a+b)`, `d = b-a`, `D2 = |d|^2` :
//   `g = D2 - |U|^2 = 4H`,  `Q = D2|U|^2 - (U.d)^2 = 4R_perp`
//   q2 : `g > 0`        q3 : `g > 0` et `3g^2 > 4Q`      q4 : `g > 0` et `g^2 > 2Q`
// L'identite avec la representation `(H,R)` est demontree dans
// `prototype/spindle_cone.hpp` ; les deux ecritures ont ete derivees
// independamment et coincident.
//
// ---------------------------------------------------------------------------
// CE QUI N'EST PAS CERTIFIE, ET COMMENT C'EST GARDE
//
// L'enumeration des ANCRES est bornee par `--dmax`. Ce n'est PAS un certificat
// de localite : le certificat est le rayon par calottes, qui n'est pas encore
// implemente. La sonde publie donc le diametre maximal REELLEMENT atteint par
// un support retenu et REFUSE la mesure (code 3) s'il depasse `0,75 dmax`.
//
// En consequence, et tant que Q16 n'a pas de reponse, ce que publie cette sonde
// est un LEDGER DE CANDIDATS sous coupure declaree, pas un nombre de supports
// certifie. Le nom des compteurs le dit.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "cloud_families.hpp"
#include "corner8_ball.hpp"

namespace {

using mhgp::i128;
using mhgp::i64;

struct Pt {
  i64 c[3] = {0, 0, 0};
};

[[noreturn]] void refuse(const std::string& raison) {
  std::fprintf(stderr, "REFUS: %s\n", raison.c_str());
  std::exit(2);
}

inline i64 d2p(const Pt& a, const Pt& b) {
  const i64 ux = a.c[0] - b.c[0], uy = a.c[1] - b.c[1], uz = a.c[2] - b.c[2];
  return ux * ux + uy * uy + uz * uz;
}

// Fuseau d'arite `q` : `g>0` et, selon l'arite, `3g^2>4Q` ou `g^2>2Q`.
inline bool fuseau(i128 D2, i128 nu, i128 dot, int arite) {
  const i128 g = D2 - nu;
  if (g <= 0) return false;
  if (arite == 2) return true;
  const i128 Q = D2 * nu - dot * dot;
  if (arite == 3) return 3 * g * g > 4 * Q;
  return g * g > 2 * Q;
}

// Triangle strictement aigu.
inline bool aigu(const Pt& a, const Pt& b, const Pt& x) {
  auto dot = [](const Pt& o, const Pt& p, const Pt& q) -> i128 {
    i128 s = 0;
    for (int i = 0; i < 3; ++i) s += (i128)(p.c[i] - o.c[i]) * (q.c[i] - o.c[i]);
    return s;
  };
  return dot(a, b, x) > 0 && dot(b, a, x) > 0 && dot(x, a, b) > 0;
}

// Circumboule d'un triangle, ambiante : `c = a + Num/(2|n|^2)`.
struct Ball {
  i128 num[3] = {0, 0, 0};
  i128 den = 0;
};

inline void cross3(const i128 u[3], const i128 v[3], i128 out[3]) {
  out[0] = u[1] * v[2] - u[2] * v[1];
  out[1] = u[2] * v[0] - u[0] * v[2];
  out[2] = u[0] * v[1] - u[1] * v[0];
}

inline bool q3_ball(const Pt& pa, const Pt& pb, const Pt& pc, Ball* out) {
  i128 u[3], v[3], n[3];
  for (int i = 0; i < 3; ++i) {
    u[i] = (i128)pb.c[i] - pa.c[i];
    v[i] = (i128)pc.c[i] - pa.c[i];
  }
  cross3(u, v, n);
  const i128 nn = n[0] * n[0] + n[1] * n[1] + n[2] * n[2];
  if (nn == 0) return false;
  i128 vn[3], nu[3];
  cross3(v, n, vn);
  cross3(n, u, nu);
  const i128 uu = u[0] * u[0] + u[1] * u[1] + u[2] * u[2];
  const i128 vv = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
  for (int i = 0; i < 3; ++i) out->num[i] = uu * vn[i] + vv * nu[i];
  out->den = 2 * nn;
  return true;
}

inline bool dedans(const Pt& base, const Ball& B, const Pt& z) {
  i128 s[3];
  for (int i = 0; i < 3; ++i) s[i] = (i128)z.c[i] - base.c[i];
  const i128 sn = s[0] * s[0] + s[1] * s[1] + s[2] * s[2];
  const i128 sd = s[0] * B.num[0] + s[1] * B.num[1] + s[2] * B.num[2];
  return B.den * sn - 2 * sd < 0;
}

inline bool dedans_q2(const Pt& a, const Pt& b, const Pt& z) {
  i128 s = 0;
  for (int i = 0; i < 3; ++i) s += (i128)(z.c[i] - a.c[i]) * (z.c[i] - b.c[i]);
  return s < 0;
}

// ---------------------------------------------------------------------------
// GRILLE UNIFORME A OFFSETS TRIES : une requete qui abandonne tot ne paie que
// les premieres couronnes.
// ---------------------------------------------------------------------------
struct Grid {
  i64 cell = 1;
  i64 lo[3] = {0, 0, 0}, dim[3] = {1, 1, 1};
  std::vector<int> start, item;
  std::vector<std::array<int, 3>> offs;
  std::vector<i64> offd2;

  void build(const std::vector<Pt>& pts, i64 cell_size) {
    cell = cell_size < 1 ? 1 : cell_size;
    i64 hi[3];
    for (int c = 0; c < 3; ++c) { lo[c] = INT64_MAX; hi[c] = INT64_MIN; }
    for (const Pt& p : pts)
      for (int c = 0; c < 3; ++c) {
        if (p.c[c] < lo[c]) lo[c] = p.c[c];
        if (p.c[c] > hi[c]) hi[c] = p.c[c];
      }
    for (int c = 0; c < 3; ++c) dim[c] = (hi[c] - lo[c]) / cell + 1;
    const size_t nc = (size_t)dim[0] * dim[1] * dim[2];
    std::vector<int> cnt(nc + 1, 0);
    std::vector<size_t> idx(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) { idx[i] = cell_of(pts[i]); ++cnt[idx[i] + 1]; }
    for (size_t i = 0; i < nc; ++i) cnt[i + 1] += cnt[i];
    start = cnt;
    item.assign(pts.size(), 0);
    std::vector<int> cur(start.begin(), start.end() - 1);
    for (size_t i = 0; i < pts.size(); ++i) item[cur[idx[i]]++] = (int)i;
  }
  size_t cell_of(const Pt& p) const {
    i64 c[3];
    for (int k = 0; k < 3; ++k) {
      c[k] = (p.c[k] - lo[k]) / cell;
      if (c[k] < 0) c[k] = 0;
      if (c[k] >= dim[k]) c[k] = dim[k] - 1;
    }
    return (size_t)((c[2] * dim[1] + c[1]) * dim[0] + c[0]);
  }
  void build_offsets(i64 rmax) {
    const int R = (int)(rmax / cell) + 2;
    for (int z = -R; z <= R; ++z)
      for (int y = -R; y <= R; ++y)
        for (int x = -R; x <= R; ++x) {
          const i64 ax = std::max(0, std::abs(x) - 1);
          const i64 ay = std::max(0, std::abs(y) - 1);
          const i64 az = std::max(0, std::abs(z) - 1);
          offs.push_back({x, y, z});
          offd2.push_back((ax * ax + ay * ay + az * az) * cell * cell);
        }
    std::vector<size_t> ord(offs.size());
    for (size_t i = 0; i < ord.size(); ++i) ord[i] = i;
    std::sort(ord.begin(), ord.end(), [&](size_t a, size_t b) { return offd2[a] < offd2[b]; });
    std::vector<std::array<int, 3>> o2(offs.size());
    std::vector<i64> d2(offs.size());
    for (size_t i = 0; i < ord.size(); ++i) { o2[i] = offs[ord[i]]; d2[i] = offd2[ord[i]]; }
    offs.swap(o2);
    offd2.swap(d2);
  }
  template <class F>
  void query(const i64 ctr[3], i64 r, F&& f) const {
    i64 c[3];
    for (int k = 0; k < 3; ++k) {
      const i64 v = ctr[k] - lo[k];
      c[k] = v >= 0 ? v / cell : -(((-v) + cell - 1) / cell);
    }
    const i64 r2 = r * r;
    for (size_t i = 0; i < offs.size(); ++i) {
      if (offd2[i] > r2) return;
      const i64 gx = c[0] + offs[i][0], gy = c[1] + offs[i][1], gz = c[2] + offs[i][2];
      if (gx < 0 || gy < 0 || gz < 0 || gx >= dim[0] || gy >= dim[1] || gz >= dim[2]) continue;
      const size_t cc = (size_t)((gz * dim[1] + gy) * dim[0] + gx);
      for (int t = start[cc]; t < start[cc + 1]; ++t)
        if (!f(item[t])) return;
    }
  }
};

// ---------------------------------------------------------------------------
struct Ledger {
  long long paires_testees = 0, ancres_q2 = 0, ancres_q3 = 0, ancres_q4 = 0;
  long long seeds_aigus = 0, paires_lentille = 0, bien_centres = 0;
  long long cand_q2 = 0, cand_q3 = 0, cand_q4 = 0;
  i64 diam2_max = 0;
  long long lens_max = 0, inner_max = 0;
};

void fusionne(Ledger* g, const Ledger& a) {
  g->paires_testees += a.paires_testees; g->ancres_q2 += a.ancres_q2;
  g->ancres_q3 += a.ancres_q3; g->ancres_q4 += a.ancres_q4;
  g->seeds_aigus += a.seeds_aigus; g->paires_lentille += a.paires_lentille;
  g->bien_centres += a.bien_centres;
  g->cand_q2 += a.cand_q2; g->cand_q3 += a.cand_q3; g->cand_q4 += a.cand_q4;
  if (a.diam2_max > g->diam2_max) g->diam2_max = a.diam2_max;
  if (a.lens_max > g->lens_max) g->lens_max = a.lens_max;
  if (a.inner_max > g->inner_max) g->inner_max = a.inner_max;
}

std::vector<Pt> nuage(const std::string& family, long long n, long long coord,
                      long long seed) {
  mhgp3v::CloudFamily fam;
  if (family == "uniform") fam = mhgp3v::CloudFamily::kUniform;
  else if (family == "eight_clusters") fam = mhgp3v::CloudFamily::kEightClusters;
  else if (family == "terrain") fam = mhgp3v::CloudFamily::kTerrain;
  else if (family == "scanline_single_pass") fam = mhgp3v::CloudFamily::kScanlineSinglePass;
  else if (family == "scanline_overlap_multiecho")
    fam = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
  else if (family == "two_lines") fam = mhgp3v::CloudFamily::kTwoLines;
  else refuse("famille inconnue : " + family);
  if (coord <= 0) coord = mhgp3v::cloud_family_default_coord(fam, (int)n);
  const auto brut = mhgp3v::make_family_cloud(fam, (int)n, (int)coord, seed);
  std::vector<Pt> P;
  P.reserve(brut.size());
  for (const auto& q : brut) P.push_back(Pt{{q.x, q.y, q.z}});
  return P;
}

// `smax` fixe les trois seuils de MORT : `smax-1`, `smax-2`, `smax-3` pour q2,
// q3, q4. Un support est retenu si son nombre d'interieurs est STRICTEMENT
// inferieur au seuil de sa lane.
void balaye(const std::vector<Pt>& P, const Grid& grid, int smax, i64 dmax,
            long long tid, long long threads, Ledger* out) {
  const int mort2 = smax - 1, mort3 = smax - 2, mort4 = smax - 3;
  const i64 dmax2 = dmax * dmax;
  const int m = (int)P.size();
  std::vector<int> nb, inner, lens, acu;
  for (int ia = (int)tid; ia < m; ia += (int)threads) {
    const Pt& A = P[(size_t)ia];
    nb.clear();
    grid.query(A.c, dmax, [&](int j) { if (j > ia) nb.push_back(j); return true; });
    for (int jb : nb) {
      const Pt& B = P[(size_t)jb];
      const i64 D2 = d2p(A, B);
      if (D2 == 0 || D2 > dmax2) continue;
      ++out->paires_testees;
      const i64 mc[3] = {(A.c[0] + B.c[0]) / 2, (A.c[1] + B.c[1]) / 2,
                         (A.c[2] + B.c[2]) / 2};
      const i128 ex = B.c[0] - A.c[0], ey = B.c[1] - A.c[1], ez = B.c[2] - A.c[2];
      // Passe 1 : les trois fuseaux. Tous sont inclus dans `B(m,D/2)`.
      long long f2 = 0, f3 = 0, f4 = 0;
      {
        const i64 rq = (i64)(0.5 * std::sqrt((double)D2)) + 2;
        grid.query(mc, rq, [&](int z) -> bool {
          if (z == ia || z == jb) return true;
          const Pt& Z = P[(size_t)z];
          const i128 ux = 2 * (i128)Z.c[0] - (A.c[0] + B.c[0]);
          const i128 uy = 2 * (i128)Z.c[1] - (A.c[1] + B.c[1]);
          const i128 uz = 2 * (i128)Z.c[2] - (A.c[2] + B.c[2]);
          const i128 nu = ux * ux + uy * uy + uz * uz;
          const i128 dot = ux * ex + uy * ey + uz * ez;
          if (!fuseau(D2, nu, dot, 2)) return true;
          ++f2;
          if (fuseau(D2, nu, dot, 3)) ++f3;
          if (fuseau(D2, nu, dot, 4)) ++f4;
          return !(f4 >= mort4 && f3 >= mort3 && f2 >= mort2);
        });
      }
      const bool a2 = (f2 < mort2), a3 = (f3 < mort3), a4 = (f4 < mort4);
      if (a2) ++out->ancres_q2;
      if (a3) ++out->ancres_q3;
      if (a4) ++out->ancres_q4;
      if (!a2 && !a3 && !a4) continue;
      // Passe 2 : `B(m,D)` fournit sommets ET interieurs.
      const i64 rin = (i64)std::sqrt((double)D2) + 2;
      inner.clear(); lens.clear(); acu.clear();
      grid.query(mc, rin, [&](int z) -> bool {
        if (z == ia || z == jb) return true;
        const Pt& Z = P[(size_t)z];
        const i128 ux = 2 * (i128)Z.c[0] - (A.c[0] + B.c[0]);
        const i128 uy = 2 * (i128)Z.c[1] - (A.c[1] + B.c[1]);
        const i128 uz = 2 * (i128)Z.c[2] - (A.c[2] + B.c[2]);
        if (ux * ux + uy * uy + uz * uz > 4 * (i128)D2) return true;
        inner.push_back(z);
        if (d2p(A, Z) <= D2 && d2p(B, Z) <= D2) lens.push_back(z);
        return true;
      });
      if ((long long)lens.size() > out->lens_max) out->lens_max = (long long)lens.size();
      if ((long long)inner.size() > out->inner_max) out->inner_max = (long long)inner.size();
      // q2 : le fuseau q2 EST l'interieur de la boule diametrale, donc `f2` est
      // exactement le rang. Rien a recompter.
      if (a2) { ++out->cand_q2; if (D2 > out->diam2_max) out->diam2_max = D2; }
      // Les porteurs aigus, PRE-RANG. Filtrer ici serait la cascade interdite.
      for (int ip : lens) {
        const Pt& Pp = P[(size_t)ip];
        if (!aigu(A, B, Pp)) continue;
        const i64 e1 = d2p(A, Pp), e2 = d2p(B, Pp);
        const bool owner = !((e1 == D2 && std::min(ia, ip) < std::min(ia, jb)) ||
                             (e2 == D2 && std::min(jb, ip) < std::min(ia, jb)));
        acu.push_back(owner ? ip : -1 - ip);
        ++out->seeds_aigus;
      }
      // q3 : porteur aigu owner, rang sous `mort3`.
      if (a3) for (int t : acu) {
        if (t < 0) continue;
        const Pt& Pp = P[(size_t)t];
        Ball B3;
        if (!q3_ball(A, B, Pp, &B3)) continue;
        long long ins = 0;
        for (int z : inner) {
          if (z == t) continue;
          if (dedans(A, B3, P[(size_t)z])) { if (++ins >= mort3) break; }
        }
        if (ins >= mort3) continue;
        ++out->cand_q3;
        if (D2 > out->diam2_max) out->diam2_max = D2;
      }
      // q4 : porteur aigu PRIMAIRE, quatrieme sommet dans la lentille.
      if (a4) for (int t : acu) {
        const int ip = t >= 0 ? t : -1 - t;
        const Pt& Pp = P[(size_t)ip];
        for (int iq : lens) {
          if (iq == ip) continue;
          // exact-once : `ip` doit etre le porteur aigu de plus petit `PointId`.
          if (iq < ip && aigu(A, B, P[(size_t)iq])) continue;
          const Pt& Pq = P[(size_t)iq];
          if (d2p(Pp, Pq) > D2) continue;
          ++out->paires_lentille;
          {   // `(a,b)` doit rester l'arete maximale canonique du quadruplet
            const int id[4] = {ia, jb, ip, iq};
            const Pt* qq[4] = {&A, &B, &Pp, &Pq};
            bool ok = true;
            for (int u2 = 0; u2 < 4 && ok; ++u2)
              for (int v2 = u2 + 1; v2 < 4; ++v2) {
                if (u2 == 0 && v2 == 1) continue;
                const i64 e = d2p(*qq[u2], *qq[v2]);
                if (e > D2) { ok = false; break; }
                if (e == D2) {
                  const int lo2 = std::min(id[u2], id[v2]);
                  const int hi2 = std::max(id[u2], id[v2]);
                  const int lo1 = std::min(ia, jb), hi1 = std::max(ia, jb);
                  if (lo2 < lo1 || (lo2 == lo1 && hi2 < hi1)) { ok = false; break; }
                }
              }
            if (!ok) continue;
          }
          if (!mhgp3v::c8::bien_centre(A.c, B.c, Pp.c, Pq.c)) continue;
          ++out->bien_centres;
          long long ins = 0;
          for (int z : inner) {
            if (z == ip || z == iq) continue;
            if (mhgp3v::c8::interieur_strict(A.c, B.c, Pp.c, Pq.c, P[(size_t)z].c)) {
              if (++ins >= mort4) break;
            }
          }
          if (ins >= mort4) continue;
          ++out->cand_q4;
          if (D2 > out->diam2_max) out->diam2_max = D2;
        }
      }
    }
  }
}

// Brute force `C(n,4)` : le juge d'exactitude, borne.
long long brute_q4(const std::vector<Pt>& P, int smax) {
  const int mort4 = smax - 3;
  const int m = (int)P.size();
  long long r = 0;
  for (int i = 0; i < m; ++i)
    for (int j = i + 1; j < m; ++j)
      for (int k = j + 1; k < m; ++k)
        for (int l = k + 1; l < m; ++l) {
          const i64* q[4] = {P[(size_t)i].c, P[(size_t)j].c, P[(size_t)k].c,
                             P[(size_t)l].c};
          if (mhgp3v::c8::orient3d(q[0], q[1], q[2], q[3]) == 0) continue;
          if (!mhgp3v::c8::bien_centre(q[0], q[1], q[2], q[3])) continue;
          long long ins = 0;
          for (int z = 0; z < m && ins < mort4; ++z) {
            if (z == i || z == j || z == k || z == l) continue;
            if (mhgp3v::c8::interieur_strict(q[0], q[1], q[2], q[3], P[(size_t)z].c)) ++ins;
          }
          if (ins < mort4) ++r;
        }
  return r;
}

long long brute_q2(const std::vector<Pt>& P, int smax) {
  const int mort2 = smax - 1;
  const int m = (int)P.size();
  long long r = 0;
  for (int i = 0; i < m; ++i)
    for (int j = i + 1; j < m; ++j) {
      if (d2p(P[(size_t)i], P[(size_t)j]) == 0) continue;
      long long ins = 0;
      for (int z = 0; z < m && ins < mort2; ++z) {
        if (z == i || z == j) continue;
        if (dedans_q2(P[(size_t)i], P[(size_t)j], P[(size_t)z])) ++ins;
      }
      if (ins < mort2) ++r;
    }
  return r;
}

}  // namespace

int main(int argc, char** argv) {
  std::string family = "uniform";
  long long n = 2000, coord = 0, seed = 1, smax = 11, threads = 0, dmax = 0;
  double dmax_esp = 10.0;   // coupure exprimee en ESPACEMENTS moyens mesures
  bool verifie = false;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](const char* p) { return a.substr(std::strlen(p)); };
    if (a == "--verifie") verifie = true;
    else if (a.rfind("--family=", 0) == 0) family = val("--family=");
    else if (a.rfind("--points=", 0) == 0) n = atoll(val("--points=").c_str());
    else if (a.rfind("--coord=", 0) == 0) coord = atoll(val("--coord=").c_str());
    else if (a.rfind("--seed=", 0) == 0) seed = atoll(val("--seed=").c_str());
    else if (a.rfind("--smax=", 0) == 0) smax = atoll(val("--smax=").c_str());
    else if (a.rfind("--dmax=", 0) == 0) dmax = atoll(val("--dmax=").c_str());
    else if (a.rfind("--dmax-espacements=", 0) == 0)
      dmax_esp = atof(val("--dmax-espacements=").c_str());
    else if (a.rfind("--threads=", 0) == 0) threads = atoll(val("--threads=").c_str());
    else refuse("option inconnue : " + a);
  }
  if (n < 5 || n > 200000) refuse("--points hors domaine");
  if (smax < 4 || smax > 18) refuse("--smax hors domaine");
  if (verifie && n > 200) refuse("--verifie est borne a 200 points");
  if (threads <= 0) threads = (long long)std::thread::hardware_concurrency();
  if (threads <= 0) threads = 1;

  const std::vector<Pt> P = nuage(family, n, coord, seed);
  const int m = (int)P.size();
  // L'emprise reelle donne l'espacement moyen, et donc une coupure par defaut.
  i64 lo[3] = {INT64_MAX, INT64_MAX, INT64_MAX}, hi[3] = {INT64_MIN, INT64_MIN, INT64_MIN};
  for (const Pt& p : P)
    for (int c = 0; c < 3; ++c) {
      if (p.c[c] < lo[c]) lo[c] = p.c[c];
      if (p.c[c] > hi[c]) hi[c] = p.c[c];
    }
  double vol = 1.0;
  for (int c = 0; c < 3; ++c) vol *= (double)std::max<i64>(1, hi[c] - lo[c]);
  const double espacement = std::cbrt(vol / (double)m);
  if (dmax_esp < 2.0 || dmax_esp > 64.0) refuse("--dmax-espacements hors domaine");
  if (dmax <= 0) dmax = (i64)(dmax_esp * espacement + 0.5);

  Grid grid;
  grid.build(P, std::max<i64>(1, (i64)(1.2 * espacement)));
  grid.build_offsets(dmax + 4);

  std::vector<Ledger> acc((size_t)threads);
  std::vector<std::thread> th;
  for (long long t = 0; t < threads; ++t)
    th.emplace_back(balaye, std::cref(P), std::cref(grid), (int)smax, dmax, t, threads,
                    &acc[(size_t)t]);
  for (auto& x : th) x.join();
  Ledger g;
  for (const Ledger& a : acc) fusionne(&g, a);

  const double dm = std::sqrt((double)g.diam2_max);
  std::printf("lane_source : famille=%s n=%d smax=%lld morts=%lld/%lld/%lld"
              " espacement=%.1f dmax=%lld thr=%lld\n",
              family.c_str(), m, smax, smax - 1, smax - 2, smax - 3, espacement,
              dmax, threads);
  std::printf("  ancres : testees=%lld q2=%lld q3=%lld q4=%lld (%.2f/pt)\n",
              g.paires_testees, g.ancres_q2, g.ancres_q3, g.ancres_q4,
              (double)g.ancres_q4 / (double)m);
  std::printf("  travail : seeds_aigus=%lld paires_lentille=%lld bien_centres=%lld"
              " lens_max=%lld inner_max=%lld\n",
              g.seeds_aigus, g.paires_lentille, g.bien_centres, g.lens_max, g.inner_max);
  const long long tot = g.cand_q2 + g.cand_q3 + g.cand_q4;
  std::printf("  candidats : q2=%lld q3=%lld q4=%lld total=%lld"
              " | /n : %.3f %.3f %.3f %.3f | candidats_sur_retenus=%.1f\n",
              g.cand_q2, g.cand_q3, g.cand_q4, tot,
              (double)g.cand_q2 / m, (double)g.cand_q3 / m, (double)g.cand_q4 / m,
              (double)tot / m,
              (double)g.paires_lentille / (double)std::max(1LL, g.cand_q4));
  std::printf("  coupure : diam_max=%.1f dmax=%lld rapport=%.3f\n", dm, dmax,
              dm / (double)dmax);

  if (verifie) {
    const long long b4 = brute_q4(P, (int)smax);
    const long long b2 = brute_q2(P, (int)smax);
    std::printf("verifie : brute_q2=%lld ancre_q2=%lld brute_q4=%lld ancre_q4=%lld\n",
                b2, g.cand_q2, b4, g.cand_q4);
    if (b4 != g.cand_q4 || b2 != g.cand_q2) {
      std::fprintf(stderr, "DESACCORD: l'enumeration par ancre differe du brute force\n");
      return 1;
    }
    return 0;
  }
  if (tot == 0) { std::fprintf(stderr, "PLANCHER: aucun candidat\n"); return 3; }
  if (dm > 0.75 * (double)dmax) {
    std::fprintf(stderr, "REFUS: diametre retenu %.1f > 0,75 dmax=%lld — mesure tronquee\n",
                 dm, dmax);
    return 3;
  }
  return 0;
}
