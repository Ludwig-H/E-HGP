// MorseHGP3D v5 — SONDE (jamais un claim) : la fibre A x B x C tue-t-elle
// des blocs entiers, et combien de TRAVAIL cela eviterait-il ?
//
// Version 2, apres la reponse d'audit `REPONSE_A_CLAUDE_BLOCS_ABC_20260829.md`,
// qui a retire quatre formulations de la version 1. Les corrections :
//
//   1. LE VRAI IDEAL est `min_exact_ball_depth` = min sur les triplets valides
//      du bloc de la profondeur EXACTE de leur circumboule, pas le compte des
//      temoins COMMUNS. Un bloc est reellement tuable ssi CHACUN de ses
//      triplets est tue, donc ssi ce minimum atteint h_3. Le compte commun
//      `tb` n'en est qu'un MINORANT : deux triplets peuvent mourir par neuf
//      temoins incompatibles alors que leur intersection en contient moins de
//      neuf. `tb < h3` ne refutait donc rien, et la v1 le laissait croire.
//   2. LA BASELINE porte sur TOUTES les ancres actives du bloc, pas sur la
//      seule paire (ra.first, rb.first). Le bloc meurt au niveau PAIRE ssi
//      toutes ses ancres actives ont h_3 temoins universels de fuseau.
//   3. LES BLOCS SANS TRIPLET VALIDE ne sont plus comptes comme « tuables » :
//      aucun classifieur de BOITES ne les reconnait encore. Ils sont publies
//      a part, comme une cible, jamais comme un acquis.
//   4. LA MASSE CAPEE est publiee : les blocs trop gros pour la force brute
//      sont comptes en blocs ET en triplets, pour qu'un cap ne se fasse pas
//      passer pour une absence.
//
// Et la mesure est ponderee par le TRAVAIL evite : un bloc pese le nombre de
// rescans de profondeur qu'il declencherait, soit (triplets valides) x (taille
// du candidat de cover). Tuer un gros bloc ne vaut pas tuer un petit.
//
// Ce que la sonde ne fait toujours pas : proposer un certificat de BOITES.
// Elle borne ce qu'un tel certificat pourrait au mieux atteindre, en payant la
// force brute que le certificat devra eviter.
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

  // Denombrement des blocs pour un echantillonnage SYSTEMATIQUE a pas constant
  // (deterministe et reproductible). Ce n'est pas un echantillon aleatoire :
  // une inference devra varier la phase ou utiliser un hash stable des IDs,
  // car l'ordre des rectangles/handles peut etre correle avec la geometrie.
  generate_detail::AnchorScratch sc;
  u64 blocs_total = 0;
  for (const AliveRect& ar : alive) {
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
    blocs_total += (u64)sc.handles.size();
  }
  if (blocs_total == 0) return 3;
  const u64 pas = std::max<u64>(1, blocs_total / std::max<u64>(1, (u64)blocs_cible));

  u64 vus = 0, echantillon = 0;
  u64 vides = 0, capes = 0, capes_triplets = 0;
  u64 tuables_bloc = 0, tuables_paire = 0, tuables_bloc_seulement = 0, juges = 0;
  u64 triplets_cumules = 0;
  // Ponderation par TRAVAIL : un bloc coute (triplets valides) x (candidats),
  // c'est-a-dire les rescans de profondeur qu'il declenche.
  u64 travail_total = 0, travail_tue_bloc = 0, travail_tue_paire = 0, travail_vide = 0;
  std::vector<std::pair<i32, i32>> ancres;
  std::vector<Q3Form> formes;
  std::vector<i32> carriers;
  std::vector<i32> candidats;  // sites du candidat de cover du rectangle

  for (const AliveRect& ar : alive) {
    const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
    candidats.clear();
    for (const NodeRef hz : sc.handles) {
      const NodeRange rz = ix.range_of(hz);
      for (i32 uz = rz.first; uz <= rz.last; ++uz) candidats.push_back(uz);
    }
    for (const NodeRef h : sc.handles) {
      const u64 mon_bloc = vus++;
      if (mon_bloc % pas != 0) continue;
      ++echantillon;
      const NodeRange rc = ix.range_of(h);
      formes.clear(); ancres.clear(); carriers.clear();
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
            ancres.push_back({ua, ub});
            carriers.push_back(ux);
          }
        }
      // RETRACTATION 4 : un cap se publie, il ne se tait pas.
      if (depasse) { ++capes; capes_triplets += (u64)formes.size(); continue; }
      // RETRACTATION 3 : un bloc sans triplet valide n'est PAS un acquis —
      // aucun classifieur de boites ne le reconnait encore.
      if (formes.empty()) { ++vides; travail_vide += (u64)candidats.size(); continue; }
      ++juges;
      triplets_cumules += (u64)formes.size();
      const u64 travail = (u64)formes.size() * (u64)candidats.size();
      travail_total += travail;

      // RETRACTATION 1 : LE VRAI IDEAL — minimum sur les triplets du bloc de la
      // profondeur EXACTE de leur circumboule. Sortie anticipee a h_3 : le
      // minimum n'a pas besoin d'etre connu au-dela du seuil.
      u64 min_depth = ~0ull;
      for (size_t t = 0; t < formes.size() && min_depth >= h3; ++t) {
        u64 prof = 0;
        for (const i32 uz : candidats) {
          if (uz == ancres[t].first || uz == ancres[t].second || uz == carriers[t]) continue;
          if (q3_power(formes[t], ix.upos[(size_t)uz]) < 0 && ++prof >= h3) break;
        }
        min_depth = std::min(min_depth, prof);
      }
      const bool kb = min_depth >= h3;

      // RETRACTATION 2 : baseline sur TOUTES les ancres actives du bloc. Le
      // bloc meurt au niveau PAIRE ssi chacune de ses ancres actives a h_3
      // temoins universels de fuseau.
      bool kp = true;
      for (size_t t = 0; t < formes.size() && kp; ++t) {
        if (t > 0 && ancres[t] == ancres[t - 1]) continue;  // ancres consecutives par construction
        const P3& pa = ix.upos[(size_t)ancres[t].first];
        const P3& pb = ix.upos[(size_t)ancres[t].second];
        u64 n3 = 0;
        for (const i32 uz : candidats) {
          if (uz == ancres[t].first || uz == ancres[t].second) continue;
          if (in_spindle(Lane::kQ3, pa, pb, ix.upos[(size_t)uz]) && ++n3 >= h3) break;
        }
        if (n3 < h3) kp = false;
      }

      if (kb) { ++tuables_bloc; travail_tue_bloc += travail; }
      if (kp) { ++tuables_paire; travail_tue_paire += travail; }
      if (kb && !kp) ++tuables_bloc_seulement;
    }
  }

  std::printf("block_witness_v2 famille=%s n=%d h3=%llu rectangles=%zu blocs_total=%llu pas=%llu echantillon=%llu\n",
              cloud_family_name(family), n, (unsigned long long)h3, alive.size(),
              (unsigned long long)blocs_total, (unsigned long long)pas, (unsigned long long)echantillon);
  std::printf("  blocs SANS triplet valide = %llu (%.1f %%) — CIBLE, pas un acquis : aucun classifieur de boites ne les reconnait\n",
              (unsigned long long)vides, echantillon ? 100.0 * (double)vides / (double)echantillon : 0.0);
  std::printf("  blocs CAPES (> %zu triplets, non juges) = %llu, portant au moins %llu triplets\n",
              max_triplets, (unsigned long long)capes, (unsigned long long)capes_triplets);
  if (juges == 0) { std::printf("  aucun bloc juge\n"); return 3; }
  std::printf("  blocs juges = %llu ; triplets valides par bloc = %.1f\n",
              (unsigned long long)juges, (double)triplets_cumules / (double)juges);
  std::printf("  IDEAL VRAI (min_exact_ball_depth >= h3) : %llu blocs (%.1f %%)\n",
              (unsigned long long)tuables_bloc, 100.0 * (double)tuables_bloc / (double)juges);
  std::printf("  BASELINE PAIRE (toutes ancres actives, W_3 >= h3) : %llu blocs (%.1f %%) ; ideal SEUL %llu (%.1f %%)\n",
              (unsigned long long)tuables_paire, 100.0 * (double)tuables_paire / (double)juges,
              (unsigned long long)tuables_bloc_seulement, 100.0 * (double)tuables_bloc_seulement / (double)juges);
  std::printf("  PONDERE PAR LE TRAVAIL (triplets x candidats) : ideal %.1f %%, baseline paire %.1f %% du travail des blocs juges\n",
              travail_total ? 100.0 * (double)travail_tue_bloc / (double)travail_total : 0.0,
              travail_total ? 100.0 * (double)travail_tue_paire / (double)travail_total : 0.0);
  return 0;
}
