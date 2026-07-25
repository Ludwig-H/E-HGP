#include "morsehgp3d/gpu/anchored_pair_candidate_recertifier.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include "pair_support_smoke_clouds.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(__linux__)
#include <sys/resource.h>
#endif

#if !defined(MORSEHGP3D_GIT_SHA)
#error "The Phase 14Q P8f component smoke requires a canonical Git SHA"
#endif

namespace {

using Clock = std::chrono::steady_clock;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::AnchoredPairCandidateProposalAudit;
using morsehgp3d::gpu::AnchoredPairCandidateProposalBatchResult;
using morsehgp3d::gpu::AnchoredPairCandidateProposalBudget;
using morsehgp3d::gpu::AnchoredPairCandidateProposalContext;
using morsehgp3d::gpu::AnchoredPairCandidateProposalQuery;
using morsehgp3d::gpu::ExactAnchoredPairCandidateRecertifierAudit;
using morsehgp3d::gpu::ExactAnchoredPairCandidateRecertifierBudget;
using morsehgp3d::gpu::ExactAnchoredPairCandidateRecertifierResult;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceBuildAudit;
using morsehgp3d::gpu::MortonLbvhDeviceBuildResult;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLeaseAudit;
using morsehgp3d::gpu::recertify_exact_anchored_pair_candidate_proposal;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;
namespace smoke_clouds = morsehgp3d::tools::pair_support_smoke;

constexpr std::size_t kMaximumWitnessBankSize = 64U;
constexpr std::size_t kWitnessGeometryBytesPerEntry =
    morsehgp3d::gpu::
        anchored_pair_candidate_gpu_precomputed_witness_geometry_bytes_per_entry;
constexpr std::size_t kTranscriptRecordBytes = 16U;
constexpr std::size_t kTraversalNodeBytes = 80U;
constexpr std::uint64_t kFnvOffsetBasis =
    UINT64_C(14695981039346656037);
constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);

struct Options {
  std::string family{"uniform_latin"};
  std::size_t point_count{50'000U};
  std::size_t query_count{64U};
  std::size_t maximum_order{10U};
  std::size_t bank_window{4'096U};
  std::size_t transcript_record_capacity{1'048'576U};
  std::size_t repetition_count{1U};
};

struct MortonWindowCandidate {
  double squared_distance{};
  PointId point_id{};
};

struct SampledAnchor {
  PointId anchor_point_id{};
  std::size_t morton_leaf_position{};
  std::vector<PointId> witness_bank_point_ids;
};

struct DurationSummary {
  std::uint64_t minimum_ns{};
  std::uint64_t median_ns{};
  std::uint64_t maximum_ns{};
};

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::overflow_error(message);
  }
  return left * right;
}

[[nodiscard]] std::size_t checked_node_count(std::size_t point_count) {
  if (point_count == 0U ||
      point_count >
          std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    throw std::overflow_error("the LBVH node count overflows size_t");
  }
  return 2U * point_count - 1U;
}

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    const char* message) {
  if (!std::in_range<std::uint64_t>(value)) {
    throw std::overflow_error(message);
  }
  return static_cast<std::uint64_t>(value);
}

template <typename Duration>
[[nodiscard]] std::uint64_t nanoseconds(Duration duration) {
  const auto count =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration)
          .count();
  if (count < 0 || !std::in_range<std::uint64_t>(count)) {
    throw std::runtime_error(
        "the monotonic component-smoke clock moved backwards or overflowed");
  }
  return static_cast<std::uint64_t>(count);
}

[[nodiscard]] std::size_t parse_positive_size(
    std::string_view text,
    std::string_view option) {
  std::size_t value = 0U;
  const auto parsed =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc{} ||
      parsed.ptr != text.data() + text.size() || value == 0U) {
    throw std::invalid_argument(
        std::string{option} + " expects a strictly positive integer");
  }
  return value;
}

