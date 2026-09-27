#include "../b_full_a_manifest_20260927/native_a.hpp"
#include "../b_full_a_manifest_20260927/fixtures.hpp"
#include "events.hpp"
#include <iostream>
#include <string_view>

namespace am = mhgp9::audit::a_manifest;
namespace ae = mhgp9::audit::a_events;
using namespace mhgp9::tower;

namespace {
std::array<std::string, 3> mutation_reasons;
void mutations(const am::Manifest& input, const am::Output& correct) {
  constexpr std::array mutations{ae::Mutant::ClosedParents, ae::Mutant::OmitContributionFreeHistory, ae::Mutant::ReversePlateau};
  for (size_t i = 0; i < mutations.size(); ++i) {
    if (!mutation_reasons[i].empty()) continue;
    try { am::compare_output(ae::build(input, true, mutations[i]).output, correct); }
    catch (const std::runtime_error& e) {
      const std::string reason = e.what();
      am::need(reason == "event.predecessor_missing" || reason == "event.parent_identity_collision" ||
               reason == "replay.occurrence_roots" || reason == "replay.anchors" || reason == "replay.actions_parents" ||
               reason == "replay.contribution", "event.unexpected_mutation_refusal");
      mutation_reasons[i] = reason;
    }
  }
}
am::Manifest abstract_program(u64 count, u64 seed) {
  am::Manifest m; m.k = 2; m.ball_count = count; m.domain = {19, 121};
  const auto random = [&]() { seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17; return seed; };
  u32 run = 2, first = 0;
  for (u64 b = 0; b < count; ++b) {
    if (b && (b + 1 == count || random() % 3 == 0)) run += 2;
    const u32 ball = static_cast<u32>(count - 1 - b);
    if (!b || m.blocks.back().run != run) first = ball;
    std::vector<u64> earlier;
    for (u64 a = 0; a < b; ++a) if (m.blocks[a].run < run) earlier.push_back(a);
    u64 queries = earlier.empty() ? 0 : random() % 5;
    if (b + 1 == count) queries = earlier.size();
    for (u64 j = 0; j < queries; ++j) {
      const u64 target = b + 1 == count ? earlier[j] : earlier[random() % earlier.size()]; m.target.push_back(target);
      if (random() % 7 == 0) m.target.push_back(target);
    }
    const u16 contribution = queries == 0 || random() % 3 == 0 ? u16{1} : u16{0};
    const u64 scale = b + 2;
    m.blocks.push_back({ball, run, first, b, {{u64(run) * scale, 0, 0}, static_cast<i128>(scale)}, contribution, 2, false, true});
    m.representative_begin.push_back(m.target.size());
  }
  am::validate_manifest(m); return m;
}
}

