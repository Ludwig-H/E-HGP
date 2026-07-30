#include "../cuda/phase15_morton_yao48_ranked_pair_tile_classifier_internal.hpp"

#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/predicates.hpp"
#include "morsehgp3d/gpu/exact_closed_rank23_pair_terminal_catalog.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/hierarchy/exact_ranked_diametral_pair_catalog_reference.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cuda_runtime_api.h>

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::gpu::ExactClosedRank23PairTerminalCatalog;
using morsehgp3d::gpu::ExactClosedRank23PairTerminalCatalogConfig;
using morsehgp3d::gpu::ExactClosedRank23PairTerminalCatalogStatus;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceBuildResult;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::MortonYao48RankedPairTileCatalogLeaseAudit;
using morsehgp3d::gpu::build_exact_closed_rank23_pair_terminal_catalog;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48RankedPairTileCatalogPrivateViewAccess;
using morsehgp3d::hierarchy::ExactRankedDiametralPairCatalogReferenceResult;
using morsehgp3d::hierarchy::ExactRankedDiametralPairRecord;
using morsehgp3d::hierarchy::
    build_exact_ranked_diametral_pair_catalog_reference;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

inline constexpr std::string_view kSchema =
    "morsehgp3d.phase15.morton_yao48_ranked_pair_tile_classifier_"
    "qualification.v1";
inline constexpr std::string_view kBackend = "cuda_g4";
inline constexpr std::string_view kProfile = "hgp_reduced";
inline constexpr std::string_view kMode =
    "native_end_to_end_ranked_pair_catalog_qualification";
inline constexpr std::string_view kDeploymentStatus = "component_only";
inline constexpr std::string_view kPublicStatus = "not_claimed";
inline constexpr std::size_t kMaximumPointCount = 512U;
inline constexpr std::size_t kMaximumClosedRank = 3U;
inline constexpr std::int64_t kCoordinateDenominator = INT64_C(1) << 20U;
inline constexpr std::int64_t kMaximumAbsoluteScaledCoordinate =
    INT64_C(64) * kCoordinateDenominator;
inline constexpr std::uint64_t kFnvOffsetBasis =
    UINT64_C(14695981039346656037);
inline constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);

#if defined(MORSEHGP3D_GIT_SHA)
inline constexpr std::string_view kGitSha = MORSEHGP3D_GIT_SHA;
#else
inline constexpr std::string_view kGitSha = "unavailable";
#endif

struct Options {
  std::size_t point_count{257U};
  std::size_t anchor_tile_capacity{17U};
  bool single_run{false};
};

struct FixtureFeatures {
  std::size_t strict_collinear_point_count{};
  std::size_t exact_shell_point_count{};
  std::size_t signed_permutation_point_count{};
  std::size_t jitter_grid_point_count{};
  std::size_t clustered_point_count{};
  std::size_t deterministic_fill_point_count{};
};

struct GeneratedFixture {
  std::vector<CertifiedPoint3> points;
  FixtureFeatures features{};
};

struct ReferenceBuckets {
  ExactRankedDiametralPairCatalogReferenceResult rank_two;
  ExactRankedDiametralPairCatalogReferenceResult rank_three;
};

struct ResidentCatalogCopy {
  std::vector<std::uint64_t> source_coordinate_bits;
  std::vector<std::uint64_t> support_u;
  std::vector<std::uint64_t> support_v;
  std::vector<std::uint8_t> closed_rank;
  std::vector<std::uint64_t> rank_offsets;
  std::vector<std::uint64_t> strict_offsets;
  std::vector<std::uint64_t> strict_point_ids;
  std::vector<std::uint64_t> shell_offsets;
  std::vector<std::uint64_t> shell_point_ids;
  std::vector<ExactLevel> recomputed_squared_levels;
  std::uint64_t digest{kFnvOffsetBasis};
};

struct RunResult {
  std::size_t anchor_tile_capacity{};
  std::size_t commit_count{};
  ReferenceBuckets reference;
  MortonYao48RankedPairTileCatalogLeaseAudit audit{};
  ResidentCatalogCopy catalog;
};

[[noreturn]] void fail(const std::string& message) {
  throw std::runtime_error(message);
}

void require(bool condition, const std::string& message) {
  if (!condition) {
    fail(message);
  }
}

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_linear_factor(
    std::size_t required_capacity,
    std::size_t point_count,
    const char* message) {
  require(point_count != 0U, "a linear capacity requires a nonempty source");
  const std::size_t factor = required_capacity == 0U
      ? 0U
      : 1U + (required_capacity - 1U) / point_count;
  require(
      factor <= morsehgp3d::gpu::
          exact_closed_rank23_pair_terminal_catalog_maximum_linear_capacity_factor,
      message);
  return factor;
}

