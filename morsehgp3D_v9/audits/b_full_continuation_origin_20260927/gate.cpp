// Explicit reuse of the frozen real-draft capture seam, not a product edit.
// It keeps the real geometric generator, native FULL builder and field judge.
#include "capture_helpers.hpp"
#include "../b_full_first_parent_20260927/first.hpp"

namespace origin {
namespace rd = real_drafts;
namespace fp = mhgp9::audit_first_parent;
using namespace mhgp9::tower;
using Point = mhgp9::gen::Point3;
struct Fixture { const char* name; std::vector<Point> points; unsigned k; bool regular; };
std::vector<Fixture> fixtures() {
  // These exact fixtures are attributed to tests/tower/full_ball_tower_gate.cpp.
  return {
    {"pair", {{0,0,0},{2,0,0}}, 2, true},
    {"support3", {{0,0,0},{2,2,0},{2,0,2}}, 3, true},
    {"support4", {{0,0,0},{2,2,0},{2,0,2},{0,2,2}}, 4, true},
    {"u18_tetra", {{0,0,0},{262142,262142,0},{262142,0,262142},{0,262142,262142}}, 4, true},
    {"square", {{0,0,0},{2,0,0},{2,2,0},{0,2,0}}, 4, false},
    {"growth_ABCZ", {{1,8,0},{5,10,0},{9,8,0},{5,0,0}}, 4, false},
    {"growth_ABCZ_doubled_lot", {{1,8,0},{5,10,0},{9,8,0},{5,0,0},
                                 {201,8,0},{205,10,0},{209,8,0},{205,0,0}}, 3, false},
    {"inert_ball", {{2,2,2},{2,0,0},{0,2,0},{0,0,2},{0,0,0}}, 5, false}
  };
}
u64 continuations(const FullCoverageFlatDraft& d) {
  u64 n=0;
  for (size_t a=0;a<d.actions();++a) n += d.parent_begin[a+1]-d.parent_begin[a]==1;
  return n;
}
FullCoverageFlatDraft without_continuations(const FullCoverageFlatDraft& d) {
  FullCoverageFlatDraft out;
  for (size_t b=0;b<d.level.size();++b) {
    bool opened=false;
    for (size_t a=d.batch_begin[b];a<d.batch_begin[b+1];++a) {
      const auto p0=d.parent_begin[a],pn=d.parent_begin[a+1]-p0;
      const auto c0=d.contribution_begin[a],cn=d.contribution_begin[a+1]-c0;
      if (pn==1) continue;
      if (!opened) {out.open_batch(d.level[b]);opened=true;}
      out.add_action(std::span<const FullNodeId>(d.parent).subspan(p0,pn),
                     std::span<const FullCoverageRef>(d.contribution).subspan(c0,cn));
    }
  }
  return out;
}
void mutant() {
  const auto f=fixtures()[5];
  const auto measured=rd::run(f.points,f.k,1,0,false);
  const auto& slot=rd::slots[3];
  rd::need(continuations(slot.draft)>0,"origin.mutant_not_vacuous");
  const auto d=without_continuations(slot.draft);
  fp::Work work;
  const auto trial=fp::encode(3,slot.bank,d,false,fp::Mutant::None,&work);
  const auto& native=measured.chain.tower.orders[2].forest;
  rd::need(work.fast && trial.status==FullCertificateStatus::kOk,"origin.mutant_structurally_accepted");
  rd::need(native.nodes().size()==trial.nodes.size() && native.parents()==trial.parents &&
           native.successors()==trial.successors && native.contributions().size()>trial.contributions.size(),
           "origin.mutant_loses_only_dated_growth");
  for (size_t i=0;i<trial.nodes.size();++i) {
    const auto& a=native.nodes()[i];const auto& b=trial.nodes[i];
    rd::need(a.level==b.level && a.first==b.first && a.parent_count==b.parent_count,"origin.mutant_nodes");
  }
  std::cout<<"{\"schema\":\"mhgp9_continuation_origin_mutant_v1\",\"status\":\"killed\",\"cause\":\"dated_contribution_lost_with_identical_topology\"}\n";
}
void gate() {
  u64 chains=0,orders=0,regular_chains=0,extended_chains=0,fast=0,fallback=0,growth=0,extended_without=0;
  for (auto f:fixtures()) for (unsigned permutation=0;permutation<2;++permutation) {
    if (permutation) std::reverse(f.points.begin(),f.points.end());
    for (int threads:{0,4}) {
      const auto measured=rd::run(f.points,f.k,threads==0?1:4,threads,false);
      const bool regular=measured.chain.catalogue.extra_shell_balls==0;
      if (f.regular) rd::need(regular,"origin.declared_regular_fixture");
      ++chains;regular_chains+=regular;extended_chains+=!regular;
      u64 local=0;
      for (unsigned k=1;k<=f.k;++k) {
        const auto& slot=rd::slots[k];
        const auto count=continuations(slot.draft);local+=count;
        if (regular) rd::need(count==0,"origin.regular_has_no_continuation");
        for (bool reverse:{false,true}) {
          fp::Work work;
          const auto trial=fp::encode(k,slot.bank,slot.draft,reverse,fp::Mutant::None,&work);
          rd::equal(measured.chain.tower.orders[k-1].forest,trial);
          rd::need(work.fallback==(count!=0) && work.fast==(count==0),"origin.actual_path");
          fast+=work.fast;fallback+=work.fallback;
        }
        ++orders;
      }
      growth+=local;extended_without+=!regular && local==0;
      if (std::string_view(f.name).starts_with("growth_")) rd::need(local>0,"origin.native_growth_exercised");
      if (std::string_view(f.name).ends_with("doubled_lot")) rd::need(measured.chain.tower_stats.grouped_lots>0,"origin.grouped_lot");
    }
  }
  rd::need(chains==32 && regular_chains>=16 && extended_chains>0 && extended_without>0 &&
           orders>100 && growth>0 && fast>0 && fallback>0,"origin.nonvacuity");
  std::cout<<"{\"schema\":\"mhgp9_continuation_origin_v1\",\"status\":\"passed\",\"chains\":"<<chains
    <<",\"orders\":"<<orders<<",\"regular_chains\":"<<regular_chains<<",\"extended_chains\":"<<extended_chains
    <<",\"extended_without_continuation\":"<<extended_without<<",\"continuation_actions\":"<<growth
    <<",\"fast_comparisons\":"<<fast<<",\"fallback_comparisons\":"<<fallback<<",\"GCP_used\":false}\n";
}
} // namespace origin
int main(int argc,char** argv) {
  try {
    if (argc==2 && std::string_view(argv[1])=="--drop-continuations") {origin::mutant();return 1;}
    real_drafts::need(argc==1,"origin.usage");origin::gate();return 0;
  } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 2;}
}
