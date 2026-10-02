// Statuts transactionnels, raisons (X-macro sur reasons.def), Outcome et Result : aucune exception ne traverse une
// frontiere de module, aucun assert. Port de src/core/status.hpp de la v10 (raccord R2, commit 865f5e6).
//
// Ce qui change par rapport a la v10 :
//   - Outcome est [[nodiscard]] : un refus ignore est une erreur de compilation (-Werror) ;
//   - un refus a toujours une raison : fail(Reason::none) et Result construit depuis une issue ok rendent
//     refusal_without_reason (invariant_violated). La v10 admettait Result<T> r = fail(Reason::none), un succes sans
//     valeur ;
//   - un Result refuse ne contient aucun T (stockage discrimine) : le construire n'alloue pas et ne leve pas. La v10
//     construisait un T par defaut dans tout refus ; si ce constructeur alloue, le refus de guarded pouvait lui-meme
//     lever (audit du 2 octobre 2026). L'acces a la valeur est verifie ;
//   - merge : fusion deterministe de deux issues, pour les reductions paralleles ;
//   - guarded : la garde std::bad_alloc des sondes du raccord R2 (cli::guarded) devient l'outil commun des frontieres
//     de module ;
//   - exit_code : la correspondance entre issue et code de sortie a un seul auteur. Les sondes de la v10 rendaient 2
//     pour tout refus, y compris un invariant viole, que leur en-tete annoncait a 3.
#pragma once

#include <exception>
#include <new>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

#include "core/types.hpp"

namespace mhgp11 {

enum class Status : u8 { ok, invalid_input, unsupported_degeneracy, resource_exhausted, invariant_violated };

enum class Reason : u16 {
#define MHGP11_REASON(name, status, module) name,
#include "core/reasons.def"
#undef MHGP11_REASON
};

// Nombre de raisons de la table : les valeurs de Reason sont 0 .. kReasonCount - 1, dans l'ordre de reasons.def.
inline constexpr u16 kReasonCount = 0
#define MHGP11_REASON(name, status, module) +1
#include "core/reasons.def"
#undef MHGP11_REASON
    ;

// Statut d'une raison ; une valeur hors table est invariant_violated. Les switch n'ont pas de default : le
// compilateur signale ainsi toute raison oubliee (-Wswitch).
constexpr Status status_of(Reason r) noexcept {
  switch (r) {
#define MHGP11_REASON(name, status, module) \
  case Reason::name:                        \
    return Status::status;
#include "core/reasons.def"
#undef MHGP11_REASON
  }
  return Status::invariant_violated;
}

constexpr std::string_view reason_name(Reason r) noexcept {
  switch (r) {
#define MHGP11_REASON(name, status, module) \
  case Reason::name:                        \
    return #name;
#include "core/reasons.def"
#undef MHGP11_REASON
  }
  return "unknown";
}

constexpr std::string_view status_name(Status s) noexcept {
  switch (s) {
    case Status::ok: return "ok";
    case Status::invalid_input: return "invalid_input";
    case Status::unsupported_degeneracy: return "unsupported_degeneracy";
    case Status::resource_exhausted: return "resource_exhausted";
    case Status::invariant_violated: return "invariant_violated";
  }
  return "unknown";
}

// Issue d'une operation : succes (reason == none) ou refus. `order` = plus petit K en echec (0 = sans objet).
struct [[nodiscard]] Outcome {
  Reason reason = Reason::none;
  Order order = 0;

  constexpr Status status() const noexcept { return status_of(reason); }
  constexpr bool ok() const noexcept { return reason == Reason::none; }

  // Priorite deterministe : un refus precede un succes ; entre refus, le plus petit K, puis la plus petite raison
  // dans l'ordre de reasons.def. Ordre total strict sur (ok, order, reason) : deux issues qui ne se precedent pas
  // sont egales.
  constexpr bool precedes(const Outcome& o) const noexcept {
    if (ok() != o.ok()) return !ok();
    if (order != o.order) return order < o.order;
    return static_cast<u16>(reason) < static_cast<u16>(o.reason);
  }