[[nodiscard]] std::uint64_t unordered_pair_count(std::size_t point_count) {
  return static_cast<std::uint64_t>(point_count) *
      static_cast<std::uint64_t>(point_count - 1U) / UINT64_C(2);
}

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    const char* message) {
  std::size_t value = 0U;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto parsed = std::from_chars(begin, end, value);
  if (parsed.ec != std::errc{} || parsed.ptr != end) {
    throw std::invalid_argument(message);
  }
  return value;
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--point-count" && index + 1 < argc) {
      options.point_count =
          parse_size(argv[++index], "invalid --point-count");
    } else if (
        argument == "--anchor-tile-capacity" && index + 1 < argc) {
      options.anchor_tile_capacity =
          parse_size(argv[++index], "invalid --anchor-tile-capacity");
    } else if (argument == "--single-run") {
      options.single_run = true;
    } else {
      throw std::invalid_argument(
          "usage: gpu_morton_yao48_ranked_pair_tile_classifier_"
          "qualification [--point-count N] [--anchor-tile-capacity N] "
          "[--single-run]");
    }
  }
  if (options.point_count < 3U || options.point_count > kMaximumPointCount) {
    throw std::out_of_range("--point-count must lie in [3,512]");
  }
  if (options.anchor_tile_capacity == 0U ||
      options.anchor_tile_capacity >
          morsehgp3d::gpu::
              morton_yao48_device_tiled_pair_frontier_maximum_anchor_tile_capacity) {
    throw std::out_of_range(
        "--anchor-tile-capacity must lie in [1,4096]");
  }
  return options;
}

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t value) noexcept {
  value += UINT64_C(0x9e3779b97f4a7c15);
  value = (value ^ (value >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
  value = (value ^ (value >> 27U)) * UINT64_C(0x94d049bb133111eb);
  return value ^ (value >> 31U);
}

void append_scaled_point(
    GeneratedFixture& fixture,
    std::set<std::array<std::int64_t, 3U>>& used,
    const std::array<std::int64_t, 3U>& coordinate,
    std::size_t& feature_count,
    std::size_t point_count) {
  if (fixture.points.size() >= point_count || !used.insert(coordinate).second) {
    return;
  }
  for (const std::int64_t value : coordinate) {
    if (value < -kMaximumAbsoluteScaledCoordinate ||
        value > kMaximumAbsoluteScaledCoordinate) {
      throw std::overflow_error(
          "a fixture coordinate escaped its certified dyadic envelope");
    }
  }
  const auto decode = [](std::int64_t value) {
    return std::ldexp(static_cast<double>(value), -20);
  };
  const double x = decode(coordinate[0U]);
  const double y = decode(coordinate[1U]);
  const double z = decode(coordinate[2U]);
  require(
      std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
          std::ldexp(x, 20) == static_cast<double>(coordinate[0U]) &&
          std::ldexp(y, 20) == static_cast<double>(coordinate[1U]) &&
          std::ldexp(z, 20) == static_cast<double>(coordinate[2U]),
      "a fixture coordinate lost its exact binary64 representation");
  fixture.points.push_back(CertifiedPoint3::from_binary64(x, y, z));
  ++feature_count;
}

void append_local_rank_fixtures(
    GeneratedFixture& fixture,
    std::set<std::array<std::int64_t, 3U>>& used,
    std::size_t point_count) {
  constexpr std::int64_t unit = kCoordinateDenominator;
  for (const auto& coordinate :
       std::array<std::array<std::int64_t, 3U>, 3U>{
           std::array<std::int64_t, 3U>{-50 * unit, -50 * unit, -50 * unit},
           std::array<std::int64_t, 3U>{-48 * unit, -50 * unit, -50 * unit},
           std::array<std::int64_t, 3U>{-49 * unit, -50 * unit, -50 * unit}}) {
    append_scaled_point(
        fixture,
        used,
        coordinate,
        fixture.features.strict_collinear_point_count,
        point_count);
  }
  for (const auto& coordinate :
       std::array<std::array<std::int64_t, 3U>, 3U>{
           std::array<std::int64_t, 3U>{-50 * unit, -45 * unit, -50 * unit},
           std::array<std::int64_t, 3U>{-48 * unit, -45 * unit, -50 * unit},
           std::array<std::int64_t, 3U>{-49 * unit, -44 * unit, -50 * unit}}) {
    append_scaled_point(
        fixture,
        used,
        coordinate,
        fixture.features.exact_shell_point_count,
        point_count);
  }
}

void append_shell_permutations(
    GeneratedFixture& fixture,
    std::set<std::array<std::int64_t, 3U>>& used,
    std::size_t point_count) {
  constexpr std::int64_t unit = kCoordinateDenominator;
  constexpr std::array<std::array<std::uint8_t, 3U>, 6U> permutations{{
      {{0U, 1U, 2U}},
      {{0U, 2U, 1U}},
      {{1U, 0U, 2U}},
      {{1U, 2U, 0U}},
      {{2U, 0U, 1U}},
      {{2U, 1U, 0U}},
  }};
  for (const auto& permutation : permutations) {
    for (std::uint8_t sign_mask = 0U; sign_mask < 8U; ++sign_mask) {
      constexpr std::array<std::int64_t, 3U> source{
          0, 3 * unit, 4 * unit};
      std::array<std::int64_t, 3U> coordinate{};
      for (std::size_t position = 0U; position < 3U; ++position) {
        const std::size_t axis = permutation[position];
        const std::int64_t sign =
            (sign_mask & static_cast<std::uint8_t>(1U << axis)) == 0U
                ? 1
                : -1;
        coordinate[axis] = sign * source[position];
      }
      append_scaled_point(
          fixture,
          used,
          coordinate,
          fixture.features.signed_permutation_point_count,
          point_count);
    }
  }
}

void append_jitter_grid(
    GeneratedFixture& fixture,
    std::set<std::array<std::int64_t, 3U>>& used,
    std::size_t point_count) {
  constexpr std::int64_t unit = kCoordinateDenominator;
  constexpr std::uint64_t seed = UINT64_C(0x15a048d1e7c93b25);
  std::size_t logical_index = 0U;
  for (std::int64_t x_index = -2; x_index <= 2; ++x_index) {
    for (std::int64_t y_index = -2; y_index <= 2; ++y_index) {
      for (std::int64_t z_index = -2; z_index <= 2; ++z_index) {
        const std::uint64_t random =
            splitmix64(seed ^ static_cast<std::uint64_t>(logical_index));
        const std::int64_t jitter_x =
            static_cast<std::int64_t>(random & UINT64_C(0x1ff)) - 256;
        const std::int64_t jitter_y =
            static_cast<std::int64_t>((random >> 9U) & UINT64_C(0x1ff)) - 256;
        const std::int64_t jitter_z =
            static_cast<std::int64_t>((random >> 18U) & UINT64_C(0x1ff)) -
            256;
        append_scaled_point(
            fixture,
            used,
            {12 * unit + x_index * (unit / 2) + jitter_x,
             -9 * unit + y_index * (unit / 2) + jitter_y,
             7 * unit + z_index * (unit / 2) + jitter_z},
            fixture.features.jitter_grid_point_count,
            point_count);
        ++logical_index;
      }
    }
  }
}

void append_clusters(
    GeneratedFixture& fixture,
    std::set<std::array<std::int64_t, 3U>>& used,
    std::size_t point_count) {
  constexpr std::int64_t unit = kCoordinateDenominator;
  constexpr std::uint64_t seed = UINT64_C(0x71a35c09d42ef68b);
  constexpr std::array<std::array<std::int64_t, 3U>, 4U> centers{{
      {{-24 * unit, -18 * unit, 20 * unit}},
      {{22 * unit, -21 * unit, -17 * unit}},
      {{-19 * unit, 23 * unit, -22 * unit}},
      {{25 * unit, 19 * unit, 18 * unit}},
  }};
  for (std::size_t cluster = 0U; cluster < centers.size(); ++cluster) {
    for (std::size_t member = 0U; member < 32U; ++member) {
      const std::uint64_t random = splitmix64(
          seed ^ (static_cast<std::uint64_t>(cluster) << 32U) ^
          static_cast<std::uint64_t>(member));
      const auto perturb = [random](unsigned int shift) {
        return static_cast<std::int64_t>(
                   (random >> shift) & UINT64_C(0xffff)) -
            INT64_C(32768);
      };
      append_scaled_point(
          fixture,
          used,
          {centers[cluster][0U] + perturb(0U),
           centers[cluster][1U] + perturb(16U),
           centers[cluster][2U] + perturb(32U)},
          fixture.features.clustered_point_count,
          point_count);
    }
  }
}

void append_deterministic_fill(
    GeneratedFixture& fixture,
    std::set<std::array<std::int64_t, 3U>>& used,
    std::size_t point_count) {
  constexpr std::int64_t unit = kCoordinateDenominator;
  constexpr std::uint64_t seed = UINT64_C(0xd08f63a419be725c);
  std::uint64_t logical_index = 0U;
  while (fixture.points.size() < point_count) {
    const std::uint64_t random_y = splitmix64(seed ^ logical_index);
    const std::uint64_t random_z = splitmix64(
        std::rotl(seed, 31) ^ logical_index);
    append_scaled_point(
        fixture,
        used,
        {36 * unit +
             static_cast<std::int64_t>(logical_index * UINT64_C(257)),
         static_cast<std::int64_t>(
             random_y % static_cast<std::uint64_t>(24 * unit)) -
             12 * unit,
         static_cast<std::int64_t>(
             random_z % static_cast<std::uint64_t>(24 * unit)) -
             12 * unit},
        fixture.features.deterministic_fill_point_count,
        point_count);
    ++logical_index;
  }
}

[[nodiscard]] GeneratedFixture generate_fixture(std::size_t point_count) {
  GeneratedFixture fixture;
  fixture.points.reserve(point_count);
  std::set<std::array<std::int64_t, 3U>> used;
  append_local_rank_fixtures(fixture, used, point_count);
  append_shell_permutations(fixture, used, point_count);
  append_jitter_grid(fixture, used, point_count);
  append_clusters(fixture, used, point_count);
  append_deterministic_fill(fixture, used, point_count);
  require(
      fixture.points.size() == point_count,
      "the deterministic fixture has the wrong cardinality");
  require(
      fixture.features.strict_collinear_point_count >= 3U,
      "the deterministic fixture omitted the strict rank-three seed");
  return fixture;
}

void check_cuda(cudaError_t status, const char* operation) {
  if (status != cudaSuccess) {
    throw std::runtime_error(
        std::string{operation} + ": " + cudaGetErrorString(status));
  }
}

template <typename T>
[[nodiscard]] std::vector<T> copy_from_device(
    const T* source,
    std::size_t count,
    const char* operation) {
  if (count == 0U) {
    return {};
  }
  require(source != nullptr, std::string{operation} + " has a null source");
  std::vector<T> result(count);
  check_cuda(
      cudaMemcpy(
          result.data(), source, count * sizeof(T), cudaMemcpyDeviceToHost),
      operation);
  return result;
}

[[nodiscard]] std::size_t payload_count(
    const ExactRankedDiametralPairCatalogReferenceResult& bucket,
    bool strict) {
  std::size_t count = 0U;
  for (const ExactRankedDiametralPairRecord& record : bucket.records) {
    count = checked_add(
        count,
        strict ? record.strict_interior_ids.size()
               : record.extra_shell_ids.size(),
        "the bounded reference payload count overflowed");
  }
  return count;
}

[[nodiscard]] std::vector<const ExactRankedDiametralPairRecord*>
ordered_expected_records(const ReferenceBuckets& reference) {
  std::vector<const ExactRankedDiametralPairRecord*> records;
  records.reserve(
      reference.rank_two.records.size() +
      reference.rank_three.records.size());
  for (const ExactRankedDiametralPairRecord& record :
       reference.rank_two.records) {
    records.push_back(&record);
  }
  for (const ExactRankedDiametralPairRecord& record :
       reference.rank_three.records) {
    records.push_back(&record);
  }
  return records;
}

[[nodiscard]] ExactLevel diametral_level_from_resident_bits(
    const ResidentCatalogCopy& catalog,
    std::size_t point_count,
    PointId first,
    PointId second) {
  require(
      first < point_count && second < point_count,
      "a resident support PointId is outside the source cloud");
  const auto point = [&catalog, point_count](PointId point_id) {
    std::array<std::uint64_t, 3U> bits{};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      bits[axis] = catalog.source_coordinate_bits[
          axis * point_count + static_cast<std::size_t>(point_id)];
    }
    return CertifiedPoint3::from_binary64_bits(bits);
  };
  const auto squared_distance =
      morsehgp3d::exact::squared_distance(point(first), point(second));
  return ExactLevel{
      squared_distance.numerator(),
      squared_distance.denominator() * BigInt{4}};
}

void hash_byte(std::uint64_t& digest, std::uint8_t value) noexcept {
  digest ^= value;
  digest *= kFnvPrime;
}

void hash_word(std::uint64_t& digest, std::uint64_t value) noexcept {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    hash_byte(
        digest,
        static_cast<std::uint8_t>((value >> shift) & UINT64_C(0xff)));
  }
}

