#pragma once
#include <cstdint>
#include <limits>

namespace audit_euler {
using U32 = std::uint32_t;
inline constexpr U32 kNone = std::numeric_limits<U32>::max();

// Borrowed certified Euler table: [tin,tout), unique tin per node. N may equal
// kNone; then tout==kNone is a valid exclusive end. Node indices remain <kNone.
struct Entry {
  U32 node = 0;
  U32 tin = 0;
  U32 tout = 0;
};

inline bool valid(const Entry& a, U32 n) noexcept {
  return a.node != kNone && a.node < n && a.tin < a.tout && a.tout <= n;
}

inline bool left_better(const Entry& a, const Entry& b) noexcept {
  if (a.tout != b.tout) return a.tout < b.tout;
#if defined(AUDIT_MUTANT_DROP_TIN)
  return false;  // causal mutant: selected final-child ancestor wins a tie
#else
  if (a.tin != b.tin) return a.tin > b.tin;
  return a.node < b.node;  // irrelevant under unique Euler tin; deterministic
#endif
}

inline bool right_better(const Entry& a, const Entry& b) noexcept {
  if (a.tin != b.tin) return a.tin > b.tin;
  if (a.node != b.node) return a.node < b.node;
  return a.tout < b.tout;
}

// Empty is explicit. No node sentinel, signed conversion, negation, packed
// integer key, addition or multiplication participates in the comparisons.
struct Summary {
  bool has = false;
  Entry left{};
  Entry right{};

  void push(const Entry& a) noexcept {
    if (!has) {
      has = true;
      left = right = a;
      return;
    }
    if (left_better(a, left)) left = a;
    if (right_better(a, right)) right = a;
  }

  void merge(const Summary& other) noexcept {
    if (!other.has) return;
    if (!has) {
      *this = other;
      return;
    }
    if (left_better(other.left, left)) left = other.left;
    if (right_better(other.right, right)) right = other.right;
  }
};
}  // namespace audit_euler
