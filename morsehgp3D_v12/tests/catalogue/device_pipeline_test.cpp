// Voie appareil complete (device_pipeline.hpp) jouee sur l'hote par l'executeur Pool, contre la voie CPU : meme
// Catalogue (export, grand livre, niveaux, table) sur les temoins du catalogue, dont les feuilles NON RESOLUES par
// l'appareil (triangle long : etendue B + 1 > 16, politique exacte ; coquille de 48 sites : feuilles de 48 sites, warp
// virtuel), rejouees sur l'hote avant admission (voie hybride) ; diagnostics physiques comptes comme la voie CPU
// (paliers, reecritures) et reprises par cause, transferts comptes et durees disjointes (CST-0235) ; memes refus
// (WIT-SPHERE50 : shell_capacity) ; etat resident reutilise d'un appel a l'autre ; memes octets a 1, 3 et 8 fils. Puis
// la vraie voie appareil (device_open) : sans GPU device_unavailable, avec GPU la meme comparaison sur l'appareil.
#include "device_support.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::catalogue_detail;
using namespace mhgp12::device_test;

namespace {

struct Case {
  const char* name;
  Points points;
  int k;
  u32 leaf;
};

std::vector<Case> witnesses() {
  Points cube{{8, 8, 8}};
  for (u32 x : {0u, 16u})
    for (u32 y : {0u, 16u})
      for (u32 z : {0u, 16u}) cube.push_back({x, y, z});
  return {
      {"carre", {{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}}, 2, 5},
      {"rectangle", {{3, 3, 4}, {3, 4, 3}, {5, 5, 4}, {5, 4, 5}}, 1, 4},
      {"translation", {{0, 1, 1}, {1, 0, 1}, {1, 1, 0}}, 1, 4},
      {"triangle_long", {{kCoordMax, 0, 0}, {0, kCoordMax, 0}, {0, 0, kCoordMax}}, 2, 5},
      {"cube_et_centre", cube, 5, 8},
      {"coquille24", shell({1, 2, 2}, 1u << 18, 1u << 19), 2, 16},
      {"coquille48", shell({1, 2, 3}, 1u << 18, 3u << 18), 5, 24},
      {"site_seul", {{5, 5, 5}}, 3, 6},
      {"nuage2500", scatter(2500, 65535, 7), 5, 24},
  };
}

// Diagnostics physiques de la voie appareil comptes comme ceux de la voie CPU : paliers des feuilles, etendue maximale,
// reecritures (feuille de plus de 64 emissions rejouee par la meme source : sur l'appareil ou dans la reprise de
// l'hote) ; reprises sur l'hote par cause (plus de 32 sites : warp virtuel ; etendue au-dela de 16 : politique exacte).
void check_physical(const CatalogueDiagnostics& got, const CatalogueDiagnostics& ref) {
  CHECK_EQ(got.leaves_narrow, ref.leaves_narrow);
  CHECK_EQ(got.leaves_medium, ref.leaves_medium);
  CHECK_EQ(got.leaves_wide, ref.leaves_wide);
  CHECK_EQ(got.max_leaf_span, ref.max_leaf_span);
  CHECK_EQ(got.leaves_rewritten, ref.leaves_rewritten);
  CHECK_EQ(got.leaves_rewritten, got.rewritten_device + got.rewritten_host);
  CHECK_EQ(got.replayed_wide, ref.leaves_virtual_warp);
  CHECK_EQ(got.replayed_span, ref.leaves_exact);
  CHECK(got.replayed_leaves <= got.replayed_wide + got.replayed_span);
  CHECK(got.replayed_leaves >= std::max(got.replayed_wide, got.replayed_span));
}

// Durees publiees disjointes (CST-0235) : etapes nettes des transferts, transferts a part ; leur somme tient dans la
// duree murale de l'appel.
u64 parts_of(const CatalogueDiagnostics& d) {
  return d.traversal_ns + d.count_ns + d.fill_ns + d.levels_ns + d.sort_ns + d.assemble_ns + d.table_ns +
         d.transfer_ns + d.publish_ns;
}

}  // namespace

