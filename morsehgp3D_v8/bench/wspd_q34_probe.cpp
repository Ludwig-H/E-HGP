// Explicit reuse of the three-way LiDAR probe's file reader and work tables.
// Its one-edge benchmark and rational emission judge are NOT called here.
#define MHGP8_Q4_LIDAR_PROBE_LIBRARY
#include "q4_lidar_probe.cpp"
#undef MHGP8_Q4_LIDAR_PROBE_LIBRARY
#include "pipeline/wspd_q34.hpp"

namespace {
#define F(T,m) Field<T>{#m,&T::m}
constexpr std::array global_fields{
 F(WspdQ34Work,input_rectangles),F(WspdQ34Work,expanded_pairs),F(WspdQ34Work,q3_edges),
 F(WspdQ34Work,q4_edges),F(WspdQ34Work,both_edges),F(WspdQ34Work,cover_builds),
 F(WspdQ34Work,cover_sites),F(WspdQ34Work,max_cover_sites),F(WspdQ34Work,peak_cover_bytes),
 F(WspdQ34Work,q3_emitted),F(WspdQ34Work,q4_emitted),F(WspdQ34Work,payload_shell_ids),
 F(WspdQ34Work,peak_edge_buffer_bytes)};
constexpr std::array q3_fields{
 F(WspdQ3Work,edge_queries),F(WspdQ3Work,seed_node_visits),F(WspdQ3Work,seed_bound_tests),
 F(WspdQ3Work,seed_point_tests),F(WspdQ3Work,seed_rejected_nodes),F(WspdQ3Work,seed_split_nodes),
 F(WspdQ3Work,seed_rejected_sites),F(WspdQ3Work,acute_seeds),F(WspdQ3Work,owner_tests),
 F(WspdQ3Work,owner_rejections),F(WspdQ3Work,seeds),F(WspdQ3Work,ball_builds),
 F(WspdQ3Work,census_range_visits),F(WspdQ3Work,census_point_tests),F(WspdQ3Work,census_inside_sites),
 F(WspdQ3Work,census_outside_sites),F(WspdQ3Work,census_shell_sites),F(WspdQ3Work,depth_rejections),
 F(WspdQ3Work,early_unread_sites),F(WspdQ3Work,shell_sort_comparisons),F(WspdQ3Work,shell_ids),
 F(WspdQ3Work,emitted),F(WspdQ3Work,peak_shell_bytes)};
constexpr std::array global_front_fields{
 F(WspdFrontWork,product_visits),F(WspdFrontWork,diagonal_splits),F(WspdFrontWork,diagonal_leaves),
 F(WspdFrontWork,disjoint_splits),F(WspdFrontWork,separation_tests),F(WspdFrontWork,witness_searches),
 F(WspdFrontWork,witness_descent_steps),F(WspdFrontWork,witness_box_distance_tests),
 F(WspdFrontWork,proposed_sites),F(WspdFrontWork,proposals_in_factors),F(WspdFrontWork,h_bound_tests),
 F(WspdFrontWork,xi_bound_tests),F(WspdFrontWork,witness_lane_credits),F(WspdFrontWork,fully_rejected_products),
 F(WspdFrontWork,emitted_rectangles),F(WspdFrontWork,emitted_factor_sites),F(WspdFrontWork,max_factor_size),
 F(WspdFrontWork,leaf_pair_rectangles),F(WspdFrontWork,max_stack_size),F(WspdFrontWork,max_product_depth),
 F(WspdFrontWork,extended_products),F(WspdFrontWork,extended_proposals),F(WspdFrontWork,extended_proposals_in_factors),
 F(WspdFrontWork,extended_credits),F(WspdFrontWork,extended_rejections),F(WspdFrontWork,inherited_credits),
 F(WspdFrontWork,inherited_duplicates),F(WspdFrontWork,extended_inherited_duplicates),
 F(WspdFrontWork,inherited_rejections),F(WspdFrontWork,emitted_witness_credits)};
constexpr std::array parallel_fields{
 F(WspdQ34ParallelWork,requested_workers),F(WspdQ34ParallelWork,started_workers),
 F(WspdQ34ParallelWork,target_jobs),F(WspdQ34ParallelWork,jobs),F(WspdQ34ParallelWork,completed_jobs),
 F(WspdQ34ParallelWork,terminal_jobs),F(WspdQ34ParallelWork,prefix_product_visits),
 F(WspdQ34ParallelWork,job_storage_bytes),F(WspdQ34ParallelWork,worker_state_bytes),
 F(WspdQ34ParallelWork,callback_storage_bytes),F(WspdQ34ParallelWork,edge_buffer_bytes_sum)};
constexpr std::array worker_fields{
 F(WspdQ34WorkerWork,jobs),F(WspdQ34WorkerWork,front_products),F(WspdQ34WorkerWork,input_rectangles),
 F(WspdQ34WorkerWork,expanded_pairs),F(WspdQ34WorkerWork,q3_emitted),F(WspdQ34WorkerWork,q4_emitted),
 F(WspdQ34WorkerWork,peak_edge_buffer_bytes)};
#undef F
static_assert(sizeof(WspdQ34ParallelWork)==parallel_fields.size()*sizeof(u64));
static_assert(sizeof(WspdQ34WorkerWork)==worker_fields.size()*sizeof(u64));
static_assert(sizeof(WspdQ3Work)==q3_fields.size()*sizeof(u64));
static_assert(sizeof(WspdQ34Work)==global_fields.size()*sizeof(u64)+sizeof(Q34EdgeCoverWork)+
 sizeof(WspdQ3Work)+sizeof(Q4LocalEdgeWork)+sizeof(Q4WindowEdgeWork));
static_assert(sizeof(WspdFrontWork)==(global_front_fields.size()+19)*sizeof(u64));

struct StreamingOutput {
 u64 callbacks{},q3{},q4{},support_ids{},shell_ids{},xor_hash{},sum_hash{};
 void collect(const Q34SeedCandidate& c,std::size_t n,unsigned k) {
  require((c.arity==3 || c.arity==4) && c.depth<k+2-c.arity,"invalid candidate arity/depth");
  u64 hash=14695981039346656037ULL;
  word(hash,c.arity);word(hash,c.depth);
  for(unsigned i=0;i<c.arity;++i) {
   require(c.support_ids[i]<n && (i==0 || c.support_ids[i-1]<c.support_ids[i]),"invalid support IDs");
   require(std::binary_search(c.shell_first.begin(),c.shell_first.end(),c.support_ids[i]) ||
     std::binary_search(c.shell_second.begin(),c.shell_second.end(),c.support_ids[i]),"support absent from shell");
   word(hash,c.support_ids[i]);counter_add(support_ids);
  }
  for(auto value:c.ball.coefficients()) {
   const auto bits=static_cast<u128>(value);
   word(hash,static_cast<u64>(bits));word(hash,static_cast<u64>(bits>>64));
  }
  for(const auto part:{c.shell_first,c.shell_second})
   require(std::is_sorted(part.begin(),part.end()),"unordered shell part");
  std::size_t a=0,b=0,previous=0,count=0;
  while(a<c.shell_first.size() || b<c.shell_second.size()) {
   const auto id=b==c.shell_second.size() || (a<c.shell_first.size() && c.shell_first[a]<c.shell_second[b])
     ?c.shell_first[a++]:c.shell_second[b++];
   require(id<n && (count==0 || id>previous),"duplicate/invalid shell ID");
   word(hash,id);previous=id;++count;counter_add(shell_ids);
  }
  word(hash,count);counter_add(callbacks);counter_add(c.arity==3?q3:q4);
  xor_hash^=hash;sum_hash+=hash; // Explicit modulo-2^64 canonical reduction.
 }
 void merge(const StreamingOutput& other) {
  counter_add(callbacks,other.callbacks);counter_add(q3,other.q3);counter_add(q4,other.q4);
  counter_add(support_ids,other.support_ids);counter_add(shell_ids,other.shell_ids);
  xor_hash^=other.xor_hash;sum_hash+=other.sum_hash;
 }
 void dump() const {
  std::cout<<"{\"callbacks\":"<<callbacks<<",\"q3\":"<<q3<<",\"q4\":"<<q4
   <<",\"support_ids\":"<<support_ids<<",\"shell_ids\":"<<shell_ids
   <<",\"xor\":\""<<std::hex<<xor_hash<<"\",\"sum\":\""<<sum_hash<<std::dec<<"\"}";
 }
};
template<std::size_t N> void dump_u64_array(const std::array<u64,N>& values) {
 std::cout<<'[';for(std::size_t i=0;i<N;++i) {if(i)std::cout<<',';std::cout<<values[i];}std::cout<<']';
}
void dump_global_front(const WspdFrontResult& result) {
 std::cout<<"{\"total_unordered_pairs\":"<<result.total_unordered_pairs
  <<",\"active_lane_mask\":"<<static_cast<unsigned>(result.active_lane_mask)<<",\"work\":{";
 dump_fields(result.work,global_front_fields);
 std::cout<<",\"rejected_pair_mass\":";dump_u64_array(result.work.rejected_pair_mass);
 std::cout<<",\"residual_pair_mass\":";dump_u64_array(result.work.residual_pair_mass);
 std::cout<<",\"lane_rectangles\":";dump_u64_array(result.work.lane_rectangles);
 std::cout<<",\"size_class_rectangles\":";dump_u64_array(result.work.size_class_rectangles);
 std::cout<<",\"size_class_pair_mass\":";dump_u64_array(result.work.size_class_pair_mass);
 std::cout<<"}}";
}
void dump_global_work(const WspdQ34Work& work) {
 std::cout<<'{';dump_fields(work,global_fields);
 std::cout<<",\"cover\":";audit_dump(work.cover,cover_fields);
 std::cout<<",\"q3\":";audit_dump(work.q3,q3_fields);
 std::cout<<",\"local28\":{\"edge\":";audit_dump(work.local,local_edge_fields);
 std::cout<<",\"geometry\":";dump_geometry(work.local.geometry);
 std::cout<<",\"atlas\":{";dump_fields(work.local.atlas,atlas_fields);
 std::cout<<",\"partition\":";audit_dump(work.local.atlas.partition,partition_fields);
 std::cout<<",\"domain\":{\"disk_tests\":"<<work.local.atlas.domain.disk_tests
  <<",\"facet_tests\":"<<work.local.atlas.domain.facet_tests<<"}}";
 std::cout<<",\"sweep\":";audit_dump(work.local.sweep,local_sweep_fields);
 std::cout<<"},\"window30\":{\"edge\":";audit_dump(work.window,window_edge_fields);
 std::cout<<",\"geometry\":";dump_geometry(work.window.geometry);
 std::cout<<",\"selection\":";audit_dump(work.window.selection,selection_fields);
 std::cout<<",\"sweep\":";dump_shallow_sweep(work.window.sweep.sweep);
 std::cout<<",\"window\":";audit_dump(work.window.sweep.window,window_fields);std::cout<<"}}";
}
int global_run(int argc,char** argv) {
 require(argc>=8 && argc<=10,"usage: mhgp8_wspd_q34_probe file n K s mask backend workers [samples|pure] [digest|records]");
 const auto n=number(argv[2]),k=number(argv[3]),s=number(argv[4]),mask=number(argv[5]);
 const auto backend=number(argv[6]),workers=number(argv[7]);
 const std::string_view mode=argc>=9?argv[8]:"samples",output_mode=argc>=10?argv[9]:"digest";
 require(n>0 && k>=1 && k<=10 && s>0 && s<=std::numeric_limits<unsigned>::max() &&
  (mask==2 || mask==4 || mask==6) && (backend==28 || backend==30) && workers>0 &&
  (mode=="samples" || mode=="pure") && (output_mode=="digest" || output_mode=="records"),"invalid global probe options");
 const auto started=Clock::now();
 auto input=read_lidar(argv[1]);const auto source_n=input.points.size();
 require(n<=source_n,"requested prefix larger than file");input.points.resize(n);
 input.hash=14695981039346656037ULL;word(input.hash,n);
 for(const auto point:input.points) for(std::size_t axis=0;axis<3;++axis)word(input.hash,point[axis]);
 const auto loaded=Clock::now();const auto cloud=prepare_cloud(input.points);const auto prepared=Clock::now();
 const auto index=make_q2_cloud_index(cloud);const auto indexed=Clock::now();
 WspdQ34Options options;
 options.requested_lane_mask=static_cast<std::uint8_t>(mask);
 options.front_mode=mode=="samples"?WspdFrontMode::MidpointSamples:WspdFrontMode::Pure;
 options.q4_backend=backend==28?WspdQ4Backend::Local28:WspdQ4Backend::Window30;
 std::vector<StreamingOutput> outputs(workers);
 std::vector<Output> payloads(output_mode=="records"?workers:0);
 const auto parallel=run_wspd_q34_parallel(index,static_cast<unsigned>(k),static_cast<unsigned>(s),options,workers,
  [&](std::size_t slot,const auto& c) {
   outputs[slot].collect(c,n,static_cast<unsigned>(k));
   if(output_mode=="records")payloads[slot].collect(c,n,k);
  });
 const auto& result=parallel.pipeline;
 const auto processed=Clock::now();
 StreamingOutput output;Output full;std::size_t worker_record_bytes=0;
 for(const auto& part:outputs)output.merge(part);
 for(auto& part:payloads) {
  worker_record_bytes+=part.retained_bytes();
  for(auto& record:part.records)full.records.push_back(std::move(record));
 }
 if(output_mode=="records")full.finish();
 require(output.q3==result.work.q3_emitted && output.q4==result.work.q4_emitted &&
  output.shell_ids==result.work.payload_shell_ids,"global callback ledger differs");
 const auto done=Clock::now();
 std::cout<<std::fixed<<std::setprecision(6)
  <<"{\"schema\":\"mhgp8_wspd_q34_probe_v1\",\"status\":\"completed\","
    "\"scope\":\"global_q3_q4_candidate_stream_not_catalogue_or_full\",\"backend\":\"cpu_reference\","
    "\"profile\":\"quantized_u16_input_only\",\"public_status\":\"not_claimed\","
  <<"\"n\":"<<n<<",\"source_n\":"<<source_n<<",\"kmax\":"<<k<<",\"s\":"<<s
  <<",\"mask\":"<<mask<<",\"q4_backend\":"<<backend<<",\"workers\":"<<workers
  <<",\"front_mode\":\""<<mode<<"\",\"output_mode\":\""<<output_mode<<"\",\"input_hash\":"<<input.hash
  <<",\"validation\":\"payload_structure_and_ledger_only_small_exhaustive_gate_separate\",\"output\":";
 output.dump();std::cout<<",\"front\":";dump_global_front(result.front);
 std::cout<<",\"work\":";dump_global_work(result.work);
 std::cout<<",\"parallel\":";audit_dump(parallel.parallel,parallel_fields);
 std::cout<<",\"workers_work\":[";
 for(std::size_t slot=0;slot<parallel.workers.size();++slot) {
  if(slot)std::cout<<',';
  audit_dump(parallel.workers[slot],worker_fields);
 }
 std::cout<<']';
 std::cout<<",\"cloud_work\":";audit_dump(cloud->work(),cloud_fields);
 std::cout<<",\"index_work\":";audit_dump(index->work(),index_fields);
 std::cout<<",\"memory\":{\"id_bytes\":"<<sizeof(std::size_t)<<",\"input_capacity_bytes\":"<<input.points.capacity()*sizeof(Point3)
  <<",\"cloud_retained_bytes\":"<<cloud->retained_bytes()<<",\"index_retained_bytes\":"<<index->retained_bytes()
  <<",\"worker_record_capacity_bytes_before_merge\":"<<worker_record_bytes
  <<",\"merged_record_capacity_bytes\":"<<full.retained_bytes()<<"},\"timings_ms\":{\"load_prefix_hash\":"<<ms(started,loaded)
  <<",\"cloud\":"<<ms(loaded,prepared)<<",\"index\":"<<ms(prepared,indexed)<<",\"front_edges_collect\":"<<ms(indexed,processed)
  <<",\"record_normalization\":"<<ms(processed,done)<<",\"pipeline_including_shared_preparation\":"<<ms(loaded,processed)
  <<",\"total_before_serialization_and_release\":"<<ms(started,done)<<"}";
 if(output_mode=="records") {std::cout<<",\"records\":";dump_records(full);}
 std::cout<<"}\n";return 0;
}
}
int main(int argc,char** argv) {
 try {return global_run(argc,argv);}
 catch(const std::exception& error) {std::cerr<<"mhgp8_wspd_q34_probe: "<<error.what()<<'\n';return 1;}
}
