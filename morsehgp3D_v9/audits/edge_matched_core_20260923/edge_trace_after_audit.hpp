#pragma once

// Audit-only header copied into a temporary v9 source tree by prepare_source.py.
// One record per successful diametral-core proof; never part of the product.
// The fourth word packs the lane masks: pre | (post << 8).
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

namespace mhgp9::gen::edge_trace_audit {

static_assert(std::endian::native == std::endian::little);
struct Record {
  std::uint32_t raw_a;
  std::uint32_t raw_b;
  std::uint32_t core_sites;
  std::uint32_t masks_before_after_core;
};
static_assert(sizeof(Record) == 16);

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

inline void record_core(std::size_t a, std::size_t b, std::size_t core_sites,
                        std::uint8_t mask_before_core, std::uint8_t mask_after_core,
                        std::size_t cloud_sites) {
  // The engine twin uses the same binary without tracing. The runner refuses
  // a missing or incomplete trace for every batch case.
  if (!std::getenv("MHGP9_AUDIT_TRACE_DIR")) return;
  static const auto ids = read_raw_ids();
  if (ids.size() != cloud_sites || a >= ids.size() || b >= ids.size() || a == b ||
      core_sites > std::numeric_limits<std::uint32_t>::max() ||
      (mask_before_core & ~6U) != 0 || mask_before_core == 0 ||
      (mask_after_core & ~mask_before_core) != 0)
    throw std::runtime_error("audit trace invalid core record");
  auto raw_a = ids[a], raw_b = ids[b];
  if (raw_b < raw_a) { const auto tmp = raw_a; raw_a = raw_b; raw_b = tmp; }
  thread_local Writer writer;
  writer.append({raw_a, raw_b, static_cast<std::uint32_t>(core_sites),
                 static_cast<std::uint32_t>(mask_before_core) |
                     (static_cast<std::uint32_t>(mask_after_core) << 8)});
}

}  // namespace mhgp9::gen::edge_trace_audit
