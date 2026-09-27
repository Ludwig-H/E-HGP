// Standalone attachment consumer. The build makes a one-token main rename
// of the frozen weighted exporter; its API and serializer are unchanged.
#include "native_weighted_export_renamed.hpp"
#include "../../audits/b_full_a_manifest_20260927/native_a.hpp"

namespace attachment_export {
using namespace weighted_export;
namespace am=mhgp9::audit::a_manifest;
struct Attachment {
  std::vector<PointId> vertices;
  ExactLevel beta;
  u64 node=full_ball_detail::absent,anchor_node=full_ball_detail::absent;
  u32 terminal_ball=am::absent32;
  u64 descending_steps=0,same_radius_steps=0;
};
struct Result {
  Export weighted;
  std::vector<u32> anchors;
  std::vector<Attachment> attachments;
  u64 original_digest=0,observed_digest=0;
  u64 intruder_queries=0,intruder_nodes=0,intruder_power_tests=0,interior_ranges=0;
  u64 absent_anchors=0,anchored_balls=0;
  unsigned observed_workers=2;
  AnchorMebWork meb_work;
  double enumeration_ms=0,observed_ms=0,comparison_ms=0,resolution_ms=0;
};
// Explicit small port of b_full_a_real_20260927/probe.cpp::same_tower.
// Digests supplement, never replace, this every-field equality check.
void same_tower(const FullBallTowerResult& a,const FullBallTowerResult& b) {
  need(a.status==FullBallStatus::kCompleteRelative && b.status==FullBallStatus::kCompleteRelative &&
       a.orders.size()==b.orders.size(),"attachment.full_status");
  for(std::size_t k=0;k<a.orders.size();++k) {
    const auto& x=a.orders[k].forest; const auto& y=b.orders[k].forest;
    need(x.order()==y.order() && x.parents()==y.parents() && x.successors()==y.successors() &&
      x.nodes().size()==y.nodes().size() && x.contributions().size()==y.contributions().size() &&
      a.orders[k].lower_nodes==b.orders[k].lower_nodes,"attachment.full_structure");
    for(std::size_t j=0;j<x.nodes().size();++j) {
      const auto& p=x.nodes()[j];const auto& q=y.nodes()[j];
      need(p.level==q.level && p.first==q.first && p.parent_count==q.parent_count,"attachment.full_node");
    }
    for(std::size_t j=0;j<x.contributions().size();++j) {
      const auto& p=x.contributions()[j];const auto& q=y.contributions()[j];
      need(p.level==q.level && p.segment==q.segment && p.ref.population==q.ref.population &&
        p.ref.shell_mask==q.ref.shell_mask && p.ref.include_interior==q.ref.include_interior,"attachment.full_contribution");
    }
    const auto& p=*x.populations();const auto& q=*y.populations();
    need(p.domain()==q.domain() && p.rows().size()==q.rows().size(),"attachment.full_bank");
    for(std::size_t j=0;j<p.rows().size();++j)
      need(p.rows()[j].interior==q.rows()[j].interior && p.rows()[j].shell==q.rows()[j].shell,"attachment.full_population");
  }
}
// Port of Builder::intruder_work, exact lower/upper power bounds. The
// selected geometry ranks stay sorted; no cap or truncation of the search.
i32 intruder(const CloudIndex& ix,const BallKey& key,std::span<const i32> selected,Result& out) {
  ++out.intruder_queries;
  const census_detail::AxisBounds bounds(key);
  std::vector<NodeRef> stack{ix.root()};
  const auto member=[&](i32 id){return std::binary_search(selected.begin(),selected.end(),id);};
  while(!stack.empty()) {
    const auto node=stack.back();stack.pop_back();++out.intruder_nodes;
    i128 lo,hi;bounds.bounds(ix.box_of(node),&lo,&hi);
    if(lo>=0) continue;
    if(hi<0) {
      ++out.interior_ranges;const auto range=ix.range_of(node);
      for(i32 id=range.first;id<=range.last;++id) if(!member(id)) return id;
    } else if(is_leaf(node)) {
      const auto id=leaf_index(node);
      if(!member(id)) {++out.intruder_power_tests;if(key.power(ix.upos[static_cast<std::size_t>(id)])<0) return id;}
    } else {stack.push_back(ix.nodes[static_cast<std::size_t>(node)].right);stack.push_back(ix.nodes[static_cast<std::size_t>(node)].left);}
  }
  return -1;
}
Result attach(std::span<const Point3> points,unsigned k,std::size_t workers,const mhgp9::ChainResult& native) {
  Result out;
  auto start=Clock::now();out.weighted=enumerate(points,k,native);out.enumeration_ms=elapsed(start);
  std::vector<InputPoint> input;
  for(std::size_t id=0;id<points.size();++id) input.push_back({static_cast<PointId>(id),position(points[id])});
  const auto ix=build_cloud_index(input);
  need(ix.valid && !ix.has_duplicate_positions() && ix.upos.size()==points.size(),"attachment.index");
  std::vector<i32> rank(points.size(),-1);
  for(std::size_t u=0;u<ix.upos.size();++u) {
    const auto id=ix.point_id(static_cast<i32>(u));
    need(id<points.size() && rank[id]==-1 && ix.upos[u]==position(points[id]),"attachment.index_identity");
    rank[id]=static_cast<i32>(u);
  }
  am::Capture capture;FullBallTowerResult observed;
  out.observed_workers=static_cast<unsigned>(std::max<std::size_t>(2,workers));
  const FullBallTowerOptions options{.overlap_static=true,.pipelined_tail=true,.hash_grouping=true,.persistent_pool=true};
  start=Clock::now();
  {
    am::CaptureScope scope(capture);
    observed.orders=full_ball_detail::AObservedBuilder(ix,native.catalogue_balls,k,observed.stats,
      static_cast<int>(out.observed_workers),{},true,&observed.times,options,nullptr).run();
  }
  out.observed_ms=elapsed(start);
  observed.status=FullBallStatus::kCompleteRelative;observed.reason="observed_builder_complete";
  need(observed.orders.size()==k,"attachment.observed_orders");
  if(k>1) {
    for(unsigned order=1;order<=k;++order) {
      const auto& slot=capture.slots[order];
      need(slot.starts==1 && slot.finishes==1 && slot.input.k==order,"attachment.capture_complete");
      need(slot.input.spatial_ids.size()==points.size(),"attachment.capture_index_size");
      for(std::size_t u=0;u<points.size();++u)
        need(slot.input.spatial_ids[u]==ix.point_id(static_cast<i32>(u)),"attachment.capture_index_identity");
    }
    capture.complete=true;out.anchors=capture.slots[k].output.anchors;
    need(out.anchors.size()==native.catalogue_balls.size(),"attachment.anchor_domain");
  }
  start=Clock::now();same_tower(native.tower,observed);
  out.original_digest=mhgp9::tower_digest(native.tower);out.observed_digest=mhgp9::tower_digest(observed);
  need(out.original_digest==native.tower_digest && out.original_digest==out.observed_digest,"attachment.digest");
  const auto& forest=native.tower.orders[k-1].forest;
  if(k>1) {
    const auto& slot=capture.slots[k];
    need(slot.output.next==forest.successors(),"attachment.capture_successors");
    for(std::size_t ball=0;ball<out.anchors.size();++ball) {
      const auto& b=native.catalogue_balls[ball];
      const bool active=k>=b.n_interior+b.arity-1 && k<=b.n_interior+b.n_shell;
      need(active==(out.anchors[ball]!=am::absent32),"attachment.anchor_rank_domain");
      if(!active) {++out.absent_anchors;continue;}
      ++out.anchored_balls;
      need(full_coverage_root_at(forest,out.anchors[ball],b.level,true)==out.anchors[ball],"attachment.closed_anchor");
    }
  }
  out.comparison_ms=elapsed(start);
  // Free the instrumented forest/capture only after admitting every slot and
  // comparing every native field. Retain just the fixed-order anchor map.
  observed=FullBallTowerResult{};capture=am::Capture{};
  std::set<std::vector<PointId>> facets;
  for(const auto& coface:out.weighted.cofaces) for(std::size_t omit=0;omit<coface.vertices.size();++omit) {
    auto facet=coface.vertices;facet.erase(facet.begin()+static_cast<std::ptrdiff_t>(omit));facets.insert(std::move(facet));
  }
  start=Clock::now();
  const auto meb=[&](const std::vector<i32>& sites) {
    std::array<P3,kFacetMaxK> p{};
    need(!sites.empty() && sites.size()<=p.size(),"attachment.meb_size");
    for(std::size_t j=0;j<sites.size();++j) p[j]=ix.upos[static_cast<std::size_t>(sites[j])];
    const auto value=anchor_meb(std::span<const P3>(p.data(),sites.size()),out.meb_work);
    need(value.status==AnchorMebStatus::kOk,"attachment.exact_meb");return value;
  };
  for(const auto& facet:facets) {
    Attachment row;row.vertices=facet;
    std::vector<i32> sites;for(auto id:facet) sites.push_back(rank[id]);std::sort(sites.begin(),sites.end());
    auto local=meb(sites);row.beta=local.level;
    if(k==1) {
      // K1's initial nodes are emitted in increasing domain PointId order.
      row.node=facet.front();row.anchor_node=row.node;
      need(row.node<forest.nodes().size() && forest.nodes()[row.node].parent_count==0 &&
        same_exact_level(forest.nodes()[row.node].level,row.beta),"attachment.k1_birth");
      const auto coverage=full_coverage_at(forest,row.node,row.beta,true);
      need(coverage.status==FullCertificateStatus::kOk && coverage.values==facet,"attachment.k1_identity");
    } else for(;;) {
      need(compare_exact_level(local.level,row.beta)<=0,"attachment.above_original_birth");
      const auto it=std::lower_bound(native.catalogue_balls.begin(),native.catalogue_balls.end(),local.key,
        [](const BallData& b,const BallKey& key){return b.key<key;});
      if(it!=native.catalogue_balls.end() && it->key==local.key) {
        const auto ball=static_cast<std::size_t>(it-native.catalogue_balls.begin());
        need(same_exact_level(it->level,local.level),"attachment.terminal_level");
        if(out.anchors[ball]!=am::absent32) {
          row.terminal_ball=static_cast<u32>(ball);row.anchor_node=out.anchors[ball];
          row.node=full_coverage_root_at(forest,row.anchor_node,row.beta,true);
          need(row.node!=kFullCoverageAbsent,"attachment.closed_birth_root");break;
        }
      }
      const auto z=intruder(ix,local.key,sites,out);need(z>=0,"attachment.missing_terminal_or_intruder");
      need(local.support_size>0 && local.support_slots[0]<sites.size(),"attachment.support_slot");
      sites[local.support_slots[0]]=z;std::sort(sites.begin(),sites.end());
      need(std::adjacent_find(sites.begin(),sites.end())==sites.end(),"attachment.exchange_duplicate");
      auto next=meb(sites);const int comparison=compare_exact_level(next.level,local.level);
      need(comparison<=0,"attachment.radius_increase");
      if(comparison==0) {
        need(next.key==local.key && next.selected_shell_count+1==local.selected_shell_count,"attachment.shell_not_decreased");
        ++row.same_radius_steps;
      } else ++row.descending_steps;
      local=next;
    }
    out.attachments.push_back(std::move(row));
  }
  out.resolution_ms=elapsed(start);return out;
}
void output(const Result& data,std::span<const Point3> points,unsigned k,std::size_t workers,
            const mhgp9::ChainResult& native,const Checks& checks,bool replay,double wall,double check_ms) {
  std::cout<<"{\"schema\":\"mhgp9_weighted_full_attachment_export_v1\",\"status\":\"completed\",\"weighted\":";
  weighted_export::output(data.weighted,points,k,workers,native,checks,replay,wall,check_ms,data.enumeration_ms);
  std::cout<<",\"anchors\":[";
  for(std::size_t i=0;i<data.anchors.size();++i) {if(i)std::cout<<',';if(data.anchors[i]==am::absent32)std::cout<<"null";else std::cout<<data.anchors[i];}
  std::cout<<"],\"attachments\":[";
  u64 descent=0,equal=0;
  for(std::size_t i=0;i<data.attachments.size();++i) {
    if(i)std::cout<<',';
    const auto& row=data.attachments[i];descent+=row.descending_steps;equal+=row.same_radius_steps;
    std::cout<<"{\"vertices\":";array(row.vertices);std::cout<<",\"beta\":";level(row.beta);
    std::cout<<",\"node\":"<<row.node<<",\"anchor_node\":"<<row.anchor_node<<",\"terminal_ball\":";
    if(row.terminal_ball==am::absent32)std::cout<<"null";else std::cout<<row.terminal_ball;
    std::cout<<",\"descending_steps\":"<<row.descending_steps<<",\"same_radius_steps\":"<<row.same_radius_steps<<'}';
  }
  std::cout<<"],\"validation\":{\"observed_native_every_field_equal\":true,\"all_capture_slots_admitted\":"<<(k>1?"true":"false")
    <<",\"anchors_available\":"<<(k>1?"true":"false")<<",\"native_tower_digest\":\""<<data.original_digest
    <<"\",\"observed_tower_digest\":\""<<data.observed_digest<<"\"},\"stats\":{\"facets\":"<<data.attachments.size()
    <<",\"anchored_balls\":"<<data.anchored_balls<<",\"absent_anchors\":"<<data.absent_anchors
    <<",\"observed_workers\":"<<data.observed_workers<<",\"descending_steps\":"<<descent<<",\"same_radius_steps\":"<<equal
    <<",\"meb_calls\":"<<data.meb_work.calls<<",\"intruder_queries\":"<<data.intruder_queries
    <<",\"intruder_nodes\":"<<data.intruder_nodes<<",\"intruder_power_tests\":"<<data.intruder_power_tests
    <<",\"interior_ranges\":"<<data.interior_ranges<<",\"observed_full_ms\":"<<data.observed_ms
    <<",\"comparison_ms\":"<<data.comparison_ms<<",\"resolution_ms\":"<<data.resolution_ms<<"}}\n";
}
void gate() {
  const std::vector<Point3> square{{0,0,0},{2,0,0},{2,2,0},{0,2,0}};
  const std::vector<Point3> e5{{0,0,7},{0,9,6},{1,4,0},{0,0,1},{4,1,2}};
  u64 cases=0,facets=0,steps=0;
  for(const auto& points:{square,e5}) for(unsigned k:{1u,2u,3u}) {
    const auto native=run_catalogue(points,k,1);const auto data=attach(points,k,1,native);
    ++cases;facets+=data.attachments.size();
    for(const auto& row:data.attachments) steps+=row.descending_steps+row.same_radius_steps;
    if(k!=2)continue;
    const std::vector<PointId> target{0,2};
    const auto it=std::find_if(data.attachments.begin(),data.attachments.end(),[&](const auto& row){return row.vertices==target;});
    need(it!=data.attachments.end(),"gate.special_facet");
    if(points.size()==4) {
      need(same_exact_level(it->beta,promote_level(Rational128{2,1})) && it->descending_steps==0 && it->same_radius_steps==0,
        "gate.square_diagonal_birth");
      const auto& f=native.tower.orders[1].forest;need(f.nodes()[it->node].parent_count==4,"gate.square_four_way_merge");
      for(const auto& c:f.contributions()) need(!same_exact_level(c.level,it->beta),"gate.square_anchor_without_contribution");
    } else {
      need(same_exact_level(it->beta,promote_level(Rational128{33,2})) && it->descending_steps>0,"gate.e5_silent_birth");
      need(compare_exact_level(native.tower.orders[1].forest.nodes()[it->node].level,it->beta)<0,"gate.e5_attach_older_segment");
    }
  }
  need(steps>0,"gate.intruder_descent_exercised");
  std::cout<<"{\"schema\":\"mhgp9_weighted_attachment_gate_v1\",\"status\":\"passed\",\"cases\":"<<cases
    <<",\"facets\":"<<facets<<",\"exchange_steps\":"<<steps<<",\"square_silent_anchor\":true,\"e5_silent_attachment\":true}\n";
}
} // namespace attachment_export
int main(int argc,char** argv) {
  try {
    using namespace attachment_export;
    if(argc==2 && std::string_view(argv[1])=="--gate") {attachment_export::gate();return 0;}
    const char* input=nullptr;unsigned k=0;std::size_t workers=1;bool replay=false;std::set<std::string_view> seen;
    for(int i=1;i<argc;++i) {
      const std::string_view arg=argv[i];need(seen.insert(arg).second,"duplicate_argument");
      if(arg=="--verify-coverage"){replay=true;continue;}
      need(i+1<argc,"missing_argument_value");const char* value=argv[++i];
      if(arg=="--input")input=value;
      else if(arg=="--k"){const auto x=integer(value);need(x>=1 && x<=10,"k_range_1_10");k=static_cast<unsigned>(x);}
      else if(arg=="--workers"){const auto x=integer(value);need(x>=1 && x<=256,"workers_range");workers=static_cast<std::size_t>(x);}
      else throw std::runtime_error("unknown_argument");
    }
    need(input && k,"usage_input_k");const auto points=read_points(input);need(k<=points.size(),"k_exceeds_point_count");
    auto start=Clock::now();const auto native=run_catalogue(points,k,workers);const auto wall=elapsed(start);
    start=Clock::now();const auto checks=validate(native.tower.orders[k-1].forest,points.size(),replay);const auto check_ms=elapsed(start);
    const auto attached=attach(points,k,workers,native);
    attachment_export::output(attached,points,k,workers,native,checks,replay,wall,check_ms);return 0;
  } catch(const mhgp9::tower::full_ball_detail::Failure& error) {std::cerr<<"native_attachment_export: "<<error.reason<<'\n';return 2;}
    catch(const std::exception& error) {std::cerr<<"native_attachment_export: "<<error.what()<<'\n';return 2;}
}
