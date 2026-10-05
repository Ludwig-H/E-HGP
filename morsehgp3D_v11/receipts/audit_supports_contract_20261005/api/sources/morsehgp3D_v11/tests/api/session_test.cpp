// Portes de la Session, du calcul et de la publication de l'api (tests/api/tests.cmake, mhgp11_api_session_*) :
//   released      : calcul sur les fixtures gravees a W1 et W4, publication d'un dossier complet, budget revenu a zero
//                   apres liberation du produit, close conforme ;
//   product_alive : close rend budget_not_released (invariant viole, code 3) tant qu'un produit ou un tampon de la
//                   Session vit, puis conforme apres liberation ;
//   refusals      : refus de Session::make et de compute dans l'ordre du paragraphe 5, sans reservation ni rapport ;
//   equivalence   : les forets de compute sont celles de build_full aux parametres par defaut (meme objet), a W1, W2
//                   et W4 ;
//   tree_digest   : tree_k_sha256 egal a une serialisation independante, invariant par permutation et reetiquetage ;
//   provenance    : forme des decimaux recopies, refus avant toute creation de fichier, recopie dans le manifeste.
#include <array>
#include <optional>
#include <string>

#include "api/internal.hpp"
#include "api_support.hpp"
#include "cloud/cloud.hpp"
#include "index/index.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::api_test;

namespace {

api::Session session_of(u32 workers, u64 budget = MemoryBudget::kUnlimited) {
  Result<api::Session> made = api::Session::make({budget, workers});
  if (!made.ok()) std::terminate();
  return std::move(made).take();
}

// Tour de reference : chemin sequentiel, parametres par defaut du catalogue et des forets (aucune option rapide).
std::optional<FullTower> reference_tower(const Points& p, Order k, MemoryBudget& budget) {
  auto cloud = prepare_cloud(p.x, p.y, p.z, p.ids, CoordWidth(), budget);
  if (!cloud.ok()) return std::nullopt;
  auto index = build_index(std::move(cloud.value()), {}, budget);
  if (!index.ok()) return std::nullopt;
  CatalogueParams params;
  params.kmax = k;
  auto domain = prepare_full_domain(std::move(index.value()), params, budget);
  if (!domain.ok()) return std::nullopt;
  auto tower = build_full(std::move(domain.value()), budget);
  if (!tower.ok()) return std::nullopt;
  return std::move(tower).take();
}

// Serialisation independante de tree_k_sha256 (paragraphe 6.6), octet par octet.
io::Digest digest_by_hand(const OrderForest& f) {
  std::vector<u8> bytes{'M', 'H', 'G', 'P', '1', '1', 'T', 'K'};
  const auto put = [&](u64 value, int width) {
    for (int b = 0; b < width; ++b) bytes.push_back(static_cast<u8>(value >> (8 * b)));
  };
  put(f.order(), 8);
  put(f.nodes().size(), 8);
  for (const ForestNode& node : f.nodes()) {
    put(idx(node.parent), 4);
    put(idx(node.rank), 4);
    put(node.birth_key, 4);
    put(node.child_count, 4);
  }
  for (const NodeIdx child : f.edges()) put(idx(child), 4);
  io::Sha256 sha;
  sha.update(std::span<const u8>(bytes.data(), bytes.size()));
  return sha.finish();
}

Points permuted_relabeled(const Points& p) {
  Points q;
  for (u32 j = 0; j < p.size(); ++j) {
    const u32 i = (j * 11 + 3) % p.size();  // permutation : pgcd(11, n) = 1 pour toutes les tailles employees
    q.x.push_back(p.x[i]);
    q.y.push_back(p.y[i]);
    q.z.push_back(p.z[i]);
    q.ids.push_back(make_id<PointId>(j == 0 ? kNone : 1000003u * (j + 1)));
  }
  return q;
}

}  // namespace

