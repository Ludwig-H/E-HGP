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
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/hierarchy/local_germination.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
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
using morsehgp3d::hierarchy::LocalGerminationProductionAudit;
using morsehgp3d::hierarchy::generate_local_germination_candidates;
using morsehgp3d::hierarchy::local_germination_certificate_admissible;
using morsehgp3d::hierarchy::local_germination_production_identity_holds;
using morsehgp3d::hierarchy::local_germination_resume_induction_holds;
using morsehgp3d::hierarchy::local_germination_proof_basis;
using morsehgp3d::hierarchy::sealed_tangent_covering_radius_millidegrees;
using morsehgp3d::hierarchy::ExactHigherSupportEvent;
using morsehgp3d::hierarchy::ExactHigherSupportExtraShellDiagnostic;
using morsehgp3d::hierarchy::ExactHigherSupportVerificationBasis;
using morsehgp3d::hierarchy::ExactLocalGerminationChain;
using morsehgp3d::hierarchy::canonical_name;
using morsehgp3d::hierarchy::verification_basis_consumable_by_mass_partition;
using morsehgp3d::hierarchy::verification_basis_guarantees;
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
    std::size_t support_size,
    std::size_t tangent_direction_count = 0U) {
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
  config.tangent_direction_count = tangent_direction_count;
  std::set<Support> emitted;
  // Tallied by the CONSUMER, deliberately separate from the dedup set, so that
  // the production identity compares two independent accountings.
  LocalGerminationProductionAudit audit;
  const LocalGerminationCertificate certificate =
      generate_local_germination_candidates(
          index,
          cloud,
          support_size,
          config,
          [&](const Support& candidate, std::size_t size) {
            require(size == support_size, "the sink received the wrong arity");
            if (size == 3U) {
              ++audit.observed_triple_emissions;
            } else {
              ++audit.observed_quadruple_emissions;
            }
            emitted.insert(candidate);
          });
  audit.accepted_supports = reference.size();

  const LocalGerminationCounters& counters = certificate.counters;
  std::string reason;
  require(
      certificate.applicable && certificate.executed,
      std::string{family} +
          ": an executed applicable arity lost its lifecycle state");
  require(
      local_germination_certificate_admissible(certificate, reason),
      std::string{"a legitimate certificate was refused on "} + reason);
  require(
      local_germination_production_identity_holds(certificate, audit, reason),
      std::string{"the production identity failed on "} + reason);

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
          std::to_string(support_size) +
          (tangent_direction_count == 0U
               ? " (exhaustive seed loop)"
               : " (certified tangent restriction)"));
  // This finite exact differential establishes equality on the fixture, but it
  // does not certify every binary64 rejection on arbitrary inputs.  The current
  // producer must therefore remain structurally admissible yet publicly
  // incomplete until outward intervals and exact fail-open fallbacks exist.
  require(
      !certificate.floating_rejections_certified &&
          !certificate.completeness_guaranteed(),
      std::string{family} +
          ": a finite differential promoted floating proposals to a proof");

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
  std::string reason;
  LocalGerminationCertificate default_certificate;
  require(
      !default_certificate.completeness_guaranteed(),
      "a default-constructed certificate claimed completeness");
  require(
      !local_germination_certificate_admissible(
          default_certificate, reason) &&
          reason == "certificate_not_applicable",
      "a default-constructed certificate was admissible");

  LocalGerminationCertificate pending = default_certificate;
  pending.applicable = true;
  pending.support_size = 4U;
  pending.maximum_relevant_closed_rank = 11U;
  require(
      !pending.completeness_guaranteed() &&
          !local_germination_certificate_admissible(pending, reason) &&
          reason == "certificate_not_executed",
      "an applicable but unexecuted certificate claimed a guarantee");

  const auto fresh = []() {
    LocalGerminationCertificate certificate;
    certificate.applicable = true;
    certificate.executed = true;
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
  require(
      local_germination_certificate_admissible(fresh(), reason),
      "the theorem's own constants were refused");
  require(
      !fresh().completeness_guaranteed(),
      "structural constants alone certified binary64 rejections");

  LocalGerminationCertificate forged_floating_proof = fresh();
  forged_floating_proof.floating_rejections_certified = true;
  require(
      !local_germination_certificate_admissible(
          forged_floating_proof, reason) &&
          reason ==
              "floating_rejections_claimed_certified_without_interval_proof",
      "an unimplemented floating rejection proof was admitted");

  LocalGerminationCertificate dangling_remainder = fresh();
  dangling_remainder.unexamined_seed_pair_count = 1U;
  require(
      !dangling_remainder.completeness_guaranteed() &&
          !local_germination_certificate_admissible(
              dangling_remainder, reason) &&
          reason == "operational_deadline_accounting",
      "an uncensored certificate retained an unexamined seed pair");

  LocalGerminationCertificate empty_censor = fresh();
  empty_censor.censored_by_operational_deadline = true;
  require(
      !empty_censor.completeness_guaranteed() &&
          !local_germination_certificate_admissible(empty_censor, reason) &&
          reason == "operational_deadline_accounting",
      "a censored certificate claimed that no seed pair remained");

  LocalGerminationCertificate honest_censor = fresh();
  honest_censor.censored_by_operational_deadline = true;
  honest_censor.unexamined_seed_pair_count = 1U;
  require(
      local_germination_certificate_admissible(honest_censor, reason) &&
          !honest_censor.completeness_guaranteed(),
      "an honest censored invocation was confused with a complete one");

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

  // A restricted seed loop is allowed -- it is how the enumeration scales --
  // but it may not also claim to be exhaustive.  The certified restriction is
  // D <= 2 R(p), which this basis does not carry.
  LocalGerminationCertificate honest_cutoff = fresh();
  honest_cutoff.seed_restriction =
      LocalGerminationCertificate::SeedRestriction::declared_heuristic_cutoff;
  honest_cutoff.seed_neighbourhood_cutoff_multiple = 6U;
  honest_cutoff.seed_loop_exhaustive_over_pairs = false;
  require(
      !honest_cutoff.completeness_guaranteed(),
      "a heuristic cutoff claimed to guarantee completeness");
  require(
      local_germination_certificate_admissible(honest_cutoff, reason),
      "an honestly declared seed restriction was refused");

  // The six signed axes are the only direction set with a proof on record, and
  // the sealed value rounds arccos(1/sqrt(3)) UP so it is never below the truth.
  require(
      sealed_tangent_covering_radius_millidegrees(6U) == 54736U,
      "the sealed covering radius of the six axes drifted");
  require(
      sealed_tangent_covering_radius_millidegrees(14U) == 36207U,
      "the sealed covering radius of the fourteen directions drifted");
  require(
      sealed_tangent_covering_radius_millidegrees(26U) == 27570U,
      "the sealed covering radius of the twenty-six directions drifted");
  require(
      sealed_tangent_covering_radius_millidegrees(48U) == 0U,
      "a covering radius was sealed without a proof");

  // The sealed values must round the exact radii UP: a sealed value below the
  // truth would admit an understated covering and lose a direction.
  const double exact_6 =
      std::acos(1.0 / std::sqrt(3.0)) * 180.0 / std::numbers::pi;
  const double exact_14 =
      std::acos(1.0 / std::sqrt(5.0 - 2.0 * std::sqrt(3.0))) * 180.0 /
      std::numbers::pi;
  const double exact_26 =
      std::acos(1.0 / std::sqrt(9.0 - 2.0 * std::sqrt(2.0) -
                                2.0 * std::sqrt(6.0))) *
      180.0 / std::numbers::pi;
  require(
      static_cast<double>(sealed_tangent_covering_radius_millidegrees(6U)) >=
              exact_6 * 1000.0 &&
          static_cast<double>(
              sealed_tangent_covering_radius_millidegrees(14U)) >=
              exact_14 * 1000.0 &&
          static_cast<double>(
              sealed_tangent_covering_radius_millidegrees(26U)) >=
              exact_26 * 1000.0,
      "a sealed covering radius rounds below its exact value");

  LocalGerminationCertificate proved_axes = fresh();
  proved_axes.seed_restriction =
      LocalGerminationCertificate::SeedRestriction::declared_heuristic_cutoff;
  proved_axes.seed_loop_exhaustive_over_pairs = false;
  proved_axes.seed_neighbourhood_cutoff_multiple = 6U;
  proved_axes.tangent_direction_count = 6U;
  proved_axes.tangent_covering_radius_millidegrees = 54736U;
  proved_axes.tangent_covering_radius_proved = true;
  require(
      local_germination_certificate_admissible(proved_axes, reason),
      "the proved six-axis declaration was refused");

  // Understating the covering radius leaves a direction untested and can lose a
  // support: refused by name.
  LocalGerminationCertificate understated = proved_axes;
  understated.tangent_covering_radius_millidegrees = 54735U;
  require(
      !local_germination_certificate_admissible(understated, reason) &&
          reason == "tangent_covering_radius_below_the_sealed_proof",
      "a covering radius below the sealed proof was admitted");

  // A finer set may be used -- it is measurably far better -- but it may not
  // claim a proof that is not on record.
  // The tangent restriction is geometrically certified, but the complete
  // producer is not: binary64 rejection predicates remain proposal-only.
  LocalGerminationCertificate certified = fresh();
  certified.seed_restriction =
      LocalGerminationCertificate::SeedRestriction::certified_tangent_bound;
  certified.seed_loop_exhaustive_over_pairs = false;
  certified.tangent_direction_count = 26U;
  certified.tangent_covering_radius_millidegrees = 27570U;
  certified.tangent_covering_radius_proved = true;
  require(
      local_germination_certificate_admissible(certified, reason) &&
          !certified.floating_rejections_certified &&
          !certified.completeness_guaranteed(),
      "the certified tangent regime promoted uncertified floating rejections");

  LocalGerminationCertificate unproved_certified = certified;
  unproved_certified.tangent_direction_count = 48U;
  unproved_certified.tangent_covering_radius_proved = false;
  require(
      !local_germination_certificate_admissible(unproved_certified, reason) &&
          reason ==
              "a_certified_tangent_bound_without_a_proved_covering_radius",
      "a certified regime without a proved covering radius was admitted");

  LocalGerminationCertificate proved_26 = fresh();
  proved_26.seed_restriction =
      LocalGerminationCertificate::SeedRestriction::declared_heuristic_cutoff;
  proved_26.seed_loop_exhaustive_over_pairs = false;
  proved_26.seed_neighbourhood_cutoff_multiple = 6U;
  proved_26.tangent_direction_count = 26U;
  proved_26.tangent_covering_radius_millidegrees = 27570U;
  proved_26.tangent_covering_radius_proved = true;
  require(
      local_germination_certificate_admissible(proved_26, reason),
      "the proved twenty-six-direction declaration was refused");

  LocalGerminationCertificate estimated = fresh();
  estimated.seed_restriction =
      LocalGerminationCertificate::SeedRestriction::declared_heuristic_cutoff;
  estimated.seed_loop_exhaustive_over_pairs = false;
  estimated.seed_neighbourhood_cutoff_multiple = 6U;
  estimated.tangent_direction_count = 48U;
  estimated.tangent_covering_radius_millidegrees = 24000U;
  estimated.tangent_covering_radius_proved = false;
  require(
      local_germination_certificate_admissible(estimated, reason),
      "an honestly estimated covering radius was refused");

  LocalGerminationCertificate pretending = estimated;
  pretending.tangent_covering_radius_proved = true;
  require(
      !local_germination_certificate_admissible(pretending, reason) &&
          reason == "tangent_covering_radius_claimed_proved_without_a_seal",
      "an unproved covering radius claimed as proved was admitted");

  LocalGerminationCertificate orphan_proof = fresh();
  orphan_proof.tangent_covering_radius_proved = true;
  require(
      !local_germination_certificate_admissible(orphan_proof, reason) &&
          reason == "tangent_covering_radius_proved_without_a_direction_set",
      "a proof without a direction set was admitted");

  LocalGerminationCertificate lying_cutoff = fresh();
  lying_cutoff.seed_restriction =
      LocalGerminationCertificate::SeedRestriction::declared_heuristic_cutoff;
  lying_cutoff.seed_neighbourhood_cutoff_multiple = 6U;
  lying_cutoff.seed_loop_exhaustive_over_pairs = true;
  require(
      !local_germination_certificate_admissible(lying_cutoff, reason) &&
          reason == "a_restricted_seed_loop_claimed_to_be_exhaustive",
      "a restricted seed loop claiming exhaustiveness was admitted");
}

