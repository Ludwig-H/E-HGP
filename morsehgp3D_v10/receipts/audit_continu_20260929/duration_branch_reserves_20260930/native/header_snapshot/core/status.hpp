// Statuts transactionnels et raisons (X-macro sur reasons.def), sans exception a travers les couches.
#pragma once

#include <string_view>
#include <utility>

#include "core/types.hpp"

namespace mhgp10 {

enum class Status : u8 { ok, invalid_input, unsupported_degeneracy, resource_exhausted, invariant_violated };

enum class Reason : u16 {
#define MHGP10_REASON(name, status) name,
#include "core/reasons.def"
#undef MHGP10_REASON
};

constexpr Status status_of(Reason r) {
  switch (r) {
#define MHGP10_REASON(name, status) \
  case Reason::name:                \
    return Status::status;
#include "core/reasons.def"
#undef MHGP10_REASON
  }
  return Status::invariant_violated;
}

constexpr std::string_view reason_name(Reason r) {
  switch (r) {
#define MHGP10_REASON(name, status) \
  case Reason::name:                \
    return #name;
#include "core/reasons.def"
#undef MHGP10_REASON
  }
  return "unknown";
}

constexpr std::string_view status_name(Status s) {
  switch (s) {
    case Status::ok: return "ok";
    case Status::invalid_input: return "invalid_input";
    case Status::unsupported_degeneracy: return "unsupported_degeneracy";
    case Status::resource_exhausted: return "resource_exhausted";
    case Status::invariant_violated: return "invariant_violated";
  }
  return "unknown";
}

// Issue d'une operation. `order` = plus petit K en echec (0 = sans objet).
struct Outcome {
  Reason reason = Reason::none;
  Order order = 0;
  Status status() const { return status_of(reason); }
  bool ok() const { return reason == Reason::none; }
  // Priorite deterministe : plus petit K, puis plus petite raison dans l'ordre de reasons.def.
  bool precedes(const Outcome& o) const {
    if (ok() != o.ok()) return !ok();
    if (order != o.order) return order < o.order;
    return static_cast<u16>(reason) < static_cast<u16>(o.reason);
  }
};

inline Outcome fail(Reason r, Order k = 0) { return Outcome{r, k}; }

template <class T>
class [[nodiscard]] Result {
 public:
  Result(T value) : value_(std::move(value)) {}
  Result(Outcome o) : outcome_(o) {}
  bool ok() const { return outcome_.ok(); }
  const Outcome& outcome() const { return outcome_; }
  T& value() { return value_; }
  const T& value() const { return value_; }
  T take() { return std::move(value_); }

 private:
  T value_{};
  Outcome outcome_{};
};

}  // namespace mhgp10

// Rend invariant_violated avec la raison si la condition est fausse (jamais assert).
#define MHGP10_CHECK(cond, reason_name_)                   \
  do {                                                     \
    if (!(cond)) return ::mhgp10::fail(::mhgp10::Reason::reason_name_); \
  } while (0)
