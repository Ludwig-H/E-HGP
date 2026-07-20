#include "morsehgp3d/contract/canonical_id.hpp"

#include <iostream>
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

}  // namespace

int main() {
  using morsehgp3d::contract::CanonicalId;
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