  friend constexpr bool operator==(const Outcome&, const Outcome&) = default;
};

// Refus de raison r a l'ordre k. Un refus a toujours une raison : fail(Reason::none) rend refusal_without_reason.
constexpr Outcome fail(Reason r, Order k = 0) noexcept {
  return Outcome{r == Reason::none ? Reason::refusal_without_reason : r, k};
}

// Fusion deterministe : rend celle des deux issues qui precede l'autre (le minimum de l'ordre total). Commutative,
// associative, idempotente : le resultat d'une reduction ne depend ni de l'ordre d'arrivee des fils ni du
// parenthesage. Toute reduction de refus passe par elle, entre ordres comme a l'interieur d'un ordre : la v10 gardait
// par endroits le premier refus arrive, et son statut dependait alors de l'ordonnancement.
constexpr Outcome merge(const Outcome& a, const Outcome& b) noexcept { return b.precedes(a) ? b : a; }

// Code de sortie d'une sonde pour une issue (docs/ARCHITECTURE.md, paragraphe 5) : 0 conforme, 3 invariant viole,
// 2 tout autre refus.
constexpr int exit_code(const Outcome& o) noexcept {
  if (o.ok()) return 0;
  return o.status() == Status::invariant_violated ? 3 : 2;
}

// Resultat entier ou refus, jamais un prefixe : ok() dit lequel. Un refus ne contient aucun T : le construire
// n'alloue pas et ne leve pas. T se deplace sans lever (static_assert), pour qu'un succes se rende sans risque.
//
// value() et take() exigent ok(). Cet acces est verifie, toujours, pas seulement en build de test : sur un refus il
// termine le processus (std::terminate), que toute porte lit comme un arret anormal. C'est le seul endroit ou une
// precondition violee ne peut pas devenir un refus : la signature rend une reference, il n'y a ni valeur a rendre
// ni issue a renvoyer, et lire un stockage vide serait un comportement indefini. take() consomme le Result
// (std::move(r).take()).
template <class T>
class [[nodiscard]] Result {
  static_assert(!std::same_as<T, Outcome>, "Result<Outcome> n'a pas de sens : rendre Outcome");
  static_assert(std::is_nothrow_move_constructible_v<T>,
                "mhgp11_result_deplacement : Result<T> exige un T qui se deplace sans lever");

 public:
  Result(T value) noexcept : value_(std::move(value)) {}
  Result(Outcome refusal) noexcept : outcome_(refusal.ok() ? fail(Reason::refusal_without_reason) : refusal) {}

  bool ok() const noexcept { return outcome_.ok(); }
  const Outcome& outcome() const noexcept { return outcome_; }
  T& value() noexcept {
    require_value();
    return *value_;
  }
  const T& value() const noexcept {
    require_value();
    return *value_;
  }
  T take() && noexcept {
    require_value();
    return std::move(*value_);
  }

 private:
  void require_value() const noexcept {
    if (!value_.has_value()) std::terminate();
  }

  std::optional<T> value_;
  Outcome outcome_{};
};

// Frontiere de module : rend run(), ou le refus memory_budget si une std::bad_alloc en sort (run rend Outcome ou
// Result<T>). Seule std::bad_alloc est convertie, sans catch (...) : une erreur de programmation ne devient jamais un
// refus memoire (reparation du raccord R2 de la v10 : sous une limite d'adressage, l'exception sortait de main).
template <class Run>
auto guarded(Run&& run) -> decltype(run()) {
  try {
    return run();
  } catch (const std::bad_alloc&) {
    return fail(Reason::memory_budget);
  }
}

}  // namespace mhgp11

// Rend le refus `reason_name_` si la condition est fausse (jamais assert ; une precondition interne violee rend une
// raison de statut invariant_violated). A employer dans une fonction qui rend Outcome ou Result<T>.
#define MHGP11_CHECK(cond, reason_name_)                                \
  do {                                                                  \
    if (!(cond)) return ::mhgp11::fail(::mhgp11::Reason::reason_name_); \
  } while (0)

// Propage un refus : evalue `expr` (de type Outcome) et la rend si elle n'est pas ok.
#define MHGP11_TRY(expr)                                                                     \
  do {                                                                                       \
    if (const ::mhgp11::Outcome mhgp11_try_ = (expr); !mhgp11_try_.ok()) return mhgp11_try_; \
  } while (0)
