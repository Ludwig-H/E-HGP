// Banc CPU MEB bornee puis census, 48 parties choisies. Pas une descente de la tour FULL.
#include <iostream>

#include "meb_checks.hpp"

using namespace mhgp11;
using namespace index_bench;
using namespace meb_bench;

namespace {
constexpr u32 kQueries = 48;

std::array<SiteIdx, 12> part(u32 ordinal, u32 count) {
  const u32 group = ordinal / 12, size = 1 + ordinal % 12;
  const u64 start = (u64{group} * count / 4 + u64{size - 1} * (count / 48 + 1)) % count;
  const u32 step = group % 2 == 0 ? 1 : count / 12;
  std::array<SiteIdx, 12> result{};
  for (u32 i = 0; i < size; ++i) result[i] = make_id<SiteIdx>(static_cast<u32>((start + u64{i} * step) % count));
  std::sort(result.begin(), result.begin() + size);
  return result;
}

template <class T>
void integer(std::ostream& out, const T& value) {
  const auto wide = num::to_wide(value);
  word(out, wide.neg ? 1 : 0); word(out, wide.words.size());
  for (u64 w : wide.words) word(out, w);
}

void serialize(std::ostream& out, std::span<const SiteIdx> selected, u32 threshold,
               const BoundedMeb& meb, const Census& population) {
  word(out, selected.size()); word(out, threshold);
  for (std::size_t i = 0; i < 12; ++i) word(out, i < selected.size() ? idx(selected[i]) : kNone);
  word(out, meb.support().size());
  for (std::size_t i = 0; i < 4; ++i) word(out, i < meb.support().size() ? idx(meb.support()[i]) : kNone);
  const auto& sphere = meb.sphere();
  const auto anchor = sphere.anchor();  // La vue coordinates() doit survivre a toute la boucle C++20.
  for (u32 c : anchor.coordinates()) word(out, c);
  for (auto n : sphere.numerator()) integer(out, n);
  integer(out, sphere.denominator());
  integer(out, sphere.level().numerator()); integer(out, sphere.level().denominator());
  word(out, population.kind() == CensusKind::saturated ? 1 : 0);
  word(out, population.interior().size()); word(out, population.shell().size());
  for (SiteIdx s : population.interior()) word(out, idx(s));
  for (SiteIdx s : population.shell()) word(out, idx(s));
}

void status(const Outcome& outcome) {
  std::cout << "\"status\":\"" << status_name(outcome.status()) << "\",\"reason\":\""
            << reason_name(outcome.reason) << '"';
}

Outcome refused(u32 ordinal, const char* stage, const Outcome& outcome) {
  std::cout << "{\"phase\":\"query\",\"ordinal\":" << ordinal << ",\"stage\":\"" << stage << "\",";
  status(outcome); std::cout << "}\n" << std::flush;
  return outcome;
}

struct Totals {
  u64 complete = 0, saturated = 0, meb_ns = 0, census_ns = 0, wrapper_ns = 0, reference_ns = 0;
  std::array<u64, 4> supports{};
};

Outcome query(const GlobalIndex& index, u32 ordinal, std::ostream& output, MemoryBudget& budget, Totals& totals) {
  const auto selected = part(ordinal, index.cloud().sites());
  const std::span<const SiteIdx> points(selected.data(), 1 + ordinal % 12);
  const u32 threshold = std::array<u32, 3>{5, 10, 13}[ordinal % 3];
  budget.restart_peak();
  Stopwatch meb_clock;
  auto meb = bounded_meb(index.cloud(), points);
  const u64 meb_ns = meb_clock.nanoseconds(), meb_peak = budget.peak(), meb_after = budget.used();
  if (!meb.ok()) return refused(ordinal, "meb", meb.outcome());
  budget.restart_peak();
  Stopwatch census_clock;
  auto population = census(index, meb.value().sphere(), threshold, budget);
  const u64 census_ns = census_clock.nanoseconds(), census_peak = budget.peak(), census_after = budget.used();
  if (!population.ok()) return refused(ordinal, "census", population.outcome());
  budget.restart_peak();
  Stopwatch wrapper_clock;
  auto combined = meb_census(index, points, threshold, budget);
  const u64 wrapper_ns = wrapper_clock.nanoseconds(), wrapper_peak = budget.peak(), wrapper_after = budget.used();
  if (!combined.ok()) return refused(ordinal, "wrapper", combined.outcome());
  Stopwatch reference_clock;
  const auto support_outcome = support_check(index.cloud(), points, meb.value());
  const auto scan_outcome = scan(index.cloud(), meb.value().sphere(), threshold, population.value());
  const bool wrapper_ok = same_meb(meb.value(), combined.value().meb()) &&
                          same_census(population.value(), combined.value().population());
  const u64 reference_ns = reference_clock.nanoseconds();
  const auto& m = meb.value().ledger(); const auto& c = population.value().ledger();
  const bool saturated = population.value().kind() == CensusKind::saturated;
  const bool reference_ok = support_outcome.ok() && scan_outcome.ok() && wrapper_ok;
  std::cout << "{\"phase\":\"query\",\"ordinal\":" << ordinal << ",\"size\":" << points.size()
            << ",\"meb_search\":\"first_strict_containing_v1\""
            << ",\"threshold\":" << threshold << ",\"status\":\"ok\",\"reason\":\"none\",\"support_size\":"
            << meb.value().support().size() << ",\"meb_ns\":" << meb_ns << ",\"census_ns\":" << census_ns
            << ",\"wrapper_ns\":" << wrapper_ns << ",\"reference_ns\":" << reference_ns
            << ",\"reference_ok\":" << (reference_ok ? "true" : "false")
            << ",\"meb_peak_bytes\":" << meb_peak << ",\"meb_after_bytes\":" << meb_after
            << ",\"census_peak_bytes\":" << census_peak << ",\"census_after_bytes\":" << census_after
            << ",\"wrapper_peak_bytes\":" << wrapper_peak << ",\"wrapper_after_bytes\":" << wrapper_after
            << ",\"kind\":\"" << (saturated ? "saturated" : "complete") << "\",\"interior\":"
            << population.value().interior().size() << ",\"shell\":" << population.value().shell().size()
            << ",\"meb_logical\":{\"presentations\":" << m.presentations << ",\"nondegenerate\":" << m.nondegenerate
            << ",\"positive\":" << m.positive << ",\"containing\":" << m.containing
            << ",\"comparisons\":" << m.comparisons << ",\"point_tests\":" << m.point_tests
            << "},\"census_logical\":{\"nodes\":" << c.nodes << ",\"bounds\":" << c.bounds
            << ",\"point_tests\":" << c.point_tests << ",\"inside_blocks\":" << c.inside_blocks
            << ",\"outside_blocks\":" << c.outside_blocks << ",\"passes\":" << c.passes << "}}\n" << std::flush;
  if (!reference_ok) return fail(Reason::arithmetic_invariant);
  serialize(output, points, threshold, meb.value(), population.value());
  totals.complete += !saturated; totals.saturated += saturated;
  ++totals.supports[meb.value().support().size() - 1];
  totals.meb_ns += meb_ns; totals.census_ns += census_ns;
  totals.wrapper_ns += wrapper_ns; totals.reference_ns += reference_ns;
  return output ? Outcome{} : fail(Reason::output_unwritable);
}

Outcome run(char** argv, u64 bytes) {
  MemoryBudget budget(bytes);
  Stopwatch read_clock;
  auto input = read_input(argv[1], argv[2], budget);
  const u64 read_ns = read_clock.nanoseconds();
  if (!input.ok()) return input.outcome();
  Stopwatch cloud_clock;
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                            input.value().ids.span(), CoordWidth(), budget);
  const u64 cloud_ns = cloud_clock.nanoseconds();
  if (!cloud.ok()) return cloud.outcome();
  if (cloud.value().sites() < 12 || cloud.value().sites() != cloud.value().weight())
    return fail(Reason::parameter_out_of_range);
  input.value() = {};
  std::cout << "{\"phase\":\"cloud\",\"read_ns\":" << read_ns << ",\"cloud_ns\":" << cloud_ns
            << ",\"sites\":" << cloud.value().sites() << ",\"points\":" << cloud.value().weight()
            << ",\"peak_reserved_bytes\":" << budget.peak() << ",\"reserved_after_bytes\":" << budget.used()
            << "}\n" << std::flush;
  budget.restart_peak();
  Stopwatch index_clock;
  auto index = build_index(std::move(cloud.value()), IndexParams{8}, budget);
  const u64 index_ns = index_clock.nanoseconds();
  std::cout << "{\"phase\":\"index\","; status(index.outcome());
  std::cout << ",\"coord_bits\":" << kCoordBits << ",\"wall_ns\":" << index_ns
            << ",\"peak_reserved_bytes\":" << budget.peak() << ",\"reserved_after_bytes\":" << budget.used();
  if (!index.ok()) { std::cout << "}\n" << std::flush; return index.outcome(); }
  std::cout << ",\"nodes\":" << index.value().nodes() << ",\"max_depth\":" << index.value().max_depth()
            << ",\"leaf_size\":" << index.value().leaf_size() << ",\"node_bytes\":" << sizeof(index_detail::Node)
            << "}\n" << std::flush;
  std::ofstream output(argv[3], std::ios::binary | std::ios::trunc);
  if (!output) return fail(Reason::output_unwritable);
  output.write("MHGP11MEB1", 10);
  word(output, kCoordBits); word(output, index.value().cloud().sites()); word(output, kQueries);
  Totals total;
  for (u32 ordinal = 0; ordinal < kQueries; ++ordinal) MHGP11_TRY(query(index.value(), ordinal, output, budget, total));
  std::cout << "{\"phase\":\"summary\",\"queries\":48,\"complete\":" << total.complete
            << ",\"saturated\":" << total.saturated << ",\"meb_ns\":" << total.meb_ns
            << ",\"census_ns\":" << total.census_ns << ",\"wrapper_ns\":" << total.wrapper_ns
            << ",\"reference_ns\":" << total.reference_ns << ",\"reserved_after_bytes\":" << budget.used()
            << ",\"support_sizes\":[";
  for (std::size_t i = 0; i < 4; ++i) std::cout << (i == 0 ? "" : ",") << total.supports[i];
  std::cout << "]}\n" << std::flush;
  output.close();
  return output ? Outcome{} : fail(Reason::output_unwritable);
}
}  // namespace

int main(int argc, char** argv) {
  u64 bytes = 0;
  if (argc != 5 || !parse(argv[4], bytes)) return 2;
  const Outcome outcome = guarded([&]() { return run(argv, bytes); });
  std::cout << "{\"phase\":\"exit\","; status(outcome); std::cout << "}\n";
  return exit_code(outcome);
}
