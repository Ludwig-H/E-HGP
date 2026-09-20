// Explicit adaptation of tranche25's auxiliary dense_forecast.cpp. Outside
// the 163-source product inventory. Only fixture/CLI/JSON helpers are reused;
// the included probe's renamed main is never called. No dense census runs.
#define main mhgp8_auxiliary_unused_cover_probe_main
#include "../../../bench/q34_cover_probe.cpp"
#undef main
#include "lanes/q34_collective.hpp"

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

Q34PoolOptions forecast_options(std::string_view mode) {
  if(mode=="jung_universal") return {Q34ChordBound::Jung,Q34PoolReduction::Universal};
  if(mode=="jung_collective") return {Q34ChordBound::Jung,Q34PoolReduction::Collective};
  if(mode=="variance_universal") return {Q34ChordBound::Variance,Q34PoolReduction::Universal};
  if(mode=="variance_collective") return {Q34ChordBound::Variance,Q34PoolReduction::Collective};
  throw std::invalid_argument("unknown auxiliary filter mode");
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

int dense_run(std::size_t n,std::size_t k,std::size_t budget,std::string_view mode) {
  const auto options=forecast_options(mode);
  const auto started=Clock::now();
  std::vector<Point3> points{{900,1000,1000},{1100,1000,1000}};
  points.reserve(n);
  for(std::size_t i=0;points.size()<n;++i) {
    const auto layer=i/(41*21),offset=40+layer/2;
    points.push_back({static_cast<std::uint16_t>(980+i%41),
      static_cast<std::uint16_t>(1120+(i/41)%21),
      static_cast<std::uint16_t>(layer%2==0 ? 1000+offset : 1000-offset)});
  }
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
  std::size_t workspace_bytes=0;
  {
    Q34PoolWorkspace workspace; // Really shared across ALL n-2 assessments.
    for(std::size_t x=2;x<n;++x) {
      const auto result=assess_q34_family_pool(pool,x,k,options,workspace);
      require(result.work.seed_queries==1 && result.work.seed_owner_rejections==0,
              "owned dense assessment inactive");
      merge_forecast(work,result.work);
    }
    workspace_bytes=workspace.retained_bytes();
  } // Scratch release is included in the measured filter interval.
  const auto filtered=Clock::now();
  const auto cloud_work=cloud->work();
  const auto index_work=index->work();
  const auto cover_work=cover->work();
  const auto pool_work=pool->work();
  const auto input_bytes=points.capacity()*sizeof(Point3),cloud_bytes=cloud->retained_bytes();
  const auto index_bytes=index->retained_bytes(),cover_bytes=cover->retained_bytes(),pool_bytes=pool->retained_bytes();
  const auto survivors=n-2-work.q4_rejected;
  const u64 lower_bound=static_cast<u64>(n)*survivors;
  require(work.certificate_builds==n-2 && work.peak_event_bytes==workspace_bytes,
          "assessment preparation/capacity differs");
  const auto checked=Clock::now();
  pool.reset();cover.reset();index.reset();cloud.reset();std::vector<Point3>().swap(points);
  const auto done=Clock::now();
  std::cout<<std::setprecision(17)
    <<"{\"schema\":\"mhgp8_dense_collective_forecast_v1\",\"status\":\"completed\","
    <<"\"scope\":\"executed_product_pool_assessments_known_seeds_then_unexecuted_scan_lower_bound\","
    <<"\"public_status\":\"not_claimed\",\"backend\":\"cpu_reference\",\"profile\":\"quantized_u16_input_only\","
    <<"\"recipe\":\"dense_grid_prefix_41x21x38_v1\",\"threads\":1,\"edge_ids\":[0,1],"
    <<"\"timing_scope\":\"auxiliary_generation_validation_preparation_filter_release_excludes_json\","
    <<"\"family_sweeps_executed\":0,\"edge_generator_executed\":false,\"candidates_computed\":false,"
    <<"\"n\":"<<n<<",\"kmax\":"<<k<<",\"budget\":"<<budget<<",\"mode\":\""<<mode<<"\",\"input_hash\":"<<input_hash
    <<",\"pool_hash\":"<<pool_hash<<",\"seeds\":"<<n-2<<",\"cover_sites\":"<<n
    <<",\"validation\":{\"fixture_point_checks\":"<<fixture_point_checks<<",\"owned_seed_checks\":"<<owned_seed_checks
    <<",\"pool_ids_checked\":"<<pool_ids_checked<<"},\"work\":{";
  dump_fields(work,forecast_fields);
  std::cout<<"},\"forecast\":{\"q4_survivors\":"<<survivors<<",\"future_scan_lower_bound\":"<<lower_bound
    <<",\"formula\":\"cover_sites_times_q4_survivors\",\"includes_sorting\":false,\"measured_scan_work\":false}"
    <<",\"cloud_work\":{";dump_fields(cloud_work,cloud_fields);std::cout<<'}';
  std::cout<<",\"index_work\":{";dump_fields(index_work,index_fields);std::cout<<'}';
  std::cout<<",\"cover_work\":{";dump_fields(cover_work,cover_fields);std::cout<<'}';
  std::cout<<",\"pool_work\":{\"range_visits\":"<<pool_work.range_visits<<",\"selected_sites\":"<<pool_work.selected_sites<<'}'
    <<",\"memory\":{\"scope\":\"retained_capacities_not_RSS_shared_event_workspace\",\"id_bytes\":"<<sizeof(std::size_t)
    <<",\"input_capacity_bytes\":"<<input_bytes<<",\"cloud_retained_bytes\":"<<cloud_bytes
    <<",\"index_retained_bytes\":"<<index_bytes<<",\"cover_retained_bytes\":"<<cover_bytes<<",\"pool_retained_bytes\":"<<pool_bytes
    <<",\"workspace_retained_bytes\":"<<workspace_bytes<<'}'
    <<",\"timings\":{\"generation_ms\":"<<ms(started,generated)<<",\"fixture_validation_ms\":"<<ms(generated,fixture_checked)
    <<",\"cloud_ms\":"<<ms(fixture_checked,prepared)<<",\"index_ms\":"<<ms(prepared,indexed)
    <<",\"cover_ms\":"<<ms(indexed,covered)<<",\"pool_ms\":"<<ms(covered,pooled)
    <<",\"pool_validation_ms\":"<<ms(pooled,pool_checked)<<",\"filter_only_ms\":"<<ms(pool_checked,filtered)
    <<",\"result_validation_ms\":"<<ms(filtered,checked)<<",\"release_ms\":"<<ms(checked,done)
    <<",\"auxiliary_total_ms\":"<<ms(started,done)<<"}}\n";
  return 0;
}
}
int main(int argc,char**argv) {
 try {
  if(argc!=5) throw std::invalid_argument("usage: dense_forecast 8000|16000|32000 K5_or_10 C32_or_64 mode");
  const auto n=number(argv[1]),k=number(argv[2]),budget=number(argv[3]);
  if((n!=8000&&n!=16000&&n!=32000)||(k!=5&&k!=10)||(budget!=32&&budget!=64))
    throw std::invalid_argument("outside auxiliary experiment plan");
  return dense_run(n,k,budget,argv[4]);
 }catch(const std::exception&error){std::cerr<<"dense collective forecast: "<<error.what()<<'\n';return 1;}
}
