#include "lanes/q34_collective.hpp"

#include "lanes/family_certificate.hpp"

#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace mhgp8 {
namespace {

i64 distance_squared(Point3 a, Point3 b) noexcept {
  i64 result = 0;
  for (std::size_t axis = 0; axis != 3; ++axis) {
    const i64 delta = static_cast<i64>(a[axis]) - b[axis];
    result += delta * delta;
  }
  return result;  // <=3*65535^2<2^34.
}

auto edge_key(std::size_t a, std::size_t b) noexcept {
  return std::pair{std::min(a, b), std::max(a, b)};
}

struct RootBound { i64 ceiling; u64 iterations; };

RootBound ceil_sqrt(i128 target) {
  if (target <= 0) throw std::logic_error("mhgp8 variance root requires a positive target");
  // Here target=q*S<=54M^6<2^102. All squared search bounds fit i128.
  const auto high_word = static_cast<u64>(target >> 64);
  const unsigned bits = high_word != 0
      ? 64U + static_cast<unsigned>(std::bit_width(high_word))
      : static_cast<unsigned>(std::bit_width(static_cast<u64>(target)));
  i64 low = 0, high = i64{1} << ((bits + 1U) / 2U);
  u64 iterations = 0;
  while (high - low > 1) {
    const auto middle = low + (high - low) / 2;
    if (static_cast<i128>(middle) * middle >= target) high = middle;
    else low = middle;
    ++iterations;
  }
  const i128 previous = static_cast<i128>(high) - 1;
  if (static_cast<i128>(high) * high < target || previous * previous >= target)
    throw std::logic_error("mhgp8 variance integer-root enclosure failed");
  return {high, iterations};
}

void finish_status(Q34PoolAssessment& assessment, std::size_t kmax) {
  auto& work = assessment.work;
  if (assessment.q3_rejected) counter_add(work.q3_rejected);
  if (assessment.q4_rejected) counter_add(work.q4_rejected);
  const bool q3_survives = !assessment.q3_rejected;
  const bool q4_survives = kmax >= 3 && !assessment.q4_rejected;
  if (!q3_survives && !q4_survives) counter_add(work.both_rejected);
  else if (q3_survives && q4_survives) counter_add(work.both_survivors);
  else if (q3_survives) counter_add(work.q3_only_survivors);
  else counter_add(work.q4_only_survivors);
}

}  // namespace

std::size_t Q34PoolWorkspace::retained_bytes() const {
  if (events_.capacity() > std::numeric_limits<std::size_t>::max() / sizeof(std::size_t))
    throw std::overflow_error("mhgp8 collective workspace storage exceeds size_t");
  return events_.capacity() * sizeof(std::size_t);
}

