#include "sched/pool.hpp"

#include <algorithm>

namespace mhgp10::sched {

namespace {
thread_local bool tls_in_region = false;

// Leve le drapeau pour la duree d'un run_chunks et le restaure en toute sortie, normale ou exceptionnelle (audit du
// 29 septembre 2026 : un fil appelant quitte par une exception gardait le drapeau, et tous ses parallel_for suivants
// passaient en serie, comptes comme imbriques).
class RegionFlag {
 public:
  RegionFlag() : was_(tls_in_region) { tls_in_region = true; }
  ~RegionFlag() { tls_in_region = was_; }
  RegionFlag(const RegionFlag&) = delete;
  RegionFlag& operator=(const RegionFlag&) = delete;

 private:
  bool was_;
};
}  // namespace

bool in_parallel_region() { return tls_in_region; }

Pool::Pool(unsigned threads) {
  if (threads == 0) threads = std::max(1u, std::thread::hardware_concurrency());
  workers_.reserve(threads - 1);
  try {
    for (unsigned i = 1; i < threads; ++i) workers_.emplace_back([this, i] { worker_loop(i); });
  } catch (...) {
    shutdown();  // les fils deja crees : un std::thread joignable detruit appellerait std::terminate
    throw;
  }
}

Pool::~Pool() { shutdown(); }

void Pool::shutdown() noexcept {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stop_ = true;
  }
  wake_.notify_all();
  for (auto& t : workers_) t.join();
}

// Reclamation saturante : si le compteur vaut begin < n, la tranche [begin, begin + min(grain, n - begin)) est prise
// par un seul echange compare. Le compteur ne prend que des valeurs de [0, n] et ne decroit jamais, quels que soient
// n et grain : aucune arithmetique ne deborde, et chaque tranche est reclamee au plus une fois (verificateur du tour 1,
// 29 septembre 2026 : fetch_add portait le compteur jusqu'a n + P * grain, qui repassait par 0 au-dela de 2^64, et une
// tranche etait executee deux fois).
// Aucune exception ne sort : la premiere levee par body est conservee dans le travail, et le compteur de tranches
// est porte a n, si bien que toute tranche reclamee apres la capture est refusee (le compteur reste a n) ; seules
// les tranches deja reclamees, au plus une par fil, se terminent.
void Pool::run_chunks(Job& job, unsigned id) noexcept {
  const RegionFlag region;
  try {
    for (;;) {
      const u64 begin = job.next.fetch_add(job.grain, std::memory_order_relaxed);
      if (begin >= job.n) break;
      (*job.body)(begin, std::min(job.n, begin + job.grain), id);
    }
    u64 begin = job.n;
    while (begin < job.n) {
      const u64 end = begin + std::min(job.grain, job.n - begin);
      if (!job.next.compare_exchange_weak(begin, end, std::memory_order_relaxed)) continue;  // begin relu
      (*job.body)(begin, end, id);
      begin = job.next.load(std::memory_order_relaxed);
    }
  } catch (...) {
    job.next.store(job.n, std::memory_order_relaxed);
    if (!job.failed.exchange(true, std::memory_order_relaxed)) job.error = std::current_exception();
  }
}

void Pool::worker_loop(unsigned id) {
  u64 seen = 0;
  for (;;) {
    Job* job;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wake_.wait(lock, [&] { return stop_ || (current_ != nullptr && generation_ != seen); });
      if (stop_) return;
      seen = generation_;
      job = current_;
      ++job->users;
    }
    run_chunks(*job, id);
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (--job->users == 0) done_.notify_all();
    }
  }
}

void Pool::parallel_for(u64 n, u64 grain, const std::function<void(u64, u64, unsigned)>& body) {
  if (n == 0) return;
  if (grain == 0) grain = 1;
  if (tls_in_region || workers_.empty()) {
    if (tls_in_region) nested_.fetch_add(1, std::memory_order_relaxed);
    for (u64 b = 0; b < n;) {  // memes tranches, sans b + grain qui deborderait pres de 2^64
      const u64 e = b + std::min(grain, n - b);
      body(b, e, 0);
      b = e;
    }
    return;
  }
  // Un ouvrier en retard ne voit jamais les champs d'un autre travail : il capture le descripteur sous le verrou, et
  // l'appelant ferme la capture avant d'attendre ceux qui l'ont pris (regression du 29 septembre 2026 : un ouvrier
  // inscrit apres la fin d'un travail lisait le compteur du suivant et en executait des tranches deux fois).
  // Exceptions (audit du 29 septembre 2026) : run_chunks les capture dans le travail, y compris dans l'appelant ;
  // la fermeture de la capture et l'attente des utilisateurs precedent donc toujours la sortie de parallel_for, et
  // body comme le descripteur restent vivants tant qu'un ouvrier les lit. La relance vient apres.
  Job job;
  job.body = &body;
  job.n = n;
  job.grain = grain;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    current_ = &job;
    ++generation_;
  }
  wake_.notify_all();
  run_chunks(job, 0);
  {
    std::unique_lock<std::mutex> lock(mutex_);
    current_ = nullptr;
    done_.wait(lock, [&] { return job.users == 0; });
  }
  if (job.error) std::rethrow_exception(job.error);
}

}  // namespace mhgp10::sched
