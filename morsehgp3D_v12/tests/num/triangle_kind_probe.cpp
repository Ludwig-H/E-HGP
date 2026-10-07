// Neuf coordonnees par requete ; domaine valide avant classification et ancienne fabrique q3.
#include <array>
#include <iostream>
#include <optional>
#include "num/num.hpp"
using namespace mhgp12;
using namespace mhgp12::num;
int main() {
  std::cout<<"bits "<<kCoordBits<<'\n';
  i64 first=0;
  while (std::cin>>first) {
    std::array<Point,3> p{}; std::optional<Outcome> invalid;
    for (u32 i=0;i<3;++i) {
      i64 x=first,y=0,z=0;
      if ((i!=0 && !(std::cin>>x)) || !(std::cin>>y>>z)) return 2;
      const auto made=Point::make(x,y,z);
      if (!made.ok()) { if (!invalid) invalid=made.outcome(); }
      else p[i]=made.value();
    }
    if (invalid) { std::cout<<"refused "<<reason_name(invalid->reason)<<'\n'; continue; }
    const auto kind=classify_triangle(p[0],p[1],p[2]);
    const char* name=kind==TriangleKind::strict?"strict":kind==TriangleKind::degenerate?"degenerate":"non_strict";
    const auto sphere=Sphere::through(p[0],p[1],p[2]); if (!sphere.ok()) return 3;
    std::cout<<"ok "<<name<<' '<<sphere.value().has_value()<<' '<<strictly_acute(p[0],p[1],p[2])<<'\n';
  }
  return std::cin.eof()?0:2;
}
