// MorseHGP3D v3 — LE PRODUCTEUR TERMINAL SCELLE (audit S1, delta cbac109).
//
// Le recu de source n'est constructible QUE par ce producteur : il enumere
// lui-meme (`flat_catalogue`), et scelle le recu sur SES sorties, au seul
// endroit ou « enumeration achevee sans censure de rang » est un FAIT du
// producteur — jamais une assertion d'appelant. Le jeton ET le recu sont non
// trivialement copiables, a constructeur prive : la forge fraiche — y
// compris par `std::bit_cast` — est refusee a la COMPILATION ; le reemploi
// d'un recu sur une autre table est refuse par les digests SHA-256.
//
// PERIMETRE A JAMAIS : ORACLE BORNE n <= 32, jamais un chemin public ni un
// candidat 50 k. La fermeture exige `smax >= n` alors que l'interface borne
// `smax <= kMaxRank = 32` ; le pipeline qui le consomme enumere le catalogue
// DEUX fois (une pour la table, une ici) et refuse le producteur parallele.
// Le futur producteur streame aura son propre domaine complet, ses identites
// count/fill et un recu independant de `smax >= n`.
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
    if (out.ok) {
      // L'identite du producteur (contrat, profil, schema de taches, statut
      // terminal, version de code digeree) accompagne les digests de donnees
      // — la factory ne certifie que ce contrat, au statut kOk.
      Sha256Stream code;
      const char* version = "mhgp3v_sealed_source_flat_catalogue_v1";
      code.update(version, std::char_traits<char>::length(version));
      out.receipt.push_back(HybridSourceReceipt(
          SourceProducerToken{}, sidecar_points_digest(points),
          sidecar_catalogue_digest(out.catalogue), code.finalize(),
          kSidecarProducerFlatCatalogueSealed, kSidecarProfileQuantizedU16,
          kSidecarTaskSchemaSequentialV1, /*terminal_status=*/0, smax,
          (int)points.size(), /*enumeration_completed=*/true));
    }
    return out;
  }
};

}  // namespace mhgp3v
