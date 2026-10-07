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
#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

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

inline bool read(const std::string& path, LeafDump& d, std::string& error) {
  detail::Reader r;
  r.f = std::fopen(path.c_str(), "rb");
  if (r.f == nullptr) {
    error = "ouverture en lecture impossible : " + path;
    return false;
  }
  Header& h = d.header;
  r.get(&h, sizeof(h));
  if (!r.ok || h.magic != kMagic || h.version != kVersion || h.n_counters != kCounters) {
    std::fclose(r.f);
    error = "en-tete MHGP12LF v1 invalide : " + path;
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
  // Coherence structurelle : debuts croissants, feuilles dans la liste, m <= 32, boites non vides.
  for (u64 j = 0; j < h.n_leaves; ++j) {
    const Job& job = d.jobs[j];
    if (job.m == 0 || job.m > kMaxLeafSites || job.begin + job.m > h.n_leaf_sites ||
        d.record_begin[j] > d.record_begin[j + 1] || d.population_begin[j] > d.population_begin[j + 1]) {
      error = "feuille incoherente dans " + path;
      return false;
    }
    for (int a = 0; a < 3; ++a)
      if (job.lo[a] >= job.hi[a]) {
        error = "boite vide dans " + path;
        return false;
      }
  }
  if (d.record_begin[h.n_leaves] != h.n_records || d.population_begin[h.n_leaves] != h.n_population) {
    error = "totaux incoherents dans " + path;
    return false;
  }
  return true;
}

}  // namespace mhgp12::dump
