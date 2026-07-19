#include "morsehgp3d/gpu/k1_boruvka.hpp"

#include "morsehgp3d/exact/point.hpp"

#include <algorithm>
#include <bit>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#ifndef MORSEHGP3D_WORK_PROFILE_BACKEND
#define MORSEHGP3D_WORK_PROFILE_BACKEND "fake_gpu"
#endif

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::gpu::K1BoruvkaCandidateAudit;
using morsehgp3d::gpu::K1BoruvkaCandidateContext;
using morsehgp3d::gpu::K1BoruvkaChunkedEmissionAudit;
using morsehgp3d::gpu::K1BoruvkaChunkedRoundResolution;
using morsehgp3d::gpu::K1BoruvkaChunkingPolicy;
using morsehgp3d::gpu::K1BoruvkaEmissionStatus;
using morsehgp3d::gpu::K1BoruvkaMortonSeedAudit;
using morsehgp3d::gpu::K1BoruvkaMortonSeedPolicy;
using morsehgp3d::gpu::K1BoruvkaSeedStatus;
using morsehgp3d::hierarchy::ExactEmstEdge;
using morsehgp3d::hierarchy::K1BoruvkaComponentMinimum;
using morsehgp3d::hierarchy::K1BoruvkaRoundContraction;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

constexpr std::string_view backend_name =
    MORSEHGP3D_WORK_PROFILE_BACKEND;
constexpr std::string_view schema_name =
    "morsehgp3d.phase5.k1_boruvka_morton_work_profile.v1";
constexpr std::string_view scientific_scope =
    "producer_only_work_profile_without_independent_replay_or_reference_oracle";
constexpr std::size_t maximum_profile_point_count = 1'000'000U;
static_assert(
    static_cast<PointId>(maximum_profile_point_count) <=
    CanonicalPointCloud::max_point_count);

struct Arguments {
  std::string family;
  std::size_t point_count{};
  std::size_t candidate_record_budget{};
  std::vector<std::size_t> window_radii;
  std::uint64_t seed{};
  std::string git_sha;
};

struct SeedWork {
  std::size_t source_count{};
  std::size_t inspected_neighbor_count{};
  std::size_t external_neighbor_count{};
  std::size_t floating_proposal_count{};
  std::size_t exact_selected_proposal_count{};
  std::size_t exact_strict_improvement_count{};
  std::size_t exact_fallback_count{};
  std::size_t exact_seed_distance_evaluation_count{};
};

struct RoundWork {
  std::size_t round_index{};
  std::size_t pre_component_count{};
  std::size_t post_component_count{};
  std::size_t logical_candidate_count{};
  std::size_t source_chunk_count{};
  std::size_t peak_chunk_candidate_count{};
  std::size_t max_source_candidate_count{};
  std::size_t gpu_count_pass_node_visit_count{};
  std::size_t gpu_emit_pass_node_visit_count{};
  std::size_t gpu_uniform_component_prune_count{};
  std::size_t gpu_strict_aabb_prune_count{};
  std::size_t gpu_invalid_bound_descent_count{};
  std::size_t cpu_exact_aabb_bound_evaluation_count{};
  std::size_t cpu_required_candidate_count{};
  std::size_t cpu_exact_candidate_distance_evaluation_count{};
  SeedWork seed;
};

struct WorkTotals {
  std::size_t round_count{};
  std::size_t logical_candidate_count{};
  std::size_t source_chunk_count{};
  std::size_t peak_chunk_candidate_count{};
  std::size_t max_source_candidate_count{};
  std::size_t gpu_count_pass_node_visit_count{};
  std::size_t gpu_emit_pass_node_visit_count{};
  std::size_t gpu_uniform_component_prune_count{};
  std::size_t gpu_strict_aabb_prune_count{};
  std::size_t gpu_invalid_bound_descent_count{};
  std::size_t cpu_exact_aabb_bound_evaluation_count{};
  std::size_t cpu_required_candidate_count{};
  std::size_t cpu_exact_candidate_distance_evaluation_count{};
  SeedWork seed;
};

struct RoundWitness {
  std::vector<K1BoruvkaComponentMinimum> component_minima;
  std::vector<ExactEmstEdge> accepted_edges;
  std::size_t post_component_count{};
};

