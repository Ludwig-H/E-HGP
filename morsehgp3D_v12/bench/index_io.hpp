// Entrees entieres et mots de preuve du banc index, repris explicitement du banc catalogue v11.
#pragma once
#include <algorithm>
#include <array>
#include <charconv>
#include <fstream>
#include <string_view>
#include "index/index.hpp"

namespace index_bench {
using namespace mhgp12;
struct Input {
  Buffer<u32> x, y, z;
  Buffer<PointId> ids;
};

bool parse(std::string_view text, u64& value) {
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  return error == std::errc{} && end == text.data() + text.size();
}

u32 little(const unsigned char* p) {
  return u32{p[0]} | (u32{p[1]} << 8) | (u32{p[2]} << 16) | (u32{p[3]} << 24);
}

Result<Input> read_input(const char* xyz, const char* ids, MemoryBudget& budget) {
  std::ifstream points(xyz, std::ios::binary | std::ios::ate), identities(ids, std::ios::binary | std::ios::ate);
  if (!points || !identities) return fail(Reason::input_unreadable);
  const auto bytes = points.tellg(), id_bytes = identities.tellg();
  if (bytes <= 0 || bytes % 12 != 0 || id_bytes < 0 || id_bytes != bytes / 3)
    return fail(Reason::input_unreadable);
  const u64 count = static_cast<u64>(bytes / 12);
  if (count >= kNone) return fail(Reason::index_overflow_u32);
  Input input;
  MHGP12_TRY(input.x.allocate(count, budget));
  MHGP12_TRY(input.y.allocate(count, budget));
  MHGP12_TRY(input.z.allocate(count, budget));
  MHGP12_TRY(input.ids.allocate(count, budget));
  points.seekg(0); identities.seekg(0);
  std::array<unsigned char, 12 * 4096> block{};
  std::array<unsigned char, 4 * 4096> names{};
  for (u64 begin = 0; begin < count; begin += 4096) {
    const u64 n = std::min(u64{4096}, count - begin);
    points.read(reinterpret_cast<char*>(block.data()), static_cast<std::streamsize>(12 * n));
    identities.read(reinterpret_cast<char*>(names.data()), static_cast<std::streamsize>(4 * n));
    if (!points || !identities) return fail(Reason::input_unreadable);
    for (u64 j = 0; j < n; ++j) {
      input.x[begin + j] = little(block.data() + 12 * j);
      input.y[begin + j] = little(block.data() + 12 * j + 4);
      input.z[begin + j] = little(block.data() + 12 * j + 8);
      input.ids[begin + j] = make_id<PointId>(little(names.data() + 4 * j));
    }
  }
  return input;
}

void word(std::ostream& out, u64 value) {
  std::array<char, 8> bytes{};
  for (unsigned i = 0; i < 8; ++i) bytes[i] = static_cast<char>((value >> (8 * i)) & 255);
  out.write(bytes.data(), bytes.size());
}


}  // namespace index_bench
