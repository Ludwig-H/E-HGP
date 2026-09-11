// Prototype auditeur : MEB d'abord (Welzl exact), puis canonicalisation sur la
// COQUILLE. Theoreme : tout support accepte definit une boule contenant tous les
// sites avec son support au bord ; cette boule EST la MEB (unique), donc le
// support canonique est toujours un sous-ensemble de la coquille.
#include "src/forest/anchor_meb.hpp"
#include "src/cloud/families.hpp"
#include "src/tree/cloud_index.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <chrono>
using namespace mhgp7;
using anchor_meb_detail::Candidate;
using anchor_meb_detail::form;

struct Counters { unsigned long long cand = 0, pow = 0; };

// Boule d'un sous-ensemble R (|R|<=4) : premier support admissible DANS R.
static bool trivial_meb(std::span<const P3> sites, const std::vector<u8>& R,
                        Candidate& out, Counters& c) {
  const u8 m = (u8)R.size();
  auto tryset = [&](std::array<u8,4> sl, u8 q) {
    ++c.cand; Candidate cand;
    if (!form(sites, sl, q, cand)) return false;
    for (u8 i = 0; i < m; ++i) { ++c.pow; if (cand.power(sites[R[i]]) > 0) return false; }
    out = cand; return true;
  };
  for (u8 a=0;a<m;++a) for (u8 b=a+1;b<m;++b) if (tryset({R[a],R[b],0,0},2)) return true;
  for (u8 a=0;a<m;++a) for (u8 b=a+1;b<m;++b) for (u8 d=b+1;d<m;++d)
    if (tryset({R[a],R[b],R[d],0},3)) return true;
  for (u8 a=0;a<m;++a) for (u8 b=a+1;b<m;++b) for (u8 d=b+1;d<m;++d) for (u8 e=d+1;e<m;++e)
    if (tryset({R[a],R[b],R[d],R[e]},4)) return true;
  return false;
}

// Welzl RECURSIF correct : la base R est bornee a 4 points (une MEB en 3D est
// determinee par au plus quatre points), donc trivial_meb coute <= 11 candidats.
static bool welzl_rec(std::span<const P3> sites, std::vector<u8>& P, std::vector<u8>& R,
                      Candidate& out, Counters& c) {
  if (P.empty() || R.size() == 4) return trivial_meb(sites, R, out, c);
  const u8 p = P.back(); P.pop_back();
  bool ok = welzl_rec(sites, P, R, out, c);
  if (ok) { ++c.pow; if (out.power(sites[p]) <= 0) { P.push_back(p); return true; } }
  R.push_back(p);
  ok = welzl_rec(sites, P, R, out, c);
  R.pop_back();
  P.push_back(p);
  return ok;
}

static bool welzl(std::span<const P3> sites, Candidate& out, Counters& c) {
  const u8 n = (u8)sites.size();
  std::vector<u8> P, R;
  for (u8 i = 0; i < n; ++i) P.push_back(i);
  std::reverse(P.begin(), P.end());  // ordre deterministe
  return welzl_rec(sites, P, R, out, c);
}

