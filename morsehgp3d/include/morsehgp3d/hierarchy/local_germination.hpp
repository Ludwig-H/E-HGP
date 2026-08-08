#pragma once

#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace morsehgp3d::hierarchy {

// Reference host generator of local proposals for minimal well-centred
// supports of size three and four.
//
// It replaces the product subdivision, whose cost is measured proportional to
// C(n,3)+C(n,4), by an enumeration seeded on the DIAMETER PAIR of each support.
// Its candidate locus rests on two proved statements, both recorded with their proofs
// in docs/math/OPTIMISATIONS_JUNG_SUPPORTS_3_4.md:
//
//   * Jung -- a minimal well-centred support has its circumball AS its
//     miniball, so r <= gamma_m diam(U) with gamma_3 = 1/sqrt(3) for a planar
//     triangle and gamma_4 = sqrt(3/8) for a tetrahedron; together with the
//     trivial diam(U) <= 2r this confines the circumradius to the COMPACT
//     interval [D/2, gamma_m D] once the diameter edge is fixed;
//   * the cascade -- with the diameter edge fixed, the locus of compatible
//     circumcentres is a disc of radius sqrt(gamma^2 - 1/4) D in the
//     perpendicular bisector plane, and adding a third vertex reduces it to a
//     segment of half-length sqrt(gamma^2 D^2 - r_triangle^2).
//
// The geometric lemmas above are exact, but this prototype does not yet turn
// every rejection into a certified decision.  Covering positions, range
// queries and several gates still use binary64 without a complete outward
// interval and exact fallback.  The generator therefore emits proposals and
// its bounded differentials are useful falsifiers, but its current executable
// certificate MUST NOT claim completeness.  The exact terminal classification
// -- analyze_circumcenter_support_integer then the indexed closed-ball query --
// remains the sole authority on acceptance, unchanged.
//
// This host reference deliberately scans the cloud for its neighbourhoods
// instead of traversing the LBVH.  It exists to compare finite fixtures against
// the exhaustive census and to count the work an implementation would do; a
// device implementation must use the index.

struct LocalGerminationConfig {
  // s_max = min(K+1, n), exactly as the higher-support stream derives it.
  std::size_t maximum_relevant_closed_rank{};
  // Hexagonal rings covering the disc of compatible centres (J7).  Zero means
  // the single-ball seed test (J6).
  std::size_t seed_disc_ring_count{2U};
  // Positions covering the segment of compatible centres (J8).  One means the
  // single-ball incremental test.
  std::size_t segment_position_count{16U};
  // The squared Jung constant actually used, as an EXACT rational.  Zero means
  // "take the theorem's value": 1/3 for a triangle, 3/8 for a tetrahedron.
  // It is a configuration field precisely so that a caller can declare a
  // different one and have the certificate refuse it -- a completeness claim
  // must be falsifiable at its constant.
  std::uint64_t jung_squared_numerator{};
  std::uint64_t jung_squared_denominator{};
  // Certified margin subtracted from every test radius, as 2^exponent times the
  // diameter.  Must be at most zero, so the margin never exceeds the diameter.
  int certified_margin_exponent{-20};
  // Direction set used to bound the tangent radius R(p).  Zero leaves the seed
  // loop exhaustive; 6, 14 or 26 select a set whose covering radius is sealed
  // with a proof.  The geometric restriction is D <= 2 R(p); the current
  // binary64 implementation of R(p) is still proposal-only.
  std::size_t tangent_direction_count{};
  // Restriction of the seed loop to pairs closer than this multiple of the
  // local s_max-nearest-neighbour radius.  Zero means NO restriction: the seed
  // loop is exhaustive over pairs.  Exhaustion closes the seed identity, not
  // the still-uncertified floating rejections inside each seed.
  //
  // Any positive value is a DATA-DEPENDENT restriction that the theorem does
  // not supply.  The certified restriction is D <= 2 R(p), where R(p) is the
  // largest radius of a ball through p, centred inside the convex hull, holding
  // at most s_max points -- and computing it is the directional covering route
  // that was measured and set aside.  Until that route is implemented, a
  // positive cutoff must be declared and must NOT claim to be justified.
  std::size_t seed_neighbourhood_cutoff_multiple{};
};

struct LocalGerminationCounters {
  std::size_t pairs_examined{};
  std::size_t pairs_retained{};
  std::size_t third_vertices_examined{};
  std::size_t third_vertices_free_rejected{};
  std::size_t third_vertices_retained{};
  std::size_t triple_candidates{};
  std::size_t quadruple_candidates{};
  std::size_t population_queries{};

