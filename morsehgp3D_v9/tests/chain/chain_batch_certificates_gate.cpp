// MorseHGP3D v9 — porte des certificats de voie morte par lots (S3,
// 23 septembre 2026).
//
// Juge : le chemin par lots avec un appel de certificats (coeur puis cover de
// tous les survivants, reference CPU run_q34_certificate_batch_cpu) prend les
// memes decisions que le chemin moteur ; memes candidats, meme travail.
//   - generateur : multiensembles tries (arite, cle exacte, supports,
//     profondeur, coquille) egaux au moteur, trois familles, K2 (q3 seul),
//     K3, K5, K10, un et quatre fils ; travail des certificats, du cover, des
//     voies et des emissions egal au chemin par lots sans certificats ;
//   - mise en attente : un appel qui rend un survivant sur trois au CPU
//     (chemin moteur complet) donne les memes candidats et le meme travail ;
//   - mutants causaux tues : masque elargi, drapeau d'attente invalide,
//     compteur menteur, attente qui garde ses compteurs (refuses par les
//     identites du chemin par lots) ; voie q3 retiree de chaque survivant avec
//     un registre coherent (candidats differents) ;
//   - chaine : meme condense FULL, catalogue et registre que le moteur, et
//     meme condense canonique du catalogue complet (cle par cle) ; ce condense
//     egale celui du catalogue publie et change si une boule manque ou si un
//     identifiant de coquille change ;
//     levier GPU : refus explicite sans GPU, meme condense avec ; leviers
//     incoherents refuses avec leur raison.
//
//   mhgp9_chain_batch_certificates_gate [--n=1500]
//
// Code 0 conforme, 1 desaccord ou mutant survivant (`cause=`), 2 argument,
// 3 plancher.
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "../../src/chain/tower_chain.hpp"
#include "../../src/gen/pipeline/wspd_q34.hpp"
#include "../gen/front_fixtures.hpp"

