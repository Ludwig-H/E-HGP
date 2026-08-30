// MorseHGP3D v5 — FIXTURE CROISEE du credit d'extremite par secteur
// (src/lanes/sector_kill.hpp, docs/CREDIT_SECTEUR.md).
//
// Configuration fournie par l'audit du 30 aout 2026
// (audits/QUESTION_CLAUDE_Q4_APRES_Q3_20260830.md) et gravee ici :
//
//   a = (0,1000,1000)   b = (2000,1000,1000)   D² = 4 000 000   h = 2
//   e = (10,990,990)    i = (10,910,910)       o = (10,1020,1020)
//   A = {a,e,i}   B = {b}   base = 1 (e est le seul credit W3 universel)
//   cnt     = [1,2,2,2,2,1,1,1]
//   cnt_hors = [1,0,0,0,0,1,1,1]
//
// Chaque secteur atteint h = 2 par l'UNE des deux branches, mais jamais la
// meme : les deux minima GLOBAUX valent 1. La forme
//   min_k max( cnt[k], cnt_hors[k] + base )     tue      (= 2)
//   max( min_k cnt[k], min_k (cnt_hors[k]+base) ) ne tue pas (= 1)
// C'est la contradiction minimale qui distingue le maximum pris SECTEUR PAR
// SECTEUR du maximum pris globalement — c'est-a-dire le contrat annonce de sa
// contrefacon. Mutant `sector-credit-global` : retablit la forme globale.
//
// Codes : 0 nominal, 1 desaccord, 2 refus avant calcul, 4 mutant tue.
#include <cstdio>
#include <string>
#include <vector>

#include "../src/lanes/sector_kill.hpp"
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
  const bool mutant = MHGP5_MUTANT("sector-credit-global");
  const P3 pa{0, 1000, 1000}, pb{2000, 1000, 1000};
  const P3 pe{10, 990, 990}, pi{10, 910, 910}, po{10, 1020, 1020};
  std::vector<InputPoint> in;
  const auto add = [&](const P3& p) { in.push_back(InputPoint{(PointId)in.size(), p}); };
  add(pa); add(pb); add(pe); add(pi); add(po);
  const CloudIndex ix = build_cloud_index(in);
  if (!ix.valid || ix.has_duplicate_positions()) { std::printf("REFUS : index\n"); return 2; }
  const auto find = [&](const P3& p) {
    for (i32 u = 0; u < (i32)ix.upos.size(); ++u) {
      const P3& q = ix.upos[(size_t)u];
      if (q.x == p.x && q.y == p.y && q.z == p.z) return u;
    }
    return (i32)-1;
  };
  const i32 ua = find(pa), ub = find(pb), ue = find(pe), ui = find(pi), uo = find(po);
  if (ua < 0 || ub < 0 || ue < 0 || ui < 0 || uo < 0) return 2;
  const i64 D2 = p3_norm2(p3_sub(pb, pa));
  // Boite A = plage d'indices contenant a, e, i et EXCLUANT o ; boite B = {b}.
  const i32 af = std::min(ua, std::min(ue, ui)), al = std::max(ua, std::max(ue, ui));
  if (uo >= af && uo <= al) { std::printf("REFUS : o tombe dans la plage A [%d,%d]\n", af, al); return 2; }
  if (ub >= af && ub <= al) { std::printf("REFUS : b tombe dans la plage A\n"); return 2; }
  std::vector<CoverPoint> cover;
  cover_query(ix, pa, pb, D2, 3, &cover);
  const u64 h = 2;
  u32 cnt[8];
  u64 wmin = 0;
  anchor_sector_kill(cover, ix.upos, ua, ub, pa, pb, D2, 12, 1000000, &wmin, cnt);
  std::printf("cover=%zu cnt=[", cover.size());
  for (int k = 0; k < 8; ++k) std::printf("%u%s", cnt[k], k < 7 ? "," : "]\n");
  const EndpointCredit ec{1, af, al, ub, ub};
  u64 wm = 0;
  const bool killed = anchor_sector_kill(cover, ix.upos, ua, ub, pa, pb, D2, 12, h, &wm, nullptr, true, &ec);
  // Les deux minima GLOBAUX doivent valoir 1 : c'est ce qui rend le cas croise.
  u64 mn_pur = cnt[0];
  for (int k = 1; k < 8; ++k) mn_pur = std::min(mn_pur, (u64)cnt[k]);
  std::printf("h=%llu base=1 min_pur=%llu witness_min=%llu tue=%d mutant=%d\n", (unsigned long long)h,
              (unsigned long long)mn_pur, (unsigned long long)wm, killed ? 1 : 0, mutant ? 1 : 0);
  if (mn_pur >= h) { std::printf("REFUS : le compte pur suffit deja, le cas n'est pas croise\n"); return 2; }
  if (mutant) {
    if (!killed) return 4;  // mutant TUE : la forme globale rate la mort croisee
    std::printf("MUTANT NON TUE\n");
    return 1;
  }
  if (!killed) { std::printf("DESACCORD : la forme par secteur ne tue pas le cas croise\n"); return 1; }
  std::printf("sector_credit_crossed_fixture OK\n");
  return 0;
}
