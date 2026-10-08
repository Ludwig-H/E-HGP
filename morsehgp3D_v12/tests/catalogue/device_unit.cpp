// Portes de la fin d'etage partagee et de la voie appareil (tranche T1-b), jouees sur l'hote par l'executeur Pool :
// le meme code (noyaux et pilotes en source unique) que l'executeur CUDA. Sans GPU, la voie appareil reelle rend
// device_unavailable (porte device_open) ; avec un GPU (session G4), la meme porte compare la voie appareil reelle a la
// voie CPU. Refus propres a la voie appareil : device_unavailable (resource_exhausted), device_fault
// (invariant_violated).
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "device_support.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::catalogue_detail;
using namespace mhgp12::device_test;

namespace {

// Cle F3 de reference, calculee sur les entiers de num (meme fonction que sort.cpp de 671072339).
template <int Words>
double reference_magnitude(const num::Wide<Words>& value) {
  const int length = value.bit_length();
  if (length <= 64) return static_cast<double>(value.words[0]);
  const int shift = length - 64, word = shift / 64, bit = shift % 64;
  u64 top = value.words[word] >> bit;
  if (bit != 0) top |= value.words[word + 1] << (64 - bit);
  return std::ldexp(static_cast<double>(top), shift);
}

// Niveau de num pour le support p (q points) ; rien si le support est degenere.
std::optional<num::Level> num_level(const std::array<std::array<u32, 3>, 4>& p, u32 q) {
  std::array<num::Point, 4> pt{};
  for (u32 k = 0; k < q; ++k) pt[k] = num::Point::make(p[k][0], p[k][1], p[k][2]).value();
  auto sphere = q == 2 ? num::Sphere::through(pt[0], pt[1])
                : q == 3 ? num::Sphere::through(pt[0], pt[1], pt[2])
                         : num::Sphere::through(pt[0], pt[1], pt[2], pt[3]);
  if (!sphere.ok() || !sphere.value()) return std::nullopt;
  return sphere.value()->level();
}

bool words_equal(const fin::LevelWords& w, const num::Level& level) {
  const auto n = num::to_wide(level.numerator());
  const auto d = num::to_wide(level.denominator());
  for (int i = 0; i < fin::kNumWords; ++i)
    if (w.n[i] != (i < static_cast<int>(n.words.size()) ? n.words[i] : 0)) return false;
  for (int i = 0; i < fin::kDenWords; ++i)
    if (w.d[i] != (i < static_cast<int>(d.words.size()) ? d.words[i] : 0)) return false;
  return true;
}

}  // namespace

// Niveaux exacts a mots contre num::Sphere::through (memes numerateurs et denominateurs non reduits), cle F3 contre la
// cle de reference, comparaison exacte contre num::compare : supports aleatoires a q = 2, 3, 4 et etendues 3 a B.
MHGP12_TEST(level_words, 3000) {
  device_test::Mix mix{20261007};
  std::vector<std::pair<fin::LevelWords, num::Level>> made;
  for (const int bits : {3, 8, 16, 20, kCoordBits}) {
    for (u32 q = 2; q <= 4; ++q) {
      for (int trial = 0; trial < 150; ++trial) {
        std::array<std::array<u32, 3>, 4> p{};
        for (u32 k = 0; k < q; ++k)
          for (int a = 0; a < 3; ++a) p[k][a] = static_cast<u32>(mix.next() & ((u64{1} << bits) - 1));
        const auto level = num_level(p, q);
        if (!level) continue;
        const u32* pts[4] = {p[0].data(), p[1].data(), p[2].data(), p[3].data()};
        fin::LevelWords w{};
        bool fault = false;
        fin::ball_level(pts, q, w, fault);
        CHECK(!fault);
        CHECK(words_equal(w, *level));
        const double key = reference_magnitude(num::to_wide(level->numerator())) /
                           reference_magnitude(num::to_wide(level->denominator()));
        u64 bits_ref = 0;
        std::memcpy(&bits_ref, &key, sizeof bits_ref);
        CHECK_EQ(fin::level_key_bits(w), bits_ref);
        made.emplace_back(w, *level);
      }
    }
  }
  for (std::size_t i = 1; i < made.size(); ++i) {
    CHECK_EQ(fin::compare_levels(made[i - 1].first, made[i].first), num::compare(made[i - 1].second, made[i].second));
    CHECK_EQ(fin::compare_levels(made[i].first, made[i].first), 0);
  }
  // Supports degeneres : faute (le pilote la rend en catalogue_invariant).
  const std::array<u32, 3> a{1, 2, 3}, b{2, 4, 6}, c{3, 6, 9}, d{4, 1, 0};
  const u32* line[4] = {a.data(), b.data(), c.data(), d.data()};
  fin::LevelWords w{};
  bool fault = false;
  fin::ball_level(line, 3, w, fault);
  CHECK(fault);
}

