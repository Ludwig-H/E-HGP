// Acces interne unique aux noeuds immuables ; aucun nouvel acces public aux plages de l'index.
#pragma once
#include "index/index.hpp"

namespace mhgp12::index_detail {
struct Access {
  static std::span<const Node> nodes(const GlobalIndex& index) noexcept { return index.nodes_.span(); }
};
}  // namespace mhgp12::index_detail
