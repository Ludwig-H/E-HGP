#pragma once

#include <atomic>
#include <cstddef>
#include <exception>
#include <thread>
#include <utility>
#include <vector>

namespace mhgp8::parallel_detail {

struct ThreadLauncher {
  template <class Function>
  std::thread operator()(Function&& function) const {
    return std::thread(std::forward<Function>(function));
  }
};

// The launcher parameter allows a deterministic partial-launch failure
// test without process-global fault hooks. Every started thread is joined
// before its callback, failure slots or cancellation flag leave scope.
template <class Work, class Launcher = ThreadLauncher>
void run_joined_workers(std::size_t count, Work&& work, Launcher launch = {}) {
  if (count == 0) return;
  std::atomic<bool> cancel{false};
  std::vector<std::exception_ptr> failures(count);
  auto invoke = [&](std::size_t worker) {
    try {
      work(worker, cancel);
    } catch (...) {
      failures[worker] = std::current_exception();
      cancel.store(true, std::memory_order_relaxed);
    }
  };
  if (count == 1) {
    invoke(0);
  } else {
    std::vector<std::thread> threads;
    threads.reserve(count);
    try {
      for (std::size_t worker = 0; worker < count; ++worker) {
        threads.push_back(launch([&, worker] { invoke(worker); }));
      }
    } catch (...) {
      cancel.store(true, std::memory_order_relaxed);
      for (auto& thread : threads) thread.join();
      throw;
    }
    for (auto& thread : threads) thread.join();
  }
  // Multiple simultaneous failures have unspecified chronological order.
  // A stable slot order selects one only after all workers have stopped.
  for (const auto& failure : failures) if (failure) std::rethrow_exception(failure);
}

}  // namespace mhgp8::parallel_detail
