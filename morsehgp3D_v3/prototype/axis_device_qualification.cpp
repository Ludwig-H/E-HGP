// MorseHGP3D v3 — QUALIFICATION HOTE/DEVICE DE `Q4SeedAxisTopR4`.
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference_et_cuda,
//         profile=quantized_u16_input_only, mode=diagnostic_exact_borne,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// LA QUESTION POSEE, ET SON PERIMETRE EXACT
//
// A `smax=6`, `n=6000`, le noyau d'axe divise les candidats par cinquante-neuf
// sans deplacer le mur de temps : la selection balaie encore tous les sites de
// chaque `Q4Seed3`, et ce balayage devient le terme dominant. Cette
// qualification met CE terme sur le GPU et mesure deux choses :
//
//   1. la PARITE : chaque champ de chaque verdict, hote contre device, bit a
//      bit. Un seul ecart et le binaire rend 1 ;
//   2. le DEBIT : millisecondes du kernel seul, sans transfert, avec un warmup
//      non chronometre.
//
// CE QU'ELLE NE MESURE PAS. Ni la chaine complete, ni un `warm_e2e`, ni un SLO,
// ni le contrat. Le lot est CONSTRUIT SUR L'HOTE et transfere ; le transfert est
// publie separement et n'est pas soustrait. Un debit de kernel n'est pas un
// debit de bout en bout, et ce fichier ne pretend pas le contraire.
//
// CODES DE SORTIE
//   0  parite et mesure    1  desaccord hote/device    2  refus avant calcul
//   3  plancher de couverture
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "prototype/axis_device_job.hpp"
#include "prototype/cloud_families.hpp"

namespace {

using mhgp::i128;
using mhgp::i64;

[[noreturn]] void refuse(const std::string& r) {
  std::fprintf(stderr, "REFUS: %s\n", r.c_str());
  std::exit(2);
}

struct Pt { i64 c[3] = {0, 0, 0}; };

inline i64 d2p(const Pt& a, const Pt& b) {
  const i64 x = a.c[0] - b.c[0], y = a.c[1] - b.c[1], z = a.c[2] - b.c[2];
  return x * x + y * y + z * z;
}

inline bool fuseau4(i128 D2, i128 nu, i128 dot) {
  const i128 g = D2 - nu;
  if (g <= 0) return false;
  return g * g > 2 * (D2 * nu - dot * dot);
}

inline bool aigu(const Pt& a, const Pt& b, const Pt& x) {
  auto dot = [](const Pt& o, const Pt& p, const Pt& q) -> i128 {
    i128 s = 0;
    for (int i = 0; i < 3; ++i) s += (i128)(p.c[i] - o.c[i]) * (q.c[i] - o.c[i]);
    return s;
  };
  return dot(a, b, x) > 0 && dot(b, a, x) > 0 && dot(x, a, b) > 0;
}

std::vector<Pt> nuage(const std::string& f, long long n, long long seed) {
  mhgp3v::CloudFamily fam;
  if (f == "uniform") fam = mhgp3v::CloudFamily::kUniform;
  else if (f == "eight_clusters") fam = mhgp3v::CloudFamily::kEightClusters;
  else if (f == "terrain") fam = mhgp3v::CloudFamily::kTerrain;
  else refuse("famille inconnue : " + f);
  const int coord = mhgp3v::cloud_family_default_coord(fam, (int)n);
  const auto b = mhgp3v::make_family_cloud(fam, (int)n, coord, seed);
  std::vector<Pt> P;
  P.reserve(b.size());
  for (const auto& q : b) P.push_back(Pt{{q.x, q.y, q.z}});
  return P;
}

}  // namespace