  [[nodiscard]] std::size_t candidates() const noexcept {
    return triple_candidates + quadruple_candidates;
  }
};

// Historical identifier of the geometric proof basis.  The name is preserved
// for wire compatibility, but the executable must additionally certify its
// floating rejections before it can turn that argument into completeness.
inline constexpr std::string_view local_germination_proof_basis =
    "jung_diameter_seed_disc_and_segment_covering_completeness_v1";

// What a run relied upon, in a form a verifier can refuse.
struct LocalGerminationCertificate {
  // Lifecycle is explicit.  `applicable` says that the requested closed-rank
  // ceiling admits this arity; `executed` says that the generator was actually
  // invoked and returned a certificate for it.  In particular, an applicable
  // arity skipped after an earlier operational deadline is distinct from an
  // arity that the request did not contain at all.
  //
  // Both default to false so a value-initialised slot can never accidentally
  // claim completeness merely because SeedRestriction defaults to exhaustive.
  bool applicable{false};
  bool executed{false};
  std::string proof_basis;
  std::size_t support_size{};
  std::size_t maximum_relevant_closed_rank{};
  std::uint64_t jung_squared_numerator{};
  std::uint64_t jung_squared_denominator{};
  std::size_t seed_disc_ring_count{};
  std::size_t segment_position_count{};
  int certified_margin_exponent{};
  std::size_t seed_neighbourhood_cutoff_multiple{};
  // How the seed loop was restricted, and therefore what its completeness rests
  // on.  Only the first two guarantee it.
  enum class SeedRestriction : std::uint8_t {
    exhaustive_over_pairs = 0U,
    certified_tangent_bound = 1U,
    declared_heuristic_cutoff = 2U,
  };
  SeedRestriction seed_restriction{SeedRestriction::exhaustive_over_pairs};
  // The direction set used to bound the tangent radius R(p), and the covering
  // radius claimed for it in millidegrees.  A covering radius that is too small
  // would leave a direction untested and could lose a support, so the verifier
  // refuses any value below the sealed proved one for the declared count.
  std::size_t tangent_direction_count{};
  std::size_t tangent_covering_radius_millidegrees{};
  // True only when the declared covering radius is a PROVED value for the
  // declared direction set, not an estimate.  Today exactly one entry is
  // sealed with a proof.
  bool tangent_covering_radius_proved{};
  // True only after every floating-point rejection that can omit a candidate
  // has an outward interval or an exact fail-open fallback.  The current host
  // prototype has no such proof and always leaves this false.  The structural
  // verifier rejects a producer that tries to set it under the v1 basis.
  bool floating_rejections_certified{false};
  // True only when the seed loop was exhaustive over pairs.  A run that
  // restricted it owes a completeness argument the theorem does not supply, and
  // the verifier refuses a certificate that claims otherwise.
  bool seed_loop_exhaustive_over_pairs{true};

  // An operational deadline interrupted the seed loop.  This is an observation
  // censored by a session guard, not a defect and not a budget -- but a seed
  // loop that stopped early has not visited every seed, so whatever the
  // restriction regime was, the run is NOT complete.  Recording it in the
  // certificate is what stops a censored measurement being read as a
  // completeness claim.
  bool censored_by_operational_deadline{false};
  // Pairs the seed loop had left to examine when the deadline bit.  Zero on
  // every uncensored run, and the honest measure of how far a censored one got.
  std::size_t unexamined_seed_pair_count{};

  // Completeness additionally requires certified rejection predicates.  The
  // current implementation deliberately returns false even for an uncensored
  // exhaustive run: the exact differential validates its finite fixtures, not
  // every binary64 decision on every input.
  [[nodiscard]] bool completeness_guaranteed() const noexcept {
    return applicable && executed && !censored_by_operational_deadline &&
        unexamined_seed_pair_count == 0U && floating_rejections_certified &&
        (seed_restriction == SeedRestriction::exhaustive_over_pairs ||
         (seed_restriction == SeedRestriction::certified_tangent_bound &&
          tangent_covering_radius_proved));
  }
  // The mass partition identity does NOT survive this basis: the generator
  // never enumerates the universe, so unexamined mass is excluded by the
  // theorem rather than resolved by pruning.  Declaring it available would be
  // a false claim, and the verifier refuses a certificate that does.
  bool mass_partition_identity_available{false};
  bool completeness_basis_declared{true};
  LocalGerminationCounters counters;
};

