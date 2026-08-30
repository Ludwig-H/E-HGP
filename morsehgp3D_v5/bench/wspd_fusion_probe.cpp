// MorseHGP3D v5 — SONDE de mesure (jamais un claim) : la descente WSPD est-elle
// payee trois fois ?
//
// CONSTAT DE STRUCTURE. `generate_candidates` appelle `alive_rectangles` une
// fois par lane (generate.hpp, trois sites). Or les decisions de la descente —
// le predicat `separated`, `box_w2`, le choix du facteur scinde — ne dependent
// PAS de la lane : seul le test de mort `fc.c[lane_idx] >= h_q` en depend. Et
// `count_universal_witnesses` accepte deja un masque trois bits, partage
// l'elagage `hmax4_boxes`, la marche d'arbre et l'evaluation aux coins
// (`corner64_universal_34` decide q3 ET q4 en un appel).
//
// CE QUE LA SONDE MESURE, en counter-only et a un fil :
//   bras A (statu quo) : trois descentes independantes, masques 0b001/0b010/0b100 ;
//   bras B (fusionnee)  : UNE descente, chaque rectangle portant un masque des
//                         lanes encore indecises ; un rectangle n'est scinde que
//                         si au moins une lane y survit.
// Compteurs compares : rectangles visites, appels au compteur de temoins, nœuds
// d'arbre visites par ce compteur, evaluations de coins, et le mur des deux bras.
//
// PORTE DE CORRECTION (c'est elle qui fait foi, pas le gain) : les trois listes
// de rectangles vivants du bras B doivent etre IDENTIQUES a celles du bras A —
// meme cardinal, meme ordre, memes (a, b), meme `core`. Le code de sortie est 3
// si un seul element differe, 2 en cas de refus avant calcul, 0 sinon.
//
// La sonde ne construit aucun cover, n'emet aucun candidat et ne touche pas au
// chemin produit. Elle ne prouve aucune borne : elle chiffre une redondance.
//
// Usage : mhgp5_wspd_fusion_probe --family=F --n=N [--coord=C] [--seed=S] [--s=SEP]
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/generate.hpp"
#include "../src/spindle/witness_count.hpp"

using namespace mhgp5;

