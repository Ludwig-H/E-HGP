// What the certified germination generator costs at the contract's scale.
//
// The question this answers has never been asked.  Selectivity was measured at
// n=512 -- 1.81 % of the arity-three universe on uniform_latin, 81.24 % on
// eight_clusters -- and the repository has said repeatedly that selectivity is
// data-dependent and means nothing below scale.  Meanwhile the generator that
// is actually running, the product subdivision, was measured on 8 August to
// examine 1.83 times the universe at every size, because a prune certificate
// covers 1.1 tuples.
//
// So before wiring the germinated stream into the anchored session -- which
// means meeting consumers that refuse its verification basis by name, and is a
// real piece of work -- it is worth knowing whether it terminates at 50 000
// points at all, and at what candidate-per-record ratio.  If that ratio is
// 1e3, wiring it is the whole game.  If it is 1e6, it is not.
//
// The seed regime matters and is a command-line choice, because the two are
// not the same claim:
//   --seed-regime exhaustive   O(n^2) pairs, complete without further argument
//   --seed-regime certified    D <= 2 R(p) under a direction set whose covering
//                              radius is sealed with a proof; complete by
//                              theorem, and the only regime that can run at
//                              50 000 points
//
// Everything published here is profiling_only / not_claimed.  A probe measures
// what an enumeration costs; it certifies no pipeline, closes no gate and
// promotes no status.

#include "../../src/tools/pair_support_smoke_clouds.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/germinated_higher_support_stream.hpp"
#include "morsehgp3d/hierarchy/local_germination.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <chrono>
#include <cstddef>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace smoke_clouds = morsehgp3d::tools::pair_support_smoke;

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::GerminatedHigherSupportResult;
using morsehgp3d::hierarchy::LocalGerminationCertificate;
using morsehgp3d::hierarchy::LocalGerminationConfig;
using morsehgp3d::hierarchy::build_germinated_higher_support_stream;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

using Clock = std::chrono::steady_clock;

constexpr std::string_view schema =
    "morsehgp3d.germinated_higher_support.scale_probe.v1";

struct Options {
  std::size_t point_count{50000U};
  std::size_t maximum_order{5U};
  std::string family{"uniform_latin"};
  std::string seed_regime{"certified"};
  std::size_t tangent_direction_count{26U};
  std::size_t seed_disc_ring_count{2U};
  std::size_t segment_position_count{16U};
  std::string output_path;
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
    value = value * 10U + static_cast<std::size_t>(character - '0');
  }
  return value;
}

[[nodiscard]] Options parse(int argc, char** argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    const auto next = [&]() -> std::string_view {
      if (index + 1 >= argc) {
        throw std::invalid_argument(
            std::string{argument} + " requires a value");
      }
      ++index;
      return std::string_view{argv[index]};
    };
    if (argument == "--point-count") {
      options.point_count = parse_size(next(), argument);
    } else if (argument == "--maximum-order") {
      options.maximum_order = parse_size(next(), argument);
    } else if (argument == "--family") {
      options.family = std::string{next()};
    } else if (argument == "--seed-regime") {
      options.seed_regime = std::string{next()};
    } else if (argument == "--tangent-directions") {
      options.tangent_direction_count = parse_size(next(), argument);
    } else if (argument == "--seed-disc-rings") {
      options.seed_disc_ring_count = parse_size(next(), argument);
    } else if (argument == "--segment-positions") {
      options.segment_position_count = parse_size(next(), argument);
    } else if (argument == "--output") {
      options.output_path = std::string{next()};
    } else {
      throw std::invalid_argument(
          "unknown option " + std::string{argument});
    }
  }
  if (options.family != "uniform_latin" &&
      options.family != "eight_clusters") {
    throw std::invalid_argument(
        "--family must be uniform_latin or eight_clusters");
  }
  if (options.seed_regime != "exhaustive" &&
      options.seed_regime != "certified") {
    throw std::invalid_argument(
        "--seed-regime must be exhaustive or certified");
  }
  if (options.seed_regime == "certified" &&
      options.tangent_direction_count == 0U) {
    throw std::invalid_argument(
        "the certified seed regime requires a proved direction set");
  }
  return options;
}

[[nodiscard]] std::string_view restriction_name(
    LocalGerminationCertificate::SeedRestriction restriction) {
  switch (restriction) {
    case LocalGerminationCertificate::SeedRestriction::exhaustive_over_pairs:
      return "exhaustive_over_pairs";
    case LocalGerminationCertificate::SeedRestriction::certified_tangent_bound:
      return "certified_tangent_bound";
    case LocalGerminationCertificate::SeedRestriction::
        declared_heuristic_cutoff:
      return "declared_heuristic_cutoff";
  }
  return "unknown";
}

