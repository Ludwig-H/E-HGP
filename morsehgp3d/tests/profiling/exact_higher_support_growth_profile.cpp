#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/resource.h>
#endif

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::canonical_integer_string;
using morsehgp3d::hierarchy::ExactHigherSupportStopReason;
using morsehgp3d::hierarchy::ExactHigherSupportStreamAudit;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactHigherSupportStreamStatus;
using morsehgp3d::hierarchy::build_exact_higher_support_stream;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

constexpr std::string_view schema =
    "morsehgp3d.exact_higher_support.growth_profile.v1";
constexpr std::array<std::string_view, 3U> profile_names{
    "uniform_dyadic", "separated_clusters", "multiscale_clusters"};
constexpr std::array<std::size_t, 3U> default_sizes{32U, 64U, 128U};
constexpr std::array<std::size_t, 3U> requested_orders{1U, 5U, 10U};

struct Record {
  std::string_view profile{};
  std::size_t point_count{};
  std::size_t requested_order{};
  bool executed{};
  bool arity_applicable{};
  bool growth_favorable{};
  std::string status;
  std::string stop_reason;
  BigInt triangle_universe{0};
  BigInt tetrahedron_universe{0};
  ExactHigherSupportStreamBudget budget{};
  ExactHigherSupportStreamAudit audit{};
  std::uint64_t wall_time_ns{};
  std::uint64_t peak_rss_bytes{};
  std::uint64_t peak_rss_growth_bytes{};
};

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t value) {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

[[nodiscard]] double signed_dyadic(std::uint64_t value) {
  constexpr std::uint64_t mask = (std::uint64_t{1} << 20U) - 1U;
  constexpr double denominator = static_cast<double>(std::uint64_t{1} << 19U);
  return static_cast<double>(value & mask) / denominator - 1.0;
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] std::vector<CertifiedPoint3> make_points(
    std::string_view profile,
    std::size_t point_count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  constexpr std::array<std::array<double, 3U>, 8U> centers{{
      {{-0.75, -0.75, -0.75}},
      {{0.75, -0.75, -0.75}},
      {{-0.75, 0.75, -0.75}},
      {{0.75, 0.75, -0.75}},
      {{-0.75, -0.75, 0.75}},
      {{0.75, -0.75, 0.75}},
      {{-0.75, 0.75, 0.75}},
      {{0.75, 0.75, 0.75}},
  }};
  for (std::size_t index = 0U; index < point_count; ++index) {
    const std::uint64_t key = static_cast<std::uint64_t>(index) + 1U;
    const double x = signed_dyadic(splitmix64(key * 3U));
    const double y = signed_dyadic(splitmix64(key * 3U + 1U));
    const double z = signed_dyadic(splitmix64(key * 3U + 2U));
    if (profile == profile_names[0]) {
      points.push_back(point(x, y, z));
      continue;
    }
    const std::size_t cluster = index % centers.size();
    const double radius = profile == profile_names[1]
                              ? 1.0 / 64.0
                              : 1.0 / static_cast<double>(
                                    std::uint64_t{1}
                                    << static_cast<unsigned int>(
                                           6U + 2U * (cluster % 4U)));
    points.push_back(point(
        centers[cluster][0] + radius * x,
        centers[cluster][1] + radius * y,
        centers[cluster][2] + radius * z));
  }
  return points;
}

[[nodiscard]] BigInt binomial(std::size_t n, std::size_t k) {
  if (k > n) {
    return 0;
  }
  k = std::min(k, n - k);
  BigInt result = 1;
  for (std::size_t index = 1U; index <= k; ++index) {
    result *= n - k + index;
    result /= index;
  }
  return result;
}

[[nodiscard]] ExactHigherSupportStreamBudget bounded_budget(
    std::size_t point_count,
    bool contract_test) {
  const std::size_t work = contract_test ? 512U : 5'000U;
  const std::size_t record = contract_test ? 512U : 5'000U;
  const std::size_t frontier = contract_test ? 512U : 2'048U;
  const std::size_t reference_multiplier = point_count + 4U;
  return ExactHigherSupportStreamBudget{
      work,
      frontier,
      64U,
      record,
      record * reference_multiplier,
      record * 64U,
      work,
      work * point_count};
}

[[nodiscard]] std::uint64_t peak_rss_bytes() {
#if defined(__linux__)
  // getrusage(RUSAGE_SELF).ru_maxrss can retain a pre-exec high-water mark
  // when a profiling binary is launched by a long-lived orchestration
  // process.  Linux exposes the high-water mark of the current mm directly;
  // prefer it so the receipt measures this executable rather than its
  // launcher.
  std::ifstream status{"/proc/self/status"};
  std::string line;
  while (std::getline(status, line)) {
    if (!line.starts_with("VmHWM:")) {
      continue;
    }
    std::istringstream fields{line.substr(6U)};
    std::uint64_t kibibytes = 0U;
    std::string unit;
    if (fields >> kibibytes >> unit && unit == "kB" &&
        kibibytes <= std::numeric_limits<std::uint64_t>::max() / 1024U) {
      return kibibytes * 1024U;
    }
    break;
  }
#endif
#if defined(__unix__) || defined(__APPLE__)
  rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) != 0 || usage.ru_maxrss < 0) {
    return 0U;
  }
  const std::uint64_t observed = static_cast<std::uint64_t>(usage.ru_maxrss);
