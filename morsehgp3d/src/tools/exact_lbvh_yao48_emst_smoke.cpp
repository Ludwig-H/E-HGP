#include "morsehgp3d/hierarchy/exact_lbvh_yao48_emst.hpp"
#include "pair_support_smoke_clouds.hpp"

#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace smoke_clouds = morsehgp3d::tools::pair_support_smoke;

struct Options {
  std::string family{"uniform_latin"};
  std::size_t point_count{1000U};
  std::size_t seed_window_radius{32U};
};

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    const char* option_name) {
  std::size_t value = 0U;
  const char* const begin = text.data();
  const char* const end = begin + text.size();
  const auto [cursor, error] = std::from_chars(begin, end, value);
  if (error != std::errc{} || cursor != end) {
    throw std::invalid_argument(
        std::string{option_name} + " requires a nonnegative integer");
  }
  return value;
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view option{argv[index]};
    if (option == "--help") {
      std::cout
          << "Usage: morsehgp3d_exact_lbvh_yao48_emst_smoke "
             "[--family uniform_latin|eight_clusters] "
             "[--point-count N] [--seed-window-radius W]\n";
      std::exit(0);
    }
    if (index + 1 >= argc) {
      throw std::invalid_argument(
          std::string{option} + " requires a value");
    }
    const std::string_view value{argv[++index]};
    if (option == "--family") {
      options.family = std::string{value};
    } else if (option == "--point-count") {
      options.point_count = parse_size(value, "--point-count");
    } else if (option == "--seed-window-radius") {
      options.seed_window_radius =
          parse_size(value, "--seed-window-radius");
    } else {
      throw std::invalid_argument("unknown option: " + std::string{option});
    }
  }
  if (options.point_count == 0U) {
    throw std::invalid_argument("--point-count must be positive");
  }
  if (options.family != "uniform_latin" &&
      options.family != "eight_clusters") {
    throw std::invalid_argument(
        "--family must be uniform_latin or eight_clusters");
  }
  return options;
}

[[nodiscard]] std::vector<morsehgp3d::exact::CertifiedPoint3> make_points(
    const Options& options) {
  return options.family == "uniform_latin"
             ? smoke_clouds::uniform_latin_points(options.point_count)
             : smoke_clouds::eight_clusters_points(options.point_count);
}

template <typename Clock>
[[nodiscard]] std::uint64_t elapsed_nanoseconds(
    const std::chrono::time_point<Clock>& begin,
    const std::chrono::time_point<Clock>& end) {
  const auto elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin)
          .count();
  if (elapsed < 0) {
    throw std::logic_error("a benchmark clock moved backwards");
  }
  return static_cast<std::uint64_t>(elapsed);
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options options = parse_options(argc, argv);
    using Clock = std::chrono::steady_clock;
    const auto generation_begin = Clock::now();
    const std::vector<morsehgp3d::exact::CertifiedPoint3> points =
        make_points(options);
    const morsehgp3d::spatial::CanonicalPointCloud cloud =
        morsehgp3d::spatial::CanonicalPointCloud::rejecting_duplicates(
            std::span<const morsehgp3d::exact::CertifiedPoint3>{points});
    const auto generation_end = Clock::now();
    const morsehgp3d::spatial::MortonLbvhIndex index =
        morsehgp3d::spatial::MortonLbvhIndex::build(cloud);
    const auto lbvh_end = Clock::now();
    const morsehgp3d::hierarchy::ExactLbvhYao48EmstResult result =
        morsehgp3d::hierarchy::build_exact_lbvh_yao48_emst(
            index,
            cloud,
            morsehgp3d::hierarchy::ExactLbvhYao48EmstConfig{
                options.seed_window_radius});
    const auto search_end = Clock::now();
    const auto& counters = result.counters;
    if (!result.locally_structurally_valid()) {
      throw std::runtime_error(
          "the exact LBVH Yao48 benchmark result is not structurally valid");
    }

    std::cout
        << "{\"schema_version\":1"
        << ",\"backend\":\"" << result.backend << "\""
        << ",\"profile\":\"" << result.profile << "\""
        << ",\"mode\":\"" << result.mode << "\""
        << ",\"public_status\":\"" << result.public_status << "\""
        << ",\"family\":\"" << options.family << "\""
        << ",\"point_count\":" << options.point_count
        << ",\"seed_window_radius\":"
        << counters.morton_seed_window_radius
        << ",\"generation_nanoseconds\":"
        << elapsed_nanoseconds(generation_begin, generation_end)
        << ",\"lbvh_build_nanoseconds\":"
        << elapsed_nanoseconds(generation_end, lbvh_end)
        << ",\"search_nanoseconds\":"
        << elapsed_nanoseconds(lbvh_end, search_end)
        << ",\"directed_candidate_count\":"
        << counters.directed_candidate_count
        << ",\"unique_candidate_edge_count\":"
        << counters.unique_candidate_edge_count
        << ",\"selected_edge_count\":" << counters.selected_edge_count
        << ",\"seed_distance_evaluation_count\":"
        << counters.seed_distance_evaluation_count
        << ",\"node_visit_count\":" << counters.node_visit_count
        << ",\"strict_aabb_prune_count\":"
        << counters.strict_aabb_prune_count
        << ",\"equal_bound_descent_count\":"
        << counters.equal_bound_descent_count
        << ",\"cone_mask_empty_count\":"
        << counters.cone_mask_empty_count
        << ",\"leaf_distance_evaluation_count\":"
        << counters.leaf_distance_evaluation_count
        << ",\"maximum_node_visit_count_per_source\":"
        << counters.maximum_node_visit_count_per_source
        << ",\"maximum_leaf_distance_evaluation_count_per_source\":"
        << counters.maximum_leaf_distance_evaluation_count_per_source
        << ",\"seeded_subtree_skip_count\":"
        << counters.seeded_subtree_skip_count
        << ",\"fixed_output_record_byte_count\":"
        << counters.fixed_output_record_byte_count
        << ",\"complete_source_coverage\":"
        << (counters.complete_source_coverage ? "true" : "false")
        << ",\"no_candidate_truncation\":"
        << (counters.no_candidate_truncation ? "true" : "false")
        << ",\"higher_order_delaunay_mosaic_materialized\":"
        << (counters.higher_order_delaunay_mosaic_materialized
                ? "true"
                : "false")
        << ",\"global_pair_matrix_materialized\":"
        << (counters.global_pair_matrix_materialized ? "true" : "false")
        << ",\"public_status_claimed\":"
        << (counters.public_status_claimed ? "true" : "false")
        << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "exact LBVH Yao48 smoke failed: " << error.what() << '\n';
    return 1;
  }
}
