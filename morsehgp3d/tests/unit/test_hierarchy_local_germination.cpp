// Completeness certificate of the local germination generator.
//
// The generator claims one thing: every minimal well-centred support of closed
// rank at most s_max is emitted.  This test falsifies that claim the only way
// that means anything -- against the EXHAUSTIVE enumeration of C(n,3)+C(n,4)
// classified by the same exact primitives the product path uses at a terminal.
// If a single accepted support is missing from the generator's output, the test
// fails and names it.
//
// It runs on both sanctioned families, and `eight_clusters` is mandatory here:
// `uniform_latin` contains no minimal well-centred quadruple at any measured
// size, so a test on it alone would never exercise arity four.

#include "morsehgp3d/exact/integer_circumcenter.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/support.hpp"
#include "morsehgp3d/hierarchy/higher_support_closed_ball.hpp"
#include "morsehgp3d/hierarchy/local_germination.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::CircumcenterSupportStatus;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::exact::IntegerCircumcenterAnalysis;
using morsehgp3d::exact::analyze_circumcenter_support_integer;
using morsehgp3d::hierarchy::ExactHigherSupportClosedBallClassification;
using morsehgp3d::hierarchy::ExactHigherSupportClosedBallOutcome;
using morsehgp3d::hierarchy::ExactHigherSupportIndexedClosedBallQuery;
using morsehgp3d::hierarchy::LocalGerminationCertificate;
using morsehgp3d::hierarchy::LocalGerminationConfig;
using morsehgp3d::hierarchy::LocalGerminationCounters;
using morsehgp3d::hierarchy::generate_local_germination_candidates;
using morsehgp3d::hierarchy::local_germination_certificate_admissible;
using morsehgp3d::hierarchy::local_germination_proof_basis;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

int failures = 0;

void require(bool condition, std::string_view message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAILED: " << message << '\n';
  }
}

using Support = std::array<PointId, 4>;

[[nodiscard]] std::vector<CertifiedPoint3> uniform_latin_points(
    std::size_t point_count) {
  constexpr std::size_t modulus = 65537U;
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    const std::size_t value = index + 1U;
    points.push_back(CertifiedPoint3::from_binary64(
        static_cast<double>(value) / static_cast<double>(modulus),
        static_cast<double>((value * 25173U + 13849U) % modulus) /
            static_cast<double>(modulus),
        static_cast<double>((value * 13849U + 25173U) % modulus) /
            static_cast<double>(modulus)));
  }
  return points;
}

[[nodiscard]] std::vector<CertifiedPoint3> eight_clusters_points(
    std::size_t point_count) {
  constexpr double local_scale = 1.0 / 1048576.0;
  constexpr double transverse_scale = 1.0 / 4194304.0;
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    const std::size_t cluster = index % 8U;
    const std::size_t local = index / 8U + 1U;
    points.push_back(CertifiedPoint3::from_binary64(
        ((cluster & 1U) == 0U ? 0.25 : 0.75) +
            static_cast<double>(local) * local_scale,
        ((cluster & 2U) == 0U ? 0.25 : 0.75) +
            static_cast<double>((local * 40503U + cluster * 7919U) % 65536U) *
                transverse_scale,
        ((cluster & 4U) == 0U ? 0.25 : 0.75) +
            static_cast<double>((local * 25717U + cluster * 104729U) % 65536U) *
                transverse_scale));
  }
  return points;
}

// The one classification the product path uses at a terminal, transcribed
// exactly: geometry gate first, closed-ball rank only for a minimal support,
// acceptance only when the shell equals the support.
template <std::size_t SupportSize>
[[nodiscard]] bool accepted(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const Support& ids,
    std::size_t maximum_relevant_closed_rank,
    std::size_t frontier_bound) {
  std::array<ExactRational3, SupportSize> support_points{};
  for (std::size_t position = 0U; position < SupportSize; ++position) {
    support_points[position] = cloud.point(ids[position]).exact();
  }
  const IntegerCircumcenterAnalysis analysis =
      analyze_circumcenter_support_integer(support_points);
  if (analysis.status != CircumcenterSupportStatus::minimal) {
    return false;
  }
  const ExactHigherSupportClosedBallClassification classification =
      ExactHigherSupportIndexedClosedBallQuery::classify(
          index,
          cloud,
          ids,
          SupportSize,
          *analysis.center,
          *analysis.squared_level,
          maximum_relevant_closed_rank - SupportSize,
          frontier_bound);
  if (classification.outcome ==
      ExactHigherSupportClosedBallOutcome::rank_exceeded) {
    return false;
  }
  return classification.shell_count == SupportSize;
}

