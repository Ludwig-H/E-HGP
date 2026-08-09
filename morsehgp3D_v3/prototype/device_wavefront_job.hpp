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
// LE REFUS ÉTAIT COMPTÉ, IL N'ÉTAIT PAS JUGÉ — ET C'ÉTAIT UN P0
// ---------------------------------------------------------------------------
//
// L'ancienne boucle de référence exécutait `continue` sur chaque sommet refusé, et
// `summarise` comptait le refus puis écartait ses flats et son masque. Le plancher
// `--min-refused` prouvait donc que la BRANCHE avait été prise, jamais que son
// résultat avait été conservé. La conséquence est nette : un mutant qui refuse TOUS
// les sommets restait vert, avec zéro flat, zéro couple et zéro désaccord.
//
// Trois choses ferment cela, et il faut les trois.
//
// D'abord un oracle non borné est exécuté pour CHAQUE statut, refus compris, y
// compris pour les refus d'admission qui n'entraient même pas dans le lot.
// Ensuite le ledger `refuses = rejoues + en_attente + fatals` est publié par
// raison : une raison sans rejeu est un trou, pas une statistique. Enfin
// l'identité de masse est exigée — le CPU complet, sur TOUS les sommets, doit
// égaler l'union des résultats committés et rejoués, flat par flat et couple par
// couple.
//
// ---------------------------------------------------------------------------
// COMPTE ET MASQUE NE SONT PAS UNE SIGNATURE
// ---------------------------------------------------------------------------
//
// Un masque nul ne certifie pas l'ordre. Sur six points cosphériques, un sommet
// porte vingt flats et un masque nul : TOUTE permutation des vingt flats conserve
// `(flat_count, mask)`. Une régression de clef canonique passerait donc la porte.
// Chaque verdict porte maintenant un digest ORDONNÉ des flats livrés — base,
// taille de fermeture et bits d'admissibilité, repliés dans l'ordre de livraison.
#pragma once

#include "mhgp/mhgp.hpp"
#include "prototype/order_k_device_core.hpp"

