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
using morsehgp3d::hierarchy::ExactHigherSupportVerificationBasis;
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

  std::string reason;
  LocalGerminationCertificate extended = previous;
  extended.counters.pairs_examined = 180U;
  extended.counters.quadruple_candidates = 700U;
  extended.counters.population_queries = 1500U;
  require(
      local_germination_resume_induction_holds(previous, extended, reason),
      std::string{"a legitimate resumption was refused on "} + reason);

  require(
      local_germination_resume_induction_holds(previous, previous, reason),
      "an idle resumption was refused");

  // A run that changed its constant is a different producer, and appending its
  // records to the first would silently mix two completeness arguments.
  LocalGerminationCertificate reconstant = extended;
  reconstant.jung_squared_numerator = 1U;
  reconstant.jung_squared_denominator = 2U;  // conservative, but DIFFERENT
  require(
      !local_germination_resume_induction_holds(previous, reconstant, reason) &&
          reason == "jung_squared_changed",
      "a resumption under another Jung constant was admitted");

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
    check_verification_basis_contract();
    check_production_identity_falsification();
    check_resume_induction();
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
