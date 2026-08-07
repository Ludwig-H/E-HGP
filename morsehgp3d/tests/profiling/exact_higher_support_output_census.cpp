// Exhaustive exact census of the USEFUL OUTPUT of the higher-support stage.
//
// The growth profile next door measures the FRONTIER ENGINE: how much of the
// universe the exclusion architecture manages to decide per unit of work.  It
// answers "how fast does the current algorithm go".  This tool answers the
// orthogonal question the measurements of 7 August 2026 left open: "how many
// records does the answer actually contain".
//
// It therefore does not use the frontier at all.  It enumerates the canonical
// universe C(n,3) + C(n,4) exhaustively and classifies every single support
// with the same two exact primitives the product path uses at a terminal --
// exact::analyze_circumcenter_support_integer for the geometry gate and
// hierarchy::ExactHigherSupportIndexedClosedBallQuery::classify for the closed
// rank.  Those primitives are shared by BOTH verification bases (fresh CPU
// replay and tile-certified device), so a count produced here is by
// construction the count the product path would emit, not an approximation of
// it.
//
// The closed-ball query is run once per support at the LARGEST requested order,
// and the observed closed rank of every accepted support is histogrammed.  A
// support whose observed closed rank is r is accepted at maximum order K
// exactly when r <= min(K+1, n) -- the interior cap is
// maximum_relevant_closed_rank - support_size and the shell of an accepted
// support equals the support -- so one exhaustive pass yields the exact output
// count for every order at once.
//
// Everything published here is `profiling_only` / `not_claimed`.  A census is
// a measurement of the answer's size; it certifies no pipeline, closes no gate
// and promotes no status.

#include "../../src/tools/pair_support_smoke_clouds.hpp"

#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/exact/integer_circumcenter.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/support.hpp"
#include "morsehgp3d/hierarchy/higher_support_closed_ball.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <ostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace smoke_clouds = morsehgp3d::tools::pair_support_smoke;

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::CircumcenterSupportStatus;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::exact::IntegerCircumcenterAnalysis;
using morsehgp3d::exact::analyze_circumcenter_support_integer;
using morsehgp3d::exact::canonical_integer_string;
using morsehgp3d::hierarchy::ExactHigherSupportClosedBallClassification;
using morsehgp3d::hierarchy::ExactHigherSupportClosedBallOutcome;
using morsehgp3d::hierarchy::ExactHigherSupportIndexedClosedBallQuery;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

using Clock = std::chrono::steady_clock;

constexpr std::string_view schema =
    "morsehgp3d.exact_higher_support.output_census.v1";
constexpr std::size_t maximum_supported_order = 10U;

// The sealed measurement this census is falsifiable against: the certified
// n=32 / K=5 / uniform_latin product run of 7 August 2026 published exactly
// 171 accepted events and zero extra-shell diagnostics on the host
// fresh-replay basis, and the tile-certified device path reproduced them.
constexpr std::size_t contract_point_count = 32U;
constexpr std::size_t contract_maximum_order = 5U;
constexpr std::size_t contract_accepted_event_count = 171U;
constexpr std::size_t contract_extra_shell_diagnostic_count = 0U;
constexpr std::string_view contract_evidence =
    "docs/validation/phase15_r2f_host_fresh_replay_n32_k5_g4_67facb1.json";

struct Options {
  std::size_t point_count{32U};
  std::string family{"uniform_latin"};
  std::size_t maximum_order{maximum_supported_order};
  std::string output_path;
  bool contract_test{false};
  std::size_t progress_interval{1U << 20U};
};

// One arity of the census.  Every enumerated support lands in exactly one of
// the five terminal categories, which is asserted against the exact binomial
// universe before anything is published.
struct ArityCensus {
  std::size_t support_size{};
  bool applicable{};
  BigInt universe{0};
  std::size_t enumerated{};
  std::size_t affinely_dependent{};
  std::size_t boundary_reduced{};
  std::size_t exterior_circumcenter{};
  std::size_t above_rank{};
  std::size_t accepted{};
  std::size_t extra_shell{};
  // Indexed by observed closed rank; length maximum_relevant_closed_rank + 1.
  std::vector<std::size_t> accepted_by_closed_rank;
  std::vector<std::size_t> extra_shell_by_closed_rank;
  std::size_t closed_ball_query_count{};
  std::size_t closed_ball_node_visit_count{};
  std::uint64_t wall_time_ns{};
};