Q34PoolAssessment assess_q34_family_pool(Q34WitnessPoolPtr pool, std::size_t x_id,
    std::size_t kmax, Q34PoolOptions options, Q34PoolWorkspace& workspace) {
  if (!pool || kmax == 0 ||
      (options.chord != Q34ChordBound::Jung && options.chord != Q34ChordBound::Variance) ||
      (options.reduction != Q34PoolReduction::Universal &&
       options.reduction != Q34PoolReduction::Collective))
    throw std::invalid_argument("mhgp8 collective pool requires pool, positive K and valid options");
  const auto points = pool->cover()->index()->cloud().points();
  if (x_id >= points.size()) throw std::out_of_range("mhgp8 collective seed ID outside cloud");
  const auto edge = pool->cover()->edge_ids();
  const auto a = points[edge[0]], b = points[edge[1]], x = points[x_id];
  const i64 d = distance_squared(a, b), e = distance_squared(a, x);
  const i64 other = distance_squared(b, x);
  if (d + e <= other || d + other <= e || e + other <= d)
    throw std::invalid_argument("mhgp8 collective seed must be strictly acute");

  workspace.events_.clear();
  Q34PoolAssessment result{};
  auto& work = result.work;
  work.peak_event_bytes = static_cast<u64>(workspace.retained_bytes());
  const auto proposed_owner = edge_key(edge[0], edge[1]);
  const std::array<std::pair<i64, std::pair<std::size_t, std::size_t>>, 2> alternatives{{
      {e, edge_key(edge[0], x_id)}, {other, edge_key(edge[1], x_id)}}};
  for (const auto& [length, key] : alternatives) {
    counter_add(work.seed_owner_tests);
    if (length > d || (length == d && key < proposed_owner)) {
      counter_add(work.seed_owner_rejections);
      return result;
    }
  }
  if (kmax < 2 || pool->ids().empty()) return result;

  counter_add(work.seed_queries);
  const auto certificate = Q34FamilyCertificate::make(a, b, x);
  if (!certificate) throw std::logic_error("mhgp8 owned acute collective seed lacks certificate");
  counter_add(work.certificate_builds);
  counter_add(work.sqrt_iterations, certificate->sqrt_iterations());
  const auto& family = certificate->family();
  result.jung_parameter_bound = certificate->radius_parameter_bound();
  result.parameter_bound = result.jung_parameter_bound;
  if (options.chord == Q34ChordBound::Variance) {
    counter_add(work.variance_bounds);
    const i128 gram = family.gram();
    const i128 product = static_cast<i128>(e) * other;
    const i128 s = 2 * gram - product, t = 4 * gram - product;
    if (s <= 0 || t <= s)
      throw std::logic_error("mhgp8 maximal acute seed has invalid variance chord");
    // Audit 4215dd16: mu^2<=D*S^2/T, but DO NOT form D*S*S in i128.
    // 0<S<T and D integral imply q=ceil(D*S/T)<=D. G<=9M^4 gives
    // S<=18M^4, hence D*S and q*S<=54M^6<2^102. Quotient/remainder
    // ceiling avoids an extra numerator+denominator intermediate.
    const i128 numerator = static_cast<i128>(d) * s;
    const i128 quotient = numerator / t + (numerator % t != 0 ? 1 : 0);
    const auto bound = ceil_sqrt(quotient * s);
    counter_add(work.variance_sqrt_iterations, bound.iterations);
    result.parameter_bound = std::min(result.parameter_bound, bound.ceiling);
  }

  const auto ids = pool->ids();
  const bool q4_available = kmax >= 3;
  const bool collective = options.reduction == Q34PoolReduction::Collective && q4_available;
  std::size_t credit3 = 0, credit4 = 0, population = 0;
  for (const auto id : ids) {
    counter_add(work.proposed_sites);
    counter_add(work.paired_predicate_tests);
    // One shared P/B evaluation per proposed site in this initial scan.
    // P and U*B each fit i128; |P|+U*|B|<2^104, as for the old certificate.
    const i128 power = family.power(points[id]);
    const i64 side = family.side(points[id]);
    const i128 scaled_side = static_cast<i128>(result.parameter_bound) * side;
    const i128 absolute_scaled = scaled_side < 0 ? -scaled_side : scaled_side;
    if (!result.q3_rejected && power < 0) {
      ++credit3;
      counter_add(work.q3_credits);
      if (credit3 == kmax - 1) result.q3_rejected = true;
    }
    if (q4_available && !result.q4_rejected && power + absolute_scaled < 0) {
      ++credit4;
      counter_add(work.q4_universal_credits);
      if (credit4 == kmax - 2) {
        result.q4_rejected = true;
        counter_add(work.q4_universal_rejected);
      }
    }

    // If universals already killed q4, no collective minimum will be needed.
    // Continue q3 independently; preceding event preparation remains charged.
    if (collective && !result.q4_rejected) {
      if (side == 0) {
        counter_add(work.constant_tests);
        if (power < 0) ++population;
      } else {
        counter_add(work.endpoint_tests, 2);
        const i128 left = power + scaled_side;   // P-(-U)*B.
        const i128 right = power - scaled_side;  // P-(+U)*B.
        if (side > 0) {
          if (left < 0) ++population;  // Entry root strictly before -U.
          if (left >= 0 && right <= 0) {
            workspace.events_.push_back(id);
            counter_add(work.event_count);
          }
        } else {
          if (left <= 0) ++population;  // Exit root AT -U still active just before it.
          if (left <= 0 && right >= 0) {
            workspace.events_.push_back(id);
            counter_add(work.event_count);
          }
        }
      }
    }
    if (result.q3_rejected && (!q4_available || result.q4_rejected)) break;
  }

  if (collective && !result.q4_rejected) {
    counter_add(work.collective_queries);
    if (population > ids.size()) throw std::logic_error("mhgp8 collective initial population overflow");
    auto& events = workspace.events_;
    std::sort(events.begin(), events.end(), [&](std::size_t left, std::size_t right) {
      counter_add(work.sort_comparisons);
      const int order = family.compare_roots(points[left], points[right]);
      return order < 0 || (order == 0 && left < right);
    });
    std::size_t minimum = population;
    for (std::size_t first = 0; first < events.size();) {
      std::size_t last = first + 1;
      while (last < events.size()) {
        counter_add(work.group_comparisons);
        if (family.compare_roots(points[events[first]], points[events[last]]) != 0) break;
        ++last;
      }
      std::size_t entries = 0, exits = 0;
      for (auto position = first; position < last; ++position) {
        counter_add(work.event_side_tests);
        const auto side = family.side(points[events[position]]);
        if (side == 0) throw std::logic_error("mhgp8 collective event has zero side");
        if (side > 0) ++entries;
        else ++exits;
      }
      if (exits > population) throw std::logic_error("mhgp8 collective exit underflow");
      minimum = std::min(minimum, population);
      population -= exits;
      if (entries > ids.size() - population)
        throw std::logic_error("mhgp8 collective entry overflow");
      population += entries;
      counter_add(work.groups);
      work.max_group = std::max(work.max_group, static_cast<u64>(last - first));
      first = last;
    }
    // Initially we counted just BEFORE -U. Exits at -U are removed in their
    // first group; entries at +U are absent at its strict group observation.
    // Every open interval has population >= an adjacent group (or the left
    // endpoint if there is no group). Thus endpoints plus strict groups give
    // the full closed-chord minimum. NEVER saturate this subtractive sweep,
    // and never stop merely because an early group meets/fails the threshold.
    result.collective_minimum = minimum;
    counter_add(work.collective_minimum_sum, static_cast<u64>(minimum));
    if (minimum >= kmax - 2) {
      result.q4_rejected = true;
      counter_add(work.collective_q4_rejected);
    }
  }
  work.peak_event_bytes = static_cast<u64>(workspace.retained_bytes());
  finish_status(result, kmax);
  return result;
}

}  // namespace mhgp8
