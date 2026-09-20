// Explicit test-helper reuse only: the previous gate is compiled with a
// renamed entry point, not executed as this gate and not inherited as proof.
// This supplies candidate copying, rational exhaustive census and ownership.
#define main mhgp8_pruning_gate_helper_entry
#include "q34_family_pruning_gate.cpp"
#undef main

#include "lanes/q34_collective.hpp"

namespace {

struct CollectiveGate : Gate {
  u64 assessments{}, collective_queries{}, universal_shortcuts{}, collective_gains{};
  u64 tighter_bounds{}, variance_roundings{}, wide_variance_numerators{};
  u64 lower_tangencies{}, upper_tangencies{}, equal_mixed_roots{}, endpoint_dips{};
  u64 initial_saturation_drops{}, exact_minimum_checks{}, workspace_reuses{};
  u64 option_calls{}, zero_budget_calls{}, jung_universal_pairs{};
  u64 foreign_owner_calls{};
};

constexpr std::array<mhgp8::Q34PoolOptions,4> pool_options{{
  {mhgp8::Q34ChordBound::Jung,mhgp8::Q34PoolReduction::Universal},
  {mhgp8::Q34ChordBound::Jung,mhgp8::Q34PoolReduction::Collective},
  {mhgp8::Q34ChordBound::Variance,mhgp8::Q34PoolReduction::Universal},
  {mhgp8::Q34ChordBound::Variance,mhgp8::Q34PoolReduction::Collective}
}};

Big ceiling(const Rational& value) {
  if (value.numerator() < 0) throw std::runtime_error("nonnegative rational ceiling expected");
  return (value.numerator()+value.denominator()-1)/value.denominator();
}

struct PoolOracle {
  struct Site { Big power, side; };
  Big jung, active;
  Rational true_variance_bound;
  std::vector<Site> sites;
  std::size_t q3_inside{}, universal_inside{}, minimum{}, lower_depth{}, upper_depth{};
  std::size_t events{}, groups{}, max_group{}, coplanar{};
  bool lower_tangent{}, upper_tangent{}, mixed_equal{}, dip{}, initial_drop{};
};

PoolOracle oracle_pool(CollectiveGate& gate, const Points& points, Ids ids,
                      std::span<const std::size_t> pool,
                      mhgp8::Q34ChordBound bound, std::size_t threshold) {
  const auto face = reference_certificate(points,ids);
  const Rational diameter(distance_squared(points[ids[0]],points[ids[1]]));
  const auto r = face.face.radius_squared;
  // Derived from centre/radius, not EX or the product's prepared integers.
  const Rational s = Rational(face.gram)*(Rational(2)-Rational(4)*r/diameter);
  const Rational t = Rational(4*face.gram)*(Rational(1)-r/diameter);
  gate.require(s.denominator() == 1 && t.denominator() == 1 && s.numerator() > 0 && t.numerator() > 0,
               "rational variance factors are not positive integral quantities");
  const Rational quotient = diameter*s/t;
  const Big staged_target = ceiling(quotient)*s.numerator();
  const Big tightened = std::min(face.limit,ceil_sqrt(staged_target));
  PoolOracle result;
  result.jung = face.limit;
  result.active = bound == mhgp8::Q34ChordBound::Variance ? tightened : face.limit;
  result.true_variance_bound = Rational(4*face.gram)*
      (diameter*diameter/(Rational(4)*(diameter-r))-r);
  gate.require(Rational(tightened*tightened) >= result.true_variance_bound && tightened <= face.limit,
               "staged variance rounding does not enclose the rational chord");
  if (bound == mhgp8::Q34ChordBound::Variance) {
    if (tightened < face.limit) ++gate.tighter_bounds;
    if (quotient.denominator() != 1 || ceil_sqrt(staged_target)*ceil_sqrt(staged_target) != staged_target)
      ++gate.variance_roundings;
    if (oracle::bits(diameter.numerator()*s.numerator()*s.numerator()) > 127)
      ++gate.wide_variance_numerators;
  }
  std::map<Rational,std::pair<std::size_t,std::size_t>> roots;
  std::vector<Rational> probes{Rational(-result.active),Rational(result.active)};
  for (const auto id : pool) {
    const Rational p = Rational(face.gram)*face.face.power(points[id]);
    gate.require(p.denominator() == 1, "nonintegral rational scaled pool power");
    const Big side = oracle::dot(face.normal,oracle::difference(points[id],points[ids[0]]));
    result.sites.push_back({p.numerator(),side});
    if (p.numerator() < 0) ++result.q3_inside;
    if (p.numerator()+result.active*oracle::absolute(side) < 0) ++result.universal_inside;
    if (side == 0) { ++result.coplanar; continue; }
    // boost::rational<cpp_int> must receive a positive denominator: its
    // bounded-integer minimum-denominator guard is not valid for cpp_int.
    const Rational root(side < 0 ? Big(-p.numerator()) : p.numerator(),oracle::absolute(side));
    if (root < Rational(-result.active) || root > Rational(result.active)) continue;
    ++result.events;
    probes.push_back(root);
    auto& signs = roots[root];
    if (side > 0) ++signs.first; else ++signs.second;
    result.lower_tangent = result.lower_tangent || root == Rational(-result.active);
    result.upper_tangent = result.upper_tangent || root == Rational(result.active);
  }
  const auto depth = [&](const Rational& mu) {
    std::size_t count = 0;
    for (const auto& site : result.sites)
      if (Rational(site.power)-mu*Rational(site.side) < Rational(0)) ++count;
    return count;
  };
  result.lower_depth = depth(Rational(-result.active));
  result.upper_depth = depth(Rational(result.active));
  result.minimum = pool.size();
  for (const auto& probe : probes) result.minimum = std::min(result.minimum,depth(probe));
  // The independent judge evaluates every candidate parameter against EVERY
  // point. It does not update a count, sort entry/exit events, or saturate.
  for (const auto& [root,signs] : roots) {
    static_cast<void>(root);
    result.mixed_equal = result.mixed_equal || (signs.first != 0 && signs.second != 0);
    result.max_group = std::max(result.max_group,signs.first+signs.second);
  }
  result.groups = roots.size();
  result.dip = result.minimum < std::min(result.lower_depth,result.upper_depth);
  result.initial_drop = result.lower_depth >= threshold && result.minimum < threshold;
  return result;
}

Points collective_overlap() {
  return {{10,10,10},{12,12,10},{12,10,12},{12,10,10},{10,10,11}};
}
Points collective_dip() {
  return {{10,10,10},{12,12,10},{12,10,12},{11,9,10},{10,10,11}};
}

std::vector<u64> filter_words(const mhgp8::Q34PoolWork& w) {
  return {w.seed_owner_tests,w.seed_owner_rejections,w.seed_queries,w.certificate_builds,
    w.sqrt_iterations,w.variance_bounds,w.variance_sqrt_iterations,w.proposed_sites,
    w.paired_predicate_tests,w.q3_credits,w.q4_universal_credits,w.q3_rejected,
    w.q4_universal_rejected,w.collective_queries,w.endpoint_tests,w.constant_tests,
    w.event_count,w.sort_comparisons,w.group_comparisons,w.event_side_tests,w.groups,
    w.collective_minimum_sum,w.collective_q4_rejected,w.q4_rejected,w.both_rejected,
    w.q3_only_survivors,w.q4_only_survivors,w.both_survivors};
}
std::vector<u64> pruning_projection(const mhgp8::Q34PoolWork& w) {
  return {w.seed_queries,w.certificate_builds,w.sqrt_iterations,w.proposed_sites,
    w.paired_predicate_tests,w.q3_credits,w.q4_universal_credits,w.q3_rejected,
    w.q4_rejected,w.both_rejected,w.q3_only_survivors,w.q4_only_survivors,w.both_survivors};
}

mhgp8::Q34PoolAssessment check_assessment(CollectiveGate& gate, const Points& points,
    const mhgp8::Q34WitnessPoolPtr& pool, std::size_t x, std::size_t k,
    mhgp8::Q34PoolOptions options, mhgp8::Q34PoolWorkspace& workspace) {
  const auto previous_bytes = workspace.retained_bytes();
  const auto result = mhgp8::assess_q34_family_pool(pool,x,k,options,workspace);
  ++gate.assessments;
  const auto& w = result.work;
  gate.require(w.peak_event_bytes == workspace.retained_bytes() &&
               workspace.retained_bytes() >= previous_bytes,
               "workspace capacity was lost or not charged on a reused assessment");
  if (previous_bytes != 0) ++gate.workspace_reuses;
  const auto edge = pool->cover()->edge_ids();
  const Ids ids{edge[0],edge[1],x};
  gate.require(w.seed_owner_tests > 0 && w.seed_owner_rejections == 0,
               "valid primitive assessment lost ownership work");
  if (k < 2 || pool->ids().empty()) {
    gate.require(!result.q3_rejected && !result.q4_rejected &&
      result.jung_parameter_bound == 0 && result.parameter_bound == 0 &&
      w.seed_queries == 0 && w.certificate_builds == 0 && w.proposed_sites == 0 &&
      w.collective_queries == 0 && w.variance_bounds == 0,
      "inactive primitive performed filtering or exposed a bound");
    return result;
  }
  const auto reference = oracle_pool(gate,points,ids,pool->ids(),options.chord,k >= 3 ? k-2 : 0);
  gate.require(Big(result.jung_parameter_bound) == reference.jung &&
               Big(result.parameter_bound) == reference.active,
               "collective bound differs from independent staged rational variance rounding");
  gate.require(w.seed_queries == 1 && w.certificate_builds == 1 && w.sqrt_iterations > 0 &&
               w.variance_bounds == static_cast<u64>(options.chord == mhgp8::Q34ChordBound::Variance),
               "assessment certificate and bound ledger");
  const bool reject3 = reference.q3_inside >= k-1;
  const bool universal4 = k >= 3 && reference.universal_inside >= k-2;
  const bool collective = k >= 3 && options.reduction == mhgp8::Q34PoolReduction::Collective && !universal4;
  const bool reject4 = universal4 || (collective && reference.minimum >= k-2);
  if (k >= 3) {
    if (reject3 && reject4) ++gate.both_rejections;
    else if (reject3) ++gate.q3_only_rejections;
    else if (reject4) ++gate.q4_only_rejections;
    else ++gate.neither_rejections;
  }
  gate.require(result.q3_rejected == reject3 && result.q4_rejected == reject4,
               "collective rejection differs from exact pool minimum and separate thresholds");
  gate.require(w.q3_credits == std::min(reference.q3_inside,k-1) &&
               w.q4_universal_credits == (k >= 3 ? std::min(reference.universal_inside,k-2) : 0),
               "universal credits were not separately saturated");
  gate.require(w.q3_rejected == static_cast<u64>(reject3) &&
               w.q4_universal_rejected == static_cast<u64>(universal4) &&
               w.q4_rejected == static_cast<u64>(reject4) &&
               w.collective_queries == static_cast<u64>(collective),
               "universal/collective rejection counters were conflated");
  gate.require(w.both_rejected+w.q3_only_survivors+w.q4_only_survivors+w.both_survivors == 1 &&
               w.both_rejected == static_cast<u64>(reject3 && (k < 3 || reject4)),
               "collective available-lane status partition");
  std::size_t expected_proposals = 0, credit3 = 0, credit4 = 0;
  for (const auto& site : reference.sites) {
    if (credit3 == k-1 && (k < 3 || credit4 == k-2)) break;
    if (site.power < 0 && credit3 < k-1) ++credit3;
    if (k >= 3 && site.power+reference.active*oracle::absolute(site.side) < 0 && credit4 < k-2) ++credit4;
    ++expected_proposals;
  }
  gate.require(w.proposed_sites == expected_proposals && w.paired_predicate_tests == expected_proposals,
               "initial pool scan work or early universal stopping changed");
  if (collective) {
    gate.require(result.collective_minimum == reference.minimum &&
                 w.collective_minimum_sum == reference.minimum &&
                 w.collective_q4_rejected == static_cast<u64>(reject4),
                 "collective sweep minimum differs from all rational roots and both closed endpoints");
    ++gate.collective_queries; ++gate.exact_minimum_checks;
    if (reject4) ++gate.collective_gains;
    if (reference.lower_tangent) ++gate.lower_tangencies;
    if (reference.upper_tangent) ++gate.upper_tangencies;
    if (reference.mixed_equal) ++gate.equal_mixed_roots;
    if (reference.dip) ++gate.endpoint_dips;
    if (reference.initial_drop) ++gate.initial_saturation_drops;
    gate.require(w.proposed_sites == pool->ids().size(), "collective minimum used a truncated pool");
    gate.require(w.peak_event_bytes >= w.event_count*sizeof(std::size_t), "collective event capacity uncharged");
    gate.require(w.event_count == reference.events && w.groups == reference.groups &&
      w.max_group == reference.max_group && w.event_side_tests == reference.events &&
      w.group_comparisons == (reference.events == 0 ? 0 : reference.events-1) &&
      w.constant_tests == reference.coplanar &&
      w.endpoint_tests == 2*(pool->ids().size()-reference.coplanar),
      "collective event/group/endpoint work differs from exact rational pool classification");
  } else {
    gate.require(w.collective_minimum_sum == 0 && w.collective_q4_rejected == 0 &&
                 w.groups == 0 && w.sort_comparisons == 0,
                 "inactive collective sweep performed sorting or reported a minimum");
    if (universal4 && options.reduction == mhgp8::Q34PoolReduction::Collective) ++gate.universal_shortcuts;
  }
  if (options.reduction == mhgp8::Q34PoolReduction::Universal)
    gate.require(w.endpoint_tests == 0 && w.constant_tests == 0 && w.event_count == 0,
                 "universal mode secretly prepared collective events");
  return result;
}

void check_collective_edge(CollectiveGate& gate, const Points& points, Edge edge,
                          std::size_t k, std::size_t budget) {
  gate.require(points.size() <= 40, "collective oracle exceeded bounded small-cloud domain");
  const auto cloud = mhgp8::prepare_cloud(points);
  const auto cover = mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(cloud),edge);
  const auto pool = mhgp8::Q34WitnessPool::make(cover,budget);
  check_pool(gate,pool,budget);
  edge = cover->edge_ids();
  std::array<Output,4> per_seed_outputs;
  std::array<std::vector<u64>,4> sums;
  std::array<u64,4> max_group{},reads{};
  for (auto& sum : sums) sum.resize(filter_words({}).size());
  mhgp8::Q34PoolWorkspace scratch;
  for (std::size_t x = 0; x != points.size(); ++x) {
    if (x == edge[0] || x == edge[1]) continue;
    const Ids ids{edge[0],edge[1],x};
    if (!oracle::make(select(points,ids)).ball || owner(points,ids) != edge) continue;
    const auto expected = oracle_seed(gate,points,edge,x,k);
    Output baseline;
    const auto original = mhgp8::run_q34_cover_seed_candidates(cover,x,k,[&](const auto& item) { baseline.push_back(copy(item)); });
    normalize(baseline);
    gate.require(baseline == expected, "covered reference disagrees with rational oracle");
    std::array<bool,4> rejected4{};
    for (std::size_t option_id = 0; option_id != pool_options.size(); ++option_id) {
      const auto options = pool_options[option_id];
      const auto assessment = check_assessment(gate,points,pool,x,k,options,scratch);
      rejected4[option_id] = assessment.q4_rejected;
      Output actual;
      const auto work = mhgp8::run_q34_collective_seed_candidates(pool,x,k,options,[&](const auto& item) { actual.push_back(copy(item)); });
      normalize(actual);
      gate.require(actual == expected, "collective seed lost an exact ball, depth, presentation or shell");
      gate.require(work.peak_live_buffer_bytes >= work.covered.peak_buffer_bytes &&
                   work.peak_live_buffer_bytes >= work.filter.peak_event_bytes,
                   "coexisting collective/fallback capacity not charged");
      if (budget == 0 || k < 2) {
        gate.require(covered_signature(work.covered) == covered_signature(original),
                     "inactive collective wrapper changed old covered counters");
        gate.require(filter_words(work.filter) == std::vector<u64>(filter_words(work.filter).size()) &&
                     work.filter.peak_event_bytes == 0 && work.filter.max_group == 0,
                     "inactive wrapper did not bypass filtering");
        if (budget == 0) ++gate.zero_budget_calls;
      } else {
        gate.require(filter_words(work.filter) == filter_words(assessment.work) &&
                     work.filter.max_group == assessment.work.max_group,
                     "wrapper filter differs from standalone assessment");
      }
      if (option_id == 0) {
        Output old_output;
        const auto previous = mhgp8::run_q34_pruned_seed_candidates(pool,x,k,[&](const auto& item) { old_output.push_back(copy(item)); });
        normalize(old_output);
        gate.require(old_output == actual && covered_signature(previous.covered) == covered_signature(work.covered) &&
                     pruning_projection(work.filter) == pruning_signature(previous.pruning),
                     "Jung Universal changed tranche25 outputs or semantic work");
        ++gate.jung_universal_pairs;
      }
      if (assessment.q3_rejected)
        gate.require(work.covered.seed.q3_point_tests == 0, "collective rejected q3 still scanned");
      if (assessment.q4_rejected)
        gate.require(work.covered.seed.family.sites == 0, "collective rejected q4 still opened events");
      if (assessment.q3_rejected && assessment.q4_rejected)
        gate.require(work.covered.site_reads == 0, "both rejected lanes retained the full scan");
      const auto words = filter_words(work.filter);
      for (std::size_t i = 0; i != words.size(); ++i) sums[option_id][i] += words[i];
      max_group[option_id] = std::max(max_group[option_id],work.filter.max_group);
      reads[option_id] += work.covered.site_reads;
      per_seed_outputs[option_id].insert(per_seed_outputs[option_id].end(),actual.begin(),actual.end());
      ++gate.option_calls; ++gate.seed_calls;
    }
    gate.require(!rejected4[0] || (rejected4[1] && rejected4[2] && rejected4[3]),
                 "strengthening a certificate lost a universal rejection");
    gate.require(!(rejected4[1] || rejected4[2]) || rejected4[3], "combined strengthening lost a rejection");
    ++gate.reference_calls;
  }
  Output baseline;
  const auto original = mhgp8::run_q34_edge_candidates(cover,k,[&](const auto& item) { baseline.push_back(copy(item)); });
  normalize(baseline);
  for (std::size_t option_id = 0; option_id != pool_options.size(); ++option_id) {
    Output output;
    const auto work = mhgp8::run_q34_collective_edge_candidates(pool,k,pool_options[option_id],
      [&](const auto& item) { output.push_back(copy(item)); });
    normalize(output); normalize(per_seed_outputs[option_id]);
    gate.require(output == baseline && output == per_seed_outputs[option_id],
                 "collective edge differs from all canonical seed or covered outputs");
    gate.require(generator_signature(work.edge) == generator_signature(original), "collective filtering changed edge generation");
    if (k >= 2) gate.require(filter_words(work.filter) == sums[option_id] &&
      work.filter.max_group == max_group[option_id] && work.edge.covered.site_reads == reads[option_id],
      "edge collective work is not the sum/max of independent seeds");
    if (budget == 0) gate.require(covered_signature(work.edge.covered) == covered_signature(original.covered),
                                  "zero-budget collective edge changed reference work");
    gate.require(work.peak_live_buffer_bytes >= work.filter.peak_event_bytes &&
                 work.peak_live_buffer_bytes >= work.edge.covered.peak_buffer_bytes,
                 "edge coupled capacity maximum is understated");
    for (const auto& item : output) {
      if (item.arity == 3) ++gate.q3; else ++gate.q4;
      gate.max_shell = std::max(gate.max_shell,static_cast<u64>(item.shell.size()));
    }
    gate.candidates += output.size();
    ++gate.edge_calls;
  }
  gate.require(Points(cloud->points().begin(),cloud->points().end()) == points, "collective path changed immutable coordinates");
}

