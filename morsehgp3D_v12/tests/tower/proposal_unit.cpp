// Portes de la proposition de l'etage G (T2-d) : voie entiere exacte (src/tower/proposal.hpp, exact_small_support) et
// DWelzl amorce par la paire la plus eloignee. La proposition ne decide rien (LEM-T1 ou certificat exact derriere) :
// ces portes jugent qu'elle est JUSTE la ou elle conclut, contre le certificat exact de la tour (certify_part :
// certificat barycentrique, partie dans la boule, canonisation par positions parmi les sites de F sur la sphere) et le
// repli exact (exact_support, bounded_meb de la v11). Parties gravees (paire, triangles aigu, droit et obtus, sites
// alignes, rectangle a deux diagonales dans les deux ordres, carre, tetraedre) et parties tirees de 2 a 12 sites ;
// integration dans le resolveur (routes) : un triangle aigu resolu par resolve_part prend LEM-T1 sans aucun repli.
#include <cstdio>

#include "tower/proposal.hpp"
#include "unit_support.hpp"

using namespace tower_test;
using tower_detail::Part;

namespace {

struct Totals {
  u64 parts = 0, exact = 0, pairs = 0, triangles = 0, floating = 0, seeded_certified = 0;
};

// Boule de la cible d'une partie de l'ordre k (naissance ou cellule), comme decode de unit.cpp.
u32 target_ball(const Case& c, Order k, u32 target) {
  const ResolvedOrder& o = c.resolution->order(k);
  return target_is_cell(target) ? idx(o.cell_balls()[target_index(target)]) : o.birth_keys()[target];
}

Part part_of(const std::vector<u32>& ids) {
  Part f;
  for (u32 s : ids) f.id[f.k++] = s;
  return f;
}

// Juge une partie (SiteIdx croissants) : voie entiere, puis DWelzl amorce quand elle ne conclut pas.
void judge(const Case& c, const Part& f, Totals& t) {
  const auto d = c.domain();
  i64 local[tower_detail::kMaxPart][3];
  for (u32 i = 0; i < f.k; ++i)
    for (int a = 0; a < 3; ++a)
      local[i][a] = i64{d.points[f.id[i]].coordinates()[a]} - i64{d.points[f.id[0]].coordinates()[a]};
  int support[3] = {-1, -1, -1};
  const auto position = [&](int i) noexcept { return d.points[f.id[i]].coordinates(); };
  const int q = tower_detail::exact_small_support(local, static_cast<int>(f.k), position, support);
  ++t.parts;
  std::array<u32, 4> exact_s{};
  auto exact_q = tower_detail::exact_support(d, f, exact_s);
  REQUIRE(exact_q.ok());
  if (q > 0) {
    std::array<u32, 4> s{kNone, kNone, kNone, kNone};
    for (int i = 0; i < q; ++i) s[i] = f.id[support[i]];
    std::sort(s.begin(), s.begin() + q);
    auto cert = tower_detail::certify_part(d, f, std::span<const u32>(s.data(), static_cast<std::size_t>(q)));
    REQUIRE(cert.ok());
    // Support certifie, partie dans la boule, et c'est le support canonique parmi les sites de F sur la sphere.
    CHECK(cert.value().has_value());
    if (cert.value()) {
      CHECK_EQ(u32{cert.value()->arity}, static_cast<u32>(q));
      CHECK(std::equal(s.begin(), s.begin() + q, cert.value()->support.begin()));
    }
    ++t.exact;
    ++(q == 2 ? t.pairs : t.triangles);
    return;
  }
  CHECK(f.k >= 4);                // la voie entiere conclut toujours a deux et trois sites
  CHECK(exact_q.value() >= 3);    // a quatre sites ou plus, elle ne renonce que si la boule n'est pas diametrale
  CHECK(support[0] >= 0 && support[1] > support[0]);  // paire la plus eloignee rendue pour amorcer DWelzl
  tower_detail::DWelzl w;
  for (u32 i = 0; i < f.k; ++i)
    for (int a = 0; a < 3; ++a) w.p[i][a] = static_cast<double>(local[i][a]);
  const tower_detail::DBall ball = w.run(static_cast<int>(f.k), support[0], support[1]);
  ++t.floating;
  if (!w.ok || ball.nr < 2 || ball.nr > 4) return;
  std::array<u32, 4> s{kNone, kNone, kNone, kNone};
  for (int i = 0; i < ball.nr; ++i) s[i] = f.id[ball.R[i]];
  std::sort(s.begin(), s.begin() + ball.nr);
  auto cert = tower_detail::certify_part(d, f, std::span<const u32>(s.data(), static_cast<std::size_t>(ball.nr)));
  REQUIRE(cert.ok());
  if (cert.value()) ++t.seeded_certified;
}

}  // namespace

