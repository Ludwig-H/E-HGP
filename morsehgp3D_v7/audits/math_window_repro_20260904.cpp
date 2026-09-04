// Audit borne : supports OBig exhaustifs, fenetre de rang, puis payload.
// Aucune decision geometrique produit ne sert a construire la reference.
#ifndef MHGP7_AUDIT_GATE_SOURCE
#define MHGP7_AUDIT_GATE_SOURCE "../tests/silent_incidence_gate.cpp"
#endif
#define main mhgp7_window_original_gate_main
#include MHGP7_AUDIT_GATE_SOURCE
#undef main

#include <cstdlib>

namespace {
struct SupportCensus {
  OracleBall ball;
  unsigned inside = 0, shell = 0;
  int q = 0;
};

struct WindowOracle {
  std::vector<P3> pts;
  std::vector<SupportCensus> supports;
  std::map<unsigned, size_t> unique;
  explicit WindowOracle(const std::vector<P3>& input) : pts(input) {
    check(pts.size() <= 24, "oracle masques bornes a 24 points");
    for (unsigned a = 0; a < pts.size(); ++a)
      for (unsigned b = a + 1; b < pts.size(); ++b) {
        add((1u << a) | (1u << b));
        for (unsigned c = b + 1; c < pts.size(); ++c) {
          add((1u << a) | (1u << b) | (1u << c));
          for (unsigned d = c + 1; d < pts.size(); ++d)
            add((1u << a) | (1u << b) | (1u << c) | (1u << d));
        }
      }
  }
  void add(unsigned mask) {
    SupportCensus s;
    if (!support_ball(pts, mask, &s.ball)) return;
    s.q = std::popcount(mask);
    for (unsigned i = 0; i < pts.size(); ++i) {
      const int sign = s.ball.power(pts[i]).sign();
      if (sign < 0) s.inside |= 1u << i;
      if (sign == 0) s.shell |= 1u << i;
    }
    const size_t index = supports.size();
    supports.push_back(s);
    const auto it = unique.find(s.shell);
    if (it == unique.end()) unique.emplace(s.shell, index);
    else {
      // Deux supports positifs sur une meme coquille donnent son unique MEB.
      const auto& prior = supports[it->second];
      check(prior.inside == s.inside && compare(prior.ball, s.ball) == 0,
            "unicite MEB de la coquille commune");
      if (s.q < prior.q) it->second = index;
    }
  }
  Oracle gamma() const {
    Oracle result({});
    result.pts = pts;
    for (const auto& s : supports) result.supports.push_back(s.ball);
    result.meb.assign(1u << pts.size(), supports.size());
    for (unsigned mask = 1; mask < result.meb.size(); ++mask) {
      if (std::popcount(mask) < 2) continue;
      for (size_t i = 0; i < supports.size(); ++i) {
        const auto& s = supports[i];
        if ((s.ball.support & mask) == s.ball.support &&
            (mask & ~(s.inside | s.shell)) == 0) {
          result.meb[mask] = i;
          break;
        }
      }
      check(result.meb[mask] != supports.size(), "MEB exhaustive totale par census OBig");
    }
    return result;
  }
};

unsigned ids_mask(const CloudIndex& ix, std::span<const i32> ids) {
  unsigned result = 0;
  for (i32 id : ids) result |= 1u << ix.point_id(id);
  return result;
}

std::vector<P3> random_points(size_t n, u64 seed) {
  std::vector<P3> result;
  u64 state = seed;
  for (size_t i = 0; i < n; ++i) {
    P3 p{};
    i64* coords[] = {&p.x, &p.y, &p.z};
    for (i64* c : coords) {
      state ^= state << 13; state ^= state >> 7; state ^= state << 17;
      *c = (i64)(state & 65535u);
    }
    result.push_back(p);
  }
  return result;
}

std::vector<P3> boundary_points(bool relevant) {
  std::vector<P3> result = {{0, 1000, 1000}, {2000, 1000, 1000}, {1000, 2000, 1000}};
  const auto random = random_points(relevant ? 9 : 10, 78234729);
  for (const auto& p : random)
    result.push_back({800 + p.x % 401, 800 + p.y % 401, 800 + p.z % 401});
  return result;
}

struct CaseStats {
  size_t support_count = 0, unique = 0, relevant = 0, omitted = 0;
  size_t boundary[3] = {}, beyond[3] = {};
  size_t plateau_relevant = 0, plateau_omitted = 0;
  size_t direct = 0, callbacks = 0, silent = 0;
  size_t strict_core_contacts = 0;
};

CaseStats run_case(const std::vector<P3>& pts, u64 smax, bool judge_gamma, bool reverse,
                   bool expect_degenerate, bool mutant) {
  WindowOracle oracle(pts);
  CaseStats count;
  count.support_count = oracle.supports.size(); count.unique = oracle.unique.size();
  std::map<unsigned, size_t> expected;
  for (const auto& [shell, id] : oracle.unique) {
    const auto& s = oracle.supports[id];
    const int rank = std::popcount(s.inside) + s.q;
    const bool plateau = std::popcount(s.shell) != s.q;
    if ((u64)rank <= smax) {
      expected.emplace(shell, id); ++count.relevant;
      if ((u64)rank == smax) ++count.boundary[s.q - 2];
      if (plateau) ++count.plateau_relevant;
    } else {
      ++count.omitted; ++count.beyond[s.q - 2];
      if (plateau) ++count.plateau_omitted;
    }
  }
  std::vector<InputPoint> input;
  for (unsigned i = 0; i < pts.size(); ++i) input.push_back({i, pts[i]});
  if (reverse) std::reverse(input.begin(), input.end());
  const auto ix = build_cloud_index(input);
  GenerateOptions opt; opt.smax = smax; opt.threads = reverse ? 2 : 1;
  GenerateStats gs;
  std::vector<BallCandidate> candidates;
  generate_candidates(ix, opt, &candidates, &gs);
  check(gs.cap_refus == kCapRefusNone, "generation complete sans cap");
  sort_candidates(&candidates, opt.threads); deduplicate_candidates(&candidates);
  ExpandStats es;
  std::vector<Survivor> survivors;
  prefilter_balls(ix, candidates, smax, opt.threads, &survivors, &es);
  std::vector<BallData> balls;
  const auto census = census_balls(ix, candidates, survivors, smax, 12, opt.threads, &balls, &es);
  check(census == PipelineStatus::kCompleteRegular, "census source termine");
  std::set<unsigned> found;
  bool source_ok = true;
  for (const auto& ball : balls) {
    const unsigned shell = ids_mask(ix, ball.shell());
    const auto it = expected.find(shell);
    if (it == expected.end() || !found.insert(shell).second) { source_ok = false; continue; }
    const auto& ref = oracle.supports[it->second];
    if (ids_mask(ix, ball.interior()) != ref.inside || ball.arity != ref.q ||
        compare(ball.level, ref.ball) != 0) source_ok = false;
  }
  const bool source_contents_ok = source_ok;
  source_ok = source_ok && found.size() == expected.size();
  std::printf("window n=%zu smax=%llu reverse=%d supports=%zu unique=%zu relevant=%zu omitted=%zu boundary_q2=%zu boundary_q3=%zu boundary_q4=%zu beyond_q2=%zu beyond_q3=%zu beyond_q4=%zu plateau_relevant=%zu plateau_omitted=%zu actual=%zu source_ok=%d\n",
              pts.size(), (unsigned long long)smax, reverse, count.support_count, count.unique,
              count.relevant, count.omitted, count.boundary[0], count.boundary[1], count.boundary[2],
              count.beyond[0], count.beyond[1], count.beyond[2], count.plateau_relevant,
              count.plateau_omitted, balls.size(), source_ok);
  if (mutant) {
    std::set<unsigned> wanted_after_mutation;
    for (const auto& [shell, id] : expected)
      if ((u64)(std::popcount(oracle.supports[id].inside) + oracle.supports[id].q) < smax)
        wanted_after_mutation.insert(shell);
    check(!source_ok && source_contents_ok && found == wanted_after_mutation &&
          count.boundary[0] + count.boundary[1] + count.boundary[2] > 0,
          "mutant retire exactement les boules de rang smax, aucune autre");
    return count;
  }
  check(source_ok, "catalogue complet, census et rang minimal contre OBig");
  check((count.plateau_relevant != 0) == expect_degenerate, "statut de regularite pertinent attendu");
  if (!source_ok || failures != 0) return count;
  if (!expect_degenerate) {
    for (u64 K = 1; K < smax; ++K) {
      std::map<unsigned, size_t> wanted;
      for (const auto& [shell, id] : expected) {
        const auto& s = oracle.supports[id];
        if ((u64)(std::popcount(s.inside) + s.q - 1) == K) wanted.emplace(shell | s.inside, id);
      }
      std::vector<ForestEvent> events;
      expand_events_k(ix, balls, K, smax - 1, opt.threads, &events, &es);
      std::set<unsigned> seen;
      for (const auto& e : events) {
        const unsigned mask = event_mask(e);
        const auto it = wanted.find(mask);
        check(it != wanted.end() && seen.insert(mask).second, "coface directe exacte et unique");
        if (it != wanted.end()) {
          const auto& s = oracle.supports[it->second];
          unsigned shell = 0;
          for (size_t i = 0; i < e.q; ++i) shell |= 1u << e.support[i];
          check(shell == s.shell && compare(e.level, s.ball) == 0, "support direct et niveau exact");
        }
      }
      check(seen.size() == wanted.size(), "expansion complete sur chaque ordre de la fenetre");
      count.direct += events.size();
    }
  }
  if (!judge_gamma && !expect_degenerate) return count;
  Oracle gamma = judge_gamma ? oracle.gamma() : Oracle({});
  RunOptions ro; ro.smax = smax; ro.threads = reverse ? 2 : 1;
  ro.complete_silent_incidence = true;
  ro.forest_layout = reverse ? ForestLayout::kCsr : ForestLayout::kClassic;
  ro.on_forest = [&](u64 K, const std::vector<ForestEvent>& events, const ForestResult& forest) {
    ++count.callbacks;
    std::vector<ForestEvent> direct, extra;
    std::set<unsigned> actual_direct, expected_direct;
    for (const auto& [shell, id] : expected) {
      const auto& ref = oracle.supports[id];
      if ((u64)(std::popcount(ref.inside) + ref.q - 1) == K)
        expected_direct.insert(shell | ref.inside);
    }
    for (const auto& e : events) {
      const unsigned mask = event_mask(e);
      check(compare(e.level, gamma.ball(mask)) == 0, "pipeline evenement au niveau OBig exact");
      if (gamma.direct(mask)) {
        direct.push_back(e);
        check(actual_direct.insert(mask).second, "pipeline direct sans doublon");
      } else extra.push_back(e);
    }
    check(actual_direct == expected_direct, "pipeline recoit tout le catalogue direct de chaque K");
    if (K != 2 && K != 10) return;
    count.silent += extra.size();
    for (const auto& [shell, id] : oracle.unique) {
      const auto& omitted = oracle.supports[id];
      if ((u64)(std::popcount(omitted.inside) + omitted.q) <= smax ||
          std::popcount(shell) == omitted.q) continue;
      std::set<unsigned> core;
      for (unsigned q : actual_direct)
        for (unsigned bits = q; bits; bits &= bits - 1) core.insert(q ^ (bits & (0u - bits)));
      for (unsigned f : core) {
        if (f & ~(shell | omitted.inside)) continue;
        check(compare(gamma.ball(f), omitted.ball) < 0, "contact coeur strict sous bloc irregulier omis");
        bool prior_incidence = false;
        for (unsigned i = 0; i < pts.size(); ++i)
          if (!(f & (1u << i)) && ((shell | omitted.inside) & (1u << i)) &&
              compare(gamma.ball(f | (1u << i)), omitted.ball) < 0) prior_incidence = true;
        check(prior_incidence, "contact coeur possede une coface strictement anterieure");
        ++count.strict_core_contacts;
      }
    }
    check(compare_cuts(gamma, (int)K, direct, extra), "pipeline complet H0 ouvert ferme sur coeur et couverture");
    check(compare_normalized(gamma, (int)K, forest), "pipeline q_R et transitions normalises");
    check(compare_delta_cuts(gamma, (int)K, forest), "payload seul reconstruit Gamma a chaque coupe");
  };
  const auto result = run_pipeline(input, ro);
  const auto desired = expect_degenerate ? PipelineStatus::kUnsupportedDegeneracy : PipelineStatus::kCompleteRegular;
  check(result.status == desired, "statut pipeline suivant regularite de fenetre");
  check(count.callbacks == (expect_degenerate ? 0 : smax - 1), "callbacks non vacants ou refus avant publication");
  std::printf("pipeline status=%d expected=%d callbacks=%zu direct=%zu silent_judged=%zu core_contacts=%zu cuts=%llu delta_cuts=%llu failure_total=%d reason=%s\n",
              (int)result.status, (int)desired, count.callbacks, count.direct, count.silent,
              count.strict_core_contacts,
              (unsigned long long)judged_cuts, (unsigned long long)delta_cuts, failures, result.message.c_str());
  return count;
}

void shell_rank_mutant() {
  // Audit-only corruption at the declared census seam. Product source and
  // official mutant registry stay unchanged. The intended dent is a lost
  // unsupported_degeneracy status, not an asserted H0 mismatch.
  const auto pts = boundary_points(true);
  std::vector<InputPoint> input;
  for (unsigned i = 0; i < pts.size(); ++i) input.push_back({i, pts[i]});
  RunOptions opt; opt.complete_silent_incidence = true;
  size_t removed = 0, callbacks = 0;
  opt.prefilter_census_override = [&](const CloudIndex& ix,
      const std::vector<BallCandidate>& candidates, u64 smax, size_t shell_cap,
      std::vector<Survivor>* survivors, std::vector<BallData>* balls, ExpandStats* stats) {
    prefilter_balls(ix, candidates, smax, 1, survivors, stats);
    if (census_balls(ix, candidates, *survivors, smax, shell_cap, 1, balls, stats) !=
        PipelineStatus::kCompleteRegular) return std::string("unexpected census failure");
    balls->erase(std::remove_if(balls->begin(), balls->end(), [&](const BallData& b) {
      const bool erase = (u64)b.n_interior + b.n_shell > smax;
      if (erase) {
        check(b.arity == 2 && b.n_interior == 9 && b.n_shell == 3 &&
              ids_mask(ix, b.shell()) == 7u, "mutation exactement AB/Y avec neuf interieurs");
        ++removed;
      }
      return erase;
    }), balls->end());
    return std::string();
  };
  opt.on_forest = [&](u64, const std::vector<ForestEvent>&, const ForestResult&) { ++callbacks; };
  const auto result = run_pipeline(input, opt);
  check(removed == 1 && callbacks == 10 && result.status == PipelineStatus::kCompleteRegular,
        "mauvais rang shell perd exactement le refus pertinent au lieu de le conserver");
  std::printf("audit_mutation=shell_cardinality_as_support_arity removed=%zu callbacks=%zu observed_status=%d required_nominal_status=%d failures=%d\n",
              removed, callbacks, (int)result.status, (int)PipelineStatus::kUnsupportedDegeneracy, failures);
}
}  // namespace

