#define main level_comparator_cli_main
#include "level_comparator.cpp"
#undef main
using namespace level_comparator;
static unsigned checks=0,failures=0;
static void check(bool value) { ++checks; if(!value) ++failures; }
static Rational small(std::uint64_t n,std::uint64_t d) { return {{n,0,0,0,0},{d,0,0,0}}; }
int main() {
  auto out=compare(small(9,2),small(4,1));
  check(out.status==Status::Ready && out.cmp==1);
  check(out.lhs.words[0]==9 && out.rhs.words[0]==8 && out.lhs.high==0 && out.rhs.high==0);
  check(compare(small(0,2),small(0,1)).cmp==0);
  check(compare(small(1,2),small(2,4)).cmp==0);
  check(compare(small(1,3),small(1,2)).cmp==-1);
  const Rational maximum{{UINT64_MAX,UINT64_MAX,UINT64_MAX,UINT64_MAX,1023},
                         {UINT64_MAX,UINT64_MAX,UINT64_MAX,255}};
  out=compare(maximum,maximum);
  check(out.status==Status::Ready && out.cmp==0 && out.lhs.high==0 && out.rhs.high==0);
  auto value=maximum; value.numerator[4]=1024;
  out=compare(value,maximum); check(out.status==Status::NumDomain && out.side=='A');
  out=compare(maximum,value); check(out.status==Status::NumDomain && out.side=='B');
  value=maximum; value.denominator[3]=256;
  check(compare(value,maximum).status==Status::DenDomain);
  check(compare(maximum,value).status==Status::DenDomain);
  value=maximum; value.denominator={};
  check(compare(value,maximum).status==Status::ZeroDen);
  check(compare(maximum,value).status==Status::ZeroDen);
  Rational near=maximum; near.numerator[0]--;
  check(compare(near,maximum).cmp==-1);
  const Num all{UINT64_MAX,UINT64_MAX,UINT64_MAX,UINT64_MAX,UINT64_MAX};
  const Den all_d{UINT64_MAX,UINT64_MAX,UINT64_MAX,UINT64_MAX};
  const auto overflow=checked_product8(all,all_d);
  check(overflow.high==UINT64_MAX && overflow.words[0]==1);
  check(overflow.words[1]==0 && overflow.words[2]==0 && overflow.words[3]==0);
  check(overflow.words[4]==UINT64_MAX && overflow.words[5]==UINT64_MAX-1);
  check(overflow.words[6]==UINT64_MAX && overflow.words[7]==UINT64_MAX);
  const auto boundary=checked_product8(all,{0,0,0,1});
  check(boundary.high==0);
  for(unsigned j=0;j<8;++j) check(boundary.words[j]==(j<3?0:UINT64_MAX));
  const auto two=checked_product8({UINT64_MAX,0,0,0,0},{UINT64_MAX,0,0,0});
  check(two.high==0 && two.words[0]==1 && two.words[1]==UINT64_MAX-1);
  std::cout<<"{\"schema\":\"mhgp10_level_comparator_smoke_v1\",\"checks\":"<<checks<<",\"failures\":"<<failures<<"}\n";
  return failures?1:0;
}
