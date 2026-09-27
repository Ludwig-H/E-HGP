// Explicit read-only reuse of input recipes, loader and front helpers from
// ddf4776d7. The old entry point is renamed and never invoked by this driver.
#define main factor_plan_unused_main
#include "../b_q34_factor_plan_20260926/probe.cpp"
#undef main
#include "bands.hpp"

namespace bp = mhgp9::audit::bands;
constexpr const char* band_schema = "mhgp9_q34_bands_v2";
struct BandChecks { u64 cases{}, pairs{}, cells{}, native_fronts{}, native_rectangles{}, refusals{}; };

unsigned reference_mask(fp::Credit ac, fp::Credit bc, unsigned k, unsigned mask) {
  unsigned out = 0;
  if (k >= 2 && (mask & 2U) != 0 && unsigned(ac.q3)+bc.q3 < k-1U) out |= 2U;
  if (k >= 3 && (mask & 4U) != 0 && unsigned(ac.q4)+bc.q4 < k-2U) out |= 4U;
  return out;
}

// Compare every occupied credit cell without expanding native large products.
// Band boundaries must coincide with full B classes; overlapping descriptors
// are refused independently of masses. Exact pair masks are tested in all
// represented cells, including cells with only one active lane.
void judge_cells(const fp::Factor& a, const fp::Factor& b, unsigned k, unsigned mask,
                 const bp::Plan& plan, BandChecks& checks, bool expand) {
  ++checks.cases;
  u64 q3 = 0, q4 = 0, mass = 0;
  std::vector<std::size_t> counts(a.groups.size(), 0);
  for (const auto& band : plan.bands) {
    require(band.a_class < a.groups.size() && band.b_first < band.b_last &&
            band.b_last <= b.grouped.size(), "bands_bounds");
    ++counts[band.a_class];
  }
  for (const auto count : counts) require(count <= k+1, "bands_count_bound");
  for (std::size_t ai = 0; ai != a.groups.size(); ++ai) {
    const auto& ag = a.groups[ai];
    for (const auto& bg : b.groups) {
      ++checks.cells;
      unsigned covered = 0;
      for (const auto& band : plan.bands) if (band.a_class == ai &&
          bg.ranks.first < band.b_last && band.b_first < bg.ranks.last) {
        require(band.b_first <= bg.ranks.first && bg.ranks.last <= band.b_last, "bands_split_credit_cell");
        ++covered;
      }
      require(covered <= 1, "bands_overlap_duplicate");
      const auto expected = reference_mask(ag.credit, bg.credit, k, mask);
      require((covered == 1) == (expected != 0), "bands_coverage_differs");
      if (covered == 1) {
        require(plan.pair_mask(ai, bg.ranks.first) == expected &&
                plan.pair_mask(ai, bg.ranks.last-1) == expected, "bands_mask_differs");
        const auto size = fp::product(ag.ranks.size(), bg.ranks.size());
        mass += size;
        if ((expected & 2U) != 0) q3 += size;
        if ((expected & 4U) != 0) q4 += size;
      }
    }
  }
  require(mass == plan.union_mass && q3 == plan.q3_mass && q4 == plan.q4_mass,
          "bands_mass_differs");
  if (!expand) return;
  std::vector<unsigned char> emitted(a.grouped.size()*b.grouped.size(), 0);
  for (const auto& band : plan.bands) {
    const auto ar = a.groups[band.a_class].ranks;
    for (auto ai = ar.first; ai != ar.last; ++ai)
      for (auto bi = std::size_t{band.b_first}; bi != band.b_last; ++bi) {
        auto& cell = emitted[ai*b.grouped.size()+bi];
        require(cell == 0, "bands_pair_duplicate");
        cell = static_cast<unsigned char>(plan.pair_mask(band.a_class, bi));
      }
  }
  for (std::size_t ai = 0; ai != a.grouped.size(); ++ai)
    for (std::size_t bi = 0; bi != b.grouped.size(); ++bi) {
      ++checks.pairs;
      const auto ac = a.credits[a.grouped[ai]-a.original_ranks.first];
      const auto bc = b.credits[b.grouped[bi]-b.original_ranks.first];
      require(emitted[ai*b.grouped.size()+bi] == reference_mask(ac, bc, k, mask),
              "bands_pair_mask_coverage");
    }
}

fp::Factor manufacture(unsigned k, unsigned pattern, std::size_t origin) {
  fp::Factor out;
  const unsigned t3 = k >= 2 ? k-1 : 0, t4 = k >= 3 ? k-2 : 0;
  for (unsigned x = 0; x <= t3; ++x) for (unsigned y = 0; y <= t4; ++y) {
    const unsigned count = pattern == 0 ? 0 : pattern == 1 ? 1 :
        pattern == 2 ? 2 : (7*x + 11*y + pattern)%4;
    for (unsigned i = 0; i != count; ++i)
      out.credits.push_back({static_cast<std::uint8_t>(x),static_cast<std::uint8_t>(y)});
  }
  if ((pattern & 1U) != 0) std::reverse(out.credits.begin(), out.credits.end());
  out.original_ranks = {origin, origin+out.credits.size()};
  for (std::size_t i = 0; i != out.credits.size(); ++i) out.grouped.push_back(origin+i);
  std::stable_sort(out.grouped.begin(), out.grouped.end(), [&](auto x, auto y) {
    const auto ac = out.credits[x-origin], bc = out.credits[y-origin];
    return 10*ac.q3+ac.q4 < 10*bc.q3+bc.q4;
  });
  for (std::size_t i = 0; i != out.grouped.size();) {
    const auto first = i;
    const auto c = out.credits[out.grouped[i]-origin];
    while (i != out.grouped.size() && out.credits[out.grouped[i]-origin] == c) ++i;
    out.groups.push_back({c, {first,i}});
  }
  return out;
}

