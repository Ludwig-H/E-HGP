// Portes du module cloud : cle de Morton, controle des tailles et du domaine, sites et multiplicites contre un juge
// independant, invariance par permutation et par renumerotation, budget memoire.
//
// Juge independant (expected_of) : cle par entrelacement bit a bit (la definition, sans les masques du produit),
// tri par std::sort sur (cle, PointId), regroupement lineaire. Il ne partage avec le produit que les types de core.
#include <algorithm>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

#include "cloud/cloud.hpp"
#include "test.hpp"

using namespace mhgp12;

namespace {

struct Input {
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;

  void add(u32 px, u32 py, u32 pz, u32 id) {
    x.push_back(px);
    y.push_back(py);
    z.push_back(pz);
    ids.push_back(make_id<PointId>(id));
  }
  u64 size() const { return x.size(); }
};

struct Expected {
  std::vector<u32> x, y, z, w, val;
  std::vector<u64> off;
};

// Entrelacement bit a bit : le bit i de x a la position 3i, de y a 3i + 1, de z a 3i + 2.
u128 reference_key(u32 x, u32 y, u32 z) {
  u128 key = 0;
  for (int i = 0; i < 32; ++i) {
    key |= static_cast<u128>((x >> i) & 1u) << (3 * i);
    key |= static_cast<u128>((y >> i) & 1u) << (3 * i + 1);
    key |= static_cast<u128>((z >> i) & 1u) << (3 * i + 2);
  }
  return key;
}

Expected expected_of(const Input& in) {
  struct Row {
    u128 key;
    u32 id, x, y, z;
  };
  std::vector<Row> rows;
  for (u64 i = 0; i < in.size(); ++i)
    rows.push_back({reference_key(in.x[i], in.y[i], in.z[i]), idx(in.ids[i]), in.x[i], in.y[i], in.z[i]});
  std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
    if (a.key != b.key) return a.key < b.key;
    return a.id < b.id;
  });
  Expected e;
  for (u64 i = 0; i < rows.size(); ++i) {
    if (i == 0 || rows[i].key != rows[i - 1].key) {
      e.x.push_back(rows[i].x);
      e.y.push_back(rows[i].y);
      e.z.push_back(rows[i].z);
      e.w.push_back(0);
      e.off.push_back(i);
    }
    ++e.w.back();
    e.val.push_back(rows[i].id);
  }
  e.off.push_back(rows.size());
  return e;
}

template <class T, class U>
bool same_array(std::span<const T> got, const std::vector<U>& want) {
  if (got.size() != want.size()) return false;
  for (u64 i = 0; i < got.size(); ++i)
    if (static_cast<u64>(got[i]) != static_cast<u64>(want[i])) return false;
  return true;
}

bool well_formed(const Cloud& c) {
  const auto offsets = c.offsets();
  if (offsets.size() != u64{c.sites()} + 1 || offsets[0] != 0 || offsets.back() != c.ids().size()) return false;
  for (u32 i = 0; i < c.sites(); ++i)
    if (offsets[i + 1] <= offsets[i] || offsets[i + 1] - offsets[i] != c.w()[i]) return false;
  return true;
}

bool matches(const Cloud& c, const Expected& e) {
  return same_array(c.x(), e.x) && same_array(c.y(), e.y) && same_array(c.z(), e.z) && same_array(c.w(), e.w) &&
         same_array(c.offsets(), e.off) && same_array(c.ids(), e.val) && c.weight() == e.val.size() &&
         c.sites() == e.x.size() && well_formed(c);
}

bool same_clouds(const Cloud& a, const Cloud& b) {
  auto same = [](const auto& p, const auto& q) { return std::equal(p.begin(), p.end(), q.begin(), q.end()); };
  return same(a.x(), b.x()) && same(a.y(), b.y()) && same(a.z(), b.z()) && same(a.w(), b.w()) &&
         same(a.offsets(), b.offsets()) && same(a.ids(), b.ids()) && a.weight() == b.weight();
}

// Cles strictement croissantes d'un site au suivant, PointId strictement croissants dans un site.
bool canonical_order(const Cloud& c) {
  for (u32 s = 0; s < c.sites(); ++s) {
    if (s > 0 && !(morton_key(c.x()[s - 1], c.y()[s - 1], c.z()[s - 1]) <
                   morton_key(c.x()[s], c.y()[s], c.z()[s]))) return false;
    const std::span<const PointId> points = c.points(make_id<SiteIdx>(s));
    if (points.size() != c.w()[s] || points.empty()) return false;
    for (u64 i = 1; i < points.size(); ++i)
      if (!(idx(points[i - 1]) < idx(points[i]))) return false;
  }
  return true;
}

