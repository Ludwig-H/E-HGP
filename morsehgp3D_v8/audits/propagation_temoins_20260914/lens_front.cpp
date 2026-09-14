// Auditeur B (14 sept. 2026) — COPIE INSTRUMENTEE de morsehgp3D_v8/src/wspd/front.cpp
// (da366f7f). Prototype d audit, pas un moteur : (1) test de lentille exact
// (max_z h_min(A,B,{z}) > 0, rationnels) compte les recherches de temoins sans
// espoir ; (2) propagation des temoins certifies du parent vers ses enfants,
// par voie et par rang distinct (surete par restriction des boites) ; (3) mode
// blocks : le long de la descente du proposeur, chaque bloc Z frere du chemin
// est teste par les bornes exactes de trois boites (h_minimum, xi_bounds) et
// credite comme plage de rangs disjointe ; les rangs deja comptes a l'interieur
// d'un bloc credite ne sont jamais recomptes (max, pas somme emboitee). Le diff
// avec la source constructeur est conserve dans le recu.
#include "wspd/front.hpp"

#include "spindle/predicates.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <vector>
#include <cstdlib>

#include <boost/multiprecision/cpp_int.hpp>
namespace mhgp8 {
struct LensStats {
  u64 lens_tests{}, lens_empty{}, lens_empty_skipped_searches{}, lens_nonempty_searches{};
  u64 lens_empty_but_rejected{}, lens_nonempty_no_rejection{}, lens_nonempty_full_rejection{}, lens_nonempty_partial_rejection{};
  u64 lens_empty_leaf_pairs{}, lens_candidates{};
  u64 inherited_nonzero_products{}, rejections_by_inheritance{}, duplicate_proposals{};
  u64 block_tests{}, block_credits{}, block_credit_population{}, block_overlaps_dropped{}, samples_inside_blocks{};
};
namespace lensaudit {
using i128 = mhgp8::i128;
struct Frac { i128 num; i128 den; };  // den > 0
// exact max over real z of g(z) = min over corners (a in {al,ah}, b in {bl,bh}) of (z-a)(b-z)
static Frac axis_lens_max(i64 al, i64 ah, i64 bl, i64 bh, u64& candidates) {
  const i64 A[2] = {al, ah}; const i64 B[2] = {bl, bh};
  struct P { i64 a, b; }; P ps[4]; int np = 0;
  for (int i = 0; i < 2; ++i) for (int j = 0; j < 2; ++j) ps[np++] = {A[i], B[j]};
  // candidate z = p/q
  struct Z { i128 p; i128 q; }; Z zs[10]; int nz = 0;
  for (int k = 0; k < 4; ++k) zs[nz++] = {(i128)ps[k].a + ps[k].b, 2};
  for (int k = 0; k < 4; ++k) for (int l = k + 1; l < 4; ++l) {
    const i128 den = ((i128)ps[k].a + ps[k].b) - ((i128)ps[l].a + ps[l].b);
    if (den == 0) continue;
    i128 num = (i128)ps[k].a * ps[k].b - (i128)ps[l].a * ps[l].b;
    if (den < 0) { zs[nz++] = {-num, -den}; } else { zs[nz++] = {num, den}; }
  }
  Frac best{0, 1}; bool have = false;
  for (int c = 0; c < nz; ++c) {
    ++candidates;
    const i128 p = zs[c].p, q = zs[c].q;
    i128 m = 0; bool first = true;
    for (int k = 0; k < 4; ++k) {
      const i128 v = (p - (i128)ps[k].a * q) * ((i128)ps[k].b * q - p);  // value * q^2
      if (first || v < m) { m = v; first = false; }
    }
    const i128 d = q * q;
    if (!have || m * best.den > best.num * d) { best = {m, d}; have = true; }
  }
  return best;
}
static bool lens_nonempty(const Box3& a, const Box3& b, u64& candidates) {
  using boost::multiprecision::cpp_int;
  cpp_int sum = 0; cpp_int den = 1;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const Frac f = axis_lens_max(a.low[axis], a.high[axis], b.low[axis], b.high[axis], candidates);
    sum = sum * cpp_int(f.den) + cpp_int(f.num) * den;
    den = den * cpp_int(f.den);
  }
  return sum > 0;
}
}  // namespace lensaudit
namespace {

u64 product(std::size_t left, std::size_t right) {
  const auto value = static_cast<i128>(left) * right;
  if (value > std::numeric_limits<u64>::max()) {
    throw std::overflow_error("mhgp8 WSPD pair mass exceeds u64");
  }
  return static_cast<u64>(value);
}

i64 diagonal2(const Box3& box) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = static_cast<i64>(box.high[axis]) - box.low[axis];
    result += delta * delta;
  }
  return result;
}

