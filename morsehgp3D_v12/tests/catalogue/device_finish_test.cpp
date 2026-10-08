// Fin d'etage partagee (finish_driver.hpp) sur des enregistrements construits, contre une reference independante
// ecrite ici : niveaux de num::Sphere::through, ordre (num::compare, puis listes triees des positions de S*, la plus
// courte d'abord), rangs denses, CSR, niveaux des rangs, table par std::sort des (S*[0], S*[1], S*[2], S*[3]). Les
// boules n'ont pas a etre critiques : la fin d'etage ne juge que leur ordre. Trois jeux : supports aleatoires d'un petit
// nuage dense (egalites de niveaux en nombre), quasi-egalites de niveaux distincts dans la marge F4 (comparaison
// exacte), et egalites de niveaux de representations differentes dont les cles F3 s'inversent par rapport aux
// positions (triangle rectangle et son hypotenuse : chaine retriee en exact, repli compte).
#include <map>

#include "device_support.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::catalogue_detail;
using namespace mhgp12::device_test;

namespace {

struct Crafted {
  std::vector<std::array<u32, 4>> supports;  // indices de points, kNone au-dela de q
  std::vector<u32> q, p, m;
  std::vector<std::vector<u32>> population;  // indices de points
};

// Tri par insertion des q premieres cases (q <= 4).
void sort_first(std::array<u32, 4>& s, u32 q) {
  for (u32 k = 1; k < q && k < 4; ++k)
    for (u32 j = k; j > 0 && s[j] < s[j - 1]; --j) std::swap(s[j], s[j - 1]);
}

bool distinct_first(const std::array<u32, 4>& s, u32 q) {
  for (u32 k = 1; k < q && k < 4; ++k)
    if (s[k] == s[k - 1]) return false;
  return true;
}

u32 site_of(const Cloud& cloud, const std::array<u32, 3>& p) {
  for (u32 s = 0; s < cloud.sites(); ++s)
    if (cloud.x()[s] == p[0] && cloud.y()[s] == p[1] && cloud.z()[s] == p[2]) return s;
  return kNone;
}

std::optional<num::Level> level_of(const Cloud& cloud, const std::array<u32, 4>& s, u32 q) {
  std::array<num::Point, 4> pt{};
  for (u32 k = 0; k < q; ++k) pt[k] = num::Point::make(cloud.x()[s[k]], cloud.y()[s[k]], cloud.z()[s[k]]).value();
  auto sphere = q == 2 ? num::Sphere::through(pt[0], pt[1])
                : q == 3 ? num::Sphere::through(pt[0], pt[1], pt[2])
                         : num::Sphere::through(pt[0], pt[1], pt[2], pt[3]);
  if (!sphere.ok() || !sphere.value()) return std::nullopt;
  return sphere.value()->level();
}

std::vector<std::array<u32, 3>> positions(const Cloud& cloud, const std::array<u32, 4>& s, u32 q) {
  std::vector<std::array<u32, 3>> out;
  for (u32 k = 0; k < q; ++k) out.push_back({cloud.x()[s[k]], cloud.y()[s[k]], cloud.z()[s[k]]});
  std::sort(out.begin(), out.end());
  return out;
}

// Enregistrements (S* en SiteIdx croissants) et populations a plat ; la reference trie les memes donnees.
struct Records {
  std::vector<BallRecord> records;
  std::vector<SiteIdx> population;
  std::vector<num::Level> levels;
};

Records records_of(const Cloud& cloud, const Crafted& c) {
  Records r;
  for (std::size_t i = 0; i < c.q.size(); ++i) {
    BallRecord b{};
    std::array<u32, 4> s = c.supports[i];
    sort_first(s, c.q[i]);
    for (u32 k = 0; k < 4; ++k) b.support[k] = k < c.q[i] ? s[k] : kNone;
    b.population = r.population.size();
    b.p = static_cast<u8>(c.p[i]);
    b.m = static_cast<u8>(c.m[i]);
    b.qmin = static_cast<u8>(c.q[i]);
    for (const u32 site : c.population[i]) r.population.push_back(make_id<SiteIdx>(site));
    r.records.push_back(b);
    r.levels.push_back(*level_of(cloud, s, c.q[i]));
  }
  return r;
}

// Reference : ordre, rangs, CSR, niveaux des rangs et table, sans aucun code de la fin d'etage.
struct Reference {
  std::vector<u32> order, rank;
  std::vector<u64> offsets;
  std::vector<SiteIdx> values;
  std::vector<num::Level> rank_levels;
  std::vector<u32> table_values;
  std::vector<u64> table_offsets;
};

Reference reference(const Cloud& cloud, const Records& r) {
  Reference out;
  const u32 n = static_cast<u32>(r.records.size());
  for (u32 i = 0; i < n; ++i) out.order.push_back(i);
  auto pos = [&](u32 i) {
    std::array<u32, 4> s{};
    for (u32 k = 0; k < 4; ++k) s[k] = r.records[i].support[k];
    return positions(cloud, s, r.records[i].qmin);
  };
  std::sort(out.order.begin(), out.order.end(), [&](u32 a, u32 b) {
    const int c = num::compare(r.levels[a], r.levels[b]);
    return c != 0 ? c < 0 : pos(a) < pos(b);
  });
  u32 rank = 0;
  out.offsets.push_back(0);
  for (u32 i = 0; i < n; ++i) {
    const u32 b = out.order[i];
    if (i == 0 || num::compare(r.levels[out.order[i - 1]], r.levels[b]) < 0) {
      ++rank;
      out.rank_levels.push_back(r.levels[b]);
    }
    out.rank.push_back(rank);
    const u64 length = u64{r.records[b].p} + r.records[b].m;
    for (u64 j = 0; j < length; ++j) out.values.push_back(r.population[r.records[b].population + j]);
    out.offsets.push_back(out.offsets.back() + length);
  }
  for (u32 i = 0; i < n; ++i) out.table_values.push_back(i);
  std::sort(out.table_values.begin(), out.table_values.end(), [&](u32 a, u32 b) {
    return std::lexicographical_compare(r.records[out.order[a]].support, r.records[out.order[a]].support + 4,
                                        r.records[out.order[b]].support, r.records[out.order[b]].support + 4);
  });
  for (u32 s = 0; s <= cloud.sites(); ++s) {
    u64 at = 0;
    while (at < n && r.records[out.order[out.table_values[at]]].support[0] < s) ++at;
    out.table_offsets.push_back(at);
  }
  return out;
}

// Sorties d'une fin d'etage comparees a la reference, partie par partie.
struct Match {
  bool balls = false, offsets = false, values = false, levels = false, table = false;
  bool all() const { return balls && offsets && values && levels && table; }
};

Match compare_outputs(const Cloud& cloud, const Records& r, const fin::FinishOutput& o) {
  const Reference ref = reference(cloud, r);
  Match m;
  m.balls = o.balls.size() == ref.order.size();
  for (u64 i = 0; m.balls && i < ref.order.size(); ++i) {
    const BallRecord& e = r.records[ref.order[i]];
    const CatalogueBall& g = o.balls[i];
    for (u32 k = 0; k < 4; ++k) m.balls = m.balls && idx(g.support[k]) == e.support[k];
    m.balls = m.balls && idx(g.rank) == ref.rank[i] && g.p == e.p && g.m == e.m && g.qmin == e.qmin;
  }
  m.offsets = std::equal(ref.offsets.begin(), ref.offsets.end(), o.offsets.begin(), o.offsets.end());
  m.values = std::equal(ref.values.begin(), ref.values.end(), o.values.begin(), o.values.end());
  // Niveaux publies : case 0 nulle, puis les rangs 1..L-1, materialises au fil du flux (memes formes non reduites).
  m.levels = o.levels.size() == ref.rank_levels.size() + 1 && num::to_wide(o.levels[0].numerator()).is_zero();
  for (u64 i = 0; m.levels && i < ref.rank_levels.size(); ++i) {
    const num::Level& got = o.levels[i + 1];
    const num::Level& want = ref.rank_levels[i];
    m.levels = num::compare(got, want) == 0;
    m.levels = m.levels && num::to_wide(got.numerator()).words == num::to_wide(want.numerator()).words;
    m.levels = m.levels && num::to_wide(got.denominator()).words == num::to_wide(want.denominator()).words;
  }
  m.table = o.table_values.size() == ref.table_values.size();
  for (u64 i = 0; m.table && i < ref.table_values.size(); ++i) m.table = idx(o.table_values[i]) == ref.table_values[i];
  m.table = m.table && std::equal(ref.table_offsets.begin(), ref.table_offsets.end(), o.table_offsets.begin(),
                                  o.table_offsets.end());
  return m;
}

// Fin d'etage partagee sur l'executeur Pool, comparee a la reference ; rend les chaines retriees (et leurs elements).
u64 check_finish(const Cloud& cloud, const Records& r, u32 threads, u64* elements = nullptr) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = sched::make_pool({threads});
  if (!CHECK(pool.ok())) return 0;
  PoolExecutor b{*pool.value(), budget};
  FrontArray<BallRecord> records;
  FrontArray<SiteIdx> population;
  if (!CHECK(records.ensure(r.records.size(), budget).ok() && population.ensure(r.population.size(), budget).ok()))
    return 0;
  std::copy(r.records.begin(), r.records.end(), records.data());
  std::copy(r.population.begin(), r.population.end(), population.data());
  fin::FinishArrays<PoolExecutor> arrays;
  const fin::FinishInput in{cloud.x().data(), cloud.y().data(), cloud.z().data(), cloud.sites(), records.data(),
                            population.data(), r.records.size(), r.population.size()};
  fin::FinishStats stats;
  fin::FinishOutput o;
  if (!CHECK(fin::finish_stage(b, arrays, in, o, false, budget, *pool.value(), stats).ok())) return 0;
  const Match m = compare_outputs(cloud, r, o);
  CHECK(m.balls);
  CHECK(m.offsets);
  CHECK(m.values);
  CHECK(m.levels);
  CHECK(m.table);
  if (elements != nullptr) *elements = stats.chain_elements;
  return stats.chains_repaired;
}

