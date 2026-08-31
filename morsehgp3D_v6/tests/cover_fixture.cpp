// MorseHGP3D v6 — porte du COEFFICIENT DE COVER q4 (P0 audit du 31 aout).
//
// Contre-fixture exacte de l'auditeur : tetraedre regulier a=(110,110,110),
// b=(110,90,90), x=(90,110,90), y=(90,90,110) (D2=800) et z=(83,100,100)
// strictement interieur a sa circumboule (BallKey.power(z) = -11), mais
// |2z-a-b|² = 2916 : HORS du cover coefficient 3 (3D² = 2400), DANS le
// cover coefficient 4 (4D² = 3200). A smax=4 (h4=1), le filtre de profondeur
// de generation doit donc tuer la boule AVANT emission avec le cover 4 ;
// le defaut herite v5 (coefficient 3, mutant `q4-cover-coef3`) l'emet —
// fail-open masque par le prefiltre aval, visible seulement sur le
// multiensemble brut. La porte juge la FRONTIERE DE GENERATION, pas l'objet.
//
// Codes : 0 conforme ; 1 desaccord ; 2 refus ; 3 mutant non tue ; 4 mutant tue.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/pipeline/expand.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp6;

int main(int argc, char** argv) {
  const char* inject = nullptr;
  for (int i = 1; i < argc; ++i) {
    if (std::strncmp(argv[i], "--inject=", 9) == 0) inject = argv[i] + 9;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", argv[i]);
      return 2;
    }
  }
  if (inject && !mutants_enable(inject)) {
    std::fprintf(stderr, "mutant inconnu : %s\n", inject);
    return 2;
  }
  const P3 pts[5] = {{110, 110, 110}, {110, 90, 90}, {90, 110, 90}, {90, 90, 110}, {83, 100, 100}};
  std::vector<InputPoint> in(5);
  for (int i = 0; i < 5; ++i) in[(size_t)i] = InputPoint{(PointId)i, pts[i]};
  const CloudIndex ix = build_cloud_index(in);
  if (!ix.valid) return 2;
  // La cle fautive : circumboule du tetraedre regulier (a,b,x,y).
  const Q4Form f4 = q4_form(pts[0], pts[1], pts[2], pts[3]);
  if (f4.det == 0 || !q4_center_strictly_inside(f4, pts[0], pts[1], pts[2], pts[3])) {
    std::fprintf(stderr, "fixture invalide : tetraedre non bien centre\n");
    return 2;
  }
  const BallKey faulty = ball_key_reduce(q4_ball_form(f4));
  // Verifications litterales de la contre-fixture (gravees) :
  if (faulty.power(pts[4]) != -11) {
    std::fprintf(stderr, "fixture invalide : power(z) = attendu -11\n");
    return 2;
  }
  {
    const i64 D2 = p3_norm2(p3_sub(pts[1], pts[0]));
    const P3 u{2 * pts[4].x - pts[0].x - pts[1].x, 2 * pts[4].y - pts[0].y - pts[1].y,
               2 * pts[4].z - pts[0].z - pts[1].z};
    const i64 U = p3_norm2(u);
    if (D2 != 800 || U != 2916 || !(U > 3 * D2) || !(U <= 4 * D2)) {
      std::fprintf(stderr, "fixture invalide : D2=%lld U=%lld\n", (long long)D2, (long long)U);
      return 2;
    }
  }
  // Generation seule, un fil, smax=4 (h4=1) : compter les occurrences BRUTES.
  GenerateOptions go;
  go.smax = 4;
  go.threads = 1;
  std::vector<BallCandidate> cands;
  GenerateStats gs;
  generate_candidates(ix, go, &cands, &gs);
  u64 raw = 0;
  for (const BallCandidate& c : cands)
    if (c.key == faulty) ++raw;
  // Aval : RLE puis prefiltre — la cle ne survit jamais (profondeur 1 >= h4).
  sort_candidates(&cands, 1);
  deduplicate_candidates(&cands);
  u64 rle = 0;
  for (const BallCandidate& c : cands)
    if (c.key == faulty) ++rle;
  std::vector<Survivor> surv;
  ExpandStats es;
  prefilter_balls(ix, cands, 4, 1, &surv, &es);
  u64 alive = 0;
  for (const Survivor& s : surv)
    if (cands[s.idx].key == faulty) ++alive;
  std::fprintf(stderr, "cover_fixture raw=%llu rle=%llu survivantes=%llu\n", (unsigned long long)raw,
               (unsigned long long)rle, (unsigned long long)alive);
  if (alive != 0) return 1;  // l'objet perdrait une mort : jamais attendu
  if (inject && std::strcmp(inject, "q4-cover-coef3") == 0) {
    // Le defaut v5 : la boule fautive est emise (z hors du cover 3) puis
    // masquee par le prefiltre — exactement une occurrence brute et post-RLE.
    if (raw == 1 && rle == 1) {
      std::fprintf(stderr, "mutant q4-cover-coef3 : boule fautive emise puis masquee — tue\n");
      return 4;
    }
    return 3;
  }
  // Nominal (coefficient 4) : z est compte, la profondeur atteint h4=1, la
  // boule meurt A LA GENERATION : zero occurrence brute.
  if (raw != 0 || rle != 0) {
    std::fprintf(stderr, "cover 4 : la boule fautive est encore emise\n");
    return 1;
  }
  std::printf("cover_fixture : coefficient 4 tue a la generation ; frontiere conforme\n");
  return 0;
}
