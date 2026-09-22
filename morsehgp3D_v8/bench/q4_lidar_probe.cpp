// Constructor31 explicit port of audits/q4_kernel_composition_20260920/
// lidar_product_probe.cpp: file reader, work tables and emitted-ball judge.
// Audit results are NOT inherited. Three actual product edge APIs are compared.
// Shared constructor24 helpers remain explicit; no audit source is included.
#define main mhgp8_unused_q34_cover_probe_main
#include "q34_cover_probe.cpp"
#undef main
// Explicit independent rational judge, SHA256
// c65b607cd9a6b4883fbf6c25c5291e1a2c52fa733c983fe1d0d6ba1d87a4edf4.
#include "../tests/exact_ball_oracle.hpp"
#include "lanes/q4_local.hpp"
#include "lanes/q4_shallow.hpp"
#include "lanes/q4_window.hpp"
#include <fstream>
#include <initializer_list>
#include <map>
#include <string>

namespace {
namespace judge = mhgp8_test::ball_oracle;
// Binary containers without header: ".u16le" (6 bytes per site, three
// little-endian u16) and ".u32le" (12 bytes per site, three little-endian
// u32 whose values must be below 2^18). The input hash covers the integer
// values, not the container: the same cloud hashes identically in both.
Input read_lidar(const char* path) {
  const bool u16=std::string_view(path).ends_with(".u16le");
  const bool u32=std::string_view(path).ends_with(".u32le");
  if(u16 || u32) {
    const std::size_t width=u16?2:4,stride=3*width;
    std::ifstream stream(path,std::ios::binary|std::ios::ate);
    require(stream.good(),"cannot open binary input");
    const auto length=static_cast<std::streamoff>(stream.tellg());
    require(length>=static_cast<std::streamoff>(2*stride) && length%static_cast<std::streamoff>(stride)==0,
        u16?"u16le input must contain 6*n bytes, n>=2":"u32le input must contain 12*n bytes, n>=2");
    const auto count=static_cast<std::uintmax_t>(length/static_cast<std::streamoff>(stride));
    require(count<=std::numeric_limits<std::size_t>::max(),"binary point count too large");
    const auto n=static_cast<std::size_t>(count);
    stream.seekg(0,std::ios::beg);require(stream.good(),"binary input seek failed");
    Input input;input.points.reserve(n);word(input.hash,n);
    for(std::size_t id=0;id<n;++id) {
      std::array<char,12> bytes{};
      require(static_cast<bool>(stream.read(bytes.data(),static_cast<std::streamsize>(stride))),"truncated binary coordinates");
      std::array<Coordinate,3> p{};
      for(std::size_t axis=0;axis<3;++axis) {
        unsigned long value=0;
        for(std::size_t byte=0;byte<width;++byte)
          value|=static_cast<unsigned long>(static_cast<unsigned char>(bytes[width*axis+byte]))<<(8U*byte);
        require(value<=static_cast<unsigned long>(coordinate_limit),"coordinate outside [0, 2^18)");
        p[axis]=static_cast<Coordinate>(value);
        word(input.hash,p[axis]);
      }
      input.points.push_back({p[0],p[1],p[2]});
    }
    char extra{};stream.read(&extra,1);
    require(stream.gcount()==0 && stream.eof() && !stream.bad(),"binary input changed or read failed");
    return input; // No reorder, quantization or deduplication in this reader.
  }
  std::ifstream stream(path);
  require(stream.good(),"cannot open input");
  std::string token;
  require(static_cast<bool>(stream>>token),"missing point count");
  const auto n=number(token);require(n>=2,"input needs at least two sites");
  Input input;input.points.reserve(n);word(input.hash,n);
  for(std::size_t id=0;id<n;++id) {
    std::array<Coordinate,3> p{};
    for(std::size_t axis=0;axis<3;++axis) {
      require(static_cast<bool>(stream>>token),"truncated coordinates");
      const auto coordinate=number(token);
      require(coordinate<=static_cast<std::size_t>(coordinate_limit),"coordinate outside [0, 2^18)");
      p[axis]=static_cast<Coordinate>(coordinate);word(input.hash,coordinate);
    }
    input.points.push_back({p[0],p[1],p[2]});
  }
  require(!(stream>>token),"trailing input token");require(stream.eof() && !stream.bad(),"input read error");
  return input;  // Uniqueness is checked once by the shared product cloud factory.
}
// Published input profile: the historical 2 cm profile when every coordinate
// fits 16 bits, the 18-bit profile otherwise. Same engine, same proofs.
const char* input_profile(const Input& input) {
  for(const auto point:input.points) for(std::size_t axis=0;axis<3;++axis)
    if(point[axis]>65535) return "quantized_u18_input_only";
  return "quantized_u16_input_only";
}
const char* input_format(const char* path) {
  const std::string_view view(path);
  return view.ends_with(".u16le")?"u16le_xyz_no_header":view.ends_with(".u32le")?"u32le_xyz_no_header":"text_count_xyz";
}
struct LidarJudgeWork {u64 supports{},owner_distance_tests{},distinct_balls{},point_tests{},shell_ids{},strict_interiors{};};
LidarJudgeWork judge_emissions(const Output& output,std::span<const Point3> points,
    std::array<std::size_t,2> edge) {
  // Explicit adaptation of constructor29's emitted-ball judge, not a completeness oracle.
  // Cache the already validated record index; do not duplicate its shell payload.
  std::map<std::array<i128,5>,std::size_t> checked;
  LidarJudgeWork work;
  const auto distance=[](Point3 a,Point3 b) {
    i64 sum=0;for(std::size_t axis=0;axis<3;++axis) {const i64 d=static_cast<i64>(a[axis])-b[axis];sum+=d*d;}return sum;
  };
  const auto diameter=distance(points[edge[0]],points[edge[1]]);
  for(std::size_t position=0;position<output.records.size();++position) {
    const auto& r=output.records[position];
    require(r.arity==4 && std::binary_search(r.support.begin(),r.support.end(),edge[0]) &&
        std::binary_search(r.support.begin(),r.support.end(),edge[1]),"emission has wrong arity/edge");
    std::array<Point3,4> support{};for(std::size_t i=0;i<4;++i) support[i]=points[r.support[i]];
    const auto reference=judge::make(support);
    require(reference.ball.has_value(),"nonpositive or singular emitted support");
    for(std::size_t i=0;i<5;++i) require(judge::Big(r.coefficients[i])==reference.ball->coefficients[i],"wrong exact ball key");
    for(std::size_t i=0;i<4;++i) for(std::size_t j=i+1;j<4;++j) {
      const auto length=distance(support[i],support[j]);
      const std::array<std::size_t,2> pair{r.support[i],r.support[j]};
      require(length<diameter || (length==diameter && edge<=pair),
              "edge is not the original-ID lexicographic longest owner");
      counter_add(work.owner_distance_tests);
    }
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
constexpr std::array judge_fields{F(LidarJudgeWork,supports),F(LidarJudgeWork,owner_distance_tests),F(LidarJudgeWork,distinct_balls),F(LidarJudgeWork,point_tests),F(LidarJudgeWork,shell_ids),F(LidarJudgeWork,strict_interiors)};
constexpr std::array window_fields{
  F(Q4WindowSelectionWork,seed_queries),F(Q4WindowSelectionWork,entry_heap_insertions),F(Q4WindowSelectionWork,exit_heap_insertions),
  F(Q4WindowSelectionWork,entry_heap_replacements),F(Q4WindowSelectionWork,exit_heap_replacements),F(Q4WindowSelectionWork,heap_comparisons),F(Q4WindowSelectionWork,heap_sort_comparisons),
  F(Q4WindowSelectionWork,constant_rejected_seeds),F(Q4WindowSelectionWork,disjoint_rejected_seeds),F(Q4WindowSelectionWork,fixed_depth_rejected_seeds),
  F(Q4WindowSelectionWork,lower_bounds),F(Q4WindowSelectionWork,upper_bounds),F(Q4WindowSelectionWork,point_windows),F(Q4WindowSelectionWork,second_pass_sites),F(Q4WindowSelectionWork,window_comparisons),
  F(Q4WindowSelectionWork,lower_ids),F(Q4WindowSelectionWork,upper_ids),F(Q4WindowSelectionWork,inner_ids),F(Q4WindowSelectionWork,outside_ids),F(Q4WindowSelectionWork,fixed_inside_sites),F(Q4WindowSelectionWork,rejected_event_ids),
  F(Q4WindowSelectionWork,max_inner_ids),F(Q4WindowSelectionWork,max_endpoint_ids),F(Q4WindowSelectionWork,peak_heap_bytes),F(Q4WindowSelectionWork,peak_buffer_bytes)};
constexpr std::array window_edge_fields{
  F(Q4WindowEdgeWork,seed_candidates),F(Q4WindowEdgeWork,acute_tests),F(Q4WindowEdgeWork,acute_seeds),F(Q4WindowEdgeWork,owner_tests),F(Q4WindowEdgeWork,owner_rejections),F(Q4WindowEdgeWork,seeds),F(Q4WindowEdgeWork,peak_live_buffer_bytes)};
#undef F
#define FULL(T,a,extra) static_assert(sizeof(T)==a.size()*sizeof(u64)+extra)
FULL(Q4LocalSweepWork,local_sweep_fields,0);FULL(Q4LocalPartitionWork,partition_fields,0);FULL(Q4ShallowSetWork,selection_fields,0);
FULL(Q4LocalAtlasWork,atlas_fields,sizeof(Q4LocalPartitionWork)+sizeof(Q4LocalGeometryQueryWork));
FULL(Q4LocalGeometryWork,geometry_fields,sizeof(Q4PositiveDomainWork));FULL(Q4PositiveDomainWork,domain_fields,0);
static_assert(sizeof(Q4LocalGeometryQueryWork)==2*sizeof(u64));
FULL(Q4LocalEdgeWork,local_edge_fields,sizeof(Q4LocalAtlasWork)+sizeof(Q4LocalGeometryWork)+sizeof(Q4LocalSweepWork));
FULL(Q4ShallowSweepWork,shallow_sweep_fields,sizeof(Q4FamilyWork));
FULL(Q4ShallowEdgeWork,shallow_edge_fields,sizeof(Q4LocalGeometryWork)+sizeof(Q4ShallowSetWork)+sizeof(Q4ShallowSweepWork));
FULL(Q4WindowSelectionWork,window_fields,0);
FULL(Q4WindowSweepWork,window_fields,sizeof(Q4ShallowSweepWork));
FULL(Q4WindowEdgeWork,window_edge_fields,sizeof(Q4LocalGeometryWork)+sizeof(Q4ShallowSetWork)+sizeof(Q4WindowSweepWork));
FULL(LidarJudgeWork,judge_fields,0);
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

std::array<unsigned,3> parse_order(std::string_view text) {
  if(text=="28,29,30") return {28,29,30};
  if(text=="28,30,29") return {28,30,29};
  if(text=="29,28,30") return {29,28,30};
  if(text=="29,30,28") return {29,30,28};
  if(text=="30,28,29") return {30,28,29};
  if(text=="30,29,28") return {30,29,28};
  throw std::invalid_argument("order must be a permutation of 28,29,30");
}
void dump_local(const Q4LocalEdgeWork& work,const Output& output) {
  std::cout<<"{\"edge\":";audit_dump(work,local_edge_fields);
  std::cout<<",\"geometry\":";dump_geometry(work.geometry);
  std::cout<<",\"atlas\":{";dump_fields(work.atlas,atlas_fields);
  std::cout<<",\"partition\":";audit_dump(work.atlas.partition,partition_fields);
  std::cout<<",\"domain\":{\"disk_tests\":"<<work.atlas.domain.disk_tests
    <<",\"facet_tests\":"<<work.atlas.domain.facet_tests<<"}}";
  std::cout<<",\"sweep\":";audit_dump(work.sweep,local_sweep_fields);
  std::cout<<",\"output\":";output.dump();std::cout<<'}';
}
void dump_shallow_sweep(const Q4ShallowSweepWork& work) {
  std::cout<<'{';dump_fields(work,shallow_sweep_fields);
  std::cout<<",\"family\":";audit_dump(work.family,family_fields);std::cout<<'}';
}
void dump_shallow(const Q4ShallowEdgeWork& work,const Output& output) {
  std::cout<<"{\"edge\":";audit_dump(work,shallow_edge_fields);
  std::cout<<",\"geometry\":";dump_geometry(work.geometry);
  std::cout<<",\"selection\":";audit_dump(work.selection,selection_fields);
  std::cout<<",\"sweep\":";dump_shallow_sweep(work.sweep);
  std::cout<<",\"output\":";output.dump();std::cout<<'}';
}
void dump_window(const Q4WindowEdgeWork& work,const Output& output) {
  std::cout<<"{\"edge\":";audit_dump(work,window_edge_fields);
  std::cout<<",\"geometry\":";dump_geometry(work.geometry);
  std::cout<<",\"selection\":";audit_dump(work.selection,selection_fields);
  std::cout<<",\"sweep\":";dump_shallow_sweep(work.sweep.sweep);
  std::cout<<",\"window\":";audit_dump(work.sweep.window,window_fields);
  std::cout<<",\"output\":";output.dump();std::cout<<'}';
}
u64 sum_counts(std::initializer_list<u64> values) {
  u64 result=0;for(const auto value:values) counter_add(result,value);return result;
}
[[maybe_unused]] int lidar_run(int argc,char** argv) {
  require(argc==5 || argc==6,
      "usage: mhgp8_q4_lidar_probe input.txt|input.u16le original_edge_a original_edge_b K [28,29,30]");
  const auto k=number(argv[4]);
  require(k>=3 && k<=std::numeric_limits<std::size_t>::max()-2,
      "K must be 3..SIZE_MAX-2 (Output helper arithmetic)");
  std::array<std::size_t,2> edge{number(argv[2]),number(argv[3])};
  require(edge[0]!=edge[1],"edge IDs must be distinct");
  std::sort(edge.begin(),edge.end());
  const auto order=parse_order(argc==6?argv[5]:"28,29,30");
  const auto start=Clock::now();
  const auto input=read_lidar(argv[1]);const auto read=Clock::now();
  require(edge[1]<input.points.size(),"edge ID outside original input");
  // Input order, hence every original ID and tie-break, is unchanged.
  const auto cloud=prepare_cloud(input.points);const auto prepared=Clock::now();
  const auto index=make_q2_cloud_index(cloud);const auto indexed=Clock::now();
  const auto cover=Q34EdgeCover::make(index,edge);const auto covered=Clock::now();
  const Q4LocalOptions options{Q4CenterDomainMode::Positive,7,4096,512,32,true};
  Output local,shallow,window;
  Q4LocalEdgeWork work28;
  Q4ShallowEdgeWork work29;
  Q4WindowEdgeWork work30;
  std::array<double,3> edge_ms{};
  for(const auto lane:order) {
    const auto before=Clock::now();
    if(lane==28) {
      work28=run_q4_local_edge_candidates(cover,k,options,
          [&](const auto& c){local.collect(c,input.points.size(),k);});
    } else if(lane==29) {
      work29=run_q4_shallow_edge_candidates(cover,k,
          [&](const auto& c){shallow.collect(c,input.points.size(),k);});
    } else {
      work30=run_q4_window_edge_candidates(cover,k,
          [&](const auto& c){window.collect(c,input.points.size(),k);});
    }
    edge_ms[lane-28]=ms(before,Clock::now());
  }
  const auto processed=Clock::now();
  local.finish();shallow.finish();window.finish();
  require(local.records==shallow.records && local.records==window.records,
      "28/29/30 exact normalized record comparison failed");
  require(local.hash==shallow.hash && local.hash==window.hash,
      "28/29/30 digest mismatch after exact record comparison");
  require(local.q3==0 && shallow.q3==0 && window.q3==0 &&
      local.q4==work28.sweep.emitted && shallow.q4==work29.sweep.emitted &&
      window.q4==work30.sweep.sweep.emitted,"q4 output ledger mismatch");
  require(local.shell_ids_visited==work28.sweep.shell_ids &&
      shallow.shell_ids_visited==work29.sweep.shell_ids &&
      window.shell_ids_visited==work30.sweep.sweep.shell_ids,"shell ledger mismatch");
  const auto compared=Clock::now();
  // All three complete payloads are equal. Judge that common payload ONCE;
  // this is emitted-ball validity, not completeness of the large-cloud output.
  const auto judgement=judge_emissions(local,input.points,edge);
  const auto done=Clock::now();
  const auto shared_prepare=ms(read,covered);
  const auto& w=work30.sweep.window;
  const auto& f=work30.sweep.sweep.family;
  const auto root_comparisons30=sum_counts({w.heap_comparisons,w.heap_sort_comparisons,
      w.window_comparisons,f.sort_comparisons,f.group_comparisons});
  const auto root_comparisons29=sum_counts({work29.sweep.family.sort_comparisons,
      work29.sweep.family.group_comparisons});
  std::cout<<std::fixed<<std::setprecision(6)
    <<"{\"schema\":\"mhgp8_q4_lidar_probe_v1\",\"status\":\"passed\","
      "\"scope\":\"one_original_edge_q4_three_actual_product_apis_not_global_producer\","
      "\"phase\":\"exploration_v8_hors_registre\",\"backend\":\"cpu_reference\","
      "\"profile\":\""<<input_profile(input)<<"\",\"mode\":\"implementation_v8_p0\","
      "\"public_status\":\"not_claimed\",\"completeness_large_cloud_claimed\":false,"
      "\"input_reindexed\":false,\"input_format\":\""
    <<input_format(argv[1])
    <<"\",\"edge_ids\":["<<edge[0]<<','<<edge[1]
    <<"],\"execution_order\":["<<order[0]<<','<<order[1]<<','<<order[2]
    <<"],\"n\":"<<input.points.size()<<",\"kmax\":"<<k
    <<",\"input_fnv1a64_u64le_n_xyz\":"<<input.hash<<",\"cover_sites\":"<<cover->site_count()
    <<",\"local_options\":{\"domain\":\"Positive\",\"depth\":7,\"node_budget\":4096,"
      "\"z_test_budget\":512,\"leaf_sites\":32,\"clip_events\":true}"
    <<",\"cloud_work\":";audit_dump(cloud->work(),cloud_fields);
  std::cout<<",\"index_work\":";audit_dump(index->work(),index_fields);
  std::cout<<",\"cover_work\":";audit_dump(cover->work(),cover_fields);
  std::cout<<",\"local28\":";dump_local(work28,local);
  std::cout<<",\"shallow29\":";dump_shallow(work29,shallow);
  std::cout<<",\"window30\":";dump_window(work30,window);
  std::cout<<",\"comparison\":{\"normalized_record_sets\":3,\"equal_pairs\":2,"
      "\"judged_common_payloads\":1}"
    <<",\"judge\":";audit_dump(judgement,judge_fields);
  // One full common list, after exact equality of all three lists, not digest-only.
  std::cout<<",\"records\":";dump_records(local);
  std::cout<<",\"derived_work\":{"
      "\"local28_active_sites\":"<<work28.sweep.active_sites
    <<",\"local28_root_order_comparisons\":"<<sum_counts({work28.sweep.sort_comparisons,work28.sweep.group_comparisons})
    <<",\"shallow29_first_pass_sites\":"<<work29.sweep.family.sites
    <<",\"shallow29_root_order_comparisons\":"<<root_comparisons29
    <<",\"window30_first_pass_sites\":"<<f.sites
    <<",\"window30_second_pass_sites\":"<<w.second_pass_sites
    <<",\"window30_total_pass_sites\":"<<sum_counts({f.sites,w.second_pass_sites})
    <<",\"window30_root_order_comparisons\":"<<root_comparisons30<<'}';
  std::cout<<",\"memory\":{\"id_bytes\":"<<sizeof(std::size_t)
    <<",\"input_point_capacity_bytes\":"<<input.points.capacity()*sizeof(Point3)
    <<",\"cloud_retained_bytes\":"<<cloud->retained_bytes()
    <<",\"index_retained_bytes\":"<<index->retained_bytes()
    <<",\"cover_retained_bytes\":"<<cover->retained_bytes()
    <<",\"local_output_retained_bytes\":"<<local.retained_bytes()
    <<",\"shallow_output_retained_bytes\":"<<shallow.retained_bytes()
    <<",\"window_output_retained_bytes\":"<<window.retained_bytes()
    <<",\"scope\":\"Separate capacities, not RSS; earlier outputs coexist with later lanes; product peaks exclude shared input/cloud/index/cover, outputs, judge map and allocator metadata\"}"
    <<",\"timings_ms\":{\"read_hash\":"<<ms(start,read)
    <<",\"cloud\":"<<ms(read,prepared)<<",\"index\":"<<ms(prepared,indexed)
    <<",\"cover\":"<<ms(indexed,covered)
    <<",\"local28_prepare_edge_collect\":"<<edge_ms[0]
    <<",\"shallow29_prepare_edge_collect\":"<<edge_ms[1]
    <<",\"window30_prepare_edge_collect\":"<<edge_ms[2]
    <<",\"local28_shared_prepare_plus_edge_sum\":"<<shared_prepare+edge_ms[0]
    <<",\"shallow29_shared_prepare_plus_edge_sum\":"<<shared_prepare+edge_ms[1]
    <<",\"window30_shared_prepare_plus_edge_sum\":"<<shared_prepare+edge_ms[2]
    <<",\"three_lanes_wall\":"<<ms(covered,processed)
    <<",\"sort_digest_exact_compare\":"<<ms(processed,compared)
    <<",\"independent_judge\":"<<ms(compared,done)
    <<",\"total_before_json\":"<<ms(start,done)
    <<"},\"timing_scope\":\"Actual serial order is recorded; each edge interval includes its OWN geometry/atlas or shallow selection, generation, census, callbacks and private destruction. Shared cloud/index/cover is timed once; inclusive sums reuse that interval and are NOT three independent walls. Judge scans n once per distinct emitted ball. JSON and final shared/output destruction excluded\"}\n";
  return 0;
}
} // namespace
#ifndef MHGP8_Q4_LIDAR_PROBE_LIBRARY
int main(int argc,char** argv) {
  try {return lidar_run(argc,argv);}
  catch(const std::exception& error) {std::cerr<<"q4 LiDAR probe: "<<error.what()<<'\n';return 1;}
}
#endif
