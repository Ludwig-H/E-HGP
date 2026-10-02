// Encodage JSON partage des diagnostics catalogue ; aucun objet natif ni padding dans le format de preuve.
#pragma once
#include <ostream>
#include "catalogue/catalogue.hpp"

namespace mhgp11::bench {

inline void catalogue_ledger_json(std::ostream& out, const CatalogueLedger& l) {
  out << "{\"nodes\":" << l.nodes << ",\"leaves\":" << l.leaves << ",\"filter_tests\":" << l.filter_tests
      << ",\"dominance_tests\":" << l.dominance_tests << ",\"prefixes\":" << l.prefixes
      << ",\"judged\":" << l.judged << ",\"census_tests\":" << l.census_tests
      << ",\"emitted\":" << l.emitted << ",\"incidences\":" << l.incidences
      << ",\"q4_candidates\":" << l.q4_candidates << ",\"q4_levels\":" << l.q4_levels
      << ",\"region_pair_tests\":" << l.region_pair_tests << ",\"region_pair_rejects\":" << l.region_pair_rejects
      << ",\"region_line_tests\":" << l.region_line_tests << ",\"region_line_rejects\":" << l.region_line_rejects
      << ",\"region_line_evaluations\":" << l.region_line_evaluations
      << ",\"region_line_cache_hits\":" << l.region_line_cache_hits
      << ",\"region_line_fallbacks\":" << l.region_line_fallbacks
      << ",\"max_leaf\":" << l.max_leaf << ",\"max_depth\":" << l.max_depth << '}';
}

inline void catalogue_diagnostics_json(std::ostream& out, const CatalogueDiagnostics& value) {
  const auto& p = value.planning();
  out << "{\"schema\":\"ehgp.v11.catalogue_diagnostics.v1\",\"record_bytes\":" << sizeof(CatalogueTaskDiagnostic)
      << ",\"reserved_bytes\":" << value.tasks().size() * sizeof(CatalogueTaskDiagnostic)
      << ",\"planning\":{\"adaptive\":" << (p.adaptive ? "true" : "false")
      << ",\"memory_fallback\":" << (p.memory_fallback ? "true" : "false")
      << ",\"plan_nodes\":" << p.plan_nodes << ",\"plan_leaves\":" << p.plan_leaves
      << ",\"empty_leaves\":" << p.empty_leaves << ",\"rounds\":" << p.rounds
      << ",\"priority_tests\":" << p.priority_tests << ",\"replay_bytes\":" << p.replay_bytes << "},\"tasks\":[";
  u32 ordinal = 0;
  for (const auto& t : value.tasks()) {
    if (ordinal != 0) out << ',';
    out << "{\"ordinal\":" << ordinal++ << ",\"path\":[" << t.path[0] << ',' << t.path[1]
        << "],\"path_known\":" << (t.path_known ? "true" : "false")
        << ",\"inside_known\":" << (t.inside_known ? "true" : "false")
        << ",\"lo\":[" << t.lo[0] << ',' << t.lo[1] << ',' << t.lo[2]
        << "],\"hi\":[" << t.hi[0] << ',' << t.hi[1] << ',' << t.hi[2]
        << "],\"depth\":" << t.depth << ",\"count\":" << t.count << ",\"capacity\":" << t.capacity
        << ",\"inside\":" << t.inside << ",\"count_ns\":" << t.count_ns << ",\"fill_ns\":" << t.fill_ns
        << ",\"ledger\":";
    catalogue_ledger_json(out,t.ledger); out << '}';
  }
  out << "]}";
}

}  // namespace mhgp11::bench
