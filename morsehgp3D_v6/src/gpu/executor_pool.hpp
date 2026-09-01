// MorseHGP3D v6 — POOL D'EXECUTEURS DEVICE BORNE ET PERSISTANT (G0' du plan
// GPU v6 ; hote pur, sans CUDA). PORT CONTRACTUEL de
// morsehgp3D_v5/src/gpu/executor_pool.hpp (pin : voir docs/PROVENANCE.md),
// requalifie par la porte mhgp6_executor_pool — plus la DENT v5 REPAREE :
// le confinement de panne device. En v5, close_fatal existait mais aucun
// wrapper ne l'appelait sur une erreur CUDA. Le confinement appartient au
// WORKER (REPONSE_AUDITEURS_MULTICPU § 5.6 : un confinement cote producteur
// laisse une course causale — le worker peut depiler et executer le travail
// suivant sur l'executeur empoisonne avant que le producteur reveille n'ait
// ferme ; sonde auditeur : 199 reutilisations sur 200) : le fil de pool qui
// voit une DeviceFatalError FERME l'admission et ANNULE la file AVANT de
// notifier le ticket fatal, puis sort — l'executeur empoisonne n'est jamais
// reutilise. Le producteur recoit l'erreur typee et la mappe en refus
// transactionnel (run.hpp), jamais un verdict ni un prefixe publie. Mutant
// `pool-worker-resume-after-fatal` : le worker ne ferme pas et reboucle —
// tue par la fixture du travail poste-fatal (porte du pool, scenario 10).
//
// Avant : chaque ouvrier CPU (jusqu'a 48) possedait un executeur device
// `thread_local` ephemere (flux, evenements, tampons crees puis detruits avec
// l'equipe), tous soumettant en concurrence a un seul device ; les lots
// residuels etaient vides sequentiellement apres la jointure.
// Apres : `gpu_executors` executeurs (1 a 8) crees UNE fois par lane et
// possedes par autant de fils de pool ; les producteurs CPU soumettent un
// travail (lot + parametres) dans une file bornee (contre-pression) et
// ATTENDENT son achevement — l'emission reste dans le producteur, dans son
// tampon et son ordre, donc la fusion deterministe par ouvrier est inchangee ;
// les residus passent par la meme file.
//
// QUATRE CORRECTIONS DE SURETE (audit du 28 aout, P0 du chemin de reception) :
//   1. TICKET — la notification se fait SOUS `Ticket::mu`, et plus rien n'est
//      touche du ticket apres. Auparavant `notify_all()` suivait la
//      liberation du verrou : un reveil spurieux pouvait laisser le
//      producteur detruire son ticket de pile avant la notification (UB).
//   2. DEMARRAGE TRANSACTIONNEL — l'`Executor` est construit sous capture
//      d'exception et son etat remonte au constructeur, qui ne revient que
//      lorsque les N fils sont PRETS. En cas d'echec (executeur ou lancement
//      de fil apres un demarrage partiel) : fermeture, reveil, jonction de
//      tous les fils deja crees, puis relance. Auparavant l'exception
//      s'echappait du corps du fil (`std::terminate`) et un `emplace_back`
//      qui echouait laissait des fils non joints.
//   3. EMBOITEMENT — `submit_and_wait()` appele DEPUIS un travail de pool est
//      refuse immediatement (marqueur `thread_local` NON ALLOUANT : un simple
//      pointeur). G0 n'a besoin d'aucun emboitement : un refus clair vaut
//      mieux qu'un blocage (certain a N = 1).
//   4. DOMAINE — le nombre d'executeurs est REFUSE hors de [1, 8], jamais
//      clampe silencieusement ; tout entier de 1 a 8 est accepte, 3 compris.
//
// Mutants : `pool-serial` (un seul executeur quel que soit N : la preuve de
// concurrence serait verte par vacuite) et `pool-drop-exception` (exception
// d'un travail avalee au lieu d'etre relancee dans le producteur) — tues par
// la porte du pool.
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "../core/mutants.hpp"
#include "../core/types.hpp"

