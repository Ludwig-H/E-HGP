// Tri par base stable et deterministe des entrees de l'etage G (index des naissances, premieres sondes en masse G-L5,
// CONTRAT_TOUR.md, paragraphe 4.3) : chiffres de 8 bits de key >> low_bit, du poids faible au poids fort (256 flux
// d'ecriture par dispersion : ils tiennent dans le cache de premier niveau, 11 bits le debordaient). Par passe :
// histogramme de chaque bloc d'entrees (taches du Pool), places par sommes prefixes dans l'ordre (chiffre, bloc),
// dispersion de chaque bloc dans l'ordre de ses entrees. Les places ne dependent que des donnees : memes octets a tout
// nombre de fils, et un tri stable (a chiffres egaux, ordre d'entree). Une passe dont toutes les entrees ont le meme
// chiffre est sautee (rien ne bouge). Ecrit pour l'appareil : histogrammes, sommes prefixes et dispersion sont les
// primitives d'un tri par base sur le GPU.
#include <algorithm>

#include "tower/internal.hpp"

namespace mhgp12::tower_detail {
namespace {

constexpr u32 kDigitBits = 8;
constexpr u32 kDigits = 1u << kDigitBits;
constexpr u64 kMinBlock = u64{1} << 12;  // entrees par bloc d'histogramme, au moins
constexpr u64 kMaxBlocks = 512;          // au plus : la somme prefixe (sequentielle) reste sous 2^17 cases

// Taille d'un bloc : fonction de n seul (le resultat d'un tri stable n'en depend pas, seul le travail en depend).
u64 radix_block(u64 entries) noexcept { return std::max(kMinBlock, (entries + kMaxBlocks - 1) / kMaxBlocks); }
u64 radix_blocks(u64 entries) noexcept { return (entries + radix_block(entries) - 1) / radix_block(entries); }

struct RadixPass {
  std::span<const KeyedEntry> in;
  std::span<KeyedEntry> out;
  std::span<u64> places;  // kDigits * blocs, rangees par chiffre puis par bloc
  u32 shift = 0;
  u64 blocks = 0, block_size = 0;

  u32 digit(const KeyedEntry& e) const noexcept {
    return shift >= 64 ? 0u : static_cast<u32>(e.key >> shift) & (kDigits - 1);
  }
  static Outcome histogram(void* raw, u64 begin, u64 end, u32) noexcept {
    auto& s = *static_cast<RadixPass*>(raw);
    std::array<u64, kDigits> local;
    for (u64 block = begin; block < end; ++block) {
      local.fill(0);
      const u64 lo = block * s.block_size, hi = std::min<u64>(s.in.size(), lo + s.block_size);
      for (u64 i = lo; i < hi; ++i) ++local[s.digit(s.in[i])];
      for (u32 d = 0; d < kDigits; ++d) s.places[u64{d} * s.blocks + block] = local[d];
    }
    return {};
  }
  static Outcome scatter(void* raw, u64 begin, u64 end, u32) noexcept {
    auto& s = *static_cast<RadixPass*>(raw);
    std::array<u64, kDigits> place;
    for (u64 block = begin; block < end; ++block) {
      for (u32 d = 0; d < kDigits; ++d) place[d] = s.places[u64{d} * s.blocks + block];
      const u64 lo = block * s.block_size, hi = std::min<u64>(s.in.size(), lo + s.block_size);
      for (u64 i = lo; i < hi; ++i) s.out[place[s.digit(s.in[i])]++] = s.in[i];
    }
    return {};
  }
};

// Sommes prefixes exclusives dans l'ordre (chiffre, bloc) ; vrai si un seul chiffre porte toutes les entrees.
bool prefix_places(std::span<u64> places, u64 blocks, u64 entries) noexcept {
  u64 sum = 0;
  bool single = false;
  for (u64 d = 0; d < kDigits; ++d) {
    const u64 first = sum;
    for (u64 b = 0; b < blocks; ++b) {
      const u64 count = places[d * blocks + b];
      places[d * blocks + b] = sum;
      sum += count;
    }
    single = single || sum - first == entries;
  }
  return single;
}

}  // namespace

u64 radix_bytes(u64 entries) noexcept { return radix_blocks(entries) * kDigits * sizeof(u64); }

Result<std::span<KeyedEntry>> radix_sort(std::span<KeyedEntry> data, std::span<KeyedEntry> scratch, u32 low_bit,
                                         Buffer<u64>& places, MemoryBudget& budget, sched::Pool& pool) noexcept {
  if (scratch.size() != data.size() || low_bit > 64) return fail(Reason::tower_invariant);
  if (data.size() < 2 || low_bit == 64) return data;
  RadixPass pass;
  pass.blocks = radix_blocks(data.size());
  pass.block_size = radix_block(data.size());
  if (places.size() < pass.blocks * kDigits) MHGP12_TRY(places.allocate(pass.blocks * kDigits, budget));
  pass.places = places.span().first(pass.blocks * kDigits);
  std::span<KeyedEntry> from = data, to = scratch;
  for (u32 shift = low_bit; shift < 64; shift += kDigitBits) {
    pass.shift = shift;
    pass.in = from;
    pass.out = to;
    MHGP12_TRY(pool.parallel_for(pass.blocks, 1, &pass, &RadixPass::histogram));
    if (prefix_places(pass.places, pass.blocks, data.size())) continue;  // chiffre unique : ordre inchange
    MHGP12_TRY(pool.parallel_for(pass.blocks, 1, &pass, &RadixPass::scatter));
    std::swap(from, to);
  }
  return from;
}

}  // namespace mhgp12::tower_detail