void print_usage() {
  std::cout
      << "usage: morsehgp3d_gpu_anchored_pair_candidate_proposal_"
         "component_smoke [options]\n"
      << "  --family uniform_latin|eight_clusters\n"
      << "  --point-count N\n"
      << "  --query-count Q           (1 <= Q <= 4096)\n"
      << "  --K K                     (1 <= K <= 10)\n"
      << "  --bank-window W           (at most W Morton neighbors per anchor)\n"
      << "  --record-capacity T       (physical 16-byte transcript records)\n"
      << "  --repetitions R\n";
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  for (int argument_index = 1;
       argument_index < argc;
       ++argument_index) {
    const std::string_view option{argv[argument_index]};
    if (option == "--help" || option == "-h") {
      print_usage();
      std::exit(EXIT_SUCCESS);
    }
    if (argument_index + 1 >= argc) {
      throw std::invalid_argument(
          std::string{option} + " requires one value");
    }
    const std::string_view value{argv[++argument_index]};
    if (option == "--family") {
      options.family = std::string{value};
    } else if (option == "--point-count") {
      options.point_count = parse_positive_size(value, option);
    } else if (option == "--query-count") {
      options.query_count = parse_positive_size(value, option);
    } else if (option == "--K" || option == "--k") {
      options.maximum_order = parse_positive_size(value, option);
    } else if (option == "--bank-window") {
      options.bank_window = parse_positive_size(value, option);
    } else if (option == "--record-capacity") {
      options.transcript_record_capacity =
          parse_positive_size(value, option);
    } else if (option == "--repetitions") {
      options.repetition_count = parse_positive_size(value, option);
    } else {
      throw std::invalid_argument(
          "unknown component-smoke option " + std::string{option});
    }
  }

  if (options.family != "uniform_latin" &&
      options.family != "eight_clusters") {
    throw std::invalid_argument(
        "--family must be uniform_latin or eight_clusters");
  }
  if (options.point_count < 2U) {
    throw std::invalid_argument("--point-count must be at least two");
  }
  if (options.point_count > static_cast<std::size_t>(INT_MAX)) {
    throw std::length_error(
        "--point-count exceeds the certified CUB int item-count envelope");
  }
  if (options.point_count > CanonicalPointCloud::max_point_count) {
    throw std::length_error(
        "--point-count exceeds the canonical PointId envelope");
  }
  if (options.query_count >= options.point_count) {
    throw std::invalid_argument(
        "--query-count must leave at least one larger oriented PointId");
  }
  if (options.query_count >
      morsehgp3d::gpu::anchored_pair_candidate_gpu_maximum_query_tile_size) {
    throw std::invalid_argument(
        "--query-count exceeds the fixed 4096-query device tile");
  }
  if (options.maximum_order == 0U || options.maximum_order > 10U) {
    throw std::invalid_argument("--K must be in [1, 10]");
  }

  static_cast<void>(checked_node_count(options.point_count));
  static_cast<void>(checked_product(
      options.transcript_record_capacity,
      kTranscriptRecordBytes,
      "the physical transcript byte capacity overflows size_t"));
  static_cast<void>(checked_product(
      options.query_count,
      kMaximumWitnessBankSize,
      "the sampled witness-bank PointId capacity overflows size_t"));
  static_cast<void>(checked_product(
      options.query_count,
      options.bank_window,
      "the bounded Morton-window work envelope overflows size_t"));
  return options;
}

[[nodiscard]] std::vector<CertifiedPoint3> generate_points(
    const Options& options) {
  if (options.family == "uniform_latin") {
    return smoke_clouds::uniform_latin_points(options.point_count);
  }
  return smoke_clouds::eight_clusters_points(options.point_count);
}

[[nodiscard]] std::vector<PointId> sampled_anchor_point_ids(
    std::size_t point_count,
    std::size_t query_count) {
  const std::size_t oriented_anchor_count = point_count - 1U;
  const std::size_t base_step = oriented_anchor_count / query_count;
  const std::size_t remainder = oriented_anchor_count % query_count;
  if (base_step == 0U) {
    throw std::logic_error(
        "the sampled anchors would not be strictly increasing");
  }

  std::vector<PointId> anchors;
  anchors.reserve(query_count);
  std::size_t current = 0U;
  std::size_t carry = 0U;
  for (std::size_t query_index = 0U;
       query_index < query_count;
       ++query_index) {
    if (!std::in_range<PointId>(current) ||
        current >= oriented_anchor_count) {
      throw std::logic_error(
          "the sampled anchor arithmetic left its PointId domain");
    }
    anchors.push_back(static_cast<PointId>(current));
    current = checked_add(
        current,
        base_step,
        "the sampled anchor step overflows size_t");
    carry = checked_add(
        carry,
        remainder,
        "the sampled anchor carry overflows size_t");
    if (carry >= query_count) {
      carry -= query_count;
      current = checked_add(
          current,
          1U,
          "the sampled anchor remainder step overflows size_t");
    }
  }
  if (!std::is_sorted(anchors.begin(), anchors.end()) ||
      std::adjacent_find(anchors.begin(), anchors.end()) != anchors.end()) {
    throw std::logic_error("the sampled anchors are not canonical");
  }
  return anchors;
}

[[nodiscard]] std::size_t locate_sampled_anchors(
    std::span<const morsehgp3d::spatial::MortonLeafRecord> leaves,
    std::span<SampledAnchor> sampled_anchors) {
  std::unordered_map<PointId, std::size_t> query_by_anchor;
  query_by_anchor.reserve(sampled_anchors.size());
  for (std::size_t query_index = 0U;
       query_index < sampled_anchors.size();
       ++query_index) {
    const auto inserted = query_by_anchor.emplace(
        sampled_anchors[query_index].anchor_point_id, query_index);
    if (!inserted.second) {
      throw std::logic_error("the sampled anchor lookup has a duplicate");
    }
    sampled_anchors[query_index].morton_leaf_position =
        std::numeric_limits<std::size_t>::max();
  }

  std::size_t remaining = sampled_anchors.size();
  std::size_t inspected_leaf_count = 0U;
  for (std::size_t leaf_position = 0U;
       leaf_position < leaves.size() && remaining != 0U;
       ++leaf_position) {
    ++inspected_leaf_count;
    const auto found = query_by_anchor.find(leaves[leaf_position].point_id);
    if (found == query_by_anchor.end()) {
      continue;
    }
    SampledAnchor& anchor = sampled_anchors[found->second];
    if (anchor.morton_leaf_position !=
        std::numeric_limits<std::size_t>::max()) {
      throw std::logic_error(
          "a sampled anchor occurs twice in the certified Morton leaves");
    }
    anchor.morton_leaf_position = leaf_position;
    --remaining;
  }
  if (remaining != 0U) {
    throw std::logic_error(
        "the certified Morton leaves omit a sampled anchor");
  }
  return inspected_leaf_count;
}

