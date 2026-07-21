#include "morsehgp3d/exact/binary64_neighbors.hpp"
#include "morsehgp3d/spatial/h_polytope_reference.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud_aabb.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactBoundedHPolytopeReferenceDecision;
using morsehgp3d::spatial::ExactHPolytopeBoundaryKind;
using morsehgp3d::spatial::ExactHPolytopeHalfspace3;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;
using morsehgp3d::spatial::StrictDyadicPaddingDecision;
using morsehgp3d::spatial::StrictlyPaddedDyadicAabb3Result;
using morsehgp3d::spatial::build_exact_bounded_h_polytope_reference;
using morsehgp3d::spatial::build_exact_point_cloud_aabb;
using morsehgp3d::spatial::build_strictly_padded_dyadic_aabb;
using morsehgp3d::spatial::verify_strictly_padded_dyadic_aabb;

constexpr std::uint64_t positive_maximum_finite_bits =
    UINT64_C(0x7fefffffffffffff);
constexpr std::uint64_t negative_maximum_finite_bits =
    UINT64_C(0xffefffffffffffff);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message << " (unexpected exception: "
              << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] std::uint64_t word(double value) {
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CertifiedPoint3 point_bits(
    std::uint64_t x,
    std::uint64_t y,
    std::uint64_t z) {
  return CertifiedPoint3::from_binary64_bits({x, y, z});
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

void check_complete_verification(
    const CanonicalPointCloud& cloud,
    const StrictlyPaddedDyadicAabb3Result& result,
    const std::string& message) {
  const auto verification =
      verify_strictly_padded_dyadic_aabb(cloud, result);
  check(
      verification.audit_certified &&
          verification.decision_certified &&
          verification.failure_masks_certified &&
          verification.payload_shape_certified &&
          verification.exact_extrema_certified &&
          verification.extremum_witnesses_certified &&
          verification.finite_adjacent_padding_certified &&
          verification.exact_positive_padding_certified &&
          verification.all_sites_strictly_inside_certified &&
          verification.convex_hull_strictly_inside_certified &&
          verification.result_certified,
      message);
}

void test_binary64_finite_neighbors() {
  using morsehgp3d::exact::binary64_negative_zero;
  using morsehgp3d::exact::binary64_sign_mask;
  using morsehgp3d::exact::next_canonical_finite_binary64_bits;
  using morsehgp3d::exact::previous_canonical_finite_binary64_bits;

  check(
      previous_canonical_finite_binary64_bits(0U) ==
          binary64_sign_mask + UINT64_C(1) &&
          next_canonical_finite_binary64_bits(0U) == UINT64_C(1),
      "zero has the two minimum subnormal finite neighbours");
  check(
      next_canonical_finite_binary64_bits(
          binary64_sign_mask + UINT64_C(1)) == UINT64_C(0) &&
          previous_canonical_finite_binary64_bits(UINT64_C(1)) ==
              UINT64_C(0),
      "the two minimum subnormals meet at canonical positive zero");
  check(
      !previous_canonical_finite_binary64_bits(
           negative_maximum_finite_bits)
           .has_value() &&
          !next_canonical_finite_binary64_bits(
               positive_maximum_finite_bits)
               .has_value(),
      "the two outward finite extrema report no finite neighbour");

  const std::uint64_t one = word(1.0);
  check(
      previous_canonical_finite_binary64_bits(one) == one - UINT64_C(1) &&
          next_canonical_finite_binary64_bits(one) == one + UINT64_C(1),
      "a binade boundary uses its immediate bitwise neighbours");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(previous_canonical_finite_binary64_bits(
            binary64_negative_zero));
      },
      "negative zero is rejected as a noncanonical neighbour input");
  check_throws<std::domain_error>(
      [] {
        static_cast<void>(next_canonical_finite_binary64_bits(
            UINT64_C(0x7ff0000000000000)));
      },
      "infinity is rejected as a finite-neighbour input");
}

