#include "morsehgp3d/exact/expansion.hpp"
#include "morsehgp3d/exact/predicates.hpp"

#include <array>
#include <bit>
#include <cfenv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <xmmintrin.h>
#endif

namespace {

using morsehgp3d::exact::CertificationStage;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLabelMoments;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExpansionResult;
using morsehgp3d::exact::FilterState;
using morsehgp3d::exact::PredicateCounters;
using morsehgp3d::exact::PredicateDecision;
using morsehgp3d::exact::PredicateSign;
using morsehgp3d::exact::compare_squared_distances;
using morsehgp3d::exact::decide_orientation_3d;
using morsehgp3d::exact::decide_squared_distance_order;
using morsehgp3d::exact::evaluate_power_bisector;
using morsehgp3d::exact::expansion_orientation_3d;
using morsehgp3d::exact::expansion_power_bisector_side;
using morsehgp3d::exact::expansion_squared_distance_order;
using morsehgp3d::exact::orientation_3d_determinant;
using morsehgp3d::exact::orientation_3d;
using morsehgp3d::exact::predicate_sign;
using morsehgp3d::exact::squared_distance;
using morsehgp3d::exact::detail::ErrorFreePair;
using morsehgp3d::exact::detail::FloatingExpansion;
using morsehgp3d::exact::detail::Fp64EnvironmentGuard;
using morsehgp3d::exact::detail::add_expansions;
using morsehgp3d::exact::detail::difference_expansion;
using morsehgp3d::exact::detail::expansion_arithmetic_exceptions_clear;
using morsehgp3d::exact::detail::grow_expansion;
using morsehgp3d::exact::detail::multiply_expansions;
using morsehgp3d::exact::detail::negate_expansion;
using morsehgp3d::exact::detail::subtract_expansions;
using morsehgp3d::exact::detail::two_product;
using morsehgp3d::exact::detail::two_sum;

int failures = 0;

void check(bool condition, std::string_view message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactLabelMoments label(
    const std::vector<CertifiedPoint3>& points) {
  std::vector<std::uint32_t> identifiers;
  identifiers.reserve(points.size());
  for (std::size_t index = 0; index < points.size(); ++index) {
    identifiers.push_back(static_cast<std::uint32_t>(index));
  }
  return ExactLabelMoments::from_canonical_ids(identifiers, points);
}

[[nodiscard]] ExactRational exact(double value) {
  return ExactRational::from_binary64(value);
}

[[nodiscard]] ExactRational exact_value(const ErrorFreePair& value) {
  return exact(value.rounded) + exact(value.error);
}

[[nodiscard]] ExactRational exact_value(const FloatingExpansion& value) {
  ExactRational result;
  for (const double component : value.components()) {
    result = result + exact(component);
  }
  return result;
}

[[nodiscard]] PredicateSign exact_sign(const ExactRational& value) noexcept {
  return predicate_sign(value.sign());
}

void check_expansion_value(
    const FloatingExpansion& expansion,
    const ExactRational& expected,
    std::string_view message) {
  check(expansion.valid(), message);
  if (!expansion.valid()) {
    return;
  }
  check(exact_value(expansion) == expected, message);
  check(expansion.sign() == exact_sign(expected), message);
  if (expansion.components().size() > 1U) {
    for (const double component : expansion.components()) {
      check(component != 0.0, "a nontrivial expansion eliminates zero components");
    }
  }
}

void check_certified_expansion_sign(
    const ExpansionResult& result,
    const ExactRational& expected,
    std::string_view message) {
  check(result.state() == FilterState::certified, message);
  check(result.sign().has_value(), message);
  if (result.sign().has_value()) {
    check(*result.sign() == exact_sign(expected), message);
  }
}

void check_uncertain(const ExpansionResult& result, std::string_view message) {
  check(result.state() == FilterState::uncertain, message);
  check(!result.sign().has_value(), message);
}

void check_single_certification(
    const PredicateCounters& counters,
    CertificationStage expected_stage,
    std::uint64_t expected_zeros,
    std::string_view message) {
  const std::uint64_t expected_fp64 =
      expected_stage == CertificationStage::fp64_filtered ? 1U : 0U;
  const std::uint64_t expected_expansion =
      expected_stage == CertificationStage::expansion ? 1U : 0U;
  const std::uint64_t expected_multiprecision =
      expected_stage == CertificationStage::cpu_multiprecision ? 1U : 0U;
  check(
      counters.certified_decisions() == 1U &&
          counters.fp64_filtered_certified() == expected_fp64 &&
          counters.expansion_certified() == expected_expansion &&
          counters.cpu_multiprecision_certified() == expected_multiprecision &&
          counters.exact_zeros() == expected_zeros &&
          counters.remaining_unknown() == 0U,
      message);
}

class SplitMix64 {
 public:
  explicit SplitMix64(std::uint64_t seed) noexcept : state_(seed) {}

  [[nodiscard]] std::uint64_t next() noexcept {
    state_ += UINT64_C(0x9e3779b97f4a7c15);
    std::uint64_t value = state_;
    value = (value ^ (value >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27U)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31U);
  }

  [[nodiscard]] double moderate_dyadic() noexcept {
    constexpr std::uint64_t coefficient_range = UINT64_C(2000001);
    const std::int64_t coefficient =
        static_cast<std::int64_t>(next() % coefficient_range) - INT64_C(1000000);
    const int exponent = static_cast<int>(next() % UINT64_C(25)) - 12;
    return std::ldexp(static_cast<double>(coefficient), exponent);
  }

  [[nodiscard]] double coordinate() noexcept {
    constexpr std::uint64_t coefficient_range = UINT64_C(2049);
    const std::int64_t coefficient =
        static_cast<std::int64_t>(next() % coefficient_range) - INT64_C(1024);
    const int exponent = static_cast<int>(next() % UINT64_C(17)) - 8;
    return std::ldexp(static_cast<double>(coefficient), exponent);
  }

 private:
  std::uint64_t state_;
};

class ScopedFloatingEnvironment {
 public:
  ScopedFloatingEnvironment() noexcept : saved_(std::fegetenv(&environment_) == 0) {}

  ScopedFloatingEnvironment(const ScopedFloatingEnvironment&) = delete;
  ScopedFloatingEnvironment& operator=(const ScopedFloatingEnvironment&) = delete;

  ~ScopedFloatingEnvironment() {
    if (saved_) {
      static_cast<void>(std::fesetenv(&environment_));
    }
  }

  [[nodiscard]] bool saved() const noexcept { return saved_; }

 private:
  std::fenv_t environment_{};
  bool saved_{false};
};

void test_deterministic_error_free_transforms() {
  Fp64EnvironmentGuard environment;
  check(environment.supported(), "deterministic EFT tests have a strict FP64 environment");
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return;
  }

  const double half_ulp = std::ldexp(1.0, -53);
  const ErrorFreePair inexact_sum = two_sum(1.0, half_ulp);
  check(inexact_sum.valid, "two_sum accepts a finite halfway addition");
  check(
      exact_value(inexact_sum) == exact(1.0) + exact(half_ulp),
      "two_sum reconstructs the exact halfway addition");

  const ErrorFreePair cancellation = two_sum(1.0, -1.0);
  check(cancellation.valid, "two_sum accepts exact cancellation");
  check(
      exact_value(cancellation).is_zero() && cancellation.rounded == 0.0 &&
          cancellation.error == 0.0,
      "two_sum canonicalizes exact cancellation to positive zero terms");
  check(
      !std::signbit(cancellation.rounded) && !std::signbit(cancellation.error),
      "two_sum does not retain a signed zero after cancellation");

  const double factor = std::ldexp(1.0, -27);
  const double left = 1.0 + factor;
  const double right = 1.0 - factor;
  const ErrorFreePair inexact_product = two_product(left, right);
  check(inexact_product.valid, "two_product accepts a finite inexact product");
  check(
      exact_value(inexact_product) == exact(left) * exact(right),
      "two_product reconstructs its exact binary64 product");

  const ErrorFreePair exact_product = two_product(1.5, 2.5);
  check(exact_product.valid, "two_product accepts an exactly representable product");
  check(
      exact_value(exact_product) == ExactRational::from_binary64(3.75) &&
          exact_product.error == 0.0 && !std::signbit(exact_product.error),
      "two_product canonicalizes a zero residual");

  check(
      expansion_arithmetic_exceptions_clear(),
      "moderate deterministic EFTs raise no rejected FP exception");
  check(environment.restore(), "deterministic EFT tests restore the FP environment");
}

void test_deterministic_expansion_arithmetic() {
  Fp64EnvironmentGuard environment;
  check(
      environment.supported(),
      "deterministic expansion tests have a strict FP64 environment");
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return;
  }

