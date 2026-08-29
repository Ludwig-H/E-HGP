// MorseHGP3D v5 — porte du filtre EXPERIMENTAL d'enveloppe de cover.
//
// Deux contrats independants :
//   1. arithmetique ponctuelle fermee aux frontieres q3/q4, y compris quand
//      les carres depassent i64 ;
//   2. appariement OFF/ON du pipeline complet : multiensemble brut trie,
//      boules RLE, evenements/niveaux/forets (digests) et statut identiques.
// Les compteurs de travail PEUVENT changer ; la porte exige seulement une
// compaction non vacante dans chaque lane. `--route=cover|query` force le lieu
// des pretests sans changer l'objet. Codes : 0 conforme, 1 desaccord, 2 refus,
// 3 porte vacante, 4 mutant tue.
#include <cstdio>
#include <limits>
#include <string>

#include "../src/cloud/families.hpp"
#include "../src/lanes/edge_cover.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

namespace {

i64 anchor_dist2q(const P3& a, const P3& b, const P3& z) {
  const P3 w{2 * z.x - a.x - b.x, 2 * z.y - a.y - b.y, 2 * z.z - a.z - b.z};
  return p3_norm2(w);
}

bool contains(EdgeEnvelope envelope, const P3& a, const P3& b, const P3& z) {
  const i64 D2 = p3_norm2(p3_sub(b, a));
  return edge_envelope_contains(envelope, a, b, z, D2, anchor_dist2q(a, b, z));
}

i128 xi_by_cross(const P3& a, const P3& b, const P3& z) {
  const P3 d = p3_sub(b, a);
  const P3 w{2 * z.x - a.x - b.x, 2 * z.y - a.y - b.y, 2 * z.z - a.z - b.z};
  const P3 c = p3_cross(d, w);
  return (i128)c.x * c.x + (i128)c.y * c.y + (i128)c.z * c.z;
}

bool oracle_contains(EdgeEnvelope envelope, const P3& a, const P3& b, const P3& z) {
  if (envelope == EdgeEnvelope::kNone) return true;
  const i64 D2 = p3_norm2(p3_sub(b, a));
  const i64 S = anchor_dist2q(a, b, z) - D2;
  if (S <= 0) return true;
  const i128 s2 = (i128)S * S;
  const i128 xi = xi_by_cross(a, b, z);
  return envelope == EdgeEnvelope::kQ3 ? 3 * s2 <= 4 * xi : s2 <= 2 * xi;
}

u64 sum2(const u64 a[2]) { return a[0] + a[1]; }

bool same_candidates(const std::vector<BallCandidate>& a, const std::vector<BallCandidate>& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i)
    if (!(a[i].key == b[i].key) || !(a[i].level == b[i].level) || a[i].arity != b[i].arity) return false;
  return true;
}

std::string digest_events_and_levels(u64 K, const std::vector<ForestEvent>& events,
                                     const ForestResult& forest) {
  digest_detail::Writer d;
  d.tag("mhgp5-test:cover-envelope-events-levels");
  d.u64v(K);
  d.u64v(events.size());
  for (const ForestEvent& e : events) {
    d.u8v(e.q);
    d.u8v(e.d);
    d.u32v(e.active_mask);
    for (u8 i = 0; i < e.q; ++i) d.u32v(e.support[i]);
    for (u8 i = 0; i < e.d; ++i) d.u32v(e.interior[i]);
    d.level(e.level);
  }
  d.u64v(forest.batch_levels.size());
  for (const ExactLevel& level : forest.batch_levels) d.level(level);
  return d.hex();
}