namespace mhgp3v {
namespace device {

// Trente-deux flats par sommet, deux directions chacun : le verdict d'un sommet
// tient dans un mot de 64 bits. Le maximum observé sur les campagnes permanentes
// est très en deçà, et le différentiel publie le high-water.
inline constexpr int kMaxFlatsPerVertex = 32;

// Le lot ne peut pas être arbitrairement grand : les multiplications de tailles
// doivent rester exactes avant toute allocation. Ces deux bornes sont vérifiées
// par `validate_job`, jamais supposées.
inline constexpr int kMaxPointCount = 1 << 24;
inline constexpr int kMaxVertexCount = 1 << 22;
inline constexpr int kCoordinateCeiling = 65535;   // le profil u16 déclaré

enum class VerdictStatus : int {
  kOk = 0,
  kFlatOverflow,        // plus de kMaxFlatsPerVertex flats : refus, rejeu hôte
  // UNE FERMETURE NE PEUT PAS DÉPASSER LA CAPACITÉ APRÈS ADMISSION.
  //
  // Toute fermeture est un sous-ensemble de la coquille, et l'admission a déjà
  // borné la coquille par `kMaxShell = kMaxClosure`. Ce statut n'est donc pas un
  // repli normal : c'est une VIOLATION D'INVARIANT, et le juge la traite comme
  // fatale au lieu de la rejouer comme un refus ordinaire.
  kInvariantViolation,
};

struct VertexVerdict {
  unsigned long long admissible = 0;   // bit 2*f + slot, slot 0 = direction -1
  unsigned long long signature = 0;    // digest ORDONNÉ des flats livrés
  int flat_count = 0;
  int status = (int)VerdictStatus::kOk;
};
static_assert(sizeof(VertexVerdict) == 24, "le verdict doit rester sans remplissage");

// Le digest FNV-1a, écrit une fois, hôte et device.
MHGP_HD inline unsigned long long fold_digest(unsigned long long seed, long long value) {
  unsigned long long h = seed;
  const unsigned long long bits = (unsigned long long)value;
  for (int byte = 0; byte < 8; ++byte) {
    h ^= (bits >> (8 * byte)) & 0xFFULL;
    h *= 1099511628211ULL;
  }
  return h;
}

inline constexpr unsigned long long kDigestSeed = 1469598103934665603ULL;

MHGP_HD inline unsigned long long cloud_digest(const mhgp::P3* points, int count) {
  unsigned long long h = kDigestSeed;
  h = fold_digest(h, count);
  for (int i = 0; i < count; ++i) {
    h = fold_digest(h, points[i].x);
    h = fold_digest(h, points[i].y);
    h = fold_digest(h, points[i].z);
  }
  return h;
}

struct WavefrontJob {
  const mhgp::P3* points = nullptr;
  int point_count = 0;
  const mhgp::i32* root_base = nullptr;
  int root_size = 0;
  const BoundedVertex* vertices = nullptr;
  int vertex_count = 0;
  // L'EMPREINTE DU NUAGE VOYAGE AVEC LE LOT. Une taille et un pointeur ne
  // s'authentifient pas l'un l'autre ; l'appelant déclare ce qu'il envoie et
  // `validate_job` le recalcule.
  unsigned long long declared_cloud_digest = 0;
};

// ---------------------------------------------------------------------------
// L'AUTHENTIFICATION DU LOT
// ---------------------------------------------------------------------------
//
// Le job transportait des pointeurs et des tailles bruts. Un job
// `root_size = 0, root_base = nullptr` obtenait encore `kOk`, `point_count`
// n'était jamais lu, et une entrée malformée devenait un accès hors limites
// DEVICE au lieu d'un refus AVANT lancement. Chaque champ est maintenant
// vérifié, et la validation rend une raison nommée.
enum class JobVerdict : int {
  kValid = 0,
  kNullPoints,
  kPointCountOutOfRange,
  kCoordinateOffGrid,
  kCloudDigestMismatch,
  kNullRoot,
  kRootSizeNotFour,
  kRootIdOutOfRange,
  kRootNotIndependent,
  kNullVertices,
  kVertexCountOutOfRange,
  kShellSizeOutOfRange,
  kShellNotStrictlySorted,
  kShellIdOutOfRange,
  kInteriorSizeAboveContract,
  kInteriorNotStrictlySorted,
  kInteriorIdOutOfRange,
  kShellMeetsInterior,
  kLevelIsNotInteriorSize,
  kAllocationWouldOverflow,
};

MHGP_HD inline const char* job_verdict_name(JobVerdict v) {
  switch (v) {
    case JobVerdict::kValid: return "valid";
    case JobVerdict::kNullPoints: return "null_points";
    case JobVerdict::kPointCountOutOfRange: return "point_count_out_of_range";
    case JobVerdict::kCoordinateOffGrid: return "coordinate_off_u16_grid";
    case JobVerdict::kCloudDigestMismatch: return "cloud_digest_mismatch";
    case JobVerdict::kNullRoot: return "null_root";
    case JobVerdict::kRootSizeNotFour: return "root_size_not_four";
    case JobVerdict::kRootIdOutOfRange: return "root_id_out_of_range";
    case JobVerdict::kRootNotIndependent: return "root_not_independent";
    case JobVerdict::kNullVertices: return "null_vertices";
    case JobVerdict::kVertexCountOutOfRange: return "vertex_count_out_of_range";
    case JobVerdict::kShellSizeOutOfRange: return "shell_size_out_of_range";
    case JobVerdict::kShellNotStrictlySorted: return "shell_not_strictly_sorted";
    case JobVerdict::kShellIdOutOfRange: return "shell_id_out_of_range";
    case JobVerdict::kInteriorSizeAboveContract: return "interior_size_above_contract";
    case JobVerdict::kInteriorNotStrictlySorted: return "interior_not_strictly_sorted";
    case JobVerdict::kInteriorIdOutOfRange: return "interior_id_out_of_range";
    case JobVerdict::kShellMeetsInterior: return "shell_meets_interior";
    case JobVerdict::kLevelIsNotInteriorSize: return "level_is_not_interior_size";
    case JobVerdict::kAllocationWouldOverflow: return "allocation_would_overflow";
  }
  return "unknown";
}

MHGP_HD inline JobVerdict validate_job(const WavefrontJob& job) {
  if (job.points == nullptr) return JobVerdict::kNullPoints;
  if (job.point_count < 4 || job.point_count > kMaxPointCount)
    return JobVerdict::kPointCountOutOfRange;
  for (int i = 0; i < job.point_count; ++i) {
    const mhgp::P3& p = job.points[i];
    if (p.x < 0 || p.x > kCoordinateCeiling || p.y < 0 || p.y > kCoordinateCeiling ||
        p.z < 0 || p.z > kCoordinateCeiling)
      return JobVerdict::kCoordinateOffGrid;
  }
  if (cloud_digest(job.points, job.point_count) != job.declared_cloud_digest)
    return JobVerdict::kCloudDigestMismatch;

  if (job.root_base == nullptr) return JobVerdict::kNullRoot;
  if (job.root_size != 4) return JobVerdict::kRootSizeNotFour;
  for (int i = 0; i < 4; ++i)
    if (job.root_base[i] < 0 || job.root_base[i] >= job.point_count)
      return JobVerdict::kRootIdOutOfRange;
  if (orient3d_bounded(job.points[job.root_base[0]], job.points[job.root_base[1]],
                       job.points[job.root_base[2]], job.points[job.root_base[3]]) == 0)
    return JobVerdict::kRootNotIndependent;

  if (job.vertices == nullptr) return JobVerdict::kNullVertices;
  if (job.vertex_count < 1 || job.vertex_count > kMaxVertexCount)
    return JobVerdict::kVertexCountOutOfRange;
  // Les multiplications de tailles doivent rester exactes en 64 bits AVANT toute
  // allocation. Les deux plafonds ci-dessus les rendent vraies; on le vérifie
  // quand même, parce qu'un plafond modifié sans revérification est le scénario
  // exact d'un dépassement silencieux.
  {
    const unsigned long long vertex_bytes =
        (unsigned long long)sizeof(BoundedVertex) * (unsigned long long)job.vertex_count;
    const unsigned long long verdict_bytes =
        (unsigned long long)sizeof(VertexVerdict) * (unsigned long long)job.vertex_count;
    const unsigned long long point_bytes =
        (unsigned long long)sizeof(mhgp::P3) * (unsigned long long)job.point_count;
    if (vertex_bytes / (unsigned long long)sizeof(BoundedVertex) !=
            (unsigned long long)job.vertex_count ||
        verdict_bytes / (unsigned long long)sizeof(VertexVerdict) !=
            (unsigned long long)job.vertex_count ||
        point_bytes / (unsigned long long)sizeof(mhgp::P3) !=
            (unsigned long long)job.point_count)
      return JobVerdict::kAllocationWouldOverflow;
  }

  for (int i = 0; i < job.vertex_count; ++i) {
    const BoundedVertex& v = job.vertices[i];
    if (v.shell_size < 3 || v.shell_size > kMaxShell) return JobVerdict::kShellSizeOutOfRange;
    if (v.interior_size < 0 || v.interior_size > kMaxInterior)
      return JobVerdict::kInteriorSizeAboveContract;
    if (v.level != v.interior_size) return JobVerdict::kLevelIsNotInteriorSize;
    for (int t = 0; t < v.shell_size; ++t) {
      if (v.shell[t] < 0 || v.shell[t] >= job.point_count) return JobVerdict::kShellIdOutOfRange;
      if (t > 0 && v.shell[t] <= v.shell[t - 1]) return JobVerdict::kShellNotStrictlySorted;
    }
    for (int t = 0; t < v.interior_size; ++t) {
      if (v.interior[t] < 0 || v.interior[t] >= job.point_count)
        return JobVerdict::kInteriorIdOutOfRange;
      if (t > 0 && v.interior[t] <= v.interior[t - 1])
        return JobVerdict::kInteriorNotStrictlySorted;
      if (contains(v.shell, v.shell_size, v.interior[t])) return JobVerdict::kShellMeetsInterior;
    }
  }
  return JobVerdict::kValid;
}

// L'évaluation d'UN sommet. C'est exactement le corps que le kernel exécute par
// thread, et que l'hôte exécute en boucle.
MHGP_HD inline void evaluate_vertex(const WavefrontJob& job, int index, VertexVerdict* out) {
  const BoundedVertex& v = job.vertices[index];
  VertexVerdict verdict;
  verdict.signature = kDigestSeed;
  int flats = 0;
  bool overflow = false;
  const bool complete = for_each_flat_from(job.points, v, 0, 1, 2,
                                           [&](const BoundedFlat& flat, int, int, int) {
    if (flats >= kMaxFlatsPerVertex) { overflow = true; return false; }
    int slots = 0;
    for (int slot = 0; slot < 2; ++slot) {
      const int direction = slot == 0 ? -1 : 1;
      if (pair_admissible(job.points, v, flat.base, direction, job.root_base, job.root_size)) {
        verdict.admissible |= (1ULL << (2 * flats + slot));
        slots |= (1 << slot);
      }
    }
    // LA SIGNATURE EST ORDONNÉE : elle replie la base, la taille de fermeture et
    // les deux bits, à la position de livraison. Permuter deux flats la change.
    verdict.signature = fold_digest(verdict.signature, flats);
    verdict.signature = fold_digest(verdict.signature, flat.base[0]);
    verdict.signature = fold_digest(verdict.signature, flat.base[1]);
    verdict.signature = fold_digest(verdict.signature, flat.base[2]);
    verdict.signature = fold_digest(verdict.signature, flat.closure_size);
    verdict.signature = fold_digest(verdict.signature, slots);
    ++flats;
    return true;
  });
  verdict.flat_count = flats;
  if (!complete) verdict.status = (int)VerdictStatus::kInvariantViolation;
  else if (overflow) verdict.status = (int)VerdictStatus::kFlatOverflow;
  *out = verdict;
}

// ---------------------------------------------------------------------------
// LE LEDGER DU LOT
// ---------------------------------------------------------------------------
//
// `vertices = count` agrégeait refus compris et ne publiait jamais `accepted`;
// `flat_high_water` excluait les refus; `mismatches` n'était alimenté par
// personne. Chaque grandeur est maintenant nommée et séparée, et l'identité
// `refuses = rejoues + en_attente + fatals` est vérifiable ligne à ligne.
struct WavefrontReceipt {
  long long presented = 0;             // sommets soumis au noyau borné
  long long accepted = 0;              // statut kOk
  long long refused_flat_overflow = 0;
  long long refused_admission_shell = 0;
  long long refused_admission_interior = 0;
  long long refused_admission_too_small = 0;
  long long fatal_invariant = 0;       // kInvariantViolation : jamais rejoué, toujours fatal
  long long replayed = 0;              // refus effectivement rejoués sur le chemin non borné
  long long pending = 0;               // refus non rejoués : un trou, pas une statistique
  long long committed_flats = 0;       // flats des sommets kOk
  long long committed_couples = 0;
  long long replayed_flats = 0;        // flats reconstruits au rejeu
  long long replayed_couples = 0;
  long long reference_flats = 0;       // le CPU complet, sur TOUS les sommets
  long long reference_couples = 0;
  long long mismatches = 0;
  int flat_high_water = 0;             // sur TOUS les sommets, refus compris
  int committed_flat_high_water = 0;   // sur les seuls sommets kOk

