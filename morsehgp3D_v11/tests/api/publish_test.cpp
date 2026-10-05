// Portes de la publication et des parametres du moteur de l'api (tests/api/tests.cmake, mhgp11_api_session_*) :
//   after_publish : publish rend published_complete et l'empreinte du manifeste de D ; un D apparu avant le commit
//                   rend output_conflict, none et aucune empreinte ; finish (fin d'appel, docs/SORTIES.md,
//                   paragraphe 3, etape 9) avec un produit vivant rend budget_not_released (code 3) et retire le
//                   dossier (none) ; si un D.pending apparu empeche le retrait, D reste publie et complet :
//                   published_complete et l'empreinte de son manifeste (double echec, paragraphe 9) ; withdraw sans
//                   publication rend none ;
//   session_identity : un produit calcule par une Session A et publie par une Session B (budget nul) est refuse
//                   parameter_out_of_range avant toute creation de sortie et toute ecriture du rapport ; publie par A
//                   deplacee (jeton stable au deplacement), il passe (audit general a65903a7b, P1, fixture
//                   receipts/audit_geant_20261005/native/api_session_identity.cpp) ;
//   engine        : les parametres fixes du moteur sont ceux du masque 16379 des sondes, decode comme
//                   bench/full_probe.cpp (main), champ par champ pour K = 1 a 12 ; et compute les emploie : pics
//                   reserves de chaque etage egaux a ceux du meme enchainement aux parametres decodes, a W1, sur des
//                   nuages ou les parametres par defaut donnent un autre pic (temoin de sensibilite).
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

#include "api/internal.hpp"
#include "api_support.hpp"
#include "cloud/cloud.hpp"
#include "index/index.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::api_test;

