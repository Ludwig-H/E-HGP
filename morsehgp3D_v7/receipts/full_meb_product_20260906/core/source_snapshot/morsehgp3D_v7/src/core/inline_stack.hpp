// POD LIFO: the first InlineCapacity entries stay inline; excess entries
// use an ordinary vector. A failed overflow push leaves the LIFO unchanged.
// back/pop_back require !empty(), exactly like the replaced vector calls.
#pragma once

#include <array>
#include <cstddef>
#include <type_traits>
#include <vector>

namespace mhgp7 {

template<class T, std::size_t InlineCapacity = 64>
class InlineStack {
  static_assert(InlineCapacity > 0);
  static_assert(std::is_trivial_v<T> && std::is_standard_layout_v<T>);

 public:
  InlineStack() = default;
  InlineStack(const InlineStack&) = delete;
  InlineStack& operator=(const InlineStack&) = delete;
  InlineStack(InlineStack&&) = delete;
  InlineStack& operator=(InlineStack&&) = delete;

  bool empty() const noexcept { return inline_size_ == 0; }
  std::size_t size() const noexcept { return inline_size_ + overflow_.size(); }
  T& back() noexcept { return overflow_.empty() ? inline_[inline_size_ - 1] : overflow_.back(); }
  const T& back() const noexcept { return overflow_.empty() ? inline_[inline_size_ - 1] : overflow_.back(); }

  void push_back(const T& value) {
    if (inline_size_ < InlineCapacity) {
      inline_[inline_size_] = value;
      ++inline_size_;
    } else {
      // No inline state changes before an allocation that may throw.
      overflow_.push_back(value);
    }
  }

  void pop_back() noexcept {
    if (overflow_.empty()) --inline_size_;
    else overflow_.pop_back();
  }

  void clear() noexcept {
    overflow_.clear();
    inline_size_ = 0;
  }

 private:
  std::array<T, InlineCapacity> inline_;
  std::size_t inline_size_ = 0;
  std::vector<T> overflow_;
};

}  // namespace mhgp7
