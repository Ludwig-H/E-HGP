// Independent bounded audit prototype. It proposes rank slabs, certifies
// whole tails with the current Wq block predicate, and retains every failed
// certificate. It does not modify the producer, construct a WSPD or execute
// a FULL tower. Cartesian enumeration occurs only in the bounded judges.
#include "spindle/predicates.hpp"

#include <boost/multiprecision/cpp_int.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using mhgp8::Box3;
using mhgp8::Lane;
using mhgp8::Point3;
using mhgp8::u64;
using Big = boost::multiprecision::cpp_int;
using Pair = std::pair<std::size_t, std::size_t>;

struct Failure : std::runtime_error {
  using std::runtime_error::runtime_error;
};

void require(bool condition, const char* cause) {
  if (!condition) throw Failure(cause);
}

struct Range {
  std::size_t begin{};
  std::size_t end{};
  [[nodiscard]] std::size_t size() const { return end - begin; }
};

struct Input {
  std::vector<Point3> points;
  std::size_t m{};
};

struct Work {
  u64 sort_comparisons{};
  u64 prefix_sites{};
  u64 suffix_sites{};
  u64 anchor_box_sites{};
  u64 witness_sites{};
  u64 chunks{};
  u64 attempted_tails{};
  u64 retained_tails{};
  u64 certified_tails{};
  u64 mutation_hits{};
  mhgp8::PredicateWork predicates;
};

struct Task {
  Range a;
  Range b;
  Range witness_ranks;
  bool killed{};
  bool proposed_tail{};
};

struct Plan {
  std::vector<std::size_t> a_order;
  std::vector<std::size_t> b_order;
  std::vector<Task> tasks;
  u64 candidate_pairs{};
  Work work;
};

enum class Mutation { None, ClosedBoundary };

u64 product(std::size_t a, std::size_t b) {
  require(b == 0 || a <= std::numeric_limits<u64>::max() / b,
          "model.pair_count_overflow");
  return static_cast<u64>(a) * static_cast<u64>(b);
}

void extend(Box3& box, const Point3& point) {
  box.low = {std::min(box.low.x, point.x), std::min(box.low.y, point.y),
             std::min(box.low.z, point.z)};
  box.high = {std::max(box.high.x, point.x), std::max(box.high.y, point.y),
              std::max(box.high.z, point.z)};
}

void validate(const Input& input, unsigned separation) {
  require(input.m > 0 && input.m <= input.points.size() &&
          input.points.size() - input.m == input.m, "input.equal_nonempty_factors");
  require(separation >= 8 && separation <= 12, "input.separation_domain");
  std::set<std::tuple<unsigned, unsigned, unsigned>> positions;
  for (const auto& point : input.points)
    require(positions.emplace(point.x, point.y, point.z).second,
            "input.duplicate_position");
  Box3 a{input.points[0], input.points[0]};
  Box3 b{input.points[input.m], input.points[input.m]};
  for (std::size_t i = 0; i < input.m; ++i) {
    extend(a, input.points[i]);
    extend(b, input.points[input.m + i]);
  }
  mhgp8::i64 gap2 = 0;
  mhgp8::i64 a_diagonal2 = 0;
  mhgp8::i64 b_diagonal2 = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto gap = std::max({mhgp8::i64{0},
        static_cast<mhgp8::i64>(a.low[axis]) - b.high[axis],
        static_cast<mhgp8::i64>(b.low[axis]) - a.high[axis]});
    const auto aw = static_cast<mhgp8::i64>(a.high[axis]) - a.low[axis];
    const auto bw = static_cast<mhgp8::i64>(b.high[axis]) - b.low[axis];
    gap2 += gap * gap;
    a_diagonal2 += aw * aw;
    b_diagonal2 += bw * bw;
  }
  require(static_cast<mhgp8::i128>(gap2) >=
          static_cast<mhgp8::i128>(separation) * separation *
              std::max(a_diagonal2, b_diagonal2), "input.actual_separation");
}

