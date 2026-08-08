#include "../cuda/phase15_jung_chord_csr_tile_internal.hpp"
#include "pair_support_smoke_clouds.hpp"

#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/gpu/jung_chord_csr_tile.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::gpu::JungChordAnchor;
using morsehgp3d::gpu::JungChordAnchorTile;
using morsehgp3d::gpu::JungChordCsrTileAdvance;
using morsehgp3d::gpu::JungChordCsrTileConfig;
using morsehgp3d::gpu::JungChordCsrTileContext;
using morsehgp3d::gpu::JungChordCsrTileStatus;
using morsehgp3d::gpu::JungChordCsrTileStopReason;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::detail::Phase15JungChordCsrQualificationSnapshot;
using morsehgp3d::gpu::detail::
    Phase15JungChordCsrTilePrivateViewAccess;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

inline constexpr std::string_view kSchema =
    "morsehgp3d.phase15.jung_chord_csr_tile.qualification_scale.v1";
#if defined(MORSEHGP3D_GIT_SHA)
inline constexpr std::string_view kGitSha = MORSEHGP3D_GIT_SHA;
#else
inline constexpr std::string_view kGitSha = "unavailable";
#endif

struct Options final {
  std::size_t point_count{50'000U};
  std::string family{"uniform_latin"};
  std::size_t support_size{4U};
  std::size_t maximum_closed_rank{11U};
  std::size_t morton_window{8U};
  std::size_t chord_capacity{64U * 1024U * 1024U};
  bool oracle{};
};

struct ExactAnchor final {
  std::uint64_t constant_interior{};
  std::vector<PointId> active;
  std::vector<PointId> support_eligible;
  std::vector<PointId> lens_eligible;
};

struct OracleAudit final {
  bool performed{};
  bool passed{};
  std::size_t anchor_count{};
  std::size_t false_depth_rejection_count{};
  std::size_t active_omission_count{};
  std::size_t fail_open_extra_count{};
  std::size_t constant_lower_bound_violation_count{};
  std::size_t support_eligible_false_negative_count{};
  std::size_t support_eligible_unjustified_nonambiguous_count{};
  std::size_t anchor_identity_violation_count{};
  std::size_t csr_structure_violation_count{};
  std::size_t duplicate_or_invalid_chord_id_count{};
  std::size_t unknown_flag_count{};
  std::size_t copied_chord_count{};
  std::size_t copied_byte_count{};
  bool exact_q4_boundary_sign_fixture_passed{};
  bool cuda_q4_boundary_fixture_exercised{};
};

[[nodiscard]] std::uint64_t ns_between(
    Clock::time_point begin,
    Clock::time_point end) {
  const auto elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
  return elapsed < 0 ? 0U : static_cast<std::uint64_t>(elapsed);
}

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    const char* option) {
  std::size_t value = 0U;
  const auto result =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
    throw std::invalid_argument(std::string{"invalid value for "} + option);
  }
  return value;
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view option{argv[index]};
    if (option == "--oracle") {
      options.oracle = true;
      continue;
    }
    if (index + 1 >= argc) {
      throw std::invalid_argument(std::string{"missing value after "} +
                                  std::string{option});
    }
    const std::string_view value{argv[++index]};
    if (option == "--point-count") {
      options.point_count = parse_size(value, "--point-count");
    } else if (option == "--family") {
      options.family = std::string{value};
    } else if (option == "--support-size") {
      options.support_size = parse_size(value, "--support-size");
    } else if (option == "--maximum-closed-rank") {
      options.maximum_closed_rank =
          parse_size(value, "--maximum-closed-rank");
    } else if (option == "--morton-window") {
      options.morton_window = parse_size(value, "--morton-window");
    } else if (option == "--chord-capacity") {
      options.chord_capacity = parse_size(value, "--chord-capacity");
    } else {
      throw std::invalid_argument(std::string{"unknown option "} +
                                  std::string{option});
    }
  }
  if (options.point_count < 2U || options.morton_window == 0U ||
      (options.support_size != 3U && options.support_size != 4U) ||
      options.maximum_closed_rank < options.support_size ||
      options.maximum_closed_rank >
          morsehgp3d::gpu::jung_chord_csr_tile_maximum_closed_rank ||
      options.chord_capacity == 0U ||
      (options.family != "uniform_latin" &&
       options.family != "eight_clusters")) {
    throw std::invalid_argument("inconsistent Jung-chord qualification options");
  }
  if (options.oracle &&
      (options.point_count > 32U ||
       options.morton_window + 1U < options.point_count)) {
    throw std::invalid_argument(
        "--oracle requires n<=32 and a Morton window covering every pair");
  }
  return options;
}

