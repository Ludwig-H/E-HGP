// Transferts entre la memoire de l'appelant et celle d'un executeur du catalogue (CST-0235, raccord complet) :
// compteur de l'executeur (duree, octets dans chaque sens, operations) et chrono d'etape net de ces transferts, pour
// publier des durees d'etape disjointes et les transferts a part. Sur l'appareil, chaque copie est chronometree apres
// l'attente des noyaux en file (device_cuda.cu) ; sur l'hote, ce sont des copies de memoire (exec_host.hpp).
#pragma once

#include "core/core.hpp"

namespace mhgp12::catalogue_detail {

struct TransferMeter {
  u64 ns = 0, h2d_bytes = 0, d2h_bytes = 0, ops = 0;
  void note(const Stopwatch& watch, u64 h2d, u64 d2h) noexcept {
    ns += watch.nanoseconds();
    h2d_bytes += h2d;
    d2h_bytes += d2h;
    ++ops;
  }
};

// Deltas d'un compteur cumule (executeur resident) depuis `start`.
inline TransferMeter meter_since(const TransferMeter& now, const TransferMeter& start) noexcept {
  return TransferMeter{now.ns - start.ns, now.h2d_bytes - start.h2d_bytes, now.d2h_bytes - start.d2h_bytes,
                       now.ops - start.ops};
}

// Chrono d'une etape net des transferts de l'executeur faits pendant l'etape.
template <class B>
class NetWatch {
 public:
  explicit NetWatch(const B& b) noexcept : b_(b), start_(b.meter.ns) {}
  u64 nanoseconds() const noexcept {
    const u64 wall = watch_.nanoseconds(), moved = b_.meter.ns - start_;
    return wall > moved ? wall - moved : 0;
  }

 private:
  const B& b_;
  u64 start_;
  Stopwatch watch_;
};

}  // namespace mhgp12::catalogue_detail
