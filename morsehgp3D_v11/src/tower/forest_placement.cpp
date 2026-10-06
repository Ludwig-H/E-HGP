// Placement des taches du pipeline (forest_placement.hpp) : topologie lue dans /sys, plan pur, affinite rendue.
#include "tower/forest_placement.hpp"

#include <cstdio>

namespace mhgp11::tower_detail {

bool parse_cpu_list(std::string_view text, std::array<bool, kPlacementCpus>& out) noexcept {
  out.fill(false);
  while (!text.empty() && (text.back() == '\n' || text.back() == ' ')) text.remove_suffix(1);
  if (text.empty()) return false;
  u32 i = 0;
  while (i < text.size()) {
    u32 a = 0, b = 0, digits = 0;
    while (i < text.size() && text[i] >= '0' && text[i] <= '9' && digits < 5) a = a * 10 + u32(text[i++] - '0'), ++digits;
    if (digits == 0 || a >= kPlacementCpus) return false;
    b = a;
    if (i < text.size() && text[i] == '-') {
      ++i;
      b = 0; digits = 0;
      while (i < text.size() && text[i] >= '0' && text[i] <= '9' && digits < 5) b = b * 10 + u32(text[i++] - '0'), ++digits;
      if (digits == 0 || b >= kPlacementCpus || b < a) return false;
    }
    for (u32 c = a; c <= b; ++c) out[c] = true;
    if (i < text.size()) {
      if (text[i] != ',') return false;
      ++i;
      if (i == text.size()) return false;
    }
  }
  return true;
}

bool cores_from_lists(const std::array<std::string_view, kPlacementCpus>& lists, CpuCores& out) noexcept {
  out.count = 0;
  std::array<bool, kPlacementCpus> siblings{}, seen{};
  for (u32 c = 0; c < kPlacementCpus; ++c) {
    if (lists[c].empty() || seen[c]) continue;
    if (!parse_cpu_list(lists[c], siblings) || !siblings[c]) return false;
    u16 first = kNoThread, second = kNoThread;
    for (u32 s = 0; s < kPlacementCpus; ++s) {
      if (!siblings[s] || lists[s].empty()) continue;  // frere non autorise au processus
      if (seen[s]) return false;  // un fil dans deux coeurs : listes contradictoires
      seen[s] = true;
      if (first == kNoThread) first = static_cast<u16>(s);
      else if (second == kNoThread) second = static_cast<u16>(s);
    }
    out.first[out.count] = first;
    out.second[out.count] = second;
    ++out.count;
  }
  return out.count != 0;
}

bool read_cpu_cores(CpuCores& out) noexcept {
  out.count = 0;
#if defined(__linux__)
  cpu_set_t allowed;
  CPU_ZERO(&allowed);
  if (sched_getaffinity(0, sizeof(allowed), &allowed) != 0) return false;
  // Textes sur la pile du fil appelant (48 Kio, un appel par passe du pipeline) : aucun etat global ni TLS.
  std::array<std::array<char, 48>, kPlacementCpus> text{};
  std::array<std::string_view, kPlacementCpus> lists{};
  for (u32 c = 0; c < kPlacementCpus; ++c) {
    if (!CPU_ISSET(c, &allowed)) continue;
    char path[96];
    std::snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%u/topology/thread_siblings_list", c);
    std::FILE* file = std::fopen(path, "r");
    if (file == nullptr) return false;
    const bool read = std::fgets(text[c].data(), static_cast<int>(text[c].size()), file) != nullptr;
    std::fclose(file);
    if (!read) return false;
    lists[c] = std::string_view(text[c].data());
  }
  return cores_from_lists(lists, out);
#else
  return false;
#endif
}

#if defined(__linux__)
namespace {
void add_core(CpuSet& set, u16 first, u16 second) noexcept {
  if (first != kNoThread) CPU_SET(first, &set);
  if (second != kNoThread) CPU_SET(second, &set);
}
}  // namespace
#endif

PipelinePlacement plan_pipeline(const CpuCores& cores, u32 lanes, u32 kmax) noexcept {
  PipelinePlacement plan;
#if defined(__linux__)
  // Taches lourdes : publications des ordres K et K-1, balayages des ordres hauts K et K-1 (ceux qui existent).
  const u32 heavy = kmax >= 3 ? 4 : kmax == 2 ? 3 : 0;
  if (kmax < 2 || kmax > 5 || lanes == 0 || cores.count < heavy + 4) return plan;
  for (u32 i = 0; i < heavy; ++i)
    if (cores.second[cores.count - 1 - i] == kNoThread) return plan;  // un coeur lourd sans frere SMT : pas de plan
  CPU_ZERO(&plan.light);
  CPU_ZERO(&plan.resolvers);
  for (u32 i = 0; i < heavy; ++i) {
    const u32 core = cores.count - 1 - i;  // les derniers coeurs
    CPU_ZERO(&plan.heavy[i]);
    CPU_SET(cores.first[core], &plan.heavy[i]);
    CPU_SET(cores.second[core], &plan.light);
  }
  for (u32 core = 0; core + heavy < cores.count; ++core) add_core(plan.resolvers, cores.first[core], cores.second[core]);
  plan.cores = cores.count;
  plan.lanes = lanes;
  plan.kmax = kmax;
  plan.heavy_count = heavy;
#else
  (void)cores; (void)lanes; (void)kmax;
#endif
  return plan;
}

const CpuSet* PipelinePlacement::set_of(u32 task) const noexcept {
  if (cores == 0) return nullptr;
  if (task < lanes) return &resolvers;
  if (task < lanes + kmax) {  // publication de l'ordre k = task - lanes + 1
    const u32 k = task - lanes + 1;
    if (k == kmax) return &heavy[0];
    if (k + 1 == kmax) return &heavy[1];
    return &light;
  }
  const u32 upper = task - lanes - kmax + 2;  // balayage de l'ordre haut 2..K
  if (upper > kmax) return nullptr;
  if (upper == kmax) return &heavy[2];
  if (upper + 1 == kmax && heavy_count == 4) return &heavy[3];
  return &light;
}

ScopedAffinity::ScopedAffinity(const CpuSet* set) noexcept {
#if defined(__linux__)
  if (set == nullptr) return;
  if (sched_getaffinity(0, sizeof(saved_), &saved_) != 0) return;
  active_ = sched_setaffinity(0, sizeof(cpu_set_t), set) == 0;
#else
  (void)set;
#endif
}

ScopedAffinity::~ScopedAffinity() {
#if defined(__linux__)
  if (active_) sched_setaffinity(0, sizeof(saved_), &saved_);
#endif
}

}  // namespace mhgp11::tower_detail