int main(int argc, char** argv) {
  try {
    am::need(argc == 2 && (std::string_view(argv[1]) == "--gate" || std::string_view(argv[1]) == "--preflight"), "event.usage");
    const bool full = std::string_view(argv[1]) == "--gate";
    u64 fixtures = 0, captures = 0, replays = 0, vertices = 0, edges = 0, groups = 0, parents = 0, draft_parents = 0, forest_parents = 0;
    u64 silent = 0, continuations = 0, max_parents = 0, capacity = 0, queries = 0, query_steps = 0;
    for (const auto& fixture : am::fixtures::all()) {
      if (!full && std::string_view(fixture.name) == "ico12") continue;
      for (bool variant : {false, true}) {
        const auto input = am::fixtures::input(fixture, variant); const auto index = build_cloud_index(input);
        auto balls = am::fixtures::catalogue(fixture, index);
        if (variant) am::fixtures::distinct_representations(balls);
        ++fixtures;
        for (bool hashed : {false, true}) {
          am::Capture capture; FullBallStats stats; FullBallTimes times;
          FullBallTowerOptions options{.overlap_static = true, .hash_grouping = hashed};
          size_t orders = 0;
          {
            am::CaptureScope scope(capture);
            const auto result = full_ball_detail::AObservedBuilder(index, balls, fixture.k, stats, 4, {}, true, &times, options).run();
            orders = result.size();
          }
          for (unsigned k = 1; k <= orders; ++k)
            am::need(capture.slots[k].starts == 1 && capture.slots[k].finishes == 1, "event.capture_incomplete");
          capture.complete = true; ++captures;
          for (unsigned k = 1; k <= orders; ++k) {
            const auto manifest = am::make_manifest(capture, k);
            am::validate_binding(manifest, capture.slots[k]);
            const auto reference = am::replay_a(manifest);
            am::compare_output(reference, capture.slots[k].output);
            mutations(manifest, reference);
            for (bool root_high : {false, true}) {
              const auto candidate = ae::build(manifest, root_high);
              am::compare_output(candidate.output, reference);
              am::compare_output(candidate.output, capture.slots[k].output);
              const auto& w = candidate.work;
              am::need(w.edges == manifest.target.size() && w.groups <= w.vertices &&
                       w.forest_edges + w.components == w.vertices && w.ancestor_queries == w.vertices + w.edges &&
                       w.ancestor_steps == ae::product(w.ancestor_queries, std::bit_width(w.vertices)) &&
                       w.parent_event_incidences <= w.edges && w.output_capacity <= w.combined_capacity_observed_max &&
                       w.temporary_capacity_observed_max <= w.combined_capacity_observed_max,
                       "event.work_identity");
              ++replays; vertices += w.vertices; edges += w.edges; groups += w.groups; parents += w.parent_event_incidences;
              draft_parents += w.parent_draft_incidences; forest_parents += w.parent_forest_incidences;
              silent += w.silent_groups; capacity = std::max(capacity, w.combined_capacity_observed_max);
              queries += w.ancestor_queries; query_steps += w.ancestor_steps;
              for (size_t a = 0; a < reference.draft.actions(); ++a) {
                const u64 count = reference.draft.parent_begin[a + 1] - reference.draft.parent_begin[a];
                continuations += count == 1; max_parents = std::max(max_parents, count);
              }
            }
          }
        }
      }
    }
    // Combinatorial boundaries are not geometric catalogues or LiDAR tests.
    am::Manifest singleton; singleton.k = 1; singleton.domain = {std::numeric_limits<PointId>::max()};
    const auto single_reference = am::replay_a(singleton);
    for (bool root_high : {false, true}) am::compare_output(ae::build(singleton, root_high).output, single_reference);
    u64 abstract_replays = 0;
    for (u64 count : {u64{8}, u64{32}, u64{96}, u64{257}}) for (u64 seed = 1; seed <= 8; ++seed) {
      const auto input = abstract_program(count, seed * 927 + count); const auto reference = am::replay_a(input);
      mutations(input, reference);
      for (bool root_high : {false, true}) { am::compare_output(ae::build(input, root_high).output, reference); ++abstract_replays; }
    }
    am::need(silent && continuations && (!full || max_parents >= 32), "event.coverage");
    for (const auto& reason : mutation_reasons) am::need(!reason.empty(), "event.mutation_not_killed");
    std::cout << "{\"schema\":\"mhgp9_full_a_events_v1\",\"status\":\"passed\",\"full_gate\":" << (full ? "true" : "false")
              << ",\"fixtures\":" << fixtures << ",\"captures\":" << captures << ",\"event_replays\":" << replays
              << ",\"vertices\":" << vertices << ",\"edges\":" << edges << ",\"groups\":" << groups
              << ",\"parent_event_incidences\":" << parents << ",\"parent_draft_incidences\":" << draft_parents
              << ",\"parent_forest_incidences\":" << forest_parents << ",\"silent_groups\":" << silent
              << ",\"continuations\":" << continuations << ",\"max_parents\":" << max_parents
              << ",\"max_combined_capacity_observed_bytes\":" << capacity << ",\"ancestor_queries\":" << queries
              << ",\"ancestor_steps\":" << query_steps << ",\"boundary_replays\":2,\"abstract_replays\":" << abstract_replays
              << ",\"mutant_reasons\":[\"" << mutation_reasons[0] << "\",\"" << mutation_reasons[1] << "\",\"" << mutation_reasons[2]
              << "\"],\"parallel\":false,\"FULL_executed_by_candidate\":false,\"GCP_used\":false}\n";
    return 0;
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}