void hash_string(std::uint64_t& digest, std::string_view value) noexcept {
  for (const char character : value) {
    hash_byte(digest, static_cast<std::uint8_t>(character));
  }
  hash_byte(digest, UINT8_C(0xff));
}

[[nodiscard]] std::uint64_t catalog_digest(
    const ResidentCatalogCopy& catalog) {
  std::uint64_t digest = kFnvOffsetBasis;
  const auto hash_words = [&digest](const std::vector<std::uint64_t>& words) {
    hash_word(digest, static_cast<std::uint64_t>(words.size()));
    for (const std::uint64_t word : words) {
      hash_word(digest, word);
    }
  };
  hash_words(catalog.support_u);
  hash_words(catalog.support_v);
  hash_word(digest, static_cast<std::uint64_t>(catalog.closed_rank.size()));
  for (const std::uint8_t rank : catalog.closed_rank) {
    hash_byte(digest, rank);
  }
  hash_words(catalog.rank_offsets);
  hash_words(catalog.strict_offsets);
  hash_words(catalog.strict_point_ids);
  hash_words(catalog.shell_offsets);
  hash_words(catalog.shell_point_ids);
  hash_word(
      digest,
      static_cast<std::uint64_t>(catalog.recomputed_squared_levels.size()));
  for (const ExactLevel& level : catalog.recomputed_squared_levels) {
    hash_string(digest, level.numerator_string());
    hash_string(digest, level.denominator_string());
  }
  return digest;
}

