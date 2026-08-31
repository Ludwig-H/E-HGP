// MorseHGP3D v6 — lecture stricte des entiers signes exposes par les pilotes.
#pragma once

#include <charconv>
#include <cstring>

#include "types.hpp"

namespace mhgp6 {

inline bool parse_i64_exact(const char* text, i64* out) {
  if (text == nullptr || text[0] == '\0' || out == nullptr) return false;
  const char* end = text + std::strlen(text);
  i64 value = 0;
  const auto parsed = std::from_chars(text, end, value, 10);
  if (parsed.ec != std::errc() || parsed.ptr != end) return false;
  *out = value;
  return true;
}

}  // namespace mhgp6