[[nodiscard]] double ordinary_binary64_squared_distance(
    const CanonicalPointCloud& cloud,
    PointId first_point_id,
    PointId second_point_id) {
  double squared_distance = 0.0;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const double difference =
        cloud.point(first_point_id).binary64_coordinate(axis) -
        cloud.point(second_point_id).binary64_coordinate(axis);
    squared_distance += difference * difference;
  }
  if (!std::isfinite(squared_distance)) {
    throw std::runtime_error(
        "a bounded Morton-window binary64 distance is not finite");
  }
  return squared_distance;
}

[[nodiscard]] bool candidate_less(
    const MortonWindowCandidate& left,
    const MortonWindowCandidate& right) noexcept {
  if (left.squared_distance != right.squared_distance) {
    return left.squared_distance < right.squared_distance;
  }
  return left.point_id < right.point_id;
}

void build_sampled_witness_banks(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t bank_window,
    std::span<SampledAnchor> sampled_anchors,
    std::size_t& inspected_neighbor_count,
    std::size_t& distance_evaluation_count,
    std::size_t& witness_reference_count,
    std::size_t& maximum_bank_size) {
  const auto leaves = index.leaves();
  const std::size_t effective_bank_capacity = std::min(
      {kMaximumWitnessBankSize, bank_window, cloud.size() - 1U});
  const std::size_t shortlist_capacity = checked_add(
      effective_bank_capacity,
      1U,
      "the bounded Morton shortlist capacity overflows size_t");
  for (SampledAnchor& sampled_anchor : sampled_anchors) {
    if (sampled_anchor.morton_leaf_position >= leaves.size()) {
      throw std::logic_error(
          "a sampled anchor has no certified Morton position");
    }
    std::vector<MortonWindowCandidate> shortlist;
    // The replacement path inserts one better candidate before dropping the
    // previous worst entry.  Reserve that transient slot once per sampled
    // anchor so the bounded top-64 loop never reallocates.
    shortlist.reserve(shortlist_capacity);
    std::size_t inspected_for_anchor = 0U;
    const auto inspect = [&](std::size_t leaf_position) {
      const PointId point_id = leaves[leaf_position].point_id;
      if (point_id == sampled_anchor.anchor_point_id) {
        throw std::logic_error(
            "a bounded Morton witness window revisited its anchor");
      }
      const MortonWindowCandidate candidate{
          ordinary_binary64_squared_distance(
              cloud, sampled_anchor.anchor_point_id, point_id),
          point_id};
      const auto insertion = std::lower_bound(
          shortlist.begin(), shortlist.end(), candidate, candidate_less);
      if (shortlist.size() < effective_bank_capacity) {
        shortlist.insert(insertion, candidate);
      } else if (insertion != shortlist.end()) {
        shortlist.insert(insertion, candidate);
        shortlist.pop_back();
      }
      ++inspected_for_anchor;
    };

    const std::size_t anchor_position =
        sampled_anchor.morton_leaf_position;
    const std::size_t left_extent = anchor_position;
    const std::size_t right_extent =
        leaves.size() - anchor_position - 1U;
    const std::size_t maximum_offset =
        std::max(left_extent, right_extent);
    for (std::size_t offset = 1U;
         offset <= maximum_offset && inspected_for_anchor < bank_window;
         ++offset) {
      if (offset <= left_extent) {
        inspect(anchor_position - offset);
      }
      if (offset <= right_extent && inspected_for_anchor < bank_window) {
        inspect(anchor_position + offset);
      }
    }
    if (inspected_for_anchor == 0U || shortlist.empty()) {
      throw std::logic_error(
          "a sampled anchor produced an empty bounded witness bank");
    }
    inspected_neighbor_count = checked_add(
        inspected_neighbor_count,
        inspected_for_anchor,
        "the Morton neighbor inspection count overflows size_t");
    distance_evaluation_count = checked_add(
        distance_evaluation_count,
        inspected_for_anchor,
        "the binary64 distance count overflows size_t");
    sampled_anchor.witness_bank_point_ids.reserve(shortlist.size());
    for (const MortonWindowCandidate& candidate : shortlist) {
      sampled_anchor.witness_bank_point_ids.push_back(candidate.point_id);
    }
    witness_reference_count = checked_add(
        witness_reference_count,
        sampled_anchor.witness_bank_point_ids.size(),
        "the witness reference count overflows size_t");
    maximum_bank_size = std::max(
        maximum_bank_size,
        sampled_anchor.witness_bank_point_ids.size());
  }
}

[[nodiscard]] std::uint64_t digest_word(
    std::uint64_t digest,
    std::uint64_t word) noexcept {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
  return digest;
}

[[nodiscard]] std::uint64_t sampled_bank_digest(
    std::span<const SampledAnchor> sampled_anchors) {
  std::uint64_t digest = kFnvOffsetBasis;
  digest = digest_word(digest, UINT64_C(0x50384542414e4b31));
  digest = digest_word(
      digest,
      checked_u64(
          sampled_anchors.size(),
          "the sampled query count does not fit uint64"));
  for (const SampledAnchor& anchor : sampled_anchors) {
    digest = digest_word(digest, anchor.anchor_point_id);
    digest = digest_word(
        digest,
        checked_u64(
            anchor.witness_bank_point_ids.size(),
            "a witness bank size does not fit uint64"));
    for (const PointId witness : anchor.witness_bank_point_ids) {
      digest = digest_word(digest, witness);
    }
  }
  return digest;
}

