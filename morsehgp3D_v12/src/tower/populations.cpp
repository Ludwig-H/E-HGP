// Index des naissances de l'ordre k (LEM-POP ; principe de src/tower/population_lookup.cpp de la v11 gelee,
// ac081a06f) : si une k-partie F est EXACTEMENT la population I u U d'une boule b de Cat_K (p + m = k), alors b
// contient F et S* est dans U, donc la plus petite boule de F est b, et la cellule (b, k) est une naissance (t = m) :
// pas terminal. Les populations sont distinctes (unicite de la plus petite boule) : la reponse est unique, et une
// population repetee est refusee (tower_invariant), jamais masquee.
//
// Disposition (T2-c, remplace la table a sondage lineaire construite sequentiellement par ordre) : les E naissances de
// population exacte k sont rangees dans l'ordre CANONIQUE (empreinte, population) : empreinte additive (somme modulo
// 2^64 des empreintes de leurs sites, masquee), puis k SiteIdx croissants ; un repertoire donne la premiere place de
// chaque seau (d bits de tete de l'empreinte, 2^d >= E) ; chaque place porte une fiche contigue : empreinte,
// naissance, rang, population. Une recherche lit le repertoire puis cherche par dichotomie sur (empreinte, population)
// dans le seau : O(k log(E)) meme si toutes les empreintes sont egales (masque nul), sans passer par la CSR du
// catalogue. Construction parallele et deterministe sur le Pool : comptage par blocs de naissances, places denses par
// sommes prefixes, populations lues dans l'ordre des boules, tri par base stable sur les bits du seau (radix.cpp),
// repertoire, tri de chaque seau, fiches ; l'ordre canonique strict est controle a la construction (tower_invariant).
// Les tampons sont gardes d'un ordre a l'autre (croissance seulement) : l'etage construit l'index de chaque ordre dans
// le meme objet. Toute reponse exige l'egalite exacte des SiteIdx.
#include <algorithm>
#include <bit>

#include "tower/internal.hpp"

namespace mhgp12::tower_detail {
namespace {

constexpr u64 kBirthBlock = 4096;  // naissances par bloc de comptage et de remplissage
constexpr u64 kPlaceGrain = 8192;  // places par tranche du repertoire et des fiches
constexpr u64 kBucketGrain = 8192;  // seaux par tranche de tri

bool exact_population(const CatalogueBall& data, u32 k) noexcept { return u64{data.p} + data.m == k; }

// Population triee de la boule b (fusion de I et U, croissants et disjoints) dans out ; rend son cardinal.
u32 merged_population(const Catalogue& catalogue, u32 b, u32* out, u32 room) noexcept {
  const auto inner = catalogue.interior(make_id<BallIdx>(b)), shell = catalogue.shell(make_id<BallIdx>(b));
  std::size_t i = 0, j = 0;
  u32 n = 0;
  while ((i < inner.size() || j < shell.size()) && n < room)
    out[n++] = idx((j == shell.size() || (i < inner.size() && idx(inner[i]) < idx(shell[j]))) ? inner[i++]
                                                                                                : shell[j++]);
  return n + static_cast<u32>(inner.size() - i + shell.size() - j);
}

// Ordre canonique : empreinte, puis population (k mots).
int compare_rows(const u32* a, const u32* b, u32 k) noexcept {
  for (u32 i = 0; i < k; ++i)
    if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
  return 0;
}

template <class T>
Outcome grow(Buffer<T>& buffer, u64 n, MemoryBudget& budget) noexcept {
  return buffer.size() >= n ? Outcome{} : buffer.allocate(n, budget);
}

struct Build {
  const Catalogue& catalogue;
  std::span<const u32> birth_keys;
  u32 k;
  u64 mask;
  std::span<u64> block_start;     // blocs + 1 : premiere place dense du bloc
  std::span<KeyedEntry> entries;  // E entrees (empreinte, naissance, place dense)
  std::span<u32> dense_rows;      // E * k : populations dans l'ordre des naissances

