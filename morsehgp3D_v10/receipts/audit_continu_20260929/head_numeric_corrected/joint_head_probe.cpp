#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "head/head.hpp"

using namespace mhgp10;

// Two components of equal positive mass, two points, one N-ary merge.
// No scene generator, catalogue, tower or archived H3 fixture is executed.
PointDendrogram make(double leaf_level, double merge_level, u32 weight, bool zero_merge) {
  PointDendrogram d;
  d.level = {leaf_level, merge_level};
  d.node_rank = {0, 0, zero_merge ? 0U : 1U};
  d.parent = {2, 2, kNone};
  d.child_off = {0, 0, 0, 2};
  d.child_val = {0, 1};
  d.point_node = {0, 1};
  d.point_rank = {0, 0};
  d.point_weight = {weight, weight};
  return d;
}

struct Observation {
  bool validated = false;
  unsigned nonfinite_lambda = 0, nonfinite_birth = 0, nonfinite_stability = 0, nan_stability = 0;
  std::vector<u32> selected;
  bool analytical_agreement = false;
};

Observation run(const char* name, double leaf_level, double merge_level, u32 weight, double z,
                bool zero_merge = false) {
  const PointDendrogram d = make(leaf_level, merge_level, weight, zero_merge);
  ClusterParams p;
  p.min_cluster_size = weight;
  p.z = z;
  p.allow_single_cluster = true;
  const Outcome vd = validate(d), vp = validate(p);
  Observation o;
  o.validated = vd.ok() && vp.ok();
  std::cout << "case=" << name << " z=" << z << " per_point_weight=" << weight
            << " total_integer_mass=" << u64(weight) * 2
            << " validate_d=" << status_name(vd.status()) << "/" << reason_name(vd.reason)
            << " validate_p=" << status_name(vp.status()) << "/" << reason_name(vp.reason);
  if (!o.validated) {
    std::cout << '\n';
    return o;
  }
  const Clustering cl = cluster(d, p);
  o.selected = cl.selected;
  for (double x : cl.tree.point_lambda) o.nonfinite_lambda += !std::isfinite(x);
  for (double x : cl.tree.birth) o.nonfinite_birth += !std::isfinite(x);
  for (double x : cl.tree.stability) {
    o.nonfinite_stability += !std::isfinite(x);
    o.nan_stability += std::isnan(x);
  }
  std::cout << " nonfinite_lambda=" << o.nonfinite_lambda
            << " nonfinite_birth=" << o.nonfinite_birth
            << " nonfinite_stability=" << o.nonfinite_stability
            << " nan_stability=" << o.nan_stability << " selected=";
  for (u32 x : cl.selected) std::cout << x << ',';
  std::cout << " labels=";
  for (i32 x : cl.label) std::cout << x << ',';
  if (!zero_merge) {
    // Independent closed-form condensation of THIS two-leaf tree. Only two
    // lambda values and three lifetimes: no product CondensedTree is reused.
    const long double a = std::pow(static_cast<long double>(leaf_level), -0.5L * z);
    const long double b = std::pow(static_cast<long double>(merge_level), -0.5L * z);
    const long double root_stability = 2.0L * weight * b;
    const long double child_sum = 2.0L * weight * (a - b);
    // Exact selection condition for z in {1,2} on positive inputs:
    // z=1: children win iff merge_level > 4*leaf_level;
    // z=2: children win iff merge_level > 2*leaf_level.
    // These multiplications are exact power-of-two scales in the five cases.
    const double threshold_factor = z == 1.0 ? 4.0 : 2.0;
    const bool child_wins_exact = merge_level > threshold_factor * leaf_level;
    const std::vector<u32> expected = child_wins_exact ? std::vector<u32>{1, 2} : std::vector<u32>{0};
    o.analytical_agreement = std::isfinite(root_stability) && std::isfinite(child_sum) && cl.selected == expected;
    std::cout << std::setprecision(25) << " oracle_root_stability=" << root_stability
              << " oracle_children_sum=" << child_sum << " expected_selected=";
    for (u32 x : expected) std::cout << x << ',';
    std::cout << " analytical_agreement=" << o.analytical_agreement;
  }
  std::cout << '\n';
  return o;
}

int main() {
  std::cout << std::scientific << std::setprecision(17)
            << "long_double_max_exponent=" << std::numeric_limits<long double>::max_exponent << '\n';
  const u32 w = std::numeric_limits<u32>::max();
  const Observation u18_z1 = run("u18_range_z1", 0.25, 0.75, w, 1.0);
  const Observation u18_z2 = run("u18_range_z2", 0.25, 0.75, w, 2.0);
  const Observation unweighted = run("finite_lambda_unweighted_z2", 1e-300, 9e-300, 1, 2.0);
  const Observation weighted = run("finite_lambda_weighted_z2", 1e-300, 9e-300, w, 2.0);
  const Observation zero = run("zero_merge_z1", 0.0, 0.25, 2, 1.0, true);
  const bool controls = u18_z1.validated && u18_z2.validated && unweighted.validated &&
                        u18_z1.analytical_agreement && u18_z2.analytical_agreement && unweighted.analytical_agreement &&
                        u18_z1.nonfinite_stability == 0 && u18_z2.nonfinite_stability == 0 &&
                        unweighted.nonfinite_stability == 0;
  const bool weighted_reproduced = weighted.validated && weighted.nonfinite_lambda == 0 &&
                                   weighted.nonfinite_birth == 0 && weighted.nonfinite_stability == 3 &&
                                   weighted.nan_stability == 0 && !weighted.analytical_agreement &&
                                   weighted.selected == std::vector<u32>{0};
  const bool zero_reproduced = zero.validated && zero.nan_stability == 2;
  std::cout << "controls_pass=" << controls << " weighted_overflow_and_EOM_mismatch_reproduced="
            << weighted_reproduced << " zero_merge_NaN_reproduced=" << zero_reproduced << '\n';
  return controls && weighted_reproduced && zero_reproduced ? 0 : 1;
}
