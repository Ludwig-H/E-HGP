// Session de l'api (api.hpp) : l'unique MemoryBudget et l'unique Pool d'un appel (docs/ARCHITECTURE.md, regle 3 et
// paragraphe 7.1), l'auto-test F5 a la creation et la fin de vie controlee. Ecrit a neuf : la v10 n'avait pas de
// Session (budget global par defaut, retire de la v11 par core/buffer.hpp).
// Portes : mhgp11_api_session_* (liberation, produit vivant, refus), mhgp11_api_session_fault_* (Pool et compte du
// budget refuses), mhgp11_api_selftest_fault_* ; mutants session_sans_liberation, selftest_omis.
#include <memory>

#include "api/internal.hpp"
#include "api/selftest.hpp"

namespace mhgp11::api {

Result<Session> Session::make(const SessionParams& params) noexcept {
  // 1. Pool : parameter_out_of_range (workers hors de 1..256), session_overhead (fil ou allocation refuses).
  Result<std::unique_ptr<sched::Pool>> pool = sched::make_pool({params.workers});
  if (!pool.ok()) return pool.outcome();
  // 2. Compte partage du budget : sa construction alloue (std::make_shared) et peut lever std::bad_alloc.
  Result<std::unique_ptr<MemoryBudget>> budget = guarded([&]() -> Result<std::unique_ptr<MemoryBudget>> {
    return std::make_unique<MemoryBudget>(params.budget_bytes);
  });
  if (!budget.ok()) return budget.outcome();
  // 3. Auto-test F5, sur le fil appelant ; les fils du Pool ont herite de son environnement flottant a leur creation.
  MHGP11_TRY(api_detail::environment_selftest());
  return Session(std::move(pool).take(), std::move(budget).take());
}

Outcome Session::close() noexcept {
  if (budget_ == nullptr) return {};
  return budget_->released();
}

std::string_view output_name(OutputKind kind) noexcept {
  switch (kind) {
    case OutputKind::full: return "full";
  }
  return "unknown";
}

std::string_view stage_name(Stage stage) noexcept {
  switch (stage) {
    case Stage::cloud: return "cloud";
    case Stage::index: return "index";
    case Stage::domain: return "domain";
    case Stage::tree: return "tree";
    case Stage::attach: return "attach";
    case Stage::output: return "output";
    case Stage::write: return "write";
    case Stage::total: return "total";
  }
  return "unknown";
}

}  // namespace mhgp11::api
