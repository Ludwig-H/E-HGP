// Tranche30: explicit port of the frozen28/29 fixture and independent judge
// helpers, with both shallow29 and window30 REALLY executed. No old proof
// or old build is reused as qualification of this new engine.
#define main mhgp8_window_unused_cover_probe_main
#include "q34_cover_probe.cpp"
#undef main
#include "../tests/exact_ball_oracle.hpp"
#include "lanes/q4_local.hpp"
#include "lanes/q4_shallow.hpp"
#include "lanes/q4_window.hpp"

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


#define F(type, member) Field<type>{#member, &type::member}
constexpr std::array window_fields{
  F(Q4WindowSelectionWork, seed_queries), F(Q4WindowSelectionWork, entry_heap_insertions),
  F(Q4WindowSelectionWork, exit_heap_insertions), F(Q4WindowSelectionWork, entry_heap_replacements),
  F(Q4WindowSelectionWork, exit_heap_replacements), F(Q4WindowSelectionWork, heap_comparisons),
  F(Q4WindowSelectionWork, heap_sort_comparisons), F(Q4WindowSelectionWork, constant_rejected_seeds),
  F(Q4WindowSelectionWork, disjoint_rejected_seeds), F(Q4WindowSelectionWork, fixed_depth_rejected_seeds),
  F(Q4WindowSelectionWork, lower_bounds), F(Q4WindowSelectionWork, upper_bounds),
  F(Q4WindowSelectionWork, point_windows), F(Q4WindowSelectionWork, second_pass_sites),
  F(Q4WindowSelectionWork, window_comparisons), F(Q4WindowSelectionWork, lower_ids),
  F(Q4WindowSelectionWork, upper_ids), F(Q4WindowSelectionWork, inner_ids),
  F(Q4WindowSelectionWork, outside_ids), F(Q4WindowSelectionWork, fixed_inside_sites),
  F(Q4WindowSelectionWork, rejected_event_ids), F(Q4WindowSelectionWork, max_inner_ids),
  F(Q4WindowSelectionWork, max_endpoint_ids), F(Q4WindowSelectionWork, peak_heap_bytes),
  F(Q4WindowSelectionWork, peak_buffer_bytes)};
constexpr std::array window_edge_fields{
  F(Q4WindowEdgeWork, seed_candidates), F(Q4WindowEdgeWork, acute_tests),
  F(Q4WindowEdgeWork, acute_seeds), F(Q4WindowEdgeWork, owner_tests),
  F(Q4WindowEdgeWork, owner_rejections), F(Q4WindowEdgeWork, seeds),
  F(Q4WindowEdgeWork, peak_live_buffer_bytes)};
#undef F
static_assert(sizeof(Q4WindowSelectionWork)==window_fields.size()*sizeof(u64));
static_assert(sizeof(Q4WindowSweepWork)==sizeof(Q4ShallowSweepWork)+sizeof(Q4WindowSelectionWork));
static_assert(sizeof(Q4WindowEdgeWork)==window_edge_fields.size()*sizeof(u64)+sizeof(Q4LocalGeometryWork)+
              sizeof(Q4ShallowSetWork)+sizeof(Q4WindowSweepWork));
void dump_window(const Q4WindowEdgeWork& value) {
  std::cout << '{';dump_fields(value,window_edge_fields);
  std::cout << ",\"geometry\":{";dump_fields(value.geometry,geometry_fields);
  std::cout << ",\"domain\":{";dump_fields(value.geometry.domain,positive_fields);
  std::cout << "}},\"selection\":{";dump_fields(value.selection,selection_fields);
  std::cout << "},\"sweep\":{\"sweep\":{";dump_fields(value.sweep.sweep,shallow_sweep_fields);
  std::cout << ",\"family\":{";dump_fields(value.sweep.sweep.family,family_fields);
  std::cout << "}},\"window\":{";dump_fields(value.sweep.window,window_fields);
  std::cout << "}}}";
}
int window_run(std::size_t n, std::string_view recipe, std::size_t k) {
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
  const auto baseline_work=run_q4_shallow_edge_candidates(cover,k,[&](const auto& candidate) {
    require(candidate.arity==4,"shallow29 baseline emitted another lane");
    baseline_output.collect(candidate,n,k);
  });
  const auto baseline_ran=Clock::now();
  baseline_output.finish();
  const auto baseline_checked=Clock::now();
  Output output;
  const auto work=run_q4_window_edge_candidates(cover,k,[&](const auto& candidate) {
    require(candidate.arity==4,"shallow q4-only entry emitted another lane");
    output.collect(candidate,n,k);
  });
  const auto ran=Clock::now();
  output.finish();
  require(baseline_work.seeds==work.seeds && work.seeds<=initial_seeds &&
          work.selection==baseline_work.selection &&
          output.q3==0 && work.sweep.sweep.emitted==output.q4 && work.sweep.sweep.shell_ids==output.shell_ids_visited,
          "shallow output or retained-seed ledger differs");
  require(baseline_output.records==output.records && baseline_output.hash==output.hash,
          "shallow complete output differs from actual shallow29 execution");
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
    << "{\"schema\":\"mhgp8_q4_window_probe_v1\",\"status\":\"completed\","
    << "\"scope\":\"one_edge_q4_exact_root_window_not_global_producer\",\"public_status\":\"not_claimed\","
    << "\"backend\":\"cpu_reference\",\"profile\":\"quantized_u16_input_only\",\"threads\":1,\"edge_ids\":[0,1],"
    << "\"timing_scope\":\"enclosing_wall_including_validation_release_pipeline_sums_share_preparation\","
    << "\"baseline\":\"q4_shallow29_really_executed_q4_only\","
    << "\"n\":" << n << ",\"regime\":\"" << recipe << "\",\"kmax\":" << k
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
    << ",\"baseline_work\":";dump_shallow(baseline_work);
  std::cout << ",\"baseline_digest\":";baseline_output.dump();
  std::cout << ",\"work\":";dump_window(work);
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
    << ",\"window_prepare_run_sum_ms\":" << shared+ms(baseline_checked,ran)
    << ",\"total_ms\":" << ms(start,done) << "}}\n";
  return 0;
}
}  // namespace

int main(int argc,char** argv) {
  try {
    if (argc!=4) throw std::invalid_argument("usage: mhgp8_q4_window_probe n recipe K5_or_10");
    const auto n=number(argv[1]),k=number(argv[3]);
    const std::string_view recipe(argv[2]);
    if (n<8 || (k!=5 && k!=10) ||
        (recipe!="far" && recipe!="cap" && recipe!="adversarial" && !dense_recipe(recipe)) ||
        (recipe=="cap" && n>35307) || (recipe=="adversarial" && n>514))
      throw std::invalid_argument("outside fixture domain; not an algorithm quota");
    return window_run(n,recipe,k);
  } catch (const std::exception& error) {
    std::cerr << "q4 window probe: " << error.what() << '\n';return 1;
  }
}