// A request whose rank ceiling excludes an arity is not a zero-work complete
// run.  The producer returns an identified non-applicable lifecycle slot and
// neither invokes the sink nor publishes a proof basis.
void check_non_applicable_producer_state() {
  const CanonicalPointCloud cloud = CanonicalPointCloud::rejecting_duplicates(
      uniform_latin_points(8U));
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  LocalGerminationConfig config;
  config.maximum_relevant_closed_rank = 3U;
  bool sink_called = false;
  const LocalGerminationCertificate certificate =
      generate_local_germination_candidates(
          index,
          cloud,
          4U,
          config,
          [&](const Support&, std::size_t) { sink_called = true; });

  require(!sink_called, "a non-applicable arity invoked its sink");
  require(
      !certificate.applicable && !certificate.executed &&
          certificate.support_size == 4U &&
          certificate.maximum_relevant_closed_rank == 3U &&
          certificate.proof_basis.empty() &&
          certificate.counters.candidates() == 0U &&
          !certificate.completeness_guaranteed(),
      "the producer misreported a non-applicable arity");

  std::string reason;
  require(
      !local_germination_certificate_admissible(certificate, reason) &&
          reason == "certificate_not_applicable",
      "a non-applicable lifecycle slot was accepted as a certificate");
  require(
      !local_germination_production_identity_holds(
          certificate, LocalGerminationProductionAudit{}, reason) &&
          reason == "certificate_not_applicable",
      "a non-applicable lifecycle slot acquired a production identity");
}

