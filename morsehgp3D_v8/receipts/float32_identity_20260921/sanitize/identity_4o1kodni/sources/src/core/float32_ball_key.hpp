#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "core/float32_ball.hpp"

namespace mhgp8 {

struct Float32KeyWork {
  // Factory validity and exact coefficient arithmetic are paid here. During
  // from_support, only exact_* arithmetic/decodes grow in this subrecord;
  // no support validity or power query is repeated by coefficient expansion.
  Float32BallWork support;
  std::uint64_t q2_requests{}, q3_requests{}, q4_requests{}, from_support_requests{};
  std::uint64_t rejected_supports{}, keys_created{};
  std::uint64_t canonical_gcd_calls{}, canonical_divisions{}, packed_words{};
  bool operator==(const Float32KeyWork&) const = default;
};

// Emission-only geometric identity: primitive integers (A,Bx,By,Bz,C), A>0,
// gcd=1, defining A*|Q|^2+B.Q+C with Q=point*2^149. No support IDs, arity,
// q_min, hierarchy level or radius-only identity. Equal keys mean the same
// entire ball across arities. This is NOT a radius-order comparator.
//
// Unique word encoding v1: first word 1; then five coefficients. Zero is
// header0. Nonzero header=(word_count<<1)|negative, followed by trailing
// zero bit count and the odd magnitude's active u32 words, least significant
// first. No redundant highest zero word. This is a portable WORD encoding;
// a byte stream must serialize each word explicitly little endian.
// Only checked factories publish a key, with no mutable view or raw decoder.
// Dynamic storage is confined to emission, never a rejected candidate. Value
// copies (including optional factory return moves) copy the owned encoding;
// from_support returns a direct prvalue without that extra copy.
class Float32BallKey final {
 public:
  [[nodiscard]] static std::optional<Float32BallKey> make_q2(
      const Float32Point3& a, const Float32Point3& b, Float32KeyWork& work);
  [[nodiscard]] static std::optional<Float32BallKey> make_q3(
      std::array<Float32Point3, 3> points, Float32KeyWork& work);
  [[nodiscard]] static std::optional<Float32BallKey> make_q4(
      std::array<Float32Point3, 4> points, Float32KeyWork& work);
  [[nodiscard]] static Float32BallKey from_support(const Float32Ball& support, Float32KeyWork& work);

  Float32BallKey(const Float32BallKey&) = default;
  // Const storage intentionally makes moving a value copy its encoding;
  // no observable moved-from empty/noncanonical key is introduced.
  Float32BallKey(Float32BallKey&&) = default;
  Float32BallKey& operator=(const Float32BallKey&) = delete;
  Float32BallKey& operator=(Float32BallKey&&) = delete;
  [[nodiscard]] std::span<const std::uint32_t> serialized_words() const noexcept { return words_; }
  [[nodiscard]] bool operator==(const Float32BallKey& other) const noexcept { return words_ == other.words_; }
  [[nodiscard]] std::size_t capacity_bytes() const noexcept { return words_.capacity() * sizeof(std::uint32_t); }

 private:
  explicit Float32BallKey(std::vector<std::uint32_t> words) : words_(std::move(words)) {}
  [[nodiscard]] static Float32BallKey canonical(
      std::array<float32_ball_detail::FixedSigned, 5> coefficients, Float32KeyWork& work);
  const std::vector<std::uint32_t> words_;
};

}  // namespace mhgp8
