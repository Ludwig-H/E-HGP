// Sonde d'orientation : candidat juge avant materialisation, plans de requete independants du support.
#include <array>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include "num/num.hpp"
using namespace mhgp11;
using namespace mhgp11::num;
namespace {
template<class T>
std::string hex(const T& value) {
  const auto wide=to_wide(value); std::ostringstream out;
  if (wide.sign()<0) out<<'-';
  out<<std::hex<<std::setfill('0');
  for (std::size_t i=wide.words.size();i!=0;--i) out<<std::setw(16)<<wide.words[i-1];
  return out.str();
}
template<class Ball>
bool print_base(const Ball& ball,const std::array<Point,7>& p) {
  const auto forward=orientation(p[4],p[5],p[6],ball), reverse=orientation(p[4],p[6],p[5],ball);
  const auto inside=strictly_inside(ball,p[0],p[1],p[2],p[3]);
  if (!forward.ok() || !reverse.ok() || !inside.ok()) return false;
  const auto anchor = ball.anchor();  // Le Point possede les coordonnees pendant toute la boucle C++20.
  for (const auto x : anchor.coordinates()) std::cout<<' '<<x;
  for (const auto n : ball.numerator()) std::cout<<' '<<hex(n);
  std::cout<<' '<<hex(ball.denominator())<<' '<<ball.orientation_i128_certified()
           <<' '<<forward.value()<<' '<<reverse.value()<<' '<<inside.value();
  return true;
}
bool print_sphere(const Sphere& ball,const std::array<Point,7>& p) {
  if (!print_base(ball,p)) return false;
  std::cout<<' '<<hex(ball.level().numerator())<<' '<<hex(ball.level().denominator());
  return true;
}
Result<std::optional<Sphere>> make_sphere(i64 q,const std::array<Point,7>& p) {
  if (q==1) return std::optional<Sphere>{Sphere::point(p[0])};
  if (q==2) return Sphere::through(p[0],p[1]);
  return Sphere::through(p[0],p[1],p[2]);
}
}

int main() {
  std::cout<<"bits "<<kCoordBits<<'\n';
  i64 q=0;
  while (std::cin>>q) {
    std::array<Point,7> points{}; std::optional<Outcome> invalid;
    for (auto& point : points) {
      i64 x=0,y=0,z=0;
      if (!(std::cin>>x>>y>>z)) return 2;
      const auto made=Point::make(x,y,z);
      if (!made.ok()) { if (!invalid) invalid=made.outcome(); }
      else point=made.value();
    }
    if (q<1 || q>4) { std::cout<<"refused parameter_out_of_range\n"; continue; }
    if (invalid) { std::cout<<"refused "<<reason_name(invalid->reason)<<'\n'; continue; }
    if (q!=4) {
      const auto made=make_sphere(q,points);
      if (!made.ok()) return 3;
      if (!made.value()) { std::cout<<"degenerate\n"; continue; }
      std::cout<<"ok sphere";
      if (!print_sphere(*made.value(),points)) return 3;
    } else {
      const auto made=Q4Candidate::through(points[0],points[1],points[2],points[3]);
      if (!made.ok()) return 3;
      if (!made.value()) { std::cout<<"degenerate\n"; continue; }
      std::cout<<"ok candidate";
      if (!print_base(*made.value(),points)) return 3;
      const auto full=made.value()->materialize();
      if (!full.ok()) return 3;
      std::cout<<" materialized";
      if (!print_sphere(full.value(),points)) return 3;
      const auto eager=Sphere::through(points[0],points[1],points[2],points[3]);
      if (!eager.ok() || !eager.value()) return 3;
      std::cout<<" eager";
      if (!print_sphere(*eager.value(),points)) return 3;
    }
    std::cout<<'\n';
  }
  return std::cin.eof()?0:2;
}
