// HostGroup regression gate for the S4b q4 lane: two distinct shallow
// positive roots in the SAME J8 bucket, with lower ID in the first group.
// Build against v9's current src/gpu headers; no product file is modified.
#include "q4_lanes.hpp"

#include <array>
#include <cstdint>
#include <cstdio>

int main() {
  using namespace mhgp9::gpu;
  std::array<std::array<std::int32_t, 3>, 5> points{{
      {{0, 0, 0}}, {{200, 0, 0}}, {{100, 120, 0}},
      {{100, 0, 110}}, {{100, 0, 111}},
  }};
  std::array<u32, 5> ranks{{0, 1, 2, 3, 4}};
  std::array<u32, 5> ids{{0, 1, 2, 3, 4}};
  std::array<LaneRecord, 8> records{};
  std::array<u32, 16> positions{}, bits{}, list{};

  LanesIndex index{};
  index.rank_ids = ids.data();
  LanesSlab slab{};
  slab.points = &points[0][0];
  slab.ranks = ranks.data();
  slab.records = records.data();
  slab.record_capacity = static_cast<u32>(records.size());
  const Q4Slab q4{positions.data(), bits.data(), list.data(),
                  static_cast<u32>(positions.size())};

  Q4Work work{};
  u32 count = 0;
  const auto status = q4_seed(HostGroup{}, index, points[0].data(),
                              points[1].data(), 0, 1, 2, slab,
                              static_cast<u32>(points.size()), 5, q4,
                              count, work);
  std::printf("status=%u groups=%llu emitted=%llu records=%u\n",
              static_cast<unsigned>(status),
              static_cast<unsigned long long>(work.groups),
              static_cast<unsigned long long>(work.emitted), count);
  for (u32 i = 0; i < count; ++i) {
    const auto& r = records[i];
    std::printf("record%u support=%u,%u,%u,%u depth=%u shell=%u\n",
                i, r.support[0], r.support[1], r.support[2],
                r.support[3], r.depth, r.shell);
  }
  if (status != CertificateStatus::decided || count != 2 ||
      work.groups != 2 || work.emitted != 2) return 1;
  const auto& first = records[0];
  const auto& second = records[1];
  const std::array<u32, 4> expected_first{{0, 1, 2, 3}};
  const std::array<u32, 4> expected_second{{0, 1, 2, 4}};
  for (int j = 0; j < 4; ++j)
    if (first.support[j] != expected_first[j] ||
        second.support[j] != expected_second[j]) return 1;
  if (first.depth != 0 || second.depth != 1 ||
      first.shell != 4 || second.shell != 4) return 1;
  bool distinct_keys = false;
  for (int j = 0; j < 5; ++j)
    distinct_keys = distinct_keys || first.key[j] != second.key[j];
  return distinct_keys ? 0 : 1;
}