void validate_reference(const ReferenceBuckets& reference) {
  require(
      reference.rank_two.requested_order == 1U &&
          reference.rank_two.target_closed_rank == 2U &&
          reference.rank_two.locally_structurally_valid(),
      "the exact CPU rank-two reference did not close");
  require(
      reference.rank_three.requested_order == 2U &&
          reference.rank_three.target_closed_rank == 3U &&
          reference.rank_three.locally_structurally_valid(),
      "the exact CPU rank-three reference did not close");
}

[[nodiscard]] std::vector<ExactRankedDiametralPairRecord>
exhaustive_all_pair_bucket(
    const CanonicalPointCloud& cloud,
    std::size_t target_closed_rank) {
  std::vector<ExactRankedDiametralPairRecord> records;
  for (std::size_t first_index = 0U;
       first_index < cloud.size();
       ++first_index) {
    const PointId first = static_cast<PointId>(first_index);
    for (std::size_t second_index = first_index + 1U;
         second_index < cloud.size();
         ++second_index) {
      const PointId second = static_cast<PointId>(second_index);
      ExactRankedDiametralPairRecord record;
      record.support_ids = {first, second};
      const ExactRational pair_squared_distance =
          morsehgp3d::exact::squared_distance(
              cloud.point(first), cloud.point(second));
      record.squared_level = ExactLevel{ExactRational{
          pair_squared_distance.numerator(),
          pair_squared_distance.denominator() * BigInt{4}}};
      for (std::size_t witness_index = 0U;
           witness_index < cloud.size();
           ++witness_index) {
        const PointId witness = static_cast<PointId>(witness_index);
        ExactRational phi;
        for (std::size_t axis = 0U; axis < 3U; ++axis) {
          phi = phi +
              (cloud.point(witness).coordinate(axis) -
               cloud.point(first).coordinate(axis)) *
                  (cloud.point(witness).coordinate(axis) -
                   cloud.point(second).coordinate(axis));
        }
        const int sign = phi.sign();
        if (sign < 0) {
          record.strict_interior_ids.push_back(witness);
        } else if (sign == 0 && witness != first && witness != second) {
          record.extra_shell_ids.push_back(witness);
        }
      }
      record.closed_rank = checked_add(
          checked_add(
              2U,
              record.strict_interior_ids.size(),
              "the exhaustive strict closed rank overflowed"),
          record.extra_shell_ids.size(),
          "the exhaustive shell closed rank overflowed");
      require(
          record.closed_rank <= cloud.size(),
          "the exhaustive closed rank exceeds the source cloud");
      record.exterior_count = cloud.size() - record.closed_rank;
      if (record.closed_rank == target_closed_rank) {
        records.push_back(std::move(record));
      }
    }
  }
  return records;
}