Plan propose(const Input& input, Lane lane, unsigned h,
             Mutation mutation = Mutation::None) {
  require(h > 0 && h <= 10, "model.threshold");
  mhgp8::require_valid_lane(lane);
  Plan plan;
  const auto m = input.m;
  const std::size_t w = (h + 1U) / 2U;
  const std::size_t length = w + 1;
  plan.a_order.resize(m);
  plan.b_order.resize(m);
  std::iota(plan.a_order.begin(), plan.a_order.end(), std::size_t{0});
  std::iota(plan.b_order.begin(), plan.b_order.end(), m);
  const auto less = [&](std::size_t left, std::size_t right) {
    mhgp8::counter_add(plan.work.sort_comparisons);
    const auto& a = input.points[left];
    const auto& b = input.points[right];
    return std::tie(a.y, a.x, a.z, left) < std::tie(b.y, b.x, b.z, right);
  };
  std::sort(plan.a_order.begin(), plan.a_order.end(), less);
  std::sort(plan.b_order.begin(), plan.b_order.end(), less);

  // These scans prevent rebuilding each long B tail by visiting all its sites.
  std::vector<Box3> prefixes(m);
  std::vector<Box3> suffixes(m);
  Box3 prefix{input.points[plan.b_order[0]], input.points[plan.b_order[0]]};
  Box3 suffix{input.points[plan.b_order[m - 1]], input.points[plan.b_order[m - 1]]};
  for (std::size_t i = 0; i < m; ++i) {
    extend(prefix, input.points[plan.b_order[i]]);
    prefixes[i] = prefix;
    mhgp8::counter_add(plan.work.prefix_sites);
    const auto j = m - 1 - i;
    extend(suffix, input.points[plan.b_order[j]]);
    suffixes[j] = suffix;
    mhgp8::counter_add(plan.work.suffix_sites);
  }

  for (std::size_t begin = 0; begin < m;) {
    const std::size_t end = begin + std::min(length, m - begin);
    const Range anchors{begin, end};
    Box3 a_box{input.points[plan.a_order[begin]], input.points[plan.a_order[begin]]};
    for (std::size_t i = begin; i < end; ++i) {
      extend(a_box, input.points[plan.a_order[i]]);
      mhgp8::counter_add(plan.work.anchor_box_sites);
    }
    mhgp8::counter_add(plan.work.chunks);
    const std::size_t left_end = begin > w ? begin - w : 0;
    const std::size_t right_begin = end + std::min(w, m - end);
    const auto retain = [&](Range columns) {
      plan.tasks.push_back({anchors, columns, {}, false, false});
      mhgp8::counter_add(plan.candidate_pairs, product(anchors.size(), columns.size()));
    };
    const auto tail = [&](Range columns, Range witnesses, const Box3& b_box) {
      require(witnesses.size() == w, "model.witness_rank_count");
      mhgp8::counter_add(plan.work.attempted_tails);
      Box3 z_box{input.points[plan.a_order[witnesses.begin]],
                 input.points[plan.a_order[witnesses.begin]]};
      // Two disjoint ID ranges, one in each factor. The box is merely their
      // geometric enclosure; its volume never supplies a population count.
      for (std::size_t i = witnesses.begin; i < witnesses.end; ++i) {
        extend(z_box, input.points[plan.a_order[i]]);
        extend(z_box, input.points[plan.b_order[i]]);
        mhgp8::counter_add(plan.work.witness_sites, 2);
      }
      const auto decision = mhgp8::classify_witness_block(
          lane, a_box, b_box, z_box, plan.work.predicates);
      bool certified = decision == mhgp8::BlockDecision::Credit;
      if (mutation == Mutation::ClosedBoundary && lane == Lane::Q2 && !certified &&
          mhgp8::spindle_detail::h_minimum(a_box, b_box, z_box) >= 0) {
        certified = true;
        mhgp8::counter_add(plan.work.mutation_hits);
      }
      const bool killed = certified && product(2, witnesses.size()) >= h;
      plan.tasks.push_back({anchors, columns, witnesses, killed, true});
      if (killed) {
        mhgp8::counter_add(plan.work.certified_tails);
      } else {
        // NoCredit refutes this proposed witness population, not its anchors.
        mhgp8::counter_add(plan.work.retained_tails);
        mhgp8::counter_add(plan.candidate_pairs, product(anchors.size(), columns.size()));
      }
    };
    if (left_end != 0) tail({0, left_end}, {left_end, begin}, prefixes[left_end - 1]);
    retain({left_end, right_begin});
    if (right_begin != m) tail({right_begin, m}, {end, right_begin}, suffixes[right_begin]);
    begin = end;
  }
  return plan;
}

