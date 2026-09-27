// New, explicitly separate consumer of the complete v9 ball catalogue.
// Reuse the frozen read-only T_K serializer, never its catalogue-dropping run().
#define main mhgp9_frozen_fixed_k_export_main
#include "../../audits/b_point_hierarchy_k_20260927/native_export.cpp"
#undef main

#include <bit>
#include <functional>
#include <numeric>

namespace weighted_export {
using namespace fixed_k;
using Mask = local_plateau::Mask;
struct Ball {
  BallKey key;
  ExactLevel beta;
  unsigned q_min;
  std::vector<PointId> interior, shell;
  std::vector<Mask> minimal_support_masks;
};
struct Coface {
  std::vector<PointId> vertices;
  ExactLevel beta;
  std::size_t ball;
  Mask shell_mask;
};
struct Export {
  std::vector<Ball> balls;
  std::vector<Coface> cofaces;
  u64 regular_balls=0, extra_balls=0, support_table_slots=0;
  u64 cardinality_candidates=0, rejected_center_masks=0, max_shell=0;
};
P3 position(const Point3& p) { return {p.x,p.y,p.z}; }
mhgp9::ChainResult run_catalogue(std::span<const Point3> points,unsigned k,std::size_t workers) {
  mhgp9::ChainOptions options;
  options.kmax=k; options.workers=workers; options.keep_catalogue=true; options.catalogue_digest=true;
  auto result=mhgp9::run_tower_chain(points,options);
  need(result.status==mhgp9::ChainStatus::kComplete,result.reason.c_str());
  need(result.tower.status==FullBallStatus::kCompleteRelative && result.tower.orders.size()==k &&
       result.kmax_effective==k,"complete_requested_tower");
  return result;
}
Export enumerate(std::span<const Point3> points,unsigned k,const mhgp9::ChainResult& result) {
  need(k>=1 && k<=10 && k<=points.size(),"export_order_domain");
  need(result.status==mhgp9::ChainStatus::kComplete && result.kmax_effective==k,"export_complete_catalogue");
  // Same deterministic construction as tower_chain.cpp:1020. Only restore
  // Morton-rank -> input-PointId; do not regenerate any ball or coface.
  std::vector<InputPoint> input;
  for(std::size_t i=0;i<points.size();++i) input.push_back({static_cast<PointId>(i),position(points[i])});
  const auto ix=build_cloud_index(input);
  need(ix.valid && !ix.has_duplicate_positions() && ix.upos.size()==points.size(),"export_unique_index");
  Export out;
  out.balls.reserve(result.catalogue_balls.size());
  for(const auto& source:result.catalogue_balls) {
    need(source.n_shell>=2 && source.n_shell<=kBallShellMax && source.n_interior<=kBallInteriorMax,
         "export_population_domain");
    need(source.n_interior+source.arity<=k+1,"export_rank_window");
    if(!out.balls.empty()) need(out.balls.back().key<source.key,"export_catalogue_strict_key_order");
    Ball ball{source.key,source.level,source.arity,{},{},{}};
    const auto convert=[&](std::span<const i32> ranks,bool shell) {
      std::vector<PointId> ids;
      for(const auto rank:ranks) {
        need(rank>=0 && static_cast<std::size_t>(rank)<ix.upos.size(),"export_geometry_rank");
        const auto id=ix.point_id(rank);
        need(id<points.size() && ix.upos[static_cast<std::size_t>(rank)]==position(points[id]),"export_rank_identity");
        const auto power=source.key.power(position(points[id]));
        need(shell ? power==0 : power<0,"export_population_power");
        ids.push_back(id);
      }
      std::sort(ids.begin(),ids.end());
      need(std::adjacent_find(ids.begin(),ids.end())==ids.end(),"export_duplicate_population_id");
      return ids;
    };
    ball.interior=convert(source.interior(),false); ball.shell=convert(source.shell(),true);
    out.max_shell=std::max<u64>(out.max_shell,ball.shell.size());
    const auto wanted=k+1-ball.interior.size();
    const auto emit=[&](Mask mask) {
      Coface c{ball.interior,ball.beta,out.balls.size(),mask};
      for(std::size_t bit=0;bit<ball.shell.size();++bit) if(mask&(Mask{1}<<bit)) c.vertices.push_back(ball.shell[bit]);
      std::sort(c.vertices.begin(),c.vertices.end());
      need(c.vertices.size()==k+1 && std::adjacent_find(c.vertices.begin(),c.vertices.end())==c.vertices.end(),
           "export_coface_cardinality");
      out.cofaces.push_back(std::move(c));
    };
    if(ball.shell.size()==ball.q_min) {
      // Chain already certifies this complete shell is a strictly positive
      // minimal support. No proper subset contains its center.
      ++out.regular_balls;
      const auto all=static_cast<Mask>((1u<<ball.shell.size())-1);
      ball.minimal_support_masks.push_back(all);
      if(wanted==ball.shell.size()) { ++out.cardinality_candidates; emit(all); }
    } else {
      ++out.extra_balls;
      local_plateau::LocalCensus census{ball.key,{},{}};
      for(auto id:ball.interior) census.interior.push_back({id,position(points[id])});
      for(auto id:ball.shell) census.shell.push_back({id,position(points[id])});
      const auto table=local_plateau::ShellTable::prepare(std::move(census));
      need(table.q_min()==ball.q_min,"export_qmin_mismatch");
      ball.minimal_support_masks=table.minimal_supports();
      const auto& contains=table.contains_center();
      out.support_table_slots+=contains.size();
      for(unsigned mask=1;mask<contains.size();++mask) {
        if(static_cast<std::size_t>(std::popcount(mask))!=wanted) continue;
        ++out.cardinality_candidates;
        if(contains[mask]) emit(static_cast<Mask>(mask)); else ++out.rejected_center_masks;
      }
    }
    // Keep every catalogue ball, even if it emits no coface of this order or
    // is topologically inert. Inert does not imply zero Chapter-9 weight.
    out.balls.push_back(std::move(ball));
  }
  std::sort(out.cofaces.begin(),out.cofaces.end(),[](const auto& a,const auto& b){return a.vertices<b.vertices;});
  for(std::size_t i=1;i<out.cofaces.size();++i)
    need(out.cofaces[i-1].vertices!=out.cofaces[i].vertices,"export_duplicate_coface");
  return out;
}
std::string signed_decimal(i128 value) {
  const auto magnitude=uabs128(value);
  return (value<0 ? "-" : "")+decimal({static_cast<u64>(magnitude),static_cast<u64>(magnitude>>64),0});
}
void output(const Export& data,std::span<const Point3> points,unsigned k,std::size_t workers,
            const mhgp9::ChainResult& result,const Checks& checks,bool replay,double wall,double validation_ms,double export_ms) {
  std::cout<<"{\"schema\":\"mhgp9_weighted_catalogue_export_v1\",\"status\":\"completed\","
    "\"catalog_universe\":\"gabriel_complete_boundary\",\"beta_unit\":\"squared_radius_grid_units\","
    "\"catalogue_rank_window\":\"interior_count_plus_q_min_le_K_plus_1\","
    "\"outside_shell_boundary_allowed\":true,\"shell_cap\":12,\"native\":";
  fixed_k::output(points,k,workers,result,checks,replay,wall,validation_ms);
  std::cout<<",\"catalogue\":[";
  for(std::size_t i=0;i<data.balls.size();++i) {
    if(i) std::cout<<',';
    const auto& ball=data.balls[i];
    std::cout<<"{\"id\":"<<i<<",\"key\":{\"a\":\""<<signed_decimal(ball.key.a)<<"\",\"b\":[";
    for(unsigned axis=0;axis<3;++axis) { if(axis) std::cout<<','; std::cout<<'"'<<signed_decimal(ball.key.b[axis])<<'"'; }
    std::cout<<"],\"c\":\""<<signed_decimal(ball.key.c)<<"\"},\"beta\":"; level(ball.beta);
    std::cout<<",\"q_min\":"<<ball.q_min<<",\"interior\":"; array(ball.interior);
    std::cout<<",\"shell\":"; array(ball.shell);
    std::cout<<",\"minimal_support_masks\":"; array(ball.minimal_support_masks); std::cout<<'}';
  }
  std::cout<<"],\"cofaces\":[";
  for(std::size_t i=0;i<data.cofaces.size();++i) {
    if(i) std::cout<<',';
    const auto& c=data.cofaces[i];
    std::cout<<"{\"vertices\":"; array(c.vertices); std::cout<<",\"beta\":"; level(c.beta);
    std::cout<<",\"ball\":"<<c.ball<<",\"shell_mask\":"<<c.shell_mask<<'}';
  }
  std::cout<<"],\"stats\":{\"catalogue_balls\":"<<data.balls.size()<<",\"regular_balls\":"<<data.regular_balls
    <<",\"extra_balls\":"<<data.extra_balls<<",\"cofaces\":"<<data.cofaces.size()
    <<",\"support_table_slots\":"<<data.support_table_slots<<",\"cardinality_candidates\":"<<data.cardinality_candidates
    <<",\"rejected_center_masks\":"<<data.rejected_center_masks<<",\"max_shell\":"<<data.max_shell
    <<",\"global_combinations_enumerated\":0,\"index_and_enumeration_ms\":"<<export_ms<<"}}\n";
}

// Tiny gate only: enumerates all cofaces, and enumerates positive supports of
// size <=4 to certify their MEB. The production path never calls this oracle.
template<class Fn> void combinations(std::size_t n,std::size_t size,Fn fn) {
  std::vector<PointId> ids;
  const auto visit=[&](auto&& self,std::size_t begin)->void {
    if(ids.size()==size) { fn(ids); return; }
    for(std::size_t i=begin;i<n && n-i>=size-ids.size();++i) {
      ids.push_back(static_cast<PointId>(i)); self(self,i+1); ids.pop_back();
    }
  };
  visit(visit,0);
}
AnchorMebResult tiny_meb(std::span<const Point3> points,const std::vector<PointId>& ids) {
  AnchorMebResult found;
  for(std::size_t q=2;q<=std::min<std::size_t>(4,ids.size());++q) {
    combinations(ids.size(),q,[&](const auto& slots) {
      if(found.status==AnchorMebStatus::kOk) return;
      std::vector<P3> support;
      for(auto slot:slots) support.push_back(position(points[ids[slot]]));
      AnchorMebWork work;
      const auto candidate=anchor_meb(support,work);
      need(candidate.status==AnchorMebStatus::kOk,"gate_support_meb");
      for(auto id:ids) if(candidate.key.power(position(points[id]))>0) return;
      found=candidate;
    });
    if(found.status==AnchorMebStatus::kOk) return found;
  }
  throw std::runtime_error("gate_meb_not_found");
}
void gate() {
  need(signed_decimal(-123)=="-123" && signed_decimal(0)=="0","signed_decimal");
  const std::vector<std::pair<std::vector<Point3>,std::vector<unsigned>>> fixtures{
    {{{0,0,0},{2,0,0},{2,2,0},{0,2,0}},{1,2,3,4}},
    {{{3,1,1},{1,3,1},{1,1,3},{0,1,1},{1,0,1},{1,1,0},{1,1,1}},{1,2,3,5}},
    {{{2,1,1},{0,1,1},{1,2,1},{1,0,1},{1,1,2},{1,1,0}},{1,2,3,5}},
    {{{0,0,0},{4,0,0},{0,4,0},{0,0,4},{7,3,2},{9,8,5},{2,9,11},
       {13,6,5},{5,17,3},{21,4,19},{8,12,23},{27,29,31}},{5,10}}};
  u64 cases=0, subsets=0, cofaces=0, rejected=0, coverage_queries=0;
  for(const auto& [original,orders]:fixtures) for(bool reverse:{false,true}) for(auto k:orders) {
    auto points=original; if(reverse) std::reverse(points.begin(),points.end());
    const auto result=run_catalogue(points,k,1);
    const auto actual=enumerate(points,k,result);
    std::map<std::vector<PointId>,ExactLevel> expected;
    combinations(points.size(),k+1,[&](const auto& ids) {
      ++subsets;
      const auto meb=tiny_meb(points,ids);
      for(PointId id=0;id<points.size();++id)
        if(!std::binary_search(ids.begin(),ids.end(),id) && meb.key.power(position(points[id]))<0) return;
      expected.emplace(ids,meb.level);
    });
    need(expected.size()==actual.cofaces.size(),"gate_exhaustive_coface_count");
    for(const auto& c:actual.cofaces) {
      const auto it=expected.find(c.vertices);
      need(it!=expected.end() && same_exact_level(it->second,c.beta),"gate_exhaustive_coface_identity_level");
    }
    const auto checks=validate(result.tower.orders[k-1].forest,points.size(),true);
    ++cases; cofaces+=actual.cofaces.size(); rejected+=actual.rejected_center_masks;
    coverage_queries+=checks.coverage_queries;
  }
  need(rejected>0,"gate_qmin_alone_not_sufficient");
  std::vector<Point3> cap;
  for(const auto p:std::vector<Point3>{{5,0,0},{0,5,0},{-5,0,0},{0,-5,0},{3,4,0},{3,-4,0},
       {-3,4,0},{-3,-4,0},{4,3,0},{4,-3,0},{-4,3,0},{-4,-3,0},{0,0,5},{0,0,-5}})
    cap.push_back({p.x+5,p.y+5,p.z+5});
  mhgp9::ChainOptions options; options.kmax=1; options.keep_catalogue=true;
  const auto unsupported=mhgp9::run_tower_chain(cap,options);
  need(unsupported.status==mhgp9::ChainStatus::kUnsupportedDegeneracy,"gate_shell_cap_must_refuse");
  std::cout<<"{\"schema\":\"mhgp9_weighted_export_gate_v1\",\"status\":\"passed\",\"cases\":"<<cases
    <<",\"exhaustive_subsets\":"<<subsets<<",\"cofaces\":"<<cofaces<<",\"rejected_center_masks\":"<<rejected
    <<",\"coverage_queries\":"<<coverage_queries<<",\"unsupported_shell_checked\":true}\n";
}
} // namespace weighted_export