static AnchorMebResult run(std::span<const P3> sites, Counters& c) {
  AnchorMebResult r;
  const u8 n = (u8)sites.size();
  if (n == 1) { AnchorMebWork w{}; return anchor_meb(sites, w); }
  Candidate meb;
  if (!welzl(sites, meb, c)) { r.status = AnchorMebStatus::kInvariantViolated; r.reason="welzl_fail"; return r; }
  // Coquille = sites de puissance nulle pour la MEB.
  std::vector<u8> shell;
  for (u8 i = 0; i < n; ++i) { ++c.pow; if (meb.power(sites[i]) == 0) shell.push_back(i); }
  // Canonicalisation : meme ordre lexicographique, plus petit q d'abord, sur la COQUILLE.
  const u8 m = (u8)shell.size();
  auto emit = [&](std::array<u8,4> sl, u8 q) {
    ++c.cand; Candidate cand;
    if (!form(sites, sl, q, cand)) return false;
    u8 sh = 0;
    for (u8 i = 0; i < n; ++i) { ++c.pow; const i128 p = cand.power(sites[i]);
      if (p > 0) return false; if (p == 0) ++sh; }
    r.support_size = q; r.support_slots = sl; r.selected_shell_count = sh;
    if (q == 2) { r.key = q2_ball_key(cand.a, cand.b);
      r.level = promote_level(q2_exact_level(p3_norm2(p3_sub(cand.a, cand.b)))); }
    else if (q == 3) { r.key = q3_ball_key(cand.three);
      r.level = promote_level(q3_exact_level(cand.a, cand.b, sites[sl[2]])); }
    else { r.key = ball_key_reduce(q4_ball_form(cand.four)); r.level = q4_level_raw(cand.four); }
    r.status = AnchorMebStatus::kOk; r.reason = "anchor_meb_exact_local"; return true;
  };
  for (u8 a=0;a<m;++a) for (u8 b=a+1;b<m;++b) if (emit({shell[a],shell[b],0,0},2)) return r;
  for (u8 a=0;a<m;++a) for (u8 b=a+1;b<m;++b) for (u8 d=b+1;d<m;++d)
    if (emit({shell[a],shell[b],shell[d],0},3)) return r;
  for (u8 a=0;a<m;++a) for (u8 b=a+1;b<m;++b) for (u8 d=b+1;d<m;++d) for (u8 e=d+1;e<m;++e)
    if (emit({shell[a],shell[b],shell[d],shell[e]},4)) return r;
  r.status = AnchorMebStatus::kInvariantViolated; r.reason = "canon_fail"; return r;
}

static bool same(const AnchorMebResult& a, const AnchorMebResult& b) {
  return a.status==b.status && a.key==b.key && same_exact_level(a.level,b.level) &&
         a.support_size==b.support_size && a.support_slots==b.support_slots &&
         a.selected_shell_count==b.selected_shell_count;
}


// Force brute avec les deux optimisations exactes : q=2 restreint aux paires de
// distance maximale, et confinement testant d'abord les deux points extremes.
static AnchorMebResult brute_q2(std::span<const P3> sites, Counters& c) {
  AnchorMebResult r;
  const u8 n = (u8)sites.size();
  if (n == 1) { AnchorMebWork w{}; return anchor_meb(sites, w); }
  i64 best = -1; u8 e0 = 0, e1 = 1;
  for (u8 i = 0; i < n; ++i) for (u8 j = i + 1; j < n; ++j) {
    const i64 d = p3_norm2(p3_sub(sites[i], sites[j]));
    if (d > best) { best = d; e0 = i; e1 = j; } }
  bool done = false;
  auto attempt = [&](std::array<u8,4> sl, u8 q) {
    ++c.cand; Candidate cand;
    if (!form(sites, sl, q, cand)) return false;
    u8 sh = 0;
    for (u8 idx : {e0, e1}) { ++c.pow; const i128 p = cand.power(sites[idx]);
      if (p > 0) return false; if (p == 0) ++sh; }
    for (u8 i = 0; i < n; ++i) { if (i == e0 || i == e1) continue;
      ++c.pow; const i128 p = cand.power(sites[i]);
      if (p > 0) return false; if (p == 0) ++sh; }
    r.support_size = q; r.support_slots = sl; r.selected_shell_count = sh;
    if (q == 2) { r.key = q2_ball_key(cand.a, cand.b);
      r.level = promote_level(q2_exact_level(p3_norm2(p3_sub(cand.a, cand.b)))); }
    else if (q == 3) { r.key = q3_ball_key(cand.three);
      r.level = promote_level(q3_exact_level(cand.a, cand.b, sites[sl[2]])); }
    else { r.key = ball_key_reduce(q4_ball_form(cand.four)); r.level = q4_level_raw(cand.four); }
    r.status = AnchorMebStatus::kOk; r.reason = "anchor_meb_exact_local"; return true;
  };
  for (u8 a=0;a<n&&!done;++a) for (u8 b=a+1;b<n&&!done;++b)
    if (p3_norm2(p3_sub(sites[a],sites[b])) == best) done = attempt({a,b,0,0},2);
  for (u8 a=0;a<n&&!done;++a) for (u8 b=a+1;b<n&&!done;++b) for (u8 d=b+1;d<n&&!done;++d)
    done = attempt({a,b,d,0},3);
  for (u8 a=0;a<n&&!done;++a) for (u8 b=a+1;b<n&&!done;++b) for (u8 d=b+1;d<n&&!done;++d)
    for (u8 e=d+1;e<n&&!done;++e) done = attempt({a,b,d,e},4);
  if (!done) { r.status = AnchorMebStatus::kInvariantViolated; r.reason = "brute_fail"; }
  return r;
}

