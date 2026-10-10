// Portes de l'index des naissances et des premieres sondes en masse (tranche T2-c, CONTRAT_TOUR.md, paragraphe 4.3,
// G-L5) : tri par base stable et deterministe ; index contre une reference independante (table ordonnee des
// populations exactes) avec l'empreinte du produit et avec des masques faibles qui provoquent des collisions
// d'empreintes (masque nul : une seule empreinte, un seul seau) ; candidats de la jointure triee egaux a ceux d'une
// sonde par representant ; resolution complete rejouee avec un index a collisions : memes cibles et memes compteurs
// que l'etage. Index de plusieurs ordres construits ensemble (T2-d-B2) egaux a ceux construits seuls.
#include <map>
#include <random>

#include "tower/profile.hpp"
#include "unit_support.hpp"

using namespace tower_test;
using tower_detail::KeyedEntry;
using tower_detail::Part;
using tower_detail::PopulationTable;

namespace {

std::vector<Xyz> random_cloud(u32 sites, u64 seed, u32 bits) {
  std::mt19937_64 rng(seed);
  std::vector<Xyz> pts;
  for (u32 i = 0; i < sites; ++i)
    pts.push_back({static_cast<u32>(rng() >> (64 - bits)), static_cast<u32>(rng() >> (64 - bits)),
                   static_cast<u32>(rng() >> (64 - bits))});
  std::sort(pts.begin(), pts.end());
  pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
  return pts;
}

// Trace stricte I u A du representant r de la cellule c (meme construction que la passe de l'etage).
Part trace_of(const Catalogue& cat, const ResolvedOrder& o, u64 c, u64 r) {
  const auto inner = cat.interior(o.cell_balls()[c]), shell = cat.shell(o.cell_balls()[c]);
  std::vector<u32> ids;
  for (const SiteIdx s : inner) ids.push_back(idx(s));
  for (u64 rest = o.trace_masks()[r]; rest != 0; rest &= rest - 1) ids.push_back(idx(shell[__builtin_ctzll(rest)]));
  std::sort(ids.begin(), ids.end());
  Part f;
  for (const u32 s : ids) f.id[f.k++] = s;
  return f;
}

// Reference : population triee -> naissance, pour les naissances de population exacte k.
std::map<std::vector<u32>, u32> reference(const Catalogue& cat, const ResolvedOrder& o, Order k) {
  std::map<std::vector<u32>, u32> out;
  for (u32 i = 0; i < o.births(); ++i) {
    const BallIdx b = make_id<BallIdx>(o.birth_keys()[i]);
    const auto& data = cat.balls_data()[idx(b)];
    if (data.p + data.m != k) continue;
    std::vector<u32> row;
    for (const SiteIdx s : cat.interior(b)) row.push_back(idx(s));
    for (const SiteIdx s : cat.shell(b)) row.push_back(idx(s));
    std::sort(row.begin(), row.end());
    out.emplace(row, i);
  }
  return out;
}

std::optional<u32> expected(const std::map<std::vector<u32>, u32>& ref, const Part& f) {
  const auto at = ref.find(std::vector<u32>(f.id.begin(), f.id.begin() + f.k));
  return at == ref.end() ? std::nullopt : std::optional<u32>(at->second);
}

}  // namespace

// Tri par base : stable (a chiffres egaux, ordre d'entree), egal a std::stable_sort sur key >> low_bit, memes octets a
// 1 et 8 fils ; low_bit = 64 : rien ne bouge.
MHGP12_TEST(radix_stable, 12) {
  for (const u32 low : {0u, 40u, 61u, 64u}) {
    std::mt19937_64 rng(low + 7);
    std::vector<KeyedEntry> input(70000);
    for (u32 i = 0; i < input.size(); ++i) input[i] = KeyedEntry{rng() & 0xF0F0F0F0FFFF00FFull, i, 0};
    std::vector<KeyedEntry> want = input;
    std::stable_sort(want.begin(), want.end(), [&](const KeyedEntry& a, const KeyedEntry& b) {
      return (low >= 64 ? 0 : a.key >> low) < (low >= 64 ? 0 : b.key >> low);
    });
    std::vector<std::vector<KeyedEntry>> got;
    for (const u32 threads : {1u, 8u}) {
      auto pool = sched::make_pool({threads});
      REQUIRE(pool.ok());
      MemoryBudget budget(MemoryBudget::kUnlimited);
      std::vector<KeyedEntry> data = input, scratch(input.size());
      Buffer<u64> places;
      auto sorted = tower_detail::radix_sort(data, scratch, low, places, budget, *pool.value());
      REQUIRE(sorted.ok());
      got.emplace_back(sorted.value().begin(), sorted.value().end());
    }
    auto same = [](const std::vector<KeyedEntry>& a, const std::vector<KeyedEntry>& b) {
      return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                        [](const KeyedEntry& x, const KeyedEntry& y) { return x.key == y.key && x.index == y.index; });
    };
    CHECK(same(got[0], want));
    CHECK(same(got[1], want));
    CHECK(same(got[0], got[1]));
  }
}

