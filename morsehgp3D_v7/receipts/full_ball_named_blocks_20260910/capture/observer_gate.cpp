// Validation executable ONLY. The 50k mode is explicit and never a benchmark.
#include "observer.hpp"

// Reuse the already independent bounded catalogue/oracle fixture helpers.
// The renamed main is not called by the 50k path; no subset oracle sees 50k.
#define main mhgp7_private_original_bounded_gate_main
#include "morsehgp3D_v7/tests/full_ball_tower_gate.cpp"
#undef main

#include "morsehgp3D_v7/src/cloud/families.hpp"
#include "morsehgp3D_v7/src/core/parse.hpp"
#include "morsehgp3D_v7/src/pipeline/generate.hpp"
#include "morsehgp3D_v7/bench/full_gabriel_semantic_digest.hpp"
#ifdef MHGP7_NAMED_BLOCK_CUDA
#include "morsehgp3D_v7/src/gpu/census_route.cuh"
#endif

namespace ng = named_block_gate;
namespace {

std::string physical(const mhgp7::FullBallTowerResult& tower) {
  using namespace mhgp7;
  Sha256 h; h.tag("private-named-observer-physical-v1"); h.u64le(tower.orders.size());
  for (const auto& order : tower.orders) {
    const auto& f = order.forest; h.u64le(f.order()); h.u64le(f.nodes().size());
    for (const auto& n : f.nodes()) { full_probe_digest::level_wire(h,n.level); h.u64le(n.first); h.u64le(n.parent_count); }
    h.u64le(f.parents().size()); for (u64 p : f.parents()) h.u64le(p);
    for (u64 p : f.successors()) h.u64le(p);
    h.u64le(f.contributions().size());
    for (const auto& c : f.contributions()) {
      full_probe_digest::level_wire(h,c.level); h.u64le(c.segment); h.u64le(c.ref.population);
      h.u64le(c.ref.shell_mask); h.u64le(c.ref.include_interior);
    }
    for (u64 p : order.lower_nodes) h.u64le(p);
  }
  const auto& bank = *tower.orders.front().forest.populations();
  h.u64le(bank.domain().size()); for (auto p : bank.domain()) h.u64le(p);
  h.u64le(bank.rows().size());
  for (const auto& r : bank.rows()) {
    h.u64le(r.interior.size()); for (auto p : r.interior) h.u64le(p);
    h.u64le(r.shell.size()); for (auto p : r.shell) h.u64le(p);
  }
  return h.hex();
}

template<class Function>
void refused(const char* why, Function&& fn) {
  try { fn(); }
  catch (const std::runtime_error& error) { ng::need(std::string_view(error.what()) == why, "selftest.wrong_rejection"); return; }
  throw std::runtime_error("selftest.mutant_survived");
}

std::string stale_mutant(const mhgp7::CloudIndex& ix, const std::vector<mhgp7::BallData>& balls, unsigned kmax) {
  ng::Observer observer(ix, {}, true);
  ng::Scope scope(observer); ng::raw_anchor_mutant = true;
  try {
    const auto tower = mhgp7::build_full_ball_tower(ix, balls, kmax);
    if (tower.status != mhgp7::FullBallStatus::kCompleteRelative)
      throw std::runtime_error("named.raw_anchor_mutant_inconclusive");
  } catch (const std::runtime_error& error) {
    if (std::string_view(error.what()) == "named.pre_root_not_live") return error.what();
    throw;
  }
  throw std::runtime_error("named.raw_anchor_mutant_survived");
}

void small_selftest() {
  using namespace mhgp7;
  unsigned cases = 0, rejected = 0, matches = 0;
  u64 roots_checked = 0, grouped = 0;
  for (const auto& fixture : fixtures()) {
    if (std::string_view(fixture.name) != "square" &&
        std::string_view(fixture.name) != "growth_ABCZ_doubled_lot") continue;
    const auto in = input(fixture, 0); const auto ix = build_cloud_index(in);
    const oracle::Model model(fixture.points);
    const auto balls = catalogue(fixture.points, ix, model, fixture.kmax);
    std::vector<ng::Expectation> targets;
    if (std::string_view(fixture.name) == "square") {
      targets = {{1,2,{1,{-2,-2,0},0},{{2,0,0},1},4,0,false,{}, {0,1,2,3}},
                 {2,3,{1,{-2,-2,0},0},{{2,0,0},1},0,15,false,{}, {0,1,2,3}}};
    } else {
      targets = {{3,3,{1,{-10,-10,0},25},{{25,0,0},1},1,8,false,{}, {0,1,2,3}},
                 {4,3,{1,{-410,-10,0},42025},{{25,0,0},1},1,8,false,{}, {4,5,6,7}}};
    }
    const auto plain = build_full_ball_tower(ix, balls, fixture.kmax);
    ng::need(plain.status == FullBallStatus::kCompleteRelative, "selftest.plain_refusal");
    ng::Observer observer(ix, targets, true);
    FullBallTowerResult observed;
    { ng::Scope scope(observer); observed = build_full_ball_tower(ix, balls, fixture.kmax); }
    ng::need(observed.status == FullBallStatus::kCompleteRelative, "selftest.observed_refusal");
    observer.finish(observed);
    ng::need(physical(plain) == physical(observed), "selftest.observer_changed_physical_output");
    ng::need(plain.stats.anchor_hits == observed.stats.anchor_hits &&
        plain.stats.intruder_queries == observed.stats.intruder_queries, "selftest.observer_changed_resolver_work");
    auto missing = observer; missing.observed[0].before = 0;
    refused("named.block_not_observed_exactly_once", [&] { missing.finish(observed); }); ++rejected;
    auto wrong_count = observer; wrong_count.observed[0].roots.pop_back();
    refused("named.global_parent_cardinality", [&] { wrong_count.finish(observed); }); ++rejected;
    auto wrong_mask = observer; wrong_mask.observed[0].mask ^= 1;
    refused("named.contribution", [&] { wrong_mask.finish(observed); }); ++rejected;
    auto no_anchor = observer; no_anchor.observed[0].anchor = ng::absent;
    refused("named.anchor_not_live_at_closed_cut", [&] { no_anchor.finish(observed); }); ++rejected;
    roots_checked += observer.root_checks; grouped += observed.stats.grouped_lots;
    matches += static_cast<unsigned>(targets.size()); ++cases;
  }
  std::string killed;
  refused("named.raw_anchor_mutant_inconclusive", [&] { (void)stale_mutant(CloudIndex{}, {}, 2); });
  for (const auto& fixture : fixtures()) if (std::string_view(fixture.name) == "E5") {
    const auto in = input(fixture,0); const auto ix = build_cloud_index(in);
    const oracle::Model model(fixture.points); const auto balls = catalogue(fixture.points,ix,model,fixture.kmax);
    killed = stale_mutant(ix,balls,fixture.kmax);
  }
  ng::need(cases == 2 && matches == 4 && rejected == 8 && grouped > 0 && roots_checked > 8 && !killed.empty(),
      "selftest.nonvacuity");
  std::printf("{\"schema\":\"mhgp7-named-block-observer-selftest-v1\",\"status\":\"passed\","
      "\"cases\":%u,\"matched_blocks\":%u,\"observer_rejections\":%u,\"strict_root_checks\":%llu,"
      "\"grouped_lots\":%llu,\"physical_outputs_equal\":true,\"mutant_inconclusive_rejections\":1,\"raw_anchor_mutant_killed\":true,"
      "\"raw_anchor_mutant_reason\":\"%s\",\"real_50000_executed\":false,\"benchmark\":false}\n",
      cases,matches,rejected,static_cast<unsigned long long>(roots_checked),
      static_cast<unsigned long long>(grouped),killed.c_str());
}

void pinned_50000(int threads) {
  using namespace mhgp7;
  std::fprintf(stderr,"validation_stage=input\n");
  const auto in = make_family_input(CloudFamily::kUniform,50000,65536,3);
  ng::need(full_probe_digest::input(in) == ng::input_sha,"named.pinned_input_digest");
  const auto ix = build_cloud_index(in);
  ng::need(ix.valid && !ix.has_duplicate_positions(),"named.input_index");
  std::vector<BallCandidate> candidates;
  GenerateOptions options; options.s = 8; options.smax = 11; options.threads = threads;
  GenerateStats gs;
  std::fprintf(stderr,"validation_stage=generate_s8\n");
  generate_candidates(ix,options,&candidates,&gs);
  ng::need(gs.cap_refus == kCapRefusNone && !gs.invariant_jneg,"named.generation_refused");
  const auto mass = expected_pair_mass(ix);
  for (size_t q = 0; q < 3; ++q)
    ng::need(gs.ledger_emitted_mass[q]+gs.ledger_killed_mass[q] == mass,"named.pair_mass_ledger");
  rle_candidates(&candidates,threads);
  std::vector<Survivor> survivors; std::vector<BallData> balls; ExpandStats es;
  std::fprintf(stderr,"validation_stage=census\n");
#ifdef MHGP7_NAMED_BLOCK_CUDA
  constexpr const char* backend = "cuda_census_cpu_full_validation_only";
  gpu::CensusRouteCosts costs;
  const auto error = gpu::device_prefilter_census_route(ix,candidates,11,kBallShellMax,&survivors,&balls,&es,262144,&costs);
  ng::need(error.empty(),"named.cuda_census_refused");
#else
  constexpr const char* backend = "cpu_reference_validation_only";
  prefilter_balls(ix,candidates,11,threads,&survivors,&es);
  ng::need(census_balls(ix,candidates,survivors,11,kBallShellMax,threads,&balls,&es) == PipelineStatus::kCompleteRegular,
      "named.census_refused");
#endif
  std::vector<BallCandidate>().swap(candidates); std::vector<Survivor>().swap(survivors);
  std::fprintf(stderr,"validation_stage=raw_anchor_mutant_same_census\n");
  const auto killed = stale_mutant(ix,balls,10);
  std::fprintf(stderr,"validation_stage=nominal_named_blocks\n");
  ng::Observer observer(ix,ng::real_expectations());
  FullBallTowerResult tower;
  { ng::Scope scope(observer); tower = build_full_ball_tower(ix,balls,10); }
  ng::need(tower.status == FullBallStatus::kCompleteRelative,tower.reason);
  observer.finish(tower);
  std::printf("{\"schema\":\"mhgp7-full-named-blocks-gate-v1\",\"status\":\"passed\","
      "\"backend\":\"%s\",\"input_digest\":\"%s\",\"n\":50000,\"s\":8,\"kmax\":10,\"threads\":%d,"
      "\"benchmark\":false,\"contract_qualified\":false,\"public_status\":\"not_claimed\","
      "\"global_catalogue_completeness_verified\":false,\"final_node_arities_verified\":false,"
      "\"raw_anchor_mutant_killed\":true,\"raw_anchor_mutant_reason\":\"%s\",\"blocks\":[",
      backend,ng::input_sha,threads,killed.c_str());
  for (size_t j = 0; j < observer.expected.size(); ++j) {
    if (j) std::printf(",");
    const auto& want = observer.expected[j]; const auto& got = observer.observed[j];
    std::printf("{\"expected\":"); ng::print_definition(want);
    std::printf(",\"pre_lot_roots\":"); ng::print_ids(got.roots);
    std::printf(",\"observed_catalogue_index\":%llu,\"prior_node_count\":%llu,\"anchor_after_lot\":%llu,"
        "\"contribution_mask\":%u,\"contribution_interior\":%s,\"before_observations\":%zu,\"after_observations\":%zu}",
        static_cast<unsigned long long>(got.catalogue_index),static_cast<unsigned long long>(got.prior_nodes),
        static_cast<unsigned long long>(got.anchor),got.mask,got.interior ? "true" : "false",got.before,got.after);
  }
  std::printf("]}\n");
}
}

int main(int argc,char** argv) {
  try {
    if (argc == 2 && std::string_view(argv[1]) == "--selftest") small_selftest();
    else if (argc == 2 && std::string_view(argv[1]) == "--describe") ng::print_definitions();
    else if (argc == 3 && std::string_view(argv[1]) == "--pinned-50000" &&
        std::string_view(argv[2]).starts_with("--threads=")) {
      mhgp7::i64 threads = 0;
      if (!mhgp7::parse_i64_exact(argv[2]+10,&threads) || threads < 1 || threads > std::numeric_limits<int>::max()) return 2;
      pinned_50000(static_cast<int>(threads));
    } else return 2;
    return std::fflush(stdout) == 0 ? 0 : 2;
  } catch (const Failure& error) {
    std::fprintf(stderr,"FAIL oracle_helper %s\n",error.why); return 1;
  } catch (const std::exception& error) {
    std::fprintf(stderr,"FAIL %s\n",error.what()); return 1;
  }
}