void test_singleton_signed_zero_certificate_and_base_polytope() {
  const std::array<CertifiedPoint3, 1> points{point_bits(
      morsehgp3d::exact::binary64_negative_zero,
      UINT64_C(0),
      morsehgp3d::exact::binary64_negative_zero)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const auto result = build_strictly_padded_dyadic_aabb(cloud);

  check(
      result.decision == StrictDyadicPaddingDecision::complete &&
          result.unavailable_lower_axis_mask == 0U &&
          result.unavailable_upper_axis_mask == 0U &&
          result.certificate.has_value(),
      "a signed-zero singleton produces a complete certificate");
  if (!result.certificate.has_value()) {
    return;
  }
  const auto& certificate = *result.certificate;
  const std::array<std::uint64_t, 3> expected_lower{
      morsehgp3d::exact::binary64_sign_mask + UINT64_C(1),
      morsehgp3d::exact::binary64_sign_mask + UINT64_C(1),
      morsehgp3d::exact::binary64_sign_mask + UINT64_C(1)};
  const std::array<std::uint64_t, 3> expected_upper{
      UINT64_C(1), UINT64_C(1), UINT64_C(1)};
  const ExactRational minimum_subnormal =
      ExactRational::from_binary64_bits(UINT64_C(1));
  check(
      certificate.exact_site_aabb.bounds.lower_binary64_bits ==
              std::array<std::uint64_t, 3>{} &&
          certificate.exact_site_aabb.bounds.upper_binary64_bits ==
              std::array<std::uint64_t, 3>{} &&
          certificate.omega.lower_binary64_bits == expected_lower &&
          certificate.omega.upper_binary64_bits == expected_upper,
      "signed zeros are canonicalized and padded by one subnormal word");
  check(
      certificate.lower_padding ==
              std::array<ExactRational, 3>{
                  minimum_subnormal, minimum_subnormal, minimum_subnormal} &&
          certificate.upper_padding ==
              std::array<ExactRational, 3>{
                  minimum_subnormal, minimum_subnormal, minimum_subnormal},
      "the six singleton margins are exact and strictly positive");
  check(
      result.audit.point_count == 1U &&
          result.audit.exact_coordinate_evaluation_count == 3U &&
          result.audit.exact_extremum_comparison_count == 0U,
      "the singleton scan exposes exact fixed work counters");
  check_complete_verification(
      cloud, result, "the singleton certificate passes every fresh check");

  const std::array<ExactHPolytopeHalfspace3, 0> no_halfspaces{};
  const auto base_polytope = build_exact_bounded_h_polytope_reference(
      no_halfspaces, certificate.omega);
  const std::array<ExactHPolytopeBoundaryKind, 6> expected_kinds{
      ExactHPolytopeBoundaryKind::box_lower_x,
      ExactHPolytopeBoundaryKind::box_upper_x,
      ExactHPolytopeBoundaryKind::box_lower_y,
      ExactHPolytopeBoundaryKind::box_upper_y,
      ExactHPolytopeBoundaryKind::box_lower_z,
      ExactHPolytopeBoundaryKind::box_upper_z};
  check(
      base_polytope.decision ==
              ExactBoundedHPolytopeReferenceDecision::complete_nonempty &&
          base_polytope.affine_dimension == 3U &&
          base_polytope.vertices.size() == 8U &&
          base_polytope.boundary_planes.size() == expected_kinds.size(),
      "the padded box is a valid full-dimensional C0 H-polytope");
  for (std::size_t index = 0U;
       index < base_polytope.boundary_planes.size() &&
       index < expected_kinds.size();
       ++index) {
    check(
        base_polytope.boundary_planes[index].kind == expected_kinds[index] &&
            !base_polytope.boundary_planes[index].constraint_id.has_value(),
        "each C0 box face remains artificial and semantically unlabelled");
  }
}

void test_extrema_ties_permutation_and_lbvh_factorization() {
  const std::vector<CertifiedPoint3> points{
      point(3.0, 7.0, -1.0),
      point(-2.0, 6.0, 1.0),
      point(3.0, 5.0, 2.0),
      point(-2.0, 5.0, 0.0)};
  const CanonicalPointCloud first_cloud = canonical_cloud(points);
  const auto first = build_strictly_padded_dyadic_aabb(first_cloud);

  std::array<std::size_t, 4> permutation{0U, 1U, 2U, 3U};
  std::size_t permutation_count = 0U;
  do {
    std::vector<CertifiedPoint3> permuted;
    permuted.reserve(permutation.size());
    for (const std::size_t source_index : permutation) {
      permuted.push_back(points[source_index]);
    }
    const CanonicalPointCloud permuted_cloud = canonical_cloud(permuted);
    check(
        build_strictly_padded_dyadic_aabb(permuted_cloud) == first,
        "every input permutation preserves the complete result");
    ++permutation_count;
  } while (std::next_permutation(permutation.begin(), permutation.end()));
  check(permutation_count == 24U, "all four-point permutations are tested");
  check(
      first.audit.point_count == 4U &&
          first.audit.exact_coordinate_evaluation_count == 12U &&
          first.audit.exact_extremum_comparison_count == 18U,
      "the four-point scan accounts for 3n evaluations and 6(n-1) comparisons");
  check(first.certificate.has_value(), "the tied-extrema fixture is paddable");
  if (!first.certificate.has_value()) {
    return;
  }
  check(
      first.certificate->exact_site_aabb.lower_witness_point_ids ==
              std::array<PointId, 3>{0U, 0U, 3U} &&
          first.certificate->exact_site_aabb.upper_witness_point_ids ==
              std::array<PointId, 3>{2U, 3U, 2U},
      "every tied extremum retains the least canonical PointId");
  const MortonLbvhIndex index = MortonLbvhIndex::build(first_cloud);
  check(
      index.root_aabb() == first.certificate->exact_site_aabb.bounds &&
          build_exact_point_cloud_aabb(first_cloud) ==
              first.certificate->exact_site_aabb,
      "the LBVH root and Phase 8 share the exact extrema primitive");
  check_complete_verification(
      first_cloud, first, "the tied-extrema certificate passes fresh replay");
}

void test_nontrivial_affine_degeneracies() {
  const std::array<CertifiedPoint3, 3> collinear_points{
      point(-2.0, 1.0, 1.0),
      point(0.0, 1.0, 1.0),
      point(3.0, 1.0, 1.0)};
  const CanonicalPointCloud collinear_cloud = canonical_cloud(collinear_points);
  const auto collinear =
      build_strictly_padded_dyadic_aabb(collinear_cloud);
  check(
      collinear.certificate.has_value() &&
          collinear.certificate->exact_site_aabb.bounds
                  .lower_binary64_bits[1] == word(1.0) &&
          collinear.certificate->exact_site_aabb.bounds
                  .upper_binary64_bits[1] == word(1.0) &&
          collinear.certificate->exact_site_aabb.bounds
                  .lower_binary64_bits[2] == word(1.0) &&
          collinear.certificate->exact_site_aabb.bounds
                  .upper_binary64_bits[2] == word(1.0),
      "a nontrivial collinear cloud pads both constant axes");
  check_complete_verification(
      collinear_cloud,
      collinear,
      "the nontrivial collinear certificate passes fresh replay");

  const std::array<CertifiedPoint3, 4> coplanar_points{
      point(-1.0, -1.0, 2.0),
      point(-1.0, 1.0, 2.0),
      point(1.0, -1.0, 2.0),
      point(1.0, 1.0, 2.0)};
  const CanonicalPointCloud coplanar_cloud = canonical_cloud(coplanar_points);
  const auto coplanar = build_strictly_padded_dyadic_aabb(coplanar_cloud);
  check(
      coplanar.certificate.has_value() &&
          coplanar.certificate->exact_site_aabb.bounds
                  .lower_binary64_bits[2] == word(2.0) &&
          coplanar.certificate->exact_site_aabb.bounds
                  .upper_binary64_bits[2] == word(2.0),
      "a nontrivial coplanar cloud pads its constant normal axis");
  check_complete_verification(
      coplanar_cloud,
      coplanar,
      "the nontrivial coplanar certificate passes fresh replay");
}

void test_inward_extrema_succeed_and_outward_extrema_fail_closed() {
  const std::uint64_t negative_inner =
      negative_maximum_finite_bits - UINT64_C(1);
  const std::uint64_t positive_inner =
      positive_maximum_finite_bits - UINT64_C(1);
  const std::array<CertifiedPoint3, 2> interior_points{
      point_bits(negative_inner, UINT64_C(0), UINT64_C(0)),
      point_bits(UINT64_C(0), positive_inner, word(1.0))};
  const CanonicalPointCloud interior_cloud = canonical_cloud(interior_points);
  const auto interior = build_strictly_padded_dyadic_aabb(interior_cloud);
  check(
      interior.certificate.has_value() &&
          interior.certificate->omega.lower_binary64_bits[0] ==
              negative_maximum_finite_bits &&
          interior.certificate->omega.upper_binary64_bits[1] ==
              positive_maximum_finite_bits,
      "the words immediately inside both extrema remain finitely paddable");
  check_complete_verification(
      interior_cloud, interior, "the near-extreme finite padding is certified");

  const std::array<CertifiedPoint3, 2> boundary_points{
      point_bits(negative_maximum_finite_bits, UINT64_C(0), UINT64_C(0)),
      point_bits(UINT64_C(0), positive_maximum_finite_bits, word(1.0))};
  const CanonicalPointCloud boundary_cloud = canonical_cloud(boundary_points);
  const auto boundary = build_strictly_padded_dyadic_aabb(boundary_cloud);
  check(
      boundary.decision ==
              StrictDyadicPaddingDecision::unsupported_finite_binary64_range &&
          boundary.unavailable_lower_axis_mask == UINT8_C(1) &&
          boundary.unavailable_upper_axis_mask == UINT8_C(2) &&
          !boundary.certificate.has_value(),
      "all nonpaddable axes are reported without a partial clipping box");
  check(
      verify_strictly_padded_dyadic_aabb(boundary_cloud, boundary)
          .result_certified,
      "the explicit unsupported decision is itself freshly certified");

  auto bad_mask = boundary;
  bad_mask.unavailable_upper_axis_mask = 0U;
  check(
      !verify_strictly_padded_dyadic_aabb(boundary_cloud, bad_mask)
           .result_certified,
      "a mutated failure mask is rejected");
  auto bad_payload = boundary;
  bad_payload.certificate = interior.certificate;
  check(
      !verify_strictly_padded_dyadic_aabb(boundary_cloud, bad_payload)
           .result_certified,
      "an unsupported decision cannot carry a clipping-box payload");
}

void test_fresh_verifier_rejects_mutations_and_wrong_cloud() {
  const std::array<CertifiedPoint3, 3> points{
      point(-2.0, 0.0, 4.0),
      point(1.0, -3.0, 0.0),
      point(5.0, 2.0, -1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const auto valid = build_strictly_padded_dyadic_aabb(cloud);
  check_complete_verification(cloud, valid, "the mutation base is valid");
  if (!valid.certificate.has_value()) {
    return;
  }

  auto bad_face = valid;
  bad_face.certificate->omega.lower_binary64_bits[0] =
      bad_face.certificate->exact_site_aabb.bounds.lower_binary64_bits[0];
  check(
      !verify_strictly_padded_dyadic_aabb(cloud, bad_face).result_certified,
      "a nonadjacent, nonstrict box face is rejected");
  auto bad_witness = valid;
  bad_witness.certificate->exact_site_aabb.lower_witness_point_ids[0] =
      PointId{1};
  check(
      !verify_strictly_padded_dyadic_aabb(cloud, bad_witness).result_certified,
      "a mutated extremum witness is rejected");
  auto bad_extremum = valid;
  bad_extremum.certificate->exact_site_aabb.bounds.lower_binary64_bits[0] =
      word(-1.0);
  check(
      !verify_strictly_padded_dyadic_aabb(cloud, bad_extremum)
           .result_certified,
      "a mutated exact source extremum is rejected");
  auto bad_padding = valid;
  bad_padding.certificate->lower_padding[1] = ExactRational{};
  check(
      !verify_strictly_padded_dyadic_aabb(cloud, bad_padding).result_certified,
      "a mutated exact padding is rejected");
  auto bad_claim = valid;
  bad_claim.certificate->convex_hull_strictly_inside_certified = false;
  check(
      !verify_strictly_padded_dyadic_aabb(cloud, bad_claim).result_certified,
      "a cleared convex-hull claim is rejected");
  auto bad_audit = valid;
  ++bad_audit.audit.exact_coordinate_evaluation_count;
  check(
      !verify_strictly_padded_dyadic_aabb(cloud, bad_audit).result_certified,
      "a mutated work counter is rejected");
  auto bad_decision = valid;
  bad_decision.decision =
      StrictDyadicPaddingDecision::unsupported_finite_binary64_range;
  check(
      !verify_strictly_padded_dyadic_aabb(cloud, bad_decision)
           .result_certified,
      "a mutated complete decision is rejected");

  const std::array<CertifiedPoint3, 3> other_points{
      point(-2.0, 0.0, 4.0),
      point(1.0, -3.0, 0.0),
      point(6.0, 2.0, -1.0)};
  const CanonicalPointCloud other_cloud = canonical_cloud(other_points);
  check(
      !verify_strictly_padded_dyadic_aabb(other_cloud, valid)
           .result_certified,
      "a certificate cannot be replayed against a different cloud");

  CanonicalPointCloud moved_from = canonical_cloud(points);
  const CanonicalPointCloud moved_to = std::move(moved_from);
  check(moved_to.size() == points.size(), "the move fixture retains its cloud");
  check_throws<std::invalid_argument>(
      [&moved_from] {
        static_cast<void>(build_strictly_padded_dyadic_aabb(moved_from));
      },
      "a moved-from cloud is rejected before certificate construction");
}

void test_decision_strings() {
  check(
      morsehgp3d::spatial::to_string(StrictDyadicPaddingDecision::complete) ==
              "complete" &&
          morsehgp3d::spatial::to_string(
              StrictDyadicPaddingDecision::unsupported_finite_binary64_range) ==
              "unsupported_finite_binary64_range",
      "strict padding decisions have stable strings");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(morsehgp3d::spatial::to_string(
            static_cast<StrictDyadicPaddingDecision>(255)));
      },
      "an invalid strict padding decision cannot be stringified");
}

}  // namespace

int main() {
  test_binary64_finite_neighbors();
  test_singleton_signed_zero_certificate_and_base_polytope();
  test_extrema_ties_permutation_and_lbvh_factorization();
  test_nontrivial_affine_degeneracies();
  test_inward_extrema_succeed_and_outward_extrema_fail_closed();
  test_fresh_verifier_rejects_mutations_and_wrong_cloud();
  test_decision_strings();

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "all strict point-cloud AABB tests passed\n";
  return 0;
}
