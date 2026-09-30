#include "cloud/cloud.hpp"

#include <algorithm>
#include <numeric>
#include <vector>

namespace mhgp10 {

namespace {
u64 spread21(u64 v) {
  v &= 0x1FFFFFull;
  v = (v | (v << 32)) & 0x1F00000000FFFFull;
  v = (v | (v << 16)) & 0x1F0000FF0000FFull;
  v = (v | (v << 8)) & 0x100F00F00F00F00Full;
  v = (v | (v << 4)) & 0x10C30C30C30C30C3ull;
  v = (v | (v << 2)) & 0x1249249249249249ull;
  return v;
}
}  // namespace

u64 morton3(u32 x, u32 y, u32 z) { return spread21(x) | (spread21(y) << 1) | (spread21(z) << 2); }

Result<Cloud> prepare_cloud(std::span<const u32> x, std::span<const u32> y, std::span<const u32> z,
                            std::span<const u32> point_ids, int bits, MemoryBudget& budget) {
  const u64 n = x.size();
  if (n == 0) return fail(Reason::empty_input);
  if (y.size() != n || z.size() != n || point_ids.size() != n) return fail(Reason::size_mismatch);
  if (bits < 1 || bits > kMaxCoordinateBits) return fail(Reason::parameter_out_of_range);
  if (n >= kNone) return fail(Reason::index_overflow_u32);
  const u32 limit = (bits == 32) ? ~u32{0} : ((u32{1} << bits) - 1);
  for (u64 i = 0; i < n; ++i)
    if (x[i] > limit || y[i] > limit || z[i] > limit) return fail(Reason::coordinate_out_of_domain);
  {
    std::vector<u32> sorted(point_ids.begin(), point_ids.end());
    std::sort(sorted.begin(), sorted.end());
    if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end()) return fail(Reason::duplicate_point_id);
  }
  // Tri par (Morton, coordonnees, PointId) : ordre total canonique.
  std::vector<u64> key(n);
  for (u64 i = 0; i < n; ++i) key[i] = morton3(x[i], y[i], z[i]);
  std::vector<u32> order(n);
  std::iota(order.begin(), order.end(), 0u);
  std::sort(order.begin(), order.end(), [&](u32 a, u32 b) {
    if (key[a] != key[b]) return key[a] < key[b];
    return point_ids[a] < point_ids[b];
  });
  u64 sites = 0;
  for (u64 i = 0; i < n; ++i)
    if (i == 0 || key[order[i]] != key[order[i - 1]]) ++sites;
  Cloud c;
  c.bits = bits;
  if (!c.x.allocate(sites, budget) || !c.y.allocate(sites, budget) || !c.z.allocate(sites, budget) ||
      !c.w.allocate(sites, budget) || !c.ids.off.allocate(sites + 1, budget) || !c.ids.val.allocate(n, budget))
    return fail(Reason::memory_budget);
  u64 s = 0;
  c.ids.off[0] = 0;
  for (u64 i = 0; i < n; ++i) {
    const u32 p = order[i];
    if (i > 0 && key[p] == key[order[i - 1]]) {
      ++c.w[s - 1];
    } else {
      c.x[s] = x[p];
      c.y[s] = y[p];
      c.z[s] = z[p];
      c.w[s] = 1;
      ++s;
    }
    c.ids.val[i] = static_cast<PointId>(point_ids[p]);
    c.ids.off[s] = static_cast<u32>(i + 1);
  }
  c.weight = n;
  return c;
}

}  // namespace mhgp10
