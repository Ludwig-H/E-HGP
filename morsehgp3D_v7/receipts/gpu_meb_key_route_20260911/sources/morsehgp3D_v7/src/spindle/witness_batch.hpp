// Synchronous witness batches. The request wire is host-only; CUDA backends
// serialize their own explicit device wire and publish no partial result.
#pragma once

#include <span>
#include <stdexcept>
#include <vector>

#include "witness_count.hpp"

namespace mhgp7 {

struct WitnessBatchRequest {
  NodeRef a = 0, b = 0;
  u8 mask = 0;
  bool corners = false;
  u64 request_id = 0;
};

enum class WitnessBatchStatus : u8 {
  kUnwritten = 0, kOk, kInvalidRequest, kStackOverflow, kCounterOverflow
};

struct WitnessBatchResult {
  FusedCounts counts;
  u64 request_id = ~u64{0};
  u64 generation = ~u64{0};
  WitnessBatchStatus status = WitnessBatchStatus::kUnwritten;
};

// This is an association/shape check, NOT a geometric certificate. Counts
// still require a qualified backend. Call before consuming ANY batch row.
inline void validate_witness_batch(std::span<const WitnessBatchRequest> requests,
                                   const u64 h[3], u64 generation,
                                   std::span<const WitnessBatchResult> results) {
  if (requests.size() != results.size()) throw std::runtime_error("witness_batch_size");
  for (size_t i = 0; i < requests.size(); ++i) {
    const auto& request = requests[i];
    const auto& result = results[i];
    if ((request.mask & ~u8{7}) != 0 || result.status != WitnessBatchStatus::kOk ||
        result.request_id != request.request_id || result.generation != generation)
      throw std::runtime_error("witness_batch_association");
    for (unsigned q = 0; q < 3; ++q)
      if (result.counts.c[q] > h[q] ||
          (!(request.mask & (u8{1} << q)) && result.counts.c[q] != 0))
        throw std::runtime_error("witness_batch_count");
  }
}

}  // namespace mhgp7