[[nodiscard]] std::size_t parse_size(
    std::string_view text, std::string_view option) {
  std::size_t value = 0U;
  if (text.empty()) {
    throw std::invalid_argument(std::string{option} + " requires a value");
  }
  for (const char character : text) {
    if (character < '0' || character > '9') {
      throw std::invalid_argument(
          std::string{option} + " requires a decimal value");
    }
    const std::size_t digit = static_cast<std::size_t>(character - '0');
    if (value > (static_cast<std::size_t>(-1) - digit) / 10U) {
      throw std::invalid_argument(std::string{option} + " overflows size_t");
    }
    value = value * 10U + digit;
  }
  return value;
}

[[nodiscard]] BigInt binomial(std::size_t n, std::size_t k) {
  BigInt value = 0;
  if (k > n) {
    return value;
  }
  value = 1;
  for (std::size_t step = 0U; step < k; ++step) {
    value *= static_cast<unsigned long long>(n - step);
    value /= static_cast<unsigned long long>(step + 1U);
  }
  return value;
}

[[nodiscard]] std::vector<CertifiedPoint3> generate_points(
    const Options& options) {
  if (options.family == "uniform_latin") {
    return smoke_clouds::uniform_latin_points(options.point_count);
  }
  if (options.family == "eight_clusters") {
    return smoke_clouds::eight_clusters_points(options.point_count);
  }
  throw std::invalid_argument(
      "--family must be uniform_latin or eight_clusters");
}

// The single classification of one canonical support, transcribed from
// ExactHigherSupportStreamBuilder::classify_terminal_support so that the two
// cannot drift: geometry gate first, closed-ball rank only for a minimal
// support, acceptance only when the shell equals the support.
template <std::size_t SupportSize>
void classify_support(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const std::array<PointId, 4>& support_ids,
    std::size_t maximum_relevant_closed_rank,
    std::size_t traversal_frontier_bound,
    ArityCensus& census) {
  std::array<ExactRational3, SupportSize> support_points{};
  for (std::size_t index_in_support = 0U; index_in_support < SupportSize;
       ++index_in_support) {
    support_points[index_in_support] =
        cloud.point(support_ids[index_in_support]).exact();
  }
  const IntegerCircumcenterAnalysis analysis =
      analyze_circumcenter_support_integer(support_points);
  switch (analysis.status) {
    case CircumcenterSupportStatus::affinely_dependent:
      census.affinely_dependent += 1U;
      return;
    case CircumcenterSupportStatus::boundary_reduced:
      census.boundary_reduced += 1U;
      return;
    case CircumcenterSupportStatus::exterior_circumcenter:
      census.exterior_circumcenter += 1U;
      return;
    case CircumcenterSupportStatus::minimal:
      break;
  }
  if (!analysis.center.has_value() || !analysis.squared_level.has_value()) {
    throw std::logic_error("a minimal higher support omitted its exact sphere");
  }
  const std::size_t interior_cap =
      maximum_relevant_closed_rank - SupportSize;
  const ExactHigherSupportClosedBallClassification classification =
      ExactHigherSupportIndexedClosedBallQuery::classify(
          index,
          cloud,
          support_ids,
          SupportSize,
          *analysis.center,
          *analysis.squared_level,
          interior_cap,
          traversal_frontier_bound);
  census.closed_ball_query_count += 1U;
  census.closed_ball_node_visit_count +=
      classification.counters.node_visit_count;
  if (classification.outcome ==
      ExactHigherSupportClosedBallOutcome::rank_exceeded) {
    census.above_rank += 1U;
    return;
  }
  const std::size_t observed_closed_rank =
      classification.interior_ids.size() + classification.shell_count;
  if (observed_closed_rank >= census.accepted_by_closed_rank.size()) {
    throw std::logic_error(
        "an accepted higher support reported a closed rank above its cap");
  }
  if (classification.shell_count == SupportSize) {
    census.accepted += 1U;
    census.accepted_by_closed_rank[observed_closed_rank] += 1U;
    return;
  }
  census.extra_shell += 1U;
  census.extra_shell_by_closed_rank[observed_closed_rank] += 1U;
}

