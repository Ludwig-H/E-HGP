#pragma once

#include "lanes/q4_center_map.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace mhgp9::gen {

// Center cells are dyadic squares of the bisector plane, in the integer
// parametrization (alpha, beta) scaled by 2^scale_bits. The root cell
// [-2,2]^2 can be halved max_depth times while keeping integer corners; the
// atlas refines to depth<=7, so this scale keeps every block/point bound of
// the partition in i64 (proofs in q4_local_partition.cpp, M=262143 at 18
// bits) instead of the former 2^44 in i128. Real cells, hence every
// classification and emission, are unchanged.
struct Q4LocalCell {
  static constexpr unsigned scale_bits = 20;
  static constexpr i64 scale = i64{1} << scale_bits;
  static constexpr unsigned max_depth = 20;
  i64 left{-2*scale}, right{2*scale}, bottom{-2*scale}, top{2*scale};
  unsigned depth{};
  bool owns_right{true}, owns_top{true};
  bool operator==(const Q4LocalCell&) const = default;
};
struct Q4LocalForm { i64 constant{}, x{}, y{}; };
// Exact rational point of the center plane in UNSCALED cell coordinates
// (alpha, beta) = (x, y) / den with den > 0: the cell [left, right] contains
// it iff left <= scale * x / den <= right (same for beta). A q3 circumcenter
// has |x|, |y|, den < 2^117 at 18 bits: the atlas locates it by exact long
// division (q4_local.cpp), never by forming scale * x or corner * den.
struct Q4LocalCenter { i128 x{}, y{}, den{}; };
struct Q4LocalBounds { i128 minimum{}, maximum{}; };
struct Q4LocalGeometryQueryWork {
  u64 disk_tests{}, facet_tests{};
  bool operator==(const Q4LocalGeometryQueryWork&) const = default;
};
struct Q4LocalGeometryWork {
  Q4PositiveDomainWork domain;
  u64 preparations{}, cover_node_visits{}, cover_range_advances{};
  u64 cover_disjoint_nodes{}, cover_splits{}, cover_blocks{};
  u64 cover_sites{}, cover_excluded_sites{}, cover_node_ids_copied{};
  u64 projection_points{}, hull_sort_comparisons{}, hull_orientation_tests{};
  u64 hull_vertices{}, facets{}, peak_retained_bytes{};
};
struct Q4LocalPartitionWork {
  u64 root_factories{}, child_factories{}, refine_factories{}, input_nodes{}, input_sites{};
  u64 inherited_inside_sites{}, node_visits{}, block_bound_tests{}, point_tests{};
  u64 z_splits{}, inside_nodes{}, outside_nodes{}, inside_sites{}, outside_sites{};
  u64 active_nodes{}, active_sites{}, budget_unexamined_nodes{}, budget_ambiguous_nodes{};
  u64 frontier_ids_copied{}, peak_retained_bytes{};
  bool operator==(const Q4LocalPartitionWork&) const = default;
};

class Q4LocalGeometry;
class Q4LocalFragment;
class Q4LocalPartitionResult;
using Q4LocalGeometryPtr = std::shared_ptr<const Q4LocalGeometry>;
using Q4LocalFragmentPtr = std::shared_ptr<const Q4LocalFragment>;

// Owned immutable edge context. Explicit requalification of tranche27's
// integer center-plane geometry; this is NOT its compressed rejection cache.
// Decomposes the cover ranges into disjoint certified spatial node IDs ONCE.
// No coordinate/point-ID copy, scalar n-site preparation or per-site form list.
class Q4LocalGeometry final {
 public:
  [[nodiscard]] static Q4LocalGeometryPtr make(Q34EdgeCoverPtr cover, Q4CenterDomainMode mode);
  Q4LocalGeometry(const Q4LocalGeometry&) = delete;
  Q4LocalGeometry& operator=(const Q4LocalGeometry&) = delete;
  Q4LocalGeometry(Q4LocalGeometry&&) = delete;
  Q4LocalGeometry& operator=(Q4LocalGeometry&&) = delete;
  [[nodiscard]] const Q34EdgeCoverPtr& cover() const noexcept { return cover_; }
  [[nodiscard]] std::span<const std::size_t> cover_nodes() const noexcept { return cover_nodes_; }
  [[nodiscard]] Q4LocalForm form(std::size_t original_id) const;
  // Circumcenter of the q3 seed (a, b, x): the point of x's center line that
  // minimizes the real radius, i.e. the projection of the midpoint onto the
  // line in the real metric of the (non-orthogonal) cell basis. Exact in
  // i128 (|x|,|y| < 2^117, den < 2^117, proof in the .cpp). Requires x off
  // the line ab (any strictly acute seed): a zero determinant throws.
  [[nodiscard]] Q4LocalCenter q3_center(std::size_t x_id) const;
  // Bounds of scale*L on the CLOSED cell, regardless of emission ownership.
  // Public forms must satisfy |constant|<=15M^2, |x|,|y|<=8M^2,
  // M=coordinate_limit; larger/forged values are rejected before arithmetic.
  [[nodiscard]] Q4LocalBounds bounds(Q4LocalForm form, Q4LocalCell cell) const;
  // Bounds of scale*L on INDEX-node-box x CLOSED cell. Validates the node
  // ID and cell before use. The minimum is rounded down from its continuous
  // quadratic relaxation; it is not the exact discrete-site minimum.
  [[nodiscard]] Q4LocalBounds node_bounds(std::size_t node_id, Q4LocalCell cell) const;
  [[nodiscard]] bool outside(Q4LocalCell cell, Q4LocalGeometryQueryWork& work) const;
  [[nodiscard]] const Q4LocalGeometryWork& work() const noexcept { return work_; }
  // Own dynamic capacities only, excluding fixed objects/shared ownership.
  [[nodiscard]] std::size_t retained_bytes() const;

