// MorseHGP3D v5 — SONDE (jamais un claim) : secteurs q3 atteignables
// par un handle C, pour une ancre exacte (a,b).
//
// Les huit secteurs sont exactement les triangles fermes de sector_kill.hpp.
// Aucune conversion du centre ni aucun angle flottant : la direction du centre
// q3 est celle de la projection transverse de x-m, et deux produits mixtes
// entiers classent cette direction dans les vrais cones. Le masque de Box(C)
// est un SUR-ENSEMBLE : un secteur n'est retire que si une de ses deux
// orientations affines est strictement negative sur toute la boite.
//
// La statistique par handle ne tue pas une ancre. Une seconde cohorte bornee de
// rectangles forme donc aussi l'union de tous les handles pour chaque ancre.
// Cette sonde reste counter-only et q3 seulement.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/lanes/edge_cover.hpp"
#include "../src/lanes/q3.hpp"
#include "../src/lanes/sector_kill.hpp"
#include "../src/pipeline/generate.hpp"

#ifndef MHGP5_PROBE_PIN
#define MHGP5_PROBE_PIN "unstamped"
#endif
#ifndef MHGP5_PROBE_DIRTY
#define MHGP5_PROBE_DIRTY "unknown"
#endif

using namespace mhgp5;

namespace {

struct SectorFrame {
  i64 d[3] = {0, 0, 0};
  i64 p[8][3] = {};
  i128 orientation = 0;
};

struct HandleObservation {
  u8 exact_mask = 0;
  u8 box_mask = 0;
  u64 seeds = 0;
  u64 boundary_seeds = 0;
  bool sampled = false;
};

inline u64 mix64(u64 x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

inline i128 det3(const i64 a[3], const i64 b[3], const i64 c[3]) {
  return (i128)a[0] * ((i128)b[1] * c[2] - (i128)b[2] * c[1])
       - (i128)a[1] * ((i128)b[0] * c[2] - (i128)b[2] * c[0])
       + (i128)a[2] * ((i128)b[0] * c[1] - (i128)b[1] * c[0]);
}

inline bool make_sector_frame(const P3& pa, const P3& pb, i64 d2, SectorFrame* f) {
  i64 u[3], v[3];
  if (!bisector_basis(pa, pb, d2, 12, u, v)) return false;
  f->d[0] = pb.x - pa.x;
  f->d[1] = pb.y - pa.y;
  f->d[2] = pb.z - pa.z;
  for (int i = 0; i < 3; ++i) {
    f->p[0][i] = u[i];
    f->p[1][i] = u[i] + v[i];
    f->p[2][i] = v[i];
    f->p[3][i] = -u[i] + v[i];
    f->p[4][i] = -u[i];
    f->p[5][i] = -u[i] - v[i];
    f->p[6][i] = -v[i];
    f->p[7][i] = u[i] - v[i];
  }
  f->orientation = det3(f->d, f->p[0], f->p[1]);
  return f->orientation != 0;
}

inline i128 oriented(const SectorFrame& f, const i64 a[3], const i64 b[3]) {
  const i128 value = det3(f.d, a, b);
  return f.orientation > 0 ? value : -value;
}

inline void doubled_offset(const P3& pa, const P3& pb, const P3& px, i64 y[3]) {
  y[0] = 2 * px.x - pa.x - pb.x;
  y[1] = 2 * px.y - pa.y - pb.y;
  y[2] = 2 * px.z - pa.z - pb.z;
}

inline u8 exact_sector_mask(const SectorFrame& f, const P3& pa, const P3& pb, const P3& px) {
  i64 y[3];
  doubled_offset(pa, pb, px, y);
  u8 mask = 0;
  for (int k = 0; k < 8; ++k) {
    const int k1 = (k + 1) & 7;
    if (oriented(f, f.p[k], y) >= 0 && oriented(f, y, f.p[k1]) >= 0)
      mask |= (u8)(1u << k);
  }
  return mask;
}

inline i128 box_orientation_max(const SectorFrame& f, const P3& pa, const P3& pb,
                                const AxisBox& box, int k, bool left) {
  i128 best = 0;
  bool first = true;
  for (int corner = 0; corner < 8; ++corner) {
    const P3 x{
        (corner & 1) ? box.hi[0] : box.lo[0],
        (corner & 2) ? box.hi[1] : box.lo[1],
        (corner & 4) ? box.hi[2] : box.lo[2]};
    i64 y[3];
    doubled_offset(pa, pb, x, y);
    const int k1 = (k + 1) & 7;
    const i128 value = left ? oriented(f, f.p[k], y) : oriented(f, y, f.p[k1]);
    if (first || value > best) {
      best = value;
      first = false;
    }
  }
  return best;
}

inline u8 box_sector_mask(const SectorFrame& f, const P3& pa, const P3& pb, const AxisBox& box) {
  u8 mask = 0;
  for (int k = 0; k < 8; ++k) {
    if (box_orientation_max(f, pa, pb, box, k, true) >= 0 &&
        box_orientation_max(f, pa, pb, box, k, false) >= 0)
      mask |= (u8)(1u << k);
  }
  return mask;
}

inline int popcount8(u8 mask) {
  int count = 0;
  for (int k = 0; k < 8; ++k) count += (mask >> k) & 1u;
  return count;
}

inline bool mask_deep(u8 mask, const u32 counts[8], u64 h) {
  if (mask == 0) return false;
  for (int k = 0; k < 8; ++k)
    if ((mask & (u8)(1u << k)) != 0 && (u64)counts[k] < h) return false;
  return true;
}

inline HandleObservation observe_handle(const CloudIndex& ix, NodeRef handle, i32 ua, i32 ub,
                                        const P3& pa, const P3& pb, i64 d2,
                                        const SectorFrame& frame, bool sampled) {
  HandleObservation out;
  out.sampled = sampled;
  out.box_mask = box_sector_mask(frame, pa, pb, ix.box_of(handle));
  const NodeRange rc = ix.range_of(handle);
  for (i32 ux = rc.first; ux <= rc.last; ++ux) {
    if (ux == ua || ux == ub) continue;
    const P3& px = ix.upos[(size_t)ux];
    if (!is_acute_seed(pa, pb, px, d2, ix.point_id(ua), ix.point_id(ub), ix.point_id(ux))) continue;
    const u8 seed_mask = exact_sector_mask(frame, pa, pb, px);
    out.exact_mask |= seed_mask;
    out.boundary_seeds += popcount8(seed_mask) > 1 ? 1 : 0;
    ++out.seeds;
  }
  return out;
}

inline bool sampled_handle(u64 ordinal, u64 total, u64 target, u64 seed) {
  if (target >= total) return true;
  return mix64(ordinal ^ mix64(seed)) % total < target;
}

inline bool sector_mask_selfcheck() {
  {
    const P3 a{100, 100, 100}, b{112, 124, 136};
    const P3 x1{103, 142, 99}, x2{88, 142, 104};
    SectorFrame frame;
    if (!make_sector_frame(a, b, p3_norm2(p3_sub(b, a)), &frame)) return false;
    if (exact_sector_mask(frame, a, b, x1) != 0x01 || exact_sector_mask(frame, a, b, x2) != 0x01) return false;
    AxisBox box;
    box.lo[0] = 88; box.hi[0] = 103;
    box.lo[1] = 142; box.hi[1] = 142;
    box.lo[2] = 99; box.hi[2] = 104;
    if ((box_sector_mask(frame, a, b, box) & 0x01) == 0) return false;
  }
  {
    const P3 a{100, 100, 100}, b{116, 132, 148};
    const P3 x1{81, 146, 113}, x2{75, 146, 115};
    SectorFrame frame;
    if (!make_sector_frame(a, b, p3_norm2(p3_sub(b, a)), &frame)) return false;
    if (exact_sector_mask(frame, a, b, x1) != 0x01 || exact_sector_mask(frame, a, b, x2) != 0x02) return false;
  }
  {
    const P3 a{100, 100, 100}, b{140, 100, 100}, boundary{120, 100, 121};
    SectorFrame frame;
    if (!make_sector_frame(a, b, p3_norm2(p3_sub(b, a)), &frame)) return false;
    if (exact_sector_mask(frame, a, b, boundary) != 0x81) return false;
    AxisBox midpoint;
    midpoint.lo[0] = midpoint.hi[0] = 120;
    midpoint.lo[1] = midpoint.hi[1] = 100;
    midpoint.lo[2] = midpoint.hi[2] = 100;
    if (box_sector_mask(frame, a, b, midpoint) != 0xff) return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 8000;
  int coord = 0;
  int seed = 3;
  size_t target_handles = 3000;
  size_t union_rect_target = 64;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--family=", 0) == 0) {
      if (!parse_cloud_family(arg.c_str() + 9, &family)) return 2;
    } else if (arg.rfind("--n=", 0) == 0) {
      n = std::atoi(arg.c_str() + 4);
    } else if (arg.rfind("--blocs=", 0) == 0) {
      target_handles = (size_t)std::atoll(arg.c_str() + 8);
    } else if (arg.rfind("--union-rects=", 0) == 0) {
      union_rect_target = (size_t)std::atoll(arg.c_str() + 14);
    } else if (arg.rfind("--seed=", 0) == 0) {
      seed = std::atoi(arg.c_str() + 7);
    } else if (arg.rfind("--coord=", 0) == 0) {
      coord = std::atoi(arg.c_str() + 8);
    } else {
      return 2;
    }
  }
  if (n < 4 || target_handles == 0) return 2;
  if (!sector_mask_selfcheck()) {
    std::printf("REFUS : sector_mask_selfcheck\n");
    return 3;
  }
  if (coord <= 0) coord = cloud_family_default_coord(family, n);

  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, seed));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  const u64 smax = 11;
  const u64 h3 = lane_h(Lane::kQ3, smax);
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax), h3, lane_h(Lane::kQ4, smax)};
  std::vector<AliveRect> alive;
  u64 wspd_visits = 0;
  u64 workers = 0;
  generate_detail::alive_rectangles(ix, 8, h_of, 1, 1, &alive, &wspd_visits, &workers);
  generate_detail::AnchorScratch sc;

  u64 total_handles = 0;
  for (const AliveRect& ar : alive) {
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
    total_handles += (u64)sc.handles.size();
  }
  if (total_handles == 0) return 3;
  const u64 handle_target = std::min<u64>((u64)target_handles, total_handles);

  std::vector<std::pair<u64, size_t>> rect_keys;
  rect_keys.reserve(alive.size());
  for (size_t i = 0; i < alive.size(); ++i)
    rect_keys.push_back({mix64((u64)i ^ mix64((u64)(u32)seed + 0xa0761d6478bd642fULL)), i});
  std::sort(rect_keys.begin(), rect_keys.end());
  std::vector<u8> union_rect(alive.size(), 0);
  const size_t union_take = std::min(union_rect_target, rect_keys.size());
  for (size_t i = 0; i < union_take; ++i) union_rect[rect_keys[i].second] = 1;

  u64 handle_ordinal = 0;
  u64 handles_sampled = 0;
  u64 handle_groups = 0;
  u64 handle_nonempty = 0;
  u64 handle_empty = 0;
  u64 handle_seeds = 0;
  u64 handle_multi_sector_groups = 0;
  u64 handle_boundary_seeds = 0;
  u64 handle_box_all8 = 0;
  u64 frame_failures = 0;
  u64 exact_not_subset = 0;
  u64 decision_invariants = 0;
  u64 handle_full8_killed = 0;
  u64 handle_exact_killed = 0;
  u64 handle_box_killed = 0;
  u64 handle_box_gain = 0;
  u64 exact_hist[9] = {};
  u64 box_hist[9] = {};

  u64 anchor_groups = 0;
  u64 anchor_nonempty = 0;
  u64 anchor_empty = 0;
  u64 anchor_seeds = 0;
  u64 anchor_exact_not_subset = 0;
  u64 anchor_full8_killed = 0;
  u64 anchor_exact_killed = 0;
  u64 anchor_box_killed = 0;
  u64 anchor_box_gain = 0;
  u64 anchor_oracle_nonempty_box_killed = 0;
  u64 anchor_oracle_nonempty_box_gain = 0;
  u64 anchor_exact_hist[9] = {};
  u64 anchor_box_hist[9] = {};
  u64 anchor_oracle_nonempty_box_hist[9] = {};
  u64 witness_visits = 0;

  for (size_t ir = 0; ir < alive.size(); ++ir) {
    const AliveRect& ar = alive[ir];
    const NodeRange ra = ix.range_of(ar.r.a);
    const NodeRange rb = ix.range_of(ar.r.b);
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
    std::vector<u8> sampled(sc.handles.size(), 0);
    bool any_sampled = false;
    for (size_t ih = 0; ih < sc.handles.size(); ++ih) {
      const bool take = sampled_handle(handle_ordinal++, total_handles, handle_target, (u64)(u32)seed);
      sampled[ih] = take ? 1 : 0;
      any_sampled = any_sampled || take;
      handles_sampled += take ? 1 : 0;
    }
    const bool measure_union = union_rect[ir] != 0;
    if (!any_sampled && !measure_union) continue;

    for (i32 ua = ra.first; ua <= ra.last; ++ua) {
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 d2 = p3_norm2(p3_sub(pb, pa));
        SectorFrame frame;
        if (d2 == 0 || !make_sector_frame(pa, pb, d2, &frame)) {
          ++frame_failures;
          continue;
        }

        std::vector<HandleObservation> observations;
        observations.reserve(sc.handles.size());
        bool needs_counts = false;
        u8 union_exact = 0;
        u8 union_box = 0;
        u8 union_box_oracle_nonempty = 0;
        u64 union_seeds = 0;
        for (size_t ih = 0; ih < sc.handles.size(); ++ih) {
          if (sampled[ih] == 0 && !measure_union) {
            observations.push_back(HandleObservation{});
            continue;
          }
          const HandleObservation obs = observe_handle(ix, sc.handles[ih], ua, ub, pa, pb, d2, frame, sampled[ih] != 0);
          observations.push_back(obs);
          if (obs.sampled) {
            ++handle_groups;
            if (obs.exact_mask == 0) {
              ++handle_empty;
            } else {
              ++handle_nonempty;
              handle_seeds += obs.seeds;
              needs_counts = true;
            }
          }
          if (measure_union) {
            union_exact |= obs.exact_mask;
            union_box |= obs.box_mask;
            if (obs.exact_mask != 0) union_box_oracle_nonempty |= obs.box_mask;
            union_seeds += obs.seeds;
          }
        }
        if (measure_union) {
          ++anchor_groups;
          if (union_exact == 0) {
            ++anchor_empty;
          } else {
            ++anchor_nonempty;
            anchor_seeds += union_seeds;
            needs_counts = true;
          }
        }
        if (!needs_counts) continue;

        anchor_cover_from_handles(ix, sc.handles, pa, pb, d2, 3, &sc.cover, &witness_visits, &sc.cover_tmp);
        u32 counts[8] = {};
        u64 witness_min = 0;
        const bool full8_killed = anchor_sector_kill(sc.cover, ix.upos, ua, ub, pa, pb, d2, 12, h3,
                                                     &witness_min, counts);

        for (const HandleObservation& obs : observations) {
          if (!obs.sampled || obs.exact_mask == 0) continue;
          const int ne = popcount8(obs.exact_mask);
          const int nb = popcount8(obs.box_mask);
          ++exact_hist[ne];
          ++box_hist[nb];
          handle_multi_sector_groups += ne > 1 ? 1 : 0;
          handle_boundary_seeds += obs.boundary_seeds;
          handle_box_all8 += obs.box_mask == 0xff ? 1 : 0;
          const bool subset = (obs.exact_mask & (u8)~obs.box_mask) == 0;
          exact_not_subset += subset ? 0 : 1;
          const bool exact_killed = mask_deep(obs.exact_mask, counts, h3);
          const bool box_killed = mask_deep(obs.box_mask, counts, h3);
          handle_full8_killed += full8_killed ? 1 : 0;
          handle_exact_killed += exact_killed ? 1 : 0;
          handle_box_killed += box_killed ? 1 : 0;
          handle_box_gain += box_killed && !full8_killed ? 1 : 0;
          if ((!subset && box_killed) || (box_killed && !exact_killed) ||
              (full8_killed && !box_killed)) ++decision_invariants;
        }

        if (measure_union && union_exact != 0) {
          const int ne = popcount8(union_exact);
          const int nb = popcount8(union_box);
          const int no = popcount8(union_box_oracle_nonempty);
          ++anchor_exact_hist[ne];
          ++anchor_box_hist[nb];
          ++anchor_oracle_nonempty_box_hist[no];
          const bool subset = (union_exact & (u8)~union_box) == 0;
          anchor_exact_not_subset += subset ? 0 : 1;
          const bool exact_killed = mask_deep(union_exact, counts, h3);
          const bool box_killed = mask_deep(union_box, counts, h3);
          const bool oracle_nonempty_box_killed = mask_deep(union_box_oracle_nonempty, counts, h3);
          anchor_full8_killed += full8_killed ? 1 : 0;
          anchor_exact_killed += exact_killed ? 1 : 0;
          anchor_box_killed += box_killed ? 1 : 0;
          anchor_box_gain += box_killed && !full8_killed ? 1 : 0;
          anchor_oracle_nonempty_box_killed += oracle_nonempty_box_killed ? 1 : 0;
          anchor_oracle_nonempty_box_gain += oracle_nonempty_box_killed && !full8_killed ? 1 : 0;
          if ((!subset && box_killed) || (box_killed && !exact_killed) ||
              (oracle_nonempty_box_killed && !exact_killed) ||
              (full8_killed && (!box_killed || !oracle_nonempty_box_killed))) ++decision_invariants;
        }
      }
    }
  }

  std::printf("sector_reach_v2 pin=%s worktree_modifie=%s\n", MHGP5_PROBE_PIN, MHGP5_PROBE_DIRTY);
  std::printf("  famille=%s n=%d coord=%d seed=%d h3=%llu alive_rects=%zu handles=%llu sampled=%llu union_rects=%zu\n",
              cloud_family_name(family), n, coord, seed, (unsigned long long)h3, alive.size(),
              (unsigned long long)total_handles, (unsigned long long)handles_sampled, union_take);
  std::printf("  HANDLE LOCAL [ne tue pas l ancre] : groupes=%llu nonvides=%llu vides=%llu seeds=%llu\n",
              (unsigned long long)handle_groups, (unsigned long long)handle_nonempty,
              (unsigned long long)handle_empty, (unsigned long long)handle_seeds);
  std::printf("    masque exact histogramme 1..8 :");
  for (int k = 1; k <= 8; ++k) std::printf(" %llu", (unsigned long long)exact_hist[k]);
  std::printf("\n    surmasque Box(C) histogramme 1..8 :");
  for (int k = 1; k <= 8; ++k) std::printf(" %llu", (unsigned long long)box_hist[k]);
  std::printf("\n    groupes_multi_secteurs=%llu seeds_sur_frontiere=%llu box_all8=%llu exact_hors_box=%llu\n",
              (unsigned long long)handle_multi_sector_groups, (unsigned long long)handle_boundary_seeds,
              (unsigned long long)handle_box_all8,
              (unsigned long long)exact_not_subset);
  std::printf("    killed full8=%llu exact_oracle=%llu box_candidate=%llu gain_box_vs_full8=%llu\n",
              (unsigned long long)handle_full8_killed, (unsigned long long)handle_exact_killed,
              (unsigned long long)handle_box_killed, (unsigned long long)handle_box_gain);

  std::printf("  UNION ANCRE [tous handles des rectangles bottom-k] : groupes=%llu nonvides=%llu vides=%llu seeds=%llu\n",
              (unsigned long long)anchor_groups, (unsigned long long)anchor_nonempty,
              (unsigned long long)anchor_empty, (unsigned long long)anchor_seeds);
  std::printf("    masque exact histogramme 1..8 :");
  for (int k = 1; k <= 8; ++k) std::printf(" %llu", (unsigned long long)anchor_exact_hist[k]);
  std::printf("\n    surmasque Box(C) histogramme 1..8 :");
  for (int k = 1; k <= 8; ++k) std::printf(" %llu", (unsigned long long)anchor_box_hist[k]);
  std::printf("\n    Box(C) des seuls handles reellement non vides [ORACLE] 1..8 :");
  for (int k = 1; k <= 8; ++k) std::printf(" %llu", (unsigned long long)anchor_oracle_nonempty_box_hist[k]);
  std::printf("\n    exact_hors_box=%llu killed full8=%llu exact_oracle=%llu box_candidate=%llu gain_box_vs_full8=%llu\n",
              (unsigned long long)anchor_exact_not_subset, (unsigned long long)anchor_full8_killed,
              (unsigned long long)anchor_exact_killed, (unsigned long long)anchor_box_killed,
              (unsigned long long)anchor_box_gain);
  std::printf("    oracle_nonempty_box_killed=%llu gain_vs_full8=%llu [plafond, pas un candidat]\n",
              (unsigned long long)anchor_oracle_nonempty_box_killed,
              (unsigned long long)anchor_oracle_nonempty_box_gain);
  std::printf("  cout : witness_point_visits=%llu frame_failures=%llu decision_invariants=%llu\n",
              (unsigned long long)witness_visits, (unsigned long long)frame_failures,
              (unsigned long long)decision_invariants);

  if (handles_sampled == 0 || exact_not_subset != 0 || anchor_exact_not_subset != 0 ||
      frame_failures != 0 || decision_invariants != 0) return 3;
  return 0;
}
