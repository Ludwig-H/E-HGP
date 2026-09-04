// Audit courant. Les corruptions ci-dessous ne touchent que des copies du
// payload; aucune implementation produit n'est modifiee.
#ifndef MHGP7_AUDIT_GATE_SOURCE
#define MHGP7_AUDIT_GATE_SOURCE "../tests/silent_incidence_gate.cpp"
#endif
#define main mhgp7_followup_original_gate_main
#include MHGP7_AUDIT_GATE_SOURCE
#undef main

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  if (std::strcmp(argv[1], "--gate-control") == 0)
    return mhgp7_followup_original_gate_main(1, argv);
  if (std::strncmp(argv[1], "--mutant=", 9) == 0)
    return mhgp7_followup_original_gate_main(argc, argv);
  mhgp7_oracle::clear_overflow();
  if (std::strcmp(argv[1], "--triangle") == 0) {
    const std::vector<P3> pts{{0, 0, 0}, {6, 0, 0}, {2, 3, 0}};
    const Oracle oracle(pts);
    const auto events = product_direct(build_cloud_index(pts), 2);
    const auto forest = build_forest(events, 1, ForestLayout::kClassic, true);
    if (forest.delta_count() != 1) return 1;
    const auto first = forest.delta(0);
    const ExactLevel expected_level{{325, 0, 0}, 36};
    const bool ok = oracle.regular && forest.normalized_reduced && forest.refusal.empty() &&
        first.parents.empty() && first.born.size() == 3 && forest.nodes == 0 &&
        compare(first.level, oracle.ball(7)) == 0 && compare(expected_level, oracle.ball(7)) == 0 &&
        compare_normalized(oracle, 2, forest) && compare_delta_cuts(oracle, 2, forest) &&
        !mhgp7_oracle::overflow_seen() && failures == 0;
    std::printf("triangle normalized=%d parents=%zu born=%zu nodes=%llu level=325/36\n",
                ok ? 1 : 0, first.parents.size(), first.born.size(), (unsigned long long)forest.nodes);
    return ok ? 0 : 1;
  }
  if (std::strcmp(argv[1], "--erase-born") != 0 &&
      std::strcmp(argv[1], "--drop-silent-continuation") != 0 &&
      std::strcmp(argv[1], "--replace-output") != 0) return 2;
  const auto pts = e5();
  const Oracle oracle(pts);
  const auto ix = build_cloud_index(pts);
  auto events = product_direct(ix, 2);
  const auto added = build_silent_cofaces(ix, events);
  if (added.status != SilentIncidenceStatus::kComplete) return 1;
  events.insert(events.end(), added.events.begin(), added.events.end());
  auto forest = build_forest(events, 1, ForestLayout::kClassic, true);
  if (!forest.normalized_reduced || !forest.refusal.empty() ||
      !compare_normalized(oracle, 2, forest) || !compare_delta_cuts(oracle, 2, forest) ||
      forest.delta_count() == 0) return 1;
  const auto unresolved_parents = [](const ForestResult& value) {
    // Lecteur symbolique des SEULS deltas, sans coface ni DSU de Gamma.
    // Une racine publiee est remplacee par output apres consommation des
    // jetons parents. Un parent jamais publie ne peut etre resolu.
    std::set<unsigned> tokens;
    size_t unresolved = 0;
    for (size_t i = 0; i < value.delta_count(); ++i) {
      const auto delta = value.delta(i);
      for (const FacetKey& parent : delta.parents)
        if (tokens.erase(facet_mask(parent)) != 1) ++unresolved;
      tokens.insert(facet_mask(delta.output));
    }
    return unresolved;
  };
  if (unresolved_parents(forest) != 0) return 1;
  size_t born_before = 0;
  for (const auto& delta : forest.deltas) born_before += delta.born.size();
  const size_t deltas_before = forest.delta_count();
  size_t removed = 0;
  if (std::strcmp(argv[1], "--erase-born") == 0) {
    for (auto& delta : forest.deltas) {
      removed += delta.born.size();
      delta.born.clear();
    }
    // Le compteur ne doit pas trahir artificiellement la copie corrompue.
    forest.keys_born = 0;
  } else if (std::strcmp(argv[1], "--drop-silent-continuation") == 0) {
    const ExactLevel silent_level{{33, 0, 0}, 2};
    forest.deltas.erase(std::remove_if(forest.deltas.begin(), forest.deltas.end(), [&](const auto& delta) {
      if (!same_exact_level(delta.level, silent_level)) return false;
      if (delta.parents.size() != 1 || delta.born.size() != 1) return false;
      const unsigned ac = (1u << 0) | (1u << 2);
      if (facet_mask(delta.born[0]) != ac) return false;
      ++removed;
      return true;
    }), forest.deltas.end());
    if (removed == 1) { --forest.keys_parents; --forest.keys_born; }
  } else {
    auto& first = forest.deltas.front();
    for (const FacetKey& facet : first.born) {
      if (facet_mask(facet) == facet_mask(first.output)) continue;
      first.output = facet;
      ++removed;
      break;
    }
  }
  size_t born_after = 0;
  for (const auto& delta : forest.deltas) born_after += delta.born.size();
  const bool accepted = compare_normalized(oracle, 2, forest) && compare_delta_cuts(oracle, 2, forest);
  const size_t unresolved = unresolved_parents(forest);
  std::printf("E5 mode=%s removed=%zu deltas_before=%zu deltas_after=%zu born_before=%zu born_after=%zu judge_accepts=%d unresolved_parents=%zu\n",
              argv[1], removed, deltas_before, forest.delta_count(), born_before, born_after, accepted ? 1 : 0, unresolved);
  if (mhgp7_oracle::overflow_seen() || failures != 0 || removed == 0) return 1;
  if (std::strcmp(argv[1], "--replace-output") == 0)
    return !accepted && removed == 1 && forest.delta_count() == deltas_before &&
                   born_after == born_before && unresolved == 1 ? 4 : 1;
  if (accepted) return 1;
  if (std::strcmp(argv[1], "--erase-born") == 0)
    return born_before == removed && born_after == 0 && forest.delta_count() == deltas_before ? 4 : 1;
  return removed == 1 && forest.delta_count() + 1 == deltas_before && born_after + 1 == born_before &&
                 unresolved == 1 ? 4 : 1;
}
