#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::ExactPairSupportStopReason;
using morsehgp3d::hierarchy::ExactPairSupportStreamBudget;
using morsehgp3d::hierarchy::ExactPairSupportStreamStatus;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

[[nodiscard]] double milliseconds(Clock::duration duration) {
  return std::chrono::duration<double, std::milli>{duration}.count();
}

[[nodiscard]] std::string_view status_text(
    ExactPairSupportStreamStatus status) {
  switch (status) {
    case ExactPairSupportStreamStatus::complete:
      return "complete";
    case ExactPairSupportStreamStatus::budget_exhausted:
      return "budget_exhausted";
  }
  throw std::logic_error("invalid pair-support stream status");
}

[[nodiscard]] std::string_view stop_reason_text(
    ExactPairSupportStopReason reason) {
  switch (reason) {
    case ExactPairSupportStopReason::none:
      return "none";
    case ExactPairSupportStopReason::work_unit_limit:
      return "work_unit_limit";
    case ExactPairSupportStopReason::frontier_entry_limit:
      return "frontier_entry_limit";
    case ExactPairSupportStopReason::auxiliary_frontier_entry_limit:
      return "auxiliary_frontier_entry_limit";
    case ExactPairSupportStopReason::emitted_record_limit:
      return "emitted_record_limit";
    case ExactPairSupportStopReason::emitted_point_id_reference_limit:
      return "emitted_point_id_reference_limit";
    case ExactPairSupportStopReason::global_closed_ball_query_limit:
      return "global_closed_ball_query_limit";
    case ExactPairSupportStopReason::point_classification_limit:
      return "point_classification_limit";
  }
  throw std::logic_error("invalid pair-support stop reason");
}

[[nodiscard]] std::vector<CertifiedPoint3> uniform_points(
    std::size_t point_count) {
  constexpr std::size_t modulus = 65537U;
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    const std::size_t value = index + 1U;
    const std::size_t permuted_y =
        (value * 25173U + 13849U) % modulus;
    const std::size_t permuted_z =
        (value * 13849U + 25173U) % modulus;
    points.push_back(CertifiedPoint3::from_binary64(
        static_cast<double>(value) / static_cast<double>(modulus),
        static_cast<double>(permuted_y) /
            static_cast<double>(modulus),
        static_cast<double>(permuted_z) /
            static_cast<double>(modulus)));
  }
  return points;
}

[[nodiscard]] std::vector<CertifiedPoint3> clustered_points(
    std::size_t point_count) {
  constexpr double local_scale = 1.0 / 1048576.0;
  constexpr double transverse_scale = 1.0 / 4194304.0;
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    const std::size_t cluster = index % 8U;
    const std::size_t local = index / 8U + 1U;
    const double center_x = (cluster & 1U) == 0U ? 0.25 : 0.75;
    const double center_y = (cluster & 2U) == 0U ? 0.25 : 0.75;
    const double center_z = (cluster & 4U) == 0U ? 0.25 : 0.75;
    const std::size_t permuted_y =
        (local * 40503U + cluster * 7919U) % 65536U;
    const std::size_t permuted_z =
        (local * 25717U + cluster * 104729U) % 65536U;
    points.push_back(CertifiedPoint3::from_binary64(
        center_x + static_cast<double>(local) * local_scale,
        center_y + static_cast<double>(permuted_y) * transverse_scale,
        center_z + static_cast<double>(permuted_z) * transverse_scale));
  }
  return points;
}

[[nodiscard]] ExactPairSupportStreamBudget smoke_budget(
    std::size_t point_count) {
  constexpr std::size_t query_limit = 2048U;
  if (point_count >
      std::numeric_limits<std::size_t>::max() / query_limit) {
    throw std::overflow_error("the smoke point-classification cap overflows");
  }
  return ExactPairSupportStreamBudget{
      20000U,
      65536U,
      8192U,
      2048U,
      32768U,
      query_limit,
      query_limit * point_count};
}

template <typename Generator>
void emit_run(
    std::string_view family,
    std::size_t point_count,
    Generator&& generator,
    bool first) {
  const Clock::time_point generation_start = Clock::now();
  const std::vector<CertifiedPoint3> input = generator(point_count);
  const CanonicalPointCloud cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{input});
  const Clock::time_point generation_end = Clock::now();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const Clock::time_point index_end = Clock::now();
  const ExactPairSupportStreamBudget budget = smoke_budget(point_count);
  const auto result =
      morsehgp3d::hierarchy::build_exact_pair_support_stream(
          index, cloud, 10U, budget);
  const Clock::time_point stream_end = Clock::now();

  if (!first) {
    std::cout << ",\n";
  }
  std::cout << "    {\"family\":\"" << family << "\",\"n\":"
            << point_count << ",\"canonicalize_ms\":" << std::fixed
            << std::setprecision(3)
            << milliseconds(generation_end - generation_start)
            << ",\"lbvh_ms\":"
            << milliseconds(index_end - generation_end)
            << ",\"stream_ms\":"
            << milliseconds(stream_end - index_end)
            << ",\"status\":\"" << status_text(result.status)
            << "\",\"stop_reason\":\""
            << stop_reason_text(result.stop_reason)
            << "\",\"work_units\":" << result.audit.work_unit_count
            << ",\"product_visits\":"
            << result.audit.support_product_visit_count
            << ",\"product_expansions\":"
            << result.audit.support_product_expansion_count
            << ",\"exact_phi_bounds\":"
            << result.audit.exact_phi_aabb_bound_count
            << ",\"leaf_pairs\":"
            << result.audit.leaf_pair_classification_count
            << ",\"closed_ball_queries\":"
            << result.audit.global_closed_ball_query_count
            << ",\"accepted_events\":"
            << result.audit.accepted_event_count
            << ",\"extra_shell_diagnostics\":"
            << result.audit.relevant_extra_shell_diagnostic_count
            << ",\"maximum_frontier\":"
            << result.audit.maximum_frontier_entry_count
            << ",\"resolved_pairs\":" << result.audit.resolved_pair_count
            << ",\"remaining_pairs\":"
            << result.audit.remaining_frontier_pair_count
            << ",\"forbidden_global_structure_materialized\":"
            << (result.no_forbidden_global_structure_materialized
                    ? "false"
                    : "true")
            << ",\"hierarchy_reduction_performed\":"
            << (result.hierarchy_reduction_performed ? "true" : "false")
            << '}';
}

}  // namespace

int main() {
  try {
    constexpr std::array<std::size_t, 3U> sizes{
        12500U, 25000U, 50000U};
    std::cout
        << "{\n  \"schema\":\"morsehgp3d.phase9.pair-smoke.v1\","
           "\n  \"backend\":\"reference_cpu\","
           "\n  \"profile\":\"hgp_reduced\","
           "\n  \"mode\":\"certified\","
           "\n  \"requested_maximum_order\":10,"
           "\n  \"qualification_claimed\":false,"
           "\n  \"runs\":[\n";
    bool first = true;
    for (const std::size_t size : sizes) {
      emit_run("uniform_latin", size, uniform_points, first);
      first = false;
    }
    for (const std::size_t size : sizes) {
      emit_run("eight_clusters", size, clustered_points, first);
      first = false;
    }
    std::cout << "\n  ]\n}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "direct pair-support smoke failed: " << error.what()
              << '\n';
    return 1;
  }
}
