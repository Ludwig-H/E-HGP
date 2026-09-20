// Auxiliary experiment, OUTSIDE the 158-source product qualification.
// Explicitly reuse only fixture-independent field/digest/CLI helpers. The
// included old probe's renamed entry is never called by this executable.
#define main mhgp8_auxiliary_unused_cover_probe_main
#include "../../../bench/q34_cover_probe.cpp"
#undef main
#include "lanes/family_certificate.hpp"
#include "lanes/q34_pruning.hpp"

namespace {
int dense_run(std::size_t n, std::size_t k, std::size_t budget) {
  const auto started = Clock::now();
  std::vector<Point3> points{{900,1000,1000},{1100,1000,1000}};
  points.reserve(n);
  for (std::size_t i=0; points.size()<n; ++i) {
    const auto layer=i/(41*21), offset=40+layer/2;
    points.push_back({static_cast<std::uint16_t>(980+i%41),
      static_cast<std::uint16_t>(1120+(i/41)%21),
      static_cast<std::uint16_t>(layer%2==0 ? 1000+offset : 1000-offset)});
  }
  u64 input_hash=14695981039346656037ULL;
  word(input_hash,n);
  for(auto p:points) for(unsigned axis=0;axis<3;++axis) word(input_hash,p[axis]);
  const auto generated=Clock::now();
  u64 fixture_point_checks=0, owned_seed_checks=0;
  // Independent scalar inequalities certify every known seed: acute,
  // STRICTLY longer owner edge, and contained in the complete edge cover.
  // The rectangular integer grid itself guarantees uniqueness; PreparedCloud
  // independently validates uniqueness as part of the charged preparation.
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
  require(cover->site_count()==n,"cover failed to contain all dense sites");
  auto pool=Q34WitnessPool::make(cover,budget);
  const auto pooled=Clock::now();
  require(pool->ids().size()==budget,"pool size differs");
  u64 pool_hash=14695981039346656037ULL,pool_ids_checked=0;
  const auto order=index->spatial_order();
  // This fixture's entire root is covered: ranges must merge to [0,n).
  require(cover->ranges().size()==1 && cover->ranges()[0].first==0 && cover->ranges()[0].last==n,
          "dense cover is not the complete root range");
  for(std::size_t i=0;i<budget;++i) {
    require(pool->ids()[i]==order[i*n/budget],"auxiliary pool does not match spatial quantiles");
    word(pool_hash,pool->ids()[i]); ++pool_ids_checked;
  }
  const auto pool_checked=Clock::now();
  u64 killed3=0,killed4=0,both=0,tests=0,iterations=0,credits3=0,credits4=0,certificates=0;
  for(std::size_t x=2;x<n;++x) {
    const auto certificate=Q34FamilyCertificate::make(points[0],points[1],points[x]);
    require(certificate.has_value(),"owned acute seed has no certificate");
    ++certificates; iterations+=certificate->sqrt_iterations();
    u64 c3=0,c4=0;
    for(auto id:pool->ids()) {
      const auto witness=certificate->witness(points[id]);
      c3+=witness.q3 && c3<k-1;
      c4+=witness.q4 && c4<k-2;
      ++tests;
      if(c3==k-1 && c4==k-2) break;
    }
    killed3+=c3==k-1; killed4+=c4==k-2; both+=c3==k-1 && c4==k-2;
    credits3+=c3; credits4+=c4;
  }
  const auto certified=Clock::now();
  const auto cloud_work=cloud->work();
  const auto index_work=index->work();
  const auto cover_work=cover->work();
  const auto pool_work=pool->work();
  const auto input_bytes=points.capacity()*sizeof(Point3),cloud_bytes=cloud->retained_bytes();
  const auto index_bytes=index->retained_bytes(),cover_bytes=cover->retained_bytes(),pool_bytes=pool->retained_bytes();
  const auto survivors=n-2-killed4;
  const u64 lower_bound=static_cast<u64>(n)*survivors;
  require(certificates==n-2 && killed3>=both && killed4>=both,"certificate result partition failed");
  const auto checked=Clock::now();
  pool.reset();cover.reset();index.reset();cloud.reset();std::vector<Point3>().swap(points);
  const auto done=Clock::now();
  std::cout<<std::setprecision(17)
    <<"{\"schema\":\"mhgp8_dense_family_forecast_v1\",\"status\":\"completed\","
    <<"\"scope\":\"executed_certificate_replay_known_seeds_then_unexecuted_scan_lower_bound\","
    <<"\"public_status\":\"not_claimed\",\"backend\":\"cpu_reference\",\"profile\":\"quantized_u16_input_only\","
    <<"\"recipe\":\"dense_grid_prefix_41x21x38_v1\",\"threads\":1,\"edge_ids\":[0,1],"
    <<"\"timing_scope\":\"auxiliary_generation_validation_preparation_certificates_release_excludes_json\","
    <<"\"family_sweeps_executed\":0,\"edge_generator_executed\":false,\"candidates_computed\":false,"
    <<"\"n\":"<<n<<",\"kmax\":"<<k<<",\"budget\":"<<budget<<",\"input_hash\":"<<input_hash
    <<",\"pool_hash\":"<<pool_hash<<",\"seeds\":"<<n-2<<",\"cover_sites\":"<<n
    <<",\"validation\":{\"fixture_point_checks\":"<<fixture_point_checks<<",\"owned_seed_checks\":"<<owned_seed_checks
    <<",\"pool_ids_checked\":"<<pool_ids_checked<<"},\"work\":{\"certificates\":"<<certificates
    <<",\"sqrt_iterations\":"<<iterations<<",\"paired_predicate_tests\":"<<tests
    <<",\"q3_credits\":"<<credits3<<",\"q4_credits\":"<<credits4<<",\"q3_rejected\":"<<killed3
    <<",\"q4_rejected\":"<<killed4<<",\"both_rejected\":"<<both
    <<",\"q3_only_survivors\":"<<killed4-both<<",\"q4_only_survivors\":"<<killed3-both
    <<",\"both_survivors\":"<<n-2-killed3-killed4+both
    <<"},\"forecast\":{\"q4_survivors\":"<<survivors<<",\"future_scan_lower_bound\":"<<lower_bound
    <<",\"formula\":\"cover_sites_times_q4_survivors\",\"includes_sorting\":false,\"measured_scan_work\":false}"
    <<",\"cloud_work\":{";dump_fields(cloud_work,cloud_fields);std::cout<<'}';
  std::cout<<",\"index_work\":{";dump_fields(index_work,index_fields);std::cout<<'}';
  std::cout<<",\"cover_work\":{";dump_fields(cover_work,cover_fields);std::cout<<'}';
  std::cout<<",\"pool_work\":{\"range_visits\":"<<pool_work.range_visits<<",\"selected_sites\":"<<pool_work.selected_sites<<'}'
    <<",\"memory\":{\"scope\":\"retained_capacities_not_RSS_no_event_buffers\",\"id_bytes\":"<<sizeof(std::size_t)
    <<",\"input_capacity_bytes\":"<<input_bytes<<",\"cloud_retained_bytes\":"<<cloud_bytes
    <<",\"index_retained_bytes\":"<<index_bytes<<",\"cover_retained_bytes\":"<<cover_bytes<<",\"pool_retained_bytes\":"<<pool_bytes<<'}'
    <<",\"timings\":{\"generation_ms\":"<<ms(started,generated)<<",\"fixture_validation_ms\":"<<ms(generated,fixture_checked)
    <<",\"cloud_ms\":"<<ms(fixture_checked,prepared)<<",\"index_ms\":"<<ms(prepared,indexed)
    <<",\"cover_ms\":"<<ms(indexed,covered)<<",\"pool_ms\":"<<ms(covered,pooled)
    <<",\"pool_validation_ms\":"<<ms(pooled,pool_checked)<<",\"certificate_only_ms\":"<<ms(pool_checked,certified)
    <<",\"result_validation_ms\":"<<ms(certified,checked)<<",\"release_ms\":"<<ms(checked,done)
    <<",\"auxiliary_total_ms\":"<<ms(started,done)<<"}}\n";
  return 0;
}
}
int main(int argc,char**argv) {
 try {
  if(argc!=4) throw std::invalid_argument("usage: dense_forecast 8000|16000|32000 K5_or_10 C32_or_64");
  const auto n=number(argv[1]),k=number(argv[2]),budget=number(argv[3]);
  if((n!=8000&&n!=16000&&n!=32000)||(k!=5&&k!=10)||(budget!=32&&budget!=64))
    throw std::invalid_argument("outside auxiliary experiment plan");
  return dense_run(n,k,budget);
 }catch(const std::exception&error){std::cerr<<"dense forecast: "<<error.what()<<'\n';return 1;}
}