// Index contre la reference, par ordre, avec l'empreinte du produit et deux masques faibles (0 : toutes les
// empreintes egales ; 0x3 : quatre empreintes) : chaque population exacte est retrouvee ; pour chaque representant,
// find, le candidat de la jointure triee et sa verification rendent la naissance de la reference ou rien.
MHGP12_TEST(index_reference, 60) {
  auto c = build(random_cloud(420, 20261007, 12), 5, 3);
  REQUIRE(c->outcome.ok());
  const Catalogue& cat = *c->catalogue;
  const auto domain = c->domain();
  u64 hits = 0, misses = 0;
  for (Order k = 2; k <= c->resolution->orders(); ++k) {
    const ResolvedOrder& o = c->resolution->order(k);
    const auto ref = reference(cat, o, k);
    for (const u64 mask : {~u64{0}, u64{0}, u64{3}}) {
      PopulationTable table;
      REQUIRE(table.build(cat, o.birth_keys(), o.birth_ranks(), k, c->budget, *c->pool, mask).ok());
      CHECK_EQ(table.entries(), ref.size());
      bool all_found = true;
      for (const auto& [row, birth] : ref) {
        Part f;
        for (const u32 s : row) f.id[f.k++] = s;
        const auto hit = table.find(f);
        all_found = all_found && hit && hit->birth == birth && hit->rank == o.birth_ranks()[birth];
      }
      CHECK(all_found);
      tower_detail::JoinBuffers join;
      REQUIRE(tower_detail::first_probe_candidates(domain, o, table, join, c->budget, *c->pool).ok());
      const auto candidates = join.candidates.span().first(o.representatives());
      bool agree = true;
      for (u64 cell = 0; cell < o.cells(); ++cell)
        for (u64 r = o.cell_offsets()[cell]; r < o.cell_offsets()[cell + 1]; ++r) {
          const Part f = trace_of(cat, o, cell, r);
          const auto want = expected(ref, f);
          const auto found = table.find(f);
          agree = agree && candidates[r] == table.candidate(table.key_of(f));
          agree = agree && found.has_value() == want.has_value() && (!want || found->birth == *want);
          const auto checked = candidates[r] == kNone ? std::nullopt : table.verify(candidates[r], f);
          agree = agree && checked.has_value() == want.has_value() && (!want || checked->birth == *want);
          (want ? hits : misses) += 1;
        }
      CHECK(agree);
    }
  }
  CHECK(hits > 1000);
  CHECK(misses > 100);
}

// Resolution complete rejouee avec un index a collisions (masque nul : une seule empreinte, un seul seau), par les deux
// voies de la premiere sonde : candidats de la jointure triee verifies (G-L5) et recherche exacte dans l'index (G-L7,
// voie produit) ; memes cibles et memes compteurs du travail que l'etage (empreinte du produit), ordre par ordre.
MHGP12_TEST(weak_key_resolution, 40) {
  auto c = build(random_cloud(600, 7, 14), 5, 2);
  REQUIRE(c->outcome.ok());
  const Catalogue& cat = *c->catalogue;
  const auto domain = c->domain();
  auto workspace = CensusWorkspace::make(*c->index, c->budget);
  REQUIRE(workspace.ok());
  for (Order k = 2; k <= c->resolution->orders(); ++k) {
    const ResolvedOrder& o = c->resolution->order(k);
    PopulationTable table;
    REQUIRE(table.build(cat, o.birth_keys(), o.birth_ranks(), k, c->budget, *c->pool, 0).ok());
    tower_detail::JoinBuffers join;
    REQUIRE(tower_detail::first_probe_candidates(domain, o, table, join, c->budget, *c->pool).ok());
    const auto candidates = join.candidates.span().first(o.representatives());
    const tower_detail::ResolveContext context{domain, *c->resolution, tower_detail::OrderView{k, o.birth_keys(), &table}};
    for (const bool joined : {true, false}) {
      OrderCounters counters;
      tower_detail::SectionClock clock(nullptr);
      bool same = true;
      for (u64 cell = 0; cell < o.cells(); ++cell)
        for (u64 r = o.cell_offsets()[cell]; r < o.cell_offsets()[cell + 1]; ++r) {
          const Part f = trace_of(cat, o, cell, r);
          tower_detail::FirstProbe first;
          first.done = true;
          if (!joined) first.hit = table.find(f);
          else if (candidates[r] != kNone) first.hit = table.verify(candidates[r], f);
          auto target = tower_detail::resolve_part(context, f, o.cell_ranks()[cell], *workspace.value(), counters,
                                                   clock, first);
          same = same && target.ok() && target.value() == o.targets()[r];
        }
      CHECK(same);
      const OrderCounters& want = o.counters();
      counters.births = want.births;
      counters.cells = want.cells;
      counters.inert_cells = want.inert_cells;
      counters.extended_cells = want.extended_cells;
      counters.representatives = want.representatives;
      CHECK(counters == want);
      CHECK(want.first_probe_hits > 0);
      CHECK(want.probe_hits_after_steps > 0);
      CHECK(want.cell_stops > 0);
    }
  }
}