// Tri par base stable contre std::stable_sort : cles u64 et Key2, doublons, octets communs sautes, tailles autour
// d'une tuile ; sommes prefixes contre la somme sequentielle.
MHGP12_TEST(radix_and_scan, 40) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = sched::make_pool({3});
  REQUIRE(pool.ok());
  PoolExecutor b{*pool.value(), budget};
  device_test::Mix mix{7};
  for (const u64 n : {0ull, 1ull, 2ull, 31ull, 1023ull, 1024ull, 1025ull, 5000ull, 70001ull}) {
    fin::RadixArrays<PoolExecutor, fin::Key2> a;
    REQUIRE(fin::radix_reserve(b, a, n).ok());
    std::vector<fin::Key2> keys(n);
    for (u64 i = 0; i < n; ++i) {
      keys[i] = fin::Key2{(mix.next() % 97) << 24 | 0xAB, (mix.next() % 3) << 40};  // octets communs et doublons
      a.keys[0].data()[i] = keys[i];
      a.vals[0].data()[i] = static_cast<u32>(i);
    }
    const auto cur = fin::radix_sort(b, a, n, 16);
    REQUIRE(cur.ok());
    std::vector<u32> expect(n);
    for (u64 i = 0; i < n; ++i) expect[i] = static_cast<u32>(i);
    std::stable_sort(expect.begin(), expect.end(), [&](u32 x, u32 y) { return fin::key2_cmp(keys[x], keys[y]) < 0; });
    bool equal = true;
    for (u64 i = 0; i < n; ++i) equal = equal && a.vals[cur.value()].data()[i] == expect[i];
    CHECK(equal);
    fin::ScanArrays<PoolExecutor> s;
    FrontArray<u64> in, out;
    REQUIRE(in.ensure(n + 1, budget).ok() && out.ensure(n + 1, budget).ok());
    u64 sum = 0;
    for (u64 i = 0; i < n; ++i) in.data()[i] = mix.next() % 1000;
    const auto total = fin::exclusive_scan(b, s, in.data(), out.data(), n);
    REQUIRE(total.ok());
    bool prefix = true;
    for (u64 i = 0; i < n; ++i) {
      prefix = prefix && out.data()[i] == sum;
      sum += in.data()[i];
    }
    CHECK(prefix);
    CHECK_EQ(total.value(), sum);
    const auto inplace = fin::exclusive_scan(b, s, in.data(), in.data(), n);
    CHECK(inplace.ok() && inplace.value() == sum);
    bool same_values = true;
    for (u64 i = 0; i < n; ++i) same_values = same_values && in.data()[i] == out.data()[i];
    CHECK(same_values);
  }
}