#if defined(__APPLE__)
  return observed;
#else
  if (observed > std::numeric_limits<std::uint64_t>::max() / 1024U) {
    return std::numeric_limits<std::uint64_t>::max();
  }
  return observed * 1024U;
#endif
#else
  return 0U;
#endif
}

[[nodiscard]] std::string_view status_name(
    ExactHigherSupportStreamStatus status) {
  switch (status) {
    case ExactHigherSupportStreamStatus::complete:
      return "complete";
    case ExactHigherSupportStreamStatus::budget_exhausted:
      return "budget_exhausted";
  }
  throw std::logic_error("unknown exact higher-support status");
}

[[nodiscard]] std::string_view stop_reason_name(
    ExactHigherSupportStopReason reason) {
  switch (reason) {
    case ExactHigherSupportStopReason::none:
      return "none";
    case ExactHigherSupportStopReason::work_unit_limit:
      return "work_unit_limit";
    case ExactHigherSupportStopReason::frontier_entry_limit:
      return "frontier_entry_limit";
    case ExactHigherSupportStopReason::auxiliary_frontier_entry_limit:
      return "auxiliary_frontier_entry_limit";
    case ExactHigherSupportStopReason::emitted_record_limit:
      return "emitted_record_limit";
    case ExactHigherSupportStopReason::emitted_point_id_reference_limit:
      return "emitted_point_id_reference_limit";
    case ExactHigherSupportStopReason::prune_receipt_limit:
      return "prune_receipt_limit";
    case ExactHigherSupportStopReason::global_closed_ball_query_limit:
      return "global_closed_ball_query_limit";
    case ExactHigherSupportStopReason::point_classification_limit:
      return "point_classification_limit";
  }
  throw std::logic_error("unknown exact higher-support stop reason");
}

[[nodiscard]] Record skipped_record(
    std::string_view profile,
    std::size_t point_count,
    std::size_t requested_order,
    std::string status,
    bool arity_applicable) {
  Record record;
  record.profile = profile;
  record.point_count = point_count;
  record.requested_order = requested_order;
  record.arity_applicable = arity_applicable;
  record.status = std::move(status);
  record.stop_reason = arity_applicable ? "prior_growth_unfavorable"
                                        : "support_arity_above_rank";
  record.triangle_universe = binomial(point_count, 3U);
  record.tetrahedron_universe = binomial(point_count, 4U);
  return record;
}

[[nodiscard]] Record measure(
    std::string_view profile,
    std::size_t point_count,
    std::size_t requested_order,
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    bool contract_test) {
  Record record;
  record.profile = profile;
  record.point_count = point_count;
  record.requested_order = requested_order;
  record.executed = true;
  record.arity_applicable = true;
  record.triangle_universe = binomial(point_count, 3U);
  record.tetrahedron_universe = binomial(point_count, 4U);
  record.budget = bounded_budget(point_count, contract_test);
  const std::uint64_t rss_before = peak_rss_bytes();
  const auto started = std::chrono::steady_clock::now();
  const auto result = build_exact_higher_support_stream(
      index, cloud, requested_order, record.budget);
  const auto finished = std::chrono::steady_clock::now();
  const std::uint64_t rss_after = peak_rss_bytes();
  const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
      finished - started);
  record.wall_time_ns = static_cast<std::uint64_t>(elapsed.count());
  record.peak_rss_bytes = rss_after;
  record.peak_rss_growth_bytes = rss_after >= rss_before
                                     ? rss_after - rss_before
                                     : 0U;
  record.audit = result.audit;
  record.status = std::string{status_name(result.status)};
  record.stop_reason = std::string{stop_reason_name(result.stop_reason)};
  const bool frontier_sparse =
      result.audit.maximum_frontier_entry_count <= 8U * point_count;
  record.growth_favorable = result.status ==
                                ExactHigherSupportStreamStatus::complete &&
                            frontier_sparse;
  if (result.audit.total_support_count !=
          record.triangle_universe + record.tetrahedron_universe ||
      result.audit.resolved_support_count +
              result.audit.remaining_frontier_support_count !=
          result.audit.total_support_count ||
      result.audit.maximum_frontier_entry_count >
          record.budget.maximum_frontier_entry_count ||
      !result.no_forbidden_global_structure_materialized ||
      result.hierarchy_reduction_performed) {
    throw std::logic_error("exact higher-support growth audit is inconsistent");
  }
  return record;
}

