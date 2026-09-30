// Ordonnanceur unique de la v10 : un Pool par Session, aucun fil ailleurs.
//
// Regles de determinisme : chaque tache ecrit a des positions fixees par son ordinal (jamais par
// l'ordre d'arrivee) ; les concatenations passent par des prefixes ; pas de parallelisme imbrique
// (un parallel_for appele depuis un ouvrier s'execute en serie et le signale).
#pragma once

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "core/types.hpp"

namespace mhgp10::sched {

class Pool {
 public:
  // threads = 0 : std::thread::hardware_concurrency(). Si la creation d'un fil echoue, les fils deja crees sont
  // arretes et joints avant que l'exception ne sorte du constructeur (jamais de std::thread joignable detruit).
  explicit Pool(unsigned threads = 0);
  ~Pool();
  Pool(const Pool&) = delete;
  Pool& operator=(const Pool&) = delete;

  unsigned size() const { return static_cast<unsigned>(workers_.size()) + 1; }

  // Execute body(begin, end, worker) sur [0, n) par tranches de `grain` ; le fil appelant participe.
  // Les tranches sont distribuees dynamiquement, mais body ne doit ecrire qu'en fonction de [begin, end).
  // Tranches : [k * grain, min(n, (k + 1) * grain)) pour k < ceil(n / grain) (grain = 0 vaut 1), chacune reclamee au
  // plus une fois, en parallele comme en serie. Aucune borne sur n ni sur grain : la reclamation sature a n, le
  // compteur reste dans [0, n] et aucune arithmetique ne deborde (fetch_add, avant, exigeait n + P * grain < 2^64).
  // Exceptions : la premiere exception levee par body, dans l'appelant ou dans un ouvrier, est capturee ; aucune
  // tranche reclamee apres sa capture n'est executee ; parallel_for ne rend la main qu'apres la sortie de tous les
  // fils entres dans le travail, puis relance cette exception dans l'appelant (les suivantes sont abandonnees). Le
  // pool reste utilisable. En serie (imbrication, pool a un fil), l'exception sort directement de la tranche.
  void parallel_for(u64 n, u64 grain, const std::function<void(u64, u64, unsigned)>& body);

  // Nombre d'appels imbriques executes en serie depuis la creation (0 attendu).
  u64 nested_calls() const { return nested_.load(std::memory_order_relaxed); }

 private:
  // Descripteur d'un travail, propre a un appel de parallel_for. Un ouvrier ne le lit qu'apres l'avoir capture sous
  // mutex_ ; l'appelant ferme la capture (current_ = nullptr) avant d'attendre que users retombe a 0, en sortie
  // normale comme apres une exception.
  struct Job {
    const std::function<void(u64, u64, unsigned)>* body = nullptr;
    u64 n = 0, grain = 1;
    std::atomic<u64> next{0};         // debut de la prochaine tranche, dans [0, n], croissant
    std::atomic<bool> failed{false};  // une exception a ete capturee ; elit l'unique ecrivain de error
    std::exception_ptr error;         // premiere exception ; lue par l'appelant apres users == 0 (sous mutex_)
    unsigned users = 0;  // ouvriers qui l'ont capture et ne l'ont pas rendu (sous mutex_)
  };
  void worker_loop(unsigned id);
  void shutdown() noexcept;
  static void run_chunks(Job& job, unsigned id) noexcept;

  std::vector<std::thread> workers_;
  std::mutex mutex_;
  std::condition_variable wake_;
  std::condition_variable done_;
  Job* current_ = nullptr;  // travail ouvert a la capture (sous mutex_)
  u64 generation_ = 0;
  bool stop_ = false;
  std::atomic<u64> nested_{0};
};

// Vrai dans un fil ouvrier du Pool (ou dans l'appelant pendant un parallel_for).
bool in_parallel_region();

}  // namespace mhgp10::sched
