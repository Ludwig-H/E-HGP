// MorseHGP3D v9 — porte du bassin de blocs residents des enregistrements des
// voies (gpu/record_pool.hpp, levier pinned_records, 25 septembre 2026).
//
// Juge, sur le build CPU (la memoire epinglee de l'appareil est remplacee par
// une memoire de test qui compte et qui sait echouer) :
//   - croissance a la demande : une requete plus grande remplace le bloc
//     libre le plus grand (compte : allocations, croissances, ancienne memoire
//     rendue), une plus petite le reutilise sans allocation, jamais de
//     retrecissement ; reserve() prepare un bloc sans rien louer ;
//   - exclusivite : un bloc loue n'est jamais rendu une seconde fois ; la
//     seconde location prend un autre bloc (compte second_blocks), les motifs
//     ecrits dans les deux restent intacts ;
//   - retour a la destruction : destructeur, reset() idempotent, deplacement
//     (le deplace est vide, l'affectation rend l'ancien bloc) ;
//   - locations concurrentes : huit fils louent a la fois (barriere), ecrivent
//     leur motif, le relisent, rendent ; jamais deux baux sur un bloc, jamais
//     plus de blocs que de baux simultanes ;
//   - surete aux exceptions : une allocation qui echoue (croissance ou
//     creation) laisse le bassin tel quel (bloc garde son ancienne memoire,
//     rien de loue) et l'exception atteint l'appelant ;
//   - chemin du jumeau hote (lease_records) : les enregistrements passes dans
//     un bail sont identiques octet pour octet, le vecteur est vide.
// Mutants (code 1) : MHGP9_RECORD_POOL_MUTANT_TAKE_LEASED (un bloc loue parait
// libre), MHGP9_RECORD_POOL_MUTANT_NO_RETURN (le bail ne rend pas son bloc),
// MHGP9_RECORD_POOL_MUTANT_KEEP_TAKEN_ON_THROW (un echec d'allocation laisse
// le bloc pris), MHGP9_LANES_MUTANT_PINNED_SHORT_COPY (copie courte d'un
// enregistrement dans le bail).
//
//   mhgp9_gpu_record_pool_gate [--rounds=64]
//
// Code 0 conforme, 1 desaccord ou mutant survivant (`cause=`), 2 argument,
// 3 plancher.
#include <atomic>
#include <barrier>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <new>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "../../src/gpu/lanes_host.hpp"

