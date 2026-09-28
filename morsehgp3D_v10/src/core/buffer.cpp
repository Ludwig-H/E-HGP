#include "core/buffer.hpp"

namespace mhgp10 {

MemoryBudget& default_budget() {
  static MemoryBudget budget;
  return budget;
}

}  // namespace mhgp10