namespace {

io::Digest file_digest(const std::string& path) {
  const std::string text = read_text(path);
  io::Sha256 sha;
  sha.update(std::string_view(text));
  return sha.finish();
}

const std::vector<std::string> kPublished = {"full.mhgp11ful1", "manifeste.json"};

// Parametres du masque des sondes, decodes comme bench/full_probe.cpp (main), avec les arguments que l'api leur
// donne : K, feuilles de 16 a 256 sites, max_nodes 0, ball_limit kNone.
struct EngineParams {
  CatalogueParams catalogue;
  FullParams full;
};

EngineParams decode_mask(u64 mask, Order k) {
  EngineParams e;
  e.full = FullParams{(mask & 4) != 0 ? u64{65536} : u64{0}};
  if ((mask & 8) != 0) {
    e.full.regular_batch_capacity = 4096;
    e.full.descent_lanes = 48;
    e.full.lane_memo_capacity = (mask & 4) != 0 ? u64{4096} : u64{0};
  }
  e.full.parallel_verticals = (mask & 128) != 0;
  e.full.reuse_census_workspace = (mask & 256) != 0;
  e.full.dense_birth_lookup = (mask & 512) != 0;
  e.full.reuse_regular_verticals = (mask & 1024) != 0;
  e.full.population_lookup = (mask & 4096) != 0;
  e.full.concurrent_orders = (mask & 8192) != 0;
  e.catalogue.cache_center_lines = (mask & 1) != 0;
  e.catalogue.indirect_sort = (mask & 2) != 0;
  e.catalogue.adaptive_frontier = (mask & 16) != 0;
  e.catalogue.parallel_assembly = (mask & 32) != 0;
  e.catalogue.single_pass = (mask & 64) != 0;
  e.catalogue.pair_graph = (mask & 2048) != 0;
  e.catalogue.device_leaf = (mask & 16384) != 0;
  e.catalogue.batch_leaves = (mask & 32768) != 0;
  e.catalogue.cuda_leaves = (mask & 65536) != 0;
  e.catalogue.kmax = static_cast<int>(k);
  e.catalogue.leaf_size = 16;
  e.catalogue.max_leaf = 256;
  e.catalogue.max_nodes = 0;
  e.catalogue.ball_limit = kNone;
  return e;
}

void check_same(const CatalogueParams& got, const CatalogueParams& want) {
  CHECK_EQ(got.kmax, want.kmax);
  CHECK_EQ(got.leaf_size, want.leaf_size);
  CHECK_EQ(got.max_leaf, want.max_leaf);
  CHECK_EQ(got.max_nodes, want.max_nodes);
  CHECK_EQ(got.ball_limit, want.ball_limit);
  CHECK_EQ(got.cache_center_lines, want.cache_center_lines);
  CHECK_EQ(got.indirect_sort, want.indirect_sort);
  CHECK_EQ(got.adaptive_frontier, want.adaptive_frontier);
  CHECK_EQ(got.parallel_assembly, want.parallel_assembly);
  CHECK_EQ(got.single_pass, want.single_pass);
  CHECK_EQ(got.pair_graph, want.pair_graph);
  CHECK_EQ(got.device_leaf, want.device_leaf);
  CHECK_EQ(got.batch_leaves, want.batch_leaves);
  CHECK_EQ(got.cuda_leaves, want.cuda_leaves);
}

void check_same(const FullParams& got, const FullParams& want) {
  CHECK_EQ(got.memo_capacity, want.memo_capacity);
  CHECK_EQ(got.regular_batch_capacity, want.regular_batch_capacity);
  CHECK_EQ(got.descent_lanes, want.descent_lanes);
  CHECK_EQ(got.lane_memo_capacity, want.lane_memo_capacity);
  CHECK_EQ(got.parallel_verticals, want.parallel_verticals);
  CHECK_EQ(got.reuse_census_workspace, want.reuse_census_workspace);
  CHECK_EQ(got.dense_birth_lookup, want.dense_birth_lookup);
  CHECK_EQ(got.reuse_regular_verticals, want.reuse_regular_verticals);
  CHECK_EQ(got.population_lookup, want.population_lookup);
  CHECK_EQ(got.concurrent_orders, want.concurrent_orders);
}

struct StagePeaks {
  u64 cloud = 0, index = 0, domain = 0, tree = 0;
};

// Meme enchainement que compute (prepare_cloud, build_index, prepare_full_domain sur le Pool, build_full) avec les
// parametres donnes ; pic reserve lu apres chaque etage, comme le pilote de compute (restart_peak).
std::optional<StagePeaks> reference_peaks(const Points& p, const EngineParams& e, sched::Pool& pool) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  StagePeaks peaks;
  budget.restart_peak();
  auto cloud = prepare_cloud(p.x, p.y, p.z, p.ids, CoordWidth(), budget);
  if (!cloud.ok()) return std::nullopt;
  peaks.cloud = budget.restart_peak();
  auto index = build_index(std::move(cloud.value()), {}, budget);
  if (!index.ok()) return std::nullopt;
  peaks.index = budget.restart_peak();
  auto domain = prepare_full_domain(std::move(index.value()), e.catalogue, budget, pool);
  if (!domain.ok()) return std::nullopt;
  peaks.domain = budget.restart_peak();
  auto tower = build_full(std::move(domain.value()), budget, nullptr, e.full, &pool);
  if (!tower.ok()) return std::nullopt;
  peaks.tree = budget.restart_peak();
  return peaks;
}

}  // namespace