  const double large = std::ldexp(1.0, 53);
  FloatingExpansion retained_unit = FloatingExpansion::scalar(large);
  retained_unit = grow_expansion(retained_unit, 1.0);
  retained_unit = grow_expansion(retained_unit, -large);
  check_expansion_value(
      retained_unit,
      ExactRational{morsehgp3d::exact::BigInt{1}},
      "grow_expansion retains a unit hidden by rounded cancellation");

  const FloatingExpansion negated = negate_expansion(retained_unit);
  check_expansion_value(
      negated,
      ExactRational{morsehgp3d::exact::BigInt{-1}},
      "negating an expansion preserves its exact value");

  const FloatingExpansion zero = subtract_expansions(retained_unit, retained_unit);
  check_expansion_value(
      zero, ExactRational{}, "subtracting an expansion from itself yields exact zero");
  check(
      zero.components().size() == 1U && zero.components().front() == 0.0 &&
          !std::signbit(zero.components().front()),
      "an exact zero expansion has one canonical positive-zero component");

  const FloatingExpansion left = add_expansions(
      difference_expansion(1.0, std::ldexp(1.0, -52)),
      FloatingExpansion::scalar(3.0));
  const FloatingExpansion right = difference_expansion(-2.0, std::ldexp(1.0, -50));
  const ExactRational exact_left =
      exact(1.0) - exact(std::ldexp(1.0, -52)) + exact(3.0);
  const ExactRational exact_right = exact(-2.0) - exact(std::ldexp(1.0, -50));
  check_expansion_value(left, exact_left, "expansion addition has its exact rational value");
  check_expansion_value(
      right, exact_right, "difference_expansion has its exact rational value");
  check_expansion_value(
      multiply_expansions(left, right),
      exact_left * exact_right,
      "expansion multiplication has its exact rational value");
  check_expansion_value(
      subtract_expansions(left, right),
      exact_left - exact_right,
      "expansion subtraction has its exact rational value");

