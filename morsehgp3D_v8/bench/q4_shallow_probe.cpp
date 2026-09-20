// Tranche29: explicit port of tranche28 fixture, output-judge and work-dump
// helpers. Frozen tranche28 is not edited or silently reused as qualification.
// Both the local28 and shallow29 engines REALLY execute on each input.
#define main mhgp8_shallow_unused_cover_probe_main
#include "q34_cover_probe.cpp"
#undef main
#include "../tests/exact_ball_oracle.hpp"
#include "lanes/q4_local.hpp"
#include "lanes/q4_shallow.hpp"

#include <map>

namespace {
namespace independent = mhgp8_test::ball_oracle;

struct LocalInput {
  Input input;
  u64 grid_size{}, permutation_keys{}, permutation_sort_comparisons{};
  u64 generation_auxiliary_capacity_bytes{};
};

bool dense_recipe(std::string_view recipe) {
  return recipe == "dense_prefix" || recipe == "dense_permuted";
}

LocalInput local_input(std::size_t n, std::string_view recipe) {
  if (!dense_recipe(recipe)) return {make_input(n, recipe), 0, 0, 0, 0};
  constexpr std::size_t grid_size = 41 * 21 * 38;
  require(n >= 8 && n <= grid_size + 2, "outside distinct dense grid fixture");
  LocalInput result{Input{{{900, 1000, 1000}, {1100, 1000, 1000}}}, grid_size, 0, 0, 0};
  auto& input = result.input;
  input.points.reserve(n);
  const auto append = [&](std::size_t i) {
    const auto layer = i / (41 * 21), offset = 40 + layer / 2;
    input.points.push_back({static_cast<std::uint16_t>(980 + i % 41),
      static_cast<std::uint16_t>(1120 + (i / 41) % 21),
      static_cast<std::uint16_t>(layer % 2 == 0 ? 1000 + offset : 1000 - offset)});
    ++input.proposals;
  };
  if (recipe == "dense_prefix") {
    for (std::size_t i = 0; input.points.size() < n; ++i) append(i);
  } else {
    // Explicit port of constructor27's fixture, NOT the audit's SHA256 order.
    // Permute the complete grid before taking any size-dependent prefix.
    std::vector<std::pair<u64, std::size_t>> order;
    order.reserve(grid_size);
    for (std::size_t i = 0; i < grid_size; ++i) {
      auto state = static_cast<u64>(i);
      order.emplace_back(random_word(state), i);
      ++result.permutation_keys;
    }
    std::sort(order.begin(), order.end(), [&](const auto& a, const auto& b) {
      ++result.permutation_sort_comparisons;
      return a < b;
    });
    result.generation_auxiliary_capacity_bytes = order.capacity() * sizeof(order[0]);
    for (std::size_t i = 0; input.points.size() < n; ++i) append(order[i].second);
  }  // Temporary permutation destruction is included in generation time.
  word(input.hash, n);
  for (const auto point : input.points)
    for (unsigned axis = 0; axis < 3; ++axis) word(input.hash, point[axis]);
  return result;
}

struct EmissionChecks {
  u64 supports{}, owner_distance_tests{}, distinct_balls{}, point_tests{};
  u64 shell_ids{}, strict_interiors{}, shell_capacity_bytes{};
};

// This judge checks each PUBLISHED support with independent rational Gram
// elimination, and each distinct PUBLISHED ball by a complete scalar census.
// It is not a census per root and does not prove large-cloud completeness.
EmissionChecks check_q4_emissions(const Output& output, std::span<const Point3> points) {
  struct Census { std::size_t depth{}; std::vector<std::size_t> shell; };
  std::map<std::array<i128, 5>, Census> cache;
  EmissionChecks work;
  const auto squared_distance = [](Point3 a, Point3 b) {
    i64 result = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto d = static_cast<i64>(a[axis]) - b[axis];
      result += d*d;
    }
    return result;
  };
  const auto diameter = squared_distance(points[0], points[1]);
  for (const auto& record : output.records) {
    require(record.arity == 4 && record.support[0] == 0 && record.support[1] == 1,
            "local q4 output has wrong arity or owning edge");
    std::array<Point3, 4> support{};
    for (std::size_t i = 0; i < support.size(); ++i) support[i] = points[record.support[i]];
    const auto reference = independent::make(support);
    require(reference.ball.has_value(), "emitted support is not positive and full rank");
    for (std::size_t i = 0; i < record.coefficients.size(); ++i)
      require(independent::Big(record.coefficients[i]) == reference.ball->coefficients[i],
              "emitted exact key differs from independent rational sphere");
    for (std::size_t i = 0; i < support.size(); ++i)
      for (std::size_t j = i + 1; j < support.size(); ++j) {
        require(squared_distance(support[i], support[j]) <= diameter, "emitted edge is not longest");
        ++work.owner_distance_tests;
      }
    ++work.supports;
    auto [found, inserted] = cache.try_emplace(record.coefficients);
    auto& census = found->second;
    if (inserted) {
      const auto& c = reference.ball->coefficients;
      for (std::size_t id = 0; id < points.size(); ++id) {
        const auto p = points[id];
        const independent::Big norm = independent::Big(p.x)*p.x + independent::Big(p.y)*p.y + independent::Big(p.z)*p.z;
        const independent::Big power = c[0]*norm + c[1]*p.x + c[2]*p.y + c[3]*p.z + c[4];
        if (power < 0) ++census.depth;
        if (power == 0) census.shell.push_back(id);
        ++work.point_tests;
      }
      ++work.distinct_balls;
      work.shell_ids += census.shell.size();
      work.strict_interiors += census.depth;
      work.shell_capacity_bytes += census.shell.capacity() * sizeof(std::size_t);
    }
    require(record.depth == census.depth && record.shell == census.shell,
            "published depth or complete shell differs from independent full-cloud census");
  }
  return work;  // Destruction of the judge's cache is paid inside validation.
}