void validate_reference_against_all_pairs(
    const CanonicalPointCloud& cloud,
    const ReferenceBuckets& reference) {
  const std::vector<ExactRankedDiametralPairRecord> exhaustive_rank_two =
      exhaustive_all_pair_bucket(cloud, 2U);
  const std::vector<ExactRankedDiametralPairRecord> exhaustive_rank_three =
      exhaustive_all_pair_bucket(cloud, 3U);
  require(
      reference.rank_two.records == exhaustive_rank_two &&
          reference.rank_three.records == exhaustive_rank_three,
      "the bounded Yao48 CPU reference differs from the independent "
      "all-pair/all-witness oracle");
}

void validate_catalog_against_reference(
    const CanonicalPointCloud& cloud,
    const ReferenceBuckets& reference,
    ResidentCatalogCopy& catalog) {
  const std::vector<const ExactRankedDiametralPairRecord*> expected =
      ordered_expected_records(reference);
  require(
      catalog.support_u.size() == expected.size() &&
          catalog.support_v.size() == expected.size() &&
          catalog.closed_rank.size() == expected.size(),
      "the resident catalogue record extent differs from the exact buckets");

  const std::vector<std::uint64_t> expected_rank_offsets{
      0U,
      0U,
      0U,
      static_cast<std::uint64_t>(reference.rank_two.records.size()),
      static_cast<std::uint64_t>(expected.size())};
  require(
      catalog.rank_offsets == expected_rank_offsets,
      "the resident catalogue rank offsets differ from ranks two and three");
  require(
      catalog.strict_offsets.size() == expected.size() + 1U &&
          catalog.shell_offsets.size() == expected.size() + 1U &&
          catalog.strict_offsets.front() == 0U &&
          catalog.shell_offsets.front() == 0U &&
          catalog.strict_offsets.back() == catalog.strict_point_ids.size() &&
          catalog.shell_offsets.back() == catalog.shell_point_ids.size(),
      "the resident catalogue payload offsets are not closed");

  require(
      catalog.source_coordinate_bits.size() == 3U * cloud.size(),
      "the retained source coordinate extent is not three words per point");
  for (std::size_t point_index = 0U; point_index < cloud.size(); ++point_index) {
    const auto expected_bits =
        cloud.point(static_cast<PointId>(point_index)).canonical_input_bits();
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      require(
          catalog.source_coordinate_bits[axis * cloud.size() + point_index] ==
              expected_bits[axis],
          "the retained source coordinate authority differs from the "
          "canonical cloud");
    }
  }

  catalog.recomputed_squared_levels.clear();
  catalog.recomputed_squared_levels.reserve(expected.size());
  for (std::size_t index = 0U; index < expected.size(); ++index) {
    const ExactRankedDiametralPairRecord& record = *expected[index];
    require(
        catalog.support_u[index] == record.support_ids[0U] &&
            catalog.support_v[index] == record.support_ids[1U] &&
            catalog.closed_rank[index] == record.closed_rank,
        "a resident support or closed rank differs from the exact reference");
    const std::uint64_t strict_begin = catalog.strict_offsets[index];
    const std::uint64_t strict_end = catalog.strict_offsets[index + 1U];
    const std::uint64_t shell_begin = catalog.shell_offsets[index];
    const std::uint64_t shell_end = catalog.shell_offsets[index + 1U];
    require(
        strict_begin <= strict_end &&
            strict_end <= catalog.strict_point_ids.size() &&
            shell_begin <= shell_end &&
            shell_end <= catalog.shell_point_ids.size(),
        "a resident payload segment is outside its final arena");
    require(
        std::equal(
            catalog.strict_point_ids.begin() +
                static_cast<std::ptrdiff_t>(strict_begin),
            catalog.strict_point_ids.begin() +
                static_cast<std::ptrdiff_t>(strict_end),
            record.strict_interior_ids.begin(),
            record.strict_interior_ids.end()) &&
            std::equal(
                catalog.shell_point_ids.begin() +
                    static_cast<std::ptrdiff_t>(shell_begin),
                catalog.shell_point_ids.begin() +
                    static_cast<std::ptrdiff_t>(shell_end),
                record.extra_shell_ids.begin(),
                record.extra_shell_ids.end()),
        "a resident strict or shell payload differs from the exact reference");
    ExactLevel recomputed = diametral_level_from_resident_bits(
        catalog,
        cloud.size(),
        record.support_ids[0U],
        record.support_ids[1U]);
    require(
        recomputed == record.squared_level,
        "a support level recomputed as squared distance divided by four "
        "differs from the exact CPU level");
    catalog.recomputed_squared_levels.push_back(std::move(recomputed));
  }
  catalog.digest = catalog_digest(catalog);
}