Result<Cloud> prepare(const Input& in, MemoryBudget& budget, CoordWidth width = CoordWidth()) {
  return prepare_cloud(in.x, in.y, in.z, in.ids, width, budget);
}

Reason refusal_of(const Input& in, CoordWidth width = CoordWidth()) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const Result<Cloud> r = prepare(in, budget, width);
  if (budget.used() != 0 && !r.ok()) return Reason::budget_not_released;  // un refus ne laisse rien de reserve
  return r.outcome().reason;
}

// Generateur deterministe (sorties de mt19937_64 fixees par la norme ; aucune distribution de la bibliotheque).
struct Random {
  std::mt19937_64 engine;
  explicit Random(u64 seed) : engine(seed) {}
  u32 below(u64 bound) { return static_cast<u32>(engine() % bound); }
};

// n points tires dans [0, range)^3 puis multiplies par scale ; identifiants id_of(i).
template <class IdOf>
Input random_input(u64 n, u64 range, u32 scale, u64 seed, IdOf id_of) {
  Random rng(seed);
  Input in;
  for (u64 i = 0; i < n; ++i) {
    const u32 x = rng.below(range) * scale, y = rng.below(range) * scale, z = rng.below(range) * scale;
    in.add(x, y, z, id_of(i));
  }
  return in;
}

u32 identity(u64 i) { return static_cast<u32>(i); }

void shuffle(Input& in, u64 seed) {
  Random rng(seed);
  for (u64 i = in.size(); i > 1; --i) {
    const u64 j = rng.engine() % i;
    std::swap(in.x[i - 1], in.x[j]);
    std::swap(in.y[i - 1], in.y[j]);
    std::swap(in.z[i - 1], in.z[j]);
    std::swap(in.ids[i - 1], in.ids[j]);
  }
}

// Les deux termes du pic documente par cloud.hpp, recalcules ici : le tri (deux tampons d'enregistrements et les
// histogrammes), puis le remplissage (enregistrements tries et resultat).
u64 sort_bytes(u64 n) {
  const u64 record = kCoordBits <= 21 ? 16 : 32;
  const u64 key_digits = (3 * static_cast<u64>(kCoordBits) + 10) / 11;
  return 2 * record * n + (3 + key_digits) * 2048 * 4;
}
u64 fill_bytes(u64 n, u64 sites) {
  const u64 record = kCoordBits <= 21 ? 16 : 32;
  return record * n + 4 * n + 16 * sites + 8 * (sites + 1);
}
u64 formula_peak(u64 n, u64 sites) { return std::max(sort_bytes(n), fill_bytes(n, sites)); }

}  // namespace

MHGP12_TEST(morton, 15) {
  // convention gravee : x porte le bit de poids faible de chaque triplet
  CHECK(morton_key(0, 0, 0) == 0);
  CHECK(morton_key(1, 0, 0) == 1);
  CHECK(morton_key(0, 1, 0) == 2);
  CHECK(morton_key(0, 0, 1) == 4);
  CHECK(morton_key(1, 1, 1) == 7);
  CHECK(morton_key(2, 0, 0) == 8);
  CHECK(morton_key(3, 1, 0) == 11);
  CHECK(morton_key(0, 0, 2) == 32);
  // bornes du profil : la plus grande position donne la cle de 3B bits a un, sans bit perdu ni masque
  const MortonKey all = morton_key(kCoordMax, kCoordMax, kCoordMax);
  CHECK(static_cast<u128>(all) == (static_cast<u128>(1) << kMortonBits) - 1);
  CHECK(static_cast<u128>(morton_key(0, 0, u32{1} << (kCoordBits - 1))) == static_cast<u128>(1) << (kMortonBits - 1));
  CHECK(morton_key(kCoordMax, kCoordMax, (u32{1} << (kCoordBits - 1)) - 1) <
        morton_key(0, 0, u32{1} << (kCoordBits - 1)));
  CHECK_EQ(static_cast<int>(sizeof(MortonKey)), kCoordBits <= 21 ? 8 : 16);

  // le produit contre la definition, sur le profil compile puis sur les deux profils (21 et 24 ; la v11 controlait
  // aussi son profil 18, abandonne)
  Random rng(20261002);
  u64 disagreements = 0, disagreements21 = 0, disagreements24 = 0;
  for (int i = 0; i < 20000; ++i) {
    const u32 a = static_cast<u32>(rng.engine()), b = static_cast<u32>(rng.engine()), c = static_cast<u32>(rng.engine());
    const u32 x = a & kCoordMax, y = b & kCoordMax, z = c & kCoordMax;
    disagreements += static_cast<u128>(morton_key(x, y, z)) != reference_key(x, y, z);
    const u32 m21 = (u32{1} << 21) - 1, m24 = (u32{1} << 24) - 1;
    disagreements21 += static_cast<u128>(detail::morton_key_for<21>(a & m21, b & m21, c & m21)) !=
                       reference_key(a & m21, b & m21, c & m21);
    disagreements24 += static_cast<u128>(detail::morton_key_for<24>(a & m24, b & m24, c & m24)) !=
                       reference_key(a & m24, b & m24, c & m24);
  }
  CHECK_EQ(disagreements, 0u);
  CHECK_EQ(disagreements21, 0u);
  CHECK_EQ(disagreements24, 0u);
}

