// MorseHGP3D v3 — interface hote du noyau device de la source par ancre.
// Le noyau lui-meme est dans `anchor_source_kernel.cu` et n'implemente aucune
// geometrie : il appelle `run_anchor_point` de `anchor_pipeline.hpp`.
#pragma once

#include <vector>

#include "prototype/anchor_pipeline.hpp"

namespace mhgp3v {
namespace anchor {

struct DeviceRun {
  WorkCounters counters{};
  std::vector<EmittedSupport> supports;
  unsigned flags = 0;
  double kernel_ms = 0.0;
  unsigned long long emitted = 0;
  unsigned long long scratch_bytes = 0;
  unsigned long long output_bytes = 0;
  int slots = 0;
};

// Le LBVH est passe a plat, deja construit et deja trie par l'hote : le noyau
// ne construit rien et ne trie rien globalement.
DeviceRun run_device(const std::vector<int>& lo, const std::vector<int>& hi,
                     const std::vector<int>& begin_, const std::vector<int>& end_,
                     const std::vector<int>& left, const std::vector<int>& right,
                     const std::vector<int>& order, const std::vector<int>& px,
                     const std::vector<int>& py, const std::vector<int>& pz, int smax,
                     bool store, unsigned long long output_capacity, int slots);

}  // namespace anchor
}  // namespace mhgp3v
