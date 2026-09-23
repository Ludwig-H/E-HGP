// MorseHGP3D v9 — porte du filtre temoin q3/q4 par lots (S2, 23 septembre 2026).
//
// Juge : le chemin par lots (gen::run_wspd_q34_batched, filtre CPU de
// reference run_q34_filter_batch_cpu) et le chemin moteur
// (run_wspd_q34_parallel, cache de ligne) prennent les memes decisions ; ils
// doivent donc emettre le meme multiensemble de candidats et la chaine doit
// publier la meme tour FULL.
//   - generateur : multiensembles tries (arite, cle exacte, profondeur,
//     coquille) egaux, trois familles, K3/K5/K10, un et quatre fils ;
//   - chaine : condense FULL, catalogue et registre egaux ; sur le chemin par
//     lots, recherches de paire = paires developpees, cache nul, survivants =
//     coeurs construits ;
//   - levier GPU : sans GPU (stub ou pas d'appareil) refus explicite
//     chain_q34_gpu_unavailable, jamais un repli CPU silencieux ; avec un GPU,
//     meme condense ; q34_gpu_filter sans q34_batch_filter refuse ;
//   - mutants causaux tues : un filtre qui retire la voie q3 de chaque
//     survivant en tenant un registre coherent change les candidats ; un
//     filtre qui ment d'une unite sur ses rejets q3 est refuse par les
//     identites de masse du generateur.
//
//   mhgp9_chain_batch_filter_gate [--n=1500]
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

// arity, exact key, support IDs, depth, shell size
using Candidate = std::tuple<unsigned, std::array<mhgp9::gen::i128, 5>, std::array<std::size_t, 4>, std::size_t,
                             std::size_t>;

int fail(const std::string& cause) {
  std::printf("cause=%s\n", cause.c_str());
  return 1;
}