void collective_fixtures(CollectiveGate& gate) {
  const Points lower{{10,10,10},{16,16,10},{16,10,16},{16,10,10}};
  const Points upper{{10,10,10},{16,16,10},{16,10,16},{12,14,14}};
  const Points regular{{10,10,10},{12,12,10},{12,10,12},{10,12,12},{11,11,11}};
  const Points narrower{{10,10,10},{14,14,10},{14,10,13},{10,14,13},{12,12,12},{13,11,11}};
  const Points extreme{{0,0,0},{60000,65000,1000},{62000,500,64000},
    {2000,63000,62000},{32000,32000,32000},{65535,65535,65535}};
  const Points q3_only{{900,1000,1000},{1100,1000,1000},{1000,1120,1040},
    {1000,1120,960},{1000,1020,1105},{1001,1020,1105}};
  const Points both{{20,20,20},{60,60,20},{60,20,60},{20,60,60},
    {40,40,40},{41,40,40},{40,41,40},{40,40,41}};
  for (const auto& points : {collective_overlap(),collective_dip(),lower,upper,narrower,extreme,q3_only,both})
    for (const auto budget : {0U,3U,32U}) check_collective_edge(gate,points,{0,1},3,budget);
  for (const auto k : {1U,2U,3U,5U,10U})
    for (const auto budget : {0U,1U,32U}) check_collective_edge(gate,regular,{0,1},k,budget);
  check_collective_edge(gate,shell30(),{0,1},5,32);
  for (const auto& points : {collective_dip(),regular})
    for (std::size_t a = 0; a != points.size(); ++a)
      for (std::size_t b = a+1; b != points.size(); ++b) {
        check_collective_edge(gate,points,{a,b},3,32);
        ++gate.exhaustive_edges;
      }
  auto reversed = collective_overlap();
  std::reverse(reversed.begin(),reversed.end());
  check_collective_edge(gate,reversed,{reversed.size()-1,reversed.size()-2},3,32);
  ++gate.permutations;
}

