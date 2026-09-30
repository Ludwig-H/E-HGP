// Audit-only singleton of a valid public point dendrogram, linked to a pinned
// existing corrected-copy library. No engine or scene generator is rebuilt.
#include <cmath>
#include <iostream>
#include "head/head.hpp"

using namespace mhgp10;

int inspect(const char* name, double level, u32 min_size, bool allow_root) {
  PointDendrogram d;
  d.level = {level};
  d.node_rank = {0};
  d.parent = {kNone};
  d.child_off = {0, 0};
  d.point_node = {0};
  d.point_rank = {0};
  d.point_weight = {1};
  ClusterParams p;
  p.min_cluster_size = min_size;
  p.z = 1.0;
  p.allow_single_cluster = allow_root;
  const Outcome vd = validate(d), vp = validate(p);
  if (!vd.ok() || !vp.ok()) return 2;
  const Clustering c = cluster(d, p);
  unsigned nonfinite_lambda = 0, nonfinite_stability = 0, nan_stability = 0;
  for (double x : c.tree.point_lambda) nonfinite_lambda += !std::isfinite(x);
  for (double x : c.tree.stability) {
    nonfinite_stability += !std::isfinite(x);
    nan_stability += std::isnan(x);
  }
  std::cout << "{\"case\":\"" << name << "\",\"validated\":true,\"mass\":1,\"mcs\":" << min_size
            << ",\"allow_root\":" << (allow_root ? "true" : "false")
            << ",\"point_lambda_nonfinite\":" << nonfinite_lambda
            << ",\"stability_nonfinite\":" << nonfinite_stability
            << ",\"stability_nan\":" << nan_stability
            << ",\"cluster_count\":" << c.tree.mass.size()
            << ",\"selected_count\":" << c.selected.size() << ",\"label\":" << c.label.at(0) << "}\n";
  const unsigned expected = level == 0.0 ? 1U : 0U;
  return nonfinite_lambda == expected && nonfinite_stability == expected && nan_stability == 0 ? 0 : 1;
}

int main() {
  int errors = 0;
  errors += inspect("positive_root_mcs5", 0.25, 5, false);
  errors += inspect("zero_root_mcs1", 0.0, 1, false);
  errors += inspect("zero_root_mcs5", 0.0, 5, false);
  errors += inspect("zero_root_mcs5_allow", 0.0, 5, true);
  errors += inspect("zero_root_mcs200", 0.0, 200, false);
  std::cout << "{\"audit_status\":\"" << (errors == 0 ? "ZERO_SINGLETON_REPRODUCED" : "UNEXPECTED")
            << "\",\"errors\":" << errors << ",\"engine_qualification\":false}\n";
  return errors == 0 ? 0 : 1;
}
