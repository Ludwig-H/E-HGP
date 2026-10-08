// Sorties de la fin d'etage en flux (tranche T2-d, transferts et publication du catalogue), ecrites une fois pour
// l'executeur CUDA et l'executeur Pool :
//   - flux de sortie : un tableau de l'executeur est remis a l'hote par tranches consecutives ; chaque tranche est
//     consommee aussitot (copie parallele vers le Buffer final, ou niveaux materialises en num::Level) ; sur
//     l'appareil, les tranches passent par la memoire epinglee de transit (demande du flux : stream_staging_bytes,
//     jamais plus que la copie d'un seul tenant d'avant), kStagingSlots cases en vol : la copie de la tranche c + 1
//     recouvre la consommation de la tranche c (double tampon) ; aucun tampon hote intermediaire (les niveaux etaient
//     copies dans un tableau de mots, puis materialises a la publication) ;
//   - sorties anticipees : les Buffer de sortie sont reserves des que leur taille exacte est connue et leurs pages
//     touchees sur le Pool (prefault) pendant que l'appareil calcule ; le contenu d'un Buffer neuf est indetermine et
//     chaque sortie est ensuite ecrite en entier, donc aucune valeur publiee n'en depend. Les sorties vivent donc plus
//     tot (des le dernier lot) : le pic du budget change, et un petit budget peut refuser a un autre endroit ;
//     toujours un refus entier (memory_budget) ou le catalogue entier, jamais un prefixe.
// Les constantes des leviers se coupent par substitution exacte (bras d'ablation du pilote G4), jamais par une option.
#pragma once

#include <cstring>
#include <span>

#include "catalogue/finish_level.hpp"
#include "catalogue/internal.hpp"
#include "catalogue/transfer_meter.hpp"
#include "sched/sched.hpp"

namespace mhgp12::catalogue_detail::fin {

// false : la fin d'etage reserve chaque sortie a sa prise, sans premier toucher (retire les deux, comme la base)
inline constexpr bool kAnticipateOutputs = true;  // sorties hote reservees et touchees avant la fin d'etage

// Sorties hote : boules canoniques, CSR des populations, niveaux des rangs (case 0 : niveau nul, puis rangs 1..L-1),
// table S* -> boule. Reservees par l'appelant (sorties anticipees) ou par la fin d'etage.
struct FinishOutput {
  Buffer<CatalogueBall> balls;
  Buffer<u64> offsets;
  Buffer<SiteIdx> values;
  Buffer<num::Level> levels;
  Buffer<u64> table_offsets;
  Buffer<BallIdx> table_values;
};

// Copie parallele de `bytes` octets (blocs de 256 Kio repartis sur le Pool).
struct ByteCopy {
  u8* to;
  const u8* from;
  u64 bytes;
  static constexpr u64 kBlock = u64{1} << 18;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    const auto& c = *static_cast<const ByteCopy*>(context);
    for (u64 b = begin; b < end; ++b) {
      const u64 at = b * kBlock, n = c.bytes - at < kBlock ? c.bytes - at : kBlock;
      std::memcpy(c.to + at, c.from + at, n);
    }
    return {};
  }
};
inline Outcome parallel_copy(void* to, const void* from, u64 bytes, sched::Pool& pool) noexcept {
  if (bytes == 0) return {};
  ByteCopy c{static_cast<u8*>(to), static_cast<const u8*>(from), bytes};
  const u64 blocks = (bytes + ByteCopy::kBlock - 1) / ByteCopy::kBlock;
  if (blocks == 1 || pool.size() == 1) return ByteCopy::body(&c, 0, blocks, 0);
  return pool.parallel_for(blocks, 1, &c, &ByteCopy::body);
}

// Consommateur de copie : tranche [first, first + count) vers le Buffer de sortie.
struct CopyChunk {
  u8* to;
  u64 elem;
  sched::Pool* pool;
  static Outcome consume(void* context, const void* chunk, u64 first, u64 count) noexcept {
    const auto& c = *static_cast<const CopyChunk*>(context);
    return parallel_copy(c.to + first * c.elem, chunk, count * c.elem, *c.pool);
  }
};

