// Compteurs deterministes (hors produit) : taux de repli des filtres F6 proposes pour le generateur v11, sur les
// candidats REELS de la trame 02 vides par l'audit L05 (/tmp/v11-audit/l05_code_catalogue/dumpq/k{5,10}).
// Aucun temps mesure. Echantillon : un bloc de 4096 enregistrements sur `pas`.
// Decisions exactes : predicats i128 de la v10 (arith/geometry.hpp). Filtres : binaire64, seuils de
// CONCEPTION_GENERATEUR.md (2^-48 x majorant semi-statique pour les centres, 2^-42 m^3 pour l'interieur).
// Verifie aussi qu'aucune decision certaine du filtre ne contredit l'exact (code 1 sinon).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "arith/geometry.hpp"
using namespace mhgp10;
using geom::P3;
constexpr int kT = 6;
struct RecT { int a[3], b[3], c[3], lo[3], hi[3]; };
struct RecQ { int a[3], b[3], c[3], d[3], lo[3], hi[3]; };

static bool box_exact(const P3& a, const geom::Center& c, const int* lo, const int* hi) {
  const i64 av[3] = {a.x, a.y, a.z};
  for (int i = 0; i < 3; ++i) {
    const i128 v = (i128(av[i]) * c.D + c.N[i]) * (i128(1) << kT);
    if (v < i128(lo[i]) * c.D || v >= i128(hi[i]) * c.D) return false;
  }
  return true;
}
// 0 dehors certain, 1 dedans certain, 2 indecis. g = a' D + 2^T N, k = h D - g ; a' = 2^T a - lo, h = hi - lo.
static int box_filter(const double* ap, const double* h, const double* N, const double* MN, double D) {
  int res = 1;
  for (int k = 0; k < 3; ++k) {
    const double g = ap[k] * D + 64.0 * N[k];
    const double Mg = std::fabs(ap[k]) * D + 64.0 * MN[k];
    const double kk = h[k] * D - g;
    const double Mk = h[k] * D + Mg;
    const double tg = 0x1p-48 * Mg, tk = 0x1p-48 * Mk;
    if (g < -tg || kk < -tk) return 0;
    if (!(g > tg && kk > tk)) res = 2;
  }
  return res;
}
template <class R, class F>
static unsigned long long sample(const char* path, unsigned long long step, F&& f) {
  FILE* fp = std::fopen(path, "rb");
  if (!fp) { std::fprintf(stderr, "absent : %s\n", path); std::exit(2); }
  std::vector<R> buf(4096);
  unsigned long long n = 0, blk = 0;
  for (;; ++blk) {
    if (std::fseek(fp, long(blk * step * buf.size() * sizeof(R)), SEEK_SET) != 0) break;
    const size_t got = std::fread(buf.data(), sizeof(R), buf.size(), fp);
    for (size_t i = 0; i < got; ++i) f(buf[i]);
    n += got;
    if (got < buf.size()) break;
  }
  std::fclose(fp);
  return n;
}

