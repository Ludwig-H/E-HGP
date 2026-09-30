#include <cmath>
#include <iostream>
#include "head/head.hpp"

int main() {
  mhgp10::PointDendrogram d;
  d.level = {1e-300, 2e-300};
  d.node_rank = {0, 0, 1};
  d.parent = {2, 2, mhgp10::kNone};
  d.child_off = {0, 0, 0, 2};
  d.child_val = {0, 1};
  d.point_node = {0, 0, 1, 1};
  d.point_rank = {0, 0, 0, 0};
  d.point_weight = {1, 1, 1, 1};
  mhgp10::ClusterParams p;
  p.min_cluster_size = 2;
  p.z = 16;
  p.allow_single_cluster = true;
  auto vd = mhgp10::validate(d);
  auto vp = mhgp10::validate(p);
  std::cout << "validate_d=" << mhgp10::status_name(vd.status()) << "/" << mhgp10::reason_name(vd.reason)
            << " validate_p=" << mhgp10::status_name(vp.status()) << "/" << mhgp10::reason_name(vp.reason) << '\n';
  if (!vd.ok() || !vp.ok()) return 2;
  const auto result = mhgp10::cluster(d, p);
  unsigned nonfinite_birth = 0, nan_stability = 0, inf_stability = 0, nonfinite_exit = 0;
  for (double x : result.tree.birth) nonfinite_birth += !std::isfinite(x);
  for (double x : result.tree.stability) {
    nan_stability += std::isnan(x);
    inf_stability += std::isinf(x);
  }
  for (double x : result.tree.point_lambda) nonfinite_exit += !std::isfinite(x);
  std::cout << "result_has_status=false clusters=" << result.tree.parent.size()
            << " nonfinite_birth=" << nonfinite_birth << " nan_stability=" << nan_stability
            << " inf_stability=" << inf_stability << " nonfinite_exit=" << nonfinite_exit << " labels=";
  for (auto x : result.label) std::cout << x << ',';
  std::cout << '\n';
}
