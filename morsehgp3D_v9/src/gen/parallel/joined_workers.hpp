#pragma once

#include <atomic>
#include <cstddef>
#include <exception>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace mhgp9::gen::parallel_detail {

struct ThreadLauncher {
  template <class Function>
  std::thread operator()(Function&& function) const {
    return std::thread(std::forward<Function>(function));
  }
};

struct NoCancelNotification {
  void operator()() const noexcept {}
};

// The launcher parameter allows a deterministic partial-launch failure
// test without process-global fault hooks. Every started thread is joined
// before its callback, failure slots or cancellation flag leave scope.
// The nonthrowing hook wakes scheduler waiters after cancellation, also
// when only part of the requested thread set could be launched. It can be
// called concurrently and more than once; it must be safe in both cases.
template <class Work, class Launcher = ThreadLauncher, class OnCancel = NoCancelNotification>
void run_joined_workers(std::size_t count, Work&& work, Launcher launch = {}, OnCancel on_cancel = {}) {
  static_assert(std::is_nothrow_invocable_v<OnCancel&>, "worker cancellation notification must be noexcept");
  if (count == 0) return;
  std::atomic<bool> cancel{false};
  std::vector<std::exception_ptr> failures(count);
  auto invoke = [&](std::size_t worker) {
    try {
      work(worker, cancel);
    } catch (...) {
      failures[worker] = std::current_exception();
      cancel.store(true, std::memory_order_relaxed);
      on_cancel();
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
      on_cancel();
      for (auto& thread : threads) thread.join();
      throw;
    }
    for (auto& thread : threads) thread.join();
  }
  // Multiple simultaneous failures have unspecified chronological order.
  // A stable slot order selects one only after all workers have stopped.
  for (const auto& failure : failures) if (failure) std::rethrow_exception(failure);
}

}  // namespace mhgp9::gen::parallel_detail