void collective_lifecycle(CollectiveGate& gate) {
  const Points points{{10,10,10},{12,12,10},{12,10,12},{10,12,12},{11,11,11}};
  auto cloud = mhgp8::prepare_cloud(points);
  auto index = mhgp8::make_q2_cloud_index(cloud);
  auto cover = mhgp8::Q34EdgeCover::make(index,{0,1});
  auto pool = mhgp8::Q34WitnessPool::make(cover,32);
  mhgp8::Q34PoolWorkspace scratch;
  const auto options = pool_options[3];
  const mhgp8::Q34SeedConsumer sink = [](const auto&) {};
  gate.rejects([&] { static_cast<void>(mhgp8::assess_q34_family_pool({},2,3,options,scratch)); }, "collective null pool accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::assess_q34_family_pool(pool,2,0,options,scratch)); }, "collective K0 accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::assess_q34_family_pool(pool,0,3,options,scratch)); }, "collective repeated endpoint accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::assess_q34_family_pool(pool,4,3,options,scratch)); }, "collective nonacute seed accepted");
  gate.rejects<std::out_of_range>([&] { static_cast<void>(mhgp8::assess_q34_family_pool(pool,points.size(),3,options,scratch)); }, "collective outside ID accepted");
  auto invalid = options;
  invalid.chord = static_cast<mhgp8::Q34ChordBound>(99);
  gate.rejects([&] { static_cast<void>(mhgp8::assess_q34_family_pool(pool,2,1,invalid,scratch)); }, "K1 bypassed invalid chord option");
  invalid = options; invalid.reduction = static_cast<mhgp8::Q34PoolReduction>(99);
  {
    const auto empty = mhgp8::Q34WitnessPool::make(cover,0);
    gate.rejects([&] { static_cast<void>(mhgp8::assess_q34_family_pool(empty,2,3,invalid,scratch)); }, "empty pool bypassed invalid reduction option");
  }
  gate.rejects([&] { static_cast<void>(mhgp8::run_q34_collective_edge_candidates(pool,3,options,{})); }, "collective null callback accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q34_collective_seed_candidates(pool,2,0,options,sink)); }, "collective wrapper K0 accepted");
  // Same tied-length geometric face, but edge(1,2) loses to original IDs(0,1).
  {
    const auto foreign_cover = mhgp8::Q34EdgeCover::make(index,{1,2});
    const auto foreign_pool = mhgp8::Q34WitnessPool::make(foreign_cover,32);
    for (const auto option : pool_options) {
      const auto assessment = mhgp8::assess_q34_family_pool(foreign_pool,0,3,option,scratch);
      const auto result = mhgp8::run_q34_collective_seed_candidates(foreign_pool,0,3,option,
        [&](const auto&) { throw std::runtime_error("foreign owner emitted a candidate"); });
      gate.require(assessment.work.seed_owner_rejections == 1 && assessment.work.proposed_sites == 0 &&
                   result.covered.seed.seed_owner_rejections == 1 && result.filter.proposed_sites == 0 &&
                   result.filter.certificate_builds == 0 && result.covered.site_reads == 0,
                   "foreign owner reached pool predicates or fallback");
      ++gate.foreign_owner_calls;
    }
  }
  static_cast<void>(check_assessment(gate,points,pool,2,3,options,scratch));
  struct Failure {};
  bool threw = false;
  try { static_cast<void>(mhgp8::run_q34_collective_seed_candidates(pool,2,3,options,
    [&](const auto&) { throw Failure{}; })); }
  catch (const Failure&) { threw = true; }
  gate.require(threw, "collective callback exception was swallowed");
  ++gate.callback_failures;
  Output expected;
  {
    const auto execute = [pool,options] {
      Output output;
      static_cast<void>(mhgp8::run_q34_collective_edge_candidates(pool,3,options,
        [&](const auto& item) { output.push_back(copy(item)); }));
      normalize(output); return output;
    };
    expected = execute();
    std::array<std::future<Output>,4> calls;
    for (auto& call : calls) call = std::async(std::launch::async,execute);
    for (auto& call : calls) { gate.require(call.get() == expected, "collective calls shared mutable workspace"); ++gate.parallel_calls; }
  }
  Output actual;
  static_cast<void>(mhgp8::run_q34_collective_edge_candidates(pool,3,options,[&](const auto& item) {
    pool.reset(); cover.reset(); index.reset(); cloud.reset(); actual.push_back(copy(item));
  }));
  normalize(actual);
  gate.require(actual == expected, "collective callback reset invalidated owned data");
}

