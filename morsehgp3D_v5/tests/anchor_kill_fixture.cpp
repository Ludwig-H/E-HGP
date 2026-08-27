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
//  F4 « frontiere W_3 a h−1 » : 8 sites (1000+e, 550, 0) (|W_3| = 8 = h3 − 1) et
//     le seed x = (1000, 1200, 0) : les 8 sites sont interieurs a sa boule
//     (profondeur 8 < 9) ⟹ boule emise ; W_3 ne tue pas (8 < 9), les secteurs
//     non plus (wmin = 0) ; le mutant `anchor-kill-h-minus-one` tue PAR W_3
//     seul (secteurs a 8 : wmin = 0 < 8) : la frontiere W_3 est qualifiee
//     separement des secteurs.
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
#include "../src/lanes/chord_kill.hpp"
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
  const bool m_ns = MHGP5_MUTANT("sector-kill-nonstrict");
  const bool m_chord = MHGP5_MUTANT("chord-nonstrict");
  bool f5_killed_by_mutant = false, f6_boundary_counted = false;
  // F1
  Fixture f1;
  f1.add(0, 0, 0); f1.add(2000, 0, 0);
  for (i64 e = 0; e < 9; ++e) { f1.add(1000 + e, 900, 0); f1.sites.push_back(P3{1000 + e, 900, 0}); f1.add(1000 + e, -900, 0); f1.sites.push_back(P3{1000 + e, -900, 0}); }
  f1.add(1000, 1200, 0); f1.sites.push_back(P3{1000, 1200, 0});  // seed aigu (profondeur 9 : les 9 z⁺) — fixture non vacue cote produit
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
  // F4
  Fixture f4;
  f4.add(0, 0, 0); f4.add(2000, 0, 0);
  for (i64 e = 0; e < 8; ++e) f4.add(1000 + e, 550, 0);
  f4.add(1000, 1200, 0); f4.sites.push_back(P3{1000, 1200, 0});
  const Verdict v4 = run(f4);
  std::printf("F4 : boules=%zu W3=%llu wmin=%llu W3-tue=%d secteurs-tue=%d cumule=%d mutant=%d\n", v4.balls, (unsigned long long)v4.w3,
              (unsigned long long)v4.wmin, v4.dead_by_w3, v4.dead_by_sectors, v4.kill, m_h1 ? 1 : 0);
  if (m_h1) {
    // Mutant : F2 est tuee a h−1 = 8 par les SECTEURS (wmin = 8) et F4 par W_3 SEUL (|W_3| = 8, wmin = 0) ;
    // dans les deux cas le seed x survivant perd sa boule -> code 4 seulement si les DEUX frontieres sont exercees.
    const bool f2_killed = v2.kill == 2 && v2.balls == 0;
    const bool f4_killed = v4.kill == 1 && !v4.dead_by_sectors && v4.balls == 0;
    if (f2_killed && f4_killed) return 4;
    std::printf("MUTANT NON TUE (F2 %d, F4 %d)\n", f2_killed ? 1 : 0, f4_killed ? 1 : 0);
    return 1;
  }
  expect(v4.balls == 1 && v4.w3 == 8 && v4.wmin == 0 && v4.kill == 0, "F4 : |W_3| = 8, secteurs 0, ancre vivante, boule {a,b,x} emise");
  expect(v2.balls == 1, "F2 : la production emet la boule {a,b,x} (profondeur 8 < 9)");
  expect(v2.kill == 0 && v2.wmin == 8, "F2 : ancre vivante, wmin = 8 = h3 − 1");
  // F3
  Fixture f3;
  f3.add(0, 0, 0); f3.add(2000, 0, 0);
  for (i64 e = 0; e < 9; ++e) { f3.add(1000 + e, 550, 0); f3.sites.push_back(P3{1000 + e, 550, 0}); }
  f3.add(1000, 1200, 0); f3.sites.push_back(P3{1000, 1200, 0});  // seed aigu (profondeur 9) — non vacue
  const Verdict v3 = run(f3);
  std::printf("F3 : boules=%zu W3=%llu wmin=%llu W3-tue=%d secteurs-tue=%d cumule=%d\n", v3.balls, (unsigned long long)v3.w3,
              (unsigned long long)v3.wmin, v3.dead_by_w3, v3.dead_by_sectors, v3.kill);
  expect(v3.w3 == 9 && v3.dead_by_w3, "F3 : W_3 exact tue (9 temoins universels)");
  expect(!v3.dead_by_sectors, "F3 : le test par secteurs NE tue PAS (sommets du polygone hors du disque) : incomparables");
  expect(v3.balls == 0, "F3 : aucune boule emise (tout seed mort)");
  expect(v3.kill == 1, "F3 : le cumul tue par W_3");
  // F5 « exemple 2.4 » : 28 sites EXACTEMENT sur la sphere diametrale (q = 0) plus le seed x = (1000, 1200, 0) :
  // depth(0) = 0, |W_3| = 0, min sur le disque ferme = 0 — AUCUN test sur le disque ne tue — et pourtant tout seed
  // est mort (x : >= 13 sites interieurs, ceux a y > 0) : les tests d'ancre ne sont jamais necessaires.
  {
    Fixture f5;
    f5.add(0, 0, 0); f5.add(2000, 0, 0);
    const i64 sph[][2] = {{600, 800}, {800, 600}, {280, 960}, {960, 280}, {352, 936}, {936, 352}, {1000, 0}};
    for (const auto& p : sph) {  // (1000,0) : deux points seulement (±1000, 0), pas quatre
      f5.add(1000, p[0], p[1]); f5.add(1000, -p[0], p[1]);
      if (p[1] != 0) { f5.add(1000, p[0], -p[1]); f5.add(1000, -p[0], -p[1]); }
    }
    f5.add(1000, 1200, 0); f5.sites.push_back(P3{1000, 1200, 0});
    const Verdict v5 = run(f5);
    std::printf("F5 : boules=%zu W3=%llu wmin=%llu cumule=%d\n", v5.balls, (unsigned long long)v5.w3, (unsigned long long)v5.wmin, v5.kill);
    expect(v5.balls == 0, "F5 : tout seed mort (aucune boule emise)");
    if (!m_ns) expect(v5.w3 == 0 && v5.wmin == 0 && v5.kill == 0, "F5 : aucun test d'ancre ne tue (necessite refutee)");
    else f5_killed_by_mutant = v5.kill != 0;  // non strict : les 28 sites de la sphere deviennent temoins
  }
  // F6 « frontiere des demi-plans seule » (primitive) : site S = (1000, 400, −200) strictement dans la boule
  // diametrale (|2w|² = 800000 < D²) et EXACTEMENT sur la frontiere du demi-plan du sommet u = (0,0,2000) :
  // 4 w2·u = −3 200 000 = |2w|² − D². Strict : S n'est pas temoin du secteur (u−v, u) ; non strict : il l'est.
  {
    const CloudIndex ix6 = build_cloud_index([] { Fixture f; f.add(0, 0, 0); f.add(2000, 0, 0); f.add(1000, 400, -200); return f.in; }());
    const P3 pa{0, 1000, 1000}, pb{2000, 1000, 1000};
    i32 ua = -1, ub = -1;
    for (i32 u = 0; u < (i32)ix6.upos.size(); ++u) {
      if (ix6.upos[(size_t)u].x == 0) ua = u;
      if (ix6.upos[(size_t)u].x == 2000) ub = u;
    }
    std::vector<CoverPoint> cover;
    cover_query(ix6, pa, pb, 4000000, 3, &cover);
    u64 wmin = 0;
    u32 cnt[8];
    anchor_sector_kill(cover, ix6.upos, ua, ub, pa, pb, 4000000, 12, 1, &wmin, cnt);
    std::printf("F6 : secteurs strict = %u %u %u %u %u %u %u %u\n", cnt[0], cnt[1], cnt[2], cnt[3], cnt[4], cnt[5], cnt[6], cnt[7]);
    if (!m_ns) expect(cnt[7] == 0 && cnt[6] == 1, "F6 : frontiere du demi-plan (sommet u) : temoin strict du secteur 6 seulement");
    else f6_boundary_counted = cnt[7] == 1;  // le mutant non strict compte la frontiere du demi-plan comme temoin
  }
  // F7 « secteurs q4 non vacus » : la configuration F1 (avec x) tuee aussi par les secteurs q4 (rho² = D²/8, h4 = 8) ?
  {
    const CloudIndex ix7 = build_cloud_index(f1.in);
    const P3 pa{0, 1000, 1000}, pb{2000, 1000, 1000};
    i32 ua = -1, ub = -1;
    for (i32 u = 0; u < (i32)ix7.upos.size(); ++u) {
      if (ix7.upos[(size_t)u].x == 0 && ix7.upos[(size_t)u].y == 1000) ua = u;
      if (ix7.upos[(size_t)u].x == 2000) ub = u;
    }
    std::vector<CoverPoint> cover;
    cover_query(ix7, pa, pb, 4000000, 3, &cover);
    u64 wmin = 0;
    const bool k4 = anchor_sector_kill(cover, ix7.upos, ua, ub, pa, pb, 4000000, 8, 8, &wmin);
    std::printf("F7 : secteurs q4 sur F1 : wmin=%llu tue=%d\n", (unsigned long long)wmin, k4 ? 1 : 0);
    expect(k4, "F7 : le test par secteurs q4 tue l'ancre de F1 (non-vacuite q4)");
  }
  // F8 « frontiere des morceaux de corde » (primitive chord_kill.hpp) : un site coplanaire (B = 0) et cospherique
  // (L = 0) a v_j = L − c_j μ̂ B = 0 a TOUS les sommets ; strict : temoin d'aucun morceau ; non strict : de tous.
  {
    ChordPieces ch;
    ch.init(2, m_chord);  // J = 2 -> μ̂ = 2
    ch.update(0.0, 1.0, 0, []() { return (i128)0; });  // lh = 0, E = 1 : indecidable en flottant -> repli exact, L = 0
    std::printf("F8 : morceaux = %u %u %u %u (mutant chord=%d)\n", ch.cnt[0], ch.cnt[1], ch.cnt[2], ch.cnt[3], m_chord ? 1 : 0);
    if (m_chord) {
      if (ch.cnt[0] == 1 && ch.cnt[1] == 1 && ch.cnt[2] == 1 && ch.cnt[3] == 1 && !failures) return 4;
      std::printf("MUTANT NON TUE (chord)\n");
      return 1;
    }
    expect(ch.cnt[0] == 0 && ch.cnt[1] == 0 && ch.cnt[2] == 0 && ch.cnt[3] == 0, "F8 : la frontiere v_j = 0 n'est pas un temoin (strict)");
    // Un site franchement interieur (L = −16, B = 0) est temoin de tous les morceaux.
    ChordPieces ch2;
    ch2.init(2, false);
    ch2.update(-16.0, 1.0, 0, []() { return (i128)-16; });
    expect(ch2.cnt[0] == 1 && ch2.cnt[1] == 1 && ch2.cnt[2] == 1 && ch2.cnt[3] == 1, "F8 : un site interieur a toute la corde est temoin de tous les morceaux");
  }
  if (m_ns) {
    // Mutant non strict : F5 (frontiere diametrale, 28 sites a q = 0) tuee a tort ET F6 (frontiere des demi-plans)
    // comptee — les deux frontieres sont exercees separement.
    if (f5_killed_by_mutant && f6_boundary_counted && !failures) return 4;
    std::printf("MUTANT NON TUE (F5 %d, F6 %d, echecs %d)\n", f5_killed_by_mutant ? 1 : 0, f6_boundary_counted ? 1 : 0, failures);
    return 1;
  }
  if (failures) return 1;
  std::printf("anchor_kill_fixture OK\n");
  return 0;
}
