// Arithmetique etroite de la feuille source unique (levier C, 6 octobre 2026) : etendue de feuille <= 2^20, droites
// J2 en i64, produit mixte q4 en i64, triangle aigu par trois produits scalaires. Chaque forme etroite est confrontee
// a sa reference i128 ou a num/ sur des cas limites (etendue exactement 2^20 posee au sommet du cube 2^B, boite
// fermee jusqu'a hi = 2^B) et sur des tirages ; une feuille d'etendue 2^20 + 1 est non resolue (profils > 20 bits),
// une feuille d'etendue maximale rend les memes compteurs que leaf.cpp.
#include <random>
#include <vector>

#include "catalogue/internal.hpp"
#include "catalogue/leaf_device.hpp"
#include "num/center_region.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::catalogue_detail;

namespace {
namespace ld = mhgp11::leaf_device;
constexpr i64 kTop = i64{1} << kCoordBits;   // hi maximal de la fermeture
constexpr i64 kSpan = ld::kNarrowSpan < kTop ? ld::kNarrowSpan : kTop - 1;  // etendue admise et realisable

int reference_line(const std::array<u32, 3>& a, const std::array<u32, 3>& b, const std::array<u32, 3>& c,
                   const std::array<i64, 3>& lo, const std::array<i64, 3>& hi) {
  const auto region = num::CenterRegion::make(lo, hi);
  const auto pa = num::Point::make(a[0], a[1], a[2]), pb = num::Point::make(b[0], b[1], b[2]);
  const auto pc = num::Point::make(c[0], c[1], c[2]);
  if (!region.ok() || !pa.ok() || !pb.ok() || !pc.ok()) return -1;
  switch (num::center_line_meets(pa.value(), pb.value(), pc.value(), region.value())) {
    case num::CenterLineRelation::degenerate: return ld::kDegenerate;
    case num::CenterLineRelation::disjoint: return ld::kDisjoint;
    default: return ld::kIntersects;
  }
}

// Un point de la fenetre [base, base + span] par axe : coin (0 ou span), ou tirage.
std::array<u32, 3> in_window(std::mt19937_64& rng, i64 base, bool corner, i64 span = kSpan) {
  std::array<u32, 3> p{};
  for (auto& v : p) {
    const i64 offset = corner ? (rng() & 1u ? span : 0) : static_cast<i64>(rng() % static_cast<u64>(span + 1));
    v = static_cast<u32>(base + offset);
  }
  return p;
}

i64 clamp(i64 v, i64 low, i64 high) { return v < low ? low : v > high ? high : v; }

// Centre du cercle circonscrit de (a, b, c), arrondi vers zero (a si les points sont alignes) : en i128, independant
// de la feuille (t = uu v - vv u, n = t x w, d = 2 |w|^2).
std::array<i64, 3> circumcenter(const std::array<u32, 3>& a, const std::array<u32, 3>& b, const std::array<u32, 3>& c) {
  i128 u[3], v[3], t[3], w[3], n[3], uu = 0, vv = 0, g = 0;
  for (int k = 0; k < 3; ++k) {
    u[k] = i128(b[k]) - a[k]; v[k] = i128(c[k]) - a[k];
    uu += u[k] * u[k]; vv += v[k] * v[k];
  }
  for (int k = 0; k < 3; ++k) w[k] = u[(k + 1) % 3] * v[(k + 2) % 3] - u[(k + 2) % 3] * v[(k + 1) % 3];
  for (int k = 0; k < 3; ++k) { t[k] = uu * v[k] - vv * u[k]; g += w[k] * w[k]; }
  for (int k = 0; k < 3; ++k) n[k] = t[(k + 1) % 3] * w[(k + 2) % 3] - t[(k + 2) % 3] * w[(k + 1) % 3];
  std::array<i64, 3> out{};
  for (int k = 0; k < 3; ++k) out[k] = g == 0 ? i64(a[k]) : static_cast<i64>(i128(a[k]) + n[k] / (2 * g));
  return out;
}

// Boite T0 dans la fenetre [base, base + kSpan] (0 <= lo < hi, hi pouvant atteindre 2^B) : la fenetre entiere
// (bornes extremes), ou une boite de demi-cote 2^r (r <= 19) placee pres de la droite des centres de (a, b, c),
// decalee d'au plus deux demi-cotes : les issues disjointe et rencontree s'y partagent pres de la frontiere.
void box_in_window(std::mt19937_64& rng, i64 base, bool extreme, const std::array<i64, 3>& center,
                   std::array<i64, 3>& lo, std::array<i64, 3>& hi) {
  const i64 half = i64{1} << (rng() % 20), top = base + kSpan;
  for (int k = 0; k < 3; ++k) {
    if (extreme) {
      lo[k] = base; hi[k] = top;
      continue;
    }
    const i64 shift = static_cast<i64>(rng() % static_cast<u64>(4 * half + 1)) - 2 * half;
    lo[k] = clamp(center[k] + shift - half, base, top - 1);
    hi[k] = clamp(center[k] + shift + half + 1, lo[k] + 1, top);
  }
}

ld::Vec as_vec(const std::array<u32, 3>& a, const std::array<u32, 3>& b) { return ld::diff(b.data(), a.data()); }
}  // namespace

