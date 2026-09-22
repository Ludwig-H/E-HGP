#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

#include "core/float32_ball_key.hpp"
#include "lanes/float32_q3_owned.hpp"
#include "wspd/float32_front.hpp"

namespace mhgp8 {

struct Float32Q3GlobalOptions {
  std::size_t kmax{10};
  std::uint32_t separation{8};
  Float32Q3OwnedMode mode{Float32Q3OwnedMode::SharedPrefix};
  bool filter{true};
  std::size_t relay_sites{8};
};

// SUM except peak_key_bytes (MAX) and the explicitly documented nested MAXs.
// edge_filter is the fixed-edge witness geometry, NOT the owned census work.
struct Float32Q3GlobalWork {
  std::uint64_t calls{}, rectangles{}, edges{}, edge_filter_calls{};
  std::uint64_t witness_node_visits{}, witness_inside_nodes{}, witness_inside_sites{};
  std::uint64_t witness_outside_nodes{}, witness_splits{}, rejected_edges{};
  std::uint64_t emitted{}, shell_ids{}, peak_key_bytes{};
  Float32FrontWork front{};
  Float32EdgeWork edge_filter{};
  Float32Q3OwnedWork owned{};
  Float32KeyWork keys{};
  bool operator==(const Float32Q3GlobalWork&) const = default;
};

struct Float32Q3GlobalEmission {
  std::array<std::size_t, 3> support; // Ascending original IDs.
  std::array<std::size_t, 2> owner;   // Ascending original IDs, minlex tie.
  std::size_t depth;
  const Float32BallKey& key;
  std::span<const std::size_t> shell; // All boundary IDs; unsorted, borrowed.
};
using Float32Q3GlobalConsumer = std::function<void(const Float32Q3GlobalEmission&)>;

// Complete positive q3 SUPPORT stream at strict depth <Kmax-1. Ball identity
// can repeat across supports: this is neither a deduplicated catalogue nor
// the whole HGP tower. q4 is NOT inferred from this stream. No interior IDs.
//
// Shared immutable index, one reusable owned-census workspace for the entire
// call; no pair table, per-edge index/cloud, or per-edge thread team. Front
// and fixed-edge certificates only reject saturated products/edges; partial
// counts are NEVER passed to the census. Every accepted support gets its key.
//
// Null index/invalid options/empty callback throw before changing work.
// A nonempty cloud with <3 sites or K1 is a valid empty stream. The CLI's
// empty-input fast path is separate (the historical index rejects n=0).
// Callback views last until return. Callback exceptions preserve partial
// work/emissions; no rollback. Simultaneous calls need separate work/context.
void validate_float32_q3_global_options(const Float32Q3GlobalOptions& options);
void run_float32_q3_global(Float32IndexPtr index, const Float32Q3GlobalOptions& options,
                           const Float32Q3GlobalConsumer& emit, Float32Q3GlobalWork& work);

} // namespace mhgp8
