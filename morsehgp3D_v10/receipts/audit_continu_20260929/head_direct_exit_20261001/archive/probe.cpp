// Private causal API fixture: no point-cloud engine, Scene, FULL, GCP or GPU.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

#include "head/head.hpp"

using namespace mhgp10;

namespace {
int failures = 0;
void check(bool ok, const char* name) {
  if (!ok) {
    ++failures;
    std::fprintf(stderr, "ECHEC head_direct_exit %s\n", name);
  }
}
bool close(double got, double expected) {
  return std::isfinite(got) &&
         std::abs(got - expected) <= 16 * std::numeric_limits<double>::epsilon() *
                                          std::max(1.0, std::abs(expected));
}
void doubles(const std::vector<double>& xs) {
  std::printf("[");
  for (size_t i = 0; i < xs.size(); ++i) std::printf("%s%.17g", i ? "," : "", xs[i]);
  std::printf("]");
}
void ids(const std::vector<u32>& xs) {
  std::printf("[");
  for (size_t i = 0; i < xs.size(); ++i) std::printf("%s%u", i ? "," : "", xs[i]);
  std::printf("]");
}
void labels(const std::vector<i32>& xs) {
  std::printf("[");
  for (size_t i = 0; i < xs.size(); ++i) std::printf("%s%d", i ? "," : "", xs[i]);
  std::printf("]");
}
void expected_condensation(const CondensedTree& t) {
  check(t.outcome.ok(), "condense outcome");
  check(t.parent == std::vector<u32>{kNone, 0, 0}, "condensed parents");
  check(t.mass == std::vector<u64>{5, 2, 2}, "condensed masses");
  check(t.birth == std::vector<double>{0, 0.5, 0.5}, "condensed births");
  check(t.point_cluster == std::vector<u32>{1, 1, 2, 2, 0}, "point owners");
  check(t.node_cluster == std::vector<u32>{1, 2, 0}, "node owners");
  check(t.stability.size() == 3, "stability inventory");
  if (t.stability.size() == 3) {
    check(close(t.stability[0], 7.0 / 3.0), "root stability 7/3");
    check(close(t.stability[1], 1) && close(t.stability[2], 1), "children stability 1");
  }
  check(t.point_lambda.size() == 5, "lambda inventory");
  if (t.point_lambda.size() == 5) {
    for (size_t i = 0; i < 4; ++i) check(t.point_lambda[i] == 1, "leaf point lambda 1");
    check(close(t.point_lambda[4], 1.0 / 3.0), "direct root point lambda 1/3");
  }
}
}  // namespace

int main() {
  PointDendrogram d;
  d.level = {1, 4, 9};
  d.node_rank = {0, 0, 1};
  d.child_off = {0, 0, 0, 2};
  d.child_val = {0, 1};
  d.parent = {2, 2, kNone};
  d.point_node = {0, 0, 1, 1, 2};
  d.point_rank = {0, 0, 0, 0, 2};
  d.point_weight = {1, 1, 1, 1, 1};

  ClusterParams p;
  p.min_cluster_size = 2;
  p.z = 1;
  p.selection = Selection::eom;
  p.allow_single_cluster = true;

  check(d.point_rank[4] > d.node_rank[2], "direct point strictly after root birth");
  const Outcome structural = validate(d);
  const Outcome domain = validate(d, p);
  check(structural.ok(), "structural validation accepts late root entry");
  check(domain.ok(), "joint numeric domain accepts fixture");
  const CondensedTree t = condense(d, p);
  expected_condensation(t);
  const Clustering cl = cluster(d, p);
  check(cl.outcome.ok(), "cluster outcome");
  expected_condensation(cl.tree);
  check(cl.selected == std::vector<u32>{0}, "EOM selects root only: 7/3 > 1+1");
  check(cl.label == std::vector<i32>{0, 0, 0, 0, -1}, "four leaf points, direct root point noise");
  check(cl.cluster_label == std::vector<i32>{0, 0, 0}, "root ancestry labels");

  std::printf("{\"schema\":\"head-direct-exit-observation-v1\",");
  std::printf("\"structural_reason\":\"%.*s\",", static_cast<int>(reason_name(structural.reason).size()),
              reason_name(structural.reason).data());
  std::printf("\"domain_reason\":\"%.*s\",", static_cast<int>(reason_name(domain.reason).size()),
              reason_name(domain.reason).data());
  std::printf("\"condense_reason\":\"%.*s\",", static_cast<int>(reason_name(t.outcome.reason).size()),
              reason_name(t.outcome.reason).data());
  std::printf("\"cluster_reason\":\"%.*s\",", static_cast<int>(reason_name(cl.outcome.reason).size()),
              reason_name(cl.outcome.reason).data());
  std::printf("\"birth\":"); doubles(t.birth);
  std::printf(",\"stability\":"); doubles(t.stability);
  std::printf(",\"point_lambda\":"); doubles(t.point_lambda);
  std::printf(",\"point_cluster\":"); ids(t.point_cluster);
  std::printf(",\"selected\":"); ids(cl.selected);
  std::printf(",\"label\":"); labels(cl.label);
  std::printf(",\"cluster_label\":"); labels(cl.cluster_label);
  std::printf(",\"failures\":%d,\"contract_match\":%s}\n", failures, failures ? "false" : "true");
  return failures ? 1 : 0;
}
