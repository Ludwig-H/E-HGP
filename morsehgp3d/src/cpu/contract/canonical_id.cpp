#include "morsehgp3d/contract/canonical_id.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace morsehgp3d::contract {
namespace {

constexpr std::array<std::uint32_t, 64U> round_constants{
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};

[[nodiscard]] std::uint8_t hex_nibble(char character) {
  if (character >= '0' && character <= '9') {
    return static_cast<std::uint8_t>(character - '0');
  }
  if (character >= 'a' && character <= 'f') {
    return static_cast<std::uint8_t>(10 + character - 'a');
  }
  throw std::invalid_argument(
      "a CanonicalId must contain lowercase hexadecimal digits only");
}

}  // namespace

CanonicalSha256Builder::CanonicalSha256Builder() noexcept
    : state_{
          0x6a09e667U,
          0xbb67ae85U,
          0x3c6ef372U,
          0xa54ff53aU,
          0x510e527fU,
          0x9b05688cU,
          0x1f83d9abU,
          0x5be0cd19U} {}

void CanonicalSha256Builder::compress_buffer() {
  std::array<std::uint32_t, 64U> schedule{};
  for (std::size_t word = 0U; word < 16U; ++word) {
    const std::size_t offset = word * 4U;
    schedule[word] =
        (static_cast<std::uint32_t>(buffer_[offset]) << 24U) |
        (static_cast<std::uint32_t>(buffer_[offset + 1U]) << 16U) |
        (static_cast<std::uint32_t>(buffer_[offset + 2U]) << 8U) |
        static_cast<std::uint32_t>(buffer_[offset + 3U]);
  }
  for (std::size_t word = 16U; word < schedule.size(); ++word) {
    const std::uint32_t previous_15 = schedule[word - 15U];
    const std::uint32_t previous_2 = schedule[word - 2U];
    const std::uint32_t sigma0 =
        std::rotr(previous_15, 7) ^ std::rotr(previous_15, 18) ^
        (previous_15 >> 3U);
    const std::uint32_t sigma1 =
        std::rotr(previous_2, 17) ^ std::rotr(previous_2, 19) ^
        (previous_2 >> 10U);
    schedule[word] = schedule[word - 16U] + sigma0 +
                     schedule[word - 7U] + sigma1;
  }

  std::uint32_t a = state_[0];
  std::uint32_t b = state_[1];
  std::uint32_t c = state_[2];
  std::uint32_t d = state_[3];
  std::uint32_t e = state_[4];
  std::uint32_t f = state_[5];
  std::uint32_t g = state_[6];
  std::uint32_t h = state_[7];
  for (std::size_t round = 0U; round < schedule.size(); ++round) {
    const std::uint32_t sum1 =
        std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
    const std::uint32_t choice = (e & f) ^ ((~e) & g);
    const std::uint32_t temporary1 =
        h + sum1 + choice + round_constants[round] + schedule[round];
    const std::uint32_t sum0 =
        std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
    const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
    const std::uint32_t temporary2 = sum0 + majority;
    h = g;
    g = f;
    f = e;
    e = d + temporary1;
    d = c;
    c = b;
    b = a;
    a = temporary1 + temporary2;
  }
  state_[0] += a;
  state_[1] += b;
  state_[2] += c;
  state_[3] += d;
  state_[4] += e;
  state_[5] += f;
  state_[6] += g;
  state_[7] += h;
  buffered_byte_count_ = 0U;
}