  check(
      !FloatingExpansion::scalar(std::numeric_limits<double>::infinity()).valid(),
      "a non-finite scalar cannot enter an expansion");
  check(
      expansion_arithmetic_exceptions_clear(),
      "moderate deterministic expansion arithmetic raises no rejected exception");
  check(environment.restore(), "deterministic expansion tests restore the FP environment");
}

void test_pseudorandom_efts_and_expansions() {
  Fp64EnvironmentGuard environment;
  check(environment.supported(), "pseudo-random EFT tests have a strict FP64 environment");
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return;
  }

  SplitMix64 generator{UINT64_C(0x6d6f727365326138)};
  constexpr std::size_t sample_count = 4096U;
  for (std::size_t sample = 0; sample < sample_count; ++sample) {
    const double a = generator.moderate_dyadic();
    const double b = generator.moderate_dyadic();
    const double c = generator.moderate_dyadic();
    const double d = generator.moderate_dyadic();
    const double e = generator.moderate_dyadic();

    const ErrorFreePair sum = two_sum(a, b);
    check(sum.valid, "pseudo-random two_sum remains valid in the moderate domain");
    if (sum.valid) {
      check(
          exact_value(sum) == exact(a) + exact(b),
          "pseudo-random two_sum equals its ExactRational oracle");
    }

    const ErrorFreePair product = two_product(a, b);
    check(product.valid, "pseudo-random two_product remains valid in the moderate domain");
    if (product.valid) {
      check(
          exact_value(product) == exact(a) * exact(b),
          "pseudo-random two_product equals its ExactRational oracle");
    }

    FloatingExpansion left = FloatingExpansion::scalar(a);
    left = grow_expansion(left, b);
    left = grow_expansion(left, c);
    FloatingExpansion right = FloatingExpansion::scalar(d);
    right = grow_expansion(right, e);
    const ExactRational exact_left = exact(a) + exact(b) + exact(c);
    const ExactRational exact_right = exact(d) + exact(e);
    check_expansion_value(
        left, exact_left, "pseudo-random grown expansion equals its rational oracle");
    check_expansion_value(
        add_expansions(left, right),
        exact_left + exact_right,
        "pseudo-random expansion sum equals its rational oracle");
    check_expansion_value(
        subtract_expansions(left, right),
        exact_left - exact_right,
        "pseudo-random expansion difference equals its rational oracle");
    check_expansion_value(
        multiply_expansions(left, right),
        exact_left * exact_right,
        "pseudo-random expansion product equals its rational oracle");
  }
  check(
      expansion_arithmetic_exceptions_clear(),
      "pseudo-random moderate arithmetic raises no rejected FP exception");
  check(environment.restore(), "pseudo-random EFT tests restore the FP environment");
}

