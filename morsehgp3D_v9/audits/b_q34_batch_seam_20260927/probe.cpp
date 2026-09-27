// Portable audit of the batch seam only. Old Pool + bands are intentionally
// both retained here; no new performance claim or engine change.
#define main factor_plan_unused_entry
#include "../b_q34_factor_plan_20260926/probe.cpp"
#undef main
#include "../b_q34_bands_20260927/bands.hpp"
#include "gen/pipeline/wspd_q34.hpp"

namespace bp = mhgp9::audit::bands;
enum class SeamMutant { None, NoSort, LoseImplicit };
struct SeamWork {
  u64 cases{}, rectangles{}, logical_pairs{}, evaluated{}, implicit_pairs{};
  u64 implicit3{}, implicit4{}, explicit3{}, explicit4{}, survivors{}, reorder_cases{};
};
struct Item { u64 ordinal; Q34SurvivingEdge edge; };

Q34FilterBatch candidate(const Q2CensusIndex& ix, unsigned k,
                        std::span<const WspdRectangle> rectangles, SeamWork& work,
                        SeamMutant mutant) {
  Q34FilterBatch out;
  out.backend = "audit_only_logical_P_separate_physical_E";
  const auto nodes = ix.spatial_nodes();
  const auto order = ix.spatial_order();
  const auto points = ix.cloud().points();
  Q34WitnessSearchWork rw{}, pw{};
  Q34WitnessBoundsWork rb{}, pb{};
  std::vector<Item> survivors;
  u64 prefix = 0;
  for (const auto& rect : rectangles) {
    ++work.rectangles;
    const auto& an = nodes[rect.a_node];
    const auto& bn = nodes[rect.b_node];
    const auto mask = filter_q34_witnesses(ix, an.box, bn.box, static_cast<std::uint8_t>(k),
        rect.lane_mask, rw, Q34WitnessBoundsMode::Affine, rb);
    out.rectangle_masks.push_back(mask);
    if (mask == 0) continue;
    const auto p = fp::product(an.range.size(), bn.range.size());
    counter_add(work.logical_pairs, p);
    counter_add(out.expanded_pairs, p); // Historical LOGICAL mass, not number of evaluated pairs.
    const fp::Plan old(ix, rect.a_node, rect.b_node, k, mask);
    const bp::Plan plan(old.a, old.b, k, mask);
    counter_add(work.implicit_pairs, p - plan.union_mass);
    const auto i3 = (mask & 2U) != 0 ? p - plan.q3_mass : 0;
    const auto i4 = (mask & 4U) != 0 ? p - plan.q4_mass : 0;
    counter_add(work.implicit3, i3); counter_add(work.implicit4, i4);
    if (mutant != SeamMutant::LoseImplicit) {
      counter_add(out.pair_q3_rejected, i3); counter_add(out.pair_q4_rejected, i4);
    }
    u64 evaluated = 0;
    for (const auto& band : plan.bands) {
      const auto ar = old.a.groups[band.a_class].ranks;
      for (auto a = ar.first; a != ar.last; ++a)
        for (auto b = std::size_t{band.b_first}; b != band.b_last; ++b) {
          ++evaluated;
          const auto ai = old.a.grouped[a], bi = old.b.grouped[b];
          const auto input_mask = static_cast<std::uint8_t>(plan.pair_mask(band.a_class, b));
          const auto m = filter_q34_witnesses(ix, singleton_box(points[order[ai]]),
              singleton_box(points[order[bi]]), static_cast<std::uint8_t>(k), input_mask,
              pw, Q34WitnessBoundsMode::Affine, pb);
          if ((input_mask & 2U) != 0 && (m & 2U) == 0) {
            ++out.pair_q3_rejected; ++work.explicit3;
          }
          if ((input_mask & 4U) != 0 && (m & 4U) == 0) {
            ++out.pair_q4_rejected; ++work.explicit4;
          }
          if (m != 0) {
            const u64 local = fp::product(ai - an.range.first, bn.range.size()) + bi - bn.range.first;
            require(local < p && local <= std::numeric_limits<u64>::max() - prefix, "seam_ordinal_overflow");
            survivors.push_back({prefix + local,
                {static_cast<std::uint32_t>(ai), static_cast<std::uint32_t>(bi), m}});
          }
        }
    }
    require(evaluated == plan.union_mass, "seam_E_mismatch");
    counter_add(work.evaluated, evaluated);
    counter_add(prefix, p);
  }
  const auto less = [](const Item& a, const Item& b) { return a.ordinal < b.ordinal; };
  if (!std::is_sorted(survivors.begin(), survivors.end(), less)) ++work.reorder_cases;
  if (mutant != SeamMutant::NoSort) std::sort(survivors.begin(), survivors.end(), less);
  for (std::size_t i = 0; i != survivors.size(); ++i) {
    if (i != 0) require(survivors[i-1].ordinal < survivors[i].ordinal, "seam_original_order");
    out.survivors.push_back(survivors[i].edge);
  }
  out.rectangle_visits = rw.node_visits;
  out.pair_visits = pw.node_visits;
  counter_add(work.survivors, out.survivors.size());
  require(pw.queries == work.evaluated, "seam_physical_queries");
  return out;
}

