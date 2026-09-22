#include "wspd/front.hpp"

#include "spindle/predicates.hpp"

#include <algorithm>
#include <array>
#include <condition_variable>
#include <deque>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace mhgp9::gen {
namespace {

u64 product(std::size_t left, std::size_t right) {
  const auto value = static_cast<i128>(left) * right;
  if (value > std::numeric_limits<u64>::max()) {
    throw std::overflow_error("mhgp9 gen WSPD pair mass exceeds u64");
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

// What a search leaves to the two children of its product. Carried BY VALUE in
// the task and never in the front object: the work of a product depends on its
// task alone, whatever the traversal order, the job plan or the worker.
struct WitnessList {
  // Ranks of the certified q2 witnesses: at most Kmax-1 <= 9, all distinct.
  std::array<std::uint32_t, 9> ranks{};
  std::uint8_t count{};
};
// wspd_proposals_fit admits at most 2^32 sites: a stored rank is an exact 32-bit copy.
static_assert(std::is_same_v<decltype(WitnessList::ranks)::value_type, std::uint32_t>);

struct Task {
  std::size_t a{};
  std::size_t b{};
  u64 depth{};
  // An index path has at most max_index_depth (54) coordinate halvings. A
  // product path has at most 108 levels and adds at most two pending siblings per level.
  // This represents a proved bound, not a limit on exploration.
  std::uint32_t dfs_pending{};
  std::uint8_t mask{};
  bool terminal{};
  // Last field: the six-value initializers above it keep their meaning, and a
  // task built without a list has an empty list, never a shifted field.
  WitnessList witnesses{};
};

class Front {
 public:
  Front(const Q2CensusIndex& index, unsigned kmax, unsigned separation,
        WspdFrontMode mode, const WspdRectangleConsumer& consumer, std::uint8_t requested_mask,
        WspdFrontProposals proposals)
      : nodes_(index.spatial_nodes()), order_(index.spatial_order()),
        points_(index.cloud().points()), kmax_(kmax), separation_(separation),
        mode_(mode), consumer_(consumer), proposals_(proposals) {
    const auto n = points_.size();
    if (!wspd_proposals_fit(proposals_, order_.size()))
      throw std::length_error("mhgp9 gen WSPD inherited witness ranks are stored on 32 bits");
    // Divide before multiplying, so even a representable choose(n,2)
    // does not require the larger ordered-pair count to fit u64.
    result_.total_unordered_pairs = n % 2 == 0 ? product(n / 2, n - 1)
                                               : product(n, (n - 1) / 2);
    for (unsigned lane = 0; lane < 3; ++lane) {
      if (lane < kmax_ && (requested_mask & (1U << lane)) != 0) {
        thresholds_[lane] = kmax_ - lane;
        result_.active_lane_mask |= static_cast<std::uint8_t>(1U << lane);
      }
    }
  }

  [[nodiscard]] Task root_task() const { return {0, 0, 0, 0, result_.active_lane_mask, false}; }
  [[nodiscard]] const WspdFrontResult& result() const { return result_; }

  // Exactly one formerly unvisited product. Children are pushed in reverse
  // canonical DFS order. Inlinable sinks avoid constructing/returning a
  // three-task array on this hot path. A terminal has already paid every
  // front test and emission counter before entering its separate sink.
  template<class Push, class Emit>
  void expand(const Task& task, Push&& push, Emit&& emit) {
    if (task.terminal) throw std::logic_error("mhgp9 gen WSPD terminal cannot be tested twice");
    counter_add(result_.work.product_visits);
    result_.work.max_product_depth = std::max(result_.work.max_product_depth, task.depth);
    result_.work.max_stack_size = std::max(result_.work.max_stack_size, static_cast<u64>(task.dfs_pending) + 1);
    const auto& a = nodes_[task.a];
    const auto& b = nodes_[task.b];
    if (task.a == task.b) {
      // Every ancestor of a diagonal product is diagonal and searched nothing.
      if (task.witnesses.count != 0) throw std::logic_error("mhgp9 gen WSPD diagonal product received witnesses");
      if (a.left == Q2SpatialNode::absent) {
        counter_add(result_.work.diagonal_leaves);
        return;
      }
      counter_add(result_.work.diagonal_splits);
      // LCA decomposition: LL, LR and RR partition unordered pairs. The
      // inherited pending count reconstructs mono stack high-water exactly
      // even when these children are prepared/replayed in another order.
      result_.work.max_stack_size = std::max(result_.work.max_stack_size, static_cast<u64>(task.dfs_pending) + 3);
      push(Task{a.right, a.right, task.depth + 1, task.dfs_pending, task.mask, false});
      push(Task{a.left, a.right, task.depth + 1, task.dfs_pending + 1, task.mask, false});
      push(Task{a.left, a.left, task.depth + 1, task.dfs_pending + 2, task.mask, false});
      return;
    }
    auto mask = task.mask;
    // A private copy: the search reads the received list and writes the list
    // of THIS product into it. No member of the front ever holds a list.
    WitnessList witnesses = task.witnesses;
    if (mode_ == WspdFrontMode::MidpointSamples) mask = filter(a, b, mask, witnesses);
    const auto mass = product(a.range.size(), b.range.size());
    for (unsigned lane = 0; lane < 3; ++lane) {
      const auto bit = static_cast<std::uint8_t>(1U << lane);
      if ((task.mask & bit) != 0 && (mask & bit) == 0)
        counter_add(result_.work.rejected_pair_mass[lane], mass);
    }
    if (mask == 0) {
      counter_add(result_.work.fully_rejected_products);
      return;
    }
    counter_add(result_.work.separation_tests);
    const auto da = diagonal2(a.box);
    const auto db = diagonal2(b.box);
    if (static_cast<i128>(gap2(a.box, b.box)) >=
        static_cast<i128>(separation_) * separation_ * std::max(da, db)) {
      account_emit(task.a, task.b, mask, mass);
      // Final q2 credits, received and new, of a searched product that is
      // emitted. With the rejections and the splits they close the credit
      // ledger of the inheritance; the list is empty unless inherit_witnesses.
      if (witnesses.count != 0) counter_add(result_.work.emitted_witness_credits, witnesses.count);
      emit(Task{task.a, task.b, task.depth, task.dfs_pending, mask, true});
      return;
    }
    counter_add(result_.work.disjoint_splits);
    const bool split_a = a.left != Q2SpatialNode::absent &&
                        (b.left == Q2SpatialNode::absent || da >= db);
    const auto& split = split_a ? a : b;
    if (split.left == Q2SpatialNode::absent)
      throw std::logic_error("mhgp9 gen WSPD distinct singleton boxes must be separated");
    result_.work.max_stack_size = std::max(result_.work.max_stack_size, static_cast<u64>(task.dfs_pending) + 2);
    // Both children receive the same list, by value.
    push(Task{split_a ? split.right : task.a, split_a ? task.b : split.right,
              task.depth + 1, task.dfs_pending, mask, false, witnesses});
    push(Task{split_a ? split.left : task.a, split_a ? task.b : split.left,
              task.depth + 1, task.dfs_pending + 1, mask, false, witnesses});
  }

  [[nodiscard]] WspdFrontResult run(Task initial) {
    if (initial.terminal) {
      consumer_(WspdRectangle{initial.a, initial.b, initial.mask});
      return result_;
    }
    // A reservation, not an exploration cap. No result is truncated.
    std::vector<Task> stack;
    stack.reserve(2 * max_index_depth + 1);
    stack.push_back(initial);
    while (!stack.empty()) {
      const auto task = stack.back();
      stack.pop_back();
      expand(task, [&](const Task& child) { stack.push_back(child); },
             [&](const Task& terminal) { consumer_(WspdRectangle{terminal.a, terminal.b, terminal.mask}); });
    }
    return result_;
  }

  WspdFrontResult run() {
    static_cast<void>(run(root_task()));
    for (unsigned lane = 0; lane < 3; ++lane) {
      const auto expected = (result_.active_lane_mask & (1U << lane)) != 0
                                ? result_.total_unordered_pairs : 0;
      auto accounted = result_.work.rejected_pair_mass[lane];
      counter_add(accounted, result_.work.residual_pair_mass[lane]);
      if (accounted != expected) throw std::logic_error("mhgp9 gen WSPD lane mass ledger failed");
    }
    return result_;
  }

 private:
  i64 midpoint_distance4(const std::array<i64, 3>& center4, const Box3& box) {
    counter_add(result_.work.witness_box_distance_tests);
    i64 result = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto delta = std::max<i64>({0, 4 * static_cast<i64>(box.low[axis]) - center4[axis],
                                         center4[axis] - 4 * static_cast<i64>(box.high[axis])});
      result += delta * delta;
    }
    return result;  // 16 times squared distance; <=48*262143^2<2^42 fits i64.
  }

  std::uint8_t filter(const Q2SpatialNode& a, const Q2SpatialNode& b, std::uint8_t mask, WitnessList& witnesses) {
    unsigned needed = kmax_;
    for (unsigned lane = 0; lane < 3; ++lane) {
      if ((mask & (1U << lane)) != 0) needed = std::min(needed, thresholds_[lane]);
    }
    // A strict universal witness cannot belong to A or B: choose that
    // endpoint and H is zero. If too few exterior sites exist, skip search.
    if (points_.size() - a.range.size() - b.range.size() < needed) {
      // Inheritance runs the q2 lane alone: `needed` is Kmax for every product
      // and the exterior population grows strictly towards the children. A
      // skipped search therefore has only skipped or diagonal ancestors.
      if (witnesses.count != 0) throw std::logic_error("mhgp9 gen WSPD skipped search received witnesses");
      return mask;
    }
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
      node = left_distance <= right_distance ? left : right;
    }
    const auto count = std::min<std::size_t>(kmax_, order_.size());
    const auto pivot = nodes_[node].range.first;
    const auto window = wspd_proposal_window(pivot, count, order_.size());
    std::array<unsigned, 3> credits{};
    // The received ranks are strict universal witnesses of this product too,
    // outside A and B (h_minimum is an exact minimum and the boxes shrink).
    // They count once each and are not credits of this run. validate_front
    // guarantees the q2 lane alone.
    const unsigned inherited = proposals_.inherit_witnesses ? witnesses.count : 0U;
    if (inherited != 0) {
      if (inherited >= thresholds_[0]) throw std::logic_error("mhgp9 gen WSPD rejected product passed its witnesses on");
      credits[0] = inherited;
      counter_add(result_.work.inherited_credits, inherited);
    }
    unsigned duplicates = 0;  // Received ranks proposed again by this search.
    // ONE inlined loop body serves three rank intervals: the historical window,
    // then, for an eligible surviving product, the left and the right complement
    // of the wider window around the same pivot. The inner loop is the historical
    // loop itself: the widened window costs the default path nothing; credits persist.
    // The received list does cost it something, whatever the option: a task is 72
    // bytes instead of 32 and is copied on every push and pop.
    bool extension = false;
    WspdProposalWindow wider{};
    for (unsigned phase = 0; phase < 3 && mask != 0; ++phase) {
      std::size_t rank = window.first;
      std::size_t last = window.last;
      if (phase == 1) {
        if (proposals_.window_factor == 1 ||
            std::max(a.range.size(), b.range.size()) > proposals_.small_factor_limit)
          break;
        // Same pivot, no second descent. kmax_<=10 and window_factor<=4.
        const auto wider_count = std::min<std::size_t>(
            static_cast<std::size_t>(kmax_) * proposals_.window_factor, order_.size());
        wider = wspd_proposal_window(pivot, wider_count, order_.size());
        if (wider.first > window.first || wider.last < window.last)
          throw std::logic_error("mhgp9 gen WSPD widened proposal window lost the historical window");
        // A search needs Kmax exterior sites, hence n >= Kmax+2 and a strictly
        // wider window: every extended product proposes at least one extra rank.
        if (wider == window) throw std::logic_error("mhgp9 gen WSPD widened proposal window added no rank");
        counter_add(result_.work.extended_products);
        extension = true;
        rank = wider.first;
        last = window.first;
      } else if (phase == 2) {
        rank = window.last;
        last = wider.last;
      }
      const auto begin = rank;
      for (; rank < last && mask != 0; ++rank) {
        counter_add(result_.work.proposed_sites);
        if (contains(a.range, rank) || contains(b.range, rank)) {
          counter_add(result_.work.proposals_in_factors);
          if (extension) counter_add(result_.work.extended_proposals_in_factors);
          continue;
        }
        if (inherited != 0) {
          // The three rank intervals of one search are disjoint: only a
          // RECEIVED rank can come back. It is proposed, never tested again.
          bool duplicate = false;
          for (unsigned index = 0; index < inherited; ++index)
            duplicate = duplicate || witnesses.ranks[index] == rank;
          if (duplicate) {
            ++duplicates;
            counter_add(result_.work.inherited_duplicates);
            if (extension) counter_add(result_.work.extended_inherited_duplicates);
            continue;
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
          if ((mask & bit) != 0 && (lane == 0 || (lane == 1 ? 3 : 2) * h2 > xi)) {
            counter_add(result_.work.witness_lane_credits);
            if (extension) counter_add(result_.work.extended_credits);
            if (++credits[lane] == thresholds_[lane]) {
              mask &= static_cast<std::uint8_t>(~bit);
            } else if (proposals_.inherit_witnesses && lane == 0) {
              // q2 lane alone, credits[0] < Kmax <= 10: index <= 8. Checked, not assumed: an
              // intra-object overflow is invisible to every sanitizer build.
              if (credits[0] - 1 >= witnesses.ranks.size())
                throw std::logic_error("mhgp9 gen WSPD witness list overflow");
              witnesses.ranks[credits[0] - 1] = static_cast<std::uint32_t>(rank);
            }
          }
        }
      }
      // Every iteration proposed exactly one rank, the last one included.
      if (extension) counter_add(result_.work.extended_proposals, static_cast<u64>(rank - begin));
    }
    // validate_front guarantees the q2 lane alone for a widened window: an empty
    // mask after an extension means that the Kmax-th q2 credit came from an extra rank.
    if (extension && mask == 0) counter_add(result_.work.extended_rejections);
    if (proposals_.inherit_witnesses) {
      if (mask == 0) {
        // The ranks that this search saw and that are certified for this
        // product: its new credits and the received ranks it proposed again. The
        // same window alone reaches Kmax credits whenever they already do.
        if (credits[0] - inherited + duplicates < thresholds_[0]) counter_add(result_.work.inherited_rejections);
      } else {
        witnesses.count = static_cast<std::uint8_t>(credits[0]);
      }
    }
    return mask;
  }

  void account_emit(std::size_t a_id, std::size_t b_id, std::uint8_t mask, u64 mass) {
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
  }

  std::span<const Q2SpatialNode> nodes_;
  std::span<const std::size_t> order_;
  std::span<const Point3> points_;
  unsigned kmax_;
  unsigned separation_;
  WspdFrontMode mode_;
  const WspdRectangleConsumer& consumer_;
  WspdFrontProposals proposals_;
  std::array<unsigned, 3> thresholds_{};
  WspdFrontResult result_;
};

void validate_front(unsigned kmax, unsigned separation_s, WspdFrontMode mode, std::uint8_t requested_lane_mask,
                    WspdFrontProposals proposals) {
  if (kmax == 0 || kmax > 10 || separation_s == 0 ||
      (mode != WspdFrontMode::Pure && mode != WspdFrontMode::MidpointSamples))
    throw std::invalid_argument("mhgp9 gen WSPD requires Kmax1..10, positive s and a valid mode");
  const unsigned available = (1U << std::min(kmax, 3U)) - 1;
  if (requested_lane_mask == 0 || requested_lane_mask > 7 || (requested_lane_mask & available) == 0)
    throw std::invalid_argument("mhgp9 gen WSPD requires a mask in 1..7 intersecting available lanes");
  if ((proposals.window_factor != 1 && proposals.window_factor != 2 && proposals.window_factor != 4) ||
      proposals.small_factor_limit == 0)
    throw std::invalid_argument("mhgp9 gen WSPD proposals require window factor 1, 2 or 4 and a positive factor limit");
  if (mode == WspdFrontMode::Pure && proposals.window_factor != 1)
    throw std::invalid_argument("mhgp9 gen WSPD Pure front proposes no witness: window factor must be 1");
  if (proposals.window_factor != 1 && (requested_lane_mask & available) != 1)
    throw std::invalid_argument("mhgp9 gen WSPD widened proposals are qualified for the q2 lane alone: mask must be 1");
  if (proposals.inherit_witnesses) {
    if (mode == WspdFrontMode::Pure)
      throw std::invalid_argument("mhgp9 gen WSPD Pure front searches no witness: inheritance must be off");
    if ((requested_lane_mask & available) != 1)
      throw std::invalid_argument("mhgp9 gen WSPD inherited witnesses are qualified for the q2 lane alone: mask must be 1");
  }
}

}  // namespace

WspdFrontResult run_wspd_front(const Q2CensusIndex& index, unsigned kmax,
                               unsigned separation_s, WspdFrontMode mode,
                               const WspdRectangleConsumer& consumer, std::uint8_t requested_lane_mask,
                               WspdFrontProposals proposals) {
  if (!consumer) throw std::invalid_argument("mhgp9 gen WSPD requires a valid consumer");
  validate_front(kmax, separation_s, mode, requested_lane_mask, proposals);
  return Front(index, kmax, separation_s, mode, consumer, requested_lane_mask, proposals).run();
}

struct WspdFrontJobs::Impl {
  Q2CensusIndexPtr index;
  unsigned kmax;
  unsigned separation;
  WspdFrontMode mode;
  std::uint8_t requested_mask;
  WspdFrontProposals proposals;
  WspdFrontResult prefix;
  std::vector<Task> jobs;
  std::size_t terminals{};