// Parties gravees : chaque branche de la voie entiere et ses departages.
MHGP12_TEST(witnesses, 60) {
  // Rectangle (diagonales egales, toutes deux diametrales du meme cercle), triangles, alignes, carre et tetraedre,
  // loin les uns des autres pour que chaque partie choisie soit exactement celle qu'on grave.
  const Xyz a{0, 0, 0}, b{4, 0, 0}, cc{4, 2, 0}, dd{0, 2, 0};                  // rectangle
  const Xyz r0{100, 100, 0}, r1{103, 100, 0}, r2{100, 104, 0};               // triangle droit en r0
  const Xyz o0{200, 0, 0}, o1{210, 0, 0}, o2{203, 2, 0};                     // obtus en o2
  const Xyz e0{300, 0, 0}, e1{304, 0, 0}, e2{302, 3, 0};                     // aigu
  const Xyz l0{400, 0, 0}, l1{401, 0, 0}, l2{403, 0, 0};                     // alignes
  const Xyz t0{500, 0, 0}, t1{502, 2, 0}, t2{502, 0, 2}, t3{500, 2, 2};      // tetraedre regulier
  const Xyz s0{600, 0, 0}, s1{602, 0, 0}, s2{602, 2, 0}, s3{600, 2, 0};      // carre
  auto c = build({a, b, cc, dd, r0, r1, r2, o0, o1, o2, e0, e1, e2, l0, l1, l2, t0, t1, t2, t3, s0, s1, s2, s3}, 2);
  REQUIRE(c->outcome.ok());
  Totals t;
  for (const auto& sites : std::vector<std::vector<Xyz>>{
           {a, cc}, {r0, r1, r2}, {o0, o1, o2}, {e0, e1, e2}, {l0, l1, l2}, {a, b, cc, dd}, {t0, t1, t2, t3},
           {s0, s1, s2, s3}, {a, b, cc}, {r1, r2}, {e0, e1, e2, o2}})
    judge(*c, c->part(sites), t);
  // Rectangle dans l'ordre ou la premiere paire la plus eloignee n'est pas S* : le departage par positions la change.
  {
    const auto d = c->domain();
    const std::vector<u32> order{c->site(b), c->site(dd), c->site(a), c->site(cc)};  // BD avant AC
    i64 local[tower_detail::kMaxPart][3];
    for (u32 i = 0; i < 4; ++i)
      for (int x = 0; x < 3; ++x)
        local[i][x] = i64{d.points[order[i]].coordinates()[x]} - i64{d.points[order[0]].coordinates()[x]};
    int support[3] = {-1, -1, -1};
    const auto position = [&](int i) noexcept { return d.points[order[i]].coordinates(); };
    CHECK_EQ(tower_detail::exact_small_support(local, 4, position, support), 2);
    CHECK(std::min(order[support[0]], order[support[1]]) == std::min(c->site(a), c->site(cc)));
    CHECK(std::max(order[support[0]], order[support[1]]) == std::max(c->site(a), c->site(cc)));
  }
  std::printf("proposal witnesses parts=%llu exact=%llu pairs=%llu triangles=%llu floating=%llu\n",
              static_cast<unsigned long long>(t.parts), static_cast<unsigned long long>(t.exact),
              static_cast<unsigned long long>(t.pairs), static_cast<unsigned long long>(t.triangles),
              static_cast<unsigned long long>(t.floating));
  CHECK(t.pairs >= 5 && t.triangles >= 1 && t.floating >= 1);
}

