// Additive correction of the test interposer only: libstdc++ stable_sort may
// allocate via nothrow new, including before the injection window. Every such
// allocation must use the same allocator as this harness's delete overloads.
#include "failure_gate_v1.cpp"

void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  try { return incremental_failure_test::allocate(n); } catch (...) { return nullptr; }
}
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept {
  try { return incremental_failure_test::allocate(n); } catch (...) { return nullptr; }
}
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { std::free(p); }