bool same_cards(const std::vector<KCardinalities>& a, const std::vector<KCardinalities>& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i].events != b[i].events || a[i].facets != b[i].facets || a[i].deltas != b[i].deltas ||
        a[i].attachments != b[i].attachments || a[i].fusions != b[i].fusions || a[i].nodes != b[i].nodes)
      return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 300, threads = 2;
  u64 min_removed = 1;
  std::string route = "default", inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--family=", 0) == 0) {
      if (!parse_cloud_family(arg.c_str() + 9, &family)) return 2;
    } else if (arg.rfind("--n=", 0) == 0) {
      n = std::atoi(arg.c_str() + 4);
    } else if (arg.rfind("--threads=", 0) == 0) {
      threads = std::atoi(arg.c_str() + 10);
    } else if (arg.rfind("--min-removed=", 0) == 0) {
      min_removed = (u64)std::atoll(arg.c_str() + 14);
    } else if (arg.rfind("--route=", 0) == 0) {
      route = arg.substr(8);
    } else if (arg.rfind("--inject=", 0) == 0) {
      inject = arg.substr(9);
    } else {
      return 2;
    }
  }
  if (n < 2 || threads < 1 || (route != "default" && route != "cover" && route != "query")) return 2;
  if (!inject.empty() && !mutants_enable(inject)) return 2;

  u64 boundary_bad = 0;
  const auto expect = [&](bool ok, const char* what) {
    if (!ok) {
      std::fprintf(stderr, "frontiere/predicat en echec : %s\n", what);
      ++boundary_bad;
    }
  };

  // Le cœur S<=0 doit rester entier, meme avec Xi=0 ; un point axial du
  // cover coefficient 3 mais hors de toute boule admissible doit etre retire.
  const P3 aa{0, 0, 0}, ab{10, 0, 0};
  expect(contains(EdgeEnvelope::kQ3, aa, ab, P3{5, 0, 0}), "q3 S<=0");
  expect(contains(EdgeEnvelope::kQ4Jung, aa, ab, P3{5, 0, 0}), "q4 S<=0");
  expect(!contains(EdgeEnvelope::kQ3, aa, ab, P3{13, 0, 0}), "q3 axial hors enveloppe");
  expect(!contains(EdgeEnvelope::kQ4Jung, aa, ab, P3{13, 0, 0}), "q4 axial hors enveloppe");

  // Egalite q3 a grande largeur : S=2^32 et 3S²=4Xi=3*2^64.
  const P3 q3a{32768, 32768, 32768}, q3b{0, 0, 32768}, q3z{0, 32768, 0};
  const i64 q3D2 = p3_norm2(p3_sub(q3b, q3a));
  const i64 q3Q = anchor_dist2q(q3a, q3b, q3z);
  const i64 q3S = q3Q - q3D2;
  const P3 q3d = p3_sub(q3b, q3a);
  const P3 q3w{2 * q3z.x - q3a.x - q3b.x, 2 * q3z.y - q3a.y - q3b.y,
               2 * q3z.z - q3a.z - q3b.z};
  const i64 q3dw = p3_dot(q3d, q3w);
  const i128 q3xi = (i128)q3D2 * q3Q - (i128)q3dw * q3dw;
  expect(q3xi == xi_by_cross(q3a, q3b, q3z), "oracle transverse q3 en desaccord");
  expect(q3Q == 3 * q3D2 && 3 * (i128)q3S * q3S == 4 * q3xi,
         "fixture q3 n'est pas sur l'egalite");
  expect(contains(EdgeEnvelope::kQ3, q3a, q3b, q3z), "q3 egalite fermee i128");

  // Egalite q4 a grande largeur : S=6*2^30 et S²=2Xi=36*2^60.
  const P3 q4a{32768, 32768, 32768}, q4b{0, 0, 0}, q4z{0, 49152, 49152};
  const i64 q4D2 = p3_norm2(p3_sub(q4b, q4a));
  const i64 q4Q = anchor_dist2q(q4a, q4b, q4z);
  const i64 q4S = q4Q - q4D2;
  const P3 q4d = p3_sub(q4b, q4a);
  const P3 q4w{2 * q4z.x - q4a.x - q4b.x, 2 * q4z.y - q4a.y - q4b.y,
               2 * q4z.z - q4a.z - q4b.z};
  const i64 q4dw = p3_dot(q4d, q4w);
  const i128 q4xi = (i128)q4D2 * q4Q - (i128)q4dw * q4dw;
  expect(q4xi == xi_by_cross(q4a, q4b, q4z), "oracle transverse q4 en desaccord");
  expect(q4Q == 3 * q4D2 && (i128)q4S * q4S == 2 * q4xi,
         "fixture q4 n'est pas sur l'egalite");
  expect(contains(EdgeEnvelope::kQ4Jung, q4a, q4b, q4z), "q4 egalite fermee i128");
  expect(!contains(EdgeEnvelope::kQ3, q4a, q4b, q4z), "q4 egalite ne discrimine pas q3");

  // Coupes strictes non axiales dans le plan mediateur : y=8 interieur q3,
  // y=9 interieur Jung q4 mais exterieur q3, y=10 exterieur q4 (D=10).
  const P3 sa{10, 10, 10}, sb{20, 10, 10};
  expect(contains(EdgeEnvelope::kQ3, sa, sb, P3{15, 18, 10}), "interieur strict q3");
  expect(!contains(EdgeEnvelope::kQ3, sa, sb, P3{15, 19, 10}) &&
             contains(EdgeEnvelope::kQ4Jung, sa, sb, P3{15, 19, 10}),
         "interieur q4/exterieur q3");
  expect(!contains(EdgeEnvelope::kQ4Jung, sa, sb, P3{15, 20, 10}), "exterieur strict q4");
  for (i64 x = 5; x <= 25; ++x)
    for (i64 y = 0; y <= 30; ++y)
      for (i64 z = 8; z <= 12; ++z) {
        const P3 p{x, y, z};
        expect(contains(EdgeEnvelope::kQ3, sa, sb, p) == oracle_contains(EdgeEnvelope::kQ3, sa, sb, p),
               "oracle produit vectoriel q3");
        expect(contains(EdgeEnvelope::kQ4Jung, sa, sb, p) == oracle_contains(EdgeEnvelope::kQ4Jung, sa, sb, p),
               "oracle produit vectoriel q4");
      }

  // PORTE GEOMETRIQUE INDEPENDANTE : l'oracle precedent recalcule le
  // predicat par un produit vectoriel, mais ne prouve pas son lien avec les
  // boules. Ici les puissances q3/q4 sont l'autorite opposee : aucun point de
  // boule fermee d'un support admissible ne peut etre retire de l'enveloppe.
  u64 q3_supports = 0, q3_ball_points = 0, q3_missed = 0;
  const P3 pqa{4, 4, 4}, pqb{12, 4, 4};
  const i64 pq_d2 = p3_norm2(p3_sub(pqb, pqa));
  for (i64 xx = 0; xx <= 16; xx += 2)
    for (i64 xy = 0; xy <= 12; xy += 2)
      for (i64 xz = 0; xz <= 12; xz += 2) {
        const P3 carrier{xx, xy, xz};
        if (!is_acute_seed(pqa, pqb, carrier, pq_d2, 0, 1, 2)) continue;
        ++q3_supports;
        const Q3Form f = q3_form(pqa, pqb, carrier);
        for (i64 zx = 0; zx <= 16; ++zx)
          for (i64 zy = 0; zy <= 12; ++zy)
            for (i64 zz = 0; zz <= 12; ++zz) {
              const P3 witness{zx, zy, zz};
              if (q3_power(f, witness) > 0) continue;
              ++q3_ball_points;
              if (!contains(EdgeEnvelope::kQ3, pqa, pqb, witness)) ++q3_missed;
            }
      }
  expect(q3_supports > 0 && q3_ball_points > 0, "porte puissance q3 vacante");
  expect(q3_missed == 0, "enveloppe q3 perd un interieur ou une coquille");

  // Tetraedres formes par quatre coins distincts d'un cube. On choisit une
  // arete de diametre, puis on ne garde que les supports dont le circumcentre
  // est strictement interieur. Les tetraedres reguliers alternes rendent la
  // porte non vacante et atteignent l'egalite de Jung.
  P3 cube[8];
  for (int bits = 0; bits < 8; ++bits)
    cube[bits] = P3{(bits & 1) ? 8 : 0, (bits & 2) ? 8 : 0, (bits & 4) ? 8 : 0};
  u64 q4_supports = 0, q4_ball_points = 0, q4_missed = 0;
  for (int i0 = 0; i0 < 8; ++i0)
    for (int i1 = i0 + 1; i1 < 8; ++i1)
      for (int i2 = i1 + 1; i2 < 8; ++i2)
        for (int i3 = i2 + 1; i3 < 8; ++i3) {
          const int ids[4] = {i0, i1, i2, i3};
          int ea = 0, eb = 1;
          i64 diameter2 = -1;
          for (int u = 0; u < 4; ++u)
            for (int v = u + 1; v < 4; ++v) {
              const i64 d2 = p3_norm2(p3_sub(cube[ids[v]], cube[ids[u]]));
              if (d2 > diameter2) { diameter2 = d2; ea = u; eb = v; }
            }
          int rest[2], nr = 0;
          for (int u = 0; u < 4; ++u)
            if (u != ea && u != eb) rest[nr++] = u;
          const P3& ta = cube[ids[ea]];
          const P3& tb = cube[ids[eb]];
          const P3& tx = cube[ids[rest[0]]];
          const P3& ty = cube[ids[rest[1]]];
          const Q4Form f = q4_form(ta, tb, tx, ty);
          if (f.det == 0 || !q4_center_strictly_inside(f, ta, tb, tx, ty)) continue;
          ++q4_supports;
          for (i64 zx = 0; zx <= 8; ++zx)
            for (i64 zy = 0; zy <= 8; ++zy)
              for (i64 zz = 0; zz <= 8; ++zz) {
                const P3 witness{zx, zy, zz};
                if (q4_power(f, witness) > 0) continue;
                ++q4_ball_points;
                if (!contains(EdgeEnvelope::kQ4Jung, ta, tb, witness)) ++q4_missed;
              }
        }
  expect(q4_supports > 0 && q4_ball_points > 0, "porte puissance q4 vacante");
  expect(q4_missed == 0, "enveloppe q4 perd un interieur ou une coquille");

  u64 lens_points = 0, lens_missed = 0;
  for (i64 x = 0; x <= 16; ++x)
    for (i64 y = 0; y <= 12; ++y)
      for (i64 z = 0; z <= 12; ++z) {
        const P3 p{x, y, z};
        if (p3_norm2(p3_sub(p, pqa)) > pq_d2 || p3_norm2(p3_sub(p, pqb)) > pq_d2) continue;
        ++lens_points;
        if (!contains(EdgeEnvelope::kQ3, pqa, pqb, p) || !contains(EdgeEnvelope::kQ4Jung, pqa, pqb, p))
          ++lens_missed;
      }
  expect(lens_points > 0 && lens_missed == 0, "lentille perdue par une enveloppe");

  RunOptions off;
  off.threads = threads;
  off.digest = true;
  off.diagnostic_raw_candidates_digest = true;
  if (route == "cover") off.pretest_query_min_points = std::numeric_limits<size_t>::max();
  if (route == "query") off.pretest_query_min_points = 0;
  std::vector<std::string> event_levels_off, event_levels_on;
  off.on_forest = [&](u64 K, const std::vector<ForestEvent>& events, const ForestResult& forest) {
    if (event_levels_off.size() <= K) event_levels_off.resize((size_t)K + 1);
    event_levels_off[(size_t)K] = digest_events_and_levels(K, events, forest);
  };
  RunOptions on = off;
  on.cover_envelope_filter = true;
  on.on_forest = [&](u64 K, const std::vector<ForestEvent>& events, const ForestResult& forest) {
    if (event_levels_on.size() <= K) event_levels_on.resize((size_t)K + 1);
    event_levels_on[(size_t)K] = digest_events_and_levels(K, events, forest);
  };
  const std::vector<InputPoint> input = make_family_input(family, n, 0, 3);
  // A un fil, le compactage stable doit aussi conserver l'ordre brut, avant
  // tout tri/RLE du pipeline.
  const CloudIndex ix = build_cloud_index(input);
  GenerateOptions goff;
  goff.threads = 1;
  goff.smax = std::min<u64>(off.smax, input.size());
  goff.pretest_query_min_points = off.pretest_query_min_points;
  GenerateOptions gon = goff;
  gon.cover_envelope_filter = true;
  std::vector<BallCandidate> raw_off, raw_on;
  GenerateStats raw_stats_off, raw_stats_on;
  generate_candidates(ix, goff, &raw_off, &raw_stats_off);
  generate_candidates(ix, gon, &raw_on, &raw_stats_on);
  const bool raw_order_same = same_candidates(raw_off, raw_on);
  const RunResult roff = run_pipeline(input, off);
  const RunResult ron = run_pipeline(input, on);

  const bool complete = roff.status == PipelineStatus::kCompleteRegular && ron.status == PipelineStatus::kCompleteRegular;
  const bool same = complete && roff.emitted == ron.emitted &&
                    roff.digest_raw_candidates == ron.digest_raw_candidates &&
                    roff.digest_balls == ron.digest_balls && roff.digest_all == ron.digest_all &&
                    roff.digest_forest == ron.digest_forest && event_levels_off == event_levels_on &&
                    same_cards(roff.cards, ron.cards);
  const u64 q3_before = sum2(ron.gen.edge_envelope_sites_before[1]);
  const u64 q3_after = sum2(ron.gen.edge_envelope_sites_after[1]);
  const u64 q4_before = sum2(ron.gen.edge_envelope_sites_before[2]);
  const u64 q4_after = sum2(ron.gen.edge_envelope_sites_after[2]);
  const u64 q3_anchors = sum2(ron.gen.edge_envelope_anchors[1]);
  const u64 q4_anchors = sum2(ron.gen.edge_envelope_anchors[2]);
  const bool counts_ok = q3_before >= q3_after && q4_before >= q4_after;
  bool route_ok = true;
  if (route == "cover")
    route_ok = ron.gen.edge_envelope_anchors[1][0] > 0 && ron.gen.edge_envelope_anchors[1][1] == 0 &&
               ron.gen.edge_envelope_anchors[2][0] > 0 && ron.gen.edge_envelope_anchors[2][1] == 0;
  if (route == "query")
    route_ok = ron.gen.edge_envelope_anchors[1][0] == 0 && ron.gen.edge_envelope_anchors[1][1] > 0 &&
               ron.gen.edge_envelope_anchors[2][0] == 0 && ron.gen.edge_envelope_anchors[2][1] > 0;
  const bool nonempty = min_removed == 0 ||
                        (q3_anchors > 0 && q4_anchors > 0 && q3_before - q3_after >= min_removed &&
                         q4_before - q4_after >= min_removed);
  std::printf("cover_envelope famille=%s n=%d route=%s statut=%d/%d brut=%s boules=%s foret=%s "
              "q3 ancres=%llu sites=%llu->%llu retires=%llu q4 ancres=%llu sites=%llu->%llu retires=%llu\n",
              cloud_family_name(family), n, route.c_str(), (int)roff.status, (int)ron.status,
              roff.digest_raw_candidates == ron.digest_raw_candidates ? "identique" : "DIFF",
              roff.digest_balls == ron.digest_balls ? "identiques" : "DIFF",
              roff.digest_all == ron.digest_all ? "identique" : "DIFF",
              (unsigned long long)q3_anchors, (unsigned long long)q3_before, (unsigned long long)q3_after,
              (unsigned long long)(q3_before - q3_after), (unsigned long long)q4_anchors,
              (unsigned long long)q4_before, (unsigned long long)q4_after,
              (unsigned long long)(q4_before - q4_after));

  const bool killed = boundary_bad != 0 || !raw_order_same || !same || !counts_ok || !route_ok;
  if (!inject.empty()) {
    if (killed) {
      std::fprintf(stderr, "MUTANT TUE : %s\n", inject.c_str());
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s survivant\n", inject.c_str());
    return 3;
  }
  if (killed) return 1;
  if (!nonempty) {
    std::fprintf(stderr, "PLANCHER : compaction q3/q4 vacante\n");
    return 3;
  }
  std::printf("cover_envelope_gate OK\n");
  return 0;
}
