// Refus d'API : construction directe, copie, affectation et vue mutable du proprietaire certifie.
// Chaque forme negative est compilee apres le temoin positif, sans executer de pointeur nul.
#include <type_traits>
#include <utility>

#include "cloud/cloud.hpp"

int main() {
#if defined(MHGP12_NEGATIVE) && MHGP12_NEGATIVE == 1
  mhgp12::Cloud forged;
  return static_cast<int>(forged.sites());
#elif defined(MHGP12_NEGATIVE) && MHGP12_NEGATIVE == 2
  static_assert(std::is_same_v<decltype(std::declval<mhgp12::Cloud&>().x()), std::span<mhgp12::u32>>,
                "mhgp12_cloud_mutable_view_interdite");
  return 0;
#elif defined(MHGP12_NEGATIVE) && MHGP12_NEGATIVE == 3
  const mhgp12::Cloud* cloud = nullptr;
  mhgp12::Cloud copy(*cloud);
  return static_cast<int>(copy.sites());
#elif defined(MHGP12_NEGATIVE) && MHGP12_NEGATIVE == 4
  mhgp12::Cloud* cloud = nullptr;
  *cloud = std::move(*cloud);
  return 0;
#else
  return std::is_nothrow_move_constructible_v<mhgp12::Cloud> ? 0 : 1;
#endif
}