// Independent bounded predicate: Gram determinant in arbitrary precision,
// rather than the product's interval or cross-product implementation.
bool oracle_witness(Lane lane, const Point3& a, const Point3& b, const Point3& z) {
  mhgp8::i64 h = 0;
  mhgp8::i64 u2 = 0;
  mhgp8::i64 v2 = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto u = static_cast<mhgp8::i64>(z[axis]) - a[axis];
    const auto v = static_cast<mhgp8::i64>(b[axis]) - z[axis];
    h += u * v;
    u2 += u * u;
    v2 += v * v;
  }
  if (h <= 0) return false;
  if (lane == Lane::Q2) return true;
  const Big h2 = Big(h) * h;
  const Big xi = Big(u2) * v2 - h2;
  return (lane == Lane::Q3 ? 3 : 2) * h2 > xi;
}

struct Checks {
  u64 plans{};
  u64 q2_plans{};
  u64 q34_plans{};
  u64 coverage_pairs{};
  u64 certificate_predicates{};
  u64 census_site_tests{};
  u64 q34_zero_predicates{};
  u64 candidate_pairs{};
  u64 final_q2_pairs{};
  u64 certified_tails{};
  u64 retained_tails{};
  u64 witness_sites{};
  u64 mutants_rejected{};
};

struct Growth {
  std::size_t n{};
  u64 candidate_pairs{};
  u64 proved_band_pairs{};
  std::size_t tasks{};
  u64 witness_sites{};
};

std::set<Pair> candidates(const Plan& plan) {
  std::set<Pair> result;
  for (const auto& task : plan.tasks) {
    if (task.killed) continue;
    for (std::size_t a = task.a.begin; a < task.a.end; ++a)
      for (std::size_t b = task.b.begin; b < task.b.end; ++b)
        require(result.emplace(plan.a_order[a], plan.b_order[b]).second,
                "judge.duplicate_candidate");
  }
  require(result.size() == plan.candidate_pairs, "judge.candidate_cardinality");
  return result;
}

std::set<Pair> exact_q2(const Input& input, unsigned h,
                      const std::set<Pair>* allowed, Checks& checks) {
  std::set<Pair> result;
  for (std::size_t a = 0; a < input.m; ++a) {
    for (std::size_t b = input.m; b < 2 * input.m; ++b) {
      if (allowed != nullptr && !allowed->contains({a, b})) continue;
      unsigned count = 0;
      for (std::size_t z = 0; z < input.points.size(); ++z) {
        if (z == a || z == b) continue;
        count += static_cast<unsigned>(oracle_witness(
            Lane::Q2, input.points[a], input.points[b], input.points[z]));
        mhgp8::counter_add(checks.census_site_tests);
      }
      if (count < h) result.emplace(a, b);
    }
  }
  return result;
}