  Impl(Q2CensusIndexPtr owner, unsigned k, unsigned s, WspdFrontMode strategy,
       std::size_t target, std::uint8_t mask, WspdFrontProposals proposal_options)
      : index(std::move(owner)), kmax(k), separation(s), mode(strategy), requested_mask(mask),
        proposals(proposal_options) {
    const WspdRectangleConsumer unused = [](const WspdRectangle&) {};
    Front preparation(*index, kmax, separation, mode, unused, requested_mask, proposals);
    std::deque<Task> pending;
    pending.push_back(preparation.root_task());
    // A true FIFO of unvisited products, not a DFS stack whose small size
    // could silently force preparation of the whole front. Count retained
    // terminals as well: preparation stores only the chosen granularity.
    while (!pending.empty() && jobs.size() < target && pending.size() < target - jobs.size()) {
      const auto task = pending.front();
      pending.pop_front();
      preparation.expand(task, [&](const Task& child) { pending.push_back(child); },
                         [&](const Task& terminal) { jobs.push_back(terminal); });
    }
    terminals = jobs.size();
    if (pending.size() > jobs.max_size() - jobs.size())
      throw std::length_error("mhgp9 gen WSPD job count exceeds vector capacity");
    jobs.reserve(jobs.size() + pending.size());
    while (!pending.empty()) {
      jobs.push_back(pending.front());
      pending.pop_front();
    }
    prefix = preparation.result();
  }
};

WspdFrontJobs::WspdFrontJobs(std::shared_ptr<const Impl> implementation)
    : implementation_(std::move(implementation)) {}
WspdFrontJobs::~WspdFrontJobs() = default;

std::size_t WspdFrontJobs::job_count() const noexcept { return implementation_->jobs.size(); }
std::size_t WspdFrontJobs::terminal_job_count() const noexcept { return implementation_->terminals; }
const WspdFrontResult& WspdFrontJobs::prefix_result() const noexcept { return implementation_->prefix; }
const Q2CensusIndex& WspdFrontJobs::index() const noexcept { return *implementation_->index; }

std::size_t WspdFrontJobs::retained_bytes() const {
  const auto capacity = implementation_->jobs.capacity();
  if (capacity > std::numeric_limits<std::size_t>::max() / sizeof(Task))
    throw std::overflow_error("mhgp9 gen WSPD retained job bytes exceed size_t");
  return capacity * sizeof(Task);
}

WspdFrontResult WspdFrontJobs::run_job(std::size_t id, const WspdRectangleConsumer& consumer) const {
  const auto& plan = *implementation_;
  if (id >= plan.jobs.size() || !consumer)
    throw std::invalid_argument("mhgp9 gen WSPD requires an existing job and valid consumer");
  return Front(*plan.index, plan.kmax, plan.separation, plan.mode, consumer, plan.requested_mask,
               plan.proposals).run(plan.jobs[id]);
}

std::unique_ptr<WspdFrontJobs> make_wspd_front_jobs(
    Q2CensusIndexPtr index, unsigned kmax, unsigned separation_s, WspdFrontMode mode,
    std::size_t target_jobs, std::uint8_t requested_lane_mask, WspdFrontProposals proposals) {
  if (!index || target_jobs == 0)
    throw std::invalid_argument("mhgp9 gen WSPD jobs require an owning index and positive target");
  validate_front(kmax, separation_s, mode, requested_lane_mask, proposals);
  auto implementation = std::make_shared<WspdFrontJobs::Impl>(
      std::move(index), kmax, separation_s, mode, target_jobs, requested_lane_mask, proposals);
  return std::unique_ptr<WspdFrontJobs>(new WspdFrontJobs(std::move(implementation)));
}

struct WspdFrontDispatch::Impl {
  std::shared_ptr<const WspdFrontJobs::Impl> plan;
  const std::size_t interval;
  const std::size_t workers;
  std::vector<Task> queue;
  std::mutex mutex;
  std::condition_variable ready;
  std::size_t head{};
  std::size_t queued{};
  std::size_t next_seed{};
  std::size_t active{};
  std::size_t entries{};
  bool cancelled{};
  bool complete{};
  std::atomic<bool> stopped{false};
  // The exact active count is protected by mutex. This atomic mirror is
  // only a donation heuristic; a stale read cannot remove a product.
  std::atomic<std::size_t> demand;
  std::atomic<std::size_t> waiting{0};
  std::atomic<u64> donated_ranks{0};  // Observation only; see donated_witness_ranks().

