#pragma once

// PRIVATE forced include for a byte-recorded overlay. Never include this in
// the nominal product. Only the overlay NoObserver::before_form calls it.
#include <cstdint>
#include <new>
#include <stdexcept>

#define MHGP7_PRIVATE_MEB_FORM_FAULT_HOOK 1
namespace mhgp7_private_fault {
enum class Kind { kNone, kBadAlloc, kLengthError, kRuntimeError };
struct State {
  Kind kind = Kind::kNone;
  bool armed = false, prospective = true;
  std::uint64_t callbacks = 0, previous_paid = 0, hits = 0;
  std::uint64_t throw_paid = 0, throw_certified = 0;
  std::uint8_t throw_q = 0;
};
inline State state;
inline void reset(Kind kind) noexcept {
  state = State{};
  state.kind = kind;
  state.armed = kind != Kind::kNone;
}
inline constexpr const char* kLengthMessage = "mhgp7_private_meb_form_length_error";
inline constexpr const char* kRuntimeMessage = "mhgp7_private_meb_form_runtime_error";
}  // namespace mhgp7_private_fault

inline void mhgp7_test_before_form(std::uint64_t paid, std::uint64_t certified,
                                  std::uint8_t q) {
  auto& s = mhgp7_private_fault::state;
  ++s.callbacks;
  if (paid == 0 || paid != s.previous_paid + 1 || q < 2 || q > 4) s.prospective = false;
  s.previous_paid = paid;
  if (!s.armed || certified < 2) return;
  // One-shot: never leave the hook armed during unwinding or test reporting.
  s.armed = false;
  ++s.hits;
  s.throw_paid = paid;
  s.throw_certified = certified;
  s.throw_q = q;
  switch (s.kind) {
    case mhgp7_private_fault::Kind::kBadAlloc: throw std::bad_alloc();
    case mhgp7_private_fault::Kind::kLengthError:
      throw std::length_error(mhgp7_private_fault::kLengthMessage);
    case mhgp7_private_fault::Kind::kRuntimeError:
      throw std::runtime_error(mhgp7_private_fault::kRuntimeMessage);
    case mhgp7_private_fault::Kind::kNone: return;
  }
}