void check_plan(const Input& input, const Plan& plan, Lane lane,
                unsigned h, bool matched_rows, Checks& checks) {
  mhgp8::counter_add(checks.plans);
  const auto m = input.m;
  const std::size_t w = (h + 1U) / 2U;
  const std::size_t length = w + 1;
  const auto chunks = (m + length - 1) / length;
  require(plan.work.chunks == chunks && plan.tasks.size() <= 3 * chunks,
          "judge.task_bound");
  require(plan.work.prefix_sites == m && plan.work.suffix_sites == m &&
          plan.work.anchor_box_sites == m, "judge.paid_linear_boxes");
  require(plan.work.witness_sites == 2 * w * plan.work.attempted_tails &&
          plan.work.witness_sites <= 4 * w * chunks, "judge.paid_witness_bound");
  require(plan.work.predicates.block_bound_tests == plan.work.attempted_tails,
          "judge.one_predicate_per_tail");
  std::vector<unsigned> coverage(m * m, 0);  // Bounded judge only.
  for (const auto& task : plan.tasks) {
    for (std::size_t a = task.a.begin; a < task.a.end; ++a) {
      for (std::size_t b = task.b.begin; b < task.b.end; ++b) {
        const auto aid = plan.a_order[a];
        const auto bid = plan.b_order[b];
        require(aid < m && bid >= m && bid < 2 * m, "judge.original_ids");
        ++coverage[aid * m + bid - m];
        mhgp8::counter_add(checks.coverage_pairs);
        if (!task.killed) continue;
        require(task.proposed_tail && 2 * task.witness_ranks.size() >= h,
                "judge.positive_population_size");
        std::set<std::size_t> witness_ids;
        for (std::size_t k = task.witness_ranks.begin; k < task.witness_ranks.end; ++k) {
          for (const auto zid : {plan.a_order[k], plan.b_order[k]}) {
            require(zid != aid && zid != bid && witness_ids.insert(zid).second,
                    "judge.witness_identity_disjointness");
            require(oracle_witness(lane, input.points[aid], input.points[bid],
                                   input.points[zid]), "judge.unsound_block_credit");
            mhgp8::counter_add(checks.certificate_predicates);
          }
        }
      }
    }
  }
  require(std::all_of(coverage.begin(), coverage.end(),
                     [](unsigned count) { return count == 1; }), "judge.coverage_partition");
  const auto kept = candidates(plan);
  mhgp8::counter_add(checks.candidate_pairs, kept.size());
  mhgp8::counter_add(checks.certified_tails, plan.work.certified_tails);
  mhgp8::counter_add(checks.retained_tails, plan.work.retained_tails);
  mhgp8::counter_add(checks.witness_sites, plan.work.witness_sites);
  if (lane == Lane::Q2) {
    mhgp8::counter_add(checks.q2_plans);
    const auto exact = exact_q2(input, h, nullptr, checks);
    const auto final = exact_q2(input, h, &kept, checks);
    require(exact == final, "judge.lost_q2_pairs");
    mhgp8::counter_add(checks.final_q2_pairs, final.size());
    if (matched_rows) {
      require(plan.work.retained_tails == 0, "judge.row_tail_not_certified");
      require(plan.candidate_pairs <= product(m, length + 2 * w), "judge.row_residue_bound");
      const auto expected = w >= m ? product(m, m) :
          product(2 * w + 1, m) - product(w, w + 1);
      require(exact.size() == expected, "judge.row_band_size");
      for (std::size_t a = 0; a < m; ++a)
        for (std::size_t b = m; b < 2 * m; ++b) {
          const int delta = static_cast<int>(input.points[a].y) - input.points[b].y;
          const bool in_band = static_cast<unsigned>(delta < 0 ? -delta : delta) <= w;
          require(exact.contains({a, b}) == in_band, "judge.physical_band");
        }
    }
  } else {
    mhgp8::counter_add(checks.q34_plans);
    require(matched_rows && plan.work.certified_tails == 0 &&
            plan.candidate_pairs == product(m, m), "judge.no_q2_transfer");
    for (std::size_t a = 0; a < m; ++a)
      for (std::size_t b = m; b < 2 * m; ++b)
        for (std::size_t z = 0; z < 2 * m; ++z) {
          if (z == a || z == b) continue;
          require(!oracle_witness(lane, input.points[a], input.points[b], input.points[z]),
                  "judge.row_q34_witness_not_empty");
          mhgp8::counter_add(checks.q34_zero_predicates);
        }
  }
}

Input rows(std::size_t m, bool permuted) {
  Input result;
  result.m = m;
  for (unsigned side = 0; side < 2; ++side)
    for (std::size_t i = 0; i < m; ++i) {
      require(i <= 65535, "fixture.u16");
      result.points.push_back({static_cast<std::uint16_t>(side == 0 ? 1000 : 60000),
                               static_cast<std::uint16_t>(i), 0});
    }
  if (permuted) {
    std::reverse(result.points.begin(), result.points.begin() + static_cast<std::ptrdiff_t>(m));
    std::rotate(result.points.begin() + static_cast<std::ptrdiff_t>(m),
                result.points.begin() + static_cast<std::ptrdiff_t>(m + m / 3),
                result.points.end());
  }
  return result;
}