MHGP11_TEST(after_publish, 54) {
  Scratch s;
  REQUIRE(s.ok());
  api::Session session = session_of(2);
  const Points points = fixtures()[2];  // growth_ABCZ
  io::Digest first{};
  {  // publication conforme, puis fin d'appel conforme
    const std::string d = s.path("A");
    auto planned = io::OutputDirectory::plan(d.c_str(), {});
    REQUIRE(planned.ok());
    {
      auto product = api::compute(session, points.view(), api::FullRequest{3});
      REQUIRE(product.ok());
      const api::Publication published = api::publish(session, product.value(), planned.value(), provenance_of(points));
      CHECK(published.ok());
      CHECK(published.state == api::PublicationState::published_complete);
      CHECK(published.manifest_sha256 == file_digest(d + "/manifeste.json"));
      first = published.manifest_sha256;
    }
    const api::Publication finished = api::finish(session, planned.value());
    CHECK(finished.ok());
    CHECK(finished.state == api::PublicationState::published_complete);
    CHECK(finished.manifest_sha256 == first);
    CHECK(planned.value().committed());
  }
  CHECK(entries(s.path("A")) == kPublished);
  CHECK(!exists(s.path("A.pending")));
  {  // D apparu avant le commit : refus sans publication, aucune empreinte rendue
    const std::string d = s.path("B");
    auto planned = io::OutputDirectory::plan(d.c_str(), {});
    REQUIRE(planned.ok());
    auto product = api::compute(session, points.view(), api::FullRequest{3});
    REQUIRE(product.ok());
    REQUIRE(::mkdir(d.c_str(), 0700) == 0);
    const api::Publication refused = api::publish(session, product.value(), planned.value(), provenance_of(points));
    CHECK_EQ(refused.outcome.reason, Reason::output_conflict);
    CHECK(refused.state == api::PublicationState::none);
    CHECK(refused.manifest_sha256 == io::Digest{});
    CHECK(!planned.value().committed());
  }
  CHECK(entries(s.path("B")).empty());
  CHECK(!exists(s.path("B.pending")));
  {  // fin d'appel avec un produit vivant : budget_not_released, code 3, dossier publie retire
    const std::string d = s.path("C");
    auto planned = io::OutputDirectory::plan(d.c_str(), {});
    REQUIRE(planned.ok());
    auto product = api::compute(session, points.view(), api::FullRequest{3});
    REQUIRE(product.ok());
    REQUIRE(api::publish(session, product.value(), planned.value(), provenance_of(points)).ok());
    CHECK(exists(d));
    const api::Publication finished = api::finish(session, planned.value());
    CHECK_EQ(finished.outcome.reason, Reason::budget_not_released);
    CHECK_EQ(exit_code(finished.outcome), 3);
    CHECK(finished.state == api::PublicationState::none);
    CHECK(finished.manifest_sha256 == io::Digest{});
    CHECK(!planned.value().committed());
    CHECK(!exists(d));
  }
  CHECK(!exists(s.path("C")) && !exists(s.path("C.pending")));
  CHECK(session.close().ok());
  {  // meme fin d'appel, retrait empeche par un D.pending apparu : D reste publie et complet (double echec)
    const std::string d = s.path("E");
    auto planned = io::OutputDirectory::plan(d.c_str(), {});
    REQUIRE(planned.ok());
    auto product = api::compute(session, points.view(), api::FullRequest{3});
    REQUIRE(product.ok());
    const api::Publication published = api::publish(session, product.value(), planned.value(), provenance_of(points));
    REQUIRE(published.ok());
    REQUIRE(::mkdir((d + ".pending").c_str(), 0700) == 0);
    const api::Publication finished = api::finish(session, planned.value());
    CHECK_EQ(finished.outcome.reason, Reason::budget_not_released);
    CHECK_EQ(exit_code(finished.outcome), 3);
    CHECK(finished.state == api::PublicationState::published_complete);
    CHECK(finished.manifest_sha256 == published.manifest_sha256);
    CHECK(finished.manifest_sha256 == file_digest(d + "/manifeste.json"));
    CHECK(planned.value().committed());
  }
  CHECK(entries(s.path("E")) == kPublished);  // le destructeur ne touche jamais un dossier publie
  CHECK(exists(s.path("E.pending")) && entries(s.path("E.pending")).empty());  // ni un D.pending qu'il n'a pas cree
  CHECK(session.close().ok());
  {  // retrait sans rien de publie : none, refus inchange
    auto planned = io::OutputDirectory::plan(s.path("F").c_str(), {});
    REQUIRE(planned.ok());
    const api::Publication withdrawn = api::withdraw(fail(Reason::output_unwritable), planned.value());
    CHECK_EQ(withdrawn.outcome.reason, Reason::output_unwritable);
    CHECK(withdrawn.state == api::PublicationState::none);
    CHECK(withdrawn.manifest_sha256 == io::Digest{});
  }
  CHECK(!exists(s.path("F")) && !exists(s.path("F.pending")));
  CHECK_EQ(api::publication_state_name(api::PublicationState::none), std::string_view("none"));
  CHECK_EQ(api::publication_state_name(api::PublicationState::published_complete),
           std::string_view("published_complete"));
  CHECK(session.close().ok());
}

