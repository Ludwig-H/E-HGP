// Independent paired audit at product31b0243a; actual edge APIs28/29, no composition.
// Explicit helper reuse: bench/q34_cover_probe.cpp SHA256
// d1bd34308488ac880cc6ea7290df1ecf487f635582cc669b1a9435fbb629f792.
#define main audit_unused_q34_cover_probe_main
#include "../../bench/q34_cover_probe.cpp"
#undef main
// Explicit independent rational judge, SHA256
// c65b607cd9a6b4883fbf6c25c5291e1a2c52fa733c983fe1d0d6ba1d87a4edf4.
#include "../../tests/exact_ball_oracle.hpp"
#include "lanes/q4_local.hpp"
#include "lanes/q4_shallow.hpp"
#include <fstream>
#include <map>
#include <string>

namespace {
namespace judge = mhgp8_test::ball_oracle;
Input read_lidar(const char* path) {
  std::ifstream stream(path);
  require(stream.good(),"cannot open input");
  std::string token;
  require(static_cast<bool>(stream>>token),"missing point count");
  const auto n=number(token);require(n>=2,"input needs edge IDs0,1");
  Input input;input.points.reserve(n);word(input.hash,n);
  for(std::size_t id=0;id<n;++id) {
    std::array<std::uint16_t,3> p{};
    for(std::size_t axis=0;axis<3;++axis) {
      require(static_cast<bool>(stream>>token),"truncated coordinates");
      const auto coordinate=number(token);require(coordinate<=65535,"coordinate outside u16");
      p[axis]=static_cast<std::uint16_t>(coordinate);word(input.hash,coordinate);
    }
    input.points.push_back({p[0],p[1],p[2]});
  }
  require(!(stream>>token),"trailing input token");require(stream.eof(),"input read error");
  return input;  // Uniqueness is checked once by the shared product cloud factory.
}
struct AuditJudgeWork {u64 supports{},owner_distance_tests{},distinct_balls{},point_tests{},shell_ids{},strict_interiors{};};
AuditJudgeWork judge_emissions(const Output& output,std::span<const Point3> points) {
  // Explicit adaptation of constructor29's emitted-ball judge, not a completeness oracle.
  // Cache the already validated record index; do not duplicate its shell payload.
  std::map<std::array<i128,5>,std::size_t> checked;
  AuditJudgeWork work;
  const auto distance=[](Point3 a,Point3 b) {
    i64 sum=0;for(std::size_t axis=0;axis<3;++axis) {const i64 d=static_cast<i64>(a[axis])-b[axis];sum+=d*d;}return sum;
  };
  const auto diameter=distance(points[0],points[1]);
  for(std::size_t position=0;position<output.records.size();++position) {
    const auto& r=output.records[position];
    require(r.arity==4 && r.support[0]==0 && r.support[1]==1,"emission has wrong arity/edge");
    std::array<Point3,4> support{};for(std::size_t i=0;i<4;++i) support[i]=points[r.support[i]];
    const auto reference=judge::make(support);
    require(reference.ball.has_value(),"nonpositive or singular emitted support");
    for(std::size_t i=0;i<5;++i) require(judge::Big(r.coefficients[i])==reference.ball->coefficients[i],"wrong exact ball key");
    for(std::size_t i=0;i<4;++i) for(std::size_t j=i+1;j<4;++j) {
      require(distance(support[i],support[j])<=diameter,"edge is not longest");counter_add(work.owner_distance_tests);
    } // Pair0,1 is lexicographically first among ALL IDs, hence wins every tie.
    counter_add(work.supports);
    const auto [found,inserted]=checked.try_emplace(r.coefficients,position);
    if(!inserted) {
      const auto& first=output.records[found->second];
      require(r.depth==first.depth && r.shell==first.shell,"same ball has inconsistent census");continue;
    }
    std::size_t depth=0,shell_position=0;
    const auto& c=reference.ball->coefficients;
    for(std::size_t id=0;id<points.size();++id) {
      const auto p=points[id];
      const judge::Big norm=judge::Big(p.x)*p.x+judge::Big(p.y)*p.y+judge::Big(p.z)*p.z;
      const judge::Big power=c[0]*norm+c[1]*p.x+c[2]*p.y+c[3]*p.z+c[4];
      if(power<0) ++depth;
      if(power==0) {require(shell_position<r.shell.size() && r.shell[shell_position]==id,"incomplete/wrong global shell");++shell_position;}
      counter_add(work.point_tests);
    }
    require(depth==r.depth && shell_position==r.shell.size(),"wrong global depth or shell");
    counter_add(work.distinct_balls);counter_add(work.shell_ids,static_cast<u64>(shell_position));counter_add(work.strict_interiors,static_cast<u64>(depth));
  }
  return work;
}
#define F(T,m) Field<T>{#m,&T::m}
constexpr std::array local_edge_fields{
  F(Q4LocalEdgeWork,node_visits),F(Q4LocalEdgeWork,bound_tests),F(Q4LocalEdgeWork,point_tests),F(Q4LocalEdgeWork,rejected_nodes),F(Q4LocalEdgeWork,split_nodes),
  F(Q4LocalEdgeWork,rejected_sites),F(Q4LocalEdgeWork,acute_seeds),F(Q4LocalEdgeWork,owner_tests),F(Q4LocalEdgeWork,owner_rejections),F(Q4LocalEdgeWork,seeds),F(Q4LocalEdgeWork,peak_live_buffer_bytes)};
constexpr std::array local_sweep_fields{
  F(Q4LocalSweepWork,seed_queries),F(Q4LocalSweepWork,seed_owner_tests),F(Q4LocalSweepWork,seed_owner_rejections),F(Q4LocalSweepWork,query_visits),F(Q4LocalSweepWork,line_tests),F(Q4LocalSweepWork,line_skips),F(Q4LocalSweepWork,leaf_queries),
  F(Q4LocalSweepWork,reference_points),F(Q4LocalSweepWork,reference_side_tests),F(Q4LocalSweepWork,active_blocks),F(Q4LocalSweepWork,active_sites),F(Q4LocalSweepWork,root_locations),F(Q4LocalSweepWork,clipped_events),F(Q4LocalSweepWork,clipped_inside),
  F(Q4LocalSweepWork,kept_events),F(Q4LocalSweepWork,constant_inside),F(Q4LocalSweepWork,constant_outside),F(Q4LocalSweepWork,constant_shell_ids),F(Q4LocalSweepWork,entries),F(Q4LocalSweepWork,exits),
  F(Q4LocalSweepWork,sort_comparisons),F(Q4LocalSweepWork,shell_sort_comparisons),F(Q4LocalSweepWork,group_comparisons),F(Q4LocalSweepWork,groups),F(Q4LocalSweepWork,boundary_skips),F(Q4LocalSweepWork,boundary_skipped_ids),
  F(Q4LocalSweepWork,depth_rejections),F(Q4LocalSweepWork,depth_skipped_ids),F(Q4LocalSweepWork,presentations),F(Q4LocalSweepWork,owner_tests),F(Q4LocalSweepWork,owner_rejections),F(Q4LocalSweepWork,positive_tests),F(Q4LocalSweepWork,positive_rejections),
  F(Q4LocalSweepWork,canonical_tests),F(Q4LocalSweepWork,canonical_rejections),F(Q4LocalSweepWork,emitted),F(Q4LocalSweepWork,shell_ids),F(Q4LocalSweepWork,groups_without_support),F(Q4LocalSweepWork,unexamined_after_emit),F(Q4LocalSweepWork,max_group),F(Q4LocalSweepWork,peak_buffer_bytes)};
constexpr std::array atlas_fields{
  F(Q4LocalAtlasWork,cells_created),F(Q4LocalAtlasWork,outside_cells),F(Q4LocalAtlasWork,deep_cells),F(Q4LocalAtlasWork,leaf_cells),F(Q4LocalAtlasWork,splits),F(Q4LocalAtlasWork,depth_stops),F(Q4LocalAtlasWork,node_stops),F(Q4LocalAtlasWork,small_stops),
  F(Q4LocalAtlasWork,active_sites_sum),F(Q4LocalAtlasWork,active_blocks_sum),F(Q4LocalAtlasWork,terminal_refinements),F(Q4LocalAtlasWork,terminal_deep_cells),F(Q4LocalAtlasWork,max_depth),F(Q4LocalAtlasWork,peak_fragment_bytes),F(Q4LocalAtlasWork,peak_build_bytes),F(Q4LocalAtlasWork,retained_bytes)};
constexpr std::array partition_fields{
  F(Q4LocalPartitionWork,root_factories),F(Q4LocalPartitionWork,child_factories),F(Q4LocalPartitionWork,refine_factories),F(Q4LocalPartitionWork,input_nodes),F(Q4LocalPartitionWork,input_sites),F(Q4LocalPartitionWork,inherited_inside_sites),
  F(Q4LocalPartitionWork,node_visits),F(Q4LocalPartitionWork,block_bound_tests),F(Q4LocalPartitionWork,point_tests),F(Q4LocalPartitionWork,z_splits),F(Q4LocalPartitionWork,inside_nodes),F(Q4LocalPartitionWork,outside_nodes),F(Q4LocalPartitionWork,inside_sites),F(Q4LocalPartitionWork,outside_sites),
  F(Q4LocalPartitionWork,active_nodes),F(Q4LocalPartitionWork,active_sites),F(Q4LocalPartitionWork,budget_unexamined_nodes),F(Q4LocalPartitionWork,budget_ambiguous_nodes),F(Q4LocalPartitionWork,frontier_ids_copied),F(Q4LocalPartitionWork,peak_retained_bytes)};
constexpr std::array geometry_fields{
  F(Q4LocalGeometryWork,preparations),F(Q4LocalGeometryWork,cover_node_visits),F(Q4LocalGeometryWork,cover_range_advances),F(Q4LocalGeometryWork,cover_disjoint_nodes),F(Q4LocalGeometryWork,cover_splits),F(Q4LocalGeometryWork,cover_blocks),
  F(Q4LocalGeometryWork,cover_sites),F(Q4LocalGeometryWork,cover_excluded_sites),F(Q4LocalGeometryWork,cover_node_ids_copied),F(Q4LocalGeometryWork,projection_points),F(Q4LocalGeometryWork,hull_sort_comparisons),F(Q4LocalGeometryWork,hull_orientation_tests),F(Q4LocalGeometryWork,hull_vertices),F(Q4LocalGeometryWork,facets),F(Q4LocalGeometryWork,peak_retained_bytes)};
constexpr std::array domain_fields{
  F(Q4PositiveDomainWork,node_visits),F(Q4PositiveDomainWork,bound_tests),F(Q4PositiveDomainWork,endpoint_box_tests),F(Q4PositiveDomainWork,endpoint_leaf_tests),F(Q4PositiveDomainWork,point_tests),F(Q4PositiveDomainWork,admitted_nodes),
  F(Q4PositiveDomainWork,rejected_nodes),F(Q4PositiveDomainWork,split_nodes),F(Q4PositiveDomainWork,excluded_endpoints),F(Q4PositiveDomainWork,admitted_sites),F(Q4PositiveDomainWork,rejected_sites),F(Q4PositiveDomainWork,box_merges)};
constexpr std::array selection_fields{
  F(Q4ShallowSetWork,preparations),F(Q4ShallowSetWork,input_sites),F(Q4ShallowSetWork,form_tests),F(Q4ShallowSetWork,zero_sites),F(Q4ShallowSetWork,positive_sites),F(Q4ShallowSetWork,negative_sites),
  F(Q4ShallowSetWork,lex_comparisons),F(Q4ShallowSetWork,orientation_tests),F(Q4ShallowSetWork,coordinate_groups),F(Q4ShallowSetWork,duplicate_ids),F(Q4ShallowSetWork,positive_layers),F(Q4ShallowSetWork,negative_layers),
  F(Q4ShallowSetWork,layer_input_groups),F(Q4ShallowSetWork,layer_input_ids),F(Q4ShallowSetWork,boundary_groups),F(Q4ShallowSetWork,degenerate_groups),F(Q4ShallowSetWork,retained_ids),F(Q4ShallowSetWork,discarded_ids),
  F(Q4ShallowSetWork,retained_id_sort_comparisons),F(Q4ShallowSetWork,record_insertions),F(Q4ShallowSetWork,group_insertions),F(Q4ShallowSetWork,hull_index_copies),F(Q4ShallowSetWork,compaction_moves),F(Q4ShallowSetWork,peak_live_bytes),F(Q4ShallowSetWork,retained_bytes)};
constexpr std::array shallow_sweep_fields{
  F(Q4ShallowSweepWork,seed_queries),F(Q4ShallowSweepWork,seed_owner_tests),F(Q4ShallowSweepWork,seed_owner_rejections),F(Q4ShallowSweepWork,removed_seed_rejections),F(Q4ShallowSweepWork,membership_comparisons),F(Q4ShallowSweepWork,depth_rejected_groups),F(Q4ShallowSweepWork,depth_skipped_ids),
  F(Q4ShallowSweepWork,presentations),F(Q4ShallowSweepWork,owner_tests),F(Q4ShallowSweepWork,owner_rejections),F(Q4ShallowSweepWork,positive_tests),F(Q4ShallowSweepWork,positive_rejections),F(Q4ShallowSweepWork,canonical_tests),F(Q4ShallowSweepWork,canonical_rejections),
  F(Q4ShallowSweepWork,groups_without_support),F(Q4ShallowSweepWork,unexamined_after_emit),F(Q4ShallowSweepWork,emitted),F(Q4ShallowSweepWork,shell_ids),F(Q4ShallowSweepWork,peak_buffer_bytes)};
constexpr std::array shallow_edge_fields{
  F(Q4ShallowEdgeWork,seed_candidates),F(Q4ShallowEdgeWork,acute_tests),F(Q4ShallowEdgeWork,acute_seeds),F(Q4ShallowEdgeWork,owner_tests),F(Q4ShallowEdgeWork,owner_rejections),F(Q4ShallowEdgeWork,seeds),F(Q4ShallowEdgeWork,peak_live_buffer_bytes)};
constexpr std::array judge_fields{F(AuditJudgeWork,supports),F(AuditJudgeWork,owner_distance_tests),F(AuditJudgeWork,distinct_balls),F(AuditJudgeWork,point_tests),F(AuditJudgeWork,shell_ids),F(AuditJudgeWork,strict_interiors)};
#undef F
#define FULL(T,a,extra) static_assert(sizeof(T)==a.size()*sizeof(u64)+extra)
FULL(Q4LocalSweepWork,local_sweep_fields,0);FULL(Q4LocalPartitionWork,partition_fields,0);FULL(Q4ShallowSetWork,selection_fields,0);
FULL(Q4LocalAtlasWork,atlas_fields,sizeof(Q4LocalPartitionWork)+sizeof(Q4LocalGeometryQueryWork));
FULL(Q4LocalGeometryWork,geometry_fields,sizeof(Q4PositiveDomainWork));FULL(Q4PositiveDomainWork,domain_fields,0);
FULL(Q4LocalEdgeWork,local_edge_fields,sizeof(Q4LocalAtlasWork)+sizeof(Q4LocalGeometryWork)+sizeof(Q4LocalSweepWork));
FULL(Q4ShallowSweepWork,shallow_sweep_fields,sizeof(Q4FamilyWork));
FULL(Q4ShallowEdgeWork,shallow_edge_fields,sizeof(Q4LocalGeometryWork)+sizeof(Q4ShallowSetWork)+sizeof(Q4ShallowSweepWork));
#undef FULL
template<class T,std::size_t N> void audit_dump(const T& work,const std::array<Field<T>,N>& fields) {std::cout<<'{';dump_fields(work,fields);std::cout<<'}';}
void dump_geometry(const Q4LocalGeometryWork& work) {
  std::cout<<'{';dump_fields(work,geometry_fields);std::cout<<",\"domain\":";audit_dump(work.domain,domain_fields);std::cout<<'}';
}
void dump_records(const Output& output) {
  std::cout<<'[';
  for(std::size_t pos=0;pos<output.records.size();++pos) {
    if(pos!=0) std::cout<<',';
    const auto& r=output.records[pos];std::cout<<"{\"arity\":"<<r.arity<<",\"depth\":"<<r.depth<<",\"support\":[";
    for(unsigned i=0;i<r.arity;++i) {if(i!=0)std::cout<<',';std::cout<<r.support[i];}
    std::cout<<"],\"coefficients\":[";
    for(std::size_t i=0;i<5;++i) {if(i!=0)std::cout<<',';std::cout<<'"'<<judge::Big(r.coefficients[i])<<'"';}
    std::cout<<"],\"shell\":[";for(std::size_t i=0;i<r.shell.size();++i) {if(i!=0)std::cout<<',';std::cout<<r.shell[i];}std::cout<<"]}";
  }
  std::cout<<']';
}
int audit_run(int argc,char** argv) {
  require(argc==3,"usage: lidar_product_probe input.txt K");const auto k=number(argv[2]);
  require(k>=3 && k<=std::numeric_limits<std::size_t>::max()-2,"audit K must be3..SIZE_MAX-2 (Output helper arithmetic)");
  const auto start=Clock::now();const auto input=read_lidar(argv[1]);const auto read=Clock::now();
  const auto cloud=prepare_cloud(input.points);const auto prepared=Clock::now();
  const auto index=make_q2_cloud_index(cloud);const auto indexed=Clock::now();
  const auto cover=Q34EdgeCover::make(index,{0,1});const auto covered=Clock::now();
  const Q4LocalOptions options{Q4CenterDomainMode::Positive,7,4096,512,32,true};Output local,shallow;
  const auto work28=run_q4_local_edge_candidates(cover,k,options,[&](const auto& c){local.collect(c,input.points.size(),k);});const auto done28=Clock::now();
  const auto work29=run_q4_shallow_edge_candidates(cover,k,[&](const auto& c){shallow.collect(c,input.points.size(),k);});const auto done29=Clock::now();
  std::sort(local.records.begin(),local.records.end());std::sort(shallow.records.begin(),shallow.records.end());
  require(local.records==shallow.records,"28/29 exact record comparison failed");
  require(std::adjacent_find(local.records.begin(),local.records.end())==local.records.end(),"duplicate paired record");
  const auto compared=Clock::now();
  const auto judgement=judge_emissions(local,input.points);const auto judged=Clock::now();
  local.finish();shallow.finish();require(local.hash==shallow.hash,"paired digest mismatch after exact comparison");const auto done=Clock::now();
  std::cout<<std::fixed<<std::setprecision(6)<<"{\"schema\":\"mhgp8_audit_lidar_product_pair_v1\",\"status\":\"passed\",\"product_commit\":\"31b0243a\","
    "\"public_status\":\"not_claimed\",\"completeness_large_cloud_claimed\":false,\"edge_ids\":[0,1],\"n\":"<<input.points.size()<<",\"kmax\":"<<k
    <<",\"input_fnv1a64_u64le_n_xyz\":"<<input.hash<<",\"cover_sites\":"<<cover->site_count()
    <<",\"local_options\":{\"domain\":\"Positive\",\"depth\":7,\"node_budget\":4096,\"z_test_budget\":512,\"leaf_sites\":32,\"clip_events\":true}"
    <<",\"cloud_work\":";audit_dump(cloud->work(),cloud_fields);std::cout<<",\"index_work\":";audit_dump(index->work(),index_fields);
  std::cout<<",\"cover_work\":";audit_dump(cover->work(),cover_fields);
  std::cout<<",\"local28\":{\"edge\":";audit_dump(work28,local_edge_fields);std::cout<<",\"geometry\":";dump_geometry(work28.geometry);
  std::cout<<",\"atlas\":{";dump_fields(work28.atlas,atlas_fields);std::cout<<",\"partition\":";audit_dump(work28.atlas.partition,partition_fields);
  std::cout<<",\"domain\":{\"disk_tests\":"<<work28.atlas.domain.disk_tests<<",\"facet_tests\":"<<work28.atlas.domain.facet_tests<<"}}";
  std::cout<<",\"sweep\":";audit_dump(work28.sweep,local_sweep_fields);std::cout<<",\"output\":";local.dump();std::cout<<'}';
  std::cout<<",\"shallow29\":{\"edge\":";audit_dump(work29,shallow_edge_fields);std::cout<<",\"geometry\":";dump_geometry(work29.geometry);
  std::cout<<",\"selection\":";audit_dump(work29.selection,selection_fields);std::cout<<",\"sweep\":{";dump_fields(work29.sweep,shallow_sweep_fields);
  std::cout<<",\"family\":";audit_dump(work29.sweep.family,family_fields);std::cout<<"},\"output\":";shallow.dump();std::cout<<'}';
  std::cout<<",\"judge\":";audit_dump(judgement,judge_fields);std::cout<<",\"records\":";dump_records(local);
  std::cout<<",\"memory\":{\"input_point_capacity_bytes\":"<<input.points.capacity()*sizeof(Point3)<<",\"cloud_retained_bytes\":"<<cloud->retained_bytes()
    <<",\"index_retained_bytes\":"<<index->retained_bytes()<<",\"cover_retained_bytes\":"<<cover->retained_bytes()
    <<",\"local_output_retained_bytes\":"<<local.retained_bytes()<<",\"shallow_output_retained_bytes\":"<<shallow.retained_bytes()
    <<",\"scope\":\"Separate capacities, not RSS; both audit outputs coexist during29; product peaks exclude these outputs and shared cloud/index/cover; judge map and allocator metadata excluded\"}"
    <<",\"timings_ms\":{\"read_hash\":"<<ms(start,read)<<",\"cloud\":"<<ms(read,prepared)<<",\"index\":"<<ms(prepared,indexed)<<",\"cover\":"<<ms(indexed,covered)
    <<",\"local28_edge_and_collect\":"<<ms(covered,done28)<<",\"shallow29_edge_and_collect\":"<<ms(done28,done29)
    <<",\"sort_exact_compare\":"<<ms(done29,compared)<<",\"independent_judge\":"<<ms(compared,judged)<<",\"finish_digest\":"<<ms(judged,done)<<",\"total_before_json\":"<<ms(start,done)
    <<"},\"timing_scope\":\"Serial28 then29; each edge time includes its own preparation, census, callbacks and private destruction; independent judge covers each emitted support and scans n sites once per distinct emitted ball; JSON and final shared/output destruction excluded\"}\n";
  return 0;
}
} // namespace
int main(int argc,char** argv) {
  try {return audit_run(argc,argv);}catch(const std::exception& error) {std::cerr<<"audit LiDAR pair: "<<error.what()<<'\n';return 1;}
}