namespace mhgp6 {
namespace gpu {

inline constexpr int kGpuExecutorsMax = 8;

// PANNE DEVICE FATALE typee : les wrappers CUDA la leveront (apres cudaError
// irrecuperable) ; le confinement ferme le pool puis la remonte. La
// conversion en refus transactionnel du RUN entier est une INTENTION C2-C5
// (les lanes serie C la feront ; rien dans run.hpp ne consomme le pool a ce
// jour) — jamais un verdict. NB : `queue_cap` borne le nombre de tickets en
// deque, pas les captures detenues par les producteurs bloques, les tampons
// des executeurs ni la VRAM (budget VRAM = contrat C2).
struct DeviceFatalError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

// TRAVAIL DE POOL EN COURS SUR CE FIL. Marqueur `thread_local` NON ALLOUANT
// (un simple pointeur) : le durcissement demande par l'audit du 28 aout
// interdit TOUT emboitement, pas seulement la reentrance sur le meme pool —
// une soumission depuis un travail de pool, quel qu'il soit, est refusee. G0
// n'a besoin d'aucun emboitement, et un refus clair vaut mieux qu'un blocage
// (certain a N = 1) ou qu'une allocation dans un chemin chaud.
inline const void*& current_pool_job() {
  thread_local const void* p = nullptr;
  return p;
}

template <class Executor>
class ExecutorPool {
 public:
  using Job = std::function<void(Executor&)>;

  explicit ExecutorPool(int executors, size_t queue_cap = 0)
      : n_(check_domain(executors)), cap_(queue_cap ? queue_cap : (size_t)n_ * 2) {
    std::exception_ptr start_exc;
    size_t launched = 0;
    try {
      threads_.reserve((size_t)n_);
      for (int i = 0; i < n_; ++i) {
        threads_.emplace_back([this] { run(); });
        ++launched;
      }
    } catch (...) {  // lancement de fil impossible apres un demarrage partiel
      start_exc = std::current_exception();
    }
    {  // Le constructeur attend que TOUS les fils lances aient construit (ou
       // echoue a construire) leur executeur : il ne revient jamais sur un
       // pool a moitie pret.
      std::unique_lock<std::mutex> lk(mu_);
      cv_ready_.wait(lk, [&] { return ready_ + failed_ >= launched; });
      if (failed_ && !start_exc) start_exc = first_exc_;
    }
    if (start_exc) {
      shutdown_and_join();
      std::rethrow_exception(start_exc);
    }
  }
  ExecutorPool(const ExecutorPool&) = delete;
  ExecutorPool& operator=(const ExecutorPool&) = delete;
  // Le proprietaire ne detruit le pool qu'apres jonction de tous les
  // producteurs — c'est deja le contrat des lanes (generate_q3/q4_device).
  ~ExecutorPool() { shutdown_and_join(); }

  int executors() const { return n_; }
  size_t queue_cap() const { return cap_; }
  u64 jobs_done() const { return done_.load(std::memory_order_acquire); }
  u32 peak_active() const { return peak_.load(std::memory_order_acquire); }
  u32 peak_queued() const { return peak_queued_.load(std::memory_order_acquire); }

  // COMPTABILITE DE FERMETURE (P1 de l'audit) : a la destruction, l'invariant
  // `soumis = reussis + echoues + annules` et `actifs = en_file = 0` doit
  // tenir. `succeeded` compte les travaux dont le corps est revenu sans
  // exception, `failed` ceux qui ont leve, `cancelled` ceux qu'une erreur
  // FATALE a retires de la file sans les executer.
  struct Counters {
    u64 submitted = 0, succeeded = 0, failed = 0, cancelled = 0;
    u32 active = 0, queued = 0;
    bool balanced() const { return submitted == succeeded + failed + cancelled && active == 0 && queued == 0; }
  };
  Counters counters() const {
    std::lock_guard<std::mutex> lk(mu_);
    Counters c;
    c.submitted = submitted_;
    c.succeeded = succeeded_;
    c.failed = failed_jobs_;
    c.cancelled = cancelled_;
    c.active = active_.load(std::memory_order_acquire);
    c.queued = (u32)queue_.size();
    return c;
  }

#if defined(MHGP6_TESTING)
  // HOOK TEST-ONLY (6e contre-lecture § 5.6) : appele par le worker
  // immediatement AVANT `active++` — SOUS mu_ au nominal (un fil closer qui
  // appelle close_fatal pendant le hook reste DERRIERE le verrou : il ne
  // peut voir que l'etat post-activation), et APRES le deverrouillage sous
  // le mutant pool-activate-after-unlock (le closer s'intercale : il voit
  // active == 0 ET file vide — le ticket est MANQUE). Ce placement tue le
  // deplacement EXACT de l'activation hors du verrou, pas un delai
  // artificiel ulterieur (l'ancien hook post-section surqualifiait la dent).
  std::function<void()> test_hook_pre_activate;
#endif