namespace {

using namespace mhgp9;
using Candidate = std::tuple<unsigned, std::array<gen::i128, 5>, std::array<std::size_t, 4>, std::size_t,
                             std::size_t>;

int fail(const std::string& cause) {
  std::printf("cause=%s\n", cause.c_str());
  return 1;
}

gen::WspdQ34Options q34_options() {
  gen::WspdQ34Options o;
  o.front_mode = gen::WspdFrontMode::MidpointSamples;
  o.requested_lane_mask = 6;
  o.q4_backend = gen::WspdQ4Backend::Local28;
  o.local = gen::Q4LocalOptions{};
  o.local.saturate_deep = true;
  o.local.retain_q3_fragments = true;
  o.witness_mode = gen::WspdQ34WitnessMode::RectanglePair;
  o.q3_census_mode = gen::WspdQ3CensusMode::GlobalBoxes;
  o.witness_bounds_mode = gen::Q34WitnessBoundsMode::Affine;
  o.q4_seed_cells = gen::Q4SeedCellOptions{gen::Q4SeedCellMode::LiveOnly, 64};
  o.q3_atlas_consultation = true;
  o.q3_leaf_census = true;
  o.dead_lanes = true;
  o.pair_witness_cache = true;
  o.dead_core = true;
  o.jobs_by_mass = true;
  return o;
}

struct Stream {
  std::mutex mutex;
  std::vector<Candidate> candidates;
  gen::WspdQ34ParallelConsumer consumer() {
    return [this](std::size_t, const gen::Q34SeedCandidate& c) {
      const std::lock_guard<std::mutex> lock(mutex);
      candidates.emplace_back(c.arity, c.ball.coefficients(), c.support_ids, c.depth,
                              c.shell_first.size() + c.shell_second.size());
    };
  }
  std::vector<Candidate> sorted() {
    std::sort(candidates.begin(), candidates.end());
    return candidates;
  }
};

// Logical work that the certificate call must leave unchanged (capacity
// peaks excluded: they depend on which object held which buffer).
bool same_work(const gen::WspdQ34Work& a, const gen::WspdQ34Work& b) {
  return a.expanded_pairs == b.expanded_pairs && a.q3_edges == b.q3_edges && a.q4_edges == b.q4_edges &&
         a.both_edges == b.both_edges && a.cover_builds == b.cover_builds && a.cover_sites == b.cover_sites &&
         a.max_cover_sites == b.max_cover_sites && a.cover == b.cover && a.core_builds == b.core_builds &&
         a.core_sites == b.core_sites && a.core_closed_edges == b.core_closed_edges &&
         a.core_cover == b.core_cover && a.dead == b.dead && a.dead_core == b.dead_core &&
         a.q3_emitted == b.q3_emitted && a.q4_emitted == b.q4_emitted && a.witness == b.witness &&
         a.q3_atlas == b.q3_atlas && a.input_rectangles == b.input_rectangles;
}

// Every certificate, cover and lane counter of the chain ledger.
bool same_certificate_ledger(const GeneratorLedger& a, const GeneratorLedger& b) {
  return a.expanded_pairs == b.expanded_pairs && a.witness_rejected_pairs == b.witness_rejected_pairs &&
         a.cover_builds == b.cover_builds && a.cover_sites == b.cover_sites &&
         a.cover_node_visits == b.cover_node_visits && a.q3_edges == b.q3_edges && a.q4_edges == b.q4_edges &&
         a.both_edges == b.both_edges && a.dead_loads == b.dead_loads && a.dead_form_sites == b.dead_form_sites &&
         a.dead_cells == b.dead_cells && a.dead_outside_cells == b.dead_outside_cells &&
         a.dead_deep_cells == b.dead_deep_cells && a.dead_failed_cells == b.dead_failed_cells &&
         a.dead_uniform_tests == b.dead_uniform_tests && a.dead_point_tests == b.dead_point_tests &&
         a.dead_q3_proved == b.dead_q3_proved && a.dead_q3_open == b.dead_q3_open &&
         a.dead_q4_proved == b.dead_q4_proved && a.dead_q4_open == b.dead_q4_open &&
         a.core_builds == b.core_builds && a.core_sites == b.core_sites &&
         a.core_closed_edges == b.core_closed_edges && a.dead_core_loads == b.dead_core_loads &&
         a.dead_core_form_sites == b.dead_core_form_sites && a.dead_core_cells == b.dead_core_cells &&
         a.dead_core_uniform_tests == b.dead_core_uniform_tests && a.dead_core_point_tests == b.dead_core_point_tests &&
         a.dead_core_q3_proved == b.dead_core_q3_proved && a.dead_core_q3_open == b.dead_core_q3_open &&
         a.dead_core_q4_proved == b.dead_core_q4_proved && a.dead_core_q4_open == b.dead_core_q4_open &&
         a.core_cover_node_visits == b.core_cover_node_visits &&
         a.core_cover_bound_tests == b.core_cover_bound_tests &&
         a.core_cover_point_tests == b.core_cover_point_tests &&
         a.dead_core_outside_cells == b.dead_core_outside_cells && a.dead_core_deep_cells == b.dead_core_deep_cells &&
         a.dead_core_failed_cells == b.dead_core_failed_cells;
}

// The CPU reference on the survivors not deferred; every third survivor is
// handed back to the workers (whole engine edge).
gen::Q34CertificateBatch deferring(const gen::Q2CensusIndexPtr& index, unsigned kmax, bool dead_core,
                                   std::span<const gen::Q34SurvivingEdge> survivors) {
  std::vector<gen::Q34SurvivingEdge> kept;
  std::vector<std::size_t> where;
  for (std::size_t i = 0; i < survivors.size(); ++i)
    if (i % 3 != 1) {
      kept.push_back(survivors[i]);
      where.push_back(i);
    }
  auto part = gen::run_q34_certificate_batch_cpu(index, kmax, dead_core, kept, 3);
  auto out = part;
  out.masks.resize(survivors.size());
  out.deferred.assign(survivors.size(), 1);
  for (std::size_t i = 0; i < survivors.size(); ++i) out.masks[i] = survivors[i].mask;
  for (std::size_t j = 0; j < where.size(); ++j) {
    out.masks[where[j]] = part.masks[j];
    out.deferred[where[j]] = 0;
  }
  return out;
}

}  // namespace