  Impl(std::shared_ptr<const WspdFrontJobs::Impl> owner, std::size_t capacity,
       std::size_t donation_interval, std::size_t worker_count)
      : plan(std::move(owner)), interval(donation_interval), workers(worker_count),
        queue(capacity), demand(worker_count) {}

  void cancel() noexcept {
    // Change the wait predicate under the same mutex as condition.wait,
    // including the interval between registering a waiter and sleeping.
    // Notification without this lock could lose the last wakeup.
    {
      std::lock_guard lock(mutex);
      cancelled = true;
      stopped.store(true, std::memory_order_relaxed);
    }
    ready.notify_all();
  }

  void register_worker() {
    std::lock_guard lock(mutex);
    if (entries >= workers)
      throw std::invalid_argument("mhgp9 gen WSPD dispatcher worker count was already consumed");
    ++entries;  // Bounded by the validated worker count, without wrapping.
  }

  bool take(Task& task, bool& seed, WspdFrontDispatchWork& work,
            const std::atomic<bool>& cancellation) {
    std::unique_lock lock(mutex);
    for (;;) {
      if (cancelled || cancellation.load(std::memory_order_relaxed)) {
        cancelled = true;
        stopped.store(true, std::memory_order_relaxed);
        ready.notify_all();
        return false;
      }
      if (complete) return false;
      if (queued != 0 || next_seed < plan->jobs.size()) {
        if (active >= workers)
          throw std::logic_error("mhgp9 gen WSPD dispatcher active workers exceed slots");
        if (queued != 0) {
          counter_add(work.stolen_started);
          task = queue[head];
          head = head + 1 == queue.size() ? 0 : head + 1;
          --queued;
          seed = false;
        } else {
          counter_add(work.seeds_started);
          task = plan->jobs[next_seed++];
          seed = true;
        }
        ++active;
        demand.store(workers - active, std::memory_order_relaxed);
        return true;
      }
      // Empty queue is insufficient while any private stack or callback
      // remains active. Conversely not-yet-started slots own no work.
      if (active == 0) {
        complete = true;
        ready.notify_all();
        return false;
      }
      counter_add(work.waits);
      const auto before = waiting.load(std::memory_order_relaxed);
      if (before >= workers)
        throw std::logic_error("mhgp9 gen WSPD dispatcher waiting workers exceed slots");
      waiting.store(before + 1, std::memory_order_relaxed);
      try {
        ready.wait(lock, [&] {
          return cancelled || cancellation.load(std::memory_order_relaxed) ||
                 queued != 0 || next_seed < plan->jobs.size() || active == 0;
        });
      } catch (...) {
        waiting.store(waiting.load(std::memory_order_relaxed) - 1, std::memory_order_relaxed);
        throw;
      }
      waiting.store(waiting.load(std::memory_order_relaxed) - 1, std::memory_order_relaxed);
      counter_add(work.wakes);
    }
  }