void check_family(
    std::string_view family,
    const std::vector<CertifiedPoint3>& input,
    std::size_t maximum_order,
    std::size_t support_size) {
  const CanonicalPointCloud cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{input});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::size_t point_count = cloud.size();
  const std::size_t rank = std::min(maximum_order + 1U, point_count);
  const std::size_t frontier_bound =
      ExactHigherSupportIndexedClosedBallQuery::proved_traversal_frontier_bound(
          index);
  if (rank < support_size) {
    return;
  }

  // The reference answer: every accepted support of this arity.
  std::set<Support> reference;
  Support ids{};
  const auto classify = [&]() {
    const bool ok = support_size == 3U
        ? accepted<3U>(index, cloud, ids, rank, frontier_bound)
        : accepted<4U>(index, cloud, ids, rank, frontier_bound);
    if (ok) {
      reference.insert(ids);
    }
  };
  const PointId bound = static_cast<PointId>(point_count);
  if (support_size == 3U) {
    for (PointId a = 0U; a + 2U < bound; ++a) {
      for (PointId b = a + 1U; b + 1U < bound; ++b) {
        for (PointId c = b + 1U; c < bound; ++c) {
          ids = {a, b, c, 0U};
          classify();
        }
      }
    }
  } else {
    for (PointId a = 0U; a + 3U < bound; ++a) {
      for (PointId b = a + 1U; b + 2U < bound; ++b) {
        for (PointId c = b + 1U; c + 1U < bound; ++c) {
          for (PointId d = c + 1U; d < bound; ++d) {
            ids = {a, b, c, d};
            classify();
          }
        }
      }
    }
  }

  LocalGerminationConfig config;
  config.maximum_relevant_closed_rank = rank;
  config.seed_disc_ring_count = 2U;
  config.segment_position_count = 8U;
  std::set<Support> emitted;
  const LocalGerminationCertificate certificate =
      generate_local_germination_candidates(
          index,
          cloud,
          support_size,
          config,
          [&](const Support& candidate, std::size_t size) {
            require(size == support_size, "the sink received the wrong arity");
            emitted.insert(candidate);
          });

  const LocalGerminationCounters& counters = certificate.counters;
  std::string reason;
  require(
      local_germination_certificate_admissible(certificate, reason),
      std::string{"a legitimate certificate was refused on "} + reason);

  std::size_t missing = 0U;
  for (const Support& support : reference) {
    if (emitted.find(support) == emitted.end()) {
      if (missing == 0U) {
        std::cerr << "  first missing support:";
        for (std::size_t position = 0U; position < support_size; ++position) {
          std::cerr << ' ' << support[position];
        }
        std::cerr << '\n';
      }
      ++missing;
    }
  }
  require(
      missing == 0U,
      std::string{family} + ": the germination generator missed an accepted "
                            "support of arity " +
          std::to_string(support_size));

  std::cout << "  " << family << " n=" << point_count
            << " K=" << maximum_order << " m=" << support_size
            << " | accepted " << reference.size() << " | emitted "
            << emitted.size() << " | pairs " << counters.pairs_retained << '/'
            << counters.pairs_examined << " | third " << counters.third_vertices_retained
            << '/' << counters.third_vertices_examined << " (free rejects "
            << counters.third_vertices_free_rejected << ")"
            << " | candidates " << counters.candidates() << " | queries "
            << counters.population_queries;
  if (!reference.empty()) {
    std::cout << " | candidates per accepted "
              << (static_cast<double>(counters.candidates()) /
                  static_cast<double>(reference.size()));
  }
  std::cout << '\n';
}

