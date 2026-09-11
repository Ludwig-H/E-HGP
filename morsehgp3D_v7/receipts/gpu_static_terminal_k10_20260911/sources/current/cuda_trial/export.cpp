// Export-only independent Gram catalogues + c03 CPU terminal traces.
// All declared K>=2 facets are nominal, never silently skipped on failure.
#define main historical_full_gate_main
#include "../source/morsehgp3D_v7/tests/full_ball_tower_gate.cpp"
#undef main
#include "fixture_types.hpp"
#include "reference.hpp"
#include "large_cases.hpp"

namespace export_terminal {
namespace tc = mhgp7::terminal_cuda_gate;
namespace t = mhgp7::gpu_terminal_private;
using namespace mhgp7;
template <class T> void numbers(const T* values, size_t n) {
  std::printf("{");
  for (size_t i = 0; i < n; ++i) {
    if (i) std::printf(",");
    if constexpr (std::is_signed_v<T>) std::printf("%lld", static_cast<long long>(values[i]));
    else std::printf("%lluULL", static_cast<unsigned long long>(values[i]));
  }
  std::printf("}");
}
void number(u64 value) { std::printf("%lluULL", static_cast<unsigned long long>(value)); }
void work(const t::Work& w) {
  std::printf("{"); number(w.calls); std::printf(","); numbers(w.supports, 5);
  const u64 fields[]{w.powers,w.materializations,w.level_materializations,w.key_lookups,w.anchor_hits,
      w.intruder_queries,w.intruder_nodes,w.intruder_power_tests,w.interior_ranges,w.same_radius_steps,
      w.descending_steps,w.max_chain_steps,w.axis_divisions,w.stack_peak};
  for (auto field : fields) { std::printf(","); number(field); }
  std::printf("}");
}
void request(const tc::PinnedRequest& p) {
  std::printf("{{0,"); number(p.request.ordinal); std::printf(",%u,", p.request.k);
  numbers(p.request.selected,10); std::printf(",{"); numbers(p.request.before.words,5);
  std::printf("}},{terminal::Status::kOk,"); number(p.result.ordinal);
  std::printf(",%u,",p.result.target); work(p.result.work); std::printf("},");
  number(p.trace_begin); std::printf(","); number(p.trace_count); std::printf("},\n");
}
void trace(const t::TraceRow& row) {
  std::printf("{"); numbers(row.sites,10);
  std::printf(",%u,{%u,terminal::meb_selection::Status::kOk,%u,%u,",row.k,row.selection.ordinal,
      row.selection.q,row.selection.shell);
  numbers(row.selection.slots,4); std::printf(","); number(row.selection.calls); std::printf(",");
  numbers(row.selection.supports,5); std::printf(","); number(row.selection.powers); std::printf("},{");
  numbers(row.key.words,10); std::printf("},{"); numbers(row.level.words,5);
  std::printf("},%d,%u},\n",row.intruder,row.terminal);
}
int run() {
  std::vector<P3> points;
  std::vector<t::CatalogEntry> balls;
  std::vector<tc::CloudFixture> clouds;
  std::vector<tc::PinnedRequest> requests;
  std::vector<t::TraceRow> traces;
  u64 q[5]{}, equal = 0, strict = 0, intruders = 0, extra = 0, gram = 0, model_cross_checks = 0;
  u64 count_k[11]{}, strict_k[11]{}, intruder_k[11]{}, q3_k[11]{}, q4_k[11]{};
  const auto plans = planned_fixtures();
  // Build every mask list before any MEB/model/catalogue computation.
  std::vector<std::vector<u32>> mask_plans;
  for (const auto& fixture : plans) mask_plans.push_back(declared_masks(fixture));
  size_t plan_index = 0;
  for (const auto& fixture : plans) {
    context = fixture.name;
    const auto ix = build_cloud_index(fixture.points);
    const FixtureOracle model(fixture);
    if (model.historical) for (u32 mask = 1; mask < (u32{1} << fixture.points.size()); ++mask) {
      const auto old = model.historical->meb(mask), assigned = model.assigned->meb(mask);
      need(old.center == assigned.center && old.radius2 == assigned.radius2,"export.T2_historical_model_equality");
      ++model_cross_checks;
    }
    // Full finite fixture catalogue, not a large-cloud product algorithm.
    const auto catalogue_rows = model.historical ? catalogue(fixture.points, ix, *model.historical,
        static_cast<unsigned>(fixture.points.size())) : large_catalogue(fixture.points,ix);
    tc::CpuTerminal reference(ix, catalogue_rows);
    tc::CloudFixture cloud{static_cast<u32>(points.size()),static_cast<u32>(ix.upos.size()),
        static_cast<u32>(balls.size()),static_cast<u32>(catalogue_rows.size()),
        static_cast<u32>(requests.size()),0};
    points.insert(points.end(),ix.upos.begin(),ix.upos.end());
    for (const auto& ball : catalogue_rows)
      balls.push_back({t::meb_key::encode_key(ball.key),t::encode_level(ball.level),ball.arity,
          ball.n_interior,ball.n_shell,0});
    const auto& masks = mask_plans[plan_index++];
    for (u32 mask : masks) {
      const auto k = static_cast<u32>(std::popcount(mask));
      context = std::string(fixture.name) + "/mask=" + std::to_string(mask);
      tc::PinnedRequest pinned{};
      pinned.request.ordinal = (u64{1} << 40) + requests.size();
      pinned.request.k = k;
      pinned.request.before = t::encode_level(ExactLevel{{12884508676ULL,0,0},1});
      u32 at = 0;
      for (u32 bit = 0; bit < ix.upos.size(); ++bit) if ((mask >> bit) & 1u)
        pinned.request.selected[at++] = static_cast<i32>(bit);
      pinned.trace_begin = traces.size();
      pinned.result = reference.resolve(pinned.request,traces);
      pinned.trace_count = traces.size() - pinned.trace_begin;
      need(pinned.result.status == t::Status::kOk && pinned.trace_count > 0,"export.nominal_terminal");
      for (u64 j = pinned.trace_begin; j < traces.size(); ++j) {
        const auto& row = traces[j];
        u32 original_mask = 0;
        for (u32 i = 0; i < row.k; ++i) {
          const auto found = std::find(fixture.points.begin(),fixture.points.end(),ix.upos[row.sites[i]]);
          need(found != fixture.points.end(),"export.trace_original_index");
          original_mask |= u32{1} << (found - fixture.points.begin());
        }
        const auto independent = model.meb(original_mask);
        need(t::compare_keys(row.key,t::meb_key::encode_key(key(independent))) == 0,
            "export.trace_Gram_key");
        need(same_exact_level(t::decode_level(row.level),level(independent.radius2)),
            "export.trace_Gram_level");
        ++gram; ++q[row.selection.q];
        q3_k[k] += row.selection.q == 3; q4_k[k] += row.selection.q == 4;
        extra += row.selection.shell > row.selection.q;
      }
      equal += pinned.result.work.same_radius_steps; strict += pinned.result.work.descending_steps;
      intruders += pinned.result.work.intruder_queries;
      ++count_k[k]; strict_k[k] += pinned.result.work.descending_steps;
      intruder_k[k] += pinned.result.work.intruder_queries;
      requests.push_back(pinned); ++cloud.request_count;
    }
    need(cloud.request_count == masks.size(),"export.all_declared_facets_accounted");
    clouds.push_back(cloud);
  }
  need(clouds.size() == 17 && requests.size() == 1577 && q[2] && q[3] && q[4] && equal && strict && intruders && extra &&
      model_cross_checks == 1022 && count_k[9] == 468 && count_k[10] == 160,
      "export.nonvacuity");
  for (u32 k : {9u,10u}) need(strict_k[k] && intruder_k[k] && q3_k[k] && q4_k[k],"export.high_K_nonvacuity");
  for (const auto& p : requests) need(p.request.ordinal > (u64{1} << 32) &&
      p.result.ordinal == p.request.ordinal,"export.large_ordinal_identity");
  std::printf("// Generated from pinned Gram catalogues and c03 CPU terminal; no inherited GPU result.\n");
  std::printf("inline constexpr P3 fixture_points[]{\n");
  for (const auto& p : points) std::printf("{%lld,%lld,%lld},\n",(long long)p.x,(long long)p.y,(long long)p.z);
  std::printf("};\ninline constexpr terminal::CatalogEntry fixture_balls[]{\n");
  for (const auto& b : balls) {
    std::printf("{{"); numbers(b.key.words,10); std::printf("},{"); numbers(b.level.words,5);
    std::printf("},%u,%u,%u,0},\n",b.arity,b.interior,b.shell);
  }
  std::printf("};\ninline constexpr CloudFixture fixture_clouds[]{\n");
  for (const auto& c : clouds) std::printf("{%u,%u,%u,%u,%u,%u},\n",c.point_begin,c.point_count,
      c.ball_begin,c.ball_count,c.request_begin,c.request_count);
  std::printf("};\ninline constexpr PinnedRequest fixture_requests[]{\n");
  for (const auto& p : requests) request(p);
  std::printf("};\ninline constexpr terminal::TraceRow fixture_traces[]{\n");
  for (const auto& row : traces) trace(row);
  std::printf("};\n");
  std::fprintf(stderr,"{\"status\":\"passed\",\"clouds\":%zu,\"requests\":%zu,\"trace_rows\":%zu,"
      "\"Gram_checked_rows\":%llu,\"q2\":%llu,\"q3\":%llu,\"q4\":%llu,\"extra_shells\":%llu,"
      "\"strict_steps\":%llu,\"same_radius_steps\":%llu,\"intruder_queries\":%llu,"
      "\"large_ordinals\":%zu,\"model_cross_checks\":%llu,\"K9_requests\":%llu,\"K10_requests\":%llu,"
      "\"K9_strict_steps\":%llu,\"K10_strict_steps\":%llu,\"K9_intruder_queries\":%llu,\"K10_intruder_queries\":%llu,"
      "\"K9_q3\":%llu,\"K10_q3\":%llu,\"K9_q4\":%llu,\"K10_q4\":%llu,"
      "\"all_declared_facets_nominal\":true,\"silently_filtered_failures\":0}\n",
      clouds.size(),requests.size(),traces.size(),(unsigned long long)gram,(unsigned long long)q[2],
      (unsigned long long)q[3],(unsigned long long)q[4],(unsigned long long)extra,
      (unsigned long long)strict,(unsigned long long)equal,(unsigned long long)intruders,requests.size(),
      (unsigned long long)model_cross_checks,(unsigned long long)count_k[9],(unsigned long long)count_k[10],
      (unsigned long long)strict_k[9],(unsigned long long)strict_k[10],
      (unsigned long long)intruder_k[9],(unsigned long long)intruder_k[10],
      (unsigned long long)q3_k[9],(unsigned long long)q3_k[10],(unsigned long long)q4_k[9],(unsigned long long)q4_k[10]);
  return 0;
}
}  // namespace export_terminal
int main(int argc,char** argv) {
  if (argc != 2) return 2;
  const auto mode = std::string_view(argv[1]);
  if (mode != "--export" && mode != "--plan") return 2;
  try { return mode == "--plan" ? export_terminal::print_plan() : export_terminal::run(); }
  catch (const Failure& error) { std::fprintf(stderr,"FAIL %s: %s\n",context.c_str(),error.why); return 1; }
  catch (const std::exception& error) { std::fprintf(stderr,"FAIL %s: %s\n",context.c_str(),error.what()); return 1; }
}