// The contract of the three verification bases.  What each one guarantees is a
// table, and the table is the contract: a consumer branches on it instead of
// re-reading prose, so a wrong entry is a wrong guarantee.
void check_verification_basis_contract() {
  using Basis = ExactHigherSupportVerificationBasis;

  const auto replay =
      verification_basis_guarantees(Basis::fresh_cpu_replay_every_commit);
  require(
      replay.mass_partition_identity_available &&
          replay.fresh_replay_every_commit &&
          !replay.requires_local_germination_completeness_certificate,
      "the fresh-replay basis lost a guarantee");

  const auto tile = verification_basis_guarantees(
      Basis::device_search_host_exact_record_classification_bigint_closure);
  require(
      tile.mass_partition_identity_available &&
          !tile.fresh_replay_every_commit &&
          !tile.requires_local_germination_completeness_certificate,
      "the tile-certified basis lost a guarantee");

  // The germination basis is the only one that does NOT partition the universe,
  // and the only one whose completeness rests on an external certificate.
  const auto germination = verification_basis_guarantees(
      Basis::local_germination_completeness_with_exact_terminal_classification);
  require(
      !germination.mass_partition_identity_available &&
          !germination.fresh_replay_every_commit &&
          germination.requires_local_germination_completeness_certificate,
      "the germination basis misdeclares what it provides");

  // Fail-closed: every consumer of an anchored chain requires the mass
  // partition identity today, so the germination basis must be refused by name
  // until the production identity replaces the coverage one on the bridge.
  require(
      verification_basis_consumable_by_mass_partition(
          Basis::fresh_cpu_replay_every_commit),
      "the fresh-replay basis became unconsumable");
  require(
      verification_basis_consumable_by_mass_partition(
          Basis::device_search_host_exact_record_classification_bigint_closure),
      "the tile-certified basis became unconsumable");
  require(
      !verification_basis_consumable_by_mass_partition(
          Basis::local_germination_completeness_with_exact_terminal_classification),
      "the germination basis was admitted by a mass-partition consumer");

  // The canonical names are part of the certificate wire and must stay stable
  // and distinct.
  require(
      canonical_name(Basis::fresh_cpu_replay_every_commit) ==
          "fresh_cpu_replay_every_commit",
      "the fresh-replay basis name drifted");
  require(
      canonical_name(
          Basis::device_search_host_exact_record_classification_bigint_closure) ==
          "device_search_host_exact_record_classification_bigint_closure",
      "the tile-certified basis name drifted");
  require(
      canonical_name(
          Basis::local_germination_completeness_with_exact_terminal_classification) ==
          "local_germination_completeness_with_exact_terminal_classification",
      "the germination basis name drifted");
}