MHGP11_TEST(released, 370) {
  for (const u32 workers : {1u, 4u}) {
    api::Session session = session_of(workers);
    CHECK_EQ(session.workers(), workers);
    for (const Points& p : fixtures()) {
      for (Order k = 1; k <= std::min<u32>(p.size(), 4); ++k) {
        api::RunReport report;
        auto product = api::compute(session, p.view(), api::FullRequest{k}, &report);
        REQUIRE(product.ok());
        CHECK(product.value().k() == k && product.value().full().domain().index().cloud().sites() == p.size());
        CHECK(session.budget().used() > 0);
        CHECK(report.at(api::Stage::tree).peak_bytes >= session.budget().used());
      }
      CHECK_EQ(session.budget().used(), 0u);
    }
    Scratch s;
    REQUIRE(s.ok());
    const std::string d = s.path("D");
    {
      auto product = api::compute(session, fixtures()[0].view(), api::FullRequest{4});
      REQUIRE(product.ok());
      auto planned = io::OutputDirectory::plan(d.c_str(), {});
      REQUIRE(planned.ok());
      api::RunReport report;
      auto published = api::publish(session, product.value(), planned.value(), api::Provenance{}, &report);
      REQUIRE(published.ok());
      CHECK(planned.value().committed());
      CHECK(published.value() == planned.value().manifest_sha256());
      CHECK(report.at(api::Stage::write).nanoseconds > 0);
    }
    CHECK(entries(d) == std::vector<std::string>({"full.mhgp11ful1", "manifeste.json"}));
    CHECK(!exists(d + ".pending"));
    const std::string manifest = read_text(d + "/manifeste.json");
    CHECK(manifest.rfind("{\"schema\":\"ehgp.v11.output.v1\",\"output\":\"full\",\"status\":\"complete\"", 0) == 0);
    CHECK(!manifest.empty() && manifest.back() == '\n');
    CHECK(session.close().ok());
    CHECK_EQ(session.budget().used(), 0u);
  }
}

MHGP11_TEST(product_alive, 12) {
  api::Session session = session_of(2);
  {
    auto product = api::compute(session, fixtures()[2].view(), api::FullRequest{3});
    REQUIRE(product.ok());
    const Outcome closed = session.close();
    CHECK_EQ(closed.reason, Reason::budget_not_released);
    CHECK_EQ(closed.status(), Status::invariant_violated);
    CHECK_EQ(exit_code(closed), 3);
  }
  CHECK(session.close().ok());
  {
    Buffer<u32> held;
    REQUIRE(held.allocate(16, session.budget()).ok());
    CHECK_EQ(session.close().reason, Reason::budget_not_released);
  }
  CHECK(session.close().ok());
  api::Session moved(std::move(session));
  CHECK(session.close().ok());  // Session deplacee : vide
  CHECK(moved.close().ok());
  CHECK_EQ(moved.workers(), 2u);
  CHECK_EQ(session.workers(), 0u);
}