// Table S* -> boule du catalogue (LEM-T1) contre une table ordonnee independante : chaque S* est retrouve ; des supports
// voisins (dernier site remplace) sont retrouves ou absents comme dans la reference.
MHGP12_TEST(support_table, 3) {
  auto c = build(random_cloud(420, 20261008, 12), 5, 2);
  REQUIRE(c->outcome.ok());
  const Catalogue& cat = *c->catalogue;
  std::map<std::array<u32, 4>, u32> ref;
  for (u32 b = 0; b < cat.balls(); ++b) {
    std::array<u32, 4> key{kNone, kNone, kNone, kNone};
    for (u32 i = 0; i < cat.balls_data()[b].qmin; ++i) key[i] = idx(cat.balls_data()[b].support[i]);
    ref.emplace(key, b);
  }
  bool all = true, neighbours = true;
  u64 present = 0, absent = 0;
  for (const auto& [key, b] : ref) {
    const u32 q = cat.balls_data()[b].qmin;
    std::array<SiteIdx, 4> probe{};
    for (u32 i = 0; i < q; ++i) probe[i] = make_id<SiteIdx>(key[i]);
    all = all && cat.find_support(std::span<const SiteIdx>(probe.data(), q)) == std::optional<BallIdx>(make_id<BallIdx>(b));
    for (u32 delta = 1; delta <= 3; ++delta) {  // dernier site remplace : support voisin, au catalogue ou non
      std::array<u32, 4> other = key;
      other[q - 1] = key[q - 1] + delta;
      if (other[q - 1] >= c->cloud().sites()) continue;
      for (u32 i = 0; i < q; ++i) probe[i] = make_id<SiteIdx>(other[i]);
      const auto want = ref.find(other);
      const auto got = cat.find_support(std::span<const SiteIdx>(probe.data(), q));
      neighbours = neighbours && (want == ref.end() ? !got.has_value() : got == std::optional<BallIdx>(make_id<BallIdx>(want->second)));
      (want == ref.end() ? absent : present) += 1;
    }
  }
  CHECK(all);
  CHECK(neighbours);
  CHECK(absent > 1000 && present > 10);
}

// Population repetee : deux naissances de meme population ne peuvent exister (unicite de la plus petite boule) ; une
// liste de naissances qui repete une boule est refusee (tower_invariant), jamais masquee.
MHGP12_TEST(duplicate_population, 3) {
  auto c = build(random_cloud(300, 11, 12), 4, 2);
  REQUIRE(c->outcome.ok());
  const ResolvedOrder& o = c->resolution->order(3);
  u32 chosen = kNone;
  for (u32 i = 0; i < o.births() && chosen == kNone; ++i) {
    const auto& data = c->catalogue->balls_data()[o.birth_keys()[i]];
    if (data.p + data.m == 3) chosen = i;
  }
  REQUIRE(chosen != kNone);
  const std::vector<u32> keys{o.birth_keys()[chosen], o.birth_keys()[chosen]};
  const std::vector<LevelRank> ranks{o.birth_ranks()[chosen], o.birth_ranks()[chosen]};
  PopulationTable table;
  const Outcome refused = table.build(*c->catalogue, keys, ranks, 3, c->budget, *c->pool);
  CHECK_EQ(refused.reason, Reason::tower_invariant);
  PopulationTable single;
  CHECK(single.build(*c->catalogue, std::span<const u32>(keys.data(), 1), std::span<const LevelRank>(ranks.data(), 1), 3,
                     c->budget, *c->pool).ok());
  CHECK_EQ(single.entries(), 1u);
}

