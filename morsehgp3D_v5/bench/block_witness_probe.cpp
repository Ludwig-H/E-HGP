// MorseHGP3D v5 — SONDE (jamais un claim) : le certificat de BLOC A x B x C
// tue-t-il la ou le certificat de RECTANGLE A x B echoue ?
//
// Question posee par Louis le 29 aout : enumerer des blocs A x B x C — la paire
// (a,b) par un rectangle WSPD, le troisieme point x par un handle — et tuer des
// blocs entiers par temoins centraux. Les handles forment une ANTICHAINE, donc
// des plages disjointes : (rectangle, handle) est bien une PARTITION des
// triplets, pas un recouvrement, et l'exact-once est gratuit.
//
// Cette sonde ne mesure PAS un certificat implementable : elle mesure le
// CERTIFICAT IDEAL, c'est-a-dire l'ensemble EXACT des temoins universels du
// bloc, obtenu par force brute sur les triplets reellement valides du bloc.
// C'est un MAJORANT de ce que tout certificat sain pourra jamais atteindre.
// Si l'ideal ne tue pas, l'idee est morte et aucune ingenierie ne la sauvera ;
// s'il tue, il reste a construire une approximation saine et bon marche, et
// c'est un autre travail.
//
// Definitions, toutes exactes en entiers :
//   - triplet VALIDE du bloc : (a,b,x) avec a dans A, b dans B, x dans C,
//     |ab| > 0 et `is_acute_seed` vrai (lentille, acuite stricte en x, owner
//     canonique) — exactement ce que la lane q3 emettrait ;
//   - temoin UNIVERSEL du bloc : un site z du candidat de cover du rectangle,
//     distinct des trois sommets, tel que `q3_power < 0` pour TOUS les
//     triplets valides du bloc (donc strictement interieur a toutes leurs
//     circumboules) ;
//   - bloc TUABLE : au moins h_3 temoins universels — alors aucun de ses
//     triplets ne peut survivre au filtre de profondeur.
// Le comparatif est le certificat de PAIRE deja en production (`in_spindle`,
// temoins universels du disque entier des centres), evalue sur les memes
// blocs : c'est lui que le bloc doit battre pour valoir un chantier.
//
// Usage : mhgp5_block_witness_probe --family=F --n=N [--blocs=K] [--max-triplets=T]
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/lanes/edge_cover.hpp"
#include "../src/lanes/q3.hpp"
#include "../src/pipeline/generate.hpp"
#include "../src/spindle/spindle.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 8000, coord = 0;
  size_t blocs_cible = 4000, max_triplets = 4096;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--family=", 0) == 0) { if (!parse_cloud_family(a.c_str() + 9, &family)) return 2; }
    else if (a.rfind("--n=", 0) == 0) n = std::atoi(a.c_str() + 4);
    else if (a.rfind("--coord=", 0) == 0) coord = std::atoi(a.c_str() + 8);
    else if (a.rfind("--blocs=", 0) == 0) blocs_cible = (size_t)std::atoll(a.c_str() + 8);
    else if (a.rfind("--max-triplets=", 0) == 0) max_triplets = (size_t)std::atoll(a.c_str() + 15);
    else return 2;
  }
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, 3));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  const u64 smax = 11;
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax), lane_h(Lane::kQ3, smax), lane_h(Lane::kQ4, smax)};
  const u64 h3 = h_of[1];
  std::vector<AliveRect> alive;
  u64 visited = 0, workers = 0;
  generate_detail::alive_rectangles(ix, 8, h_of, 1, 1, &alive, &visited, &workers);

  // Denombrement des blocs pour un echantillonnage a PAS CONSTANT (deterministe,
  // reproductible, et non biaise vers les rectangles lourds ou legers).
  generate_detail::AnchorScratch sc;
  u64 blocs_total = 0;
  for (const AliveRect& ar : alive) {
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
    blocs_total += (u64)sc.handles.size();
  }
  if (blocs_total == 0) return 3;
  const u64 pas = std::max<u64>(1, blocs_total / std::max<u64>(1, (u64)blocs_cible));

  u64 vus = 0, echantillon = 0;
  u64 vides = 0, tuables_bloc = 0, tuables_paire = 0, tuables_bloc_seulement = 0;
  u64 trop_gros = 0, triplets_cumules = 0, temoins_bloc_cumules = 0, temoins_paire_cumules = 0;
  std::vector<u64> hist_bloc(64, 0), hist_paire(64, 0);
  std::vector<std::pair<i32, i32>> paires;
  std::vector<Q3Form> formes;
  std::vector<i32> sommets;  // sommets des triplets, pour exclure a, b, x des temoins

  for (const AliveRect& ar : alive) {
    const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
    for (const NodeRef h : sc.handles) {
      const u64 mon_bloc = vus++;
      if (mon_bloc % pas != 0) continue;
      ++echantillon;
      const NodeRange rc = ix.range_of(h);
      // Triplets VALIDES du bloc.
      formes.clear();
      sommets.clear();
      bool depasse = false;
      for (i32 ua = ra.first; ua <= ra.last && !depasse; ++ua)
        for (i32 ub = rb.first; ub <= rb.last && !depasse; ++ub) {
          const P3& pa = ix.upos[(size_t)ua];
          const P3& pb = ix.upos[(size_t)ub];
          const i64 D2 = p3_norm2(p3_sub(pb, pa));
          if (D2 == 0) continue;
          for (i32 ux = rc.first; ux <= rc.last; ++ux) {
            if (ux == ua || ux == ub) continue;
            const P3& px = ix.upos[(size_t)ux];
            if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(ux))) continue;
            if (formes.size() >= max_triplets) { depasse = true; break; }
            formes.push_back(q3_form(pa, pb, px));
            sommets.push_back(ua); sommets.push_back(ub); sommets.push_back(ux);
          }
        }
      if (depasse) { ++trop_gros; continue; }
      if (formes.empty()) { ++vides; continue; }  // bloc sans triplet valide : mort sans certificat
      triplets_cumules += (u64)formes.size();

      // Candidats temoins : les points des handles du rectangle (sur-ensemble
      // fail-open du cover de chaque ancre du bloc).
      u64 tb = 0, tp = 0;
      for (const NodeRef hz : sc.handles) {
        const NodeRange rz = ix.range_of(hz);
        for (i32 uz = rz.first; uz <= rz.last; ++uz) {
          bool sommet = false;
          for (size_t k = 0; k < sommets.size(); ++k)
            if (sommets[k] == uz) { sommet = true; break; }
          if (sommet) continue;
          const P3& pz = ix.upos[(size_t)uz];
          bool universel = true;
          for (const Q3Form& f : formes)
            if (!(q3_power(f, pz) < 0)) { universel = false; break; }
          if (universel) ++tb;
        }
      }
      // Comparatif : temoins universels de PAIRE (production), pour la premiere
      // paire du bloc — `in_spindle` ne depend pas de C, c'est le point.
      {
        const P3& pa = ix.upos[(size_t)ra.first];
        const P3& pb = ix.upos[(size_t)rb.first];
        for (const NodeRef hz : sc.handles) {
          const NodeRange rz = ix.range_of(hz);
          for (i32 uz = rz.first; uz <= rz.last; ++uz)
            if (uz != ra.first && uz != rb.first && in_spindle(Lane::kQ3, pa, pb, ix.upos[(size_t)uz])) ++tp;
        }
      }
      temoins_bloc_cumules += tb;
      temoins_paire_cumules += tp;
      ++hist_bloc[std::min<size_t>(63, (size_t)tb)];
      ++hist_paire[std::min<size_t>(63, (size_t)tp)];
      const bool kb = tb >= h3, kp = tp >= h3;
      if (kb) ++tuables_bloc;
      if (kp) ++tuables_paire;
      if (kb && !kp) ++tuables_bloc_seulement;
    }
  }

  const u64 juges = echantillon - vides - trop_gros;
  std::printf("block_witness famille=%s n=%d h3=%llu rectangles=%zu blocs_total=%llu pas=%llu echantillon=%llu\n",
              cloud_family_name(family), n, (unsigned long long)h3, alive.size(),
              (unsigned long long)blocs_total, (unsigned long long)pas, (unsigned long long)echantillon);
  std::printf("  blocs SANS triplet valide = %llu (%.1f %%) ; blocs trop gros (> %zu triplets, non juges) = %llu\n",
              (unsigned long long)vides, echantillon ? 100.0 * (double)vides / (double)echantillon : 0.0,
              max_triplets, (unsigned long long)trop_gros);
  if (juges == 0) { std::printf("  aucun bloc juge\n"); return 3; }
  std::printf("  blocs juges = %llu ; triplets valides par bloc = %.1f en moyenne\n",
              (unsigned long long)juges, (double)triplets_cumules / (double)juges);
  std::printf("  temoins universels par bloc : BLOC %.2f en moyenne, PAIRE %.2f en moyenne\n",
              (double)temoins_bloc_cumules / (double)juges, (double)temoins_paire_cumules / (double)juges);
  std::printf("  TUABLES (>= h3) : par le bloc %llu (%.1f %%), par la paire %llu (%.1f %%), "
              "par le bloc SEULEMENT %llu (%.1f %%)\n",
              (unsigned long long)tuables_bloc, 100.0 * (double)tuables_bloc / (double)juges,
              (unsigned long long)tuables_paire, 100.0 * (double)tuables_paire / (double)juges,
              (unsigned long long)tuables_bloc_seulement, 100.0 * (double)tuables_bloc_seulement / (double)juges);
  std::printf("  histogramme des temoins de bloc (0..12) :");
  for (size_t k = 0; k <= 12; ++k) std::printf(" %llu", (unsigned long long)hist_bloc[k]);
  std::printf("\n");
  return 0;
}
