// Premier toucher des sorties hote du catalogue (tranche T2-d, finish_outputs.hpp) : les pages entieres d'un Buffer
// neuf sont peuplees par MADV_POPULATE_WRITE (Linux 5.14 ; contenu inchange), par blocs de 1 Mio repartis sur le
// Pool, pendant que l'appareil calcule ; si le noyau refuse, un octet est ecrit par page du bloc. Les deux pages de
// bord (debut et fin du Buffer, partagees avec d'autres blocs du tas) ne sont jamais ecrites ici : elles prendront
// leur faute a la premiere ecriture de la sortie. Aucune valeur publiee n'en depend.
#include <sys/mman.h>

#include <cstdint>

#include "catalogue/finish_outputs.hpp"

namespace mhgp12::catalogue_detail::fin {
namespace {

constexpr u64 kPage = 4096;
constexpr u64 kPrefaultBlock = u64{1} << 20;
#if defined(MADV_POPULATE_WRITE)
constexpr int kPopulateWrite = MADV_POPULATE_WRITE;
#else
constexpr int kPopulateWrite = 23;  // valeur de Linux 5.14 ; un noyau plus ancien rend EINVAL et le repli joue
#endif

// Pages entieres [first, first + bytes) (first aligne sur une page, bytes multiple d'une page).
struct Populate {
  u8* first;
  u64 bytes;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    const auto& p = *static_cast<const Populate*>(context);
    for (u64 b = begin; b < end; ++b) {
      const u64 at = b * kPrefaultBlock, n = p.bytes - at < kPrefaultBlock ? p.bytes - at : kPrefaultBlock;
      if (madvise(static_cast<void*>(p.first + at), n, kPopulateWrite) != 0) touch_pages(p.first + at, n);
    }
    return {};
  }
};

}  // namespace

void touch_pages(u8* data, u64 bytes) noexcept {
  if (bytes == 0) return;
  const std::uintptr_t begin = reinterpret_cast<std::uintptr_t>(data), end = begin + bytes;
  for (std::uintptr_t page = begin & ~std::uintptr_t{kPage - 1}; page < end; page += kPage) {
    const std::uintptr_t at = page < begin ? begin : page;  // premier octet de la page dans l'intervalle
    *reinterpret_cast<volatile u8*>(at) = 0;
  }
}

Outcome prefault(void* data, u64 bytes, sched::Pool& pool) noexcept {
  const std::uintptr_t begin = reinterpret_cast<std::uintptr_t>(data), end = begin + bytes;
  const std::uintptr_t mask = ~std::uintptr_t{kPage - 1};
  const std::uintptr_t first = (begin + kPage - 1) & mask, last = end & mask;
  if (data == nullptr || last <= first) return {};
  Populate p{reinterpret_cast<u8*>(first), static_cast<u64>(last - first)};
  const u64 blocks = (p.bytes + kPrefaultBlock - 1) / kPrefaultBlock;
  if (blocks == 1 || pool.size() == 1) return Populate::body(&p, 0, blocks, 0);
  return pool.parallel_for(blocks, 1, &p, &Populate::body);
}

}  // namespace mhgp12::catalogue_detail::fin
