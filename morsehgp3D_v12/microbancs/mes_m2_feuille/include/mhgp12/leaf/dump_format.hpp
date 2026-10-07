// Format MHGP12LF v1 : feuilles du catalogue de la v11 et leur sortie de reference (leaf.cpp), pour le microbanc
// MES-M2. Hors produit, hote seulement, bibliotheque standard. Donnees derivees de SemanticKITTI : jamais dans le depot.
//
// Disposition (petit-boutiste, sections dans cet ordre, chacune completee a un multiple de 8 octets) :
//   en-tete      Header (128 octets)
//   nuage        x[n_sites], y[n_sites], z[n_sites] (u32, ordre SiteIdx du Cloud prepare de la v11)
//   feuilles     Job[n_leaves] (64 octets : debut dans la liste, m, boite T0 demi-ouverte lo/hi), ordre du parcours
//   sites        u32[n_leaf_sites] : SiteIdx globaux croissants de chaque feuille
//   compteurs    u32[n_leaves][kCounters] : quinze compteurs logiques de leaf.cpp (ordre de leaf_device::Counts)
//   statuts      u8[n_leaves] : bit 0 leaf.cpp a rendu la feuille ; bit 1 leaf_device.hpp (v11, hote) l'a rendue non
//                resolue ; bit 2 leaf_device.hpp resolue mais different de leaf.cpp (doit rester nul)
//   debuts       u64[n_leaves + 1] : premier enregistrement de chaque feuille (prefixe), puis le total
//   emissions    Record[n_records] (8 octets, disposition de LeafRecord v11 : S* en rangs locaux, p, m, qmin), dans
//                l'ordre d'emission de leaf.cpp
//   debuts       u64[n_leaves + 1] : premiere incidence de chaque feuille, puis le total
//   populations  u8[n_population] : rangs locaux, I puis U de chaque emission, croissants
//   fin          u64 : FNV-1a 64 de tous les octets precedents
//
// Admission (constat CST-0215 de l'auditeur Codex) : une empreinte FNV-1a juste protege l'integrite des octets, PAS
// le domaine qu'attendent les noyaux. read() refuse donc, jamais en silence et avant tout noyau :
//   - avant toute allocation dependante des comptes : un en-tete hors domaine (profil different de celui du binaire,
//     K hors de 1..12, taille de feuille hors de K+3..256, max_leaf hors de taille..256, drapeaux inconnus ou sans
//     graphe de paires, comptes au-dela des domaines u32 des indices du banc, vidage sans feuille) ou dont les
//     tailles de sections ne donnent pas exactement la taille reelle du fichier ;
//   - apres lecture et controle de l'empreinte (validate) : coordonnees hors de [0, 2^B) ; feuilles qui ne pavent pas
//     la liste des sites dans l'ordre (debut_0 = 0, debut_{j+1} = debut_j + m_j, total = n_leaf_sites ; donc aucun
//     debordement de debut + m), m hors de 1..32 ; boites hors du domaine T0 (0 <= lo < hi <= 2^B) ; sites hors du
//     nuage, non strictement croissants ou repetes dans une feuille ; statut sans le bit de leaf.cpp, avec le bit
//     d'ecart du temoin v11 ou un bit inconnu ; debuts d'enregistrements et d'incidences non nuls a l'origine, non
//     monotones ou de total faux ; compteurs emitted/incidences differents des plages de la feuille ; enregistrement
//     dont qmin n'est pas 2..4, dont S* n'est pas strictement croissant dans la feuille (0xFF au-dela de qmin), dont
//     p + m depasse la feuille, dont p + qmin depasse K + 1 ; populations I et U non strictement croissantes, hors
//     de la feuille, non disjointes, coquille sans S* ; somme des incidences differente de la plage de la feuille.
// Le domaine des coordonnees et des boites est celui du profil du binaire (MHGP12_COORD_BITS) : un vidage d'un autre
// profil est refuse, jamais reinterprete.
#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#ifndef MHGP12_COORD_BITS
#error "MHGP12LF : MHGP12_COORD_BITS (profil du binaire, 21 ou 24) requis pour l'admission des vidages"
#endif

