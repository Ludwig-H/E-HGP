// Bounded audit transport. The independent judge consumes only deltas.
#include <iostream>
#include <sstream>
#include <string>

#include "../src/pipeline/run.hpp"

using namespace mhgp7;

static void integer(std::ostream& out, i128 value) {
  if (value < 0) { out << '-'; value = -value; }
  std::string digits;
  do { digits += static_cast<char>('0' + value % 10); value /= 10; } while (value);
  for (auto it = digits.rbegin(); it != digits.rend(); ++it) out << *it;
}

static void quoted(std::ostream& out, const std::string& text) {
  out << '"';
  for (char ch : text) {
    if (ch == '"' || ch == '\\') out << '\\';
    if (ch == '\n') out << "\\n";
    else if (ch == '\r') out << "\\r";
    else if (ch == '\t') out << "\\t";
    else out << ch;
  }
  out << '"';
}

static void facet(std::ostream& out, const FacetKey& key) {
  out << '[';
  for (size_t i = 0; i < key.k; ++i) out << (i ? "," : "") << key.p[i];
  out << ']';
}

static const char* status_name(PipelineStatus status) {
  switch (status) {
    case PipelineStatus::kCompleteRegular: return "complete_regular";
    case PipelineStatus::kUnsupportedDegeneracy: return "unsupported_degeneracy";
    case PipelineStatus::kResourceExhausted: return "resource_exhausted";
    case PipelineStatus::kInvalidInput: return "invalid_input";
    case PipelineStatus::kInvariantViolated: return "invariant_violated";
  }
  throw std::runtime_error("unknown status");
}

int main(int argc, char** argv) {
  try {
    if (argc > 2 || (argc == 2 && !mutants_enable(argv[1]))) return 2;
    size_t n;
    while (std::cin >> n) {
      int threads, layout, reverse, complete;
      i64 separation;
      std::cin >> threads >> layout >> reverse >> complete >> separation;
      if (n < 2 || n > 8) return 2;
      std::vector<InputPoint> input(n);
      for (auto& point : input)
        std::cin >> point.id >> point.position.x >> point.position.y >> point.position.z;
      if (!std::cin) return 2;
      if (reverse) std::reverse(input.begin(), input.end());
      RunOptions options;
      options.s = separation;
      options.threads = threads;
      options.fold_inflight = 1;
      options.fold_join_before_next_k = true;
      options.complete_silent_incidence = complete != 0;
      options.digest = true;
      options.forest_layout = layout ? ForestLayout::kCsr : ForestLayout::kClassic;
      std::vector<std::string> forests;
      options.on_forest = [&](u64 K, const auto&, const ForestResult& forest) {
        std::ostringstream out;
        out << "{\"K\":" << K << ",\"normalized\":" << forest.normalized_reduced
            << ",\"deltas\":[";
        for (size_t j = 0; j < forest.delta_count(); ++j) {
          const auto delta = forest.delta(j);
          out << (j ? "," : "") << "{\"batch\":" << delta.batch << ",\"num\":[";
          for (int k = 0; k < 3; ++k) out << (k ? "," : "") << delta.level.num[k];
          out << "],\"den\":"; integer(out, delta.level.den);
          out << ",\"output\":"; facet(out, delta.output);
          out << ",\"parents\":[";
          size_t ordinal = 0;
          for (const auto& key : delta.parents) { if (ordinal++) out << ','; facet(out, key); }
          out << "],\"born\":[";
          ordinal = 0;
          for (const auto& key : delta.born) { if (ordinal++) out << ','; facet(out, key); }
          out << "]}";
        }
        out << "]}";
        forests.push_back(out.str());
      };
      const auto run = run_pipeline(input, options);
      std::cout << "{\"status\":"; quoted(std::cout, status_name(run.status));
      std::cout << ",\"message\":"; quoted(std::cout, run.message);
      std::cout << ",\"kmax\":" << run.kmax_eff << ",\"digest\":";
      quoted(std::cout, run.digest_all);
      std::cout << ",\"silent_added\":[";
      for (size_t k = 0; k < run.silent_stats.size(); ++k)
        std::cout << (k ? "," : "") << run.silent_stats[k].added_cofaces;
      std::cout << "],\"forests\":[";
      for (size_t k = 0; k < forests.size(); ++k) std::cout << (k ? "," : "") << forests[k];
      std::cout << "]}\n";
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 2;
  }
}
