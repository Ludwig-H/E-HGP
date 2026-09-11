// Bounded audit model. Sequential sorting/scanning is not a parallel backend.
// Geometry and vertical naturality are borrowed from the separately pinned
// prototype; this gate tests the physical export convention of Builder83f1.
#ifndef MHGP7_AUDIT_GRAPH_FULL_HEADER
#error "Supply the pinned graph_full.hpp with MHGP7_AUDIT_GRAPH_FULL_HEADER"
#endif
#include MHGP7_AUDIT_GRAPH_FULL_HEADER
#include "src/pipeline/generate.hpp"
#include <cstdio>
#include <map>
#include <string>
#include <tuple>

namespace historical_export_audit {
using namespace mhgp7;
namespace ag = atlas_graph_private;
namespace gf = graph_full_private;
namespace fc = filtered_calendar_private;
using Id = fc::Id;
using Rank = u32;
using GroupKey = std::pair<Rank, Id>;
using Use = std::tuple<unsigned, Rank, Id, Id>;
enum class Fault { none, bank_ball_id, exclude_silent, global_raw, history_node_order };

void need(bool value, const char* reason) {
  if (!value) throw std::runtime_error(reason);
}

struct Block {
  u32 ball;
  Rank rank;
  Id target, ordinal;
  bool contributes;
};
struct Plan {
  std::vector<Block> blocks;
  std::map<GroupKey, Id> minimum;
  std::map<Rank, u32> raw_ball;
  std::vector<Id> history_to_physical;
};
struct Counts {
  u64 runs = 0, census_balls = 0, orders = 0, block_queries = 0;
  u64 nodes = 0, contributions = 0, verticals = 0, population_rows = 0;
  u64 silent_blocks = 0, silent_min_contributing_groups = 0;
  u64 differing_global_raw_lots = 0, differing_history_node_orders = 0;
  u64 equivalent_raw_metadata_rescalings = 0;
  std::array<u64, 4> real_mutant_rejections{};
  std::array<std::string, 4> real_mutant_causes{};
};
Counts counts;

std::vector<Plan> plans(const ag::Extraction& extraction,
                        std::span<const gf::History> histories, Fault fault) {
  const auto& atlas = extraction.atlas;
  std::vector<Plan> out(histories.size());
  for (unsigned k = 1; k <= atlas.kmax; ++k) {
    const auto& history = histories[k - 1];
    auto& plan = out[k - 1];
    const gf::Chains chains(history, std::less<Rank>{});
    Id ordinal = 0;
    for (u32 b : atlas.program(k)) {
      const auto cell = atlas.cell(b, k);
      const auto rank = atlas.slots[b].rank;
      const auto& anchor = gf::detail::birth(history, extraction.phi[cell]);
      const auto answer = chains.query({anchor.segment, rank, rank, true});
      need(answer.segment != fc::absent, "physical.block_not_admitted");
      const auto ref = atlas.contributions[cell];
      const bool contributes = ref.shell != 0 || ref.interior;
      plan.blocks.push_back({b, rank, answer.segment, ordinal++, contributes});
      plan.raw_ball.try_emplace(rank, b);  // FIRST B of THIS K/level lot, including silence.
      if (fault != Fault::exclude_silent || contributes)
        plan.minimum.try_emplace(GroupKey{rank, answer.segment}, ordinal - 1);
    }
    // A group with no contribution still needs its creation order if it merges.
    for (const auto& block : plan.blocks)
      plan.minimum.try_emplace(GroupKey{block.rank, block.target}, block.ordinal);
    auto& mapping = plan.history_to_physical;
    mapping.resize(history.nodes.size());
    std::vector<Id> order(history.nodes.size());
    std::iota(order.begin(), order.end(), Id{0});
    std::map<Id, PointId> initial_points;
    if (k == 1) for (const auto& birth : history.births)
      initial_points.emplace(birth.segment, atlas.index->point_id(static_cast<i32>(birth.id)));
    std::sort(order.begin(), order.end(), [&](Id a, Id b) {
      const Rank da = history.nodes[a].date, db = history.nodes[b].date;
      if (da != db) return da < db;
      if (k == 1 && da == 0) return initial_points.at(a) < initial_points.at(b);
      if (fault == Fault::history_node_order) return a < b;
      return plan.minimum.at({da, a}) < plan.minimum.at({db, b});
    });
    for (Id i = 0; i < order.size(); ++i) mapping[order[i]] = i;
  }
  return out;
}

FullBallTowerResult reconstruct(const ag::Extraction& extraction,
                               std::span<const gf::History> histories,
                               const gf::Export& semantic_export,
                               Fault fault = Fault::none) {
  const auto& atlas = extraction.atlas;
  const auto& index = *atlas.index;
  auto ordered = plans(extraction, histories, fault);
  std::map<u32, Use> first_use;
  for (unsigned k = 1; k <= atlas.kmax; ++k) {
    const auto& plan = ordered[k - 1];
    for (const auto& block : plan.blocks) if (block.contributes) {
      const Use key{k, block.rank, plan.minimum.at({block.rank, block.target}), block.ordinal};
      const auto [where, fresh] = first_use.emplace(block.ball, key);
      if (!fresh && key < where->second) where->second = key;
    }
  }
  std::vector<u32> contributing;
  for (const auto& [ball, use] : first_use) { (void)use; contributing.push_back(ball); }
  if (fault != Fault::bank_ball_id)
    std::sort(contributing.begin(), contributing.end(), [&](u32 a, u32 b) {
      return first_use.at(a) < first_use.at(b);
    });
  std::vector<PointId> domain;
  for (i32 i = 0; i < static_cast<i32>(index.upos.size()); ++i) domain.push_back(index.point_id(i));
  std::sort(domain.begin(), domain.end());
  std::vector<FullCoveragePopulation> populations;
  for (PointId point : domain) populations.push_back({{}, {point}});
  std::vector<Id> ball_population(atlas.balls.size(), fc::absent);
  for (u32 b : contributing) {
    FullCoveragePopulation population;
    for (i32 point : atlas.balls[b].interior()) population.interior.push_back(index.point_id(point));
    for (i32 point : atlas.balls[b].shell()) population.shell.push_back(index.point_id(point));
    std::sort(population.interior.begin(), population.interior.end());
    std::sort(population.shell.begin(), population.shell.end());
    ball_population[b] = populations.size();
    populations.push_back(std::move(population));  // Identity is BallId, never content deduplication.
  }
  const auto bank = build_full_coverage_populations(domain, populations);
  need(bank.status == FullCertificateStatus::kOk, "physical.bank_encode");
  FullBallTowerResult result;
  for (unsigned k = 1; k <= atlas.kmax; ++k) {
    const auto& history = histories[k - 1];
    const auto& plan = ordered[k - 1];
    const auto& mapping = plan.history_to_physical;
    std::vector<FullCoverageBatch> batches;
    if (k == 1) {
      FullCoverageBatch initial;
      for (Id point = 0; point < domain.size(); ++point)
        initial.actions.push_back({{}, {{point, 1, false}}});
      batches.push_back(std::move(initial));
    }
    std::vector<const Block*> blocks;
    for (const auto& block : plan.blocks) blocks.push_back(&block);
    std::sort(blocks.begin(), blocks.end(), [&](const Block* a, const Block* b) {
      const auto action_key = [&](const Block* block) {
        // The node-order mutant must actually reorder creation actions too.
        const Id group = fault == Fault::history_node_order
            ? mapping[block->target] : plan.minimum.at({block->rank, block->target});
        return std::tuple{block->rank, group, block->ordinal};
      };
      return action_key(a) < action_key(b);
    });
    for (size_t at = 0; at < blocks.size();) {
      const Rank rank = blocks[at]->rank;
      const u32 raw_ball = fault == Fault::global_raw
          ? atlas.level_balls[rank - 1] : plan.raw_ball.at(rank);
      FullCoverageBatch batch{atlas.raw_level(raw_ball), {}};
      while (at < blocks.size() && blocks[at]->rank == rank) {
        const Id target = blocks[at]->target;
        FullCoverageAction action;
        do {
          const auto& block = *blocks[at++];
          if (block.contributes) {
            const auto ref = atlas.contributions[atlas.cell(block.ball, k)];
            action.contributions.push_back({ball_population[block.ball], ref.shell, ref.interior});
          }
        } while (at < blocks.size() && blocks[at]->rank == rank && blocks[at]->target == target);
        const auto& node = history.nodes[target];
        if (node.date == rank) {
          for (Id j = 0; j < node.parent_count; ++j)
            action.parents.push_back(mapping[history.parents[node.first + j]]);
          std::sort(action.parents.begin(), action.parents.end());
        } else {
          need(node.date < rank, "physical.continuation_date");
          if (action.contributions.empty()) continue;
          action.parents.push_back(mapping[target]);
        }
        batch.actions.push_back(std::move(action));
      }
      if (!batch.actions.empty()) batches.push_back(std::move(batch));
    }
    auto encoded = build_full_coverage_certificate(k, bank.value, batches);
    need(encoded.status == FullCertificateStatus::kOk, "physical.certificate_encode");
    need(encoded.value.nodes().size() == history.nodes.size(), "physical.node_count");
    std::vector<Id> lower(history.nodes.size(), fc::absent);
    if (k > 1) {
      std::vector<Id> old_to_history(histories[k - 2].nodes.size(), fc::absent);
      for (Id h = 0; h < old_to_history.size(); ++h)
        old_to_history[semantic_export.history_to_full[k - 2][h]] = h;
      for (Id h = 0; h < history.nodes.size(); ++h) {
        const Id old_node = semantic_export.history_to_full[k - 1][h];
        const Id old_lower = semantic_export.tower.orders[k - 1].lower_nodes[old_node];
        lower[mapping[h]] = ordered[k - 2].history_to_physical[old_to_history.at(old_lower)];
      }
    }
    result.orders.push_back({std::move(encoded.value), std::move(lower)});
  }
  result.status = FullBallStatus::kCompleteRelative;
  result.reason = "audit_physical_export_relative_to_validated_histories";
  return result;
}

void same_payload(const FullBallTowerResult& a, const FullBallTowerResult& b) {
  need(a.status == FullBallStatus::kCompleteRelative && b.status == a.status &&
       a.orders.size() == b.orders.size(), "physical.complete_towers");
  need(!a.orders.empty(), "physical.empty_tower");
  for (size_t k = 0; k < a.orders.size(); ++k) {
    const auto& x = a.orders[k].forest;
    const auto& y = b.orders[k].forest;
    need(x.populations().get() == a.orders[0].forest.populations().get() &&
         y.populations().get() == b.orders[0].forest.populations().get(), "physical.unique_shared_bank");
    need(x.populations()->domain() == y.populations()->domain() &&
         x.populations()->rows().size() == y.populations()->rows().size(), "physical.bank_shape");
    for (size_t i = 0; i < x.populations()->rows().size(); ++i)
      need(x.populations()->rows()[i].interior == y.populations()->rows()[i].interior &&
           x.populations()->rows()[i].shell == y.populations()->rows()[i].shell, "physical.bank_rows");
    need(x.order() == y.order() && x.nodes().size() == y.nodes().size() &&
         x.parents() == y.parents() && x.successors() == y.successors() &&
         a.orders[k].lower_nodes == b.orders[k].lower_nodes, "physical.topology_vertical");
    for (size_t n = 0; n < x.nodes().size(); ++n)
      need(x.nodes()[n].level == y.nodes()[n].level && x.nodes()[n].first == y.nodes()[n].first &&
           x.nodes()[n].parent_count == y.nodes()[n].parent_count, "physical.raw_node");
    need(x.contributions().size() == y.contributions().size(), "physical.contribution_count");
    for (size_t c = 0; c < x.contributions().size(); ++c) {
      const auto& u = x.contributions()[c]; const auto& v = y.contributions()[c];
      need(u.level == v.level && u.segment == v.segment && u.ref.population == v.ref.population &&
           u.ref.shell_mask == v.ref.shell_mask && u.ref.include_interior == v.ref.include_interior,
           "physical.raw_contribution");
    }
  }
}

struct Fixture { const char* name; std::vector<P3> points; unsigned kmax; };

std::vector<BallData> real_census(const CloudIndex& index, unsigned kmax) {
  const u64 smax = std::min<u64>(kmax + 1, index.input_count);
  GenerateOptions options; options.s = 8; options.smax = smax; options.threads = 1;
  GenerateStats generated;
  std::vector<BallCandidate> candidates;
  generate_candidates(index, options, &candidates, &generated);
  need(generated.cap_refus == kCapRefusNone && !generated.invariant_jneg, "physical.generation");
  for (size_t lane = 0; lane < 3; ++lane)
    need(generated.ledger_emitted_mass[lane] + generated.ledger_killed_mass[lane]
         == expected_pair_mass(index), "physical.pair_ledger");
  rle_candidates(&candidates, 1);
  ExpandStats stats;
  std::vector<Survivor> survivors;
  prefilter_balls(index, candidates, smax, 1, &survivors, &stats);
  std::vector<BallData> balls;
  need(census_balls(index, candidates, survivors, smax, kBallShellMax, 1, &balls, &stats)
       == PipelineStatus::kCompleteRegular, "physical.real_census");
  return balls;
}

void run_case(const Fixture& fixture, unsigned variant) {
  std::fprintf(stderr, "case=%s variant=%u\n", fixture.name, variant);
  std::vector<InputPoint> input;
  for (size_t i = 0; i < fixture.points.size(); ++i)
    input.push_back({variant ? static_cast<PointId>(4294967295u - 100003u * i)
                             : static_cast<PointId>(i), fixture.points[i]});
  if (variant) std::reverse(input.begin(), input.end());
  const auto index = build_cloud_index(input);
  auto balls = real_census(index, fixture.kmax);
  if (std::string_view(fixture.name) == "line5_equivalent_raw_metadata") {
    // Same exact real census and geometry. Only an accepted unreduced rational
    // encoding is rescaled, so a per-K raw-row choice becomes observable.
    size_t changed = 0;
    for (auto& ball : balls)
      if (ball.n_interior == 0 && same_exact_level(ball.level, ExactLevel{{4, 0, 0}, 1})) {
        const ExactLevel original = ball.level;
        need(ball.level.num[1] == 0 && ball.level.num[2] == 0 &&
             ball.level.num[0] <= std::numeric_limits<u64>::max() / 2 &&
             ball.level.den > 0 && ball.level.den <= std::numeric_limits<i64>::max() / 2,
             "physical.raw_rescaling_range");
        ball.level.num[0] *= 2; ball.level.den *= 2;
        need(same_exact_level(original, ball.level) && original != ball.level,
             "physical.raw_rescaling_exactness");
        ++changed;
      }
    need(changed == 1, "physical.raw_rescaling_nonvacuity");
    counts.equivalent_raw_metadata_rescalings += changed;
  }
  if (variant) {
    std::reverse(balls.begin(), balls.end());
    for (auto& ball : balls) {
      std::reverse(ball.shell_ids, ball.shell_ids + ball.n_shell);
      std::reverse(ball.interior_ids, ball.interior_ids + ball.n_interior);
    }
  }
  const auto reference = build_full_ball_tower(index, balls, fixture.kmax, 1);
  need(reference.status == FullBallStatus::kCompleteRelative, "physical.reference_complete");
  const auto extraction = ag::extract(index, balls, fixture.kmax);
  std::vector<gf::History> histories;
  for (const auto& order : extraction.orders) {
    const auto certificate = fc::spanning_certificate(order.graph, std::less<Rank>{});
    fc::verify_certificate(order.graph, certificate, std::less<Rank>{});
    histories.push_back(fc::reconstruct(certificate, std::less<Rank>{}));
  }
  const auto semantic = gf::build(extraction, histories, 1);
  const auto physical = reconstruct(extraction, histories, semantic);
  same_payload(reference, physical);
  const auto normal_plans = plans(extraction, histories, Fault::none);
  const auto other_plans = plans(extraction, histories, Fault::history_node_order);
  for (size_t k = 0; k < normal_plans.size(); ++k) {
    const auto& plan = normal_plans[k];
    counts.block_queries += plan.blocks.size();
    counts.differing_history_node_orders += plan.history_to_physical != other_plans[k].history_to_physical;
    for (const auto& [rank, b] : plan.raw_ball)
      counts.differing_global_raw_lots += extraction.atlas.raw_level(b)
          != extraction.atlas.raw_level(extraction.atlas.level_balls[rank - 1]);
    for (const auto& block : plan.blocks) if (!block.contributes) {
      ++counts.silent_blocks;
      if (plan.minimum.at({block.rank, block.target}) == block.ordinal)
        counts.silent_min_contributing_groups += std::any_of(plan.blocks.begin(), plan.blocks.end(),
            [&](const Block& other) { return other.rank == block.rank && other.target == block.target && other.contributes; });
    }
  }
  constexpr std::array<Fault, 4> faults{Fault::bank_ball_id, Fault::exclude_silent,
                                      Fault::global_raw, Fault::history_node_order};
  for (size_t f = 0; f < faults.size(); ++f) {
    try { same_payload(reference, reconstruct(extraction, histories, semantic, faults[f])); }
    catch (const std::runtime_error& error) {
      const std::string_view cause(error.what());
      const bool expected = f == 0 ? cause == "physical.bank_rows"
          : f == 1 ? (cause == "physical.bank_rows" || cause == "physical.raw_contribution" ||
                      cause == "physical.topology_vertical")
          : f == 2 ? (cause == "physical.raw_node" || cause == "physical.raw_contribution")
                   : (cause == "physical.raw_contribution" || cause == "physical.topology_vertical");
      need(expected, "physical.unexpected_mutant_cause");
      ++counts.real_mutant_rejections[f];
      counts.real_mutant_causes[f] = error.what();
    }
  }
  ++counts.runs; counts.census_balls += balls.size(); counts.orders += physical.orders.size();
  counts.population_rows += physical.orders.front().forest.populations()->rows().size();
  for (const auto& order : physical.orders) {
    counts.nodes += order.forest.nodes().size();
    counts.contributions += order.forest.contributions().size();
    if (order.forest.order() > 1) counts.verticals += order.lower_nodes.size();
  }
}

void abstract_witnesses() {
  // Structural fixture only, no claim that these three blocks are a 3D census:
  // B0 silent -> p, B1 contributes -> q, B2 contributes -> p, same rank.
  // All-block minima yield calls B2,B1. Filtering silence first yields B1,B2.
  const std::array<Id, 3> target{7, 9, 7};
  const std::array<bool, 3> contributing{false, true, true};
  std::map<Id, Id> all_min, contributing_min;
  for (Id i = 0; i < target.size(); ++i) {
    all_min.try_emplace(target[i], i);
    if (contributing[i]) contributing_min.try_emplace(target[i], i);
  }
  std::vector<Id> correct{1, 2}, wrong{1, 2};
  std::sort(correct.begin(), correct.end(), [&](Id a, Id b) {
    return std::pair{all_min.at(target[a]), a} < std::pair{all_min.at(target[b]), b};
  });
  std::sort(wrong.begin(), wrong.end(), [&](Id a, Id b) {
    return std::pair{contributing_min.at(target[a]), a} < std::pair{contributing_min.at(target[b]), b};
  });
  need(correct == std::vector<Id>({2, 1}) && wrong == std::vector<Id>({1, 2}),
       "abstract.silent_group_min_nonvacuity");
  need(correct != std::vector<Id>({1, 2}), "abstract.ball_id_order_nonvacuity");
  const ExactLevel global{{1, 0, 0}, 1}, per_k{{2, 0, 0}, 2};
  need(same_exact_level(global, per_k) && global != per_k, "abstract.raw_level_nonvacuity");
  // Same creation date, groups min=2 then min=0; arbitrary history IDs 0,1.
  const std::array<Id, 2> minimum{2, 0};
  std::vector<Id> ordered{0, 1};
  std::sort(ordered.begin(), ordered.end(), [&](Id a, Id b) { return minimum[a] < minimum[b]; });
  need(ordered == std::vector<Id>({1, 0}), "abstract.node_order_nonvacuity");
}
}  // namespace historical_export_audit

