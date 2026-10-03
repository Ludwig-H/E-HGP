// Environnement flottant prive d'une porte : restauration meme si REQUIRE quitte le test.
#pragma once

#include <array>
#include <cfenv>

#if defined(__SSE__) && (defined(__i386__) || defined(__x86_64__))
#include <xmmintrin.h>
#endif

namespace mhgp11::test {

inline constexpr std::array<int, 4> kRoundModes{FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
#if defined(__SSE__) && (defined(__i386__) || defined(__x86_64__))
inline constexpr std::array<unsigned, 4> kFlushModes{0u, 0x8000u, 0x0040u, 0x8040u};
#else
inline constexpr std::array<unsigned, 1> kFlushModes{0u};
#endif

class FenvGuard {
 public:
  FenvGuard() noexcept : saved_(std::fegetenv(&environment_) == 0) {
#if defined(__SSE__) && (defined(__i386__) || defined(__x86_64__))
    csr_ = _mm_getcsr();
#endif
  }
  ~FenvGuard() {
    if (saved_) std::fesetenv(&environment_);
#if defined(__SSE__) && (defined(__i386__) || defined(__x86_64__))
    _mm_setcsr(csr_);
#endif
  }
  FenvGuard(const FenvGuard&) = delete;
  FenvGuard& operator=(const FenvGuard&) = delete;

  bool set(int mode, unsigned flush) noexcept {
    if (!saved_ || std::fesetround(mode) != 0 || std::fegetround() != mode) return false;
#if defined(__SSE__) && (defined(__i386__) || defined(__x86_64__))
    _mm_setcsr((_mm_getcsr() & ~0x8040u) | flush);
    return (_mm_getcsr() & 0x8040u) == flush;
#else
    return flush == 0;
#endif
  }

 private:
  std::fenv_t environment_{};
  bool saved_;
#if defined(__SSE__) && (defined(__i386__) || defined(__x86_64__))
  unsigned csr_ = 0;
#endif
};

}  // namespace mhgp11::test
