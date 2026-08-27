// MorseHGP3D v5 — PORTE PERMANENTE DU KERNEL AFFINE (src/pipeline/float_filter.hpp).
//
// (1) IDENTITE EXHAUSTIVE : pour toute ancre (a,b), tout seed x et tout site z
//     de petits nuages (z ∈ {a,b,x} compris), avec u = 2z−a−b, q = |u|²−D²,
//     N = W − G·d : L = G·q − 2·u·N verifie L == 4·q3_power(f3, z) et
//     L ≡ 0 (mod 4) — la division P = L/4 du cœur de seed est exacte. Chaque
//     decision flottante certifiee (affine_l_hat / affine_l_bound, fonctions de
//     production) est recoupee contre le signe exact. Nuages : uniform et
//     eight_clusters aux emprises par defaut (double exact), uniform a
//     coord=50000 (G et N inexacts en binaire64) et la fixture cocirculaire
//     ×1999 (sites a L = 0 exact : plancher de replis).
// (2) TEMOIN DE FORTE ANNULATION ±, constantes GRAVEES : G = 2^67 − 12345,
//     u = (131071,0,0), q = 2^35 + 7, N0 = floor(G·q / (2·u0)) ; deux termes
//     ~2^102 s'annulent a L = +216577 ; variante N0+1 : L = −45565. binaire64
//     rend le meme L^ pour les deux : la borne saine (E ~ 2^55) declare
//     INCERTAIN les deux ; la borne retrecie du mutant `float-small-threshold`
//     (×2^-20) certifie le signe du bruit — en desaccord avec un exact -> tue.
// (3) TEMOIN D'INTERVALLES DE JUNG, constantes gravees : lh = −2^60, e = 2^55 ;
//     (J, B) = (2^40, 2^20) -> +1 ; (2^80, 2^25) -> −1 ; (2^59, 2^29) a cheval
//     -> 0 (repli). Mutant `jung-swap-bounds` : (3) rend +1 -> tue.
// Codes : 0 conforme, 3 invariant/plancher, 4 mutant tue.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/lanes/q3.hpp"
#include "../src/pipeline/float_filter.hpp"
#include "../src/tree/cloud_index.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool m_small = MHGP5_MUTANT("float-small-threshold");  // mutant de porte
  const double shrink = m_small ? 0x1p-20 : 1.0;
  u64 ids = 0, viol = 0, cneg = 0, cpos = 0, fb = 0, mm = 0;
  std::vector<std::vector<P3>> clouds;
  clouds.push_back(make_family_cloud(CloudFamily::kUniform, 40, cloud_family_default_coord(CloudFamily::kUniform, 40), 3));
  clouds.push_back(make_family_cloud(CloudFamily::kEightClusters, 32, cloud_family_default_coord(CloudFamily::kEightClusters, 32), 3));
  clouds.push_back(make_family_cloud(CloudFamily::kUniform, 28, 50000, 5));
  {
    std::vector<P3> fx = {{11, 7, 10}, {20, 10, 10}, {15, 15, 10}, {11, 13, 10}, {18, 14, 10}, {12, 14, 10}, {15, 10, 16}};
    for (P3& p : fx) { p.x *= 1999; p.y *= 1999; p.z *= 1999; }
    clouds.push_back(std::move(fx));
  }
  std::vector<i64> su0, su1, su2, sq;
  for (const auto& cl : clouds) {
    const CloudIndex ix = build_cloud_index(cl);
    const size_t m = (size_t)ix.unique_count();
    if (!ix.valid || m != cl.size()) return 3;
    su0.resize(m); su1.resize(m); su2.resize(m); sq.resize(m);
    for (size_t ua = 0; ua + 1 < m; ++ua)
      for (size_t ub = ua + 1; ub < m; ++ub) {
        const P3& pa = ix.upos[ua];
        const P3& pb = ix.upos[ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        i64 qmax = 1, umax = 1;
        const i64 sx = pa.x + pb.x, sy = pa.y + pb.y, sz = pa.z + pb.z;
        for (size_t i = 0; i < m; ++i) {
          const P3& pz = ix.upos[i];
          const i64 u0 = 2 * pz.x - sx, u1 = 2 * pz.y - sy, u2 = 2 * pz.z - sz;
          const i64 qz = u0 * u0 + u1 * u1 + u2 * u2 - D2;
          su0[i] = u0; su1[i] = u1; su2[i] = u2; sq[i] = qz;
          qmax = std::max(qmax, qz < 0 ? -qz : qz);
          umax = std::max({umax, u0 < 0 ? -u0 : u0, u1 < 0 ? -u1 : u1, u2 < 0 ? -u2 : u2});
        }
        for (size_t x = 0; x < m; ++x) {
          if (x == ua || x == ub) continue;
          const Q3Form f3 = q3_form(pa, pb, ix.upos[x]);
          const i128 N0 = f3.w[0] - f3.g * (i128)(pb.x - pa.x);
          const i128 N1 = f3.w[1] - f3.g * (i128)(pb.y - pa.y);
          const i128 N2 = f3.w[2] - f3.g * (i128)(pb.z - pa.z);
          const double Gd = (double)f3.g, Nd0 = (double)N0, Nd1 = (double)N1, Nd2 = (double)N2;
          const double bnd = affine_l_bound(Gd, Nd0, Nd1, Nd2, (double)qmax, (double)umax) * shrink;
          for (size_t iz = 0; iz < m; ++iz) {
            const i128 L = f3.g * (i128)sq[iz] - 2 * ((i128)su0[iz] * N0 + (i128)su1[iz] * N1 + (i128)su2[iz] * N2);
            ++ids;
            if (L != 4 * q3_power(f3, ix.upos[iz])) ++viol;
            if (((u64)(u128)L & 3u) != 0) ++viol;
            const double Lh = affine_l_hat(Gd, Nd0, Nd1, Nd2, (double)su0[iz], (double)su1[iz], (double)su2[iz], (double)sq[iz]);
            if (Lh < -bnd) { ++cneg; if (!(L < 0)) ++mm; }
            else if (Lh > bnd) { ++cpos; if (!(L > 0)) ++mm; }
            else ++fb;
          }
        }
      }
  }
  // (2) Temoin de forte annulation.
  u64 wit_unc = 0, wit_mm = 0;
  bool wit_fixture_ok = true;
  {
    const i128 gw = ((i128)1 << 67) - 12345;
    const i64 u0 = 131071, qw = (1LL << 35) + 7;
    const i128 n0 = (gw * (i128)qw) / (2 * (i128)u0);
    for (int variant = 0; variant < 2; ++variant) {
      const i128 nv = n0 + variant;
      const i128 L = gw * (i128)qw - 2 * (i128)u0 * nv;
      if (L != (variant ? (i128)-45565 : (i128)216577)) wit_fixture_ok = false;
      const double gd = (double)gw, nd = (double)nv;
      const double lh = affine_l_hat(gd, nd, 0.0, 0.0, (double)u0, 0.0, 0.0, (double)qw);
      const double bnd = affine_l_bound(gd, nd, 0.0, 0.0, (double)qw, (double)u0) * shrink;
      if (lh < -bnd) { if (!(L < 0)) ++wit_mm; }
      else if (lh > bnd) { if (!(L > 0)) ++wit_mm; }
      else ++wit_unc;
    }
  }
  // (3) Temoin d'intervalles de Jung (le mutant jung-swap-bounds vit dans src/).
  u64 jw_bad = 0;
  {
    const double lh = -0x1p60, e = 0x1p55;
    const struct { double j; i64 b; int want; } jw[3] = {{0x1p40, (i64)1 << 20, 1}, {0x1p80, (i64)1 << 25, -1}, {0x1p59, (i64)1 << 29, 0}};
    for (const auto& w : jw)
      if (jung_interval_sign(lh, e, w.j * (1.0 - kJungGuard), w.j * (1.0 + kJungGuard), w.b) != w.want) ++jw_bad;
  }
  const bool floors = ids >= 1000000 && cneg >= 100 && cpos >= 100 && fb >= 1;
  std::printf("q3_affine_gate identites=%llu violations=%llu certifies_neg=%llu certifies_pos=%llu replis=%llu desaccords=%llu "
              "temoin_incertains=%llu temoin_desaccords=%llu jung_temoin_faux=%llu planchers=%d fixture=%d\n",
              (unsigned long long)ids, (unsigned long long)viol, (unsigned long long)cneg, (unsigned long long)cpos,
              (unsigned long long)fb, (unsigned long long)mm, (unsigned long long)wit_unc, (unsigned long long)wit_mm,
              (unsigned long long)jw_bad, (int)floors, (int)wit_fixture_ok);
  const bool killed = mm != 0 || wit_mm != 0 || jw_bad != 0;
  if (!inject.empty()) {
    if (killed) { std::fprintf(stderr, "MUTANT TUE : %s\n", inject.c_str()); return 4; }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s survivant\n", inject.c_str());
    return 3;
  }
  if (viol != 0 || killed || !floors || !wit_fixture_ok || wit_unc != 2) return 3;
  std::printf("q3_affine_gate OK\n");
  return 0;
}