namespace {

using namespace mhgp9;
using Pool = gpu::ResidentBlockPool<gpu::LaneRecord>;

int fail(const std::string& cause) {
  std::printf("cause=%s\n", cause.c_str());
  return 1;
}

// Counting memory that can fail its next allocation.
class TestMemory final : public gpu::BlockMemory {
 public:
  void* allocate(std::size_t bytes) override {
    if (fail_next.exchange(false)) throw std::bad_alloc();
    void* p = ::operator new(bytes, std::align_val_t{64});
    std::lock_guard<std::mutex> lock(mu);
    ++allocations;
    live.insert(p);
    return p;
  }
  void release(void* pointer) noexcept override {
    {
      std::lock_guard<std::mutex> lock(mu);
      ++releases;
      if (live.erase(pointer) != 1) ++foreign_releases;
    }
    ::operator delete(pointer, std::align_val_t{64});
  }
  std::atomic<bool> fail_next{false};
  std::mutex mu;
  std::set<void*> live;
  std::uint64_t allocations = 0, releases = 0, foreign_releases = 0;
};

void fill(gpu::LaneRecord* data, std::size_t count, unsigned char pattern) {
  std::memset(static_cast<void*>(data), pattern, count * sizeof(gpu::LaneRecord));
}

bool holds(const gpu::LaneRecord* data, std::size_t count, unsigned char pattern) {
  const auto* bytes = reinterpret_cast<const unsigned char*>(data);
  for (std::size_t i = 0; i < count * sizeof(gpu::LaneRecord); ++i)
    if (bytes[i] != pattern) return false;
  return true;
}

constexpr std::size_t record_bytes = sizeof(gpu::LaneRecord);

struct Tally {
  unsigned long long cases = 0, allocations = 0, growths = 0, second_blocks = 0, failures = 0;
  unsigned long long concurrent_leases = 0, twin_records = 0;
};

// Growth on demand, reuse, no shrink, reserve, empty lease.
int grow_on_demand(Tally& tally) {
  TestMemory memory;
  {
    Pool pool(memory);
    {
      const auto empty = pool.acquire(0);
      if (empty || empty.data() != nullptr || empty.size() != 0 || pool.stats().acquisitions != 0 ||
          memory.allocations != 0)
        return fail("grow.empty_lease");
    }
    const gpu::LaneRecord* first = nullptr;
    {
      auto a = pool.acquire(100);
      const auto s = pool.stats();
      if (!a || a.size() != 100 || a.block() != 0 || s.allocations != 1 || s.growths != 0 || s.blocks != 1 ||
          s.leased != 1 || s.bytes != Pool::grown_capacity(100) * record_bytes || memory.allocations != 1)
        return fail("grow.first");
      fill(a.data(), a.size(), 0x5a);
      first = a.data();
    }
    if (pool.stats().leased != 0) return fail("grow.returned");
    {
      auto b = pool.acquire(50);  // fits: the same block, no allocation
      if (b.data() != first || b.block() != 0 || memory.allocations != 1 || !holds(b.data(), 50, 0x5a))
        return fail("grow.reuse");
    }
    {
      auto c = pool.acquire(200);  // grows block 0, releases its former memory
      const auto s = pool.stats();
      if (c.block() != 0 || s.allocations != 2 || s.growths != 1 || s.blocks != 1 || memory.releases != 1 ||
          memory.foreign_releases != 0 || s.bytes != Pool::grown_capacity(200) * record_bytes)
        return fail("grow.growth");
      fill(c.data(), c.size(), 0x33);
    }
    {
      auto d = pool.acquire(10);  // never shrinks
      if (pool.stats().bytes != Pool::grown_capacity(200) * record_bytes || memory.allocations != 2)
        return fail("grow.no_shrink");
    }
    pool.reserve(0);
    pool.reserve(150);  // already held: nothing allocated, nothing leased
    if (pool.stats().allocations != 2 || pool.stats().leased != 0 || pool.stats().reservations != 1)
      return fail("grow.reserve_held");
    pool.reserve(1000);  // grows the free block, leases nothing
    {
      const auto s = pool.stats();
      // reserve allocates exactly the request (no headroom).
      if (s.allocations != 3 || s.growths != 2 || s.leased != 0 || s.bytes != 1000 * record_bytes)
        return fail("grow.reserve_growth");
    }
    {
      auto e = pool.acquire(1000);  // reserved: no allocation in the call
      if (pool.stats().allocations != 3) return fail("grow.reserved_acquire");
    }
    tally.allocations += pool.stats().allocations;
    tally.growths += pool.stats().growths;
  }
  if (memory.live.size() != 0 || memory.releases != memory.allocations) return fail("grow.pool_release");
  tally.cases += 9;
  return 0;
}

// Exclusivity and return on destruction, moves and reset.
int exclusivity(Tally& tally) {
  TestMemory memory;
  Pool pool(memory);
  auto a = pool.acquire(64);
  fill(a.data(), a.size(), 0x11);
  auto b = pool.acquire(64);  // block 0 is leased: another block
  // Checked before any access: a shared block would alias the first lease.
  if (!b || b.block() == a.block() || b.data() == a.data()) return fail("exclusive.same_block");
  if (pool.stats().second_blocks != 1 || pool.stats().leased != 2 || pool.stats().blocks != 2)
    return fail("exclusive.counts");
  fill(b.data(), b.size(), 0x22);
  if (!holds(a.data(), a.size(), 0x11) || !holds(b.data(), b.size(), 0x22)) return fail("exclusive.overwrite");
  // Move: the moved-from lease is empty and returns nothing.
  auto c = std::move(a);
  if (a || a.data() != nullptr || !c || c.block() != 0 || pool.stats().leased != 2) return fail("exclusive.move");
  a.reset();  // empty: no effect
  if (pool.stats().leased != 2) return fail("exclusive.reset_empty");
  // Move assignment returns the target's former block.
  c = std::move(b);
  if (pool.stats().leased != 1 || c.block() != 1 || !holds(c.data(), c.size(), 0x22)) return fail("exclusive.assign");
  c.reset();
  c.reset();  // idempotent
  if (pool.stats().leased != 0) return fail("exclusive.reset");
  {
    auto d = pool.acquire(64);  // both blocks free: the lowest index of the best fit
    if (d.block() != 0 || memory.allocations != 2) return fail("exclusive.best_fit");
  }
  {
    auto e = pool.acquire(64);
    auto f = pool.acquire(64);
    auto g = pool.acquire(8);  // the third lease: a third block (created)
    if (e.block() == f.block() || g.block() == e.block() || g.block() == f.block() || pool.stats().blocks != 3)
      return fail("exclusive.third");
  }
  if (pool.stats().leased != 0) return fail("exclusive.destruction");
  tally.second_blocks += pool.stats().second_blocks;
  tally.cases += 9;
  return 0;
}

// Eight threads lease at once, write, check and return, round after round.
int concurrent(std::size_t rounds, Tally& tally) {
  constexpr std::size_t threads = 8;
  TestMemory memory;
  Pool pool(memory);
  std::barrier sync(static_cast<std::ptrdiff_t>(threads));
  std::mutex mu;
  std::set<std::size_t> held;
  std::atomic<bool> broken{false};
  std::atomic<unsigned long long> leases{0};
  std::vector<std::thread> pool_threads;
  for (std::size_t t = 0; t < threads; ++t)
    pool_threads.emplace_back([&, t] {
      for (std::size_t r = 0; r < rounds; ++r) {
        const std::size_t count = 16 + 7 * ((t * 131 + r * 17) % 40);
        auto lease = pool.acquire(count);
        {
          std::lock_guard<std::mutex> lock(mu);
          if (!held.insert(lease.block()).second) broken = true;  // two live leases on one block
        }
        if (!broken) fill(lease.data(), lease.size(), static_cast<unsigned char>(t + 1));
        sync.arrive_and_wait();  // every lease alive at once
        if (!broken && !holds(lease.data(), lease.size(), static_cast<unsigned char>(t + 1))) broken = true;
        ++leases;
        {
          std::lock_guard<std::mutex> lock(mu);
          held.erase(lease.block());
        }
        lease.reset();
        sync.arrive_and_wait();
      }
    });
  for (auto& thread : pool_threads) thread.join();
  const auto s = pool.stats();
  if (broken) return fail("concurrent.shared_block");
  if (s.leased != 0 || s.failures != 0 || s.blocks > threads || s.acquisitions != threads * rounds)
    return fail("concurrent.counts");
  // Each round, seven of the eight acquisitions find another block leased.
  if (s.second_blocks < (threads - 1) * rounds) return fail("concurrent.second_blocks");
  tally.concurrent_leases += leases;
  tally.second_blocks += s.second_blocks;
  tally.cases += 3;
  return 0;
}

// A failed allocation leaves the pool as it was.
int exception_safety(Tally& tally) {
  TestMemory memory;
  Pool pool(memory);
  const gpu::LaneRecord* kept = nullptr;
  {
    auto a = pool.acquire(100);
    fill(a.data(), a.size(), 0x44);
    kept = a.data();
  }
  const auto before = pool.stats();
  memory.fail_next = true;
  bool thrown = false;
  try {
    auto b = pool.acquire(10000);  // growth of block 0 fails
  } catch (const std::bad_alloc&) {
    thrown = true;
  }
  auto s = pool.stats();
  if (!thrown || s.failures != 1 || s.leased != 0 || s.blocks != before.blocks || s.bytes != before.bytes ||
      s.allocations != before.allocations || memory.releases != 0)
    return fail("throw.growth");
  {
    auto c = pool.acquire(100);  // the former block, its memory and contents kept
    if (c.data() != kept || c.block() != 0 || !holds(c.data(), c.size(), 0x44) || memory.allocations != 1)
      return fail("throw.kept_block");
    memory.fail_next = true;
    thrown = false;
    try {
      auto d = pool.acquire(10);  // block 0 leased: the creation fails
    } catch (const std::bad_alloc&) {
      thrown = true;
    }
    s = pool.stats();
    if (!thrown || s.failures != 2 || s.leased != 1 || s.blocks != 1) return fail("throw.creation");
    auto e = pool.acquire(10);  // the empty entry is taken again, allocated now
    if (!e || e.block() == c.block() || pool.stats().blocks != 2 || pool.stats().leased != 2)
      return fail("throw.after_creation");
  }
  if (pool.stats().leased != 0) return fail("throw.released");
  // reserve() under a failing allocation: nothing leased, the exception out.
  memory.fail_next = true;
  thrown = false;
  try {
    pool.reserve(1U << 20);
  } catch (const std::bad_alloc&) {
    thrown = true;
  }
  if (!thrown || pool.stats().leased != 0 || pool.stats().failures != 3) return fail("throw.reserve");
  tally.failures += pool.stats().failures;
  tally.cases += 6;
  return 0;
}

// The host twin's pinned path: records moved into a lease, byte for byte.
int twin_lease(Tally& tally) {
  TestMemory memory;
  Pool pool(memory);
  for (const std::size_t n : {std::size_t{0}, std::size_t{1}, std::size_t{1000}, std::size_t{4097}}) {
    gpu::LanesOutput out;
    out.records.resize(n);
    std::uint64_t state = 0x243f6a8885a308d3ULL ^ n;
    auto* bytes = reinterpret_cast<unsigned char*>(out.records.data());
    for (std::size_t i = 0; i < n * record_bytes; ++i) {
      state = state * 6364136223846793005ULL + 1442695040888963407ULL;
      bytes[i] = static_cast<unsigned char>(state >> 56);
    }
    const std::vector<unsigned char> expected(bytes, bytes + n * record_bytes);
    gpu::lease_records(out, pool);
    if (!out.records.empty() || out.record_total() != n || out.pinned_records != true ||
        static_cast<bool>(out.pinned) != (n != 0))
      return fail("twin.shape n=" + std::to_string(n));
    if (n != 0 && std::memcmp(out.record_data(), expected.data(), expected.size()) != 0)
      return fail("twin.bytes n=" + std::to_string(n));
    tally.twin_records += n;
  }
  if (pool.stats().leased != 0) return fail("twin.released");
  tally.cases += 4;
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  std::size_t rounds = 64;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (arg.starts_with("--rounds=")) {
      const auto digits = arg.substr(9);
      const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), rounds);
      if (digits.empty() || error != std::errc{} || end != digits.data() + digits.size()) return 2;
    } else {
      return 2;
    }
  }
  if (rounds < 1 || rounds > 100000) {
    std::fprintf(stderr, "usage: mhgp9_gpu_record_pool_gate [--rounds=64]\n");
    return 2;
  }
  Tally tally;
  if (const int code = grow_on_demand(tally); code != 0) return code;
  if (const int code = exclusivity(tally); code != 0) return code;
  if (const int code = concurrent(rounds, tally); code != 0) return code;
  if (const int code = exception_safety(tally); code != 0) return code;
  if (const int code = twin_lease(tally); code != 0) return code;
  std::printf("record_pool_gate cases=%llu allocations=%llu growths=%llu second_blocks=%llu failures=%llu "
              "concurrent_leases=%llu twin_records=%llu\n",
              tally.cases, tally.allocations, tally.growths, tally.second_blocks, tally.failures,
              tally.concurrent_leases, tally.twin_records);
  if (tally.cases != 31 || tally.allocations == 0 || tally.growths == 0 || tally.second_blocks == 0 ||
      tally.failures != 3 || tally.concurrent_leases != 8 * rounds || tally.twin_records != 5098)
    return 3;
  return 0;
}
