#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <thread>

namespace scratch_fault {
std::atomic<bool> enabled{false}, claimed{false};
std::atomic<unsigned> failures{0};
std::atomic<std::uint64_t> paid_mebs{0};
std::atomic<std::size_t> failed_bytes{0};
std::atomic<bool> failed_on_caller{false};
std::thread::id caller;
thread_local bool fail_next = false;
void arm(std::uint64_t mebs, std::size_t capacity) noexcept {
  if (!enabled.load() || capacity != 0 || claimed.exchange(true)) return;
  paid_mebs.store(mebs);
  failed_on_caller.store(std::this_thread::get_id() == caller);
  fail_next = true;
}
[[gnu::noinline]] void* allocate(std::size_t n) {
  if (fail_next) {
    fail_next = false;
    failed_bytes.store(n);
    failures.fetch_add(1);
    throw std::bad_alloc();
  }
  if (void* p = std::malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}
[[gnu::noinline]] void release(void* p) noexcept { std::free(p); }
}
void* operator new(std::size_t n) { return scratch_fault::allocate(n); }
void* operator new[](std::size_t n) { return scratch_fault::allocate(n); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  try { return scratch_fault::allocate(n); } catch (...) { return nullptr; }
}
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept {
  try { return scratch_fault::allocate(n); } catch (...) { return nullptr; }
}
void operator delete(void* p) noexcept { scratch_fault::release(p); }
void operator delete[](void* p) noexcept { scratch_fault::release(p); }
void operator delete(void* p, std::size_t) noexcept { scratch_fault::release(p); }
void operator delete[](void* p, std::size_t) noexcept { scratch_fault::release(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { scratch_fault::release(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { scratch_fault::release(p); }

#define main mhgp7_embedded_fixture_driver
#include "fixture_driver.cpp"
#undef main

int main(int argc, char** argv) {
  if (argc != 3) return 2;
  try {
    std::ifstream data(argv[1]);
    unsigned count = 0, n = 0, kmax = 0, nb = 0, nc = 0; std::string name;
    data >> count >> name >> n >> kmax >> nb >> nc;
    need(count == 6 && name == "E5" && n == 5 && kmax == 5, "first_fixture_E5");
    std::vector<P3> points(n);
    for (auto& p : points) data >> p.x >> p.y >> p.z;
    auto ix = build_cloud_index(points);
    std::vector<BallData> balls(nb);
    for (auto& b : balls) {
      b.key.a = integer(data); for (auto& a : b.key.b) a = integer(data); b.key.c = integer(data);
      b.level = level(data);
      unsigned arity = 0, ni = 0, ns = 0; data >> arity >> ni >> ns;
      b.arity = static_cast<u8>(arity); b.n_interior = static_cast<u8>(ni); b.n_shell = static_cast<u8>(ns);
      need(ni <= kBallInteriorMax && ns <= kBallShellMax, "census_shape");
      for (auto sites : {std::span<i32>(b.interior_ids, ni), std::span<i32>(b.shell_ids, ns)}) {
        for (auto& site : sites) {
          unsigned input = 0; data >> input; need(input < n, "site_domain");
          auto at = std::find(ix.upos.begin(), ix.upos.end(), points[input]);
          need(at != ix.upos.end(), "morton_identity"); site = static_cast<i32>(at - ix.upos.begin());
        }
        std::sort(sites.begin(), sites.end());
      }
    }
    u64 total_paid = 0;
    for (int workers : {2, 4}) {
      scratch_fault::caller = std::this_thread::get_id();
      scratch_fault::claimed.store(false); scratch_fault::failures.store(0);
      scratch_fault::paid_mebs.store(0); scratch_fault::failed_bytes.store(0);
      scratch_fault::failed_on_caller.store(false); scratch_fault::enabled.store(true);
      auto failed = build_full_ball_tower(ix, balls, kmax, workers);
      scratch_fault::enabled.store(false);
      need(scratch_fault::failures.load() == 1 && scratch_fault::claimed.load(), "one_real_scratch_allocation_failure");
      need(scratch_fault::failed_bytes.load() == sizeof(NodeRef), "first_scratch_push_allocation_size");
      need(!scratch_fault::failed_on_caller.load() && scratch_fault::paid_mebs.load() > 0,
           "failure_in_admitted_worker_after_paid_MEB");
      need(failed.status == FullBallStatus::kResourceExhausted && failed.orders.empty(), "global_empty_resource_failure");
      need(std::string(failed.reason) == "full_ball_allocation_failed", "exact_bad_alloc_reason");
      need(failed.stats.resolve_work.calls >= scratch_fault::paid_mebs.load() && failed.stats.resolve_work.calls > 0,
           "paid_worker_work_retained_after_failure");
      need(parallel_detail::launch_active.load() == 0, "all_workers_joined_after_failure");
      total_paid += failed.stats.resolve_work.calls;
    }
    std::cout << "{\"status\":\"passed_post_admission_failure\",\"checks\":" << checks
        << ",\"failed_runs\":2,\"retained_MEB_calls\":" << total_paid << "}\n";
    char mode[] = "4";
    char* nominal[] = {argv[0], argv[1], argv[2], mode};
    need(mhgp7_embedded_fixture_driver(4, nominal) == 0, "full_nominal_reuse_after_disarm");
    need(parallel_detail::launch_active.load() == 0, "nominal_reuse_workers_joined");
    return 0;
  } catch (const Failure& error) { std::cerr << error.why << '\n'; return 1; }
  catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
