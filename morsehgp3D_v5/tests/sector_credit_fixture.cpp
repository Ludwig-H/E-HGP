// MorseHGP3D v5 — FIXTURE du CREDIT D'EXTREMITE PAR SECTEUR
// (src/lanes/sector_kill.hpp, docs/CREDIT_SECTEUR.md).
//
// Le credit base = h_a(a) + h_b(b) compte des temoins qui vivent DANS
// A union B. Les temoins de secteur pris HORS boites leur sont disjoints et
// s'y ajoutent ; ceux pris DEDANS sont deja comptes par base et ne doivent
// JAMAIS etre recredites — sans quoi l'ancre meurt sur un compte double.
//
// Configuration gravee : a = (0,0,0), b = (50,0,0) (D = 50, m = (25,0,0)) ;
// douze sites sur le cercle de rayon 20 du plan bissecteur x = 25, donc
// STRICTEMENT interieurs a la boule diametrale (20 < 25) et repartis sur les
// huit secteurs. Le credit declare des boites d'indices qui couvrent TOUS les
// sites : cnt_hors[k] = 0 pour tout k. Avec h = min_k cnt[k] + 1 et base = 1 :
//   forme honnete   : max( cnt[k], 0 + 1 ) < h pour tout k  -> ancre VIVANTE ;
//   `sector-credit-inbox` : le garde !in_boxes tombe, cnt_hors[k] = cnt[k],
//                   donc max( cnt[k], cnt[k] + 1 ) >= h     -> ancre MORTE.
// C'est la contradiction minimale qui distingue « temoin deja compte » de
// « temoin disjoint ».
// Codes : 0 nominal (vivante honnete, morte sans le garde), 1 desaccord,
// 2 refus avant calcul, 4 mutant tue.
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
  const bool mutant = MHGP5_MUTANT("sector-credit-inbox");
  std::vector<InputPoint> in;
  const auto add = [&](i64 x, i64 y, i64 z) {
    InputPoint p;
    p.id = (PointId)in.size();
    p.position = P3{x, y + 100, z + 100};  // profil u16 : coordonnees positives
    in.push_back(p);
  };
  add(0, 0, 0);   // a
  add(50, 0, 0);  // b
  const i64 ring[][2] = {{20, 0}, {-20, 0}, {0, 20}, {0, -20}, {12, 16}, {12, -16},
                         {-12, 16}, {-12, -16}, {16, 12}, {16, -12}, {-16, 12}, {-16, -12}};
  for (const auto& p : ring) add(25, p[0], p[1]);
  const CloudIndex ix = build_cloud_index(in);
  if (!ix.valid || ix.has_duplicate_positions()) { std::printf("REFUS : index\n"); return 2; }
  const P3 pa{0, 100, 100}, pb{50, 100, 100};
  i32 ua = -1, ub = -1;
  for (i32 u = 0; u < (i32)ix.upos.size(); ++u) {
    if (ix.upos[(size_t)u].x == pa.x && ix.upos[(size_t)u].y == pa.y && ix.upos[(size_t)u].z == pa.z) ua = u;
    if (ix.upos[(size_t)u].x == pb.x && ix.upos[(size_t)u].y == pb.y && ix.upos[(size_t)u].z == pb.z) ub = u;
  }
  if (ua < 0 || ub < 0) return 2;
  const i64 D2 = p3_norm2(p3_sub(pb, pa));
  std::vector<CoverPoint> cover;
  cover_query(ix, pa, pb, D2, 3, &cover);
  // Compte pur des secteurs (sans credit) : calibre h sur la configuration.
  u64 wmin = 0;
  anchor_sector_kill(cover, ix.upos, ua, ub, pa, pb, D2, 12, /*h=*/1000000, &wmin);
  if (wmin < 1) { std::printf("REFUS : secteur vide (wmin=0)\n"); return 2; }
  const u64 h = wmin + 1;
  // Boites du credit : tout l'intervalle d'indices, donc cnt_hors[k] = 0.
  const EndpointCredit ec{1, 0, (i32)ix.upos.size() - 1, 0, -1};
  u64 wmin_ec = 0;
  const bool killed = anchor_sector_kill(cover, ix.upos, ua, ub, pa, pb, D2, 12, h, &wmin_ec, nullptr, true, &ec);
  std::printf("cover=%zu temoins_min=%llu h=%llu base=1 tue=%d mutant=%d\n", cover.size(),
              (unsigned long long)wmin, (unsigned long long)h, killed ? 1 : 0, mutant ? 1 : 0);
  if (mutant) {
    if (killed) return 4;  // mutant TUE : le compte double a tue une ancre vivante
    std::printf("MUTANT NON TUE\n");
    return 1;
  }
  if (killed) { std::printf("DESACCORD : ancre tuee sans temoin disjoint\n"); return 1; }
  // Temoin de non-vacuite : le credit DOIT tuer des que les temoins sont hors boites.
  const EndpointCredit ec_hors{1, ua, ua, ub, ub};  // boites non vides mais sans aucun temoin
  u64 wmin_h = 0;
  if (!anchor_sector_kill(cover, ix.upos, ua, ub, pa, pb, D2, 12, h, &wmin_h, nullptr, true, &ec_hors)) {
    std::printf("DESACCORD : le credit hors boites ne tue pas\n");
    return 1;
  }
  std::printf("sector_credit_fixture OK\n");
  return 0;
}