// Fin d'etage sur le transit simule sous deux limites (tableaux de l'executeur : appareil ; transit, repli et
// sorties : hote) ; entrees dans un budget a part. Issue, pics, limites tenues, budgets rendus a la destruction.
struct FinishTrial {
  bool ok = false, refused = false, same = false, within = false, released = false;
  u64 host_peak = 0, device_peak = 0, chains = 0;
};

FinishTrial finish_trial(const Cloud& cloud, const Records& r, sched::Pool& pool, u64 host_limit, u64 device_limit) {
  FinishTrial t;
  MemoryBudget inputs(MemoryBudget::kUnlimited), host(host_limit), device(device_limit);
  {
    StagedExecutor b{{pool, device}};
    b.host = &host;
    b.slot_size = 4096;
    FrontArray<BallRecord> records;
    FrontArray<SiteIdx> population;
    if (!records.ensure(r.records.size(), inputs).ok() || !population.ensure(r.population.size(), inputs).ok())
      return t;
    std::copy(r.records.begin(), r.records.end(), records.data());
    std::copy(r.population.begin(), r.population.end(), population.data());
    fin::FinishArrays<StagedExecutor> arrays;
    const fin::FinishInput in{cloud.x().data(), cloud.y().data(), cloud.z().data(), cloud.sites(), records.data(),
                              population.data(), r.records.size(), r.population.size()};
    fin::FinishStats stats;
    fin::FinishOutput o;
    const Outcome done = fin::finish_stage(b, arrays, in, o, false, host, pool, stats);
    t.ok = done.ok();
    t.refused = done.reason == Reason::memory_budget;
    t.same = t.ok && compare_outputs(cloud, r, o).all();
    t.chains = stats.chains_repaired;
    t.host_peak = host.peak();
    t.device_peak = device.peak();
    t.within = host.peak() <= host_limit && device.peak() <= device_limit;
  }
  t.released = host.released().ok() && device.released().ok() && inputs.released().ok();
  return t;
}