// Premier toucher des pages des sorties anticipees (tranche T2-d, finish_outputs.hpp) : touch_pages ecrit un zero au
// premier octet de chaque page touchee par l'intervalle, jamais hors de l'intervalle ; prefault (MADV_POPULATE_WRITE,
// repli touch_pages) ne change aucun octet hors de l'intervalle, et dans l'intervalle au plus ces premiers octets.
namespace {

constexpr std::uintptr_t kPage = 4096;

// Octets de area changes par rapport au motif 0x5A : tous dans [begin, end), nuls, au debut de l'intervalle ou d'une
// page ; rend leur nombre, ou ~0 si un octet change sort de ces regles.
u64 changed_bytes(const std::vector<u8>& area, const u8* begin, const u8* end) {
  u64 count = 0;
  for (std::size_t i = 0; i < area.size(); ++i) {
    if (area[i] == 0x5A) continue;
    const u8* at = area.data() + i;
    const auto address = reinterpret_cast<std::uintptr_t>(at);
    if (at < begin || at >= end || area[i] != 0 || (at != begin && address % kPage != 0)) return ~u64{0};
    ++count;
  }
  return count;
}

}  // namespace

MHGP12_TEST(prefault_pages, 40) {
  std::vector<u8> area(16 * kPage, u8{0x5A});
  auto pool = sched::make_pool({3});
  REQUIRE(pool.ok());
  for (const u64 offset : {1u, 17u, 4095u, 4096u, 4097u}) {
    for (const u64 length : {1u, 4095u, 4096u, 12289u}) {
      std::fill(area.begin(), area.end(), u8{0x5A});
      u8* begin = area.data() + 2 * kPage + offset;
      const auto first = reinterpret_cast<std::uintptr_t>(begin);
      const u64 pages = (first + length - 1) / kPage - first / kPage + 1;
      fin::touch_pages(begin, length);
      CHECK_EQ(changed_bytes(area, begin, begin + length), pages);
      std::fill(area.begin(), area.end(), u8{0x5A});
      CHECK(fin::prefault(begin, length, *pool.value()).ok());
      CHECK(changed_bytes(area, begin, begin + length) <= pages);
    }
  }
  MemoryBudget budget(MemoryBudget::kUnlimited);
  Buffer<u8> fresh;
  REQUIRE(fresh.allocate(u64{3} << 20, budget).ok());
  CHECK(fin::prefault(fresh.data(), fresh.size(), *pool.value()).ok());
  std::fill(fresh.begin(), fresh.end(), u8{7});
  CHECK(std::all_of(fresh.begin(), fresh.end(), [](u8 v) { return v == 7; }));
}

// Memoire de transit demandee par un flux (tranche T2-d, prelecture de l'auditeur Codex) : min(cases * kStreamChunk,
// plus grand segment), donc jamais plus que la copie d'un seul tenant de son plus grand segment d'avant T2-d.
MHGP12_TEST(stream_staging, 5) {
  using catalogue_detail::kStreamChunk;
  using catalogue_detail::stream_staging_bytes;
  using catalogue_detail::StreamSegment;
  const StreamSegment small[2] = {{nullptr, 32, 1000, nullptr, nullptr, false},
                                  {nullptr, 48, 4000, nullptr, nullptr, true}};
  CHECK_EQ(stream_staging_bytes(small, 2), u64{48} * 4000);
  CHECK_EQ(stream_staging_bytes(small, 1), u64{48} * 4000);
  const StreamSegment big[2] = {{nullptr, 32, u64{10} << 20, nullptr, nullptr, false},
                                {nullptr, 8, 3, nullptr, nullptr, false}};
  CHECK_EQ(stream_staging_bytes(big, 2), 2 * kStreamChunk);
  CHECK_EQ(stream_staging_bytes(big, 1), kStreamChunk);
  CHECK_EQ(stream_staging_bytes(std::span<const StreamSegment>(), 2), u64{0});
}

// Refus propres a la voie appareil : statuts graves.
MHGP12_TEST(device_reasons, 2) {
  CHECK(status_of(Reason::device_unavailable) == Status::resource_exhausted);
  CHECK(status_of(Reason::device_fault) == Status::invariant_violated);
}
MHGP12_TEST_MAIN()
