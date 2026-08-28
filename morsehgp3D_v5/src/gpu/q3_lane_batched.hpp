// MorseHGP3D v5 — lane q3 PAR LOTS (docs/GPU.md, livraison 4, etage hote).
// Le contrat de la future lane device, ecrit et prouve sur CPU :
//   1. build_q3_batch : pour un rectangle vivant, l'hote forme les tableaux
//      plats (sites affines de chaque ancre survivante en SoA, seeds aigus
//      avec leur forme affine, leur candidat pret a emettre) — aucune
//      decision de profondeur n'est prise ici ;
//   2. un EXECUTEUR de scan rend, par seed, le verdict mort/vivant et les
//      compteurs de certification — ici `scan_q3_batch_host` (boucle plate
//      sur q3_scan_seed_shaped) ; le device (k_scan warp-par-seed de
//      device_witness.cu) remplira le meme contrat ;
//   3. emit_q3_batch : l'hote emet les candidats des seeds vivants DANS
//      L'ORDRE des seeds (ancres en (ua, ub), seeds en ordre de cover), donc
//      dans l'ordre exact de la lane de production.
// tests/q3_lane_batched_gate.cpp exige l'egalite VECTEUR A VECTEUR (ordre
// compris) des candidats q3 et l'egalite des compteurs avec generate.hpp.
// Mutant `q3-batched-emit-dead` : les seeds morts sont emis (tue en code 4).
// CONTRAT DE SYNCHRONISME de l'executeur `scan` : il est SYNCHRONE — a son
// retour, les verdicts (et pour q4 les emissions et etages) sont complets,
// l'appelant les lit puis vide le lot. Un futur recouvrement asynchrone
// exigerait des lots possedes ou double-bufferises et un handle de
// completion explicite ; ce n'est pas le contrat actuel.
#pragma once

#include <bit>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "../core/mutants.hpp"
#include "../lanes/edge_cover.hpp"
#include "../lanes/q3.hpp"
#include "../pipeline/float_filter.hpp"
#include "../pipeline/generate.hpp"
#include "q3_scan_shaped.hpp"

