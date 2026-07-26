#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/sparse_anchored_pair_session.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#if MORSEHGP3D_MASSIVE_PREFIX_HAS_CUDA
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#endif

#include <algorithm>
#include <bit>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#if defined(__linux__)
#include <sys/resource.h>
#endif

namespace {

using Clock = std::chrono::steady_clock;
using morsehgp3d::exact::CertifiedPoint3;
using namespace morsehgp3d::hierarchy;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

enum class SpatialBackend : std::uint8_t {
  automatic,
  reference_cpu,
  cuda14m,
};

struct Options {
  std::size_t point_count{50'000U};
  std::size_t maximum_order{10U};
  std::size_t prefix_advance_count{1U};
  std::size_t work_budget{64U};
  std::uint64_t seed{UINT64_C(0x14d3a5c79b)};
  SpatialBackend spatial_backend{SpatialBackend::automatic};
};

template <typename Integer>
[[nodiscard]] Integer parse_integer(std::string_view text, const char* role) {
  Integer value{};
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end) {
    throw std::invalid_argument(role);
  }
  return value;
}

[[nodiscard]] SpatialBackend parse_spatial_backend(std::string_view text) {
  if (text == "automatic") {
    return SpatialBackend::automatic;
  }
  if (text == "reference_cpu") {
    return SpatialBackend::reference_cpu;
  }
  if (text == "cuda14m") {
    return SpatialBackend::cuda14m;
  }
  throw std::invalid_argument(
      "--spatial-backend must be automatic, reference_cpu or cuda14m");
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--point-count" && index + 1 < argc) {
      options.point_count = parse_integer<std::size_t>(
          argv[++index], "invalid --point-count");
    } else if (argument == "--K" && index + 1 < argc) {
      options.maximum_order =
          parse_integer<std::size_t>(argv[++index], "invalid --K");
    } else if (argument == "--prefix-advances" && index + 1 < argc) {
      options.prefix_advance_count = parse_integer<std::size_t>(
          argv[++index], "invalid --prefix-advances");
    } else if (argument == "--work-budget" && index + 1 < argc) {
      options.work_budget = parse_integer<std::size_t>(
          argv[++index], "invalid --work-budget");
    } else if (argument == "--seed" && index + 1 < argc) {
      options.seed =
          parse_integer<std::uint64_t>(argv[++index], "invalid --seed");
    } else if (argument == "--spatial-backend" && index + 1 < argc) {
      options.spatial_backend = parse_spatial_backend(argv[++index]);
    } else {
      throw std::invalid_argument(
          "usage: massive_sparse_pair_prefix_smoke --point-count N "
          "[--K 1..10] [--prefix-advances N] [--work-budget N] "
          "[--seed N] "
          "[--spatial-backend automatic|reference_cpu|cuda14m]");
    }
  }
  if (options.point_count == 0U || options.maximum_order == 0U ||
      options.maximum_order > 10U ||
      options.maximum_order >= options.point_count ||
      options.prefix_advance_count == 0U || options.work_budget == 0U) {
    throw std::invalid_argument(
        "point-count, prefix-advances and work-budget must be positive; "
        "K must be in 1..10 and strictly below point-count");
  }
  return options;
}

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t value) noexcept {
  value += UINT64_C(0x9e3779b97f4a7c15);
  value =
      (value ^ (value >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
  value =
      (value ^ (value >> 27U)) * UINT64_C(0x94d049bb133111eb);
  return value ^ (value >> 31U);
}

[[nodiscard]] std::size_t permutation_multiplier(std::size_t point_count) {
  std::size_t multiplier = static_cast<std::size_t>(
      UINT64_C(2654435761) % static_cast<std::uint64_t>(point_count));
  multiplier = std::max(multiplier, std::size_t{1U});
  while (std::gcd(multiplier, point_count) != 1U) {
    ++multiplier;
  }
  return multiplier;
}

[[nodiscard]] std::vector<CertifiedPoint3> make_affine_uniform_cloud(
    const Options& options) {
  constexpr std::uint64_t mantissa_mask =
      (UINT64_C(1) << 52U) - UINT64_C(1);
  constexpr std::uint64_t one_exponent = UINT64_C(0x3ff0000000000000);
  if (static_cast<std::uint64_t>(options.point_count) > mantissa_mask) {
    throw std::length_error(
        "point-count exceeds the unique affine binary64 generator range");
  }
  const std::uint64_t x_step = std::max(
      UINT64_C(1),
      mantissa_mask / static_cast<std::uint64_t>(options.point_count));
  const std::size_t multiplier =
      permutation_multiplier(options.point_count);
  const std::size_t offset =
      static_cast<std::size_t>(options.seed % options.point_count);

  std::vector<CertifiedPoint3> points;
  points.reserve(options.point_count);
  for (std::size_t source_index = 0U;
       source_index < options.point_count;
       ++source_index) {
    const std::size_t logical_index = static_cast<std::size_t>(
        (static_cast<std::uint64_t>(source_index) *
             static_cast<std::uint64_t>(multiplier) +
         static_cast<std::uint64_t>(offset)) %
        static_cast<std::uint64_t>(options.point_count));
    const std::uint64_t identity =
        static_cast<std::uint64_t>(logical_index);
    points.push_back(CertifiedPoint3::from_binary64_bits(
        {one_exponent | (identity * x_step),
         one_exponent |
             (splitmix64(identity ^ options.seed) & mantissa_mask),
         one_exponent |
             (splitmix64(identity ^ std::rotl(options.seed, 23)) &
              mantissa_mask)}));
  }
  return points;
}

template <typename Duration>
[[nodiscard]] double milliseconds(Duration duration) {
  return std::chrono::duration<double, std::milli>(duration).count();
}

[[nodiscard]] std::uint64_t checked_multiply(
    std::size_t first,
    std::size_t second,
    const char* role) {
  if (first != 0U &&
      second > std::numeric_limits<std::uint64_t>::max() / first) {
    throw std::overflow_error(role);
  }
  return static_cast<std::uint64_t>(first) *
      static_cast<std::uint64_t>(second);
}

[[nodiscard]] std::uint64_t peak_host_rss_bytes() {
#if defined(__linux__)
  rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) != 0 || usage.ru_maxrss < 0) {
    throw std::runtime_error("getrusage failed");
  }
  return static_cast<std::uint64_t>(usage.ru_maxrss) * UINT64_C(1024);
#else
  return 0U;
#endif
}

