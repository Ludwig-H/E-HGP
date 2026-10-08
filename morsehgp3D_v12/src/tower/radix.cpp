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

// Passe commune a plusieurs tableaux : l'unite u est le bloc u - first[j] du tableau j (first croissant).
struct RadixMany {
  std::array<RadixPass*, kMaxRadixJobs> passes{};
  std::array<u64, kMaxRadixJobs + 1> first{};
  u32 count = 0;
  Outcome (*phase)(void*, u64, u64, u32) = nullptr;
  void add(RadixPass& pass) noexcept {
    passes[count] = &pass;
    first[count + 1] = first[count] + pass.blocks;
    ++count;
  }
  u64 units() const noexcept { return first[count]; }
  static Outcome body(void* raw, u64 begin, u64 end, u32 worker) noexcept {
    auto& s = *static_cast<RadixMany*>(raw);
    u32 j = 0;
    for (u64 u = begin; u < end;) {
      while (s.first[j + 1] <= u) ++j;
      const u64 stop = std::min(end, s.first[j + 1]);
      MHGP12_TRY(s.phase(s.passes[j], u - s.first[j], stop - s.first[j], worker));
      u = stop;
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
  RadixJob job{data, scratch, low_bit, &places, {}};
  MHGP12_TRY(radix_sort_many(std::span<RadixJob>(&job, 1), budget, pool));
  return job.sorted;
}

// Passe p de chaque tableau actif : shift = low_bit + 8 p tant qu'il est sous 64 ; histogrammes de tous les tableaux
// de la passe dans une invocation, sommes prefixes par tableau (chiffre unique : ordre inchange, dispersion sautee),
// dispersions des autres dans une seconde invocation.
Outcome radix_sort_many(std::span<RadixJob> jobs, MemoryBudget& budget, sched::Pool& pool) noexcept {
  if (jobs.size() > kMaxRadixJobs) return fail(Reason::tower_invariant);
  std::array<RadixPass, kMaxRadixJobs> pass{};
  std::array<std::span<KeyedEntry>, kMaxRadixJobs> from{}, to{};
  std::array<bool, kMaxRadixJobs> active{};
  for (std::size_t j = 0; j < jobs.size(); ++j) {
    RadixJob& job = jobs[j];
    if (job.scratch.size() != job.data.size() || job.low_bit > 64 || job.places == nullptr)
      return fail(Reason::tower_invariant);
    job.sorted = job.data;
    active[j] = job.data.size() >= 2 && job.low_bit < 64;
    if (!active[j]) continue;
    pass[j].blocks = radix_blocks(job.data.size());
    pass[j].block_size = radix_block(job.data.size());
    if (job.places->size() < pass[j].blocks * kDigits)
      MHGP12_TRY(job.places->allocate(pass[j].blocks * kDigits, budget));
    pass[j].places = job.places->span().first(pass[j].blocks * kDigits);
    from[j] = job.data;
    to[j] = job.scratch;
  }
  for (u32 round = 0;; ++round) {
    RadixMany histograms, scatters;
    histograms.phase = &RadixPass::histogram;
    scatters.phase = &RadixPass::scatter;
    for (std::size_t j = 0; j < jobs.size(); ++j) {
      if (!active[j] || u64{jobs[j].low_bit} + u64{kDigitBits} * round >= 64) continue;
      pass[j].shift = jobs[j].low_bit + kDigitBits * round;
      pass[j].in = from[j];
      pass[j].out = to[j];
      histograms.add(pass[j]);
    }
    if (histograms.count == 0) break;
    MHGP12_TRY(pool.parallel_for(histograms.units(), 1, &histograms, &RadixMany::body));
    for (u32 i = 0; i < histograms.count; ++i) {
      RadixPass& p = *histograms.passes[i];
      if (!prefix_places(p.places, p.blocks, p.in.size())) scatters.add(p);  // chiffre unique : ordre inchange
    }
    if (scatters.count == 0) continue;
    MHGP12_TRY(pool.parallel_for(scatters.units(), 1, &scatters, &RadixMany::body));
    for (std::size_t j = 0; j < jobs.size(); ++j)
      for (u32 i = 0; i < scatters.count; ++i)
        if (scatters.passes[i] == &pass[j]) std::swap(from[j], to[j]);
  }
  for (std::size_t j = 0; j < jobs.size(); ++j)
    if (active[j]) jobs[j].sorted = from[j];  // inactif (moins de deux entrees, low_bit = 64) : data, inchange
  return {};
}

}  // namespace mhgp12::tower_detail