struct RunMeasurement {
  std::vector<std::size_t> component_count_path;
  std::vector<RoundWork> rounds;
  std::vector<RoundWitness> witnesses;
  std::vector<ExactEmstEdge> emst_edges;
  ExactLevel total_squared_weight{};
  ExactLevel total_hgp_weight{};
  WorkTotals totals;
};

struct Ratio {
  std::size_t numerator{};
  std::size_t denominator{1U};
};

[[noreturn]] void fail(std::string_view message) {
  throw std::runtime_error{std::string{message}};
}

void require(bool condition, std::string_view message) {
  if (!condition) {
    fail(message);
  }
}

[[nodiscard]] std::size_t parse_size(
    std::string_view text, std::string_view label) {
  std::size_t value = 0U;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto [position, error] = std::from_chars(begin, end, value);
  if (text.empty() || error != std::errc{} || position != end) {
    fail(std::string{label} + " must be a canonical natural number");
  }
  return value;
}

[[nodiscard]] std::uint64_t parse_seed(std::string_view text) {
  std::uint64_t value = 0U;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto [position, error] = std::from_chars(begin, end, value);
  if (text.empty() || error != std::errc{} || position != end) {
    fail("--seed must be a canonical uint64");
  }
  return value;
}

[[nodiscard]] bool valid_git_sha(std::string_view value) {
  if (value.size() != 40U) {
    return false;
  }
  return std::all_of(value.begin(), value.end(), [](char character) {
    return (character >= '0' && character <= '9') ||
           (character >= 'a' && character <= 'f');
  });
}

[[nodiscard]] std::vector<std::size_t> parse_radii(
    std::string_view text) {
  std::vector<std::size_t> result;
  std::size_t begin = 0U;
  while (begin <= text.size()) {
    const std::size_t comma = text.find(',', begin);
    const std::size_t end =
        comma == std::string_view::npos ? text.size() : comma;
    const std::size_t value = parse_size(
        text.substr(begin, end - begin), "--window-radii entry");
    require(value > 0U, "Morton window radii must be positive");
    require(
        std::find(result.begin(), result.end(), value) == result.end(),
        "Morton window radii must be distinct");
    result.push_back(value);
    if (comma == std::string_view::npos) {
      break;
    }
    begin = comma + 1U;
  }
  require(!result.empty(), "--window-radii must not be empty");
  return result;
}

