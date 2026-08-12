// MorseHGP3D v3 — PIPELINE PAR POINT D'ANCRE, HOTE ET DEVICE.
//
// Ce fichier contient l'INTEGRALITE du travail d'un point d'ancre, sans STL,
// sans allocation et sans recursion. Il est compile a l'identique par le CPU
// et par nvcc : la parite hote/device n'est donc pas une coincidence a
// verifier mais une propriete de construction, et le differentiel sert a
// detecter une divergence de compilateur ou de materiel, pas une divergence
// d'algorithme.
//
// Toutes les capacites sont FIXES et preflightees. Un depassement leve un
// drapeau et fait refuser la campagne entiere ; il ne tronque jamais une
// sortie. C'est l'invariant « count, fill et consommation portent la meme
// identite » de la specification.
#pragma once

#include "prototype/anchor_envelope.hpp"

namespace mhgp3v {
namespace anchor {

// ---------------------------------------------------------------------------
// Capacites. Les high-water observes a n=12 500 sur `uniform` sont
// partners 2 566, sites 2 566, kept 685, lens 254 ; ces caps laissent une
// marge et sont publies dans le recu.
// ---------------------------------------------------------------------------
inline constexpr int kPartnerCap = 6144;
inline constexpr int kSiteCap = 5120;
inline constexpr int kKeptCap = 2048;
inline constexpr int kLensCap = 1024;

// Drapeaux de refus. Ils sont OR-es atomiquement et rendent la campagne
// refusee en code 3.
inline constexpr unsigned kOverflowPartners = 1u;
inline constexpr unsigned kOverflowSites = 2u;
inline constexpr unsigned kOverflowKept = 4u;
inline constexpr unsigned kOverflowLens = 8u;
inline constexpr unsigned kOverflowStack = 16u;
inline constexpr unsigned kOverflowOutput = 32u;
inline constexpr unsigned kDuplicatePosition = 64u;

// ---------------------------------------------------------------------------
// Vue LBVH en structure de tableaux, identique hote et device.
// ---------------------------------------------------------------------------
struct TreeView {
  const int* lo;      // 3 * nodes
  const int* hi;      // 3 * nodes
  const int* begin;   // nodes
  const int* end;     // nodes
  const int* left;    // nodes
  const int* right;   // nodes
  const int* order;   // n
  const int* px;      // n
  const int* py;
  const int* pz;
  int nodes;
  int n;
};

MHGP_HD inline P3 tree_point(const TreeView& t, int id) {
  return P3{(i64)t.px[id], (i64)t.py[id], (i64)t.pz[id]};
}

// ---------------------------------------------------------------------------
// Compteurs de travail, un jeu par fil. Ils ferment W_front et W_extend.
// ---------------------------------------------------------------------------
struct WorkCounters {
  long long front_node_visits = 0;
  long long front_witness_calls = 0;
  long long front_witness_visits = 0;
  long long front_witness_prunes = 0;
  long long lane_probe_visits = 0;
  long long candidate_pairs = 0;
  long long anchors_opened = 0;
  long long anchors_lane_dead = 0;
  long long anchors_extended = 0;
  long long anchors_disk_dead = 0;
  long long site_gather_visits = 0;
  long long site_evaluations = 0;
  long long kept_sites = 0;
  long long lens_carriers = 0;
  long long q3_candidates = 0;
  long long q4_pairs_walked = 0;   // paires de lentille parcourues, non aigues comprises
  long long q4_candidates = 0;
  long long interior_tests = 0;
  long long supports_q2 = 0;
  long long supports_q3 = 0;
  long long supports_q4 = 0;
  long long shell_extra = 0;
  long long hw_partners = 0;
  long long hw_sites = 0;
  long long hw_kept = 0;
  long long hw_lens = 0;
};

// Espace de travail d'un fil. Les tableaux vivent en memoire globale sur
// device et sur la pile de tache sur hote ; leur adresse est fournie, jamais
// allouee ici.
struct Scratch {
  int* partners;      // kPartnerCap
  i64* site_d2;       // kSiteCap
  int* site_id;       // kSiteCap
  i64* margin_g;      // kSiteCap
  i64* margin_llow;   // kSiteCap
  i64* margin_uhigh;  // kSiteCap
  int* kept;          // kKeptCap
  int* lens;          // kLensCap
  unsigned char* acute;  // kLensCap
};

// Un support emis. La cle canonique suffit au tri et a la deduplication ;
// `p` et `extra` portent le census.
struct EmittedSupport {
  unsigned long long key;
  int p;
  int extra;
};

// ---------------------------------------------------------------------------
// Geometrie de noeud, entiere.
// ---------------------------------------------------------------------------
MHGP_HD inline i64 node_min_dist2(const TreeView& t, int ni, const P3& p) {
  i64 s = 0;
  const i64 c[3] = {p.x, p.y, p.z};
  for (int d = 0; d < 3; ++d) {
    const i64 lo = t.lo[3 * ni + d], hi = t.hi[3 * ni + d];
    i64 g = 0;
    if (c[d] < lo) g = lo - c[d];
    else if (c[d] > hi) g = c[d] - hi;
    s += g * g;
  }
  return s;
}
MHGP_HD inline i64 node_max_dist2(const TreeView& t, int ni, const P3& p) {
  i64 s = 0;
  const i64 c[3] = {p.x, p.y, p.z};
  for (int d = 0; d < 3; ++d) {
    const i64 lo = t.lo[3 * ni + d], hi = t.hi[3 * ni + d];
    const i64 g = (c[d] - lo > hi - c[d]) ? (c[d] - lo) : (hi - c[d]);
    s += g * g;
  }
  return s;
}
MHGP_HD inline i64 node_extent2(const TreeView& t, int ni) {
  i64 s = 0;
  for (int d = 0; d < 3; ++d) {
    const i64 e = t.hi[3 * ni + d] - t.lo[3 * ni + d];
    s += e * e;
  }
  return s;
}

// ---------------------------------------------------------------------------
// Certificat de front : trois boules temoins communes, une par lane.
// Voir NOTE_CLAUDE_PRODUCTEUR_ANCRE_EXACT_UNE_FOIS_20260812.md section 3.3.
// ---------------------------------------------------------------------------
struct WitnessBall {
  i64 q0[3];
  i64 tau[3];
  bool usable;
};

MHGP_HD inline WitnessBall witness_ball_of(const TreeView& t, int ni, const P3& pa) {
  WitnessBall wb{};
  wb.usable = false;
  const i64 c[3] = {pa.x, pa.y, pa.z};
  for (int d = 0; d < 3; ++d) {
    wb.q0[d] = 2 * c[d] + t.lo[3 * ni + d] + t.hi[3 * ni + d];
    wb.tau[d] = 0;
  }
  const i64 g = node_min_dist2(t, ni, pa);
  if (g <= 0) return wb;
  const i64 dmin = isqrt_i128(g);
  const i64 ext = isqrt_i128(node_extent2(t, ni)) + 1;
  const i64 num[3] = {20000, 11547, 10327};
  for (int q = 0; q < 3; ++q) {
    const i64 l = (num[q] * dmin) / 10000 - ext;
    wb.tau[q] = (l > 0) ? l * l : 0;
    if (l > 0) wb.usable = true;
  }
  return wb;
}

MHGP_HD inline i64 witness_far2(const TreeView& t, int ni, const WitnessBall& wb) {
  i64 s = 0;
  for (int d = 0; d < 3; ++d) {
    const i64 lo4 = 4 * (i64)t.lo[3 * ni + d] - wb.q0[d];
    const i64 hi4 = 4 * (i64)t.hi[3 * ni + d] - wb.q0[d];
    const i64 a = lo4 < 0 ? -lo4 : lo4;
    const i64 b = hi4 < 0 ? -hi4 : hi4;
    const i64 f = a > b ? a : b;
    s += f * f;
  }
  return s;
}
MHGP_HD inline i64 witness_near2(const TreeView& t, int ni, const WitnessBall& wb) {
  i64 s = 0;
  for (int d = 0; d < 3; ++d) {
    const i64 lo4 = 4 * (i64)t.lo[3 * ni + d];
    const i64 hi4 = 4 * (i64)t.hi[3 * ni + d];
    i64 g = 0;
    if (wb.q0[d] < lo4) g = lo4 - wb.q0[d];
    else if (wb.q0[d] > hi4) g = wb.q0[d] - hi4;
    s += g * g;
  }
  return s;
}

MHGP_HD inline bool witness_closes(const TreeView& t, const WitnessBall& wb, int smax,
                                   WorkCounters* ctr, unsigned* flags) {
  int cnt[3] = {0, 0, 0};
  const int need[3] = {lane_death_threshold(smax, 2), lane_death_threshold(smax, 3),
                       lane_death_threshold(smax, 4)};
  int stack_node[64];
  unsigned stack_mask[64];
  int sp = 0;
  stack_node[sp] = 0;
  stack_mask[sp] = 7u;
  ++sp;
  while (sp > 0) {
    --sp;
    const int ni = stack_node[sp];
    unsigned mask = stack_mask[sp];
    ++ctr->front_witness_visits;
    const i64 near2 = witness_near2(t, ni, wb);
    const i64 far2 = witness_far2(t, ni, wb);
    for (int q = 0; q < 3; ++q) {
      if ((mask & (1u << q)) == 0) continue;
      if (cnt[q] >= need[q]) { mask &= ~(1u << q); continue; }
      if (near2 >= wb.tau[q]) { mask &= ~(1u << q); continue; }
      if (far2 < wb.tau[q]) {
        cnt[q] += t.end[ni] - t.begin[ni];
        mask &= ~(1u << q);
      }
    }
    if (cnt[0] >= need[0] && cnt[1] >= need[1] && cnt[2] >= need[2]) return true;
    if (mask == 0) continue;
    if (t.left[ni] < 0) {
      for (int s = t.begin[ni]; s < t.end[ni]; ++s) {
        const int id = t.order[s];
        const i64 c[3] = {(i64)t.px[id], (i64)t.py[id], (i64)t.pz[id]};
        i64 v = 0;
        for (int d = 0; d < 3; ++d) {
          const i64 g = 4 * c[d] - wb.q0[d];
          v += g * g;
        }
        for (int q = 0; q < 3; ++q)
          if ((mask & (1u << q)) != 0 && v < wb.tau[q]) ++cnt[q];
        if (cnt[0] >= need[0] && cnt[1] >= need[1] && cnt[2] >= need[2]) return true;
      }
      continue;
    }
    if (sp + 2 > 62) { *flags |= kOverflowStack; return false; }
    stack_node[sp] = t.left[ni];
    stack_mask[sp] = mask;
    ++sp;
    stack_node[sp] = t.right[ni];
    stack_mask[sp] = mask;
    ++sp;
  }
  return cnt[0] >= need[0] && cnt[1] >= need[1] && cnt[2] >= need[2];
}

// ---------------------------------------------------------------------------
// Sonde de lanes : les trois boules de milieu en une descente, sortie
// anticipee des que les trois lanes sont mortes.
// ---------------------------------------------------------------------------
struct LaneProbe {
  int p2, extra2, n3, n4;
};

MHGP_HD inline LaneProbe lane_probe(const TreeView& t, int a, int b, i64 d2, int budget2,
                                    int budget3, int budget4, WorkCounters* ctr,
                                    unsigned* flags) {
  LaneProbe out{0, 0, 0, 0};
  const P3 pa = tree_point(t, a);
  const P3 pb = tree_point(t, b);
  const i64 q0[3] = {2 * (pa.x + pb.x), 2 * (pa.y + pb.y), 2 * (pa.z + pb.z)};
  const i64 lim = 16 * d2;
  int stack[64];
  int sp = 0;
  stack[sp++] = 0;
  while (sp > 0) {
    const int ni = stack[--sp];
    ++ctr->lane_probe_visits;
    i64 smin = 0;
    for (int d = 0; d < 3; ++d) {
      const i64 lo4 = 4 * (i64)t.lo[3 * ni + d];
      const i64 hi4 = 4 * (i64)t.hi[3 * ni + d];
      i64 g = 0;
      if (q0[d] < lo4) g = lo4 - q0[d];
      else if (q0[d] > hi4) g = q0[d] - hi4;
      smin += g * g;
    }
    if (4 * smin > lim) continue;
    if (t.left[ni] < 0) {
      for (int s = t.begin[ni]; s < t.end[ni]; ++s) {
        const int id = t.order[s];
        if (id == a || id == b) continue;
        const i64 c[3] = {(i64)t.px[id], (i64)t.py[id], (i64)t.pz[id]};
        i64 v = 0;
        for (int d = 0; d < 3; ++d) {
          const i64 g = 4 * c[d] - q0[d];
          v += g * g;
        }
        const i64 v4 = 4 * v;
        if (v4 > lim) continue;
        if (v4 == lim) { ++out.extra2; continue; }
        ++out.p2;
        if (12 * v < lim) {
          ++out.n3;
          if (15 * v < lim) ++out.n4;
        }
        if (out.p2 > budget2 && out.n3 > budget3 && out.n4 > budget4) return out;
      }
      continue;
    }
    if (sp + 2 > 62) { *flags |= kOverflowStack; return out; }
    stack[sp++] = t.left[ni];
    stack[sp++] = t.right[ni];
  }
  return out;
}

// ---------------------------------------------------------------------------
// Arete maximale canonique : la plus longue, puis le plus petit couple
// (min PointId, max PointId). Un support n'est emis que depuis elle.
// ---------------------------------------------------------------------------
MHGP_HD inline bool owns_canonical_max_edge(const TreeView& t, const int* ids, int q, int a,
                                            int b) {
  i64 best2 = -1;
  int bi = -1, bj = -1;
  for (int i = 0; i < q; ++i)
    for (int j = i + 1; j < q; ++j) {
      const i64 d2 = (i64)norm2_i64(sub(tree_point(t, ids[i]), tree_point(t, ids[j])));
      const int lo = ids[i] < ids[j] ? ids[i] : ids[j];
      const int hi = ids[i] < ids[j] ? ids[j] : ids[i];
      if (d2 > best2 || (d2 == best2 && (lo < bi || (lo == bi && hi < bj)))) {
        best2 = d2;
        bi = lo;
        bj = hi;
      }
    }
  const int la = a < b ? a : b;
  const int lb = a < b ? b : a;
  return bi == la && bj == lb;
}

// ---------------------------------------------------------------------------
// Census : les toujours-interieurs sont comptes sans test, les
// toujours-exterieurs ne sont jamais charges.
// ---------------------------------------------------------------------------
MHGP_HD inline int census(const TreeView& t, const int* crossing, int ncross, int always_inside,
                          const BallForm& ball, const P3& origin, const int* ids, int q,
                          int budget, int* extra_out, WorkCounters* ctr) {
  int p = always_inside;
  int extra = 0;
  if (p > budget) return -1;
  for (int k = 0; k < ncross; ++k) {
    const int id = crossing[k];
    bool in_support = false;
    for (int i = 0; i < q; ++i)
      if (ids[i] == id) { in_support = true; break; }
    if (in_support) continue;
    ++ctr->interior_tests;
    const i128 s = power_sign_value(ball, origin, tree_point(t, id));
    if (s < 0) {
      if (++p > budget) return -1;
    } else if (s == 0) {
      ++extra;
    }
  }
  *extra_out = extra;
  return p;
}

// ---------------------------------------------------------------------------
// Le pipeline complet d'un point d'ancre.
//   `out` peut etre nul : le mode compte-seulement ne stocke rien.
//   `out_count` est incremente atomiquement par l'appelant via `reserve`.
// ---------------------------------------------------------------------------
template <typename Sink>
MHGP_HD inline void run_anchor_point(const TreeView& t, int a, int smax, Scratch sc, Sink& sink,
                                     WorkCounters* ctr, unsigned* flags) {
  const P3 pa = tree_point(t, a);
  const int budget2 = smax - 2, budget3 = smax - 3, budget4 = smax - 4;

  // ---- 1. Partenaires candidats : le certificat de front ferme des noeuds
  // entiers. Ce qui survit n'est pas « proche », c'est « non certifie mort ».
  int npart = 0;
  {
    int stack[64];
    int sp = 0;
    stack[sp++] = 0;
    while (sp > 0) {
      const int ni = stack[--sp];
      ++ctr->front_node_visits;
      const WitnessBall wb = witness_ball_of(t, ni, pa);
      if (wb.usable) {
        ++ctr->front_witness_calls;
        if (witness_closes(t, wb, smax, ctr, flags)) {
          ++ctr->front_witness_prunes;
          continue;
        }
      }
      if (t.left[ni] < 0) {
        for (int s = t.begin[ni]; s < t.end[ni]; ++s) {
          const int id = t.order[s];
          if (id <= a) continue;
          if (npart >= kPartnerCap) { *flags |= kOverflowPartners; return; }
          sc.partners[npart++] = id;
        }
        continue;
      }
      if (sp + 2 > 62) { *flags |= kOverflowStack; return; }
      stack[sp++] = t.left[ni];
      stack[sp++] = t.right[ni];
    }
  }
  if (npart == 0) return;
  ctr->candidate_pairs += npart;
  if (npart > ctr->hw_partners) ctr->hw_partners = npart;

  // ---- 2. Sonde de lanes : elle decide q2 exactement et filtre q3/q4 sans
  // lire la moindre liste de voisins.
  int nsurv = 0;
  i64 dmax_surv2 = 0;
  for (int k = 0; k < npart; ++k) {
    const int b = sc.partners[k];
    const i64 d2 = (i64)norm2_i64(sub(tree_point(t, b), pa));
    if (d2 == 0) { *flags |= kDuplicatePosition; return; }
    ++ctr->anchors_opened;
    const LaneProbe lp = lane_probe(t, a, b, d2, budget2, budget3, budget4, ctr, flags);
    if (lp.p2 <= budget2) {
      int ids[2] = {a < b ? a : b, a < b ? b : a};
      EmittedSupport e{};
      e.key = ((unsigned long long)(unsigned)ids[0] << 48) |
              ((unsigned long long)(unsigned)ids[1] << 32) | 0xFFFFFFFFULL;
      e.p = lp.p2;
      e.extra = lp.extra2;
      sink.emit(e);
      ++ctr->supports_q2;
      if (lp.extra2 > 0) ++ctr->shell_extra;
    }
    const bool lane3 = lp.n3 <= budget3;
    const bool lane4 = lp.n4 <= budget4;
    if (!lane3 && !lane4) { ++ctr->anchors_lane_dead; continue; }
    ++ctr->anchors_extended;
    // Les survivants sont reecrits en place : (b, lane3, lane4) tient dans un
    // entier, le bit 30 portant lane3 et le bit 29 lane4.
    sc.partners[nsurv++] = b | (lane3 ? (1 << 30) : 0) | (lane4 ? (1 << 29) : 0);
    if (d2 > dmax_surv2) dmax_surv2 = d2;
  }
  if (nsurv == 0) return;

  // ---- 3. Une seule liste de sites par point, dimensionnee sur les SEULES
  // ancres survivantes : rayon carre 1,5 * max D^2.
  const i64 r2 = (3 * dmax_surv2 + 1) / 2;
  int nsites = 0;
  {
    int stack[64];
    int sp = 0;
    stack[sp++] = 0;
    while (sp > 0) {
      const int ni = stack[--sp];
      ++ctr->site_gather_visits;
      if (node_min_dist2(t, ni, pa) > r2) continue;
      const bool inside = node_max_dist2(t, ni, pa) <= r2;
      if (t.left[ni] < 0 || inside) {
        for (int s = t.begin[ni]; s < t.end[ni]; ++s) {
          const int id = t.order[s];
          if (id == a) continue;
          const i64 d2 = (i64)norm2_i64(sub(tree_point(t, id), pa));
          if (d2 > r2) continue;
          if (nsites >= kSiteCap) { *flags |= kOverflowSites; return; }
          sc.site_d2[nsites] = d2;
          sc.site_id[nsites] = id;
          ++nsites;
        }
        continue;
      }
      if (sp + 2 > 62) { *flags |= kOverflowStack; return; }
      stack[sp++] = t.left[ni];
      stack[sp++] = t.right[ni];
    }
  }
  if (nsites > ctr->hw_sites) ctr->hw_sites = nsites;
  // Tri par TAS sur (d2, id) : en place, O(n log n), sans recursion et sans
  // memoire auxiliaire. Le tri par insertion precedent coutait jusqu'a
  // 13,1 millions de deplacements par fil au cap des sites (audit du
  // 12 aout) ; ce tas coute au plus 5120*13 comparaisons.
  {
    auto swap_at = [&](int i, int j) {
      const i64 d = sc.site_d2[i];
      sc.site_d2[i] = sc.site_d2[j];
      sc.site_d2[j] = d;
      const int v = sc.site_id[i];
      sc.site_id[i] = sc.site_id[j];
      sc.site_id[j] = v;
    };
    auto less_at = [&](int i, int j) {
      return sc.site_d2[i] < sc.site_d2[j] ||
             (sc.site_d2[i] == sc.site_d2[j] && sc.site_id[i] < sc.site_id[j]);
    };
    auto sift = [&](int root, int count) {
      for (;;) {
        int big = root;
        const int l = 2 * root + 1, r = l + 1;
        if (l < count && less_at(big, l)) big = l;
        if (r < count && less_at(big, r)) big = r;
        if (big == root) return;
        swap_at(root, big);
        root = big;
      }
    };
    for (int i = nsites / 2 - 1; i >= 0; --i) sift(i, nsites);
    for (int i = nsites - 1; i > 0; --i) {
      swap_at(0, i);
      sift(0, i);
    }
  }

  // ---- 4. Extension par ancre.
  for (int si = 0; si < nsurv; ++si) {
    const int packed = sc.partners[si];
    const int b = packed & 0x1FFFFFFF;
    const bool lane3 = (packed & (1 << 30)) != 0;
    const bool lane4 = (packed & (1 << 29)) != 0;
    const P3 pb = tree_point(t, b);
    const i64 d2 = (i64)norm2_i64(sub(pb, pa));

    // Prefixe utile par dichotomie sur la liste triee : |z-a|^2 <= 1,5 D^2.
    // Aucun tri des partenaires n'est necessaire, et aucun invariant ne le
    // rendait exact : le prefixe se relit en O(log) a chaque ancre.
    int site_count = 0;
    {
      int lo = 0, hi = nsites;
      while (lo < hi) {
        const int mid = lo + (hi - lo) / 2;
        if (2 * sc.site_d2[mid] <= 3 * d2) lo = mid + 1;
        else hi = mid;
      }
      site_count = lo;
    }
    ctr->site_evaluations += site_count;

    const P3 dvec = sub(pb, pa);
    const P3 ab{pa.x + pb.x, pa.y + pb.y, pa.z + pb.z};
    for (int k = 0; k < site_count; ++k) {
      const P3 pz = tree_point(t, sc.site_id[k]);
      const P3 u{2 * pz.x - ab.x, 2 * pz.y - ab.y, 2 * pz.z - ab.z};
      const i128 uu = norm2_i64(u);
      const i128 ud = dot_i64(u, dvec);
      const i64 g = (i64)(i128(d2) - uu);
      const i64 s = isqrt_i128(2 * (uu * i128(d2) - ud * ud)) + 1;
      sc.margin_g[k] = g;
      sc.margin_llow[k] = g - s;
      sc.margin_uhigh[k] = g + s;
    }

    // theta : neuvieme plus grande borne inferieure Llow sur X \ {a,b}.
    i64 theta = 0;
    bool theta_active = false;
    {
      int m = 0;
      for (int k = 0; k < site_count; ++k)
        if (sc.site_id[k] != b) ++m;
      const int depth = envelope_depth(smax);
      if (m >= depth) {
        // Selection partielle des neuf plus grands : tri par insertion sur un
        // tampon de neuf, deterministe et identique des deux cotes.
        // Insertion des neuf plus grandes valeurs de Llow, en un seul passage
        // sur la liste et sans tampon auxiliaire : deterministe, identique des
        // deux cotes.
        i64 top[kMaxEnvelopeDepth];
        int filled = 0;
        for (int k = 0; k < site_count; ++k) {
          if (sc.site_id[k] == b) continue;
          const i64 v = sc.margin_llow[k];
          if (filled < depth) {
            int j = filled - 1;
            while (j >= 0 && top[j] < v) { top[j + 1] = top[j]; --j; }
            top[j + 1] = v;
            ++filled;
          } else if (v > top[depth - 1]) {
            int j = depth - 2;
            while (j >= 0 && top[j] < v) { top[j + 1] = top[j]; --j; }
            top[j + 1] = v;
          }
        }
        theta = top[depth - 1];
        theta_active = true;
      }
    }

    int nkept = 0;
    int always_inside = 0;
    for (int k = 0; k < site_count; ++k) {
      if (sc.site_id[k] == b) continue;
      if (sc.margin_llow[k] > 0) { ++always_inside; continue; }
      if (sc.margin_uhigh[k] < 0) continue;
      if (theta_active && sc.margin_uhigh[k] < theta) continue;
      if (nkept >= kKeptCap) { *flags |= kOverflowKept; return; }
      sc.kept[nkept++] = sc.site_id[k];
    }
    ctr->kept_sites += nkept;
    if (nkept > ctr->hw_kept) ctr->hw_kept = nkept;

    const bool disk3 = lane3 && always_inside <= budget3;
    const bool disk4 = lane4 && always_inside <= budget4;
    if (!disk3 && !disk4) { ++ctr->anchors_disk_dead; continue; }

    int nl = 0;
    for (int k = 0; k < nkept; ++k) {
      const P3 px = tree_point(t, sc.kept[k]);
      if ((i64)norm2_i64(sub(px, pa)) > d2) continue;
      if ((i64)norm2_i64(sub(px, pb)) > d2) continue;
      if (nl >= kLensCap) { *flags |= kOverflowLens; return; }
      sc.lens[nl++] = sc.kept[k];
    }
    ctr->lens_carriers += nl;
    if (nl > ctr->hw_lens) ctr->hw_lens = nl;
    for (int i = 0; i < nl; ++i)
      sc.acute[i] = positive_q3(pa, pb, tree_point(t, sc.lens[i])) ? 1 : 0;

    // ---- q3 : minimum auto-centre sur chaque droite conservee.
    for (int i = 0; disk3 && i < nl; ++i) {
      const int x = sc.lens[i];
      ++ctr->q3_candidates;
      if (sc.acute[i] == 0) continue;
      int ids[4] = {a, b, x, -1};
      sort_ids(ids, 3);
      if (!owns_canonical_max_edge(t, ids, 3, a, b)) continue;
      const BallForm ball = circum_q3(pa, pb, tree_point(t, x));
      if (!ball.valid) continue;
      int extra = 0;
      const int p = census(t, sc.kept, nkept, always_inside, ball, pa, ids, 3, budget3, &extra,
                           ctr);
      if (p < 0) continue;
      EmittedSupport e{};
      e.key = ((unsigned long long)(unsigned)ids[0] << 48) |
              ((unsigned long long)(unsigned)ids[1] << 32) |
              ((unsigned long long)(unsigned)ids[2] << 16) | 0xFFFFULL;
      e.p = p;
      e.extra = extra;
      sink.emit(e);
      ++ctr->supports_q3;
      if (extra > 0) ++ctr->shell_extra;
    }

    // ---- q4 : intersections de deux droites conservees. Le Theoreme 5
    // garantit qu'au moins une des deux faces `abx`, `aby` est positive.
    for (int i = 0; disk4 && i < nl; ++i) {
      const int x = sc.lens[i];
      const P3 px = tree_point(t, x);
      for (int j = i + 1; j < nl; ++j) {
        ++ctr->q4_pairs_walked;
        if (sc.acute[i] == 0 && sc.acute[j] == 0) continue;
        const int y = sc.lens[j];
        const P3 py = tree_point(t, y);
        ++ctr->q4_candidates;
        if ((i64)norm2_i64(sub(px, py)) > d2) continue;
        const BallForm ball = circum_q4(pa, pb, px, py);
        if (!ball.valid) continue;
        if (!positive_q4(pa, pb, px, py, ball, pa)) continue;
        int ids[4] = {a, b, x, y};
        sort_ids(ids, 4);
        if (!owns_canonical_max_edge(t, ids, 4, a, b)) continue;
        int extra = 0;
        const int p = census(t, sc.kept, nkept, always_inside, ball, pa, ids, 4, budget4, &extra,
                             ctr);
        if (p < 0) continue;
        EmittedSupport e{};
        e.key = ((unsigned long long)(unsigned)ids[0] << 48) |
                ((unsigned long long)(unsigned)ids[1] << 32) |
                ((unsigned long long)(unsigned)ids[2] << 16) |
                (unsigned long long)(unsigned)ids[3];
        e.p = p;
        e.extra = extra;
        sink.emit(e);
        ++ctr->supports_q4;
        if (extra > 0) ++ctr->shell_extra;
      }
    }
  }
}

}  // namespace anchor
}  // namespace mhgp3v