// The identity of production replaces the identity of coverage, so it must be
// as refusable as the mass partition it stands in for.  Each forgery below is a
// way the accounting could drift from what was really emitted.
void check_production_identity_falsification() {
  LocalGerminationCertificate certificate;
  certificate.applicable = true;
  certificate.executed = true;
  certificate.proof_basis = std::string{local_germination_proof_basis};
  certificate.support_size = 4U;
  certificate.maximum_relevant_closed_rank = 11U;
  certificate.jung_squared_numerator = 3U;
  certificate.jung_squared_denominator = 8U;
  certificate.seed_disc_ring_count = 2U;
  certificate.segment_position_count = 16U;
  certificate.certified_margin_exponent = -20;
  certificate.counters.pairs_examined = 100U;
  certificate.counters.pairs_retained = 40U;
  certificate.counters.third_vertices_examined = 300U;
  certificate.counters.third_vertices_free_rejected = 100U;
  certificate.counters.third_vertices_retained = 150U;
  certificate.counters.quadruple_candidates = 500U;

  LocalGerminationProductionAudit audit;
  audit.observed_quadruple_emissions = 500U;
  audit.accepted_supports = 12U;

  std::string reason;
  require(
      local_germination_production_identity_holds(certificate, audit, reason),
      "a coherent production accounting was refused");

  LocalGerminationCertificate unexecuted = certificate;
  unexecuted.executed = false;
  require(
      !local_germination_production_identity_holds(
          unexecuted, audit, reason) &&
          reason == "certificate_not_executed",
      "an unexecuted certificate acquired a production identity");

  // The consumer saw fewer emissions than the producer counted.
  LocalGerminationProductionAudit short_tally = audit;
  short_tally.observed_quadruple_emissions = 499U;
  require(
      !local_germination_production_identity_holds(
          certificate, short_tally, reason) &&
          reason == "observed_quadruple_emissions",
      "a tally below the producer's count was admitted");

  // Acceptance above production is exactly the failure completeness excludes:
  // a support accepted that the generator never emitted.
  LocalGerminationProductionAudit over_accepted = audit;
  over_accepted.accepted_supports = 501U;
  require(
      !local_germination_production_identity_holds(
          certificate, over_accepted, reason) &&
          reason == "accepted_supports_exceed_production",
      "an acceptance above production was admitted");

  LocalGerminationCertificate mixed = certificate;
  mixed.counters.triple_candidates = 1U;
  require(
      !local_germination_production_identity_holds(mixed, audit, reason) &&
          reason == "a_quadruple_run_produced_triples",
      "a mixed-arity run was admitted");

  LocalGerminationCertificate inflated = certificate;
  inflated.counters.pairs_retained = 101U;
  require(
      !local_germination_production_identity_holds(inflated, audit, reason) &&
          reason == "pairs_retained_exceed_pairs_examined",
      "more pairs retained than examined was admitted");

  LocalGerminationCertificate impossible = certificate;
  impossible.counters.third_vertices_retained = 250U;  // 250 + 100 > 300
  require(
      !local_germination_production_identity_holds(impossible, audit, reason) &&
          reason == "third_vertex_accounting_exceeds_its_examination",
      "a third-vertex accounting above its examination was admitted");

  LocalGerminationCertificate orphan = certificate;
  orphan.counters.pairs_retained = 0U;
  require(
      !local_germination_production_identity_holds(orphan, audit, reason) &&
          reason == "third_vertices_without_a_retained_pair",
      "third vertices without a retained pair were admitted");

  LocalGerminationCertificate barren = certificate;
  barren.counters.third_vertices_retained = 0U;
  require(
      !local_germination_production_identity_holds(barren, audit, reason),
      "candidates without a retained third vertex were admitted");
}