void add(Crafted& c, std::array<u32, 4> s, u32 q, Mix& mix, u32 sites) {
  c.supports.push_back(s);
  c.q.push_back(q);
  c.p.push_back(mix.below(4));
  c.m.push_back(q + mix.below(3));
  std::vector<u32> pop;
  for (u32 j = 0; j < c.p.back() + c.m.back(); ++j) pop.push_back(mix.below(sites));
  c.population.push_back(pop);
}

}  // namespace

// Supports aleatoires (q = 2, 3, 4) d'un nuage dense de 45 points de [0, 6]^3 : beaucoup de niveaux egaux.
MHGP12_TEST(finish_random, 10) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const Points pts = scatter(45, 6, 11);
  auto cloud = make_cloud(pts, budget);
  REQUIRE(cloud.ok());
  Mix mix{12};
  Crafted c;
  std::map<std::array<u32, 4>, bool> seen;
  while (c.q.size() < 3000) {
    const u32 q = 2 + mix.below(3);
    std::array<u32, 4> s{kNone, kNone, kNone, kNone};
    for (u32 k = 0; k < q; ++k) s[k] = mix.below(cloud.value().sites());
    sort_first(s, q);
    if (!distinct_first(s, q) || seen.count(s)) continue;
    if (!level_of(cloud.value(), s, q)) continue;
    seen[s] = true;
    add(c, s, q, mix, cloud.value().sites());
  }
  const Records r = records_of(cloud.value(), c);
  check_finish(cloud.value(), r, 1);
  check_finish(cloud.value(), r, 3);
}