namespace mhgp5 {

struct Q3BatchAnchor {
  u32 begin = 0, count = 0;  // tranche des sites dans les tableaux SoA
};
struct Q3BatchSeed {
  SeedAffineD seed;
  u32 anchor = 0;
};
struct Q3BatchVerdict {
  u32 dead = 0, cert_neg = 0, cert_pos = 0, fallback = 0;
};

// Un lot = une suite d'ANCRES (de un ou plusieurs rectangles d'un meme
// ouvrier) ; les vecteurs sont reutilises entre lots.
struct Q3Batch {
  std::vector<i64> u0, u1, u2, q;
  std::vector<Q3BatchAnchor> anchors;
  std::vector<Q3BatchSeed> seeds;
  std::vector<BallCandidate> emit_if_alive;  // un par seed
  std::vector<Q3BatchVerdict> verdicts;      // rempli par l'executeur
  void clear() {
    u0.clear(); u1.clear(); u2.clear(); q.clear();
    anchors.clear(); seeds.clear(); emit_if_alive.clear(); verdicts.clear();
  }
};

// VUE de lot : dimensions et tableaux d'indices seulement — c'est sur elle
// que porte le contrat structurel, ce qui permet de graver la limite
// UINT32_MAX sans allocation geante (tests/batch_contract_gate.cpp).
struct Q3BatchView {
  size_t n_sites = 0, n_seeds = 0, n_anchors = 0, n_emit = 0, n_verdicts = 0;
  size_t n_u1 = 0, n_u2 = 0, n_q = 0;  // tailles des autres SoA
  const Q3BatchAnchor* anchors = nullptr;
  const Q3BatchSeed* seeds = nullptr;
};
inline Q3BatchView q3_batch_view(const Q3Batch& b) {
  Q3BatchView v;
  v.n_sites = b.u0.size(); v.n_u1 = b.u1.size(); v.n_u2 = b.u2.size(); v.n_q = b.q.size();
  v.n_seeds = b.seeds.size(); v.n_anchors = b.anchors.size(); v.n_emit = b.emit_if_alive.size();
  v.n_verdicts = b.verdicts.size();
  v.anchors = b.anchors.data(); v.seeds = b.seeds.data();
  return v;
}
// CONTRAT STRUCTUREL d'un lot q3, verifie AVANT tout scan (fail-closed :
// false + motif) : sites < 2^32 (indices u32 des kernels), quatre SoA de meme
// taille, tranche de chaque ancre dans les sites (begin + count <= n_sites,
// sans debordement), ancre de chaque seed valide, un candidat par seed.
inline bool validate_q3_batch_view(const Q3BatchView& v, std::string* why) {
  if (v.n_sites > (size_t)UINT32_MAX) { *why = "lot q3 : plus de 2^32 - 1 sites"; return false; }
  if ((v.n_anchors && !v.anchors) || (v.n_seeds && !v.seeds)) { *why = "lot q3 : pointeur nul avec un compte non nul"; return false; }
  if (v.n_u1 != v.n_sites || v.n_u2 != v.n_sites || v.n_q != v.n_sites) { *why = "lot q3 : tailles SoA differentes"; return false; }
  if (v.n_emit != v.n_seeds) { *why = "lot q3 : un candidat par seed attendu"; return false; }
  if (v.n_anchors > (size_t)UINT32_MAX || v.n_seeds > (size_t)UINT32_MAX) { *why = "lot q3 : plus de 2^32 - 1 ancres ou seeds"; return false; }
  for (size_t a = 0; a < v.n_anchors; ++a) {
    const u64 end = (u64)v.anchors[a].begin + v.anchors[a].count;
    if (end > v.n_sites) { *why = "lot q3 : tranche d'ancre hors des sites"; return false; }
  }
  for (size_t i = 0; i < v.n_seeds; ++i)
    if (v.seeds[i].anchor >= v.n_anchors) { *why = "lot q3 : ancre de seed invalide"; return false; }
  return true;
}
inline bool validate_q3_batch(const Q3Batch& b, std::string* why) { return validate_q3_batch_view(q3_batch_view(b), why); }
// Apres le scan : un verdict par seed.
inline bool validate_q3_verdicts(const Q3Batch& b, std::string* why) {
  if (b.verdicts.size() != b.seeds.size()) { *why = "lot q3 : un verdict par seed attendu apres le scan"; return false; }
  return true;
}

// Seuil de vidage en seeds (livraison 5 : LOTISSEMENT multi-rectangles). Un
// ouvrier traite ses rectangles en sequence et les AJOUTE au meme lot ; le
// PREFLIGHT de chaque ancre (cover et seeds comptes avant toute ecriture) vide
// le lot avant l'ajout qui le ferait depasser, et envoie au corps de
// production toute ancre qui ne tient pas seule dans un lot : la BORNE DURE
// d'un lot est le seuil lui-meme (mesuree et gravee : max_lot_seeds,
// max_ancre_seeds, anchors_oversized). L'ordre LOCAL de
// chaque ouvrier (rectangles, ancres, seeds) est preserve ; l'ordre brut
// global a plusieurs fils n'est pas specifie (tirage dynamique, fusion par
// ouvrier), seule la sortie post-RLE l'est. Un lot = un lancement device.
inline constexpr size_t kSeedsPerLaunch = (size_t)1 << 16;
// Seuil de vidage en SITES (memoire : q4 materialise 60 octets par site — sept
// i64 et un PointId — par ouvrier) : borne dure d'un lot = le seuil (preflight).
inline constexpr size_t kSitesPerLaunch = (size_t)1 << 20;

// Seuil de vidage en PAIRES (seed, y) pour q4 : le kernel de completion
// ecrit un octet par paire (etage) et l'hote les rapatrie ; sans borne, un
// lot de 2^16 seeds sur des lentilles de milliers de sites atteint des
// centaines de Mo par ouvrier (session G4 2e75cb42 : 119 Go de RSS, abort).
inline constexpr size_t kPairsPerLaunch = (size_t)1 << 24;

// Seuils de lot (seeds, sites, paires) — l'un ou l'autre declenche le vidage.
// Politique de ROUTAGE par ancre (executeur adaptatif) : une ancre dont le
// cover a au moins `device_min_sites` sites va au lot de l'executeur externe
// (device) ; les autres sont traitees IMMEDIATEMENT par la lane de production
// elle-meme (generate_detail::scan_anchor_q3 / process_anchor_q4 — la meme
// fonction que generate_candidates), sans materialisation. L'objet post-RLE
// ne depend pas de la politique (portes `route_*`) ; seul l'ordre brut
// d'emission en depend (compare a un fil uniquement en routage nul).
// 1 = tout au device (defaut) ; SIZE_MAX = tout a l'hote (= production).
struct BatchLimits {
  size_t seeds = kSeedsPerLaunch;
  size_t sites = kSitesPerLaunch;
  size_t pairs = kPairsPerLaunch;  // q4 seulement (estimation : seeds de l'ancre x sites de sa lentille)
  size_t device_min_sites = 1;
  size_t pretest_query_min_points = kPretestQueryMinPoints;  // politique des pretests (generate.hpp)
  // Grille de cellules : UNE seule autorite, GenerateOptions::cell_grid_min_sites
  // (l'option CLI --cell-min-sites gouverne la production ET les lanes par
  // lots/device ; 0 = mode force des tests) — aucun doublon ici.
  int gpu_executors = 4;  // pool d'executeurs device persistant (G0), 1..8
};

// HISTOGRAMME PAR CLASSE log2 (instrument recevable, audit du 28 aout 2026 :
// percentiles des lots sans allocation par lot). Classe 0 = valeur 0 ;
// classe k >= 1 = [2^(k-1), 2^k). 65 compteurs fixes. Le quantile q est
// rendu comme l'indice k de la classe qui contient la valeur de rang
// ceil(q * n) (n = nombre d'echantillons) : la valeur est < 2^k et, si k >= 1,
// >= 2^(k-1) — une borne, jamais une valeur interpolee.
struct Log2Hist {
  u64 count[65] = {};
  u64 n = 0;
  static int class_of(u64 v) {
    if (v == 0) return 0;
    const int k = 64 - std::countl_zero(v);
    return MHGP5_MUTANT("log2hist-class-shift") ? (k < 64 ? k + 1 : k) : k;  // mutant : classe decalee d'un cran
  }
  void add(u64 v) { ++count[class_of(v)]; ++n; }
  void add_from(const Log2Hist& o) {
    for (int k = 0; k < 65; ++k) count[k] += o.count[k];
    n += o.n;
  }
  // q dans ]0, 1] ; n == 0 rend 0 (aucun lot : classe de la valeur 0).
  int quantile_class(double q) const {
    if (n == 0) return 0;
    u64 rank = (u64)(q * (double)n);
    if ((double)rank < q * (double)n) ++rank;  // ceil
    if (rank < 1) rank = 1;
    if (rank > n) rank = n;
    u64 cum = 0;
    for (int k = 0; k < 65; ++k) {
      cum += count[k];
      if (cum >= rank) return k;
    }
    return 64;
  }
};

// Mesures de lotissement d'une lane (par ouvrier, fusionnees par max/somme).
struct BatchStats {
  u64 flushes = 0, max_lot_seeds = 0, max_anchor_seeds = 0, max_lot_sites = 0, max_anchor_sites = 0;
  u64 max_lot_pairs = 0, max_anchor_pairs = 0;  // q4
  // Distribution des lots vides (un echantillon par flush) : seeds, sites et,
  // en q4, paires estimees (seeds x sites de lentille) — p50/p95 par classe
  // log2, le max exact est max_lot_*.
  Log2Hist lot_seeds, lot_sites, lot_pairs;
  // Routage : `anchors_device` = ancres MATERIALISEES dans un lot device (avec
  // seeds) ; `anchors_host` = ancres traitees par le corps de production
  // (cover sous le seuil, OU ancre trop grande pour un lot : `anchors_oversized`,
  // OU sans seed en q4 : comptees dans `anchors_host`). `seeds_*` : seeds
  // reellement traites de chaque cote (contrat de non-vacuite des portes).
  u64 anchors_device = 0, anchors_host = 0, anchors_oversized = 0, seeds_device = 0, seeds_host = 0;
  void add_from(const BatchStats& o) {
    flushes += o.flushes;
    anchors_device += o.anchors_device; anchors_host += o.anchors_host; anchors_oversized += o.anchors_oversized;
    seeds_device += o.seeds_device; seeds_host += o.seeds_host;
    max_lot_seeds = std::max(max_lot_seeds, o.max_lot_seeds);
    max_anchor_seeds = std::max(max_anchor_seeds, o.max_anchor_seeds);
    max_lot_sites = std::max(max_lot_sites, o.max_lot_sites);
    max_anchor_sites = std::max(max_anchor_sites, o.max_anchor_sites);
    max_lot_pairs = std::max(max_lot_pairs, o.max_lot_pairs);
    max_anchor_pairs = std::max(max_anchor_pairs, o.max_anchor_pairs);
    lot_seeds.add_from(o.lot_seeds); lot_sites.add_from(o.lot_sites); lot_pairs.add_from(o.lot_pairs);
  }
};

// Etage 1 : formation du lot (aucun verdict), AJOUT au lot courant ; `flush`
// est appele apres chaque ancre des que le lot atteint `threshold` seeds.
// Compte anchors/anchors_killed_hist/seeds comme la lane de production.
// Routage hote : l'ancre est traitee par la LANE DE PRODUCTION elle-meme
// (generate_detail::scan_anchor_q3, la meme fonction que generate_candidates)
// et emise immediatement dans `lo` ; seules les ancres routees au device sont
// materialisees dans le lot.
template <class Flush>
inline void build_q3_batch(const CloudIndex& ix, const AliveRect& ar, const u64 h_of[3], bool float_on, bool nonstrict,
                           generate_detail::AnchorScratch& sc, Q3Batch* bdev, std::vector<BallCandidate>* lo,
                           GenerateStats* ls, const BatchLimits& lim, BatchStats* bs, Flush&& flush) {
  using namespace generate_detail;
  corner_histograms(ix, Lane::kQ3, ar.r, &sc.ha, &sc.hb);
  const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
  rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
  sc.handle_points = 0;
  for (const NodeRef h : sc.handles) { const NodeRange r = ix.range_of(h); sc.handle_points += (u64)(r.last - r.first + 1); }
  const bool pretest_by_query = sc.handle_points >= lim.pretest_query_min_points;
  if (pretest_by_query) rect_diametral_candidates(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), &sc.query, &sc.cover_nodes);
  const u64 need = h_of[1] - ar.core;
  for (i32 ua = ra.first; ua <= ra.last; ++ua)
    for (i32 ub = rb.first; ub <= rb.last; ++ub) {
      ++ls->anchors[1];
      if (sc.ha[(size_t)(ua - ra.first)] + sc.hb[(size_t)(ub - rb.first)] >= need) {
        ++ls->anchors_killed_hist[1];
        continue;
      }
      const P3& pa = ix.upos[(size_t)ua];
      const P3& pb = ix.upos[(size_t)ub];
      const i64 D2 = p3_norm2(p3_sub(pb, pa));
      if (D2 == 0) continue;
      if (pretest_by_query) {
        const int k = anchor_kill_from_candidates(sc.query, ix.upos, ua, ub, pa, pb, D2, Lane::kQ3, 12, h_of[1]);
        if (k == 1) { ++ls->anchors_killed_w3; continue; }
        if (k == 2) { ++ls->anchors_killed_sectors[1]; continue; }
      }
      anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, 3, &sc.cover, &sc.visits, &sc.cover_tmp);
      if (!pretest_by_query) {
        const int k = anchor_kill_cumulated(sc.cover, ix.upos, ua, ub, pa, pb, D2, Lane::kQ3, 12, h_of[1]);
        if (k == 1) { ++ls->anchors_killed_w3; continue; }
        if (k == 2) { ++ls->anchors_killed_sectors[1]; continue; }
      }
      // Grille de cellules (theoreme 10.5) — le MEME etage que la production
      // (anchor_grid_stage : politique, construction, compteurs), une seule
      // fois par ancre : ancre entiere ou centre de chaque seed (les seeds
      // tues ne sont jamais materialises) ; `nseeds` = seeds aigus (preflight).
      size_t nseeds = 0;
      if (anchor_grid_stage(ix, sc, ua, ub, pa, pb, D2, Lane::kQ3, h_of[1], float_on, ls, &nseeds)) continue;
      const i64 d3[3] = {pb.x - pa.x, pb.y - pa.y, pb.z - pa.z};
      // PREFLIGHT (avant toute ecriture) : taille du cover, nombre de seeds
      // aigus ; une ancre sous le seuil de routage, ou TROP GRANDE pour un lot
      // (cover > seuil de sites ou seeds > seuil de seeds), va au corps de
      // production ; sinon le lot est vide AVANT l'ajout qui le ferait
      // depasser (borne dure : un lot <= seuils, jamais seuil + ancre).
      const size_t nc = sc.cover.size();
      const bool ignore_threshold = MHGP5_MUTANT("route-ignore-threshold");
      const bool oversized = nc > lim.sites || nseeds > lim.seeds || nc > (size_t)UINT32_MAX;
      if ((nc < lim.device_min_sites && !ignore_threshold) || oversized) {
        const u64 seeds_before_h = ls->seeds[0];
        // Pretests ET grille deja appliques sur ce cover : le corps hote ne reconstruit rien.
        scan_anchor_q3(ix, sc, ua, ub, pa, pb, D2, h_of[1], float_on, nonstrict, lo, ls, AnchorPretests::kAlreadyAppliedWithGrid);
        ++bs->anchors_host;
        if (oversized) ++bs->anchors_oversized;
        bs->seeds_host += ls->seeds[0] - seeds_before_h;
        continue;
      }
      if (nseeds == 0) {  // rien a scanner : ancre comptee cote hote (aucune materialisation)
        ++bs->anchors_host;
        continue;
      }
      Q3Batch* b = bdev;
      if (b->seeds.size() + nseeds > lim.seeds || b->u0.size() + nc > lim.sites) flush();
      sc.affine_filled = false;
      u32 aidx = std::numeric_limits<u32>::max();
      const size_t seeds_before = b->seeds.size();
      for (const CoverPoint& cp : sc.cover) {
        if (cp.u == ua || cp.u == ub) continue;
        const P3& px = ix.upos[(size_t)cp.u];
        if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cp.u))) continue;
        ++ls->seeds[0];
        if (sc.grid.built && seed_center_cell_dead(sc.grid, q3_form(pa, pb, px), d3)) { ++ls->seeds_killed_cells[1]; continue; }
        if (!sc.affine_filled) {
          // Sites de l'ancre : remplis au premier seed, comme en production, puis
          // copies dans le lot (i64 seulement ; les doubles sont derives a la volee).
          sc.fill_affine_sites(ix, pa, pb, D2);
          aidx = (u32)b->anchors.size();
          const u32 begin = (u32)b->u0.size();
          const size_t nc = sc.cover.size();
          b->anchors.push_back(Q3BatchAnchor{begin, (u32)nc});
          for (size_t i = 0; i < nc; ++i) {
            b->u0.push_back(sc.su0[i]); b->u1.push_back(sc.su1[i]); b->u2.push_back(sc.su2[i]); b->q.push_back(sc.sq[i]);
          }
        }
        const Q3Form f3 = q3_form(pa, pb, px);
        const AffineSeed as(f3, pa, pb, sc, float_on);
        Q3BatchSeed s;
        s.seed.G = di_from_i128(as.G);
        s.seed.N0 = di_from_i128(as.N0);
        s.seed.N1 = di_from_i128(as.N1);
        s.seed.N2 = di_from_i128(as.N2);
        s.seed.Gd = as.Gd; s.seed.Nd0 = as.Nd0; s.seed.Nd1 = as.Nd1; s.seed.Nd2 = as.Nd2; s.seed.bound = as.bound;
        s.anchor = aidx;
        b->seeds.push_back(s);
        b->emit_if_alive.push_back(BallCandidate{q3_ball_key(f3), promote_level(q3_exact_level(pa, pb, px)), 3});
      }
      if (aidx != std::numeric_limits<u32>::max()) {
        const u64 anchor_seeds = (u64)b->seeds.size() - seeds_before;
        bs->max_anchor_seeds = std::max(bs->max_anchor_seeds, anchor_seeds);
        bs->max_anchor_sites = std::max(bs->max_anchor_sites, (u64)nc);
        ++bs->anchors_device;
        bs->seeds_device += anchor_seeds;
        if (b->seeds.size() >= lim.seeds || b->u0.size() >= lim.sites) flush();
      }
    }
}

