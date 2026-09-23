#include "certificate.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

using mhgp9::gpu::CellEntry;
using mhgp9::gpu::CertificateSlab;
using mhgp9::gpu::DeadWork;
using mhgp9::gpu::HostGroup;
using mhgp9::gpu::Prover;
using mhgp9::gpu::ProverCell;
using mhgp9::gpu::u32;

struct Trace {
  unsigned phase = 0;
  unsigned epoch = 0;
  unsigned barriers = 0;
  unsigned writes[3]{};
  unsigned point_reads = 0;
  unsigned waw = 0;
  unsigned war = 0;
  int last_write_phase = -1;
  int last_read_phase = -1;
  unsigned last_write_epoch = 0;
  unsigned last_read_epoch = 0;

  void sync() {
    ++epoch;
    ++barriers;
  }
  void write(u32 mask) {
    if (mask == 0) return;
    if (last_write_phase >= 0 && last_write_phase != static_cast<int>(phase) && last_write_epoch == epoch)
      ++waw;
    if (last_read_phase >= 0 && last_read_phase != static_cast<int>(phase) && last_read_epoch == epoch)
      ++war;
    last_write_phase = static_cast<int>(phase);
    last_write_epoch = epoch;
    writes[phase] += mhgp9::gpu::popcount32(mask);
  }
  void point_read(u32 base, u32 count) {
    // The only 17-entry ballot in this fixture is C1's point scan of next[].
    if (phase != 1 || base != 0 || count != 17) return;
    ++point_reads;
    last_read_phase = static_cast<int>(phase);
    last_read_epoch = epoch;
  }
};

struct TracedGroup {
  Trace* trace;
  HostGroup host;

  template <class Code>
  void ballot2(u32 base, u32 count, Code code, u32& first, u32& second) const {
    trace->point_read(base, count);
    host.ballot2(base, count, code, first, second);
  }
  template <class F>
  void for_set(u32 mask, F f) const {
    trace->write(mask);
    host.for_set(mask, f);
  }
  template <class F>
  void for_each(u32 count, F f) const { host.for_each(count, f); }
  bool leader() const { return host.leader(); }
  void sync() const { trace->sync(); }
};

void require(bool condition, std::string_view what) {
  if (!condition) throw std::runtime_error(std::string(what));
}

struct Observation {
  unsigned barriers;
  unsigned waw;
  unsigned war;
};

Observation run_fixture() {
  constexpr u32 capacity = 64;
  constexpr u32 current_level = 4;  // depth 6 writes level 4; prior frontier is level 3.
  std::array<u32, 2 * capacity> ranges{};
  std::array<mhgp9::gpu::i64, capacity> fc{}, fx{}, fy{};
  std::array<u32, mhgp9::gpu::prover_levels * capacity> frontiers{};
  for (u32 i = 0; i < capacity; ++i) {
    frontiers[(current_level - 1) * capacity + i] = i;
    if (i < 16) { fc[i] = -1; fx[i] = 2; }
    else if (i < 32) { fc[i] = 3; fx[i] = -2; }
    else if (i == 32) { fc[i] = -4; fx[i] = 2; }
    else { fc[i] = 1; fx[i] = 0; }
  }
  const CertificateSlab slab{ranges.data(), fc.data(), fx.data(), fy.data(), frontiers.data(), capacity};
  Prover prover{};
  prover.diameter_squared = 1;
  prover.threshold3 = 1;
  prover.n = capacity;
  DeadWork work{};
  Trace trace{};
  const TracedGroup group{&trace, {}};
  constexpr auto depth = static_cast<mhgp9::gpu::u8>(mhgp9::gpu::prover_max_depth);
  const u32* next = frontiers.data() + current_level * capacity;

  trace.phase = 0;
  const CellEntry c0 = mhgp9::gpu::enter_cell(group, prover, slab, ProverCell{0, 1, 0, 1},
                                               depth, current_level, capacity, 0, 2, work);
  require(c0.done && c0.value == 2, "C0 must return early with q3 still open");
  require(trace.writes[0] == 16, "C0 must write 16 partial sites before its early return");
  for (u32 i = 0; i < 16; ++i) require(next[i] == i, "C0 frontier content");

  trace.phase = 1;
  const CellEntry c1 = mhgp9::gpu::enter_cell(group, prover, slab, ProverCell{1, 2, 0, 1},
                                               depth, current_level, capacity, 0, 2, work);
  require(c1.done && c1.value == 0, "C1 must read its frontier then finish at max depth");
  require(trace.writes[1] == 17, "C1 must replace 16 sites and append id 32");
  require(trace.point_reads == 1, "C1 must read the 17-entry frontier after the scan sync");
  for (u32 i = 0; i < 17; ++i) require(next[i] == i + 16, "C1 frontier content");

  trace.phase = 2;
  const CellEntry c2 = mhgp9::gpu::enter_cell(group, prover, slab, ProverCell{0, 1, 1, 2},
                                               depth, current_level, capacity, 0, 2, work);
  require(c2.done && c2.value == 2, "C2 must reuse the frontier after C1's read");
  require(trace.writes[2] == 16, "C2 must overwrite 16 sites");
  for (u32 i = 0; i < 16; ++i) require(next[i] == i, "C2 frontier content");
  return {trace.barriers, trace.waw, trace.war};
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || (std::string_view(argv[1]) != "unsafe" && std::string_view(argv[1]) != "safe")) {
    std::cerr << "usage: gate unsafe|safe\n";
    return 2;
  }
  try {
    const Observation result = run_fixture();
    std::cout << "schema=s3_frontier_barrier_gate_v1\n"
              << "barriers=" << result.barriers << '\n'
              << "waw_unordered=" << result.waw << '\n'
              << "war_unordered=" << result.war << '\n';
    const bool safe = result.waw == 0 && result.war == 0;
    std::cout << "observed=" << (safe ? "safe" : "unsafe") << '\n';
    if (std::string_view(argv[1]) != (safe ? "safe" : "unsafe")) return 1;
    if (!safe && (result.waw == 0 || result.war == 0)) return 1;
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "fixture_failure=" << error.what() << '\n';
    return 1;
  }
}