[[nodiscard]] std::uint64_t recertified_output_digest(
    const ExactAnchoredPairCandidateRecertifierResult& result) {
  std::uint64_t digest = kFnvOffsetBasis;
  digest = digest_word(digest, UINT64_C(0x5038455245434552));
  digest = digest_word(
      digest,
      checked_u64(
          result.anchors.size(),
          "the recertified anchor count does not fit uint64"));
  for (const auto& anchor : result.anchors) {
    digest = digest_word(
        digest,
        checked_u64(
            anchor.query_index,
            "a recertified query index does not fit uint64"));
    digest = digest_word(digest, anchor.query_digest_fnv1a);
    digest = digest_word(digest, anchor.anchor_point_id);
    digest = digest_word(
        digest,
        checked_u64(
            anchor.maximum_closed_rank,
            "a closed-rank bound does not fit uint64"));
    digest = digest_word(
        digest,
        checked_u64(
            anchor.candidate_point_ids.size(),
            "a candidate count does not fit uint64"));
    for (const PointId candidate : anchor.candidate_point_ids) {
      digest = digest_word(digest, candidate);
    }
    digest = digest_word(
        digest,
        checked_u64(
            anchor.prune_receipts.size(),
            "a prune receipt count does not fit uint64"));
    for (const auto& receipt : anchor.prune_receipts) {
      digest = digest_word(
          digest,
          checked_u64(
              receipt.lbvh_node_index,
              "a prune node index does not fit uint64"));
      digest = digest_word(
          digest,
          checked_u64(
              receipt.leaf_begin,
              "a prune leaf begin does not fit uint64"));
      digest = digest_word(
          digest,
          checked_u64(
              receipt.leaf_end,
              "a prune leaf end does not fit uint64"));
      digest = digest_word(
          digest,
          checked_u64(
              receipt.witness_count,
              "a prune witness count does not fit uint64"));
      for (std::size_t witness_index = 0U;
           witness_index < receipt.witness_count;
           ++witness_index) {
        digest = digest_word(
            digest, receipt.witness_point_ids[witness_index]);
      }
    }
  }
  return digest;
}

[[nodiscard]] DurationSummary summarize_durations(
    const std::vector<std::uint64_t>& durations) {
  if (durations.empty()) {
    throw std::logic_error("cannot summarize an empty duration sample");
  }
  std::vector<std::uint64_t> ordered = durations;
  std::sort(ordered.begin(), ordered.end());
  return DurationSummary{
      ordered.front(),
      ordered[(ordered.size() - 1U) / 2U],
      ordered.back()};
}

void print_duration_samples(
    std::string_view field,
    const std::vector<std::uint64_t>& durations) {
  std::cout << '"' << field << "\":[";
  for (std::size_t index = 0U; index < durations.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    std::cout << durations[index];
  }
  std::cout << "],";
}

[[nodiscard]] std::uint64_t peak_host_rss_bytes() {
#if defined(__linux__)
  rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) != 0 || usage.ru_maxrss < 0) {
    throw std::runtime_error(
        "getrusage could not read the component-smoke peak RSS");
  }
  constexpr std::uint64_t kBytesPerKibibyte = UINT64_C(1024);
  if (!std::in_range<std::uint64_t>(usage.ru_maxrss)) {
    throw std::overflow_error("the peak RSS does not fit uint64");
  }
  const std::uint64_t kibibytes =
      static_cast<std::uint64_t>(usage.ru_maxrss);
  if (kibibytes >
      std::numeric_limits<std::uint64_t>::max() / kBytesPerKibibyte) {
    throw std::overflow_error("the peak RSS byte count overflows uint64");
  }
  return kibibytes * kBytesPerKibibyte;
#else
  return 0U;
#endif
}

void require_cuda_build_and_lease(
    const MortonLbvhDeviceBuildResult& build,
    const MortonLbvhDeviceTraversalLease& lease,
    const CanonicalPointCloud& cloud,
    std::size_t expected_node_count,
    std::size_t expected_persistent_device_bytes) {
  const MortonLbvhDeviceBuildAudit& build_audit = build.audit();
  const MortonLbvhDeviceTraversalLeaseAudit& lease_audit = lease.audit();
  if (!build.complete_certified_build() || !build.cuda_qualified_build() ||
      !build.certified_index().validated_for(cloud) || !lease.ready() ||
      !lease.cuda_resident() || lease_audit.point_count != cloud.size() ||
      lease_audit.certified_node_count != expected_node_count ||
      lease_audit.retained_coordinate_word_capacity != 3U * cloud.size() ||
      lease_audit.retained_morton_point_id_capacity != cloud.size() ||
      lease_audit.retained_node_capacity != expected_node_count ||
      lease_audit.retained_node_byte_capacity !=
          kTraversalNodeBytes * expected_node_count ||
      lease_audit.persistent_device_byte_capacity !=
          expected_persistent_device_bytes ||
      lease_audit.retained_host_snapshot_byte_count != 0U ||
      !lease_audit.source_snapshot_import_certified ||
      !lease_audit.canonical_coordinate_words_retained ||
      !lease_audit.active_morton_point_ids_retained ||
      !lease_audit.certified_device_nodes_retained ||
      !lease_audit.builder_transients_released ||
      !lease_audit.cuda_device_storage_retained ||
      lease_audit.host_fake_lifecycle_exercised ||
      lease_audit.second_host_snapshot_retained ||
      lease_audit.higher_order_delaunay_mosaic_materialized ||
      lease_audit.global_cell_or_coface_arena_materialized ||
      lease_audit.public_status_claimed ||
      build_audit.higher_order_delaunay_mosaic_materialized ||
      build_audit.global_cell_or_coface_arena_materialized ||
      build_audit.public_status_claimed) {
    throw std::runtime_error(
        "the CUDA Morton/LBVH traversal lease did not close its component contract");
  }
}