// Etage 2 (executeur hote) : un verdict par seed, boucle plate — la forme que
// le kernel transcrit (un warp par seed).
inline void scan_q3_batch_host(Q3Batch* b, u32 h3, bool nonstrict) {
  std::string why;
  if (!validate_q3_batch(*b, &why)) throw std::invalid_argument(why);
  b->verdicts.resize(b->seeds.size());
  for (size_t i = 0; i < b->seeds.size(); ++i) {
    const Q3BatchSeed& s = b->seeds[i];
    const Q3BatchAnchor& a = b->anchors[s.anchor];
    const AnchorSitesSoA sites{b->u0.data() + a.begin, b->u1.data() + a.begin, b->u2.data() + a.begin,
                               b->q.data() + a.begin, a.count};
    Q3BatchVerdict v;
    v.dead = q3_scan_seed_shaped(s.seed, sites, h3, std::numeric_limits<u32>::max(), &v.cert_neg, &v.cert_pos,
                                 &v.fallback, nonstrict) ? 1u : 0u;
    b->verdicts[i] = v;
  }
}

// Etage 3 : emission ordonnee et compteurs.
inline void emit_q3_batch(const Q3Batch& b, std::vector<BallCandidate>* lo, GenerateStats* ls) {
  std::string why;
  if (!validate_q3_batch(b, &why) || !validate_q3_verdicts(b, &why)) throw std::invalid_argument(why);
  const bool emit_dead = MHGP5_MUTANT("q3-batched-emit-dead");
  for (size_t i = 0; i < b.seeds.size(); ++i) {
    const Q3BatchVerdict& v = b.verdicts[i];
    ls->float_cert_neg += v.cert_neg;
    ls->float_cert_pos += v.cert_pos;
    ls->float_fallback += v.fallback;
    ls->q3_cert[0] += v.cert_neg;
    ls->q3_cert[1] += v.cert_pos;
    ls->q3_cert[2] += v.fallback;
    if (v.dead && !emit_dead) {
      ++ls->depth_killed[1];
      continue;
    }
    lo->push_back(b.emit_if_alive[i]);
    ++ls->candidates[1];
  }
}