// Triangle rectangle de cles inversees (finish_ties) : cotes a et b tels que le triangle (O, A, B) et son hypotenuse
// aient le meme niveau exact et que la cle F3 du triangle depasse celle de l'hypotenuse ; a = 0 si aucun.
namespace {
std::pair<u32, u32> inverted_right_triangle() {
  const u32 base = 1u << 19;
  for (u32 i = 1; i < 400; ++i) {
    const u32 a = base + i, b = base + 3 * i + 1;
    const std::array<u32, 3> o{0, 0, 0}, pa{a, 0, 0}, pb{0, b, 0};
    const u32* tri[4] = {o.data(), pa.data(), pb.data(), nullptr};
    const u32* hyp[4] = {pa.data(), pb.data(), nullptr, nullptr};
    fin::LevelWords lt{}, lh{};
    bool fault = false;
    fin::ball_level(tri, 3, lt, fault);
    fin::ball_level(hyp, 2, lh, fault);
    if (!fault && fin::compare_levels(lt, lh) == 0 && fin::level_key_bits(lt) > fin::level_key_bits(lh)) return {a, b};
  }
  return {0, 0};
}
}  // namespace

// Quasi-egalites et egalites de representations differentes ; le triangle rectangle (C a l'origine) a le meme niveau
// que son hypotenuse et le precede par les positions (C est le plus petit point) ; ses cles F3 sont choisies (recherche
// deterministe) pour que la cle du triangle depasse celle de l'hypotenuse : la chaine doit etre retriee en exact.
MHGP12_TEST(finish_ties, 12) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const auto [found_a, found_b] = inverted_right_triangle();
  REQUIRE(found_a != 0);
  const u32 c2 = (1u << 20) + 3;  // quasi-egalite : c^2 / 4 et (c^2 + 1) / 4, ecart relatif sous 2^-40
  const Points pts{{0, 0, 0}, {found_a, 0, 0}, {0, found_b, 0}, {0, 0, 5}, {c2, 0, 5}, {0, 7, 9}, {c2, 8, 9}};
  auto cloud = make_cloud(pts, budget);
  REQUIRE(cloud.ok());
  const Cloud& cl = cloud.value();
  const u32 o = site_of(cl, pts[0]), a = site_of(cl, pts[1]), b = site_of(cl, pts[2]);
  const u32 n0 = site_of(cl, pts[3]), n1 = site_of(cl, pts[4]), n2 = site_of(cl, pts[5]), n3 = site_of(cl, pts[6]);
  Mix mix{3};
  Crafted c;
  add(c, {o, a, b, kNone}, 3, mix, cl.sites());
  add(c, {a, b, kNone, kNone}, 2, mix, cl.sites());
  add(c, {n0, n1, kNone, kNone}, 2, mix, cl.sites());
  add(c, {n2, n3, kNone, kNone}, 2, mix, cl.sites());
  const Records r = records_of(cl, c);
  CHECK(check_finish(cl, r, 1) >= 1);
  CHECK(check_finish(cl, r, 2) >= 1);
}

// Plateaux traversant les tuiles (CST-0233, preuve de l'auditeur : << un plateau traversant plusieurs blocs reste un
// seul rang >>) : toutes les paires a distance 1 d'une grille 14 x 14 x 14 (7 644 boules de niveau 1/4, huit tuiles de
// 1 024), puis les paires a distance racine de 2 d'une rangee (niveau 1/2) ; deux rangs seulement.
MHGP12_TEST(finish_plateau, 8) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  Points grid;
  for (u32 x = 0; x < 14; ++x)
    for (u32 y = 0; y < 14; ++y)
      for (u32 z = 0; z < 14; ++z) grid.push_back({x, y, z});
  auto cloud = make_cloud(grid, budget);
  REQUIRE(cloud.ok());
  const Cloud& cl = cloud.value();
  Mix mix{5};
  Crafted c;
  for (const auto& p : grid)
    for (int a = 0; a < 3; ++a) {
      std::array<u32, 3> q = p;
      if (++q[a] >= 14) continue;
      add(c, {site_of(cl, p), site_of(cl, q), kNone, kNone}, 2, mix, cl.sites());
    }
  for (u32 x = 0; x + 1 < 14; ++x)
    add(c, {site_of(cl, {x, 0, 0}), site_of(cl, {x + 1, 1, 0}), kNone, kNone}, 2, mix, cl.sites());
  REQUIRE(c.q.size() > 7 * fin::kTileItems);
  const Records r = records_of(cl, c);
  CHECK_EQ(check_finish(cl, r, 1), 0u);
  CHECK_EQ(check_finish(cl, r, 4), 0u);
}