namespace mhgp12::dump {

using u8 = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i64 = std::int64_t;

static_assert(std::endian::native == std::endian::little, "MHGP12LF : hote petit-boutiste seulement");

inline constexpr std::array<char, 8> kMagic = {'M', 'H', 'G', 'P', '1', '2', 'L', 'F'};
inline constexpr u64 kVersion = 1;
inline constexpr u32 kCounters = 15;
inline constexpr u8 kNoLocal = 0xFF;
inline constexpr u32 kMaxLeafSites = 32;
// Domaine d'admission (voir l'en-tete du fichier).
inline constexpr u64 kProfileBits = MHGP12_COORD_BITS;      // profil du binaire qui lit
inline constexpr u64 kMaxKmax = 12;                         // K du catalogue source (mhgp12_leaf_dump, v11)
inline constexpr u64 kMaxLeafSize = 256;                    // max_leaf du produit v11
inline constexpr u64 kMaxRecordsPerLeaf = 496 + 4960 + 35960;  // C(32,2) + C(32,3) + C(32,4)
inline constexpr u64 kMaxIndex = 0xFFFFFFFFull;             // indices u32 du banc (feuilles, sites, incidences)

// Noms des quinze compteurs logiques, dans l'ordre de leaf_device::Counts (v11).
inline constexpr std::array<const char*, kCounters> kCounterNames = {
    "dominance_tests",   "prefixes",           "judged",
    "census_tests",      "emitted",            "incidences",
    "q4_candidates",     "q4_levels",          "region_pair_tests",
    "region_pair_rejects", "region_line_tests", "region_line_rejects",
    "region_line_evaluations", "region_line_cache_hits", "region_line_fallbacks"};

struct Job {  // meme disposition que catalogue_detail::LeafJob (v11)
  u64 begin = 0;
  u32 m = 0, pad = 0;
  i64 lo[3] = {0, 0, 0}, hi[3] = {0, 0, 0};
};
static_assert(sizeof(Job) == 64);

struct Record {  // meme disposition que catalogue_detail::LeafRecord (v11)
  u8 support[4];
  u8 p, m, qmin, pad;
};
static_assert(sizeof(Record) == 8);

inline constexpr u64 kFlagCache = 1, kFlagPairGraph = 2;
inline constexpr u8 kStatusReference = 1, kStatusV11DeviceUnresolved = 2, kStatusV11DeviceMismatch = 4;

struct Header {
  std::array<char, 8> magic = kMagic;
  u64 version = kVersion;
  u64 coord_bits = 0;
  u64 kmax = 0;
  u64 leaf_size = 0;
  u64 max_leaf = 0;
  u64 flags = 0;
  u64 n_sites = 0;
  u64 n_leaves = 0;
  u64 n_leaf_sites = 0;
  u64 n_records = 0;
  u64 n_population = 0;
  u64 n_counters = kCounters;
  u64 walk_leaves = 0;         // toutes les feuilles du parcours (ledger v11), feuilles hors lot comprises
  u64 walk_inline_leaves = 0;  // feuilles de plus de 32 sites traitees en ligne par leaf.cpp (absentes du vidage)
  u64 reserved = 0;
};
static_assert(sizeof(Header) == 128);

struct LeafDump {
  Header header;
  std::vector<u32> x, y, z;
  std::vector<Job> jobs;
  std::vector<u32> sites;
  std::vector<u32> counts;  // n_leaves * kCounters
  std::vector<u8> status;
  std::vector<u64> record_begin;  // n_leaves + 1
  std::vector<Record> records;
  std::vector<u64> population_begin;  // n_leaves + 1
  std::vector<u8> population;
  u64 digest = 0;  // FNV-1a 64 verifie par read() (hors format : empreinte de la lecture, citee par les outils)

