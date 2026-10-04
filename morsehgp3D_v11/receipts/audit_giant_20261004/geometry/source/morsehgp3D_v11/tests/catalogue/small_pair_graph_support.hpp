// Comparateurs de champs et visite de feuille complete ; aucun padding natif compare.
#pragma once
#include "single_pass_support.hpp"
#include "catalogue/small_pair_graph.hpp"

namespace pair_test {
using namespace single_test;

inline bool same_level(const num::Level& a, const num::Level& b) {
  return num::compare(num::to_wide(a.numerator()),num::to_wide(b.numerator()))==0 &&
         num::compare(num::to_wide(a.denominator()),num::to_wide(b.denominator()))==0;
}
inline bool same_ball(const CatalogueBall& a, const CatalogueBall& b) {
  return a.support==b.support && a.rank==b.rank && a.p==b.p && a.m==b.m && a.qmin==b.qmin;
}
inline bool same_geometry(const Catalogue& a,const Catalogue& b) {
  if (a.kmax()!=b.kmax() || a.balls()!=b.balls() || a.levels().size()!=b.levels().size() ||
      a.population().size()!=b.population().size() || a.population_offsets().size()!=b.population_offsets().size())
    return false;
  for (u32 i=0;i<a.balls();++i) if (!same_ball(a.balls_data()[i],b.balls_data()[i])) return false;
  for (u64 i=0;i<a.levels().size();++i) if (!same_level(a.levels()[i],b.levels()[i])) return false;
  return std::equal(a.population().begin(),a.population().end(),b.population().begin()) &&
         std::equal(a.population_offsets().begin(),a.population_offsets().end(),b.population_offsets().begin());
}
inline bool same_work(CatalogueLedger before,CatalogueLedger after) {
  if (before.region_pair_rejects>before.prefixes || after.region_pair_rejects>before.region_pair_rejects ||
      after.region_pair_tests>before.region_pair_tests ||
      after.prefixes!=before.prefixes-before.region_pair_rejects+after.region_pair_rejects) return false;
  before.prefixes=after.prefixes=0;
  before.region_pair_tests=after.region_pair_tests=0;
  before.region_pair_rejects=after.region_pair_rejects=0;
  return before==after;
}
struct LeafReply {
  Buffer<Emission> records;
  Buffer<SiteIdx> population;
  CatalogueLedger ledger;
};
inline Result<LeafReply> visit(const Cloud& cloud,Box box,bool graph,bool cache,MemoryBudget& budget) {
  CatalogueParams p; p.kmax=5; p.pair_graph=graph; p.cache_center_lines=cache;
  Workspace workspace;
  MHGP11_TRY(workspace.allocate(cloud.sites(),budget,cache,graph));
  Buffer<SiteIdx> sites;
  MHGP11_TRY(sites.allocate(cloud.sites(),budget));
  for (u32 i=0;i<cloud.sites();++i) sites[i]=SiteIdx{i};
  Collector count;
  Run first{cloud,p,budget,workspace,count,{}};
  MHGP11_TRY(enumerate_leaf(first,sites.span(),box));
  LeafReply result;
  MHGP11_TRY(result.records.allocate(count.balls,budget));
  MHGP11_TRY(result.population.allocate(count.incidences,budget));
  Collector fill{true,result.records.span(),result.population.span(),0,0};
  Run second{cloud,p,budget,workspace,fill,{}};
  MHGP11_TRY(enumerate_leaf(second,sites.span(),box));
  if (fill.balls!=count.balls || fill.incidences!=count.incidences || first.ledger!=second.ledger)
    return fail(Reason::catalogue_invariant);
  result.ledger=first.ledger;
  return result;
}
inline bool same_order(const LeafReply& a,const LeafReply& b) {
  if (a.records.size()!=b.records.size() || a.population.size()!=b.population.size()) return false;
  for (u64 i=0;i<a.records.size();++i) {
    const auto& x=a.records[i]; const auto& y=b.records[i];
    if (!same_ball(x.ball,y.ball) || !same_level(x.level,y.level) || x.population_begin!=y.population_begin) return false;
  }
  return std::equal(a.population.span().begin(),a.population.span().end(),b.population.span().begin());
}
inline std::vector<Xyz> line(u32 count) {
  std::vector<Xyz> out;
  for (u32 i=0;i<count;++i) out.push_back({2*i,0,0});
  return out;
}
inline std::vector<std::vector<Xyz>> fixtures() {
  return {{{0,0,0},{4,0,0}}, {{0,0,0},{4,0,0},{2,3,0}},
          {{0,0,0},{2,2,0},{2,0,2},{0,2,2}},
          {{1,2,6},{8,4,8},{2,1,3},{7,8,5}},
          {{10,5,5},{9,8,5},{5,2,1},{1,5,8},{9,2,5}},
          {{0,0,0},{4,0,0},{0,4,0},{4,4,0},{0,0,4},{4,0,4},{0,4,4},{4,4,4}},
          {{0,0,0},{kCoordMax,kCoordMax,0},{kCoordMax,0,kCoordMax},{0,kCoordMax,kCoordMax}}};
}
}  // namespace pair_test