int main(int argc, char** argv) {
  std::size_t n = 1500;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (!arg.starts_with("--n=")) return 2;
    const auto digits = arg.substr(4);
    const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), n);
    if (digits.empty() || error != std::errc{} || end != digits.data() + digits.size()) return 2;
  }
  if (n < 64 || n > 65536) {
    std::fprintf(stderr, "usage: mhgp9_chain_batch_certificates_gate [--n=1500]\n");
    return 2;
  }
  unsigned long long streams = 0, candidates = 0, deferred_runs = 0, chains = 0, mutants = 0, gpu_refusals = 0,
                     gpu_runs = 0, closed = 0;
  const auto cpu_filter = [](const gen::Q2CensusIndex& ix, unsigned k, std::span<const gen::WspdRectangle> r) {
    return gen::run_q34_filter_batch_cpu(ix, k, r, 3);
  };
  for (const std::string_view family : {"uniform", "terrain", "clusters"}) {
    const auto fixture = gen::bench::make_front_fixture(n, family, 3);
    const auto cloud = gen::prepare_cloud(fixture.points);
    const auto index = gen::make_q2_cloud_index(cloud);
    for (const unsigned kmax : {2u, 3u, 5u, 10u}) {
      const std::string where = std::string(family) + "/K" + std::to_string(kmax);
      const auto o = q34_options();
      // ---- Reference: engine stream, and the batch path without certificates.
      Stream engine, plain;
      static_cast<void>(gen::run_wspd_q34_parallel(index, kmax, 8, o, 4, engine.consumer(), 16));
      const auto reference = engine.sorted();
      const auto base = gen::run_wspd_q34_batched(index, kmax, 8, o, 3, plain.consumer(), 16, cpu_filter, nullptr);
      if (plain.sorted() != reference) return fail("batch.stream " + where);
      // ---- Certificates by batch (CPU reference), W1 and W4.
      for (const std::size_t workers : {std::size_t{1}, std::size_t{4}}) {
        Stream certified;
        const gen::Q34CertificateFilter cpu = [workers](const gen::Q2CensusIndexPtr& ix, unsigned k, bool core,
                                                        std::span<const gen::Q34SurvivingEdge> edges) {
          return gen::run_q34_certificate_batch_cpu(ix, k, core, edges, workers);
        };
        gen::WspdQ34BatchTiming timing;
        const auto r = gen::run_wspd_q34_batched(index, kmax, 8, o, workers, certified.consumer(), 16, cpu_filter,
                                                 &timing, &cpu);
        if (certified.sorted() != reference) return fail("certificates.stream " + where);
        if (!same_work(r.pipeline.work, base.pipeline.work)) return fail("certificates.work " + where);
        if (timing.certificate_backend != "cpu" || timing.deferred != 0) return fail("certificates.timing " + where);
        ++streams;
        candidates += reference.size();
        closed += r.pipeline.work.core_closed_edges;
      }
      // ---- Deferral: one survivor in three runs the whole engine edge.
      {
        Stream sink;
        const gen::Q34CertificateFilter filter = deferring;
        gen::WspdQ34BatchTiming timing;
        const auto r = gen::run_wspd_q34_batched(index, kmax, 8, o, 3, sink.consumer(), 16, cpu_filter, &timing,
                                                 &filter);
        if (sink.sorted() != reference) return fail("deferral.stream " + where);
        if (!same_work(r.pipeline.work, base.pipeline.work)) return fail("deferral.work " + where);
        if (timing.survivors >= 3 && timing.deferred == 0) return fail("deferral.count " + where);
        ++deferred_runs;
      }
      // ---- Mutants of the certificate call.
      const auto refused_by = [&](const gen::Q34CertificateFilter& filter, std::string_view reason) {
        Stream sink;
        try {
          static_cast<void>(gen::run_wspd_q34_batched(index, kmax, 8, o, 2, sink.consumer(), 16, cpu_filter,
                                                      nullptr, &filter));
        } catch (const std::logic_error& e) {
          return std::string(e.what()).find(reason) != std::string::npos;
        }
        return false;
      };
      const auto reference_call = [](const gen::Q2CensusIndexPtr& ix, unsigned k, bool core,
                                     std::span<const gen::Q34SurvivingEdge> edges) {
        return gen::run_q34_certificate_batch_cpu(ix, k, core, edges, 2);
      };
      if (base.pipeline.work.core_builds != 0) {
        const gen::Q34CertificateFilter widened = [&](const gen::Q2CensusIndexPtr& ix, unsigned k, bool core,
                                                      std::span<const gen::Q34SurvivingEdge> edges) {
          auto c = reference_call(ix, k, core, edges);
          for (std::size_t i = 0; i < edges.size(); ++i)
            if (edges[i].mask != 6) {
              c.masks[i] = 6;  // a lane its survivor did not carry
              break;
            }
          if (std::all_of(edges.begin(), edges.end(), [](const auto& e) { return e.mask == 6; })) c.masks[0] = 8;
          return c;
        };
        if (!refused_by(widened, "widened or malformed")) return fail("mutant.widened_survived " + where);
        const gen::Q34CertificateFilter flag = [&](const gen::Q2CensusIndexPtr& ix, unsigned k, bool core,
                                                   std::span<const gen::Q34SurvivingEdge> edges) {
          auto c = reference_call(ix, k, core, edges);
          c.deferred[0] = 2;
          return c;
        };
        if (!refused_by(flag, "widened or malformed")) return fail("mutant.flag_survived " + where);
        const gen::Q34CertificateFilter lie = [&](const gen::Q2CensusIndexPtr& ix, unsigned k, bool core,
                                                  std::span<const gen::Q34SurvivingEdge> edges) {
          auto c = reference_call(ix, k, core, edges);
          ++c.dead_core.uniform_tests;
          ++c.core_builds;
          return c;
        };
        if (!refused_by(lie, "lane or work identity")) return fail("mutant.counter_lie_survived " + where);
        const gen::Q34CertificateFilter kept = [&](const gen::Q2CensusIndexPtr& ix, unsigned k, bool core,
                                                   std::span<const gen::Q34SurvivingEdge> edges) {
          auto c = reference_call(ix, k, core, edges);
          c.deferred[0] = 1;  // handed back to the CPU, but its work still counted here
          c.masks[0] = edges[0].mask;
          return c;
        };
        if (!refused_by(kept, "lane or work identity")) return fail("mutant.deferred_work_survived " + where);
        mutants += 4;
        // Consistent lie: q3 removed from every edge left open, ledger balanced.
        const gen::Q34CertificateFilter strip = [&](const gen::Q2CensusIndexPtr& ix, unsigned k, bool core,
                                                    std::span<const gen::Q34SurvivingEdge> edges) {
          auto c = reference_call(ix, k, core, edges);
          for (auto& mask : c.masks)
            if ((mask & 2U) != 0) {
              mask = static_cast<std::uint8_t>(mask & ~2U);
              --c.dead.q3_open;
              ++c.dead.q3_proved;
            }
          return c;
        };
        Stream stripped;
        bool refused = false;
        try {
          static_cast<void>(gen::run_wspd_q34_batched(index, kmax, 8, o, 2, stripped.consumer(), 16, cpu_filter,
                                                      nullptr, &strip));
        } catch (const std::logic_error&) {
          refused = true;
        }
        if (!refused && stripped.sorted() == reference && base.pipeline.work.q3_emitted != 0)
          return fail("mutant.strip_q3_survived " + where);
        ++mutants;
      }
      // ---- Whole chain: same FULL tower, catalogue and ledger as the engine.
      for (const std::size_t workers : {std::size_t{1}, std::size_t{4}}) {
        ChainOptions engine_options;
        engine_options.kmax = kmax;
        engine_options.separation_s = 8;
        engine_options.workers = workers;
        engine_options.catalogue_digest = true;
        engine_options.keep_catalogue = workers == 1;
        auto batched = engine_options;
        batched.keep_catalogue = false;
        batched.q34_batch_filter = true;
        batched.q34_batch_certificates = true;
        const auto a = run_tower_chain(fixture.points, engine_options);
        const auto b = run_tower_chain(fixture.points, batched);
        if (a.status != ChainStatus::kComplete || b.status != ChainStatus::kComplete)
          return fail("chain.status " + where + " reason=" + a.reason + "|" + b.reason);
        const auto& la = a.ledger;
        const auto& lb = b.ledger;
        if (a.tower_digest != b.tower_digest || a.catalogue.unique_keys != b.catalogue.unique_keys ||
            a.catalogue.balls_by_shell != b.catalogue.balls_by_shell ||
            a.catalogue.euler_by_k != b.catalogue.euler_by_k || a.q3_emitted != b.q3_emitted ||
            a.q4_emitted != b.q4_emitted || a.catalogue_digest == 0 || a.catalogue_digest != b.catalogue_digest)
          return fail("chain.object " + where);
        if (workers == 1) {
          // The digest is that of the published catalogue and sees one change.
          auto balls = a.catalogue_balls;
          if (balls.size() < 2 || catalogue_digest(balls) != a.catalogue_digest) return fail("digest.published " + where);
          auto dropped = balls;
          dropped.erase(dropped.begin() + static_cast<std::ptrdiff_t>(dropped.size() / 2));
          auto moved = balls;
          for (auto& ball : moved)
            if (ball.n_shell != 0) {
              ++ball.shell_ids[0];
              break;
            }
          if (catalogue_digest(dropped) == a.catalogue_digest || catalogue_digest(moved) == a.catalogue_digest)
            return fail("mutant.catalogue_digest_blind " + where);
          mutants += 2;
        }
        if (!same_certificate_ledger(la, lb)) return fail("chain.ledger " + where);
        if (!b.q34_batch.used || b.q34_batch.certificate_backend != "cpu" || b.q34_batch.deferred != 0)
          return fail("chain.batch_fields " + where);
        ++chains;
        // The GPU levers: an explicit refusal without a device; with one, the
        // same tower, catalogue and certificate work, at the default slab and
        // at a 64-site slab that must defer edges to the engine path.
        for (const std::uint32_t capacity : {std::uint32_t{0}, std::uint32_t{64}}) {
          auto gpu = batched;
          gpu.q34_gpu_certificates = true;
          gpu.q34_certificate_capacity = capacity;
          const auto g = run_tower_chain(fixture.points, gpu);
          if (g.status == ChainStatus::kComplete) {
            if (g.tower_digest != a.tower_digest || g.catalogue_digest != a.catalogue_digest ||
                g.q34_batch.certificate_backend == "cpu" || !same_certificate_ledger(g.ledger, la) ||
                (capacity != 0 && (g.q34_batch.deferred == 0 || g.q34_batch.deferred >= g.q34_batch.survivors)) ||
                (capacity == 0 && g.q34_batch.deferred != 0))
              return fail("chain.gpu_object " + where + " capacity=" + std::to_string(capacity));
            ++gpu_runs;
          } else if (g.status == ChainStatus::kInvalidInput && g.reason.starts_with("chain_q34_gpu_unavailable")) {
            ++gpu_refusals;
          } else {
            return fail("chain.gpu_status " + where + " reason=" + g.reason);
          }
        }
        // Incoherent levers, refused with their reason before any work.
        auto lone = batched;
        lone.q34_batch_certificates = false;
        lone.q34_gpu_certificates = true;
        auto no_batch = engine_options;
        no_batch.q34_batch_certificates = true;
        auto no_dead = batched;
        no_dead.q34_dead_lanes = false;
        no_dead.q34_dead_core = false;
        auto cpu_capacity = batched;
        cpu_capacity.q34_certificate_capacity = 64;
        const auto l = run_tower_chain(fixture.points, lone);
        const auto m = run_tower_chain(fixture.points, no_batch);
        const auto d = run_tower_chain(fixture.points, no_dead);
        const auto c = run_tower_chain(fixture.points, cpu_capacity);
        if (l.status != ChainStatus::kInvalidInput || l.reason != "chain_q34_gpu_certificates_require_batch_certificates" ||
            m.status != ChainStatus::kInvalidInput ||
            m.reason != "chain_q34_batch_certificates_require_batch_filter_and_dead_lanes" ||
            d.status != ChainStatus::kInvalidInput ||
            d.reason != "chain_q34_batch_certificates_require_batch_filter_and_dead_lanes" ||
            c.status != ChainStatus::kInvalidInput ||
            c.reason != "chain_q34_certificate_capacity_requires_gpu_certificates_and_two_sites")
          return fail("chain.lever_refusals " + where);
      }
    }
  }
  std::printf("chain_batch_certificates_gate n=%zu streams=%llu candidates=%llu closed=%llu deferred_runs=%llu "
              "chains=%llu mutants=%llu gpu_refusals=%llu gpu_runs=%llu\n",
              n, streams, candidates, closed, deferred_runs, chains, mutants, gpu_refusals, gpu_runs);
  if (streams < 24 || candidates < 10000 || closed < 1000 || deferred_runs < 12 || chains < 24 || mutants < 70 ||
      gpu_refusals + gpu_runs < 48) {
    std::printf("cause=floor.batch_certificates\n");
    return 3;
  }
  return 0;
}