// Temoins : voie appareil (executeur Pool) identique a la voie CPU ; reprises et reecritures comptees ; transferts.
MHGP12_TEST(pipeline_witnesses, 130) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = sched::make_pool({2});
  REQUIRE(pool.ok());
  u64 replayed = 0, wide = 0, span = 0, rewritten_device = 0, rewritten_host = 0;
  for (const Case& c : witnesses()) {
    auto cloud = make_cloud(c.points, budget);
    REQUIRE(cloud.ok());
    const CatalogueParams params = params_of(c.k, c.leaf);
    CatalogueDiagnostics ref_diag;
    auto ref = cpu(cloud.value(), params, 2, budget, &ref_diag);
    dev::DeviceState<PoolExecutor> state;
    CatalogueDiagnostics diag;
    const Stopwatch wall;
    auto got = host_device(cloud.value(), params, *pool.value(), budget, state, &diag);
    const u64 wall_ns = wall.nanoseconds();
    REQUIRE(ref.ok() && got.ok());
    if (!CHECK(same(cloud.value(), ref.value(), got.value()))) std::fprintf(stderr, "ecart : %s\n", c.name);
    check_physical(diag, ref_diag);
    CHECK(diag.transfer_h2d_bytes >= 3 * sizeof(u32) * cloud.value().sites() && diag.transfer_ops >= 3);
    CHECK(parts_of(diag) <= wall_ns);
    replayed += diag.replayed_leaves;
    wide += diag.replayed_wide;
    span += diag.replayed_span;
    rewritten_device += diag.rewritten_device;
    rewritten_host += diag.rewritten_host;
    if (std::string_view(c.name) == "triangle_long")
      CHECK(diag.replayed_leaves >= 1 && diag.replayed_span >= 1 && diag.leaves_exact == 1);
    if (std::string_view(c.name) == "coquille48")
      CHECK(diag.replayed_leaves >= 8 && diag.replayed_wide == 8 && diag.rewritten_host >= 1);
  }
  CHECK(replayed >= 9);
  CHECK(wide >= 8 && span >= 1);  // les deux causes de reprise
  CHECK(rewritten_device >= 1);   // reecriture sur l'appareil (Replay), comptee
  CHECK(rewritten_host >= 1);     // reecriture dans la reprise de l'hote, comptee
  Points sphere;
  for (int x = -8; x <= 8; ++x)
    for (int y = -8; y <= 8; ++y)
      for (int z = -8; z <= 8; ++z)
        if (x * x + y * y + z * z == 50)
          sphere.push_back({static_cast<u32>(x + 8), static_cast<u32>(y + 8), static_cast<u32>(z + 8)});
  auto cloud = make_cloud(sphere, budget);
  REQUIRE(cloud.ok());
  dev::DeviceState<PoolExecutor> state;
  const u64 before = budget.used();
  auto refused = host_device(cloud.value(), params_of(2, 8), *pool.value(), budget, state);
  CHECK(!refused.ok() && refused.outcome().reason == Reason::shell_capacity);
  auto bad = host_device(cloud.value(), params_of(0, 8), *pool.value(), budget, state);
  CHECK(!bad.ok() && bad.outcome().reason == Reason::kmax_out_of_range);
  state = dev::DeviceState<PoolExecutor>{};
  CHECK_EQ(budget.used(), before);
}

// Etat resident reutilise par des nuages de tailles differentes, et memes octets a 1, 3 et 8 fils.
MHGP12_TEST(pipeline_resident, 10) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const std::vector<Case> cases = witnesses();
  dev::DeviceState<PoolExecutor> state;
  for (const char* name : {"nuage2500", "carre", "triangle_long", "nuage2500"}) {
    for (const Case& c : cases) {
      if (std::string_view(c.name) != name) continue;
      auto cloud = make_cloud(c.points, budget);
      REQUIRE(cloud.ok());
      auto pool = sched::make_pool({3});
      REQUIRE(pool.ok());
      auto ref = cpu(cloud.value(), params_of(c.k, c.leaf), 3, budget);
      auto got = host_device(cloud.value(), params_of(c.k, c.leaf), *pool.value(), budget, state);
      REQUIRE(ref.ok() && got.ok());
      CHECK(same(cloud.value(), ref.value(), got.value()));
    }
  }
  auto cloud = make_cloud(scatter(2500, 65535, 7), budget);
  REQUIRE(cloud.ok());
  std::vector<io::Digest> digests;
  for (u32 threads : {1u, 3u, 8u}) {
    auto pool = sched::make_pool({threads});
    REQUIRE(pool.ok());
    dev::DeviceState<PoolExecutor> fresh;
    auto got = host_device(cloud.value(), params_of(5, 24), *pool.value(), budget, fresh);
    REQUIRE(got.ok());
    auto digest = catalogue_digest(cloud.value(), got.value(), "fils");
    REQUIRE(digest.ok());
    digests.push_back(digest.value());
  }
  CHECK(digests[0] == digests[1] && digests[0] == digests[2]);
}

// Vraie voie appareil : sans GPU (ou construction sans CUDA) device_unavailable ; avec GPU, la voie appareil rend le
// meme Catalogue que la voie CPU sur les temoins (dont les feuilles rejouees), deux appels de suite identiques, et le
// contexte rend son budget a sa destruction.
MHGP12_TEST(device_open, 1) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto device = CatalogueDevice::open(budget);
  if (!device.ok()) {
    CHECK(device.outcome().reason == Reason::device_unavailable);
    std::printf("device_open : appareil indisponible (device_unavailable), voie appareil non jouee\n");
    return;
  }
  auto pool = sched::make_pool({4});
  REQUIRE(pool.ok());
  for (const Case& c : witnesses()) {
    auto cloud = make_cloud(c.points, budget);
    REQUIRE(cloud.ok());
    CatalogueDiagnostics ref_diag, first_diag, second_diag;
    auto ref = cpu(cloud.value(), params_of(c.k, c.leaf), 4, budget, &ref_diag);
    auto first = build_catalogue_device(cloud.value(), params_of(c.k, c.leaf), device.value(), *pool.value(), &first_diag);
    auto second = build_catalogue_device(cloud.value(), params_of(c.k, c.leaf), device.value(), *pool.value(), &second_diag);
    REQUIRE(ref.ok() && first.ok() && second.ok());
    if (!CHECK(same(cloud.value(), ref.value(), first.value()))) std::fprintf(stderr, "ecart appareil : %s\n", c.name);
    CHECK(same(cloud.value(), first.value(), second.value()));
    check_physical(first_diag, ref_diag);
    check_physical(second_diag, ref_diag);  // etat resident : compteurs de chaque appel, jamais cumules
  }
  std::printf("device_open : voie appareil jouee sur %zu temoins\n", witnesses().size());
}