// The resume induction conserves an accounting instead of a mass: a resumed run
// may extend what was produced, never revise it, and never under another
// constant.
void check_resume_induction() {
  LocalGerminationCertificate previous;
  previous.applicable = true;
  previous.executed = true;
  previous.proof_basis = std::string{local_germination_proof_basis};
  previous.support_size = 4U;
  previous.maximum_relevant_closed_rank = 11U;
  previous.jung_squared_numerator = 3U;
  previous.jung_squared_denominator = 8U;
  previous.seed_disc_ring_count = 2U;
  previous.segment_position_count = 16U;
  previous.certified_margin_exponent = -20;
  previous.counters.pairs_examined = 100U;
  previous.counters.pairs_retained = 40U;
  previous.counters.third_vertices_examined = 300U;
  previous.counters.third_vertices_retained = 150U;
  previous.counters.quadruple_candidates = 500U;
  previous.counters.population_queries = 900U;
  previous.censored_by_operational_deadline = true;
  previous.unexamined_seed_pair_count = 100U;

  std::string reason;
  LocalGerminationCertificate extended = previous;
  extended.counters.pairs_examined = 180U;
  extended.counters.quadruple_candidates = 700U;
  extended.counters.population_queries = 1500U;
  extended.unexamined_seed_pair_count = 20U;
  require(
      local_germination_resume_induction_holds(previous, extended, reason),
      std::string{"a legitimate resumption was refused on "} + reason);

  require(
      local_germination_resume_induction_holds(previous, previous, reason),
      "an idle resumption was refused");

  LocalGerminationCertificate completed = extended;
  completed.counters.pairs_examined = 200U;
  completed.censored_by_operational_deadline = false;
  completed.unexamined_seed_pair_count = 0U;
  require(
      local_germination_resume_induction_holds(extended, completed, reason),
      std::string{"a legitimate completion was refused on "} + reason);

  LocalGerminationCertificate forged_completion = previous;
  forged_completion.censored_by_operational_deadline = false;
  forged_completion.unexamined_seed_pair_count = 0U;
  require(
      !local_germination_resume_induction_holds(
          previous, forged_completion, reason) &&
          reason == "seed_pair_total_changed",
      "a censored run discarded its remaining seed pairs without work");

  LocalGerminationCertificate drifting_remainder = extended;
  drifting_remainder.unexamined_seed_pair_count = 19U;
  require(
      !local_germination_resume_induction_holds(
          previous, drifting_remainder, reason) &&
          reason == "seed_pair_total_changed",
      "a resumption changed the total seed-pair universe");

  LocalGerminationCertificate premature_uncensor = extended;
  premature_uncensor.censored_by_operational_deadline = false;
  require(
      !local_germination_resume_induction_holds(
          previous, premature_uncensor, reason) &&
          reason == "successor_operational_deadline_accounting",
      "a resumption hid a non-zero unexamined remainder");

  LocalGerminationCertificate complete_extension = completed;
  complete_extension.counters.population_queries += 1U;
  require(
      !local_germination_resume_induction_holds(
          completed, complete_extension, reason) &&
          reason == "a_complete_seed_loop_was_extended",
      "a complete seed loop acquired new work");

  LocalGerminationCertificate execution_state_changed = extended;
  execution_state_changed.executed = false;
  require(
      !local_germination_resume_induction_holds(
          previous, execution_state_changed, reason) &&
          reason == "execution_state_changed",
      "a resumption changed whether its producer had executed");

  // A run that changed its constant is a different producer, and appending its
  // records to the first would silently mix two completeness arguments.
  LocalGerminationCertificate reconstant = extended;
  reconstant.jung_squared_numerator = 1U;
  reconstant.jung_squared_denominator = 2U;  // conservative, but DIFFERENT
  require(
      !local_germination_resume_induction_holds(previous, reconstant, reason) &&
          reason == "jung_squared_changed",
      "a resumption under another Jung constant was admitted");

  LocalGerminationCertificate restricted = extended;
  restricted.seed_neighbourhood_cutoff_multiple = 6U;
  restricted.seed_loop_exhaustive_over_pairs = false;
  require(
      !local_germination_resume_induction_holds(previous, restricted, reason) &&
          reason == "seed_loop_restriction_changed",
      "a resumption that restricted its seed loop mid-chain was admitted");

  LocalGerminationCertificate recovered = extended;
  recovered.segment_position_count = 8U;
  require(
      !local_germination_resume_induction_holds(previous, recovered, reason) &&
          reason == "segment_position_count_changed",
      "a resumption under another segment covering was admitted");

  LocalGerminationCertificate rewound = extended;
  rewound.counters.quadruple_candidates = 499U;
  require(
      !local_germination_resume_induction_holds(previous, rewound, reason) &&
          reason == "quadruple_candidates_went_backwards",
      "a resumption revising its production downwards was admitted");

  LocalGerminationCertificate relaxed = extended;
  relaxed.mass_partition_identity_available = true;
  require(
      !local_germination_resume_induction_holds(previous, relaxed, reason) &&
          reason == "declared_guarantees_changed",
      "a resumption changing its declared guarantees was admitted");
}