[[nodiscard]] Arguments parse_arguments(int count, char** values) {
  Arguments arguments;
  bool family_seen = false;
  bool point_count_seen = false;
  bool budget_seen = false;
  bool radii_seen = false;
  bool seed_seen = false;
  bool git_seen = false;
  for (int index = 1; index < count; ++index) {
    const std::string_view option{values[index]};
    if (option == "--help" || option == "-h") {
      std::cout
          << "Usage: morsehgp3d_gpu_k1_boruvka_morton_work_profile "
             "--family uniform|clusters|lattice --point-count N "
             "--candidate-record-budget B --window-radii CSV --seed S "
             "--git-sha SHA\n";
      std::exit(0);
    }
    if (index + 1 >= count) {
      fail(std::string{"missing value after "} + std::string{option});
    }
    const std::string_view value{values[++index]};
    if (option == "--family") {
      require(!family_seen, "--family may be supplied only once");
      arguments.family = std::string{value};
      family_seen = true;
    } else if (option == "--point-count") {
      require(
          !point_count_seen, "--point-count may be supplied only once");
      arguments.point_count = parse_size(value, "--point-count");
      point_count_seen = true;
    } else if (option == "--candidate-record-budget") {
      require(
          !budget_seen,
          "--candidate-record-budget may be supplied only once");
      arguments.candidate_record_budget =
          parse_size(value, "--candidate-record-budget");
      budget_seen = true;
    } else if (option == "--window-radii") {
      require(
          !radii_seen, "--window-radii may be supplied only once");
      arguments.window_radii = parse_radii(value);
      radii_seen = true;
    } else if (option == "--seed") {
      require(!seed_seen, "--seed may be supplied only once");
      arguments.seed = parse_seed(value);
      seed_seen = true;
    } else if (option == "--git-sha") {
      require(!git_seen, "--git-sha may be supplied only once");
      arguments.git_sha = std::string{value};
      git_seen = true;
    } else {
      fail(std::string{"unknown option: "} + std::string{option});
    }
  }
  require(
      family_seen && point_count_seen && budget_seen && radii_seen &&
          seed_seen && git_seen,
      "all work-profile options are mandatory");
  require(
      arguments.family == "uniform" || arguments.family == "clusters" ||
          arguments.family == "lattice",
      "--family must be uniform, clusters or lattice");
  require(arguments.point_count >= 2U, "--point-count must be at least two");
  require(
      arguments.point_count <= maximum_profile_point_count,
      "--point-count exceeds the injective work-profile generator domain");
  require(
      arguments.candidate_record_budget >= arguments.point_count - 1U,
      "the work profile requires B >= n-1 so chunking cannot mask M_r");
  require(valid_git_sha(arguments.git_sha), "--git-sha must be 40 lowercase hex");
  require(
      std::all_of(
          arguments.window_radii.begin(),
          arguments.window_radii.end(),
          [](std::size_t radius) {
            return radius <= std::numeric_limits<std::size_t>::max() / 2U;
          }),
      "a Morton window radius is too large to certify its 2W bound");
  return arguments;
}

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t value) noexcept {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

[[nodiscard]] double unit_dyadic(std::uint64_t value) noexcept {
  constexpr std::uint64_t mask = (std::uint64_t{1U} << 21U) - 1U;
  constexpr double denominator = 2097152.0;
  return static_cast<double>(value & mask) / denominator;
}

[[nodiscard]] std::vector<CertifiedPoint3> make_points(
    const Arguments& arguments) {
  std::vector<CertifiedPoint3> points;
  points.reserve(arguments.point_count);
  if (arguments.family == "lattice") {
    std::size_t side = 1U;
    while (side <= arguments.point_count / side / side &&
           side * side * side < arguments.point_count) {
      ++side;
    }
    while (side * side * side < arguments.point_count) {
      ++side;
    }
    for (std::size_t index = 0U; index < arguments.point_count; ++index) {
      const std::size_t x = index % side;
      const std::size_t y = (index / side) % side;
      const std::size_t z = index / (side * side);
      points.push_back(CertifiedPoint3::from_binary64(
          static_cast<double>(x),
          static_cast<double>(y),
          static_cast<double>(z)));
    }
    return points;
  }

  const double point_denominator =
      static_cast<double>(arguments.point_count) * 64.0;
  for (std::size_t index = 0U; index < arguments.point_count; ++index) {
    const std::uint64_t keyed =
        arguments.seed ^ static_cast<std::uint64_t>(index);
    const double random_y = unit_dyadic(splitmix64(keyed));
    const double random_z = unit_dyadic(splitmix64(keyed + 1U));
    if (arguments.family == "uniform") {
      const double x =
          (static_cast<double>(index) + 0.5) /
          static_cast<double>(arguments.point_count);
      points.push_back(CertifiedPoint3::from_binary64(x, random_y, random_z));
      continue;
    }

    const std::size_t cluster = index % 8U;
    const std::size_t local_index = index / 8U;
    const double center_x = (cluster & 1U) == 0U ? 0.25 : 0.75;
    const double center_y = (cluster & 2U) == 0U ? 0.25 : 0.75;
    const double center_z = (cluster & 4U) == 0U ? 0.25 : 0.75;
    const double unique_offset =
        (static_cast<double>(local_index) + 1.0) / point_denominator;
    const double jitter_y = (random_y - 0.5) / 32.0;
    const double jitter_z = (random_z - 0.5) / 32.0;
    points.push_back(CertifiedPoint3::from_binary64(
        center_x + unique_offset,
        center_y + jitter_y,
        center_z + jitter_z));
  }
  return points;
}

[[nodiscard]] PointId checked_point_id(std::size_t index) {
  if (index > static_cast<std::size_t>(CanonicalPointCloud::max_point_id)) {
    throw std::length_error("work-profile PointId overflow");
  }
  return static_cast<PointId>(index);
}

void add_checked(std::size_t& target, std::size_t value) {
  if (value > std::numeric_limits<std::size_t>::max() - target) {
    throw std::overflow_error("work-profile counter overflow");
  }
  target += value;
}

[[nodiscard]] bool edge_less(
    const ExactEmstEdge& left, const ExactEmstEdge& right) {
  if (left.squared_length != right.squared_length) {
    return left.squared_length < right.squared_length;
  }
  if (left.u != right.u) {
    return left.u < right.u;
  }
  return left.v < right.v;
}

void require_round_certificates(
    const K1BoruvkaChunkedRoundResolution& proposal,
    bool morton_mode) {
  const K1BoruvkaCandidateAudit& audit = proposal.proposal_audit;
  const K1BoruvkaChunkedEmissionAudit& emission = proposal.emission_audit;
  require(
      proposal.emission_status ==
              K1BoruvkaEmissionStatus::
                  complete_source_ranges_candidate_payload_bound_certified &&
          audit.frozen_labels_certified && audit.rope_topology_certified &&
          audit.exact_capacity_certified && audit.no_truncation_certified &&
          audit.candidate_superset_certified &&
          audit.cpu_exact_resolution_complete &&
          emission.complete_source_partition_certified &&
          emission.count_emit_cardinality_and_visit_count_certified &&
          emission.candidate_payload_physical_bound_certified &&
          emission.max_source_candidate_count <=
              emission.peak_chunk_candidate_count &&
          emission.peak_chunk_candidate_count <=
              emission.candidate_record_budget &&
          audit.gpu_candidate_count == emission.logical_candidate_count &&
          audit.cpu_required_candidate_count <=
              emission.logical_candidate_count &&
          audit.cpu_exact_candidate_distance_evaluation_count ==
              emission.logical_candidate_count &&
          audit.gpu_count_pass_node_visit_count ==
              audit.gpu_emit_pass_node_visit_count,
      "a work-profile proposal did not close its chunked certificates");
  const K1BoruvkaMortonSeedAudit& seed = proposal.morton_seed_audit;
  if (!morton_mode) {
    require(
        proposal.seed_status == K1BoruvkaSeedStatus::not_certified &&
            seed == K1BoruvkaMortonSeedAudit{},
        "the baseline unexpectedly published a Morton seed certificate");
    return;
  }
  require(
      proposal.seed_status ==
              K1BoruvkaSeedStatus::
                  bounded_morton_window_external_exact_monotone_certified &&
          seed.complete_source_coverage_certified &&
          seed.bounded_window_certified &&
          seed.external_targets_recertified &&
          seed.exact_monotone_cutoff_certified,
      "a Morton work-profile proposal did not close its seed certificates");
}

void accumulate_seed(SeedWork& total, const SeedWork& round) {
  add_checked(total.source_count, round.source_count);
  add_checked(
      total.inspected_neighbor_count, round.inspected_neighbor_count);
  add_checked(total.external_neighbor_count, round.external_neighbor_count);
  add_checked(total.floating_proposal_count, round.floating_proposal_count);
  add_checked(
      total.exact_selected_proposal_count,
      round.exact_selected_proposal_count);
  add_checked(
      total.exact_strict_improvement_count,
      round.exact_strict_improvement_count);
  add_checked(total.exact_fallback_count, round.exact_fallback_count);
  add_checked(
      total.exact_seed_distance_evaluation_count,
      round.exact_seed_distance_evaluation_count);
}

[[nodiscard]] RunMeasurement run_measurement(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    K1BoruvkaChunkingPolicy chunking_policy,
    std::optional<K1BoruvkaMortonSeedPolicy> seed_policy) {
  RunMeasurement result;
  std::vector<PointId> labels(cloud.size());
  for (std::size_t point_index = 0U; point_index < cloud.size(); ++point_index) {
    labels[point_index] = checked_point_id(point_index);
  }
  std::size_t component_count = cloud.size();
  result.component_count_path.push_back(component_count);
  K1BoruvkaCandidateContext context{index, cloud};
  while (component_count > 1U) {
    const std::size_t round_index = result.rounds.size();
    K1BoruvkaChunkedRoundResolution proposal =
        seed_policy.has_value()
            ? context.propose_round_chunked(
                  cloud, labels, chunking_policy, *seed_policy)
            : context.propose_round_chunked(cloud, labels, chunking_policy);
    require_round_certificates(proposal, seed_policy.has_value());
    require(
        proposal.cpu_exact_component_minima.size() == component_count,
        "a work-profile round did not return one minimum per component");
    K1BoruvkaRoundContraction contraction =
        morsehgp3d::hierarchy::contract_exact_k1_boruvka_round(
            cloud, labels, proposal.cpu_exact_component_minima);

    const K1BoruvkaCandidateAudit& audit = proposal.proposal_audit;
    const K1BoruvkaChunkedEmissionAudit& emission = proposal.emission_audit;
    const K1BoruvkaMortonSeedAudit& seed = proposal.morton_seed_audit;
    RoundWork round;
    round.round_index = round_index;
    round.pre_component_count = component_count;
    round.post_component_count = contraction.post_round_component_count;
    round.logical_candidate_count = emission.logical_candidate_count;
    round.source_chunk_count = emission.source_chunk_count;
    round.peak_chunk_candidate_count = emission.peak_chunk_candidate_count;
    round.max_source_candidate_count = emission.max_source_candidate_count;
    round.gpu_count_pass_node_visit_count =
        audit.gpu_count_pass_node_visit_count;
    round.gpu_emit_pass_node_visit_count =
        audit.gpu_emit_pass_node_visit_count;
    round.gpu_uniform_component_prune_count =
        audit.gpu_uniform_component_prune_count;
    round.gpu_strict_aabb_prune_count =
        audit.gpu_strict_aabb_prune_count;
    round.gpu_invalid_bound_descent_count =
        audit.gpu_invalid_bound_descent_count;
    round.cpu_exact_aabb_bound_evaluation_count =
        audit.cpu_exact_aabb_bound_evaluation_count;
    round.cpu_required_candidate_count = audit.cpu_required_candidate_count;
    round.cpu_exact_candidate_distance_evaluation_count =
        audit.cpu_exact_candidate_distance_evaluation_count;
    round.seed.source_count = cloud.size();
    round.seed.exact_seed_distance_evaluation_count =
        seed_policy.has_value()
            ? seed.exact_seed_distance_evaluation_count
            : audit.exact_seed_count;
    if (seed_policy.has_value()) {
      round.seed.source_count = seed.source_count;
      round.seed.inspected_neighbor_count = seed.inspected_neighbor_count;
      round.seed.external_neighbor_count = seed.external_neighbor_count;
      round.seed.floating_proposal_count = seed.floating_proposal_count;
      round.seed.exact_selected_proposal_count =
          seed.exact_selected_proposal_count;
      round.seed.exact_strict_improvement_count =
          seed.exact_strict_improvement_count;
      round.seed.exact_fallback_count = seed.exact_fallback_count;
    } else {
      round.seed.exact_fallback_count = cloud.size();
    }

    result.totals.round_count += 1U;
    add_checked(
        result.totals.logical_candidate_count,
        round.logical_candidate_count);
    add_checked(
        result.totals.source_chunk_count, round.source_chunk_count);
    result.totals.peak_chunk_candidate_count = std::max(
        result.totals.peak_chunk_candidate_count,
        round.peak_chunk_candidate_count);
    result.totals.max_source_candidate_count = std::max(
        result.totals.max_source_candidate_count,
        round.max_source_candidate_count);
    add_checked(
        result.totals.gpu_count_pass_node_visit_count,
        round.gpu_count_pass_node_visit_count);
    add_checked(
        result.totals.gpu_emit_pass_node_visit_count,
        round.gpu_emit_pass_node_visit_count);
    add_checked(
        result.totals.gpu_uniform_component_prune_count,
        round.gpu_uniform_component_prune_count);
    add_checked(
        result.totals.gpu_strict_aabb_prune_count,
        round.gpu_strict_aabb_prune_count);
    add_checked(
        result.totals.gpu_invalid_bound_descent_count,
        round.gpu_invalid_bound_descent_count);
    add_checked(
        result.totals.cpu_exact_aabb_bound_evaluation_count,
        round.cpu_exact_aabb_bound_evaluation_count);
    add_checked(
        result.totals.cpu_required_candidate_count,
        round.cpu_required_candidate_count);
    add_checked(
        result.totals.cpu_exact_candidate_distance_evaluation_count,
        round.cpu_exact_candidate_distance_evaluation_count);
    accumulate_seed(result.totals.seed, round.seed);

    result.emst_edges.insert(
        result.emst_edges.end(),
        contraction.accepted_edges.begin(),
        contraction.accepted_edges.end());
    result.witnesses.push_back(RoundWitness{
        std::move(proposal.cpu_exact_component_minima),
        contraction.accepted_edges,
        contraction.post_round_component_count});
    result.rounds.push_back(round);
    labels = std::move(contraction.post_round_component_labels);
    component_count = contraction.post_round_component_count;
    result.component_count_path.push_back(component_count);
  }
  std::sort(result.emst_edges.begin(), result.emst_edges.end(), edge_less);
  require(
      result.emst_edges.size() == cloud.size() - 1U,
      "a work-profile chain did not produce a spanning tree");
  ExactRational squared_weight;
  ExactRational hgp_weight;
  for (const ExactEmstEdge& edge : result.emst_edges) {
    squared_weight = squared_weight + edge.squared_length.rational();
    hgp_weight = hgp_weight + edge.merge_level.rational();
  }
  result.total_squared_weight = ExactLevel{std::move(squared_weight)};
  result.total_hgp_weight = ExactLevel{std::move(hgp_weight)};
  return result;
}

[[nodiscard]] Ratio reduced_ratio(
    std::size_t numerator, std::size_t denominator) {
  require(denominator > 0U, "a work-profile ratio has zero denominator");
  const std::size_t divisor = std::gcd(numerator, denominator);
  return Ratio{numerator / divisor, denominator / divisor};
}

void write_level(std::ostream& output, const ExactLevel& level) {
  output << "{\"denominator\":\"" << level.denominator()
         << "\",\"numerator\":\"" << level.numerator() << "\"}";
}

void write_ratio(std::ostream& output, Ratio ratio) {
  output << "{\"denominator\":" << ratio.denominator
         << ",\"numerator\":" << ratio.numerator << '}';
}

void write_seed_work(std::ostream& output, const SeedWork& seed) {
  output << "{\"exact_fallback_count\":" << seed.exact_fallback_count
         << ",\"exact_seed_distance_evaluation_count\":"
         << seed.exact_seed_distance_evaluation_count
         << ",\"exact_selected_proposal_count\":"
         << seed.exact_selected_proposal_count
         << ",\"exact_strict_improvement_count\":"
         << seed.exact_strict_improvement_count
         << ",\"external_neighbor_count\":"
         << seed.external_neighbor_count
         << ",\"floating_proposal_count\":"
         << seed.floating_proposal_count
         << ",\"inspected_neighbor_count\":"
         << seed.inspected_neighbor_count
         << ",\"source_count\":" << seed.source_count << '}';
}

void write_round(std::ostream& output, const RoundWork& round) {
  output << "{\"cpu_exact_aabb_bound_evaluation_count\":"
         << round.cpu_exact_aabb_bound_evaluation_count
         << ",\"cpu_exact_candidate_distance_evaluation_count\":"
         << round.cpu_exact_candidate_distance_evaluation_count
         << ",\"cpu_required_candidate_count\":"
         << round.cpu_required_candidate_count
         << ",\"gpu_count_pass_node_visit_count\":"
         << round.gpu_count_pass_node_visit_count
         << ",\"gpu_emit_pass_node_visit_count\":"
         << round.gpu_emit_pass_node_visit_count
         << ",\"gpu_invalid_bound_descent_count\":"
         << round.gpu_invalid_bound_descent_count
         << ",\"gpu_strict_aabb_prune_count\":"
         << round.gpu_strict_aabb_prune_count
         << ",\"gpu_uniform_component_prune_count\":"
         << round.gpu_uniform_component_prune_count
         << ",\"logical_candidate_count\":"
         << round.logical_candidate_count
         << ",\"max_source_candidate_count\":"
         << round.max_source_candidate_count
         << ",\"peak_chunk_candidate_count\":"
         << round.peak_chunk_candidate_count
         << ",\"post_component_count\":" << round.post_component_count
         << ",\"pre_component_count\":" << round.pre_component_count
         << ",\"round_index\":" << round.round_index
         << ",\"seed_work\":";
  write_seed_work(output, round.seed);
  output << ",\"source_chunk_count\":" << round.source_chunk_count << '}';
}

void write_totals(std::ostream& output, const WorkTotals& totals) {
  output << "{\"cpu_exact_aabb_bound_evaluation_count\":"
         << totals.cpu_exact_aabb_bound_evaluation_count
         << ",\"cpu_exact_candidate_distance_evaluation_count\":"
         << totals.cpu_exact_candidate_distance_evaluation_count
         << ",\"cpu_required_candidate_count\":"
         << totals.cpu_required_candidate_count
         << ",\"gpu_count_pass_node_visit_count\":"
         << totals.gpu_count_pass_node_visit_count
         << ",\"gpu_emit_pass_node_visit_count\":"
         << totals.gpu_emit_pass_node_visit_count
         << ",\"gpu_invalid_bound_descent_count\":"
         << totals.gpu_invalid_bound_descent_count
         << ",\"gpu_strict_aabb_prune_count\":"
         << totals.gpu_strict_aabb_prune_count
         << ",\"gpu_uniform_component_prune_count\":"
         << totals.gpu_uniform_component_prune_count
         << ",\"logical_candidate_count\":"
         << totals.logical_candidate_count
         << ",\"max_source_candidate_count\":"
         << totals.max_source_candidate_count
         << ",\"peak_chunk_candidate_count\":"
         << totals.peak_chunk_candidate_count
         << ",\"round_count\":" << totals.round_count
         << ",\"seed_work\":";
  write_seed_work(output, totals.seed);
  output << ",\"source_chunk_count\":" << totals.source_chunk_count << '}';
}

void write_run(std::ostream& output, const RunMeasurement& run) {
  output << "{\"component_count_path\":[";
  for (std::size_t index = 0U;
       index < run.component_count_path.size();
       ++index) {
    if (index != 0U) {
      output << ',';
    }
    output << run.component_count_path[index];
  }
  output << "],\"exact_weights\":{\"hgp\":";
  write_level(output, run.total_hgp_weight);
  output << ",\"squared\":";
  write_level(output, run.total_squared_weight);
  output << "},\"rounds\":[";
  for (std::size_t index = 0U; index < run.rounds.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    write_round(output, run.rounds[index]);
  }
  output << "],\"totals\":";
  write_totals(output, run.totals);
  output << '}';
}

[[nodiscard]] bool same_decisions(
    const RunMeasurement& left, const RunMeasurement& right) {
  if (left.witnesses.size() != right.witnesses.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < left.witnesses.size(); ++index) {
    if (left.witnesses[index].component_minima !=
        right.witnesses[index].component_minima) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool same_contractions(
    const RunMeasurement& left, const RunMeasurement& right) {
  if (left.witnesses.size() != right.witnesses.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < left.witnesses.size(); ++index) {
    const RoundWitness& left_round = left.witnesses[index];
    const RoundWitness& right_round = right.witnesses[index];
    if (left_round.accepted_edges != right_round.accepted_edges ||
        left_round.post_component_count != right_round.post_component_count) {
      return false;
    }
  }
  return true;
}

void write_boolean(std::ostream& output, bool value) {
  output << (value ? "true" : "false");
}

void write_morton_profile(
    std::ostream& output,
    const RunMeasurement& baseline,
    const RunMeasurement& morton,
    std::size_t radius) {
  const bool decisions_unchanged = same_decisions(baseline, morton);
  const bool contractions_unchanged = same_contractions(baseline, morton);
  const bool emst_unchanged = baseline.emst_edges == morton.emst_edges;
  const bool weights_unchanged =
      baseline.total_squared_weight == morton.total_squared_weight &&
      baseline.total_hgp_weight == morton.total_hgp_weight;
  require(
      decisions_unchanged && contractions_unchanged && emst_unchanged &&
          weights_unchanged,
      "a Morton profile changed the exact Boruvka witness");
  require(
      baseline.totals.logical_candidate_count >=
          morton.totals.logical_candidate_count,
      "a monotone Morton cutoff increased the logical candidate count");
  const std::size_t savings =
      baseline.totals.logical_candidate_count -
      morton.totals.logical_candidate_count;
  output << "{\"comparison\":{\"candidate_savings_rate\":";
  write_ratio(
      output,
      reduced_ratio(savings, baseline.totals.logical_candidate_count));
  output << ",\"canonical_contractions_unchanged\":";
  write_boolean(output, contractions_unchanged);
  output << ",\"emst_edges_unchanged\":";
  write_boolean(output, emst_unchanged);
  output << ",\"exact_decisions_unchanged\":";
  write_boolean(output, decisions_unchanged);
  output << ",\"exact_weights_unchanged\":";
  write_boolean(output, weights_unchanged);
  output << ",\"fallback_rate\":";
  write_ratio(
      output,
      reduced_ratio(
          morton.totals.seed.exact_fallback_count,
          morton.totals.seed.source_count));
  output << ",\"strict_improvement_rate\":";
  write_ratio(
      output,
      reduced_ratio(
          morton.totals.seed.exact_strict_improvement_count,
          morton.totals.seed.source_count));
  output << "},\"measurement\":";
  write_run(output, morton);
  output << ",\"window_radius\":" << radius << '}';
}

void write_document(
    std::ostream& output,
    const Arguments& arguments,
    const RunMeasurement& baseline,
    const std::vector<RunMeasurement>& morton_runs) {
  output << "{\"artifact_kind\":\"work_profile\",\"backend\":\""
         << backend_name << "\",\"baseline\":";
  write_run(output, baseline);
  output << ",\"decision_backend\":\"reference_cpu\""
         << ",\"generator\":{\"algorithm\":\"deterministic_dyadic_v1\""
         << ",\"family\":\"" << arguments.family
         << "\",\"seed\":" << arguments.seed << '}'
         << ",\"git\":{\"sha\":\"" << arguments.git_sha << "\"}"
         << ",\"hierarchy_reduction_status\":\"not_performed\""
         << ",\"mode\":\"certified\",\"morton_profiles\":[";
  for (std::size_t index = 0U; index < morton_runs.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    write_morton_profile(
        output, baseline, morton_runs[index], arguments.window_radii[index]);
  }
  output << "],\"phase\":\"5\",\"point_count\":"
         << arguments.point_count
         << ",\"policies\":{\"candidate_record_budget\":"
         << arguments.candidate_record_budget << ",\"window_radii\":[";
  for (std::size_t index = 0U; index < arguments.window_radii.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    output << arguments.window_radii[index];
  }
  output << "]},\"profile\":\"hgp_reduced\""
         << ",\"qualification_claimed\":false"
         << ",\"scalability_claimed\":false,\"schema\":\""
         << schema_name << "\",\"scientific_public_status\":null"
         << ",\"scientific_result_claimed\":false"
         << ",\"scope\":\"" << scientific_scope
         << "\",\"status\":\"measured\"}\n";
}

}  // namespace

int main(int argument_count, char** argument_values) {
  try {
    const Arguments arguments =
        parse_arguments(argument_count, argument_values);
    const std::vector<CertifiedPoint3> points = make_points(arguments);
    const CanonicalPointCloud cloud =
        CanonicalPointCloud::rejecting_duplicates(
            std::span<const CertifiedPoint3>{points});
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    const K1BoruvkaChunkingPolicy chunking_policy{
        arguments.candidate_record_budget};
    const RunMeasurement baseline =
        run_measurement(index, cloud, chunking_policy, std::nullopt);
    std::vector<RunMeasurement> morton_runs;
    morton_runs.reserve(arguments.window_radii.size());
    for (const std::size_t radius : arguments.window_radii) {
      morton_runs.push_back(run_measurement(
          index,
          cloud,
          chunking_policy,
          K1BoruvkaMortonSeedPolicy{radius}));
    }
    write_document(std::cout, arguments, baseline, morton_runs);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "morsehgp3d_gpu_k1_boruvka_morton_work_profile: "
              << error.what() << '\n';
    return 1;
  }
}
