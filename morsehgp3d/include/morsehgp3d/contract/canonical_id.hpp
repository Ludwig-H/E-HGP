#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <span>
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

// Incremental SHA-256 for bounded-memory canonical manifests.  The builder is
// deliberately byte-oriented: callers own the versioned, endian-stable record
// encoding and must provide their own domain separator.
class CanonicalSha256Builder {
 public:
  CanonicalSha256Builder() noexcept;

  void update(std::span<const std::uint8_t> bytes);
  void update(std::string_view text);
  [[nodiscard]] CanonicalId finalize();

 private:
  void compress_buffer();

  std::array<std::uint32_t, 8U> state_{};
  std::array<std::uint8_t, 64U> buffer_{};
  std::size_t buffered_byte_count_{0U};
  std::uint64_t total_byte_count_{0U};
  bool finalized_{false};
};

// Hashes exactly
//   "MorseHGP3D/v2/<record_type>/" || canonical_projection_json
// with SHA-256.  The caller owns construction of the schema-version-free,
// key-sorted canonical JSON projection required by the v2 contract.
[[nodiscard]] CanonicalId canonical_v2_id_from_canonical_json_unchecked(
    std::string_view record_type,
    std::string_view canonical_projection_json);

}  // namespace morsehgp3d::contract