std::vector<Record> expected_q4() {
  auto result = expected();
  result.erase(std::remove_if(result.begin(), result.end(), [](const auto& r) { return r.arity != 4; }), result.end());
  return result;
}


#define F(type, member) Field<type>{#member, &type::member}
constexpr std::array atlas_fields{
  F(Q4LocalAtlasWork, cells_created),
  F(Q4LocalAtlasWork, outside_cells),
  F(Q4LocalAtlasWork, deep_cells),
  F(Q4LocalAtlasWork, leaf_cells),
  F(Q4LocalAtlasWork, splits),
  F(Q4LocalAtlasWork, depth_stops),
  F(Q4LocalAtlasWork, node_stops),
  F(Q4LocalAtlasWork, small_stops),
  F(Q4LocalAtlasWork, active_sites_sum),
  F(Q4LocalAtlasWork, active_blocks_sum),
  F(Q4LocalAtlasWork, terminal_refinements),
  F(Q4LocalAtlasWork, terminal_deep_cells),
  F(Q4LocalAtlasWork, max_depth),
  F(Q4LocalAtlasWork, peak_fragment_bytes),
  F(Q4LocalAtlasWork, peak_build_bytes),
  F(Q4LocalAtlasWork, retained_bytes)};
constexpr std::array sweep_fields{
  F(Q4LocalSweepWork, seed_queries),
  F(Q4LocalSweepWork, seed_owner_tests),
  F(Q4LocalSweepWork, seed_owner_rejections),
  F(Q4LocalSweepWork, query_visits),
  F(Q4LocalSweepWork, line_tests),
  F(Q4LocalSweepWork, line_skips),
  F(Q4LocalSweepWork, leaf_queries),
  F(Q4LocalSweepWork, reference_points),
  F(Q4LocalSweepWork, reference_side_tests),
  F(Q4LocalSweepWork, active_blocks),
  F(Q4LocalSweepWork, active_sites),
  F(Q4LocalSweepWork, root_locations),
  F(Q4LocalSweepWork, clipped_events),
  F(Q4LocalSweepWork, clipped_inside),
  F(Q4LocalSweepWork, kept_events),
  F(Q4LocalSweepWork, constant_inside),
  F(Q4LocalSweepWork, constant_outside),
  F(Q4LocalSweepWork, constant_shell_ids),
  F(Q4LocalSweepWork, entries),
  F(Q4LocalSweepWork, exits),
  F(Q4LocalSweepWork, sort_comparisons),
  F(Q4LocalSweepWork, shell_sort_comparisons),
  F(Q4LocalSweepWork, group_comparisons),
  F(Q4LocalSweepWork, groups),
  F(Q4LocalSweepWork, boundary_skips),
  F(Q4LocalSweepWork, boundary_skipped_ids),
  F(Q4LocalSweepWork, depth_rejections),
  F(Q4LocalSweepWork, depth_skipped_ids),
  F(Q4LocalSweepWork, presentations),
  F(Q4LocalSweepWork, owner_tests),
  F(Q4LocalSweepWork, owner_rejections),
  F(Q4LocalSweepWork, positive_tests),
  F(Q4LocalSweepWork, positive_rejections),
  F(Q4LocalSweepWork, canonical_tests),
  F(Q4LocalSweepWork, canonical_rejections),
  F(Q4LocalSweepWork, emitted),
  F(Q4LocalSweepWork, shell_ids),
  F(Q4LocalSweepWork, groups_without_support),
  F(Q4LocalSweepWork, unexamined_after_emit),
  F(Q4LocalSweepWork, max_group),
  F(Q4LocalSweepWork, peak_buffer_bytes)};