void band_gate(bp::Mutant mutant) {
  BandChecks checks;
  for (unsigned k = 1; k <= 10; ++k) for (unsigned mask : {0U,2U,4U,6U})
    for (unsigned ap = 0; ap != 6; ++ap) for (unsigned bpatt = 0; bpatt != 6; ++bpatt) {
      const auto a = manufacture(k, ap, 127), b = manufacture(k, bpatt, 911);
      const bp::Plan plan(a,b,k,mask,mutant);
      judge_cells(a,b,k,mask,plan,checks,true);
    }
  for (const char* family : {"uniform","terrain","clusters","rows"}) {
    auto points = bench::make_front_fixture(24,family,3).points;
    for (unsigned permutation = 0; permutation != 2; ++permutation) {
      if (permutation != 0) std::reverse(points.begin(),points.end());
      const auto cloud = prepare_cloud(points);
      const auto index = make_q2_cloud_index(cloud);
      for (const unsigned k : {2U,3U,5U,10U}) for (const unsigned s : {8U,10U,12U}) {
        ++checks.native_fronts;
        static_cast<void>(run_wspd_front(*index,k,s,WspdFrontMode::Pure,[&](const WspdRectangle& r) {
          const fp::Plan old(*index,r.a_node,r.b_node,k,r.lane_mask);
          const bp::Plan plan(old.a,old.b,k,r.lane_mask,mutant);
          judge_cells(old.a,old.b,k,r.lane_mask,plan,checks,true);
          require(plan.union_mass == old.union_mass && plan.q3_mass == old.q3_mass &&
              plan.q4_mass == old.q4_mass,"bands_native_mass");
          ++checks.native_rectangles;
        },6));
      }
    }
  }
  const auto good = manufacture(5,2,900);
  for (unsigned failure = 0; failure != 8; ++failure) {
    auto bad = good;
    if (failure == 0) bad.grouped.pop_back();
    if (failure == 1) bad.grouped[0] = 1;
    if (failure == 2) bad.grouped[1] = bad.grouped[0];
    if (failure == 3) bad.groups[0].ranks.first = 1;
    if (failure == 4) bad.groups[0].credit.q4 = 9;
    if (failure == 5) bad.groups.pop_back();
    bool rejected = false;
    try { const bp::Plan plan(bad,good,failure == 6 ? 0U : 5U,failure == 7 ? 8U : 6U); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected,"bands_refusal_missing"); ++checks.refusals;
  }
  std::cout << "{\"schema\":\"" << band_schema << "\",\"status\":\"pass\",\"mode\":\"gate\",\"cases\":"
    << checks.cases << ",\"pairs\":" << checks.pairs << ",\"cells\":" << checks.cells
    << ",\"native_fronts\":" << checks.native_fronts << ",\"native_rectangles\":" << checks.native_rectangles
    << ",\"refusals\":" << checks.refusals << "}\n";
}