  long long total_refused() const {
    return refused_flat_overflow + refused_admission_shell + refused_admission_interior +
           refused_admission_too_small;
  }
  bool ledger_balances() const { return total_refused() == replayed + pending; }
  bool mass_identity() const {
    return committed_flats + replayed_flats == reference_flats &&
           committed_couples + replayed_couples == reference_couples;
  }
};

inline void summarise_into(const VertexVerdict* verdicts, int count, WavefrontReceipt* receipt) {
  receipt->presented += count;
  for (int i = 0; i < count; ++i) {
    if (verdicts[i].flat_count > receipt->flat_high_water)
      receipt->flat_high_water = verdicts[i].flat_count;
    if (verdicts[i].status == (int)VerdictStatus::kInvariantViolation) {
      ++receipt->fatal_invariant;
      continue;
    }
    if (verdicts[i].status == (int)VerdictStatus::kFlatOverflow) {
      ++receipt->refused_flat_overflow;
      continue;
    }
    ++receipt->accepted;
    receipt->committed_flats += verdicts[i].flat_count;
    if (verdicts[i].flat_count > receipt->committed_flat_high_water)
      receipt->committed_flat_high_water = verdicts[i].flat_count;
    unsigned long long mask = verdicts[i].admissible;
    while (mask != 0) { ++receipt->committed_couples; mask &= mask - 1; }
  }
}

// Comparaison BIT À BIT de deux lots, SIGNATURE COMPRISE. C'est la seule chose
// que la session GPU doit établir : le device et l'hôte ne diffèrent sur aucun
// bit, ou la session échoue.
inline long long count_mismatches(const VertexVerdict* a, const VertexVerdict* b, int count) {
  long long bad = 0;
  for (int i = 0; i < count; ++i)
    if (a[i].admissible != b[i].admissible || a[i].flat_count != b[i].flat_count ||
        a[i].status != b[i].status || a[i].signature != b[i].signature)
      ++bad;
  return bad;
}

}  // namespace device
}  // namespace mhgp3v
