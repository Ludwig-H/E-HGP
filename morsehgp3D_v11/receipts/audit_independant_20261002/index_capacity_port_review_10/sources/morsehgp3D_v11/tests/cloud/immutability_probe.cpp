// Refus d'API : construction directe, copie, affectation et vue mutable du proprietaire certifie.
// Chaque forme negative est compilee apres le temoin positif, sans executer de pointeur nul.
#include <type_traits>
#include <utility>

#include "cloud/cloud.hpp"

int main() {
#if defined(MHGP11_NEGATIVE) && MHGP11_NEGATIVE == 1
  mhgp11::Cloud forged;
  return static_cast<int>(forged.sites());
#elif defined(MHGP11_NEGATIVE) && MHGP11_NEGATIVE == 2
  static_assert(std::is_same_v<decltype(std::declval<mhgp11::Cloud&>().x()), std::span<mhgp11::u32>>,
                "mhgp11_cloud_mutable_view_interdite");
  return 0;
#elif defined(MHGP11_NEGATIVE) && MHGP11_NEGATIVE == 3
  const mhgp11::Cloud* cloud = nullptr;
  mhgp11::Cloud copy(*cloud);
  return static_cast<int>(copy.sites());
#elif defined(MHGP11_NEGATIVE) && MHGP11_NEGATIVE == 4
  mhgp11::Cloud* cloud = nullptr;
  *cloud = std::move(*cloud);
  return 0;
#else
  return std::is_nothrow_move_constructible_v<mhgp11::Cloud> ? 0 : 1;
#endif
}
