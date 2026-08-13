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
#include "prototype/anchor_pipeline.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/morton_lbvh.hpp"

namespace {

// Granularite des feuilles du LBVH. Elle decide la finesse du certificat de
// front : une feuille plus fine donne une boite plus petite, donc un rayon
// temoin plus grand et une coupure plus serree, au prix de plus de noeuds.
int g_leaf_size = 8;

using mhgp::i128;
using mhgp::i64;
using mhgp::P3;
using mhgp3v::CloudFamily;
using mhgp3v::LbvhNode;
using mhgp3v::MortonLbvh;
using namespace mhgp3v::anchor;

// ---------------------------------------------------------------------------
// MUTANTS. Chacun casse UNE decision exacte. Le sujet mute est compare a la
// reference exhaustive NON mutee : toute difference tue le mutant (code 4).
// Un mutant sans juge ne prouve rien : `--inject` exige `--verify`.
// ---------------------------------------------------------------------------
enum class Inject {
  kNone,
  kFrontNoExt,        // boule temoin sans la soustraction ext/4 : coupe a tort
  kFrontQ4Only,       // ferme le produit des que la seule lane q4 est morte
  kThetaNoFailOpen,   // filtre d'enveloppe sur Llow au lieu de Uhigh
  kOwnerMinEdge,      // ancre sur l'arete minimale au lieu de la maximale
  kOwnerNoTiebreak,   // pas de tie-break entre aretes de longueur maximale
  kLensStrict,        // lentille en < D au lieu de <= D : perd les ex aequo
  kCensusNoAlways,    // census ignorant les toujours-interieurs
  kPositivityLoose,   // accepte un centre sur une face du tetraedre
  kSmaxFixedThresholds,  // fige les seuils 10/9/8 et l'enveloppe au rang neuf
};

const char* inject_name(Inject m) {
  switch (m) {
    case Inject::kNone: return "none";
    case Inject::kFrontNoExt: return "front-no-ext";
    case Inject::kFrontQ4Only: return "front-q4-only";
    case Inject::kThetaNoFailOpen: return "theta-no-fail-open";
    case Inject::kOwnerMinEdge: return "owner-min-edge";
    case Inject::kOwnerNoTiebreak: return "owner-no-tiebreak";
    case Inject::kLensStrict: return "lens-strict";
    case Inject::kCensusNoAlways: return "census-no-always-inside";
    case Inject::kPositivityLoose: return "positivity-loose";
    case Inject::kSmaxFixedThresholds: return "smax-fixed-thresholds";
  }
  return "?";
}


// ---------------------------------------------------------------------------
// Compteurs de travail. Ils ferment les gates W_front et W_extend : aucun
// n'est optionnel, aucun n'est estime.
// ---------------------------------------------------------------------------
struct Counters {
  long long front_node_visits = 0;      // noeuds ouverts par la requete d'ancre
  long long front_witness_calls = 0;    // boules temoins evaluees
  long long front_witness_visits = 0;   // noeuds visites par ces boules
  long long front_witness_prunes = 0;   // noeuds fermes par dix temoins
  long long front_witness_skipped = 0;  // tentatives evitees par la garde de densite
  long long front_mass_closed = 0;      // points fermes par un noeud prune
  long long lane_probe_visits = 0;      // noeuds visites par les sondes de lane
  long long candidate_pairs = 0;        // paires (a,b) survivantes, b > a
  long long anchors_opened = 0;         // ancres candidates ouvertes
  long long anchors_lane_dead = 0;      // ancres tuees par les boules de milieu
  long long anchors_extended = 0;       // ancres passant les boules de milieu
  long long anchors_disk_dead = 0;      // ancres tuees par les toujours-interieurs
  long long anchors_budget_early = 0;   // dont : tuees avant la fin des marges
  long long site_gather_visits = 0;     // noeuds visites par les listes de sites
  long long site_evaluations = 0;       // sites charges dans une enveloppe
  long long kept_sites = 0;             // sites survivant au filtre d'enveloppe
  long long theta_only_prunes_on_live = 0;  // doit etre identiquement nul
  long long theta_anchors_active = 0;   // ancres vivantes ou theta a ete arme
  long long lens_carriers = 0;          // carriers retenus par la lentille
  long long q3_candidates = 0;
  long long q4_pairs_walked = 0;
  long long q4_candidates = 0;
  long long interior_tests = 0;         // predicats de puissance evalues
  long long reject_positivity = 0;
  long long reject_non_acute = 0;
  long long reject_owner = 0;
  long long reject_rank = 0;
  long long reject_degenerate = 0;
  long long supports_q2 = 0;
  long long supports_q3 = 0;
  long long supports_q4 = 0;
  long long shell_extra_supports = 0;   // supports dont U_B depasse S
  long long max_site_list = 0;
  long long max_partners = 0;
  long long max_kept = 0;
  long long max_lens = 0;

