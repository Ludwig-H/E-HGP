// MorseHGP3D v5 — POOL D'EXECUTEURS DEVICE BORNE ET PERSISTANT (G0 des
// auditeurs, docs/GPU.md « Lane residente sur device » ; hote pur, sans CUDA).
//
// Avant : chaque ouvrier CPU (jusqu'a 48) possedait un executeur device
// `thread_local` ephemere (flux, evenements, tampons crees puis detruits avec
// l'equipe), tous soumettant en concurrence a un seul device ; les lots
// residuels etaient vides sequentiellement apres la jointure.
// Apres : `gpu_executors` executeurs (1, 2, 4 ou 8) crees UNE fois par lane
// et possedes par autant de fils de pool ; les producteurs CPU soumettent un
// travail (lot + parametres) dans une file bornee (contre-pression) et
// ATTENDENT son achevement — l'emission reste dans le producteur, dans son
// tampon et son ordre, donc la fusion deterministe par ouvrier est inchangee ;
// les residus passent par la meme file. Exceptions : capturees dans le fil de
// pool et relancees dans le producteur qui a soumis (jamais std::terminate).
// Mutants : `pool-serial` (un seul executeur quel que soit N : la preuve de
// concurrence serait verte par vacuite) et `pool-drop-exception` (exception
// d'un travail avalee au lieu d'etre relancee dans le producteur) — tues par
// la porte du pool.
#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "../core/mutants.hpp"
#include "../core/types.hpp"

namespace mhgp5 {
namespace gpu {

inline constexpr int kGpuExecutorsMax = 8;

template <class Executor>
class ExecutorPool {
 public:
  using Job = std::function<void(Executor&)>;

  explicit ExecutorPool(int executors, size_t queue_cap = 0)
      : n_(MHGP5_MUTANT("pool-serial") ? 1 : ((executors < 1) ? 1 : (executors > kGpuExecutorsMax ? kGpuExecutorsMax : executors))),
        cap_(queue_cap ? queue_cap : (size_t)n_ * 2) {
    threads_.reserve((size_t)n_);
    for (int i = 0; i < n_; ++i) threads_.emplace_back([this] { run(); });
  }
  ExecutorPool(const ExecutorPool&) = delete;
  ExecutorPool& operator=(const ExecutorPool&) = delete;
  ~ExecutorPool() {
    {
      std::lock_guard<std::mutex> lk(mu_);
      stop_ = true;
    }
    cv_.notify_all();
    for (std::thread& t : threads_)
      if (t.joinable()) t.join();
  }
  int executors() const { return n_; }
  u64 jobs_done() const { return done_.load(std::memory_order_acquire); }
  u32 peak_active() const { return peak_.load(std::memory_order_acquire); }

  // Soumet et ATTEND (contre-pression si la file est pleine) ; relance dans
  // l'appelant l'exception levee par le travail.
  void submit_and_wait(Job job) {
    Ticket t;
    t.job = std::move(job);
    {
      std::unique_lock<std::mutex> lk(mu_);
      cv_space_.wait(lk, [&] { return stop_ || queue_.size() < cap_; });
      if (stop_) throw std::runtime_error("pool d'executeurs arrete");
      queue_.push_back(&t);
    }
    cv_.notify_one();
    std::unique_lock<std::mutex> lk(t.mu);
    t.cv.wait(lk, [&] { return t.done; });
    if (t.exc && !MHGP5_MUTANT("pool-drop-exception")) std::rethrow_exception(t.exc);
  }

 private:
  struct Ticket {
    Job job;
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;
    std::exception_ptr exc;
  };
  void run() {
    Executor ex;  // cree UNE fois par fil de pool, detruit a l'arret
    for (;;) {
      Ticket* t = nullptr;
      {
        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait(lk, [&] { return stop_ || !queue_.empty(); });
        if (queue_.empty()) return;  // stop_ et rien a faire
        t = queue_.front();
        queue_.pop_front();
      }
      cv_space_.notify_one();
      const u32 cur = active_.fetch_add(1, std::memory_order_acq_rel) + 1;
      u32 p = peak_.load(std::memory_order_relaxed);
      while (cur > p && !peak_.compare_exchange_weak(p, cur, std::memory_order_acq_rel)) {}
      try {
        t->job(ex);
      } catch (...) {
        std::lock_guard<std::mutex> lk(t->mu);
        t->exc = std::current_exception();
      }
      active_.fetch_sub(1, std::memory_order_acq_rel);
      done_.fetch_add(1, std::memory_order_acq_rel);
      {
        std::lock_guard<std::mutex> lk(t->mu);
        t->done = true;
      }
      t->cv.notify_all();
    }
  }
  int n_;
  size_t cap_;
  std::mutex mu_;
  std::condition_variable cv_, cv_space_;
  std::deque<Ticket*> queue_;
  bool stop_ = false;
  std::vector<std::thread> threads_;
  std::atomic<u32> active_{0}, peak_{0};
  std::atomic<u64> done_{0};
};

}  // namespace gpu
}  // namespace mhgp5
