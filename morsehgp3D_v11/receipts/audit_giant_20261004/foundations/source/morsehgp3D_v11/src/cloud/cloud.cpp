// Preparation du nuage (cloud.hpp) : controle des tailles et du domaine, tri par base sur (cle de Morton, PointId),
// regroupement des positions egales en sites. Port de prepare_cloud de la v10 (src/cloud/cloud.cpp du raccord R2,
// commit 865f5e6) ; le tri indirect par std::sort y est remplace par un tri par base sequentiel.
//
// Tri par base, du chiffre de poids faible au chiffre de poids fort (LSD), chiffres de 11 bits. Les trois premiers
// chiffres sont ceux du PointId, les suivants ceux de la cle de Morton : la cle composee est (cle, PointId), la cle
// de Morton etant la plus significative. Chaque passe est un tri stable par denombrement ; par recurrence sur les
// passes, apres le chiffre j les enregistrements sont tries selon les chiffres 0 .. j. Donc :
//   - apres les trois chiffres du PointId, ils sont tries par PointId : deux identifiants egaux sont voisins, et le
//     controle des doublons est une lecture ;
//   - apres tous les chiffres, ils sont tries par cle de Morton puis par PointId : les points d'un site sont
//     voisins, PointId croissants.
// Une passe dont tous les enregistrements portent le meme chiffre est l'identite (tri stable d'une cle constante) :
// elle est sautee, ce qui ne change pas le resultat.
#include "cloud/cloud.hpp"

#include <utility>