void CanonicalSha256Builder::update(std::span<const std::uint8_t> bytes) {
  if (finalized_) {
    throw std::logic_error("a finalized SHA-256 builder cannot be updated");
  }
  if (bytes.size() >
      static_cast<std::size_t>(
          std::numeric_limits<std::uint64_t>::max() - total_byte_count_)) {
    throw std::length_error("the SHA-256 input byte length overflows uint64");
  }
  if (total_byte_count_ + static_cast<std::uint64_t>(bytes.size()) >
      std::numeric_limits<std::uint64_t>::max() / 8U) {
    throw std::length_error("the SHA-256 input bit length overflows uint64");
  }
  total_byte_count_ += static_cast<std::uint64_t>(bytes.size());
  for (const std::uint8_t byte : bytes) {
    buffer_[buffered_byte_count_] = byte;
    ++buffered_byte_count_;
    if (buffered_byte_count_ == buffer_.size()) {
      compress_buffer();
    }
  }
}

void CanonicalSha256Builder::update(std::string_view text) {
  const auto* data = reinterpret_cast<const std::uint8_t*>(text.data());
  update(std::span<const std::uint8_t>{data, text.size()});
}

CanonicalId CanonicalSha256Builder::finalize() {
  if (finalized_) {
    throw std::logic_error("a SHA-256 builder can be finalized only once");
  }
  finalized_ = true;
  const std::uint64_t bit_length = total_byte_count_ * std::uint64_t{8U};
  buffer_[buffered_byte_count_] = std::uint8_t{0x80U};
  ++buffered_byte_count_;
  if (buffered_byte_count_ > 56U) {
    while (buffered_byte_count_ < buffer_.size()) {
      buffer_[buffered_byte_count_] = std::uint8_t{0U};
      ++buffered_byte_count_;
    }
    compress_buffer();
  }
  while (buffered_byte_count_ < 56U) {
    buffer_[buffered_byte_count_] = std::uint8_t{0U};
    ++buffered_byte_count_;
  }
  for (std::size_t byte = 0U; byte < 8U; ++byte) {
    const std::size_t shift = (7U - byte) * 8U;
    buffer_[56U + byte] = static_cast<std::uint8_t>(bit_length >> shift);
  }
  buffered_byte_count_ = buffer_.size();
  compress_buffer();

  std::array<std::uint8_t, CanonicalId::byte_count> digest{};
  for (std::size_t word = 0U; word < state_.size(); ++word) {
    for (std::size_t byte = 0U; byte < 4U; ++byte) {
      const std::size_t shift = (3U - byte) * 8U;
      digest[word * 4U + byte] =
          static_cast<std::uint8_t>(state_[word] >> shift);
    }
  }
  return CanonicalId{digest};
}

CanonicalId CanonicalId::from_lower_hex(std::string_view text) {
  if (text.size() != lower_hex_character_count) {
    throw std::invalid_argument(
        "a CanonicalId must contain exactly 64 lowercase hexadecimal digits");
  }
  std::array<std::uint8_t, byte_count> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    const std::uint8_t high = hex_nibble(text[index * 2U]);
    const std::uint8_t low = hex_nibble(text[index * 2U + 1U]);
    bytes[index] = static_cast<std::uint8_t>(
        static_cast<std::uint32_t>(high) * 16U + low);
  }
  return CanonicalId{bytes};
}

std::string CanonicalId::to_lower_hex() const {
  constexpr std::string_view digits = "0123456789abcdef";
  std::string text;
  text.resize(lower_hex_character_count);
  for (std::size_t index = 0U; index < bytes_.size(); ++index) {
    text[index * 2U] = digits[bytes_[index] >> 4U];
    text[index * 2U + 1U] = digits[bytes_[index] & 0x0fU];
  }
  return text;
}

CanonicalId canonical_v2_id_from_canonical_json_unchecked(
    std::string_view record_type,
    std::string_view canonical_projection_json) {
  if (record_type.empty() || record_type.find('/') != std::string_view::npos) {
    throw std::invalid_argument(
        "a v2 canonical record type must be a nonempty path component");
  }
  CanonicalSha256Builder builder;
  builder.update("MorseHGP3D/v2/");
  builder.update(record_type);
  builder.update("/");
  builder.update(canonical_projection_json);
  return builder.finalize();
}

}  // namespace morsehgp3d::contract
