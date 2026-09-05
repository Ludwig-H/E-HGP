// Overlay-only D/E differential, production branches without MHGP7_TESTING.
// Fixtures explicitly ported from tests/meb_lazy_gate.cpp SHA122807a3fe431bd9658262f8061bcb7e2258a7832516ceff918da52d08ac3a55.
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include "../v7_meb_q2_review/source_D.hpp"
#if defined(MHGP7_Q2_REVIEW_SHELL_MUTANT)
#include "variant_shell_mutant.hpp"
#else
#include "variant.hpp"
#endif
#if defined(MHGP7_TESTING)
#error This differential must compile both production branches without MHGP7_TESTING
#endif
using namespace mhgp7;
namespace {
void check(bool p, const char* label) { if (!p) throw std::runtime_error(label); }
struct Fixture { std::vector<P3> pts; CloudIndex index; std::array<i32,11> sites{}; };
Fixture make(std::vector<P3> pts) {
  Fixture f; f.pts=std::move(pts); f.index=build_cloud_index(f.pts);
  check(f.index.valid && !f.index.has_duplicate_positions(), "fixture.index");
  for (size_t p=0;p<f.pts.size();++p) {
    bool seen=false;
    for (i32 u=0;u<f.index.unique_count();++u) if (f.index.point_id(u)==p) { f.sites[p]=u;seen=true;break; }
    check(seen,"fixture.point_id");
  }
  return f;
}
std::vector<Fixture> fixtures() {
  std::vector<Fixture> all;
  for (const auto& pts : std::vector<std::vector<P3>>{
       {{0,0,0},{65535,65535,65535}},
       {{0,0,0},{65535,65535,0},{65535,0,65535}},
       {{0,0,0},{65535,65535,0},{65535,0,65535},{0,65535,65535}},
       {{0,0,0},{8,0,0},{8,8,0},{0,8,0}},
       {{0,0,7},{0,9,6},{1,4,0},{0,0,1},{4,1,2}},
       {{0,0,0},{4,0,0},{2,0,0}},
       {{0,0,0},{4,0,0},{2,3,0},{2,0,2}},
       {{0,0,0},{46368,28657,0},{28657,17711,0}}}) all.push_back(make(pts));
  // Fixed integer stream, no floating point or random-device dependency.
  u64 state=0x6d65622d76372d31ULL;
  const auto next=[&]() { state=state*6364136223846793005ULL+1442695040888963407ULL; return state; };
  for (size_t k=0;k<160;++k) {
    std::vector<P3> pts;
    const size_t n=2+k%10;
    for (size_t j=0;j<n;++j) pts.push_back(P3{(i64)((next()>>32)&65535),(i64)((next()>>32)&65535),(i64)((next()>>32)&65535)});
    all.push_back(make(pts));
  }
  return all;
}
template<class A, class B>
bool stats_equal(const A& a,const B& b) {
  return a.core_records==b.core_records && a.core_facets==b.core_facets &&
    a.facets_with_two_intruders==b.facets_with_two_intruders && a.chain_steps==b.chain_steps &&
    a.added_cofaces==b.added_cofaces && a.terminal_direct==b.terminal_direct && a.terminal_cached==b.terminal_cached &&
    a.max_chain_length==b.max_chain_length && a.query_nodes==b.query_nodes && a.query_leaves==b.query_leaves &&
    a.query_range_skips==b.query_range_skips && a.meb_calls==b.meb_calls && a.meb_supports==b.meb_supports;
}

u64 cases=0,success=0,degenerate=0,capped=0,q2=0,q3=0,q4=0;
u64 identity_checks=0,positive=0,negative=0,zero=0,foreign_zero=0;
u64 q2_attempts=0,q2_rejected=0,q2_selected=0,q2_probe_checks=0,q2_extra_accept_checks=0;
u64 uncapped_attempts=0,uncapped_rejected=0,uncapped_selected=0,uncapped_checks=0;
u64 profile_refusals=0,extreme_positive=0,extreme_negative=0;
bool last_reference_ok=false;
u8 last_reference_q=0;
i64 relative_power(const P3& a,const P3& b,const P3& z) {
  check(p3_in_profile(a)&&p3_in_profile(b)&&p3_in_profile(z),"predicate.profile");
  const i64 power=p3_dot(p3_sub(z,a),p3_sub(z,b));
  check(power>=-12884508675LL&&power<=12884508675LL,"predicate.i64_bound");
  check((i128)power==q2_ball_key(a,b).power(z),"predicate.exact_identity");
  ++identity_checks;positive+=power>0;negative+=power<0;zero+=power==0;
  foreign_zero+=(power==0&&z!=a&&z!=b);
  return power;
}
void predicate_fixtures() {
  // All cube corners, including zero-radius algebraic identities only here.
  // MEB fixtures below still require distinct positions and n>=2.
  std::vector<P3> corners;
  for(int x=0;x<2;++x)for(int y=0;y<2;++y)for(int z=0;z<2;++z)
    corners.push_back({x*65535LL,y*65535LL,z*65535LL});
  for(const auto& a:corners)for(const auto& b:corners)for(const auto& z:corners)
    (void)relative_power(a,b,z);
  const i64 high=relative_power({0,0,0},{0,0,1},{65535,65535,65535});
  check(high==12884443140LL&&high>2147483647LL,"predicate.high_literal");
  extreme_positive=1;
  const i64 low=relative_power({0,0,0},{65535,65535,65535},{32767,32767,32767});
  check(low==-3221127168LL&&low<-2147483647LL-1,"predicate.low_literal");
  extreme_negative=1;
  for(const P3 bad:std::array<P3,2>{{{-1,0,0},{65536,0,0}}}) {
    check(!p3_in_profile(bad),"fixture.invalid_position");
    check(!build_cloud_index(std::vector<P3>{{0,0,0},bad}).valid,"fixture.invalid_index_refused");
    ++profile_refusals; // Never call the arithmetic predicate on invalid input.
  }
}
// Explicit shadow of charged q2 search, judged against the unchanged D result.
// Logical call counts only: this is neither instrumentation nor a cycle measure.
void q2_shadow(const Fixture& f,u64 cap,const silent_detail::LocalBall& selected,bool ok,int status,bool nominal) {
  u64 attempted=0,rejected=0,chosen=0,checks=0;
  for(size_t a=0;a<f.pts.size()&&!chosen;++a)
    for(size_t b=a+1;b<f.pts.size()&&!chosen;++b) {
      if(attempted>=cap)goto finished;
      ++attempted;
      bool contains=true;
      const auto& pa=f.index.upos[(size_t)f.sites[a]];
      const auto& pb=f.index.upos[(size_t)f.sites[b]];
      for(size_t i=0;i<f.pts.size();++i) {
        ++checks;
        if(relative_power(pa,pb,f.index.upos[(size_t)f.sites[i]])>0) {contains=false;break;}
      }
      if(contains)++chosen;else ++rejected;
    }
finished:
  const bool selected_q2=(ok||status==(int)SilentIncidenceStatus::kUnsupportedDegeneracy)&&selected.q==2;
  check(chosen==(selected_q2?1u:0u),"shadow.selected_q2");
  check(attempted==rejected+chosen,"shadow.conservation");
  q2_attempts+=attempted;q2_rejected+=rejected;q2_selected+=chosen;
  q2_probe_checks+=checks;q2_extra_accept_checks+=chosen*f.pts.size();
  if(nominal) {
    uncapped_attempts+=attempted;uncapped_rejected+=rejected;
    uncapped_selected+=chosen;uncapped_checks+=checks;
  }
}
template<class Ball>
void seed_output(Ball& b) {
  b.key=BallKey{17,{19,23,29},31};b.level=ExactLevel{{37,41,43},47};
  b.q=9;b.support={53,59,61,67}; // Output sentinel, never input to a predicate.
}
u64 one(const Fixture& f,u64 cap,bool nominal=false) {
  const std::vector<ForestEvent> direct;
  SilentIncidenceLimits ca;ca.max_meb_supports=cap;
  meb_q2_variant::SilentIncidenceLimits cb;cb.max_meb_supports=cap;
  SilentIncidenceResult ra;
  meb_q2_variant::SilentIncidenceResult rb;
  silent_detail::LocalBall ba;
  meb_q2_variant::silent_detail::LocalBall bb;
  seed_output(ba);seed_output(bb);
  silent_detail::Builder a(f.index,direct,ca,&ra);
  meb_q2_variant::silent_detail::Builder b(f.index,direct,cb,&rb);
  const bool oka=a.miniball(f.sites,f.pts.size(),&ba);
  last_reference_ok=oka;last_reference_q=ba.q;
  q2_shadow(f,cap,ba,oka,(int)ra.status,nominal);
  const bool okb=b.miniball(f.sites,f.pts.size(),&bb);
  check(oka==okb&&(int)ra.status==(int)rb.status&&std::strcmp(ra.reason,rb.reason)==0,"differential.status_reason");
  check(stats_equal(ra.stats,rb.stats),"differential.stats");
  check(ba.key==bb.key&&ba.level==bb.level&&ba.q==bb.q&&ba.support==bb.support,"differential.ball_literal_fields");
  check(ra.events.empty()&&rb.events.empty(),"differential.local_events_only");
  ++cases;
  if(oka) {++success;if(nominal){q2+=ba.q==2;q3+=ba.q==3;q4+=ba.q==4;}}
  else if(ra.status==SilentIncidenceStatus::kUnsupportedDegeneracy)++degenerate;
  else if(ra.status==SilentIncidenceStatus::kResourceExhausted)++capped;
  return ra.stats.meb_supports;
}
void report(const char* status,const char* divergence) {
  std::printf("meb_q2_differential=%s divergence=%s public_status=not_claimed\n",status,divergence);
  std::printf("cases=%llu success=%llu degenerate=%llu capped=%llu q2=%llu q3=%llu q4=%llu predicate_identities=%llu positive=%llu negative=%llu zero=%llu foreign_zero=%llu profile_refusals=%llu extreme_positive=%llu extreme_negative=%llu\n",
    (unsigned long long)cases,(unsigned long long)success,(unsigned long long)degenerate,(unsigned long long)capped,
    (unsigned long long)q2,(unsigned long long)q3,(unsigned long long)q4,(unsigned long long)identity_checks,
    (unsigned long long)positive,(unsigned long long)negative,(unsigned long long)zero,(unsigned long long)foreign_zero,
    (unsigned long long)profile_refusals,(unsigned long long)extreme_positive,(unsigned long long)extreme_negative);
  std::printf("logical_q2_eager_pairs=%llu rejected_pairs=%llu deferred_pairs=%llu predicate_checks=%llu extra_accept_checks=%llu reference_complete=%d reference_q=%u\n",
    (unsigned long long)q2_attempts,(unsigned long long)q2_rejected,(unsigned long long)q2_selected,
    (unsigned long long)q2_probe_checks,(unsigned long long)q2_extra_accept_checks,(int)last_reference_ok,(unsigned)last_reference_q);
  std::printf("uncapped_original_order_pairs=%llu rejected=%llu deferred=%llu predicate_checks=%llu\n",
    (unsigned long long)uncapped_attempts,(unsigned long long)uncapped_rejected,
    (unsigned long long)uncapped_selected,(unsigned long long)uncapped_checks);
}
}
int main(int argc,char**) {
  if(argc!=1)return 2;
  try {
    predicate_fixtures();
    auto all=fixtures();
#if defined(MHGP7_Q2_REVIEW_SHELL_MUTANT)
    one(all[0],1000000,true);
    throw std::runtime_error("mutant.survived");
#else
    // First168 fixtures remain byte-identical to the prior gate's generator.
    check(all.size()==168,"fixture.original_cardinality");
    all.push_back(make({{0,0,0},{65535,65535,0},{65535,0,0}})); // foreign shell zero
    all.push_back(make({{0,0,0},{0,0,1},{65535,65535,65535}})); // first rejected q2 >i32
    for(const auto& f:all) {
      const u64 used=one(f,1000000,true);
      for(u64 cap=0;cap<=std::min<u64>(used+1,551);++cap)one(f,cap);
      Fixture reversed=make(f.pts);
      std::reverse(reversed.sites.begin(),reversed.sites.begin()+f.pts.size());
      one(reversed,1000000);
    }
    check(cases>11805&&success>0&&degenerate>0&&capped>0&&q2>0&&q3>0&&q4>0,"nonvacuity.outputs");
    check(q2_rejected>0&&q2_selected>0&&q2_attempts==q2_rejected+q2_selected&&q2_extra_accept_checks>0,
          "nonvacuity.logical_cost");
    check(positive>0&&negative>0&&zero>0&&foreign_zero>0&&profile_refusals==2&&extreme_positive&&extreme_negative,
          "nonvacuity.predicates");
    report("passed","none");return 0;
#endif
  }catch(const std::exception& error) {
#if defined(MHGP7_Q2_REVIEW_SHELL_MUTANT)
    const bool causal=std::strcmp(error.what(),"differential.status_reason")==0&&last_reference_ok&&
      last_reference_q==2&&zero>0;
    report(causal?"mutant_killed":"failed",error.what());return causal?4:1;
#else
    report("failed",error.what());return 1;
#endif
  }
}