[[nodiscard]] RunResult run_once(
    const CanonicalPointCloud& cloud,
    std::size_t anchor_tile_capacity) {
  RunResult result;
  result.anchor_tile_capacity = anchor_tile_capacity;
  std::optional<ExactClosedRank23PairTerminalCatalog> terminal_catalog;
  {
    MortonLbvhBuildContext builder{cloud.size()};
    MortonLbvhDeviceBuildResult build = builder.build(cloud);
    require(
        build.complete_certified_build() && build.cuda_qualified_build() &&
            build.certified_index().validated_for(cloud) &&
            build.audit().point_count == cloud.size() &&
            build.audit().required_node_count == 2U * cloud.size() - 1U &&
            !build.audit().higher_order_delaunay_mosaic_materialized &&
            !build.audit().global_cell_or_coface_arena_materialized &&
            !build.audit().public_status_claimed,
        "the public LBVH stage did not return a certified CUDA build");

    result.reference.rank_two =
        build_exact_ranked_diametral_pair_catalog_reference(
            build.certified_index(), cloud, 1U);
    result.reference.rank_three =
        build_exact_ranked_diametral_pair_catalog_reference(
            build.certified_index(), cloud, 2U);
    validate_reference(result.reference);
    validate_reference_against_all_pairs(cloud, result.reference);

    const std::size_t record_capacity = checked_add(
        result.reference.rank_two.records.size(),
        result.reference.rank_three.records.size(),
        "the bounded reference record capacity overflowed");
    const std::size_t strict_capacity = checked_add(
        payload_count(result.reference.rank_two, true),
        payload_count(result.reference.rank_three, true),
        "the bounded strict payload capacity overflowed");
    const std::size_t shell_capacity = checked_add(
        payload_count(result.reference.rank_two, false),
        payload_count(result.reference.rank_three, false),
        "the bounded shell payload capacity overflowed");
    const std::size_t payload_capacity = checked_add(
        strict_capacity,
        shell_capacity,
        "the bounded total payload capacity overflowed");

    MortonLbvhDeviceTraversalLease traversal =
        builder.release_device_traversal_lease(build);
    require(
        traversal.ready() && traversal.cuda_resident() &&
            traversal.audit().point_count == cloud.size() &&
            traversal.audit().canonical_coordinate_words_retained &&
            traversal.audit().active_morton_point_ids_retained &&
            traversal.audit().certified_device_nodes_retained &&
            traversal.audit().cuda_device_storage_retained &&
            !traversal.audit().host_fake_lifecycle_exercised &&
            !traversal.audit().higher_order_delaunay_mosaic_materialized &&
            !traversal.audit().global_cell_or_coface_arena_materialized &&
            !traversal.audit().public_status_claimed,
        "the public LBVH traversal lease is not a resident CUDA authority");

    auto terminal = build_exact_closed_rank23_pair_terminal_catalog(
        std::move(traversal),
        ExactClosedRank23PairTerminalCatalogConfig{
            anchor_tile_capacity,
            checked_linear_factor(
                record_capacity,
                cloud.size(),
                "the bounded record oracle exceeds the linear gate"),
            checked_linear_factor(
                payload_capacity,
                cloud.size(),
                "the bounded payload oracle exceeds the linear gate"),
            0U});
    require(
        terminal.status == ExactClosedRank23PairTerminalCatalogStatus::
                               complete_exact_closed_rank23 &&
            terminal.complete() && terminal.catalog.has_value() &&
            terminal.catalog->cuda_resident() &&
            !terminal.catalog->host_fake() &&
            terminal.audit.maximum_closed_rank == kMaximumClosedRank &&
            terminal.audit.fixed_rank23_window_enforced &&
            terminal.audit.fixed_linear_capacities_validated &&
            terminal.audit.frontier_closed &&
            terminal.audit.frontier_mass_reconciled &&
            terminal.audit.classifier_mass_reconciled &&
            terminal.audit.zero_unresolved_certified &&
            terminal.audit.zero_exact_fallback_certified &&
            terminal.audit.closed_rank23_pair_catalog_complete &&
            !terminal.audit.output_withheld &&
            terminal.audit.cuda_execution_performed &&
            !terminal.audit.host_fake_lifecycle_exercised &&
            !terminal.audit.candidate_device_to_host_performed &&
            !terminal.audit.certified_prune_device_to_host_performed &&
            !terminal.audit.callback_per_pair_performed &&
            !terminal.audit.dense_pair_scan_performed &&
            !terminal.audit.global_pair_matrix_materialized &&
            !terminal.audit.ordinary_delaunay_materialized &&
            !terminal.audit.higher_order_delaunay_mosaic_materialized &&
            !terminal.audit.gamma2_complete_claimed &&
            !terminal.audit.k2_complete_claimed &&
            !terminal.audit.hierarchy_complete_claimed &&
            !terminal.audit.latency_100ms_claimed &&
            !terminal.audit.public_status_claimed,
        "the public terminal wrapper withheld or overclaimed its result");
    result.commit_count = terminal.audit.classifier_commit_count;
    terminal_catalog.emplace(std::move(*terminal.catalog));
  }

  require(
      terminal_catalog.has_value() && terminal_catalog->cuda_resident(),
      "the terminal catalogue did not retain its source after builder "
      "destruction");
  const auto& lease = terminal_catalog->resident_catalog();
  result.audit = lease.audit();
  const auto views =
      Phase15MortonYao48RankedPairTileCatalogPrivateViewAccess::inspect(
          lease);
  const std::size_t expected_record_count = checked_add(
      result.reference.rank_two.records.size(),
      result.reference.rank_three.records.size(),
      "the expected final record count overflowed");
  require(
      lease.ready() && lease.cuda_resident() && !lease.host_fake() &&
          views.ready && !views.host_fake && views.output_owner &&
          views.source_owner_authority &&
          views.source_cloud_identity_authority &&
          views.device_coordinate_bits != nullptr && views.cuda_device >= 0 &&
          result.audit.point_count == cloud.size() &&
          result.audit.maximum_closed_rank == kMaximumClosedRank &&
          result.audit.catalog_record_count == expected_record_count &&
          result.audit.rank_offset_count == kMaximumClosedRank + 2U &&
          result.audit.record_offset_count == expected_record_count + 1U &&
          result.audit.unresolved_count == 0U &&
          result.audit.exact_fallback_count == 0U &&
          result.audit.frontier_unresolved_pair_mass == 0U &&
          result.audit.candidate_count ==
              expected_record_count + result.audit.above_window_count &&
          result.audit.candidate_count +
                  result.audit.certified_pruned_pair_mass ==
              result.audit.unordered_pair_universe_count &&
          result.audit.unordered_pair_universe_count ==
              unordered_pair_count(cloud.size()) &&
          result.audit.frontier_closed &&
          result.audit.closed_rank_catalog_complete &&
          result.audit.source_identity_authenticated &&
          result.audit.source_owner_retained &&
          result.audit.source_cloud_identity_retained &&
          result.audit.source_device_coordinates_retained &&
          result.audit.monotone_chunk_journal_validated &&
          result.audit.exact_multiorder_classification_validated &&
          result.audit.count_scan_payload_layout_validated &&
          result.audit.candidate_mass_validated &&
          result.audit.frontier_global_mass_validated &&
          !result.audit.host_fake_lifecycle_exercised &&
          result.audit.cuda_device_storage_retained &&
          !result.audit.intermediate_candidate_device_to_host_performed &&
          !result.audit.certified_prune_device_to_host_performed &&
          !result.audit.callback_per_pair_performed &&
          !result.audit.dense_pair_scan_performed &&
          !result.audit.global_pair_matrix_materialized &&
          !result.audit.ordinary_delaunay_materialized &&
          !result.audit.higher_order_delaunay_mosaic_materialized &&
          !result.audit.public_status_claimed,
      "the final resident catalogue lease did not close its scoped audit");

  check_cuda(cudaSetDevice(views.cuda_device), "cudaSetDevice final catalog");
  result.catalog.source_coordinate_bits = copy_from_device(
      views.device_coordinate_bits,
      3U * views.point_count,
      "cudaMemcpy final retained source coordinates");
  result.catalog.support_u = copy_from_device(
      views.device_support_u,
      views.catalog_record_count,
      "cudaMemcpy final catalog support-u");
  result.catalog.support_v = copy_from_device(
      views.device_support_v,
      views.catalog_record_count,
      "cudaMemcpy final catalog support-v");
  result.catalog.closed_rank = copy_from_device(
      views.device_closed_rank,
      views.catalog_record_count,
      "cudaMemcpy final catalog closed-rank");
  result.catalog.rank_offsets = copy_from_device(
      views.device_rank_offsets,
      views.rank_offset_count,
      "cudaMemcpy final catalog rank offsets");
  result.catalog.strict_offsets = copy_from_device(
      views.device_strict_offsets,
      views.record_offset_count,
      "cudaMemcpy final catalog strict offsets");
  result.catalog.strict_point_ids = copy_from_device(
      views.device_strict_point_ids,
      views.strict_point_id_count,
      "cudaMemcpy final catalog strict payload");
  result.catalog.shell_offsets = copy_from_device(
      views.device_shell_offsets,
      views.record_offset_count,
      "cudaMemcpy final catalog shell offsets");
  result.catalog.shell_point_ids = copy_from_device(
      views.device_shell_point_ids,
      views.shell_point_id_count,
      "cudaMemcpy final catalog shell payload");
  validate_catalog_against_reference(cloud, result.reference, result.catalog);
  return result;
}