void write_bigint(std::ostream& output, const BigInt& value) {
  output << '"' << canonical_integer_string(value) << '"';
}

void write_record(std::ostream& output, const Record& record) {
  output << "{\"arity_applicable\":"
         << (record.arity_applicable ? "true" : "false")
         << ",\"budget\":{\"frontier_entries\":"
         << record.budget.maximum_frontier_entry_count
         << ",\"work_units\":" << record.budget.maximum_work_unit_count
         << "},\"executed\":" << (record.executed ? "true" : "false")
         << ",\"growth_favorable\":"
         << (record.growth_favorable ? "true" : "false")
         << ",\"k\":" << record.requested_order
         << ",\"metrics\":{\"global_closed_ball_query_count\":"
         << record.audit.global_closed_ball_query_count
         << ",\"leaf_classified_support_count\":";
  write_bigint(output, record.audit.leaf_classified_support_count);
  output << ",\"leaf_support_analysis_count\":"
         << record.audit.leaf_support_analysis_count
         << ",\"minimal_leaf_count\":" << record.audit.minimal_leaf_count
         << ",\"peak_closed_ball_frontier\":"
         << record.audit.maximum_closed_ball_frontier_entry_count
         << ",\"peak_frontier\":"
         << record.audit.maximum_frontier_entry_count
         << ",\"peak_rank_frontier\":"
         << record.audit.maximum_rank_frontier_entry_count
         << ",\"point_classification_count\":"
         << record.audit.point_classification_count
         << ",\"peak_rss_bytes\":" << record.peak_rss_bytes
         << ",\"peak_rss_growth_bytes\":"
         << record.peak_rss_growth_bytes
         << ",\"prune_certificate_count\":"
         << record.audit.emitted_prune_certificate_count
         << ",\"rank_pruned_support_count\":";
  write_bigint(output, record.audit.rank_pruned_support_count);
  output << ",\"rank_witness_node_visits\":"
         << record.audit.rank_witness_node_visit_count
         << ",\"rank_local_probe_attempts\":"
         << record.audit.rank_local_probe_attempt_count
         << ",\"rank_local_probe_center_seeds\":"
         << record.audit.rank_local_probe_center_seed_count
         << ",\"rank_local_probe_center_path_node_visits\":"
         << record.audit.rank_local_probe_center_path_node_visit_count
         << ",\"rank_local_probe_candidate_evaluations\":"
         << record.audit.rank_local_probe_candidate_evaluation_count
         << ",\"rank_local_probe_pruned_products\":"
         << record.audit.rank_local_probe_pruned_product_count
         << ",\"rank_local_probe_fail_open_products\":"
         << record.audit.rank_local_probe_fail_open_product_count
         << ",\"rank_local_probe_geometric_gate_skips\":"
         << record.audit.rank_local_probe_geometric_gate_skip_count
         << ",\"rank_query_outside_or_boundary_nodes\":"
         << record.audit.rank_query_outside_or_boundary_node_count
         << ",\"rank_query_outside_or_boundary_points\":"
         << record.audit.rank_query_outside_or_boundary_point_count
         << ",\"rank_search_count\":" << record.audit.rank_search_count
         << ",\"peak_rank_local_probe_candidates\":"
         << record.audit.maximum_rank_local_probe_candidate_count
         << ",\"remaining_support_count\":";
  write_bigint(output, record.audit.remaining_frontier_support_count);
  output << ",\"resolved_support_count\":";
  write_bigint(output, record.audit.resolved_support_count);
  output << ",\"support_product_visits\":"
         << record.audit.support_product_visit_count
         << ",\"wall_time_ns\":" << record.wall_time_ns
         << ",\"work_unit_count\":" << record.audit.work_unit_count
         << ",\"well_centering_pruned_support_count\":";
  write_bigint(output, record.audit.well_centering_pruned_support_count);
  output << "},\"n\":" << record.point_count << ",\"profile\":\""
         << record.profile << "\",\"ratios\":{\"rank_pruned\":{\"denominator\":";
  write_bigint(
      output, record.triangle_universe + record.tetrahedron_universe);
  output << ",\"numerator\":";
  write_bigint(output, record.audit.rank_pruned_support_count);
  output << "},\"resolved\":{\"denominator\":";
  write_bigint(
      output, record.triangle_universe + record.tetrahedron_universe);
  output << ",\"numerator\":";
  write_bigint(output, record.audit.resolved_support_count);
  output << "},\"well_centering_pruned\":{\"denominator\":";
  write_bigint(
      output, record.triangle_universe + record.tetrahedron_universe);
  output << ",\"numerator\":";
  write_bigint(output, record.audit.well_centering_pruned_support_count);
  output << "}},\"status\":\"" << record.status
         << "\",\"stop_reason\":\"" << record.stop_reason
         << "\",\"universe\":{\"support3\":";
  write_bigint(output, record.triangle_universe);
  output << ",\"support4\":";
  write_bigint(output, record.tetrahedron_universe);
  output << ",\"total\":";
  write_bigint(
      output, record.triangle_universe + record.tetrahedron_universe);
  output << "}}";
}

