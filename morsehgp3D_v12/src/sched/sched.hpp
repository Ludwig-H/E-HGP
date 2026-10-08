// Pool synchrone unique d'une Session. Port explicite R2 : source_pins.json ; aucune qualification heritee.
// Callback emprunte, aucune allocation par appel, aucun etat global ni TLS modifiable.
#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <semaphore>
#include <thread>
#include <vector>

#include "core/core.hpp"

namespace mhgp12::sched {

inline constexpr u32 kMaxWorkers = 256;
struct PoolParams {
  u32 workers = 1;  // 1..256, appelant compris ; aucune detection implicite de la machine
};

class Pool;
[[nodiscard]] Result<std::unique_ptr<Pool>> make_pool(PoolParams params = {}) noexcept;

class Pool {
 public:
  using Body = Outcome (*)(void* context, u64 begin, u64 end, u32 worker);

  Pool(const Pool&) = delete;
  Pool& operator=(const Pool&) = delete;
  Pool(Pool&&) = delete;
  Pool& operator=(Pool&&) = delete;
  ~Pool();  // arrete et joint tous les fils ; le Pool doit survivre a ses appels

  u32 size() const noexcept { return static_cast<u32>(workers_.size()) + 1; }

  // Tranches [k*grain,min(n,(k+1)*grain)), toutes executees exactement une fois sur succes du protocole.
  // grain>0 et body!=nullptr, meme si n=0 ; context peut etre nul si body le permet. L'appelant participe.
  // Equipe (8 octobre 2026) : pour c = ceil(n/grain) tranches, seuls les min(W-1, c-1) premiers ouvriers sont
  // reveilles, chacun par son propre semaphore ; une seule tranche s'execute dans l'appelant sans reveiller personne.
  // Les numeros d'ouvrier passes a body restent dans 0..W-1. Le retour attend les ouvriers engages.
  // Une invocation active seulement : appel concurrent ou reentrant refuse immediatement (pool_busy),
  // avant les parametres. Aucun rappel serie implicite, aucune attente de la fin de l'appel concurrent.
  // Les Outcome de TOUTES les tranches sont fusionnes par merge, meme apres un refus ou une exception :
  // bad_alloc devient memory_budget, toute autre exception task_exception. Pas d'annulation dependant
  // du scheduling. Une tranche interrompue par une exception n'est pas rejouee. Le Pool reste reutilisable.
  // Body/context et ses donnees restent vivants jusqu'au retour. Les effets de body ne sont pas annules : le module
  // appelant publie son resultat seulement sur succes et utilise des positions fixees par les ordinaux de tache.
  [[nodiscard]] Outcome parallel_for(u64 n, u64 grain, void* context, Body body) noexcept;

 private:
  explicit Pool(u32 workers);
  friend Result<std::unique_ptr<Pool>> make_pool(PoolParams) noexcept;

  struct Job {
    u64 n = 0, grain = 1;
    void* context = nullptr;
    Body body = nullptr;
    std::atomic<u64> next{0};
    std::array<Outcome, kMaxWorkers> outcomes{};  // une case privee par worker, lue apres acquittement
    std::atomic<u32> remaining{0};  // ouvriers engages qui n'ont pas encore acquitte ; le dernier libere done_
  };
  struct Wake {
    std::binary_semaphore go{0};  // un jeton par invocation qui engage cet ouvrier, ou a l'arret
  };
  static void run_chunks(Job& job, u32 worker) noexcept;
  void worker_loop(u32 worker);
  void shutdown() noexcept;

  // Petits tableaux bornes par W, permis par ARCHITECTURE.md de la v11, paragraphe 7.1. Les piles OS ne sont pas des
  // Buffer.
  std::vector<std::thread> workers_;
  std::unique_ptr<Wake[]> wake_;  // wake_[w - 1] reveille l'ouvrier w
  std::binary_semaphore done_{0};  // libere par le dernier ouvrier engage
  std::atomic_flag active_ = ATOMIC_FLAG_INIT;
  Job* current_ = nullptr;  // ecrit avant les jetons (release), lu apres (acquire) ; vivant jusqu'au retour
  std::atomic<bool> stop_{false};
};

}  // namespace mhgp12::sched
