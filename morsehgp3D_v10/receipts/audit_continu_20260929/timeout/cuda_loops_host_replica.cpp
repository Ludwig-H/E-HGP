// Audit only: host replicas of cuda_probe.cu's throughput recurrences.
// This does not execute or validate a CUDA kernel or any engine predicate.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>

using i128 = __int128;
volatile int64_t input = std::numeric_limits<int64_t>::min();
volatile int64_t sink = 0;

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  const int mode = std::atoi(argv[1]);
  int64_t x = input;
  if (mode == 64) {
    int64_t acc = 0;
    for (int t = 0; t < 4; ++t) {
      std::fprintf(stderr, "loop64 iteration=%d x=%lld\n", t, static_cast<long long>(x));
      acc += x * (x ^ t);
      x = (x >> 1) ^ acc;
    }
    sink = acc;
  } else if (mode == 128) {
    i128 acc = 0;
    for (int t = 0; t < 4; ++t) {
      std::fprintf(stderr, "loop128 iteration=%d x=%lld\n", t, static_cast<long long>(x));
      acc += i128(x) * (x ^ t);
      x = int64_t(acc >> 7) ^ x;
    }
    sink = int64_t(acc) ^ int64_t(acc >> 64);
  } else {
    return 2;
  }
  return 0;
}
