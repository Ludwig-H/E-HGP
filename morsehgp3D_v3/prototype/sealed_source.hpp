// MorseHGP3D v3 — LE PRODUCTEUR TERMINAL SCELLE (audit S1).
//
// Le recu de source n'est constructible QUE par ce producteur : il enumere
// lui-meme (`flat_catalogue`), et scelle le recu sur SES sorties, au seul
// endroit ou « enumeration achevee sans censure de rang » est un FAIT du
// producteur — jamais une assertion d'appelant. Forger un recu frais sur une
// table amputee est refuse a la compilation (le jeton est prive) ; le
// reemploi d'un recu sur une autre table est refuse par les digests.
//
// v0 sequentiel : le producteur parallele et la source par cellules auront
// leurs propres scelles quand leurs completudes seront recevables.
#pragma once

#include <vector>

#include "prototype/order_k_flats.hpp"
#include "prototype/validated_hybrid_sidecar.hpp"

namespace mhgp3v {

class SealedSourceProducer {
 public:
  struct Result {
    mhgp::Catalogue catalogue;
    CloudStatus status = CloudStatus::kOk;
    bool ok = false;
    std::vector<HybridSourceReceipt> receipt;   // zero ou un
  };

  static Result run(const std::vector<mhgp::P3>& points, int smax) {
    Result out;
    FlatStatistics st{};
    out.catalogue = flat_catalogue(points, smax, &st, &out.status, false, true);
    out.ok = out.status == CloudStatus::kOk;
    if (out.ok)
      out.receipt.push_back(HybridSourceReceipt(
          SourceProducerToken{}, sidecar_points_digest(points),
          sidecar_catalogue_digest(out.catalogue), smax, (int)points.size(),
          /*enumeration_completed=*/true));
    return out;
  }
};

}  // namespace mhgp3v
