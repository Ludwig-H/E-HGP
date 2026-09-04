// Allocation-failure audit of the provisional archive destructor.
// The "fail" arm refuses allocations only after a valid input was written.
// Exit 97 is a test-only witness that std::terminate was reached.
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <new>
#include <string>

#include "../src/io/archive.hpp"

namespace {
bool fail_allocations = false;
}

[[gnu::noinline]] void* operator new(std::size_t bytes) {
  if (fail_allocations) throw std::bad_alloc();
  if (void* result = std::malloc(bytes ? bytes : 1)) return result;
  throw std::bad_alloc();
}
[[gnu::noinline]] void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
[[gnu::noinline]] void operator delete(void* pointer) noexcept { std::free(pointer); }
[[gnu::noinline]] void operator delete[](void* pointer) noexcept { std::free(pointer); }
[[gnu::noinline]] void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
[[gnu::noinline]] void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }

int main(int argc, char** argv) {
  if (argc != 3) return 2;
  std::set_terminate([] { std::_Exit(97); });
  const std::filesystem::path destination = argv[1];
  const std::string mode = argv[2];
  if (mode != "normal" && mode != "fail") return 2;
  {
    mhgp7::ForestArchive archive(destination);
    archive.input({{1, {0, 0, 0}}, {2, {2, 3, 5}}});
    fail_allocations = mode == "fail";
  }
  fail_allocations = false;
  std::puts("destructor_returned");
  return 0;
}