void test_public_expansion_predicates_against_rationals() {
  const CertifiedPoint3 origin = point(0.0, 0.0, 0.0);
  check_certified_expansion_sign(
      expansion_squared_distance_order(
          origin, point(-1.0, 0.0, 0.0), point(1.0, 0.0, 0.0)),
      ExactRational{},
      "symmetric squared distances certify exact zero by expansion");
  check_certified_expansion_sign(
      expansion_orientation_3d(
          origin,
          point(1.0, 0.0, 0.0),
          point(0.0, 1.0, 0.0),
          point(1.0, 1.0, 0.0)),
      ExactRational{},
      "coplanar points certify exact zero by expansion");

  SplitMix64 generator{UINT64_C(0x657870616e73696f)};
  constexpr std::size_t sample_count = 2048U;
  for (std::size_t sample = 0; sample < sample_count; ++sample) {
    const CertifiedPoint3 witness =
        point(generator.coordinate(), generator.coordinate(), generator.coordinate());
    const CertifiedPoint3 left =
        point(generator.coordinate(), generator.coordinate(), generator.coordinate());
    const CertifiedPoint3 right = sample % 31U == 0U
                                      ? left
                                      : point(
                                            generator.coordinate(),
                                            generator.coordinate(),
                                            generator.coordinate());
    const ExactRational distance_difference =
        squared_distance(witness, left) - squared_distance(witness, right);
    check_certified_expansion_sign(
        expansion_squared_distance_order(witness, left, right),
        distance_difference,
        "pseudo-random distance expansion agrees with ExactRational");

    const CertifiedPoint3 a =
        point(generator.coordinate(), generator.coordinate(), generator.coordinate());
    const CertifiedPoint3 b =
        point(generator.coordinate(), generator.coordinate(), generator.coordinate());
    const CertifiedPoint3 c =
        point(generator.coordinate(), generator.coordinate(), generator.coordinate());
    const CertifiedPoint3 d = sample % 29U == 0U
                                  ? b
                                  : point(
                                        generator.coordinate(),
                                        generator.coordinate(),
                                        generator.coordinate());
    const ExactRational determinant = orientation_3d_determinant(a, b, c, d);
    check_certified_expansion_sign(
        expansion_orientation_3d(a, b, c, d),
        determinant,
        "pseudo-random orientation expansion agrees with ExactRational");
  }

  constexpr std::size_t power_sample_count = 1024U;
  for (std::size_t sample = 0; sample < power_sample_count; ++sample) {
    const std::size_t cardinality =
        1U + sample % morsehgp3d::exact::maximum_power_label_cardinality;
    std::vector<CertifiedPoint3> r_points;
    std::vector<CertifiedPoint3> q_points;
    r_points.reserve(cardinality);
    q_points.reserve(cardinality);
    for (std::size_t index = 0; index < cardinality; ++index) {
      r_points.push_back(
          point(generator.coordinate(), generator.coordinate(), generator.coordinate()));
      q_points.push_back(
          point(generator.coordinate(), generator.coordinate(), generator.coordinate()));
    }
    const ExactLabelMoments r = label(r_points);
    const ExactLabelMoments q = sample % 31U == 0U ? r : label(q_points);
    const CertifiedPoint3 witness =
        point(generator.coordinate(), generator.coordinate(), generator.coordinate());
    const ExactRational expected =
        evaluate_power_bisector(witness.exact(), r, q);
    check_certified_expansion_sign(
        expansion_power_bisector_side(witness, r, q),
        expected,
        "pseudo-random power expansion agrees with ExactRational");
  }
}