[[nodiscard]] CanonicalPointCloud make_cloud(const Options& options) {
  auto points = options.family == "uniform_latin"
      ? morsehgp3d::tools::pair_support_smoke::uniform_latin_points(
            options.point_count)
      : morsehgp3d::tools::pair_support_smoke::eight_clusters_points(
            options.point_count);
  if (options.oracle && options.support_size == 4U && points.size() >= 5U) {
    points[0U] = morsehgp3d::exact::CertifiedPoint3::from_binary64(
        10.0, 10.0, 0.0);
    points[1U] = morsehgp3d::exact::CertifiedPoint3::from_binary64(
        11.0, 11.0, 0.0);
    points[2U] = morsehgp3d::exact::CertifiedPoint3::from_binary64(
        11.0, 11.0, std::nextafter(1.0, 0.0));
    points[3U] = morsehgp3d::exact::CertifiedPoint3::from_binary64(
        11.0, 11.0, 1.0);
    points[4U] = morsehgp3d::exact::CertifiedPoint3::from_binary64(
        11.0, 11.0, std::nextafter(1.0, 2.0));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const morsehgp3d::exact::CertifiedPoint3>{points});
}

[[nodiscard]] std::vector<JungChordAnchor> morton_window_anchors(
    std::span<const morsehgp3d::spatial::MortonLeafRecord> leaves,
    std::size_t window) {
  if (leaves.empty()) {
    throw std::invalid_argument("cannot derive anchors from an empty index");
  }
  const std::size_t effective_window = std::min(window, leaves.size() - 1U);
  if (effective_window != 0U &&
      leaves.size() >
          std::numeric_limits<std::size_t>::max() / effective_window) {
    throw std::length_error("the Morton-window anchor reserve overflows");
  }
  std::vector<JungChordAnchor> anchors;
  anchors.reserve(leaves.size() * effective_window);
  for (std::size_t first = 0U; first < leaves.size(); ++first) {
    const std::size_t end =
        std::min(leaves.size(), first + effective_window + 1U);
    for (std::size_t second = first + 1U; second < end; ++second) {
      const PointId left = leaves[first].point_id;
      const PointId right = leaves[second].point_id;
      anchors.push_back(
          left < right ? JungChordAnchor{left, right}
                       : JungChordAnchor{right, left});
    }
  }
  std::sort(anchors.begin(), anchors.end());
  anchors.erase(std::unique(anchors.begin(), anchors.end()), anchors.end());
  return anchors;
}

[[nodiscard]] ExactAnchor exact_anchor(
    const CanonicalPointCloud& cloud,
    const JungChordAnchor& anchor,
    std::size_t support_size) {
  ExactAnchor result;
  ExactRational d[3U];
  ExactRational d2;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    d[axis] = cloud.point(anchor.support_v).coordinate(axis) -
        cloud.point(anchor.support_u).coordinate(axis);
    d2 = d2 + d[axis] * d[axis];
  }
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    const PointId point_id = static_cast<PointId>(index);
    if (point_id == anchor.support_u || point_id == anchor.support_v) {
      continue;
    }
    ExactRational a2;
    ExactRational dot;
    ExactRational xp2;
    ExactRational xq2;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const ExactRational x = cloud.point(point_id).coordinate(axis);
      const ExactRational p = cloud.point(anchor.support_u).coordinate(axis);
      const ExactRational q = cloud.point(anchor.support_v).coordinate(axis);
      const ExactRational a = x + x - p - q;
      a2 = a2 + a * a;
      dot = dot + d[axis] * a;
      xp2 = xp2 + (x - p) * (x - p);
      xq2 = xq2 + (x - q) * (x - q);
    }
    const ExactRational b = a2 - d2;
    const ExactRational gram = d2 * a2 - dot * dot;
    const ExactRational jung_q = support_size == 3U
        ? ExactRational{morsehgp3d::exact::BigInt{4}} * gram -
              ExactRational{morsehgp3d::exact::BigInt{3}} * b * b
        : ExactRational{morsehgp3d::exact::BigInt{2}} * gram - b * b;
    const bool lens_eligible = xp2 <= d2 && xq2 <= d2;
    if (lens_eligible) {
      result.lens_eligible.push_back(point_id);
    }
    if (jung_q.sign() < 0 && b.sign() < 0) {
      ++result.constant_interior;
    } else if (jung_q.sign() >= 0) {
      result.active.push_back(point_id);
      if (lens_eligible) {
        result.support_eligible.push_back(point_id);
      }
    }
  }
  return result;
}

