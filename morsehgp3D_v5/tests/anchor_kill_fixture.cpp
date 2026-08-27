// MorseHGP3D v5 — FIXTURES GRAVEES des tests d'ancre (src/lanes/sector_kill.hpp),
// issues de l'analyse du 27 aout 2026 (rapport mathematique et contradicteur).
// Ancre a = (0,0,0), b = (2000,0,0) (D = 2000, m = (1000,0,0), h3 = 9), coordonnees
// translatees de +1000 en y (profil u16).
//  F1 « exact tue, W_3 ne tue pas » : 9 + 9 sites (1000+e, ±900, 0), e = 0..8 :
//     q = −760000 + 4e² < 0 mais 3q² < 4|d×u|² ⟹ |W_3| = 0 ; profondeur 9 hors
//     de la bande |v_y| < 105,5 ⟹ tout seed mort : la production n'emet
//     AUCUNE boule {a,b,z} ; le test par secteurs tue l'ancre (wmin >= 9).
//  F2 « mort a h−1 » : 8 + 8 sites (e = 0..7, tous dans la boule diametrale :
//     ce ne sont pas des seeds) plus le seed x = (1000, 1200, 0) (aigu, ab arete
//     la plus longue), de centre v = (0, 183,3, 0) : les 8 sites z⁺ (v_y >
//     −105,5) sont interieurs, les 8 sites z⁻ (v_y < 105,5) ne le sont pas :
//     profondeur 8 < 9, la production EMET la boule {a,b,x} ; W_3 = 0 et
//     secteurs wmin = 8 < 9 : ancre vivante. Le mutant
//     `anchor-kill-h-minus-one` tue l'ancre a 8 : la boule disparait, digest
//     different (code 4).
//  F3 « W_3 tue, secteurs ne tuent pas » : 9 sites (1000+e, 550, 0) : u =
//     (2e, 1100, 0), −q = 2 790 000 > 4·1100·D/√12 ⟹ 9 temoins universels
//     (in_spindle) : W_3 tue ; le polygone circonscrit a des sommets HORS du
//     disque et le test par secteurs rend wmin = 0 : les deux tests sont
//     incomparables et doivent etre CUMULES.
// Codes : 0, 1 desaccord, 2 refus, 4 mutant tue.
#include <cstdio>
#include <string>
#include <vector>

#include "../src/lanes/q3.hpp"
#include "../src/lanes/sector_kill.hpp"
#include "../src/pipeline/generate.hpp"
#include "../src/tree/cloud_index.hpp"

using namespace mhgp5;