[[nodiscard]] std::string_view step_kind_name(
    ExactSparseAnchoredPairSessionStepKind kind) noexcept {
  switch (kind) {
    case ExactSparseAnchoredPairSessionStepKind::candidate_started:
      return "candidate_started";
    case ExactSparseAnchoredPairSessionStepKind::candidate_above_rank:
      return "candidate_above_rank";
    case ExactSparseAnchoredPairSessionStepKind::candidate_record_ready:
      return "candidate_record_ready";
    case ExactSparseAnchoredPairSessionStepKind::record_committed:
      return "record_committed";
    case ExactSparseAnchoredPairSessionStepKind::certified_prune_accounted:
      return "certified_prune_accounted";
    case ExactSparseAnchoredPairSessionStepKind::group_complete:
      return "group_complete";
    case ExactSparseAnchoredPairSessionStepKind::budget_exhausted:
      return "budget_exhausted";
    case ExactSparseAnchoredPairSessionStepKind::total_capacity_exhausted:
      return "total_capacity_exhausted";
    case ExactSparseAnchoredPairSessionStepKind::complete:
      return "complete";
  }
  return "unknown";
}

[[nodiscard]] std::string_view stop_reason_name(
    ExactSparseAnchoredPairSessionStopReason reason) noexcept {
  switch (reason) {
    case ExactSparseAnchoredPairSessionStopReason::none:
      return "none";
    case ExactSparseAnchoredPairSessionStopReason::schedule_advance_limit:
      return "schedule_advance_limit";
    case ExactSparseAnchoredPairSessionStopReason::orientation_check_limit:
      return "orientation_check_limit";
    case ExactSparseAnchoredPairSessionStopReason::grouped_traversal_node_visit_limit:
      return "grouped_traversal_node_visit_limit";
    case ExactSparseAnchoredPairSessionStopReason::grouped_traversal_exact_predicate_limit:
      return "grouped_traversal_exact_predicate_limit";
    case ExactSparseAnchoredPairSessionStopReason::classification_node_visit_limit:
      return "classification_node_visit_limit";
    case ExactSparseAnchoredPairSessionStopReason::emitted_record_limit:
      return "emitted_record_limit";
    case ExactSparseAnchoredPairSessionStopReason::emitted_point_id_reference_limit:
      return "emitted_point_id_reference_limit";
    case ExactSparseAnchoredPairSessionStopReason::total_schedule_advance_capacity:
      return "total_schedule_advance_capacity";
    case ExactSparseAnchoredPairSessionStopReason::total_orientation_check_capacity:
      return "total_orientation_check_capacity";
    case ExactSparseAnchoredPairSessionStopReason::total_grouped_traversal_node_visit_capacity:
      return "total_grouped_traversal_node_visit_capacity";
    case ExactSparseAnchoredPairSessionStopReason::total_grouped_traversal_exact_predicate_capacity:
      return "total_grouped_traversal_exact_predicate_capacity";
    case ExactSparseAnchoredPairSessionStopReason::total_admitted_candidate_capacity:
      return "total_admitted_candidate_capacity";
    case ExactSparseAnchoredPairSessionStopReason::total_classification_node_visit_capacity:
      return "total_classification_node_visit_capacity";
    case ExactSparseAnchoredPairSessionStopReason::total_output_record_capacity:
      return "total_output_record_capacity";
    case ExactSparseAnchoredPairSessionStopReason::total_output_point_id_reference_capacity:
      return "total_output_point_id_reference_capacity";
  }
  return "unknown";
}