  // ERREUR DEVICE FATALE (P1) : le pool devient a USAGE UNIQUE. L'admission
  // ferme, la premiere erreur est memorisee, les tickets en file sont ANNULES
  // avec cette erreur (jamais laisses en attente), toutes les attentes sont
  // reveillees, et les executeurs des travaux actifs ne sont plus reutilises —
  // chaque fil de pool sort apres son travail courant. Une exception de
  // travail ORDINAIRE, elle, reste recuperable : elle remonte au producteur
  // soumettant et le pool continue.
  void close_fatal(std::exception_ptr e) noexcept {
    // GARANTIE FORTE : aucune allocation sous le verrou (l'exception_ptr est
    // construit AVANT), la file est videe par l'avant et chaque ticket est
    // acquitte au moment ou il en sort — un echec partiel est impossible.
    std::exception_ptr fe = e;
    if (!fe) {
      try {
        fe = std::make_exception_ptr(std::runtime_error("pool d'executeurs : erreur device fatale"));
      } catch (...) {  // allocation impossible : on ferme quand meme, sans porteur d'erreur
      }
    }
    {
      std::lock_guard<std::mutex> lk(mu_);
      if (!fatal_) fatal_ = fe;
      stop_ = true;
      while (!queue_.empty()) {
        Ticket* t = queue_.front();
        queue_.pop_front();
        ++cancelled_;
        std::lock_guard<std::mutex> tl(t->mu);
        t->exc = fatal_;
        t->cancelled = true;  // bit INDEPENDANT de l'exception_ptr : meme si
                              // sa fabrication a echoue, le producteur ne
                              // repart jamais comme si le travail avait reussi
        t->done = true;
        t->cv.notify_all();
      }
    }
    cv_.notify_all();
    cv_space_.notify_all();
  }
  bool closed() const {
    std::lock_guard<std::mutex> lk(mu_);
    return stop_;
  }

  // Soumet et ATTEND (contre-pression si la file est pleine) ; relance dans
  // l'appelant l'exception levee par le travail. Le CONFINEMENT DE PANNE est
  // dans le worker (voir run()) : quand l'exception recue est une
  // DeviceFatalError, le pool est DEJA ferme (usage unique, file annulee,
  // executeur empoisonne jamais reutilise) au moment ou elle remonte ici.
  void submit_and_wait(Job job) {
    if (current_pool_job() != nullptr)
      throw std::runtime_error("pool d'executeurs : soumission depuis un travail de pool (aucun emboitement admis)");
    Ticket t;
    t.job = std::move(job);
    {
      std::unique_lock<std::mutex> lk(mu_);
      cv_space_.wait(lk, [&] { return stop_ || queue_.size() < cap_; });
      if (stop_) {  // admission fermee : l'erreur fatale prime sur l'arret ordinaire
        if (fatal_) std::rethrow_exception(fatal_);
        throw std::runtime_error("pool d'executeurs arrete");
      }
      queue_.push_back(&t);
      ++submitted_;  // APRES le push reussi : un bad_alloc de la deque ne desequilibre pas les comptes
      const u32 q = (u32)queue_.size();
      u32 pq = peak_queued_.load(std::memory_order_relaxed);
      while (q > pq && !peak_queued_.compare_exchange_weak(pq, q, std::memory_order_acq_rel)) {}
    }
    cv_.notify_one();
    std::unique_lock<std::mutex> lk(t.mu);
    t.cv.wait(lk, [&] { return t.done; });
    if (t.exc && !MHGP6_MUTANT("pool-drop-exception")) std::rethrow_exception(t.exc);
    if (t.cancelled)  // annule sans porteur d'erreur : JAMAIS un retour normal
      throw std::runtime_error("pool d'executeurs : travail annule par fermeture fatale (erreur sans porteur)");
  }

 private:
  struct Ticket {
    Job job;
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;
    bool cancelled = false;
    std::exception_ptr exc;
  };

  static int check_domain(int executors) {
    if (executors < 1 || executors > kGpuExecutorsMax)
      throw std::invalid_argument("pool d'executeurs : nombre hors de [1, 8] (refus, jamais un clamp)");
    return MHGP6_MUTANT("pool-serial") ? 1 : executors;
  }

  void shutdown_and_join() {
    {
      std::lock_guard<std::mutex> lk(mu_);
      stop_ = true;
    }
    cv_.notify_all();
    cv_space_.notify_all();  // un producteur en attente de place doit voir l'arret
    for (std::thread& t : threads_)
      if (t.joinable()) t.join();
    threads_.clear();
  }