// The chain: the discipline of the anchored session, on a state that has no
// frontier and no mass.  A refusal must leave the trusted certificate exactly
// as it was, and the chain must keep working afterwards -- the innocuity the
// tile-certified path had to prove too.
void check_germination_chain() {
  const auto certificate_at = [](std::size_t pairs, std::size_t quadruples,
                                 std::size_t third_retained,
                                 std::size_t unexamined) {
    LocalGerminationCertificate certificate;
    certificate.applicable = true;
    certificate.executed = true;
    certificate.proof_basis = std::string{local_germination_proof_basis};
    certificate.support_size = 4U;
    certificate.maximum_relevant_closed_rank = 11U;
    certificate.jung_squared_numerator = 3U;
    certificate.jung_squared_denominator = 8U;
    certificate.seed_disc_ring_count = 2U;
    certificate.segment_position_count = 16U;
    certificate.certified_margin_exponent = -20;
    certificate.counters.pairs_examined = pairs;
    certificate.counters.pairs_retained = pairs / 2U;
    certificate.counters.third_vertices_examined = third_retained * 2U;
    certificate.counters.third_vertices_retained = third_retained;
    certificate.counters.quadruple_candidates = quadruples;
    certificate.counters.population_queries = pairs * 3U;
    certificate.censored_by_operational_deadline = unexamined != 0U;
    certificate.unexamined_seed_pair_count = unexamined;
    return certificate;
  };
  const auto audit_for = [](std::size_t quadruples, std::size_t accepted) {
    LocalGerminationProductionAudit audit;
    audit.observed_quadruple_emissions = quadruples;
    audit.accepted_supports = accepted;
    return audit;
  };

  ExactLocalGerminationChain chain{certificate_at(100U, 200U, 60U, 200U)};
  std::string reason;

  require(
      chain.commit(
          certificate_at(200U, 400U, 120U, 100U),
          audit_for(400U, 10U), {}, {}, reason),
      std::string{"a legitimate first batch was refused on "} + reason);
  require(chain.committed_batch_count() == 1U, "the first batch did not count");
  const ExactLocalGerminationChain::Seal censored_seal = chain.seal();
  require(
      !censored_seal.sealed &&
          !censored_seal.completeness_certificate_verified &&
          !chain.sealed(),
      "a censored germination chain was sealed as complete");

  // A successor that revises its production downwards must be refused, and the
  // trusted certificate must not move.
  const LocalGerminationCertificate before_refusal = chain.trusted();
  require(
      !chain.commit(
          certificate_at(300U, 399U, 130U, 0U),
          audit_for(399U, 10U), {}, {}, reason) &&
          reason == "quadruple_candidates_went_backwards",
      "a production revised downwards was committed");
  require(
      chain.trusted().counters.quadruple_candidates ==
          before_refusal.counters.quadruple_candidates,
      "a refused commit moved the trusted certificate");

  // A successor whose consumer tally disagrees with its counters is refused by
  // the production identity, not by the induction.
  require(
      !chain.commit(
          certificate_at(300U, 600U, 130U, 0U),
          audit_for(599U, 10U), {}, {}, reason) &&
          reason == "observed_quadruple_emissions",
      "a disagreeing tally was committed");

  // A successor under another constant is a different producer.
  LocalGerminationCertificate reconstant =
      certificate_at(300U, 600U, 130U, 0U);
  reconstant.jung_squared_numerator = 1U;
  reconstant.jung_squared_denominator = 2U;
  require(
      !chain.commit(reconstant, audit_for(600U, 10U), {}, {}, reason) &&
          reason == "jung_squared_changed",
      "a resumption under another constant was committed");

  // Innocuity: after three refusals the chain still commits.
  require(
      chain.commit(
          certificate_at(300U, 600U, 130U, 0U),
          audit_for(600U, 10U), {}, {}, reason),
      std::string{"the chain stopped working after a refusal: "} + reason);
  require(chain.committed_batch_count() == 2U, "the second batch did not count");

  const ExactLocalGerminationChain::Seal seal = chain.seal();
  require(
      !seal.sealed && !chain.sealed(),
      "a transcript without cursor, payload or floating proof sealed");
  require(
      seal.basis ==
          ExactHigherSupportVerificationBasis::
              local_germination_completeness_with_exact_terminal_classification,
      "the seal declared the wrong basis");
  require(
      !seal.contiguous_seed_pair_prefix_verified &&
          !seal.accepted_payload_identity_verified &&
          !seal.floating_rejections_verified &&
          !seal.completeness_certificate_verified,
      "the diagnostic transcript forged a missing sealing obligation");
  // The seal restates what this basis does NOT provide, so no consumer has to
  // infer it.
  require(
      !seal.mass_partition_identity_available &&
          !verification_basis_consumable_by_mass_partition(seal.basis),
      "the seal claimed a mass partition it does not have");
  require(seal.committed_batch_count == 2U, "the seal miscounted its batches");
  require(
      seal.event_count == 0U,
      "the empty payload fixture unexpectedly acquired events");

  // A chain whose genesis is inadmissible commits nothing and seals nothing.
  LocalGerminationCertificate bad_genesis =
      certificate_at(10U, 10U, 5U, 0U);
  bad_genesis.jung_squared_numerator = 1U;
  bad_genesis.jung_squared_denominator = 3U;  // below 3/8 for a tetrahedron
  ExactLocalGerminationChain refused{bad_genesis};
  require(
      !refused.commit(
          certificate_at(20U, 20U, 10U, 0U),
          audit_for(20U, 1U), {}, {}, reason) &&
          reason == "the_genesis_certificate_was_not_admissible",
      "a chain with an inadmissible genesis committed");
  require(!refused.seal().sealed, "a chain with an inadmissible genesis sealed");

  LocalGerminationCertificate honest_heuristic =
      certificate_at(10U, 10U, 5U, 0U);
  honest_heuristic.seed_restriction =
      LocalGerminationCertificate::SeedRestriction::declared_heuristic_cutoff;
  honest_heuristic.seed_loop_exhaustive_over_pairs = false;
  honest_heuristic.seed_neighbourhood_cutoff_multiple = 6U;
  require(
      local_germination_certificate_admissible(honest_heuristic, reason) &&
          !honest_heuristic.completeness_guaranteed(),
      "an honest heuristic certificate lost its structural admissibility");
  ExactLocalGerminationChain heuristic_chain{honest_heuristic};
  const ExactLocalGerminationChain::Seal heuristic_seal =
      heuristic_chain.seal();
  require(
      !heuristic_seal.sealed &&
          !heuristic_seal.completeness_certificate_verified,
      "an honest heuristic invocation was sealed as complete");
}

