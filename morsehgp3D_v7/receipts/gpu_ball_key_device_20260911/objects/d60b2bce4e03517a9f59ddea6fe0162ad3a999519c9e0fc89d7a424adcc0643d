#pragma once
#include <cstddef>
#include "ball_key_device.cuh"

namespace mhgp7::gpu_ball_key_private {
// Transport contains only fixed-width u64 words. No native i128 object ABI
// crosses the device boundary. Every signed scalar is two's-complement low/high.
struct WireInput { u64 words[14]{}; };
struct WireOutput { u64 words[12]{}; };
static_assert(sizeof(WireInput) == 112 && sizeof(WireOutput) == 96);
static_assert(alignof(WireInput) == 8 && alignof(WireOutput) == 8);
static_assert(offsetof(WireInput, words) == 0 && offsetof(WireOutput, words) == 0);
MHGP7_HD inline u128 wire_u128(const u64* words) { return ((u128)words[1] << 64) | words[0]; }
MHGP7_HD inline i128 wire_i128(const u64* words) { return (i128)wire_u128(words); }
MHGP7_HD inline void wire_store(u64* words, u128 value) { words[0] = (u64)value; words[1] = (u64)(value >> 64); }
MHGP7_HD inline WireOutput wire_evaluate(const WireInput& input) {
  WireOutput output{};
  const auto* words = input.words;
  if (words[0] == 1) {
    const Reduction result = reduce({wire_i128(words + 1),
        {wire_i128(words + 3), wire_i128(words + 5), wire_i128(words + 7)}, wire_i128(words + 9)});
    output.words[0] = (u64)result.status;
    wire_store(output.words + 1, (u128)result.key.a);
    for (int i = 0; i < 3; ++i) wire_store(output.words + 3 + 2 * i, (u128)result.key.b[i]);
    wire_store(output.words + 9, (u128)result.key.c);
  } else if (words[0] == 2) {
    wire_store(output.words + 1, ugcd128(wire_u128(words + 1), wire_u128(words + 3)));
  } else if (words[0] == 3) {
    const Division result = divide128(wire_i128(words + 1), wire_i128(words + 3));
    output.words[0] = (u64)result.status;
    wire_store(output.words + 1, (u128)result.quotient);
    wire_store(output.words + 3, (u128)result.remainder);
  } else if (words[0] == 4) {
    SupportRequest request{};
    // Explicitly reject counts that would truncate on a u64 -> u32 conversion.
    if (words[1] > 4) { output.words[0] = (u64)Status::invalid_support; return output; }
    request.arity = (u32)words[1];
    for (int i = 0; i < 4; ++i)
      request.points[i] = {(i64)words[2 + i * 3], (i64)words[3 + i * 3], (i64)words[4 + i * 3]};
    const Reduction result = support_key(request);
    output.words[0] = (u64)result.status;
    wire_store(output.words + 1, (u128)result.key.a);
    for (int i = 0; i < 3; ++i) wire_store(output.words + 3 + 2 * i, (u128)result.key.b[i]);
    wire_store(output.words + 9, (u128)result.key.c);
  } else {
    output.words[0] = (u64)Status::invalid_support;
  }
  return output;
}
}  // namespace mhgp7::gpu_ball_key_private
