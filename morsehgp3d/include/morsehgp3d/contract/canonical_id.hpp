#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace morsehgp3d::contract {

// Binary representation of one public-contract content identifier.  Textual
// records use the canonical 64-character lowercase hexadecimal rendering.
class CanonicalId {
 public:
  static constexpr std::size_t byte_count = 32U;
  static constexpr std::size_t lower_hex_character_count = 64U;

  CanonicalId() = default;
  explicit CanonicalId(std::array<std::uint8_t, byte_count> bytes) noexcept
      : bytes_(bytes) {}

  [[nodiscard]] static CanonicalId from_lower_hex(std::string_view text);
  [[nodiscard]] std::string to_lower_hex() const;
  [[nodiscard]] const std::array<std::uint8_t, byte_count>& bytes()
      const noexcept {
    return bytes_;
  }

  friend bool operator==(const CanonicalId&, const CanonicalId&) = default;
  friend std::strong_ordering operator<=>(
      const CanonicalId&, const CanonicalId&) = default;

 private:
  std::array<std::uint8_t, byte_count> bytes_{};
};

// Hashes exactly
//   "MorseHGP3D/v2/<record_type>/" || canonical_projection_json
// with SHA-256.  The caller owns construction of the schema-version-free,
// key-sorted canonical JSON projection required by the v2 contract.
[[nodiscard]] CanonicalId canonical_v2_id_from_canonical_json_unchecked(
    std::string_view record_type,
    std::string_view canonical_projection_json);

}  // namespace morsehgp3d::contract
