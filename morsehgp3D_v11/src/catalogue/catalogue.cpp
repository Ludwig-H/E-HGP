// Frontiere du catalogue : validation avant allocations, refus transactionnels et stockage de feuille compte.
#include "catalogue/internal.hpp"
#include "catalogue/center_line_cache.hpp"
#include "catalogue/small_pair_graph.hpp"
#include "catalogue/single_pass_storage.hpp"

namespace mhgp11 {

Outcome check_catalogue_params(const CatalogueParams& params) noexcept {
  if (params.kmax < 1 || params.kmax > static_cast<int>(catalogue_detail::kMaxOrder))
    return fail(Reason::kmax_out_of_range);
  if (params.max_leaf < 1 || params.max_leaf > catalogue_detail::kMaxLeaf ||
      params.leaf_size < static_cast<u32>(params.kmax) + 3 || params.leaf_size > params.max_leaf ||
      params.ball_limit < 1 || params.ball_limit > kNone)
    return fail(Reason::parameter_out_of_range);
  // Part de l'hote : pour mille, et seulement avec un lot de feuilles (option sans objet ailleurs : refusee).
  if (params.split_host_permille > 1000 ||
      (params.split_host_permille != 0 && !params.batch_leaves && !params.cuda_leaves))
    return fail(Reason::parameter_out_of_range);
  return {};
}

Result<Catalogue> build_catalogue(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget) noexcept {
  MHGP11_TRY(check_catalogue_params(params));
  if (params.single_pass) return fail(Reason::parameter_out_of_range);
  if (cloud.sites() == 0) return fail(Reason::empty_input);
  for (u32 weight : cloud.w())
    if (weight != 1) return fail(Reason::multiplicity_unsupported);
  return catalogue_detail::Assembly::build(cloud, params, budget);
}

namespace catalogue_detail {

Result<num::Point> point(const Cloud& cloud, SiteIdx site) noexcept {
  const u32 i = idx(site);
  if (i >= cloud.sites()) return fail(Reason::catalogue_invariant);
  auto result = num::Point::make(cloud.x()[i], cloud.y()[i], cloud.z()[i]);
  if (!result.ok()) return fail(Reason::catalogue_invariant);
  return result.value();
}

Outcome workspace_memory_bound(u32 capacity, u32 workers, bool cache_center_lines, u64& bytes,
                               bool pair_graph) noexcept {
  bytes = 0;
  const u64 words = (u64(capacity) + 63) / 64;
  MHGP11_TRY(add_bytes<num::Point>(bytes, u64(capacity) * workers));
  MHGP11_TRY(add_bytes<u64>(bytes, 2 * u64(capacity) * words * workers));
  MHGP11_TRY(add_bytes<SiteIdx>(bytes, 2 * u64(capacity) * workers));
  MHGP11_TRY(add_bytes<u64>(bytes, pair_graph ? u64(SmallPairGraph::kCapacity) * workers : 0));
  return add_bytes<u8>(bytes, cache_center_lines ? u64(CenterLineCache::entries(capacity)) * workers : 0);
}

Outcome Workspace::allocate(u32 capacity, MemoryBudget& budget, bool cache_center_lines, bool pair_graph) noexcept {
  const u64 words = (u64(capacity) + 63) / 64;
  u64 bytes = 0;
  MHGP11_TRY(add_bytes<num::Point>(bytes, capacity));
  MHGP11_TRY(add_bytes<u64>(bytes, 2 * u64(capacity) * words));
  MHGP11_TRY(add_bytes<SiteIdx>(bytes, 2 * u64(capacity)));
  const u32 cache_entries = cache_center_lines ? CenterLineCache::entries(capacity) : 0;
  MHGP11_TRY(add_bytes<u8>(bytes, cache_entries));
  const u32 pair_entries = pair_graph ? SmallPairGraph::kCapacity : 0;
  MHGP11_TRY(add_bytes<u64>(bytes, pair_entries));
  MHGP11_TRY(budget.admit(bytes));
  MHGP11_TRY(points.allocate(capacity, budget));
  MHGP11_TRY(dominance.allocate(u64(capacity) * words, budget));
  MHGP11_TRY(dominated.allocate(u64(capacity) * words, budget));
  MHGP11_TRY(interior.allocate(capacity, budget));
  MHGP11_TRY(shell.allocate(capacity, budget));
  MHGP11_TRY(center_lines.allocate(cache_entries, budget));
  return pair_rows.allocate(pair_entries, budget);
}

Outcome Collector::accept(const CatalogueBall& ball, const num::Level& level, std::span<const SiteIdx> interior,
                           std::span<const SiteIdx> shell, const CatalogueParams& params) noexcept {
  u64 next_balls = balls, next_pop = incidences;
  MHGP11_TRY(checked_add(next_balls, 1));
  MHGP11_TRY(checked_add(next_pop, interior.size()));
  MHGP11_TRY(checked_add(next_pop, shell.size()));
  if (next_balls >= params.ball_limit) return fail(Reason::index_overflow_u32);
  if (next_pop > Buffer<SiteIdx>::kMaxCount) return fail(Reason::memory_budget);
  if (stream != nullptr) {
    if (stream_budget == nullptr || filling) return fail(Reason::catalogue_invariant);
    MHGP11_TRY(stream->append(ball, level, interior, shell, *stream_budget));
  } else if (filling) {
    if (next_balls > records.size() || next_pop > population.size()) return fail(Reason::catalogue_invariant);
    records[balls] = Emission{ball, level, incidences};
    std::copy(interior.begin(), interior.end(), population.begin() + incidences);
    std::copy(shell.begin(), shell.end(), population.begin() + incidences + interior.size());
  }
  balls = next_balls;
  incidences = next_pop;
  return {};
}

namespace {
template <class Ball>
bool center_in_box_impl(const Ball& sphere, const Box& box) noexcept {
  static_assert(5 * kCoordBits + 6 <= 127, "catalogue : N+D(a-borne) en i128");
  const auto a = sphere.anchor().coordinates();
  const i128 d = sphere.denominator();
  for (int axis = 0; axis < 3; ++axis) {
    const i128 n = sphere.numerator()[axis];
    const i128 lower = n + d * (i128(a[axis]) - box.lo[axis]);
    const i128 upper = n + d * (i128(a[axis]) - box.hi[axis]);
    if (lower < 0 || upper >= 0) return false;
  }
  return true;
}

}  // namespace

bool center_in_box(const num::Sphere& sphere, const Box& box) noexcept {
  return center_in_box_impl(sphere, box);
}

bool center_in_box(const num::Q3Candidate& sphere, const Box& box) noexcept {
  return center_in_box_impl(sphere, box);
}

bool center_in_box(const num::Q4Candidate& sphere, const Box& box) noexcept {
  return center_in_box_impl(sphere, box);
}

}  // namespace catalogue_detail
}  // namespace mhgp11