int main(int argc, char** argv) {
  std::string family = "uniform";
  long long n = 3000, seed = 1, smax = 6, max_seeds = 400000, max_sites = 200000000;
  double dmax_esp = 10.0;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](const char* p) { return a.substr(std::strlen(p)); };
    if (a.rfind("--family=", 0) == 0) family = val("--family=");
    else if (a.rfind("--points=", 0) == 0) n = atoll(val("--points=").c_str());
    else if (a.rfind("--seed=", 0) == 0) seed = atoll(val("--seed=").c_str());
    else if (a.rfind("--smax=", 0) == 0) smax = atoll(val("--smax=").c_str());
    else if (a.rfind("--max-seeds=", 0) == 0) max_seeds = atoll(val("--max-seeds=").c_str());
    else if (a.rfind("--max-sites=", 0) == 0) max_sites = atoll(val("--max-sites=").c_str());
    else if (a.rfind("--dmax-espacements=", 0) == 0)
      dmax_esp = atof(val("--dmax-espacements=").c_str());
    else refuse("option inconnue : " + a);
  }
  if (n < 100 || n > 200000) refuse("--points hors domaine");
  if (smax < 4 || smax > 18) refuse("--smax hors domaine");
  if (max_seeds < 1 || max_seeds > 20000000) refuse("--max-seeds hors domaine");

  const std::vector<Pt> P = nuage(family, n, seed);
  const int N = (int)P.size();
  const int r4 = (int)smax - 3;
  i64 lo[3] = {INT64_MAX, INT64_MAX, INT64_MAX}, hi[3] = {INT64_MIN, INT64_MIN, INT64_MIN};
  for (const Pt& p : P)
    for (int c = 0; c < 3; ++c) {
      if (p.c[c] < lo[c]) lo[c] = p.c[c];
      if (p.c[c] > hi[c]) hi[c] = p.c[c];
    }
  double vol = 1.0;
  for (int c = 0; c < 3; ++c) vol *= (double)std::max<i64>(1, hi[c] - lo[c]);
  const double esp = std::cbrt(vol / (double)N);
  const i64 dmax = (i64)(dmax_esp * esp + 0.5);

  // --- Construction du lot sur l'hote. Un balayage direct, sans grille : ce
  // n'est pas le sujet de la mesure, seulement sa matiere.
  std::vector<i64> coords((size_t)3 * N);
  for (int i = 0; i < N; ++i)
    for (int c = 0; c < 3; ++c) coords[(size_t)3 * i + c] = P[(size_t)i].c[c];
  std::vector<int> seeds, offsets, sites;
  offsets.push_back(0);
  std::vector<int> inner, lens;
  bool cap_atteint = false;
  for (int ia = 0; ia < N && !cap_atteint; ++ia)
    for (int jb = ia + 1; jb < N && !cap_atteint; ++jb) {
      const i64 D2 = d2p(P[(size_t)ia], P[(size_t)jb]);
      if (D2 == 0 || D2 > dmax * dmax) continue;
      const i128 ex = P[(size_t)jb].c[0] - P[(size_t)ia].c[0];
      const i128 ey = P[(size_t)jb].c[1] - P[(size_t)ia].c[1];
      const i128 ez = P[(size_t)jb].c[2] - P[(size_t)ia].c[2];
      long long f4 = 0;
      inner.clear(); lens.clear();
      for (int z = 0; z < N; ++z) {
        if (z == ia || z == jb) continue;
        const i128 ux = 2 * (i128)P[(size_t)z].c[0] - (P[(size_t)ia].c[0] + P[(size_t)jb].c[0]);
        const i128 uy = 2 * (i128)P[(size_t)z].c[1] - (P[(size_t)ia].c[1] + P[(size_t)jb].c[1]);
        const i128 uz = 2 * (i128)P[(size_t)z].c[2] - (P[(size_t)ia].c[2] + P[(size_t)jb].c[2]);
        const i128 nu = ux * ux + uy * uy + uz * uz;
        if (nu > 4 * (i128)D2) continue;
        inner.push_back(z);
        if (fuseau4(D2, nu, ux * ex + uy * ey + uz * ez)) ++f4;
        if (d2p(P[(size_t)ia], P[(size_t)z]) <= D2 && d2p(P[(size_t)jb], P[(size_t)z]) <= D2)
          lens.push_back(z);
      }
      if (f4 >= r4) continue;
      for (int ip : lens) {
        if (!aigu(P[(size_t)ia], P[(size_t)jb], P[(size_t)ip])) continue;
        if ((long long)seeds.size() / 3 >= max_seeds) { cap_atteint = true; break; }
        if ((long long)sites.size() + (long long)inner.size() > max_sites) {
          cap_atteint = true;
          break;
        }
        seeds.push_back(ia); seeds.push_back(jb); seeds.push_back(ip);
        for (int z : inner) if (z != ip) sites.push_back(z);
        offsets.push_back((int)sites.size());
      }
    }
  const int nseeds = (int)seeds.size() / 3;
  if (nseeds == 0) { std::fprintf(stderr, "PLANCHER: aucun Q4Seed3\n"); return 3; }

  mhgp3v::axisdev::AxisJob job;
  job.coords = coords.data();
  job.npoints = N;
  job.seed = seeds.data();
  job.site_offset = offsets.data();
  job.site_id = sites.data();
  job.nseeds = nseeds;
  job.r4 = r4;

  // --- Hote.
  std::vector<mhgp3v::axisdev::SeedOut> hote((size_t)nseeds);
  const auto t0 = std::chrono::steady_clock::now();
  for (int i = 0; i < nseeds; ++i)
    mhgp3v::axisdev::evaluate_seed(job, i, &hote[(size_t)i]);
  const auto t1 = std::chrono::steady_clock::now();
  const double ms_hote =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(t1 - t0).count();

  const long long sites_total = (long long)sites.size();
  std::printf("axis_device : famille=%s n=%d smax=%lld r4=%d seeds=%d sites=%lld"
              " sites_moyens=%.1f cap=%d\n",
              family.c_str(), N, smax, r4, nseeds, sites_total,
              (double)sites_total / nseeds, (int)cap_atteint);
  std::printf("  hote : %.2f ms, %.2f Mseeds/s, %.2f Msites/s\n", ms_hote,
              (double)nseeds / (ms_hote * 1000.0),
              (double)sites_total / (ms_hote * 1000.0));