void run() {
  Checks checks;
  for (const auto m : {std::size_t{16}, std::size_t{32}}) {
    for (const auto separation : {8U, 10U, 12U}) {
      for (const bool permuted : {false, true}) {
        const auto input = rows(m, permuted);
        validate(input, separation);
        for (const auto h : {1U, 5U, 10U})
          check_plan(input, propose(input, Lane::Q2, h), Lane::Q2, h, true, checks);
      }
      if (m == 16) {
        const auto input = rows(m, false);
        validate(input, separation);
        check_plan(input, propose(input, Lane::Q3, 9), Lane::Q3, 9, true, checks);
        check_plan(input, propose(input, Lane::Q4, 8), Lane::Q4, 8, true, checks);
      }
    }
  }
  // Unequal ordinate spacings are a valid input, not a reason to assume the
  // rank proposal successful. The sole tail has H_min=0: one of its proposed
  // witnesses is on the diameter boundary, and the other is strictly inside.
  const Input boundary{{{1000, 0, 0}, {1000, 1, 0}, {1000, 3, 0},
                         {60000, 1, 0}, {60000, 2, 0}, {60000, 4, 0}}, 3};
  validate(boundary, 12);
  const auto nominal = propose(boundary, Lane::Q2, 2);
  require(nominal.work.attempted_tails == 1 && nominal.work.retained_tails == 1 &&
          nominal.work.certified_tails == 0, "mutation.nominal_fallback");
  check_plan(boundary, nominal, Lane::Q2, 2, false, checks);
  const auto changed = propose(boundary, Lane::Q2, 2, Mutation::ClosedBoundary);
  require(changed.work.mutation_hits == 1 && changed.work.certified_tails == 1,
          "mutation.not_exercised");
  const auto all = exact_q2(boundary, 2, nullptr, checks);
  const auto allowed = candidates(changed);
  const auto wrong = exact_q2(boundary, 2, &allowed, checks);
  require(all.size() == wrong.size() + 1 && all.contains({2, 3}) &&
          !wrong.contains({2, 3}), "mutation.missing_physical_pair");
  mhgp8::counter_add(checks.mutants_rejected);
  require(checks.plans == 43 && checks.q2_plans == 37 && checks.q34_plans == 6 &&
          checks.certified_tails > 0 && checks.retained_tails > 0 &&
          checks.certificate_predicates > 1000 && checks.q34_zero_predicates > 1000 &&
          checks.mutants_rejected == 1, "gate.nonvacuity");
  // Valid u16 growth cases: count descriptors and proved formula bounds only.
  // No Cartesian expansion, geometric census or FULL output is executed here.
  std::vector<Growth> growth;
  for (const auto m : {std::size_t{4000}, std::size_t{4096}}) {
    const auto input = rows(m, false);
    validate(input, 12);
    const auto plan = propose(input, Lane::Q2, 10);
    const auto chunks = (m + 5) / 6;
    const auto band_pairs = product(11, m) - 30;
    require(plan.work.chunks == chunks && plan.tasks.size() <= 3 * chunks,
            "growth.task_bound");
    require(plan.work.prefix_sites == m && plan.work.suffix_sites == m &&
            plan.work.anchor_box_sites == m &&
            plan.work.witness_sites == 10 * plan.work.attempted_tails &&
            plan.work.witness_sites <= 20 * chunks, "growth.linear_preparation");
    require(plan.work.retained_tails == 0 && plan.work.certified_tails > 0 &&
            plan.candidate_pairs >= band_pairs &&
            plan.candidate_pairs <= product(16, m), "growth.proved_row_bound");
    growth.push_back({2 * m, plan.candidate_pairs, band_pairs,
                      plan.tasks.size(), plan.work.witness_sites});
  }
  std::cout << "{\"status\":\"passed_bounded_rectangle_model\",\"plans\":" << checks.plans
            << ",\"q2_plans\":" << checks.q2_plans
            << ",\"q34_plans\":" << checks.q34_plans
            << ",\"coverage_pairs\":" << checks.coverage_pairs
            << ",\"certificate_predicates\":" << checks.certificate_predicates
            << ",\"census_site_tests\":" << checks.census_site_tests
            << ",\"q34_zero_predicates\":" << checks.q34_zero_predicates
            << ",\"candidate_pairs\":" << checks.candidate_pairs
            << ",\"final_q2_pairs\":" << checks.final_q2_pairs
            << ",\"certified_tails\":" << checks.certified_tails
            << ",\"retained_tails\":" << checks.retained_tails
            << ",\"witness_sites\":" << checks.witness_sites
            << ",\"refuted_model_mutants\":" << checks.mutants_rejected
            << ",\"mutant_lost_q2_pairs\":1,\"product_predicate_called\":true"
               ",\"full_engine_executed\":false,\"performance_claim\":false"
               ",\"gcp_used\":false,\"growth_census_executed\":false,\"growth\":[";
  for (std::size_t i = 0; i < growth.size(); ++i) {
    if (i != 0) std::cout << ',';
    const auto& value = growth[i];
    std::cout << "{\"n\":" << value.n << ",\"s\":12,\"h\":10,\"candidate_pairs\":"
              << value.candidate_pairs << ",\"proved_band_pairs\":" << value.proved_band_pairs
              << ",\"tasks\":" << value.tasks << ",\"witness_sites\":" << value.witness_sites << '}';
  }
  std::cout << "]}\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try {
    run();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
