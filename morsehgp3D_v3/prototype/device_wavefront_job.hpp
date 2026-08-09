// MorseHGP3D v3 — LE TRAVAIL D'UN FRONT D'ONDE, écrit une fois pour l'hôte et le device.
//
// Ce fichier décrit UNE unité de travail du parcours order-k sous une forme qui
// traverse la frontière hôte/device : des tableaux plats, des tailles fixes, aucune
// allocation, aucun conteneur. Le même code est compilé par `g++` pour l'hôte et par
// `nvcc` pour `sm_120` ; c'est ce qui rend la comparaison bit à bit possible, parce
// qu'il n'y a pas deux implémentations à faire coïncider mais une seule.
//
// ---------------------------------------------------------------------------
// CE QUE CE PREMIER KERNEL FAIT, ET CE QU'IL NE FAIT PAS
// ---------------------------------------------------------------------------
//
// Il évalue, pour chaque sommet admis, l'admissibilité des couples
// (flat, direction) — le prédicat qui domine le coût du parcours et qui porte
// toute son arithmétique exacte : `orient3d` en `i128`, le signe tangent, les deux
// potentiels. C'est le poste mesuré à 8,4 fermetures reconstruites par sommet.
//
// Il ne fait PAS la descente : `neighbour_along` n'est pas encore borné, et prétendre
// porter le parcours entier avant de l'avoir borné serait faux. Ce kernel prouve donc
// trois choses et pas une de plus — que l'arithmétique exacte du chemin compile pour
// `sm_120`, qu'elle rend bit à bit les mêmes verdicts que l'hôte, et à quel débit.
//
// ---------------------------------------------------------------------------
// LE REFUS TRAVERSE AUSSI LA FRONTIÈRE
// ---------------------------------------------------------------------------
//
// Un sommet peut porter plus de flats que le masque ne peut en contenir. Comme
// partout ailleurs dans ce noyau, le dépassement est un REFUS explicite et compté,
// jamais une troncature : l'hôte rejoue le sommet sur le chemin non borné.
#pragma once

#include "mhgp/mhgp.hpp"
#include "prototype/order_k_device_core.hpp"

namespace mhgp3v {
namespace device {

// Trente-deux flats par sommet, deux directions chacun : le verdict d'un sommet
// tient dans un mot de 64 bits. Le maximum observé sur les campagnes permanentes
// est très en deçà, et le différentiel publie le high-water.
inline constexpr int kMaxFlatsPerVertex = 32;

enum class VerdictStatus : int {
  kOk = 0,
  kFlatOverflow,      // plus de kMaxFlatsPerVertex flats : refus, rejeu hôte
  kClosureOverflow,   // une fermeture dépasse la capacité : refus, rejeu hôte
};

struct VertexVerdict {
  unsigned long long admissible = 0;   // bit 2*f + slot, slot 0 = direction -1
  int flat_count = 0;
  int status = (int)VerdictStatus::kOk;
};

struct WavefrontJob {
  const mhgp::P3* points = nullptr;
  int point_count = 0;
  const mhgp::i32* root_base = nullptr;
  int root_size = 0;
  const BoundedVertex* vertices = nullptr;
  int vertex_count = 0;
};

// L'évaluation d'UN sommet. C'est exactement le corps que le kernel exécute par
// thread, et que l'hôte exécute en boucle.
MHGP_HD inline void evaluate_vertex(const WavefrontJob& job, int index, VertexVerdict* out) {
  const BoundedVertex& v = job.vertices[index];
  VertexVerdict verdict;
  int flats = 0;
  bool overflow = false;
  const bool complete = for_each_flat_from(job.points, v, 0, 1, 2,
                                           [&](const BoundedFlat& flat, int, int, int) {
    if (flats >= kMaxFlatsPerVertex) { overflow = true; return false; }
    for (int slot = 0; slot < 2; ++slot) {
      const int direction = slot == 0 ? -1 : 1;
      if (pair_admissible(job.points, v, flat.base, direction, job.root_base, job.root_size))
        verdict.admissible |= (1ULL << (2 * flats + slot));
    }
    ++flats;
    return true;
  });
  verdict.flat_count = flats;
  if (!complete) verdict.status = (int)VerdictStatus::kClosureOverflow;
  else if (overflow) verdict.status = (int)VerdictStatus::kFlatOverflow;
  *out = verdict;
}

// Le reçu d'un lot, comparé terme à terme. Aucun agrégat : deux masques différents
// sur un seul sommet suffisent à refuser le lot.
struct WavefrontReceipt {
  long long vertices = 0;
  long long flats = 0;
  long long admissible_couples = 0;
  long long refused = 0;
  long long mismatches = 0;
  int flat_high_water = 0;
};

inline WavefrontReceipt summarise(const VertexVerdict* verdicts, int count) {
  WavefrontReceipt receipt;
  receipt.vertices = count;
  for (int i = 0; i < count; ++i) {
    if (verdicts[i].status != (int)VerdictStatus::kOk) { ++receipt.refused; continue; }
    receipt.flats += verdicts[i].flat_count;
    if (verdicts[i].flat_count > receipt.flat_high_water)
      receipt.flat_high_water = verdicts[i].flat_count;
    unsigned long long mask = verdicts[i].admissible;
    while (mask != 0) { ++receipt.admissible_couples; mask &= mask - 1; }
  }
  return receipt;
}

// Comparaison BIT À BIT de deux lots. C'est la seule chose que la session GPU doit
// établir : le device et l'hôte ne diffèrent sur aucun bit, ou la session échoue.
inline long long count_mismatches(const VertexVerdict* a, const VertexVerdict* b, int count) {
  long long bad = 0;
  for (int i = 0; i < count; ++i)
    if (a[i].admissible != b[i].admissible || a[i].flat_count != b[i].flat_count ||
        a[i].status != b[i].status)
      ++bad;
  return bad;
}

}  // namespace device
}  // namespace mhgp3v
