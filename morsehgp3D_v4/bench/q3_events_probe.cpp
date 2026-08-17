// MorseHGP3D v4 — PROBE D'EVENEMENTS q3 : ancres survivantes -> supports.
//
// Chaine complete de la lane q3, counter-only :
//   WSPD ternaire fusionnee -> rectangles vivants q3 -> ancres survivantes
//   (h_coeur + h_a + h_b < h_3, par histogramme) -> porteurs x dans
//   lentille(ab) ∖ boule diametrale (requete d'arbre), acuite STRICTE
//   V² > D², owner EdgeKey -> profondeur de circum-boule exacte (Gram) ;
//   depth <= h_3 - 1 -> EVENEMENT q3 ; shell supplementaire -> refus
//   transactionnel compte a part (audit du 17 aout, Q5.2).
//
// ENREGISTREMENT UNIQUE (audit cible 5964214) : chaque evenement est UN
// record Q3Event{support, owner, ball, level, depth, interior tries} ;
// support/owner/ball/level sont formes AVANT le census (candidat), le census
// n'ajoute que depth et les interieurs. CONTRAT DE CAPACITE : profil
// K_max <= 10 — smax > 11 est REFUSE (code 2), jamais tronque.
//
// JUGE (--judge, oracle borne petit n) : enumeration brute de TOUS les
// triangles {i<j<k} — owner par longueur/EdgeKey sur les VRAIS PointId,
// acuite, profondeur, interieurs — et comparaison des RECORDS COMPLETS
// (multiensemble) : un record qui differe par un seul champ compte a la fois
// en manquant et en trop. Codes : 0 conforme, 1 desaccord, 2 refus,
// 3 invariant.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/events/q3_event.hpp"
#include "../src/events/q3_instruction.hpp"
#include "../src/events/witness_count.hpp"
#include "../src/wspd/wavefront.hpp"

namespace {

using namespace mhgp4;

struct Args {
  CloudFamily family = CloudFamily::kUniform;
  bool family_ok = true;
  int n = 2000;
  int coord = 0;
  long long seed = 3;
  i64 s = 8;
  u64 smax = 11;
  bool judge = false;
  bool exact_mode = false;  // premier extra-shell => unsupported_degeneracy
  bool census_cover = true;  // --census=tree : descente par porteur (appariee)
  bool packet = true;        // --packet=off : porte causale appariee
  bool cover_rect = true;    // --cover=root : requete d'arbre par ancre
  bool inject_cover_dmin = false;  // mutant : Dmax remplace par Dmin
  bool inject_no_exclude = false;  // mutant : packet non exclu du scan
  u64 min_events = 0;
};

bool parse_family(const char* name, CloudFamily* out) {
  const CloudFamily all[] = {CloudFamily::kUniform,
                             CloudFamily::kTerrain,
                             CloudFamily::kScanlineSinglePass,
                             CloudFamily::kScanlineOverlapMultiecho,
                             CloudFamily::kEightClusters,
                             CloudFamily::kTwoLines,
                             CloudFamily::kCollinearSeven};
  for (const CloudFamily f : all)
    if (std::strcmp(name, cloud_family_name(f)) == 0) {
      *out = f;
      return true;
    }
  return false;
}

Args parse(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return arg.compare(0, l, prefix) == 0 ? arg.c_str() + l : nullptr;
    };
    if (const char* v = val("--family=")) a.family_ok = parse_family(v, &a.family);
    else if (const char* v = val("--n=")) a.n = std::atoi(v);
    else if (const char* v = val("--coord=")) a.coord = std::atoi(v);
    else if (const char* v = val("--seed=")) a.seed = std::atoll(v);
    else if (const char* v = val("--s=")) a.s = std::atoll(v);
    else if (const char* v = val("--smax=")) a.smax = (u64)std::atoll(v);
    else if (const char* v = val("--min-events=")) a.min_events = (u64)std::atoll(v);
    else if (arg == "--judge") a.judge = true;
    else if (arg == "--exact") a.exact_mode = true;
    else if (arg == "--census=cover") a.census_cover = true;
    else if (arg == "--census=tree") a.census_cover = false;
    else if (arg == "--packet=on") a.packet = true;
    else if (arg == "--packet=off") a.packet = false;
    else if (arg == "--inject=packet-no-exclude") a.inject_no_exclude = true;
    else if (arg == "--cover=rectangle") a.cover_rect = true;
    else if (arg == "--cover=root") a.cover_rect = false;
    else if (arg == "--inject=cover-dmin") a.inject_cover_dmin = true;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      a.family_ok = false;
    }
  }
  return a;
}

