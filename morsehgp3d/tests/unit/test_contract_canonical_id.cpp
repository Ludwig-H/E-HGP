#include "morsehgp3d/contract/canonical_id.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Function>
void check_invalid_argument(Function&& function, const std::string& message) {
  bool rejected = false;
  try {
    function();
  } catch (const std::invalid_argument&) {
    rejected = true;
  } catch (...) {
  }
  check(rejected, message);
}

template <typename Function>
void check_logic_error(Function&& function, const std::string& message) {
  bool rejected = false;
  try {
    function();
  } catch (const std::logic_error&) {
    rejected = true;
  } catch (...) {
  }
  check(rejected, message);
}

}  // namespace

int main() {
  using morsehgp3d::contract::CanonicalId;
  using morsehgp3d::contract::CanonicalSha256Builder;
  using morsehgp3d::contract::
      canonical_v2_id_from_canonical_json_unchecked;

  constexpr const char* event_projection =
      "{\"center_witness_homogeneous\":{\"denominator\":\"1\","
      "\"unit\":\"input_coordinate_unit\",\"x_numerator\":\"0\","
      "\"y_numerator\":\"0\",\"z_numerator\":\"0\"},"
      "\"interior_ids\":[],\"minimal_support_ids\":[0,1,2],"
      "\"shell_ids\":[0,1,2],\"squared_level_exact\":{"
      "\"denominator\":\"1\",\"numerator\":\"1\","
      "\"unit\":\"input_coordinate_unit_squared\"}}";
  constexpr const char* expected_event_id =
      "5a65c18f98abc78841a56326cd86618827a5ef137908cc158851dbb526d29d2d";
  const CanonicalId event_id =
      canonical_v2_id_from_canonical_json_unchecked(
          "CriticalEvent", event_projection);
  check(
      event_id.to_lower_hex() == expected_event_id,
      "the independent Python v2 CriticalEvent vector matches exactly");
  check(
      CanonicalId::from_lower_hex(expected_event_id) == event_id,
      "the lowercase hexadecimal representation round-trips");
  check(
      canonical_v2_id_from_canonical_json_unchecked(
          "CriticalEvent", "{}")
              .to_lower_hex() ==
          "c5855def115f31a9c1c8c54da40dad8f632659acb6a5de05c0868abbb20ed1b5" &&
          canonical_v2_id_from_canonical_json_unchecked("Input", "[]")
                  .to_lower_hex() ==
              "9acfc97d93e6b378a936c94c9c63a3b8263358ba1d19092d3b6f51c9fd2053b5",
      "domain separation and empty canonical projections match Python");
  check(
      canonical_v2_id_from_canonical_json_unchecked(
          "CriticalEvent", "[]") !=
          canonical_v2_id_from_canonical_json_unchecked("Input", "[]"),
      "record-type domain separation changes the digest");
  CanonicalSha256Builder incremental;
  incremental.update("MorseHGP3D/v2/Critical");
  const std::array<std::uint8_t, 5U> split_bytes{
      static_cast<std::uint8_t>('E'),
      static_cast<std::uint8_t>('v'),
      static_cast<std::uint8_t>('e'),
      static_cast<std::uint8_t>('n'),
      static_cast<std::uint8_t>('t')};
  incremental.update(std::span<const std::uint8_t>{split_bytes});
  incremental.update("/");
  incremental.update(event_projection);
  check(
      incremental.finalize() == event_id,
      "incremental SHA-256 is independent of update chunk boundaries");

  constexpr std::array<std::size_t, 6U> boundary_lengths{
      0U, 55U, 56U, 63U, 64U, 65U};
  constexpr std::array<const char*, 6U> boundary_digests{
      "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
      "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318",
      "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a",
      "7d3e74a05d7db15bce4ad9ec0658ea98e3f06eeecf16b4c6fff2da457ddc2f34",
      "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb",
      "635361c48bb9eab14198e76ea8ab7f1a41685d6ad62aa9146d301d4f17eb0ae0"};
  for (std::size_t vector_index = 0U;
       vector_index < boundary_lengths.size();
       ++vector_index) {
    const std::string payload(boundary_lengths[vector_index], 'a');
    CanonicalSha256Builder contiguous;
    contiguous.update(payload);
    check(
        contiguous.finalize().to_lower_hex() ==
            boundary_digests[vector_index],
        "SHA-256 matches independent vectors around both padding and block boundaries");

    const std::size_t split =
        std::min<std::size_t>(63U, payload.size());
    CanonicalSha256Builder split_builder;
    split_builder.update(std::string_view{payload}.substr(0U, split));
    split_builder.update(std::string_view{payload}.substr(split));
    check(
        split_builder.finalize().to_lower_hex() ==
            boundary_digests[vector_index],
        "SHA-256 split updates preserve vectors across the 63/64-byte boundary");
    if (payload.size() >= 64U) {
      CanonicalSha256Builder block_split_builder;
      block_split_builder.update(
          std::string_view{payload}.substr(0U, 64U));
      block_split_builder.update(
          std::string_view{payload}.substr(64U));
      check(
          block_split_builder.finalize().to_lower_hex() ==
              boundary_digests[vector_index],
          "SHA-256 preserves vectors when an update ends on a full block");
    }
  }
  CanonicalSha256Builder abc_builder;
  abc_builder.update("abc");
  check(
      abc_builder.finalize().to_lower_hex() ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
      "SHA-256 matches the standard abc vector");
  check_logic_error(
      [&abc_builder] { abc_builder.update("x"); },
      "a finalized SHA-256 builder rejects further updates");
  check_logic_error(
      [&abc_builder] { static_cast<void>(abc_builder.finalize()); },
      "a SHA-256 builder rejects a second finalization");
  check_invalid_argument(
      [] { static_cast<void>(CanonicalId::from_lower_hex("abc")); },
      "a short canonical identifier is rejected");
  check_invalid_argument(
      [] {
        static_cast<void>(CanonicalId::from_lower_hex(
            "5A65C18F98ABC78841A56326CD86618827A5EF137908CC158851DBB526D29D2D"));
      },
      "uppercase hexadecimal is rejected");
  check_invalid_argument(
      [] {
        static_cast<void>(
            canonical_v2_id_from_canonical_json_unchecked(
                "Bad/Type", "{}"));
      },
      "a record type containing a path separator is rejected");

  if (failures != 0) {
    std::cerr << failures << " canonical-id test(s) failed\n";
    return 1;
  }
  std::cout << "all canonical-id tests passed\n";
  return 0;
}