[[nodiscard]] std::vector<Record> run_campaign(bool contract_test) {
  const std::span<const std::string_view> profiles = contract_test
      ? std::span<const std::string_view>{profile_names.data(), 1U}
      : std::span<const std::string_view>{profile_names};
  const std::span<const std::size_t> sizes = contract_test
      ? std::span<const std::size_t>{default_sizes.data(), 1U}
      : std::span<const std::size_t>{default_sizes};
  std::vector<Record> records;
  for (const std::string_view profile : profiles) {
    std::array<bool, requested_orders.size()> continue_series{
        true, true, true};
    for (const std::size_t point_count : sizes) {
      const auto points = make_points(profile, point_count);
      const CanonicalPointCloud cloud = CanonicalPointCloud::rejecting_duplicates(
          std::span<const CertifiedPoint3>{points});
      const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
      for (std::size_t order_index = 0U;
           order_index < requested_orders.size();
           ++order_index) {
        const std::size_t requested_order = requested_orders[order_index];
        if (requested_order == 1U) {
          records.push_back(skipped_record(
              profile,
              point_count,
              requested_order,
              "not_applicable",
              false));
          continue;
        }
        if (!continue_series[order_index]) {
          records.push_back(skipped_record(
              profile,
              point_count,
              requested_order,
              "skipped_growth_cutoff",
              true));
          continue;
        }
        Record record = measure(
            profile,
            point_count,
            requested_order,
            index,
            cloud,
            contract_test);
        continue_series[order_index] = record.growth_favorable;
        records.push_back(std::move(record));
      }
    }
  }
  return records;
}

void validate_contract_records(const std::vector<Record>& records) {
  if (records.size() != 3U || records[0].executed ||
      records[0].arity_applicable || !records[1].executed ||
      !records[2].executed || records[1].point_count != 32U ||
      records[1].requested_order != 5U ||
      records[2].requested_order != 10U) {
    throw std::logic_error("growth profile contract matrix differs");
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    bool contract_test = false;
    if (argc == 2 && std::string_view{argv[1]} == "--contract-test") {
      contract_test = true;
    } else if (argc != 1) {
      std::cerr << "usage: " << argv[0] << " [--contract-test]\n";
      return 2;
    }
    const std::vector<Record> records = run_campaign(contract_test);
    if (contract_test) {
      validate_contract_records(records);
    }
    std::cout
        << "{\"backend\":\"reference_cpu\",\"contract_test\":"
        << (contract_test ? "true" : "false")
        << ",\"deployment_status\":\"profiling_only\",\"matrix\":{\"k\":[1,5,10],\"n\":[32,64,128],\"profiles\":[\"uniform_dyadic\",\"separated_clusters\",\"multiscale_clusters\"]},\"public_status\":\"not_claimed\",\"records\":[";
    for (std::size_t index = 0U; index < records.size(); ++index) {
      if (index != 0U) {
        std::cout << ',';
      }
      write_record(std::cout, records[index]);
    }
    std::cout << "],\"schema\":\"" << schema << "\"}\n";
  } catch (const std::exception& error) {
    std::cerr << "exact higher-support growth profile failed: "
              << error.what() << '\n';
    return 1;
  }
  return 0;
}
