// MorseHGP3D v3 — PRODUCTEUR DE SOURCE S PAR ANCRE MAXIMALE.
//
// Specification : audits/NOTE_SOLUTION_SOURCE_ANCRE_MAXIMALE_ENVELOPPE_20260812.md
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=proposition_math_non_recue,
//         public_status=not_claimed.
//
// Ce binaire n'est PAS un benchmark et ne qualifie aucun SLO. Il produit
// Source S, publie ses compteurs de travail et se compare a une enumeration
// exhaustive bornee. Les temps imprimes sont des diagnostics.
//
// ORDONNANCE
//   1. LBVH Morton exact resident.
//   2. Pour chaque point `a` : requete mono-arbre qui elimine un noeud entier
//      des qu'une boule temoin commune contient dix PointId (certificat exact,
//      Lemmes B et C de la note de solution). Les survivants sont les
//      partenaires candidats `b > a`.
//   3. Liste de sites triee par distance a `a`, de rayon 1,2248 * Dmax : elle
//      contient tous les interieurs de toutes les boules ancrees en `a`.
//   4. Par ancre (a,b) : lentille des carriers, q3 par droite, q4 par paire,
//      positivite, arete maximale canonique, census exact.
//   5. Emission exacte-une-fois : un support n'est publie que depuis son arete
//      maximale canonique.
//
// TOUTE DECISION EST ENTIERE.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "prototype/anchor_envelope.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/morton_lbvh.hpp"

namespace {

using mhgp::i128;
using mhgp::i64;
using mhgp::P3;
using mhgp3v::CloudFamily;
using mhgp3v::LbvhNode;
using mhgp3v::MortonLbvh;
using namespace mhgp3v::anchor;

// ---------------------------------------------------------------------------
// Compteurs de travail. Ils ferment les gates W_front et W_extend : aucun
// n'est optionnel, aucun n'est estime.
// ---------------------------------------------------------------------------
struct Counters {
  long long front_node_visits = 0;      // noeuds ouverts par la requete d'ancre
  long long front_witness_calls = 0;    // boules temoins evaluees
  long long front_witness_visits = 0;   // noeuds visites par ces boules
  long long front_witness_prunes = 0;   // noeuds fermes par dix temoins
  long long candidate_pairs = 0;        // paires (a,b) survivantes, b > a
  long long anchors_opened = 0;         // ancres candidates ouvertes
  long long anchors_lane_dead = 0;      // ancres tuees par les boules de milieu
  long long anchors_extended = 0;       // ancres passant les boules de milieu
  long long anchors_disk_dead = 0;      // ancres tuees par les toujours-interieurs
  long long site_gather_visits = 0;     // noeuds visites par les listes de sites
  long long site_evaluations = 0;       // sites charges dans une enveloppe
  long long kept_sites = 0;             // sites survivant au filtre d'enveloppe
  long long lens_carriers = 0;          // carriers retenus par la lentille
  long long q3_candidates = 0;
  long long q4_candidates = 0;
  long long interior_tests = 0;         // predicats de puissance evalues
  long long reject_positivity = 0;
  long long reject_owner = 0;
  long long reject_rank = 0;
  long long reject_degenerate = 0;
  long long supports_q2 = 0;
  long long supports_q3 = 0;
  long long supports_q4 = 0;
  long long shell_extra_supports = 0;   // supports dont U_B depasse S
  long long max_site_list = 0;
  long long max_kept = 0;
  long long max_lens = 0;