// The sealed covering radius of a direction set, in millidegrees, or zero when
// no proof is on record for that count.
//
// Six signed axes: for any unit u, the identity sum u_i^2 = 1 forces
// max_i |u_i| >= 1/sqrt(3), so the nearest signed axis is within
// arccos(1/sqrt(3)) = 54.7356 degrees, and (1,1,1)/sqrt(3) attains it.  That is
// a one-line proof and it is the only one on record.
//
// Finer sets are measurably far better -- the offset factor 1 - sin(theta) goes
// from 0.1835 at six directions to 0.6001 at forty-eight, and the certified
// neighbourhood at fifty thousand points from 10 292 to 515 -- but their
// covering radius is only ESTIMATED here.  The obligation is finite and
// mechanical rather than open: the covering radius of a fixed finite set on the
// sphere is attained at a vertex of its spherical Voronoi diagram, so it is an
// exactly computable algebraic number for any concrete set.  It simply has to
// be computed once, offline, and sealed here.
[[nodiscard]] std::size_t sealed_tangent_covering_radius_millidegrees(
    std::size_t direction_count) noexcept;

// Re-derives the structural admissibility of a certificate from its declared
// constants and deadline accounting.  Admissible does NOT mean complete: a
// censored, floating-proposal or honestly heuristic invocation can be
// admissible while
// `completeness_guaranteed()` remains false.  It does not trust the producer:
// it checks that the declared squared Jung
// constant is at least the theorem's value -- a LARGER one only shrinks the
// test balls and weakens rejection, a smaller one would move the centre locus
// and could lose an accepted support -- that the disc stays real, that the
// margin is a proper fraction of the diameter, and that the basis claims
// completeness rather than coverage.  On refusal `reason` names the field.
[[nodiscard]] bool local_germination_certificate_admissible(
    const LocalGerminationCertificate& certificate, std::string& reason);

// What an INDEPENDENT observer counted at the sink boundary.  The point of the
// production identity is that these numbers are tallied by the consumer, not by
// the generator, so agreeing with the certificate's own counters is a genuine
// cross-check -- the direct analogue of the BigInt mass cross-validation the
// bridge performs under the coverage bases.
struct LocalGerminationProductionAudit {
  std::size_t observed_triple_emissions{};
  std::size_t observed_quadruple_emissions{};
  // Emissions the exact terminal classification accepted.  Acceptance is a
  // subset of production; this is what makes the identity say something.
  std::size_t accepted_supports{};
};

// The identity of PRODUCTION, which replaces the identity of COVERAGE under the
// germination basis.
//
// Under the coverage bases the chain checks a partition of the universe:
// resolved plus remaining equals C(n,3) + C(n,4).  The germination basis has no
// such partition -- the universe is never enumerated -- so what a commit can
// still check is that the records it carries are exactly what the declared
// configuration produced, and that acceptance is a subset of production.  That
// is an identity on the accounting itself, cross-checked against a consumer's
// independent tally.
//
// On refusal `reason` names the identity that failed.
[[nodiscard]] bool local_germination_production_identity_holds(
    const LocalGerminationCertificate& certificate,
    const LocalGerminationProductionAudit& audit,
    std::string& reason);

// The resume induction, under the new conserved quantity.
//
// Under the coverage bases the induction conserves a mass: the successor
// checkpoint satisfies R + C(F) = C(n,3) + C(n,4) exactly as its predecessor
// did.  The germination basis conserves an ACCOUNTING instead: a resumed run
// may only extend what the interrupted one had produced, never revise it.
//
// So the successor must declare the SAME constants -- a run that changed its
// squared Jung constant, its coverings or its margin mid-chain is a different
// run and its records may not be appended to the first -- every counter must be
// non-decreasing, and `pairs_examined + unexamined_seed_pair_count` must be
// conserved exactly.  This is only a necessary accounting invariant: the
// certificate carries no ordinal cursor or prefix digest, so it does not prove
// that the newly examined pairs form the missing contiguous suffix.  On
// refusal `reason` names the field.
[[nodiscard]] bool local_germination_resume_induction_holds(
    const LocalGerminationCertificate& previous,
    const LocalGerminationCertificate& successor,
    std::string& reason);

// Receives every emitted candidate: the support ids in strictly increasing
// order, and its size.
using LocalGerminationSink =
    std::function<void(const std::array<spatial::PointId, 4>&, std::size_t)>;