// Parties tirees de 2 a 12 sites dans un nuage pseudo-aleatoire (generateur fixe) : voie entiere juste partout ou elle
// conclut ; DWelzl amorce certifie sur toutes les autres (position generale).
MHGP12_TEST(random, 3000) {
  std::vector<Xyz> pts;
  u64 state = 20261008;
  auto next = [&]() {
    state = state * 6364136223846793005ull + 1442695040888963407ull;
    return static_cast<u32>(state >> 33);
  };
  while (pts.size() < 48) {
    const Xyz p{next() % 4096, next() % 4096, next() % 4096};
    if (std::find(pts.begin(), pts.end(), p) == pts.end()) pts.push_back(p);
  }
  auto c = build(pts, 2);
  REQUIRE(c->outcome.ok());
  Totals t;
  for (int trial = 0; trial < 2000; ++trial) {
    const u32 k = 2 + next() % 11;
    std::vector<u32> ids;
    while (ids.size() < k) {
      const u32 s = next() % c->cloud().sites();
      if (std::find(ids.begin(), ids.end(), s) == ids.end()) ids.push_back(s);
    }
    std::sort(ids.begin(), ids.end());
    judge(*c, part_of(ids), t);
  }
  std::printf("proposal random parts=%llu exact=%llu pairs=%llu triangles=%llu floating=%llu certified=%llu\n",
              static_cast<unsigned long long>(t.parts), static_cast<unsigned long long>(t.exact),
              static_cast<unsigned long long>(t.pairs), static_cast<unsigned long long>(t.triangles),
              static_cast<unsigned long long>(t.floating), static_cast<unsigned long long>(t.seeded_certified));
  CHECK(t.exact >= 300 && t.floating >= 300);
  CHECK_EQ(t.seeded_certified, t.floating);
}

// Integration dans le resolveur (resolve.cpp, propose) : le triangle aigu de inert_below_window (unit.cpp), dans les six
// permutations des axes (isometries : memes boules, autres rangs de Morton), resolu a l'ordre 3 par resolve_part. La
// proposition entiere rend le support trie en SiteIdx du triangle : LEM-T1 conclut du premier coup (route_t1), aucun
// repli (sans proposition, hors de la partie, certificat en echec), puis le pas inerte et la naissance. Au moins une
// permutation place l'apex (le site hors de la paire la plus eloignee) avant le dernier SiteIdx : un support non trie y
// serait refuse par sorted_subset.
MHGP12_TEST(routes, 40) {
  const Xyz a0{0, 0, 0}, b0{10, 0, 0}, c0{5, 8, 0}, z0{5, 2, 0}, w0{5, 3, 0}, e0{60, 60, 60};
  const int perms[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
  u64 apex_inside = 0;
  for (const auto& perm : perms) {
    auto at = [&](const Xyz& p) { return Xyz{p[perm[0]], p[perm[1]], p[perm[2]]}; };
    const Xyz a = at(a0), b = at(b0), cc = at(c0), z = at(z0), w = at(w0), e = at(e0);
    auto c = build({a, b, cc, z, w, e}, 4);
    REQUIRE(c->outcome.ok());
    const u32 next = c->ball({a, z, w}), far = c->ball({cc, e});
    REQUIRE(next != kNone && far != kNone);
    const auto f = c->part({a, b, cc});
    if (c->site(cc) != f.id[2]) ++apex_inside;  // apex (5,8,0) : la paire la plus eloignee est (a, b)
    auto direct = resolve_direct(*c, f, 3, c->rank(far));
    REQUIRE(direct.target.ok());
    CHECK_EQ(target_ball(*c, 3, direct.target.value()), next);
    CHECK_EQ(direct.counters.route_t1, 1u);
    CHECK_EQ(direct.counters.fallback_no_proposal, 0u);
    CHECK_EQ(direct.counters.fallback_not_in_part, 0u);
    CHECK_EQ(direct.counters.fallback_certificate, 0u);
    CHECK_EQ(direct.counters.route_fallback_table + direct.counters.route_fallback_census, 0u);
    CHECK_EQ(direct.counters.inert_steps, 1u);
  }
  std::printf("proposal routes permutations=6 apex_avant_le_dernier=%llu\n", static_cast<unsigned long long>(apex_inside));
  CHECK(apex_inside >= 1);
}

MHGP12_TEST_MAIN()
