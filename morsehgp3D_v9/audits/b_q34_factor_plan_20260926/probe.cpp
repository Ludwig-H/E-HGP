// Standalone audit driver. Product code is linked read-only, never shadowed.
#include "plan.hpp"
#include "gen/lanes/q34_witness_search.hpp"
#include "gen/wspd/front.hpp"
#include "front_fixtures.hpp"

#include <charconv>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>

namespace fp = mhgp9::audit::factor_plan;
using namespace mhgp9::gen;
using Clock = std::chrono::steady_clock;
constexpr const char* schema = "mhgp9_q34_factor_plan_v1";

double elapsed(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}
void require(bool value, const char* cause) {
  if (!value) throw std::runtime_error(std::string("factor_plan.") + cause);
}
template<class T> T number(std::string_view text) {
  T value{};
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (text.empty() || error != std::errc{} || end != text.data() + text.size())
    throw std::invalid_argument("factor_plan.invalid_integer");
  return value;
}
std::string quoted(std::string_view text) {
  std::ostringstream out;
  out << '"';
  for (unsigned char c : text) {
    if (c == '\\' || c == '"') out << '\\' << static_cast<char>(c);
    else if (c < 32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << unsigned(c);
    else out << static_cast<char>(c);
  }
  out << '"';
  return out.str();
}
std::string hex(u64 x) {
  std::ostringstream out;
  out << std::hex << std::setw(16) << std::setfill('0') << x;
  return out.str();
}
u64 input_hash(std::span<const Point3> points) {
  u64 hash = 14695981039346656037ULL;
  bench::front_hash_word(hash, 1);
  bench::front_hash_word(hash, points.size());
  for (const auto p : points) for (std::size_t d = 0; d != 3; ++d)
    bench::front_hash_word(hash, static_cast<u64>(p[d]));
  return hash;
}

// Independent strict Gram computation: Xi=|u|^2|v|^2-H^2, rather
// than the product's cross-component implementation. u18 fits signed128.
bool reference_point(unsigned q, Point3 a, Point3 b, Point3 z) {
  i128 h = 0, uu = 0, vv = 0;
  for (std::size_t d = 0; d != 3; ++d) {
    const i128 u = static_cast<i128>(z[d]) - a[d];
    const i128 v = static_cast<i128>(b[d]) - z[d];
    h += u * v;
    uu += u * u;
    vv += v * v;
  }
  return h > 0 && static_cast<i128>(q == 3 ? 3 : 2) * h * h > uu * vv - h * h;
}
bool reference_box(unsigned q, Point3 a, const Box3& b, Point3 z) {
  for (unsigned corner = 0; corner != 8; ++corner) {
    const Point3 p{(corner & 1U) != 0 ? b.high.x : b.low.x,
                   (corner & 2U) != 0 ? b.high.y : b.low.y,
                   (corner & 4U) != 0 ? b.high.z : b.low.z};
    if (!reference_point(q, a, p, z)) return false;
  }
  return true;
}

struct Gate {
  u64 fixtures{}, permutations{}, fronts{}, rectangles{}, pairs{}, point_checks{}, box_checks{};
  u64 rejected_q3{}, rejected_q4{}, surviving_q3{}, surviving_q4{}, both_mask{};
  u64 contacts{}, pool_missed_saturated_anchors{}, refusals{}, nonidentity_indices{};
  u64 singleton_rectangles{}, zero_reduction_rectangles{}, q3_only_masks{}, q4_only_masks{};
  u64 hash{14695981039346656037ULL};
};

void judge_plan(const Q2CensusIndex& index, const WspdRectangle& r,
                unsigned k, fp::Mutant mutant, Gate& gate) {
  const fp::Plan plan(index, r.a_node, r.b_node, k, r.lane_mask, mutant);
  const auto points = index.cloud().points();
  const auto order = index.spatial_order();
  const auto nodes = index.spatial_nodes();
  const auto judge_factor = [&](const fp::Factor& f, const Box3& own, const Box3& opposite) {
    std::vector<std::size_t> expected;
    for (auto rank = f.original_ranks.first; rank != f.original_ranks.last; ++rank)
      expected.push_back(order[rank]);
    const auto score = [&](std::size_t id) {
      i64 dot = 0;
      for (std::size_t d = 0; d != 3; ++d)
        dot += (static_cast<i64>(opposite.low[d]) + opposite.high[d] - own.low[d] - own.high[d]) * points[id][d];
      return dot;
    };
    std::sort(expected.begin(), expected.end(), [&](auto x, auto y) {
      return score(x) != score(y) ? score(x) > score(y) : x < y;
    });
    expected.resize(std::min<std::size_t>(expected.size(), k));
    require(expected == f.proposals, "original_ids_differ");
    std::vector<fp::Credit> credits;
    for (auto rank = f.original_ranks.first; rank != f.original_ranks.last; ++rank) {
      fp::Credit c{};
      const auto id = order[rank];
      for (const auto w : expected) {
        if (w == id) continue;
        for (unsigned q = 3; q != 5; ++q) {
          const auto bit = q == 3 ? 2U : 4U;
          if ((r.lane_mask & bit) == 0) continue;
          const bool ref = reference_box(q, points[id], opposite, points[w]);
          PredicateWork work;
          require(universal_witness(q == 3 ? Lane::Q3 : Lane::Q4, points[id], opposite, points[w], work) == ref,
                  "product_universal_differs");
          ++gate.box_checks;
          auto& count = q == 3 ? c.q3 : c.q4;
          if (ref && count < k + 2U - q) ++count;
        }
      }
      for (unsigned q = 3; q != 5; ++q) {
        const auto bit = q == 3 ? 2U : 4U;
        if ((r.lane_mask & bit) == 0) continue;
        unsigned exhaustive = 0;
        for (auto wr = f.original_ranks.first; wr != f.original_ranks.last; ++wr)
          if (order[wr] != id && reference_box(q, points[id], opposite, points[order[wr]])) ++exhaustive;
        if ((q == 3 ? c.q3 : c.q4) == 0 && exhaustive >= k + 2U - q)
          ++gate.pool_missed_saturated_anchors;
      }
      credits.push_back(c);
    }
    require(credits == f.credits, "strict_credit_differs");
    auto grouped = f.grouped;
    std::sort(grouped.begin(), grouped.end());
    require(grouped.size() == f.original_ranks.size(), "grouped_size");
    for (std::size_t i = 0; i != grouped.size(); ++i)
      require(grouped[i] == f.original_ranks.first + i, "grouped_rank_permutation");
    for (const auto& g : f.groups) {
      require(g.ranks.first < g.ranks.last && g.ranks.last <= f.grouped.size(), "group_bounds");
      for (auto i = g.ranks.first; i != g.ranks.last; ++i)
        require(credits[f.grouped[i] - f.original_ranks.first] == g.credit, "group_credit");
    }
    return credits;
  };
  const auto ac = judge_factor(plan.a, nodes[r.a_node].box, nodes[r.b_node].box);
  const auto bc = judge_factor(plan.b, nodes[r.b_node].box, nodes[r.a_node].box);
  std::map<std::pair<std::size_t, std::size_t>, unsigned> expanded;
  for (const auto& block : plan.blocks)
    for (auto ai = block.a.first; ai != block.a.last; ++ai)
      for (auto bi = block.b.first; bi != block.b.last; ++bi) {
        const auto key = std::make_pair(plan.a.grouped[ai], plan.b.grouped[bi]);
        require(expanded.emplace(key, block.mask).second, "duplicate_residual_pair");
      }
  u64 e3 = 0, e4 = 0, eu = 0;
  for (auto ar = plan.a.original_ranks.first; ar != plan.a.original_ranks.last; ++ar)
    for (auto br = plan.b.original_ranks.first; br != plan.b.original_ranks.last; ++br) {
      ++gate.pairs;
      unsigned expected = 0;
      const auto ca = ac[ar - plan.a.original_ranks.first], cb = bc[br - plan.b.original_ranks.first];
      if ((r.lane_mask & 2U) != 0 && ca.q3 + cb.q3 < k - 1U) expected |= 2U;
      if ((r.lane_mask & 4U) != 0 && ca.q4 + cb.q4 < k - 2U) expected |= 4U;
      const auto it = expanded.find({ar, br});
      const auto observed = it == expanded.end() ? 0U : it->second;
      require(observed == expected, "joint_mask_differs");
      bench::front_hash_word(gate.hash, observed);
      if (observed != 0) ++eu;
      if ((observed & 2U) != 0) { ++e3; ++gate.surviving_q3; }
      if ((observed & 4U) != 0) { ++e4; ++gate.surviving_q4; }
      if (observed == 6) ++gate.both_mask;
      if (observed == 2) ++gate.q3_only_masks;
      if (observed == 4) ++gate.q4_only_masks;
      for (unsigned q = 3; q != 5; ++q) {
        const auto bit = q == 3 ? 2U : 4U;
        if ((r.lane_mask & bit) == 0) continue;
        unsigned global = 0;
        for (const auto z : points) {
          const auto ref = reference_point(q, points[order[ar]], points[order[br]], z);
          PredicateWork work;
          require(point_witness(q == 3 ? Lane::Q3 : Lane::Q4, points[order[ar]], points[order[br]], z, work) == ref,
                  "product_point_differs");
          ++gate.point_checks;
          if (ref) ++global;
        }
        if ((observed & bit) == 0) {
          require(global >= k + 2U - q, "unsafe_rejection");
          if (q == 3) ++gate.rejected_q3; else ++gate.rejected_q4;
        }
      }
    }
  require(plan.union_mass == eu && plan.q3_mass == e3 && plan.q4_mass == e4, "descriptor_mass");
  if (plan.a.original_ranks.size() == 1 || plan.b.original_ranks.size() == 1) ++gate.singleton_rectangles;
  if (plan.union_mass == fp::product(plan.a.original_ranks.size(), plan.b.original_ranks.size()))
    ++gate.zero_reduction_rectangles;
  ++gate.rectangles;
}

void run_gate(fp::Mutant mutant) {
  Gate gate;
  std::vector<std::vector<Point3>> fixtures{
      {{100,100,100},{111,110,110},{421,100,100}}, // q4 cone contact; s8/10/12.
      {{300,100,100},{100,100,100},{101,100,100}}, // ha=1, hb=0; IDs permuted.
      {{100,100,100},{101,101,100},{1100,830,100},{1120,827,100}},
      {{0,0,0},{262143,0,0},{1,1,0},{262142,1,0},{100,10,10}}};
  std::vector<Point3> missed{{100,100,100}};
  for (Coordinate x = 1; x != 5; ++x) missed.push_back({100+x,100,100});
  for (Coordinate x = 5; x != 10; ++x) missed.push_back({100+x,200,100});
  missed.push_back({10100,100,100});
  fixtures.push_back(missed);
  for (const auto family : {"uniform", "terrain", "clusters", "rows"})
    fixtures.push_back(bench::make_front_fixture(24, family, 3).points);
  require(!reference_point(4, {100,100,100}, {421,100,100}, {111,110,110}), "contact_fixture");
  ++gate.contacts;
  for (auto points : fixtures) {
    ++gate.fixtures;
    for (unsigned permutation = 0; permutation != 2; ++permutation) {
      if (permutation != 0) std::reverse(points.begin(), points.end());
      ++gate.permutations;
      const auto cloud = prepare_cloud(points);
      const auto index = make_q2_cloud_index(cloud);
      bool nonidentity = false;
      for (std::size_t i = 0; i != points.size(); ++i) nonidentity |= index->spatial_order()[i] != i;
      if (nonidentity) ++gate.nonidentity_indices;
      for (const unsigned k : {2U, 3U, 5U, 10U}) for (const unsigned s : {8U, 10U, 12U}) {
        ++gate.fronts;
        // PURE keeps the test witnesses reachable; benchmark uses the actual
        // production MidpointSamples + Affine rectangle filter instead.
        const auto front = run_wspd_front(*index, k, s, WspdFrontMode::Pure,
            [&](const WspdRectangle& r) { judge_plan(*index, r, k, mutant, gate); }, 6);
        require(front.total_unordered_pairs == fp::product(points.size(), points.size()-1)/2,
                "front_total_mass");
      }
      try {
        const fp::Plan invalid(*index, 0, 0, 5, 6);
        static_cast<void>(invalid);
        throw std::runtime_error("factor_plan.overlap_not_refused");
      } catch (const std::invalid_argument&) { ++gate.refusals; }
      for (const auto& invalid : {std::pair{1U, std::uint8_t{2}}, std::pair{11U, std::uint8_t{6}},
                                 std::pair{2U, std::uint8_t{4}}}) {
        try {
          const fp::Plan bad(*index, 0, 0, invalid.first, invalid.second);
          static_cast<void>(bad);
          throw std::runtime_error("factor_plan.invalid_K_mask_not_refused");
        } catch (const std::invalid_argument&) { ++gate.refusals; }
      }
    }
  }
  require(gate.rejected_q3 > 0 && gate.rejected_q4 > 0 && gate.surviving_q3 > 0 &&
      gate.surviving_q4 > 0 && gate.both_mask > 0 && gate.nonidentity_indices > 0 &&
      gate.pool_missed_saturated_anchors > 0 && gate.refusals == 4*gate.permutations &&
      gate.singleton_rectangles > 0 && gate.zero_reduction_rectangles > 0 &&
      gate.q3_only_masks > 0 && gate.q4_only_masks > 0,
      "nonvacuity");
  std::cout << "{\"schema\":\"" << schema << "\",\"status\":\"pass\",\"mode\":\"gate\",\"coverage\":{";
#define GATE_FIELD(name) std::cout << "\"" #name "\":" << gate.name << ','
  GATE_FIELD(fixtures); GATE_FIELD(permutations); GATE_FIELD(fronts); GATE_FIELD(rectangles);
  GATE_FIELD(pairs); GATE_FIELD(point_checks); GATE_FIELD(box_checks); GATE_FIELD(rejected_q3);
  GATE_FIELD(rejected_q4); GATE_FIELD(surviving_q3); GATE_FIELD(surviving_q4); GATE_FIELD(both_mask);
  GATE_FIELD(contacts); GATE_FIELD(pool_missed_saturated_anchors); GATE_FIELD(refusals);
  GATE_FIELD(singleton_rectangles); GATE_FIELD(zero_reduction_rectangles);
  GATE_FIELD(q3_only_masks); GATE_FIELD(q4_only_masks);
#undef GATE_FIELD
  std::cout << "\"nonidentity_indices\":" << gate.nonidentity_indices << "},\"digest\":\"" << hex(gate.hash) << "\"}\n";
}

std::vector<Point3> load_frame(const std::string& path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) throw std::runtime_error("factor_plan.frame_open");
  const auto end = file.tellg();
  if (end <= 0 || end % 12 != 0) throw std::invalid_argument("factor_plan.frame_size_u32le");
  const auto bytes = static_cast<std::uintmax_t>(end);
  if (bytes / 12 > std::numeric_limits<std::size_t>::max()) throw std::overflow_error("factor_plan.frame_size");
  std::vector<Point3> points(static_cast<std::size_t>(bytes / 12));
  file.seekg(0);
  for (auto& p : points) {
    std::array<Coordinate, 3> xyz{};
    for (auto& value : xyz) {
      std::array<unsigned char, 4> raw{};
      file.read(reinterpret_cast<char*>(raw.data()), raw.size());
      if (!file) throw std::runtime_error("factor_plan.frame_short_read");
      const std::uint32_t x = static_cast<std::uint32_t>(raw[0]) | (static_cast<std::uint32_t>(raw[1]) << 8U) |
          (static_cast<std::uint32_t>(raw[2]) << 16U) | (static_cast<std::uint32_t>(raw[3]) << 24U);
      if (x > static_cast<std::uint32_t>(coordinate_limit)) throw std::invalid_argument("factor_plan.frame_outside_u18");
      value = static_cast<Coordinate>(x);
    }
    p = {xyz[0], xyz[1], xyz[2]};
  }
  require(file.peek() == std::char_traits<char>::eof(), "frame_changed_size");
  return points;
}