// A measurement, not an assertion: at these sizes the exhaustive reference is
// out of reach, so what is reported is the generator's own accounting.  It
// exists to see whether the certified restriction BITES, which is the one thing
// the completeness cells cannot show.
void report_selectivity(
    std::string_view family,
    const std::vector<CertifiedPoint3>& input,
    std::size_t maximum_order,
    std::size_t support_size,
    std::size_t tangent_direction_count) {
  const CanonicalPointCloud cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{input});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  LocalGerminationConfig config;
  config.maximum_relevant_closed_rank =
      std::min(maximum_order + 1U, cloud.size());
  config.seed_disc_ring_count = 2U;
  config.segment_position_count = 8U;
  config.tangent_direction_count = tangent_direction_count;
  const LocalGerminationCertificate certificate =
      generate_local_germination_candidates(
          index, cloud, support_size, config,
          [](const Support&, std::size_t) {});
  std::string reason;
  require(
      local_germination_certificate_admissible(certificate, reason),
      std::string{"the selectivity run produced an inadmissible certificate: "} +
          reason);
  const LocalGerminationCounters& counters = certificate.counters;
  std::cout << "  " << family << " n=" << cloud.size() << " K=" << maximum_order
            << " m=" << support_size << " directions="
            << tangent_direction_count << " | pairs "
            << counters.pairs_retained << '/' << counters.pairs_examined
            << " | third " << counters.third_vertices_retained << '/'
            << counters.third_vertices_examined << " | candidates "
            << counters.candidates() << " | queries "
            << counters.population_queries << '\n';
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
    // The certified tangent restriction must lose NOTHING: the same clouds,
    // now with the seed loop bounded by D <= 2 R(p) at both endpoints, under
    // the twenty-six-direction set whose covering radius is proved.
    std::cout << "with the certified tangent restriction (26 directions)\n";
    for (const std::size_t size : {3U, 4U}) {
      check_family("uniform_latin", uniform_latin_points(24U), 5U, size, 26U);
      check_family("eight_clusters", eight_clusters_points(24U), 5U, size, 26U);
    }
    check_family("eight_clusters", eight_clusters_points(40U), 5U, 4U, 26U);
    std::cout << "selectivity, where the restriction can bite\n";
    for (const std::size_t directions : {0U, 26U}) {
      report_selectivity("uniform_latin", uniform_latin_points(512U), 10U, 3U,
                         directions);
      report_selectivity("eight_clusters", eight_clusters_points(512U), 10U, 3U,
                         directions);
    }
    check_certificate_falsification();
    check_non_applicable_producer_state();
    check_verification_basis_contract();
    check_production_identity_falsification();
    check_resume_induction();
    check_germination_chain();
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