// A completeness claim must be falsifiable at its constant.  Each forgery below
// is exactly the kind that would silently lose accepted supports, and each must
// be refused by name.
void check_certificate_falsification() {
  const auto fresh = []() {
    LocalGerminationCertificate certificate;
    certificate.proof_basis = std::string{local_germination_proof_basis};
    certificate.support_size = 4U;
    certificate.maximum_relevant_closed_rank = 11U;
    certificate.jung_squared_numerator = 3U;
    certificate.jung_squared_denominator = 8U;
    certificate.seed_disc_ring_count = 2U;
    certificate.segment_position_count = 16U;
    certificate.certified_margin_exponent = -20;
    return certificate;
  };
  std::string reason;
  require(
      local_germination_certificate_admissible(fresh(), reason),
      "the theorem's own constants were refused");

  // A SMALLER Jung constant shrinks the locus of centres and would lose
  // supports: it must be refused, where a larger one is merely conservative.
  LocalGerminationCertificate smaller = fresh();
  smaller.jung_squared_numerator = 1U;
  smaller.jung_squared_denominator = 3U;  // 1/3 < 3/8 for a tetrahedron
  require(
      !local_germination_certificate_admissible(smaller, reason) &&
          reason == "jung_squared_below_the_theorem",
      "a squared Jung constant below the theorem was admitted");

  LocalGerminationCertificate larger = fresh();
  larger.jung_squared_numerator = 1U;
  larger.jung_squared_denominator = 2U;  // 1/2 > 3/8, conservative
  require(
      local_germination_certificate_admissible(larger, reason),
      "a conservative squared Jung constant was refused");

  LocalGerminationCertificate flat = fresh();
  flat.jung_squared_numerator = 1U;
  flat.jung_squared_denominator = 4U;
  require(
      !local_germination_certificate_admissible(flat, reason),
      "a squared Jung constant leaving no disc was admitted");

  LocalGerminationCertificate forged = fresh();
  forged.proof_basis = "some_other_basis";
  require(
      !local_germination_certificate_admissible(forged, reason) &&
          reason == "proof_basis",
      "a forged proof basis was admitted");

  LocalGerminationCertificate coverage = fresh();
  coverage.mass_partition_identity_available = true;
  require(
      !local_germination_certificate_admissible(coverage, reason),
      "a certificate claiming the mass partition identity was admitted");

  LocalGerminationCertificate silent = fresh();
  silent.completeness_basis_declared = false;
  require(
      !local_germination_certificate_admissible(silent, reason),
      "a certificate declaring no basis at all was admitted");

  LocalGerminationCertificate wide = fresh();
  wide.certified_margin_exponent = 1;
  require(
      !local_germination_certificate_admissible(wide, reason),
      "a margin exceeding the diameter was admitted");

  LocalGerminationCertificate empty_segment = fresh();
  empty_segment.segment_position_count = 0U;
  require(
      !local_germination_certificate_admissible(empty_segment, reason),
      "an empty segment covering was admitted");
}

}  // namespace

int main() {
  try {
    std::cout << "local germination completeness against the exhaustive "
                 "enumeration\n";
    for (const std::size_t order : {3U, 5U}) {
      for (const std::size_t size : {3U, 4U}) {
        check_family("uniform_latin", uniform_latin_points(24U), order, size);
        check_family("eight_clusters", eight_clusters_points(24U), order, size);
      }
    }
    // One larger cloud on the family that actually produces quadruples.
    check_family("eight_clusters", eight_clusters_points(40U), 5U, 4U);
    check_family("eight_clusters", eight_clusters_points(40U), 5U, 3U);
    check_certificate_falsification();
  } catch (const std::exception& error) {
    std::cerr << "local germination test threw: " << error.what() << '\n';
    return 1;
  }
  if (failures != 0) {
    std::cerr << failures << " local-germination test(s) failed\n";
    return 1;
  }
  std::cout << "local germination tests passed\n";
  return 0;
}