  void run() {
    std::optional<Executor> ex;
    try {
      ex.emplace();  // creation UNE fois par fil de pool
    } catch (...) {
      {
        std::lock_guard<std::mutex> lk(mu_);
        ++failed_;
        if (!first_exc_) first_exc_ = std::current_exception();
      }
      cv_ready_.notify_all();
      return;
    }
    {
      std::lock_guard<std::mutex> lk(mu_);
      ++ready_;
    }
    cv_ready_.notify_all();
    for (;;) {
      Ticket* t = nullptr;
      u32 cur = 0;
      {
        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait(lk, [&] { return stop_ || !queue_.empty(); });
        if (queue_.empty()) return;  // stop_ et rien a faire
        t = queue_.front();
        queue_.pop_front();
        // INSTANT D'ACTIVATION = le retrait, SOUS mu_ (fenetre file->actif
        // fermee, § 5.6 3e contre-lecture) : a tout instant un ticket est
        // soit EN FILE (annulable par close_fatal) soit ACTIF (il va au bout,
        // son executeur se retire ensuite) — jamais entre les deux, ou une
        // fermeture fatale linearisee dans l'intervalle le manquerait.
        // Mutant pool-activate-after-unlock : l'activation retombe apres le
        // deverrouillage (l'ancien intervalle) — le hook pre_activate migre
        // AVEC elle, le closer de la scene-signature s'intercale et expose
        // le ticket manque.
        if (!MHGP6_MUTANT("pool-activate-after-unlock")) {
#if defined(MHGP6_TESTING)
          if (test_hook_pre_activate) test_hook_pre_activate();
#endif
          cur = active_.fetch_add(1, std::memory_order_acq_rel) + 1;
        }
      }
      cv_space_.notify_one();
      if (MHGP6_MUTANT("pool-activate-after-unlock")) {
#if defined(MHGP6_TESTING)
        if (test_hook_pre_activate) test_hook_pre_activate();
#endif
        cur = active_.fetch_add(1, std::memory_order_acq_rel) + 1;
      }
      u32 p = peak_.load(std::memory_order_relaxed);
      while (cur > p && !peak_.compare_exchange_weak(p, cur, std::memory_order_acq_rel)) {}
      current_pool_job() = this;
      bool threw = false;
      std::exception_ptr fatal_exc;  // non nul ssi le travail a leve une DeviceFatalError
      try {
        t->job(*ex);
      } catch (const DeviceFatalError&) {
        threw = true;
        fatal_exc = std::current_exception();
        std::lock_guard<std::mutex> lk(t->mu);
        t->exc = fatal_exc;
      } catch (...) {
        threw = true;
        std::lock_guard<std::mutex> lk(t->mu);
        t->exc = std::current_exception();
      }
      current_pool_job() = nullptr;
      done_.fetch_add(1, std::memory_order_acq_rel);
      // CONFINEMENT (§ 5.6) : fermer l'admission et annuler la file AVANT de
      // notifier le ticket fatal et avant tout retour a la boucle — sinon ce
      // fil peut depiler le travail suivant et l'executer sur l'executeur
      // empoisonne pendant que le producteur reveille n'a pas encore agi.
      if (fatal_exc && !MHGP6_MUTANT("pool-worker-resume-after-fatal")) close_fatal(fatal_exc);
      bool poisoned = false;
      {
        // Desactivation ET compte terminal dans la MEME section critique :
        // aucun snapshot counters() transitoirement non conservatif.
        std::lock_guard<std::mutex> lk(mu_);
        active_.fetch_sub(1, std::memory_order_acq_rel);
        if (threw) ++failed_jobs_;
        else ++succeeded_;
        poisoned = (bool)fatal_;  // erreur fatale (ici ou ailleurs) : cet executeur n'est plus reutilise
      }
      {  // Notification SOUS le verrou du ticket ; apres ce bloc, `t` peut
         // avoir ete detruit par son producteur : ne plus y toucher.
        std::lock_guard<std::mutex> lk(t->mu);
        t->done = true;
        t->cv.notify_all();
      }
      if (poisoned) return;
    }
  }

  int n_;
  size_t cap_;
  mutable std::mutex mu_;
  std::condition_variable cv_, cv_space_, cv_ready_;
  std::deque<Ticket*> queue_;
  bool stop_ = false;
  size_t ready_ = 0, failed_ = 0;
  u64 submitted_ = 0, succeeded_ = 0, failed_jobs_ = 0, cancelled_ = 0;
  std::exception_ptr first_exc_, fatal_;
  std::vector<std::thread> threads_;
  std::atomic<u32> active_{0}, peak_{0}, peak_queued_{0};
  std::atomic<u64> done_{0};
};

}  // namespace gpu
}  // namespace mhgp6