[[nodiscard]] bool contains_sorted(
    const std::vector<PointId>& sorted,
    PointId value) {
  return std::binary_search(sorted.begin(), sorted.end(), value);
}

[[nodiscard]] OracleAudit run_oracle(
    const CanonicalPointCloud& cloud,
    const Options& options,
    const std::vector<JungChordAnchor>& expected_anchors,
    const Phase15JungChordCsrQualificationSnapshot& snapshot) {
  OracleAudit audit;
  audit.performed = true;
  audit.anchor_count = snapshot.anchors.size();
  const std::size_t expected_pair_count =
      cloud.size() * (cloud.size() - 1U) / 2U;
  audit.anchor_identity_violation_count =
      snapshot.anchors != expected_anchors ||
      expected_anchors.size() != expected_pair_count;
  audit.copied_chord_count = snapshot.chord_point_ids.size();
  audit.copied_byte_count =
      snapshot.anchors.size() * sizeof(JungChordAnchor) +
      snapshot.offsets.size() * sizeof(std::uint64_t) +
      snapshot.chord_point_ids.size() * sizeof(PointId) +
      snapshot.chord_flags.size() * sizeof(std::uint8_t) +
      snapshot.constant_interior_counts.size() * sizeof(std::uint64_t) +
      snapshot.anchor_flags.size() * sizeof(std::uint8_t);
  if (snapshot.offsets.size() != snapshot.anchors.size() + 1U ||
      snapshot.constant_interior_counts.size() != snapshot.anchors.size() ||
      snapshot.anchor_flags.size() != snapshot.anchors.size() ||
      snapshot.chord_flags.size() != snapshot.chord_point_ids.size()) {
    ++audit.csr_structure_violation_count;
    return audit;
  }
  audit.csr_structure_violation_count += snapshot.offsets.front() != 0U;
  audit.csr_structure_violation_count +=
      snapshot.offsets.back() != snapshot.chord_point_ids.size();
  for (std::size_t index = 1U; index < snapshot.offsets.size(); ++index) {
    audit.csr_structure_violation_count +=
        snapshot.offsets[index] < snapshot.offsets[index - 1U];
  }
  const std::uint8_t known_anchor_flags =
      morsehgp3d::gpu::detail::phase15_jung_chord_anchor_complete |
      morsehgp3d::gpu::detail::phase15_jung_chord_anchor_depth_rejected |
      morsehgp3d::gpu::detail::phase15_jung_chord_anchor_degenerate;
  const std::uint8_t known_chord_flags =
      morsehgp3d::gpu::detail::phase15_jung_chord_support_eligible |
      morsehgp3d::gpu::detail::
          phase15_jung_chord_support_eligible_ambiguous;
  const std::uint64_t maximum_constant =
      options.maximum_closed_rank - options.support_size;
  for (std::size_t anchor_index = 0U;
       anchor_index < snapshot.anchors.size();
       ++anchor_index) {
    const ExactAnchor exact =
        exact_anchor(cloud, snapshot.anchors[anchor_index], options.support_size);
    audit.unknown_flag_count +=
        (snapshot.anchor_flags[anchor_index] &
         static_cast<std::uint8_t>(~known_anchor_flags)) != 0U;
    audit.csr_structure_violation_count +=
        (snapshot.anchor_flags[anchor_index] &
         morsehgp3d::gpu::detail::phase15_jung_chord_anchor_complete) == 0U;
    const bool rejected =
        (snapshot.anchor_flags[anchor_index] &
         morsehgp3d::gpu::detail::
             phase15_jung_chord_anchor_depth_rejected) != 0U;
    if (rejected) {
      audit.false_depth_rejection_count +=
          exact.constant_interior <= maximum_constant;
      audit.csr_structure_violation_count +=
          snapshot.offsets[anchor_index] !=
          snapshot.offsets[anchor_index + 1U];
      continue;
    }
    audit.constant_lower_bound_violation_count +=
        snapshot.constant_interior_counts[anchor_index] >
        exact.constant_interior;
    const std::size_t begin =
        static_cast<std::size_t>(snapshot.offsets[anchor_index]);
    const std::size_t end =
        static_cast<std::size_t>(snapshot.offsets[anchor_index + 1U]);
    if (begin > end || end > snapshot.chord_point_ids.size()) {
      ++audit.active_omission_count;
      continue;
    }
    std::vector<PointId> actual(
        snapshot.chord_point_ids.begin() + static_cast<std::ptrdiff_t>(begin),
        snapshot.chord_point_ids.begin() + static_cast<std::ptrdiff_t>(end));
    std::sort(actual.begin(), actual.end());
    audit.duplicate_or_invalid_chord_id_count +=
        static_cast<std::size_t>(
            std::adjacent_find(actual.begin(), actual.end()) != actual.end());
    for (const PointId emitted : actual) {
      audit.duplicate_or_invalid_chord_id_count +=
          emitted >= cloud.size() || emitted == snapshot.anchors[anchor_index].support_u ||
          emitted == snapshot.anchors[anchor_index].support_v;
    }
    for (const PointId expected : exact.active) {
      audit.active_omission_count += !contains_sorted(actual, expected);
    }
    for (const PointId emitted : actual) {
      audit.fail_open_extra_count += !contains_sorted(exact.active, emitted);
    }
    for (std::size_t chord_index = begin; chord_index < end; ++chord_index) {
      const std::uint8_t flags = snapshot.chord_flags[chord_index];
      audit.unknown_flag_count +=
          (flags & static_cast<std::uint8_t>(~known_chord_flags)) != 0U;
      audit.csr_structure_violation_count +=
          (flags & morsehgp3d::gpu::detail::
                       phase15_jung_chord_support_eligible_ambiguous) != 0U &&
          (flags & morsehgp3d::gpu::detail::
                       phase15_jung_chord_support_eligible) == 0U;
      if (contains_sorted(
              exact.support_eligible,
              snapshot.chord_point_ids[chord_index])) {
        audit.support_eligible_false_negative_count +=
            (snapshot.chord_flags[chord_index] &
             morsehgp3d::gpu::detail::
                 phase15_jung_chord_support_eligible) == 0U;
      }
      const bool flagged_eligible =
          (flags & morsehgp3d::gpu::detail::
                       phase15_jung_chord_support_eligible) != 0U;
      const bool flagged_ambiguous =
          (flags & morsehgp3d::gpu::detail::
                       phase15_jung_chord_support_eligible_ambiguous) != 0U;
      audit.support_eligible_unjustified_nonambiguous_count +=
          flagged_eligible && !flagged_ambiguous &&
          !contains_sorted(
              exact.lens_eligible, snapshot.chord_point_ids[chord_index]);
    }
  }
  const ExactRational one = ExactRational::from_binary64(1.0);
  const auto q4_sign = [&](double z_value) {
    const ExactRational z = ExactRational::from_binary64(z_value);
    const ExactRational z2 = z * z;
    return (ExactRational{morsehgp3d::exact::BigInt{16}} * z2 *
            (one - z2)).sign();
  };
  audit.exact_q4_boundary_sign_fixture_passed =
      q4_sign(std::nextafter(1.0, 0.0)) > 0 && q4_sign(1.0) == 0 &&
      q4_sign(std::nextafter(1.0, 2.0)) < 0;
  if (options.support_size == 4U) {
    PointId fixture_ids[5U]{};
    bool found[5U]{};
    for (std::size_t index = 0U; index < cloud.size(); ++index) {
      const PointId point_id = static_cast<PointId>(index);
      const std::size_t source = cloud.source_index(point_id);
      if (source < 5U) {
        fixture_ids[source] = point_id;
        found[source] = true;
      }
    }
    const bool all_found =
        found[0U] && found[1U] && found[2U] && found[3U] && found[4U];
    if (all_found) {
      const JungChordAnchor fixture_anchor = fixture_ids[0U] < fixture_ids[1U]
          ? JungChordAnchor{fixture_ids[0U], fixture_ids[1U]}
          : JungChordAnchor{fixture_ids[1U], fixture_ids[0U]};
      const auto anchor_it = std::lower_bound(
          snapshot.anchors.begin(), snapshot.anchors.end(), fixture_anchor);
      if (anchor_it != snapshot.anchors.end() && *anchor_it == fixture_anchor) {
        const std::size_t anchor_index = static_cast<std::size_t>(
            std::distance(snapshot.anchors.begin(), anchor_it));
        const std::size_t begin =
            static_cast<std::size_t>(snapshot.offsets[anchor_index]);
        const std::size_t end =
            static_cast<std::size_t>(snapshot.offsets[anchor_index + 1U]);
        if (begin <= end && end <= snapshot.chord_point_ids.size()) {
          std::vector<PointId> actual(
              snapshot.chord_point_ids.begin() +
                  static_cast<std::ptrdiff_t>(begin),
              snapshot.chord_point_ids.begin() +
                  static_cast<std::ptrdiff_t>(end));
          std::sort(actual.begin(), actual.end());
          audit.cuda_q4_boundary_fixture_exercised =
              contains_sorted(actual, fixture_ids[2U]) &&
              contains_sorted(actual, fixture_ids[3U]);
        }
      }
    }
  }
  audit.passed = audit.false_depth_rejection_count == 0U &&
      audit.active_omission_count == 0U &&
      audit.constant_lower_bound_violation_count == 0U &&
      audit.support_eligible_false_negative_count == 0U &&
      audit.support_eligible_unjustified_nonambiguous_count == 0U &&
      audit.anchor_identity_violation_count == 0U &&
      audit.csr_structure_violation_count == 0U &&
      audit.duplicate_or_invalid_chord_id_count == 0U &&
      audit.unknown_flag_count == 0U &&
      (options.support_size != 4U ||
       audit.cuda_q4_boundary_fixture_exercised);
  return audit;
}