// Lane q3 complete par lots avec un EXECUTEUR quelconque `scan(Q3Batch*, u32 h3,
// bool nonstrict)` (hote : scan_q3_batch_host ; device : q3_lane_device.cuh) :
// meme signature d'effet que la lane q3 de generate_candidates (candidats
// ajoutes a `out`, stats cumulees).
template <class Scan>
inline void generate_q3_batched_with(const CloudIndex& ix, const GenerateOptions& opt, std::vector<BallCandidate>* out,
                                     GenerateStats* st, Scan&& scan, BatchLimits lim = BatchLimits{},
                                     BatchStats* batch_stats = nullptr) {
  using namespace generate_detail;
  if (lim.seeds < 1 || lim.sites < 1 || lim.pairs < 1) throw std::invalid_argument("seuils de lot < 1");
  const bool float_on = float_filter_runtime_enabled();
  const bool nonstrict = MHGP5_MUTANT("genfilter-nonstrict");
  const u64 h_of[3] = {lane_h(Lane::kQ2, opt.smax), lane_h(Lane::kQ3, opt.smax), lane_h(Lane::kQ4, opt.smax)};
  std::vector<AliveRect> alive;
  const auto t0 = std::chrono::steady_clock::now();
  alive_rectangles(ix, opt.s, h_of, 1, opt.threads, &alive, &st->rect_visited[1], &st->workers_wspd[1]);
  st->t_wspd_ms[1] += ms_since(t0);
  st->rect_alive[1] = alive.size();
  const auto t1 = std::chrono::steady_clock::now();
  // Comme run_lane (generate.hpp) : brouillons par ouvrier, fusion en ordre
  // d'ouvrier — plus un LOT par ouvrier, vide (scan + emission) au seuil et
  // a la fin de la sequence de l'ouvrier.
  const size_t nrect = alive.size();
  const size_t T = planned_workers(nrect, opt.threads);
  std::vector<std::vector<BallCandidate>> louts(T);
  std::vector<GenerateStats> lst(T);
  std::vector<AnchorScratch> lsc(T);
  for (AnchorScratch& x : lsc) x.cell_min_sites = opt.cell_grid_min_sites;  // une seule autorite (GenerateOptions)
  std::vector<Q3Batch> lb(T);
  std::vector<BatchStats> lbs(T);
  const auto flush = [&](size_t t) {
    Q3Batch& b = lb[t];
    if (b.seeds.empty() && b.anchors.empty()) return;
    lbs[t].max_lot_seeds = std::max(lbs[t].max_lot_seeds, (u64)b.seeds.size());
    lbs[t].max_lot_sites = std::max(lbs[t].max_lot_sites, (u64)b.u0.size());
    lbs[t].lot_seeds.add((u64)b.seeds.size());
    lbs[t].lot_sites.add((u64)b.u0.size());
    ++lbs[t].flushes;
    scan(&b, (u32)h_of[1], nonstrict);
    emit_q3_batch(b, &louts[t], &lst[t]);
    b.clear();
  };
  const size_t created = parallel_items(nrect, (int)T, [&](size_t i, size_t t) {
    build_q3_batch(ix, alive[i], h_of, float_on, nonstrict, lsc[t], &lb[t], &louts[t], &lst[t], lim, &lbs[t], [&] { flush(t); });
  });
  for (size_t t = 0; t < T; ++t) flush(t);
  st->workers_rects[1] = std::max(st->workers_rects[1], (u64)created);
  const bool drop = MHGP5_MUTANT("par-drop-shard");
  for (size_t t = 0; t < T; ++t) {
    if (drop && t == 0 && T > 1) continue;
    out->insert(out->end(), louts[t].begin(), louts[t].end());
    st->add_from(lst[t]);
    if (batch_stats) batch_stats->add_from(lbs[t]);
  }
  st->t_rects_ms[1] += ms_since(t1);
}

inline void generate_q3_batched(const CloudIndex& ix, const GenerateOptions& opt, std::vector<BallCandidate>* out,
                                GenerateStats* st, BatchLimits lim = BatchLimits{}, BatchStats* bs = nullptr) {
  generate_q3_batched_with(ix, opt, out, st, [](Q3Batch* b, u32 h3, bool nonstrict) { scan_q3_batch_host(b, h3, nonstrict); },
                           lim, bs);
}

}  // namespace mhgp5