struct Measure {
  u64 rectangles{}, skipped_min_factor_rectangles{}, skipped_min_factor_mass{};
  u64 skipped_capacity_rectangles{}, skipped_capacity_mass{}, P{}, P3{}, P4{}, E_union{}, E3{}, E4{};
  u64 retained_bytes_sum{}, retained_bytes_peak{}, fallback_descriptors{}, descriptor_bytes_sum{};
  u64 residual_rank_entries_sum{}, front_union{};
  fp::Work work;
  double filter_ms{}, plans_ms{};
};
void merge(fp::Work& a, const fp::Work& b) {
#define SUM(name) counter_add(a.name, b.name)
  SUM(factor_sites); SUM(selection_visits); SUM(selection_tests); SUM(selection_shifts);
  SUM(selected_sites); SUM(anchor_visits); SUM(witness_attempts); SUM(self_skips);
  SUM(q3_credits); SUM(q4_credits); SUM(grouping_visits); SUM(grouping_sort_tests);
  SUM(occupied_classes); SUM(class_slots); SUM(cells_tested); SUM(descriptors);
  SUM(predicates.universal_queries); SUM(predicates.corner_tests); SUM(predicates.point_tests);
#undef SUM
}

void run_measure(std::vector<Point3> points, unsigned k, unsigned s, std::size_t min_factor,
                 std::string_view mode, std::string_view family, Clock::time_point start, double input_ms) {
  if (k < 2 || k > 10 || (s != 8 && s != 10 && s != 12) || min_factor == 0)
    throw std::invalid_argument("factor_plan.require_K2to10_s8or10or12_min_factor_positive");
  const auto hash = input_hash(points);
  const auto index_start = Clock::now();
  const auto cloud = prepare_cloud(points);
  const auto index = make_q2_cloud_index(cloud);
  const auto index_ms = elapsed(index_start);
  Q34WitnessSearchWork search;
  Q34WitnessBoundsWork bounds;
  Measure m;
  const auto front_start = Clock::now();
  const auto front = run_wspd_front(*index, k, s, WspdFrontMode::MidpointSamples,
      [&](const WspdRectangle& r) {
        const auto& a = index->spatial_nodes()[r.a_node];
        const auto& b = index->spatial_nodes()[r.b_node];
        const auto mass = fp::product(a.range.size(), b.range.size());
        counter_add(m.front_union, mass);
        const auto filter_start = Clock::now();
        const auto mask = filter_q34_witnesses(*index, a.box, b.box, static_cast<std::uint8_t>(k), r.lane_mask,
            search, Q34WitnessBoundsMode::Affine, bounds);
        m.filter_ms += elapsed(filter_start);
        if (mask == 0) return;
        counter_add(m.P, mass);
        if ((mask & 2U) != 0) counter_add(m.P3, mass);
        if ((mask & 4U) != 0) counter_add(m.P4, mass);
        const auto plan_start = Clock::now();
        const auto minimum_threshold = (mask & 4U) != 0 ? k-2U : k-1U;
        bool skipped = false;
        if (a.range.size() + b.range.size() - 2 < minimum_threshold) {
          counter_add(m.skipped_capacity_rectangles);
          counter_add(m.skipped_capacity_mass, mass);
          skipped = true;
        } else if (std::min(a.range.size(), b.range.size()) < min_factor) {
          counter_add(m.skipped_min_factor_rectangles);
          counter_add(m.skipped_min_factor_mass, mass);
          skipped = true;
        }
        if (skipped) {
          counter_add(m.fallback_descriptors);
          counter_add(m.E_union, mass);
          if ((mask & 2U) != 0) counter_add(m.E3, mass);
          if ((mask & 4U) != 0) counter_add(m.E4, mass);
        } else {
          const fp::Plan plan(*index, r.a_node, r.b_node, k, mask);
          ++m.rectangles;
          merge(m.work, plan.work);
          counter_add(m.E_union, plan.union_mass);
          counter_add(m.E3, plan.q3_mass);
          counter_add(m.E4, plan.q4_mass);
          counter_add(m.retained_bytes_sum, plan.retained_bytes());
          m.retained_bytes_peak = std::max(m.retained_bytes_peak, static_cast<u64>(plan.retained_bytes()));
          counter_add(m.descriptor_bytes_sum, fp::product(plan.blocks.capacity(), sizeof(fp::Block)));
          counter_add(m.residual_rank_entries_sum, static_cast<u64>(plan.a.grouped.size() + plan.b.grouped.size()));
        }
        m.plans_ms += elapsed(plan_start);
      }, 6);
  const auto front_ms = elapsed(front_start);
  require(m.E_union <= m.P && m.E3 <= m.P3 && m.E4 <= m.P4 && m.P <= m.front_union,
          "mass_conservation");
  const auto full_ms = elapsed(start);
  std::cout << std::fixed << std::setprecision(6)
      << "{\"schema\":\"" << schema << "\",\"status\":\"pass\",\"mode\":" << quoted(mode)
      << ",\"n\":" << points.size() << ",\"k\":" << k << ",\"s\":" << s << ",\"min_factor\":" << min_factor
      << ",\"input_hash\":\"" << hex(hash) << "\",\"seed\":3,\"recipe_domain\":"
      << quoted(mode == "synthetic" ? "u16_recipe_u32_storage" : "u32le_u18_validated")
      << ",\"family\":" << quoted(family) << ",\"front\":{\"total_unordered_pairs\":" << front.total_unordered_pairs
      << ",\"rectangles\":" << front.work.emitted_rectangles << ",\"product_visits\":" << front.work.product_visits
      << ",\"F\":" << front.work.emitted_factor_sites << ",\"pair_mass_q3\":" << front.work.residual_pair_mass[1]
      << ",\"pair_mass_q4\":" << front.work.residual_pair_mass[2] << ",\"pair_mass_union\":" << m.front_union
      << "},\"rectangle_filter\":{\"queries\":" << search.queries << ",\"node_visits\":" << search.node_visits
      << ",\"h_bound_tests\":" << search.h_bound_tests << ",\"xi_bound_tests\":" << search.xi_bound_tests
      << ",\"affine_h_tests\":" << bounds.affine_h_tests << ",\"affine_xi_tests\":" << bounds.affine_xi_tests
      << ",\"rectangles_surviving\":" << m.rectangles+m.skipped_capacity_rectangles+m.skipped_min_factor_rectangles
      << ",\"P\":" << m.P << ",\"P3\":" << m.P3 << ",\"P4\":" << m.P4
      << "},\"plans\":{\"rectangles\":" << m.rectangles
      << ",\"skipped_min_factor_rectangles\":" << m.skipped_min_factor_rectangles
      << ",\"skipped_min_factor_mass\":" << m.skipped_min_factor_mass
      << ",\"skipped_capacity_rectangles\":" << m.skipped_capacity_rectangles
      << ",\"skipped_capacity_mass\":" << m.skipped_capacity_mass << ",\"F\":" << m.work.factor_sites
      << ",\"factor_visits\":" << m.work.selection_visits+m.work.anchor_visits;
#define WORK_FIELD(name) std::cout << ",\"" #name "\":" << m.work.name
  WORK_FIELD(selection_visits); WORK_FIELD(selection_tests); WORK_FIELD(selection_shifts);
  WORK_FIELD(selected_sites); WORK_FIELD(anchor_visits); WORK_FIELD(witness_attempts); WORK_FIELD(self_skips);
  WORK_FIELD(q3_credits); WORK_FIELD(q4_credits); WORK_FIELD(grouping_visits); WORK_FIELD(grouping_sort_tests);
  WORK_FIELD(class_slots); WORK_FIELD(occupied_classes); WORK_FIELD(cells_tested); WORK_FIELD(descriptors);
#undef WORK_FIELD
  std::cout << ",\"corner_tests\":" << m.work.predicates.corner_tests
      << ",\"point_tests\":" << m.work.predicates.point_tests
      << ",\"universal_queries\":" << m.work.predicates.universal_queries
      << ",\"fallback_descriptors\":" << m.fallback_descriptors
      << ",\"E_union\":" << m.E_union << ",\"E3\":" << m.E3 << ",\"E4\":" << m.E4
      << ",\"retained_bytes_sum\":" << m.retained_bytes_sum << ",\"retained_bytes_peak\":" << m.retained_bytes_peak
      << ",\"descriptor_bytes_sum\":" << m.descriptor_bytes_sum
      << ",\"residual_rank_entries_sum\":" << m.residual_rank_entries_sum
      << ",\"preparation_scratch_bytes\":" << fp::Plan::preparation_scratch_bytes
      << "},\"memory\":{\"cloud_bytes\":" << cloud->retained_bytes() << ",\"index_bytes\":" << index->retained_bytes()
      << ",\"input_bytes\":" << fp::product(points.capacity(), sizeof(Point3))
      << "},\"times_ms\":{\"input\":" << input_ms << ",\"index\":" << index_ms
      << ",\"front_filter_plan\":" << front_ms << ",\"rectangle_filter\":" << m.filter_ms
      << ",\"plans\":" << m.plans_ms << ",\"front_residual\":" << front_ms-m.filter_ms-m.plans_ms
      << ",\"total\":" << full_ms << "}}\n";
}