[[nodiscard]] ArityCensus census_arity(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t support_size,
    std::size_t maximum_relevant_closed_rank,
    std::size_t progress_interval) {
  const std::size_t point_count = cloud.size();
  ArityCensus census;
  census.support_size = support_size;
  census.universe = binomial(point_count, support_size);
  census.applicable = maximum_relevant_closed_rank >= support_size;
  census.accepted_by_closed_rank.assign(
      maximum_relevant_closed_rank + 1U, 0U);
  census.extra_shell_by_closed_rank.assign(
      maximum_relevant_closed_rank + 1U, 0U);
  if (!census.applicable) {
    return census;
  }
  const std::size_t traversal_frontier_bound =
      ExactHigherSupportIndexedClosedBallQuery::proved_traversal_frontier_bound(
          index);
  const Clock::time_point start = Clock::now();
  std::array<PointId, 4> support_ids{};
  const auto visit = [&]() {
    if (support_size == 3U) {
      classify_support<3U>(
          index,
          cloud,
          support_ids,
          maximum_relevant_closed_rank,
          traversal_frontier_bound,
          census);
    } else {
      classify_support<4U>(
          index,
          cloud,
          support_ids,
          maximum_relevant_closed_rank,
          traversal_frontier_bound,
          census);
    }
    census.enumerated += 1U;
    if (progress_interval != 0U &&
        census.enumerated % progress_interval == 0U) {
      std::cerr << "  arity " << support_size << ": " << census.enumerated
                << " enumerated, " << census.accepted << " accepted\n";
    }
  };
  // Canonical strictly increasing enumeration; no ordered tuple and no
  // permutation is ever materialised.
  const PointId bound = static_cast<PointId>(point_count);
  if (support_size == 3U) {
    for (PointId first = 0U; first + 2U < bound; ++first) {
      support_ids[0] = first;
      for (PointId second = first + 1U; second + 1U < bound; ++second) {
        support_ids[1] = second;
        for (PointId third = second + 1U; third < bound; ++third) {
          support_ids[2] = third;
          support_ids[3] = 0U;
          visit();
        }
      }
    }
  } else {
    for (PointId first = 0U; first + 3U < bound; ++first) {
      support_ids[0] = first;
      for (PointId second = first + 1U; second + 2U < bound; ++second) {
        support_ids[1] = second;
        for (PointId third = second + 1U; third + 1U < bound; ++third) {
          support_ids[2] = third;
          for (PointId fourth = third + 1U; fourth < bound; ++fourth) {
            support_ids[3] = fourth;
            visit();
          }
        }
      }
    }
  }
  census.wall_time_ns = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          Clock::now() - start)
          .count());
  return census;
}

// The partition assertion: every enumerated support left in exactly one
// category, and the enumeration covered the exact binomial universe.  A census
// that cannot close this identity publishes nothing.
void verify_partition(const ArityCensus& census) {
  if (!census.applicable) {
    return;
  }
  const std::size_t sum = census.affinely_dependent + census.boundary_reduced +
      census.exterior_circumcenter + census.above_rank + census.accepted +
      census.extra_shell;
  if (sum != census.enumerated) {
    throw std::logic_error(
        "the higher-support census categories do not partition its "
        "enumeration");
  }
  if (census.universe != static_cast<unsigned long long>(census.enumerated)) {
    throw std::logic_error(
        "the higher-support census enumeration does not equal its exact "
        "binomial universe");
  }
  std::size_t accepted_histogram = 0U;
  for (const std::size_t count : census.accepted_by_closed_rank) {
    accepted_histogram += count;
  }
  std::size_t extra_shell_histogram = 0U;
  for (const std::size_t count : census.extra_shell_by_closed_rank) {
    extra_shell_histogram += count;
  }
  if (accepted_histogram != census.accepted ||
      extra_shell_histogram != census.extra_shell) {
    throw std::logic_error(
        "the higher-support census rank histogram does not sum to its "
        "category count");
  }
}

// Output count at a smaller order, derived from the histogram: a support whose
// observed closed rank is r is accepted at order K exactly when
// r <= min(K+1, n) and the arity is applicable at that order.
[[nodiscard]] std::size_t accepted_at_order(
    const ArityCensus& census, std::size_t order, std::size_t point_count) {
  const std::size_t rank = std::min(order + 1U, point_count);
  if (rank < census.support_size) {
    return 0U;
  }
  std::size_t total = 0U;
  for (std::size_t observed = 0U;
       observed < census.accepted_by_closed_rank.size();
       ++observed) {
    if (observed <= rank) {
      total += census.accepted_by_closed_rank[observed];
    }
  }
  return total;
}

