#pragma once

// Private structural prototype. The borrowed history must remain immutable and
// alive. Construction is sequential; only independent batch queries use CPU
// workers. This is not a geometric FULL producer or a parallel MSF constructor.
#include "filtered_calendar.hpp"
#include <exception>
#include <thread>

namespace mhgp7::filtered_calendar_private {
struct QueryWork { Id light_steps = 0, binary_steps = 0; };
template<class Date> struct HistoricalRequest {
  Id segment;
  Date admission, cut;
  bool closed;
};
struct HistoricalAnswer { Id segment = absent; QueryWork work; };

template<class Date, class Less> class HistoricalChains {
 public:
  HistoricalChains(const History<Date>& history, Less less)
      : history_(&history), less_(less), head_(history.nodes.size(), absent),
        position_(history.nodes.size(), absent) {
    const Id n = history.nodes.size();
    require(history.successors.size() == n, "chains.successor_size");
    std::vector<Id> size(n, 1), heavy(n, absent), seen(n, 0);
    Id consumed = 0;
    for (Id i = 0; i < n; ++i) {
      const auto& node = history.nodes[i];
      require(node.first == consumed && node.first <= history.parents.size() &&
                  node.parent_count <= history.parents.size() - node.first,
              "chains.parent_offsets");
      require(node.parent_count != 1, "chains.unary_node");
      consumed += node.parent_count;
      for (Id j = 0; j < node.parent_count; ++j) {
        const Id child = history.parents[node.first + j];
        require(child < i && history.successors[child] == i && seen[child]++ == 0,
                "chains.parent_identity");
        require(less_(history.nodes[child].date, node.date), "chains.non_strict_level");
        require(size[child] <= n - size[i], "chains.subtree_overflow");
        size[i] += size[child];
        if (heavy[i] == absent || size[child] > size[heavy[i]] ||
            (size[child] == size[heavy[i]] && child < heavy[i])) heavy[i] = child;
      }
    }
    require(consumed == history.parents.size(), "chains.parent_tail");
    std::vector<Id> pending;
    for (Id i = 0; i < n; ++i) {
      const Id successor = history.successors[i];
      require(successor == absent || (successor > i && successor < n && seen[i] == 1),
              "chains.successor_identity");
      if (successor == absent) pending.push_back(i);
    }
    auto expected_roots = history.roots;
    std::sort(expected_roots.begin(), expected_roots.end());
    require(expected_roots == pending, "chains.roots");
    order_.reserve(n);
    while (!pending.empty()) {
      const Id top = pending.back();
      pending.pop_back();
      for (Id i = top; i != absent; i = heavy[i]) {
        require(head_[i] == absent, "chains.repeated_node");
        head_[i] = top;
        position_[i] = order_.size();
        order_.push_back(i);
        const auto& node = history.nodes[i];
        for (Id j = 0; j < node.parent_count; ++j) {
          const Id child = history.parents[node.first + j];
          if (child != heavy[i]) pending.push_back(child);
        }
      }
    }
    require(order_.size() == n, "chains.missing_node");
  }

  Id logical_index_bytes() const { return 3 * sizeof(Id) * order_.size(); }

  HistoricalAnswer query(const HistoricalRequest<Date>& request) const {
    const auto& history = *history_;
    Id node = request.segment;
    require(node < history.nodes.size(), "chains.query_identity");
    require(!less_(request.admission, history.nodes[node].date), "chains.query_admission");
    HistoricalAnswer result;
    if (!admitted(request.admission, request.cut, request.closed, less_)) return result;
    auto active = [&](Id id) {
      return admitted(history.nodes[id].date, request.cut, request.closed, less_);
    };
    while (true) {
      const Id top = head_[node];
      if (!active(top)) {
        Id lo = position_[top], hi = position_[node];
        while (lo < hi) {
          ++result.work.binary_steps;
          const Id mid = lo + (hi - lo) / 2;
          if (active(order_[mid])) hi = mid;
          else lo = mid + 1;
        }
        result.segment = order_[lo];
        return result;
      }
      const Id successor = history.successors[top];
      if (successor == absent || !active(successor)) {
        result.segment = top;
        return result;
      }
      ++result.work.light_steps;
      node = successor;
    }
  }

  std::vector<HistoricalAnswer> batch(const std::vector<HistoricalRequest<Date>>& requests,
                                     std::size_t workers) const {
    require(workers > 0, "chains.zero_workers");
    workers = std::min(workers, std::max(std::size_t{1}, requests.size()));
    std::vector<HistoricalAnswer> result(requests.size());
    std::vector<std::exception_ptr> errors(workers);
    auto task = [&](std::size_t worker) {
      try {
        // Quotient/remainder partition avoids requests.size()*worker overflow.
        const auto width = requests.size() / workers, extra = requests.size() % workers;
        const auto first = worker * width + std::min(worker, extra);
        const auto last = first + width + (worker < extra);
        for (auto i = first; i < last; ++i) result[i] = query(requests[i]);
      } catch (...) { errors[worker] = std::current_exception(); }
    };
    {
      std::vector<std::jthread> threads;
      for (std::size_t worker = 1; worker < workers; ++worker) threads.emplace_back(task, worker);
      task(0);
    }  // Join every worker before examining errors or publishing any output.
    for (const auto& error : errors) if (error) std::rethrow_exception(error);
    return result;
  }

 private:
  const History<Date>* history_;
  Less less_;
  std::vector<Id> head_, position_, order_;
};
}  // namespace mhgp7::filtered_calendar_private