void require_complete_cuda_proposal(
    const AnchoredPairCandidateProposalBatchResult& proposal,
    std::size_t query_count) {
  const AnchoredPairCandidateProposalAudit& audit = proposal.audit;
  const std::size_t expected_witness_geometry_bytes = checked_product(
      checked_product(
          query_count,
          kMaximumWitnessBankSize,
          "the sampled witness-geometry count overflows size_t"),
      kWitnessGeometryBytesPerEntry,
      "the sampled witness-geometry bytes overflow size_t");
  if (!proposal.validated_proposal_batch() || proposal.scientific_outcome() ||
      audit.canonical_query_count != query_count ||
      audit.complete_segment_count != query_count ||
      audit.budget_exhausted_segment_count != 0U ||
      audit.capacity_exhausted_segment_count != 0U ||
      !audit.gpu_execution_performed ||
      !audit.query_witness_geometry_precomputed ||
      audit.precomputed_witness_geometry_byte_capacity !=
          expected_witness_geometry_bytes ||
      audit.gpu_witness_geometry_precomputation_kernel_launch_count != 1U ||
      audit.gpu_kernel_launch_count != 4U ||
      audit.gpu_compaction_kernel_launch_count != 1U ||
      audit.gpu_synchronization_count != 1U ||
      !audit.device_serial_prefix_clamp_performed ||
      audit.candidate_leaf_status_recertified ||
      audit.strict_witness_masks_recertified ||
      audit.exact_closed_ball_partition_published ||
      audit.scientific_decision_published ||
      audit.hierarchy_reduction_or_attachment_published ||
      audit.forbidden_global_structure_materialized ||
      audit.transcript_accumulated_across_calls ||
      audit.public_status_claimed) {
    throw std::runtime_error(
        "the CUDA anchored-pair proposal was incomplete or exceeded component scope");
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    constexpr std::string_view kGitSha{MORSEHGP3D_GIT_SHA};
    if (kGitSha.size() != 40U) {
      throw std::runtime_error(
          "the component smoke requires a canonical 40-character Git SHA");
    }
    const Options options = parse_options(argc, argv);
    const std::size_t node_count = checked_node_count(options.point_count);
    const std::size_t traversal_node_bytes = checked_product(
        node_count,
        kTraversalNodeBytes,
        "the traversal node arena byte count overflows size_t");
    const std::size_t traversal_persistent_device_bytes = checked_add(
        checked_product(
            options.point_count,
            32U,
            "the traversal coordinate/PointId byte count overflows size_t"),
        traversal_node_bytes,
        "the traversal lease byte count overflows size_t");
    const std::size_t physical_transcript_bytes = checked_product(
        options.transcript_record_capacity,
        kTranscriptRecordBytes,
        "the physical transcript byte count overflows size_t");
    const std::size_t sampled_bank_point_id_capacity = checked_product(
        options.query_count,
        kMaximumWitnessBankSize,
        "the sampled bank capacity overflows size_t");
    const std::size_t sampled_bank_byte_capacity = checked_product(
        sampled_bank_point_id_capacity,
        sizeof(PointId),
        "the sampled bank byte capacity overflows size_t");
    const std::size_t precomputed_witness_geometry_byte_capacity =
        checked_product(
            sampled_bank_point_id_capacity,
            kWitnessGeometryBytesPerEntry,
            "the sampled witness-geometry byte capacity overflows size_t");
    const std::size_t bounded_window_work_envelope = checked_product(
        options.query_count,
        options.bank_window,
        "the bounded Morton-window work envelope overflows size_t");

    const auto total_start = Clock::now();
    const auto generation_start = Clock::now();
    std::vector<CertifiedPoint3> input_points = generate_points(options);
    const auto generation_end = Clock::now();

    const auto canonicalization_start = Clock::now();
    CanonicalPointCloud cloud =
        CanonicalPointCloud::rejecting_duplicates(input_points);
    const auto canonicalization_end = Clock::now();
    const auto input_release_start = Clock::now();
    input_points.clear();
    input_points.shrink_to_fit();
    const auto input_release_end = Clock::now();

    MortonLbvhBuildContext build_context{options.point_count};
    const auto lbvh_build_start = Clock::now();
    MortonLbvhDeviceBuildResult build = build_context.build(cloud);
    const auto lbvh_build_end = Clock::now();
    const MortonLbvhDeviceBuildAudit build_audit = build.audit();

    const auto lease_release_start = Clock::now();
    MortonLbvhDeviceTraversalLease traversal_lease =
        build_context.release_device_traversal_lease(build);
    const auto lease_release_end = Clock::now();
    require_cuda_build_and_lease(
        build,
        traversal_lease,
        cloud,
        node_count,
        traversal_persistent_device_bytes);
    const MortonLbvhDeviceTraversalLeaseAudit lease_audit =
        traversal_lease.audit();

    const MortonLbvhIndex& index = build.certified_index();
    const std::vector<PointId> anchor_point_ids =
        sampled_anchor_point_ids(options.point_count, options.query_count);
    std::vector<SampledAnchor> sampled_anchors;
    sampled_anchors.reserve(options.query_count);
    for (const PointId anchor_point_id : anchor_point_ids) {
      SampledAnchor sampled_anchor;
      sampled_anchor.anchor_point_id = anchor_point_id;
      sampled_anchors.push_back(std::move(sampled_anchor));
    }

    const auto anchor_location_start = Clock::now();
    const std::size_t inspected_morton_leaf_count =
        locate_sampled_anchors(index.leaves(), sampled_anchors);
    const auto anchor_location_end = Clock::now();

    std::size_t inspected_neighbor_count = 0U;
    std::size_t distance_evaluation_count = 0U;
    std::size_t witness_reference_count = 0U;
    std::size_t maximum_bank_size = 0U;
    const auto bank_construction_start = Clock::now();
    build_sampled_witness_banks(
        index,
        cloud,
        options.bank_window,
        sampled_anchors,
        inspected_neighbor_count,
        distance_evaluation_count,
        witness_reference_count,
        maximum_bank_size);
    const auto bank_construction_end = Clock::now();
    if (inspected_neighbor_count > bounded_window_work_envelope ||
        distance_evaluation_count != inspected_neighbor_count ||
        witness_reference_count > sampled_bank_point_id_capacity ||
        maximum_bank_size > kMaximumWitnessBankSize) {
      throw std::logic_error(
          "the sampled Morton witness-bank work escaped its preflight envelope");
    }
    const std::uint64_t bank_digest = sampled_bank_digest(sampled_anchors);

    std::vector<AnchoredPairCandidateProposalQuery> queries;
    queries.reserve(options.query_count);
    const std::size_t maximum_closed_rank = options.maximum_order + 1U;
    for (const SampledAnchor& anchor : sampled_anchors) {
      queries.push_back(AnchoredPairCandidateProposalQuery{
          anchor.anchor_point_id,
          maximum_closed_rank,
          std::span<const PointId>{anchor.witness_bank_point_ids},
          {}});
    }

    const auto proposal_context_start = Clock::now();
    AnchoredPairCandidateProposalContext proposal_context{
        cloud,
        std::move(traversal_lease),
        options.query_count,
        options.transcript_record_capacity};
    const auto proposal_context_end = Clock::now();
    if (traversal_lease.ready() ||
        proposal_context.point_count() != options.point_count ||
        proposal_context.certified_node_count() != node_count ||
        proposal_context.maximum_query_count() != options.query_count ||
        proposal_context.maximum_transcript_record_count() !=
            options.transcript_record_capacity) {
      throw std::runtime_error(
          "the proposal context did not consume the traversal lease exactly once");
    }

    const AnchoredPairCandidateProposalBudget proposal_budget{
        node_count,
        options.transcript_record_capacity,
        options.transcript_record_capacity};
    std::vector<std::uint64_t> proposal_durations;
    std::vector<std::uint64_t> recertification_durations;
    std::vector<std::uint64_t> component_durations;
    proposal_durations.reserve(options.repetition_count);
    recertification_durations.reserve(options.repetition_count);
    component_durations.reserve(options.repetition_count);

    AnchoredPairCandidateProposalAudit last_proposal_audit;
    ExactAnchoredPairCandidateRecertifierAudit last_recertification_audit;
    std::uint64_t first_proposal_digest = 0U;
    std::uint64_t last_proposal_digest = 0U;
    std::uint64_t first_buffer_epoch = 0U;
    std::uint64_t last_buffer_epoch = 0U;
    std::uint64_t expected_recertified_digest = 0U;
    std::size_t expected_candidate_count = 0U;
    std::size_t expected_prune_count = 0U;
    std::size_t expected_exact_predicate_count = 0U;

    for (std::size_t repetition = 0U;
         repetition < options.repetition_count;
         ++repetition) {
      const auto component_start = Clock::now();
      const auto proposal_start = Clock::now();
      const AnchoredPairCandidateProposalBatchResult proposal =
          proposal_context.build(cloud, queries, proposal_budget);
      const auto proposal_end = Clock::now();
      require_complete_cuda_proposal(proposal, options.query_count);

      const std::size_t exact_predicate_capacity = checked_product(
          proposal.audit.prune_proposal_count,
          options.maximum_order,
          "the exact witness recertification budget overflows size_t");
      const ExactAnchoredPairCandidateRecertifierBudget recertifier_budget{
          options.query_count,
          proposal.records.size(),
          proposal.audit.candidate_leaf_proposal_count,
          proposal.audit.prune_proposal_count,
          exact_predicate_capacity};
      const auto recertification_start = Clock::now();
      const ExactAnchoredPairCandidateRecertifierResult recertified =
          recertify_exact_anchored_pair_candidate_proposal(
              index,
              cloud,
              queries,
              proposal,
              recertifier_budget);
      const auto recertification_end = Clock::now();
      const auto component_end = Clock::now();
      if (!recertified.complete_exact_candidate_stream() ||
          recertified.scientific_morse_outcome() ||
          recertified.audit.recertified_anchor_count !=
              options.query_count ||
          recertified.audit.forbidden_global_structure_materialized ||
          recertified.audit.public_status_claimed) {
        throw std::runtime_error(
            "the reference CPU did not recertify every complete CUDA segment");
      }

      const std::uint64_t current_recertified_digest =
          recertified_output_digest(recertified);
      if (repetition == 0U) {
        first_proposal_digest = proposal.audit.proposal_digest_fnv1a;
        first_buffer_epoch = proposal.audit.buffer_epoch;
        expected_recertified_digest = current_recertified_digest;
        expected_candidate_count =
            recertified.audit.recertified_candidate_point_id_count;
        expected_prune_count =
            recertified.audit.recertified_prune_receipt_count;
        expected_exact_predicate_count =
            recertified.audit.exact_witness_predicate_count;
      } else if (
          current_recertified_digest != expected_recertified_digest ||
          recertified.audit.recertified_candidate_point_id_count !=
              expected_candidate_count ||
          recertified.audit.recertified_prune_receipt_count !=
              expected_prune_count ||
          recertified.audit.exact_witness_predicate_count !=
              expected_exact_predicate_count) {
        throw std::runtime_error(
            "repeated CUDA proposals changed the exact recertified component output");
      }

      last_proposal_digest = proposal.audit.proposal_digest_fnv1a;
      last_buffer_epoch = proposal.audit.buffer_epoch;
      last_proposal_audit = proposal.audit;
      last_recertification_audit = recertified.audit;
      proposal_durations.push_back(
          nanoseconds(proposal_end - proposal_start));
      recertification_durations.push_back(
          nanoseconds(recertification_end - recertification_start));
      component_durations.push_back(
          nanoseconds(component_end - component_start));
    }
    const auto total_end = Clock::now();

    if (last_proposal_audit.witness_point_id_reference_count !=
            witness_reference_count ||
        last_proposal_audit.maximum_observed_witness_bank_size !=
            maximum_bank_size ||
        last_proposal_audit.persistent_device_byte_capacity !=
            traversal_persistent_device_bytes ||
        last_proposal_audit.physical_transcript_record_capacity !=
            options.transcript_record_capacity ||
        last_proposal_audit.active_total_transcript_record_limit !=
            options.transcript_record_capacity ||
        last_proposal_audit.precomputed_witness_geometry_byte_capacity !=
            precomputed_witness_geometry_byte_capacity ||
        last_recertification_audit.input_transcript_record_count !=
            last_proposal_audit.compacted_host_transcript_record_count) {
      throw std::runtime_error(
          "the proposal/replay audit disagrees with the bounded component inputs");
    }

    const DurationSummary proposal_summary =
        summarize_durations(proposal_durations);
    const DurationSummary recertification_summary =
        summarize_durations(recertification_durations);
    const DurationSummary component_summary =
        summarize_durations(component_durations);

    std::cout
        << "{\"schema\":\"morsehgp3d.phase14q.p8f_anchored_pair_"
           "candidate_component_smoke.v1\","
        << "\"git_sha\":\"" << kGitSha << "\","
        << "\"phase\":\"14Q-P8f\","
        << "\"backend\":\"cuda_g4_plus_reference_cpu\","
        << "\"profile\":\"hgp_reduced\","
        << "\"mode\":\"sampled_cuda_proposal_exact_cpu_"
           "recertification_component\","
        << "\"deployment_status\":\"architecture_only\","
        << "\"public_status\":\"not_claimed\","
        << "\"family\":\"" << options.family << "\","
        << "\"point_count\":" << options.point_count << ','
        << "\"sampled_query_count\":" << options.query_count << ','
        << "\"K\":" << options.maximum_order << ','
        << "\"maximum_closed_rank\":" << maximum_closed_rank << ','
        << "\"bank_window_inspection_limit_per_anchor\":"
        << options.bank_window << ','
        << "\"maximum_witness_bank_size\":" << maximum_bank_size << ','
        << "\"witness_point_id_reference_count\":"
        << witness_reference_count << ','
        << "\"sampled_bank_digest_fnv1a\":" << bank_digest << ','
        << "\"repetitions\":" << options.repetition_count << ',';
    print_duration_samples("proposal_samples_ns", proposal_durations);
    print_duration_samples(
        "exact_recertification_samples_ns", recertification_durations);
    print_duration_samples("sampled_component_samples_ns", component_durations);
    std::cout
        << "\"generation_ns\":"
        << nanoseconds(generation_end - generation_start) << ','
        << "\"canonicalization_ns\":"
        << nanoseconds(canonicalization_end - canonicalization_start) << ','
        << "\"input_release_ns\":"
        << nanoseconds(input_release_end - input_release_start) << ','
        << "\"lbvh_build_ns\":"
        << nanoseconds(lbvh_build_end - lbvh_build_start) << ','
        << "\"traversal_lease_release_ns\":"
        << nanoseconds(lease_release_end - lease_release_start) << ','
        << "\"sampled_anchor_location_ns\":"
        << nanoseconds(anchor_location_end - anchor_location_start) << ','
        << "\"sampled_bank_construction_ns\":"
        << nanoseconds(bank_construction_end - bank_construction_start) << ','
        << "\"proposal_context_construction_ns\":"
        << nanoseconds(proposal_context_end - proposal_context_start) << ','
        << "\"proposal_min_ns\":" << proposal_summary.minimum_ns << ','
        << "\"proposal_median_ns\":" << proposal_summary.median_ns << ','
        << "\"proposal_max_ns\":" << proposal_summary.maximum_ns << ','
        << "\"exact_recertification_min_ns\":"
        << recertification_summary.minimum_ns << ','
        << "\"exact_recertification_median_ns\":"
        << recertification_summary.median_ns << ','
        << "\"exact_recertification_max_ns\":"
        << recertification_summary.maximum_ns << ','
        << "\"sampled_component_min_ns\":"
        << component_summary.minimum_ns << ','
        << "\"sampled_component_median_ns\":"
        << component_summary.median_ns << ','
        << "\"sampled_component_max_ns\":"
        << component_summary.maximum_ns << ','
        << "\"total_process_ns\":" << nanoseconds(total_end - total_start)
        << ','
        << "\"morton_leaf_scan_count\":"
        << inspected_morton_leaf_count << ','
        << "\"morton_window_neighbor_inspection_count\":"
        << inspected_neighbor_count << ','
        << "\"binary64_distance_evaluation_count\":"
        << distance_evaluation_count << ','
        << "\"bounded_window_work_envelope\":"
        << bounded_window_work_envelope << ','
        << "\"certified_node_count\":" << node_count << ','
        << "\"proposal_node_visit_count\":"
        << last_proposal_audit.node_visit_count << ','
        << "\"candidate_leaf_proposal_count\":"
        << last_proposal_audit.candidate_leaf_proposal_count << ','
        << "\"prune_proposal_count\":"
        << last_proposal_audit.prune_proposal_count << ','
        << "\"recertified_candidate_point_id_count\":"
        << expected_candidate_count << ','
        << "\"recertified_prune_receipt_count\":"
        << expected_prune_count << ','
        << "\"exact_witness_predicate_count\":"
        << expected_exact_predicate_count << ','
        << "\"replayed_node_visit_count\":"
        << last_recertification_audit.replayed_node_visit_count << ','
        << "\"recertified_output_digest_fnv1a\":"
        << expected_recertified_digest << ','
        << "\"first_proposal_digest_fnv1a\":"
        << first_proposal_digest << ','
        << "\"last_proposal_digest_fnv1a\":"
        << last_proposal_digest << ','
        << "\"first_buffer_epoch\":" << first_buffer_epoch << ','
        << "\"last_buffer_epoch\":" << last_buffer_epoch << ','
        << "\"physical_query_capacity\":"
        << last_proposal_audit.maximum_query_count << ','
        << "\"physical_transcript_record_capacity\":"
        << options.transcript_record_capacity << ','
        << "\"physical_transcript_record_capacity_bytes\":"
        << physical_transcript_bytes << ','
        << "\"compacted_host_transcript_record_count\":"
        << last_proposal_audit.compacted_host_transcript_record_count << ','
        << "\"sampled_bank_point_id_capacity_bytes\":"
        << sampled_bank_byte_capacity << ','
        << "\"precomputed_witness_geometry_capacity_bytes\":"
        << precomputed_witness_geometry_byte_capacity << ','
        << "\"active_query_h2d_bytes\":"
        << last_proposal_audit.active_host_to_device_query_byte_count << ','
        << "\"initialized_device_segment_bytes\":"
        << last_proposal_audit.initialized_device_segment_byte_count << ','
        << "\"initialized_device_transcript_bytes\":"
        << last_proposal_audit.initialized_device_transcript_byte_count << ','
        << "\"copied_device_to_host_segment_bytes\":"
        << last_proposal_audit.copied_device_to_host_segment_byte_count << ','
        << "\"copied_device_to_host_transcript_bytes\":"
        << last_proposal_audit.copied_device_to_host_transcript_byte_count
        << ','
        << "\"build_total_fixed_device_capacity_bytes\":"
        << build_audit.total_fixed_device_byte_capacity << ','
        << "\"build_sort_temporary_capacity_bytes\":"
        << build_audit.device_sort_temporary_byte_capacity << ','
        << "\"traversal_persistent_device_capacity_bytes\":"
        << lease_audit.persistent_device_byte_capacity << ','
        << "\"released_transient_device_capacity_bytes\":"
        << lease_audit.released_transient_device_byte_capacity << ','
        << "\"retained_host_snapshot_bytes\":"
        << lease_audit.retained_host_snapshot_byte_count << ','
        << "\"peak_host_rss_bytes\":" << peak_host_rss_bytes() << ','
        << "\"build_project_kernel_launch_count\":"
        << build_audit.device_kernel_launch_count << ','
        << "\"build_library_submission_count\":"
        << build_audit.device_library_submission_count << ','
        << "\"build_synchronization_count\":"
        << build_audit.device_synchronization_count << ','
        << "\"proposal_kernel_launch_count\":"
        << last_proposal_audit.gpu_kernel_launch_count << ','
        << "\"proposal_prefix_clamp_kernel_launch_count\":"
        << last_proposal_audit.gpu_compaction_kernel_launch_count << ','
        << "\"proposal_witness_geometry_kernel_launch_count\":"
        << last_proposal_audit
               .gpu_witness_geometry_precomputation_kernel_launch_count
        << ','
        << "\"proposal_synchronization_count\":"
        << last_proposal_audit.gpu_synchronization_count << ','
        << "\"cuda_device\":" << last_proposal_audit.cuda_device << ','
        << "\"device_serial_prefix_clamp_performed\":true,"
        << "\"query_witness_geometry_precomputed\":true,"
        << "\"sampled_anchor_locator_is_o_n_inverse\":false,"
        << "\"sampled_anchor_locator_entry_count\":"
        << options.query_count << ','
        << "\"all_sampled_segments_exactly_recertified\":true,"
        << "\"component_only\":true,"
        << "\"qualification_claimed\":false,"
        << "\"warm_e2e_primary_slo_claimed\":false,"
        << "\"warm_e2e_secondary_objective_claimed\":false,"
        << "\"full_pipeline_executed\":false,"
        << "\"complete_all_anchor_pass_executed\":false,"
        << "\"ten_million_product_path_claimed\":false,"
        << "\"massive_product_path_claimed\":false,"
        << "\"higher_order_supports_connected\":false,"
        << "\"scientific_result_claimed\":false,"
        << "\"hierarchy_reduction_or_attachment_published\":false,"
        << "\"forbidden_global_structure_materialized\":false,"
        << "\"public_status_claimed\":false}"
        << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