  void merge(const Counters& o) {
    front_node_visits += o.front_node_visits;
    front_witness_calls += o.front_witness_calls;
    front_witness_visits += o.front_witness_visits;
    front_witness_prunes += o.front_witness_prunes;
    candidate_pairs += o.candidate_pairs;
    anchors_opened += o.anchors_opened;
    anchors_lane_dead += o.anchors_lane_dead;
    anchors_extended += o.anchors_extended;
    anchors_disk_dead += o.anchors_disk_dead;
    site_gather_visits += o.site_gather_visits;
    site_evaluations += o.site_evaluations;
    kept_sites += o.kept_sites;
    lens_carriers += o.lens_carriers;
    q3_candidates += o.q3_candidates;
    q4_candidates += o.q4_candidates;
    interior_tests += o.interior_tests;
    reject_positivity += o.reject_positivity;
    reject_owner += o.reject_owner;
    reject_rank += o.reject_rank;
    reject_degenerate += o.reject_degenerate;
    supports_q2 += o.supports_q2;
    supports_q3 += o.supports_q3;
    supports_q4 += o.supports_q4;
    shell_extra_supports += o.shell_extra_supports;
    max_site_list = std::max(max_site_list, o.max_site_list);
    max_kept = std::max(max_kept, o.max_kept);
    max_lens = std::max(max_lens, o.max_lens);
  }
};

// ---------------------------------------------------------------------------
// Requete d'ancre : certificat de mort par boule temoin commune.
//
// Pour un point `a` et un noeud `B` de boite [lo,hi], soit G la distance
// carree minimale de `a` a la boite et ext2 = somme des (hi-lo)^2. Si
//   15 * ext2 <= 2 G
// alors la boule de centre z0 = (a + centre(B))/2 et de rayon carre G/60 est
// incluse, ouverte, dans le spindle universel W4(a,b) de TOUTE paire du
// produit. Dix PointId distincts dedans tuent les trois lanes.
//
// Preuve (note de solution, Lemme B) : pour toute paire, |m - z0| <= rad_m avec
// rad_m^2 <= ext2/8, et la condition donne rad_m <= sqrt(G/60) ; donc
// |z - m| < 2 sqrt(G/60) = sqrt(G/15) <= D/sqrt(15) = rho_4.
// Ni `a` ni `b` ne peuvent etre comptes : leur distance a z0 depasse D/2.
// ---------------------------------------------------------------------------
inline i64 aabb_min_dist2(const LbvhNode& nd, const P3& p) {
  i64 s = 0;
  const i64 c[3] = {p.x, p.y, p.z};
  for (int d = 0; d < 3; ++d) {
    i64 g = 0;
    if (c[d] < nd.lo[d]) g = nd.lo[d] - c[d];
    else if (c[d] > nd.hi[d]) g = c[d] - nd.hi[d];
    s += g * g;
  }
  return s;
}
inline i64 aabb_max_dist2(const LbvhNode& nd, const P3& p) {
  i64 s = 0;
  const i64 c[3] = {p.x, p.y, p.z};
  for (int d = 0; d < 3; ++d) {
    const i64 g = std::max(c[d] - nd.lo[d], nd.hi[d] - c[d]);
    s += g * g;
  }
  return s;
}
inline i64 aabb_extent2(const LbvhNode& nd) {
  i64 s = 0;
  for (int d = 0; d < 3; ++d) {
    const i64 e = nd.hi[d] - nd.lo[d];
    s += e * e;
  }
  return s;
}

// Compte, avec arret a `cap`, les PointId strictement dans la boule
// { z : 15 * |4z - q0|^2 < 4G } ou q0 est le centre quadruple.
// Bornes : |4z - q0| <= 262140, la somme des carres tient en i64.
struct WitnessBall {
  i64 q0[3] = {0, 0, 0};
  i64 four_g = 0;
};
inline bool witness_ball_covers(const LbvhNode& nd, const WitnessBall& wb) {
  // Tout le noeud est-il dans la boule ? On teste le coin le plus eloigne.
  i64 s = 0;
  for (int d = 0; d < 3; ++d) {
    const i64 lo4 = 4 * nd.lo[d] - wb.q0[d];
    const i64 hi4 = 4 * nd.hi[d] - wb.q0[d];
    const i64 far = std::max(lo4 < 0 ? -lo4 : lo4, hi4 < 0 ? -hi4 : hi4);
    s += far * far;
  }
  return 15 * s < wb.four_g;
}
inline bool witness_ball_disjoint(const LbvhNode& nd, const WitnessBall& wb) {
  i64 s = 0;
  for (int d = 0; d < 3; ++d) {
    const i64 lo4 = 4 * nd.lo[d];
    const i64 hi4 = 4 * nd.hi[d];
    i64 g = 0;
    if (wb.q0[d] < lo4) g = lo4 - wb.q0[d];
    else if (wb.q0[d] > hi4) g = wb.q0[d] - hi4;
    s += g * g;
  }
  return 15 * s >= wb.four_g;
}

int witness_count(const MortonLbvh& tree, const std::vector<P3>& pts, const WitnessBall& wb,
                  int cap, Counters* ctr) {
  int found = 0;
  int stack[64];
  int sp = 0;
  stack[sp++] = 0;
  while (sp > 0) {
    const int ni = stack[--sp];
    const LbvhNode& nd = tree.nodes[(std::size_t)ni];
    ++ctr->front_witness_visits;
    if (witness_ball_disjoint(nd, wb)) continue;
    if (witness_ball_covers(nd, wb)) {
      found += nd.end - nd.begin;
      if (found >= cap) return found;
      continue;
    }
    if (nd.left < 0) {
      for (int t = nd.begin; t < nd.end; ++t) {
        const int id = tree.order[(std::size_t)t];
        const P3& z = pts[(std::size_t)id];
        i64 s = 0;
        const i64 c[3] = {z.x, z.y, z.z};
        for (int d = 0; d < 3; ++d) {
          const i64 g = 4 * c[d] - wb.q0[d];
          s += g * g;
        }
        if (15 * s < wb.four_g) {
          if (++found >= cap) return found;
        }
      }
      continue;
    }
    stack[sp++] = nd.left;
    stack[sp++] = nd.right;
    if (sp > 60) {  // profondeur impossible sur un LBVH radix u16 equilibre
      std::fprintf(stderr, "REFUS : pile de parcours saturee\n");
      std::exit(3);
    }
  }
  return found;
}

// ---------------------------------------------------------------------------
// Etat de travail par thread.
// ---------------------------------------------------------------------------
struct Neighbour {
  i64 d2 = 0;
  int id = 0;
  bool operator<(const Neighbour& o) const { return d2 < o.d2 || (d2 == o.d2 && id < o.id); }
};

struct Workspace {
  std::vector<int> partners;        // b > a survivants
  std::vector<Neighbour> sites;     // sites tries par distance a `a`
  std::vector<SiteMargin> margins;  // marges affines sur le disque de Jung
  std::vector<int> kept;            // sites survivant au filtre d'enveloppe
  std::vector<int> lens;            // carriers : lentille ET enveloppe
  std::vector<Support> out;         // supports emis
  std::vector<int> shell_buf;
  std::vector<i64> select_buf;
  Counters ctr;
};

// Parcours d'ancre : collecte les partenaires candidats de `a`.
void collect_partners(const MortonLbvh& tree, const std::vector<P3>& pts, int a,
                      Workspace* ws) {
  const P3& pa = pts[(std::size_t)a];
  ws->partners.clear();
  int stack[64];
  int sp = 0;
  stack[sp++] = 0;
  while (sp > 0) {
    const int ni = stack[--sp];
    const LbvhNode& nd = tree.nodes[(std::size_t)ni];
    ++ws->ctr.front_node_visits;
    const i64 g = aabb_min_dist2(nd, pa);
    if (g > 0) {
      const i64 ext2 = aabb_extent2(nd);
      // 15 * ext2 <= 2 G : la boule temoin commune est non vide et ouverte.
      if (15 * ext2 <= 2 * g) {
        WitnessBall wb{};
        for (int d = 0; d < 3; ++d) {
          const i64 c[3] = {pa.x, pa.y, pa.z};
          wb.q0[d] = 2 * c[d] + nd.lo[d] + nd.hi[d];
        }
        wb.four_g = 4 * g;
        ++ws->ctr.front_witness_calls;
        const int cnt = witness_count(tree, pts, wb, kThresholdQ2, &ws->ctr);
        if (cnt >= kThresholdQ2) {
          ++ws->ctr.front_witness_prunes;
          continue;  // produit entier mort dans les trois lanes
        }
      }
    }
    if (nd.left < 0) {
      for (int t = nd.begin; t < nd.end; ++t) {
        const int id = tree.order[(std::size_t)t];
        if (id > a) ws->partners.push_back(id);
      }
      continue;
    }
    stack[sp++] = nd.left;
    stack[sp++] = nd.right;
    if (sp > 60) {
      std::fprintf(stderr, "REFUS : pile de parcours saturee\n");
      std::exit(3);
    }
  }
}

// Liste de sites : tous les points a distance carree <= r2 de `a`, tries.
void gather_sites(const MortonLbvh& tree, const std::vector<P3>& pts, int a, i64 r2,
                  Workspace* ws) {
  const P3& pa = pts[(std::size_t)a];
  ws->sites.clear();
  int stack[64];
  int sp = 0;
  stack[sp++] = 0;
  while (sp > 0) {
    const int ni = stack[--sp];
    const LbvhNode& nd = tree.nodes[(std::size_t)ni];
    ++ws->ctr.site_gather_visits;
    if (aabb_min_dist2(nd, pa) > r2) continue;
    const bool inside = aabb_max_dist2(nd, pa) <= r2;
    if (nd.left < 0 || inside) {
      for (int t = nd.begin; t < nd.end; ++t) {
        const int id = tree.order[(std::size_t)t];
        if (id == a) continue;
        const P3 r = sub(pts[(std::size_t)id], pa);
        const i64 d2 = (i64)norm2_i64(r);
        if (d2 <= r2) ws->sites.push_back(Neighbour{d2, id});
      }
      continue;
    }
    stack[sp++] = nd.left;
    stack[sp++] = nd.right;
    if (sp > 60) {
      std::fprintf(stderr, "REFUS : pile de parcours saturee\n");
      std::exit(3);
    }
  }
  std::sort(ws->sites.begin(), ws->sites.end());
}

// ---------------------------------------------------------------------------
// Arete maximale canonique : le support n'est emis que depuis elle.
// L'arete gagnante est la plus longue ; a egalite, le plus petit couple
// (min PointId, max PointId) en ordre lexicographique.
// ---------------------------------------------------------------------------
bool owns_canonical_max_edge(const std::vector<P3>& pts, const int* ids, int q, int a, int b) {
  i64 best2 = -1;
  int bi = -1, bj = -1;
  for (int i = 0; i < q; ++i)
    for (int j = i + 1; j < q; ++j) {
      const i64 d2 = (i64)norm2_i64(sub(pts[(std::size_t)ids[i]], pts[(std::size_t)ids[j]]));
      const int lo = std::min(ids[i], ids[j]);
      const int hi = std::max(ids[i], ids[j]);
      if (d2 > best2 || (d2 == best2 && (lo < bi || (lo == bi && hi < bj)))) {
        best2 = d2;
        bi = lo;
        bj = hi;
      }
    }
  return bi == std::min(a, b) && bj == std::max(a, b);
}

// ---------------------------------------------------------------------------
// Census exact : compte les interieurs stricts avec sortie anticipee, et
// collecte le shell. `budget` est le nombre maximal d'interieurs admissibles.
// Retour : -1 si le budget est depasse, sinon p.
// ---------------------------------------------------------------------------
int census(const std::vector<P3>& pts, const std::vector<int>& kept, const BallForm& ball,
           const P3& origin, const int* ids, int q, int budget, std::vector<int>* shell,
           Counters* ctr) {
  int p = 0;
  shell->clear();
  for (int id : kept) {
    bool in_support = false;
    for (int i = 0; i < q; ++i)
      if (ids[i] == id) { in_support = true; break; }
    if (in_support) continue;
    ++ctr->interior_tests;
    const i128 s = power_sign_value(ball, origin, pts[(std::size_t)id]);
    if (s < 0) {
      if (++p > budget) return -1;
    } else if (s == 0) {
      shell->push_back(id);
    }
  }
  return p;
}

// ---------------------------------------------------------------------------
// Extension d'une ancre (a,b) : q2 direct, q3 par droite, q4 par paire.
// ---------------------------------------------------------------------------
void extend_anchor(const std::vector<P3>& pts, int a, int b, i64 d2, int smax, bool use_filter,
                   Workspace* ws) {
  const P3& pa = pts[(std::size_t)a];
  const P3& pb = pts[(std::size_t)b];
  // Prefixe des sites utiles : |z-a|^2 <= 1,5 D^2. Tout interieur d'une boule
  // ancree par l'arete maximale (a,b) y est, puisque R <= D sqrt(3/8).
  int site_count = 0;
  {
    const std::vector<Neighbour>& s = ws->sites;
    while (site_count < (int)s.size() && 2 * s[(std::size_t)site_count].d2 <= 3 * d2) ++site_count;
  }
  ws->ctr.site_evaluations += site_count;
  if (site_count > ws->ctr.max_site_list) ws->ctr.max_site_list = site_count;

  // ------------------------------------------- passe A : marges g seulement
  // g = D2 - |2z-a-b|^2 est un i64 sans racine. Il suffit aux trois lanes et
  // au census q2 : la racine entiere, seule operation chere, n'est payee que
  // sur les ancres qui survivent.
  ws->margins.clear();
  ws->margins.resize((std::size_t)site_count);
  {
    const P3 ab{pa.x + pb.x, pa.y + pb.y, pa.z + pb.z};
    for (int t = 0; t < site_count; ++t) {
      SiteMargin& sm = ws->margins[(std::size_t)t];
      const int id = ws->sites[(std::size_t)t].id;
      const P3& pz = pts[(std::size_t)id];
      const P3 u{2 * pz.x - ab.x, 2 * pz.y - ab.y, 2 * pz.z - ab.z};
      sm.id = id;
      sm.d2a = ws->sites[(std::size_t)t].d2;
      sm.g = d2 - (i64)norm2_i64(u);
    }
  }

  // --------------------------------------------- lanes par boule de milieu
  // Les trois temoins universels du Lemme B sont exactement trois seuils sur
  // la marge g deja calculee, avec u = 2z-a-b et |u|^2 = D2 - g :
  //   q2 : |u|^2 < D2        <=>  g > 0            (boule diametrale)
  //   q3 : 3|u|^2 < D2       <=>  3g > 2 D2        (rayon D/sqrt(12))
  //   q4 : 15|u|^2 < 4 D2    <=>  15g > 11 D2      (rayon D/sqrt(15))
  // Un tel temoin est strictement interieur a TOUTE boule admissible ancree
  // par (a,b) ; il ne peut etre ni a, ni b, ni un carrier (qui est sur la
  // sphere). Le census q2 est lu au meme passage.
  int p2 = 0, extra2 = 0, n3 = 0, n4 = 0;
  const int budget2 = smax - 2, budget3 = smax - 3, budget4 = smax - 4;
  for (int t = 0; t < site_count; ++t) {
    const SiteMargin& sm = ws->margins[(std::size_t)t];
    if (sm.id == b) continue;
    ++ws->ctr.interior_tests;
    if (sm.g > 0) {
      if (p2 <= budget2) ++p2;
      if (3 * sm.g > 2 * d2) {
        ++n3;
        if (15 * sm.g > 11 * d2) ++n4;
      }
    } else if (sm.g == 0) {
      ++extra2;
    }
  }
  const bool lane3 = n3 <= budget3;
  const bool lane4 = n4 <= budget4;

  // ------------------------------------------------------------------ q2
  if (p2 > budget2) {
    ++ws->ctr.reject_rank;
  } else {
    Support sup{};
    sup.id[0] = std::min(a, b);
    sup.id[1] = std::max(a, b);
    sup.q = 2;
    sup.p = p2;
    sup.shell_extra = extra2;
    if (extra2 > 0) ++ws->ctr.shell_extra_supports;
    ws->out.push_back(sup);
    ++ws->ctr.supports_q2;
  }
  if (!lane3 && !lane4) {
    ++ws->ctr.anchors_lane_dead;
    return;
  }
  ++ws->ctr.anchors_extended;

  // ------------------------------ passe B : amplitude exacte et filtre theta
  // Sur le disque de Jung q4, l'amplitude de F_z vaut sqrt(2Q) avec
  // Q = |U|^2 D2 - (U.d)^2. La racine entiere donne les bornes ENTIERES
  // Llow <= L et Uhigh >= U*, donc un filtre fail-open.
  {
    const P3 d = sub(pb, pa);
    const P3 ab{pa.x + pb.x, pa.y + pb.y, pa.z + pb.z};
    for (int t = 0; t < site_count; ++t) {
      SiteMargin& sm = ws->margins[(std::size_t)t];
      const P3& pz = pts[(std::size_t)sm.id];
      const P3 u{2 * pz.x - ab.x, 2 * pz.y - ab.y, 2 * pz.z - ab.z};
      const i128 uu = i128(d2) - i128(sm.g);
      const i128 ud = dot_i64(u, d);
      const i64 s = isqrt_i128(2 * (uu * i128(d2) - ud * ud)) + 1;
      sm.llow = sm.g - s;
      sm.uhigh = sm.g + s;
    }
  }
  i64 theta = 0;
  bool theta_active = false;
  if (use_filter) {
    ws->select_buf.clear();
    for (int t = 0; t < site_count; ++t)
      if (ws->margins[(std::size_t)t].id != b)
        ws->select_buf.push_back(ws->margins[(std::size_t)t].llow);
    if ((int)ws->select_buf.size() >= kThresholdQ3) {
      std::nth_element(ws->select_buf.begin(), ws->select_buf.begin() + (kThresholdQ3 - 1),
                       ws->select_buf.end(), std::greater<i64>());
      theta = ws->select_buf[(std::size_t)(kThresholdQ3 - 1)];
      theta_active = true;
    }
  }
  // `always_inside` : Llow > 0 signifie F_z > 0 sur TOUT le disque, donc un
  // interieur strict de toute boule admissible. C'est un minorant exact de p,
  // strictement plus fort que la boule de milieu.
  ws->kept.clear();
  int always_inside = 0;
  for (int t = 0; t < site_count; ++t) {
    const SiteMargin& sm = ws->margins[(std::size_t)t];
    if (sm.id != b && sm.llow > 0) ++always_inside;
    if (theta_active && sm.uhigh < theta) continue;  // Theoreme D : jamais interieur
    ws->kept.push_back(sm.id);
  }
  ws->ctr.kept_sites += (long long)ws->kept.size();
  if ((long long)ws->kept.size() > ws->ctr.max_kept) ws->ctr.max_kept = (long long)ws->kept.size();
  const bool disk3 = lane3 && always_inside <= budget3;
  const bool disk4 = lane4 && always_inside <= budget4;
  if (!disk3 && !disk4) {
    ++ws->ctr.anchors_disk_dead;
    return;
  }

  // ------------------------------------------------------- lentille carriers
  // Un carrier d'un support ancre par l'arete MAXIMALE (a,b) verifie
  // |x-a| <= D et |x-b| <= D. Il est en outre exactement sur la sphere, donc
  // sa marge doit changer de signe sur le disque : Llow <= 0 <= Uhigh. Enfin
  // il survit au filtre theta (Theoreme E : un carrier n'est jamais ecarte).
  ws->lens.clear();
  for (int t = 0; t < site_count; ++t) {
    const SiteMargin& sm = ws->margins[(std::size_t)t];
    if (sm.id == b) continue;
    if (theta_active && sm.uhigh < theta) continue;   // hors enveloppe mobile
    if (sm.llow > 0 || sm.uhigh < 0) continue;        // la droite ne coupe pas le disque
    if (sm.d2a > d2) continue;                        // |x-a| <= D
    if ((i64)norm2_i64(sub(pts[(std::size_t)sm.id], pb)) > d2) continue;  // |x-b| <= D
    ws->lens.push_back(sm.id);
  }
  ws->ctr.lens_carriers += (long long)ws->lens.size();
  if ((long long)ws->lens.size() > ws->ctr.max_lens) ws->ctr.max_lens = (long long)ws->lens.size();

  // ------------------------------------------------------------------ q3
  const int nl = (int)ws->lens.size();
  for (int i = 0; disk3 && i < nl; ++i) {
    const int x = ws->lens[(std::size_t)i];
    ++ws->ctr.q3_candidates;
    if (!positive_q3(pa, pb, pts[(std::size_t)x])) { ++ws->ctr.reject_positivity; continue; }
    int ids[4] = {a, b, x, -1};
    sort_ids(ids, 3);
    if (!owns_canonical_max_edge(pts, ids, 3, a, b)) { ++ws->ctr.reject_owner; continue; }
    const BallForm ball = circum_q3(pa, pb, pts[(std::size_t)x]);
    if (!ball.valid) { ++ws->ctr.reject_degenerate; continue; }
    const int p = census(pts, ws->kept, ball, pa, ids, 3, smax - 3, &ws->shell_buf, &ws->ctr);
    if (p < 0) { ++ws->ctr.reject_rank; continue; }
    Support sup{};
    for (int k = 0; k < 3; ++k) sup.id[k] = ids[k];
    sup.q = 3;
    sup.p = p;
    sup.shell_extra = (int)ws->shell_buf.size();
    if (sup.shell_extra > 0) ++ws->ctr.shell_extra_supports;
    ws->out.push_back(sup);
    ++ws->ctr.supports_q3;
  }

  // ------------------------------------------------------------------ q4
  for (int i = 0; disk4 && i < nl; ++i) {
    const int x = ws->lens[(std::size_t)i];
    const P3& px = pts[(std::size_t)x];
    for (int j = i + 1; j < nl; ++j) {
      const int y = ws->lens[(std::size_t)j];
      const P3& py = pts[(std::size_t)y];
      ++ws->ctr.q4_candidates;
      // L'arete maximale doit rester (a,b) : seule |x-y| reste a verifier,
      // les quatre autres aretes sont bornees par la lentille.
      const i64 dxy = (i64)norm2_i64(sub(px, py));
      if (dxy > d2) { ++ws->ctr.reject_owner; continue; }
      const BallForm ball = circum_q4(pa, pb, px, py);
      if (!ball.valid) { ++ws->ctr.reject_degenerate; continue; }
      if (!positive_q4(pa, pb, px, py, ball, pa)) { ++ws->ctr.reject_positivity; continue; }
      int ids[4] = {a, b, x, y};
      sort_ids(ids, 4);
      if (!owns_canonical_max_edge(pts, ids, 4, a, b)) { ++ws->ctr.reject_owner; continue; }
      const int p = census(pts, ws->kept, ball, pa, ids, 4, smax - 4, &ws->shell_buf, &ws->ctr);
      if (p < 0) { ++ws->ctr.reject_rank; continue; }
      Support sup{};
      for (int k = 0; k < 4; ++k) sup.id[k] = ids[k];
      sup.q = 4;
      sup.p = p;
      sup.shell_extra = (int)ws->shell_buf.size();
      if (sup.shell_extra > 0) ++ws->ctr.shell_extra_supports;
      ws->out.push_back(sup);
      ++ws->ctr.supports_q4;
    }
  }
}

// ---------------------------------------------------------------------------
// Producteur complet, parallele par point d'ancre.
// ---------------------------------------------------------------------------
struct RunResult {
  std::vector<Support> supports;
  Counters ctr;
  double wall_front_s = 0;
  double wall_extend_s = 0;
};

void run_range(const MortonLbvh& tree, const std::vector<P3>& pts, int lo, int hi, int smax,
               bool exhaustive_front, bool use_filter, Workspace* ws) {
  const int n = (int)pts.size();
  for (int a = lo; a < hi; ++a) {
    if (exhaustive_front) {
      ws->partners.clear();
      for (int b = a + 1; b < n; ++b) ws->partners.push_back(b);
    } else {
      collect_partners(tree, pts, a, ws);
    }
    if (ws->partners.empty()) continue;
    ws->ctr.candidate_pairs += (long long)ws->partners.size();
    // Rayon de la liste de sites : 1,5 * max D^2.
    i64 dmax2 = 0;
    for (int b : ws->partners)
      dmax2 = std::max(dmax2, (i64)norm2_i64(sub(pts[(std::size_t)b], pts[(std::size_t)a])));
    const i64 r2 = (3 * dmax2 + 1) / 2;
    gather_sites(tree, pts, a, r2, ws);
    // Les partenaires sont traites par distance croissante : le prefixe de
    // sites utile est alors monotone et la liste est relue en sequence.
    std::sort(ws->partners.begin(), ws->partners.end(), [&](int u, int v) {
      const i64 du = (i64)norm2_i64(sub(pts[(std::size_t)u], pts[(std::size_t)a]));
      const i64 dv = (i64)norm2_i64(sub(pts[(std::size_t)v], pts[(std::size_t)a]));
      return du < dv || (du == dv && u < v);
    });
    for (int b : ws->partners) {
      const i64 d2 = (i64)norm2_i64(sub(pts[(std::size_t)b], pts[(std::size_t)a]));
      if (d2 == 0) {
        std::fprintf(stderr, "REFUS : deux PointId a la meme position (%d,%d)\n", a, b);
        std::exit(2);
      }
      ++ws->ctr.anchors_opened;
      extend_anchor(pts, a, b, d2, smax, use_filter, ws);
    }
  }
}

RunResult produce(const std::vector<P3>& pts, int smax, int threads, bool exhaustive_front,
                  bool use_filter) {
  RunResult res{};
  MortonLbvh tree;
  tree.build(pts, 8);
  const int n = (int)pts.size();
  const int nthreads = std::max(1, threads);
  std::vector<Workspace> spaces((std::size_t)nthreads);
  const auto t0 = std::chrono::steady_clock::now();
  {
    std::vector<std::thread> pool;
    std::atomic<int> cursor{0};
    const int chunk = std::max(1, n / (nthreads * 64) + 1);
    for (int t = 0; t < nthreads; ++t) {
      pool.emplace_back([&, t]() {
        for (;;) {
          const int lo = cursor.fetch_add(chunk);
          if (lo >= n) break;
          const int hi = std::min(n, lo + chunk);
          run_range(tree, pts, lo, hi, smax, exhaustive_front, use_filter,
                    &spaces[(std::size_t)t]);
        }
      });
    }
    for (auto& th : pool) th.join();
  }
  const auto t1 = std::chrono::steady_clock::now();
  res.wall_extend_s = std::chrono::duration<double>(t1 - t0).count();
  for (auto& w : spaces) {
    res.ctr.merge(w.ctr);
    res.supports.insert(res.supports.end(), w.out.begin(), w.out.end());
    w.out.clear();
    w.out.shrink_to_fit();
  }
  std::sort(res.supports.begin(), res.supports.end(), [](const Support& u, const Support& v) {
    return support_key(u) < support_key(v);
  });
  return res;
}

// ---------------------------------------------------------------------------
// CLI strict : aucun suffixe accepte, aucun argument excedentaire ignore.
// ---------------------------------------------------------------------------
bool parse_ll(const char* s, long long* out) {
  if (s == nullptr || *s == '\0') return false;
  char* end = nullptr;
  errno = 0;
  const long long v = std::strtoll(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0') return false;
  *out = v;
  return true;
}

[[noreturn]] void refuse(const char* msg) {
  std::fprintf(stderr, "REFUS : %s\n", msg);
  std::exit(2);
}

}  // namespace

int main(int argc, char** argv) {
  long long n = 200;
  long long coord = -1;
  long long seed = 1;
  long long smax = 11;
  long long threads = 1;
  CloudFamily family = CloudFamily::kUniform;
  bool verify = false;
  bool no_filter = false;
  bool emit = false;
  long long min_supports = 0;
  long long min_anchors = 0;
  long long min_prunes = 0;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto eq = arg.find('=');
    const std::string key = (eq == std::string::npos) ? arg : arg.substr(0, eq);
    const std::string val = (eq == std::string::npos) ? std::string() : arg.substr(eq + 1);
    long long parsed = 0;
    if (key == "--points") { if (!parse_ll(val.c_str(), &parsed)) refuse("--points invalide"); n = parsed; }
    else if (key == "--coord") { if (!parse_ll(val.c_str(), &parsed)) refuse("--coord invalide"); coord = parsed; }
    else if (key == "--seed") { if (!parse_ll(val.c_str(), &parsed)) refuse("--seed invalide"); seed = parsed; }
    else if (key == "--smax") { if (!parse_ll(val.c_str(), &parsed)) refuse("--smax invalide"); smax = parsed; }
    else if (key == "--threads") { if (!parse_ll(val.c_str(), &parsed)) refuse("--threads invalide"); threads = parsed; }
    else if (key == "--min-supports") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-supports invalide"); min_supports = parsed; }
    else if (key == "--min-anchors") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-anchors invalide"); min_anchors = parsed; }
    else if (key == "--min-prunes") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-prunes invalide"); min_prunes = parsed; }
    else if (key == "--verify") { verify = true; }
    else if (key == "--no-filter") { no_filter = true; }
    else if (key == "--emit-supports") { emit = true; }
    else if (key == "--family") {
      if (val == "uniform") family = CloudFamily::kUniform;
      else if (val == "terrain") family = CloudFamily::kTerrain;
      else if (val == "scanline_single_pass") family = CloudFamily::kScanlineSinglePass;
      else if (val == "scanline_overlap_multiecho") family = CloudFamily::kScanlineOverlapMultiecho;
      else refuse("--family inconnue");
    } else {
      refuse("argument inconnu");
    }
  }
  if (n < 2 || n > 65535) refuse("--points hors du profil dense u16 [2, 65535]");
  if (smax < 4 || smax > 24) refuse("--smax hors bornes");
  if (threads < 1 || threads > 256) refuse("--threads hors bornes");
  if (coord < 0) coord = mhgp3v::cloud_family_default_coord(family, (int)n);
  if (coord < 2 || coord > 65536) refuse("--coord hors bornes");

