// MorseHGP3D v5 — FIXTURE du test d'ancre par secteurs (src/lanes/sector_kill.hpp).
// Configuration gravee : a = (0,0,0), b = (50,0,0) (D = 50, m = (25,0,0)) ;
// 37 sites EXACTEMENT sur la sphere diametrale (|2z-(a+b)|² = D²) — 25 dans
// le demi-espace z < 0 ou sur l'equateur, plus 12 sur le cercle z = 0 — tous
// a z <= 0 ; cette fixture exerce la FRONTIERE DIAMETRALE (|2w|² = D², sommet
// 0) ; la frontiere des demi-plans seule est F6 de anchor_kill_fixture ; un seed x = (25,0,30) (aigu, ab arete la plus
// longue) dont la boule circonscrite a son centre (25,0,55/12) : les sites de
// la sphere diametrale y sont strictement interieurs ssi z > 0 — aucun ici —
// donc x SURVIT et la production emet la boule {a,b,x}. Le test strict ne
// compte aucun temoin (|2w|² = D² n'est pas < D²) ; le mutant
// `sector-kill-nonstrict` compte ces sites frontiere comme universels sur les
// secteurs ou w.p >= 0 et TUE l'ancre : la boule disparait (code 4). C'est la
// contradiction minimale qui distingue « > » de « >= ».
// Codes : 0 (nominal : boule presente, temoins stricts = 0, temoins non
// stricts >= h3), 1 desaccord, 4 mutant tue.
#include <cstdio>
#include <string>
#include <vector>

#include "../src/lanes/q3.hpp"
#include "../src/lanes/sector_kill.hpp"
#include "../src/pipeline/generate.hpp"
#include "../src/tree/cloud_index.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--inject=", 0) == 0) inject = a.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool mutant = MHGP5_MUTANT("sector-kill-nonstrict");
  std::vector<InputPoint> in;
  const auto add = [&](i64 x, i64 y, i64 z) {
    InputPoint p;
    p.id = (PointId)in.size();
    p.position = P3{x, y + 100, z + 100};  // translation : coordonnees positives (profil u16)
    in.push_back(p);
  };
  add(0, 0, 0);    // a (id 0)
  add(50, 0, 0);   // b (id 1)
  add(25, 0, 30);  // x (id 2)
  // Sphere diametrale, demi-espace z <= 0 : plan x = 25 (y² + z² = 625) puis x = 25 ± 15 (y² + z² = 400).
  const i64 eq[][2] = {{25, 0}, {-25, 0}, {0, -25}, {7, -24}, {-7, -24}, {24, -7}, {-24, -7}, {15, -20}, {-15, -20}, {20, -15}, {-20, -15}};
  for (const auto& p : eq) add(25, p[0], p[1]);
  const i64 r20[][2] = {{20, 0}, {-20, 0}, {0, -20}, {12, -16}, {-12, -16}, {16, -12}, {-16, -12}};
  for (const auto& p : r20) { add(40, p[0], p[1]); add(10, p[0], p[1]); }
  // Plan z = 0 : (x-25)² + y² = 625, y != 0 — avec (25,±25,0), (40,±20,0), (10,±20,0) deja ajoutes,
  // neuf sites par signe de y (les secteurs voisins de ±e1 n'ont que des temoins non stricts a z = 0).
  const i64 z0[][2] = {{1, 7}, {5, 15}, {18, 24}, {32, 24}, {45, 15}, {49, 7}};
  for (const auto& p : z0) { add(p[0], p[1], 0); add(p[0], -p[1], 0); }
  const CloudIndex ix = build_cloud_index(in);
  if (!ix.valid || ix.has_duplicate_positions()) { std::printf("REFUS : index\n"); return 2; }
  // Verification directe sur le cover exact de l'ancre (a,b).
  P3 pa{0, 100, 100}, pb{50, 100, 100}, px{25, 100, 130};
  i32 ua = -1, ub = -1;
  for (i32 u = 0; u < (i32)ix.upos.size(); ++u) {
    if (ix.upos[(size_t)u].x == pa.x && ix.upos[(size_t)u].y == pa.y && ix.upos[(size_t)u].z == pa.z) ua = u;
    if (ix.upos[(size_t)u].x == pb.x && ix.upos[(size_t)u].y == pb.y && ix.upos[(size_t)u].z == pb.z) ub = u;
  }
  if (ua < 0 || ub < 0) return 2;
  const i64 D2 = p3_norm2(p3_sub(pb, pa));
  std::vector<CoverPoint> cover;
  cover_query(ix, pa, pb, D2, 3, &cover);
  u64 wmin = 0;
  const bool killed = anchor_sector_kill(cover, ix.upos, ua, ub, pa, pb, D2, 12, 9, &wmin);
  std::printf("cover=%zu temoins_min=%llu tue=%d mutant=%d\n", cover.size(), (unsigned long long)wmin, killed ? 1 : 0, mutant ? 1 : 0);
  // Production complete : la boule {a,b,x} doit etre emise (nominal).
  GenerateOptions opt;
  std::vector<BallCandidate> out;
  GenerateStats st;
  generate_candidates(ix, opt, &out, &st);
  const BallKey want = q3_ball_key(q3_form(pa, pb, px));
  bool present = false;
  for (const BallCandidate& c : out) if (c.arity == 3 && c.key == want) present = true;
  std::printf("candidats=%zu boule_abx=%d ancres_secteurs=%llu\n", out.size(), present ? 1 : 0, (unsigned long long)st.anchors_killed_sectors[1]);
  if (mutant) {
    if (!present && killed && wmin >= 9) return 4;  // mutant TUE : la boule survivante a disparu
    std::printf("MUTANT NON TUE\n");
    return 1;
  }
  if (!present || killed || wmin != 0) return 1;
  std::printf("sector_kill_fixture OK\n");
  return 0;
}
