// L04 audit : harnais des predicats de arith/geometry.hpp (lecture seule du depot : en-tetes inclus tels quels).
// Entree (stdin) : lignes "a b c d z anchor_unused" = 5 points (15 entiers). Sortie : valeurs decimales exactes.
#include <cstdio>
#include <iostream>
#include <string>

#include "arith/geometry.hpp"

using namespace mhgp10;
using geom::P3;

static std::string s128(i128 v) {
  if (v == 0) return "0";
  const bool neg = v < 0;
  u128 m = neg ? u128(0) - static_cast<u128>(v) : static_cast<u128>(v);
  std::string s;
  while (m) {
    s.push_back(char('0' + int(m % 10)));
    m /= 10;
  }
  if (neg) s.push_back('-');
  return std::string(s.rbegin(), s.rend());
}

int main() {
  long long v[15];
  geom::Level prev4{};
  bool have_prev4 = false;
  for (;;) {
    for (int i = 0; i < 15; ++i)
      if (!(std::cin >> v[i])) return 0;
    const P3 a{v[0], v[1], v[2]}, b{v[3], v[4], v[5]}, c{v[6], v[7], v[8]}, d{v[9], v[10], v[11]}, z{v[12], v[13], v[14]};
    geom::Center c3{}, c4{}, c2{};
    geom::center2(a, b, c2);
    const bool ok3 = geom::center3(a, b, c, c3);
    const bool ok4 = geom::center4(a, b, c, d, c4);
    std::printf("C2 %s %s %s %s\n", s128(c2.N[0]).c_str(), s128(c2.N[1]).c_str(), s128(c2.N[2]).c_str(), s128(c2.D).c_str());
    std::printf("C3 %d", int(ok3));
    if (ok3) std::printf(" %s %s %s %s", s128(c3.N[0]).c_str(), s128(c3.N[1]).c_str(), s128(c3.N[2]).c_str(), s128(c3.D).c_str());
    std::printf("\nC4 %d", int(ok4));
    if (ok4) std::printf(" %s %s %s %s", s128(c4.N[0]).c_str(), s128(c4.N[1]).c_str(), s128(c4.N[2]).c_str(), s128(c4.D).c_str());
    std::printf("\n");
    // cote de z et de d pour chaque forme
    std::printf("S2 %s %d\n", s128(geom::side_key(c2, a, z)).c_str(), geom::side(c2, a, z));
    if (ok3) std::printf("S3 %s %d %s %d\n", s128(geom::side_key(c3, a, z)).c_str(), geom::side(c3, a, z),
                         s128(geom::side_key(c3, a, d)).c_str(), geom::side(c3, a, d));
    if (ok4) std::printf("S4 %s %d\n", s128(geom::side_key(c4, a, z)).c_str(), geom::side(c4, a, z));
    std::printf("OR %d\n", geom::orient(a, b, c, d));
    // orientation du centre par rapport au plan (b, c, z) : forme q4 en i128 et en large ; forme q3 en large (et en i128, hors contrat)
    if (ok4) std::printf("OC4 %d %d\n", geom::orient_center(b, c, z, a, c4), geom::orient_center_wide(b, c, z, a, c4));
    if (ok3) std::printf("OC3W %d\n", geom::orient_center_wide(b, d, z, a, c3));
#ifdef L04_Q3_I128
    if (ok3) std::printf("OC3N %d\n", geom::orient_center(b, d, z, a, c3));
#endif
    if (ok4) {
      const P3* t[4] = {&a, &b, &c, &d};
      std::printf("IN4 %d\n", int(geom::strictly_inside_tetra(t, a, c4)));
    }
    std::printf("AC %d\n", int(geom::acute(a, b, c)));
    std::printf("MID %d %d\n", int(geom::is_midpoint(a, b, a, c2)), ok3 ? int(geom::is_midpoint(a, b, a, c3)) : -1);
    const geom::Level l2 = geom::level2(a, b);
    std::printf("L2 %s %s\n", arith::to_string(l2.num).c_str(), arith::to_string(l2.den).c_str());
    geom::Level l3{}, l4{};
    if (ok3) {
      l3 = geom::level3(a, b, c);
      std::printf("L3 %s %s\n", arith::to_string(l3.num).c_str(), arith::to_string(l3.den).c_str());
    }
    if (ok4) {
      l4 = geom::level4(c4);
      std::printf("L4 %s %s\n", arith::to_string(l4.num).c_str(), arith::to_string(l4.den).c_str());
    }
    std::printf("CMP %d %d %d\n", ok3 ? geom::compare(l2, l3) : 9, ok4 ? geom::compare(l2, l4) : 9, (ok3 && ok4) ? geom::compare(l3, l4) : 9);
    if (ok4) {
      if (have_prev4) std::printf("CMP44 %d\n", geom::compare(prev4, l4));
      prev4 = l4;
      have_prev4 = true;
    }
    std::printf("END\n");
  }
}