  void merge(const Counters& o) {
    front_node_visits += o.front_node_visits;
    front_witness_calls += o.front_witness_calls;
    front_witness_visits += o.front_witness_visits;
    front_witness_prunes += o.front_witness_prunes;
    front_witness_skipped += o.front_witness_skipped;
    front_mass_closed += o.front_mass_closed;
    lane_probe_visits += o.lane_probe_visits;
    candidate_pairs += o.candidate_pairs;
    anchors_opened += o.anchors_opened;
    anchors_lane_dead += o.anchors_lane_dead;
    anchors_extended += o.anchors_extended;
    anchors_disk_dead += o.anchors_disk_dead;
    anchors_budget_early += o.anchors_budget_early;
    site_gather_visits += o.site_gather_visits;
    site_evaluations += o.site_evaluations;
    kept_sites += o.kept_sites;
    theta_only_prunes_on_live += o.theta_only_prunes_on_live;
    theta_anchors_active += o.theta_anchors_active;
    lens_carriers += o.lens_carriers;
    q3_candidates += o.q3_candidates;
    q4_pairs_walked += o.q4_pairs_walked;
    q4_candidates += o.q4_candidates;
    interior_tests += o.interior_tests;
    reject_positivity += o.reject_positivity;
    reject_non_acute += o.reject_non_acute;
    reject_owner += o.reject_owner;
    reject_rank += o.reject_rank;
    reject_degenerate += o.reject_degenerate;
    supports_q2 += o.supports_q2;
    supports_q3 += o.supports_q3;
    supports_q4 += o.supports_q4;
    shell_extra_supports += o.shell_extra_supports;
    max_site_list = std::max(max_site_list, o.max_site_list);
    max_partners = std::max(max_partners, o.max_partners);
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

// Trois boules temoins communes, une par lane, autour du meme centre quadruple
// q0 = 2a + lo + hi. Pour toute paire (a,b) du produit, le milieu m verifie
// |m - z0| <= ext/4 ou ext est la diagonale de la boite. Donc, avec
//   R_q = Dmin / c_q - ext/4,      c_2 = 2, c_3 = sqrt(12), c_4 = sqrt(15),
// la boule { z : |4z - q0| < 4 R_q } est incluse, ouverte, dans la boule de
// milieu de lane q de TOUTE paire du produit. Les seuils 10/9/8 s'appliquent
// alors separement, et le produit n'est ferme que si les trois lanes le sont.
//
// Les rayons sont rendus ENTIERS par la racine entiere et une minoration
// rationnelle stricte de 4/c_q : 2, 11547/10000 < 4/sqrt(12) et
// 10327/10000 < 4/sqrt(15). Le rayon publie est donc toujours inferieur ou
// egal au rayon vrai — le certificat ne peut jamais couper a tort.
struct WitnessBall {
  i64 q0[3] = {0, 0, 0};
  i64 tau[3] = {0, 0, 0};  // rayons carres quadruples, lanes q2/q3/q4
  bool usable = false;
};

inline WitnessBall make_witness_ball(const LbvhNode& nd, const P3& pa, Inject inject) {
  WitnessBall wb{};
  for (int d = 0; d < 3; ++d) wb.q0[d] = 2 * pa.x * 0;  // initialise, rempli ci-dessous
  const i64 c[3] = {pa.x, pa.y, pa.z};
  for (int d = 0; d < 3; ++d) wb.q0[d] = 2 * c[d] + nd.lo[d] + nd.hi[d];
  const i64 g = aabb_min_dist2(nd, pa);
  if (g <= 0) return wb;
  const i64 dmin = isqrt_i128(g);                 // <= Dmin
  i64 ext = isqrt_i128(aabb_extent2(nd)) + 1;  // >= diagonale
  if (inject == Inject::kFrontNoExt) ext = 0;
  const i64 num[3] = {20000, 11547, 10327};       // 4/c_q * 10000, minore
  for (int q = 0; q < 3; ++q) {
    const i64 l = (num[q] * dmin) / 10000 - ext;  // <= 4 R_q
    wb.tau[q] = (l > 0) ? l * l : 0;
    if (l > 0) wb.usable = true;
  }
  return wb;
}

inline i64 witness_far2(const LbvhNode& nd, const WitnessBall& wb) {
  i64 s = 0;
  for (int d = 0; d < 3; ++d) {
    const i64 lo4 = 4 * nd.lo[d] - wb.q0[d];
    const i64 hi4 = 4 * nd.hi[d] - wb.q0[d];
    const i64 far = std::max(lo4 < 0 ? -lo4 : lo4, hi4 < 0 ? -hi4 : hi4);
    s += far * far;
  }
  return s;
}
inline i64 witness_near2(const LbvhNode& nd, const WitnessBall& wb) {
  i64 s = 0;
  for (int d = 0; d < 3; ++d) {
    const i64 lo4 = 4 * nd.lo[d];
    const i64 hi4 = 4 * nd.hi[d];
    i64 g = 0;
    if (wb.q0[d] < lo4) g = lo4 - wb.q0[d];
    else if (wb.q0[d] > hi4) g = wb.q0[d] - hi4;
    s += g * g;
  }
  return s;
}

// ---------------------------------------------------------------------------
// SONDE DE LANES : les trois boules de milieu en une seule descente.
//
// Avec q0 = 2(a+b) et s = |4z - q0|^2 = 16 |z-m|^2, les trois temoins
// universels du Lemme B s'ecrivent sans division :
//   q2 (boule diametrale, rayon D/2)     :  4 s < 16 D2
//   q3 (rayon D/sqrt(12))                : 12 s < 16 D2
//   q4 (rayon D/sqrt(15))                : 15 s < 16 D2
// La sortie anticipee tombe des que les TROIS lanes sont mortes. Le compte q2
// strict est en outre le census exact du support q2, et l'egalite 4s = 16 D2
// designe exactement son extra-shell.
// Bornes : |4z - q0| <= 524280, donc s <= 8,3e11 et 15 s <= 1,3e13 en i64.
// ---------------------------------------------------------------------------
struct LaneProbe {
  int p2 = 0, extra2 = 0, n3 = 0, n4 = 0;
};

void lane_probe(const MortonLbvh& tree, const std::vector<P3>& pts, int a, int b, i64 d2,
                int budget2, int budget3, int budget4, LaneProbe* out, Counters* ctr) {
  const P3& pa = pts[(std::size_t)a];
  const P3& pb = pts[(std::size_t)b];
  const i64 q0[3] = {2 * (pa.x + pb.x), 2 * (pa.y + pb.y), 2 * (pa.z + pb.z)};
  const i64 lim = 16 * d2;
  *out = LaneProbe{};
  int stack[64];
  int sp = 0;
  stack[sp++] = 0;
  while (sp > 0) {
    const int ni = stack[--sp];
    const LbvhNode& nd = tree.nodes[(std::size_t)ni];
    ++ctr->lane_probe_visits;
    i64 smin = 0;
    for (int d = 0; d < 3; ++d) {
      const i64 lo4 = 4 * nd.lo[d];
      const i64 hi4 = 4 * nd.hi[d];
      i64 g = 0;
      if (q0[d] < lo4) g = lo4 - q0[d];
      else if (q0[d] > hi4) g = q0[d] - hi4;
      smin += g * g;
    }
    if (4 * smin > lim) continue;  // hors de la boule diametrale fermee
    if (nd.left < 0) {
      for (int t = nd.begin; t < nd.end; ++t) {
        const int id = tree.order[(std::size_t)t];
        if (id == a || id == b) continue;
        const P3& z = pts[(std::size_t)id];
        i64 s = 0;
        for (int d = 0; d < 3; ++d) {
          const i64 g = 4 * (d == 0 ? z.x : (d == 1 ? z.y : z.z)) - q0[d];
          s += g * g;
        }
        const i64 s4 = 4 * s;
        if (s4 > lim) continue;
        if (s4 == lim) { ++out->extra2; continue; }
        ++out->p2;
        if (12 * s < lim) {
          ++out->n3;
          if (15 * s < lim) ++out->n4;
        }
        if (out->p2 > budget2 && out->n3 > budget3 && out->n4 > budget4) return;
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

// Ferme-t-on le produit entier ? Le compte s'arrete des que les trois lanes
// ont atteint leur seuil ; son cout est donc borne par les seuils, pas par la
// population du voisinage.
bool witness_closes(const MortonLbvh& tree, const std::vector<P3>& pts, const WitnessBall& wb,
                    int smax, Inject inject, Counters* ctr) {
  int cnt[3] = {0, 0, 0};
  int need[3] = {lane_death_threshold(smax, 2), lane_death_threshold(smax, 3),
                 lane_death_threshold(smax, 4)};
  if (inject == Inject::kSmaxFixedThresholds) {
    need[0] = kThresholdQ2;
    need[1] = kThresholdQ3;
    need[2] = kThresholdQ4;
  }
  if (inject == Inject::kFrontQ4Only) need[0] = need[1] = 0;
  // Le masque porte les lanes qui ont encore besoin de ce sous-arbre. Une lane
  // creditee d'un noeud entier en sort : aucun double comptage n'est possible.
  struct Frame { int node; unsigned mask; };
  Frame stack[64];
  int sp = 0;
  stack[sp++] = Frame{0, 7u};
  while (sp > 0) {
    const Frame fr = stack[--sp];
    unsigned mask = fr.mask;
    const LbvhNode& nd = tree.nodes[(std::size_t)fr.node];
    ++ctr->front_witness_visits;
    const i64 near2 = witness_near2(nd, wb);
    const i64 far2 = witness_far2(nd, wb);
    for (int q = 0; q < 3; ++q) {
      if ((mask & (1u << q)) == 0) continue;
      if (cnt[q] >= need[q]) { mask &= ~(1u << q); continue; }
      if (near2 >= wb.tau[q]) { mask &= ~(1u << q); continue; }  // noeud hors boule
      if (far2 < wb.tau[q]) {                                    // noeud entierement dedans
        cnt[q] += nd.end - nd.begin;
        mask &= ~(1u << q);
      }
    }
    if (cnt[0] >= need[0] && cnt[1] >= need[1] && cnt[2] >= need[2]) return true;
    if (mask == 0) continue;
    if (nd.left < 0) {
      for (int t = nd.begin; t < nd.end; ++t) {
        const int id = tree.order[(std::size_t)t];
        const P3& z = pts[(std::size_t)id];
        const i64 c[3] = {z.x, z.y, z.z};
        i64 s = 0;
        for (int d = 0; d < 3; ++d) {
          const i64 g = 4 * c[d] - wb.q0[d];
          s += g * g;
        }
        for (int q = 0; q < 3; ++q)
          if ((mask & (1u << q)) != 0 && s < wb.tau[q]) ++cnt[q];
        if (cnt[0] >= need[0] && cnt[1] >= need[1] && cnt[2] >= need[2]) return true;
      }
      continue;
    }
    stack[sp++] = Frame{nd.left, mask};
    stack[sp++] = Frame{nd.right, mask};
    if (sp > 60) {
      std::fprintf(stderr, "REFUS : pile de parcours saturee\n");
      std::exit(3);
    }
  }
  return cnt[0] >= need[0] && cnt[1] >= need[1] && cnt[2] >= need[2];
}

// ---------------------------------------------------------------------------
// Etat de travail par thread.
// ---------------------------------------------------------------------------
struct Neighbour {
  i64 d2 = 0;
  int id = 0;
  bool operator<(const Neighbour& o) const { return d2 < o.d2 || (d2 == o.d2 && id < o.id); }
};

struct SurvivingAnchor {
  int b = 0;
  i64 d2 = 0;
  bool lane3 = false, lane4 = false;
};

struct Workspace {
  bool store = true;
  Inject inject = Inject::kNone;
  int fixture = 0;
  bool engine_pipeline = false;
  bool density_guard = false;
  std::vector<int> partners;
  std::vector<SurvivingAnchor> survivors;        // b > a survivants
  std::vector<Neighbour> sites;     // sites tries par distance a `a`
  std::vector<SiteMargin> margins;  // marges affines sur le disque de Jung
  std::vector<int> kept;            // sites survivant au filtre d'enveloppe
  std::vector<int> lens;            // carriers : lentille ET enveloppe
  std::vector<unsigned char> acute; // face abx positive, parallele a lens
  std::vector<Support> out;         // supports emis
  std::vector<int> shell_buf;
  std::vector<i64> select_buf;
  Counters ctr;
};

// Parcours d'ancre : collecte les partenaires candidats de `a`.
void collect_partners(const MortonLbvh& tree, const std::vector<P3>& pts, int a, int smax,
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
    const WitnessBall wb = make_witness_ball(nd, pa, ws->inject);
    if (wb.usable) {
      // GARDE DE DENSITE — HEURISTIQUE OPT-IN, JAMAIS UN CERTIFICAT. Elle
      // estime la population de la boule temoin par la densite du noeud
      // partenaire, qui ne borne pas cette population ailleurs : sauter une
      // tentative peut donc PERDRE un prune, jamais inventer ni supprimer un
      // support. Elle est desarmee par defaut et identique dans les deux
      // moteurs, sans quoi leur ledger de front ne peut pas etre compare.
      bool engage = true;
      if (ws->density_guard) {
        const i128 r = (i128)isqrt_i128((i128)wb.tau[0]) / 4;
        const i128 e = (i128)isqrt_i128(aabb_extent2(nd)) + 1;
        const i128 mass = (i128)(nd.end - nd.begin);
        const i128 lhs = 2177 * r * r * r * mass;
        const i128 rhs = 100 * (i128)lane_death_threshold(smax, 2) * e * e * e;
        if (lhs < rhs) {
          engage = false;
          ++ws->ctr.front_witness_skipped;
        }
      }
      if (engage) {
        ++ws->ctr.front_witness_calls;
        if (witness_closes(tree, pts, wb, smax, ws->inject, &ws->ctr)) {
          ++ws->ctr.front_witness_prunes;
          ws->ctr.front_mass_closed += nd.end - nd.begin;
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
bool owns_canonical_max_edge(const std::vector<P3>& pts, const int* ids, int q, int a, int b,
                            Inject inject) {
  const bool want_min = (inject == Inject::kOwnerMinEdge);
  i64 best2 = want_min ? ((i64)1 << 62) : -1;
  int bi = -1, bj = -1;
  for (int i = 0; i < q; ++i)
    for (int j = i + 1; j < q; ++j) {
      const i64 d2 = (i64)norm2_i64(sub(pts[(std::size_t)ids[i]], pts[(std::size_t)ids[j]]));
      const int lo = std::min(ids[i], ids[j]);
      const int hi = std::max(ids[i], ids[j]);
      const bool better = want_min ? (d2 < best2) : (d2 > best2);
      const bool tie = (d2 == best2) && (lo < bi || (lo == bi && hi < bj));
      if (better || tie) {
        best2 = d2;
        bi = lo;
        bj = hi;
      }
    }
  if (inject == Inject::kOwnerNoTiebreak) {
    // Sans regle canonique, toute arete de longueur maximale se croit owner :
    // un support a plusieurs aretes maximales est alors emis plusieurs fois.
    const i64 dab = (i64)norm2_i64(sub(pts[(std::size_t)a], pts[(std::size_t)b]));
    return dab == best2;
  }
  return bi == std::min(a, b) && bj == std::max(a, b);
}

// ---------------------------------------------------------------------------
// Census exact : compte les interieurs stricts avec sortie anticipee, et
// collecte le shell. `budget` est le nombre maximal d'interieurs admissibles.
// Retour : -1 si le budget est depasse, sinon p.
// ---------------------------------------------------------------------------
// `always_inside` sites (Llow > 0) sont strictement interieurs a TOUTE boule
// du disque : ils sont comptes sans etre testes. Les sites `Uhigh < 0` sont
// strictement exterieurs : ils ne sont jamais charges. Seuls les sites dont la
// marge change de signe demandent le predicat exact.
int census(const std::vector<P3>& pts, const std::vector<int>& crossing, int always_inside,
           const BallForm& ball, const P3& origin, const int* ids, int q, int budget,
           std::vector<int>* shell, Counters* ctr) {
  int p = always_inside;
  if (p > budget) return -1;
  shell->clear();
  for (int id : crossing) {
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
void extend_anchor(const std::vector<P3>& pts, int a, int b, i64 d2, int smax, bool lane3,
                   bool lane4, bool theta_audit, Workspace* ws) {
  const P3& pa = pts[(std::size_t)a];
  const P3& pb = pts[(std::size_t)b];
  const int budget3 = smax - 3, budget4 = smax - 4;
  // Prefixe des sites utiles : |z-a|^2 <= 1,5 D^2. Tout interieur d'une boule
  // ancree par l'arete maximale (a,b) y est, puisque R <= D sqrt(3/8).
  int site_count = 0;
  {
    const std::vector<Neighbour>& s = ws->sites;
    while (site_count < (int)s.size() && 2 * s[(std::size_t)site_count].d2 <= 3 * d2) ++site_count;
  }
  if (site_count > ws->ctr.max_site_list) ws->ctr.max_site_list = site_count;
  ws->margins.clear();
  ws->margins.resize((std::size_t)site_count);

  // ------------------------------ passe B : amplitude exacte et filtre theta
  // Sur le disque de Jung q4, l'amplitude de F_z vaut sqrt(2Q) avec
  // Q = |U|^2 D2 - (U.d)^2. La racine entiere donne les bornes ENTIERES
  // Llow <= L et Uhigh >= U*, donc un filtre fail-open.
  //
  // MORT PAR BUDGET, AU PLUS TOT. `Llow>0` certifie que le site est
  // strictement interieur a TOUTE boule admissible de l'ancre. Un
  // SOUS-ENSEMBLE de tels sites est donc un minorant exact de `p` : des qu'il
  // depasse le budget de la derniere lane vivante, l'ancre est morte et la
  // decision ne peut plus changer, puisque le compte ne fait que croitre. La
  // sortie est identique — c'est la MEME decision, prise plus tot.
  //
  // Elle est efficace parce que `sites` est trie par distance a `a` : avec
  // `e=z-a`, `d=b-a`, `r=|e|` et `phi` l'angle de `e` a `d`, on a exactement
  // `g = 4 e.d - 4r^2` et `Q = 4 r^2 D^2 sin^2(phi)`, donc `Llow>0` equivaut a
  // `D (4 cos(phi) - 2 sqrt(2) sin(phi)) > 4r + 1/r`. Pour `D` grand devant
  // `r`, la condition devient `tan(phi) < sqrt(2)`, soit
  // `phi < 54,7356 degres` : sur une ancre LONGUE, tout voisin proche de `a`
  // dans ce cone certifie. Les premiers sites lus sont donc precisement ceux
  // qui tuent l'ancre.
  const int death_budget = lane3 ? budget3 : budget4;
  int certified_inside = 0;
  int evaluated = 0;
  {
    const P3 d = sub(pb, pa);
    const P3 ab{pa.x + pb.x, pa.y + pb.y, pa.z + pb.z};
    for (int t = 0; t < site_count; ++t) {
      SiteMargin& sm = ws->margins[(std::size_t)t];
      const int id = ws->sites[(std::size_t)t].id;
      const P3& pz = pts[(std::size_t)id];
      const P3 u{2 * pz.x - ab.x, 2 * pz.y - ab.y, 2 * pz.z - ab.z};
      const i128 uu = norm2_i64(u);
      const i128 ud = dot_i64(u, d);
      sm.id = id;
      sm.d2a = ws->sites[(std::size_t)t].d2;
      sm.g = (i64)(i128(d2) - uu);
      const i64 s = isqrt_i128(2 * (uu * i128(d2) - ud * ud)) + 1;
      sm.llow = sm.g - s;
      sm.uhigh = sm.g + s;
      ++evaluated;
      if (id != b && sm.llow > 0 && ++certified_inside > death_budget) {
        ws->ctr.site_evaluations += evaluated;
        ++ws->ctr.anchors_disk_dead;
        ++ws->ctr.anchors_budget_early;
        return;
      }
    }
  }
  ws->ctr.site_evaluations += evaluated;
  i64 theta = 0;
  bool theta_active = false;
  if (theta_audit) {
    ws->select_buf.clear();
    for (int t = 0; t < site_count; ++t)
      if (ws->margins[(std::size_t)t].id != b)
        ws->select_buf.push_back(ws->margins[(std::size_t)t].llow);
    const int depth =
        (ws->inject == Inject::kSmaxFixedThresholds) ? kThresholdQ3 : envelope_depth(smax);
    if ((int)ws->select_buf.size() >= depth) {
      std::nth_element(ws->select_buf.begin(), ws->select_buf.begin() + (depth - 1),
                       ws->select_buf.end(), std::greater<i64>());
      theta = ws->select_buf[(std::size_t)(depth - 1)];
      theta_active = true;
    }
  }
  // `always_inside` : Llow > 0 signifie F_z > 0 sur TOUT le disque, donc un
  // interieur strict de toute boule admissible. C'est un minorant exact de p,
  // strictement plus fort que la boule de milieu.
  ws->kept.clear();
  int always_inside = 0;
  long long theta_extra = 0;
  for (int t = 0; t < site_count; ++t) {
    const SiteMargin& sm = ws->margins[(std::size_t)t];
    if (sm.id == b) continue;
    if (sm.llow > 0) { ++always_inside; continue; }   // interieur de toute boule
    if (sm.uhigh < 0) continue;                       // exterieur a toute boule
    const i64 probe = (ws->inject == Inject::kThetaNoFailOpen) ? sm.llow : sm.uhigh;
    if (theta_active && probe < theta) { ++theta_extra; continue; }
    ws->kept.push_back(sm.id);
  }
  ws->ctr.kept_sites += (long long)ws->kept.size();
  if ((long long)ws->kept.size() > ws->ctr.max_kept) ws->ctr.max_kept = (long long)ws->kept.size();
  const int effective_always =
      (ws->inject == Inject::kCensusNoAlways) ? 0 : always_inside;
  const bool disk3 = lane3 && always_inside <= budget3;
  const bool disk4 = lane4 && always_inside <= budget4;
  if (disk3 || disk4) {
    ws->ctr.theta_only_prunes_on_live += theta_extra;
    if (theta_active) ++ws->ctr.theta_anchors_active;
  }
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
  for (int id : ws->kept) {
    const P3& px = pts[(std::size_t)id];
    const i64 lim = (ws->inject == Inject::kLensStrict) ? d2 - 1 : d2;
    if ((i64)norm2_i64(sub(px, pa)) > lim) continue;  // |x-a| <= D
    if ((i64)norm2_i64(sub(px, pb)) > lim) continue;  // |x-b| <= D
    ws->lens.push_back(id);
  }
  ws->ctr.lens_carriers += (long long)ws->lens.size();
  if ((long long)ws->lens.size() > ws->ctr.max_lens) ws->ctr.max_lens = (long long)ws->lens.size();

  // ------------------------------------------------------------------ q3
  // La face `abx` positive est marquee : le Theoreme 5 garantit qu'un q4
  // positif d'arete maximale (a,b) possede AU MOINS UNE face positive parmi
  // `abx` et `aby`. La boucle q4 s'y restreint sans perdre un support.
  const int nl = (int)ws->lens.size();
  ws->acute.assign((std::size_t)nl, 0);
  for (int i = 0; i < nl; ++i)
    ws->acute[(std::size_t)i] = positive_q3(pa, pb, pts[(std::size_t)ws->lens[(std::size_t)i]]) ? 1 : 0;
  for (int i = 0; disk3 && i < nl; ++i) {
    const int x = ws->lens[(std::size_t)i];
    ++ws->ctr.q3_candidates;
    if (ws->acute[(std::size_t)i] == 0) { ++ws->ctr.reject_positivity; continue; }
    int ids[4] = {a, b, x, -1};
    sort_ids(ids, 3);
    if (!owns_canonical_max_edge(pts, ids, 3, a, b, ws->inject)) { ++ws->ctr.reject_owner; continue; }
    const BallForm ball = circum_q3(pa, pb, pts[(std::size_t)x]);
    if (!ball.valid) { ++ws->ctr.reject_degenerate; continue; }
    const int p = census(pts, ws->kept, effective_always, ball, pa, ids, 3, budget3,
                         &ws->shell_buf, &ws->ctr);
    if (p < 0) { ++ws->ctr.reject_rank; continue; }
    Support sup{};
    for (int k = 0; k < 3; ++k) sup.id[k] = ids[k];
    sup.q = 3;
    sup.p = p;
    sup.shell_extra = (int)ws->shell_buf.size();
    if (sup.shell_extra > 0) ++ws->ctr.shell_extra_supports;
    if (ws->store) ws->out.push_back(sup);
    ++ws->ctr.supports_q3;
  }

  // ------------------------------------------------------------------ q4
  for (int i = 0; disk4 && i < nl; ++i) {
    const int x = ws->lens[(std::size_t)i];
    const P3& px = pts[(std::size_t)x];
    for (int j = i + 1; j < nl; ++j) {
      ++ws->ctr.q4_pairs_walked;
      const int y = ws->lens[(std::size_t)j];
      if (ws->acute[(std::size_t)i] == 0 && ws->acute[(std::size_t)j] == 0) {
        ++ws->ctr.reject_non_acute;
        continue;
      }
      const P3& py = pts[(std::size_t)y];
      ++ws->ctr.q4_candidates;
      // L'arete maximale doit rester (a,b) : seule |x-y| reste a verifier,
      // les quatre autres aretes sont bornees par la lentille.
      const i64 dxy = (i64)norm2_i64(sub(px, py));
      if (dxy > d2) { ++ws->ctr.reject_owner; continue; }
      const BallForm ball = circum_q4(pa, pb, px, py);
      if (!ball.valid) { ++ws->ctr.reject_degenerate; continue; }
      if (!positive_q4(pa, pb, px, py, ball, pa, ws->inject == Inject::kPositivityLoose)) {
        ++ws->ctr.reject_positivity;
        continue;
      }
      int ids[4] = {a, b, x, y};
      sort_ids(ids, 4);
      if (!owns_canonical_max_edge(pts, ids, 4, a, b, ws->inject)) { ++ws->ctr.reject_owner; continue; }
      const int p = census(pts, ws->kept, effective_always, ball, pa, ids, 4, budget4,
                            &ws->shell_buf, &ws->ctr);
      if (p < 0) { ++ws->ctr.reject_rank; continue; }
      Support sup{};
      for (int k = 0; k < 4; ++k) sup.id[k] = ids[k];
      sup.q = 4;
      sup.p = p;
      sup.shell_extra = (int)ws->shell_buf.size();
      if (sup.shell_extra > 0) ++ws->ctr.shell_extra_supports;
      if (ws->store) ws->out.push_back(sup);
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
               bool exhaustive_front, bool theta_audit, Workspace* ws) {
  // `ws->inject` porte le mutant : il n'est jamais actif sur la reference.
  const int n = (int)pts.size();
  for (int a = lo; a < hi; ++a) {
    if (exhaustive_front) {
      ws->partners.clear();
      for (int b = a + 1; b < n; ++b) ws->partners.push_back(b);
    } else {
      collect_partners(tree, pts, a, smax, ws);
    }
    if (ws->partners.empty()) continue;
    ws->ctr.candidate_pairs += (long long)ws->partners.size();
    ws->ctr.max_partners =
        std::max(ws->ctr.max_partners, (long long)ws->partners.size());
    const int budget2 = smax - 2, budget3 = smax - 3, budget4 = smax - 4;

    // ---- Sonde de lanes : elle decide q2 exactement et filtre q3/q4. Elle
    // ne lit AUCUNE liste de sites : sa descente s'arrete des que les trois
    // lanes sont mortes, ce qui borne son cout par le seuil et non par le
    // voisinage. C'est ce qui empeche les ancres a grand D de payer un
    // prefixe proportionnel au nuage.
    ws->survivors.clear();
    i64 dmax_surv2 = 0;
    for (int b : ws->partners) {
      const i64 d2 = (i64)norm2_i64(sub(pts[(std::size_t)b], pts[(std::size_t)a]));
      if (d2 == 0) {
        std::fprintf(stderr, "REFUS : deux PointId a la meme position (%d,%d)\n", a, b);
        std::exit(2);
      }
      ++ws->ctr.anchors_opened;
      LaneProbe lp{};
      lane_probe(tree, pts, a, b, d2, budget2, budget3, budget4, &lp, &ws->ctr);
      if (lp.p2 <= budget2) {
        Support sup{};
        sup.id[0] = std::min(a, b);
        sup.id[1] = std::max(a, b);
        sup.q = 2;
        sup.p = lp.p2;
        sup.shell_extra = lp.extra2;
        if (lp.extra2 > 0) ++ws->ctr.shell_extra_supports;
        if (ws->store) ws->out.push_back(sup);
        ++ws->ctr.supports_q2;
      } else {
        ++ws->ctr.reject_rank;
      }
      const bool lane3 = lp.n3 <= budget3;
      const bool lane4 = lp.n4 <= budget4;
      if (!lane3 && !lane4) { ++ws->ctr.anchors_lane_dead; continue; }
      ++ws->ctr.anchors_extended;
      ws->survivors.push_back(SurvivingAnchor{b, d2, lane3, lane4});
      dmax_surv2 = std::max(dmax_surv2, d2);
    }
    if (ws->survivors.empty()) continue;

    // ---- Une seule liste de sites par point d'ancre, dimensionnee sur les
    // SEULES ancres survivantes : rayon carre 1,5 * max D^2.
    gather_sites(tree, pts, a, (3 * dmax_surv2 + 1) / 2, ws);
    std::sort(ws->survivors.begin(), ws->survivors.end(),
              [](const SurvivingAnchor& u, const SurvivingAnchor& v) {
                return u.d2 < v.d2 || (u.d2 == v.d2 && u.b < v.b);
              });
    for (const SurvivingAnchor& sa : ws->survivors)
      extend_anchor(pts, a, sa.b, sa.d2, smax, sa.lane3, sa.lane4, theta_audit, ws);
  }
}

RunResult produce(const std::vector<P3>& pts, int smax, int threads, bool exhaustive_front,
                  bool theta_audit, bool store, Inject inject, bool density_guard) {
  RunResult res{};
  MortonLbvh tree;
  tree.build(pts, g_leaf_size);
  const int n = (int)pts.size();
  const int nthreads = std::max(1, threads);
  std::vector<Workspace> spaces((std::size_t)nthreads);
  for (auto& w : spaces) { w.store = store; w.inject = inject; w.density_guard = density_guard; }
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
          run_range(tree, pts, lo, hi, smax, exhaustive_front, theta_audit,
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
// MOTEUR `pipeline` : exactement le code de `anchor_pipeline.hpp`, celui que
// nvcc compilera. Il ne partage aucune structure STL avec le moteur de
// reference : la comparaison des deux moteurs, elle-meme comparee a
// l'exhaustif, forme la chaine de garde hote -> pipeline -> device.
// ---------------------------------------------------------------------------
struct VectorSink {
  std::vector<Support>* out;
  bool store;
  void emit(const EmittedSupport& e) {
    if (!store) return;
    Support s{};
    unsigned long long k = e.key;
    int q = 0;
    for (int i = 0; i < 4; ++i) {
      const unsigned v = (unsigned)((k >> (16 * (3 - i))) & 0xFFFFULL);
      if (v != 0xFFFFu) { s.id[i] = (int)v; ++q; }
      else s.id[i] = -1;
    }
    s.q = q;
    s.p = e.p;
    s.shell_extra = e.extra;
    out->push_back(s);
  }
};

struct PipelineScratchOwner {
  std::vector<int> partners, kept, lens, site_id;
  std::vector<i64> site_d2, margin_g, margin_llow, margin_uhigh;
  std::vector<unsigned char> acute;
  Scratch view() {
    partners.resize(kPartnerCap);
    site_d2.resize(kSiteCap);
    site_id.resize(kSiteCap);
    margin_g.resize(kSiteCap);
    margin_llow.resize(kSiteCap);
    margin_uhigh.resize(kSiteCap);
    kept.resize(kKeptCap);
    lens.resize(kLensCap);
    acute.resize(kLensCap);
    Scratch sc{};
    sc.partners = partners.data();
    sc.site_d2 = site_d2.data();
    sc.site_id = site_id.data();
    sc.margin_g = margin_g.data();
    sc.margin_llow = margin_llow.data();
    sc.margin_uhigh = margin_uhigh.data();
    sc.kept = kept.data();
    sc.lens = lens.data();
    sc.acute = acute.data();
    return sc;
  }
};

struct FlatTree {
  std::vector<int> lo, hi, begin_, end_, left, right, order, px, py, pz;
  TreeView view() const {
    TreeView t{};
    t.lo = lo.data();
    t.hi = hi.data();
    t.begin = begin_.data();
    t.end = end_.data();
    t.left = left.data();
    t.right = right.data();
    t.order = order.data();
    t.px = px.data();
    t.py = py.data();
    t.pz = pz.data();
    t.nodes = (int)begin_.size();
    t.n = (int)px.size();
    return t;
  }
};

FlatTree flatten(const MortonLbvh& tree, const std::vector<P3>& pts) {
  FlatTree f;
  const int nn = (int)tree.nodes.size();
  f.lo.resize(3 * (std::size_t)nn);
  f.hi.resize(3 * (std::size_t)nn);
  f.begin_.resize((std::size_t)nn);
  f.end_.resize((std::size_t)nn);
  f.left.resize((std::size_t)nn);
  f.right.resize((std::size_t)nn);
  for (int i = 0; i < nn; ++i) {
    const LbvhNode& nd = tree.nodes[(std::size_t)i];
    for (int d = 0; d < 3; ++d) {
      f.lo[3 * (std::size_t)i + (std::size_t)d] = (int)nd.lo[d];
      f.hi[3 * (std::size_t)i + (std::size_t)d] = (int)nd.hi[d];
    }
    f.begin_[(std::size_t)i] = nd.begin;
    f.end_[(std::size_t)i] = nd.end;
    f.left[(std::size_t)i] = nd.left;
    f.right[(std::size_t)i] = nd.right;
  }
  f.order = tree.order;
  const int n = (int)pts.size();
  f.px.resize((std::size_t)n);
  f.py.resize((std::size_t)n);
  f.pz.resize((std::size_t)n);
  for (int i = 0; i < n; ++i) {
    f.px[(std::size_t)i] = (int)pts[(std::size_t)i].x;
    f.py[(std::size_t)i] = (int)pts[(std::size_t)i].y;
    f.pz[(std::size_t)i] = (int)pts[(std::size_t)i].z;
  }
  return f;
}

RunResult produce_pipeline(const std::vector<P3>& pts, int smax, int threads, bool store,
                           bool theta_audit) {
  RunResult res{};
  MortonLbvh tree;
  tree.build(pts, g_leaf_size);
  const FlatTree flat = flatten(tree, pts);
  const TreeView tv = flat.view();
  const int n = (int)pts.size();
  const int nthreads = std::max(1, threads);
  std::vector<WorkCounters> wc((std::size_t)nthreads);
  std::vector<std::vector<Support>> outs((std::size_t)nthreads);
  std::vector<unsigned> flags((std::size_t)nthreads, 0u);
  const auto t0 = std::chrono::steady_clock::now();
  {
    std::vector<std::thread> pool;
    std::atomic<int> cursor{0};
    const int chunk = std::max(1, n / (nthreads * 64) + 1);
    for (int t = 0; t < nthreads; ++t) {
      pool.emplace_back([&, t]() {
        PipelineScratchOwner owner;
        Scratch sc = owner.view();
        VectorSink sink{&outs[(std::size_t)t], store};
        for (;;) {
          const int lo = cursor.fetch_add(chunk);
          if (lo >= n) break;
          const int hi = std::min(n, lo + chunk);
          for (int a = lo; a < hi; ++a)
            run_anchor_point(tv, a, smax, sc, sink, &wc[(std::size_t)t],
                             &flags[(std::size_t)t], theta_audit);
        }
      });
    }
    for (auto& th : pool) th.join();
  }
  res.wall_extend_s =
      std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  unsigned all = 0;
  for (unsigned f : flags) all |= f;
  if (all != 0) {
    std::fprintf(stderr, "REFUS : capacite du pipeline depassee, drapeaux=%u\n", all);
    std::exit(3);
  }
  for (int t = 0; t < nthreads; ++t) {
    const WorkCounters& w = wc[(std::size_t)t];
    res.ctr.front_node_visits += w.front_node_visits;
    res.ctr.front_witness_calls += w.front_witness_calls;
    res.ctr.front_witness_visits += w.front_witness_visits;
    res.ctr.front_witness_prunes += w.front_witness_prunes;
    res.ctr.front_witness_skipped += w.front_witness_skipped;
    res.ctr.front_mass_closed += w.front_mass_closed;
    res.ctr.lane_probe_visits += w.lane_probe_visits;
    res.ctr.candidate_pairs += w.candidate_pairs;
    res.ctr.anchors_opened += w.anchors_opened;
    res.ctr.anchors_lane_dead += w.anchors_lane_dead;
    res.ctr.anchors_extended += w.anchors_extended;
    res.ctr.anchors_disk_dead += w.anchors_disk_dead;
    res.ctr.anchors_budget_early += w.anchors_budget_early;
    res.ctr.site_gather_visits += w.site_gather_visits;
    res.ctr.site_evaluations += w.site_evaluations;
    res.ctr.kept_sites += w.kept_sites;
    res.ctr.theta_only_prunes_on_live += w.theta_only_prunes_on_live;
    res.ctr.theta_anchors_active += w.theta_anchors_active;
    res.ctr.lens_carriers += w.lens_carriers;
    res.ctr.q3_candidates += w.q3_candidates;
    res.ctr.q4_pairs_walked += w.q4_pairs_walked;
    res.ctr.q4_candidates += w.q4_candidates;
    res.ctr.interior_tests += w.interior_tests;
    res.ctr.reject_positivity += w.reject_positivity;
    res.ctr.reject_non_acute += w.reject_non_acute;
    res.ctr.reject_owner += w.reject_owner;
    res.ctr.reject_rank += w.reject_rank;
    res.ctr.reject_degenerate += w.reject_degenerate;
    res.ctr.supports_q2 += w.supports_q2;
    res.ctr.supports_q3 += w.supports_q3;
    res.ctr.supports_q4 += w.supports_q4;
    res.ctr.shell_extra_supports += w.shell_extra;
    res.ctr.max_site_list = std::max(res.ctr.max_site_list, w.hw_sites);
    res.ctr.max_partners = std::max(res.ctr.max_partners, w.hw_partners);
    res.ctr.max_kept = std::max(res.ctr.max_kept, w.hw_kept);
    res.ctr.max_lens = std::max(res.ctr.max_lens, w.hw_lens);
    res.supports.insert(res.supports.end(), outs[(std::size_t)t].begin(),
                        outs[(std::size_t)t].end());
    outs[(std::size_t)t].clear();
    outs[(std::size_t)t].shrink_to_fit();
  }
  std::sort(res.supports.begin(), res.supports.end(), [](const Support& u, const Support& v) {
    return support_key(u) < support_key(v);
  });
  return res;
}

// ---------------------------------------------------------------------------
// PARITE DES DEUX MOTEURS SUR LE LEDGER COMPLET.
//
// Le differentiel `--verify` ne compare que des SUPPORTS. Il a laisse passer
// un defaut reel : le moteur `pipeline` — celui que nvcc compile — n'avait
// aucun compteur de rejet et le recu imprimait `rejets ... = 0` la ou le
// moteur de reference en comptait 1 169 095. Un compteur absent n'est pas un
// compteur nul. Cette porte compare donc les TRENTE compteurs, nommement, et
// refuse en code 1 au premier ecart.
// ---------------------------------------------------------------------------
struct NamedCounter {
  const char* name;
  long long Counters::*field;
};

const NamedCounter kNamedCounters[] = {
    {"front_node_visits", &Counters::front_node_visits},
    {"front_witness_calls", &Counters::front_witness_calls},
    {"front_witness_visits", &Counters::front_witness_visits},
    {"front_witness_prunes", &Counters::front_witness_prunes},
    {"front_witness_skipped", &Counters::front_witness_skipped},
    {"front_mass_closed", &Counters::front_mass_closed},
    {"lane_probe_visits", &Counters::lane_probe_visits},
    {"candidate_pairs", &Counters::candidate_pairs},
    {"anchors_opened", &Counters::anchors_opened},
    {"anchors_lane_dead", &Counters::anchors_lane_dead},
    {"anchors_extended", &Counters::anchors_extended},
    {"anchors_disk_dead", &Counters::anchors_disk_dead},
    {"anchors_budget_early", &Counters::anchors_budget_early},
    {"site_gather_visits", &Counters::site_gather_visits},
    {"site_evaluations", &Counters::site_evaluations},
    {"kept_sites", &Counters::kept_sites},
    {"theta_only_prunes_on_live", &Counters::theta_only_prunes_on_live},
    {"theta_anchors_active", &Counters::theta_anchors_active},
    {"lens_carriers", &Counters::lens_carriers},
    {"q3_candidates", &Counters::q3_candidates},
    {"q4_pairs_walked", &Counters::q4_pairs_walked},
    {"q4_candidates", &Counters::q4_candidates},
    {"interior_tests", &Counters::interior_tests},
    {"reject_positivity", &Counters::reject_positivity},
    {"reject_non_acute", &Counters::reject_non_acute},
    {"reject_owner", &Counters::reject_owner},
    {"reject_rank", &Counters::reject_rank},
    {"reject_degenerate", &Counters::reject_degenerate},
    {"supports_q2", &Counters::supports_q2},
    {"supports_q3", &Counters::supports_q3},
    {"supports_q4", &Counters::supports_q4},
    {"shell_extra_supports", &Counters::shell_extra_supports},
    {"max_partners", &Counters::max_partners},
    {"max_site_list", &Counters::max_site_list},
    {"max_kept", &Counters::max_kept},
    {"max_lens", &Counters::max_lens},
};

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
  // `theta` est PROUVE redondant sur toute ancre vivante ; il n'est donc plus
  // sur le chemin par defaut. `--theta-audit` le rearme pour que le compteur
  // de redondance et le mutant `theta-no-fail-open` restent exercables.
  bool theta_audit = false;
  bool compare_engines = false;
  bool density_guard = false;
  bool no_store = false;
  long long leaf = 8;
  Inject inject = Inject::kNone;
  int fixture = 0;
  bool engine_pipeline = false;
  bool emit = false;
  long long min_supports = 0;
  long long min_anchors = 0;
  long long min_prunes = 0;
  long long min_theta_active = 0;
  long long min_budget_early = 0;

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
    else if (key == "--min-theta-active") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-theta-active invalide"); min_theta_active = parsed; }
    else if (key == "--min-budget-early") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-budget-early invalide"); min_budget_early = parsed; }
    else if (key == "--verify") { verify = true; }
    else if (key == "--theta-audit") { theta_audit = true; }
    else if (key == "--compare-engines") { compare_engines = true; }
    // LA GARDE N'EXISTE PLUS QUE DANS LE MOTEUR CPU `produce`. La combiner au
    // moteur pipeline serait une option SILENCIEUSEMENT INERTE : refus.
    else if (key == "--density-guard") { density_guard = true; }
    else if (key == "--leaf") { if (!parse_ll(val.c_str(), &parsed)) refuse("--leaf invalide"); leaf = parsed; }
    else if (key == "--no-store") { no_store = true; }
    else if (key == "--engine") {
      if (val == "pipeline") engine_pipeline = true;
      else if (val == "reference") engine_pipeline = false;
      else refuse("--engine inconnu");
    }
    else if (key == "--inject") {
      if (val == "front-no-ext") inject = Inject::kFrontNoExt;
      else if (val == "front-q4-only") inject = Inject::kFrontQ4Only;
      else if (val == "theta-no-fail-open") inject = Inject::kThetaNoFailOpen;
      else if (val == "owner-min-edge") inject = Inject::kOwnerMinEdge;
      else if (val == "owner-no-tiebreak") inject = Inject::kOwnerNoTiebreak;
      else if (val == "lens-strict") inject = Inject::kLensStrict;
      else if (val == "census-no-always-inside") inject = Inject::kCensusNoAlways;
      else if (val == "positivity-loose") inject = Inject::kPositivityLoose;
      else if (val == "smax-fixed-thresholds") inject = Inject::kSmaxFixedThresholds;
      else refuse("--inject inconnu");
    }
    else if (key == "--emit-supports") { emit = true; }
    else if (key == "--fixture") {
      if (val == "ties") fixture = 1;
      else if (val == "q4only") fixture = 2;
      else refuse("--fixture inconnue");
    }
    else if (key == "--family") {
      if (val == "uniform") family = CloudFamily::kUniform;
      else if (val == "terrain") family = CloudFamily::kTerrain;
      else if (val == "scanline_single_pass") family = CloudFamily::kScanlineSinglePass;
      else if (val == "scanline_overlap_multiecho") family = CloudFamily::kScanlineOverlapMultiecho;
      else if (val == "eight_clusters") family = CloudFamily::kEightClusters;
      else refuse("--family inconnue");
    } else {
      refuse("argument inconnu");
    }
  }
  if (n < 2 || n > 65535) refuse("--points hors du profil dense u16 [2, 65535]");
  if (smax < 4 || smax > 2 + kMaxEnvelopeDepth) refuse("--smax hors bornes");
  if (threads < 1 || threads > 256) refuse("--threads hors bornes");
  if (coord < 0) coord = mhgp3v::cloud_family_default_coord(family, (int)n);
  if (coord < 2 || coord > 65536) refuse("--coord hors bornes");
  if (leaf < 1 || leaf > 256) refuse("--leaf hors bornes");
  g_leaf_size = (int)leaf;
  if (verify && no_store) refuse("--verify exige le stockage des supports");
  if (inject != Inject::kNone && !verify) refuse("un mutant sans juge ne prouve rien");
  if (inject != Inject::kNone && engine_pipeline) refuse("les mutants visent le moteur de reference");
  // LA GARDE DE DENSITE A QUITTE LE CHEMIN PARTAGE HOTE/DEVICE. Elle ne survit
  // que dans le moteur CPU `reference`, comme harnais de diagnostic desarme.
  // L'accepter en mode `pipeline` en ferait une option SILENCIEUSEMENT INERTE,
  // ce qui est pire qu'une option absente : le refus est explicite.
  if (density_guard && engine_pipeline)
    refuse("--density-guard n'existe plus dans le moteur pipeline : elle a quitte le chemin partage hote/device");
  // Un mutant qui casse un filtre desarme ne prouve rien : il serait tue par
  // hasard ou survivrait par vacuite. Le refus est contractuel, avant calcul.
  if (inject == Inject::kThetaNoFailOpen && !theta_audit)
    refuse("le mutant theta exige --theta-audit, sinon le filtre est desarme");
  if (min_theta_active > 0 && !theta_audit)
    refuse("--min-theta-active exige --theta-audit");
  if (compare_engines && engine_pipeline)
    refuse("--compare-engines lance lui-meme les deux moteurs");
  if (compare_engines && inject != Inject::kNone)
    refuse("--compare-engines compare deux moteurs sains, pas un mutant");
  if (compare_engines && no_store)
    refuse("--compare-engines exige le stockage des supports");

  // FIXTURE GRAVEE `ties` : le tetraedre regulier (0,0,0),(2,2,0),(2,0,2),(0,2,2)
  // a ses SIX aretes de carre 8 et son circumcentre exact en (1,1,1), donc
  // strictement interieur. Ses quatre faces sont equilaterales : chacune a
  // TROIS aretes maximales. C'est le cas ou la regle canonique de tie-break
  // decide seule de l'unicite de l'emission.
  std::vector<P3> pts;
  if (fixture == 1) {
    pts = {P3{0, 0, 0},   P3{2, 2, 0},   P3{2, 0, 2},   P3{0, 2, 2},
           P3{40, 40, 40}, P3{0, 40, 1}, P3{40, 1, 0}, P3{1, 0, 40}};
    n = (long long)pts.size();
  } else if (fixture == 2) {
    // FIXTURE GRAVEE `q4only` : huit points serres exactement au milieu d'une
    // longue paire. La boule temoin q4 les contient tous les huit, donc la
    // lane q4 est morte ; mais la boule DIAMETRALE n'en contient que huit, et
    // le seuil q2 est dix : le support q2 (a,b) reste pertinent avec p=8.
    // Fermer un noeud sur la seule lane q4 le detruit.
    pts = {P3{0, 0, 0},     P3{1000, 0, 0}, P3{500, 0, 0}, P3{501, 0, 0},
           P3{500, 1, 0},   P3{500, 0, 1},  P3{501, 1, 0}, P3{501, 0, 1},
           P3{500, 1, 1},   P3{501, 1, 1}};
    n = (long long)pts.size();
  } else {
    pts = mhgp3v::make_family_cloud(family, (int)n, (int)coord, seed);
    if ((long long)pts.size() != n) refuse("la famille n'a pas rendu le cardinal demande");
  }

  const auto t_start = std::chrono::steady_clock::now();
  RunResult res =
      engine_pipeline
          ? produce_pipeline(pts, (int)smax, (int)threads, !no_store, theta_audit)
          : produce(pts, (int)smax, (int)threads, false, theta_audit, !no_store, inject,
                    density_guard);
  const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - t_start).count();

  std::printf("AnchorSourceReceipt-v1\n");
  std::printf("cadre phase=exploration_v3_hors_registre backend=cpu_reference"
              " profile=quantized_u16_input_only mode=proposition_math_non_recue"
              " public_status=not_claimed\n");
  std::printf("cloud family=%s n=%lld coord=%lld seed=%lld smax=%lld threads=%lld inject=%s\n",
              mhgp3v::cloud_family_name(family), n, coord, seed, smax, threads,
              inject_name(inject));
  const Counters& c = res.ctr;
  std::printf("front node_visits=%lld witness_calls=%lld witness_visits=%lld prunes=%lld"
              " witness_skipped=%lld mass_closed=%lld garde_densite=%s lane_visits=%lld"
              " candidate_pairs=%lld anchors=%lld lane_dead=%lld etendues=%lld"
              " disk_dead=%lld budget_early=%lld\n",
              c.front_node_visits, c.front_witness_calls, c.front_witness_visits,
              c.front_witness_prunes, c.front_witness_skipped, c.front_mass_closed,
              density_guard ? "oui" : "non", c.lane_probe_visits, c.candidate_pairs,
              c.anchors_opened, c.anchors_lane_dead, c.anchors_extended, c.anchors_disk_dead,
              c.anchors_budget_early);
  std::printf("extend gather_visits=%lld site_evaluations=%lld kept_sites=%lld lens_carriers=%lld"
              " q3_candidates=%lld q4_paires_parcourues=%lld q4_candidates=%lld"
              " interior_tests=%lld\n",
              c.site_gather_visits, c.site_evaluations, c.kept_sites, c.lens_carriers,
              c.q3_candidates, c.q4_pairs_walked, c.q4_candidates, c.interior_tests);
  std::printf("rejets positivite=%lld non_aigu=%lld owner=%lld rang=%lld degenere=%lld\n",
              c.reject_positivity, c.reject_non_acute, c.reject_owner, c.reject_rank,
              c.reject_degenerate);
  std::printf("supports q2=%lld q3=%lld q4=%lld total=%lld extra_shell=%lld\n",
              c.supports_q2, c.supports_q3, c.supports_q4,
              c.supports_q2 + c.supports_q3 + c.supports_q4, c.shell_extra_supports);
  std::printf("high_water partners=%lld site_list=%lld kept=%lld lens=%lld\n",
              c.max_partners, c.max_site_list, c.max_kept, c.max_lens);
  std::printf("theta arme=%s ancres_vivantes_armees=%lld prunes_en_plus_sur_ancre_vivante=%lld\n",
              theta_audit ? "oui" : "non", c.theta_anchors_active,
              c.theta_only_prunes_on_live);
  std::printf("moteur %s caps partners=%d sites=%d kept=%d lens=%d\n",
              engine_pipeline ? "pipeline" : "reference", kPartnerCap, kSiteCap, kKeptCap,
              kLensCap);
  std::printf("temps wall_s=%.6f\n", wall);

  // Identite exacte-une-fois : aucune cle ne doit apparaitre deux fois.
  long long duplicates = 0;
  for (std::size_t i = 1; i < res.supports.size(); ++i)
    if (support_key(res.supports[i - 1]) == support_key(res.supports[i])) ++duplicates;
  if (no_store) std::printf("identite non_verifiee=no-store\n");
  else
    std::printf("identite occurrences=%zu cles_uniques=%zu doublons=%lld\n", res.supports.size(),
                res.supports.size() - (std::size_t)duplicates, duplicates);
  if (duplicates != 0) {
    if (inject != Inject::kNone) {
      std::fprintf(stderr, "MUTANT TUE : %s casse l'emission exacte-une-fois\n",
                   inject_name(inject));
      return 4;
    }
    std::fprintf(stderr, "REFUS : l'emission exacte-une-fois est violee\n");
    return 3;
  }

  // INVARIANT DE REDONDANCE DE `theta`. Sur une ancre restee vivante, moins de
  // `smax-2` sites verifient `Llow>0`; la `(smax-2)`-ieme plus grande borne
  // inferieure est donc <= 0, et `Uhigh < theta` implique `Uhigh < 0`, deja
  // couvert par `always_outside`. Le compteur ne peut donc pas etre non nul.
  // S'il l'est, le theoreme est faux ou le code ne l'implemente pas : refus.
  if (inject == Inject::kNone && c.theta_only_prunes_on_live != 0) {
    std::fprintf(stderr,
                 "REFUS : theta a retire %lld sites d'ancres vivantes, le theoreme"
                 " de redondance est falsifie\n",
                 c.theta_only_prunes_on_live);
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
    RunResult ref = produce(pts, (int)smax, 1, true, false, true, Inject::kNone, false);
    bool same = ref.supports.size() == res.supports.size();
    if (same)
      for (std::size_t i = 0; i < ref.supports.size(); ++i)
        if (support_key(ref.supports[i]) != support_key(res.supports[i]) ||
            ref.supports[i].p != res.supports[i].p ||
            ref.supports[i].shell_extra != res.supports[i].shell_extra) {
          // `extra` est compare : un filtre qui perdrait un membre de coquille
          // laisserait `p` intact et passerait sans cette colonne.
          same = false;
          break;
        }
    std::printf("verify exhaustif=%zu produit=%zu accord=%s\n", ref.supports.size(),
                res.supports.size(), same ? "OUI" : "NON");
    if (!same) {
      if (inject != Inject::kNone) {
        std::fprintf(stderr, "MUTANT TUE : %s casse l'identite de Source S\n",
                     inject_name(inject));
        return 4;
      }
      std::fprintf(stderr, "DESACCORD : un certificat a supprime ou invente des supports\n");
      return 1;
    }
    if (inject != Inject::kNone) {
      std::fprintf(stderr, "MUTANT SURVIVANT : %s n'a rien change\n", inject_name(inject));
      return 3;
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
  if (compare_engines) {
    const RunResult other =
        produce_pipeline(pts, (int)smax, (int)threads, true, theta_audit);
    long long ecarts = 0;
    for (const NamedCounter& nc : kNamedCounters) {
      const long long u = res.ctr.*(nc.field);
      const long long v = other.ctr.*(nc.field);
      if (u != v) {
        std::printf("ECART %s reference=%lld pipeline=%lld\n", nc.name, u, v);
        ++ecarts;
      }
    }
    long long ecarts_supports = 0;
    if (other.supports.size() != res.supports.size()) {
      ++ecarts_supports;
    } else {
      for (std::size_t i = 0; i < res.supports.size(); ++i)
        if (support_key(res.supports[i]) != support_key(other.supports[i]) ||
            res.supports[i].p != other.supports[i].p ||
            res.supports[i].shell_extra != other.supports[i].shell_extra)
          ++ecarts_supports;
    }
    std::printf("parite_moteurs compteurs=%zu ecarts_compteurs=%lld"
                " supports_reference=%zu supports_pipeline=%zu ecarts_supports=%lld accord=%s\n",
                sizeof(kNamedCounters) / sizeof(kNamedCounters[0]), ecarts, res.supports.size(),
                other.supports.size(), ecarts_supports,
                (ecarts == 0 && ecarts_supports == 0) ? "OUI" : "NON");
    if (ecarts != 0 || ecarts_supports != 0) {
      std::fprintf(stderr, "DESACCORD : les deux moteurs ne publient pas le meme ledger\n");
      return 1;
    }
  }

  if (c.theta_anchors_active < min_theta_active) {
    std::fprintf(stderr, "REFUS : plancher d'ancres theta %lld > %lld\n", min_theta_active,
                 c.theta_anchors_active);
    return 3;
  }
  if (c.anchors_budget_early < min_budget_early) {
    std::fprintf(stderr, "REFUS : plancher de morts anticipees %lld > %lld\n", min_budget_early,
                 c.anchors_budget_early);
    return 3;
  }
  return 0;
}