int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string_view(argv[1]) == "--gate") { run_gate(fp::Mutant::None); return 0; }
    if (argc == 3 && std::string_view(argv[1]) == "--mutant") {
      const std::string_view name = argv[2];
      if (name == "equality") run_gate(fp::Mutant::Equality);
      else if (name == "overlap") run_gate(fp::Mutant::Overlap);
      else if (name == "ids") run_gate(fp::Mutant::Ids);
      else throw std::invalid_argument("factor_plan.unknown_mutant");
      throw std::runtime_error("factor_plan.mutant_survived");
    }
    const auto start = Clock::now();
    std::size_t min_factor = 2;
    if (argc > 1 && std::string_view(argv[argc-1]).starts_with("--min-factor=")) {
      min_factor = number<std::size_t>(std::string_view(argv[argc-1]).substr(13));
      --argc;
    }
    if (argc == 6 && std::string_view(argv[1]) == "--synthetic") {
      const auto n = number<std::size_t>(argv[3]);
      const auto k = number<unsigned>(argv[4]), s = number<unsigned>(argv[5]);
      auto fixture = bench::make_front_fixture(n, argv[2], 3);
      const auto input_ms = elapsed(start);
      run_measure(std::move(fixture.points), k, s, min_factor, "synthetic", argv[2], start, input_ms);
      return 0;
    }
    if (argc == 5 && std::string_view(argv[1]) == "--frame") {
      const auto k = number<unsigned>(argv[3]), s = number<unsigned>(argv[4]);
      auto points = load_frame(argv[2]);
      const auto input_ms = elapsed(start);
      run_measure(std::move(points), k, s, min_factor, "frame", "none", start, input_ms);
      return 0;
    }
    throw std::invalid_argument("factor_plan.usage_gate_or_mutant_NAME_or_synthetic_KIND_N_K_S_or_frame_PATH_K_S");
  } catch (const std::exception& e) {
    std::cout << "{\"schema\":\"" << schema << "\",\"status\":\"failed\",\"cause\":" << quoted(e.what()) << "}\n";
    return 1;
  }
}