mhgp9::gen::WspdQ34Options q34_options() {
  // The chain's configuration (tower_chain.cpp), witness cache on for the engine path.
  mhgp9::gen::WspdQ34Options o;
  o.front_mode = mhgp9::gen::WspdFrontMode::MidpointSamples;
  o.requested_lane_mask = 6;
  o.q4_backend = mhgp9::gen::WspdQ4Backend::Local28;
  o.local = mhgp9::gen::Q4LocalOptions{};
  o.local.saturate_deep = true;
  o.local.retain_q3_fragments = true;
  o.witness_mode = mhgp9::gen::WspdQ34WitnessMode::RectanglePair;
  o.q3_census_mode = mhgp9::gen::WspdQ3CensusMode::GlobalBoxes;
  o.witness_bounds_mode = mhgp9::gen::Q34WitnessBoundsMode::Affine;
  o.q4_seed_cells = mhgp9::gen::Q4SeedCellOptions{mhgp9::gen::Q4SeedCellMode::LiveOnly, 64};
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
  mhgp9::gen::WspdQ34ParallelConsumer consumer() {
    return [this](std::size_t, const mhgp9::gen::Q34SeedCandidate& c) {
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
    std::fprintf(stderr, "usage: mhgp9_chain_batch_filter_gate [--n=1500]\n");
    return 2;
  }
  using namespace mhgp9;
  unsigned long long streams = 0, candidates = 0, chains = 0, survivors = 0, mutants = 0, gpu_refusals = 0,
                     gpu_runs = 0;
  for (const std::string_view family : {"uniform", "terrain", "clusters"}) {
    const auto fixture = gen::bench::make_front_fixture(n, family, 3);
    const auto cloud = gen::prepare_cloud(fixture.points);
    const auto index = gen::make_q2_cloud_index(cloud);
    for (const unsigned kmax : {3u, 5u, 10u}) {
      const std::string where = std::string(family) + "/K" + std::to_string(kmax);
      // ---- Generator streams, engine vs batch, W1 and W4.
      std::vector<Candidate> reference;
      for (const std::size_t workers : {std::size_t{1}, std::size_t{4}}) {
        Stream engine, batch;
        const auto o = q34_options();
        static_cast<void>(gen::run_wspd_q34_parallel(index, kmax, 8, o, workers, engine.consumer(), 16));
        gen::WspdQ34BatchTiming timing;
        const auto cpu = [workers](const gen::Q2CensusIndex& ix, unsigned k, std::span<const gen::WspdRectangle> r) {
          return gen::run_q34_filter_batch_cpu(ix, k, r, workers);
        };
        const auto rb = gen::run_wspd_q34_batched(index, kmax, 8, o, workers, batch.consumer(), 16, cpu, &timing);
        const auto e = engine.sorted(), b = batch.sorted();
        if (e != b) return fail("batch.stream " + where + " workers=" + std::to_string(workers));
        if (reference.empty()) reference = e;
        else if (reference != e) return fail("engine.workers " + where);
        if (timing.backend != "cpu" || timing.survivors != rb.pipeline.work.core_builds ||
            rb.pipeline.work.witness.pairs.queries != rb.pipeline.work.expanded_pairs ||
            rb.pipeline.work.witness.cache_rejected_pairs != 0)
          return fail("batch.ledger " + where);
        ++streams;
        candidates += e.size();
        survivors += timing.survivors;
      }
      // ---- Causal mutants of the batch filter.
      if (kmax >= 3) {
        const auto o = q34_options();
        Stream stripped;
        const auto strip_q3 = [](const gen::Q2CensusIndex& ix, unsigned k, std::span<const gen::WspdRectangle> r) {
          auto batch = gen::run_q34_filter_batch_cpu(ix, k, r, 2);
          std::vector<gen::Q34SurvivingEdge> kept;
          for (auto edge : batch.survivors) {
            if ((edge.mask & 2U) != 0) {
              edge.mask = static_cast<std::uint8_t>(edge.mask & ~2U);
              ++batch.pair_q3_rejected;  // a consistent lie: the ledger still balances
            }
            if (edge.mask != 0) kept.push_back(edge);
          }
          batch.survivors = kept;
          return batch;
        };
        bool refused = false;
        try {
          static_cast<void>(gen::run_wspd_q34_batched(index, kmax, 8, o, 2, stripped.consumer(), 16, strip_q3, nullptr));
        } catch (const std::logic_error&) {
          refused = true;
        }
        if (!refused && stripped.sorted() == reference) return fail("mutant.strip_q3_survived " + where);
        ++mutants;
        Stream lying;
        const auto lie = [](const gen::Q2CensusIndex& ix, unsigned k, std::span<const gen::WspdRectangle> r) {
          auto batch = gen::run_q34_filter_batch_cpu(ix, k, r, 2);
          ++batch.pair_q3_rejected;
          return batch;
        };
        refused = false;
        try {
          static_cast<void>(gen::run_wspd_q34_batched(index, kmax, 8, o, 2, lying.consumer(), 16, lie, nullptr));
        } catch (const std::logic_error&) {
          refused = true;
        }
        if (!refused) return fail("mutant.ledger_lie_survived " + where);
        ++mutants;
      }
      // ---- Whole chain: same FULL tower, same catalogue and ledger.
      for (const std::size_t workers : {std::size_t{1}, std::size_t{4}}) {
        ChainOptions base;
        base.kmax = kmax;
        base.separation_s = 8;
        base.workers = workers;
        auto batched = base;
        batched.q34_batch_filter = true;
        const auto a = run_tower_chain(fixture.points, base);
        const auto b = run_tower_chain(fixture.points, batched);
        if (a.status != ChainStatus::kComplete || b.status != ChainStatus::kComplete)
          return fail("chain.status " + where + " reason=" + a.reason + "|" + b.reason);
        const auto& la = a.ledger;
        const auto& lb = b.ledger;
        if (a.tower_digest != b.tower_digest || a.catalogue.unique_keys != b.catalogue.unique_keys ||
            a.catalogue.balls_by_shell != b.catalogue.balls_by_shell || a.catalogue.euler_by_k != b.catalogue.euler_by_k ||
            a.q3_emitted != b.q3_emitted || a.q4_emitted != b.q4_emitted)
          return fail("chain.object " + where);
        if (la.expanded_pairs != lb.expanded_pairs || la.witness_rejected_pairs != lb.witness_rejected_pairs ||
            la.witness_rejected_rectangles != lb.witness_rejected_rectangles || la.cover_builds != lb.cover_builds ||
            la.core_builds != lb.core_builds || la.core_closed_edges != lb.core_closed_edges ||
            la.q3_edges != lb.q3_edges || la.q4_edges != lb.q4_edges ||
            la.witness_input_pair_mass != lb.witness_input_pair_mass)
          return fail("chain.ledger " + where);
        if (!b.q34_batch.used || a.q34_batch.used || lb.witness_pair_queries != lb.expanded_pairs ||
            lb.witness_cache_rejected_pairs != 0 || b.q34_batch.survivors != lb.core_builds)
          return fail("chain.batch_fields " + where);
        ++chains;
        // The GPU lever: an explicit refusal without a device, the same tower with one.
        auto gpu = batched;
        gpu.q34_gpu_filter = true;
        const auto g = run_tower_chain(fixture.points, gpu);
        if (g.status == ChainStatus::kComplete) {
          if (g.tower_digest != a.tower_digest || g.q34_batch.backend == "cpu") return fail("chain.gpu_object " + where);
          ++gpu_runs;
        } else if (g.status == ChainStatus::kInvalidInput && g.reason.starts_with("chain_q34_gpu_unavailable")) {
          ++gpu_refusals;
        } else {
          return fail("chain.gpu_status " + where + " reason=" + g.reason);
        }
        auto lone = base;
        lone.q34_gpu_filter = true;
        const auto l = run_tower_chain(fixture.points, lone);
        if (l.status != ChainStatus::kInvalidInput || l.reason != "chain_q34_gpu_filter_requires_batch_filter")
          return fail("chain.gpu_without_batch " + where);
      }
    }
  }
  std::printf("chain_batch_filter_gate n=%zu streams=%llu candidates=%llu survivors=%llu chains=%llu mutants=%llu "
              "gpu_refusals=%llu gpu_runs=%llu\n", n, streams, candidates, survivors, chains, mutants, gpu_refusals,
              gpu_runs);
  if (streams < 18 || candidates < 10000 || survivors < 1000 || chains < 18 || mutants < 18 ||
      gpu_refusals + gpu_runs < 18)
  {
    std::printf("cause=floor.batch_filter\n");
    return 3;
  }
  return 0;
}
