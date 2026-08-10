// MorseHGP3D v3 — L'API du kernel face-owner device.
//
// Discipline du front d'onde : l'hote appelle un symbole qu'il ne definit
// pas ; seule l'unite CUDA (`faceowner_device_kernel.cu`, compilee sous
// MHGP3V_ENABLE_CUDA) le definit. Le harnais de qualification compile sans
// CUDA et refuse alors proprement — il ne PRETEND jamais un chemin device.
//
// Le contrat est un FLUX D'ARETES : pour un ordre k, le device doit rendre
// exactement les branches d'etoile dedupliquees, triees par (lot d'activation
// du membre, owner, membre), que le chemin CPU `collect_edges` produit — la
// qualification les compare ARETE PAR ARETE, puis les temps par phase et le
// manifeste memoire sont publies. Le rejeu DSU reste sur l'hote.
#pragma once

#include <vector>

namespace mhgp3v {
namespace device {

struct FaceOwnerDeviceEdge {
  int activation_batch = 0;
  int owner = 0;
  int member = 0;
};

struct FaceOwnerDeviceResult {
  std::vector<FaceOwnerDeviceEdge> edges;   // triees, dedupliquees
  long long incidences = 0;                 // emises sur device
  long long signatures = 0;                 // groupes de clefs distinctes
  long long device_bytes = 0;               // allocation device totale
  double emit_milliseconds = 0.0;
  double sort_milliseconds = 0.0;
  double reduce_milliseconds = 0.0;
};

// Entrees : membres CSR en identifiants DENSES (< 2^21), offsets par
// generateur, offsets d'incidences (prefixes exclusifs des binomiales
// C(rang, k), dernier element = total), rangs, rang d'activation canonique et
// lot par generateur. Rend faux avec un message sur toute erreur CUDA.
bool run_faceowner_order_on_gpu(const std::vector<int>& members_csr,
                                const std::vector<long long>& member_offsets,
                                const std::vector<long long>& incidence_offsets,
                                const std::vector<int>& ranks,
                                const std::vector<int>& activation_rank,
                                const std::vector<int>& batch_of, int k,
                                FaceOwnerDeviceResult* result, char* error,
                                int error_capacity);

// Interroge la VRAM (libre, totale) — defini par l'unite CUDA seulement.
bool query_device_memory(long long* free_bytes, long long* total_bytes, char* error,
                         int error_capacity);

}  // namespace device
}  // namespace mhgp3v
