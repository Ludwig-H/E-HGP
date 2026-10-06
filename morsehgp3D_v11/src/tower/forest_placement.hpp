// Placement des taches du pipeline des ordres concurrents sur les coeurs physiques (levier O1 de la carte de l'etage
// tree, 6 octobre 2026). A W48, le publieur de l'ordre 5 consomme 81 a 151 ms de CPU en cohabitation avec les 39
// resolutions, contre 35 a 46 ms a un fil : la queue de l'etage (2 a 59 ms) vient de ce debit. Plan : chaque tache
// serie lourde (publications et balayages des deux ordres les plus hauts) recoit un fil materiel d'un coeur dont le
// frere SMT n'heberge que les taches legeres (publications et balayages bas, le plus souvent bloques) ; les
// resolutions se partagent les fils des autres coeurs. Indication d'ordonnancement seulement : aucune decision ni
// aucun resultat n'en depend, l'affinite du fil est rendue a la fin de chaque tache. Inactif (cores = 0) si la
// topologie est illisible, sans SMT, trop petite, hors Linux, ou si K > 5.
#pragma once

#include <array>
#include <string_view>

#include "core/core.hpp"

#if defined(__linux__)
#include <sched.h>
#endif

namespace mhgp11::tower_detail {

inline constexpr u32 kPlacementCpus = 1024;  // CPU_SETSIZE de la glibc
inline constexpr u16 kNoThread = 0xFFFF;

// Coeurs physiques autorises au processus : premier et second fil materiel (kNoThread sans second), par cle de coeur
// croissante (plus petit fil de la liste des freres).
struct CpuCores {
  u32 count = 0;
  std::array<u16, kPlacementCpus> first{}, second{};
};

// Liste de freres au format du noyau ("0,24", "0-1", "3,5-6") en masque de bits ; faux si mal formee ou hors borne.
[[nodiscard]] bool parse_cpu_list(std::string_view text, std::array<bool, kPlacementCpus>& out) noexcept;
// Coeurs depuis les listes de freres de chaque fil autorise (lists[c] vide : fil absent ou non autorise) ; faux si
// les listes se contredisent.
[[nodiscard]] bool cores_from_lists(const std::array<std::string_view, kPlacementCpus>& lists, CpuCores& out) noexcept;
// Coeurs du processus courant (affinite du processus et /sys/devices/system/cpu) ; faux hors Linux ou si illisible.
[[nodiscard]] bool read_cpu_cores(CpuCores& out) noexcept;

#if defined(__linux__)
using CpuSet = cpu_set_t;
#else
struct CpuSet {};
#endif

// Plan du pipeline : taches [0, lanes) de resolution, [lanes, lanes + K) publications des ordres 1..K,
// [lanes + K, lanes + 2K - 1) balayages des ordres hauts 2..K.
struct PipelinePlacement {
  u32 cores = 0;  // coeurs du plan ; 0 : aucun placement
  u32 lanes = 0, kmax = 0, heavy_count = 0;
  std::array<CpuSet, 4> heavy{};  // P_K, P_{K-1}, V_K, V_{K-1} (ceux qui existent)
  CpuSet light{}, resolvers{};
  // Ensemble de la tache, nullptr sans placement.
  const CpuSet* set_of(u32 task) const noexcept;
};
[[nodiscard]] PipelinePlacement plan_pipeline(const CpuCores& cores, u32 lanes, u32 kmax) noexcept;

// Affinite du fil courant fixee pour la duree de l'objet, puis rendue ; sans effet si set est nul ou si le noyau
// refuse (aucune decision n'en depend).
class ScopedAffinity {
 public:
  explicit ScopedAffinity(const CpuSet* set) noexcept;
  ~ScopedAffinity();
  ScopedAffinity(const ScopedAffinity&) = delete;
  ScopedAffinity& operator=(const ScopedAffinity&) = delete;

 private:
  CpuSet saved_{};
  bool active_ = false;
};

}  // namespace mhgp11::tower_detail