MHGP12_TEST(width, 12) {
  CHECK_EQ(CoordWidth().bits(), kCoordBits);
  CHECK_EQ(CoordWidth().max(), kCoordMax);
  CHECK(!CoordWidth::of(0).has_value());
  CHECK(!CoordWidth::of(-1).has_value());
  CHECK(!CoordWidth::of(kCoordBits + 1).has_value());
  CHECK_EQ(CoordWidth::of(32).has_value(), kCoordBits == 32);
  REQUIRE(CoordWidth::of(1).has_value());
  CHECK_EQ(CoordWidth::of(1)->bits(), 1);
  CHECK_EQ(CoordWidth::of(1)->max(), 1u);
  REQUIRE(CoordWidth::of(kCoordBits).has_value());
  CHECK_EQ(CoordWidth::of(kCoordBits)->max(), kCoordMax);
  REQUIRE(CoordWidth::of(kCoordBits - 1).has_value());
  CHECK_EQ(CoordWidth::of(kCoordBits - 1)->max(), kCoordMax >> 1);
}

MHGP12_TEST(sizes, 14) {
  CHECK(check_cloud_sizes(1, 1, 1, 1).ok());
  CHECK(check_cloud_sizes(40, 40, 40, 40).ok());
  CHECK_EQ(check_cloud_sizes(0, 0, 0, 0).reason, Reason::empty_input);
  CHECK_EQ(check_cloud_sizes(0, 3, 3, 3).reason, Reason::empty_input);
  CHECK_EQ(check_cloud_sizes(0, 0, 0, 0).status(), Status::invalid_input);
  CHECK_EQ(check_cloud_sizes(3, 2, 3, 3).reason, Reason::size_mismatch);
  CHECK_EQ(check_cloud_sizes(3, 3, 4, 3).reason, Reason::size_mismatch);
  CHECK_EQ(check_cloud_sizes(3, 3, 3, 0).reason, Reason::size_mismatch);
  CHECK_EQ(check_cloud_sizes(3, 2, 3, 3).status(), Status::invalid_input);
  // cardinaux artificiels, sans tableau : kNone - 1 points sont admis, kNone ne le sont plus
  const u64 none = kNone;
  CHECK(check_cloud_sizes(none - 1, none - 1, none - 1, none - 1).ok());
  CHECK_EQ(check_cloud_sizes(none, none, none, none).reason, Reason::index_overflow_u32);
  CHECK_EQ(check_cloud_sizes(none, none, none, none).status(), Status::resource_exhausted);
  CHECK_EQ(check_cloud_sizes(u64{1} << 40, u64{1} << 40, u64{1} << 40, u64{1} << 40).reason,
           Reason::index_overflow_u32);
  CHECK_EQ(check_cloud_sizes(none, none - 1, none, none).reason, Reason::size_mismatch);
}

