#include <array>
#include <bit>
#include <cfenv>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#if defined(__SSE2__)
#include <xmmintrin.h>
#endif
#if defined(__FAST_MATH__)
#error "Certified filter forbids fast-math"
#endif
namespace relative_filter {
using Mag = std::array<std::uint64_t, 3>; // little-endian limbs
using Point = std::array<std::uint32_t, 3>;
struct Signed { int sign; Mag magnitude; };
struct Interval { double lo=0, hi=0; };
enum class Status { Ready, Interior, Exterior, Ambiguous, Environment, Denominator, Coefficient, Box };
inline const char* name(Status s) {
  switch(s) {
    case Status::Ready:return "READY";
    case Status::Interior:return "INTERIOR";
    case Status::Exterior:return "EXTERIOR";
    case Status::Ambiguous:return "AMBIGU";
    case Status::Environment:return "REFUSED_ENV";
    case Status::Denominator:return "REFUSED_DEN";
    case Status::Coefficient:return "REFUSED_SIGN";
    case Status::Box:return "REFUSED_BOX";
  }
  return "INVALID_STATUS";
}
inline bool environment() {
  if(std::fegetround()!=FE_TONEAREST) return false;
#if defined(__SSE2__)
  const unsigned csr=_mm_getcsr();
  return (csr & 0xe040u)==0 && (csr & 0x1f80u)==0x1f80u; // RN, FTZ/DAZ OFF, SSE traps masked
#else
  return false; // no portable FTZ/DAZ certification in this prototype
#endif
}
namespace detail {
inline double down(double x) { return std::nextafter(x,-std::numeric_limits<double>::infinity()); }
inline double up(double x) { return std::nextafter(x,std::numeric_limits<double>::infinity()); }
inline bool zero(const Mag& x) { return (x[0]|x[1]|x[2])==0; }
inline bool valid(const Signed& x) {
  return (zero(x.magnitude) && x.sign==0) ||
         (!zero(x.magnitude) && (x.sign==1 || x.sign==-1));
}
inline Interval magnitude(const Mag& x) {
  if(zero(x)) return {};
  unsigned bits=0;
  for(unsigned j=3;j>0;--j) if(x[j-1]!=0) {
    bits=(j-1)*64+64-static_cast<unsigned>(std::countl_zero(x[j-1])); break;
  }
  const unsigned shift=bits>53?bits-53:0;
  std::uint64_t head=0;
  for(unsigned i=0;i<bits-shift;++i)
    head|=((x[(shift+i)/64]>>((shift+i)%64))&1u)<<i;
  bool rest=false;
  for(unsigned i=0;i<shift;++i) rest|=((x[i/64]>>(i%64))&1u)!=0;
#ifdef MUTANT_DROP_REST
  rest=false;
#endif
  const double lo=std::ldexp(static_cast<double>(head),static_cast<int>(shift));
  return {lo,rest?up(lo):lo}; // head has at most 53 bits; ldexp is exact
}
inline Interval signed_value(const Signed& x) {
  const auto z=magnitude(x.magnitude);
  return x.sign<0?Interval{-z.hi,-z.lo}:z;
}
inline Interval quotient(Interval n, Interval d) {
  if(n.lo==0 && n.hi==0) return {};
  if(n.lo>=0) return {down(n.lo/d.hi),up(n.hi/d.lo)};
  if(n.hi<=0) return {down(n.lo/d.lo),up(n.hi/d.hi)};
  return {down(n.lo/d.lo),up(n.hi/d.lo)};
}
inline Interval subtract(Interval x, Interval y) {
  return {down(x.lo-y.hi),up(x.hi-y.lo)};
}
inline Interval add_nonnegative(Interval x, Interval y) {
  return {std::fmax(0,down(x.lo+y.lo)),up(x.hi+y.hi)};
}
inline Interval square(Interval x) {
  const double l=(x.lo<=0 && x.hi>=0)?0:std::fmin(std::fabs(x.lo),std::fabs(x.hi));
  const double h=std::fmax(std::fabs(x.lo),std::fabs(x.hi));
  return {std::fmax(0,down(l*l)),up(h*h)};
}
inline double relative(std::uint32_t x,std::uint32_t anchor) {
  return static_cast<double>(static_cast<std::int64_t>(x)-static_cast<std::int64_t>(anchor));
}
}
inline Status integer_interval(const Signed& x, Interval& out) {
  if(!environment()) return Status::Environment;
  if(!detail::valid(x)) return Status::Coefficient;
  out=detail::signed_value(x); return Status::Ready;
}
struct Answer { Status status=Status::Ambiguous; Interval distance; };
class Sphere {
  std::array<Interval,3> rho_{};
  Interval radius_{};
  Point anchor_{};
  bool prepared_=false;
 public:
  const auto& rho() const { return rho_; }
  Interval radius() const { return radius_; }
  Status prepare(Point a,const std::array<Signed,3>& n,const Mag& denominator) {
    prepared_=false;
    if(!environment()) return Status::Environment;
    if(detail::zero(denominator)) return Status::Denominator;
    for(const auto& coefficient:n) if(!detail::valid(coefficient)) return Status::Coefficient;
    const auto d=detail::magnitude(denominator);
    anchor_=a; radius_={};
    for(unsigned j=0;j<3;++j) {
      rho_[j]=detail::quotient(detail::signed_value(n[j]),d);
      radius_=detail::add_nonnegative(radius_,detail::square(rho_[j]));
    }
    prepared_=true; return Status::Ready;
  }
  Answer site(Point x) const {
    if(!environment()) return {Status::Environment,{}};
    if(!prepared_) return {Status::Coefficient,{}};
    Interval distance{};
    for(unsigned j=0;j<3;++j) {
      const double v=detail::relative(x[j],anchor_[j]);
      distance=detail::add_nonnegative(distance,detail::square(detail::subtract({v,v},rho_[j])));
    }
    Status status=distance.lo>radius_.hi?Status::Exterior:
                  distance.hi<radius_.lo?Status::Interior:Status::Ambiguous;
#ifdef MUTANT_FIXED_MARGIN
    double r=0,d=0;
    for(unsigned j=0;j<3;++j) {
      const double c=(rho_[j].lo+rho_[j].hi)*.5;
      const double v=detail::relative(x[j],anchor_[j])-c;
      r+=c*c; d+=v*v;
    }
    status=d>r+.02?Status::Exterior:d<r-.02?Status::Interior:Status::Ambiguous;
#endif
    return {status,distance};
  }
  Answer box(Point lo,Point hi) const {
    if(!environment()) return {Status::Environment,{}};
    if(!prepared_) return {Status::Coefficient,{}};
    for(unsigned j=0;j<3;++j) if(lo[j]>hi[j]) return {Status::Box,{}};
    Interval distance{};
    for(unsigned j=0;j<3;++j) {
      const double l=detail::relative(lo[j],anchor_[j]),h=detail::relative(hi[j],anchor_[j]);
      double gl=0;
      if(l>rho_[j].hi) gl=std::fmax(0,detail::down(l-rho_[j].hi));
      else if(rho_[j].lo>h) gl=std::fmax(0,detail::down(rho_[j].lo-h));
      const double gh=std::fmax(0,std::fmax(detail::up(l-rho_[j].lo),detail::up(rho_[j].hi-h)));
      distance=detail::add_nonnegative(distance,{std::fmax(0,detail::down(gl*gl)),detail::up(gh*gh)});
    }
    return {distance.lo>radius_.hi?Status::Exterior:Status::Ambiguous,distance};
  }
};
static_assert(std::numeric_limits<double>::is_iec559 && sizeof(double)==8);
static_assert(sizeof(std::uint64_t)==8 && sizeof(std::uint32_t)==4);
} // namespace relative_filter
namespace cli {
using namespace relative_filter;
template<class T> bool read(std::istringstream& in,T& value) {
  std::string token;
  if(!(in>>token)) return false;
  if constexpr(std::is_unsigned_v<T>) if(token[0]=='-') return false;
  const auto r=std::from_chars(token.data(),token.data()+token.size(),value);
  return r.ec==std::errc{} && r.ptr==token.data()+token.size();
}
bool point(std::istringstream& in,Point& p) {
  for(auto& v:p) if(!read(in,v)) return false;
  return true;
}
bool signed_value(std::istringstream& in,Signed& v) {
  return read(in,v.sign) && read(in,v.magnitude[2]) && read(in,v.magnitude[1]) && read(in,v.magnitude[0]);
}
bool end(std::istringstream& in) { std::string extra; return !(in>>extra); }
void interval(Interval v) {
  std::cout<<'['<<std::bit_cast<std::uint64_t>(v.lo)<<','<<std::bit_cast<std::uint64_t>(v.hi)<<']';
}
void status(char op,Status s) { std::cout<<"{\"op\":\""<<op<<"\",\"status\":\""<<name(s)<<'"'; }
}
int main(int argc,char** argv) {
  using namespace relative_filter;
  for(int i=1;i<argc;++i) {
    const std::string option=argv[i];
    const int mode=option=="--round=nearest"?FE_TONEAREST:option=="--round=up"?FE_UPWARD:
                   option=="--round=down"?FE_DOWNWARD:option=="--round=zero"?FE_TOWARDZERO:-1;
    if(mode!=-1) { if(std::fesetround(mode)!=0) return 2; }
#if defined(__SSE2__)
    else if(option=="--ftz") _mm_setcsr(_mm_getcsr()|(1u<<15));
    else if(option=="--daz") _mm_setcsr(_mm_getcsr()|(1u<<6));
#endif
    else return 2;
  }
  std::string line;
  while(std::getline(std::cin,line)) {
    std::istringstream in(line); char op=0;
    if(!(in>>op)) continue;
    Point a{},lo{},hi{}; std::array<Signed,3> n{}; Mag d{};
    bool ok=false; Interval value{};
    if(op=='C') {
      ok=cli::signed_value(in,n[0]) && cli::end(in);
      if(ok) { const auto s=integer_interval(n[0],value); cli::status(op,s);
        if(s==Status::Ready) { std::cout<<",\"value\":"; cli::interval(value); } std::cout<<"}\n"; }
    } else if(op=='S' || op=='B') {
      ok=cli::point(in,a) && cli::point(in,lo) && (op=='S' || cli::point(in,hi));
      for(auto& v:n) ok=ok && cli::signed_value(in,v);
      ok=ok && cli::read(in,d[2]) && cli::read(in,d[1]) && cli::read(in,d[0]) && cli::end(in);
      if(ok) {
        Sphere ball; const auto s=ball.prepare(a,n,d);
        const auto answer=s==Status::Ready?(op=='S'?ball.site(lo):ball.box(lo,hi)):Answer{s,{}};
        cli::status(op,answer.status);
        if(s==Status::Ready) {
          std::cout<<",\"rho\":[";
          for(unsigned j=0;j<3;++j) { if(j) std::cout<<','; cli::interval(ball.rho()[j]); }
          std::cout<<"],\"radius\":"; cli::interval(ball.radius());
          std::cout<<",\"distance\":"; cli::interval(answer.distance);
        }
        std::cout<<"}\n";
      }
    }
    if(!ok) std::cout<<"{\"op\":\"?\",\"status\":\"INVALID_INPUT\"}\n";
  }
  return std::cin.bad()?2:0;
}