// Index de plusieurs ordres construits ENSEMBLE (T2-d-B2, Session recouverte : PopulationTable::build_all, une
// invocation du Pool par phase pour toutes les tables) : chaque table egale celle de build seul (memes entrees, meme
// reponse de find et meme PLACE du candidat pour chaque trace et chaque population exacte, donc memes fiches et meme
// ordre), avec l'empreinte du produit et un masque nul (une seule empreinte), a 1, 2 et 8 fils. Refus : celui de la
// construction ordre par ordre, la premiere table en echec dans l'ordre des sources (population repetee aux ordres 3
// et 4 : tower_invariant de l'ordre 3 ; a l'ordre 4 seul : de l'ordre 4 ; source invalide apres une table en echec :
// la table en echec).
namespace {
// Construction ordre par ordre (reference du refus) : issue de la premiere table en echec, comme build_tables avant.
Outcome one_by_one(std::span<PopulationTable* const> tables, std::span<const tower_detail::PopulationSource> sources,
                   const Catalogue& cat, MemoryBudget& budget, sched::Pool& pool) {
  for (std::size_t j = 0; j < tables.size(); ++j)
    MHGP12_TRY(tables[j]->build(cat, sources[j].birth_keys, sources[j].birth_ranks, sources[j].k, budget, pool));
  return {};
}
}  // namespace