MHGP12_TEST(refusals, 30) {
  Input four;
  four.add(0, 0, 0, 0);
  four.add(8, 0, 0, 1);
  four.add(0, 8, 0, 2);
  four.add(0, 0, 8, 3);
  CHECK_EQ(refusal_of(four), Reason::none);

  CHECK_EQ(refusal_of(Input{}), Reason::empty_input);
  for (int axis = 0; axis < 4; ++axis) {  // un tableau plus court que les autres
    Input bad = four;
    if (axis == 0) bad.x.pop_back();
    if (axis == 1) bad.y.pop_back();
    if (axis == 2) bad.z.pop_back();
    if (axis == 3) bad.ids.pop_back();
    CHECK_EQ(refusal_of(bad), Reason::size_mismatch);
  }

  // domaine du profil : 2^B - 1 est admis sur chaque axe, 2^B est refuse sur chaque axe, a chaque rang. Au profil 32
  // tout u32 est dans le domaine du profil : le meme temoin se joue a la largeur declaree 31.
  const CoordWidth limit = kCoordBits < 32 ? CoordWidth() : *CoordWidth::of(31);
  for (int axis = 0; axis < 3; ++axis) {
    for (u64 at : {u64{0}, u64{3}}) {
      Input edge = four, out = four, far = four;
      std::vector<u32>& e = axis == 0 ? edge.x : axis == 1 ? edge.y : edge.z;
      std::vector<u32>& o = axis == 0 ? out.x : axis == 1 ? out.y : out.z;
      std::vector<u32>& f = axis == 0 ? far.x : axis == 1 ? far.y : far.z;
      e[at] = limit.max();
      o[at] = limit.max() + 1;
      f[at] = 0xFFFFFFFFu;
      CHECK_EQ(refusal_of(edge, limit), Reason::none);
      CHECK_EQ(refusal_of(out, limit), Reason::coordinate_out_of_domain);
      CHECK_EQ(refusal_of(far, limit), Reason::coordinate_out_of_domain);
    }
  }
  // largeur declaree plus etroite que le profil : 8 demande 4 bits
  REQUIRE(CoordWidth::of(4).has_value() && CoordWidth::of(3).has_value());
  CHECK_EQ(refusal_of(four, *CoordWidth::of(4)), Reason::none);
  CHECK_EQ(refusal_of(four, *CoordWidth::of(3)), Reason::coordinate_out_of_domain);
  CHECK_EQ(status_of(Reason::coordinate_out_of_domain), Status::invalid_input);

  // identifiants en double : voisins ou non, valeurs extremes, positions egales ou non
  Input dup = four;
  dup.ids[3] = dup.ids[0];
  CHECK_EQ(refusal_of(dup), Reason::duplicate_point_id);
  dup = four;
  dup.ids[0] = make_id<PointId>(0xFFFFFFFFu);
  CHECK_EQ(refusal_of(dup), Reason::none);  // 0xFFFFFFFF est un PointId comme un autre
  dup.ids[2] = make_id<PointId>(0xFFFFFFFFu);
  CHECK_EQ(refusal_of(dup), Reason::duplicate_point_id);
  dup = four;
  dup.add(8, 0, 0, 1);  // meme position ET meme identifiant
  CHECK_EQ(refusal_of(dup), Reason::duplicate_point_id);
  CHECK_EQ(status_of(Reason::duplicate_point_id), Status::invalid_input);

  // ordre des refus : le domaine est juge avant les identifiants
  Input both = four;
  both.x[1] = limit.max() + 1;
  both.ids[3] = both.ids[0];
  CHECK_EQ(refusal_of(both, limit), Reason::coordinate_out_of_domain);
}

MHGP12_TEST(fixture, 12) {
  // huit points, six sites ; attendu ecrit a la main, dans l'ordre de Morton (cles 0, 1, 2, 4, 8, 11)
  Input in;
  in.add(1, 0, 0, 7);
  in.add(0, 1, 0, 3);
  in.add(0, 0, 1, 9);
  in.add(0, 0, 0, 2);
  in.add(1, 0, 0, 5);
  in.add(2, 0, 0, 4000000000u);
  in.add(3, 1, 0, 0);
  in.add(0, 0, 0, 1);
  const Expected want{{0, 1, 0, 0, 2, 3}, {0, 0, 1, 0, 0, 1}, {0, 0, 0, 1, 0, 0}, {2, 2, 1, 1, 1, 1},
                      {1, 2, 5, 7, 3, 9, 4000000000u, 0}, {0, 2, 4, 5, 6, 7, 8}};
  MemoryBudget budget(MemoryBudget::kUnlimited);
  {
    const Result<Cloud> r = prepare(in, budget);
    REQUIRE(r.ok());
    const Cloud& c = r.value();
    CHECK(matches(c, want));
    CHECK_EQ(c.sites(), 6u);
    CHECK_EQ(c.weight(), 8u);
    CHECK(canonical_order(c));
    const std::span<const PointId> second = c.points(make_id<SiteIdx>(1));
    REQUIRE(second.size() == 2);
    CHECK_EQ(idx(second[0]), 5u);
    CHECK_EQ(idx(second[1]), 7u);
    CHECK(matches(c, expected_of(in)));  // le juge independant rend le meme attendu : il est lui-meme juge ici
  }
  CHECK(budget.released().ok());

  // bits hauts du profil : le dernier chiffre du tri porte le bit 3B - 1 de la cle
  const u32 high = u32{1} << (kCoordBits - 1);
  Input top;
  top.add(kCoordMax, kCoordMax, kCoordMax, 12);
  top.add(0, 0, high, 11);
  top.add(kCoordMax, kCoordMax, high - 1, 10);
  top.add(0, 0, 0, 13);
  const Expected want_top{{0, kCoordMax, 0, kCoordMax}, {0, kCoordMax, 0, kCoordMax}, {0, high - 1, high, kCoordMax},
                          {1, 1, 1, 1}, {13, 10, 11, 12}, {0, 1, 2, 3, 4}};
  const Result<Cloud> r = prepare(top, budget);
  REQUIRE(r.ok());
  CHECK(matches(r.value(), want_top));
}

