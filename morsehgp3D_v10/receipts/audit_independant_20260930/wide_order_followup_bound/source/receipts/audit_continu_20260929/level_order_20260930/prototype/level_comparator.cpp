#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#if !defined(__SIZEOF_INT128__)
#error "This audit prototype requires unsigned 128-bit compiler arithmetic"
#endif
namespace level_comparator {
__extension__ using U128 = unsigned __int128;
using Num=std::array<std::uint64_t,5>; // LOW -> HIGH internally
using Den=std::array<std::uint64_t,4>;
using Words=std::array<std::uint64_t,8>;
struct Rational { Num numerator{}; Den denominator{}; };
struct Product { Words words{}; std::uint64_t high=0; };
enum class Status { Ready, NumDomain, DenDomain, ZeroDen, Overflow };
struct Answer { Status status=Status::Ready; char side='-'; int cmp=0; Product lhs{},rhs{}; };
inline const char* name(Status status) {
  switch(status) {
    case Status::Ready:return "READY";
    case Status::NumDomain:return "REFUSED_NUM_DOMAIN";
    case Status::DenDomain:return "REFUSED_DEN_DOMAIN";
    case Status::ZeroDen:return "REFUSED_ZERO_DEN";
    case Status::Overflow:return "REFUSED_OVERFLOW";
  }
  return "INVALID_STATUS";
}
inline Status domain(const Rational& value) {
  if(value.numerator[4]>>10) return Status::NumDomain; // at most 266 magnitude bits
  if(value.denominator[3]>>8) return Status::DenDomain; // at most 200 magnitude bits
  if((value.denominator[0]|value.denominator[1]|value.denominator[2]|value.denominator[3])==0)
    return Status::ZeroDen;
  return Status::Ready;
}
// Full raw 5x4-limb product. Output has 8 words; the omitted ninth word is
// explicitly reported as high. No partial/truncated product may be accepted
// when high!=0. Per sum: a*b + word + carry <= 2^128-1, so U128 does not wrap.
inline Product checked_product8(const Num& a,const Den& b) {
  Product out{};
  for(unsigned i=0;i<5;++i) {
    std::uint64_t carry=0;
    for(unsigned j=0;j<4;++j) {
      const U128 sum=static_cast<U128>(a[i])*b[j]+out.words[i+j]+carry;
      out.words[i+j]=static_cast<std::uint64_t>(sum);
      carry=static_cast<std::uint64_t>(sum>>64);
    }
    // Earlier rows touched at most word i+3; word i+4 is still zero.
    if(i<4) out.words[i+4]=carry;
    else out.high=carry;
  }
#ifdef MUTANT_IGNORE_HIGH
  out.high=0;
#endif
  return out;
}
inline int compare_words(const Words& lhs,const Words& rhs) {
  for(unsigned j=8;j>0;--j) {
    if(lhs[j-1]<rhs[j-1]) return -1;
    if(lhs[j-1]>rhs[j-1]) return 1;
  }
  return 0;
}
#ifdef MUTANT_FLOAT
template<std::size_t N> double approximation(const std::array<std::uint64_t,N>& a) {
  double out=0;
  for(std::size_t j=N;j>0;--j) out=std::ldexp(out,64)+static_cast<double>(a[j-1]);
  return out;
}
#endif
inline Answer compare(const Rational& a,const Rational& b) {
  Answer out{};
  // BOTH input domains are certified BEFORE either multiplication.
  const auto left=domain(a),right=domain(b);
  if(left!=Status::Ready) { out.status=left; out.side='A'; return out; }
  if(right!=Status::Ready) { out.status=right; out.side='B'; return out; }
  out.lhs=checked_product8(a.numerator,b.denominator);
  out.rhs=checked_product8(b.numerator,a.denominator);
  if(out.lhs.high!=0 || out.rhs.high!=0) { out.status=Status::Overflow; return out; }
  out.cmp=compare_words(out.lhs.words,out.rhs.words);
#ifdef MUTANT_FLOAT
  const double x=approximation(a.numerator)/approximation(a.denominator);
  const double y=approximation(b.numerator)/approximation(b.denominator);
  out.cmp=x<y?-1:x>y?1:0;
#endif
  return out;
}
static_assert(sizeof(std::uint64_t)==8 && sizeof(U128)==16);
} // namespace level_comparator
namespace cli {
using namespace level_comparator;
template<std::size_t N> bool read(std::istringstream& in,std::array<std::uint64_t,N>& a) {
  for(std::size_t j=N;j>0;--j) {
    std::string token;
    if(!(in>>token) || token[0]=='-') return false;
    const auto result=std::from_chars(token.data(),token.data()+token.size(),a[j-1]);
    if(result.ec!=std::errc{} || result.ptr!=token.data()+token.size()) return false;
  }
  return true;
}
bool rational(std::istringstream& in,Rational& a) {
  return read(in,a.numerator) && read(in,a.denominator);
}
bool end(std::istringstream& in) { std::string extra; return !(in>>extra); }
void words(const Words& a) {
  std::cout<<'[';
  for(unsigned j=8;j>0;--j) { if(j!=8) std::cout<<','; std::cout<<a[j-1]; }
  std::cout<<']';
}
}
int main(int argc,char**) {
  using namespace level_comparator;
  if(argc!=1) return 2;
  std::string line;
  while(std::getline(std::cin,line)) {
    std::istringstream in(line); char op=0;
    if(!(in>>op)) continue;
    Rational a{},b{}; bool ok=false;
    if(op=='C') {
      ok=cli::rational(in,a) && cli::rational(in,b) && cli::end(in);
      if(ok) {
        const auto out=compare(a,b);
        std::cout<<"{\"op\":\"C\",\"status\":\""<<name(out.status)<<'"';
        if(out.status==Status::Ready) {
          std::cout<<",\"cmp\":"<<out.cmp<<",\"lhs\":"; cli::words(out.lhs.words);
          std::cout<<",\"rhs\":"; cli::words(out.rhs.words);
        } else if(out.side!='-') std::cout<<",\"side\":\""<<out.side<<'"';
        std::cout<<"}\n";
      }
    } else if(op=='M') {
      ok=cli::rational(in,a) && cli::end(in);
      if(ok) {
        const auto out=checked_product8(a.numerator,a.denominator);
        std::cout<<"{\"op\":\"M\",\"status\":\""<<name(out.high?Status::Overflow:Status::Ready)
                 <<"\",\"high_word\":"<<out.high;
        if(out.high==0) { std::cout<<",\"product\":"; cli::words(out.words); }
        std::cout<<"}\n";
      }
    }
    if(!ok) std::cout<<"{\"op\":\"?\",\"status\":\"INVALID_INPUT\"}\n";
  }
  return std::cin.bad()?2:0;
}