// Publie un record complet : interieurs convertis en PointId puis tries.
Q3Event make_event(const SupportKey3& sk, const EdgeKey& ek, const Q3BallKey& bk,
                   const Q3Level& lv, const i32* interior_u, u64 n_interior,
                   const std::vector<PointId>& pid_of) {
  Q3Event e;
  e.support = sk;
  e.owner = ek;
  e.ball = bk;
  e.level = lv;
  n_interior = std::min<u64>(n_interior, e.interior.size());  // borne prouvable
  e.depth = (u8)n_interior;
  for (u64 t = 0; t < n_interior; ++t)
    e.interior[t] = pid_of[(size_t)interior_u[t]];
  // Tri par insertion (n <= 8) : evite l'introsort generique et son faux
  // positif -Warray-bounds de GCC 13 sur les sous-intervalles de tableau fixe.
  for (u64 t = 1; t < n_interior; ++t) {
    const PointId v = e.interior[t];
    u64 w = t;
    for (; w > 0 && e.interior[w - 1] > v; --w) e.interior[w] = e.interior[w - 1];
    e.interior[w] = v;
  }
  return e;
}

// COVER PARTAGE PAR ANCRE (audit du 17 aout, § 6.2) : tout porteur ET tout
// point interieur d'une circum-boule q3 pertinente de l'ancre (a,b) verifie
// |z-m| <= (sqrt(3)/2)·D, soit |2z-(a+b)|² <= 3·D² (ferme, i64). Une seule
// requete sert la lentille et le census ; la liste est TRIEE par distance
// croissante au milieu, pour que le scan temoin par porteur atteigne h_3 au
// plus vite (les points proches du milieu sont interieurs a la plupart des
// circum-boules).
struct CoverPoint {
  i32 u;
  i64 dist2q;  // |2z-(a+b)|²
};

