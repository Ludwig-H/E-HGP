#define main relative_filter_cli_main
#include "relative_filter.cpp"
#undef main
using namespace relative_filter;
static unsigned checks=0,failures=0;
static void check(bool ok) { ++checks; if(!ok) ++failures; }
static Signed coefficient(std::int64_t n) { return {n<0?-1:n>0?1:0,Mag{static_cast<std::uint64_t>(n<0?-n:n),0,0}}; }
int main() {
  const unsigned initial=_mm_getcsr(); const int initial_round=std::fegetround();
  if(std::fesetround(FE_TONEAREST)!=0) return 2;
  _mm_setcsr((_mm_getcsr() & ~0xe040u)|0x1f80u);
  Sphere ball; std::array<Signed,3> n{coefficient(2),coefficient(0),coefficient(0)};
  check(ball.prepare({0,0,0},n,{1,0,0})==Status::Ready);
  check(ball.site({0,0,0}).status==Status::Ambiguous);
  check(ball.site({2,0,0}).status==Status::Interior);
  check(ball.site({5,0,0}).status==Status::Exterior);
  check(ball.box({5,0,0},{6,0,0}).status==Status::Exterior);
  check(ball.box({0,0,0},{3,1,1}).status==Status::Ambiguous);
  check(ball.box({3,0,0},{2,0,0}).status==Status::Box);
  Interval v; const Signed head{1,Mag{9007199254740993ULL,0,0}};
  check(integer_interval(head,v)==Status::Ready);
  check(v.lo==9007199254740992. && v.hi==9007199254740994.);
  check(integer_interval({-1,head.magnitude},v)==Status::Ready);
  check(v.lo==-9007199254740994. && v.hi==-9007199254740992.);
  check(integer_interval({0,{1,0,0}},v)==Status::Coefficient);
  const unsigned controls=_mm_getcsr() & ~0x3fu;
  check(std::fesetround(FE_UPWARD)==0);
  check(ball.site({2,0,0}).status==Status::Environment);
  check(ball.box({2,0,0},{3,0,0}).status==Status::Environment);
  check(integer_interval(head,v)==Status::Environment);
  check(std::fesetround(FE_TONEAREST)==0);
  const unsigned rn=_mm_getcsr();
  for(unsigned mode: {0x2000u,0x4000u,0x6000u,0x8000u,0x40u}) {
    _mm_setcsr((rn & ~0xe040u)|mode);
    const auto answer=ball.site({2,0,0}).status;
    const auto box=ball.box({2,0,0},{3,0,0}).status;
    const auto converted=integer_interval(head,v);
    _mm_setcsr(rn);
    check(answer==Status::Environment); check(box==Status::Environment);
    check(converted==Status::Environment);
  }
  check((_mm_getcsr() & ~0x3fu)==controls && std::fegetround()==FE_TONEAREST);
  check(ball.prepare({0,0,0},n,{0,0,0})==Status::Denominator);
  check(ball.site({2,0,0}).status==Status::Coefficient);
  const std::array<Point,3> a{{{0,0,0},{15000010,0,15000010},{4000000000u,4000000000u,4000000000u}}};
  const std::array<std::array<Point,3>,3> x{{{{{0,0,0},{15000005,15000005,0},{15000005,0,15000005}}},{{{15000010,0,15000010},{15000010,15000010,0},{0,0,0}}},{{{4000000000u,4000000000u,4000000000u},{4000200005u,4000000000u,4000000000u},{4000040001u,4000200005u,4000000000u}}}}};
  const std::array<std::array<std::int64_t,3>,3> nums{{{30000010,15000005,15000005},{-15000010,15000010,-30000020},{1000025,840021,0}}};
  for(unsigned i=0;i<3;++i) {
    for(unsigned j=0;j<3;++j) n[j]=coefficient(nums[i][j]);
    check(ball.prepare(a[i],n,{i==2?10u:3u,0,0})==Status::Ready);
    for(const auto& p:x[i]) check(ball.site(p).status==Status::Ambiguous);
  }
  for(auto& component:n) component={1,Mag{UINT64_MAX,UINT64_MAX,UINT64_MAX}};
  check(ball.prepare({UINT32_MAX,UINT32_MAX,UINT32_MAX},n,{1,0,0})==Status::Ready);
  check(std::isfinite(ball.radius().hi));
  check(ball.site({0,0,0}).status==Status::Ambiguous);
  std::fesetround(initial_round); _mm_setcsr(initial);
  std::cout<<"{\"schema\":\"mhgp10_relative_filter_smoke_v1\",\"checks\":"<<checks<<",\"failures\":"<<failures<<"}\n";
  return failures?1:0;
}