[[nodiscard]] std::size_t extra_shell_at_order(
    const ArityCensus& census, std::size_t order, std::size_t point_count) {
  const std::size_t rank = std::min(order + 1U, point_count);
  if (rank < census.support_size) {
    return 0U;
  }
  std::size_t total = 0U;
  for (std::size_t observed = 0U;
       observed < census.extra_shell_by_closed_rank.size();
       ++observed) {
    if (observed <= rank) {
      total += census.extra_shell_by_closed_rank[observed];
    }
  }
  return total;
}

void write_counts(std::ostream& stream, const std::vector<std::size_t>& counts) {
  stream << '[';
  for (std::size_t index = 0U; index < counts.size(); ++index) {
    if (index != 0U) {
      stream << ',';
    }
    stream << counts[index];
  }
  stream << ']';
}

void write_arity(std::ostream& stream, const ArityCensus& census) {
  stream << "{\"above_rank\":" << census.above_rank
         << ",\"accepted\":" << census.accepted
         << ",\"accepted_by_closed_rank\":";
  write_counts(stream, census.accepted_by_closed_rank);
  stream << ",\"affinely_dependent\":" << census.affinely_dependent
         << ",\"applicable\":" << (census.applicable ? "true" : "false")
         << ",\"boundary_reduced\":" << census.boundary_reduced
         << ",\"closed_ball_node_visits\":" << census.closed_ball_node_visit_count
         << ",\"closed_ball_queries\":" << census.closed_ball_query_count
         << ",\"enumerated\":" << census.enumerated
         << ",\"exterior_circumcenter\":" << census.exterior_circumcenter
         << ",\"extra_shell\":" << census.extra_shell
         << ",\"extra_shell_by_closed_rank\":";
  write_counts(stream, census.extra_shell_by_closed_rank);
  stream << ",\"support_size\":" << census.support_size
         << ",\"universe\":\"" << canonical_integer_string(census.universe)
         << "\",\"wall_time_ns\":" << census.wall_time_ns << '}';
}

