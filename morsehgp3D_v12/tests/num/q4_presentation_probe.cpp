// Une ligne q + huit Point : presentation puis tetraedre de requete independant ; aucune vie temporaire empruntee.
#include <array>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include "num/num.hpp"
using namespace mhgp12;
using namespace mhgp12::num;
namespace {
template<class T> std::string hex(const T& value) {
  const auto w=to_wide(value); std::ostringstream out;
  if (w.sign()<0) out<<'-';
  out<<std::hex<<std::setfill('0');
  for (std::size_t i=w.words.size();i!=0;--i) out<<std::setw(16)<<w.words[i-1];
  return out.str();
}
template<class Ball> bool output(const Ball& ball,const std::array<Point,8>& p) {
  const auto a=strictly_inside(ball,p[0],p[1],p[2],p[3]);
  const auto b=strictly_inside(ball,p[4],p[5],p[6],p[7]);
  if (!a.ok() || !b.ok()) return false;
  const auto anchor=ball.anchor();
  std::cout<<' '<<ball.q4_presentation_strictly_inside()<<' '<<a.value()<<' '<<b.value();
  for (auto v:anchor.coordinates()) std::cout<<' '<<v;
  for (auto v:ball.numerator()) std::cout<<' '<<hex(v);
  std::cout<<' '<<hex(ball.denominator());
  return true;
}
bool output_sphere(const Sphere& ball,const std::array<Point,8>& p) {
  if (!output(ball,p)) return false;
  std::cout<<' '<<hex(ball.level().numerator())<<' '<<hex(ball.level().denominator());
  return true;
}
Result<std::optional<Sphere>> make(i64 q,const std::array<Point,8>& p) {
  if (q==1) return std::optional<Sphere>{Sphere::point(p[0])};
  if (q==2) return Sphere::through(p[0],p[1]);
  return Sphere::through(p[0],p[1],p[2]);
}
}
int main() {
  std::cout<<"bits "<<kCoordBits<<'\n';
  i64 q=0;
  while (std::cin>>q) {
    std::array<Point,8> p{}; std::optional<Outcome> invalid;
    for (auto& v:p) {
      i64 x=0,y=0,z=0;
      if (!(std::cin>>x>>y>>z)) return 2;
      auto made=Point::make(x,y,z);
      if (!made.ok()) { if (!invalid) invalid=made.outcome(); }
      else v=made.value();
    }
    if (q<1 || q>4) { std::cout<<"refused parameter_out_of_range\n"; continue; }
    if (invalid) { std::cout<<"refused "<<reason_name(invalid->reason)<<'\n'; continue; }
    if (q<4) {
      auto s=make(q,p); if (!s.ok()) return 3;
      if (!s.value()) { std::cout<<"degenerate\n"; continue; }
      std::cout<<"ok sphere"; if (!output_sphere(*s.value(),p)) return 3;
    } else {
      auto c=Q4Candidate::through(p[0],p[1],p[2],p[3]); if (!c.ok()) return 3;
      if (!c.value()) { std::cout<<"degenerate\n"; continue; }
      std::cout<<"ok candidate"; if (!output(*c.value(),p)) return 3;
      auto s=c.value()->materialize(); if (!s.ok()) return 3;
      std::cout<<" materialized"; if (!output_sphere(s.value(),p)) return 3;
      auto t=Sphere::through(p[0],p[1],p[2],p[3]); if (!t.ok() || !t.value()) return 3;
      std::cout<<" eager"; if (!output_sphere(*t.value(),p)) return 3;
    }
    std::cout<<'\n';
  }
  return std::cin.eof()?0:2;
}