constexpr std::array local_edge_fields{
  F(Q4LocalEdgeWork, node_visits),
  F(Q4LocalEdgeWork, bound_tests),
  F(Q4LocalEdgeWork, point_tests),
  F(Q4LocalEdgeWork, rejected_nodes),
  F(Q4LocalEdgeWork, split_nodes),
  F(Q4LocalEdgeWork, rejected_sites),
  F(Q4LocalEdgeWork, acute_seeds),
  F(Q4LocalEdgeWork, owner_tests),
  F(Q4LocalEdgeWork, owner_rejections),
  F(Q4LocalEdgeWork, seeds),
  F(Q4LocalEdgeWork, peak_live_buffer_bytes)};
constexpr std::array geometry_fields{
  F(Q4LocalGeometryWork, preparations),
  F(Q4LocalGeometryWork, cover_node_visits),
  F(Q4LocalGeometryWork, cover_range_advances),
  F(Q4LocalGeometryWork, cover_disjoint_nodes),
  F(Q4LocalGeometryWork, cover_splits),
  F(Q4LocalGeometryWork, cover_blocks),
  F(Q4LocalGeometryWork, cover_sites),
  F(Q4LocalGeometryWork, cover_excluded_sites),
  F(Q4LocalGeometryWork, cover_node_ids_copied),
  F(Q4LocalGeometryWork, projection_points),
  F(Q4LocalGeometryWork, hull_sort_comparisons),
  F(Q4LocalGeometryWork, hull_orientation_tests),
  F(Q4LocalGeometryWork, hull_vertices),
  F(Q4LocalGeometryWork, facets),
  F(Q4LocalGeometryWork, peak_retained_bytes)};
constexpr std::array partition_fields{
  F(Q4LocalPartitionWork, root_factories),
  F(Q4LocalPartitionWork, child_factories),
  F(Q4LocalPartitionWork, refine_factories),
  F(Q4LocalPartitionWork, input_nodes),
  F(Q4LocalPartitionWork, input_sites),
  F(Q4LocalPartitionWork, inherited_inside_sites),
  F(Q4LocalPartitionWork, node_visits),
  F(Q4LocalPartitionWork, block_bound_tests),
  F(Q4LocalPartitionWork, point_tests),
  F(Q4LocalPartitionWork, z_splits),
  F(Q4LocalPartitionWork, inside_nodes),
  F(Q4LocalPartitionWork, outside_nodes),
  F(Q4LocalPartitionWork, inside_sites),
  F(Q4LocalPartitionWork, outside_sites),
  F(Q4LocalPartitionWork, active_nodes),
  F(Q4LocalPartitionWork, active_sites),
  F(Q4LocalPartitionWork, budget_unexamined_nodes),
  F(Q4LocalPartitionWork, budget_ambiguous_nodes),
  F(Q4LocalPartitionWork, frontier_ids_copied),
  F(Q4LocalPartitionWork, peak_retained_bytes)};