int main(int argc,char** argv) {
  try {
    using namespace weighted_export;
    if(argc==2 && std::string_view(argv[1])=="--gate") { weighted_export::gate(); return 0; }
    const char* input=nullptr; unsigned k=0; std::size_t workers=1; bool replay=false;
    std::set<std::string_view> seen;
    for(int i=1;i<argc;++i) {
      const std::string_view arg=argv[i]; need(seen.insert(arg).second,"duplicate_argument");
      if(arg=="--verify-coverage") { replay=true; continue; }
      need(i+1<argc,"missing_argument_value"); const char* value=argv[++i];
      if(arg=="--input") input=value;
      else if(arg=="--k") { const auto x=integer(value); need(x>=1 && x<=10,"k_range_1_10"); k=static_cast<unsigned>(x); }
      else if(arg=="--workers") { const auto x=integer(value); need(x>=1 && x<=256,"workers_range"); workers=static_cast<std::size_t>(x); }
      else throw std::runtime_error("unknown_argument");
    }
    need(input && k,"usage_input_k"); const auto points=read_points(input); need(k<=points.size(),"k_exceeds_point_count");
    const auto start=Clock::now(); const auto result=run_catalogue(points,k,workers); const auto wall=elapsed(start);
    const auto validation_start=Clock::now(); const auto checks=validate(result.tower.orders[k-1].forest,points.size(),replay);
    const auto validation_ms=elapsed(validation_start);
    const auto export_start=Clock::now(); const auto data=enumerate(points,k,result); const auto export_ms=elapsed(export_start);
    weighted_export::output(data,points,k,workers,result,checks,replay,wall,validation_ms,export_ms); return 0;
  } catch(const std::exception& e) { std::cerr<<"native_weighted_export: "<<e.what()<<'\n'; return 2; }
}
