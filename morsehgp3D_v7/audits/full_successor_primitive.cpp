// Independent transport: the implementation under test is the captured header.
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

#include "src/forest/full_gabriel.hpp"

#ifdef MHGP7_TESTING
#error "This audit must exercise the product successor path."
#endif

int main() {
  using mhgp7::FullNodeId;
  using mhgp7::u64;
  namespace detail = mhgp7::full_gabriel_detail;
  size_t cases = 0;
  if (!(std::cin >> cases) || cases > 20000) return 2;
  for (size_t row = 0; row < cases; ++row) {
    size_t case_id = 0, count = 0, calls = 0;
    u64 steps = 0, normalized = 0;
    if (!(std::cin >> case_id >> count >> calls >> steps >> normalized) ||
        count > 64 || calls > 64 || normalized > steps) return 2;
    std::vector<FullNodeId> successors(count);
    for (size_t i = 0; i < count; ++i) {
      if (!(std::cin >> successors[i]) || successors[i] < i ||
          successors[i] >= count) return 2;
    }
    for (size_t call = 0; call < calls; ++call) {
      FullNodeId token = 0, root = 0;
      u64 cap = 0;
      if (!(std::cin >> token >> cap >> root)) return 2;
      const auto status = detail::normalize_successor(
          successors, token, root, steps, normalized, cap);
      const char* name = "invalid";
      switch (status) {
        case detail::SuccessorStatus::kOk: name = "ok"; break;
        case detail::SuccessorStatus::kUnknownAnchor: name = "unknown"; break;
        case detail::SuccessorStatus::kBudget: name = "budget"; break;
      }
      std::cout << "{\"case\":" << case_id << ",\"call\":" << call
                << ",\"status\":\"" << name << "\",\"root\":" << root
                << ",\"steps\":" << steps << ",\"normalized\":" << normalized
                << ",\"accounting\":\"" << mhgp7::kFullGabrielSuccessorAccounting
                << "\",\"successors\":[";
      for (size_t i = 0; i < successors.size(); ++i) {
        if (i != 0) std::cout << ',';
        std::cout << successors[i];
      }
      std::cout << "]}\n";
    }
  }
  std::cin >> std::ws;
  return std::cin.eof() && std::cout.good() ? 0 : 2;
}
