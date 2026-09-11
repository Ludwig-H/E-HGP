#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#define main inherited_main
#include "meb_hybrid.cpp"
#undef main
#include "guarded_run.hpp"
#pragma GCC diagnostic pop
int main(){
 const std::vector<P3> bad{{2,3,2},{2,0,0},{0,2,2},{1,0,0},{2,2,0},{3,0,1},{0,2,3}};
 const std::vector<P3> ordinary{{0,0,0},{6,0,0},{1,0,0},{2,0,0},{3,0,0},{4,0,0},{5,0,0}};
 Counters raw_work; AnchorMebWork nominal_work;
 const auto raw = hybrid(bad, raw_work); const auto nominal = anchor_meb(bad, nominal_work);
 Candidate proposal; Counters proposal_raw; const bool proposal_ok = welzl(bad, proposal, proposal_raw);
 if(nominal.status != AnchorMebStatus::kOk || raw.status != AnchorMebStatus::kInvariantViolated || std::string_view(raw.reason) != "canon_fail" || !proposal_ok || proposal.power(bad[6]) != 176) return 1;
 std::puts("PASS original_rejection reference_ok hybrid_canon_fail outside_power=176");
 for(const auto& sites : {bad,ordinary}) {
   AnchorMebWork reference_work,fallback_work;Counters proposal_work;bool fell_back=false;
   auto reference=anchor_meb(sites,reference_work);
   auto guarded=guarded_run(sites,proposal_work,fallback_work,fell_back);
   bool expected_fallback=sites==bad;
   bool passed=reference.status==AnchorMebStatus::kOk && same(reference,guarded) && fell_back==expected_fallback && proposal_work.cand>0 && proposal_work.pow>0 && fallback_work.calls==(expected_fallback?1u:0u);
   printf("%s fallback=%d all_fields_equal=%d proposal_candidates=%llu proposal_powers=%llu fallback_calls=%llu fallback_powers=%llu\n",passed?"PASS":"FAIL",fell_back,same(reference,guarded),proposal_work.cand,proposal_work.pow,(unsigned long long)fallback_work.calls,(unsigned long long)fallback_work.power_tests);
   if(!passed)return 1;
 }
 return 0;
}