int collective_selftest() {
  CollectiveGate gate;
  collective_fixtures(gate);
  collective_lifecycle(gate);
  gate.require(gate.collective_gains > 0 && gate.universal_shortcuts > 0 &&
    gate.tighter_bounds > 0 && gate.variance_roundings > 0 && gate.wide_variance_numerators > 0,
    "collective/variance non-vacuity floors");
  gate.require(gate.lower_tangencies > 0 && gate.upper_tangencies > 0 && gate.equal_mixed_roots > 0 &&
    gate.endpoint_dips > 0 && gate.initial_saturation_drops > 0 && gate.workspace_reuses > 0,
    "closed endpoint/group/unsaturated sweep non-vacuity floors");
  gate.require(gate.zero_budget_calls > 0 && gate.jung_universal_pairs > 0 && gate.q3 > 0 && gate.q4 > 0 &&
    gate.max_shell >= 30 && gate.foreign_owner_calls == 4 && gate.parallel_calls == 4,
    "collective pipeline non-vacuity floors");
  gate.require(gate.q3_only_rejections > 0 && gate.q4_only_rejections > 0 &&
    gate.both_rejections > 0 && gate.neither_rejections > 0,
    "independent q3/q4 rejection non-vacuity floors");
  std::cout << "{\"schema\":\"mhgp8_q34_collective_gate_v1\",\"status\":\"passed\"";
#define MHGP8_COLLECTIVE_FIELD(name) std::cout << ",\"" #name "\":" << gate.name
  MHGP8_COLLECTIVE_FIELD(checks); MHGP8_COLLECTIVE_FIELD(assessments);
  MHGP8_COLLECTIVE_FIELD(collective_queries); MHGP8_COLLECTIVE_FIELD(universal_shortcuts);
  MHGP8_COLLECTIVE_FIELD(collective_gains); MHGP8_COLLECTIVE_FIELD(tighter_bounds);
  MHGP8_COLLECTIVE_FIELD(variance_roundings); MHGP8_COLLECTIVE_FIELD(wide_variance_numerators);
  MHGP8_COLLECTIVE_FIELD(q3_only_rejections); MHGP8_COLLECTIVE_FIELD(q4_only_rejections);
  MHGP8_COLLECTIVE_FIELD(both_rejections); MHGP8_COLLECTIVE_FIELD(neither_rejections);
  MHGP8_COLLECTIVE_FIELD(lower_tangencies); MHGP8_COLLECTIVE_FIELD(upper_tangencies);
  MHGP8_COLLECTIVE_FIELD(equal_mixed_roots); MHGP8_COLLECTIVE_FIELD(endpoint_dips);
  MHGP8_COLLECTIVE_FIELD(initial_saturation_drops); MHGP8_COLLECTIVE_FIELD(exact_minimum_checks);
  MHGP8_COLLECTIVE_FIELD(workspace_reuses); MHGP8_COLLECTIVE_FIELD(option_calls);
  MHGP8_COLLECTIVE_FIELD(zero_budget_calls); MHGP8_COLLECTIVE_FIELD(jung_universal_pairs);
  MHGP8_COLLECTIVE_FIELD(foreign_owner_calls); MHGP8_COLLECTIVE_FIELD(edge_calls);
  MHGP8_COLLECTIVE_FIELD(seed_calls); MHGP8_COLLECTIVE_FIELD(reference_calls);
  MHGP8_COLLECTIVE_FIELD(oracle_completions); MHGP8_COLLECTIVE_FIELD(oracle_sites);
  MHGP8_COLLECTIVE_FIELD(candidates); MHGP8_COLLECTIVE_FIELD(q3); MHGP8_COLLECTIVE_FIELD(q4);
  MHGP8_COLLECTIVE_FIELD(max_shell); MHGP8_COLLECTIVE_FIELD(pool_calls);
  MHGP8_COLLECTIVE_FIELD(exhaustive_edges); MHGP8_COLLECTIVE_FIELD(permutations);
  MHGP8_COLLECTIVE_FIELD(invalid_inputs); MHGP8_COLLECTIVE_FIELD(callback_failures);
  MHGP8_COLLECTIVE_FIELD(parallel_calls);
#undef MHGP8_COLLECTIVE_FIELD
  std::cout << "}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try { return collective_selftest(); }
  catch (const std::exception& error) {
    std::cerr << "q34 collective gate: " << error.what() << '\n';
    return 1;
  }
}