// Emits candidate supports of size `support_size`, keyed on a pair that
// realises the support's diameter.  The GUARANTEE is completeness: every
// minimal well-centred support of closed rank at most
// `maximum_relevant_closed_rank` is emitted at least once.
//
// It is deliberately NOT uniqueness.  When a support's diameter is attained by
// several pairs, each of them owns it and emits it, so the consumer must
// deduplicate.  Breaking the tie here would mean comparing distances for
// equality in binary64, which is exactly the kind of fragile decision this
// project refuses to take outside exact arithmetic; the exact tie-break belongs
// with the exact classifier, which sees the support anyway.
// An operational guard, passed BY CALL and never sealed into the config, on
// the model the device bridge has carried since R2-h.  It bounds how long a
// measurement may run; it is not a budget, and its firing censors an
// observation rather than reporting a defect.  A default-constructed guard is
// disarmed.
//
// The deadline is tested between seed pairs, so it is honoured to within one
// pair's worth of work -- the same shape as the higher stage's operational
// quantum.
struct LocalGerminationGuard {
  bool armed{false};
  std::chrono::steady_clock::time_point deadline{};
  // Seed pairs between two deadline tests, which bounds the overshoot to that
  // many pairs' work.
  //
  // The default is small on purpose.  At arity four one seed pair carries its
  // whole third-and-fourth-vertex cascade, so an interval of 4096 was measured
  // to turn a 12 s deadline into 63.8 s -- a factor 5.3, and exactly the defect
  // 9d72726 named in the higher stage: a deadline that overshoots by an
  // unbounded factor is not a deadline.  A clock read costs tens of
  // nanoseconds against a pair costing milliseconds, so testing often is
  // cheap; the field stays configurable for a caller who has measured
  // otherwise.
  std::size_t seed_pairs_per_deadline_test{64U};
};

[[nodiscard]] LocalGerminationCertificate generate_local_germination_candidates(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t support_size,
    const LocalGerminationConfig& config,
    const LocalGerminationSink& sink,
    const LocalGerminationGuard& guard = LocalGerminationGuard{});

// The anchored chain of the germination basis.
//
// It is a SEPARATE object rather than a new mode of the frontier-based session,
// and deliberately so: that session's whole state is a frontier and a mass
// partition, neither of which exists here, and bending it would put the two
// already certified bases at risk for no gain.  What is kept is the discipline,
// not the state -- one trusted certificate, a deterministic successor, and a
// refusal that leaves the trusted state exactly as it was.
//
// A commit is admitted only when all three structural contracts hold at once: the
// successor certificate is admissible, its production identity holds against
// the consumer's own tally, and it extends the trusted certificate rather than
// revising it.  Everything is checked BEFORE anything is mutated, so a refusal
// cannot leave a half-applied state and the chain never needs poisoning.  The
// current API cannot bind a resumed interval to a contiguous seed-pair cursor
// and does not bind accepted audit counts to the event payload vectors.  It is
// therefore deliberately UNSEALABLE until both identities are implemented.
class ExactLocalGerminationChain {
 public:
  struct Seal {
    bool sealed{};
    ExactHigherSupportVerificationBasis basis{
        ExactHigherSupportVerificationBasis::
            local_germination_completeness_with_exact_terminal_classification};
    LocalGerminationCertificate certificate;
    std::size_t committed_batch_count{};
    std::size_t event_count{};
    std::size_t diagnostic_count{};
    // Restated on the seal so a consumer never has to infer it: this basis
    // provides no mass partition, and its completeness rests on both the
    // declared constant being admissible and the seed loop being complete.
    bool mass_partition_identity_available{false};
    bool contiguous_seed_pair_prefix_verified{};
    bool accepted_payload_identity_verified{};
    bool floating_rejections_verified{};
    bool completeness_certificate_verified{};
  };

  explicit ExactLocalGerminationChain(LocalGerminationCertificate genesis);

  // Returns false and leaves the chain untouched when any contract fails;
  // `reason` then names the contract and its field.
  [[nodiscard]] bool commit(
      const LocalGerminationCertificate& successor,
      const LocalGerminationProductionAudit& audit,
      std::vector<ExactHigherSupportEvent> events,
      std::vector<ExactHigherSupportExtraShellDiagnostic> diagnostics,
      std::string& reason);

  [[nodiscard]] const LocalGerminationCertificate& trusted() const noexcept {
    return trusted_;
  }
  [[nodiscard]] std::size_t committed_batch_count() const noexcept {
    return committed_batch_count_;
  }
  [[nodiscard]] const std::vector<ExactHigherSupportEvent>& events()
      const noexcept {
    return events_;
  }
  [[nodiscard]] bool sealed() const noexcept { return sealed_; }

  [[nodiscard]] Seal seal();

 private:
  LocalGerminationCertificate trusted_;
  std::vector<ExactHigherSupportEvent> events_;
  std::vector<ExactHigherSupportExtraShellDiagnostic> diagnostics_;
  std::size_t committed_batch_count_{};
  bool genesis_admissible_{};
  bool sealed_{};
};

}  // namespace morsehgp3d::hierarchy