MHGP12_TEST(build_all, 70) {
  u64 queries = 0;
  for (const u32 threads : {1u, 2u, 8u}) {
    auto c = build(random_cloud(420, 20261007, 12), 5, threads);
    REQUIRE(c->outcome.ok());
    const Catalogue& cat = *c->catalogue;
    const Order orders = c->resolution->orders();
    for (const u64 mask : {~u64{0}, u64{0}}) {
      std::array<PopulationTable, 12> alone, together;
      std::array<PopulationTable*, 12> ptrs{};
      std::array<tower_detail::PopulationSource, 12> sources{};
      const u32 n = orders - 1;
      for (u32 i = 0; i < n; ++i) {
        const Order k = static_cast<Order>(i + 2);
        const ResolvedOrder& o = c->resolution->order(k);
        REQUIRE(alone[i].build(cat, o.birth_keys(), o.birth_ranks(), k, c->budget, *c->pool, mask).ok());
        ptrs[i] = &together[i];
        sources[i] = tower_detail::PopulationSource{o.birth_keys(), o.birth_ranks(), k};
      }
      REQUIRE(PopulationTable::build_all(std::span<PopulationTable* const>(ptrs.data(), n),
                                         std::span<const tower_detail::PopulationSource>(sources.data(), n), cat,
                                         c->budget, *c->pool, mask).ok());
      bool same = true;
      for (u32 i = 0; i < n; ++i) {
        const Order k = static_cast<Order>(i + 2);
        const ResolvedOrder& o = c->resolution->order(k);
        same = same && together[i].entries() == alone[i].entries() && together[i].shift() == alone[i].shift();
        for (const auto& [row, birth] : reference(cat, o, k)) {
          Part f;
          for (const u32 s : row) f.id[f.k++] = s;
          const auto a = alone[i].find(f), b = together[i].find(f);
          same = same && a && b && a->birth == b->birth && a->rank == b->rank && a->birth == birth;
          same = same && alone[i].candidate(alone[i].key_of(f)) == together[i].candidate(together[i].key_of(f));
          ++queries;
        }
        for (u64 cell = 0; cell < o.cells(); ++cell)
          for (u64 r = o.cell_offsets()[cell]; r < o.cell_offsets()[cell + 1]; ++r) {
            const Part f = trace_of(cat, o, cell, r);
            const auto a = alone[i].find(f), b = together[i].find(f);
            same = same && a.has_value() == b.has_value() && (!a || (a->birth == b->birth && a->rank == b->rank));
            same = same && alone[i].candidate(alone[i].key_of(f)) == together[i].candidate(together[i].key_of(f));
            ++queries;
          }
      }
      CHECK(same);
    }
    // Tables a une entree et vide parmi d'autres (tri par base inactif : moins de deux entrees) : memes tables que
    // seules (defaut trouve par radix_stable et duplicate_population le 8 octobre : un tableau inactif publie vide).
    {
      const ResolvedOrder& o3 = c->resolution->order(3);
      u32 one = kNone;
      for (u32 i = 0; i < o3.births() && one == kNone; ++i)
        if (cat.balls_data()[o3.birth_keys()[i]].p + cat.balls_data()[o3.birth_keys()[i]].m == 3) one = i;
      REQUIRE(one != kNone);
      const ResolvedOrder& o4 = c->resolution->order(4);
      const std::array<tower_detail::PopulationSource, 3> src{
          tower_detail::PopulationSource{o3.birth_keys().subspan(one, 1), o3.birth_ranks().subspan(one, 1), 3},
          tower_detail::PopulationSource{o4.birth_keys(), o4.birth_ranks(), 4},
          tower_detail::PopulationSource{{}, {}, 5}};
      std::array<PopulationTable, 3> alone, together;
      std::array<PopulationTable*, 3> ptrs{&together[0], &together[1], &together[2]};
      for (u32 i = 0; i < 3; ++i)
        REQUIRE(alone[i].build(cat, src[i].birth_keys, src[i].birth_ranks, src[i].k, c->budget, *c->pool).ok());
      REQUIRE(PopulationTable::build_all(ptrs, src, cat, c->budget, *c->pool).ok());
      CHECK_EQ(together[0].entries(), 1u);
      CHECK_EQ(together[2].entries(), 0u);
      bool small = true;
      for (u32 i = 0; i < 3; ++i) small = small && together[i].entries() == alone[i].entries();
      for (const auto& [row, birth] : reference(cat, o4, 4)) {
        Part f;
        for (const u32 s : row) f.id[f.k++] = s;
        small = small && alone[1].candidate(alone[1].key_of(f)) == together[1].candidate(together[1].key_of(f));
      }
      CHECK(small);
    }
    // Refus : populations repetees (meme naissance deux fois) aux ordres 3 et 4, puis a l'ordre 4 seul.
    std::array<std::vector<u32>, 12> keys;
    std::array<std::vector<LevelRank>, 12> ranks;
    for (Order k = 2; k <= orders; ++k) {
      const ResolvedOrder& o = c->resolution->order(k);
      keys[k].assign(o.birth_keys().begin(), o.birth_keys().end());
      ranks[k].assign(o.birth_ranks().begin(), o.birth_ranks().end());
    }
    auto repeat = [&](Order k) {
      for (u32 i = 0; i < keys[k].size(); ++i)
        if (cat.balls_data()[keys[k][i]].p + cat.balls_data()[keys[k][i]].m == k) {
          keys[k].push_back(keys[k][i]);
          ranks[k].push_back(ranks[k][i]);
          return true;
        }
      return false;
    };
    REQUIRE(orders >= 4 && repeat(4));
    for (const bool also_three : {false, true}) {
      if (also_three) REQUIRE(repeat(3));
      std::array<PopulationTable, 12> a, b;
      std::array<PopulationTable*, 12> pa{}, pb{};
      std::array<tower_detail::PopulationSource, 12> src{};
      const u32 n = orders - 1;
      for (u32 i = 0; i < n; ++i) {
        pa[i] = &a[i];
        pb[i] = &b[i];
        src[i] = tower_detail::PopulationSource{keys[i + 2], ranks[i + 2], static_cast<Order>(i + 2)};
      }
      const Outcome want = one_by_one(std::span<PopulationTable* const>(pa.data(), n),
                                      std::span<const tower_detail::PopulationSource>(src.data(), n), cat, c->budget,
                                      *c->pool);
      const Outcome got = PopulationTable::build_all(std::span<PopulationTable* const>(pb.data(), n),
                                                     std::span<const tower_detail::PopulationSource>(src.data(), n),
                                                     cat, c->budget, *c->pool);
      CHECK_EQ(want.reason, Reason::tower_invariant);
      CHECK_EQ(u32{want.order}, also_three ? 3u : 4u);
      CHECK(got.reason == want.reason && got.order == want.order);
      if (also_three) {  // source invalide (k hors domaine) apres la table en echec : l'issue reste celle de l'ordre 3
        src[n - 1].k = 13;
        const Outcome bad = PopulationTable::build_all(std::span<PopulationTable* const>(pb.data(), n),
                                                       std::span<const tower_detail::PopulationSource>(src.data(), n),
                                                       cat, c->budget, *c->pool);
        CHECK(bad.reason == Reason::tower_invariant && bad.order == 3);
      }
    }
  }
  CHECK(queries > 10000);
}

MHGP12_TEST_MAIN()