 private:
  friend class Q4LocalFragment;
  friend struct Q4LocalEngine;
  friend struct Q4SeedCellEngine;
  using Vec = std::array<i64,3>;
  struct Facet { Vec normal{}; i128 height{}; };
  explicit Q4LocalGeometry(Q34EdgeCoverPtr cover, Q4CenterDomainMode mode);
  void decompose_cover();
  void prepare_hull(const Box3& box);
  // Only genuine forms from this certified geometry and owned atlas cells.
  [[nodiscard]] Q4LocalBounds bounds_unchecked(Q4LocalForm form, Q4LocalCell cell) const;
  [[nodiscard]] Q4LocalBounds node_bounds_unchecked(std::size_t node_id, Q4LocalCell cell) const;

  Q34EdgeCoverPtr cover_;
  Q4PositiveDomainPtr domain_;
  Vec v_{}, a_basis_{}, b_basis_{}, midpoint_twice_{};
  i64 diameter_squared_{}, gram_aa_{}, gram_ab_{}, gram_bb_{};
  std::size_t axis_i_{}, axis_j_{};
  std::array<Facet,9> facets_{};
  std::size_t facet_count_{};
  std::vector<std::size_t> cover_nodes_;
  Q4LocalGeometryWork work_{};
};

// Exact partition of the SAME cover on one CLOSED cell. For every center t
// in the cell, depth_cover(t)=inside_count()+sum_active[L_z(t)<0], and the
// ENTIRE shell consists of active sites with L_z(t)==0. Active nodes form a
// disjoint frontier in increasing spatial rank; IDs here name INDEX NODES,
// never original sites. min==0 MUST remain active. Counts never saturate.
// a,b may remain active, and can never be credited or discarded as exterior.
//
// A child inherits the exact count and classifies only its parent's frontier.
// Budgets limit geometric tests, NOT coverage: all unexamined/ambiguous blocks
// remain active. budget0 retains the complete input frontier. A fragment is
// not a compressed DEEP certificate, and its count alone cannot return IDs
// of uniformly interior points. No q3 consequence or global transfer is implied.
//
// Construction/queries require no mutable shared state. Allocation/counter
// failure publishes no partial fragment and leaves parent/geometry unchanged.
class Q4LocalFragment final {
 public:
  [[nodiscard]] static Q4LocalFragmentPtr root(Q4LocalGeometryPtr geometry, u64 z_test_budget);
  // Quadrant bit0 chooses right, bit1 top. Split ties belong right/top;
  // external right/top ownership is inherited. max_depth cannot be split.
  [[nodiscard]] static Q4LocalFragmentPtr child(Q4LocalFragmentPtr parent, unsigned quadrant,
                                               u64 z_test_budget);
  // Continue classification on the SAME closed cell, inheriting its exact
  // count/frontier. Does not split centers or change emission ownership.
  // budget0 is a valid identity partition; failure leaves parent unchanged.
  [[nodiscard]] static Q4LocalFragmentPtr refine(Q4LocalFragmentPtr parent, u64 z_test_budget);
  // Explicit alternative: either an EXACT fragment as above, or a terminal
  // lower-bound certificate. Threshold>=1; a saturated prefix is NEVER
  // published as Q4LocalFragment and cannot be supplied to child/refine/sweep.
  [[nodiscard]] static Q4LocalPartitionResult root_until(
      Q4LocalGeometryPtr geometry, u64 z_test_budget, std::size_t threshold);
  [[nodiscard]] static Q4LocalPartitionResult child_until(
      Q4LocalFragmentPtr parent, unsigned quadrant, u64 z_test_budget, std::size_t threshold);
  [[nodiscard]] static Q4LocalPartitionResult refine_until(
      Q4LocalFragmentPtr parent, u64 z_test_budget, std::size_t threshold);
  Q4LocalFragment(const Q4LocalFragment&) = delete;
  Q4LocalFragment& operator=(const Q4LocalFragment&) = delete;
  Q4LocalFragment(Q4LocalFragment&&) = delete;
  Q4LocalFragment& operator=(Q4LocalFragment&&) = delete;
  [[nodiscard]] const Q4LocalGeometryPtr& geometry() const noexcept { return geometry_; }
  [[nodiscard]] const Q4LocalCell& cell() const noexcept { return cell_; }
  [[nodiscard]] std::size_t inside_count() const noexcept { return inside_count_; }
  [[nodiscard]] std::span<const std::size_t> active_nodes() const noexcept { return active_nodes_; }
  [[nodiscard]] std::size_t active_sites() const noexcept { return active_sites_; }
  [[nodiscard]] const Q4LocalPartitionWork& work() const noexcept { return work_; }
  // Output frontier capacity only; no DFS stack or hidden site lists. This
  // excludes fixed metadata, shared objects and allocator reallocation overlap.
  [[nodiscard]] std::size_t retained_bytes() const;

