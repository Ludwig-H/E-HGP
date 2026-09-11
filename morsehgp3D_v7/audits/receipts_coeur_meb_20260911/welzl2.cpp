// Welzl REPARE : cas de base = boule passant PAR R (pas la MEB de R).
// Filtres de minimalite de form() retires : acuite pour q3, centre interieur
// pour q4 ; seules les gardes de non-degenerescence g>0 et det>0 restent.
#define main inherited_main
#include "meb_hybrid.cpp"
#undef main
#include <chrono>

static bool boundary_ball(std::span<const P3> s, const std::vector<u8>& R, Candidate& out, Counters& c) {
  const u8 m = (u8)R.size();
  ++c.cand;
  if (m == 0) return false;
  if (m == 1) { out.q=2; out.slots={R[0],R[0],0,0}; out.a=s[R[0]]; out.b=s[R[0]]; return true; }
  if (m == 2) { out.q=2; out.slots={R[0],R[1],0,0}; out.a=s[R[0]]; out.b=s[R[1]]; return true; }
  if (m == 3) { out.q=3; out.slots={R[0],R[1],R[2],0}; out.a=s[R[0]]; out.b=s[R[1]];
                out.three=q3_form(s[R[0]],s[R[1]],s[R[2]]); return out.three.g>0; }
  out.q=4; out.slots={R[0],R[1],R[2],R[3]}; out.a=s[R[0]]; out.b=s[R[1]];
  out.four=q4_form(s[R[0]],s[R[1]],s[R[2]],s[R[3]]); return out.four.det>0;
}
static bool welzl2_rec(std::span<const P3> s, std::vector<u8>& P, std::vector<u8>& R, Candidate& out, Counters& c) {
  if (P.empty() || R.size()==4) return boundary_ball(s,R,out,c);
  const u8 p = P.back(); P.pop_back();
  bool ok = welzl2_rec(s,P,R,out,c);
  if (ok) { ++c.pow; if (out.power(s[p]) <= 0) { P.push_back(p); return true; } }
  R.push_back(p);
  ok = welzl2_rec(s,P,R,out,c);
  R.pop_back(); P.push_back(p);
  return ok;
}
static bool welzl2(std::span<const P3> s, Candidate& out, Counters& c) {
  std::vector<u8> P, R;
  for (u8 i=(u8)s.size(); i-- > 0;) P.push_back(i);
  return welzl2_rec(s,P,R,out,c);
}
static AnchorMebResult run2(std::span<const P3> s, Counters& c, bool& guarded) {
  AnchorMebResult r; const u8 n=(u8)s.size();
  if (n==1) { AnchorMebWork w{}; return anchor_meb(s,w); }
  Candidate meb;
  if (!welzl2(s,meb,c)) { guarded=true; return brute_q2(s,c); }
  std::vector<u8> shell;
  for (u8 i=0;i<n;++i){ ++c.pow; const i128 p=meb.power(s[i]);
    if (p>0){ guarded=true; return brute_q2(s,c);} if(p==0) shell.push_back(i); }
  const u8 m=(u8)shell.size();
  auto emit=[&](std::array<u8,4> sl,u8 q){
    ++c.cand; Candidate cd; if(!form(s,sl,q,cd)) return false;
    u8 sh=0; for(u8 i=0;i<n;++i){ ++c.pow; const i128 p=cd.power(s[i]);
      if(p>0) return false; if(p==0)++sh; }
    r.support_size=q; r.support_slots=sl; r.selected_shell_count=sh;
    if(q==2){ r.key=q2_ball_key(cd.a,cd.b); r.level=promote_level(q2_exact_level(p3_norm2(p3_sub(cd.a,cd.b)))); }
    else if(q==3){ r.key=q3_ball_key(cd.three); r.level=promote_level(q3_exact_level(cd.a,cd.b,s[sl[2]])); }
    else { r.key=ball_key_reduce(q4_ball_form(cd.four)); r.level=q4_level_raw(cd.four); }
    r.status=AnchorMebStatus::kOk; r.reason="anchor_meb_exact_local"; return true; };
  for(u8 a=0;a<m;++a)for(u8 b=a+1;b<m;++b) if(emit({shell[a],shell[b],0,0},2)) return r;
  for(u8 a=0;a<m;++a)for(u8 b=a+1;b<m;++b)for(u8 d=b+1;d<m;++d) if(emit({shell[a],shell[b],shell[d],0},3)) return r;
  for(u8 a=0;a<m;++a)for(u8 b=a+1;b<m;++b)for(u8 d=b+1;d<m;++d)for(u8 e=d+1;e<m;++e)
    if(emit({shell[a],shell[b],shell[d],shell[e]},4)) return r;
  guarded=true; return brute_q2(s,c);
}
int main(){
  bool k7_ok=false; unsigned bad_ex=0;
  // Epreuve 1 : la contre-fixture K7 exacte
  {
    std::vector<P3> f{{2,3,2},{2,0,0},{0,2,2},{1,0,0},{2,2,0},{3,0,1},{0,2,3}};
    AnchorMebWork w; Counters c; bool g=false;
    auto a=anchor_meb(std::span<const P3>(f),w); auto b=run2(std::span<const P3>(f),c,g);
    k7_ok = same(a,b);
    printf("epreuve K7 : %s  repli=%s\n", k7_ok?"IDENTIQUE":"DIVERGENT", g?"oui":"non");
  }
  // Epreuve 2 : leur balayage exhaustif
  {
    const std::vector<P3> o{{2,3,2},{2,0,0},{0,2,2},{1,0,0},{2,2,0},{3,0,1},{0,2,3},{2,2,1},{2,2,3},{1,2,1}};
    unsigned cases=0,bad=0,gu=0;
    for(unsigned n=2;n<=10;++n)for(unsigned mask=0;mask<1024;++mask){
      if((unsigned)__builtin_popcount(mask)!=n)continue;
      std::vector<P3> s; for(unsigned i=0;i<10;++i) if(mask&(1u<<i)) s.push_back(o[i]);
      AnchorMebWork w; Counters c; bool g=false;
      auto a=anchor_meb(std::span<const P3>(s),w); auto b=run2(std::span<const P3>(s),c,g);
      ++cases; if(!same(a,b))++bad; if(g)++gu; }
    bad_ex = bad;
    printf("epreuve exhaustive : %u cas, %u divergences, %u replis\n",cases,bad,gu);
  }
  // Epreuve 3 : 200k tirages + chronometrage
  const auto in=make_family_input(CloudFamily::kUniform,20000,65536,3);
  const auto ix=build_cloud_index(in); const auto& pts=ix.upos; const size_t m=pts.size();
  unsigned long long rng=88172645463325252ULL; auto nx=[&]{rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return rng;};
  std::vector<std::pair<i64,u32>> near; unsigned long long cases=0,bad=0,gu=0,rc=0,vc=0,rp=0,vp=0;
  double tref=0,tvar=0;
  for(int it=0; it<200000; ++it){
    const u32 c0=(u32)(nx()%m); near.clear();
    for(int t=0;t<160;++t){const u32 j=(u32)(nx()%m); if(j!=c0) near.push_back({p3_norm2(p3_sub(pts[j],pts[c0])),j});}
    std::sort(near.begin(),near.end());
    const int K=2+(int)(nx()%9); std::vector<P3> s{pts[c0]};
    for(int t=0;t<K-1&&t<(int)near.size();++t) s.push_back(pts[near[t].second]);
    bool dup=false; for(size_t i=0;i<s.size()&&!dup;++i)for(size_t j=0;j<i;++j) if(s[i]==s[j]){dup=true;break;}
    if(dup||s.size()<2) continue;
    AnchorMebWork w{}; Counters c{}; bool g=false;
    auto t0=std::chrono::steady_clock::now(); auto a=anchor_meb(std::span<const P3>(s),w);
    tref+=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
    auto t1=std::chrono::steady_clock::now(); auto b=run2(std::span<const P3>(s),c,g);
    tvar+=std::chrono::duration<double>(std::chrono::steady_clock::now()-t1).count();
    ++cases; if(!same(a,b))++bad; if(g)++gu;
    for(int q=2;q<=4;++q) rc+=w.supports_by_size[q];
    rp+=w.power_tests; vc+=c.cand; vp+=c.pow; }
  printf("epreuve aleatoire : %llu cas, %llu divergences, %llu replis (%.4f%%)\n",cases,bad,gu,100.0*gu/cases);
  printf("candidats  ref=%.1f  welzl2=%.1f  gain=%.2fx\n",(double)rc/cases,(double)vc/cases,(double)rc/vc);
  printf("puissances ref=%.1f  welzl2=%.1f  gain=%.2fx\n",(double)rp/cases,(double)vp/cases,(double)rp/vp);
  printf("TEMPS ref=%.3fs welzl2=%.3fs speedup=%.2fx\n",tref,tvar,tref/tvar);
  const bool all_ok = k7_ok && bad_ex==0 && bad==0;
  printf("VERDICT %s (K7=%s exhaustif=%u divergences aleatoires=%llu)\n",
         all_ok?"PASS":"FAIL", k7_ok?"ok":"DIVERGENT", bad_ex, bad);
  return all_ok?0:1; }