MHGP11_TEST(session_identity, 24) {
  Scratch s;
  REQUIRE(s.ok());
  api::Session a = session_of(1);
  api::Session b = session_of(1, 0);
  const Points points = points_of({{0, 0, 0}, {2, 0, 0}}, 7);
  auto made = api::compute(a, points.view(), api::FullRequest{1});
  REQUIRE(made.ok());
  std::optional<api::Product> product(std::move(made).take());
  CHECK(product->computed_by(a));
  CHECK(!product->computed_by(b));
  CHECK(a.budget().used() != 0);
  CHECK_EQ(b.budget().used(), 0u);
  const std::string d = s.path("D");
  auto planned = io::OutputDirectory::plan(d.c_str(), {});
  REQUIRE(planned.ok());
  api::RunReport report;
  report.at(api::Stage::output) = {11, 22};
  report.at(api::Stage::write) = {33, 44};
  const api::Publication refused = api::publish(b, *product, planned.value(), provenance_of(points), &report);
  CHECK_EQ(refused.outcome.reason, Reason::parameter_out_of_range);
  CHECK(refused.state == api::PublicationState::none);
  CHECK(refused.manifest_sha256 == io::Digest{});
  CHECK(report.at(api::Stage::output).nanoseconds == 11 && report.at(api::Stage::output).peak_bytes == 22);
  CHECK(report.at(api::Stage::write).nanoseconds == 33 && report.at(api::Stage::write).peak_bytes == 44);
  CHECK(!planned.value().committed());
  CHECK(!exists(d) && !exists(d + ".pending"));
  // Le jeton suit la Session deplacee ; la Session vide n'en a plus.
  api::Session moved(std::move(a));
  CHECK(product->computed_by(moved));
  CHECK(!product->computed_by(a));
  const api::Publication published = api::publish(moved, *product, planned.value(), provenance_of(points), &report);
  CHECK(published.ok());
  CHECK(published.state == api::PublicationState::published_complete);
  CHECK(entries(d) == kPublished);
  CHECK(report.at(api::Stage::write).peak_bytes != 44);
  CHECK_EQ(moved.close().reason, Reason::budget_not_released);
  product.reset();
  CHECK(moved.close().ok());
  CHECK(b.close().ok());
  CHECK(a.close().ok());
}

MHGP11_TEST(engine, 347) {
  CHECK_EQ(api_detail::kEngineMask, 16379u);  // masque qualifie des sondes (docs/SORTIES.md, paragraphe 1)
  for (Order k = 1; k <= api::kMaxOrder; ++k) {
    const EngineParams want = decode_mask(api_detail::kEngineMask, k);
    check_same(api_detail::catalogue_params(k), want.catalogue);
    check_same(api_detail::full_params(), want.full);
  }
  auto pool = sched::make_pool({1});
  REQUIRE(pool.ok());
  api::Session session = session_of(1);
  std::vector<Points> clouds = {fixtures()[7], random_points(60, 9, 11), random_points(200, 64, 5)};
  u32 sensitive = 0, compared = 0;
  for (const Points& p : clouds) {
    for (const Order k : {Order{1}, Order{3}, Order{5}}) {
      if (k > p.size()) continue;
      api::RunReport report;
      {
        auto product = api::compute(session, p.view(), api::FullRequest{k}, &report);
        REQUIRE(product.ok());
      }
      const EngineParams mask = decode_mask(api_detail::kEngineMask, k);
      const auto want = reference_peaks(p, mask, *pool.value());
      EngineParams defaults = mask;
      defaults.full = FullParams{};
      const auto slow = reference_peaks(p, defaults, *pool.value());
      REQUIRE(want.has_value() && slow.has_value());
      CHECK_EQ(report.at(api::Stage::cloud).peak_bytes, want->cloud);
      CHECK_EQ(report.at(api::Stage::index).peak_bytes, want->index);
      CHECK_EQ(report.at(api::Stage::domain).peak_bytes, want->domain);
      CHECK_EQ(report.at(api::Stage::tree).peak_bytes, want->tree);
      sensitive += slow->tree != want->tree ? 1u : 0u;
      ++compared;
    }
  }
  std::printf("engine comparaisons=%u sensibles=%u\n", compared, sensitive);
  CHECK(compared >= 8);
  CHECK(sensitive >= 1);  // les parametres par defaut se voient au pic de l'etage tree : la comparaison n'est pas vide
  CHECK(session.close().ok());
}
