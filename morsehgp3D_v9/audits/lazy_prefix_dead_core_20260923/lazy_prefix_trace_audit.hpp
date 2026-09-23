#pragma once

// AUDIT ONLY. One 32-byte record per completed diametral-core proof.
#include <atomic>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace mhgp9::gen::lazy_prefix_trace_audit {
static_assert(std::endian::native == std::endian::little);

struct Record {
  std::uint32_t raw_a, raw_b, core_sites, max_prefix;
  std::uint32_t masks_before_after, depth2_scanned, depth2_full, depth2_descended;
};
static_assert(sizeof(Record) == 32);

inline std::vector<std::uint32_t> read_raw_ids() {
  const char* path = std::getenv("MHGP9_AUDIT_RAW_IDS");
  if (!path || !*path) throw std::runtime_error("audit trace needs MHGP9_AUDIT_RAW_IDS");
  std::ifstream in(path, std::ios::binary | std::ios::ate);
  if (!in) throw std::runtime_error("audit trace cannot open raw ID map");
  const auto length = in.tellg();
  if (length < 0 || length % 4 != 0) throw std::runtime_error("audit trace invalid raw ID map length");
  const auto count = static_cast<std::uint64_t>(length) / 4;
  if (count > std::numeric_limits<std::uint32_t>::max())
    throw std::runtime_error("audit trace raw ID map too large");
  std::vector<std::uint32_t> ids(static_cast<std::size_t>(count));
  in.seekg(0);
  in.read(reinterpret_cast<char*>(ids.data()), length);
  if (!in) throw std::runtime_error("audit trace short raw ID map");
  return ids;
}

class Writer {
 public:
  Writer() {
    const char* directory = std::getenv("MHGP9_AUDIT_TRACE_DIR");
    if (!directory || !*directory) throw std::runtime_error("audit trace needs MHGP9_AUDIT_TRACE_DIR");
    static std::atomic<unsigned> next{0};
    const auto id = next.fetch_add(1, std::memory_order_relaxed);
    const std::string path = std::string(directory) + "/part_" + std::to_string(id) + ".bin";
    file_ = std::fopen(path.c_str(), "wb");
    if (!file_) throw std::runtime_error("audit trace cannot open output");
    if (std::setvbuf(file_, nullptr, _IOFBF, 1 << 20) != 0)
      throw std::runtime_error("audit trace cannot buffer output");
  }
  Writer(const Writer&) = delete;
  Writer& operator=(const Writer&) = delete;
  ~Writer() {
    if (file_ && std::fclose(file_) != 0) std::abort();
  }
  void append(Record record) {
    if (std::fwrite(&record, sizeof(record), 1, file_) != 1)
      throw std::runtime_error("audit trace write failed");
  }
 private:
  std::FILE* file_{};
};

inline void record(std::size_t a, std::size_t b, std::size_t core_sites,
                   std::size_t prefix, std::uint8_t before, std::uint8_t after,
                   std::uint32_t depth2_scanned, std::uint32_t depth2_full,
                   std::uint32_t depth2_descended, std::size_t cloud_sites) {
  if (!std::getenv("MHGP9_AUDIT_TRACE_DIR")) return;
  static const auto ids = read_raw_ids();
  if (ids.size() != cloud_sites || a >= ids.size() || b >= ids.size() || a == b ||
      core_sites > std::numeric_limits<std::uint32_t>::max() || prefix > core_sites ||
      (before & ~6U) != 0 || before == 0 || (after & ~before) != 0 ||
      depth2_full > depth2_scanned || depth2_descended > depth2_full)
    throw std::runtime_error("audit trace invalid lazy-prefix record");
  auto raw_a = ids[a], raw_b = ids[b];
  if (raw_b < raw_a) { const auto tmp = raw_a; raw_a = raw_b; raw_b = tmp; }
  thread_local Writer writer;
  writer.append({raw_a, raw_b, static_cast<std::uint32_t>(core_sites),
                 static_cast<std::uint32_t>(prefix),
                 static_cast<std::uint32_t>(before) | (static_cast<std::uint32_t>(after) << 8),
                 depth2_scanned, depth2_full, depth2_descended});
}
}  // namespace mhgp9::gen::lazy_prefix_trace_audit