void test_public_underflow_and_overflow_fail_closed() {
  const double minimum_subnormal = std::numeric_limits<double>::denorm_min();
  const double maximum = std::numeric_limits<double>::max();
  const CertifiedPoint3 origin = point(0.0, 0.0, 0.0);

  const ExpansionResult distance_underflow = expansion_squared_distance_order(
      origin, point(minimum_subnormal, 0.0, 0.0), origin);
  check_uncertain(
      distance_underflow,
      "a squared minimum-subnormal displacement fails closed at expansion stage");
  PredicateCounters distance_underflow_counters;
  const PredicateDecision distance_underflow_decision =
      decide_squared_distance_order(
          origin,
          point(minimum_subnormal, 0.0, 0.0),
          origin,
          &distance_underflow_counters);
  check(
      distance_underflow_decision.sign() == PredicateSign::positive &&
          distance_underflow_decision.certification_stage() ==
              CertificationStage::cpu_multiprecision,
      "distance underflow falls back to its positive exact sign");
  check_single_certification(
      distance_underflow_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "distance underflow records exactly one terminal certification");

  const ExpansionResult distance_overflow = expansion_squared_distance_order(
      point(maximum, 0.0, 0.0),
      point(-maximum, 0.0, 0.0),
      point(maximum, 0.0, 0.0));
  check_uncertain(distance_overflow, "an overflowing distance difference fails closed");
  PredicateCounters distance_overflow_counters;
  const PredicateDecision distance_overflow_decision =
      decide_squared_distance_order(
          point(maximum, 0.0, 0.0),
          point(-maximum, 0.0, 0.0),
          point(maximum, 0.0, 0.0),
          &distance_overflow_counters);
  check(
      distance_overflow_decision.sign() == PredicateSign::positive &&
          distance_overflow_decision.certification_stage() ==
              CertificationStage::cpu_multiprecision,
      "distance overflow falls back to its positive exact sign");
  check_single_certification(
      distance_overflow_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "distance overflow records exactly one terminal certification");

  const ExpansionResult orientation_underflow = expansion_orientation_3d(
      origin,
      point(minimum_subnormal, 0.0, 0.0),
      point(0.0, minimum_subnormal, 0.0),
      point(0.0, 0.0, minimum_subnormal));
  check_uncertain(
      orientation_underflow,
      "a determinant below binary64 subnormal range fails closed");
  PredicateCounters orientation_underflow_counters;
  const PredicateDecision orientation_underflow_decision = decide_orientation_3d(
      origin,
      point(minimum_subnormal, 0.0, 0.0),
      point(0.0, minimum_subnormal, 0.0),
      point(0.0, 0.0, minimum_subnormal),
      &orientation_underflow_counters);
  check(
      orientation_underflow_decision.sign() == PredicateSign::positive &&
          orientation_underflow_decision.certification_stage() ==
              CertificationStage::cpu_multiprecision,
      "orientation underflow falls back to its positive exact sign");
  check_single_certification(
      orientation_underflow_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "orientation underflow records exactly one terminal certification");

  const ExpansionResult orientation_overflow = expansion_orientation_3d(
      origin,
      point(maximum, 0.0, 0.0),
      point(0.0, maximum, 0.0),
      point(0.0, 0.0, maximum));
  check_uncertain(orientation_overflow, "an overflowing determinant fails closed");
  PredicateCounters orientation_overflow_counters;
  const PredicateDecision orientation_overflow_decision = decide_orientation_3d(
      origin,
      point(maximum, 0.0, 0.0),
      point(0.0, maximum, 0.0),
      point(0.0, 0.0, maximum),
      &orientation_overflow_counters);
  check(
      orientation_overflow_decision.sign() == PredicateSign::positive &&
          orientation_overflow_decision.certification_stage() ==
              CertificationStage::cpu_multiprecision,
      "orientation overflow falls back to its positive exact sign");
  check_single_certification(
      orientation_overflow_counters,
      CertificationStage::cpu_multiprecision,
      0U,
      "orientation overflow records exactly one terminal certification");

  check_uncertain(
      expansion_power_bisector_side(
          origin,
          label({point(minimum_subnormal, 0.0, 0.0)}),
          label({origin})),
      "a minimum-subnormal squared power cost fails closed");
  check_uncertain(
      expansion_power_bisector_side(
          point(maximum, 0.0, 0.0),
          label({point(-maximum, 0.0, 0.0)}),
          label({point(maximum, 0.0, 0.0)})),
      "an overflowing power difference fails closed");
}

