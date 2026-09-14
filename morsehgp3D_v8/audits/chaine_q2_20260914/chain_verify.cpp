// Auditeur B (14 sept. 2026) — verificateur bout-en-bout de la chaine front + census q2
// (run_wspd_q2_census, sources e3af11a7), toutes combinaisons de modes, contre force brute.
// Auditeur B : vérification exécutée de la chaîne front + census q2 (run_wspd_q2_census, e3af11a7)
// pour toutes les combinaisons de modes, contre une force brute exacte sur tous les sites :
// chaque paire non ordonnée avec p<Kmax intérieurs stricts doit être émise exactement une fois,
// avec exactement ses IDs intérieurs et toute sa coquille (a et b compris) et sa clé ;
// aucune paire avec p>=Kmax ne doit être émise.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "pipeline/wspd_q2_census.hpp"
#include "front_fixtures.hpp"
using namespace mhgp8;
struct Emitted { std::vector<std::size_t> interior, shell; Q2BallKey key; unsigned count; };
static std::vector<Point3> adversarial(const std::string& name, unsigned seed) {
  std::vector<Point3> p; std::set<std::tuple<int,int,int>> seen;
  auto add = [&](int x, int y, int z) { if (x<0||y<0||z<0||x>65535||y>65535||z>65535) return; if (seen.insert({x,y,z}).second) p.push_back({(std::uint16_t)x,(std::uint16_t)y,(std::uint16_t)z}); };
  if (name == "grid5") { for (int x=0;x<5;++x) for (int y=0;y<5;++y) for (int z=0;z<5;++z) add(100+7*x,100+7*y,100+7*z); }
  else if (name == "cospherical") { // sphère r=13 centre (1000,1000,1000): représentations 169=a²+b²+c²
    for (int a=-13;a<=13;++a) for (int b=-13;b<=13;++b) for (int c=-13;c<=13;++c) if (a*a+b*b+c*c==169) add(1000+a,1000+b,1000+c);
    for (int a=-5;a<=5;++a) for (int b=-5;b<=5;++b) for (int c=-5;c<=5;++c) if (a*a+b*b+c*c==25) add(1000+a,1000+b,1000+c);
    add(1000,1000,1000); add(1000,1000,1001); add(1300,1000,1000); add(700,1000,1000); }
  else if (name == "collinear") { for (int i=0;i<40;++i) add(10+37*i, 500, 500); for (int i=0;i<10;++i) add(200+13*i, 501, 500); add(0,0,0); add(65535,65535,65535); }
  else if (name == "cube_corners") { for (int c=0;c<8;++c) add(2000+((c&1)?300:0), 2000+((c&2)?300:0), 2000+((c&4)?300:0)); add(2150,2150,2150); add(2150,2150,2151); add(2000,2150,2150); add(60000,60000,60000); add(60000,60000,60001); add(60001,60000,60000); }
  else if (name == "extremes") { add(0,0,0); add(65535,0,0); add(0,65535,0); add(0,0,65535); add(65535,65535,65535); add(32767,32768,32767); add(32768,32767,32768); add(1,1,1); add(65534,65534,65534); add(40000,20000,60000); add(40001,20000,60000); add(40000,20001,60000); }
  else if (name == "halfint") { std::srand(seed); for (int i=0;i<60;++i) add(1001+2*(std::rand()%30), 2001+2*(std::rand()%30), 3001+2*(std::rand()%30)); }
  else if (name == "dense_ball") { std::srand(seed); for (int i=0;i<80;++i) { int a=std::rand()%21-10, b=std::rand()%21-10, c=std::rand()%21-10; if (a*a+b*b+c*c<=100) add(5000+a,5000+b,5000+c); } add(5000,5000,5000); add(5010,5000,5000); add(4990,5000,5000); }
  return p;
}
int main(int argc, char** argv) {
  if (argc < 5) { std::fprintf(stderr, "usage: family|adv:name n kmax s [seed]\n"); return 2; }
  const std::string fam = argv[1]; const std::size_t n = std::strtoull(argv[2], nullptr, 10);
  const unsigned kmax = std::atoi(argv[3]); const unsigned s = std::atoi(argv[4]); const unsigned seed = argc > 5 ? std::atoi(argv[5]) : 3;
  std::vector<Point3> pts;
  if (fam.rfind("adv:", 0) == 0) pts = adversarial(fam.substr(4), seed); else pts = bench::make_front_fixture(n, fam, seed).points;
  auto cloud = prepare_cloud(pts); auto index = make_q2_cloud_index(cloud);
  const auto P = index->cloud().points(); const std::size_t N = P.size();
  // brute force truth
  auto truth_of = [&](std::size_t i, std::size_t j, std::vector<std::size_t>& in, std::vector<std::size_t>& sh) {
    in.clear(); sh.clear();
    for (std::size_t k = 0; k < N; ++k) {
      i64 h = 0; for (int ax=0;ax<3;++ax) h += ((i64)P[k][ax]-P[i][ax])*((i64)P[j][ax]-P[k][ax]);
      if (h > 0) in.push_back(k); else if (h == 0) sh.push_back(k);
    }
  };
  u64 total_mismatch = 0, total_checked = 0, total_emitted = 0, total_alive = 0;
  struct Combo { WspdFrontMode f; Q2CensusMode c; Q2SiblingMode sb; Q2WitnessOrder o; const char* name; };
  const Combo combos[] = {
    {WspdFrontMode::Pure, Q2CensusMode::Pairwise, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, "pure/pairwise"},
    {WspdFrontMode::Pure, Q2CensusMode::SharedBlocks, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, "pure/shared"},
    {WspdFrontMode::MidpointSamples, Q2CensusMode::Pairwise, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, "samples/pairwise"},
    {WspdFrontMode::MidpointSamples, Q2CensusMode::SharedBlocks, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, "samples/shared"},
    {WspdFrontMode::MidpointSamples, Q2CensusMode::SharedBlocks, Q2SiblingMode::Saturating, Q2WitnessOrder::GlobalDfs, "samples/shared/sibling"},
    {WspdFrontMode::MidpointSamples, Q2CensusMode::SharedBlocks, Q2SiblingMode::Disabled, Q2WitnessOrder::ComplementFirst, "samples/shared/complement"},
    {WspdFrontMode::MidpointSamples, Q2CensusMode::SharedBlocks, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, "samples/shared/sibling+complement"},
    {WspdFrontMode::Pure, Q2CensusMode::SharedBlocks, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, "pure/shared/sibling+complement"},
  };
  for (const auto& cb : combos) {
    std::map<std::pair<std::size_t,std::size_t>, Emitted> emitted; u64 dup = 0;
    auto consumer = [&](const Q2Support& sp) {
      auto key = std::make_pair(std::min(sp.a_id, sp.b_id), std::max(sp.a_id, sp.b_id));
      if (emitted.count(key)) ++dup;
      Emitted e; e.interior.assign(sp.interior.begin(), sp.interior.end()); e.shell.assign(sp.shell.begin(), sp.shell.end()); e.key = sp.key; e.count = 1;
      std::sort(e.interior.begin(), e.interior.end()); std::sort(e.shell.begin(), e.shell.end());
      emitted[key] = e;
    };
    auto res = run_wspd_q2_census(*index, kmax, s, cb.f, cb.c, consumer, cb.sb, cb.o);
    u64 mism = 0, alive = 0, checked = 0; std::vector<std::size_t> in, sh; std::string first;
    for (std::size_t i = 0; i < N; ++i) for (std::size_t j = i + 1; j < N; ++j) {
      ++checked; truth_of(i, j, in, sh);
      const bool should = in.size() < kmax;
      auto it = emitted.find({i, j});
      bool ok = true;
      if (should) {
        ++alive;
        if (it == emitted.end()) ok = false;
        else {
          std::vector<std::size_t> shell_full = sh; // shell includes endpoints (H=0 for k=i or k=j)
          if (it->second.interior != in || it->second.shell != shell_full) ok = false;
          Q2BallKey k{}; for (int ax=0;ax<3;++ax) { k.center_twice[ax] = (std::uint32_t)P[i][ax] + P[j][ax]; i64 d = (i64)P[i][ax]-P[j][ax]; k.diameter_squared += (u64)(d*d); }
          if (!(it->second.key == k)) ok = false;
        }
      } else if (it != emitted.end()) ok = false;
      if (!ok) { ++mism; if (first.empty()) first = "pair (" + std::to_string(P[i].x) + "," + std::to_string(P[i].y) + "," + std::to_string(P[i].z) + ")-(" + std::to_string(P[j].x) + "," + std::to_string(P[j].y) + "," + std::to_string(P[j].z) + ") p=" + std::to_string(in.size()) + (it==emitted.end()?" MISSING":" WRONG/EXTRA"); }
    }
    total_mismatch += mism; total_checked += checked; total_emitted += emitted.size(); total_alive += alive;
    std::printf("%-36s n=%zu kmax=%u s=%u emitted=%zu alive=%llu dup=%llu mismatch=%llu accepted=%llu rejected=%llu sibling_rejected=%llu order_switches=%llu %s\n", cb.name, N, kmax, s, emitted.size(), (unsigned long long)alive, (unsigned long long)dup, (unsigned long long)mism,
      (unsigned long long)res.census.accepted_pairs, (unsigned long long)res.census.rejected_pairs, (unsigned long long)res.sibling_work.rejected_pairs, (unsigned long long)res.order_work.phase_switches, first.c_str());
  }
  std::printf("SUMMARY family=%s n=%zu kmax=%u s=%u checked=%llu alive=%llu emitted=%llu mismatch=%llu\n", fam.c_str(), N, kmax, s, (unsigned long long)total_checked, (unsigned long long)total_alive, (unsigned long long)total_emitted, (unsigned long long)total_mismatch);
  return total_mismatch == 0 ? 0 : 1;
}