  void release_fragment() noexcept {
    bool last_active = false;
    {
      std::lock_guard lock(mutex);
      --active;  // Exactly one release for each successful take().
      demand.store(workers - active, std::memory_order_relaxed);
      last_active = active == 0;
    }
    // Finishing a fragment creates no queued work. Only the last active
    // worker can make the termination predicate newly true; donations and
    // cancellation already send their own notifications.
    if (last_active) ready.notify_all();
  }

  bool offer(const Task& task, WspdFrontDispatchWork& work) {
    counter_add(work.donor_checks);
    if (demand.load(std::memory_order_relaxed) == 0 || stopped.load(std::memory_order_relaxed)) {
      counter_add(work.offer_no_demand);
      return false;
    }
    std::unique_lock lock(mutex, std::try_to_lock);
    if (!lock.owns_lock()) {
      counter_add(work.offer_attempts);
      counter_add(work.offer_busy);
      return false;
    }
    if (cancelled) {
      counter_add(work.offer_no_demand);
      return false;
    }
    counter_add(work.offer_attempts);
    if (queued == queue.size()) {
      counter_add(work.offer_full);
      return false;
    }
    if (task.terminal)
      throw std::logic_error("mhgp9 gen WSPD donation must be an unvisited product");
    counter_add(work.donations);
    // Avoid overflow in head+queued even for an enormous representable
    // capacity. Task assignment cannot throw or transfer a borrowed view.
    const auto tail = queued >= queue.size() - head ? queued - (queue.size() - head) : head + queued;
    queue[tail] = task;
    donated_ranks.fetch_add(task.witnesses.count, std::memory_order_relaxed);
    ++queued;
    work.max_queue_size = std::max(work.max_queue_size, static_cast<u64>(queued));
    lock.unlock();
    ready.notify_one();
    return true;
  }

