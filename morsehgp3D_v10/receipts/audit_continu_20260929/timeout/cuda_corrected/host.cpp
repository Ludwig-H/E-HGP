#include <cstdint>
#include <cstdio>
#include <limits>
#include <cstddef>
#include <initializer_list>
using u128 = unsigned __int128;
using i128 = __int128;

// Host replica only. These recurrence bodies are checked verbatim against
// cuda_probe.cu by run.py; the CUDA index/launch machinery is not replicated.
int64_t loop64(int64_t input, int iters) {
  uint64_t x = uint64_t(input), acc = 0;
  for (int t = 0; t < iters; ++t) {
    acc += x * (x ^ uint64_t(t));
    x = (x >> 1) ^ acc;
  }
  return int64_t(acc);
}
int64_t loop128(int64_t input, int iters) {
  uint64_t x = uint64_t(input);
  u128 acc = 0;
  for (int t = 0; t < iters; ++t) {
    acc += u128(x) * (x ^ uint64_t(t));
    x = uint64_t(acc >> 7) ^ x;
  }
  return int64_t(uint64_t(acc) ^ uint64_t(acc >> 64));
}
int main() {
  const int64_t values[] = {INT64_MIN, INT64_MIN + 1, -2, -1, 0, 1, 2, INT64_MAX};
  for (int64_t v : values) {
    for (int t : {0, 1, 2, 16, 64, 4096}) {
      std::printf("L %lld %d %llu %llu\n", (long long)v, t,
                  (unsigned long long)uint64_t(loop64(v, t)),
                  (unsigned long long)uint64_t(loop128(v, t)));
    }
  }
  for (int64_t a : values) for (int64_t b : values)
    for (int64_t c : values) for (int64_t d : values) {
      const i128 x = i128(a) * b, y = i128(c) * d;
      const int got = x < y ? -1 : (x > y ? 1 : 0);
      std::printf("C %lld %lld %lld %lld %d\n", (long long)a, (long long)b,
                  (long long)c, (long long)d, got);
    }
}