i64 gap2(const Box3& a, const Box3& b) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = std::max<i64>({0, static_cast<i64>(a.low[axis]) - b.high[axis],
                                      static_cast<i64>(b.low[axis]) - a.high[axis]});
    result += delta * delta;
  }
  return result;
}

bool contains(Range range, std::size_t rank) {
  return range.first <= rank && rank < range.last;
}

struct Task {
  std::size_t a;
  std::size_t b;
  std::uint8_t mask;
  u64 depth;
  std::array<std::uint8_t, 3> inherited{};            // certified witnesses inherited per lane
  std::array<std::array<std::uint32_t, 10>, 3> ranks{};  // their spatial ranks (distinct per lane)
  std::array<std::uint8_t, 3> nblocks{};                 // certified disjoint Z blocks per lane (rank ranges)
  std::array<std::array<std::array<std::uint32_t, 2>, 4>, 3> blocks{};
};

class Front {
 public:
  LensStats* lens_{};
  bool skip_when_empty_{};
  Front(const Q2CensusIndex& index, unsigned kmax, unsigned separation,
        WspdFrontMode mode, const WspdRectangleConsumer& consumer, LensStats* lens, bool skip, bool propagate, bool do_lens, bool blocks)
      : lens_(lens), skip_when_empty_(skip), propagate_(propagate), do_lens_(do_lens), blocks_(blocks), block_gate_(std::getenv("MHGP8_BLOCK_GATE") ? std::atoll(std::getenv("MHGP8_BLOCK_GATE")) : 0), block_slack_(std::getenv("MHGP8_BLOCK_SLACK") ? std::atoll(std::getenv("MHGP8_BLOCK_SLACK")) : 0), nodes_(index.spatial_nodes()), order_(index.spatial_order()),
        points_(index.cloud().points()), kmax_(kmax), separation_(separation),
        mode_(mode), consumer_(consumer) {
    const auto n = points_.size();
    // Divide before multiplying, so even a representable choose(n,2)
    // does not require the larger ordered-pair count to fit u64.
    result_.total_unordered_pairs = n % 2 == 0 ? product(n / 2, n - 1)
                                               : product(n, (n - 1) / 2);
    for (unsigned lane = 0; lane < 3; ++lane) {
      if (lane < kmax_) {
        thresholds_[lane] = kmax_ - lane;
        result_.active_lane_mask |= static_cast<std::uint8_t>(1U << lane);
      }
    }
    // A reservation, not an exploration cap. The u16 product path has
    // at most 96 coordinate halvings; no result is truncated if it grows.
    stack_.reserve(97);
  }