void write_report(
    std::ostream& stream,
    const Options& options,
    std::size_t point_count,
    std::size_t maximum_relevant_closed_rank,
    const ArityCensus& triples,
    const ArityCensus& quadruples,
    bool contract_verified) {
  const BigInt universe = triples.universe + quadruples.universe;
  stream << "{\"backend\":\"reference_cpu\""
         << ",\"canonical_point_count\":" << point_count
         << ",\"contract_verified\":" << (contract_verified ? "true" : "false")
         << ",\"deployment_status\":\"profiling_only\""
         << ",\"family\":\"" << options.family << "\""
         << ",\"maximum_relevant_closed_rank\":" << maximum_relevant_closed_rank
         << ",\"nonclaims\":\"census_of_the_output_size_only_no_pipeline_"
            "certification_no_scale_gate_no_slo_no_public_status_and_no_"
            "extrapolation_beyond_the_enumerated_point_counts\""
         << ",\"orders\":[";
  bool first_order = true;
  for (std::size_t order = 1U; order <= options.maximum_order; ++order) {
    if (!first_order) {
      stream << ',';
    }
    first_order = false;
    const std::size_t triple_accepted =
        accepted_at_order(triples, order, point_count);
    const std::size_t quadruple_accepted =
        accepted_at_order(quadruples, order, point_count);
    const std::size_t triple_extra =
        extra_shell_at_order(triples, order, point_count);
    const std::size_t quadruple_extra =
        extra_shell_at_order(quadruples, order, point_count);
    const std::size_t records =
        triple_accepted + quadruple_accepted + triple_extra + quadruple_extra;
    stream << "{\"accepted_events\":"
           << (triple_accepted + quadruple_accepted)
           << ",\"accepted_quadruples\":" << quadruple_accepted
           << ",\"accepted_triples\":" << triple_accepted
           << ",\"extra_shell_diagnostics\":" << (triple_extra + quadruple_extra)
           << ",\"maximum_order\":" << order
           << ",\"maximum_relevant_closed_rank\":"
           << std::min(order + 1U, point_count)
           << ",\"records\":" << records
           << ",\"records_per_point\":";
    if (point_count == 0U) {
      stream << "null";
    } else {
      std::ostringstream ratio;
      ratio.setf(std::ios::fixed);
      ratio.precision(6);
      ratio << static_cast<double>(records) / static_cast<double>(point_count);
      stream << ratio.str();
    }
    stream << '}';
  }
  stream << "],\"point_count\":" << options.point_count
         << ",\"public_status\":\"not_claimed\""
         << ",\"quadruples\":";
  write_arity(stream, quadruples);
  stream << ",\"role\":\"profile_only\""
         << ",\"schema\":\"" << schema << "\""
         << ",\"triples\":";
  write_arity(stream, triples);
  stream << ",\"universe\":\"" << canonical_integer_string(universe) << "\"}"
         << '\n';
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view option{argv[index]};
    if (option == "--contract-test") {
      options.contract_test = true;
      continue;
    }
    if (option == "--help" || option == "-h") {
      std::cout
          << "usage: exact_higher_support_output_census"
             " [--point-count N] [--family uniform_latin|eight_clusters]"
             " [--maximum-order K] [--output PATH] [--progress-interval N]"
             " [--contract-test]\n";
      std::exit(0);
    }
    if (index + 1 >= argc) {
      throw std::invalid_argument(std::string{option} + " requires a value");
    }
    const std::string_view value{argv[index + 1]};
    ++index;
    if (option == "--point-count") {
      options.point_count = parse_size(value, option);
    } else if (option == "--family") {
      options.family = std::string{value};
    } else if (option == "--maximum-order") {
      options.maximum_order = parse_size(value, option);
    } else if (option == "--output") {
      options.output_path = std::string{value};
    } else if (option == "--progress-interval") {
      options.progress_interval = parse_size(value, option);
    } else {
      throw std::invalid_argument("unknown option " + std::string{option});
    }
  }
  if (options.contract_test) {
    options.point_count = contract_point_count;
    options.family = "uniform_latin";
    options.maximum_order = maximum_supported_order;
  }
  if (options.point_count == 0U) {
    throw std::invalid_argument("--point-count must be strictly positive");
  }
  if (options.maximum_order == 0U ||
      options.maximum_order > maximum_supported_order) {
    throw std::invalid_argument("--maximum-order must be in [1, 10]");
  }
  return options;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options options = parse_options(argc, argv);
    const std::vector<CertifiedPoint3> input = generate_points(options);
    const CanonicalPointCloud cloud = CanonicalPointCloud::rejecting_duplicates(
        std::span<const CertifiedPoint3>{input});
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    const std::size_t point_count = cloud.size();
    const std::size_t effective_maximum_order =
        std::min(options.maximum_order, point_count);
    const std::size_t maximum_relevant_closed_rank =
        std::min(effective_maximum_order + 1U, point_count);

    std::cerr << "census: n=" << point_count << " family=" << options.family
              << " maximum_order=" << options.maximum_order
              << " maximum_relevant_closed_rank="
              << maximum_relevant_closed_rank << '\n';
    const ArityCensus triples = census_arity(
        index, cloud, 3U, maximum_relevant_closed_rank,
        options.progress_interval);
    verify_partition(triples);
    const ArityCensus quadruples = census_arity(
        index, cloud, 4U, maximum_relevant_closed_rank,
        options.progress_interval);
    verify_partition(quadruples);

    bool contract_verified = false;
    if (options.contract_test) {
      const std::size_t accepted =
          accepted_at_order(triples, contract_maximum_order, point_count) +
          accepted_at_order(quadruples, contract_maximum_order, point_count);
      const std::size_t extra_shell =
          extra_shell_at_order(triples, contract_maximum_order, point_count) +
          extra_shell_at_order(quadruples, contract_maximum_order, point_count);
      if (accepted != contract_accepted_event_count) {
        std::ostringstream message;
        message << "the exhaustive census disagrees with the sealed product "
                   "run "
                << contract_evidence << ": " << accepted
                << " accepted events against "
                << contract_accepted_event_count;
        throw std::logic_error(message.str());
      }
      if (extra_shell != contract_extra_shell_diagnostic_count) {
        std::ostringstream message;
        message << "the exhaustive census disagrees with the sealed product "
                   "run "
                << contract_evidence << ": " << extra_shell
                << " extra-shell diagnostics against "
                << contract_extra_shell_diagnostic_count;
        throw std::logic_error(message.str());
      }
      contract_verified = true;
    }

    if (!options.output_path.empty()) {
      std::ofstream file{options.output_path};
      if (!file) {
        throw std::runtime_error(
            "cannot open " + options.output_path + " for writing");
      }
      write_report(
          file,
          options,
          point_count,
          maximum_relevant_closed_rank,
          triples,
          quadruples,
          contract_verified);
    }
    write_report(
        std::cout,
        options,
        point_count,
        maximum_relevant_closed_rank,
        triples,
        quadruples,
        contract_verified);
  } catch (const std::exception& error) {
    std::cerr << "exact higher-support output census failed: " << error.what()
              << '\n';
    return 1;
  }
  return 0;
}
