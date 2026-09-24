// MorseHGP3D v9 — porte de la voie q3 par lots dans la chaine (S4a,
// 23 septembre 2026).
//
// Juge : la chaine avec le levier q34_batch_q3 (voie q3 des survivants
// certifies en un appel, sans atlas, emulation hote de gpu/lanes.hpp ; les
// enregistrements deviennent directement des presentations) publie la meme
// tour et le meme catalogue que la chaine S3 sans le levier.
//   - trois familles et une fixture cospherique dans le domaine de la tour
//     (coquilles de 8 a 10 sites), K2 (q3 seul), K3, K5, K10, un et quatre fils : meme condense FULL, meme
//     condense canonique du catalogue, memes emissions q3/q4, toutes les
//     voies demandees decidees, juge de l'appel (juge_lanes_filter) sur toutes ;
//   - ardoise reduite (q34_lanes_capacity) : des aretes sont rendues au CPU
//     (traine) et les condenses ne changent pas ;
//   - S4b (q34_batch_q4) : la voie q4 dans le meme appel, jugee (voie q4 du
//     moteur), memes condenses et meme registre des voies ; ardoise et tampon
//     d'evenements reduits : traine, memes condenses ;
//   - registre semantique des voies (auditeur A) : q3_edges, q4_edges,
//     both_edges, covers et voies ouvertes egaux sans le levier, avec lui et
//     avec l'ardoise reduite (une arete dont q3 part en traine et dont q4 est
//     ouverte compte une fois dans both_edges) ;
//   - leviers incoherents refuses avec leur raison (dont une ardoise au-dela
//     de 2^20 sites) ; le levier GPU sans GPU
//     est un refus explicite (jamais un repli silencieux).
//
//   mhgp9_chain_batch_q3_gate [--n=1000]
//
// Code 0 conforme, 1 desaccord (`cause=`), 2 argument, 3 plancher.
#include <array>
#include <charconv>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "../../src/chain/tower_chain.hpp"
#include "../gen/front_fixtures.hpp"

namespace {

using namespace mhgp9;

int fail(const std::string& cause) {
  std::printf("cause=%s\n", cause.c_str());
  return 1;
}

// Cospherical fixture within the tower's domain (shells <= 12): on each of
// two spheres of radius 35, the six axis points and four more points of the
// great circle z = 0; the q3 balls of its triangles have shells of 8 to 10.
std::vector<gen::Point3> sphere_fixture() {
  std::vector<gen::Point3> spheres;
  for (const std::int32_t cx : {1000, 1100}) {
    for (const auto& d : {std::array<int, 3>{35, 0, 0}, {-35, 0, 0}, {0, 35, 0}, {0, -35, 0}, {0, 0, 35},
                          {0, 0, -35}, {21, 28, 0}, {-21, 28, 0}, {21, -28, 0}, {-21, -28, 0}})
      spheres.push_back({cx + d[0], 1000 + d[1], 1000 + d[2]});
  }
  return spheres;
}

}  // namespace