int main(int argc, char** argv) {
  using namespace historical_export_audit;
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try {
    const std::vector<Fixture> fixtures{
      {"line3", {{0, 0, 0}, {2, 0, 0}, {5, 0, 0}}, 3},
      {"square4", {{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {2, 2, 0}}, 4},
      {"line5_equivalent_raw_metadata", {{0, 0, 0}, {2, 0, 0}, {4, 0, 0}, {10, 0, 0}, {14, 0, 0}}, 5},
      {"spatial8", {{7, 42, 83}, {91, 12, 64}, {33, 88, 9}, {54, 20, 71},
                      {18, 61, 39}, {76, 53, 95}, {42, 7, 24}, {62, 94, 47}}, 8},
      {"spatial16", {{7, 42, 83}, {91, 12, 64}, {33, 88, 9}, {54, 20, 71},
                       {18, 61, 39}, {76, 53, 95}, {42, 7, 24}, {62, 94, 47},
                       {3, 29, 58}, {85, 73, 15}, {29, 36, 97}, {58, 65, 3},
                       {12, 81, 52}, {67, 17, 33}, {95, 46, 79}, {38, 57, 16}}, 10},
    };
    for (const auto& fixture : fixtures) for (unsigned variant : {0, 1}) run_case(fixture, variant);
    abstract_witnesses();
    need(counts.runs == 10 && counts.orders == 60 && counts.nodes > 100 &&
         counts.contributions > 100 && counts.block_queries > 100 && counts.silent_blocks > 0,
         "physical.nonvacuity");
    need(counts.equivalent_raw_metadata_rescalings == 2 && counts.differing_global_raw_lots > 0 &&
         counts.real_mutant_rejections[2] > 0, "physical.raw_fixture_nonvacuity");
    std::printf("{\"status\":\"passed\",\"scope\":\"bounded_Builder83f1_physical_payload\","
        "\"runs\":%llu,\"orders\":%llu,\"census_balls\":%llu,\"normal_export_block_queries\":%llu,"
        "\"nodes\":%llu,\"contributions\":%llu,\"verticals\":%llu,\"population_rows\":%llu,"
        "\"silent_blocks\":%llu,\"silent_min_contributing_groups\":%llu,"
        "\"differing_global_raw_lots\":%llu,\"differing_history_node_orders\":%llu,"
        "\"equivalent_raw_metadata_rescalings\":%llu,"
        "\"real_mutant_rejections\":[%llu,%llu,%llu,%llu],"
        "\"real_mutant_causes\":[\"%s\",\"%s\",\"%s\",\"%s\"],"
        "\"mutant_order\":[\"bank_ball_id\",\"exclude_silent\",\"global_raw\",\"history_node_order\"],"
        "\"abstract_causal_witnesses\":4,\"abstract_geometry_claimed\":false,"
        "\"counter_scope\":\"normal export once per fixture; reference, mutant and diagnostic reruns excluded\","
        "\"vertical_method\":\"pinned graph_full export then explicit ID transport\","
        "\"parallel_backend\":false,\"gcp_used\":false}\n",
        static_cast<unsigned long long>(counts.runs), static_cast<unsigned long long>(counts.orders),
        static_cast<unsigned long long>(counts.census_balls), static_cast<unsigned long long>(counts.block_queries),
        static_cast<unsigned long long>(counts.nodes), static_cast<unsigned long long>(counts.contributions),
        static_cast<unsigned long long>(counts.verticals), static_cast<unsigned long long>(counts.population_rows),
        static_cast<unsigned long long>(counts.silent_blocks), static_cast<unsigned long long>(counts.silent_min_contributing_groups),
        static_cast<unsigned long long>(counts.differing_global_raw_lots), static_cast<unsigned long long>(counts.differing_history_node_orders),
        static_cast<unsigned long long>(counts.equivalent_raw_metadata_rescalings),
        static_cast<unsigned long long>(counts.real_mutant_rejections[0]), static_cast<unsigned long long>(counts.real_mutant_rejections[1]),
        static_cast<unsigned long long>(counts.real_mutant_rejections[2]), static_cast<unsigned long long>(counts.real_mutant_rejections[3]),
        counts.real_mutant_causes[0].c_str(), counts.real_mutant_causes[1].c_str(),
        counts.real_mutant_causes[2].c_str(), counts.real_mutant_causes[3].c_str());
    return 0;
  } catch (const full_ball_detail::Failure& error) {
    std::fprintf(stderr, "GEOMETRY %s\n", error.reason);
  } catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL %s\n", error.what());
  }
  return 1;
}