  enum class Origin : unsigned { Root = 0, Child = 1, Refine = 2 };
  // Passkey: only the three factories can name Key, so std::make_shared can
  // build the fragment and its control block in ONE allocation while the
  // constructor stays private in effect. Never call this directly.
  struct Key { private: explicit Key() = default; friend class Q4LocalFragment; };
  Q4LocalFragment(Key, Q4LocalGeometryPtr geometry, Q4LocalCell cell, std::size_t inherited,
                  std::span<const std::size_t> input, u64 budget, Origin origin,
                  std::size_t stop_after = 0);

 private:
  [[nodiscard]] static Q4LocalPartitionResult finish_until(
      std::shared_ptr<Q4LocalFragment> fragment, std::size_t input_sites);
  void retain(std::size_t node_id);
  Q4LocalGeometryPtr geometry_;
  Q4LocalCell cell_;
  std::size_t inside_count_{}, active_sites_{};
  std::vector<std::size_t> active_nodes_;
  Q4LocalPartitionWork work_{};
  bool saturated_{};
};

// Factory-only tagged union: exact_fragment()!=nullptr XOR saturated().
// Prefix work records ACTUALLY visited/classified nodes, not the unvisited
// input population. It does not satisfy a complete-partition ledger when
// saturated. unvisited_sites() is a population, never "tests saved".
class Q4LocalPartitionResult final {
 public:
  // Value copies preserve both the tag and its immutable owner. A user-
  // declared copy constructor suppresses destructive implicit moves: even
  // std::move(result) copies the shared pointers and leaves result valid.
  Q4LocalPartitionResult(const Q4LocalPartitionResult&) = default;
  Q4LocalPartitionResult& operator=(const Q4LocalPartitionResult&) = delete;
  [[nodiscard]] bool saturated() const noexcept { return !exact_; }
  [[nodiscard]] const Q4LocalGeometryPtr& geometry() const noexcept { return geometry_; }
  [[nodiscard]] const Q4LocalFragmentPtr& exact_fragment() const noexcept { return exact_; }
  [[nodiscard]] std::size_t certified_depth() const noexcept { return depth_; }
  [[nodiscard]] const Q4LocalCell& cell() const noexcept { return cell_; }
  [[nodiscard]] const Q4LocalPartitionWork& work() const noexcept { return work_; }
  [[nodiscard]] std::size_t unvisited_sites() const noexcept { return unvisited_; }
  // Capacity alive in the discarded temporary, not current retained bytes.
  [[nodiscard]] std::size_t temporary_bytes() const noexcept { return temporary_; }
 private:
  friend class Q4LocalFragment;
  Q4LocalPartitionResult(Q4LocalGeometryPtr geometry, Q4LocalFragmentPtr exact, std::size_t depth, Q4LocalCell cell,
      Q4LocalPartitionWork work, std::size_t unvisited, std::size_t temporary)
      : geometry_(std::move(geometry)), exact_(std::move(exact)), depth_(depth), cell_(cell), work_(work),
        unvisited_(unvisited), temporary_(temporary) {}
  Q4LocalGeometryPtr geometry_;
  Q4LocalFragmentPtr exact_;
  std::size_t depth_{};
  Q4LocalCell cell_;
  Q4LocalPartitionWork work_;
  std::size_t unvisited_{}, temporary_{};
};

}  // namespace mhgp9::gen