// Niveaux exacts des rangs depuis leurs mots : num::Level::make, memes numerateur et denominateur non reduits que
// num::Sphere::through (finish_level.hpp).
struct Materialize {
  const LevelWords* words;
  num::Level* levels;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    const auto& m = *static_cast<const Materialize*>(context);
    for (u64 r = begin; r < end; ++r) {
      num::Wide<kNumWords> n{};
      num::Wide<kDenWords> d{};
      for (int i = 0; i < kNumWords; ++i) n.words[i] = m.words[r].n[i];
      for (int i = 0; i < kDenWords; ++i) d.words[i] = m.words[r].d[i];
      const auto level = num::Level::make(n, d);
      if (!level.ok()) return fail(Reason::catalogue_invariant);
      m.levels[r] = level.value();
    }
    return {};
  }
};

// Consommateur des niveaux : la tranche [first, first + count) des rangs 1..L-1 materialisee aux cases first + 1...
struct LevelChunk {
  num::Level* to;  // case du rang 1
  sched::Pool* pool;
  static Outcome consume(void* context, const void* chunk, u64 first, u64 count) noexcept {
    const auto& c = *static_cast<const LevelChunk*>(context);
    Materialize m{static_cast<const LevelWords*>(chunk), c.to + first};
    return c.pool->parallel_for(count, 4096, &m, &Materialize::body);
  }
};

// Premier toucher des pages de [data, data + bytes) reparti sur le Pool (outputs.cpp) : MADV_POPULATE_WRITE sur les
// pages entieres (contenu inchange) ; si le noyau le refuse, un octet ecrit par page, toujours dans l'intervalle (le
// contenu d'un Buffer neuf est indetermine). Aucune valeur publiee n'en depend.
[[nodiscard]] Outcome prefault(void* data, u64 bytes, sched::Pool& pool) noexcept;
// Ecriture d'un octet par page touchee par [data, data + bytes), jamais hors de l'intervalle (repli de prefault).
void touch_pages(u8* data, u64 bytes) noexcept;

// Sorties anticipees de la voie appareil (kAnticipateOutputs), a leur taille exacte : boules, decalages, populations et
// table des que les comptes du dernier lot sont connus, niveaux apres le balayage des debuts de rang ; reservation
// puis premier toucher sur le Pool pendant que l'appareil calcule. outputs_ns compte la duree murale de l'hote pour les
// deux (meme quand un noyau avance pendant ce temps : les etapes nettes ne sont alors pas la duree propre des noyaux).
// Sans anticipation, la fin d'etage reserve les sorties a leur prise, comme avant la tranche T2-d.
template <class T>
Outcome prepare_one(Buffer<T>& out, u64 n, MemoryBudget& budget, sched::Pool& pool, TransferMeter& meter) noexcept {
  const Stopwatch watch;
  MHGP12_TRY(out.allocate(n, budget));
  MHGP12_TRY(prefault(out.data(), n * sizeof(T), pool));
  meter.outputs_ns += watch.nanoseconds();
  meter.outputs_bytes += n * sizeof(T);
  return {};
}

inline Outcome prepare_outputs(FinishOutput& out, u64 balls, u64 incidences, u32 sites, MemoryBudget& budget,
                               sched::Pool& pool, TransferMeter& meter) noexcept {
  if (balls >= kNone) return fail(Reason::index_overflow_u32);
  MHGP12_TRY(prepare_one(out.balls, balls, budget, pool, meter));
  MHGP12_TRY(prepare_one(out.offsets, balls + 1, budget, pool, meter));
  MHGP12_TRY(prepare_one(out.values, incidences, budget, pool, meter));
  MHGP12_TRY(prepare_one(out.table_offsets, u64{sites} + 1, budget, pool, meter));
  return prepare_one(out.table_values, balls, budget, pool, meter);
}

}  // namespace mhgp12::catalogue_detail::fin
