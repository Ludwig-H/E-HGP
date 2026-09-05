// Review-only derivation of revision2/tests/meb_lazy_gate.cpp
// SHA256 122807a3fe431bd9658262f8061bcb7e2258a7832516ceff918da52d08ac3a55.
// Only include, instrumentation observations/plancher, output label and
// argument prohibition differ. All fixtures, reference, budgets and field
// comparisons are retained. Zero logical-counter globals below are NOT observations.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include "../v7_meb_lazy_gate_proposal/revision2/proposal.hpp"
#if defined(MHGP7_TESTING)
#error This differential must compile the production branch without MHGP7_TESTING
#endif

using namespace mhgp7;
namespace {
// Explicit C reference port: ONLY miniball, plus its fail/charge adapters.
// Original source SHA256 fddde6e233eea8e80d23af4d42b50952e7c49a50ed84357e973279ff14d555e8,
// src/forest/silent_incidence.hpp:147-203. The loops, predicates, materialization
// order, acceptance and final shell check below are unchanged from that C pin.
bool reference_miniball(const CloudIndex& ix, const SilentIncidenceLimits& caps,
                       SilentIncidenceResult& out, const std::array<i32, 11>& sites,
                       size_t n, silent_detail::LocalBall* ball) {
    const auto fail = [&](SilentIncidenceStatus status, const char* reason) {
      out.status = status; out.reason = reason; return false;
    };
    const auto charge = [&](u64& counter, u64 limit, const char* reason) {
      if (counter >= limit) return fail(SilentIncidenceStatus::kResourceExhausted, reason);
      ++counter; return true;
    };
    ++out.stats.meb_calls;
    bool found = false;
    const auto accept = [&](const BallKey& key, const ExactLevel& level,
                            const std::array<i32, 4>& support, u8 q) {
      for (size_t i = 0; i < n; ++i)
        if (key.power(ix.upos[(size_t)sites[i]]) > 0) return false;
      ball->key = key;
      ball->level = level;
      ball->support = support;
      ball->q = q;
      return true;
    };
    for (size_t a = 0; a < n && !found; ++a)
      for (size_t b = a + 1; b < n && !found; ++b) {
        if (!charge(out.stats.meb_supports, caps.max_meb_supports, "silent_meb_support_budget")) return false;
        const P3& pa = ix.upos[(size_t)sites[a]];
        const P3& pb = ix.upos[(size_t)sites[b]];
        found = accept(q2_ball_key(pa, pb), promote_level(q2_exact_level(p3_norm2(p3_sub(pa, pb)))),
                       {sites[a], sites[b], 0, 0}, 2);
      }
    for (size_t a = 0; a < n && !found; ++a)
      for (size_t b = a + 1; b < n && !found; ++b)
        for (size_t c = b + 1; c < n && !found; ++c) {
          if (!charge(out.stats.meb_supports, caps.max_meb_supports, "silent_meb_support_budget")) return false;
          const P3& pa = ix.upos[(size_t)sites[a]];
          const P3& pb = ix.upos[(size_t)sites[b]];
          const P3& pc = ix.upos[(size_t)sites[c]];
          if (p3_dot(p3_sub(pb, pa), p3_sub(pc, pa)) <= 0 ||
              p3_dot(p3_sub(pa, pb), p3_sub(pc, pb)) <= 0 ||
              p3_dot(p3_sub(pa, pc), p3_sub(pb, pc)) <= 0) continue;
          const Q3Form f = q3_form(pa, pb, pc);
          if (f.g <= 0) continue;
          found = accept(q3_ball_key(f), promote_level(q3_exact_level(pa, pb, pc)),
                         {sites[a], sites[b], sites[c], 0}, 3);
        }
    for (size_t a = 0; a < n && !found; ++a)
      for (size_t b = a + 1; b < n && !found; ++b)
        for (size_t c = b + 1; c < n && !found; ++c)
          for (size_t d = c + 1; d < n && !found; ++d) {
            if (!charge(out.stats.meb_supports, caps.max_meb_supports, "silent_meb_support_budget")) return false;
            const P3& pa = ix.upos[(size_t)sites[a]];
            const P3& pb = ix.upos[(size_t)sites[b]];
            const P3& pc = ix.upos[(size_t)sites[c]];
            const P3& pd = ix.upos[(size_t)sites[d]];
            const Q4Form f = q4_form(pa, pb, pc, pd);
            if (f.det == 0 || !q4_center_strictly_inside(f, pa, pb, pc, pd)) continue;
            found = accept(ball_key_reduce(q4_ball_form(f)), q4_level_raw(f),
                           {sites[a], sites[b], sites[c], sites[d]}, 4);
          }
    if (!found) return fail(SilentIncidenceStatus::kInvariantViolated, "silent_no_local_miniball");
    size_t shell = 0;
    for (size_t i = 0; i < n; ++i)
      if (ball->key.power(ix.upos[(size_t)sites[i]]) == 0) ++shell;
    if (shell != ball->q)
      return fail(SilentIncidenceStatus::kUnsupportedDegeneracy, "silent_local_nonessential_shell");
    return true;
  }

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
bool stats_equal(const SilentIncidenceStats& a,const SilentIncidenceStats& b) {
  return a.core_records==b.core_records && a.core_facets==b.core_facets &&
    a.facets_with_two_intruders==b.facets_with_two_intruders && a.chain_steps==b.chain_steps &&
    a.added_cofaces==b.added_cofaces && a.terminal_direct==b.terminal_direct && a.terminal_cached==b.terminal_cached &&
    a.max_chain_length==b.max_chain_length && a.query_nodes==b.query_nodes && a.query_leaves==b.query_leaves &&
    a.query_range_skips==b.query_range_skips && a.meb_calls==b.meb_calls && a.meb_supports==b.meb_supports;
}
u64 cases=0, success=0, degenerate=0, capped=0, q2=0,q3=0,q4=0, saved_q3=0,saved_q4=0;
void skipped_constructions(const Fixture& f,const silent_detail::LocalBall& selected) {
  const size_t n=f.pts.size();
  const auto outside=[&](const BallForm& raw) {
    for(const auto& z:f.pts) if(raw.a*p3_norm2(z)+raw.b[0]*z.x+raw.b[1]*z.y+raw.b[2]*z.z+raw.c>0)return true;
    return false;
  };
  if(selected.q<3)return;
  for(size_t a=0;a<n;++a)for(size_t b=a+1;b<n;++b)for(size_t c=b+1;c<n;++c) {
    const auto& pa=f.pts[a];const auto& pb=f.pts[b];const auto& pc=f.pts[c];
    if(p3_dot(p3_sub(pb,pa),p3_sub(pc,pa))<=0||p3_dot(p3_sub(pa,pb),p3_sub(pc,pb))<=0||p3_dot(p3_sub(pa,pc),p3_sub(pb,pc))<=0)continue;
    const auto form=q3_form(pa,pb,pc);if(form.g<=0)continue;
    if(outside(q3_ball_form(form)))++saved_q3;else return;
  }
  if(selected.q<4)return;
  for(size_t a=0;a<n;++a)for(size_t b=a+1;b<n;++b)for(size_t c=b+1;c<n;++c)for(size_t d=c+1;d<n;++d) {
    const auto form=q4_form(f.pts[a],f.pts[b],f.pts[c],f.pts[d]);
    if(form.det==0||!q4_center_strictly_inside(form,f.pts[a],f.pts[b],f.pts[c],f.pts[d]))continue;
    if(outside(q4_ball_form(form)))++saved_q4;else return;
  }
}

std::string active_mutant;
u64 logical_reject3=0, logical_reject4=0, built3=0, built4=0, rejected_built3=0, rejected_built4=0;
u8 mismatch_reference_q=0;
bool mismatch_reference_complete=false;
u64 one(const Fixture& f,u64 cap,bool count_saved=false) {
  const std::vector<ForestEvent> direct;
  SilentIncidenceLimits caps;caps.max_meb_supports=cap;
  SilentIncidenceResult ra,rb;
  silent_detail::LocalBall ba,bb;
  const bool oka=reference_miniball(f.index,caps,ra,f.sites,f.pts.size(),&ba);
  silent_detail::Builder b(f.index,direct,caps,&rb);
  const bool okb=b.miniball(f.sites,f.pts.size(),&bb);
  mismatch_reference_q=ba.q;mismatch_reference_complete=oka;
  check(oka==okb && ra.status==rb.status && std::strcmp(ra.reason,rb.reason)==0,"differential.status_reason");
  check(stats_equal(ra.stats,rb.stats),"differential.stats");
  check(ba.key==bb.key && ba.level==bb.level && ba.q==bb.q && ba.support==bb.support,"differential.ball_bits_fields");
  // Neither local miniball function publishes events. This is only a local
  // container check, not a transactional-publication test of completion/run.
  check(ra.events.empty()&&rb.events.empty(),"differential.local_events_empty");
  ++cases;
  if(oka) {++success;if(count_saved){q2+=ba.q==2;q3+=ba.q==3;q4+=ba.q==4;skipped_constructions(f,ba);}}
  else if(ra.status==SilentIncidenceStatus::kUnsupportedDegeneracy)++degenerate;
  else if(ra.status==SilentIncidenceStatus::kResourceExhausted)++capped;
  return ra.stats.meb_supports;
}
void report(const char* status,const char* why) {
  std::printf("meb_production_differential=%s mutant=%s divergence=%s public_status=not_claimed\n",status,
              active_mutant.empty()?"none":active_mutant.c_str(),why);
  std::printf("meb_lazy_counts cases=%llu success=%llu degenerate=%llu capped=%llu q2=%llu q3=%llu q4=%llu logical_reject3=%llu logical_reject4=%llu materialized3=%llu materialized4=%llu rejected_materialized3=%llu rejected_materialized4=%llu reference_minimal_q=%u reference_complete=%d\n",
    (unsigned long long)cases,(unsigned long long)success,(unsigned long long)degenerate,(unsigned long long)capped,
    (unsigned long long)q2,(unsigned long long)q3,(unsigned long long)q4,(unsigned long long)logical_reject3,
    (unsigned long long)logical_reject4,(unsigned long long)built3,(unsigned long long)built4,
    (unsigned long long)rejected_built3,(unsigned long long)rejected_built4,(unsigned)mismatch_reference_q,(int)mismatch_reference_complete);
}
}
int main(int argc,char** argv) {
  if (argc != 1) return 2;  // This production-only replay accepts no injection.
  if(argc==2&&std::string(argv[1]).starts_with("--mutant="))active_mutant=std::string(argv[1]).substr(9);
  else if(argc!=1)return 2;
  if(argc==2&&active_mutant.empty())return 2;
  if(!active_mutant.empty()&&active_mutant!="silent-meb-q3-reject-shell"&&
     active_mutant!="silent-meb-q4-reject-shell"&&active_mutant!="silent-meb-eager-materialization")return 2;
  if(!active_mutant.empty()) {
    if(!mutants_enable(active_mutant))return 2;
  }
  try {
    const auto fs=fixtures();
    if(active_mutant=="silent-meb-q3-reject-shell") {
      one(fs[1],1000000,true);
      throw std::runtime_error("mutant_survived.q3");
    }
    if(active_mutant=="silent-meb-q4-reject-shell") {
      one(fs[2],1000000,true);
      throw std::runtime_error("mutant_survived.q4");
    }
    for(const auto& f:fs) {
      const u64 used=one(f,1000000,true);
      for(u64 cap=0;cap<=std::min<u64>(used+1,551);++cap)one(f,cap);
      Fixture reversed=make(f.pts);std::reverse(reversed.sites.begin(),reversed.sites.begin()+f.pts.size());
      one(reversed,1000000);
    }
    check(q2>0&&q3>0&&q4>0&&degenerate>0&&capped>0&&saved_q3>0&&saved_q4>0,"nonvacuity");
    if(rejected_built3||rejected_built4) {
      check(active_mutant=="silent-meb-eager-materialization"&&rejected_built3>0&&rejected_built4>0,
            "unrecognized.cost_divergence");
      report("mutant_killed","logical_rejected_materialization");return 4;
    }
    check(active_mutant.empty(),"mutant_survived.eager");
    report("passed","none");return 0;
  }catch(const std::exception& e) {
    const bool causal=std::strcmp(e.what(),"differential.status_reason")==0&&mismatch_reference_complete&&
      ((active_mutant=="silent-meb-q3-reject-shell"&&mismatch_reference_q==3&&logical_reject3>0)||
       (active_mutant=="silent-meb-q4-reject-shell"&&mismatch_reference_q==4&&logical_reject4>0));
    report(causal?"mutant_killed":"failed",e.what());return causal?4:1;
  }
}
