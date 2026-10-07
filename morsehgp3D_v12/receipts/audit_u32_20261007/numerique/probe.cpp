// Sonde d'audit : seuls les arguments/protocoles sont nouveaux, toutes les decisions sont celles de num.
#include <array>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include "num/geometry.hpp"
using namespace mhgp12;
using namespace mhgp12::num;
template<class T> std::string hex(const T& value) {
  const auto wide=to_wide(value); std::ostringstream out;
  if(wide.sign()<0)out<<'-';
  out<<std::hex<<std::setfill('0');
  for(std::size_t i=wide.words.size();i;i--)out<<std::setw(16)<<wide.words[i-1];
  return out.str();
}
void lanes(const LaneCount& l){std::cout<<' '<<l.native<<' '<<l.certified<<' '<<l.checked<<' '<<l.wide;}
int main(){
  std::optional<Sphere> previous; int q;
  while(std::cin>>q){
    std::array<Point,8> p{};
    for(auto& x:p){i64 a,b,c;if(!(std::cin>>a>>b>>c))return 2;auto made=Point::make(a,b,c);if(!made.ok())return 3;x=made.value();}
    LaneCount construction;
    auto make=[&]() -> Result<std::optional<Sphere>> {
      if(q==2)return Sphere::through(p[0],p[1]);
      if(q==3){auto c=Q3Candidate::through(p[0],p[1],p[2],&construction);if(!c.ok())return c.outcome();if(!c.value())return std::optional<Sphere>{};auto s=c.value()->materialize();if(!s.ok())return s.outcome();return std::optional<Sphere>{s.value()};}
      auto c=Q4Candidate::through(p[0],p[1],p[2],p[3],&construction);if(!c.ok())return c.outcome();if(!c.value())return std::optional<Sphere>{};auto s=c.value()->materialize();if(!s.ok())return s.outcome();return std::optional<Sphere>{s.value()};
    };
    auto made=make();if(!made.ok()){std::cout<<"refused "<<reason_name(made.outcome().reason)<<'\n';continue;}
    if(!made.value()){std::cout<<"degenerate\n";continue;}
    const auto& s=*made.value();
    std::cout<<"ok";for(const auto& n:s.numerator())std::cout<<' '<<hex(n);
    std::cout<<' '<<hex(s.denominator())<<' '<<hex(s.level().numerator())<<' '<<hex(s.level().denominator());
    std::cout<<' '<<int(s.support_span())<<' '<<s.power_domain()<<' '<<s.orientation_domain()<<' '<<s.q4_presentation_strictly_inside();
    lanes(construction);
    for(int i=4;i<8;i++){LaneCount l;auto v=power(s,p[i],&l);if(!v.ok())return 4;std::cout<<' '<<hex(v.value());lanes(l);}
    LaneCount o,m,c,l;
    auto orient=orientation(p[4],p[5],p[6],s,&o);if(!orient.ok())return 5;
    std::cout<<' '<<orient.value();lanes(o);std::cout<<' '<<is_midpoint(s,p[4],p[5],&m);lanes(m);
    std::cout<<' '<<(previous?compare_centers(s,*previous,&c):0);lanes(c);
    std::cout<<' '<<(previous?compare(s.level(),previous->level(),&l):0);lanes(l);
    std::cout<<'\n'; previous=s;
  }
  return std::cin.eof()?0:2;
}