int main(int argc, char** argv) {
  mhgp7_oracle::clear_overflow();
  const std::string mode = argc > 1 ? argv[1] : "--boundary-outside";
  bool mutant = mode == "--mutant=depth-threshold-minus-one" || mode == "--mutant=shell-rank";
  if (mode == "--mutant=shell-rank") {
    shell_rank_mutant();
  } else if (mutant) {
    check(mutants_enable("depth-threshold-minus-one"), "mutant officiel reconnu");
    run_case(random_points(13, 132741), 11, false, false, false, true);
  } else if (mode == "--boundary-outside" || mode == "--boundary-inside") {
    const bool relevant = mode == "--boundary-inside";
    const auto s = run_case(boundary_points(relevant), 11, !relevant, false, relevant, false);
    check(relevant ? s.plateau_relevant > 0 : s.plateau_relevant == 0 && s.plateau_omitted > 0,
          "degenerescence ciblee de part et autre du seuil");
    check(relevant || s.strict_core_contacts > 0, "lemme de contact du coeur non vacant");
  } else if (mode == "--random") {
    const size_t n = argc > 2 ? (size_t)std::strtoul(argv[2], nullptr, 10) : 13;
    if (n < 12 || n > 24) { std::fprintf(stderr, "n must lie in [12,24]\n"); return 2; }
    const u64 seed = argc > 3 ? std::strtoull(argv[3], nullptr, 10) : 132741;
    const bool reverse = argc > 4 && std::strcmp(argv[4], "reverse") == 0;
    const auto s = run_case(random_points(n, seed), 11, n == 13, reverse, false, false);
    check(s.omitted > 0 && s.boundary[0] + s.boundary[1] + s.boundary[2] > 0,
          "frontiere de rang non vacante");
  } else {
    std::fprintf(stderr, "unknown mode\n"); return 2;
  }
  check(!mhgp7_oracle::overflow_seen(), "OBig640 sans debordement");
  std::printf("window_verdict failures=%d overflow=%d mutant=%d\n", failures,
              mhgp7_oracle::overflow_seen(), mutant);
  return failures ? 1 : mutant ? 4 : 0;
}
