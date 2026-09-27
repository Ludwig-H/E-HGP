// Real-catalogue harness. Explicit orchestration/input port from
// b_full_real_drafts_20260927/probe.cpp SHA256
// 2c5c2265506b1649761ceb54d21e4541eab8ddbf4910f9227cc4e1987128bfdc.
// No old hook substitution: the separately named, pinned AObservedBuilder
// supplies the three already-qualified A hooks. Exactly one chain TU.
#include "../b_full_a_manifest_20260927/native_a.hpp"
#include "../b_full_a_events_20260927/events.hpp"
#include "../b_full_a_min_label_20260927/min_label.hpp"
#include "../../src/chain/tower_chain.cpp"
#include "../../src/gpu/filter_runner_stub.cpp"
#include "front_fixtures.hpp"
#include <charconv>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sys/resource.h>

namespace ar {
namespace am=mhgp9::audit::a_manifest;
namespace ae=mhgp9::audit::a_events;
namespace ml=mhgp9::audit::a_min_label;
using namespace mhgp9::tower;
using mhgp9::gen::Point3;
using Clock=am::Clock;
using am::need;
using am::milliseconds;
using ml::bytes;
using ml::plus;
template<class T> void release(T& value) {value=T{};}
u64 hash_points(std::span<const Point3> points) {
  u64 h=14695981039346656037ULL;
  mhgp9::gen::bench::front_hash_word(h,1);mhgp9::gen::bench::front_hash_word(h,points.size());
  for(auto p:points) for(auto x:{p.x,p.y,p.z}) mhgp9::gen::bench::front_hash_word(h,x);
  return h;
}
long rss() {rusage r{};need(getrusage(RUSAGE_SELF,&r)==0,"real.rss");return r.ru_maxrss;}
u64 index_bytes(const CloudIndex& x) {
  return plus(plus(plus(bytes(x.keys),bytes(x.upos)),plus(bytes(x.bucket_start),bytes(x.bucket_ids))),
              plus(bytes(x.wsum),bytes(x.nodes)));
}
u64 output_bytes(const am::Output& o) {
  u64 n=0;const auto add=[&](const auto& v){n=plus(n,bytes(v));};
  add(o.anchors);add(o.next);add(o.runs);add(o.birth_ball);add(o.root_begin);add(o.roots);
  add(o.draft.level);add(o.draft.batch_begin);add(o.draft.parent_begin);add(o.draft.parent);
  add(o.draft.contribution_begin);add(o.draft.contribution);return n;
}
u64 input_bytes(const am::Input& i) {
  u64 n=0;const auto add=[&](const auto& v){n=plus(n,bytes(v));};
  add(i.domain);add(i.spatial_ids);add(i.program);add(i.level_run);add(i.count);add(i.targets);
  add(i.level);add(i.regular);add(i.shell_size);add(i.interior);add(i.contribution);return n;
}
u64 manifest_bytes(const am::Manifest& m) {
  return plus(plus(bytes(m.domain),bytes(m.blocks)),plus(bytes(m.representative_begin),bytes(m.target)));
}
u64 output_hash(const am::Output& o) {
  u64 h=14695981039346656037ULL;const auto word=[&](u64 v){mhgp9::gen::bench::front_hash_word(h,v);};
  const auto seq=[&](const auto& v){word(v.size());for(auto x:v) word(x);};
  const auto level=[&](const ExactLevel& l){for(auto x:l.num) word(x);word(static_cast<u64>(l.den));word(static_cast<u64>(static_cast<u128>(l.den)>>64));};
  seq(o.anchors);seq(o.next);seq(o.runs);seq(o.birth_ball);seq(o.root_begin);seq(o.roots);
  word(o.draft.level.size());for(const auto& l:o.draft.level) level(l);
  seq(o.draft.batch_begin);seq(o.draft.parent_begin);seq(o.draft.parent);seq(o.draft.contribution_begin);
  word(o.draft.contribution.size());for(const auto& c:o.draft.contribution) {word(c.population);word(c.shell_mask);word(c.include_interior);}
  const auto& s=o.stats;for(u64 x:{s.anchor_blocks,s.regular_blocks,s.extra_blocks,s.representatives,s.births,s.merges,
    s.contributions,s.inert_blocks,s.singleton_lots,s.grouped_lots,s.lot_dsu_slots}) word(x);
  return h;
}
void check_index(const CloudIndex& ix,std::span<const Point3> points) {
  need(ix.valid && ix.input_count==points.size() && ix.upos.size()==points.size() && !ix.has_duplicate_positions(),"real.index_shape");
  std::vector<bool> seen(points.size());
  for(size_t u=0;u<ix.upos.size();++u) {
    const auto id=ix.point_id(static_cast<i32>(u));
    need(id<points.size() && !seen[id] && ix.multiplicity(static_cast<i32>(u))==1,"real.index_id");seen[id]=true;
    need(ix.upos[u]==mhgp9::to_p3(points[id]) && ix.keys[u]==morton48(ix.upos[u]),"real.index_position");
  }
}
void same_index(const CloudIndex& a,const CloudIndex& b) {
  need(a.valid==b.valid && a.input_count==b.input_count && a.keys==b.keys && a.upos==b.upos &&
    a.bucket_start==b.bucket_start && a.bucket_ids==b.bucket_ids && a.wsum==b.wsum && a.nodes.size()==b.nodes.size(),"gate.index_tables");
  for(size_t j=0;j<a.nodes.size();++j) {
    const auto& x=a.nodes[j];const auto& y=b.nodes[j];
    need(x.left==y.left && x.right==y.right && x.first==y.first && x.last==y.last && x.parent==y.parent,"gate.index_links");
    for(unsigned d=0;d<3;++d) need(x.clo[d]==y.clo[d] && x.chi[d]==y.chi[d] && x.tlo[d]==y.tlo[d] && x.thi[d]==y.thi[d],"gate.index_boxes");
  }
}
void same_catalogue(std::span<const BallData> a,std::span<const BallData> b) {
  need(a.size()==b.size(),"gate.catalogue_size");
  for(size_t j=0;j<a.size();++j) {
    const auto& x=a[j];const auto& y=b[j];
    need(x.key==y.key && x.level==y.level && x.arity==y.arity && x.n_interior==y.n_interior && x.n_shell==y.n_shell &&
      std::equal(x.interior().begin(),x.interior().end(),y.interior().begin()) &&
      std::equal(x.shell().begin(),x.shell().end(),y.shell().begin()),"gate.catalogue_ball_id");
  }
}
void same_tower(const FullBallTowerResult& a,const FullBallTowerResult& b) {
  need(a.status==FullBallStatus::kCompleteRelative && b.status==FullBallStatus::kCompleteRelative && a.orders.size()==b.orders.size(),"gate.full_status");
  for(size_t k=0;k<a.orders.size();++k) {
    const auto& x=a.orders[k].forest;const auto& y=b.orders[k].forest;
    need(x.order()==y.order() && x.parents()==y.parents() && x.successors()==y.successors() &&
      x.nodes().size()==y.nodes().size() && x.contributions().size()==y.contributions().size() &&
      a.orders[k].lower_nodes==b.orders[k].lower_nodes,"gate.full_structure");
    for(size_t j=0;j<x.nodes().size();++j) {
      const auto& p=x.nodes()[j];const auto& q=y.nodes()[j];
      need(p.level==q.level && p.first==q.first && p.parent_count==q.parent_count,"gate.full_node");
    }
    for(size_t j=0;j<x.contributions().size();++j) {
      const auto& p=x.contributions()[j];const auto& q=y.contributions()[j];
      need(p.level==q.level && p.segment==q.segment && am::same_ref(p.ref,q.ref),"gate.full_contribution");
    }
    const auto& p=*x.populations();const auto& q=*y.populations();
    need(p.domain()==q.domain() && p.rows().size()==q.rows().size(),"gate.full_bank");
    for(size_t j=0;j<p.rows().size();++j) need(p.rows()[j].interior==q.rows()[j].interior && p.rows()[j].shell==q.rows()[j].shell,"gate.full_population");
  }
}
mhgp9::ChainOptions options(unsigned k,size_t workers,bool hash=true) {
  mhgp9::ChainOptions o;o.kmax=k;o.workers=workers;o.tower_static_threads=static_cast<int>(workers);
  o.separation_s=8;o.catalogue_digest=true;o.keep_catalogue=true;o.run_tower=false;o.tower_hash_grouping=hash;
  return o; // All other native CPU defaults are pinned. No GPU levers.
}
struct Row {
  unsigned k{};u64 balls{},blocks{},regular_blocks{},extra_blocks{},targets{},nodes{},actions{},contributions{},digest{};
  u64 input_capacity{},output_capacity{},manifest_capacity{};
  double native_a_instrumented_ms{},copy_ms{},manifest_ms{},binding_ms{},checks_ms{},manifest_cleanup_ms{},slot_cleanup_ms{};
  std::array<double,3> minimum_wall_ms{},event_wall_ms{},minimum_cleanup_ms{},event_cleanup_ms{};
  std::array<ml::Times,3> minimum_times;std::array<ae::Times,3> event_times;
  ml::Work minimum_work;ae::Work event_work;
};
struct Report {
  unsigned k{};size_t workers{};bool hashed{};
  u64 n{},input_hash{},balls{},catalogue_digest{},presentation_digest{},tower_digest{},index_capacity{},catalogue_capacity{},capture_capacity{};
  double chain_wall_ms{},chain_reported_ms{},chain_catalogue_digest_ms{},chain_q2_ms{},chain_q34_ms{},chain_census_ms{};
  double reindex_ms{},identity_ms{},capture_full_wall_ms{},admit_ms{},tower_digest_ms{},copy_ms_sum{};
  double upstream_cleanup_ms{},capture_tail_cleanup_ms{},external_ms{};
  FullBallTimes native_times;std::vector<Row> rows;
};
Report run(const std::vector<Point3>& points,unsigned k,size_t workers,bool hashed,bool gate) {
  need(workers>=2 && workers<=static_cast<size_t>(std::numeric_limits<int>::max()),"real.static_workers");
  const auto all_start=Clock::now();Report r;r.n=points.size();r.workers=workers;r.hashed=hashed;
  auto start=Clock::now();r.input_hash=hash_points(points);r.identity_ms=milliseconds(start);
  auto o=options(k,workers,hashed);start=Clock::now();auto chain=mhgp9::run_tower_chain(points,o);r.chain_wall_ms=milliseconds(start);
  need(chain.status==mhgp9::ChainStatus::kComplete,chain.reason.c_str());
  need(chain.tower.orders.empty() && chain.kmax_effective==k && chain.tower_digest==0,"real.catalogue_only");
  r.k=k;r.balls=chain.catalogue_balls.size();r.catalogue_digest=chain.catalogue_digest;r.presentation_digest=chain.presentation_digest;
  r.chain_reported_ms=chain.times.total_ms;r.chain_catalogue_digest_ms=chain.times.catalogue_digest_ms;
  r.chain_q2_ms=chain.times.q2_ms;r.chain_q34_ms=chain.times.q34_ms;r.chain_census_ms=chain.times.census_ms;
  r.catalogue_capacity=bytes(chain.catalogue_balls);
  start=Clock::now();auto ix=mhgp9::build_tower_index(points);r.reindex_ms=milliseconds(start);r.index_capacity=index_bytes(ix);
  start=Clock::now();check_index(ix,points);r.identity_ms+=milliseconds(start);
  am::Capture capture;FullBallTowerResult observed;
  const FullBallTowerOptions to{.overlap_static=o.tower_overlap_static,.pipelined_tail=o.tower_pipelined_tail,
      .hash_grouping=o.tower_hash_grouping,.persistent_pool=o.tower_persistent_pool};
  start=Clock::now();
  {am::CaptureScope scope(capture);
    observed.orders=full_ball_detail::AObservedBuilder(ix,chain.catalogue_balls,k,observed.stats,
      static_cast<int>(workers),{},o.tower_meb_proposal,&observed.times,to,nullptr).run();}
  r.capture_full_wall_ms=milliseconds(start);r.native_times=observed.times;
  start=Clock::now();need(observed.orders.size()==k,"real.complete_orders");
  for(unsigned order=1;order<=k;++order) {
    const auto& s=capture.slots[order];need(s.starts==1 && s.finishes==1 && s.input.k==order,"real.complete_capture");
    need(s.input.spatial_ids.size()==ix.upos.size(),"real.capture_index_size");
    for(size_t u=0;u<ix.upos.size();++u) need(s.input.spatial_ids[u]==ix.point_id(static_cast<i32>(u)),"real.capture_index_identity");
    r.copy_ms_sum+=s.copy_ms;r.capture_capacity=plus(r.capture_capacity,plus(input_bytes(s.input),output_bytes(s.output)));
  }
  capture.complete=true;observed.status=FullBallStatus::kCompleteRelative;observed.reason="audit_native_builder_completed";r.admit_ms=milliseconds(start);
  start=Clock::now();r.tower_digest=mhgp9::tower_digest(observed);r.tower_digest_ms=milliseconds(start);
  if(gate) {
    // This expensive full chain is a bounded gate, NEVER a real-case baseline.
    o.run_tower=true;const auto reference=mhgp9::run_tower_chain(points,o);
    need(reference.status==mhgp9::ChainStatus::kComplete,"gate.reference_complete");
    same_catalogue(chain.catalogue_balls,reference.catalogue_balls);same_tower(observed,reference.tower);
    need(r.tower_digest==reference.tower_digest && r.catalogue_digest==reference.catalogue_digest &&
      r.presentation_digest==reference.presentation_digest,"gate.three_digests");
    const auto repeated=mhgp9::build_tower_index(points);same_index(ix,repeated);
    std::vector<InputPoint> input(points.size());
    for(size_t j=0;j<points.size();++j) input[j]={static_cast<PointId>(j),mhgp9::to_p3(points[j])};
    const auto direct=build_cloud_index(input);same_index(ix,direct);
  }
  start=Clock::now();release(observed);release(ix);release(chain);r.upstream_cleanup_ms=milliseconds(start);
  // The hooks own every input/output vector. No bank/index/catalogue span is
  // read after the preceding release. One owned manifest and one candidate
  // result at a time; both candidates see exactly that same manifest.
  for(unsigned order=1;order<=k;++order) {
    auto& s=capture.slots[order];Row row;row.k=order;row.input_capacity=input_bytes(s.input);row.output_capacity=output_bytes(s.output);
    row.native_a_instrumented_ms=r.native_times.lots_by_k[order];row.copy_ms=s.copy_ms;
    start=Clock::now();auto m=am::make_manifest(capture,order);row.manifest_ms=milliseconds(start);
    row.manifest_capacity=manifest_bytes(m);row.balls=m.ball_count;row.blocks=m.blocks.size();row.targets=m.target.size();
    row.regular_blocks=s.output.stats.regular_blocks;row.extra_blocks=s.output.stats.extra_blocks;
    start=Clock::now();am::validate_binding(m,s);row.digest=output_hash(s.output);row.binding_ms=milliseconds(start);
    row.nodes=s.output.next.size();row.actions=s.output.draft.actions();row.contributions=s.output.draft.contribution.size();
    if(gate) {const auto small=am::replay_a(m);am::compare_output(small,s.output);}
    for(size_t rep=0;rep<3;++rep) {
      const auto minimum=[&] {
        auto t=Clock::now();auto result=ml::build(m);row.minimum_wall_ms[rep]=milliseconds(t);
        row.minimum_times[rep]=result.times;
        t=Clock::now();am::compare_output(result.output,s.output);row.checks_ms+=milliseconds(t);
        if(rep==0) row.minimum_work=result.work;
        t=Clock::now();release(result);row.minimum_cleanup_ms[rep]=milliseconds(t);
      };
      const auto event=[&] {
        auto t=Clock::now();auto result=ae::build(m,true);row.event_wall_ms[rep]=milliseconds(t);
        row.event_times[rep]=result.times;
        t=Clock::now();am::compare_output(result.output,s.output);row.checks_ms+=milliseconds(t);
        if(rep==0) row.event_work=result.work;
        t=Clock::now();release(result);row.event_cleanup_ms[rep]=milliseconds(t);
      };
      if(rep%2==0) {minimum();event();}else {event();minimum();}
    }
    need(row.minimum_work.vertices==row.event_work.vertices && row.minimum_work.edges==row.event_work.edges &&
      row.minimum_work.groups==row.event_work.groups && row.minimum_work.contributions==row.contributions &&
      row.event_work.contributions==row.contributions && row.minimum_work.history_entries==row.nodes,"real.work_identity");
    start=Clock::now();release(m);row.manifest_cleanup_ms=milliseconds(start);
    start=Clock::now();release(s);row.slot_cleanup_ms=milliseconds(start);r.rows.push_back(row);
  }
  start=Clock::now();release(capture);r.capture_tail_cleanup_ms=milliseconds(start);r.external_ms=milliseconds(all_start);return r;
}
std::vector<Point3> read_frame(const std::string& name) {
  std::ifstream file(name,std::ios::binary);need(static_cast<bool>(file),"frame.open");
  std::vector<unsigned char> data((std::istreambuf_iterator<char>(file)),{});
  need(!data.empty() && data.size()%12==0,"frame.shape");std::vector<Point3> points(data.size()/12);
  for(size_t i=0;i<points.size();++i) {
    std::array<u32,3> xyz{};for(size_t d=0;d<3;++d) {
      for(size_t j=0;j<4;++j) xyz[d]|=static_cast<u32>(data[12*i+4*d+j])<<(8*j);
      need(xyz[d]<=262143,"frame.u18");
    }
    points[i]={static_cast<mhgp9::gen::Coordinate>(xyz[0]),static_cast<mhgp9::gen::Coordinate>(xyz[1]),static_cast<mhgp9::gen::Coordinate>(xyz[2])};
  }
  return points;
}
void times(const ml::Times& t) {
  std::cout<<"{\"validate_ms\":"<<t.validate_ms<<",\"forest_ms\":"<<t.forest_ms<<",\"ancestors_ms\":"<<t.ancestors_ms
    <<",\"groups_ms\":"<<t.groups_ms<<",\"parents_ms\":"<<t.parents_ms<<",\"histories_ms\":"<<t.histories_ms
    <<",\"output_ms\":"<<t.output_ms<<",\"total_ms\":"<<t.total_ms<<'}';
}
void times(const ae::Times& t) {
  std::cout<<"{\"validate_ms\":"<<t.validate_ms<<",\"forest_ms\":"<<t.forest_ms<<",\"ancestors_ms\":"<<t.ancestors_ms
    <<",\"groups_ms\":"<<t.groups_ms<<",\"parents_ms\":"<<t.parents_ms<<",\"redirects_ms\":"<<t.redirects_ms
    <<",\"output_ms\":"<<t.output_ms<<",\"total_ms\":"<<t.total_ms<<'}';
}
template<class T> void series(const std::array<T,3>& a) {
  std::cout<<'[';for(size_t j=0;j<a.size();++j) {if(j) std::cout<<',';if constexpr(std::is_same_v<T,double>) std::cout<<a[j];else times(a[j]);}std::cout<<']';
}
template<class W> void work(const W& w) {
  std::cout<<"{\"V\":"<<w.vertices<<",\"E\":"<<w.edges<<",\"G\":"<<w.groups<<",\"C\":"<<w.contributions
    <<",\"forest_edges\":"<<w.forest_edges<<",\"components\":"<<w.components<<",\"P_event\":"<<w.parent_event_incidences
    <<",\"P_draft\":"<<w.parent_draft_incidences<<",\"P_forest\":"<<w.parent_forest_incidences
    <<",\"dsu_steps\":"<<w.dsu_steps<<",\"ancestor_entries\":"<<w.ancestor_entries
    <<",\"ancestor_queries\":"<<w.ancestor_queries<<",\"ancestor_steps\":"<<w.ancestor_steps
    <<",\"predecessor_steps\":"<<w.predecessor_steps<<",\"silent_groups\":"<<w.silent_groups
    <<",\"temporary_capacity_observed_max\":"<<w.temporary_capacity_observed_max
    <<",\"combined_capacity_observed_max\":"<<w.combined_capacity_observed_max<<",\"output_capacity\":"<<w.output_capacity;
  if constexpr(std::is_same_v<W,ml::Work>) std::cout<<",\"loss_entries\":"<<w.loss_entries<<",\"history_entries\":"<<w.history_entries
    <<",\"omitted_history_groups\":"<<w.omitted_history_groups<<",\"predecessor_queries\":"<<w.predecessor_queries;
  else std::cout<<",\"continuation_rounds\":"<<w.continuation_rounds<<",\"continuation_tests\":"<<w.continuation_tests;
  std::cout<<'}';
}
void report(const Report& r,const char* mode,double input_ms,double points_cleanup_ms,double process_ms) {
  std::cout<<std::setprecision(12)<<"{\"schema\":\"mhgp9_full_a_real_v1\",\"status\":\"passed\",\"scope\":\"real_catalogue_serial_A_comparison\""
    <<",\"mode\":\""<<mode<<"\",\"family\":\""<<(std::string_view(mode)=="synthetic"?"uniform":"none")<<"\",\"seed\":3,\"s\":8"
    <<",\"n\":"<<r.n<<",\"input_hash_u64\":"<<r.input_hash<<",\"k\":"<<r.k<<",\"workers\":"<<r.workers
    <<",\"static_threads\":"<<r.workers<<",\"hash_grouping\":"<<(r.hashed?"true":"false")
    <<",\"sealed_catalogue\":false,\"GCP_used\":false,\"candidate_parallel\":false,\"FULL_executed_by_candidate\":false"
    <<",\"catalogue_balls\":"<<r.balls<<",\"catalogue_digest\":"<<r.catalogue_digest
    <<",\"presentation_digest\":"<<r.presentation_digest<<",\"observed_native_tower_digest\":"<<r.tower_digest
    <<",\"index_capacity_bytes\":"<<r.index_capacity<<",\"catalogue_capacity_bytes\":"<<r.catalogue_capacity
    <<",\"capture_capacity_bytes_initial\":"<<r.capture_capacity<<",\"peak_rss_kib\":"<<rss()
    <<",\"times_ms\":{\"input\":"<<input_ms<<",\"chain_wall\":"<<r.chain_wall_ms<<",\"chain_reported\":"<<r.chain_reported_ms
    <<",\"chain_catalogue_digest\":"<<r.chain_catalogue_digest_ms<<",\"chain_q2\":"<<r.chain_q2_ms<<",\"chain_q34\":"<<r.chain_q34_ms
    <<",\"chain_census\":"<<r.chain_census_ms<<",\"reindex\":"<<r.reindex_ms<<",\"identity\":"<<r.identity_ms
    <<",\"capture_full_wall\":"<<r.capture_full_wall_ms<<",\"admission\":"<<r.admit_ms<<",\"tower_digest\":"<<r.tower_digest_ms
    <<",\"capture_copy_sum\":"<<r.copy_ms_sum<<",\"upstream_cleanup\":"<<r.upstream_cleanup_ms
    <<",\"capture_tail_cleanup\":"<<r.capture_tail_cleanup_ms<<",\"external\":"<<r.external_ms
    <<",\"points_cleanup\":"<<points_cleanup_ms<<",\"process\":"<<process_ms
    <<",\"native_validate\":"<<r.native_times.validate_ms<<",\"native_static\":"<<r.native_times.static_ms
    <<",\"native_lots\":"<<r.native_times.lots_ms<<",\"native_populations\":"<<r.native_times.populations_ms
    <<",\"native_images\":"<<r.native_times.images_ms<<",\"native_bank\":"<<r.native_times.bank_ms<<",\"native_encode\":"<<r.native_times.encode_ms
    <<"},\"rows\":[";
  bool first=true;for(const auto& row:r.rows) {
    if(!first) std::cout<<',';
    first=false;
    std::cout<<"{\"k\":"<<row.k<<",\"balls\":"<<row.balls<<",\"blocks\":"<<row.blocks
      <<",\"regular_blocks\":"<<row.regular_blocks<<",\"extra_blocks\":"<<row.extra_blocks<<",\"targets\":"<<row.targets
      <<",\"nodes\":"<<row.nodes<<",\"actions\":"<<row.actions<<",\"contributions\":"<<row.contributions<<",\"output_digest\":"<<row.digest
      <<",\"capture_input_capacity_bytes\":"<<row.input_capacity<<",\"capture_output_capacity_bytes\":"<<row.output_capacity
      <<",\"manifest_capacity_bytes\":"<<row.manifest_capacity<<",\"native_a_instrumented_ms\":"<<row.native_a_instrumented_ms
      <<",\"capture_copy_ms\":"<<row.copy_ms<<",\"manifest_ms\":"<<row.manifest_ms<<",\"binding_ms\":"<<row.binding_ms
      <<",\"checks_ms_sum\":"<<row.checks_ms<<",\"manifest_cleanup_ms\":"<<row.manifest_cleanup_ms<<",\"slot_cleanup_ms\":"<<row.slot_cleanup_ms
      <<",\"minimum_work\":";work(row.minimum_work);std::cout<<",\"event_work\":";work(row.event_work);
    std::cout<<",\"minimum_wall_ms\":";series(row.minimum_wall_ms);std::cout<<",\"event_wall_ms\":";series(row.event_wall_ms);
    std::cout<<",\"minimum_cleanup_ms\":";series(row.minimum_cleanup_ms);std::cout<<",\"event_cleanup_ms\":";series(row.event_cleanup_ms);
    std::cout<<",\"minimum_times\":";series(row.minimum_times);std::cout<<",\"event_times\":";series(row.event_times);std::cout<<'}';
  }
  std::cout<<"]}\n";
}
} // namespace ar