namespace {
int failures = 0;
void expect(bool ok, const char* what) {
  if (!ok) { std::printf("ECHEC : %s\n", what); ++failures; } else std::printf("ok : %s\n", what);
}
struct Fixture {
  std::vector<InputPoint> in;
  std::vector<P3> sites;
  void add(i64 x, i64 y, i64 z) {
    InputPoint p;
    p.id = (PointId)in.size();
    p.position = P3{x, y + 1000, z + 1000};
    in.push_back(p);
  }
};
// Rend : nombre de boules {a,b,z} emises (z parmi les sites), |W_3|, wmin des secteurs, verdict cumule.
struct Verdict {
  size_t balls = 0;
  u64 w3 = 0, wmin = 0;
  int kill = 0;
  bool dead_by_w3 = false, dead_by_sectors = false;
};
Verdict run(const Fixture& f, bool expect_index = true) {
  Verdict v;
  const CloudIndex ix = build_cloud_index(f.in);
  if (!ix.valid || ix.has_duplicate_positions()) { if (expect_index) std::printf("REFUS index\n"); v.kill = -1; return v; }
  const P3 pa{0, 1000, 1000}, pb{2000, 1000, 1000};
  i32 ua = -1, ub = -1;
  for (i32 u = 0; u < (i32)ix.upos.size(); ++u) {
    const P3& p = ix.upos[(size_t)u];
    if (p.x == pa.x && p.y == pa.y && p.z == pa.z) ua = u;
    if (p.x == pb.x && p.y == pb.y && p.z == pb.z) ub = u;
  }
  const i64 D2 = p3_norm2(p3_sub(pb, pa));
  std::vector<CoverPoint> cover;
  cover_query(ix, pa, pb, D2, 3, &cover);
  // Tri stable en classes radiales comme anchor_cover_from_handles (cover_query trie par dist2q : compatible).
  for (const CoverPoint& cz : cover)
    if (cz.u != ua && cz.u != ub && in_spindle(Lane::kQ3, pa, pb, ix.upos[(size_t)cz.u])) ++v.w3;
  v.dead_by_w3 = anchor_universal_kill(cover, ix.upos, ua, ub, pa, pb, D2, Lane::kQ3, 9);
  v.dead_by_sectors = anchor_sector_kill(cover, ix.upos, ua, ub, pa, pb, D2, 12, 9, &v.wmin);
  v.kill = anchor_kill_cumulated(cover, ix.upos, ua, ub, pa, pb, D2, Lane::kQ3, 12, 9);
  GenerateOptions opt;
  std::vector<BallCandidate> out;
  GenerateStats st;
  generate_candidates(ix, opt, &out, &st);
  std::printf("  production : rect_alive=%llu ancres=%llu tuees_hist=%llu w3=%llu secteurs=%llu seeds=%llu tues=%llu candidats_q3=%llu (q2=%llu q4=%llu)\n",
              (unsigned long long)st.rect_alive[1], (unsigned long long)st.anchors[1], (unsigned long long)st.anchors_killed_hist[1],
              (unsigned long long)st.anchors_killed_w3, (unsigned long long)st.anchors_killed_sectors[1], (unsigned long long)st.seeds[0],
              (unsigned long long)st.depth_killed[1], (unsigned long long)st.candidates[1], (unsigned long long)st.candidates[0],
              (unsigned long long)st.candidates[2]);
  for (const P3& z : f.sites) {
    const P3 pz{z.x, z.y + 1000, z.z + 1000};
    const BallKey want = q3_ball_key(q3_form(pa, pb, pz));
    for (const BallCandidate& c : out)
      if (c.arity == 3 && c.key == want) { ++v.balls; break; }
  }
  return v;
}
}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--inject=", 0) == 0) inject = a.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool m_h1 = MHGP5_MUTANT("anchor-kill-h-minus-one");
  // F1
  Fixture f1;
  f1.add(0, 0, 0); f1.add(2000, 0, 0);
  for (i64 e = 0; e < 9; ++e) { f1.add(1000 + e, 900, 0); f1.sites.push_back(P3{1000 + e, 900, 0}); f1.add(1000 + e, -900, 0); f1.sites.push_back(P3{1000 + e, -900, 0}); }
  const Verdict v1 = run(f1);
  std::printf("F1 : boules=%zu W3=%llu wmin=%llu W3-tue=%d secteurs-tue=%d cumule=%d\n", v1.balls, (unsigned long long)v1.w3,
              (unsigned long long)v1.wmin, v1.dead_by_w3, v1.dead_by_sectors, v1.kill);
  expect(v1.balls == 0, "F1 : aucune boule {a,b,z} emise par la production (tout seed mort)");
  expect(v1.w3 == 0, "F1 : |W_3| = 0 (le test uniforme ne tue pas)");
  expect(v1.dead_by_sectors, "F1 : le test par secteurs tue l'ancre");
  // F2
  Fixture f2;
  f2.add(0, 0, 0); f2.add(2000, 0, 0);
  for (i64 e = 0; e < 8; ++e) { f2.add(1000 + e, 900, 0); f2.add(1000 + e, -900, 0); }
  f2.add(1000, 1200, 0); f2.sites.push_back(P3{1000, 1200, 0});  // le seed x
  const Verdict v2 = run(f2);
  std::printf("F2 : boules=%zu W3=%llu wmin=%llu W3-tue=%d secteurs-tue=%d cumule=%d mutant=%d\n", v2.balls, (unsigned long long)v2.w3,
              (unsigned long long)v2.wmin, v2.dead_by_w3, v2.dead_by_sectors, v2.kill, m_h1 ? 1 : 0);
  if (m_h1) {
    // Mutant : l'ancre est tuee a h−1 = 8 alors que le seed x survit -> boule {a,b,x} perdue.
    if (v2.kill != 0 && v2.balls == 0) return 4;
    std::printf("MUTANT NON TUE\n");
    return 1;
  }
  expect(v2.balls == 1, "F2 : la production emet la boule {a,b,x} (profondeur 8 < 9)");
  expect(v2.kill == 0 && v2.wmin == 8, "F2 : ancre vivante, wmin = 8 = h3 − 1");
  // F3
  Fixture f3;
  f3.add(0, 0, 0); f3.add(2000, 0, 0);
  for (i64 e = 0; e < 9; ++e) { f3.add(1000 + e, 550, 0); f3.sites.push_back(P3{1000 + e, 550, 0}); }
  const Verdict v3 = run(f3);
  std::printf("F3 : boules=%zu W3=%llu wmin=%llu W3-tue=%d secteurs-tue=%d cumule=%d\n", v3.balls, (unsigned long long)v3.w3,
              (unsigned long long)v3.wmin, v3.dead_by_w3, v3.dead_by_sectors, v3.kill);
  expect(v3.w3 == 9 && v3.dead_by_w3, "F3 : W_3 exact tue (9 temoins universels)");
  expect(!v3.dead_by_sectors, "F3 : le test par secteurs NE tue PAS (sommets du polygone hors du disque) : incomparables");
  expect(v3.balls == 0, "F3 : aucune boule emise (tout seed mort)");
  expect(v3.kill == 1, "F3 : le cumul tue par W_3");
  if (failures) return 1;
  std::printf("anchor_kill_fixture OK\n");
  return 0;
}