struct SpatialIndexOwner {
  std::optional<MortonLbvhIndex> cpu_index;
#if MORSEHGP3D_MASSIVE_PREFIX_HAS_CUDA
  std::optional<morsehgp3d::gpu::MortonLbvhDeviceBuildResult> cuda_build;
#endif
  std::string_view backend{"not_built"};
  bool cuda_qualified{false};
  bool builder_released{false};
  std::uint64_t fixed_device_byte_capacity{};
  std::uint64_t required_snapshot_byte_count{};
  std::size_t device_kernel_launch_count{};
  std::size_t device_library_submission_count{};

  [[nodiscard]] const MortonLbvhIndex& index() const {
#if MORSEHGP3D_MASSIVE_PREFIX_HAS_CUDA
    if (cuda_build.has_value()) {
      return cuda_build->certified_index();
    }
#endif
    if (!cpu_index.has_value()) {
      throw std::logic_error("the massive smoke has no spatial index");
    }
    return *cpu_index;
  }
};

[[nodiscard]] SpatialIndexOwner build_spatial_index(
    const Options& options,
    const CanonicalPointCloud& cloud) {
  const bool request_cuda =
      options.spatial_backend == SpatialBackend::cuda14m ||
      options.spatial_backend == SpatialBackend::automatic;
#if MORSEHGP3D_MASSIVE_PREFIX_HAS_CUDA
  if (request_cuda) {
    SpatialIndexOwner owner;
    {
      morsehgp3d::gpu::MortonLbvhBuildContext context{cloud.size()};
      owner.cuda_build.emplace(context.build(cloud));
      if (!owner.cuda_build->cuda_qualified_build() ||
          !owner.cuda_build->certified_index().validated_for(cloud)) {
        throw std::runtime_error(
            "14M did not return a CUDA-qualified certified index");
      }
      const morsehgp3d::gpu::MortonLbvhDeviceBuildAudit& audit =
          owner.cuda_build->audit();
      owner.fixed_device_byte_capacity =
          static_cast<std::uint64_t>(audit.total_fixed_device_byte_capacity);
      owner.required_snapshot_byte_count =
          static_cast<std::uint64_t>(audit.required_snapshot_byte_count);
      owner.device_kernel_launch_count = audit.device_kernel_launch_count;
      owner.device_library_submission_count =
          audit.device_library_submission_count;
    }
    owner.backend = "cuda14m";
    owner.cuda_qualified = true;
    owner.builder_released = true;
    return owner;
  }
#else
  if (options.spatial_backend == SpatialBackend::cuda14m) {
    throw std::runtime_error(
        "cuda14m was requested from a CPU-only build");
  }
  static_cast<void>(request_cuda);
#endif
  SpatialIndexOwner owner;
  owner.cpu_index.emplace(MortonLbvhIndex::build(cloud));
  if (!owner.cpu_index->validated_for(cloud)) {
    throw std::runtime_error("the reference CPU LBVH failed validation");
  }
  owner.backend = "reference_cpu";
  owner.builder_released = true;
  return owner;
}