int main(int argc, char** argv) {
  try {
    SeamMutant mutant = SeamMutant::None;
    if (argc == 2 && std::string_view(argv[1]) == "no-sort") mutant = SeamMutant::NoSort;
    else if (argc == 2 && std::string_view(argv[1]) == "lose-implicit") mutant = SeamMutant::LoseImplicit;
    else require(argc == 1, "seam_cli");
    SeamWork total;
    for (const char* family : {"uniform", "terrain", "clusters", "rows"})
      for (const unsigned n : {16U, 40U, 80U}) {
        auto points = bench::make_front_fixture(n, family, 3).points;
        for (unsigned permutation = 0; permutation != 2; ++permutation) {
          if (permutation != 0) std::reverse(points.begin(), points.end());
          const auto ix = make_q2_cloud_index(prepare_cloud(points));
          for (unsigned k : {2U, 3U, 5U, 10U}) for (unsigned s : {8U, 10U, 12U})
            for (auto mode : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
              std::vector<WspdRectangle> rectangles;
              static_cast<void>(run_wspd_front(*ix, k, s, mode,
                  [&](const WspdRectangle& r) { rectangles.push_back(r); }, 6));
              SeamWork work;
              const auto actual = candidate(*ix, k, rectangles, work, mutant);
              const auto expected = run_q34_filter_batch_cpu(*ix, k, rectangles, 1);
              require(actual.rectangle_masks == expected.rectangle_masks, "seam_rectangle_masks");
              require(actual.expanded_pairs == expected.expanded_pairs, "seam_logical_mass");
              require(actual.survivors == expected.survivors, "seam_exact_ordered_survivors");
              require(actual.pair_q3_rejected == expected.pair_q3_rejected &&
                      actual.pair_q4_rejected == expected.pair_q4_rejected, "seam_lane_accounting");
              require(work.logical_pairs == work.evaluated + work.implicit_pairs, "seam_pair_partition");
              require(actual.rectangle_visits == expected.rectangle_visits, "seam_rectangle_work");
              ++total.cases;
              total.rectangles += work.rectangles; total.logical_pairs += work.logical_pairs;
              total.evaluated += work.evaluated; total.implicit_pairs += work.implicit_pairs;
              total.implicit3 += work.implicit3; total.implicit4 += work.implicit4;
              total.explicit3 += work.explicit3; total.explicit4 += work.explicit4;
              total.survivors += work.survivors; total.reorder_cases += work.reorder_cases;
            }
        }
      }
    require(total.cases == 576 && total.implicit_pairs > 0 && total.implicit3 > 0 && total.implicit4 > 0 &&
            total.survivors > 0 && total.reorder_cases > 0, "seam_nonvacuity");
    std::cout << "{\"schema\":\"mhgp9_q34_batch_seam_v1\",\"status\":\"pass\","
              << "\"cases\":" << total.cases << ",\"rectangles\":" << total.rectangles
              << ",\"logical_pairs\":" << total.logical_pairs << ",\"evaluated\":" << total.evaluated
              << ",\"implicit_pairs\":" << total.implicit_pairs << ",\"implicit3\":" << total.implicit3
              << ",\"implicit4\":" << total.implicit4 << ",\"survivors\":" << total.survivors
              << ",\"reorder_cases\":" << total.reorder_cases << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