MHGP11_TEST(line_reference, 200000) {
  std::mt19937_64 rng(20261006);
  int differences = 0;
  std::array<int, 4> outcomes{};
  for (int trial = 0; trial < 200000; ++trial) {
    // Fenetre d'etendue kSpan au sommet du cube (sites jusqu'a 2^B - 1, fermeture jusqu'a hi = 2^B), a l'origine, ou
    // quelconque ; sites et boite dans la fenetre, donc etendue de feuille <= kSpan.
    const int place = trial % 3;
    const i64 base = place == 0 ? kTop - kSpan : place == 1 ? 0 : static_cast<i64>(rng() % (kTop - kSpan));
    const i64 sites = place == 0 ? kSpan - 1 : kSpan;
    const bool corner = trial % 4 == 0, extreme = trial % 5 == 0;
    const auto a = in_window(rng, base, corner, sites), b = in_window(rng, base, corner, sites);
    const auto c = in_window(rng, base, corner, sites);
    std::array<i64, 3> lo{}, hi{};
    box_in_window(rng, base, extreme, circumcenter(a, b, c), lo, hi);
    const int got = ld::center_line_meets(a.data(), b.data(), c.data(), lo.data(), hi.data());
    const int want = reference_line(a, b, c, lo, hi);
    differences += got != want;
    outcomes[want < 0 ? 3 : want] += 1;
    CHECK_EQ(got, want);
  }
  CHECK_EQ(differences, 0);
  // Contre le vert par vacuite : chaque issue du lemme Z est jouee souvent (et aucune reference refusee).
  CHECK(outcomes[ld::kDegenerate] >= 2000 && outcomes[ld::kDisjoint] >= 20000 && outcomes[ld::kIntersects] >= 20000);
  CHECK_EQ(outcomes[3], 0);
}

MHGP11_TEST(acute_and_triple, 120000) {
  std::mt19937_64 rng(4061020);
  for (int trial = 0; trial < 40000; ++trial) {
    const i64 base = trial % 2 == 0 ? kTop - 1 - kSpan : 0;
    const bool corner = trial % 3 == 0;
    const auto a = in_window(rng, base, corner), b = in_window(rng, base, corner), c = in_window(rng, base, corner);
    const auto d = in_window(rng, base, corner);
    // Definition par trois produits scalaires aux trois sommets, contre la forme partagee uu, vv, uv.
    const bool explicit_acute = ld::dot(as_vec(a, b), as_vec(a, c)) > 0 && ld::dot(as_vec(b, a), as_vec(b, c)) > 0 &&
                                ld::dot(as_vec(c, a), as_vec(c, b)) > 0;
    CHECK_EQ(ld::strictly_acute(a.data(), b.data(), c.data()), explicit_acute);
    // Produit mixte : i64 (u x v).sv contre u.(v x sv) en i128, et contre orientation4.
    const ld::Vec u = as_vec(a, b), v = as_vec(a, c), sv = as_vec(a, d);
    const i64 narrow = ld::dot(ld::cross(u, v), sv);
    const ld::Vec vs = ld::cross(v, sv);
    const i128 wide = i128(u.v[0]) * vs.v[0] + i128(u.v[1]) * vs.v[1] + i128(u.v[2]) * vs.v[2];
    CHECK(i128(narrow) == wide);
    CHECK_EQ(ld::sign(narrow), ld::orientation4(a.data(), b.data(), c.data(), d.data()));
  }
}

namespace {
struct LeafRun {
  u32 status = ld::kUnresolved;
  ld::Counts counts;
  Result<CatalogueLedger> reference = fail(Reason::catalogue_invariant);
};

// Feuille de tous les sites : leaf_device::run_leaf (comptage) et leaf.cpp (voie graphe, J2 memorise).
LeafRun run_both(const std::vector<std::array<u32, 3>>& points, const Box& box, MemoryBudget& budget) {
  LeafRun out;
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  std::vector<SiteIdx> sites;
  std::vector<u32> local;
  for (auto p : points) {
    x.push_back(p[0]); y.push_back(p[1]); z.push_back(p[2]);
    ids.push_back(make_id<PointId>(static_cast<u32>(ids.size())));
    sites.push_back(make_id<SiteIdx>(static_cast<u32>(sites.size())));
    local.push_back(static_cast<u32>(local.size()));
  }
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth(), budget);
  if (!cloud.ok()) return out;
  ld::Input in;
  in.x = cloud.value().x().data(); in.y = cloud.value().y().data(); in.z = cloud.value().z().data();
  in.sites = local.data(); in.m = static_cast<u32>(local.size());
  for (int j = 0; j < 3; ++j) { in.lo[j] = box.lo[j]; in.hi[j] = box.hi[j]; }
  in.kmax = 5; in.cache = true;
  ld::CountSink sink;
  out.status = ld::run_leaf(in, out.counts, sink);
  Workspace workspace;
  if (!workspace.allocate(cloud.value().sites(), budget, true, true).ok()) return out;
  CatalogueParams params;
  params.pair_graph = true; params.cache_center_lines = true;
  Collector collector;
  Run run{cloud.value(), params, budget, workspace, collector, {}};
  const Outcome done = enumerate_leaf(run, sites, box);
  out.reference = done.ok() ? Result<CatalogueLedger>(run.ledger) : Result<CatalogueLedger>(done);
  return out;
}