MHGP12_TEST(judge, 51) {
  const u64 full = u64{1} << kCoordBits;
  std::vector<Input> clouds;
  clouds.push_back(random_input(2000, full, 1, 1, [](u64 i) { return static_cast<u32>(i * 2654435761u + 17); }));
  clouds.push_back(random_input(3000, 8, 1, 2, identity));                       // boite de 512 positions : doublons
  clouds.push_back(random_input(500, 1, 1, 3, identity));                        // un seul site de poids 500
  clouds.push_back(random_input(1, full, 1, 4, identity));                       // un point
  clouds.push_back(random_input(1500, 8, u32{1} << (kCoordBits - 3), 5, identity));  // bits hauts seulement
  // identifiants dont un seul chiffre de 11 bits varie (bas, milieu, haut) : les autres passes sont sautees
  clouds.push_back(random_input(1200, 64, 1, 6, [](u64 i) { return static_cast<u32>(i) + 0xABC00000u; }));
  clouds.push_back(random_input(1200, 64, 1, 7, [](u64 i) { return static_cast<u32>(i << 11) + 5u; }));
  clouds.push_back(random_input(900, 64, 1, 8, [](u64 i) { return static_cast<u32>(i << 22) + 0x3FFFFFu; }));
  // identifiants decroissants : l'ordre d'entree est l'inverse de l'ordre attendu dans chaque site
  clouds.push_back(random_input(2500, 4, 1, 9, [](u64 i) { return 0xFFFFFFFFu - static_cast<u32>(i); }));
  // une droite : y et z constants
  Input line = random_input(1000, full, 1, 10, identity);
  for (u64 i = 0; i < line.size(); ++i) line.y[i] = line.z[i] = 5;
  clouds.push_back(line);
  // taille d'interet : 32 000 points, 5 % de positions doublees
  Input large = random_input(32000, full, 1, 11, [](u64 i) { return static_cast<u32>(i * 40503u + 99); });
  for (u64 i = 0; i < large.size(); i += 20) {
    large.x[i] = large.x[i + 1];
    large.y[i] = large.y[i + 1];
    large.z[i] = large.z[i + 1];
  }
  clouds.push_back(large);
  // multiplicites ET identifiants etales sur 32 bits : l'ordre dans un site depend des trois chiffres du PointId
  clouds.push_back(random_input(3000, 6, 1, 12, [](u64 i) { return static_cast<u32>(i * 2654435761u + 17); }));

  MemoryBudget budget(MemoryBudget::kUnlimited);
  u64 weighted = 0;
  for (const Input& in : clouds) {
    const Result<Cloud> r = prepare(in, budget);
    REQUIRE(r.ok());
    const Cloud& c = r.value();
    CHECK(matches(c, expected_of(in)));
    CHECK(canonical_order(c));
    CHECK_EQ(c.weight(), in.size());
    weighted += c.sites() < in.size();
  }
  CHECK_EQ(clouds.size(), 12u);
  CHECK(weighted >= 6);  // plancher : des nuages a multiplicites ont bien ete juges
  CHECK(budget.released().ok());
}