namespace mhgp11 {

namespace {

// Un point pour le tri. index < kNone (controle des tailles) : il tient dans un u32.
struct Record {
  MortonKey key;
  u32 id;     // PointId
  u32 index;  // rang du point dans l'entree
};
static_assert(sizeof(Record) == (kCoordBits <= 21 ? 16 : 32), "cloud : taille du tri dans la formule memoire");

inline constexpr int kDigitBits = 11;
inline constexpr u32 kBuckets = u32{1} << kDigitBits;
inline constexpr int kIdDigits = 3;
inline constexpr int kKeyDigits = (kMortonBits + kDigitBits - 1) / kDigitBits;
inline constexpr int kDigits = kIdDigits + kKeyDigits;
// Les chiffres couvrent tous les bits du PointId (32) et de la cle (3B) : aucun bit n'echappe au tri.
static_assert(kIdDigits * kDigitBits >= 32, "cloud : les chiffres du PointId couvrent 32 bits");
static_assert(kKeyDigits * kDigitBits >= kMortonBits, "cloud : les chiffres de la cle couvrent 3B bits");
// Le plus grand decalage reste dans le type decale.
static_assert(kDigitBits * (kIdDigits - 1) < 32, "cloud : decalage du PointId dans u32");
static_assert(kDigitBits * (kKeyDigits - 1) < static_cast<int>(sizeof(MortonKey)) * 8,
              "cloud : decalage de la cle dans son type");

// Chiffre de rang `pass` : passes 0 .. kIdDigits - 1 sur le PointId, puis kKeyDigits passes sur la cle.
constexpr u32 digit_of(const Record& r, int pass) noexcept {
  if (pass < kIdDigits) return (r.id >> (kDigitBits * pass)) & (kBuckets - 1);
  return static_cast<u32>(r.key >> (kDigitBits * (pass - kIdDigits))) & (kBuckets - 1);
}

// Vrai si toute coordonnee est <= max, ou max = 2^b - 1. Une valeur depasse max si et seulement si elle a un bit de
// rang >= b ; le OU de toutes les coordonnees a un tel bit si et seulement si l'une d'elles en a un.
bool in_domain(std::span<const u32> x, std::span<const u32> y, std::span<const u32> z, u32 max) noexcept {
  u32 any = 0;
  for (u64 i = 0; i < x.size(); ++i) any |= x[i] | y[i] | z[i];
  return any <= max;
}

// Ecrit les enregistrements dans l'ordre de l'entree et compte, pour chaque passe, l'effectif de chaque chiffre.
// Chaque effectif est au plus n < 2^32 : il tient dans un u32.
void fill_records(std::span<const u32> x, std::span<const u32> y, std::span<const u32> z,
                  std::span<const PointId> ids, Record* records, u32* counts) noexcept {
  for (u64 i = 0; i < u64{kBuckets} * kDigits; ++i) counts[i] = 0;
  for (u64 i = 0; i < x.size(); ++i) {
    const Record r{morton_key(x[i], y[i], z[i]), idx(ids[i]), static_cast<u32>(i)};
    records[i] = r;
    for (int pass = 0; pass < kDigits; ++pass) ++counts[u64{kBuckets} * pass + digit_of(r, pass)];
  }
}

// Passes first .. last - 1 du tri. A l'entree comme a la sortie, src porte les enregistrements et dst est le tampon
// de travail. Les effectifs d'un chiffre ne dependent pas de l'ordre des enregistrements : ceux de fill_records
// valent pour toutes les passes. La somme des effectifs d'une passe vaut n < 2^32 : les positions tiennent dans u32.
void sort_passes(Record*& src, Record*& dst, u64 n, u32* counts, int first, int last) noexcept {
  for (int pass = first; pass < last; ++pass) {
    u32* bucket = counts + u64{kBuckets} * pass;
    if (bucket[digit_of(src[0], pass)] == n) continue;
    u32 start = 0;
    for (u32 b = 0; b < kBuckets; ++b) {
      const u32 count = bucket[b];
      bucket[b] = start;
      start += count;
    }
    for (u64 i = 0; i < n; ++i) dst[bucket[digit_of(src[i], pass)]++] = src[i];
    std::swap(src, dst);
  }
}

// Enregistrements tries par PointId : vrai si deux voisins portent le meme identifiant.
bool has_duplicate_id(const Record* sorted, u64 n) noexcept {
  for (u64 i = 1; i < n; ++i)
    if (sorted[i].id == sorted[i - 1].id) return true;
  return false;
}

// Enregistrements tries par cle : nombre de cles distinctes. n >= 1.
u64 count_sites(const Record* sorted, u64 n) noexcept {
  u64 sites = 1;
  for (u64 i = 1; i < n; ++i)
    if (sorted[i].key != sorted[i - 1].key) ++sites;
  return sites;
}

// Vues internes, valables pendant le remplissage uniquement. Aucun appelant ne peut les obtenir du proprietaire.
struct Output {
  std::span<u32> x, y, z, w;
  std::span<u64> offsets;
  std::span<PointId> ids;
};

// Remplit le resultat depuis les enregistrements tries par (cle, PointId). Les tableaux du nuage ont deja leur taille.
void fill_cloud(const Record* sorted, u64 n, std::span<const u32> x, std::span<const u32> y,
                std::span<const u32> z, Output cloud) noexcept {
  u64 sites = 0;
  for (u64 i = 0; i < n; ++i) {
    const Record& r = sorted[i];
    if (i == 0 || r.key != sorted[i - 1].key) {
      cloud.x[sites] = x[r.index];
      cloud.y[sites] = y[r.index];
      cloud.z[sites] = z[r.index];
      cloud.w[sites] = 0;
      cloud.offsets[sites] = i;
      ++sites;
    }
    ++cloud.w[sites - 1];
    cloud.ids[i] = make_id<PointId>(r.id);
  }
  cloud.offsets[sites] = n;
}

}  // namespace

Outcome check_cloud_sizes(u64 x, u64 y, u64 z, u64 ids) noexcept {
  if (x == 0) return fail(Reason::empty_input);
  if (y != x || z != x || ids != x) return fail(Reason::size_mismatch);
  if (x >= kNone) return fail(Reason::index_overflow_u32);
  return {};
}

Result<Cloud> prepare_cloud(std::span<const u32> x, std::span<const u32> y, std::span<const u32> z,
                            std::span<const PointId> ids, CoordWidth width, MemoryBudget& budget) noexcept {
  MHGP11_TRY(check_cloud_sizes(x.size(), y.size(), z.size(), ids.size()));
  const u64 n = x.size();
  if (!in_domain(x, y, z, width.max())) return fail(Reason::coordinate_out_of_domain);

  Buffer<Record> first, second;
  Buffer<u32> counts;
  MHGP11_TRY(first.allocate(n, budget));
  MHGP11_TRY(second.allocate(n, budget));
  MHGP11_TRY(counts.allocate(u64{kBuckets} * kDigits, budget));
  fill_records(x, y, z, ids, first.data(), counts.data());
  Record* src = first.data();
  Record* dst = second.data();
  sort_passes(src, dst, n, counts.data(), 0, kIdDigits);
  if (has_duplicate_id(src, n)) return fail(Reason::duplicate_point_id);
  sort_passes(src, dst, n, counts.data(), kIdDigits, kDigits);

  // Le tampon qui ne porte pas le resultat du tri et les histogrammes sont rendus avant d'allouer le resultat.
  counts.reset();
  (src == first.data() ? second : first).reset();
  const u64 sites = count_sites(src, n);
  Cloud cloud;
  MHGP11_TRY(cloud.x_.allocate(sites, budget));
  MHGP11_TRY(cloud.y_.allocate(sites, budget));
  MHGP11_TRY(cloud.z_.allocate(sites, budget));
  MHGP11_TRY(cloud.w_.allocate(sites, budget));
  MHGP11_TRY(cloud.ids_.off.allocate(sites + 1, budget));
  MHGP11_TRY(cloud.ids_.val.allocate(n, budget));
  fill_cloud(src, n, x, y, z, {cloud.x_.span(), cloud.y_.span(), cloud.z_.span(), cloud.w_.span(),
                              cloud.ids_.off.span(), cloud.ids_.val.span()});
  cloud.weight_ = n;
  return cloud;
}

}  // namespace mhgp11
