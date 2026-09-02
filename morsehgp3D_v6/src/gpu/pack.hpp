// MorseHGP3D v6 — ADAPTATEUR CANDIDATS -> ENCODEUR PUR (C6, jalon 2 de
// REPONSE_AUDITEUR_CONCEPTION_C6_20260902).
//
// `src/gpu/wire.hpp` porte l'encodeur PUR a offsets fixes (`pack_ball_in`,
// `pack_ball_range`) : il ne connait que `BallKey` et le seuil `h`, donc il
// reste compilable par nvcc sans trainer le pipeline hote. Cet en-tete y
// branche la SOURCE reelle du chemin produit — un tableau de `BallCandidate`
// et `smax` — sans copier une seule boule dans un tampon intermediaire.
//
// SEUIL : `h = smax + 1 - arite`, calcule EXACTEMENT comme la boucle
// `append_ball_in` du pilote (cli/mhgp6_cuda.cu, src/gpu/pilot.hpp), en u64
// et sans garde supplementaire — un profil ou arite > smax + 1 replie donc
// de la meme facon des deux cotes (la contrainte h_q = s_max - q + 1 est
// posee en amont, pas ici). Toute divergence de ce calcul casserait
// l'egalite `pack == append` que prouve tests/wire_pack_gate.cpp.
//
// Cet en-tete ne remplace RIEN dans le chemin produit : la bascule de
// `append_ball_in` vers `pack_*` est le palier C6a.
#pragma once

#include <cstddef>

#include "../pipeline/candidates.hpp"
#include "wire.hpp"

namespace mhgp6 {
namespace gpu {

// Seuil de profondeur d'une boule d'arite `arity` sous `smax`.
inline u64 wire_threshold(u64 smax, u8 arity) { return smax + 1 - (u64)arity; }

// Vue NON POSSEDANTE d'une plage de candidats (l'appelant garde le tableau
// et decide du decoupage ; la vue ne copie rien).
struct CandidateSpan {
  const BallCandidate* data = nullptr;
  size_t size = 0;
};

// Encode `span` aux offsets FIXES [base_index, base_index + span.size) du
// tampon `dst` de `dst_bytes` octets. Aucune allocation, aucun push_back,
// aucun etat partage : plusieurs fils peuvent appeler cette fonction sur des
// plages DISJOINTES du MEME tampon. Prevalidation des tailles puis
// validation de TOUTES les boules AVANT la premiere ecriture ; un refus
// laisse la plage intacte (jamais une ecriture partielle).
inline PackStatus pack_candidate_range(u8* dst, size_t dst_bytes, size_t base_index, CandidateSpan span,
                                       u64 smax) {
  if (span.size != 0 && span.data == nullptr) return PackStatus::kNullBuffer;
  const BallCandidate* cands = span.data;
  return pack_ball_range(dst, dst_bytes, base_index, span.size,
                         [cands, smax](size_t i, BallKey* k, u64* h) {
                           *k = cands[i].key;
                           *h = wire_threshold(smax, cands[i].arity);
                         });
}

}  // namespace gpu
}  // namespace mhgp6