void emit_certificate(
    std::ostream& out,
    std::string_view key,
    const LocalGerminationCertificate& certificate) {
  out << "    \"" << key << "\":{"
      << "\"proof_basis\":\"" << certificate.proof_basis << "\","
      << "\"support_size\":" << certificate.support_size << ','
      << "\"seed_restriction\":\""
      << restriction_name(certificate.seed_restriction) << "\","
      << "\"completeness_guaranteed\":"
      << (certificate.completeness_guaranteed() ? "true" : "false") << ','
      << "\"tangent_direction_count\":"
      << certificate.tangent_direction_count << ','
      << "\"tangent_covering_radius_millidegrees\":"
      << certificate.tangent_covering_radius_millidegrees << ','
      << "\"tangent_covering_radius_proved\":"
      << (certificate.tangent_covering_radius_proved ? "true" : "false") << ','
      << "\"pairs_examined\":" << certificate.counters.pairs_examined << ','
      << "\"pairs_retained\":" << certificate.counters.pairs_retained << ','
      << "\"third_vertices_examined\":"
      << certificate.counters.third_vertices_examined << ','
      << "\"third_vertices_retained\":"
      << certificate.counters.third_vertices_retained << ','
      << "\"triple_candidates\":" << certificate.counters.triple_candidates
      << ','
      << "\"quadruple_candidates\":"
      << certificate.counters.quadruple_candidates << ','
      << "\"population_queries\":" << certificate.counters.population_queries
      << '}';
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options options = parse(argc, argv);

    const Clock::time_point generation_start = Clock::now();
    const std::vector<CertifiedPoint3> points =
        options.family == "uniform_latin"
            ? smoke_clouds::uniform_latin_points(options.point_count)
            : smoke_clouds::eight_clusters_points(options.point_count);
    const CanonicalPointCloud cloud =
        CanonicalPointCloud::rejecting_duplicates(points);
    const Clock::time_point index_start = Clock::now();
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    const Clock::time_point germination_start = Clock::now();

    const std::size_t maximum_relevant_closed_rank =
        std::min(options.maximum_order + 1U, cloud.size());

    LocalGerminationConfig config;
    config.seed_disc_ring_count = options.seed_disc_ring_count;
    config.segment_position_count = options.segment_position_count;
    // A positive cutoff is a data-dependent restriction the theorem does not
    // supply, so this probe never sets one.  The only restriction it may use
    // is the certified tangent bound.
    config.seed_neighbourhood_cutoff_multiple = 0U;
    config.tangent_direction_count = options.seed_regime == "certified"
        ? options.tangent_direction_count
        : 0U;

    const GerminatedHigherSupportResult result =
        build_germinated_higher_support_stream(
            index, cloud, maximum_relevant_closed_rank, config);
    const Clock::time_point finish = Clock::now();

    const auto milliseconds = [](Clock::time_point from,
                                 Clock::time_point to) {
      return std::chrono::duration<double, std::milli>(to - from).count();
    };
    const double germination_ms = milliseconds(germination_start, finish);
    const std::size_t records = result.events.size();
    const std::size_t classified = result.counters.classified;

    std::ostringstream out;
    out << std::fixed << std::setprecision(3);
    out << "{\n"
        << "  \"schema\":\"" << schema << "\",\n"
        << "  \"deployment_status\":\"profiling_only\",\n"
        << "  \"public_status\":\"not_claimed\",\n"
        << "  \"point_count\":" << cloud.size() << ",\n"
        << "  \"family\":\"" << options.family << "\",\n"
        << "  \"maximum_order\":" << options.maximum_order << ",\n"
        << "  \"maximum_relevant_closed_rank\":"
        << maximum_relevant_closed_rank << ",\n"
        << "  \"seed_regime\":\"" << options.seed_regime << "\",\n"
        << "  \"timings_ms\":{\"generation\":"
        << milliseconds(generation_start, index_start)
        << ",\"lbvh\":" << milliseconds(index_start, germination_start)
        << ",\"germination_and_classification\":" << germination_ms << "},\n"
        << "  \"production_identity_holds\":"
        << (result.production_identity_holds ? "true" : "false") << ",\n"
        << "  \"refusal_reason\":\"" << result.refusal_reason << "\",\n"
        << "  \"accepted_events\":" << records << ",\n"
        << "  \"duplicate_accepted_emissions\":"
        << result.duplicate_accepted_emissions << ",\n"
        << "  \"classification\":{"
        << "\"classified\":" << classified << ','
        << "\"affinely_dependent\":" << result.counters.affinely_dependent
        << ','
        << "\"boundary_reduced\":" << result.counters.boundary_reduced << ','
        << "\"exterior_circumcenter\":"
        << result.counters.exterior_circumcenter << ','
        << "\"above_rank\":" << result.counters.above_rank << ','
        << "\"extra_shell\":" << result.counters.extra_shell << ','
        << "\"accepted\":" << result.counters.accepted << ','
        << "\"closed_ball_query_count\":"
        << result.counters.closed_ball_query_count << ','
        << "\"closed_ball_node_visit_count\":"
        << result.counters.closed_ball_node_visit_count << "},\n"
        << "  \"certificates\":{\n";
    emit_certificate(out, "arity3", result.certificates[0]);
    out << ",\n";
    emit_certificate(out, "arity4", result.certificates[1]);
    out << "\n  },\n";

    // The two numbers the contract is read in.
    out << "  \"candidates_per_record\":"
        << (records == 0U
                ? 0.0
                : static_cast<double>(classified) /
                      static_cast<double>(records))
        << ",\n"
        << "  \"microseconds_per_classified_support\":"
        << (classified == 0U
                ? 0.0
                : germination_ms * 1000.0 / static_cast<double>(classified))
        << ",\n"
        << "  \"microseconds_per_record\":"
        << (records == 0U
                ? 0.0
                : germination_ms * 1000.0 / static_cast<double>(records))
        << "\n}\n";

    std::cout << out.str();
    if (!options.output_path.empty()) {
      std::ofstream file{options.output_path};
      if (!file) {
        throw std::runtime_error(
            "cannot open " + options.output_path + " for writing");
      }
      file << out.str();
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "germinated higher-support scale probe failed: "
              << error.what() << '\n';
    return 1;
  }
}