int main(int argc,char** argv) {
  try {
    using namespace ar;
    if(argc==2 && std::string_view(argv[1])=="--gate") {
      // Coordinate fixtures explicitly from qualified manifest fixtures.hpp;
      // the catalogue here comes from the real generator, not its oracle.
      std::vector<std::vector<Point3>> fixtures{
        {{0,0,0},{2,0,0},{2,2,0},{0,2,0}},
        {{1,8,0},{5,10,0},{9,8,0},{5,0,0}},
        mhgp9::gen::bench::make_front_fixture(12,"uniform",3).points};
      u64 cases=0,orders=0,comparisons=0,nonidentity=0,continuations=0,extra_blocks=0;
      for(const auto& fixture:fixtures) for(bool reverse:{false,true}) {
        auto points=fixture;if(reverse) std::reverse(points.begin(),points.end());
        const auto ix=mhgp9::build_tower_index(points);for(size_t j=0;j<ix.upos.size();++j) nonidentity+=ix.point_id(static_cast<i32>(j))!=j;
        const auto r=run(points,static_cast<unsigned>(std::min<size_t>(5,points.size())),reverse?4:2,reverse,true);++cases;
        for(const auto& row:r.rows) {++orders;comparisons+=6;continuations+=row.actions-row.nodes;extra_blocks+=row.extra_blocks;}
      }
      need(cases==6 && orders==26 && nonidentity && continuations && extra_blocks,"gate.real_coverage");
      std::cout<<"{\"schema\":\"mhgp9_full_a_real_gate_v1\",\"status\":\"passed\",\"cases\":"<<cases<<",\"orders\":"<<orders
        <<",\"candidate_comparisons\":"<<comparisons<<",\"nonidentity_index_positions\":"<<nonidentity
        <<",\"continuations\":"<<continuations<<",\"extra_blocks\":"<<extra_blocks
        <<",\"GCP_used\":false,\"FULL_executed_by_candidate\":false}\n";return 0;
    }
    need((argc==3 && std::string_view(argv[1])=="--frame") ||
         (argc==4 && std::string_view(argv[1])=="--synthetic" && std::string_view(argv[2])=="uniform"),"real.usage");
    const auto process_start=Clock::now(),input_start=Clock::now();std::vector<Point3> points;const char* mode="frame";
    if(argc==3) points=read_frame(argv[2]);
    else {
      mode="synthetic";size_t n=0;const std::string_view text=argv[3];const auto parsed=std::from_chars(text.data(),text.data()+text.size(),n);
      need(parsed.ec==std::errc{} && parsed.ptr==text.data()+text.size(),"real.synthetic_size");
      points=mhgp9::gen::bench::make_front_fixture(n,"uniform",3).points;
    }
    const double input_ms=milliseconds(input_start);const auto result=run(points,5,4,true,false);
    const auto cleanup_start=Clock::now();release(points);const double cleanup_ms=milliseconds(cleanup_start);
    report(result,mode,input_ms,cleanup_ms,milliseconds(process_start));return 0;
  } catch(const mhgp9::tower::full_ball_detail::Failure& f) {std::cerr<<f.reason<<'\n';return 2;}
    catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 2;}
}