void band_measure(std::vector<Point3> points, unsigned k, unsigned s, std::string_view mode,
                  std::string_view family, Clock::time_point start, double input_ms) {
  if (k < 2 || k > 10 || (s != 8 && s != 10 && s != 12))
    throw std::invalid_argument("bands.measure_K_s");
  const auto hash = input_hash(points);
  const auto index_start = Clock::now();
  const auto cloud = prepare_cloud(points);
  const auto index = make_q2_cloud_index(cloud);
  const auto index_ms = elapsed(index_start);
  Q34WitnessSearchWork search;
  Q34WitnessBoundsWork bounds;
  BandChecks checks;
  u64 P=0,E=0,E3=0,E4=0,F=0,planned=0,fallback=0,cells=0,band_count=0;
  u64 cell_bytes=0,band_bytes=0,plan_bytes=0,band_owned=0,validation_ranks=0,row_tests=0;
  double filter_ms=0,old_ms=0,band_ms=0,verification_ms=0;
  const auto front_start=Clock::now();
  const auto front = run_wspd_front(*index,k,s,WspdFrontMode::MidpointSamples,[&](const WspdRectangle& r) {
    const auto& a=index->spatial_nodes()[r.a_node];
    const auto& b=index->spatial_nodes()[r.b_node];
    auto tick=Clock::now();
    const auto mask=filter_q34_witnesses(*index,a.box,b.box,static_cast<std::uint8_t>(k),r.lane_mask,
                                      search,Q34WitnessBoundsMode::Affine,bounds);
    filter_ms+=elapsed(tick);
    if (mask == 0) return;
    const auto mass=fp::product(a.range.size(),b.range.size()); P+=mass;
    const unsigned threshold=(mask & 4U) != 0 ? k-2U : k-1U;
    if (std::min(a.range.size(),b.range.size()) < 2 || a.range.size()+b.range.size()-2 < threshold) {
      ++fallback; E+=mass;
      if ((mask & 2U) != 0) E3+=mass;
      if ((mask & 4U) != 0) E4+=mass;
      return;
    }
    tick=Clock::now(); const fp::Plan old(*index,r.a_node,r.b_node,k,mask); old_ms+=elapsed(tick);
    tick=Clock::now(); const bp::Plan plan(old.a,old.b,k,mask); band_ms+=elapsed(tick);
    tick=Clock::now(); judge_cells(old.a,old.b,k,mask,plan,checks,false); verification_ms+=elapsed(tick);
    require(old.union_mass == plan.union_mass && old.q3_mass == plan.q3_mass &&
        old.q4_mass == plan.q4_mass,"bands_native_mass");
    ++planned; F+=old.work.factor_sites; E+=plan.union_mass; E3+=plan.q3_mass; E4+=plan.q4_mass;
    cells+=old.blocks.size(); band_count+=plan.bands.size();
    cell_bytes+=old.blocks.capacity()*sizeof(fp::Block); band_bytes+=plan.bands.capacity()*sizeof(bp::Band);
    plan_bytes+=old.retained_bytes(); band_owned+=plan.retained_bytes();
    validation_ranks+=plan.work.validation_ranks; row_tests+=plan.work.row_tests;
  },6);
  const auto total=elapsed(start), front_ms=elapsed(front_start);
  std::cout << std::fixed << std::setprecision(6) << "{\"schema\":\"" << band_schema
    << "\",\"status\":\"pass\",\"mode\":" << quoted(mode) << ",\"scope\":\"representation_only_old_cells_still_paid\""
    << ",\"n\":" << points.size() << ",\"k\":" << k << ",\"s\":" << s << ",\"seed\":3,\"min_factor\":2"
    << ",\"family\":" << quoted(family) << ",\"input_hash\":\"" << hex(hash) << "\",\"metrics\":{"
    << "\"P\":" << P << ",\"E\":" << E << ",\"E3\":" << E3 << ",\"E4\":" << E4
    << ",\"F\":" << F << ",\"front_F\":" << front.work.emitted_factor_sites
    << ",\"planned\":" << planned << ",\"fallback\":" << fallback << ",\"cells\":" << cells
    << ",\"bands\":" << band_count << ",\"cell_descriptor_bytes\":" << cell_bytes
    << ",\"band_descriptor_bytes\":" << band_bytes << ",\"old_plan_bytes\":" << plan_bytes
    << ",\"band_owned_bytes\":" << band_owned << ",\"rank_entries_added\":0,\"credit_entries_added\":0"
    << ",\"validation_ranks\":" << validation_ranks << ",\"row_tests\":" << row_tests
    << ",\"verified_cells\":" << checks.cells << ",\"histogram_slots\":" << 341*planned
    << ",\"histogram_prefix_steps\":" << 100*planned << "},\"times_ms\":{\"input\":" << input_ms
    << ",\"index\":" << index_ms << ",\"front_filter_plans_verify\":" << front_ms
    << ",\"rectangle_filter\":" << filter_ms << ",\"old_plan\":" << old_ms << ",\"bands\":" << band_ms
    << ",\"verification\":" << verification_ms << ",\"total\":" << total << "}}\n";
}

int main(int argc,char** argv) {
  try {
    if (argc == 2 && std::string_view(argv[1]) == "--gate") { band_gate(bp::Mutant::None); return 0; }
    if (argc == 3 && std::string_view(argv[1]) == "--mutant") {
      const std::string_view name=argv[2];
      if (name == "overlap") band_gate(bp::Mutant::Overlap);
      else if (name == "mask6") band_gate(bp::Mutant::Mask6);
      else throw std::invalid_argument("bands.unknown_mutant");
      throw std::runtime_error("bands.mutant_survived");
    }
    const auto start=Clock::now();
    if (argc == 6 && std::string_view(argv[1]) == "--synthetic") {
      auto fixture=bench::make_front_fixture(number<std::size_t>(argv[3]),argv[2],3);
      const auto input_ms=elapsed(start);
      band_measure(std::move(fixture.points),number<unsigned>(argv[4]),number<unsigned>(argv[5]),
                   "synthetic",argv[2],start,input_ms); return 0;
    }
    if (argc == 5 && std::string_view(argv[1]) == "--frame") {
      auto points=load_frame(argv[2]); const auto input_ms=elapsed(start);
      band_measure(std::move(points),number<unsigned>(argv[3]),number<unsigned>(argv[4]),
                   "frame","none",start,input_ms); return 0;
    }
    throw std::invalid_argument("bands.usage");
  } catch (const std::exception& e) {
    std::cout << "{\"schema\":\"" << band_schema << "\",\"status\":\"failed\",\"cause\":" << quoted(e.what()) << "}\n";
    return 1;
  }
}