  const u32* leaf_sites(u64 j) const { return sites.data() + jobs[j].begin; }
  const u32* leaf_counts(u64 j) const { return counts.data() + j * kCounters; }
};

inline u64 fnv1a(u64 h, const void* data, std::size_t bytes) {
  const auto* p = static_cast<const unsigned char*>(data);
  for (std::size_t i = 0; i < bytes; ++i) {
    h ^= p[i];
    h *= 1099511628211ull;
  }
  return h;
}
inline constexpr u64 kFnvBasis = 14695981039346656037ull;

namespace detail {
struct Writer {
  std::FILE* f = nullptr;
  u64 hash = kFnvBasis;
  bool ok = true;
  void put(const void* data, std::size_t bytes) {
    if (!ok || bytes == 0) return;
    hash = fnv1a(hash, data, bytes);
    ok = std::fwrite(data, 1, bytes, f) == bytes;
  }
  template <class T>
  void section(const std::vector<T>& v) {
    put(v.data(), v.size() * sizeof(T));
    const std::size_t rest = (8 - (v.size() * sizeof(T)) % 8) % 8;
    static const char zeros[8] = {};
    put(zeros, rest);
  }
};
struct Reader {
  std::FILE* f = nullptr;
  u64 hash = kFnvBasis;
  bool ok = true;
  void get(void* data, std::size_t bytes) {
    if (!ok || bytes == 0) return;
    ok = std::fread(data, 1, bytes, f) == bytes;
    if (ok) hash = fnv1a(hash, data, bytes);
  }
  template <class T>
  void section(std::vector<T>& v, u64 count) {
    v.resize(count);
    get(v.data(), count * sizeof(T));
    const std::size_t rest = (8 - (count * sizeof(T)) % 8) % 8;
    char pad[8];
    get(pad, rest);
  }
};
}  // namespace detail

inline bool write(const std::string& path, const LeafDump& d, std::string& error) {
  const Header& h = d.header;
  if (d.x.size() != h.n_sites || d.y.size() != h.n_sites || d.z.size() != h.n_sites || d.jobs.size() != h.n_leaves ||
      d.sites.size() != h.n_leaf_sites || d.counts.size() != h.n_leaves * kCounters || d.status.size() != h.n_leaves ||
      d.record_begin.size() != h.n_leaves + 1 || d.records.size() != h.n_records ||
      d.population_begin.size() != h.n_leaves + 1 || d.population.size() != h.n_population) {
    error = "tailles incoherentes avec l'en-tete";
    return false;
  }
  detail::Writer w;
  w.f = std::fopen(path.c_str(), "wb");
  if (w.f == nullptr) {
    error = "ouverture en ecriture impossible : " + path;
    return false;
  }
  w.put(&h, sizeof(h));
  w.section(d.x);
  w.section(d.y);
  w.section(d.z);
  w.section(d.jobs);
  w.section(d.sites);
  w.section(d.counts);
  w.section(d.status);
  w.section(d.record_begin);
  w.section(d.records);
  w.section(d.population_begin);
  w.section(d.population);
  const u64 digest = w.hash;
  if (w.ok) w.ok = std::fwrite(&digest, 1, sizeof(digest), w.f) == sizeof(digest);
  w.ok = (std::fclose(w.f) == 0) && w.ok;
  if (!w.ok) error = "ecriture incomplete : " + path;
  return w.ok;
}

namespace detail {
inline u64 padded(u64 bytes) { return bytes + (8 - bytes % 8) % 8; }

// En-tete : domaine des champs et taille exacte du fichier, AVANT toute allocation dependante des comptes. Les bornes
// sont posees dans un ordre qui rend les produits suivants exempts de depassement (u64).
inline bool admit_header(const Header& h, u64 file_bytes, std::string& why) {
  if (h.coord_bits != kProfileBits) {
    why = "profil " + std::to_string(h.coord_bits) + " different du profil du binaire " + std::to_string(kProfileBits);
    return false;
  }
  if (h.kmax < 1 || h.kmax > kMaxKmax) {
    why = "K = " + std::to_string(h.kmax) + " hors de 1..12";
    return false;
  }
  if (h.leaf_size < h.kmax + 3 || h.leaf_size > kMaxLeafSize) {
    why = "taille de feuille " + std::to_string(h.leaf_size) + " hors de K+3..256";
    return false;
  }
  if (h.max_leaf < h.leaf_size || h.max_leaf > kMaxLeafSize) {
    why = "max_leaf " + std::to_string(h.max_leaf) + " hors de taille..256";
    return false;
  }
  if ((h.flags & ~(kFlagCache | kFlagPairGraph)) != 0 || (h.flags & kFlagPairGraph) == 0) {
    why = "drapeaux " + std::to_string(h.flags) + " inconnus ou sans graphe de paires";
    return false;
  }
  if (h.n_sites == 0 || h.n_sites > kMaxIndex || h.n_leaves == 0 || h.n_leaves > kMaxIndex) {
    why = "nombre de sites ou de feuilles hors de 1..2^32-1";
    return false;
  }
  if (h.n_leaf_sites < h.n_leaves || h.n_leaf_sites > u64{kMaxLeafSites} * h.n_leaves) {
    why = "liste des sites hors de [feuilles, 32 feuilles]";
    return false;
  }
  if (h.n_records > kMaxRecordsPerLeaf * h.n_leaves || h.n_population > kMaxIndex ||
      h.n_population > u64{kMaxLeafSites} * h.n_records) {
    why = "enregistrements ou incidences hors domaine";
    return false;
  }
  if (h.walk_leaves < h.n_leaves || h.walk_inline_leaves != h.walk_leaves - h.n_leaves) {
    why = "comptes du parcours incoherents";
    return false;
  }
  if (h.reserved != 0) {
    why = "champ reserve non nul";
    return false;
  }
  u64 expected = sizeof(Header);
  expected += 3 * padded(4 * h.n_sites);
  expected += padded(sizeof(Job) * h.n_leaves);
  expected += padded(4 * h.n_leaf_sites);
  expected += padded(4 * kCounters * h.n_leaves);
  expected += padded(h.n_leaves);
  expected += padded(8 * (h.n_leaves + 1));
  expected += padded(sizeof(Record) * h.n_records);
  expected += padded(8 * (h.n_leaves + 1));
  expected += padded(h.n_population);
  expected += sizeof(u64);
  if (expected != file_bytes) {
    why = "taille du fichier " + std::to_string(file_bytes) + " differente de celle de l'en-tete " +
          std::to_string(expected);
    return false;
  }
  return true;
}
}  // namespace detail

// Domaine semantique d'un vidage lu (voir l'en-tete du fichier). Rend faux et une raison au premier ecart.
inline bool validate(const LeafDump& d, std::string& why) {
  const Header& h = d.header;
  const u64 side = u64{1} << kProfileBits;
  if (d.x.size() != h.n_sites || d.y.size() != h.n_sites || d.z.size() != h.n_sites ||
      d.jobs.size() != h.n_leaves || d.sites.size() != h.n_leaf_sites || d.counts.size() != h.n_leaves * kCounters ||
      d.status.size() != h.n_leaves || d.record_begin.size() != h.n_leaves + 1 || d.records.size() != h.n_records ||
      d.population_begin.size() != h.n_leaves + 1 || d.population.size() != h.n_population) {
    why = "tailles des tableaux differentes de l'en-tete";
    return false;
  }
  for (u64 s = 0; s < h.n_sites; ++s)
    if (d.x[s] >= side || d.y[s] >= side || d.z[s] >= side) {
      why = "coordonnee du site " + std::to_string(s) + " hors de [0, 2^" + std::to_string(kProfileBits) + ")";
      return false;
    }
  if (d.record_begin[0] != 0 || d.population_begin[0] != 0 || d.record_begin[h.n_leaves] != h.n_records ||
      d.population_begin[h.n_leaves] != h.n_population) {
    why = "debuts d'enregistrements ou d'incidences : origine ou total faux";
    return false;
  }
  u64 at = 0;  // debut attendu de la feuille courante (pavage de la liste des sites)
  for (u64 j = 0; j < h.n_leaves; ++j) {
    const Job& job = d.jobs[j];
    const std::string leaf = " (feuille " + std::to_string(j) + ")";
    if (job.m == 0 || job.m > kMaxLeafSites || job.pad != 0 || job.begin != at || job.m > h.n_leaf_sites - at) {
      why = "feuille hors du pavage de la liste des sites ou m hors de 1..32" + leaf;
      return false;
    }
    at += job.m;
    for (int a = 0; a < 3; ++a)
      if (job.lo[a] < 0 || job.lo[a] >= job.hi[a] || job.hi[a] > static_cast<i64>(side)) {
        why = "boite hors du domaine T0 [0, 2^B]" + leaf;
        return false;
      }
    const u32* sites = d.sites.data() + job.begin;
    for (u32 i = 0; i < job.m; ++i)
      if (sites[i] >= h.n_sites || (i > 0 && sites[i] <= sites[i - 1])) {
        why = "site hors du nuage, non croissant ou repete" + leaf;
        return false;
      }
    const u8 st = d.status[j];
    if ((st & ~u8(kStatusReference | kStatusV11DeviceUnresolved | kStatusV11DeviceMismatch)) != 0 ||
        (st & kStatusReference) == 0 || (st & kStatusV11DeviceMismatch) != 0) {
      why = "statut " + std::to_string(st) + " incoherent (bit leaf.cpp absent, ecart du temoin v11 ou bit inconnu)" +
            leaf;
      return false;
    }
    const u64 r0 = d.record_begin[j], r1 = d.record_begin[j + 1];
    const u64 p0 = d.population_begin[j], p1 = d.population_begin[j + 1];
    if (r1 < r0 || r1 > h.n_records || p1 < p0 || p1 > h.n_population || r1 - r0 > kMaxRecordsPerLeaf) {
      why = "debuts d'enregistrements ou d'incidences non monotones ou hors bornes" + leaf;
      return false;
    }
    const u32* c = d.counts.data() + j * kCounters;
    if (c[4] != r1 - r0 || c[5] != p1 - p0) {  // emitted, incidences (ordre de leaf_device::Counts)
      why = "compteurs emitted/incidences differents des plages de la feuille" + leaf;
      return false;
    }
    u64 pop = p0;
    for (u64 b = r0; b < r1; ++b) {
      const Record& r = d.records[b];
      const std::string rec = " (enregistrement " + std::to_string(b) + ", feuille " + std::to_string(j) + ")";
      if (r.qmin < 2 || r.qmin > 4 || r.pad != 0 || r.m < r.qmin || u32(r.p) + r.m > job.m ||
          u64(r.p) + r.qmin > h.kmax + 1) {
        why = "enregistrement hors domaine (qmin, p, m, p + qmin <= K + 1)" + rec;
        return false;
      }
      for (u32 k = 0; k < 4; ++k) {
        const bool used = k < r.qmin;
        if (used ? (r.support[k] >= job.m || (k > 0 && r.support[k] <= r.support[k - 1]))
                 : r.support[k] != kNoLocal) {
          why = "S* hors de la feuille, non croissant ou mal termine" + rec;
          return false;
        }
      }
      const u64 need = u64(r.p) + r.m;
      if (need > p1 - pop) {
        why = "incidences de l'enregistrement au-dela de la plage de la feuille" + rec;
        return false;
      }
      const u8* in = d.population.data() + pop;
      u64 interior = 0, shell = 0;
      for (u64 i = 0; i < need; ++i) {
        const bool first_of_part = i == 0 || i == r.p;
        if (in[i] >= job.m || (!first_of_part && in[i] <= in[i - 1])) {
          why = "population hors de la feuille ou non strictement croissante" + rec;
          return false;
        }
        (i < r.p ? interior : shell) |= u64{1} << in[i];
      }
      u64 support = 0;
      for (u32 k = 0; k < r.qmin; ++k) support |= u64{1} << r.support[k];
      if ((interior & shell) != 0 || (support & ~shell) != 0) {
        why = "populations I et U non disjointes ou coquille sans S*" + rec;
        return false;
      }
      pop += need;
    }
    if (pop != p1) {
      why = "somme des incidences differente de la plage de la feuille" + leaf;
      return false;
    }
  }
  if (at != h.n_leaf_sites) {
    why = "les feuilles ne pavent pas toute la liste des sites";
    return false;
  }
  return true;
}

inline bool read(const std::string& path, LeafDump& d, std::string& error) {
  d = LeafDump{};
  detail::Reader r;
  r.f = std::fopen(path.c_str(), "rb");
  if (r.f == nullptr) {
    error = "ouverture en lecture impossible : " + path;
    return false;
  }
  long end = -1;
  if (std::fseek(r.f, 0, SEEK_END) == 0) end = std::ftell(r.f);
  if (end < 0 || std::fseek(r.f, 0, SEEK_SET) != 0) {
    std::fclose(r.f);
    error = "taille du fichier illisible : " + path;
    return false;
  }
  Header& h = d.header;
  r.get(&h, sizeof(h));
  if (!r.ok || h.magic != kMagic || h.version != kVersion || h.n_counters != kCounters) {
    std::fclose(r.f);
    error = "en-tete MHGP12LF v1 invalide : " + path;
    return false;
  }
  std::string why;
  if (!detail::admit_header(h, static_cast<u64>(end), why)) {
    std::fclose(r.f);
    error = "en-tete refuse (" + why + ") : " + path;
    return false;
  }
  r.section(d.x, h.n_sites);
  r.section(d.y, h.n_sites);
  r.section(d.z, h.n_sites);
  r.section(d.jobs, h.n_leaves);
  r.section(d.sites, h.n_leaf_sites);
  r.section(d.counts, h.n_leaves * kCounters);
  r.section(d.status, h.n_leaves);
  r.section(d.record_begin, h.n_leaves + 1);
  r.section(d.records, h.n_records);
  r.section(d.population_begin, h.n_leaves + 1);
  r.section(d.population, h.n_population);
  const u64 expected = r.hash;
  u64 digest = 0;
  const bool tail = r.ok && std::fread(&digest, 1, sizeof(digest), r.f) == sizeof(digest);
  char extra = 0;
  const bool at_end = std::fread(&extra, 1, 1, r.f) == 0;
  std::fclose(r.f);
  if (!r.ok || !tail || !at_end || digest != expected) {
    error = "vidage tronque ou altere (FNV-1a) : " + path;
    return false;
  }
  d.digest = expected;
  if (!validate(d, why)) {
    error = "vidage refuse (" + why + ") : " + path;
    return false;
  }
  return true;
}

// Empreinte FNV-1a 64 en hexadecimal (16 chiffres), telle que les outils la citent dans leurs sorties JSON.
inline std::string digest_hex(u64 digest) {
  static const char kHex[] = "0123456789abcdef";
  std::string out(16, '0');
  for (int i = 15; i >= 0; --i, digest >>= 4) out[static_cast<std::size_t>(i)] = kHex[digest & 15u];
  return out;
}

}  // namespace mhgp12::dump