constexpr std::array domain_query_fields{
  F(Q4LocalGeometryQueryWork, disk_tests),
  F(Q4LocalGeometryQueryWork, facet_tests)};
constexpr std::array positive_fields{
  F(Q4PositiveDomainWork, node_visits),
  F(Q4PositiveDomainWork, bound_tests),
  F(Q4PositiveDomainWork, endpoint_box_tests),
  F(Q4PositiveDomainWork, endpoint_leaf_tests),
  F(Q4PositiveDomainWork, point_tests),
  F(Q4PositiveDomainWork, admitted_nodes),
  F(Q4PositiveDomainWork, rejected_nodes),
  F(Q4PositiveDomainWork, split_nodes),
  F(Q4PositiveDomainWork, excluded_endpoints),
  F(Q4PositiveDomainWork, admitted_sites),
  F(Q4PositiveDomainWork, rejected_sites),
  F(Q4PositiveDomainWork, box_merges)};
#undef F
static_assert(sizeof(Q4LocalSweepWork) == sweep_fields.size()*sizeof(u64));
static_assert(sizeof(Q4LocalPartitionWork) == partition_fields.size()*sizeof(u64));
static_assert(sizeof(Q4LocalGeometryQueryWork) == domain_query_fields.size()*sizeof(u64));
static_assert(sizeof(Q4LocalGeometryWork) == geometry_fields.size()*sizeof(u64)+sizeof(Q4PositiveDomainWork));
static_assert(sizeof(Q4LocalAtlasWork) == atlas_fields.size()*sizeof(u64)+sizeof(Q4LocalPartitionWork)+sizeof(Q4LocalGeometryQueryWork));
static_assert(sizeof(Q4LocalEdgeWork) == local_edge_fields.size()*sizeof(u64)+sizeof(Q4LocalAtlasWork)+sizeof(Q4LocalGeometryWork)+sizeof(Q4LocalSweepWork));

void dump_local(const Q4LocalEdgeWork& value) {
  std::cout << '{'; dump_fields(value, local_edge_fields);
  std::cout << ",\"atlas\":{"; dump_fields(value.atlas, atlas_fields);
  std::cout << ",\"partition\":{"; dump_fields(value.atlas.partition, partition_fields);
  std::cout << "},\"domain\":{"; dump_fields(value.atlas.domain, domain_query_fields);
  std::cout << "}},\"geometry\":{"; dump_fields(value.geometry, geometry_fields);
  std::cout << ",\"domain\":{"; dump_fields(value.geometry.domain, positive_fields);
  std::cout << "}},\"sweep\":{"; dump_fields(value.sweep, sweep_fields); std::cout << "}}";
}


#define F(type, member) Field<type>{#member, &type::member}
constexpr std::array selection_fields{
  F(Q4ShallowSetWork, preparations), F(Q4ShallowSetWork, input_sites),
  F(Q4ShallowSetWork, form_tests), F(Q4ShallowSetWork, zero_sites),
  F(Q4ShallowSetWork, positive_sites), F(Q4ShallowSetWork, negative_sites),
  F(Q4ShallowSetWork, lex_comparisons), F(Q4ShallowSetWork, orientation_tests),
  F(Q4ShallowSetWork, coordinate_groups), F(Q4ShallowSetWork, duplicate_ids),
  F(Q4ShallowSetWork, positive_layers), F(Q4ShallowSetWork, negative_layers),
  F(Q4ShallowSetWork, layer_input_groups), F(Q4ShallowSetWork, layer_input_ids),
  F(Q4ShallowSetWork, boundary_groups), F(Q4ShallowSetWork, degenerate_groups),
  F(Q4ShallowSetWork, retained_ids), F(Q4ShallowSetWork, discarded_ids),
  F(Q4ShallowSetWork, retained_id_sort_comparisons), F(Q4ShallowSetWork, record_insertions),
  F(Q4ShallowSetWork, group_insertions), F(Q4ShallowSetWork, hull_index_copies),
  F(Q4ShallowSetWork, compaction_moves), F(Q4ShallowSetWork, peak_live_bytes),
  F(Q4ShallowSetWork, retained_bytes)};
