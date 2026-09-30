// Independent narrow test: a real pthread_create failure after successful thread creation.
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <pthread.h>
#include <system_error>

#include "sched/pool.hpp"

namespace {
std::atomic<unsigned> calls{0}, created{0}, joined{0};
unsigned fail_at = 0;
}

extern "C" int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
                               void* (*start)(void*), void* arg) {
  using Fn = int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
  static const auto real = reinterpret_cast<Fn>(dlsym(RTLD_NEXT, "pthread_create"));
  if (!real) std::abort();
  if (++calls == fail_at) return EAGAIN;
  const int result = real(thread, attr, start, arg);
  if (!result) ++created;
  return result;
}

extern "C" int pthread_join(pthread_t thread, void** result) {
  using Fn = int (*)(pthread_t, void**);
  static const auto real = reinterpret_cast<Fn>(dlsym(RTLD_NEXT, "pthread_join"));
  if (!real) std::abort();
  const int code = real(thread, result);
  if (!code) ++joined;
  return code;
}

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  fail_at = unsigned(std::strtoul(argv[1], nullptr, 10));
  if (fail_at < 1 || fail_at > 3) return 2;
  bool caught = false;
  try {
    mhgp10::sched::Pool first(4);
  } catch (const std::system_error& error) {
    caught = error.code().value() == EAGAIN;
  }
  const unsigned first_created = created.load(), first_joined = joined.load();
  if (!caught || first_created != fail_at-1 || first_joined != first_created) return 3;
  std::atomic<unsigned> hits{0};
  {
    mhgp10::sched::Pool second(3);
    second.parallel_for(1000, 1, [&](mhgp10::u64 b, mhgp10::u64 e, unsigned) {
      hits.fetch_add(unsigned(e-b));
    });
  }
  if (hits != 1000 || created != first_created+2 || joined != created ||
      mhgp10::sched::in_parallel_region()) return 4;
  std::printf("PASS fail_at=%u first_created=%u first_joined=%u total_created=%u total_joined=%u hits=%u\n",
              fail_at, first_created, first_joined, created.load(), joined.load(), hits.load());
}