// Onze sites groupes sous le sommet du cube (dont le sommet 2^B - 1, les autres de 2^B - 4097 a 2^B - 1) et un site
// lointain sur la diagonale, a far : l'etendue de la feuille vaut max(2^B - 1, hi) - far. Les triangles du groupe
// gardent les certificats natifs (q3 et orientation) : la feuille device est resolue tant que l'etendue est admise.
std::vector<std::array<u32, 3>> cluster_and_far(i64 far) {
  std::mt19937_64 rng(61020261);
  std::vector<std::array<u32, 3>> out;
  out.push_back({static_cast<u32>(kTop - 1), static_cast<u32>(kTop - 1), static_cast<u32>(kTop - 1)});
  for (int i = 0; i < 10; ++i)
    out.push_back({static_cast<u32>(kTop - 1 - static_cast<i64>(rng() % 4097)),
                   static_cast<u32>(kTop - 1 - static_cast<i64>(rng() % 4097)),
                   static_cast<u32>(kTop - 1 - static_cast<i64>(rng() % 4097))});
  out.push_back({static_cast<u32>(far), static_cast<u32>(far), static_cast<u32>(far)});
  return out;
}

void same_counts(const LeafRun& run) {
  CHECK_EQ(run.status, ld::kOk);
  REQUIRE(run.reference.ok());
  const CatalogueLedger& l = run.reference.value();
  const ld::Counts& c = run.counts;
  CHECK_EQ(c.dominance_tests, l.dominance_tests); CHECK_EQ(c.prefixes, l.prefixes); CHECK_EQ(c.judged, l.judged);
  CHECK_EQ(c.census_tests, l.census_tests); CHECK_EQ(c.emitted, l.emitted); CHECK_EQ(c.incidences, l.incidences);
  CHECK_EQ(c.q4_candidates, l.q4_candidates); CHECK_EQ(c.q4_levels, l.q4_levels);
  CHECK_EQ(c.region_line_tests, l.region_line_tests); CHECK_EQ(c.region_line_rejects, l.region_line_rejects);
  CHECK_EQ(c.region_line_evaluations, l.region_line_evaluations);
  CHECK_EQ(c.region_line_cache_hits, l.region_line_cache_hits);
}
}  // namespace

MHGP11_TEST(span_refusal, 30) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  // Etendue admise maximale (site lointain a 2^B - 1 - kSpan), boite dans le groupe : memes compteurs que leaf.cpp,
  // prefixes, droites J2 et candidats q4 exerces.
  const Box inner{{kTop - 3000, kTop - 3000, kTop - 3000}, {kTop - 1000, kTop - 1000, kTop - 1000}};
  const auto admitted = run_both(cluster_and_far(kTop - 1 - kSpan), inner, budget);
  same_counts(admitted);
  CHECK(admitted.counts.prefixes > 0 && admitted.counts.region_line_evaluations > 0);
  CHECK(admitted.counts.q4_candidates > 0 && admitted.counts.emitted > 0);
  // Etendue exactement kSpan portee par la fermeture : boite jusqu'a hi = 2^B, site lointain a 2^B - kSpan.
  const Box closure{{kTop - 3000, kTop - 3000, kTop - 3000}, {kTop, kTop, kTop}};
  same_counts(run_both(cluster_and_far(kTop - kSpan), closure, budget));
  // Profils > 20 bits : une unite de plus (2^20 + 1), feuille non resolue avant tout prefixe ; leaf.cpp la traite.
  if constexpr (kCoordBits > 20) {
    const auto wide = run_both(cluster_and_far(kTop - 2 - kSpan), inner, budget);
    CHECK_EQ(wide.status, ld::kUnresolved);
    CHECK_EQ(wide.counts.prefixes, 0u);
    CHECK(wide.reference.ok() && wide.reference.value().prefixes > 0);
    // Sites d'etendue kSpan, mais la fermeture hi = 2^B porte l'enveloppe a 2^20 + 1 : non resolue aussi.
    const auto by_closure = run_both(cluster_and_far(kTop - 1 - kSpan), closure, budget);
    CHECK_EQ(by_closure.status, ld::kUnresolved);
  } else {
    CHECK(kSpan == kTop - 1);  // etendue maximale realisable : admise sans test
    CHECK(kTop <= ld::kNarrowSpan);
    CHECK(admitted.reference.ok());
  }
  CHECK(budget.released().ok());
}

MHGP11_TEST_MAIN()