constexpr std::array shallow_sweep_fields{
  F(Q4ShallowSweepWork, seed_queries), F(Q4ShallowSweepWork, seed_owner_tests),
  F(Q4ShallowSweepWork, seed_owner_rejections), F(Q4ShallowSweepWork, removed_seed_rejections),
  F(Q4ShallowSweepWork, membership_comparisons), F(Q4ShallowSweepWork, depth_rejected_groups),
  F(Q4ShallowSweepWork, depth_skipped_ids), F(Q4ShallowSweepWork, presentations),
  F(Q4ShallowSweepWork, owner_tests), F(Q4ShallowSweepWork, owner_rejections),
  F(Q4ShallowSweepWork, positive_tests), F(Q4ShallowSweepWork, positive_rejections),
  F(Q4ShallowSweepWork, canonical_tests), F(Q4ShallowSweepWork, canonical_rejections),
  F(Q4ShallowSweepWork, groups_without_support), F(Q4ShallowSweepWork, unexamined_after_emit),
  F(Q4ShallowSweepWork, emitted), F(Q4ShallowSweepWork, shell_ids),
  F(Q4ShallowSweepWork, peak_buffer_bytes)};
constexpr std::array shallow_edge_fields{
  F(Q4ShallowEdgeWork, seed_candidates), F(Q4ShallowEdgeWork, acute_tests),
  F(Q4ShallowEdgeWork, acute_seeds), F(Q4ShallowEdgeWork, owner_tests),
  F(Q4ShallowEdgeWork, owner_rejections), F(Q4ShallowEdgeWork, seeds),
  F(Q4ShallowEdgeWork, peak_live_buffer_bytes)};
#undef F
static_assert(sizeof(Q4ShallowSetWork)==selection_fields.size()*sizeof(u64));
static_assert(sizeof(Q4ShallowSweepWork)==shallow_sweep_fields.size()*sizeof(u64)+sizeof(Q4FamilyWork));
static_assert(sizeof(Q4ShallowEdgeWork)==shallow_edge_fields.size()*sizeof(u64)+sizeof(Q4ShallowSetWork)+
              sizeof(Q4LocalGeometryWork)+sizeof(Q4ShallowSweepWork));

void dump_shallow(const Q4ShallowEdgeWork& value) {
  std::cout << '{'; dump_fields(value, shallow_edge_fields);
  std::cout << ",\"geometry\":{"; dump_fields(value.geometry, geometry_fields);
  std::cout << ",\"domain\":{"; dump_fields(value.geometry.domain, positive_fields);
  std::cout << "}},\"selection\":{"; dump_fields(value.selection, selection_fields);
  std::cout << "},\"sweep\":{"; dump_fields(value.sweep, shallow_sweep_fields);
  std::cout << ",\"family\":{"; dump_fields(value.sweep.family, family_fields);
  std::cout << "}}}";
}