[[nodiscard]] int run(const Options& options) {
  const Clock::time_point total_start = Clock::now();
  double generation_ms = 0.0;
  double canonicalization_ms = 0.0;
  const Clock::time_point generation_start = Clock::now();
  CanonicalPointCloud cloud = [&]() {
    std::vector<CertifiedPoint3> input = make_affine_uniform_cloud(options);
    const Clock::time_point generated = Clock::now();
    generation_ms = milliseconds(generated - generation_start);
    CanonicalPointCloud canonical =
        CanonicalPointCloud::rejecting_duplicates(input);
    canonicalization_ms = milliseconds(Clock::now() - generated);
    return canonical;
  }();
  const bool input_released_before_spatial_build = true;

  const Clock::time_point spatial_start = Clock::now();
  SpatialIndexOwner spatial = build_spatial_index(options, cloud);
  const double spatial_ms = milliseconds(Clock::now() - spatial_start);
  const MortonLbvhIndex& index = spatial.index();

  if (!spatial.builder_released || !index.validated_for(cloud)) {
    throw std::runtime_error(
        "the spatial owner was not reduced to a certified index");
  }

  if (options.prefix_advance_count >
      std::numeric_limits<std::size_t>::max() / options.work_budget) {
    throw std::overflow_error("prefix total work capacity overflows size_t");
  }
  const std::size_t total_work_capacity =
      options.prefix_advance_count * options.work_budget;
  const ExactMortonGroupedAnchoredPairScheduleConfig schedule_config{
      32U, 64U};
  const ExactSparseAnchoredPairSessionTotalCapacity total_capacity{
      total_work_capacity,
      total_work_capacity,
      total_work_capacity,
      total_work_capacity,
      total_work_capacity,
      total_work_capacity,
      0U,
      0U};
  ExactSparseAnchoredPairSession session =
      ExactSparseAnchoredPairSession::start(
          index,
          cloud,
          options.maximum_order + 1U,
          schedule_config,
          total_capacity);
  if (!session.validated_for(index, cloud) || !session.records().empty()) {
    throw std::runtime_error("the bounded P8l prefix did not start cleanly");
  }

  const ExactSparseAnchoredPairSessionAdvanceBudget advance_budget{
      {options.work_budget,
       options.work_budget,
       options.work_budget,
       options.work_budget},
      {options.work_budget},
      0U,
      0U};
  const Clock::time_point prefix_start = Clock::now();
  std::size_t executed_advance_count = 0U;
  ExactSparseAnchoredPairSessionStepKind last_kind =
      ExactSparseAnchoredPairSessionStepKind::budget_exhausted;
  ExactSparseAnchoredPairSessionStopReason last_reason =
      ExactSparseAnchoredPairSessionStopReason::none;
  for (; executed_advance_count < options.prefix_advance_count;
       ++executed_advance_count) {
    ExactSparseAnchoredPairSessionStep step =
        session.advance(index, cloud, advance_budget);
    last_kind = step.kind();
    last_reason = step.stop_reason();
    if (step.kind() == ExactSparseAnchoredPairSessionStepKind::complete ||
        step.kind() ==
            ExactSparseAnchoredPairSessionStepKind::total_capacity_exhausted ||
        session.poisoned()) {
      ++executed_advance_count;
      break;
    }
  }
  const double prefix_ms = milliseconds(Clock::now() - prefix_start);
  if (!session.validated_for(index, cloud) || session.poisoned() ||
      !session.records().empty()) {
    throw std::runtime_error(
        "the massive P8l prefix retained output or lost provenance");
  }

  const ExactSparseAnchoredPairSessionAudit& audit = session.audit();
  const ExactMortonGroupedAnchoredPairCandidateAudit& candidate_audit =
      session.candidate_audit();
  const ExactMortonGroupedAnchoredPairScheduleAudit& schedule_audit =
      session.schedule_audit();
  const std::uint64_t cloud_point_bytes = checked_multiply(
      cloud.size(), sizeof(CertifiedPoint3), "cloud point bytes overflow");
  const std::uint64_t cloud_source_index_bytes = checked_multiply(
      cloud.size(), sizeof(std::size_t), "cloud source bytes overflow");
  const std::uint64_t lbvh_host_snapshot_bytes =
      cloud.size() == 0U
          ? 0U
          : checked_multiply(cloud.size(), 184U, "LBVH bytes overflow") -
                80U;

  std::cout << std::fixed << std::setprecision(3) << std::boolalpha
            << "{\"schema\":\"morsehgp3d.phase14."
               "massive_sparse_pair_prefix_smoke.v1\","
            << "\"backend\":\"cuda_g4_plus_reference_cpu\","
            << "\"profile\":\"hgp_reduced\","
            << "\"mode\":\"massive_sparse_pair_prefix_smoke\","
            << "\"deployment_status\":\"architecture_only\","
            << "\"public_status\":\"not_claimed\","
            << "\"point_count\":" << cloud.size() << ','
            << "\"K\":" << options.maximum_order << ','
            << "\"spatial_backend_used\":\"" << spatial.backend << "\","
            << "\"spatial_cuda_qualified\":" << spatial.cuda_qualified
            << ','
            << "\"input_released_before_spatial_build\":"
            << input_released_before_spatial_build << ','
            << "\"builder_released_before_pair_prefix\":"
            << spatial.builder_released << ','
            << "\"pipeline_complete\":false,"
            << "\"pair_authority_sealed\":false,"
            << "\"scientific_result_materialized\":false,"
            << "\"massive_product_path_claimed\":false,"
            << "\"ten_million_point_capacity_claimed\":false,"
            << "\"public_status_claimed\":false,"
            << "\"point_count_exceeds_ten_million\":"
            << (cloud.size() > 10'000'000U) << ','
            << "\"prefix\":{\"requested_advance_count\":"
            << options.prefix_advance_count
            << ",\"executed_advance_count\":" << executed_advance_count
            << ",\"work_budget\":" << options.work_budget
            << ",\"last_step_kind\":\"" << step_kind_name(last_kind)
            << "\",\"last_stop_reason\":\""
            << stop_reason_name(last_reason)
            << "\",\"session_complete\":" << session.complete()
            << ",\"total_capacity_exhausted\":"
            << session.total_capacity_exhausted()
            << ",\"retained_record_count\":" << session.records().size()
            << "},"
            << "\"work\":{\"session_advance_calls\":"
            << audit.advance_call_count
            << ",\"schedule_advances\":"
            << candidate_audit.schedule_advance_count
            << ",\"orientation_checks\":"
            << candidate_audit.orientation_check_count
            << ",\"grouped_node_visits\":"
            << candidate_audit.grouped_traversal_node_visit_count
            << ",\"grouped_exact_predicates\":"
            << candidate_audit.grouped_traversal_exact_predicate_count
            << ",\"classification_node_visits\":"
            << audit.classification_node_visit_count
            << ",\"maximum_pending_anchor_subgroups\":"
            << schedule_audit.maximum_pending_anchor_subgroup_count << "},"
            << "\"memory\":{\"cloud_point_payload_bytes\":"
            << cloud_point_bytes
            << ",\"cloud_source_index_bytes\":"
            << cloud_source_index_bytes
            << ",\"lbvh_host_snapshot_bytes\":"
            << lbvh_host_snapshot_bytes
            << ",\"spatial_fixed_device_byte_capacity\":"
            << spatial.fixed_device_byte_capacity
            << ",\"spatial_required_snapshot_bytes\":"
            << spatial.required_snapshot_byte_count
            << ",\"peak_host_rss_bytes\":" << peak_host_rss_bytes()
            << "},"
            << "\"cuda_work\":{\"kernel_launches\":"
            << spatial.device_kernel_launch_count
            << ",\"library_submissions\":"
            << spatial.device_library_submission_count << "},"
            << "\"timings_ms\":{\"generation\":" << generation_ms
            << ",\"canonicalization\":" << canonicalization_ms
            << ",\"spatial_build\":" << spatial_ms
            << ",\"pair_prefix\":" << prefix_ms
            << ",\"total\":" << milliseconds(Clock::now() - total_start)
            << "},"
            << "\"forbidden_materialization_counts\":{"
            << "\"global_pair_arena\":0,\"higher_support_records\":0,"
            << "\"terminal_facade_records\":0,\"event_journal_records\":0,"
            << "\"saddle_seed_records\":0,\"forest_nodes\":0,"
            << "\"higher_order_delaunay_cells\":0,\"cofaces\":0,"
            << "\"incidences\":0,\"gamma_structures\":0}}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(parse_options(argc, argv));
  } catch (const std::exception& error) {
    std::cerr << "massive_sparse_pair_prefix_smoke: " << error.what()
              << '\n';
    return 2;
  }
}
