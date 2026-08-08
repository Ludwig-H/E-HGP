// MorseHGP3D v2 — quantification et orchestration.
//
// Domaine contractuel : le nuage QUANTIFIE. La quantification sur la grille
// entiere [0, 2^kCoordBits) est une transformation d'entree explicite, publiee
// avec son echelle et son origine ; elle n'est pas un repli exact vis-a-vis des
// binary64 d'origine (cf. WARNING_AUDIT_COMPLETUDE.md §4). Tous les resultats
// portent sur le nuage quantifie et sur lui seul.

#include "mhgp/mhgp.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace mhgp {

std::vector<P3> quantize(const std::vector<double>& xyz, double* scale_out,
                         double* origin_out) {
  const size_t n = xyz.size() / 3;
  std::vector<P3> out(n);
  if (n == 0) return out;
  double lo[3] = {xyz[0], xyz[1], xyz[2]};
  double hi[3] = {xyz[0], xyz[1], xyz[2]};
  for (size_t i = 0; i < n; ++i)
    for (int d = 0; d < 3; ++d) {
      lo[d] = std::min(lo[d], xyz[3 * i + static_cast<size_t>(d)]);
      hi[d] = std::max(hi[d], xyz[3 * i + static_cast<size_t>(d)]);
    }
  double span = 0.0;
  for (int d = 0; d < 3; ++d) span = std::max(span, hi[d] - lo[d]);
  if (span <= 0.0) span = 1.0;
  const double scale = static_cast<double>(kCoordMax) / span;
  for (size_t i = 0; i < n; ++i) {
    for (int d = 0; d < 3; ++d) {
      const double v = (xyz[3 * i + static_cast<size_t>(d)] - lo[d]) * scale;
      const i64 q = static_cast<i64>(std::llround(v));
      const i64 c = std::min<i64>(kCoordMax, std::max<i64>(0, q));
      if (d == 0) out[i].x = c;
      else if (d == 1) out[i].y = c;
      else out[i].z = c;
    }
  }
  if (scale_out) *scale_out = scale;
  if (origin_out) { origin_out[0] = lo[0]; origin_out[1] = lo[1]; origin_out[2] = lo[2]; }
  return out;
}

Result run(const std::vector<P3>& pts, const Options& opt) {
  using clock = std::chrono::steady_clock;
  Result r;
  r.n = static_cast<i32>(pts.size());
  r.k_max = opt.k_max;
  const int s_max = std::min<int>(opt.k_max + 1, static_cast<int>(pts.size()));

  const auto t0 = clock::now();
  r.catalogue = build_catalogue(pts, s_max, opt);
  const auto t1 = clock::now();
  r.timings.catalogue_ms =
      std::chrono::duration<double, std::milli>(t1 - t0).count();

  for (size_t i = 0; i < r.catalogue.certified.size(); ++i)
    if (!r.catalogue.certified[i]) ++r.uncertified_points;

  // Une coquille cospherique sort du domaine declare (DESIGN.md §6.4) : la
  // sphere correspondante a ete rejetee, donc la fusion qu'elle porte manque a
  // toutes les forets. Rien ne peut etre publie comme autoritaire.
  r.out_of_declared_domain =
      r.catalogue.degenerate_shells > 0 || r.catalogue.shell_anomalies > 0;

  // Un catalogue censure ne doit pas alimenter un constructeur de foret
  // autoritaire (WARNING_AUDIT_IMPLEMENTATION_2 §4).
  if (r.uncertified_points > 0 || r.out_of_declared_domain) {
    r.forests_suppressed = true;
  } else {
    const int k_lo = opt.all_orders ? 1 : opt.k_max;
    for (int k = k_lo; k <= opt.k_max; ++k) {
      Forest f = build_forest(pts, r.catalogue, k);
      if (!f.authoritative) {
        // une foret partielle ne doit jamais atteindre un consommateur
        r.forests.clear();
        r.forests_suppressed = true;
        r.censored_orders.push_back(k);
        break;
      }
      r.forests.push_back(std::move(f));
    }
  }
  const auto t2 = clock::now();
  r.timings.forest_ms =
      std::chrono::duration<double, std::milli>(t2 - t1).count();
  r.timings.total_ms =
      std::chrono::duration<double, std::milli>(t2 - t0).count();
  return r;
}

}  // namespace mhgp
