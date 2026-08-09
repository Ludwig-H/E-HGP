#pragma once

#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/gpu/jung_center_cover_prune_mass.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace morsehgp3d::gpu::detail {

struct JungCenterCoverExactReceiptBox final {
  std::array<exact::ExactRational, jung_center_cover_axis_count> lower{};
  std::array<exact::ExactRational, jung_center_cover_axis_count> upper{};
};

enum class JungCenterCoverPatchDecodeStatus : std::uint8_t {
  absent = 0U,
  valid = 1U,
  invalid = 2U,
};

[[nodiscard]] inline JungCenterCoverPatchDecodeStatus
decode_jung_center_cover_receipt_patch(
    const JungCenterCoverQualificationReceipt& receipt,
    std::size_t patch_index,
    JungCenterCoverExactReceiptBox& result) {
  if (patch_index >= jung_center_cover_patch_count) {
    return JungCenterCoverPatchDecodeStatus::invalid;
  }
  const std::uint64_t patch_bit = UINT64_C(1) << patch_index;
  if ((receipt.patch_bounds_valid_mask & patch_bit) == 0U) {
    return JungCenterCoverPatchDecodeStatus::absent;
  }
  for (std::size_t axis = 0U; axis < jung_center_cover_axis_count; ++axis) {
    const std::size_t offset =
        patch_index * jung_center_cover_axis_count + axis;
    const std::uint64_t lower_bits =
        receipt.patch_lower_binary64_bits[offset];
    const std::uint64_t upper_bits =
        receipt.patch_upper_binary64_bits[offset];
    if (!exact::is_finite_binary64_bits(lower_bits) ||
        !exact::is_finite_binary64_bits(upper_bits)) {
      return JungCenterCoverPatchDecodeStatus::invalid;
    }
    result.lower[axis] = exact::ExactRational::from_binary64_bits(lower_bits);
    result.upper[axis] = exact::ExactRational::from_binary64_bits(upper_bits);
    if (result.upper[axis] < result.lower[axis]) {
      return JungCenterCoverPatchDecodeStatus::invalid;
    }
  }
  return JungCenterCoverPatchDecodeStatus::valid;
}

[[nodiscard]] inline std::size_t
jung_center_cover_patch_cover_violation_count(
    const std::array<
        JungCenterCoverExactReceiptBox,
        jung_center_cover_patch_count>& patches,
    const JungCenterCoverExactReceiptBox& required_cover) {
  std::size_t violations = 0U;
  for (std::size_t axis = 0U; axis < jung_center_cover_axis_count; ++axis) {
    std::array<const JungCenterCoverExactReceiptBox*, 4U> ordinal_patches{};
    for (std::size_t ordinal = 0U; ordinal < ordinal_patches.size();
         ++ordinal) {
      ordinal_patches[ordinal] =
          &patches[ordinal << (2U * axis)];
    }
    violations +=
        required_cover.lower[axis] <
        ordinal_patches.front()->lower[axis];
    violations +=
        ordinal_patches.back()->upper[axis] <
        required_cover.upper[axis];
    for (std::size_t ordinal = 0U;
         ordinal + 1U < ordinal_patches.size();
         ++ordinal) {
      const auto& current = *ordinal_patches[ordinal];
      const auto& next = *ordinal_patches[ordinal + 1U];
      violations += current.upper[axis] < next.lower[axis];
      violations += next.lower[axis] < current.lower[axis];
      violations += next.upper[axis] < current.upper[axis];
    }
    for (std::size_t patch_index = 0U;
         patch_index < patches.size();
         ++patch_index) {
      const std::size_t ordinal =
          (patch_index >> (2U * axis)) & 3U;
      const auto& reference = *ordinal_patches[ordinal];
      violations +=
          patches[patch_index].lower[axis] != reference.lower[axis];
      violations +=
          patches[patch_index].upper[axis] != reference.upper[axis];
    }
  }
  return violations;
}

}  // namespace morsehgp3d::gpu::detail