// Identite des sites par egalite de cle exacte (CST-0202) : une cle tronquee aurait fusionne deux positions
// distinctes, et un departage par PointId aurait separe deux vrais doublons par un troisieme site de meme cle.
// Temoins de l'auditeur Codex (receipts/audit_contrats_20261007/numerique/witness.py, morton_identity), au profil
// compile : au profil 32 les trois positions sont exactement (0,0,0), (1,0,0) et (2^32-1,0,0).
MHGP12_TEST(identity, 30) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  // Trois positions dont les deux premieres auraient la meme cle apres un decalage de B - 21 bits (11 au profil 32).
  Input three;
  three.add(0, 0, 0, 1);
  three.add(1, 0, 0, 2);
  three.add(kCoordMax, 0, 0, 3);
  CHECK(reference_key(0, 0, 0) != reference_key(1, 0, 0));
  CHECK((reference_key(0, 0, 0) >> 33) == (reference_key(1, 0, 0) >> 33));  // meme cle une fois tronquee
  {
    const Result<Cloud> r = prepare(three, budget);
    REQUIRE(r.ok());
    CHECK_EQ(r.value().sites(), 3u);
    CHECK(matches(r.value(), expected_of(three)));
  }
  // Deux vrais doublons separes dans l'entree par un troisieme site : (A,1), (B,2), (A,3) donnent deux sites, A de
  // multiplicite 2 (PointId 1 et 3), B de multiplicite 1.
  Input interleaved;
  interleaved.add(0, 0, 0, 1);
  interleaved.add(1, 0, 0, 2);
  interleaved.add(0, 0, 0, 3);
  {
    const Result<Cloud> r = prepare(interleaved, budget);
    REQUIRE(r.ok());
    const Cloud& c = r.value();
    REQUIRE(c.sites() == 2u);
    CHECK(c.x()[0] == 0u && c.w()[0] == 2u && c.w()[1] == 1u);
    const auto first = c.points(make_id<SiteIdx>(0));
    REQUIRE(first.size() == 2u);
    CHECK(idx(first[0]) == 1u && idx(first[1]) == 3u);
    CHECK(idx(c.points(make_id<SiteIdx>(1))[0]) == 2u);
  }
  // Permutations de l'entree, PointId stables : meme nuage, octet pour octet, pour les deux temoins reunis et des
  // positions aux deux bords du domaine.
  Input all = three;
  for (u64 i = 0; i < interleaved.size(); ++i)
    all.add(interleaved.x[i], interleaved.y[i] + 5, interleaved.z[i], idx(interleaved.ids[i]) + 10);
  all.add(kCoordMax, kCoordMax, kCoordMax, 20);
  all.add(kCoordMax, kCoordMax, kCoordMax, 21);
  all.add(kCoordMax - 1, kCoordMax, kCoordMax, 22);
  const Result<Cloud> reference = prepare(all, budget);
  REQUIRE(reference.ok());
  CHECK(matches(reference.value(), expected_of(all)));
  CHECK_EQ(reference.value().sites(), 7u);
  for (u64 seed = 0; seed < 8; ++seed) {
    Input permuted = all;
    shuffle(permuted, 700 + seed);
    const Result<Cloud> again = prepare(permuted, budget);
    REQUIRE(again.ok());
    CHECK(same_clouds(reference.value(), again.value()));
  }
}

MHGP12_TEST(permutation, 14) {
  const u64 full = u64{1} << kCoordBits;
  MemoryBudget budget(MemoryBudget::kUnlimited);
  u64 round = 0;
  for (const Input& base : {random_input(4000, 16, 1, 21, [](u64 i) { return static_cast<u32>(i * 7919u + 3); }),
                            random_input(4000, full, 1, 22, identity)}) {
    const Result<Cloud> reference = prepare(base, budget);
    REQUIRE(reference.ok());
    for (u64 seed = 0; seed < 6; ++seed) {
      Input permuted = base;
      shuffle(permuted, 100 + 10 * round + seed);
      const Result<Cloud> again = prepare(permuted, budget);
      REQUIRE(again.ok());
      CHECK(same_clouds(reference.value(), again.value()));
    }
    // temoin : la permutation a bien change l'entree
    Input permuted = base;
    shuffle(permuted, 100 + 10 * round);
    CHECK(permuted.ids != base.ids);
    ++round;
  }
}

MHGP12_TEST(renumbering, 12) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const Input base = random_input(3000, 12, 1, 31, identity);
  const Result<Cloud> reference = prepare(base, budget);
  REQUIRE(reference.ok());
  const Cloud& a = reference.value();
  CHECK(a.sites() < base.size());  // des sites a plusieurs points : l'ordre dans un site est exerce
  // renumerotations injectives : multiplication par un impair modulo 2^32 (bijection, non monotone), complement
  const auto odd = [](u32 id) { return id * 2654435761u + 0x9E3779B9u; };
  const auto complement = [](u32 id) { return ~id; };
  for (int which = 0; which < 2; ++which) {
    Input renumbered = base;
    for (PointId& id : renumbered.ids) id = make_id<PointId>(which == 0 ? odd(idx(id)) : complement(idx(id)));
    const Result<Cloud> other = prepare(renumbered, budget);
    REQUIRE(other.ok());
    const Cloud& b = other.value();
    auto same = [](const auto& p, const auto& q) { return std::equal(p.begin(), p.end(), q.begin(), q.end()); };
    // sites, ordre, multiplicites et decalages : inchanges
    CHECK(same(a.x(), b.x()) && same(a.y(), b.y()) && same(a.z(), b.z()));
    CHECK(same(a.w(), b.w()) && same(a.offsets(), b.offsets()));
    CHECK_EQ(a.weight(), b.weight());
    // table site -> PointId : l'image de celle d'origine, triee dans chaque site
    bool mapped = true;
    for (u32 s = 0; s < a.sites(); ++s) {
      std::vector<u32> want;
      for (PointId id : a.points(make_id<SiteIdx>(s))) want.push_back(which == 0 ? odd(idx(id)) : complement(idx(id)));
      std::sort(want.begin(), want.end());
      const std::span<const PointId> got = b.points(make_id<SiteIdx>(s));
      mapped = mapped && got.size() == want.size();
      for (u64 i = 0; mapped && i < want.size(); ++i) mapped = idx(got[i]) == want[i];
    }
    CHECK(mapped);
    CHECK(!same(a.ids(), b.ids()));  // temoin : la table a bien change
  }
}

