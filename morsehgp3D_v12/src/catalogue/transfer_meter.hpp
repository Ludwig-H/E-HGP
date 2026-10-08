// Transferts entre la memoire de l'appelant et celle d'un executeur du catalogue (CST-0235, raccord complet) :
// compteur de l'executeur (duree, octets dans chaque sens, operations) et chrono d'etape net de ces transferts, pour
// publier des durees d'etape disjointes et les transferts a part. Sur l'appareil, chaque copie est chronometree apres
// l'attente des noyaux en file (device_cuda.cu) ; sur l'hote, ce sont des copies de memoire (exec_host.hpp).
//
// Tranche T2-d (sorties en flux, finish_outputs.hpp) : deux postes de plus, eux aussi hors des etapes : preparation
// des sorties (reservation anticipee des Buffer de sortie et premier toucher de leurs pages sur le Pool, pendant que
// l'appareil calcule) et publication (niveaux materialises en num::Level au fil du flux). Un flux de sortie compte UNE
// operation par tableau, ses octets, et sa duree murale moins le temps de ses consommateurs de publication ; ses
// tranches sont comptees a part (chunks).
#pragma once

#include <span>

#include "core/core.hpp"

namespace mhgp12::catalogue_detail {

struct TransferMeter {
  u64 ns = 0, h2d_bytes = 0, d2h_bytes = 0, ops = 0;
  u64 outputs_ns = 0, outputs_bytes = 0, publish_ns = 0, chunks = 0;
  void note(const Stopwatch& watch, u64 h2d, u64 d2h) noexcept {
    ns += watch.nanoseconds();
    h2d_bytes += h2d;
    d2h_bytes += d2h;
    ++ops;
  }
  // Duree hors etape de toute sorte (transferts, preparation des sorties, publication) : ce que NetWatch retire.
  u64 off_stage() const noexcept { return ns + outputs_ns + publish_ns; }
};

// Flux de sortie (finish_outputs.hpp) : constantes des leviers (bras d'ablation du pilote par substitution exacte).
inline constexpr u64 kStagingSlots = 2;            // tranches en vol dans la memoire epinglee (double tampon)
inline constexpr u64 kMaxSlots = 8;
inline constexpr u64 kStreamChunk = u64{8} << 20;  // octets d'une tranche au plus
static_assert(kStagingSlots >= 1 && kStagingSlots <= kMaxSlots, "catalogue : 1 a 8 tranches en vol");

// Segment d'un flux : n elements de `elem` octets a `source` (memoire de l'executeur), remis par tranches a consume
// (context, tranche lisible sur l'hote, premier indice, nombre). Temps du consommateur : publication (niveaux) ou
// transfert (copies).
struct StreamSegment {
  const void* source;
  u64 elem, n;
  Outcome (*consume)(void* context, const void* chunk, u64 first, u64 count) noexcept;
  void* context;
  bool publication;
};

// Memoire de transit demandee par un flux de `slots` cases : min(slots * kStreamChunk, plus grand segment) octets.
// Avant la tranche T2-d, chaque sortie etait copiee d'un seul tenant par une memoire de transit de min(64 Mio, sortie)
// octets : la demande du flux n'est jamais plus grande que celle de son plus grand segment d'alors (l'executeur CUDA
// la porte au moins a 1 Mio et garde le maximum des demandes, device_cuda.cu).
inline u64 stream_staging_bytes(std::span<const StreamSegment> segments, u64 slots) noexcept {
  u64 largest = 0;
  for (const StreamSegment& s : segments) largest = s.n * s.elem > largest ? s.n * s.elem : largest;
  const u64 full = slots * kStreamChunk;
  return largest < full ? largest : full;
}

// Sequence des tranches d'un flux sur un executeur a transit X : X::slots() cases (au plus kMaxSlots) de
// X::slot_bytes() octets, X::slot(c) (adresse hote de la case), X::issue(c, source, octets) (copie asynchrone de la
// memoire de l'executeur vers la case), X::wait(c) (copie de la case terminee). La tranche t occupe la case t % slots ;
// la tranche t + slots n'est lancee qu'apres la consommation de la tranche t, qui libere sa case. Une tranche ne
// chevauche jamais deux segments. Rend la duree des consommateurs de publication ; chunks compte les tranches.
template <class X>
Result<u64> stream_staged(X& x, std::span<const StreamSegment> segments, u64& chunks) noexcept {
  struct Pending {
    u64 segment, first, count;
  };
  Pending ring[kMaxSlots] = {};
  const u64 slots = x.slots();
  if (slots < 1 || slots > kMaxSlots) return fail(Reason::catalogue_invariant);
  for (const StreamSegment& s : segments)
    if (s.n != 0 && (s.elem == 0 || s.elem > x.slot_bytes())) return fail(Reason::catalogue_invariant);
  u64 segment = 0, at = 0, issued = 0, consumed = 0, publication = 0;
  const auto next = [&](Pending& p) {
    while (segment < segments.size() && at >= segments[segment].n) {
      ++segment;
      at = 0;
    }
    if (segment == segments.size()) return false;
    const StreamSegment& s = segments[segment];
    const u64 per = x.slot_bytes() / s.elem;
    p = Pending{segment, at, s.n - at < per ? s.n - at : per};
    at += p.count;
    return true;
  };
  const auto launch = [&](const Pending& p) {
    const StreamSegment& s = segments[p.segment];
    ring[issued % slots] = p;
    return x.issue(issued % slots, static_cast<const u8*>(s.source) + p.first * s.elem, p.count * s.elem);
  };
  Pending p{};
  for (; issued < slots && next(p); ++issued) MHGP12_TRY(launch(p));
  for (; consumed < issued; ++consumed) {
    const u64 slot = consumed % slots;
    MHGP12_TRY(x.wait(slot));
    const Pending q = ring[slot];
    const StreamSegment& s = segments[q.segment];
    const Stopwatch watch;
    MHGP12_TRY(s.consume(s.context, x.slot(slot), q.first, q.count));
    if (s.publication) publication += watch.nanoseconds();
    ++chunks;
    if (next(p)) {
      MHGP12_TRY(launch(p));
      ++issued;
    }
  }
  return publication;
}

// Deltas d'un compteur cumule (executeur resident) depuis `start`.
inline TransferMeter meter_since(const TransferMeter& now, const TransferMeter& start) noexcept {
  TransferMeter d;
  d.ns = now.ns - start.ns;
  d.h2d_bytes = now.h2d_bytes - start.h2d_bytes;
  d.d2h_bytes = now.d2h_bytes - start.d2h_bytes;
  d.ops = now.ops - start.ops;
  d.outputs_ns = now.outputs_ns - start.outputs_ns;
  d.outputs_bytes = now.outputs_bytes - start.outputs_bytes;
  d.publish_ns = now.publish_ns - start.publish_ns;
  d.chunks = now.chunks - start.chunks;
  return d;
}

// Chrono d'une etape net des transferts, de la preparation des sorties et de la publication faits pendant l'etape.
template <class B>
class NetWatch {
 public:
  explicit NetWatch(const B& b) noexcept : b_(b), start_(b.meter.off_stage()) {}
  u64 nanoseconds() const noexcept {
    const u64 wall = watch_.nanoseconds(), moved = b_.meter.off_stage() - start_;
    return wall > moved ? wall - moved : 0;
  }

 private:
  const B& b_;
  u64 start_;
  Stopwatch watch_;
};

}  // namespace mhgp12::catalogue_detail