// Chaine longue du repli exact (tranche T2-d, rapatriement compact) : 200 translatees de l'hypotenuse du triangle
// rectangle de cles inversees, meme niveau et meme forme, donc memes cles F3 : une seule chaine de voisins incertains
// de plus de 200 elements, que le triangle (cle plus grande, positions plus petites) clot mal ordonne. Les bornes de la
// chaine sont lues par fenetres de kChainWindow cles doublees : la chaine entiere doit etre retriee (le triangle en
// tete), sinon la reverification echoue.
namespace {

constexpr u32 kCopies = 200;

Points long_chain_points(u32 a, u32 b) {
  Points pts{{0, 0, 0}, {a, 0, 0}, {0, b, 0}};
  for (u32 j = 1; j <= kCopies; ++j) {
    pts.push_back({a, 0, 3 * j});
    pts.push_back({0, b, 3 * j});
  }
  return pts;
}

Crafted long_chain(const Cloud& cl, const Points& pts) {
  Mix mix{17};
  Crafted c;
  add(c, {site_of(cl, pts[0]), site_of(cl, pts[1]), site_of(cl, pts[2]), kNone}, 3, mix, cl.sites());
  for (u32 j = 0; j <= kCopies; ++j)
    add(c, {site_of(cl, pts[1 + 2 * j]), site_of(cl, pts[2 + 2 * j]), kNone, kNone}, 2, mix, cl.sites());
  return c;
}

}  // namespace

MHGP12_TEST(finish_long_chain, 8) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const auto [a, b] = inverted_right_triangle();
  REQUIRE(a != 0);
  const Points pts = long_chain_points(a, b);
  auto cloud = make_cloud(pts, budget);
  REQUIRE(cloud.ok());
  const Cloud& cl = cloud.value();
  const Crafted c = long_chain(cl, pts);
  REQUIRE(c.q.size() > 2 * fin::kChainWindow + 2);
  const Records r = records_of(cl, c);
  u64 elements = 0;
  CHECK_EQ(check_finish(cl, r, 1, &elements), 1u);
  CHECK(elements == c.q.size());  // toute la chaine : les 201 paires et le triangle
  CHECK_EQ(check_finish(cl, r, 3, &elements), 1u);
  CHECK(elements == c.q.size());
}

// Repli exact sous budget serre (tranche T2-d, prelecture Codex) : la chaine longue sur le transit simule, tableaux de
// l'executeur comptes dans le budget de l'appareil ; memoire de transit, positions, fenetres de cles, chaines et
// sorties dans celui de l'hote. Pic de chacun sur une fin d'etage reussie (chaine retriee) ; au pic, meme fin d'etage ;
// un octet de moins, et en dessous, refus memory_budget ; limites tenues, budgets rendus a la destruction.
MHGP12_TEST(finish_budget, 16) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const auto [a, b] = inverted_right_triangle();
  REQUIRE(a != 0);
  const Points pts = long_chain_points(a, b);
  auto cloud = make_cloud(pts, budget);
  REQUIRE(cloud.ok());
  const Cloud& cl = cloud.value();
  const Records r = records_of(cl, long_chain(cl, pts));
  auto pool = sched::make_pool({1});
  REQUIRE(pool.ok());
  const u64 none = MemoryBudget::kUnlimited;
  const FinishTrial free = finish_trial(cl, r, *pool.value(), none, none);
  REQUIRE(free.ok && free.same && free.released && free.chains == 1);
  for (int side = 0; side < 2; ++side) {
    const u64 peak = side == 0 ? free.host_peak : free.device_peak;
    REQUIRE(peak > 0);
    const FinishTrial at = side == 0 ? finish_trial(cl, r, *pool.value(), peak, none)
                                     : finish_trial(cl, r, *pool.value(), none, peak);
    CHECK(at.ok && at.same && at.within && at.released);
    for (const u64 limit : {peak - 1, peak / 2, u64{4096}, u64{0}}) {
      const FinishTrial t = side == 0 ? finish_trial(cl, r, *pool.value(), limit, none)
                                      : finish_trial(cl, r, *pool.value(), none, limit);
      if (!CHECK(t.refused && t.within && t.released))
        std::fprintf(stderr, "fin d'etage sous budget : cote %d, limite %llu\n", side,
                     static_cast<unsigned long long>(limit));
    }
  }
}

