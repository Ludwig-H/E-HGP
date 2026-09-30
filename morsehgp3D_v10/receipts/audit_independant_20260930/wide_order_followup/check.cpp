#include <algorithm>
#include <iomanip>
#include <stdexcept>
#include <vector>
#define main frozen_level_protocol_main
#include "source/receipts/audit_continu_20260929/level_order_20260930/prototype/level_comparator.cpp"
#undef main
#include "source/src/cloud/grid32_primitives.hpp"

namespace {
using namespace level_comparator;
using mhgp10::grid32::PointGrid32;
using mhgp10::grid32::squared_distance;
std::string decimal(U128 value) {
  std::string out;
  do { out.push_back(char('0'+value%10)); value/=10; } while(value);
  std::reverse(out.begin(),out.end()); return out;
}
Rational ratio(U128 numerator,U128 denominator) {
  Rational out;
  out.numerator[0]=std::uint64_t(numerator);
  out.numerator[1]=std::uint64_t(numerator>>64);
  out.denominator[0]=std::uint64_t(denominator);
  out.denominator[1]=std::uint64_t(denominator>>64);
  return out;
}
void require(bool ok,const char* reason) { if(!ok) throw std::runtime_error(reason); }
int ready_cmp(const Rational& a,const Rational& b) {
  auto result=compare(a,b);
  require(result.status==Status::Ready,"nonready arithmetic entered valid comparator");
  return result.cmp;
}
struct Row { Rational level; unsigned id; };
bool checked_less(const Row& a,const Row& b) {
  int c=ready_cmp(a.level,b.level);
  return c?c<0:a.id<b.id;
}
// Intentionally wrong future adapter. Test its three pairwise answers only;
// never pass this cyclic relation to std::sort.
bool naive_less(const Row& a,const Row& b) {
  int c=compare(a.level,b.level).cmp;
  return c?c<0:a.id<b.id;
}
void pair_case(std::uint32_t m,const char* tag) {
  PointGrid32 a{},b{m,0,0},c{m,1,0};
  U128 n=squared_distance(a,b),np=squared_distance(a,c);
  Rational first=ratio(n,4),second=ratio(np,4),threshold=ratio(n/4,1);
  double x=static_cast<double>(n)/4, y=static_cast<double>(np)/4;
  require(np==n+1,"geometric distance difference must be one");
  require(ready_cmp(first,second)<0,"distinct q2 exact levels were lost");
  require(ready_cmp(first,threshold)==0 && ready_cmp(second,threshold)>0,
          "closed integer threshold admitted wrong q2 level");
  std::vector<Row> rows{{second,0},{first,2},{ratio(n*4,16),3},{threshold,4},{ratio(0,1),7}};
  for(const auto& row:rows) require(domain(row.level)==Status::Ready,"preflight refused valid row");
  std::sort(rows.begin(),rows.end(),checked_less);
  unsigned expected[]={7,2,3,4,0};
  for(unsigned i=0;i<rows.size();++i) require(rows[i].id==expected[i],"valid exact order or equality tie failed");
  auto end=std::upper_bound(rows.begin(),rows.end(),threshold,
    [](const Rational& e,const Row& row){return ready_cmp(e,row.level)<0;});
  require(end-rows.begin()==4,"exact upper bound lost closed equality");
  Row invalid{ratio(1,0),1},low{first,2},high{second,0};
  require(compare(high.level,invalid.level).status==Status::ZeroDen,
          "prototype did not refuse zero denominator");
  const bool ab=naive_less(low,high),bc=naive_less(high,invalid),ca=naive_less(invalid,low);
  require(ab && bc && ca,"refusal-as-equality cycle was not observed");
  std::cout<<"{\"case\":\""<<tag<<"\",\"m\":"<<m
           <<",\"first_numerator\":\""<<decimal(n)<<"\",\"second_numerator\":\""<<decimal(np)
           <<"\",\"denominator\":4,\"threshold\":\""<<decimal(n/4)
           <<"\",\"cmp\":"<<ready_cmp(first,second)
           <<",\"first_at_most\":true,\"second_at_most\":false"
           <<",\"double_first\":"<<std::setprecision(17)<<x<<",\"double_second\":"<<y
           <<",\"double_collision\":"<<(x==y?"true":"false")
           <<",\"exact_sorted_ids\":[7,2,3,4,0],\"closed_upper_bound\":4"
           <<",\"invalid_status\":\"REFUSED_ZERO_DEN\",\"naive_relation_cycle\":[true,true,true]}\n";
}
void threshold66_case() {
  const U128 e=squared_distance(PointGrid32{},PointGrid32{0xffffffffu,0xffffffffu,0xffffffffu});
  require(e>U128{0xffffffffffffffffULL},"threshold is not beyond u64");
  Rational threshold=ratio(e,1);
  // D=2^200-1. Multiply the 66-bit geometric threshold by D using shifts
  // independently of the prototype's checked_product8 (no full arithmetic oracle here).
  // e*D = (e<<200)-e, represented in five words by explicit carry/borrow.
  Num numerator{};
  numerator[3]=std::uint64_t(e)<<8;
  numerator[4]=std::uint64_t(e>>56);
  const std::uint64_t e0=std::uint64_t(e),e1=std::uint64_t(e>>64);
  numerator[0]=std::uint64_t(0)-e0;
  std::uint64_t borrow=e0!=0;
  numerator[1]=std::uint64_t(0)-e1-borrow;
  borrow=(e1!=0 || borrow!=0);
  numerator[2]=std::uint64_t(0)-borrow;
  numerator[3]-=borrow;
  Den denominator{{~std::uint64_t{0},~std::uint64_t{0},~std::uint64_t{0},255}};
  Rational middle{numerator,denominator},lower=middle,upper=middle;
  --lower.numerator[0]; ++upper.numerator[0];
  require(domain(lower)==Status::Ready && domain(middle)==Status::Ready && domain(upper)==Status::Ready,
          "threshold bridge lies outside comparator domain");
  int lo=ready_cmp(lower,threshold),eq=ready_cmp(middle,threshold),hi=ready_cmp(upper,threshold);
  require(lo==-1 && eq==0 && hi==1,"66-bit threshold bridge lost ordering");
  std::cout<<"{\"case\":\"knn66_bridge\",\"threshold\":\""<<decimal(e)
           <<"\",\"threshold_bits\":66,\"denominator\":\"1606938044258990275541962092341162602522202993782792835301375\""
           <<",\"middle_numerator_words_low_to_high\":[";
  for(unsigned i=0;i<5;++i) { if(i) std::cout<<','; std::cout<<numerator[i]; }
  std::cout<<"],\"cmp_below_equal_above\":["<<lo<<','<<eq<<','<<hi<<"]"
           <<",\"scope\":\"geometric integer threshold; three surrounding rationals need not be MEB levels\"}\n";
}
}
int main() {
  try {
    pair_case(1u<<23,"u24_q2");
    pair_case(1u<<31,"u32_q2");
    threshold66_case();
    std::cout<<"{\"status\":\"PASS\",\"geometric_pairs\":2,\"threshold_bridges\":1,\"invalid_sort_executed\":false,\"FULL_tested\":false,\"GCP_used\":false}\n";
  } catch(const std::exception& e) {
    std::cerr<<e.what()<<'\n'; return 1;
  }
}