static AnchorMebResult hybrid(std::span<const P3> s, Counters& c) {
  return s.size() >= 7 ? run(s, c) : brute_q2(s, c);
}

int main(int argc, char** argv) {
  const int n = argc>1?std::atoi(argv[1]):20000;
  const int samples = argc>2?std::atoi(argv[2]):40000;
  const auto in = make_family_input(CloudFamily::kUniform, n, 65536, 3);
  const auto ix = build_cloud_index(in);
  const auto& pts = ix.upos; const size_t m = pts.size();
  unsigned long long cases=0, mism=0, candK[11]={0}, powK[11]={0}, rcandK[11]={0}, rpowK[11]={0}, nK[11]={0};
  double tref=0, thyb=0;
  std::vector<std::pair<i64,u32>> near;
  unsigned long long rng=88172645463325252ULL;
  auto nx=[&]{ rng^=rng<<13; rng^=rng>>7; rng^=rng<<17; return rng; };
  for (int s=0;s<samples;++s) {
    const u32 c=(u32)(nx()%m); near.clear();
    for (int t=0;t<160;++t){ const u32 j=(u32)(nx()%m); if(j!=c) near.push_back({p3_norm2(p3_sub(pts[j],pts[c])),j}); }
    std::sort(near.begin(),near.end());
    const int K=2+(int)(nx()%9);
    std::vector<P3> sites{pts[c]};
    for (int t=0;t<K-1&&t<(int)near.size();++t) sites.push_back(pts[near[t].second]);
    bool dup=false;
    for(size_t i=0;i<sites.size()&&!dup;++i) for(size_t j=0;j<i;++j) if(sites[i]==sites[j]){dup=true;break;}
    if(dup||sites.size()<2) continue;
    AnchorMebWork w{}; Counters c2{};
    auto ta=std::chrono::steady_clock::now();
    const auto r0=anchor_meb(std::span<const P3>(sites),w);
    tref+=std::chrono::duration<double>(std::chrono::steady_clock::now()-ta).count();
    auto tb=std::chrono::steady_clock::now();
    const auto r1=hybrid(std::span<const P3>(sites),c2);
    thyb+=std::chrono::duration<double>(std::chrono::steady_clock::now()-tb).count();
    const size_t kk=sites.size(); ++nK[kk]; ++cases;
    unsigned long long rc=0; for(int q=2;q<=4;++q) rc+=w.supports_by_size[q];
    rcandK[kk]+=rc; rpowK[kk]+=w.power_tests; candK[kk]+=c2.cand; powK[kk]+=c2.pow;
    if(!same(r0,r1)) ++mism;
  }
  std::printf("cases=%llu mismatches=%llu\n",cases,mism);
  std::printf("TEMPS  reference=%.3fs  hybride=%.3fs  speedup=%.2fx\n",tref,thyb,tref/(thyb>0?thyb:1e-9));
  std::printf("\n  K   appels  cand(ref)  cand(hybr)    pow(ref)  pow(hybr)    gain_cand  gain_pow\n");
  unsigned long long tc=0,tp=0,trc=0,trp=0;
  for(int k=2;k<=10;++k) if(nK[k]){
    tc+=candK[k];tp+=powK[k];trc+=rcandK[k];trp+=rpowK[k];
    std::printf("%3d %8llu %10.1f %12.1f %10.1f %11.1f %10.2fx %8.2fx\n",k,nK[k],
      (double)rcandK[k]/nK[k],(double)candK[k]/nK[k],(double)rpowK[k]/nK[k],(double)powK[k]/nK[k],
      (double)rcandK[k]/(candK[k]?candK[k]:1),(double)rpowK[k]/(powK[k]?powK[k]:1));
  }
  std::printf("TOTAL          %10.1f %12.1f %10.1f %11.1f %10.2fx %8.2fx\n",
    (double)trc/cases,(double)tc/cases,(double)trp/cases,(double)tp/cases,
    (double)trc/(tc?tc:1),(double)trp/(tp?tp:1));
  return mism?1:0;
}