void cover_query(const CloudIndex& ix, const P3& pa, const P3& pb, i64 D2,
                 std::vector<CoverPoint>* out) {
  out->clear();
  if (ix.nodes.empty()) return;
  const i64 m2[3] = {pa.x + pb.x, pa.y + pb.y, pa.z + pb.z};
  const i64 bound = 3 * D2;
  const auto box_min_dist2q = [&](const AxisBox& b) {
    i64 s = 0;
    for (int i = 0; i < 3; ++i) {
      i64 d = 0;
      if (2 * b.lo[i] > m2[i]) d = 2 * b.lo[i] - m2[i];
      else if (2 * b.hi[i] < m2[i]) d = m2[i] - 2 * b.hi[i];
      s += d * d;
    }
    return s;
  };
  std::vector<NodeRef> stack{0};
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = box_of_node(ix, z);
    if (box_min_dist2q(bz) > bound) continue;
    if (z < 0) {
      const i32 u = -1 - z;
      const P3& p = ix.upos[(size_t)u];
      i64 d2 = 0;
      const i64 c[3] = {p.x, p.y, p.z};
      for (int i = 0; i < 3; ++i) {
        const i64 t = 2 * c[i] - m2[i];
        d2 += t * t;
      }
      if (d2 <= bound) out->push_back(CoverPoint{u, d2});
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  std::sort(out->begin(), out->end(),
            [](const CoverPoint& x, const CoverPoint& y) {
              return x.dist2q != y.dist2q ? x.dist2q < y.dist2q : x.u < y.u;
            });
}

// COVER RECTANGULAIRE (audit du 17 aout, reponse apres ebc823) : une seule
// traversee haute de l'arbre PAR RECTANGLE vivant. Boite des sommes
// S_AB = [A.lo+B.lo, A.hi+B.hi] par axe ; un nœud temoin Z est elague une
// fois pour toutes si dist(2Box(Z), S_AB)² > 3·Dmax², ou Dmax² majore la
// longueur d'ancre sur le rectangle (coins opposes). Le resultat est une
// ANTICHAINE de handles ; chaque ancre part des handles, jamais de la
// racine, et applique le filtre exact |2z-(a+b)|² <= 3·|a-b|².
void rect_cover_handles(const CloudIndex& ix, const AxisBox& A, const AxisBox& B,
                        bool mutant_dmin, std::vector<NodeRef>* out,
                        u64* tree_nodes) {
  out->clear();
  if (ix.nodes.empty()) return;
  i64 slo[3], shi[3], dmax2 = 0, dmin2 = 0;
  for (int i = 0; i < 3; ++i) {
    slo[i] = A.lo[i] + B.lo[i];
    shi[i] = A.hi[i] + B.hi[i];
    const i64 e1 = A.hi[i] - B.lo[i];
    const i64 e2 = B.hi[i] - A.lo[i];
    const i64 w = std::max(std::llabs(e1), std::llabs(e2));
    dmax2 += w * w;
    i64 lo = 0;
    if (B.lo[i] > A.hi[i]) lo = B.lo[i] - A.hi[i];
    else if (A.lo[i] > B.hi[i]) lo = A.lo[i] - B.hi[i];
    dmin2 += lo * lo;
  }
  const i64 bound = 3 * (mutant_dmin ? dmin2 : dmax2);
  std::vector<NodeRef> stack{0};
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    ++*tree_nodes;
    const AxisBox bz = box_of_node(ix, z);
    i64 gap2 = 0;
    for (int i = 0; i < 3; ++i) {
      i64 g = 0;
      if (2 * bz.lo[i] > shi[i]) g = 2 * bz.lo[i] - shi[i];
      else if (2 * bz.hi[i] < slo[i]) g = slo[i] - 2 * bz.hi[i];
      gap2 += g * g;
    }
    if (gap2 > bound) continue;
    if (z < 0) {
      out->push_back(z);
      continue;
    }
    const NodeRange r = range_of(ix, z);
    if (r.last - r.first + 1 <= 32) {
      out->push_back(z);
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
}

// Filtre exact d'une ancre depuis les handles, puis SEAUX radiaux stables
// (32 seaux par counting sort — l'ordre n'est pas contractuel, audit § 6.1).
void anchor_cover_from_handles(const CloudIndex& ix,
                               const std::vector<NodeRef>& handles, const P3& pa,
                               const P3& pb, i64 D2, std::vector<CoverPoint>* out,
                               u64* point_visits) {
  out->clear();
  const i64 m2[3] = {pa.x + pb.x, pa.y + pb.y, pa.z + pb.z};
  const i64 bound = 3 * D2;
  for (const NodeRef h : handles) {
    const NodeRange r = range_of(ix, h);
    for (i32 u = r.first; u <= r.last; ++u) {
      ++*point_visits;
      const P3& p = ix.upos[(size_t)u];
      const i64 t0 = 2 * p.x - m2[0];
      const i64 t1 = 2 * p.y - m2[1];
      const i64 t2 = 2 * p.z - m2[2];
      const i64 d2 = t0 * t0 + t1 * t1 + t2 * t2;
      if (d2 <= bound) out->push_back(CoverPoint{u, d2});
    }
  }
  // Counting sort en 32 seaux sur dist2q (stable, sans tri par ancre).
  constexpr int kBins = 32;
  u32 cnt[kBins + 1] = {};
  const auto bin_of = [&](i64 d2) {
    return (int)((i128)kBins * d2 / (bound + 1));
  };
  for (const CoverPoint& cp : *out) ++cnt[bin_of(cp.dist2q) + 1];
  for (int b = 1; b <= kBins; ++b) cnt[b] += cnt[b - 1];
  std::vector<CoverPoint> tmp(out->size());
  for (const CoverPoint& cp : *out) tmp[cnt[bin_of(cp.dist2q)]++] = cp;
  out->swap(tmp);
}

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  const Args a = parse(argc, argv);
  if (!a.family_ok || a.n < 4 || a.s < 1) {
    std::fprintf(stderr, "REFUS : arguments invalides\n");
    return 2;
  }
  const int coord = a.coord > 0 ? a.coord : cloud_family_default_coord(a.family, a.n);
  const std::vector<P3> pts = make_family_cloud(a.family, a.n, coord, a.seed);
  if (a.smax > 11) {
    // Contrat de capacite (audit cible 5964214, § 2) : profil K_max <= 10,
    // tampons de certificats et d'interieurs dimensionnes h_3 - 1 <= 8.
    // Refus explicite plutot qu'une troncature silencieuse des paquets.
    std::fprintf(stderr, "REFUS : profil K_max<=10 (smax<=11) — smax=%llu\n",
                 (unsigned long long)a.smax);
    return 2;
  }
  const u64 smax_eff = std::min<u64>(a.smax, pts.size());
  if (smax_eff < 5) {
    std::fprintf(stderr, "REFUS : s_max effectif trop petit\n");
    return 2;
  }
  const u64 h3 = lane_h(Lane::kQ3, smax_eff);
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax_eff), h3, lane_h(Lane::kQ4, smax_eff)};
  const auto t0 = std::chrono::steady_clock::now();
  const CloudIndex ix = build_cloud_index(pts);
  if ((size_t)ix.unique_count() != pts.size()) {
    std::fprintf(stderr, "REFUS unsupported_degeneracy : positions dupliquees\n");
    return 2;
  }

  // 1. WSPD ternaire, lane q3 seulement : blocs morts / rectangles vivants.
  struct AliveQ3 { WspdRect r; u64 core; };
  std::vector<AliveQ3> alive;
  if (!ix.nodes.empty()) {
    std::vector<WspdRect> wave, next;
    for (const RadixNode& n : ix.nodes) wave.push_back(WspdRect{n.left, n.right});
    while (!wave.empty()) {
      next.clear();
      for (const WspdRect& r : wave) {
        const FusedCounts fc =
            count_universal_witnesses_234(ix, r.a, r.b, h_of, 0b010, false);
        if (fc.c[1] >= h3) continue;  // bloc mort pour q3
        i64 ba[3], bb[3];
        const auto va = detail::node_view(ix, r.a, ba);
        const auto vb = detail::node_view(ix, r.b, bb);
        if (detail::separated(va, vb, a.s, 1)) {
          const FusedCounts ff =
              count_universal_witnesses_234(ix, r.a, r.b, h_of, 0b010, true);
          if (ff.c[1] < h3) alive.push_back(AliveQ3{r, ff.c[1]});
          continue;
        }
        const i64 w2a = detail::box_w2(va);
        const i64 w2b = detail::box_w2(vb);
        const bool split_a = (r.a >= 0) && (r.b < 0 || w2a >= w2b);
        const NodeRef keep = split_a ? r.b : r.a;
        const RadixNode& n = ix.nodes[(size_t)(split_a ? r.a : r.b)];
        next.push_back(split_a ? WspdRect{n.left, keep} : WspdRect{keep, n.left});
        next.push_back(split_a ? WspdRect{n.right, keep} : WspdRect{keep, n.right});
      }
      wave.swap(next);
    }
  }
  const auto t1 = std::chrono::steady_clock::now();

  // 2. Instruction. Raccords de l'audit du 17 aout : vrais PointId (apres
  // refus des doublons, un par position unique), filtre h_coeur+h_a+h_b
  // AVANT l'expansion des ancres (autorite 8 coins, disjonction prouvee),
  // exact-once visible, refus transactionnel des coquilles en mode --exact.
  std::vector<Q3Event> records;
  u64 anchors_seen = 0, anchors_killed_ha = 0, carriers_seen = 0,
      shell_refused = 0, raw_events = 0, power_tests = 0, interior_ids_total = 0,
      rect_cover_nodes = 0, anchor_point_visits = 0;
  std::vector<PointId> pid_of((size_t)ix.unique_count());
  for (size_t u = 0; u < pid_of.size(); ++u)
    pid_of[u] = ix.bucket_ids[ix.bucket_start[u]];
  const auto pid = [&](i32 u) { return pid_of[(size_t)u]; };
  std::vector<CoverPoint> cover;
  std::vector<u64> ha, hb;
  u64 cover_points = 0;
  for (const AliveQ3& ar : alive) {
    const NodeRange ra = range_of(ix, ar.r.a);
    const NodeRange rb = range_of(ix, ar.r.b);
    const AxisBox boxA = box_of_node(ix, ar.r.a);
    const AxisBox boxB = box_of_node(ix, ar.r.b);
    const int na = ra.last - ra.first + 1;
    const int nb = rb.last - rb.first + 1;
    const u64 need = h3 - ar.core;
    ha.assign((size_t)na, 0);
    hb.assign((size_t)nb, 0);
    for (int ia = 0; ia < na; ++ia)
      for (int iz = 0; iz < na; ++iz) {
        if (iz == ia) continue;
        if (universal_over_corners(Lane::kQ3, ix.upos[(size_t)(ra.first + ia)],
                                   boxB, ix.upos[(size_t)(ra.first + iz)]))
          ++ha[(size_t)ia];
      }
    for (int ib = 0; ib < nb; ++ib)
      for (int iz = 0; iz < nb; ++iz) {
        if (iz == ib) continue;
        if (universal_over_corners(Lane::kQ3, ix.upos[(size_t)(rb.first + ib)],
                                   boxA, ix.upos[(size_t)(rb.first + iz)]))
          ++hb[(size_t)ib];
      }
    // Paquets d'IDs certifies (audit § 5.1) : cœur du rectangle une fois,
    // h_a/h_b par extremite pendant les boucles ci-dessus re-executees en
    // mode collecte. Rectangle vivant => cœur < h_3 <= 9 : tampons de 8.
    std::vector<NodeRef> handles;
    if (a.cover_rect)
      rect_cover_handles(ix, boxA, boxB, a.inject_cover_dmin, &handles,
                         &rect_cover_nodes);
    i32 core_ids[8];
    const u64 core_n =
        a.packet ? collect_universal_ids_q3(ix, ar.r.a, ar.r.b, 8, core_ids) : 0;
    if (a.packet && core_n != ar.core) {
      // INVARIANT (audit § 2) : le collecteur du cœur doit rendre exactement
      // h_cœur IDs — une divergence compteur/collecteur modifie l'objet.
      std::fprintf(stderr, "INVARIANT : core_ids=%llu != h_coeur=%llu\n",
                   (unsigned long long)core_n, (unsigned long long)ar.core);
      return 3;
    }
    std::vector<std::array<i32, 8>> ha_ids((size_t)na), hb_ids((size_t)nb);
    std::vector<u8> ha_idn((size_t)na, 0), hb_idn((size_t)nb, 0);
    if (a.packet) {
      for (int ia = 0; ia < na; ++ia)
        for (int iz = 0; iz < na && ha_idn[(size_t)ia] < 8; ++iz) {
          if (iz == ia) continue;
          if (universal_over_corners(Lane::kQ3, ix.upos[(size_t)(ra.first + ia)],
                                     boxB, ix.upos[(size_t)(ra.first + iz)]))
            ha_ids[(size_t)ia][ha_idn[(size_t)ia]++] = ra.first + iz;
        }
      for (int ib = 0; ib < nb; ++ib)
        for (int iz = 0; iz < nb && hb_idn[(size_t)ib] < 8; ++iz) {
          if (iz == ib) continue;
          if (universal_over_corners(Lane::kQ3, ix.upos[(size_t)(rb.first + ib)],
                                     boxA, ix.upos[(size_t)(rb.first + iz)]))
            hb_ids[(size_t)ib][hb_idn[(size_t)ib]++] = rb.first + iz;
        }
    }
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        ++anchors_seen;
        if (ha[(size_t)(ua - ra.first)] + hb[(size_t)(ub - rb.first)] >= need) {
          ++anchors_killed_ha;
          continue;
        }
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        if (a.cover_rect)
          anchor_cover_from_handles(ix, handles, pa, pb, D2, &cover,
                                    &anchor_point_visits);
        else
          cover_query(ix, pa, pb, D2, &cover);
        cover_points += cover.size();
        // Paquet de l'ancre : cœur ∪ ha(a) ∪ hb(b), disjoints par theoreme,
        // total < h_3 (l'ancre a surveccu). Chaque ID est STRICTEMENT
        // interieur a toute circum-boule de l'ancre : profondeur de base.
        i32 packet_ids[26];
        u64 base = 0;
        if (a.packet) {
          const u8 nha = ha_idn[(size_t)(ua - ra.first)];
          const u8 nhb = hb_idn[(size_t)(ub - rb.first)];
          // INVARIANTS (audit § 2) : sur une ancre SURVIVANTE, les
          // collecteurs h_a/h_b n'ont pas pu saturer leur tampon — ils
          // doivent rendre exactement les comptes de l'histogramme, et le
          // paquet est sans doublon (theoreme de disjonction, verifie).
          if (nha != ha[(size_t)(ua - ra.first)] ||
              nhb != hb[(size_t)(ub - rb.first)]) {
            std::fprintf(stderr, "INVARIANT : collecteur ha/hb != histogramme\n");
            return 3;
          }
          for (u64 t = 0; t < core_n; ++t) packet_ids[base++] = core_ids[t];
          for (u8 t = 0; t < nha; ++t)
            packet_ids[base++] = ha_ids[(size_t)(ua - ra.first)][t];
          for (u8 t = 0; t < nhb; ++t)
            packet_ids[base++] = hb_ids[(size_t)(ub - rb.first)][t];
          if (base != core_n + nha + nhb || base >= h3) {
            std::fprintf(stderr, "INVARIANT : taille de paquet %llu\n",
                         (unsigned long long)base);
            return 3;
          }
          for (u64 t = 0; t < base; ++t)
            for (u64 t2 = t + 1; t2 < base; ++t2)
              if (packet_ids[t] == packet_ids[t2]) {
                std::fprintf(stderr, "INVARIANT : doublon dans le paquet\n");
                return 3;
              }
        }
        const auto in_packet = [&](i32 u) {
          if (a.inject_no_exclude) return false;  // MUTANT : double compte
          for (u64 t = 0; t < base; ++t)
            if (packet_ids[t] == u) return true;
          return false;
        };
        // Porteurs de l'ancre (SoA), puis scan SITE-MAJOR : chaque site du
        // cover defile devant tous les porteurs actifs (audit § 6) — memes
        // predicats, saturation par masque, InteriorIds collectes.
        // CANDIDAT COMPLET (audit cible 5964214, § 1) : support, owner,
        // BallKey et niveau sont formes ICI, AVANT tout census — le census
        // n'ajoutera que la profondeur et la liste des interieurs.
        struct Carrier {
          i32 ux;
          Q3Form f;
          SupportKey3 support;
          EdgeKey owner;
          Q3BallForm ball_raw;  // forme pre-census ; canonisee a la publication
          Q3Level level_raw;
          u64 depth;
          u8 ni;
          bool alive, shell;
          i32 interior[8];
        };
        std::vector<Carrier> carriers;
        for (const CoverPoint& cp : cover) {
          const i32 ux = cp.u;
          if (ux == ua || ux == ub) continue;
          const P3& px = ix.upos[(size_t)ux];
          if (p3_norm2(p3_sub(px, pa)) > D2 || p3_norm2(p3_sub(px, pb)) > D2)
            continue;
          const P3 v{2 * px.x - pa.x - pb.x, 2 * px.y - pa.y - pb.y,
                     2 * px.z - pa.z - pb.z};
          if (p3_norm2(v) <= D2) continue;
          const i64 l_ax = p3_norm2(p3_sub(px, pa));
          const i64 l_bx = p3_norm2(p3_sub(px, pb));
          if (!anchor_owns_q3(D2, l_ax, l_bx, pid(ua), pid(ub), pid(ux)))
            continue;
          ++carriers_seen;
          Carrier c{ux,
                    q3_form(pa, pb, px),
                    support_key3(pid(ua), pid(ub), pid(ux)),
                    edge_key(pid(ua), pid(ub)),
                    Q3BallForm{},
                    Q3Level{},
                    base,
                    0,
                    true,
                    false,
                    {}};
          c.ball_raw = q3_ball_form(c.f);
          c.level_raw = q3_level_raw(pa, pb, px);
          if (c.level_raw.num <= 0 || c.level_raw.den <= 0) {
            std::fprintf(stderr, "INVARIANT : niveau non positif\n");
            return 3;
          }
          carriers.push_back(c);
        }
        u64 active = carriers.size();
        if (a.census_cover) {
          for (const CoverPoint& wz : cover) {
            if (active == 0) break;
            if (wz.u == ua || wz.u == ub || in_packet(wz.u)) continue;
            const P3& pz = ix.upos[(size_t)wz.u];
            for (Carrier& c : carriers) {
              if (!c.alive || c.ux == wz.u) continue;
              ++power_tests;
              const i128 pw = q3_power(c.f, pz);
              if (pw < 0) {
                if (c.ni < 8) c.interior[c.ni++] = wz.u;
                if (++c.depth >= h3) {
                  c.alive = false;
                  --active;
                }
              } else if (pw == 0) {
                c.shell = true;
              }
            }
          }
        } else {
          for (Carrier& c : carriers) {
            u64 shell = 0;
            c.ni = 0;
            const u64 extra = q3_ball_depth(ix, c.f, ua, ub, c.ux, h3, &shell,
                                            false, c.interior, &c.ni);
            c.depth = extra;  // reference sans paquet : profondeur complete
            c.alive = extra < h3;
            c.shell = shell > 0;
          }
        }
        for (const Carrier& c : carriers) {
          if (!c.alive) continue;
          if (c.shell) {
            if (a.exact_mode) {
              std::fprintf(stderr,
                           "unsupported_degeneracy : extra-shell sur une boule "
                           "survivante (aucune publication partielle)\n");
              return 2;
            }
            ++shell_refused;
            continue;
          }
          ++raw_events;
          // Interieurs du record : prefixe = paquet certifie (census=cover),
          // vide en census=tree ou le collecteur a tout enumere lui-meme.
          // Tampon 16 : un mutant peut gonfler base+ni au-dela de 8 ; c'est
          // l'invariant ci-dessous qui rejette, jamais un debordement.
          i32 interior_u[16];
          u64 n_interior = 0;
          if (a.census_cover)
            for (u64 t = 0; t < base; ++t) interior_u[n_interior++] = packet_ids[t];
          for (u8 t = 0; t < c.ni; ++t) interior_u[n_interior++] = c.interior[t];
          if (n_interior != c.depth) {
            // INVARIANT (audit § 2) : la liste d'interieurs EST la
            // profondeur — toute divergence modifie l'objet mathematique.
            std::fprintf(stderr,
                         "INVARIANT : interieurs=%llu != profondeur=%llu\n",
                         (unsigned long long)n_interior,
                         (unsigned long long)c.depth);
            return 3;
          }
          interior_ids_total += n_interior;
          records.push_back(make_event(c.support, c.owner,
                                       q3_ball_key_reduce(c.ball_raw),
                                       q3_level_reduce(c.level_raw), interior_u,
                                       n_interior, pid_of));
        }
      }
  }
  // stable_sort : contourne un faux positif -Warray-bounds du heapsort de
  // GCC 13 sur les structs larges ; l'ordre d'un multiensemble est le meme.
  std::stable_sort(records.begin(), records.end());
  u64 duplicate_supports = 0;
  for (size_t t = 1; t < records.size(); ++t)
    if (records[t].support == records[t - 1].support) ++duplicate_supports;
  std::vector<Q3BallKey> ball_keys;
  ball_keys.reserve(records.size());
  for (const Q3Event& e : records) ball_keys.push_back(e.ball);
  std::sort(ball_keys.begin(), ball_keys.end());
  ball_keys.erase(std::unique(ball_keys.begin(), ball_keys.end()), ball_keys.end());
  const u64 unique_ballkeys = ball_keys.size();
  const auto t2 = std::chrono::steady_clock::now();

  // 3. Juge par RECORDS COMPLETS (oracle borne) : owner sur les vrais
  // PointId (audit cible 5964214, § 3 — plus jamais les rangs Morton), puis
  // support/owner/ball/level/profondeur/interieurs compares en multiensemble.
  u64 missing = 0, extra = 0;
  if (a.judge) {
    std::vector<Q3Event> truth;
    const int m = ix.unique_count();
    for (i32 i = 0; i < m; ++i)
      for (i32 j = i + 1; j < m; ++j)
        for (i32 k = j + 1; k < m; ++k) {
          const P3 &p1 = ix.upos[(size_t)i], &p2 = ix.upos[(size_t)j],
                   &p3v = ix.upos[(size_t)k];
          const i64 l12 = p3_norm2(p3_sub(p2, p1));
          const i64 l13 = p3_norm2(p3_sub(p3v, p1));
          const i64 l23 = p3_norm2(p3_sub(p3v, p2));
          // Owner brut : arete de longueur max, tie par EdgeKey minimale
          // sur les PIDS EXTERNES.
          struct E { i64 l; i32 x, y, apex; };
          E es[3] = {{l12, i, j, k}, {l13, i, k, j}, {l23, j, k, i}};
          int best = 0;
          for (int e = 1; e < 3; ++e) {
            if (es[e].l > es[best].l ||
                (es[e].l == es[best].l &&
                 edge_key_less(edge_key(pid(es[e].x), pid(es[e].y)),
                               edge_key(pid(es[best].x), pid(es[best].y)))))
              best = e;
          }
          const P3& oa = ix.upos[(size_t)es[best].x];
          const P3& ob = ix.upos[(size_t)es[best].y];
          const P3& ox = ix.upos[(size_t)es[best].apex];
          const P3 vv{2 * ox.x - oa.x - ob.x, 2 * ox.y - oa.y - ob.y,
                      2 * ox.z - oa.z - ob.z};
          if (p3_norm2(vv) <= es[best].l) continue;  // pas strictement aigu
          const Q3Form f = q3_form(oa, ob, ox);
          u64 shell = 0;
          i32 interior_u[8];
          u8 ni = 0;
          const u64 depth = q3_ball_depth(ix, f, es[best].x, es[best].y,
                                          es[best].apex, h3, &shell, false,
                                          interior_u, &ni);
          if (depth >= h3 || shell > 0) continue;
          if ((u64)ni != depth) {
            std::fprintf(stderr, "INVARIANT (juge) : collecteur != profondeur\n");
            return 3;
          }
          truth.push_back(make_event(
              support_key3(pid(es[best].x), pid(es[best].y), pid(es[best].apex)),
              edge_key(pid(es[best].x), pid(es[best].y)), q3_ball_key(f),
              q3_exact_level(oa, ob, ox), interior_u, depth, pid_of));
        }
    std::stable_sort(truth.begin(), truth.end());
    std::vector<Q3Event> diff;
    std::set_difference(truth.begin(), truth.end(), records.begin(), records.end(),
                        std::back_inserter(diff));
    missing = diff.size();
    diff.clear();
    std::set_difference(records.begin(), records.end(), truth.begin(), truth.end(),
                        std::back_inserter(diff));
    extra = diff.size();
  }
  const auto t3 = std::chrono::steady_clock::now();

  const auto ms = [](auto d) {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(d).count() /
           1000.0;
  };
  std::printf(
      "famille=%s n=%d coord=%d s=%lld smax=%llu seed=%lld mode=%s "
      "rect_vivants_q3=%zu ancres_vues=%llu ancres_tuees_ha=%llu "
      "porteurs=%llu evenements=%zu doublons=%llu ev_par_point=%.1f "
      "tests_puissance=%llu ids_interieurs=%llu ballkeys_uniques=%llu noeuds_cover_rect=%llu visites_filtre=%llu "
      "cover_pts=%llu shell_refus=%llu juge_manquants=%llu juge_en_trop=%llu "
      "t_wspd_ms=%.1f t_instruction_ms=%.1f t_juge_ms=%.1f\n",
      cloud_family_name(a.family), a.n, coord, (long long)a.s,
      (unsigned long long)smax_eff, a.seed,
      a.exact_mode ? "exact" : "regular_subset_diagnostic", alive.size(),
      (unsigned long long)anchors_seen, (unsigned long long)anchors_killed_ha,
      (unsigned long long)carriers_seen, records.size(),
      (unsigned long long)duplicate_supports,
      (double)records.size() / (double)pts.size(), (unsigned long long)power_tests,
      (unsigned long long)interior_ids_total, (unsigned long long)unique_ballkeys,
      (unsigned long long)rect_cover_nodes,
      (unsigned long long)anchor_point_visits,
      (unsigned long long)cover_points, (unsigned long long)shell_refused, (unsigned long long)missing,
      (unsigned long long)extra, ms(t1 - t0), ms(t2 - t1), ms(t3 - t2));

  if (a.inject_cover_dmin) {
    if (a.judge && (missing > 0 || extra > 0)) {
      std::printf("MUTANT TUE : cover-dmin perd des temoins\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : cover-dmin non discrimine\n");
    return 3;
  }
  if (a.inject_no_exclude) {
    if (a.judge && (missing > 0 || extra > 0)) {
      std::printf("MUTANT TUE : packet-no-exclude altere les evenements\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : packet-no-exclude non discrimine\n");
    return 3;
  }
  // Regime regulier : BallKeys uniques ⟺ evenements (un partage de sphere
  // entre deux supports serait une cospherite deja refusee).
  if (unique_ballkeys != records.size()) {
    std::fprintf(stderr, "INVARIANT : %llu BallKeys pour %zu evenements\n",
                 (unsigned long long)unique_ballkeys, records.size());
    return 3;
  }
  if (duplicate_supports > 0) {
    std::fprintf(stderr, "EXACT-ONCE VIOLE : %llu supports dupliques\n",
                 (unsigned long long)duplicate_supports);
    return 3;
  }
  if (a.judge && (missing > 0 || extra > 0)) return 1;
  if (records.size() < a.min_events) {
    std::fprintf(stderr, "PLANCHER : %zu evenements < %llu\n", records.size(),
                 (unsigned long long)a.min_events);
    return 3;
  }
  return 0;
}