  WspdFrontResult run() {
    push({0, 0, result_.active_lane_mask, 0});
    while (!stack_.empty()) {
      const auto task = stack_.back();
      stack_.pop_back();
      counter_add(result_.work.product_visits);
      result_.work.max_product_depth = std::max(result_.work.max_product_depth, task.depth);
      const auto& a = nodes_[task.a];
      const auto& b = nodes_[task.b];
      if (task.a == task.b) {
        if (a.left == Q2SpatialNode::absent) {
          counter_add(result_.work.diagonal_leaves);
        } else {
          counter_add(result_.work.diagonal_splits);
          // LCA decomposition: LL, LR and RR partition unordered pairs.
          push({a.right, a.right, task.mask, task.depth + 1});
          push({a.left, a.right, task.mask, task.depth + 1});
          push({a.left, a.left, task.mask, task.depth + 1});  // diagonal: nothing inherited (never searched)
        }
        continue;
      }
      auto mask = task.mask;
      found_inherited_ = task.inherited; found_ranks_ = task.ranks; found_nblocks_ = task.nblocks; found_blocks_ = task.blocks;
      if (propagate_) for (unsigned lane = 0; lane < 3; ++lane) if (task.inherited[lane] > 0) ++lens_->inherited_nonzero_products;
      if (mode_ == WspdFrontMode::MidpointSamples) {
        bool nonempty = true;
        if (lens_ && do_lens_) {
          ++lens_->lens_tests;
          nonempty = lensaudit::lens_nonempty(a.box, b.box, lens_->lens_candidates);
          if (!nonempty) { ++lens_->lens_empty; if (a.range.size() == 1 && b.range.size() == 1) ++lens_->lens_empty_leaf_pairs; }
        }
        if (!nonempty && skip_when_empty_) {
          ++lens_->lens_empty_skipped_searches;
        } else {
          const auto before = mask;
          mask = filter(a, b, mask);
          if (lens_) {
            if (!nonempty) { if (mask != before) ++lens_->lens_empty_but_rejected; }
            else { ++lens_->lens_nonempty_searches; if (mask == before) ++lens_->lens_nonempty_no_rejection; else if (mask == 0) ++lens_->lens_nonempty_full_rejection; else ++lens_->lens_nonempty_partial_rejection; }
          }
        }
      }
      const auto mass = product(a.range.size(), b.range.size());
      for (unsigned lane = 0; lane < 3; ++lane) {
        const auto bit = static_cast<std::uint8_t>(1U << lane);
        if ((task.mask & bit) != 0 && (mask & bit) == 0) {
          counter_add(result_.work.rejected_pair_mass[lane], mass);
        }
      }
      if (mask == 0) {
        counter_add(result_.work.fully_rejected_products);
        continue;
      }
      counter_add(result_.work.separation_tests);
      const auto da = diagonal2(a.box);
      const auto db = diagonal2(b.box);
      if (static_cast<i128>(gap2(a.box, b.box)) >=
          static_cast<i128>(separation_) * separation_ * std::max(da, db)) {
        emit(task.a, task.b, mask, mass);
        continue;
      }
      counter_add(result_.work.disjoint_splits);
      const bool split_a = a.left != Q2SpatialNode::absent &&
                          (b.left == Q2SpatialNode::absent || da >= db);
      const auto& split = split_a ? a : b;
      if (split.left == Q2SpatialNode::absent) {
        throw std::logic_error("mhgp8 WSPD distinct singleton boxes must be separated");
      }
      {
        Task child_r{split_a ? split.right : task.a, split_a ? task.b : split.right, mask, task.depth + 1};
        Task child_l{split_a ? split.left : task.a, split_a ? task.b : split.left, mask, task.depth + 1};
        if (propagate_) { child_r.inherited = found_inherited_; child_r.ranks = found_ranks_; child_l.inherited = found_inherited_; child_l.ranks = found_ranks_;
                          child_r.nblocks = found_nblocks_; child_r.blocks = found_blocks_; child_l.nblocks = found_nblocks_; child_l.blocks = found_blocks_; }
        push(child_r);
        push(child_l);
      }
    }
    for (unsigned lane = 0; lane < 3; ++lane) {
      const auto expected = (result_.active_lane_mask & (1U << lane)) != 0
                                ? result_.total_unordered_pairs : 0;
      auto accounted = result_.work.rejected_pair_mass[lane];
      counter_add(accounted, result_.work.residual_pair_mass[lane]);
      if (accounted != expected) throw std::logic_error("mhgp8 WSPD lane mass ledger failed");
    }
    return result_;
  }