[[nodiscard]] bool same_scientific_catalog(
    const ResidentCatalogCopy& left,
    const ResidentCatalogCopy& right) {
  return left.source_coordinate_bits == right.source_coordinate_bits &&
      left.support_u == right.support_u && left.support_v == right.support_v &&
      left.closed_rank == right.closed_rank &&
      left.rank_offsets == right.rank_offsets &&
      left.strict_offsets == right.strict_offsets &&
      left.strict_point_ids == right.strict_point_ids &&
      left.shell_offsets == right.shell_offsets &&
      left.shell_point_ids == right.shell_point_ids &&
      left.recomputed_squared_levels == right.recomputed_squared_levels &&
      left.digest == right.digest;
}

[[nodiscard]] std::string json_escape(std::string_view text) {
  std::string result;
  result.reserve(text.size());
  for (const char raw_character : text) {
    const unsigned char character =
        static_cast<unsigned char>(raw_character);
    switch (character) {
      case '"':
        result += "\\\"";
        break;
      case '\\':
        result += "\\\\";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
        if (character < 0x20U) {
          result += '?';
        } else {
          result += static_cast<char>(character);
        }
        break;
    }
  }
  return result;
}

void print_failure_json(std::string_view message) {
  std::cout
      << "{\"backend\":\"" << kBackend << "\","
      << "\"component_only\":true,"
      << "\"error\":\"" << json_escape(message) << "\","
      << "\"full_gamma2_claimed\":false,"
      << "\"full_k2_hierarchy_claimed\":false,"
      << "\"git_sha\":\"" << json_escape(kGitSha) << "\","
      << "\"mode\":\"" << kMode << "\","
      << "\"morse_hgp_hierarchy_claimed\":false,"
      << "\"profile\":\"" << kProfile << "\","
      << "\"public_status\":\"" << kPublicStatus << "\","
      << "\"schema\":\"" << kSchema << "\","
      << "\"success\":false}"
      << '\n';
}

