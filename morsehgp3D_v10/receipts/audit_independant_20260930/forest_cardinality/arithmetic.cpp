// Counter-check of source expressions; no allocation or engine execution.
#include <cstdint>
#include <iostream>

int main() {
  using u32 = std::uint32_t;
  using u64 = std::uint64_t;
  u64 births, joins, nodes, edges;
  while (std::cin >> births >> joins >> nodes >> edges) {
    const u32 nb = static_cast<u32>(births);
    const u32 nj = static_cast<u32>(joins);
    // Expressions present in kruskal() on 408d1ffe4.
    const u32 reserve = nb + nj;
    const u32 node = static_cast<u32>(nodes);
    const u32 end = static_cast<u32>(edges);
    std::cout << reserve << ' ' << node << ' ' << end << '\n';
  }
}