// Voie par tranches de cles (tranche T1-d) sur les enregistrements construits : un lot de l'hote, memoire de travail
// de toute l'arene divisee par `parts` ; meme reference que la voie complete. Rend l'issue et le nombre de tranches.
namespace {

Outcome sliced_finish(const Cloud& cloud, const Records& r, u64 parts, u64& slices, Match& match) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = sched::make_pool({3});
  if (!pool.ok()) return pool.outcome();
  std::vector<Chunk> chunks(1);
  MHGP12_TRY(chunks[0].records.allocate(r.records.size(), budget));
  MHGP12_TRY(chunks[0].population.allocate(r.population.size(), budget));
  std::copy(r.records.begin(), r.records.end(), chunks[0].records.begin());
  std::copy(r.population.begin(), r.population.end(), chunks[0].population.begin());
  u64 total = 0;
  for (const BallRecord& b : r.records)
    total += fin::kSliceBytesPerBall + fin::kSliceBytesPerIncidence * (u64{b.p} + b.m);
  PoolExecutor b{*pool.value(), budget};
  fin::FinishArrays<PoolExecutor> arrays;
  fin::SliceArrays<PoolExecutor> sa;
  Buffer<u64> ball_at, incidence_at;
  fin::ArenaView view;
  MHGP12_TRY(fin::arena_bases(chunks, ball_at, incidence_at, view, budget));
  const fin::SliceInput in{cloud.x().data(), cloud.y().data(), cloud.z().data(), cloud.sites(), view, &chunks};
  fin::FinishOutput o;
  fin::FinishStats stats;
  MHGP12_TRY(fin::finish_sliced(b, arrays, sa, in, o, total / parts + 1, budget, *pool.value(), stats));
  slices = stats.slices;
  match = compare_outputs(cloud, r, o);
  return {};
}

}  // namespace

// Tranches sur les jeux construits : supports aleatoires (niveaux egaux en nombre), quasi-egalites et egalites de
// representations differentes (chaine retriee), chaine longue de 202 elements. Une tranche ne coupe jamais une chaine
// de voisins incertains : avec une memoire de travail plus petite que la chaine, refus memory_budget (plateau
// incoupable) ; sinon, la meme fin d'etage que la reference, a l'octet.
MHGP12_TEST(finish_sliced, 10) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const auto [a, b] = inverted_right_triangle();
  REQUIRE(a != 0);
  const Points chain_pts = long_chain_points(a, b);
  auto chain_cloud = make_cloud(chain_pts, budget);
  REQUIRE(chain_cloud.ok());
  const Records chain = records_of(chain_cloud.value(), long_chain(chain_cloud.value(), chain_pts));
  const Points dense = scatter(45, 6, 11);
  auto dense_cloud = make_cloud(dense, budget);
  REQUIRE(dense_cloud.ok());
  Mix mix{12};
  Crafted c;
  std::map<std::array<u32, 4>, bool> seen;
  while (c.q.size() < 600) {  // comme finish_random : supports distincts d'un nuage dense, niveaux egaux en nombre
    const u32 q = 2 + mix.below(3);
    std::array<u32, 4> s{kNone, kNone, kNone, kNone};
    for (u32 k = 0; k < q; ++k) s[k] = mix.below(dense_cloud.value().sites());
    sort_first(s, q);
    if (!distinct_first(s, q) || seen.count(s) || !level_of(dense_cloud.value(), s, q)) continue;
    seen[s] = true;
    add(c, s, q, mix, dense_cloud.value().sites());
  }
  const Records random = records_of(dense_cloud.value(), c);
  for (const u64 parts : {u64{1}, u64{2}, u64{7}}) {
    u64 slices = 0;
    Match m;
    CHECK(sliced_finish(chain_cloud.value(), chain, parts, slices, m).ok() == (parts == 1));
    if (parts == 1) CHECK(m.all() && slices == 1);
    const Outcome done = sliced_finish(dense_cloud.value(), random, parts, slices, m);
    if (done.ok()) CHECK(m.all() && slices >= parts);
    else CHECK(done.reason == Reason::memory_budget);  // plateau de niveaux egaux plus lourd qu'une tranche
  }
}