MHGP12_TEST(budget, 21) {
  const Input in = random_input(5000, 14, 1, 41, identity);  // 2744 positions au plus : multiplicites
  const Expected want = expected_of(in);
  const u64 n = in.size(), sites = want.x.size();
  const u64 result_bytes = 16 * sites + 8 * (sites + 1) + 4 * n;
  u64 peak = 0;
  {
    MemoryBudget budget(MemoryBudget::kUnlimited);
    {
      const Result<Cloud> r = prepare(in, budget);
      REQUIRE(r.ok());
      CHECK_EQ(budget.used(), result_bytes);  // apres l'appel, seul le resultat reste reserve
      peak = budget.peak();
      CHECK_EQ(peak, formula_peak(n, sites));
      CHECK(sort_bytes(n) > fill_bytes(n, sites));  // ici le tri domine
    }
    CHECK(budget.released().ok());
  }
  // le pic mesure est exactement le plus petit budget qui admet le nuage
  {
    MemoryBudget exact(peak);
    const Result<Cloud> r = prepare(in, exact);
    REQUIRE(r.ok());
    CHECK(matches(r.value(), want));
    CHECK_EQ(exact.peak(), peak);
  }
  {
    MemoryBudget short_by_one(peak - 1);
    const Result<Cloud> r = prepare(in, short_by_one);
    CHECK_EQ(r.outcome().reason, Reason::memory_budget);
    CHECK_EQ(r.outcome().status(), Status::resource_exhausted);
    CHECK_EQ(short_by_one.used(), 0u);
  }
  // refus a chaque etage : tri (budget nul, puis un seul des deux tampons), puis resultat
  for (u64 limit : {u64{0}, u64{100}, formula_peak(n, sites) / 2}) {
    MemoryBudget small(limit);
    CHECK_EQ(prepare(in, small).outcome().reason, Reason::memory_budget);
    CHECK_EQ(small.used(), 0u);
  }
  // nuage sans doublon, plus grand : le pic suit encore la formule
  const Input distinct = random_input(20000, u64{1} << kCoordBits, 1, 42, identity);
  const u64 distinct_sites = expected_of(distinct).x.size();
  MemoryBudget budget(MemoryBudget::kUnlimited);
  {
    const Result<Cloud> r = prepare(distinct, budget);
    REQUIRE(r.ok());
    CHECK_EQ(budget.peak(), formula_peak(distinct.size(), distinct_sites));
    // jusqu'a 21 bits le remplissage domine ici : les deux termes de la formule sont exerces
    CHECK(kCoordBits > 21 || fill_bytes(distinct.size(), distinct_sites) > sort_bytes(distinct.size()));
  }
  CHECK(budget.released().ok());
  // un refus d'entree ne laisse rien de reserve
  Input dup = in;
  dup.ids[10] = dup.ids[20];
  CHECK_EQ(prepare(dup, budget).outcome().reason, Reason::duplicate_point_id);
  CHECK_EQ(budget.used(), 0u);
}