int main(int argc, char** argv) {
  std::size_t n = 1000;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (arg.starts_with("--n=")) {
      const auto digits = arg.substr(4);
      const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), n);
      if (digits.empty() || error != std::errc{} || end != digits.data() + digits.size()) return 2;
    } else {
      return 2;
    }
  }
  if (n < 64 || n > 65536) {
    std::fprintf(stderr, "usage: mhgp9_chain_batch_q3_gate [--n=1000]\n");
    return 2;
  }
  unsigned long long cases = 0, asked = 0, judged = 0, records = 0, tails = 0, refusals = 0, both = 0,
                     q4_emitted = 0, q4_tails = 0, fused_seeds = 0;
  for (const std::string_view family : {"uniform", "terrain", "clusters", "spheres"}) {
    const auto points = family == "spheres" ? sphere_fixture() : gen::bench::make_front_fixture(n, family, 3).points;
    for (const unsigned kmax : {2U, 3U, 5U, 10U}) {
      for (const std::size_t workers : {std::size_t{1}, std::size_t{4}}) {
        const std::string where = std::string(family) + "/K" + std::to_string(kmax) + "/W" + std::to_string(workers);
        ChainOptions base;
        base.kmax = kmax;
        base.workers = workers;
        base.catalogue_digest = true;
        base.q34_batch_filter = true;
        base.q34_batch_certificates = true;
        auto lanes = base;
        lanes.q34_batch_q3 = true;
        lanes.q34_lanes_judge = true;
        auto small = lanes;
        small.q34_lanes_judge = false;
        small.q34_lanes_capacity = 24;
        auto with_q4 = lanes;
        with_q4.q34_batch_q4 = true;
        auto with_q4_small = with_q4;
        with_q4_small.q34_lanes_judge = false;
        with_q4_small.q34_lanes_capacity = 24;
        with_q4_small.q34_lanes_events = 2;
        // v24: q2 on its own thread during the batch calls (host path here).
        auto with_q4_q2 = with_q4;
        with_q4_q2.q2_during_device = true;
        // v25 (L15): the fused q3 + q4 pass in the lanes tasks.
        auto with_q4_fused = with_q4;
        with_q4_fused.q34_lanes_fused = true;
        with_q4_q2.q34_lanes_fused = true;
        const auto a = run_tower_chain(points, base);
        const auto b = run_tower_chain(points, lanes);
        const auto c = run_tower_chain(points, small);
        const auto d = run_tower_chain(points, with_q4);
        const auto e = run_tower_chain(points, with_q4_small);
        const auto f = run_tower_chain(points, with_q4_q2);
        const auto g = run_tower_chain(points, with_q4_fused);
        if (a.status != ChainStatus::kComplete || b.status != ChainStatus::kComplete ||
            c.status != ChainStatus::kComplete || d.status != ChainStatus::kComplete ||
            e.status != ChainStatus::kComplete || f.status != ChainStatus::kComplete ||
            g.status != ChainStatus::kComplete)
          return fail("status " + where + " " + b.reason + " " + c.reason + " " + d.reason + " " + e.reason + " " +
                      f.reason + " " + g.reason);
        // v25: the fused pass declares the separate phases' ledger and
        // records; its counters exist under the lever only.
        const auto& lg = g.q34_batch;
        if (lg.lanes_records != d.q34_batch.lanes_records || g.ledger.lanes4_seeds != d.ledger.lanes4_seeds ||
            g.ledger.lanes4_pass_chunks != d.ledger.lanes4_pass_chunks ||
            g.ledger.lanes_census_point_tests != d.ledger.lanes_census_point_tests ||
            d.q34_batch.lanes_fused_seeds != 0 || d.q34_batch.lanes_fused_chunks != 0 ||
            (kmax < 3 && lg.lanes_fused_seeds != 0) || lg.lanes_fused_seeds > g.ledger.lanes4_seeds)
          return fail("fused " + where);
        fused_seeds += lg.lanes_fused_seeds;
        if (f.q2_accepted_pairs != a.q2_accepted_pairs || (kmax >= 2 && f.times.q2_wait_ms > f.times.q2_ms + 0.05) ||
            (kmax < 2 && f.times.q2_wait_ms != 0))
          return fail("q2_overlap " + where);
        for (const auto* r : {&b, &c, &d, &e, &f, &g})
          if (a.tower_digest != r->tower_digest || a.catalogue_digest != r->catalogue_digest ||
              a.q3_emitted != r->q3_emitted || a.q4_emitted != r->q4_emitted)
            return fail("digest " + where);
        if (kmax >= 3) {
          const auto& ld = d.q34_batch;
          if (ld.lanes_decided != ld.lanes_asked || ld.lanes_judged != ld.lanes_decided ||
              d.ledger.lanes4_emitted + 0 != d.q4_emitted || d.ledger.lanes4_edges == 0)
            return fail("q4_lanes " + where);
          q4_emitted += d.ledger.lanes4_emitted;
          q4_tails += e.q34_batch.lanes_deferred;
        }
        if (a.q3_emitted != b.q3_emitted || a.q4_emitted != b.q4_emitted || a.q3_emitted != c.q3_emitted ||
            a.q4_emitted != c.q4_emitted)
          return fail("emitted " + where);
        const auto lanes_of = [](const ChainResult& r) {
          const auto& l = r.ledger;
          return std::array<std::uint64_t, 7>{l.q3_edges, l.q4_edges, l.both_edges, l.cover_builds, l.cover_sites,
                                              l.dead_q3_open, l.dead_q4_open};
        };
        if (lanes_of(a) != lanes_of(b) || lanes_of(a) != lanes_of(c) || lanes_of(a) != lanes_of(d) ||
            lanes_of(a) != lanes_of(e))
          return fail("ledger " + where);
        both += a.ledger.both_edges;
        const auto& lb = b.q34_batch;
        const auto& lc = c.q34_batch;
        if (lb.lanes_backend != "cpu" || lb.lanes_decided != lb.lanes_asked || lb.lanes_deferred != 0 ||
            lb.lanes_judged != lb.lanes_decided || lb.lanes_records != b.ledger.lanes_emitted ||
            b.ledger.lanes_edges != lb.lanes_decided)
          return fail("lanes " + where);
        if (lc.lanes_asked != lb.lanes_asked || lc.lanes_decided + lc.lanes_deferred != lc.lanes_asked)
          return fail("small " + where);
        if (kmax >= 2 && lb.lanes_asked == 0 && family != "spheres") return fail("no_asked " + where);
        asked += lb.lanes_asked;
        judged += lb.lanes_judged;
        records += lb.lanes_records;
        tails += lc.lanes_deferred;
        ++cases;
      }
    }
  }
  // Incoherent levers, and the GPU lever without a device: explicit refusals.
  const auto fixture = gen::bench::make_front_fixture(n, "uniform", 3);
  const auto refused = [&](const ChainOptions& o, const std::string& reason) {
    const auto r = run_tower_chain(fixture.points, o);
    return r.status == ChainStatus::kInvalidInput && r.reason.starts_with(reason);
  };
  ChainOptions ok;
  ok.kmax = 5;
  ok.workers = 2;
  ok.q34_batch_filter = true;
  ok.q34_batch_certificates = true;
  {
    auto o = ok;
    o.q34_batch_certificates = false;
    o.q34_batch_q3 = true;
    if (!refused(o, "chain_q34_batch_q3_requires_batch_certificates")) return fail("refusal.batch_q3");
    ++refusals;
  }
  {
    auto o = ok;
    o.q34_gpu_q3 = true;
    if (!refused(o, "chain_q34_gpu_q3_requires_batch_q3")) return fail("refusal.gpu_q3");
    ++refusals;
  }
  {
    auto o = ok;
    o.q34_lanes_judge = true;
    if (!refused(o, "chain_q34_lanes_judge_requires_batch_q3")) return fail("refusal.judge");
    ++refusals;
  }
  {
    auto o = ok;
    o.q34_batch_q3 = true;
    o.q34_lanes_capacity = 1;
    if (!refused(o, "chain_q34_lanes_capacity_requires_batch_q3_and_two_sites")) return fail("refusal.capacity");
    ++refusals;
  }
  {
    auto o = ok;
    o.q34_batch_q3 = true;
    o.q34_lanes_capacity = (1U << 20) + 1;
    if (!refused(o, "chain_q34_lanes_capacity_requires_batch_q3_and_two_sites")) return fail("refusal.capacity_high");
    ++refusals;
  }
  {
    auto o = ok;
    o.q34_lanes_capacity = 64;
    if (!refused(o, "chain_q34_lanes_capacity_requires_batch_q3_and_two_sites")) return fail("refusal.capacity_off");
    ++refusals;
  }
  {
    auto o = ok;
    o.q34_batch_q4 = true;
    if (!refused(o, "chain_q34_batch_q4_requires_batch_q3")) return fail("refusal.batch_q4");
    ++refusals;
  }
  {
    auto o = ok;
    o.q34_batch_q3 = true;
    o.q34_lanes_events = 8;
    if (!refused(o, "chain_q34_lanes_events_requires_batch_q4")) return fail("refusal.events");
    ++refusals;
  }
  {
    auto o = ok;
    o.q34_batch_filter = false;
    o.q34_batch_certificates = false;
    o.q2_during_device = true;  // v24: requires the batch path
    if (!refused(o, "chain_q2_during_device_requires_batch_filter")) return fail("refusal.q2_during_device");
    ++refusals;
  }
  {
    auto o = ok;
    o.q34_batch_q3 = true;
    o.q34_lanes_fused = true;  // v25: requires the q4 lanes
    if (!refused(o, "chain_q34_lanes_fused_requires_batch_q4")) return fail("refusal.lanes_fused");
    ++refusals;
  }
  {
    // Review before R20: a refusal of the generator after the device
    // preparation started and took the index (dead-lane core without its
    // certificate, found by the batch path's validation right after the
    // front) unwinds the chain; the preparation owns its index, the refusal
    // stays explicit.
    auto o = ok;
    o.q34_batch_certificates = false;
    o.q34_dead_lanes = false;
    o.q34_dead_core = true;
    o.q34_gpu_filter = true;
    o.q2_during_device = true;
    if (!refused(o, "chain_invalid_argument: mhgp9 gen dead-lane core")) return fail("refusal.prepared_unwind");
    ++refusals;
  }
  bool device = false;
  {
    auto o = ok;
    o.q34_batch_q3 = true;
    o.q34_gpu_q3 = true;
    const auto r = run_tower_chain(fixture.points, o);
    if (r.status == ChainStatus::kComplete) {
      device = true;  // a CUDA build on a device: the same tower as the CPU lever
      auto cpu = ok;
      cpu.q34_batch_q3 = true;
      if (run_tower_chain(fixture.points, cpu).tower_digest != r.tower_digest) return fail("gpu.digest");
    } else if (r.status != ChainStatus::kInvalidInput || !r.reason.starts_with("chain_q34_gpu_unavailable")) {
      return fail("gpu.refusal " + r.reason);
    }
    ++refusals;
  }
  std::printf("chain_batch_q3_gate n=%zu cases=%llu asked=%llu judged=%llu records=%llu tails=%llu both=%llu "
              "q4_emitted=%llu q4_tails=%llu fused_seeds=%llu refusals=%llu device=%s\n",
              n, cases, asked, judged, records, tails, both, q4_emitted, q4_tails, fused_seeds, refusals,
              device ? "yes" : "no");
  // tails + both > asked (auditor A): some edge has its q3 lane in the tail
  // AND its q4 lane open, the case the both_edges comparison guards.
  if (cases == 0 || asked == 0 || judged != asked || records == 0 || tails == 0 || both == 0 || refusals != 12 ||
      tails + both <= asked || q4_emitted == 0 || q4_tails == 0 || fused_seeds == 0)
    return 3;
  return 0;
}