void print_success_json(
    const Options& options,
    const GeneratedFixture& fixture,
    const std::vector<RunResult>& runs) {
  const RunResult& primary = runs.front();
  std::size_t total_commits = 0U;
  for (const RunResult& run : runs) {
    total_commits = checked_add(
        total_commits, run.commit_count, "the total commit count overflowed");
  }
  std::cout
      << "{\"anchor_tile_capacities\":[";
  for (std::size_t index = 0U; index < runs.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    std::cout << runs[index].anchor_tile_capacity;
  }
  std::cout
      << "],\"backend\":\"" << kBackend << "\","
      << "\"bounded_cpu_oracle\":true,"
      << "\"catalog_digest_fnv1a\":" << primary.catalog.digest << ','
      << "\"closed_ranks_qualified\":[2,3],"
      << "\"component_only\":true,"
      << "\"deployment_status\":\"" << kDeploymentStatus << "\","
      << "\"deterministic_dyadic_fixture\":true,"
      << "\"exact_fallback_count\":0,"
      << "\"exact_level_persisted_in_gpu_soa\":false,"
      << "\"exact_levels_divide_by_four_recomputed\":true,"
      << "\"exact_levels_match_cpu_oracle\":true,"
      << "\"fixture_clustered_point_count\":"
      << fixture.features.clustered_point_count << ','
      << "\"fixture_exact_shell_point_count\":"
      << fixture.features.exact_shell_point_count << ','
      << "\"fixture_jitter_grid_point_count\":"
      << fixture.features.jitter_grid_point_count << ','
      << "\"fixture_strict_collinear_point_count\":"
      << fixture.features.strict_collinear_point_count << ','
      << "\"full_gamma2_claimed\":false,"
      << "\"full_k2_hierarchy_claimed\":false,"
      << "\"git_sha\":\"" << json_escape(kGitSha) << "\","
      << "\"global_cell_or_coface_arena_materialized\":false,"
      << "\"global_pair_matrix_materialized\":false,"
      << "\"hierarchy_reduction_performed\":false,"
      << "\"industrial_scale_claimed\":false,"
      << "\"independent_all_pair_all_witness_oracle\":true,"
      << "\"intermediate_candidate_d2h\":false,"
      << "\"maximum_closed_rank\":3,"
      << "\"maximum_oracle_point_count\":" << kMaximumPointCount << ','
      << "\"mode\":\"" << kMode << "\","
      << "\"morse_hgp_hierarchy_claimed\":false,"
      << "\"ordinary_delaunay_materialized\":false,"
      << "\"point_count\":" << options.point_count << ','
      << "\"profile\":\"" << kProfile << "\","
      << "\"public_pipeline_lbvh_frontier_classifier_finish\":true,"
      << "\"public_terminal_wrapper_and_lease_lifetime_qualified\":true,"
      << "\"public_status\":\"" << kPublicStatus << "\","
      << "\"qualification_capacity_source\":\"bounded_cpu_oracle_only\","
      << "\"qualification_final_catalog_and_source_d2h\":true,"
      << "\"rank_two_record_count\":"
      << primary.reference.rank_two.records.size() << ','
      << "\"rank_three_record_count\":"
      << primary.reference.rank_three.records.size() << ','
      << "\"requested_orders\":[1,2],"
      << "\"run_count\":" << runs.size() << ','
      << "\"schema\":\"" << kSchema << "\","
      << "\"single_run\":" << (options.single_run ? "true" : "false")
      << ','
      << "\"strict_payload_point_id_count\":"
      << primary.catalog.strict_point_ids.size() << ','
      << "\"success\":true,"
      << "\"support2_rank23_catalog_qualified\":true,"
      << "\"shell_payload_point_id_count\":"
      << primary.catalog.shell_point_ids.size() << ','
      << "\"tile_capacity_invariance_qualified\":"
      << (runs.size() > 1U ? "true" : "false") << ','
      << "\"total_commit_count\":" << total_commits << ','
      << "\"zero_fallback_required_for_success\":true}"
      << '\n';
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options options = parse_options(argc, argv);
    const GeneratedFixture fixture = generate_fixture(options.point_count);
    const CanonicalPointCloud cloud =
        CanonicalPointCloud::rejecting_duplicates(
            std::span<const CertifiedPoint3>{fixture.points});
    require(
        cloud.size() == options.point_count,
        "canonicalization changed the deterministic fixture cardinality");

    std::vector<std::size_t> tile_capacities;
    if (!options.single_run) {
      tile_capacities.push_back(1U);
    }
    if (tile_capacities.empty() ||
        tile_capacities.back() != options.anchor_tile_capacity) {
      tile_capacities.push_back(options.anchor_tile_capacity);
    }

    std::vector<RunResult> runs;
    runs.reserve(tile_capacities.size());
    for (const std::size_t tile_capacity : tile_capacities) {
      runs.push_back(run_once(cloud, tile_capacity));
    }
    if (runs.size() > 1U) {
      require(
          runs[0U].reference.rank_two == runs[1U].reference.rank_two &&
              runs[0U].reference.rank_three ==
                  runs[1U].reference.rank_three &&
              same_scientific_catalog(
                  runs[0U].catalog, runs[1U].catalog),
          "tile capacities one and the requested capacity changed the exact "
          "scientific catalogue");
    }
    print_success_json(options, fixture, runs);
    return 0;
  } catch (const std::exception& error) {
    print_failure_json(error.what());
    return 1;
  }
}