  const std::vector<P3> pts = mhgp3v::make_family_cloud(family, (int)n, (int)coord, seed);
  if ((long long)pts.size() != n) refuse("la famille n'a pas rendu le cardinal demande");

  const auto t_start = std::chrono::steady_clock::now();
  RunResult res = produce(pts, (int)smax, (int)threads, false, !no_filter);
  const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - t_start).count();

  std::printf("AnchorSourceReceipt-v1\n");
  std::printf("cadre phase=exploration_v3_hors_registre backend=cpu_reference"
              " profile=quantized_u16_input_only mode=proposition_math_non_recue"
              " public_status=not_claimed\n");
  std::printf("cloud family=%s n=%lld coord=%lld seed=%lld smax=%lld threads=%lld\n",
              mhgp3v::cloud_family_name(family), n, coord, seed, smax, threads);
  const Counters& c = res.ctr;
  std::printf("front node_visits=%lld witness_calls=%lld witness_visits=%lld prunes=%lld"
              " candidate_pairs=%lld anchors=%lld lane_dead=%lld etendues=%lld"
              " disk_dead=%lld\n",
              c.front_node_visits, c.front_witness_calls, c.front_witness_visits,
              c.front_witness_prunes, c.candidate_pairs, c.anchors_opened,
              c.anchors_lane_dead, c.anchors_extended, c.anchors_disk_dead);
  std::printf("extend gather_visits=%lld site_evaluations=%lld kept_sites=%lld lens_carriers=%lld"
              " q3_candidates=%lld q4_candidates=%lld interior_tests=%lld\n",
              c.site_gather_visits, c.site_evaluations, c.kept_sites, c.lens_carriers,
              c.q3_candidates, c.q4_candidates, c.interior_tests);
  std::printf("rejets positivite=%lld owner=%lld rang=%lld degenere=%lld\n",
              c.reject_positivity, c.reject_owner, c.reject_rank, c.reject_degenerate);
  std::printf("supports q2=%lld q3=%lld q4=%lld total=%lld extra_shell=%lld\n",
              c.supports_q2, c.supports_q3, c.supports_q4,
              c.supports_q2 + c.supports_q3 + c.supports_q4, c.shell_extra_supports);
  std::printf("high_water site_list=%lld kept=%lld lens=%lld\n", c.max_site_list, c.max_kept,
              c.max_lens);
  std::printf("temps wall_s=%.6f\n", wall);

  // Identite exacte-une-fois : aucune cle ne doit apparaitre deux fois.
  long long duplicates = 0;
  for (std::size_t i = 1; i < res.supports.size(); ++i)
    if (support_key(res.supports[i - 1]) == support_key(res.supports[i])) ++duplicates;
  std::printf("identite occurrences=%zu cles_uniques=%zu doublons=%lld\n", res.supports.size(),
              res.supports.size() - (std::size_t)duplicates, duplicates);
  if (duplicates != 0) {
    std::fprintf(stderr, "REFUS : l'emission exacte-une-fois est violee\n");
    return 3;
  }

  if (emit) {
    for (const Support& s : res.supports) {
      std::printf("S q=%d p=%d extra=%d ids=", s.q, s.p, s.shell_extra);
      for (int i = 0; i < s.q; ++i) std::printf("%s%d", i ? "," : "", s.id[i]);
      std::printf("\n");
    }
  }

  if (verify) {
    RunResult ref = produce(pts, (int)smax, 1, true, false);
    bool same = ref.supports.size() == res.supports.size();
    if (same)
      for (std::size_t i = 0; i < ref.supports.size(); ++i)
        if (support_key(ref.supports[i]) != support_key(res.supports[i]) ||
            ref.supports[i].p != res.supports[i].p) { same = false; break; }
    std::printf("verify exhaustif=%zu produit=%zu accord=%s\n", ref.supports.size(),
                res.supports.size(), same ? "OUI" : "NON");
    if (!same) {
      std::fprintf(stderr, "DESACCORD : le certificat de front a supprime des supports\n");
      return 1;
    }
  }

  long long total = c.supports_q2 + c.supports_q3 + c.supports_q4;
  if (total < min_supports) {
    std::fprintf(stderr, "REFUS : plancher de supports %lld > %lld\n", min_supports, total);
    return 3;
  }
  if (c.anchors_opened < min_anchors) {
    std::fprintf(stderr, "REFUS : plancher d'ancres %lld > %lld\n", min_anchors, c.anchors_opened);
    return 3;
  }
  if (c.front_witness_prunes < min_prunes) {
    std::fprintf(stderr, "REFUS : plancher de prunes %lld > %lld\n", min_prunes,
                 c.front_witness_prunes);
    return 3;
  }
  return 0;
}