int main(int argc, char** argv) {
  if (argc < 3) return 2;
  const std::string dir = argv[1];
  const unsigned long long step = std::strtoull(argv[2], nullptr, 10);
  unsigned long long bad = 0;
  // ---- triplets : aigu (exact), centre q3 dans la boite (filtre)
  unsigned long long t_acute = 0, t_in = 0, t_unc = 0, t_exactdbl = 0;
  const unsigned long long nt = sample<RecT>((dir + "/triples.bin").c_str(), step, [&](const RecT& r) {
    const P3 a{r.a[0], r.a[1], r.a[2]}, b{r.b[0], r.b[1], r.b[2]}, c{r.c[0], r.c[1], r.c[2]};
    if (!geom::acute(a, b, c)) return;
    ++t_acute;
    geom::Center ctr;
    geom::center3(a, b, c, ctr);
    const bool in = box_exact(a, ctr, r.lo, r.hi);
    t_in += in;
    const double u[3] = {double(b.x - a.x), double(b.y - a.y), double(b.z - a.z)};
    const double v[3] = {double(c.x - a.x), double(c.y - a.y), double(c.z - a.z)};
    const double uu = u[0] * u[0] + u[1] * u[1] + u[2] * u[2], vv = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    const double w[3] = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
    const double t[3] = {uu * v[0] - vv * u[0], uu * v[1] - vv * u[1], uu * v[2] - vv * u[2]};
    const double N[3] = {t[1] * w[2] - t[2] * w[1], t[2] * w[0] - t[0] * w[2], t[0] * w[1] - t[1] * w[0]};
    const double MN[3] = {std::fabs(t[1] * w[2]) + std::fabs(t[2] * w[1]), std::fabs(t[2] * w[0]) + std::fabs(t[0] * w[2]),
                          std::fabs(t[0] * w[1]) + std::fabs(t[1] * w[0])};
    const double D = 2 * (w[0] * w[0] + w[1] * w[1] + w[2] * w[2]);
    const double ap[3] = {double((i64(r.a[0]) << kT) - r.lo[0]), double((i64(r.a[1]) << kT) - r.lo[1]), double((i64(r.a[2]) << kT) - r.lo[2])};
    const double h[3] = {double(r.hi[0] - r.lo[0]), double(r.hi[1] - r.lo[1]), double(r.hi[2] - r.lo[2])};
    const int f = box_filter(ap, h, N, MN, D);
    if (f == 2) ++t_unc;
    else if ((f == 1) != in) ++bad;
    // tout exact en binaire64 ? (majorant de |a' D| + 64 |N| et de h D sous 2^53)
    double mx = 0;
    for (int k = 0; k < 3; ++k) mx = std::max(mx, std::max(std::fabs(ap[k]) * D + 64.0 * MN[k], h[k] * D));
    t_exactdbl += mx < 0x1p53;
  });
  // ---- quadruplets : centre q4 dans la boite (filtre), interieur strict (filtre, seuils 2^-42 et 2^-44)
  unsigned long long q_in = 0, q_box_unc = 0, q_box_exactdbl = 0, q_ins = 0, q_ins_unc42 = 0, q_ins_unc44 = 0, q_deg = 0, q_judged = 0;
  unsigned long long q_ins_on_inbox = 0, q_ins_unc42_on_inbox = 0, q_insc_unc = 0, q_insc_unc_inbox = 0;
  const unsigned long long nq = sample<RecQ>((dir + "/quads.bin").c_str(), step, [&](const RecQ& r) {
    const P3 a{r.a[0], r.a[1], r.a[2]}, b{r.b[0], r.b[1], r.b[2]}, c{r.c[0], r.c[1], r.c[2]}, d{r.d[0], r.d[1], r.d[2]};
    geom::Center ctr;
    const bool nd = geom::center4(a, b, c, d, ctr);
    q_deg += !nd;
    const P3* t4[4] = {&a, &b, &c, &d};
    const bool in = nd && box_exact(a, ctr, r.lo, r.hi);
    const bool ins = nd && geom::strictly_inside_tetra(t4, a, ctr);
    q_in += in;
    q_ins += ins;
    q_judged += in && ins;
    const double u[3] = {double(b.x - a.x), double(b.y - a.y), double(b.z - a.z)};
    const double v[3] = {double(c.x - a.x), double(c.y - a.y), double(c.z - a.z)};
    const double s[3] = {double(d.x - a.x), double(d.y - a.y), double(d.z - a.z)};
    const double uu = u[0] * u[0] + u[1] * u[1] + u[2] * u[2], vv = v[0] * v[0] + v[1] * v[1] + v[2] * v[2], ss = s[0] * s[0] + s[1] * s[1] + s[2] * s[2];
    const double uv = u[0] * v[0] + u[1] * v[1] + u[2] * v[2], us = u[0] * s[0] + u[1] * s[1] + u[2] * s[2], vs = v[0] * s[0] + v[1] * s[1] + v[2] * s[2];
    // centre (Cramer)
    const double c1[3] = {v[1] * s[2] - v[2] * s[1], v[2] * s[0] - v[0] * s[2], v[0] * s[1] - v[1] * s[0]};
    const double c2[3] = {s[1] * u[2] - s[2] * u[1], s[2] * u[0] - s[0] * u[2], s[0] * u[1] - s[1] * u[0]};
    const double c3[3] = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
    const double det = u[0] * c1[0] + u[1] * c1[1] + u[2] * c1[2];
    if (det != 0) {
      const double sg = det > 0 ? 1.0 : -1.0, D = 2 * std::fabs(det);
      double N[3], MN[3];
      for (int k = 0; k < 3; ++k) {
        N[k] = sg * (uu * c1[k] + vv * c2[k] + ss * c3[k]);
        MN[k] = uu * std::fabs(c1[k]) + vv * std::fabs(c2[k]) + ss * std::fabs(c3[k]);
      }
      const double ap[3] = {double((i64(r.a[0]) << kT) - r.lo[0]), double((i64(r.a[1]) << kT) - r.lo[1]), double((i64(r.a[2]) << kT) - r.lo[2])};
      const double h[3] = {double(r.hi[0] - r.lo[0]), double(r.hi[1] - r.lo[1]), double(r.hi[2] - r.lo[2])};
      const int f = box_filter(ap, h, N, MN, D);
      if (f == 2) ++q_box_unc;
      else if ((f == 1) != in) ++bad;
      double mx = 0;
      for (int k = 0; k < 3; ++k) mx = std::max(mx, std::max(std::fabs(ap[k]) * D + 64.0 * MN[k], h[k] * D));
      q_box_exactdbl += mx < 0x1p53;
      // interieur strict, forme retenue (reemploi du centre de Cramer, numerateur non normalise Np = sg N)
      const double Np[3] = {sg * N[0], sg * N[1], sg * N[2]};
      const double* cj[3] = {c1, c2, c3};
      double val[3], mag[3];
      for (int j = 0; j < 3; ++j) {
        val[j] = Np[0] * cj[j][0] + Np[1] * cj[j][1] + Np[2] * cj[j][2];
        mag[j] = MN[0] * std::fabs(cj[j][0]) + MN[1] * std::fabs(cj[j][1]) + MN[2] * std::fabs(cj[j][2]);
      }
      const double T2 = 2 * det * det, L = T2 - val[0] - val[1] - val[2], ML = mag[0] + mag[1] + mag[2] + T2;
      const double ta = 0x1p-48, tl = 0x1p-47 * ML;
      const bool cin = val[0] > ta * mag[0] && val[1] > ta * mag[1] && val[2] > ta * mag[2] && L > tl;
      const bool cout = val[0] < -ta * mag[0] || val[1] < -ta * mag[1] || val[2] < -ta * mag[2] || L < -tl;
      if (!cin && !cout) { ++q_insc_unc; q_insc_unc_inbox += in; }
      else if (cin != ins) ++bad;
    }
    // interieur (Gram)
    const double g11 = vv * ss - vs * vs, g22 = uu * ss - us * us, g33 = uu * vv - uv * uv;
    const double g12 = vs * us - uv * ss, g13 = uv * vs - vv * us, g23 = us * uv - uu * vs;
    const double A = g11 * uu + g12 * vv + g13 * ss, B = g12 * uu + g22 * vv + g23 * ss, C = g13 * uu + g23 * vv + g33 * ss;
    const double D2 = 2 * (uu * g11 + uv * g12 + us * g13);
    const double L0 = D2 - A - B - C;
    const double m = std::max(uu, std::max(vv, ss)), m3 = m * m * m;
    for (int pass = 0; pass < 2; ++pass) {
      const double tau = (pass == 0 ? 0x1p-42 : 0x1p-44) * m3;
      const bool cin = A > tau && B > tau && C > tau && L0 > tau;
      const bool cout = A < -tau || B < -tau || C < -tau || L0 < -tau;
      if (!cin && !cout) {
        if (pass == 0) { ++q_ins_unc42; q_ins_unc42_on_inbox += in; } else ++q_ins_unc44;
      } else if (pass == 0 && cin != ins) ++bad;
    }
    q_ins_on_inbox += in && ins;
  });
  std::printf("triplets echantillonnes %llu | aigus %llu | centre dans la boite %llu | indecis du filtre de boite q3 %llu (%.5f %% des aigus) | boite q3 exacte en binaire64 %.2f %% des aigus\n",
              nt, t_acute, t_in, t_unc, 100.0 * double(t_unc) / double(std::max<unsigned long long>(t_acute, 1)), 100.0 * double(t_exactdbl) / double(std::max<unsigned long long>(t_acute, 1)));
  std::printf("quadruplets echantillonnes %llu | degeneres %llu | centre dans la boite %llu | interieurs %llu | juges %llu | indecis boite q4 %llu (%.5f %%) | boite q4 exacte en binaire64 %.2f %% | indecis interieur seuil 2^-42 : %llu (%.4f %%), dont dans la boite %llu ; seuil 2^-44 : %llu (%.4f %%)\n",
              nq, q_deg, q_in, q_ins, q_judged, q_box_unc, 100.0 * double(q_box_unc) / double(nq), 100.0 * double(q_box_exactdbl) / double(nq), q_ins_unc42,
              100.0 * double(q_ins_unc42) / double(nq), q_ins_unc42_on_inbox, q_ins_unc44, 100.0 * double(q_ins_unc44) / double(nq));
  std::printf("interieur, forme retenue (centre de Cramer, seuils 2^-48 M et 2^-47 M) : indecis %llu (%.5f %%), dont centre dans la boite %llu\n", q_insc_unc,
              100.0 * double(q_insc_unc) / double(nq), q_insc_unc_inbox);
  std::printf("decisions certaines contredites par l'exact : %llu\n", bad);
  return bad ? 1 : 0;
}
