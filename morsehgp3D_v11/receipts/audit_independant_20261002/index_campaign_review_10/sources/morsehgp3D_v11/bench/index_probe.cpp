// Banc CPU index/census : 64 requetes q1..4 sur chaque entree entiere, aucun catalogue ni FULL.
// Le scan global est independant du parcours d'index, mais reutilise les predicats num qualifies.
#include <iostream>
#include <optional>

#include "index_io.hpp"

using namespace mhgp11;
using namespace index_bench;

namespace {
constexpr u32 kQueries = 64;

num::Point point(const Cloud& cloud, u32 s) {
  return num::Point::make(cloud.x()[s], cloud.y()[s], cloud.z()[s]).value();
}

std::array<u32, 4> support(u32 ordinal, u32 count) {
  const u32 group = ordinal / 4;
  const u64 start = u64{group} * count / 16;
  const u32 step = group % 2 == 0 ? 1 : count / 4;
  std::array<u32, 4> out{};
  for (u32 i = 0; i < 4; ++i) out[i] = static_cast<u32>((start + u64{i} * step) % count);
  return out;
}

Result<std::optional<num::Sphere>> sphere(const Cloud& cloud, const std::array<u32, 4>& s, u32 q) {
  const auto a = point(cloud, s[0]);
  if (q == 1) return std::optional<num::Sphere>(num::Sphere::point(a));
  const auto b = point(cloud, s[1]);
  if (q == 2) return num::Sphere::through(a, b);
  const auto c = point(cloud, s[2]);
  if (q == 3) return num::Sphere::through(a, b, c);
  return num::Sphere::through(a, b, c, point(cloud, s[3]));
}

bool ordered(std::span<const SiteIdx> sites, u32 size) {
  for (std::size_t i = 0; i < sites.size(); ++i)
    if (idx(sites[i]) >= size || (i > 0 && idx(sites[i - 1]) >= idx(sites[i]))) return false;
  return true;
}

Outcome scan(const Cloud& cloud, const num::Sphere& query, u32 threshold, const Census& result) {
  const auto inner = result.interior(), shell = result.shell();
  if (!ordered(inner, cloud.sites()) || !ordered(shell, cloud.sites())) return fail(Reason::arithmetic_invariant);
  const bool saturated = result.kind() == CensusKind::saturated;
  u64 pi = 0, pu = 0, witness = 0;
  for (u32 s = 0; s < cloud.sites(); ++s) {
    auto side = num::side(query, point(cloud, s));
    if (!side.ok()) return side.outcome();
    if (side.value() < 0) {
      if (!saturated && (pi >= inner.size() || idx(inner[pi]) != s)) return fail(Reason::arithmetic_invariant);
      ++pi;
      witness += std::binary_search(inner.begin(), inner.end(), make_id<SiteIdx>(s));
    } else if (side.value() == 0) {
      if (!saturated && (pu >= shell.size() || idx(shell[pu]) != s)) return fail(Reason::arithmetic_invariant);
      ++pu;
    }
  }
  const bool valid = saturated ? pi >= threshold && inner.size() == threshold && shell.empty() && witness == threshold
                               : pi < threshold && pi == inner.size() && pu == shell.size();
  return valid ? Outcome{} : fail(Reason::arithmetic_invariant);
}

void status(const Outcome& outcome) {
  std::cout << "\"status\":\"" << status_name(outcome.status()) << "\",\"reason\":\""
            << reason_name(outcome.reason) << '"';
}

struct Totals {
  u64 queries = 0, complete = 0, saturated = 0, degenerate = 0, query_ns = 0, reference_ns = 0;
  std::array<u32, 4> arities{};
};

Outcome queries(const GlobalIndex& index, std::ostream& out, MemoryBudget& budget) {
  const Cloud& cloud = index.cloud();
  Totals totals;
  for (u32 ordinal = 0; ordinal < kQueries; ++ordinal) {
    const u32 q = 1 + ordinal % 4, threshold = std::array<u32, 3>{5, 10, 13}[ordinal % 3];
    const auto s = support(ordinal, cloud.sites());
    word(out, q); word(out, threshold);
    for (u32 i = 0; i < 4; ++i) word(out, i < q ? s[i] : kNone);
    Stopwatch factory_clock;
    auto query = sphere(cloud, s, q);
    const u64 factory_ns = factory_clock.nanoseconds();
    if (!query.ok()) return query.outcome();
    word(out, query.value().has_value() ? 0 : 1);
    if (!query.value()) {
      ++totals.degenerate;
      std::cout << "{\"phase\":\"query\",\"ordinal\":" << ordinal << ",\"arity\":" << q
                << ",\"threshold\":" << threshold << ",\"status\":\"degenerate\",\"factory_ns\":"
                << factory_ns << "}\n" << std::flush;
      continue;
    }
    budget.restart_peak();
    Stopwatch timer;
    auto found = census(index, *query.value(), threshold, budget);
    const u64 wall_ns = timer.nanoseconds();
    std::cout << "{\"phase\":\"query\",\"ordinal\":" << ordinal << ",\"arity\":" << q
              << ",\"threshold\":" << threshold << ",\"factory_ns\":" << factory_ns << ',';
    status(found.outcome());
    std::cout << ",\"wall_ns\":" << wall_ns << ",\"peak_reserved_bytes\":" << budget.peak()
              << ",\"reserved_after_bytes\":" << budget.used();
    if (!found.ok()) { std::cout << "}\n" << std::flush; return found.outcome(); }
    const auto& result = found.value(); const auto& ledger = result.ledger();
    const bool saturated = result.kind() == CensusKind::saturated;
    Stopwatch ref_clock;
    const Outcome reference = scan(cloud, *query.value(), threshold, result);
    const u64 ref_ns = ref_clock.nanoseconds();
    std::cout << ",\"reference_ns\":" << ref_ns << ",\"reference_ok\":" << (reference.ok() ? "true" : "false")
              << ",\"kind\":\"" << (saturated ? "saturated" : "complete") << "\",\"interior\":"
              << result.interior().size() << ",\"shell\":" << result.shell().size()
              << ",\"logical\":{\"nodes\":" << ledger.nodes << ",\"bounds\":" << ledger.bounds
              << ",\"point_tests\":" << ledger.point_tests << ",\"inside_blocks\":" << ledger.inside_blocks
              << ",\"outside_blocks\":" << ledger.outside_blocks << ",\"passes\":" << ledger.passes << "}}\n"
              << std::flush;
    MHGP11_TRY(reference);
    word(out, saturated ? 1 : 0); word(out, result.interior().size()); word(out, result.shell().size());
    for (SiteIdx id : result.interior()) word(out, idx(id));
    for (SiteIdx id : result.shell()) word(out, idx(id));
    ++totals.queries; ++totals.arities[q - 1];
    totals.saturated += saturated; totals.complete += !saturated;
    totals.query_ns += wall_ns; totals.reference_ns += ref_ns;
  }
  std::cout << "{\"phase\":\"summary\",\"queries\":" << totals.queries << ",\"complete\":" << totals.complete
            << ",\"saturated\":" << totals.saturated << ",\"degenerate\":" << totals.degenerate
            << ",\"query_ns\":" << totals.query_ns << ",\"reference_ns\":" << totals.reference_ns
            << ",\"reserved_after_bytes\":" << budget.used() << ",\"arities\":[";
  for (u32 i = 0; i < 4; ++i) std::cout << (i == 0 ? "" : ",") << totals.arities[i];
  std::cout << "]}\n" << std::flush;
  return out ? Outcome{} : fail(Reason::output_unwritable);
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
  if (cloud.value().sites() < 4 || cloud.value().sites() != cloud.value().weight())
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
  output.write("MHGP11IDX1", 10);
  word(output, kCoordBits); word(output, index.value().cloud().sites()); word(output, kQueries);
  MHGP11_TRY(queries(index.value(), output, budget));
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