[[nodiscard]] std::string_view status_name(JungChordCsrTileStatus status) {
  switch (status) {
    case JungChordCsrTileStatus::tile_complete:
      return "tile_complete";
    case JungChordCsrTileStatus::capacity_exhausted:
      return "capacity_exhausted";
    case JungChordCsrTileStatus::malformed_input:
      return "malformed_input";
    case JungChordCsrTileStatus::execution_failure:
      return "execution_failure";
  }
  return "unknown";
}

[[nodiscard]] std::string_view stop_name(JungChordCsrTileStopReason stop) {
  switch (stop) {
    case JungChordCsrTileStopReason::none:
      return "none";
    case JungChordCsrTileStopReason::anchor_capacity:
      return "anchor_capacity";
    case JungChordCsrTileStopReason::chord_point_id_capacity:
      return "chord_point_id_capacity";
    case JungChordCsrTileStopReason::malformed_input:
      return "malformed_input";
    case JungChordCsrTileStopReason::traversal_failure:
      return "traversal_failure";
    case JungChordCsrTileStopReason::count_emit_mismatch:
      return "count_emit_mismatch";
  }
  return "unknown";
}

void print_json(
    const Options& options,
    const JungChordCsrTileAdvance& result,
    const OracleAudit& oracle,
    std::uint64_t build_ns,
    std::uint64_t anchor_ns,
    std::uint64_t classify_ns) {
  const auto& a = result.tile.has_value() ? result.tile->audit() : result.audit;
  std::cout << "{\n"
            << "  \"schema\": \"" << kSchema << "\",\n"
            << "  \"git_sha\": \"" << kGitSha << "\",\n"
            << "  \"phase\": \"P15-HOCUDA-P0\",\n"
            << "  \"backend\": \"cuda_g4_plus_reference_cpu_oracle\",\n"
            << "  \"execution_backend\": \"cuda_g4\",\n"
            << "  \"profile\": \"hgp_reduced\",\n"
            << "  \"mode\": \"proposal_only_shallow_higher_support_work_falsifier_v1\",\n"
            << "  \"deployment_status\": \"prototype_only\",\n"
            << "  \"public_status\": \"not_claimed\",\n"
            << "  \"native_q3_gate_closed\": false,\n"
            << "  \"native_q4_gate_closed\": false,\n"
            << "  \"family\": \"" << options.family << "\",\n"
            << "  \"point_count\": " << options.point_count << ",\n"
            << "  \"support_size\": " << options.support_size << ",\n"
            << "  \"maximum_closed_rank\": "
            << options.maximum_closed_rank << ",\n"
            << "  \"anchor_source\": \"morton_window_proposal_only\",\n"
            << "  \"morton_window\": " << options.morton_window << ",\n"
            << "  \"anchor_count_a\": " << a.anchor_count << ",\n"
            << "  \"unordered_pair_universe\": "
            << (options.point_count * (options.point_count - 1U) / 2U)
            << ",\n"
            << "  \"anchor_completeness_claimed\": false,\n"
            << "  \"count_node_visits_V\": "
            << a.count_pass_node_visit_count << ",\n"
            << "  \"emit_node_visits\": "
            << a.emit_pass_node_visit_count << ",\n"
            << "  \"count_leaf_tests\": "
            << a.count_pass_leaf_test_count << ",\n"
            << "  \"emit_leaf_tests\": "
            << a.emit_pass_leaf_test_count << ",\n"
            << "  \"range_candidate_mass_W\": "
            << a.range_candidate_point_count_sum << ",\n"
            << "  \"constant_interior_lower_bound_C\": "
            << a.constant_interior_lower_bound_sum << ",\n"
            << "  \"bulk_constant_interior_mass\": "
            << a.bulk_constant_interior_point_count << ",\n"
            << "  \"bulk_constant_exterior_mass\": "
            << a.bulk_constant_exterior_point_count << ",\n"
            << "  \"range_pruned_mass\": "
            << a.certified_outward_jung_range_pruned_point_count << ",\n"
            << "  \"resident_chord_count_M\": "
            << a.committed_chord_point_id_count << ",\n"
            << "  \"depth_rejected_anchors\": "
            << a.depth_rejected_anchor_count << ",\n"
            << "  \"census_censored_by_depth_cutoff\": "
            << (a.census_censored_by_depth_cutoff ? "true" : "false")
            << ",\n"
            << "  \"ambiguous_fail_open_chords\": "
            << a.ambiguous_fail_open_chord_count << ",\n"
            << "  \"certified_active_chords\": "
            << a.certified_active_chord_count << ",\n"
            << "  \"degenerate_anchors\": "
            << a.degenerate_anchor_count << ",\n"
            << "  \"support_eligible_chords\": "
            << a.support_eligible_chord_count << ",\n"
            << "  \"support_eligible_ambiguous_chords\": "
            << a.support_eligible_ambiguous_chord_count << ",\n"
            << "  \"range_candidate_p50\": "
            << a.range_candidate_count_p50 << ",\n"
            << "  \"range_candidate_p95\": "
            << a.range_candidate_count_p95 << ",\n"
            << "  \"range_candidate_p99\": "
            << a.range_candidate_count_p99 << ",\n"
            << "  \"range_candidate_max\": "
            << a.range_candidate_count_maximum << ",\n"
            << "  \"chord_p50\": " << a.chord_count_p50 << ",\n"
            << "  \"chord_p95\": " << a.chord_count_p95 << ",\n"
            << "  \"chord_p99\": " << a.chord_count_p99 << ",\n"
            << "  \"chord_max\": " << a.chord_count_maximum << ",\n"
            << "  \"support_eligible_p50\": "
            << a.support_eligible_count_p50 << ",\n"
            << "  \"support_eligible_p95\": "
            << a.support_eligible_count_p95 << ",\n"
            << "  \"support_eligible_p99\": "
            << a.support_eligible_count_p99 << ",\n"
            << "  \"support_eligible_max\": "
            << a.support_eligible_count_maximum << ",\n"
            << "  \"quadratic_chord_pair_baseline_count\": "
            << a.quadratic_chord_pair_baseline_count << ",\n"
            << "  \"quadratic_chord_pair_baseline_saturated\": "
            << (a.quadratic_chord_pair_baseline_saturated ? "true" : "false")
            << ",\n"
            << "  \"quadratic_chord_pair_baseline_executed\": false,\n"
            << "  \"installed_device_bytes\": "
            << a.physical_device_arena_capacity_bytes << ",\n"
            << "  \"required_device_bytes\": "
            << a.required_device_arena_capacity_bytes << ",\n"
            << "  \"retained_output_device_bytes\": "
            << a.retained_output_device_byte_count << ",\n"
            << "  \"logical_output_device_bytes\": "
            << a.logical_output_device_byte_count << ",\n"
            << "  \"configured_capacity_upper_bound_bytes\": "
            << a.configured_capacity_upper_bound_byte_count << ",\n"
            << "  \"cub_scan_bytes\": "
            << a.cub_scan_temporary_capacity_bytes << ",\n"
            << "  \"anchor_h2d_bytes\": "
            << a.anchor_host_to_device_byte_count << ",\n"
            << "  \"control_d2h_bytes\": "
            << a.control_device_to_host_byte_count << ",\n"
            << "  \"chord_d2h_bytes_product_path\": "
            << (options.oracle ? 0U : a.chord_device_to_host_byte_count)
            << ",\n"
            << "  \"kernels\": " << a.cuda_kernel_launch_count << ",\n"
            << "  \"library_submissions\": "
            << a.cuda_library_submission_count << ",\n"
            << "  \"synchronizations\": "
            << a.cuda_synchronization_count << ",\n"
            << "  \"build_ns\": " << build_ns << ",\n"
            << "  \"anchor_generation_ns\": " << anchor_ns << ",\n"
            << "  \"classify_wall_ns\": " << classify_ns << ",\n"
            << "  \"anchor_upload_ns\": "
            << a.anchor_upload_nanoseconds << ",\n"
            << "  \"count_kernel_ns\": " << a.count_kernel_nanoseconds
            << ",\n"
            << "  \"scan_ns\": " << a.scan_nanoseconds << ",\n"
            << "  \"capacity_preflight_ns\": "
            << a.capacity_preflight_nanoseconds << ",\n"
            << "  \"emit_kernel_ns\": " << a.emit_kernel_nanoseconds
            << ",\n"
            << "  \"total_device_ns\": "
            << a.total_device_nanoseconds << ",\n"
            << "  \"status\": \"" << status_name(result.status)
            << "\",\n"
            << "  \"stop_reason\": \"" << stop_name(result.stop_reason)
            << "\",\n"
            << "  \"oracle_performed\": "
            << (oracle.performed ? "true" : "false") << ",\n"
            << "  \"hostile_q4_points_injected\": "
            << (options.oracle && options.support_size == 4U
                    ? "true"
                    : "false")
            << ",\n"
            << "  \"oracle_passed\": "
            << (oracle.passed ? "true" : "false") << ",\n"
            << "  \"oracle_active_omissions\": "
            << oracle.active_omission_count << ",\n"
            << "  \"oracle_fail_open_extras\": "
            << oracle.fail_open_extra_count << ",\n"
            << "  \"oracle_false_depth_rejections\": "
            << oracle.false_depth_rejection_count << ",\n"
            << "  \"oracle_constant_lower_bound_violations\": "
            << oracle.constant_lower_bound_violation_count << ",\n"
            << "  \"oracle_support_eligible_false_negatives\": "
            << oracle.support_eligible_false_negative_count << ",\n"
            << "  \"oracle_support_eligible_unjustified_nonambiguous\": "
            << oracle.support_eligible_unjustified_nonambiguous_count
            << ",\n"
            << "  \"oracle_anchor_identity_violations\": "
            << oracle.anchor_identity_violation_count << ",\n"
            << "  \"oracle_csr_structure_violations\": "
            << oracle.csr_structure_violation_count << ",\n"
            << "  \"oracle_duplicate_or_invalid_chord_ids\": "
            << oracle.duplicate_or_invalid_chord_id_count << ",\n"
            << "  \"oracle_unknown_flags\": "
            << oracle.unknown_flag_count << ",\n"
            << "  \"exact_q4_boundary_sign_fixture_passed\": "
            << (oracle.exact_q4_boundary_sign_fixture_passed
                    ? "true"
                    : "false")
            << ",\n"
            << "  \"cuda_q4_boundary_fixture_exercised\": "
            << (oracle.cuda_q4_boundary_fixture_exercised
                    ? "true"
                    : "false")
            << ",\n"
            << "  \"qualification_snapshot_chords\": "
            << oracle.copied_chord_count << ",\n"
            << "  \"qualification_snapshot_d2h_bytes\": "
            << oracle.copied_byte_count << ",\n"
            << "  \"unbudgeted_profile\": true,\n"
            << "  \"output_capacity_bounded\": true,\n"
            << "  \"slo_eligible\": false,\n"
            << "  \"component_only\": true,\n"
            << "  \"per_anchor_diagnostics_collected\": "
            << (a.per_anchor_diagnostics_collected ? "true" : "false")
            << ",\n"
            << "  \"diagnostic_census_non_slo\": true,\n"
            << "  \"global_pair_matrix_materialized\": false,\n"
            << "  \"support_tuple_universe_materialized\": false,\n"
            << "  \"higher_order_delaunay_mosaic_materialized\": false,\n"
            << "  \"scientific_claim\": false\n"
            << "}\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options options = parse_options(argc, argv);
    const CanonicalPointCloud cloud = make_cloud(options);
    MortonLbvhBuildContext builder{cloud.size()};
    const auto build_begin = Clock::now();
    auto build = builder.build(cloud);
    const auto build_end = Clock::now();
    const auto anchor_begin = Clock::now();
    auto anchors = morton_window_anchors(
        build.certified_index().leaves(), options.morton_window);
    const auto anchor_end = Clock::now();
    if (anchors.empty() ||
        anchors.size() >
            morsehgp3d::gpu::jung_chord_csr_tile_maximum_anchor_capacity) {
      throw std::length_error(
          "the explicit Morton-window anchor tile exceeds the P0 capacity");
    }
    const std::vector<JungChordAnchor> expected_oracle_anchors =
        options.oracle ? anchors : std::vector<JungChordAnchor>{};
    auto traversal = builder.release_device_traversal_lease(build);
    JungChordCsrTileContext context{
        std::move(traversal),
        JungChordCsrTileConfig{
            options.support_size,
            options.maximum_closed_rank,
            anchors.size(),
            options.chord_capacity,
            true}};
    const auto classify_begin = Clock::now();
    auto result = context.classify(JungChordAnchorTile{
        "morton_window_proposal_only",
        options.morton_window,
        std::move(anchors)});
    const auto classify_end = Clock::now();
    OracleAudit oracle;
    if (options.oracle && result.tile.has_value()) {
      const auto snapshot =
          Phase15JungChordCsrTilePrivateViewAccess::qualification_snapshot(
              *result.tile);
      oracle = run_oracle(
          cloud, options, expected_oracle_anchors, snapshot);
    }
    print_json(
        options,
        result,
        oracle,
        ns_between(build_begin, build_end),
        ns_between(anchor_begin, anchor_end),
        ns_between(classify_begin, classify_end));
    const bool complete =
        result.status == JungChordCsrTileStatus::tile_complete &&
        result.tile.has_value();
    return complete && (!options.oracle || oracle.passed) ? 0 : 1;
  } catch (const std::exception& error) {
    std::cerr << "gpu_jung_chord_csr_tile_qualification: " << error.what()
              << '\n';
    return 2;
  }
}