#ifdef MHGP3V_AXIS_CUDA
  std::vector<mhgp3v::axisdev::SeedOut> dev((size_t)nseeds);
  double ms_dev = 0.0;
  char err[512] = {0};
  if (!mhgp3v::axisdev::launch_axis_on_gpu(job, dev.data(), &ms_dev, err, (int)sizeof(err))) {
    std::fprintf(stderr, "REFUS: CUDA : %s\n", err);
    return 2;
  }
  long long ecarts = 0;
  for (int i = 0; i < nseeds; ++i) {
    const auto& h = hote[(size_t)i];
    const auto& d = dev[(size_t)i];
    bool egal = h.verdict == d.verdict && h.p == d.p && h.k == d.k &&
                h.n_entrants == d.n_entrants && h.n_sortants == d.n_sortants &&
                h.prof_min == d.prof_min && h.deborde_sortie == d.deborde_sortie;
    for (int t = 0; t < mhgp3v::axisdev::kCapSortie && egal; ++t)
      egal = h.entrants[t] == d.entrants[t] && h.sortants[t] == d.sortants[t];
    if (!egal) ++ecarts;
  }
  std::printf("  device : %.2f ms, %.2f Mseeds/s, %.2f Msites/s, acceleration=%.1fx\n",
              ms_dev, (double)nseeds / (ms_dev * 1000.0),
              (double)sites_total / (ms_dev * 1000.0), ms_hote / ms_dev);
  std::printf("  parite : ecarts=%lld sur %d verdicts\n", ecarts, nseeds);
  if (ecarts > 0) {
    std::fprintf(stderr, "DESACCORD: %lld verdicts different entre hote et device\n", ecarts);
    return 1;
  }
#else
  std::printf("  device : absent (compile sans MHGP3V_ENABLE_CUDA)\n");
#endif
  return 0;
}