MHGP12_TEST(ownership, 27) {
  CHECK(!std::is_default_constructible_v<Cloud>);
  CHECK(!std::is_copy_constructible_v<Cloud>);
  CHECK(!std::is_copy_assignable_v<Cloud>);
  CHECK(!std::is_move_assignable_v<Cloud>);
  CHECK(std::is_nothrow_move_constructible_v<Cloud>);
  // Meme un proprietaire non const n'offre que des vues const ; aucun Buffer/Csr mutable ne s'echappe.
  CHECK((std::is_same_v<decltype(std::declval<Cloud&>().x()), std::span<const u32>>));
  CHECK((std::is_same_v<decltype(std::declval<Cloud&>().y()), std::span<const u32>>));
  CHECK((std::is_same_v<decltype(std::declval<Cloud&>().z()), std::span<const u32>>));
  CHECK((std::is_same_v<decltype(std::declval<Cloud&>().w()), std::span<const u32>>));
  CHECK((std::is_same_v<decltype(std::declval<Cloud&>().offsets()), std::span<const u64>>));
  CHECK((std::is_same_v<decltype(std::declval<Cloud&>().ids()), std::span<const PointId>>));
  CHECK((std::is_same_v<decltype(std::declval<Cloud&>().points(make_id<SiteIdx>(0))), std::span<const PointId>>));
  MemoryBudget budget(MemoryBudget::kUnlimited);
  {
    Input in;
    in.add(5, 2, 3, 10);
    in.add(0, 0, 0, 8);
    in.add(5, 2, 3, 2);
    const Expected expected = expected_of(in);
    Result<Cloud> r = prepare(in, budget);
    REQUIRE(r.ok());
    CHECK(matches(r.value(), expected));
    const u64 bytes = budget.used(), peak = budget.peak();
    CHECK(r.value().x().data() != in.x.data() && r.value().y().data() != in.y.data() &&
          r.value().z().data() != in.z.data() && r.value().ids().data() != in.ids.data());
    const auto borrowed = r.value().x();
    Cloud owner(std::move(r).take());
    CHECK(borrowed.data() == owner.x().data());
    CHECK_EQ(budget.used(), bytes);
    CHECK_EQ(budget.peak(), peak);
    // Les alias conserves par l'appelant visent l'entree uniquement.
    in.x[0] = kCoordMax;
    in.y[1] = kCoordMax;
    in.z[2] = kCoordMax;
    in.ids[0] = make_id<PointId>(99);
    CHECK(matches(owner, expected));
    in = Input{};
    CHECK(matches(owner, expected));
    Cloud moved(std::move(owner));
    CHECK(matches(moved, expected));
    CHECK_EQ(budget.used(), bytes);
    CHECK_EQ(budget.peak(), peak);
    CHECK_EQ(owner.weight(), 0u);
    CHECK(owner.sites() == 0 && owner.x().empty() && owner.y().empty() && owner.z().empty() && owner.w().empty() &&
          owner.offsets().empty() && owner.ids().empty());
    CHECK(borrowed.data() == moved.x().data() && borrowed[1] == 5);
  }
  CHECK(budget.released().ok());
}

MHGP12_TEST(refusal_priority, 10) {
  // Au profil 32 tout u32 est dans le domaine du profil : le hors-domaine se joue a la largeur declaree 31.
  const CoordWidth limit = kCoordBits < 32 ? CoordWidth() : *CoordWidth::of(31);
  MemoryBudget zero(0);
  Input in;
  CHECK_EQ(prepare(in, zero, limit).outcome().reason, Reason::empty_input);
  in.add(limit.max() + 1, 0, 0, 0);
  in.ids.clear();
  CHECK_EQ(prepare(in, zero, limit).outcome().reason, Reason::size_mismatch);
  in.ids.push_back(make_id<PointId>(0));
  CHECK_EQ(prepare(in, zero, limit).outcome().reason, Reason::coordinate_out_of_domain);
  in.x[0] = 0;
  in.add(0, 0, 0, 0);
  CHECK_EQ(prepare(in, zero, limit).outcome().reason, Reason::memory_budget);
  CHECK_EQ(zero.peak(), 0u);
  CHECK(zero.released().ok());
  MemoryBudget ample(MemoryBudget::kUnlimited);
  CHECK_EQ(prepare(in, ample, limit).outcome().reason, Reason::duplicate_point_id);
  CHECK(ample.released().ok());
  in.x[0] = limit.max() + 1;
  CHECK_EQ(prepare(in, ample, limit).outcome().reason, Reason::coordinate_out_of_domain);
  CHECK(ample.released().ok());
}

MHGP12_TEST(shared_budget, 11) {
  const Input in = random_input(100, 8, 1, 55, identity);
  const Expected expected = expected_of(in);
  const u64 sites = expected.x.size();
  const u64 bytes = 16 * sites + 8 * (sites + 1) + 4 * in.size();
  MemoryBudget budget(formula_peak(in.size(), sites) + bytes);
  {
    const Result<Cloud> first = prepare(in, budget);
    REQUIRE(first.ok());
    {
      Buffer<u8> held;
      REQUIRE(held.allocate(1, budget).ok());
      CHECK_EQ(prepare(in, budget).outcome().reason, Reason::memory_budget);
      CHECK_EQ(budget.used(), bytes + 1);
      CHECK(matches(first.value(), expected));
    }
    CHECK_EQ(budget.used(), bytes);
    const Result<Cloud> second = prepare(in, budget);
    REQUIRE(second.ok());
    CHECK(matches(first.value(), expected));
    CHECK(matches(second.value(), expected));
    CHECK_EQ(budget.used(), 2 * bytes);
  }
  CHECK(budget.released().ok());
}

MHGP12_TEST_MAIN()