 private:
  void push(Task task) {
    stack_.push_back(task);
    result_.work.max_stack_size = std::max(result_.work.max_stack_size,
                                          static_cast<u64>(stack_.size()));
  }

  i64 midpoint_distance4(const std::array<i64, 3>& center4, const Box3& box) {
    counter_add(result_.work.witness_box_distance_tests);
    i64 result = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto delta = std::max<i64>({0, 4 * static_cast<i64>(box.low[axis]) - center4[axis],
                                         center4[axis] - 4 * static_cast<i64>(box.high[axis])});
      result += delta * delta;
    }
    return result;  // 16 times squared distance; <=48*65535^2 fits i64.
  }

  bool propagate_{};
  bool do_lens_{true};
  bool blocks_{};
  i64 block_gate_{0};
  i64 block_slack_{0};
  std::array<std::uint8_t, 3> found_nblocks_{};
  std::array<std::array<std::array<std::uint32_t, 2>, 4>, 3> found_blocks_{};
  static bool in_blocks(const std::array<std::array<std::uint32_t, 2>, 4>& bl, unsigned nb, std::size_t rank) {
    for (unsigned i = 0; i < nb; ++i) if (rank >= bl[i][0] && rank < bl[i][1]) return true;
    return false;
  }
  static bool overlaps(const std::array<std::array<std::uint32_t, 2>, 4>& bl, unsigned nb, std::size_t f, std::size_t l) {
    for (unsigned i = 0; i < nb; ++i) if (f < bl[i][1] && bl[i][0] < l) return true;
    return false;
  }
  std::array<std::uint8_t, 3> found_inherited_{};
  std::array<std::array<std::uint32_t, 10>, 3> found_ranks_{};
  std::uint8_t filter(const Q2SpatialNode& a, const Q2SpatialNode& b, std::uint8_t mask) {
    unsigned needed = kmax_;
    for (unsigned lane = 0; lane < 3; ++lane) {
      if ((mask & (1U << lane)) != 0) needed = std::min(needed, thresholds_[lane]);
    }
    // Inherited witnesses (certified on the parent boxes) remain universal on child boxes.
    std::array<unsigned, 3> credits{};
    if (propagate_) {
      for (unsigned lane = 0; lane < 3; ++lane) {
        const auto bit = static_cast<std::uint8_t>(1U << lane);
        if ((mask & bit) == 0) continue;
        unsigned c = 0;
        for (unsigned i = 0; i < found_nblocks_[lane]; ++i) c += found_blocks_[lane][i][1] - found_blocks_[lane][i][0];
        for (unsigned i = 0; i < found_inherited_[lane]; ++i) if (!in_blocks(found_blocks_[lane], found_nblocks_[lane], found_ranks_[lane][i])) ++c;
        credits[lane] = c;
        if (credits[lane] >= thresholds_[lane]) { mask &= static_cast<std::uint8_t>(~bit); ++lens_->rejections_by_inheritance; }
      }
      if (mask == 0) return mask;
    }
    const auto try_block = [&](std::size_t node_id) {
      const auto& zn = nodes_[node_id];
      if (zn.range.size() < 2) return;  // singletons are handled as samples
      // The block must be outside both factors as a rank range.
      if (zn.range.first < a.range.last && a.range.first < zn.range.last) return;
      if (zn.range.first < b.range.last && b.range.first < zn.range.last) return;
      bool need_any = false;
      for (unsigned lane = 0; lane < 3; ++lane) if ((mask & (1U << lane)) != 0 && found_nblocks_[lane] < 4 && !overlaps(found_blocks_[lane], found_nblocks_[lane], zn.range.first, zn.range.last)) need_any = true;
      if (!need_any) return;
      ++lens_->block_tests;
      PredicateWork pw{};
      const auto hmin = spindle_detail::h_minimum(a.box, b.box, zn.box);
      if (hmin <= 0) return;
      i128 xih = 0; bool have_xi = false;
      for (unsigned lane = 0; lane < 3; ++lane) {
        const auto bit = static_cast<std::uint8_t>(1U << lane);
        if ((mask & bit) == 0 || found_nblocks_[lane] >= 4) continue;
        if (overlaps(found_blocks_[lane], found_nblocks_[lane], zn.range.first, zn.range.last)) { ++lens_->block_overlaps_dropped; continue; }
        bool ok = lane == 0;
        if (!ok) { if (!have_xi) { xih = spindle_detail::xi_bounds(a.box, b.box, zn.box).high; have_xi = true; } ok = (lane == 1 ? 3 : 2) * spindle_detail::square(hmin) > xih; }
        if (!ok) continue;
        ++lens_->block_credits; lens_->block_credit_population += zn.range.size();
        found_blocks_[lane][found_nblocks_[lane]++] = {static_cast<std::uint32_t>(zn.range.first), static_cast<std::uint32_t>(zn.range.last)};
        // ranks already counted individually inside this block must not be double counted
        unsigned inside = 0;
        for (unsigned i = 0; i < found_inherited_[lane]; ++i) if (found_ranks_[lane][i] >= zn.range.first && found_ranks_[lane][i] < zn.range.last) ++inside;
        credits[lane] += static_cast<unsigned>(zn.range.size()) - inside;
        if (credits[lane] >= thresholds_[lane]) mask &= static_cast<std::uint8_t>(~bit);
      }
      static_cast<void>(pw);
    };
    // A strict universal witness cannot belong to A or B: choose that
    // endpoint and H is zero. If too few exterior sites exist, skip search.
    if (points_.size() - a.range.size() - b.range.size() < needed) return mask;
    counter_add(result_.work.witness_searches);
    std::array<i64, 3> center4{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
      center4[axis] = static_cast<i64>(a.box.low[axis]) + a.box.high[axis] +
                     b.box.low[axis] + b.box.high[axis];
    }
    std::size_t node = 0;
    while (nodes_[node].left != Q2SpatialNode::absent) {
      counter_add(result_.work.witness_descent_steps);
      const auto left = nodes_[node].left;
      const auto right = nodes_[node].right;
      // Heuristic one-path proposal only, not an exact nearest-neighbor
      // query. Ties choose left; no hidden search of the other subtree.
      const auto left_distance = midpoint_distance4(center4, nodes_[left].box);
      const auto right_distance = midpoint_distance4(center4, nodes_[right].box);
      const auto chosen = left_distance <= right_distance ? left : right;
      if (blocks_ && propagate_) {
        const auto other = chosen == left ? right : left;
        const auto dc = chosen == left ? left_distance : right_distance;
        const auto dother = chosen == left ? right_distance : left_distance;
        // Gate: only siblings whose box is about as close to the midpoint as the chosen path (factor block_gate_).
        if (block_gate_ == 0 || dother <= block_gate_ * dc + block_slack_) { try_block(other); if (mask == 0) return mask; }
      }
      node = chosen;
    }
    const auto count = std::min<std::size_t>(kmax_, order_.size());
    const auto pivot = nodes_[node].range.first;
    const auto first = std::min(pivot > count / 2 ? pivot - count / 2 : 0,
                                order_.size() - count);
    const auto last = first + count;
    for (auto rank = first; rank < last && mask != 0; ++rank) {
      counter_add(result_.work.proposed_sites);
      if (contains(a.range, rank) || contains(b.range, rank)) {
        counter_add(result_.work.proposals_in_factors);
        continue;
      }
      std::array<bool, 3> already{};
      if (propagate_) {
        for (unsigned lane = 0; lane < 3; ++lane) {
          for (unsigned i = 0; i < found_inherited_[lane]; ++i) if (found_ranks_[lane][i] == rank) { already[lane] = true; ++lens_->duplicate_proposals; break; }
          if (!already[lane] && in_blocks(found_blocks_[lane], found_nblocks_[lane], rank)) { already[lane] = true; ++lens_->samples_inside_blocks; }
        }
      }
      const auto z = singleton_box(points_[order_[rank]]);
      counter_add(result_.work.h_bound_tests);
      const auto h = spindle_detail::h_minimum(a.box, b.box, z);
      if (h <= 0) continue;
      i128 xi = 0;
      if ((mask & 6U) != 0) {
        counter_add(result_.work.xi_bound_tests);
        xi = spindle_detail::xi_bounds(a.box, b.box, z).high;
      }
      const auto h2 = spindle_detail::square(h);
      for (unsigned lane = 0; lane < 3; ++lane) {
        const auto bit = static_cast<std::uint8_t>(1U << lane);
        if ((mask & bit) != 0 && !already[lane] && (lane == 0 || (lane == 1 ? 3 : 2) * h2 > xi)) {
          counter_add(result_.work.witness_lane_credits);
          if (propagate_ && found_inherited_[lane] < 10) { found_ranks_[lane][found_inherited_[lane]] = static_cast<std::uint32_t>(rank); ++found_inherited_[lane]; }
          if (++credits[lane] == thresholds_[lane]) mask &= static_cast<std::uint8_t>(~bit);
        }
      }
    }
    return mask;
  }

  void emit(std::size_t a_id, std::size_t b_id, std::uint8_t mask, u64 mass) {
    const auto na = nodes_[a_id].range.size();
    const auto nb = nodes_[b_id].range.size();
    const auto maximum = std::max(na, nb);
    auto& work = result_.work;
    counter_add(work.emitted_rectangles);
    counter_add(work.emitted_factor_sites, static_cast<u64>(na));
    counter_add(work.emitted_factor_sites, static_cast<u64>(nb));
    work.max_factor_size = std::max(work.max_factor_size, static_cast<u64>(maximum));
    const unsigned bin = maximum == 1 ? 0 : maximum < 8 ? 1 : maximum < 64 ? 2
                                                       : maximum < 1024 ? 3 : 4;
    counter_add(work.size_class_rectangles[bin]);
    counter_add(work.size_class_pair_mass[bin], mass);
    if (maximum == 1) counter_add(work.leaf_pair_rectangles);
    for (unsigned lane = 0; lane < 3; ++lane) {
      if ((mask & (1U << lane)) != 0) {
        counter_add(work.lane_rectangles[lane]);
        counter_add(work.residual_pair_mass[lane], mass);
      }
    }
    consumer_(WspdRectangle{a_id, b_id, mask});
  }

  std::span<const Q2SpatialNode> nodes_;
  std::span<const std::size_t> order_;
  std::span<const Point3> points_;
  unsigned kmax_;
  unsigned separation_;
  WspdFrontMode mode_;
  const WspdRectangleConsumer& consumer_;
  std::array<unsigned, 3> thresholds_{};
  WspdFrontResult result_;
  std::vector<Task> stack_;
};

}  // namespace

WspdFrontResult run_wspd_front_lens(const Q2CensusIndex& index, unsigned kmax,
                               unsigned separation_s, WspdFrontMode mode,
                               const WspdRectangleConsumer& consumer, LensStats* lens, bool skip, bool propagate, bool do_lens, bool blocks) {
  if (kmax == 0 || kmax > 10 || separation_s == 0 ||
      (mode != WspdFrontMode::Pure && mode != WspdFrontMode::MidpointSamples) || !consumer) {
    throw std::invalid_argument("mhgp8 WSPD requires Kmax1..10, positive s, valid mode and consumer");
  }
  return Front(index, kmax, separation_s, mode, consumer, lens, skip, propagate, do_lens, blocks).run();
}

}  // namespace mhgp8
