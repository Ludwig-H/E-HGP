#pragma once
// Representation-only sidecar over already certified credits. Not a public
// geometric certificate factory. No borrowed mutable buffer survives build.
#include "../b_q34_factor_plan_20260926/plan.hpp"
#include <span>

namespace mhgp9::audit::ordered_rows {
namespace fp = factor_plan;
using namespace mhgp9::gen;
enum class Mutant { None, ReverseB, WidenMask };

inline unsigned mask_for(fp::Credit a, fp::Credit b, unsigned k, unsigned mask) {
  unsigned out = 0;
  if (k >= 2 && (mask & 2U) != 0 && unsigned(a.q3) + b.q3 < k-1U) out |= 2U;
  if (k >= 3 && (mask & 4U) != 0 && unsigned(a.q4) + b.q4 < k-2U) out |= 4U;
  return out;
}

class Plan final {
 public:
  Plan(std::span<const fp::Credit> a, std::span<const fp::Credit> b,
       unsigned k, unsigned mask, Mutant mutant = Mutant::None) {
    if (k < 1 || k > 10 || (mask & ~6U) != 0 ||
        a.size() > std::numeric_limits<std::uint32_t>::max() ||
        b.size() > std::numeric_limits<std::uint32_t>::max())
      throw std::invalid_argument("ordered_rows.domain");
    const unsigned t3 = k >= 2 ? k-1 : 0, t4 = k >= 3 ? k-2 : 0;
    std::array<std::uint32_t,100> counts{};
    std::array<std::uint8_t,100> slots{};
    slots.fill(255);
    const auto valid = [&](fp::Credit c) {
      if (c.q3 > t3 || c.q4 > t4) throw std::invalid_argument("ordered_rows.credit");
    };
    for (const auto c : a) { valid(c); ++counts[10U*c.q3+c.q4]; }
    for (const auto c : b) valid(c);
    // Unconditional histogram initialization/inspection is reported as 100
    // slots per plan, in addition to the factor scans and the W comparisons.
    offsets_.push_back(0);
    for (unsigned code = 0; code != 100; ++code) {
      if (counts[code] == 0) continue;
      slots[code] = static_cast<std::uint8_t>(classes_++);
      const fp::Credit ac{static_cast<std::uint8_t>(code/10), static_cast<std::uint8_t>(code%10)};
      for (std::size_t j = 0; j != b.size(); ++j) {
        ++work_;
        const auto i = mutant == Mutant::ReverseB ? b.size()-1-j : j;
        auto m = mask_for(ac,b[i],k,mask);
        if (m == 0) continue;
        if (mutant == Mutant::WidenMask) m = 6;
        ranks_.push_back(static_cast<std::uint32_t>(i));
        masks_.push_back(static_cast<std::uint8_t>(m));
        counter_add(mass_,counts[code]);
        if ((m & 2U) != 0) counter_add(q3_,counts[code]);
        if ((m & 4U) != 0) counter_add(q4_,counts[code]);
      }
      if (offsets_.back() != ranks_.size()) ++nonempty_;
      offsets_.push_back(ranks_.size());
    }
    a_slot_.reserve(a.size());
    for (const auto c : a) a_slot_.push_back(slots[10U*c.q3+c.q4]);
  }
  Plan(const Plan&) = delete;
  Plan& operator=(const Plan&) = delete;
  Plan(Plan&&) = delete;
  Plan& operator=(Plan&&) = delete;
  [[nodiscard]] auto a_slots() const { return std::span<const std::uint8_t>(a_slot_); }
  [[nodiscard]] auto offsets() const { return std::span<const u64>(offsets_); }
  [[nodiscard]] auto ranks() const { return std::span<const std::uint32_t>(ranks_); }
  [[nodiscard]] auto masks() const { return std::span<const std::uint8_t>(masks_); }
  [[nodiscard]] u64 mass() const { return mass_; }
  [[nodiscard]] u64 q3_mass() const { return q3_; }
  [[nodiscard]] u64 q4_mass() const { return q4_; }
  [[nodiscard]] u64 work() const { return work_; }
  [[nodiscard]] u64 classes() const { return classes_; }
  [[nodiscard]] u64 nonempty() const { return nonempty_; }
  [[nodiscard]] std::size_t logical_bytes() const {
    return sizeof(*this)+a_slot_.size()+offsets_.size()*sizeof(u64)+ranks_.size()*5;
  }
  [[nodiscard]] std::size_t retained_bytes() const {
    return sizeof(*this)+a_slot_.capacity()+offsets_.capacity()*sizeof(u64)+
        ranks_.capacity()*sizeof(std::uint32_t)+masks_.capacity();
  }
 private:
  std::vector<std::uint8_t> a_slot_,masks_;
  std::vector<std::uint32_t> ranks_;
  std::vector<u64> offsets_;
  u64 mass_{},q3_{},q4_{},work_{},classes_{},nonempty_{};
};
} // namespace mhgp9::audit::ordered_rows
