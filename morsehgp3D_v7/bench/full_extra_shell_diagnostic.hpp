// Optional, allocation-free diagnostic of an already completed census.
// No geometry decision, catalogue, hierarchy or global-parent certificate.
#pragma once

#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "../src/pipeline/expand.hpp"

namespace mhgp7::extra_shell_diagnostic {

inline constexpr const char* kSchema = "mhgp7-extra-shell-diagnostic-v1";

struct Context {
  u64 n, s, kmax_requested, smax;
  const char* input_digest;
};

inline void decimal(FILE* out, i128 value) {
  char buffer[40];
  size_t used = 0;
  u128 magnitude = uabs128(value);
  do {
    buffer[used++] = static_cast<char>('0' + magnitude % 10);
    magnitude /= 10;
  } while (magnitude);
  std::fputc('"', out);
  if (value < 0) std::fputc('-', out);
  while (used) std::fputc(buffer[--used], out);
  std::fputc('"', out);
}

inline bool valid(const CloudIndex& ix, const BallData& b, const Context& context) {
  if (!context.input_digest || std::strlen(context.input_digest) != 64 ||
      context.n != static_cast<u64>(ix.unique_count()) || b.arity < 2 || b.arity > 4 ||
      b.n_interior > kBallInteriorMax || b.n_shell > kBallShellMax ||
      b.n_shell <= b.arity || b.key.a <= 0 || b.level.den <= 0) return false;
  for (const char* p = context.input_digest; *p; ++p)
    if (!(*p >= '0' && *p <= '9') && !(*p >= 'a' && *p <= 'f')) return false;
  i32 seen[kBallInteriorMax + kBallShellMax]{};
  size_t count = 0;
  for (const auto ids : {b.interior(), b.shell()}) {
    for (i32 id : ids) {
      if (id < 0 || id >= ix.unique_count()) return false;
      for (size_t j = 0; j < count; ++j) if (seen[j] == id) return false;
      seen[count++] = id;
    }
  }
  return true;
}

inline void points(FILE* out, const CloudIndex& ix, std::span<const i32> ids) {
  std::fputc('[', out);
  bool first = true;
  for (i32 id : ids) {
    if (!first) std::fputc(',', out);
    first = false;
    const P3& p = ix.upos[static_cast<size_t>(id)];
    std::fprintf(out, "{\"geometry_index\":%" PRId32 ",\"point_id\":%" PRIu64
        ",\"xyz\":[%" PRId64 ",%" PRId64 ",%" PRId64 "]}",
        id, static_cast<u64>(ix.point_id(id)), p.x, p.y, p.z);
  }
  std::fputc(']', out);
}

inline bool write(FILE* out, const CloudIndex& ix, const BallData& b,
                  u64 ball_index, const Context& context) {
  if (!out || !valid(ix, b, context)) return false;
  std::fprintf(out, "{\"schema\":\"%s\",\"type\":\"extra_shell\",\"diagnostic_only\":true,"
      "\"n\":%" PRIu64 ",\"s\":%" PRIu64 ",\"kmax_requested\":%" PRIu64
      ",\"smax\":%" PRIu64 ",\"input_digest\":\"%s\",\"ball_index\":%" PRIu64 ",\"ball_key\":{\"a\":",
      kSchema, context.n, context.s, context.kmax_requested, context.smax, context.input_digest, ball_index);
  decimal(out, b.key.a);
  std::fputs(",\"b\":[", out);
  for (size_t j = 0; j < 3; ++j) {
    if (j) std::fputc(',', out);
    decimal(out, b.key.b[j]);
  }
  std::fputs("],\"c\":", out); decimal(out, b.key.c);
  std::fprintf(out, "},\"squared_radius\":{\"numerator_u64_le\":[%" PRIu64 ",%" PRIu64
      ",%" PRIu64 "],\"denominator\":", b.level.num[0], b.level.num[1], b.level.num[2]);
  decimal(out, b.level.den);
  std::fprintf(out, "},\"minimal_arity\":%u,\"interior\":", static_cast<unsigned>(b.arity));
  points(out, ix, b.interior());
  std::fputs(",\"shell\":", out); points(out, ix, b.shell());
  std::fputs("}\n", out);
  return std::fflush(out) == 0 && !std::ferror(out);
}

}  // namespace mhgp7::extra_shell_diagnostic