namespace {

struct Compteurs {
  u64 rect_visites = 0;    // rectangles retires de la vague
  u64 appels_temoins = 0;  // appels a count_universal_witnesses
  u64 noeuds = 0;          // nœuds d'arbre visites par ces appels
  u64 coins = 0;           // evaluations de coins (autorite de feuille q3/q4)
  u64 vague_pic = 0;       // plus grande vague — le POSTE MEMOIRE de la descente
  double ms = 0.0;
};

// Bras A : la descente actuelle, une lane a la fois. Transcription fidele de
// `alive_rectangles` a `postsep_refine_levels = 0` et un fil, instrumentee.
void descente_par_lane(const CloudIndex& ix, i64 s, const u64 h_of[3], int li, std::vector<AliveRect>* out,
                       Compteurs* c) {
  out->clear();
  if (ix.nodes.empty()) return;
  const u8 mask = (u8)(1u << li);
  const u64 h = h_of[li];
  std::vector<WspdRect> vague, suivante;
  vague.reserve(ix.nodes.size());
  for (const RadixNode& n : ix.nodes) vague.push_back(WspdRect{n.left, n.right});
  while (!vague.empty()) {
    c->vague_pic = std::max(c->vague_pic, (u64)vague.size());
    suivante.clear();
    for (const WspdRect& r : vague) {
      ++c->rect_visites;
      ++c->appels_temoins;
      const FusedCounts fc = count_universal_witnesses(ix, r.a, r.b, h_of, mask, false);
      c->noeuds += fc.nodes_visited;
      c->coins += fc.corner_evals;
      if (fc.c[li] >= h) continue;
      const AxisBox va = ix.box_of(r.a), vb = ix.box_of(r.b);
      if (wspd_detail::separated(va, vb, s, 1)) {
        ++c->appels_temoins;
        const FusedCounts ff = count_universal_witnesses(ix, r.a, r.b, h_of, mask, true);
        c->noeuds += ff.nodes_visited;
        c->coins += ff.corner_evals;
        if (ff.c[li] < h) out->push_back(AliveRect{r, ff.c[li]});
        continue;
      }
      const i64 w2a = wspd_detail::box_w2(va), w2b = wspd_detail::box_w2(vb);
      const bool split_a = (r.a >= 0) && (r.b < 0 || w2a >= w2b);
      const NodeRef keep = split_a ? r.b : r.a;
      const RadixNode& n = ix.nodes[(size_t)(split_a ? r.a : r.b)];
      suivante.push_back(split_a ? WspdRect{n.left, keep} : WspdRect{keep, n.left});
      suivante.push_back(split_a ? WspdRect{n.right, keep} : WspdRect{keep, n.right});
    }
    vague.swap(suivante);
  }
}

// Bras B : une seule descente. `ouvertes` = lanes dont le sort n'est pas encore
// scelle sur ce rectangle. Une lane sort du masque des qu'elle est morte (elle
// n'a plus rien a apprendre de ce sous-arbre) ; le rectangle n'est scinde que
// si le masque reste non vide. Les decisions de scission etant independantes de
// la lane, chaque lane voit exactement la meme suite de rectangles qu'au bras A.
struct RectMasque {
  WspdRect r;
  u8 ouvertes;
};

void descente_fusionnee(const CloudIndex& ix, i64 s, const u64 h_of[3], std::vector<AliveRect> out[3],
                        Compteurs* c) {
  for (int li = 0; li < 3; ++li) out[li].clear();
  if (ix.nodes.empty()) return;
  std::vector<RectMasque> vague, suivante;
  vague.reserve(ix.nodes.size());
  for (const RadixNode& n : ix.nodes) vague.push_back(RectMasque{WspdRect{n.left, n.right}, 0b111});
  while (!vague.empty()) {
    c->vague_pic = std::max(c->vague_pic, (u64)vague.size());
    suivante.clear();
    for (const RectMasque& rm : vague) {
      ++c->rect_visites;
      ++c->appels_temoins;
      const FusedCounts fc = count_universal_witnesses(ix, rm.r.a, rm.r.b, h_of, rm.ouvertes, false);
      c->noeuds += fc.nodes_visited;
      c->coins += fc.corner_evals;
      u8 vivantes = 0;
      for (int li = 0; li < 3; ++li)
        if ((rm.ouvertes & (1u << li)) && fc.c[li] < h_of[li]) vivantes |= (u8)(1u << li);
      if (!vivantes) continue;
      const AxisBox va = ix.box_of(rm.r.a), vb = ix.box_of(rm.r.b);
      if (wspd_detail::separated(va, vb, s, 1)) {
        ++c->appels_temoins;
        const FusedCounts ff = count_universal_witnesses(ix, rm.r.a, rm.r.b, h_of, vivantes, true);
        c->noeuds += ff.nodes_visited;
        c->coins += ff.corner_evals;
        for (int li = 0; li < 3; ++li)
          if ((vivantes & (1u << li)) && ff.c[li] < h_of[li]) out[li].push_back(AliveRect{rm.r, ff.c[li]});
        continue;
      }
      const i64 w2a = wspd_detail::box_w2(va), w2b = wspd_detail::box_w2(vb);
      const bool split_a = (rm.r.a >= 0) && (rm.r.b < 0 || w2a >= w2b);
      const NodeRef keep = split_a ? rm.r.b : rm.r.a;
      const RadixNode& n = ix.nodes[(size_t)(split_a ? rm.r.a : rm.r.b)];
      suivante.push_back(RectMasque{split_a ? WspdRect{n.left, keep} : WspdRect{keep, n.left}, vivantes});
      suivante.push_back(RectMasque{split_a ? WspdRect{n.right, keep} : WspdRect{keep, n.right}, vivantes});
    }
    vague.swap(suivante);
  }
}

bool memes_listes(const std::vector<AliveRect>& x, const std::vector<AliveRect>& y) {
  if (x.size() != y.size()) return false;
  for (size_t i = 0; i < x.size(); ++i)
    if (x[i].r.a != y[i].r.a || x[i].r.b != y[i].r.b || x[i].core != y[i].core) return false;
  return true;
}

double ms_depuis(std::chrono::steady_clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

}  // namespace

int main(int argc, char** argv) {
  std::printf("wspd_fusion_probe pin_configure=%s worktree_src_modifie=%s\n", MHGP5_PROBE_PIN, MHGP5_PROBE_DIRTY);
  CloudFamily family = CloudFamily::kUniform;
  int n = 2000, coord = 0;
  long long seed = 3, s = 8;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--family=", 0) == 0) { if (!parse_cloud_family(a.c_str() + 9, &family)) return 2; }
    else if (a.rfind("--n=", 0) == 0) n = std::atoi(a.c_str() + 4);
    else if (a.rfind("--coord=", 0) == 0) coord = std::atoi(a.c_str() + 8);
    else if (a.rfind("--seed=", 0) == 0) seed = std::atoll(a.c_str() + 7);
    else if (a.rfind("--s=", 0) == 0) s = std::atoll(a.c_str() + 4);
    else return 2;
  }
  if (n < 2 || s < 8) return 2;  // le plancher de profil s >= 8 vaut aussi pour la sonde
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, seed));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  const u64 smax = 11;
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax), lane_h(Lane::kQ3, smax), lane_h(Lane::kQ4, smax)};

  std::vector<AliveRect> a_out[3];
  Compteurs A;
  {
    const auto t0 = std::chrono::steady_clock::now();
    for (int li = 0; li < 3; ++li) descente_par_lane(ix, s, h_of, li, &a_out[li], &A);
    A.ms = ms_depuis(t0);
  }

  std::vector<AliveRect> b_out[3];
  Compteurs B;
  {
    const auto t0 = std::chrono::steady_clock::now();
    descente_fusionnee(ix, s, h_of, b_out, &B);
    B.ms = ms_depuis(t0);
  }

  bool identique = true;
  for (int li = 0; li < 3; ++li) identique = identique && memes_listes(a_out[li], b_out[li]);

  const auto pct = [](u64 av, u64 ap) { return av ? 100.0 * (1.0 - (double)ap / (double)av) : 0.0; };
  std::printf("famille=%s n=%d coord=%d seed=%lld s=%lld smax=%llu\n", cloud_family_name(family), n, coord, seed, s,
              (unsigned long long)smax);
  std::printf("rect_vivants q2/q3/q4 = %zu/%zu/%zu ; listes_identiques=%s\n", a_out[0].size(), a_out[1].size(),
              a_out[2].size(), identique ? "OUI" : "NON");
  std::printf("bras_A_trois_descentes  rect_visites=%llu appels_temoins=%llu noeuds=%llu coins=%llu mur_ms=%.1f\n",
              (unsigned long long)A.rect_visites, (unsigned long long)A.appels_temoins, (unsigned long long)A.noeuds,
              (unsigned long long)A.coins, A.ms);
  std::printf("bras_B_fusionnee        rect_visites=%llu appels_temoins=%llu noeuds=%llu coins=%llu mur_ms=%.1f\n",
              (unsigned long long)B.rect_visites, (unsigned long long)B.appels_temoins, (unsigned long long)B.noeuds,
              (unsigned long long)B.coins, B.ms);
  std::printf("economie                rect_visites=%.1f %% appels_temoins=%.1f %% noeuds=%.1f %% coins=%.1f %% mur=%.1f %%\n",
              pct(A.rect_visites, B.rect_visites), pct(A.appels_temoins, B.appels_temoins), pct(A.noeuds, B.noeuds),
              pct(A.coins, B.coins), A.ms > 0 ? 100.0 * (1.0 - B.ms / A.ms) : 0.0);
  // CE QUE LA FUSION COUTE. La vague du bras B porte l'UNION des lanes encore
  // indecises et un octet de masque par rectangle ; les trois listes de vivants
  // coexistent au lieu d'etre recyclees. Les deux postes sont nommes ici et non
  // estimes : ils decident si la fusion est recevable sous le contrat memoire.
  const u64 a_vivants = (u64)std::max(a_out[0].size(), std::max(a_out[1].size(), a_out[2].size()));
  const u64 b_vivants = (u64)(b_out[0].size() + b_out[1].size() + b_out[2].size());
  std::printf("memoire                 vague_pic A=%llu (%llu o) B=%llu (%llu o) ; vivants_residents A=%llu (%llu o) B=%llu (%llu o)\n",
              (unsigned long long)A.vague_pic, (unsigned long long)(A.vague_pic * sizeof(WspdRect)),
              (unsigned long long)B.vague_pic, (unsigned long long)(B.vague_pic * sizeof(RectMasque)),
              (unsigned long long)a_vivants, (unsigned long long)(a_vivants * sizeof(AliveRect)),
              (unsigned long long)b_vivants, (unsigned long long)(b_vivants * sizeof(AliveRect)));
  if (!identique) {
    std::fprintf(stderr, "INVARIANT VIOLE : les listes de rectangles vivants different entre les deux bras\n");
    return 3;
  }
  return 0;
}