int shallow_run(std::size_t n, std::string_view recipe, std::size_t k, std::size_t leaf_sites) {
  const Q4LocalOptions baseline_options{Q4CenterDomainMode::Positive,7,4096,512,leaf_sites,true};
  const auto start=Clock::now();
  auto generated_input=local_input(n,recipe);
  auto& input=generated_input.input;
  const auto generated=Clock::now();
  const auto initial_seeds=recipe=="adversarial" || dense_recipe(recipe) ? n-2 : 2;
  const auto covered_sites=recipe=="far" ? 6 : n;
  u64 fixture_checks=0;
  for (std::size_t id=0; id<n; ++id) {
    const auto p=input.points[id];
    const i64 x=static_cast<i64>(p.x)-1000,y=static_cast<i64>(p.y)-1000,z=static_cast<i64>(p.z)-1000;
    const bool inside=x*x+y*y+z*z<=40000;
    const bool owned=x>-100 && x<100 && x*x+y*y+z*z>10000 &&
      (x+100)*(x+100)+y*y+z*z<=40000 && (x-100)*(x-100)+y*y+z*z<=40000;
    require(inside==(recipe!="far" || id<6) && owned==(id>=2 && id<2+initial_seeds),
            "shallow fixture cover/initial seed certificate failed");
    ++fixture_checks;
  }
  const auto fixture_checked=Clock::now();
  auto cloud=prepare_cloud(input.points);
  const auto prepared=Clock::now();
  auto index=make_q2_cloud_index(cloud);
  const auto indexed=Clock::now();
  auto cover=Q34EdgeCover::make(index,{0,1});
  const auto covered=Clock::now();
  require(cover->site_count()==covered_sites,"unexpected shallow fixture cover");
  Output baseline_output;
  const auto baseline_work=run_q4_local_edge_candidates(cover,k,baseline_options,[&](const auto& candidate) {
    require(candidate.arity==4,"local28 baseline emitted another lane");
    baseline_output.collect(candidate,n,k);
  });
  const auto baseline_ran=Clock::now();
  baseline_output.finish();
  const auto baseline_checked=Clock::now();
  Output output;
  const auto work=run_q4_shallow_edge_candidates(cover,k,[&](const auto& candidate) {
    require(candidate.arity==4,"shallow q4-only entry emitted another lane");
    output.collect(candidate,n,k);
  });
  const auto ran=Clock::now();
  output.finish();
  require(baseline_work.seeds==initial_seeds && work.seeds<=initial_seeds &&
          output.q3==0 && work.sweep.emitted==output.q4 && work.sweep.shell_ids==output.shell_ids_visited,
          "shallow output or retained-seed ledger differs");
  require(baseline_output.records==output.records && baseline_output.hash==output.hash,
          "shallow complete output differs from actual local28 execution");
  if (recipe=="far" || recipe=="cap") require(output.records==expected_q4(),"closed-form q4 fixture differs");
  const auto judged=check_q4_emissions(output,input.points);
  const auto cloud_work=cloud->work();
  const auto index_work=index->work();
  const auto cover_work=cover->work();
  const auto input_bytes=input.points.capacity()*sizeof(Point3),cloud_bytes=cloud->retained_bytes();
  const auto index_bytes=index->retained_bytes(),cover_bytes=cover->retained_bytes();
  const auto baseline_bytes=baseline_output.retained_bytes(),output_bytes=output.retained_bytes();
  const auto checked=Clock::now();
  cover.reset();index.reset();cloud.reset();
  baseline_output.release();output.release();std::vector<Point3>().swap(input.points);
  const auto done=Clock::now();
  const double shared=ms(start,generated)+ms(fixture_checked,covered);
  std::cout << std::setprecision(17)
    << "{\"schema\":\"mhgp8_q4_shallow_probe_v1\",\"status\":\"completed\","
    << "\"scope\":\"one_edge_q4_dual_layers_and_retained_sweeps_not_global_producer\",\"public_status\":\"not_claimed\","
    << "\"backend\":\"cpu_reference\",\"profile\":\"quantized_u16_input_only\",\"threads\":1,\"edge_ids\":[0,1],"
    << "\"timing_scope\":\"enclosing_wall_including_validation_release_pipeline_sums_share_preparation\","
    << "\"baseline\":\"q4_local28_really_executed_q4_only\","
    << "\"n\":" << n << ",\"regime\":\"" << recipe << "\",\"kmax\":" << k
    << ",\"baseline_options\":{\"domain\":\"positive\",\"max_depth\":7,\"node_budget\":4096,"
    << "\"z_test_budget\":512,\"leaf_sites\":" << leaf_sites << ",\"clip_events\":true}"
    << ",\"input_hash\":" << input.hash << ",\"expected_initial_seeds\":" << initial_seeds << ",\"cover_sites\":" << covered_sites
    << ",\"generation\":{\"proposals\":" << input.proposals << ",\"duplicates\":" << input.duplicates
    << ",\"random_calls\":" << input.random_calls << ",\"grid_size\":" << generated_input.grid_size
    << ",\"permutation_keys\":" << generated_input.permutation_keys
    << ",\"permutation_sort_comparisons\":" << generated_input.permutation_sort_comparisons
    << ",\"auxiliary_capacity_bytes\":" << generated_input.generation_auxiliary_capacity_bytes << '}'
    << ",\"validation\":{\"fixture_point_tests\":" << fixture_checks
    << ",\"supports\":" << judged.supports << ",\"owner_distance_tests\":" << judged.owner_distance_tests
    << ",\"distinct_balls\":" << judged.distinct_balls << ",\"point_tests\":" << judged.point_tests
    << ",\"shell_ids\":" << judged.shell_ids << ",\"strict_interiors\":" << judged.strict_interiors
    << ",\"shell_capacity_bytes\":" << judged.shell_capacity_bytes
    << ",\"method\":\"independent_rational_support_and_full_census_per_published_ball_not_large_completeness\"}"
    << ",\"baseline_work\":";dump_local(baseline_work);
  std::cout << ",\"baseline_digest\":";baseline_output.dump();
  std::cout << ",\"work\":";dump_shallow(work);
  std::cout << ",\"digest\":";output.dump();
  std::cout << ",\"cloud_work\":{";dump_fields(cloud_work,cloud_fields);
  std::cout << "},\"index_work\":{";dump_fields(index_work,index_fields);
  std::cout << "},\"cover_work\":{";dump_fields(cover_work,cover_fields);
  std::cout << "},\"memory\":{\"scope\":\"dynamic_capacities_not_RSS_fixed_objects_or_judge_map_nodes\","
    << "\"id_bytes\":" << sizeof(std::size_t) << ",\"input_capacity_bytes\":" << input_bytes
    << ",\"cloud_retained_bytes\":" << cloud_bytes << ",\"index_retained_bytes\":" << index_bytes
    << ",\"cover_retained_bytes\":" << cover_bytes << ",\"baseline_output_retained_bytes\":" << baseline_bytes
    << ",\"output_retained_bytes\":" << output_bytes << '}'
    << ",\"timings\":{\"generation_ms\":" << ms(start,generated)
    << ",\"fixture_validation_ms\":" << ms(generated,fixture_checked)
    << ",\"cloud_ms\":" << ms(fixture_checked,prepared) << ",\"index_ms\":" << ms(prepared,indexed)
    << ",\"cover_ms\":" << ms(indexed,covered) << ",\"baseline_run_callback_ms\":" << ms(covered,baseline_ran)
    << ",\"baseline_validation_ms\":" << ms(baseline_ran,baseline_checked)
    << ",\"run_callback_ms\":" << ms(baseline_checked,ran) << ",\"validation_ms\":" << ms(ran,checked)
    << ",\"release_ms\":" << ms(checked,done)
    << ",\"baseline_prepare_run_sum_ms\":" << shared+ms(covered,baseline_ran)
    << ",\"shallow_prepare_run_sum_ms\":" << shared+ms(baseline_checked,ran)
    << ",\"total_ms\":" << ms(start,done) << "}}\n";
  return 0;
}
}  // namespace

int main(int argc,char** argv) {
  try {
    if (argc!=5) throw std::invalid_argument("usage: mhgp8_q4_shallow_probe n recipe K5_or_10 baseline_leaf_sites");
    const auto n=number(argv[1]),k=number(argv[3]),leaf=number(argv[4]);
    const std::string_view recipe(argv[2]);
    if (n<8 || (k!=5 && k!=10) ||
        (recipe!="far" && recipe!="cap" && recipe!="adversarial" && !dense_recipe(recipe)) ||
        (recipe=="cap" && n>35307) || (recipe=="adversarial" && n>514))
      throw std::invalid_argument("outside fixture domain; not an algorithm quota");
    return shallow_run(n,recipe,k,leaf);
  } catch (const std::exception& error) {
    std::cerr << "q4 shallow probe: " << error.what() << '\n';return 1;
  }
}