MHGP11_TEST(refusals, 50) {
  CHECK_EQ(api::Session::make({MemoryBudget::kUnlimited, 0}).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(api::Session::make({MemoryBudget::kUnlimited, 257}).outcome().reason, Reason::parameter_out_of_range);
  api::Session session = session_of(2);
  const Points line = points_of({{0, 0, 0}, {2, 0, 0}, {4, 0, 0}});
  Points ragged = line, far = line, twice = line, repeated = line, mixed = line;
  ragged.y.pop_back();
  far.x[1] = kCoordMax + 1;
  twice.ids[2] = twice.ids[0];
  repeated.x[1] = 0;  // deux points a l'origine : deux sites
  mixed.x[1] = kCoordMax + 1;
  mixed.ids[2] = mixed.ids[0];
  struct Case {
    const Points* points;
    Order k;
    Reason want;
  };
  const Points empty;
  const std::array<Case, 11> cases = {{{&line, 0, Reason::parameter_out_of_range},
                                       {&line, 13, Reason::parameter_out_of_range},
                                       {&empty, 1, Reason::empty_input},
                                       {&ragged, 1, Reason::size_mismatch},
                                       {&far, 1, Reason::coordinate_out_of_domain},
                                       {&twice, 1, Reason::duplicate_point_id},
                                       {&mixed, 1, Reason::coordinate_out_of_domain},
                                       {&repeated, 1, Reason::multiplicity_unsupported},
                                       {&repeated, 3, Reason::multiplicity_unsupported},  // avant K > sites
                                       {&line, 4, Reason::parameter_out_of_range},
                                       {&far, 0, Reason::parameter_out_of_range}}};  // K avant le nuage
  for (const Case& c : cases) {
    api::RunReport report;
    report.at(api::Stage::cloud).nanoseconds = 12345;
    auto refused = api::compute(session, c.points->view(), api::FullRequest{c.k}, &report);
    CHECK(!refused.ok());
    CHECK_EQ(refused.outcome().reason, c.want);
    CHECK_EQ(session.budget().used(), 0u);
    CHECK_EQ(report.at(api::Stage::cloud).nanoseconds, 12345u);  // rapport intact sur refus
  }
  // Budget trop petit : memory_budget, sans reservation laissee ; la Session reste utilisable.
  api::Session tight = session_of(1, 4096);
  auto starved = api::compute(tight, random_points(40, 64, 3).view(), api::FullRequest{3});
  CHECK(!starved.ok() && starved.outcome().reason == Reason::memory_budget);
  CHECK_EQ(tight.budget().used(), 0u);
  CHECK(tight.close().ok());
  CHECK(session.close().ok());
}

MHGP11_TEST(equivalence, 217) {
  std::vector<Points> clouds = fixtures();
  clouds.push_back(random_points(60, 9, 11));       // petite boite : cospheriques et coquilles etendues
  clouds.push_back(random_points(60, kCoordMax + 1, 5));  // position generique sur tout le domaine
  api::Session one = session_of(1), two = session_of(2), four = session_of(4);
  MemoryBudget reference_budget(MemoryBudget::kUnlimited);
  for (const Points& p : clouds) {
    for (Order k = 1; k <= std::min<u32>(p.size(), 5); ++k) {
      const auto reference = reference_tower(p, k, reference_budget);
      REQUIRE(reference.has_value());
      auto a = api::compute(one, p.view(), api::FullRequest{k});
      auto b = api::compute(two, p.view(), api::FullRequest{k});
      auto c = api::compute(four, p.view(), api::FullRequest{k});
      REQUIRE(a.ok() && b.ok() && c.ok());
      CHECK(same_tower(a.value().full(), *reference));
      CHECK(same_tower(b.value().full(), *reference) && same_tower(c.value().full(), *reference));
    }
  }
  CHECK(one.close().ok() && two.close().ok() && four.close().ok());
}

MHGP11_TEST(tree_digest, 165) {
  api::Session session = session_of(2);
  std::vector<Points> clouds = fixtures();
  clouds.push_back(random_points(40, 7, 17));
  for (const Points& p : clouds) {
    std::vector<io::Digest> per_order;
    for (Order k = 1; k <= std::min<u32>(p.size(), 4); ++k) {
      auto a = api::compute(session, p.view(), api::FullRequest{k});
      auto b = api::compute(session, permuted_relabeled(p).view(), api::FullRequest{k});
      REQUIRE(a.ok() && b.ok());
      const OrderForest& forest = a.value().full().order(k);
      const io::Digest digest = api::tree_k_sha256(forest);
      CHECK(digest == digest_by_hand(forest));
      CHECK(digest == api::tree_k_sha256(b.value().full().order(k)));
      per_order.push_back(digest);
    }
    for (std::size_t i = 1; i < per_order.size(); ++i) CHECK(per_order[i] != per_order[i - 1]);
  }
  CHECK(session.close().ok());
}

MHGP11_TEST(provenance, 46) {
  for (const char* good : {"0.001", "1", "12.5", "0.10", "3"}) CHECK(api::valid_grid_step(good));
  for (const char* bad : {"0", "0.0", "", ".5", "5.", "-1", "+1", "1e-3", "1,5", "1.2.3", " 1", "1 "})
    CHECK(!api::valid_grid_step(bad));
  CHECK(api::valid_grid_step(std::string(64, '7')));
  CHECK(!api::valid_grid_step(std::string(65, '7')));
  for (const char* good : {"-12.5", "0", "-0", "79602", "1.000"}) CHECK(api::valid_origin_coordinate(good));
  for (const char* bad : {"", "-", "+1", "--1", "1.", ".1", "1e3", "1,2"}) CHECK(!api::valid_origin_coordinate(bad));
  CHECK(api::valid_origin_coordinate("-" + std::string(63, '1')));
  CHECK(!api::valid_origin_coordinate("-" + std::string(64, '1')));

  api::Session session = session_of(1);
  Scratch s;
  REQUIRE(s.ok());
  auto product = api::compute(session, fixtures()[1].view(), api::FullRequest{2});
  REQUIRE(product.ok());
  const std::string d = s.path("D");
  api::Provenance bad_step, partial, good;
  bad_step.grid_step = "1e-3";
  partial.origin = {"1", "", ""};
  good.grid_step = "0.001";
  good.origin = {"-1.5", "2", "0"};
  good.budget_bytes = 123456789;
  for (const api::Provenance* p : {&bad_step, &partial}) {
    auto planned = io::OutputDirectory::plan(d.c_str(), {});
    REQUIRE(planned.ok());
    auto refused = api::publish(session, product.value(), planned.value(), *p);
    CHECK(!refused.ok() && refused.outcome().reason == Reason::parameter_out_of_range);
    CHECK(!exists(d) && !exists(d + ".pending"));
  }
  {
    auto planned = io::OutputDirectory::plan(d.c_str(), {});
    REQUIRE(planned.ok());
    CHECK(api::publish(session, product.value(), planned.value(), good).ok());
  }
  const std::string manifest = read_text(d + "/manifeste.json");
  CHECK(manifest.find("\"parameters\":{\"budget_bytes\":123456789,\"grid_step\":\"0.001\","
                      "\"origin\":[\"-1.5\",\"2\",\"0\"]}") != std::string::npos);
  CHECK(manifest.find("\"k\":2,") != std::string::npos);
}

MHGP11_TEST_MAIN()
