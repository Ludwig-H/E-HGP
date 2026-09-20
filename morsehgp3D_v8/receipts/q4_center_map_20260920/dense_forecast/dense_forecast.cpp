// Explicit adaptation of tranche26's auxiliary dense_forecast.cpp. Outside
// the 170-source product inventory. Only fixture/CLI/JSON helpers are reused;
// the included probe's renamed main is never called. No dense census runs.
#define main mhgp8_auxiliary_unused_cover_probe_main
#include "../../../bench/q34_cover_probe.cpp"
#undef main
#include "lanes/q4_center_map.hpp"

namespace {
#define F(member) Field<Q34PoolWork>{#member, &Q34PoolWork::member}
constexpr std::array forecast_fields{
  F(seed_owner_tests), F(seed_owner_rejections), F(seed_queries), F(certificate_builds),
  F(sqrt_iterations), F(variance_bounds), F(variance_sqrt_iterations), F(proposed_sites),
  F(paired_predicate_tests), F(q3_credits), F(q4_universal_credits), F(q3_rejected),
  F(q4_universal_rejected), F(collective_queries), F(endpoint_tests), F(constant_tests),
  F(event_count), F(sort_comparisons), F(group_comparisons), F(event_side_tests), F(groups),
  F(max_group), F(collective_minimum_sum), F(collective_q4_rejected), F(q4_rejected),
  F(both_rejected), F(q3_only_survivors), F(q4_only_survivors), F(both_survivors), F(peak_event_bytes)};
#undef F
static_assert(sizeof(Q34PoolWork) == forecast_fields.size()*sizeof(u64));


#define F(type, member) Field<type>{#member, &type::member}
constexpr std::array map_fields{
  F(Q4CenterMapWork, preparations),
  F(Q4CenterMapWork, prepared_forms),
  F(Q4CenterMapWork, projection_points),
  F(Q4CenterMapWork, hull_sort_comparisons),
  F(Q4CenterMapWork, hull_orientation_tests),
  F(Q4CenterMapWork, hull_vertices),
  F(Q4CenterMapWork, facets),
  F(Q4CenterMapWork, queries),
  F(Q4CenterMapWork, seed_owner_tests),
  F(Q4CenterMapWork, seed_owner_rejections),
  F(Q4CenterMapWork, rejected_queries),
  F(Q4CenterMapWork, unknown_queries),
  F(Q4CenterMapWork, query_visits),
  F(Q4CenterMapWork, line_tests),
  F(Q4CenterMapWork, line_skips),
  F(Q4CenterMapWork, cells_created),
  F(Q4CenterMapWork, pending_ids_copied),
  F(Q4CenterMapWork, pending_lists_created),
  F(Q4CenterMapWork, node_storage_growths),
  F(Q4CenterMapWork, cells_evaluated),
  F(Q4CenterMapWork, disk_tests),
  F(Q4CenterMapWork, facet_tests),
  F(Q4CenterMapWork, outside_cells),
  F(Q4CenterMapWork, deep_cells),
  F(Q4CenterMapWork, witness_tests),
  F(Q4CenterMapWork, inside_credits),
  F(Q4CenterMapWork, outside_witnesses),
  F(Q4CenterMapWork, splits),
  F(Q4CenterMapWork, compressed_deep),
  F(Q4CenterMapWork, compressed_outside),
  F(Q4CenterMapWork, depth_stops),
  F(Q4CenterMapWork, budget_stops),
  F(Q4CenterMapWork, empty_stops),
  F(Q4CenterMapWork, max_depth),
  F(Q4CenterMapWork, peak_pending_bytes),
  F(Q4CenterMapWork, peak_retained_bytes)};
constexpr std::array domain_fields{
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
static_assert(sizeof(Q4CenterMapWork) == sizeof(Q4PositiveDomainWork) + map_fields.size() * sizeof(u64));
static_assert(sizeof(Q4PositiveDomainWork) == domain_fields.size() * sizeof(u64));

Q4CenterDomainMode domain_option(std::string_view name) {
  if(name=="disk") return Q4CenterDomainMode::Disk;
  if(name=="positive") return Q4CenterDomainMode::Positive;
  throw std::invalid_argument("unknown auxiliary center domain");
}
u64 permutation_key(u64 value) {
  value+=0x9e3779b97f4a7c15ULL;
  value=(value^(value>>30))*0xbf58476d1ce4e5b9ULL;
  value=(value^(value>>27))*0x94d049bb133111ebULL;
  return value^(value>>31);
}

void merge_forecast(Q34PoolWork& total,const Q34PoolWork& value) {
  for(const auto& field:forecast_fields) {
    auto& dst=total.*field.member;
    const auto src=value.*field.member;
    if(field.member==&Q34PoolWork::max_group || field.member==&Q34PoolWork::peak_event_bytes)
      dst=std::max(dst,src);
    else counter_add(dst,src);
  }
}

int dense_run(std::size_t n,std::size_t k,std::string_view recipe,std::string_view domain,unsigned depth) {
  constexpr std::size_t budget=64, node_budget=4096, grid_size=41*21*38;
  const Q34PoolOptions options{Q34ChordBound::Variance,Q34PoolReduction::Collective};
  const Q4CenterMapOptions map_options{domain_option(domain),depth,node_budget};
  validate_q4_center_map_options(map_options);
  const auto started=Clock::now();
  std::vector<Point3> points{{900,1000,1000},{1100,1000,1000}};
  points.reserve(n);
  u64 permutation_keys=0,permutation_comparisons=0;
  std::size_t generation_auxiliary_bytes=0;
  const auto append=[&](std::size_t i) {
    const auto layer=i/(41*21),offset=40+layer/2;
    points.push_back({static_cast<std::uint16_t>(980+i%41),
      static_cast<std::uint16_t>(1120+(i/41)%21),
      static_cast<std::uint16_t>(layer%2==0 ? 1000+offset : 1000-offset)});
  };
  if(recipe=="dense_prefix") {
    for(std::size_t i=0;points.size()<n;++i) append(i);
  } else {
    // Permute the COMPLETE grid before taking any n-dependent prefix.
    // This recipe differs from the independent audit's SHA256 sort.
    std::vector<std::pair<u64,std::size_t>> permutation;
    permutation.reserve(grid_size);
    for(std::size_t i=0;i<grid_size;++i) {
      permutation.emplace_back(permutation_key(static_cast<u64>(i)),i);
      ++permutation_keys;
    }
    std::sort(permutation.begin(),permutation.end(),[&](const auto& x,const auto& y) {
      ++permutation_comparisons;return x<y;
    });
    generation_auxiliary_bytes=permutation.capacity()*sizeof(permutation[0]);
    for(std::size_t i=0;points.size()<n;++i) append(permutation[i].second);
  } // Permutation destruction is paid in generation, not retained by the engine.
  u64 input_hash=14695981039346656037ULL;
  word(input_hash,n);
  for(auto p:points) for(unsigned axis=0;axis<3;++axis) word(input_hash,p[axis]);
  const auto generated=Clock::now();
  u64 fixture_point_checks=0,owned_seed_checks=0;
  for(std::size_t id=0;id<n;++id) {
    const auto p=points[id];
    const i64 x=static_cast<i64>(p.x)-1000,y=static_cast<i64>(p.y)-1000,z=static_cast<i64>(p.z)-1000;
    require(x*x+y*y+z*z<=40000,"dense fixture site outside cover");
    ++fixture_point_checks;
    if(id<2) continue;
    require(x>-100 && x<100 && x*x+y*y+z*z>10000 &&
      (x+100)*(x+100)+y*y+z*z<40000 && (x-100)*(x-100)+y*y+z*z<40000,
      "dense seed is not acute/strictly owned");
    ++owned_seed_checks;
  }
  const auto fixture_checked=Clock::now();
  auto cloud=prepare_cloud(points);
  const auto prepared=Clock::now();
  auto index=make_q2_cloud_index(cloud);
  const auto indexed=Clock::now();
  auto cover=Q34EdgeCover::make(index,{0,1});
  const auto covered=Clock::now();
  auto pool=Q34WitnessPool::make(cover,budget);
  const auto pooled=Clock::now();
  require(cover->site_count()==n && pool->ids().size()==budget,"dense cover/pool size differs");
  require(cover->ranges().size()==1 && cover->ranges()[0].first==0 && cover->ranges()[0].last==n,
          "dense cover is not the complete root range");
  u64 pool_hash=14695981039346656037ULL,pool_ids_checked=0;
  const auto order=index->spatial_order();
  for(std::size_t i=0;i<budget;++i) {
    require(pool->ids()[i]==order[i*n/budget],"pool does not match spatial quantiles");
    word(pool_hash,pool->ids()[i]);++pool_ids_checked;
  }
  const auto pool_checked=Clock::now();
  Q34PoolWork work{};
  Q4CenterMapWork map_work{};
  std::size_t workspace_bytes=0,map_retained_bytes=0,peak_live_bytes=0;
  u64 q3_only_after_map=0,both_rejected_by_map=0;
  double assessment_ms=0,map_prepare_ms=0,map_query_ms=0;
  {
    Q34PoolWorkspace workspace;
    std::unique_ptr<Q4CenterMap> map;
    for(std::size_t x=2;x<n;++x) {
      const auto assessment_start=Clock::now();
      const auto result=assess_q34_family_pool(pool,x,k,options,workspace);
      const auto assessment_end=Clock::now();
      assessment_ms+=ms(assessment_start,assessment_end);
      require(result.work.seed_queries==1 && result.work.seed_owner_rejections==0,
              "owned dense assessment inactive");
      merge_forecast(work,result.work);
      peak_live_bytes=std::max(peak_live_bytes,workspace.retained_bytes()+(map?map->retained_bytes():0));
      if(!result.q4_rejected) {
        if(!map) {
          const auto before=Clock::now();
          map=Q4CenterMap::make(pool,k,map_options);
          map_prepare_ms+=ms(before,Clock::now());
        }
        const auto before=Clock::now();
        const bool rejected=map->reject_seed(x);
        map_query_ms+=ms(before,Clock::now());
        peak_live_bytes=std::max(peak_live_bytes,workspace.retained_bytes()+map->last_query_peak_bytes());
        if(rejected) {
          if(result.q3_rejected) ++both_rejected_by_map;
          else ++q3_only_after_map;
        }
      }
    }
    workspace_bytes=workspace.retained_bytes();
    if(map) { map_work=map->work();map_retained_bytes=map->retained_bytes(); }
  } // Both scratch and map destruction are paid in the enclosing interval.
  const auto filtered=Clock::now();
  const auto cloud_work=cloud->work();
  const auto index_work=index->work();
  const auto cover_work=cover->work();
  const auto pool_work=pool->work();
  const auto input_bytes=points.capacity()*sizeof(Point3),cloud_bytes=cloud->retained_bytes();
  const auto index_bytes=index->retained_bytes(),cover_bytes=cover->retained_bytes(),pool_bytes=pool->retained_bytes();
  const auto survivors_before_map=n-2-work.q4_rejected;
  const auto survivors=survivors_before_map-map_work.rejected_queries;
  const u64 lower_bound=static_cast<u64>(n)*survivors;
  require(work.certificate_builds==n-2 && work.peak_event_bytes==workspace_bytes,
          "assessment preparation/capacity differs");
  const auto checked=Clock::now();
  pool.reset();cover.reset();index.reset();cloud.reset();std::vector<Point3>().swap(points);
  const auto done=Clock::now();
  std::cout<<std::setprecision(17)
    <<"{\"schema\":\"mhgp8_dense_center_map_forecast_v1\",\"status\":\"completed\","
    <<"\"scope\":\"executed_pool_assessments_then_shared_lazy_map_known_seeds_with_unexecuted_scan_lower_bound\","
    <<"\"public_status\":\"not_claimed\",\"backend\":\"cpu_reference\",\"profile\":\"quantized_u16_input_only\","
    <<"\"recipe\":\""<<recipe<<"\",\"permutation\":\""<<(recipe=="dense_prefix"?"none":"full_grid_splitmix64_order_v1")<<"\",\"threads\":1,\"edge_ids\":[0,1],"
    <<"\"timing_scope\":\"auxiliary_generation_validation_preparation_filter_map_release_excludes_json\","
    <<"\"family_sweeps_executed\":0,\"edge_generator_executed\":false,\"candidates_computed\":false,"
    <<"\"n\":"<<n<<",\"kmax\":"<<k<<",\"budget\":"<<budget<<",\"filter_mode\":\"variance_collective\",\"domain\":\""<<domain<<"\",\"map_depth\":"<<depth<<",\"map_node_budget\":"<<node_budget<<",\"input_hash\":"<<input_hash
    <<",\"pool_hash\":"<<pool_hash<<",\"seeds\":"<<n-2<<",\"cover_sites\":"<<n
    <<",\"generation\":{\"grid_size\":"<<grid_size<<",\"permutation_keys\":"<<permutation_keys
    <<",\"permutation_sort_comparisons\":"<<permutation_comparisons<<",\"auxiliary_capacity_bytes\":"<<generation_auxiliary_bytes<<'}'
    <<",\"validation\":{\"fixture_point_checks\":"<<fixture_point_checks<<",\"owned_seed_checks\":"<<owned_seed_checks
    <<",\"pool_ids_checked\":"<<pool_ids_checked<<"},\"work\":{";
  dump_fields(work,forecast_fields);
  std::cout<<"},\"map_work\":{";dump_fields(map_work,map_fields);
  std::cout<<",\"domain\":{";dump_fields(map_work.domain,domain_fields);
  std::cout<<"}},\"q3_only_after_map\":"<<q3_only_after_map<<",\"both_rejected_by_map\":"<<both_rejected_by_map
    <<",\"forecast\":{\"q4_survivors_before_map\":"<<survivors_before_map
    <<",\"future_scan_lower_bound_before_map\":"<<static_cast<u64>(n)*survivors_before_map<<",\"q4_survivors\":"<<survivors<<",\"future_scan_lower_bound\":"<<lower_bound
    <<",\"formula\":\"cover_sites_times_q4_survivors\",\"includes_sorting\":false,\"measured_scan_work\":false}"
    <<",\"cloud_work\":{";dump_fields(cloud_work,cloud_fields);std::cout<<'}';
  std::cout<<",\"index_work\":{";dump_fields(index_work,index_fields);std::cout<<'}';
  std::cout<<",\"cover_work\":{";dump_fields(cover_work,cover_fields);std::cout<<'}';
  std::cout<<",\"pool_work\":{\"range_visits\":"<<pool_work.range_visits<<",\"selected_sites\":"<<pool_work.selected_sites<<'}'
    <<",\"memory\":{\"scope\":\"retained_capacities_not_RSS_shared_workspace_and_map\",\"id_bytes\":"<<sizeof(std::size_t)
    <<",\"input_capacity_bytes\":"<<input_bytes<<",\"cloud_retained_bytes\":"<<cloud_bytes
    <<",\"index_retained_bytes\":"<<index_bytes<<",\"cover_retained_bytes\":"<<cover_bytes<<",\"pool_retained_bytes\":"<<pool_bytes
    <<",\"workspace_retained_bytes\":"<<workspace_bytes<<",\"map_retained_bytes\":"<<map_retained_bytes<<",\"peak_live_buffer_bytes\":"<<peak_live_bytes<<'}'
    <<",\"timings\":{\"generation_ms\":"<<ms(started,generated)<<",\"fixture_validation_ms\":"<<ms(generated,fixture_checked)
    <<",\"cloud_ms\":"<<ms(fixture_checked,prepared)<<",\"index_ms\":"<<ms(prepared,indexed)
    <<",\"cover_ms\":"<<ms(indexed,covered)<<",\"pool_ms\":"<<ms(covered,pooled)
    <<",\"pool_validation_ms\":"<<ms(pooled,pool_checked)<<",\"filter_map_wall_ms\":"<<ms(pool_checked,filtered)<<",\"assessment_ms\":"<<assessment_ms<<",\"map_prepare_ms\":"<<map_prepare_ms<<",\"map_query_ms\":"<<map_query_ms
    <<",\"result_validation_ms\":"<<ms(filtered,checked)<<",\"release_ms\":"<<ms(checked,done)
    <<",\"auxiliary_total_ms\":"<<ms(started,done)<<"}}\n";
  return 0;
}
}
int main(int argc,char**argv) {
 try {
  if(argc!=6) throw std::invalid_argument("usage: dense_forecast 8000|16000|32000 K5_or_10 dense_prefix|dense_permuted disk|positive depth5_or_7");
  const auto n=number(argv[1]),k=number(argv[2]),depth=number(argv[5]);
  const std::string_view recipe(argv[3]);
  if((n!=8000&&n!=16000&&n!=32000)||(k!=5&&k!=10)||
      (recipe!="dense_prefix"&&recipe!="dense_permuted")||(depth!=5&&depth!=7))
    throw std::invalid_argument("outside auxiliary experiment plan");
  return dense_run(n,k,recipe,argv[4],static_cast<unsigned>(depth));
 }catch(const std::exception&error){std::cerr<<"dense center map forecast: "<<error.what()<<'\n';return 1;}
}