void test_unique_expansion_certification_counters() {
  PredicateCounters distance_counters;
  const PredicateDecision distance = decide_squared_distance_order(
      point(0.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      &distance_counters);
  check(
      distance.sign() == PredicateSign::zero &&
          distance.certification_stage() == CertificationStage::expansion,
      "distance cancellation terminates at expansion stage");
  check_single_certification(
      distance_counters,
      CertificationStage::expansion,
      1U,
      "distance cancellation records one expansion certification");

  const double one_ulp_below_one = std::nextafter(1.0, 0.0);
  PredicateCounters strict_distance_counters;
  const auto strict_distance = compare_squared_distances(
      point(0.0, 0.0, 0.0),
      point(one_ulp_below_one, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      &strict_distance_counters);
  check(
      strict_distance.decision.sign() == PredicateSign::negative &&
          strict_distance.decision.certification_stage() ==
              CertificationStage::expansion &&
          strict_distance.left_squared_distance <
              strict_distance.right_squared_distance,
      "a strict one-ULP distance order and its rich witness agree at expansion stage");
  check_single_certification(
      strict_distance_counters,
      CertificationStage::expansion,
      0U,
      "a rich nonzero distance result records one expansion certification");

  PredicateCounters orientation_counters;
  const PredicateDecision orientation = decide_orientation_3d(
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(1.0, 1.0, 0.0),
      &orientation_counters);
  check(
      orientation.sign() == PredicateSign::zero &&
          orientation.certification_stage() == CertificationStage::expansion,
      "orientation cancellation terminates at expansion stage");
  check_single_certification(
      orientation_counters,
      CertificationStage::expansion,
      1U,
      "orientation cancellation records one expansion certification");

  PredicateCounters rich_orientation_counters;
  const auto rich_orientation = orientation_3d(
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(1.0, 1.0, 0.0),
      &rich_orientation_counters);
  check(
      rich_orientation.decision.sign() == PredicateSign::zero &&
          rich_orientation.decision.certification_stage() ==
              CertificationStage::expansion &&
          rich_orientation.determinant.is_zero(),
      "a rich orientation result keeps its exact zero witness at expansion stage");
  check_single_certification(
      rich_orientation_counters,
      CertificationStage::expansion,
      1U,
      "a rich orientation result records one expansion certification");
}

void test_public_api_preserves_fenv() {
  ScopedFloatingEnvironment restore_at_exit;
  check(restore_at_exit.saved(), "the FENV preservation test saves the caller environment");
  if (!restore_at_exit.saved()) {
    return;
  }

  const std::array<int, 4> rounding_modes{
      FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO};
  const ExactLabelMoments power_r = label({point(2.0, 0.0, 0.0)});
  const ExactLabelMoments power_q = label({point(1.0, 0.0, 0.0)});
  for (const int rounding_mode : rounding_modes) {
    if (std::fesetround(rounding_mode) != 0 ||
        std::feclearexcept(FE_ALL_EXCEPT) != 0 ||
        std::feraiseexcept(FE_INVALID | FE_DIVBYZERO) != 0) {
      check(false, "the platform accepts the requested FENV test setup");
      continue;
    }
    const int flags_before = std::fetestexcept(FE_ALL_EXCEPT);
    const ExpansionResult distance = expansion_squared_distance_order(
        point(0.0, 0.0, 0.0),
        point(-1.0, 0.0, 0.0),
        point(1.0, 0.0, 0.0));
    check(
        std::fegetround() == rounding_mode &&
            std::fetestexcept(FE_ALL_EXCEPT) == flags_before,
        "distance expansion preserves rounding mode and exception flags");
    const ExpansionResult orientation = expansion_orientation_3d(
        point(0.0, 0.0, 0.0),
        point(1.0, 0.0, 0.0),
        point(0.0, 1.0, 0.0),
        point(1.0, 1.0, 0.0));
    check(
        std::fegetround() == rounding_mode &&
            std::fetestexcept(FE_ALL_EXCEPT) == flags_before,
        "orientation expansion preserves rounding mode and exception flags");
    const ExpansionResult power = expansion_power_bisector_side(
        point(1.5, 0.0, 0.0), power_r, power_q);
    check(
        std::fegetround() == rounding_mode &&
            std::fetestexcept(FE_ALL_EXCEPT) == flags_before,
        "power expansion preserves rounding mode and exception flags");
    if (rounding_mode == FE_TONEAREST) {
      check_certified_expansion_sign(
          distance, ExactRational{}, "round-to-nearest enables distance expansion");
      check_certified_expansion_sign(
          orientation,
          ExactRational{},
          "round-to-nearest enables orientation expansion");
      check_certified_expansion_sign(
          power, ExactRational{}, "round-to-nearest enables power expansion");
    } else {
      check_uncertain(distance, "a non-nearest mode disables distance expansion");
      check_uncertain(orientation, "a non-nearest mode disables orientation expansion");
      check_uncertain(power, "a non-nearest mode disables power expansion");
    }
  }

  check(std::fesetround(FE_TONEAREST) == 0, "underflow FENV test selects round-to-nearest");
  check(std::feclearexcept(FE_ALL_EXCEPT) == 0, "underflow FENV test clears flags");
  check(std::feraiseexcept(FE_INVALID) == 0, "underflow FENV test raises a caller flag");
  const int flags_before_underflow = std::fetestexcept(FE_ALL_EXCEPT);
  check_uncertain(
      expansion_squared_distance_order(
          point(0.0, 0.0, 0.0),
          point(std::numeric_limits<double>::denorm_min(), 0.0, 0.0),
          point(0.0, 0.0, 0.0)),
      "an underflowing public expansion remains uncertain with caller flags set");
  check(
      std::fegetround() == FE_TONEAREST &&
          std::fetestexcept(FE_ALL_EXCEPT) == flags_before_underflow,
      "an internal underflow does not leak into the caller FENV");

#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
  constexpr unsigned int flush_to_zero_mask = 1U << 15U;
  constexpr unsigned int denormals_are_zero_mask = 1U << 6U;
  constexpr unsigned int rounding_control_mask = 3U << 13U;
  constexpr unsigned int exception_status_mask = 0x3fU;
  constexpr unsigned int preserved_mask = flush_to_zero_mask |
                                           denormals_are_zero_mask |
                                           rounding_control_mask |
                                           exception_status_mask;
  const unsigned int original_mxcsr = _mm_getcsr();
  const std::array<unsigned int, 3> altered_modes{
      (original_mxcsr | denormals_are_zero_mask) & ~flush_to_zero_mask,
      (original_mxcsr | flush_to_zero_mask) & ~denormals_are_zero_mask,
      original_mxcsr | flush_to_zero_mask | denormals_are_zero_mask};
  for (const unsigned int altered_mode : altered_modes) {
    _mm_setcsr(altered_mode);
    const ExpansionResult distance = expansion_squared_distance_order(
        point(0.0, 0.0, 0.0),
        point(-1.0, 0.0, 0.0),
        point(1.0, 0.0, 0.0));
    const ExpansionResult orientation = expansion_orientation_3d(
        point(0.0, 0.0, 0.0),
        point(1.0, 0.0, 0.0),
        point(0.0, 1.0, 0.0),
        point(1.0, 1.0, 0.0));
    const ExpansionResult power = expansion_power_bisector_side(
        point(1.5, 0.0, 0.0), power_r, power_q);
    check_uncertain(distance, "FTZ or DAZ disables the distance expansion");
    check_uncertain(orientation, "FTZ or DAZ disables the orientation expansion");
    check_uncertain(power, "FTZ or DAZ disables the power expansion");
    check(
        (_mm_getcsr() & preserved_mask) == (altered_mode & preserved_mask),
        "public expansions preserve MXCSR FTZ, DAZ, rounding, and status bits");
  }
  _mm_setcsr(original_mxcsr);
#endif
}

}  // namespace

int main() {
  test_deterministic_error_free_transforms();
  test_deterministic_expansion_arithmetic();
  test_pseudorandom_efts_and_expansions();
  test_public_expansion_predicates_against_rationals();
  test_public_underflow_and_overflow_fail_closed();
  test_unique_expansion_certification_counters();
  test_public_api_preserves_fenv();

  if (failures != 0) {
    std::cerr << failures << " expansion test(s) failed\n";
    return 1;
  }
  std::cout << "MorseHGP3D expansion tests passed\n";
  return 0;
}
