// MorseHGP3D v3 — L'ENTREE AUTORITAIRE DU FOLD HYBRIDE (audit S1, porte 2).
//
// PERIMETRE : ORACLE BORNE n <= 32 a jamais (audit delta cbac109) — la
// fermeture exige `smax >= n` et l'interface borne `smax <= 32`. Ce chemin
// n'est ni « public » ni un candidat 50 k ; c'est la voie AUTORITAIRE DU
// JUGE BORNE. Elle ne recoit jamais un `Catalogue` brut accompagne d'un
// booleen ou d'un recu constructible : elle recoit `const
// ValidatedHybridSidecar&`. Les drapeaux principaux viennent des CERTIFICATS
// de la factory (le moteur ne recalcule pas les miniboules de certificat),
// les donnees viennent du snapshot possede par le sidecar, et toute
// pretention de famille complete passe par la fermeture certifiee du recu de
// producteur — sinon refus atomique avant le fold. L'API brute de
// `build_saturated_fold_hybrid` reste le HARNAIS relatif des portes
// differentielles.
#pragma once

#include "prototype/saturated_fold_hybrid.hpp"
#include "prototype/validated_hybrid_sidecar.hpp"

namespace mhgp3v {

inline SaturatedFold build_saturated_fold_hybrid_validated(
    const ValidatedHybridSidecar& sidecar, int maximum_order, bool keep_partitions,
    HybridReceipt* receipt, bool enforce_event_guard, bool prefix_fallback,
    HybridPairLedger* ledger = nullptr, bool factorise_exaequo = false,
    bool fast_exaequo = false) {
  SaturatedFold refused;
  refused.maximum_order = maximum_order;
  if (!sidecar.ok()) {
    refused.refusal = "sidecar invalide : aucun chemin autoritaire";
    return refused;
  }
  if (!sidecar.closure_certified_all_orders()) {
    refused.refusal = "fermeture des carriers non certifiee par le recu de source — la"
                      " pretention de famille complete n'est pas recue";
    return refused;
  }
  const std::vector<char> principal = sidecar.principal_flags();
  return build_saturated_fold_hybrid(
      sidecar.points(), (int)sidecar.points().size(), sidecar.catalogue(), maximum_order,
      keep_partitions, receipt, enforce_event_guard, {}, prefix_fallback, ledger,
      /*prefix_all=*/false, factorise_exaequo, fast_exaequo, &principal);
}

}  // namespace mhgp3v