  template<bool Cooperate>
  WspdFrontDispatchResult run(const WspdRectangleConsumer& consumer,
                              const std::atomic<bool>& cancellation) {
    if (!consumer)
      throw std::invalid_argument("mhgp9 gen WSPD dispatcher requires a valid consumer");
    register_worker();
    Front front(*plan->index, plan->kmax, plan->separation, plan->mode, consumer, plan->requested_mask,
                plan->proposals);
    WspdFrontDispatchWork work;
    std::vector<Task> stack;
    bool owns_fragment = false;
    bool seed = false;
    try {
      // One seed at a time: potential depth(A)+depth(B)<=2*max_index_depth
      // implies at most 2*max_index_depth+1 pending entries. Reservation,
      // never an exploration cap.
      stack.reserve(2 * max_index_depth + 1);
      std::size_t until_poll = interval;
      Task initial;
      while (take(initial, seed, work, cancellation)) {
        owns_fragment = true;
        if (initial.terminal) {
          consumer(WspdRectangle{initial.a, initial.b, initial.mask});
        } else {
          stack.push_back(initial);
          work.max_local_stack_size = std::max<u64>(work.max_local_stack_size, 1);
          while (!stack.empty()) {
            const auto task = stack.back();
            stack.pop_back();
            front.expand(task, [&](const Task& child) {
              stack.push_back(child);
              work.max_local_stack_size = std::max(work.max_local_stack_size, static_cast<u64>(stack.size()));
            }, [&](const Task& terminal) {
              consumer(WspdRectangle{terminal.a, terminal.b, terminal.mask});
            });
            if constexpr (Cooperate) {
              if (--until_poll == 0) {
                until_poll = interval;
                if (stopped.load(std::memory_order_relaxed) || cancellation.load(std::memory_order_relaxed)) {
                  cancel();
                  break;
                }
                // Give a whole untouched product in O(1), retaining local
                // work. The donor removes it only after publication succeeds.
                if (stack.size() > 1 && offer(stack.back(), work)) stack.pop_back();
              }
            }
          }
          if (!stack.empty()) {
            release_fragment();
            owns_fragment = false;
            break;  // Cancellation: this fragment is not completed.
          }
        }
        counter_add(seed ? work.seeds_completed : work.stolen_completed);
        release_fragment();
        owns_fragment = false;
      }
    } catch (...) {
      cancel();
      if (owns_fragment) release_fragment();
      throw;
    }
    return {front.result(), work};
  }
};

WspdFrontDispatch::WspdFrontDispatch(std::unique_ptr<Impl> implementation)
    : implementation_(std::move(implementation)) {}
WspdFrontDispatch::~WspdFrontDispatch() = default;

WspdFrontDispatchResult WspdFrontDispatch::run_worker(
    const WspdRectangleConsumer& consumer, const std::atomic<bool>& cancellation) {
  try {
    return implementation_->workers == 1 ? implementation_->run<false>(consumer, cancellation)
                                         : implementation_->run<true>(consumer, cancellation);
  } catch (...) {
    // Also cover validation/registration/Front-construction failures that
    // precede the private stack's active-fragment cleanup region.
    implementation_->cancel();
    throw;
  }
}

void WspdFrontDispatch::cancel() noexcept { implementation_->cancel(); }
std::size_t WspdFrontDispatch::queue_capacity() const noexcept { return implementation_->queue.size(); }
std::size_t WspdFrontDispatch::worker_count() const noexcept { return implementation_->workers; }
std::size_t WspdFrontDispatch::waiting_workers() const noexcept {
  return implementation_->waiting.load(std::memory_order_relaxed);
}
u64 WspdFrontDispatch::donated_witness_ranks() const noexcept {
  return implementation_->donated_ranks.load(std::memory_order_relaxed);
}

std::size_t WspdFrontDispatch::retained_bytes() const {
  const auto capacity = implementation_->queue.capacity();
  if (capacity > std::numeric_limits<std::size_t>::max() / sizeof(Task))
    throw std::overflow_error("mhgp9 gen WSPD dispatch queue bytes exceed size_t");
  return capacity * sizeof(Task);
}

std::unique_ptr<WspdFrontDispatch> WspdFrontJobs::make_dispatch(
    std::size_t queue_capacity, std::size_t donation_interval, std::size_t worker_count) const {
  if (queue_capacity == 0 || donation_interval == 0 || worker_count == 0)
    throw std::invalid_argument("mhgp9 gen WSPD dispatcher requires positive capacity, interval and workers");
  auto implementation = std::make_unique<WspdFrontDispatch::Impl>(
      implementation_, queue_capacity, donation_interval, worker_count);
  return std::unique_ptr<WspdFrontDispatch>(new WspdFrontDispatch(std::move(implementation)));
}

}  // namespace mhgp9::gen
