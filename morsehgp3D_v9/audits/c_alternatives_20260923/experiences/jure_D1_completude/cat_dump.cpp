// Juge D1 : vidage du catalogue v9 (cle, p, q_min, u) sur un petit nuage texte.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "src/chain/tower_chain.hpp"
using mhgp9::gen::Point3;
__extension__ typedef __int128 i128;
__extension__ typedef unsigned __int128 u128;
static std::string dec(i128 v) {
  if (v == 0) return "0";
  bool neg = v < 0; u128 u = neg ? (u128)(-(v + 1)) + 1 : (u128)v; std::string s;
  while (u) { s.push_back(char('0' + int(u % 10))); u /= 10; }
  if (neg) s.push_back('-');
  return std::string(s.rbegin(), s.rend());
}
int main(int argc, char** argv) {
  if (argc < 4) return 2;
  mhgp9::ChainOptions opt;
  opt.kmax = (unsigned)std::atoi(argv[1]);
  opt.workers = (size_t)std::atoi(argv[2]);
  opt.run_tower = std::atoi(argv[3]) != 0;
  opt.keep_catalogue = true;
  if (argc > 4) opt.separation_s = (unsigned)std::atoi(argv[4]);
  std::vector<Point3> pts; long x, y, z;
  while (std::scanf("%ld %ld %ld", &x, &y, &z) == 3)
    pts.push_back(Point3{(mhgp9::gen::Coordinate)x, (mhgp9::gen::Coordinate)y, (mhgp9::gen::Coordinate)z});
  const auto res = mhgp9::run_tower_chain(pts, opt);
  std::printf("STATUS %s %s\n", mhgp9::chain_status_name(res.status), res.reason.c_str());
  for (const auto& b : res.catalogue_balls)
    std::printf("B %s %s %s %s %s %d %d %d\n", dec(b.key.a).c_str(), dec(b.key.b[0]).c_str(), dec(b.key.b[1]).c_str(),
                dec(b.key.b[2]).c_str(), dec(b.key.c).c_str(), (int)b.n_interior, (int)b.arity, (int)b.n_shell);
  if (opt.run_tower && res.status == mhgp9::ChainStatus::kComplete)
    std::printf("DIGEST %016llx\n", (unsigned long long)res.tower_digest);
  return 0;
}
