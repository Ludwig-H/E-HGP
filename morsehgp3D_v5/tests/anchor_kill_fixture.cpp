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
//  F9 « ancre au-dessus d'une vallee » (theoreme 10.5) : toutes les cellules necessaires mortes, ancre tuee
//     sans enumerer un seed ; F10 « frontiere des sommets de la grille » (13 sites entiers de la sphere
//     diametrale, egalite exacte aux sommets i' = 0 : tue `cell-kill-nonstrict`).
//  F11 « frontieres de la LOCALISATION » (reponse des auditeurs, 28 aout 2026) : ancre a = (0,0,0),
//     b = (2000,0,0), base u = (0,0,2000), v = (0,−2000,0), cellule (i, j) = {z ∈ [250 i, 250 (i+1)],
//     y ∈ [−250 (j+1), −250 j]} ; 9 sites (1000+e, 0, 1200), e = 0..8, temoins (y-independants : cy² se
//     simplifie) de TOUTE cellule i >= 1 (centre a z > 183,4) et d'AUCUNE cellule i <= 0 : la colonne
//     i = 1 est morte a EXACTEMENT h3 = 9 temoins, la colonne i = 0 vivante. Seeds : x_corner = (750, 0, 1250),
//     centre v3 = (0, 0, 250) EXACTEMENT sur le coin (i' = 1, j' = 0) ; x_edge = (1020, −816, 1088), centre
//     (αG, βG) = (1, 3/4) EXACTEMENT sur l'arete i' = 1 de la cellule j = 0 ; x_dead = (1000, 0, 1400), centre
//     αG = 1,371 dans la cellule (1, 0) morte (tue en q3 ; sa corde q4, le long de y, ne rencontre que la
//     colonne i = 1 : tuee) ; x_chord = (1000, 300, 1400), centre αG = 1,434 (mort) mais corde q4 d'extremites
//     αG = 0,927 et 1,941 : la boite traverse la colonne i = 0 vivante. Contrat de localisation : la boite
//     consultee contient TOUTES les cellules fermees contenant le centre exact (coin : i ∈ {0, 1}, j ∈ {−1, 0} ;
//     arete : i ∈ {0, 1}, j = 0) ; le seed n'est PAS tue (conservatif) ; l'oracle ON/OFF est egal ; le
//     mutant `cell-locate-eps-zero` (marge nulle) consulte une seule colonne : tue (code 4).
// Codes : 0, 1 desaccord, 2 refus, 4 mutant tue.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "../src/lanes/q3.hpp"
#include "../src/lanes/cell_grid.hpp"
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
  const bool m_cell_ns = MHGP5_MUTANT("cell-kill-nonstrict");
  const bool m_chord = MHGP5_MUTANT("chord-nonstrict");
  const bool m_eps0 = MHGP5_MUTANT("cell-locate-eps-zero");
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
  // F9 « ancre au-dessus d'une vallee » (theoreme 10.5, grille de cellules) : vallee a fond plat (h = −600,
  // |w| >= 0,30 D), parois raides pres de a et b (aucun temoin universel W_4), surface remontant au-dela (pas 40) ;
  // la boule diametrale est pleine mais aucun secteur (apex + bord) n'a de temoin commun : les secteurs
  // q4 ne tuent pas ; toutes les cellules necessaires de la grille q4 sont mortes : l'ancre est tuee sans
  // enumerer un seed ; la production contrefactuelle (sans pretests) n'emet aucune boule q4 de cette ancre.
  {
    Fixture f;  // ancre a=(800,0,0), b=(2800,0,0) (avant translation +1000 en y, z) ; la cuvette s'etend au-dela
    f.add(800, 0, 0);
    f.add(2800, 0, 0);
    // Vallee a fond plat h = −600 pour |x − 1800| <= 900, parois de pente 6 (h = 0 en a et b), remontee au-dela.
    for (i64 x = 0; x <= 3600; x += 40)
      for (i64 y = -600; y <= 600; y += 40) {
        const i64 ax = x >= 1800 ? x - 1800 : 1800 - x;
        const i64 h = ax <= 900 ? -600 : -600 + 6 * (ax - 900);
        if ((x == 800 || x == 2800) && y == 0) continue;  // positions de a et b
        f.add(x, y, h);
      }
    const CloudIndex ix = build_cloud_index(f.in);
    expect(ix.valid && !ix.has_duplicate_positions(), "F9 : index valide");
    const P3 pa{800, 1000, 1000}, pb{2800, 1000, 1000};
    i32 ua = -1, ub = -1;
    for (i32 u = 0; u < (i32)ix.upos.size(); ++u) {
      const P3& p = ix.upos[(size_t)u];
      if (p.x == pa.x && p.y == pa.y && p.z == pa.z) ua = u;
      if (p.x == pb.x && p.y == pb.y && p.z == pb.z) ub = u;
    }
    const i64 D2 = p3_norm2(p3_sub(pb, pa));
    generate_detail::AnchorScratch sc;
    cover_query(ix, pa, pb, D2, 3, &sc.cover);
    u64 diam = 0, near_m = 0;
    for (const CoverPoint& cz : sc.cover) {
      if (cz.u == ua || cz.u == ub) continue;
      if (cz.dist2q < D2) ++diam;
      if (100 * cz.dist2q < 36 * D2) ++near_m;  // |2w|² < 0,36 D² <=> |w| < 0,30 D
    }
    u64 wmin4 = 0;
    const bool sect4 = anchor_sector_kill(sc.cover, ix.upos, ua, ub, pa, pb, D2, 8, 8, &wmin4);
    const bool w4 = anchor_universal_kill(sc.cover, ix.upos, ua, ub, pa, pb, D2, Lane::kQ4, 8);
    CellGrid g;
    const bool built = g.build(sc.cover, ix.upos, ua, ub, pa, pb, D2, 8, 8, float_filter_runtime_enabled());
    std::vector<BallCandidate> on, off;
    GenerateStats son, soff;
    const u64 h4 = lane_h(Lane::kQ4, 11);
    const bool float_on = float_filter_runtime_enabled();
    sc.affine_filled = false;
    generate_detail::process_anchor_q4(ix, sc, ua, ub, pa, pb, D2, h4, float_on, false, false, false, &on, &son, generate_detail::AnchorPretests::kApply);
    sc.affine_filled = false;
    generate_detail::process_anchor_q4(ix, sc, ua, ub, pa, pb, D2, h4, float_on, false, false, false, &off, &soff, generate_detail::AnchorPretests::kCounterfactual);
    std::printf("F9 : cover=%zu boule_diametrale=%llu pres_de_m=%llu W4=%d secteurs_q4=%d (wmin=%llu) grille : construite=%d necessaires=%u mortes=%u ; "
                "production ON : ancres_cellules=%llu seeds=%llu candidats=%zu ; OFF : seeds=%llu coeur=%llu corde=%llu candidats=%zu\n",
                sc.cover.size(), (unsigned long long)diam, (unsigned long long)near_m, w4 ? 1 : 0, sect4 ? 1 : 0, (unsigned long long)wmin4,
                built ? 1 : 0, g.needed_cells, g.dead_cells, (unsigned long long)son.anchors_killed_cells[2], (unsigned long long)son.seeds[1], on.size(),
                (unsigned long long)soff.seeds[1], (unsigned long long)soff.seeds_killed_core, (unsigned long long)soff.seeds_killed_chord, off.size());
    expect(sc.cover.size() >= kCellGridMinSites, "F9 : cover dense (politique de grille)");
    expect(diam >= 100 && near_m == 0, "F9 : boule diametrale pleine, vide a moins de 0,30 D de m");
    expect(!w4 && !sect4, "F9 : ni W_4 ni les secteurs q4 ne tuent l'ancre (apex + bord sans temoin commun)");
    for (int j = -CellGrid::G; j < CellGrid::G; ++j)
      for (int i = -CellGrid::G; i < CellGrid::G; ++i)
        if (CellGrid::cell_needed(i, j) && !g.cell_dead(i, j) && g.needed_cells - g.dead_cells <= 12)
          std::printf("  cellule vivante (%d,%d) temoins=%u\n", i, j, (unsigned)g.cnt[j + CellGrid::G][i + CellGrid::G]);
    expect(built && g.all_dead, "F9 : toutes les cellules necessaires de la grille q4 sont mortes");
    expect(son.anchors_killed_cells[2] == 1 && on.empty(), "F9 : la production tue l'ancre par la grille sans enumerer un seed");
    expect(soff.seeds[1] >= 100 && off.empty(), "F9 : contrefactuel : >= 100 seeds aigus, tous morts, aucune boule (objet inchange)");
  }
  // F11 « frontieres de la localisation » (en-tete) : centre exactement sur un coin / une arete de cellule,
  // cellule fermee adjacente vivante ; corde q4 dont la boite traverse une cellule vivante.
  {
    Fixture f;
    f.add(0, 0, 0);
    f.add(2000, 0, 0);
    for (i64 e = 0; e < 9; ++e) f.add(1000 + e, 0, 1200);  // 9 temoins de toute cellule i >= 1, d'aucune cellule i <= 0
    const P3 x_corner{750, 0, 1250}, x_edge{1020, -816, 1088}, x_dead{1000, 0, 1400}, x_chord{1000, 300, 1400};
    for (const P3& x : {x_corner, x_edge, x_dead, x_chord}) f.add(x.x, x.y, x.z);
    const CloudIndex ix = build_cloud_index(f.in);
    expect(ix.valid && !ix.has_duplicate_positions(), "F11 : index valide");
    const P3 pa{0, 1000, 1000}, pb{2000, 1000, 1000};
    i32 ua = -1, ub = -1;
    for (i32 u = 0; u < (i32)ix.upos.size(); ++u) {
      const P3& p = ix.upos[(size_t)u];
      if (p.x == pa.x && p.y == pa.y && p.z == pa.z) ua = u;
      if (p.x == pb.x && p.y == pb.y && p.z == pb.z) ub = u;
    }
    const i64 D2 = p3_norm2(p3_sub(pb, pa));
    const auto tr = [](const P3& p) { return P3{p.x, p.y + 1000, p.z + 1000}; };
    generate_detail::AnchorScratch sc;
    sc.cell_min_sites = 0;  // mode FORCE : grille sur cette (petite) ancre
    cover_query(ix, pa, pb, D2, 3, &sc.cover);
    const bool float_on = float_filter_runtime_enabled();
    CellGrid g3, g4;
    const bool b3 = g3.build(sc.cover, ix.upos, ua, ub, pa, pb, D2, 12, 9, float_on);
    const bool b4 = g4.build(sc.cover, ix.upos, ua, ub, pa, pb, D2, 8, 8, float_on);
    const int G = CellGrid::G;
    std::printf("F11 : cover=%zu grilles=%d/%d base u=(%lld,%lld,%lld) v=(%lld,%lld,%lld) temoins (1,0)=%u (1,-1)=%u (0,0)=%u (0,-1)=%u ; q4 (1,0)=%u\n",
                sc.cover.size(), b3 ? 1 : 0, b4 ? 1 : 0, (long long)g3.u[0], (long long)g3.u[1], (long long)g3.u[2], (long long)g3.v[0],
                (long long)g3.v[1], (long long)g3.v[2], g3.cnt[G][G + 1], g3.cnt[G - 1][G + 1], g3.cnt[G][G], g3.cnt[G - 1][G], g4.cnt[G][G + 1]);
    expect(b3 && b4, "F11 : grilles q3 (h = 9) et q4 (h = 8) construites");
    expect(g3.u[0] == 0 && g3.u[1] == 0 && g3.u[2] == 2000 && g3.v[0] == 0 && g3.v[1] == -2000 && g3.v[2] == 0, "F11 : base u = (0,0,2000), v = (0,-2000,0)");
    // x_corner est sur la sphere des boules centrees aux sommets i' = 1 (egalite EXACTE 8 w·p = 4|w|² − D²) :
    // temoin strict d'aucune cellule i = 1, temoin NON STRICT (mutant cell-kill-nonstrict) de toutes : 9 -> 10.
    const u32 col1_expected = m_cell_ns ? 10 : 9;
    expect(g3.cnt[G][G + 1] == col1_expected && g3.cnt[G - 1][G + 1] == col1_expected && g3.cnt[G][G] == 0 && g3.cnt[G - 1][G] == 0,
           "F11 : cellules (1,0), (1,-1) a EXACTEMENT h3 = 9 temoins stricts (10 non stricts : x_corner) ; (0,0), (0,-1) sans temoin");
    expect(g3.cell_dead(1, 0) && g3.cell_dead(1, -1) && !g3.cell_dead(0, 0) && !g3.cell_dead(0, -1) && !g3.all_dead,
           "F11 : colonne i = 1 morte, colonne i = 0 vivante, ancre vivante");
    expect(g4.cell_dead(1, 0) && !g4.cell_dead(0, 0), "F11 : grille q4 : (1,0) morte, (0,0) vivante");
    const i64 d[3] = {pb.x - pa.x, pb.y - pa.y, pb.z - pa.z};
    const Q3Form f_corner = q3_form(pa, pb, tr(x_corner)), f_edge = q3_form(pa, pb, tr(x_edge)), f_dead = q3_form(pa, pb, tr(x_dead));
    int rc[4] = {99, 99, 99, 99}, re[4] = {99, 99, 99, 99}, rd[4] = {99, 99, 99, 99}, rq[4] = {99, 99, 99, 99}, rqd[4] = {99, 99, 99, 99};
    i128 pu, pv, den;
    generate_detail::seed_center_coords(g3, f_corner, d, &pu, &pv, &den);
    const bool okc = g3.locate_box(pu, pv, den, rc);
    generate_detail::seed_center_coords(g3, f_edge, d, &pu, &pv, &den);
    const bool oke = g3.locate_box(pu, pv, den, re);
    generate_detail::seed_center_coords(g3, f_dead, d, &pu, &pv, &den);
    const bool okd = g3.locate_box(pu, pv, den, rd);
    // Cordes q4 : x_chord (boite traversant i = 0 vivante) et x_dead (colonne i = 1 seule).
    const auto chord_box = [&](const P3& x, const CellGrid& g, int r[4]) {
      const P3 px = tr(x);
      const Q3Form f3 = q3_form(pa, pb, px);
      const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
      i128 pu0, pv0, pu1, pv1, dn;
      if (!generate_detail::seed_chord_coords(g, f3, d, nrm, D2, p3_norm2(p3_sub(px, pa)), p3_norm2(p3_sub(px, pb)), &pu0, &pv0, &pu1, &pv1, &dn)) return false;
      return g.segment_box(pu0, pv0, pu1, pv1, dn, r);
    };
    const bool okq = chord_box(x_chord, g4, rq), okqd = chord_box(x_dead, g4, rqd);
    std::printf("F11 : boites consultees (i0,i1,j0,j1) : coin=(%d,%d,%d,%d) arete=(%d,%d,%d,%d) mort=(%d,%d,%d,%d) corde=(%d,%d,%d,%d) corde_morte=(%d,%d,%d,%d) (mutant eps0=%d)\n",
                rc[0], rc[1], rc[2], rc[3], re[0], re[1], re[2], re[3], rd[0], rd[1], rd[2], rd[3], rq[0], rq[1], rq[2], rq[3], rqd[0], rqd[1], rqd[2], rqd[3], m_eps0 ? 1 : 0);
    expect(okc && oke && okd && okq && okqd, "F11 : toutes les localisations dans le domaine");
    // Production par ancre, ON (grille forcee) contre OFF (contrefactuel) : meme multiensemble, grille exercee.
    const u64 h3 = lane_h(Lane::kQ3, 11), h4 = lane_h(Lane::kQ4, 11);
    std::vector<BallCandidate> on3, off3, on4, off4;
    GenerateStats son3, soff3, son4, soff4;
    sc.affine_filled = false;
    generate_detail::scan_anchor_q3(ix, sc, ua, ub, pa, pb, D2, h3, float_on, false, &on3, &son3, generate_detail::AnchorPretests::kApply);
    sc.affine_filled = false;
    generate_detail::scan_anchor_q3(ix, sc, ua, ub, pa, pb, D2, h3, float_on, false, &off3, &soff3, generate_detail::AnchorPretests::kCounterfactual);
    sc.affine_filled = false;
    generate_detail::process_anchor_q4(ix, sc, ua, ub, pa, pb, D2, h4, float_on, false, false, false, &on4, &son4, generate_detail::AnchorPretests::kApply);
    sc.affine_filled = false;
    generate_detail::process_anchor_q4(ix, sc, ua, ub, pa, pb, D2, h4, float_on, false, false, false, &off4, &soff4, generate_detail::AnchorPretests::kCounterfactual);
    const auto same = [](std::vector<BallCandidate> a, std::vector<BallCandidate> b) {
      rle_candidates(&a, 1);
      rle_candidates(&b, 1);
      if (a.size() != b.size()) return false;
      for (size_t i = 0; i < a.size(); ++i) if (!(a[i].key == b[i].key) || !(a[i].level == b[i].level)) return false;
      return true;
    };
    std::printf("F11 : q3 ON seeds=%llu tues_cellules=%llu candidats=%zu / OFF candidats=%zu ; q4 ON seeds=%llu tues_cellules=%llu candidats=%zu / OFF candidats=%zu ; grilles tentees=%llu/%llu construites=%llu/%llu\n",
                (unsigned long long)son3.seeds[0], (unsigned long long)son3.seeds_killed_cells[1], on3.size(), off3.size(), (unsigned long long)son4.seeds[1],
                (unsigned long long)son4.seeds_killed_cells[2], on4.size(), off4.size(), (unsigned long long)son3.grids_attempted[1], (unsigned long long)son4.grids_attempted[2],
                (unsigned long long)son3.grids_built[1], (unsigned long long)son4.grids_built[2]);
    expect(same(on3, off3) && same(on4, off4), "F11 : oracle ON/OFF egal en q3 et en q4");
    expect(son3.grids_attempted[1] == 1 && son3.grids_built[1] == 1 && son4.grids_attempted[2] == 1 && son4.grids_built[2] == 1 &&
           son3.anchors_killed_cells[1] == 0 && son4.anchors_killed_cells[2] == 0, "F11 : mode force : une grille par lane, ancre vivante");
    expect(son3.seeds_killed_cells[1] >= 1 && son4.seeds_killed_cells[2] >= 1 && !off3.empty(),
           "F11 : non-vacuite : >= 1 seed tue par cellules en q3 (x_dead, x_chord) et en q4 (x_dead), boules q3 emises");
    expect(generate_detail::seed_center_cell_dead(g3, f_dead, d) && rd[0] == 1 && rd[1] == 1, "F11 : x_dead : centre dans la cellule (1,0) morte, seed q3 tue");
    expect(chord_box(x_dead, g4, rqd) && rqd[0] == 1 && rqd[1] == 1, "F11 : x_dead : corde q4 dans la colonne i = 1 seule (tuee)");
    if (m_eps0) {
      // Marge nulle : le centre exactement sur l'arete i' = 1 n'est plus localise dans ses DEUX cellules fermees
      // (une seule colonne consultee, selon l'arrondi) — contrat de localisation viole.
      const bool killed = (rc[0] == rc[1]) || (re[0] == re[1]);
      if (killed && !failures) return 4;
      std::printf("MUTANT NON TUE (cell-locate-eps-zero : coin i∈[%d,%d], arete i∈[%d,%d], echecs %d)\n", rc[0], rc[1], re[0], re[1], failures);
      return 1;
    }
    expect(rc[0] == 0 && rc[1] == 1 && rc[2] == -1 && rc[3] == 0, "F11 : coin (1,0) exact : les quatre cellules fermees (0..1) x (-1..0) sont consultees");
    expect(re[0] == 0 && re[1] == 1 && re[2] == 0 && re[3] == 0, "F11 : arete i' = 1 exacte (βG = 3/4) : les deux cellules fermees (0,0) et (1,0) sont consultees");
    expect(!generate_detail::seed_center_cell_dead(g3, f_corner, d) && !generate_detail::seed_center_cell_dead(g3, f_edge, d),
           "F11 : coin et arete : une cellule fermee consultee est vivante, seed NON tue (conservatif)");
    expect(rq[0] == 0 && rq[1] == 1 && !generate_detail::seed_chord_cell_dead(g4, q3_form(pa, pb, tr(x_chord)), d, p3_cross(p3_sub(pb, pa), p3_sub(tr(x_chord), pa)), D2,
                                                                          p3_norm2(p3_sub(tr(x_chord), pa)), p3_norm2(p3_sub(tr(x_chord), pb))),
           "F11 : corde q4 de x_chord : boite i ∈ [0,1] traversant la colonne vivante, seed NON tue");
  }
  // F10 « frontiere des sommets de la grille » (primitive cell_grid.hpp) : ancre a=(0,0,0), b=(2000,0,0) ;
  // base u=(0,0,2000), v=(0,−2000,0), sommets p=(0,−250j',250i') ; 13 sites entiers de la sphere diametrale
  // dans le plan y=0, z>0 (s²+t²=10⁶) : 4w'·p = G(|w'|²−D²) = 0 EXACTEMENT aux sommets i'=0 ; strict : aucun
  // n'est temoin de la colonne i=0 (ni des colonnes i<0) ; mutant `cell-kill-nonstrict` : tous les 13 le sont.
  {
    Fixture f;
    f.add(0, 0, 0);
    f.add(2000, 0, 0);
    const i64 st[][2] = {{600, 800}, {-600, 800}, {800, 600}, {-800, 600}, {280, 960}, {-280, 960}, {960, 280},
                         {-960, 280}, {352, 936}, {-352, 936}, {936, 352}, {-936, 352}, {0, 1000}};
    for (const auto& q : st) f.add(1000 + q[0], 0, q[1]);
    const CloudIndex ix = build_cloud_index(f.in);
    expect(ix.valid && !ix.has_duplicate_positions(), "F10 : index valide");
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
    u64 on_sphere = 0;
    for (const CoverPoint& cz : cover) if (cz.u != ua && cz.u != ub && cz.dist2q == D2) ++on_sphere;
    CellGrid g;
    const bool built = g.build(cover, ix.upos, ua, ub, pa, pb, D2, 8, 8, float_filter_runtime_enabled());
    u32 col0_min = 255, col0_max = 0, col1_min = 255, colm1_max = 0;
    for (int j = -CellGrid::G; j < CellGrid::G; ++j) {
      col0_min = std::min<u32>(col0_min, g.cnt[j + CellGrid::G][CellGrid::G]);
      col0_max = std::max<u32>(col0_max, g.cnt[j + CellGrid::G][CellGrid::G]);
      col1_min = std::min<u32>(col1_min, g.cnt[j + CellGrid::G][CellGrid::G + 1]);
      colm1_max = std::max<u32>(colm1_max, g.cnt[j + CellGrid::G][CellGrid::G - 1]);
    }
    std::printf("F10 : sites_sur_sphere=%llu base u=(%lld,%lld,%lld) v=(%lld,%lld,%lld) colonne i=0 : temoins [%u,%u] ; i=1 : min %u ; i=−1 : max %u (mutant cell-ns=%d)\n",
                (unsigned long long)on_sphere, (long long)g.u[0], (long long)g.u[1], (long long)g.u[2], (long long)g.v[0], (long long)g.v[1], (long long)g.v[2],
                col0_min, col0_max, col1_min, colm1_max, m_cell_ns ? 1 : 0);
    expect(built && on_sphere == 13, "F10 : grille construite, 13 sites sur la sphere diametrale");
    expect(col1_min == 13 && colm1_max == 0, "F10 : colonne i=1 (sommets stricts) : 13 temoins ; colonne i=−1 : aucun");
    if (m_cell_ns) {
      if (col0_min == 13 && !failures) return 4;
      std::printf("MUTANT NON TUE (cell-kill-nonstrict : colonne i=0 min %u)\n", col0_min);
      return 1;
    }
    expect(col0_max == 0, "F10 : la frontiere exacte des sommets i'=0 n'est temoin d'aucune cellule (strict)");
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