  static Outcome count(void* raw, u64 begin, u64 end, u32) noexcept {
    auto& s = *static_cast<Build*>(raw);
    for (u64 block = begin; block < end; ++block) {
      u64 n = 0;
      const u64 hi = std::min<u64>(s.birth_keys.size(), (block + 1) * kBirthBlock);
      for (u64 i = block * kBirthBlock; i < hi; ++i)
        n += exact_population(s.catalogue.balls_data()[s.birth_keys[i]], s.k) ? 1 : 0;
      s.block_start[block + 1] = n;
    }
    return {};
  }
  static Outcome fill(void* raw, u64 begin, u64 end, u32) noexcept {
    auto& s = *static_cast<Build*>(raw);
    for (u64 block = begin; block < end; ++block) {
      u64 e = s.block_start[block];
      const u64 hi = std::min<u64>(s.birth_keys.size(), (block + 1) * kBirthBlock);
      for (u64 i = block * kBirthBlock; i < hi; ++i) {
        const u32 b = s.birth_keys[i];
        if (!exact_population(s.catalogue.balls_data()[b], s.k)) continue;
        u32* row = s.dense_rows.data() + e * s.k;
        if (merged_population(s.catalogue, b, row, s.k) != s.k) return fail(Reason::tower_invariant, s.k);
        u64 key = 0;
        for (u32 j = 0; j < s.k; ++j) key += site_key(row[j]);
        s.entries[e] = KeyedEntry{key & s.mask, static_cast<u32>(i), static_cast<u32>(e)};
        ++e;
      }
      if (e != s.block_start[block + 1]) return fail(Reason::tower_invariant, s.k);
    }
    return {};
  }
};

}  // namespace

u64 PopulationTable::bytes_for(u64 entries, Order k) noexcept {
  const u32 bits = entries <= 1 ? 0u : static_cast<u32>(std::bit_width(entries - 1));
  const u64 table = entries * (kHeaderWords + u64{k}) * sizeof(u32) + ((u64{1} << bits) + 1) * sizeof(u32);
  const u64 building = 2 * entries * sizeof(KeyedEntry) + entries * u64{k} * sizeof(u32) +
                       ((entries + kBirthBlock - 1) / kBirthBlock + 1) * sizeof(u64) + radix_bytes(entries);
  return table + building;
}

Outcome PopulationTable::build(const Catalogue& catalogue, std::span<const u32> birth_keys,
                               std::span<const LevelRank> birth_ranks, Order k, MemoryBudget& budget,
                               sched::Pool& pool, u64 key_mask) noexcept {
  if (k < 2 || k > kMaxPart || birth_keys.size() > kMaxOrderBirths || birth_ranks.size() != birth_keys.size())
    return fail(Reason::tower_invariant, k);
  k_ = k;
  width_ = kHeaderWords + k;
  key_mask_ = key_mask;
  entries_ = 0;
  const u64 blocks = (birth_keys.size() + kBirthBlock - 1) / kBirthBlock;
  MHGP12_TRY(grow(block_start_, blocks + 1, budget));
  block_start_[0] = 0;
  Build fill{catalogue, birth_keys, k, key_mask, block_start_.span(), {}, {}};
  MHGP12_TRY(pool.parallel_for(blocks, 1, &fill, &Build::count));
  for (u64 b = 0; b < blocks; ++b) block_start_[b + 1] += block_start_[b];
  const u64 entries = block_start_[blocks];
  const u32 bits = entries <= 1 ? 0u : static_cast<u32>(std::bit_width(entries - 1));
  MHGP12_TRY(grow(made_, entries, budget));
  MHGP12_TRY(grow(scratch_, entries, budget));
  MHGP12_TRY(grow(dense_rows_, entries * k, budget));
  MHGP12_TRY(grow(records_, entries * width_, budget));
  MHGP12_TRY(grow(directory_, (u64{1} << bits) + 1, budget));
  fill.entries = made_.span().first(entries);
  fill.dense_rows = dense_rows_.span().first(entries * k);
  MHGP12_TRY(pool.parallel_for(blocks, 1, &fill, &Build::fill));
  shift_ = 64 - bits;
  directory_size_ = (u64{1} << bits) + 1;
  auto sorted = radix_sort(made_.span().first(entries), scratch_.span().first(entries), shift_, places_, budget,
                           pool);
  if (!sorted.ok()) return sorted.outcome();
  MHGP12_TRY(lay_out(sorted.value(), birth_ranks, pool));
  entries_ = entries;
  return {};
}

// Places triees par seau -> repertoire, tri de chaque seau sur (empreinte, population), fiches et controle de l'ordre
// canonique strict : seaux croissants, puis (empreinte, population) strictement croissant (population repetee
// refusee).
Outcome PopulationTable::lay_out(std::span<KeyedEntry> sorted, std::span<const LevelRank> birth_ranks,
                                 sched::Pool& pool) noexcept {
  struct Layout {
    PopulationTable& table;
    std::span<KeyedEntry> sorted;
    std::span<const LevelRank> birth_ranks;
    const u32* row(const KeyedEntry& e) const noexcept { return table.dense_rows_.data() + u64{e.aux} * table.k_; }
    bool less(const KeyedEntry& a, const KeyedEntry& b) const noexcept {
      return a.key != b.key ? a.key < b.key : compare_rows(row(a), row(b), table.k_) < 0;
    }
    static Outcome directory(void* raw, u64 begin, u64 end, u32) noexcept {
      auto& s = *static_cast<Layout*>(raw);
      PopulationTable& t = s.table;
      const u64 n = s.sorted.size(), hi = std::min<u64>(n, end * kPlaceGrain);
      for (u64 pos = begin * kPlaceGrain; pos < hi; ++pos) {
        const u64 bucket = t.bucket_of(s.sorted[pos].key);
        const u64 previous = pos == 0 ? 0 : t.bucket_of(s.sorted[pos - 1].key);
        if (pos > 0 && previous > bucket) return fail(Reason::tower_invariant, t.k_);  // seaux croissants
        for (u64 j = pos == 0 ? 0 : previous + 1; j <= bucket; ++j) t.directory_[j] = static_cast<u32>(pos);
        if (pos + 1 == n)
          for (u64 j = bucket + 1; j < t.directory_size_; ++j) t.directory_[j] = static_cast<u32>(n);
      }
      return {};
    }
    static Outcome buckets(void* raw, u64 begin, u64 end, u32) noexcept {
      auto& s = *static_cast<Layout*>(raw);
      const u64 last = std::min<u64>(s.table.directory_size_ - 1, end * kBucketGrain);
      for (u64 j = begin * kBucketGrain; j < last; ++j) {
        const u64 lo = s.table.directory_[j], hi = s.table.directory_[j + 1];
        if (hi - lo > 1)
          std::sort(s.sorted.begin() + static_cast<std::ptrdiff_t>(lo),
                    s.sorted.begin() + static_cast<std::ptrdiff_t>(hi),
                    [&](const KeyedEntry& a, const KeyedEntry& b) { return s.less(a, b); });
      }
      return {};
    }
    static Outcome records(void* raw, u64 begin, u64 end, u32) noexcept {
      auto& s = *static_cast<Layout*>(raw);
      PopulationTable& t = s.table;
      const u64 hi = std::min<u64>(s.sorted.size(), end * kPlaceGrain);
      for (u64 pos = begin * kPlaceGrain; pos < hi; ++pos) {
        const KeyedEntry& e = s.sorted[pos];
        if (pos > 0 && !s.less(s.sorted[pos - 1], e)) return fail(Reason::tower_invariant, t.k_);  // ordre strict
        u32* r = t.records_.data() + pos * t.width_;
        r[0] = static_cast<u32>(e.key);
        r[1] = static_cast<u32>(e.key >> 32);
        r[2] = e.index;
        r[3] = idx(s.birth_ranks[e.index]);
        std::copy_n(s.row(e), t.k_, r + kHeaderWords);
      }
      return {};
    }
  } layout{*this, sorted, birth_ranks};
  if (sorted.empty()) {
    for (u64 j = 0; j < directory_size_; ++j) directory_[j] = 0;
    return {};
  }
  const u64 places = (sorted.size() + kPlaceGrain - 1) / kPlaceGrain;
  MHGP12_TRY(pool.parallel_for(places, 1, &layout, &Layout::directory));
  MHGP12_TRY(pool.parallel_for((directory_size_ - 1 + kBucketGrain - 1) / kBucketGrain, 1, &layout, &Layout::buckets));
  return pool.parallel_for(places, 1, &layout, &Layout::records);
}

u64 PopulationTable::key_of(const Part& f) const noexcept {
  u64 key = 0;
  for (u32 i = 0; i < f.k; ++i) key += site_key(f.id[i]);
  return key & key_mask_;
}

// Signe de (empreinte, population) de la place pos moins (key, F).
int PopulationTable::compare_at(u64 pos, u64 key, const Part& f) const noexcept {
  const u64 at = key_at(pos);
  if (at != key) return at < key ? -1 : 1;
  return compare_rows(record(pos) + kHeaderWords, f.id.data(), k_);
}

// Premiere place de [lo, hi) au moins egale a (key, F) (F nul : la premiere d'empreinte au moins key).
u64 PopulationTable::lower_bound(u64 lo, u64 hi, u64 key, const Part* f) const noexcept {
  while (lo < hi) {
    const u64 mid = lo + (hi - lo) / 2;
    const u64 at = key_at(mid);
    const bool below = at != key ? at < key : (f != nullptr && compare_at(mid, key, *f) < 0);
    if (below) lo = mid + 1;
    else hi = mid;
  }
  return lo;
}

std::optional<PopulationHit> PopulationTable::find(const Part& f, u64 key) const noexcept {
  if (entries_ == 0 || f.k != k_) return std::nullopt;
  const u64 bucket = bucket_of(key), hi = directory_[bucket + 1];
  const u64 pos = lower_bound(directory_[bucket], hi, key, &f);
  if (pos < hi && compare_at(pos, key, f) == 0) return hit_at(pos);  // egalite exacte des SiteIdx
  return std::nullopt;
}

u32 PopulationTable::candidate(u64 key) const noexcept {
  if (entries_ == 0) return kNone;
  const u64 bucket = bucket_of(key), hi = directory_[bucket + 1];
  const u64 pos = lower_bound(directory_[bucket], hi, key, nullptr);
  return pos < hi && key_at(pos) == key ? static_cast<u32>(pos) : kNone;
}

std::optional<PopulationHit> PopulationTable::verify(u32 candidate, const Part& f) const noexcept {
  if (f.k != k_) return std::nullopt;
  if (compare_rows(record(candidate) + kHeaderWords, f.id.data(), k_) == 0) return hit_at(candidate);
  const u64 key = key_at(candidate), hi = directory_[bucket_of(key) + 1];  // collision d'empreintes : dichotomie
  const u64 pos = lower_bound(u64{candidate} + 1, hi, key, &f);
  if (pos < hi && compare_at(pos, key, f) == 0) return hit_at(pos);  // autre place de meme empreinte
  return std::nullopt;
}

void PopulationTable::prefetch(u32 candidate) const noexcept {
  if (candidate == kNone) return;
  const u32* r = record(candidate);
  __builtin_prefetch(r);
  __builtin_prefetch(r + width_ - 1);
}

void PopulationTable::prefetch_directory(u64 key) const noexcept {
  if (entries_ != 0) __builtin_prefetch(directory_.data() + bucket_of(key));
}

void PopulationTable::prefetch_bucket(u64 key) const noexcept {
  if (entries_ == 0) return;
  const u64 bucket = bucket_of(key);
  const u32 pos = directory_[bucket];
  if (pos >= directory_[bucket + 1]) return;
  const u32* r = record(pos);
  __builtin_prefetch(r);
  __builtin_prefetch(r + width_ - 1);
}

}  // namespace mhgp12::tower_detail
