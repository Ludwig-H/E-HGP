// Portes de la Session, du calcul et de la publication de l'api (tests/api/tests.cmake, mhgp11_api_session_*) :
//   released      : calcul sur les fixtures gravees a W1 et W4, publication d'un dossier complet, budget revenu a zero
//                   apres liberation du produit, close conforme ;
//   product_alive : close rend budget_not_released (invariant viole, code 3) tant qu'un produit ou un tampon de la
//                   Session vit, puis conforme apres liberation ;
//   refusals      : refus de Session::make et de compute dans l'ordre du paragraphe 5, sans reservation ni rapport ;
//   equivalence   : les forets de compute sont celles de build_full aux parametres par defaut (meme objet), a W1, W2
//                   et W4 ;
//   tree_digest   : tree_k_sha256 version 2 egale a une serialisation independante (enfants recalcules depuis les
//                   parents), aux trois valeurs gravees de l'auditeur pour le profil, et au champ publie du manifeste
//                   pour K = 1 a 4 ; invariante par permutation et reetiquetage ;
//   provenance    : forme des decimaux recopies, refus avant toute creation de fichier, recopie dans le manifeste.
// Les groupes after_publish (etat publie, fin d'appel, retrait) et engine (parametres du masque 16379) sont dans
// publish_test.cpp.
#include <array>
#include <optional>
#include <string>
#include <string_view>

#include "api/internal.hpp"
#include "api_support.hpp"
#include "cloud/cloud.hpp"
#include "index/index.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::api_test;

namespace {

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

void put(std::vector<u8>& bytes, u64 value, int width) {
  for (int b = 0; b < width; ++b) bytes.push_back(static_cast<u8>(value >> (8 * b)));
}

io::Digest sha_of(const std::vector<u8>& bytes) {
  io::Sha256 sha;
  sha.update(std::span<const u8>(bytes.data(), bytes.size()));
  return sha.finish();
}

// Serialisation independante de la signature tree_k_sha256 version 2 (docs/SORTIES.md, paragraphe 8), octet par
// octet, comme le modele de l'auditeur (check_d2_signature.py, reponse D.2) : enfants recalcules depuis les parents,
// jamais lus dans la foret ; genre deduit de la cle de naissance et de l'ordre ; naissance en SiteIdx.
io::Digest digest_by_hand(const FullDomain& domain, const OrderForest& f) {
  const Cloud& cloud = domain.index().cloud();
  std::vector<u8> geometry{'M', 'H', 'G', 'P', '1', '1', 'G', 'X'};
  put(geometry, kCoordBits, 8);
  put(geometry, cloud.sites(), 8);
  for (u32 s = 0; s < cloud.sites(); ++s)
    for (const auto& axis : {cloud.x(), cloud.y(), cloud.z()}) put(geometry, axis[s], 4);
  const u32 count = static_cast<u32>(f.nodes().size());
  std::vector<std::vector<u32>> children(count);
  for (u32 i = 0; i < count; ++i)
    if (idx(f.nodes()[i].parent) != kNone) children[idx(f.nodes()[i].parent)].push_back(i);
  std::vector<u8> bytes{'M', 'H', 'G', 'P', '1', '1', 'T', 'K'};
  for (const u64 word : {u64{2}, static_cast<u64>(kCoordBits), u64{f.order()}, u64{cloud.sites()}, u64{count}})
    put(bytes, word, 8);
  const io::Digest shape = sha_of(geometry);
  bytes.insert(bytes.end(), shape.begin(), shape.end());
  for (u32 i = 0; i < count; ++i) {
    const ForestNode& node = f.nodes()[i];
    put(bytes, idx(node.parent), 4);
    put(bytes, idx(node.rank), 4);
    if (node.birth_key == kNone) {
      bytes.insert(bytes.end(), {2, 0});
    } else if (f.order() == 1) {
      bytes.insert(bytes.end(), {0, 1});
      put(bytes, node.birth_key, 4);
    } else {
      const CatalogueBall& ball = domain.catalogue().balls_data()[node.birth_key];
      bytes.insert(bytes.end(), {1, ball.qmin});
      for (u32 j = 0; j < ball.qmin; ++j) put(bytes, idx(ball.support[j]), 4);
    }
    put(bytes, children[i].size(), 4);
    for (const u32 child : children[i]) put(bytes, child, 4);
  }
  return sha_of(bytes);
}

std::string hex_of(const io::Digest& digest) {
  const auto text = io::to_hex(digest);
  return std::string(text.data(), text.size());
}

// Valeur du champ "tree_k_sha256" d'un manifeste (64 chiffres hexadecimaux), vide s'il manque.
std::string published_tree(const std::string& manifest) {
  const std::string key = "\"tree_k_sha256\":\"";
  const std::size_t at = manifest.find(key);
  return at == std::string::npos ? std::string() : manifest.substr(at + key.size(), 64);
}

// Signatures version 2 gravees, par profil. Les trois premieres sont les fixtures de l'auditeur (reponse D.2,
// aef7182b3) : 21 et 24 bits publiees par lui ; 18 bits calculees par son modele (check_d2_signature.py, signature
// avec bits = 18). Les deux suivantes sont des temoins de non-regression : une fusion multiple a K = 1 (carre, quatre
// feuilles sous une fusion) et le temoin D2 de l'auditeur a K = 3 (A, B, C, Z, W : naissances de S* d'arite 2 et 3,
// racine a quatre enfants) ; leur structure a ete extraite de la tour native par la contre-lecture du 5 octobre
// (sig_dump), puis hachee par le modele de l'auditeur pour chaque coord_bits.
struct Engraved {
  Points points;
  Order k;
  std::array<std::string_view, 3> by_profile;  // 18, 21, 24 bits
};

std::vector<Engraved> engraved() {
  return {
      {points_of({{3, 2, 1}}), 1,
       {"c66a89f61f6ae1c86ac97ecf5ee1a7fbd3c8550e5db40af6f5d825a44f1121da",
        "8e991c2147dd5344a2012998e4052bddec912f533989cc8bef0376f26d7d12e6",
        "4ee1bfe286ebb0d15a9b19d649cb7233f83b790e4472d94ba9acf36252070996"}},
      {points_of({{0, 2, 0}, {2, 0, 0}}), 1,
       {"15d33ee089f3fe5303e3b73dbc1d58716032a7c2e32bfffe5ce5ba19872f3af8",
        "4878adc74b9cd5b03f6c5d95e60c95faeb0d4462e785ed01eeff093c214e5c46",
        "eb765ff8c867e7fb001eccb6f618938de063d4d0526aa7b712d12197aea2f863"}},
      {points_of({{0, 0, 0}, {1, 0, 0}, {2, 0, 0}}), 2,
       {"d5161a76bcaf3850956f7f7d2bd5b75768d34d794ccceeea2ca81d3430eac5ea",
        "82f39093f987bd9dccaa10339251072d3c033e7f8dfa4dc34e0c7a17ba20a0a1",
        "e6d408f77aa2acebd8c06e86cc476c515bbf893bd208699657b5d6bd45228961"}},
      {points_of({{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}}), 1,
       {"e5bfbd1781ee606cf7848252df34742db083c4a37fb2239a23d6a2e10b923a92",
        "9da84e033219013487f6e7b265c1b78c077f6dccf98375d21df5b0f316a3e479",
        "2bcd17be84e1e3e0c55040db7df3cd8d1d19c53f23a800277969b3d5962fa470"}},
      {points_of({{2, 10, 0}, {18, 10, 0}, {10, 20, 0}, {9, 3, 0}, {11, 3, 0}}), 3,
       {"83a54d4d1779d5658daf29c6f006df03cef3ccc2fedda9323db6ce4862f34125",
        "4a03cf4a0005c59593fced5e2435d11b8df57f516cde23bdf9528b4b257dafb5",
        "9db8fc5266ea184b289eb3d4cadcb5727d2769ede7055d8762c2520cbaa74702"}},
  };
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

MHGP11_TEST(released, 372) {
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
      const api::Publication published =
          api::publish(session, product.value(), planned.value(), provenance_of(fixtures()[0]), &report);
      REQUIRE(published.ok());
      CHECK(planned.value().committed());
      CHECK(published.state == api::PublicationState::published_complete);
      CHECK(published.manifest_sha256 == planned.value().manifest_sha256());
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

MHGP11_TEST(tree_digest, 314) {
  api::Session session = session_of(2);
  const std::size_t profile = kCoordBits == 18 ? 0 : kCoordBits == 21 ? 1 : 2;
  for (const Engraved& e : engraved()) {
    auto product = api::compute(session, e.points.view(), api::FullRequest{e.k});
    REQUIRE(product.ok());
    const FullTower& tower = product.value().full();
    CHECK_EQ(hex_of(api::tree_k_sha256(tower.domain(), tower.order(e.k))), std::string(e.by_profile[profile]));
    CHECK(digest_by_hand(tower.domain(), tower.order(e.k)) == api::tree_k_sha256(tower.domain(), tower.order(e.k)));
  }
  Scratch s;
  REQUIRE(s.ok());
  std::vector<Points> clouds = fixtures();
  clouds.push_back(random_points(40, 7, 17));
  u32 published = 0;
  for (const Points& p : clouds) {
    std::vector<io::Digest> per_order;
    for (Order k = 1; k <= std::min<u32>(p.size(), 4); ++k) {
      auto a = api::compute(session, p.view(), api::FullRequest{k});
      auto b = api::compute(session, permuted_relabeled(p).view(), api::FullRequest{k});
      REQUIRE(a.ok() && b.ok());
      const FullTower& tower = a.value().full();
      const io::Digest digest = api::tree_k_sha256(tower.domain(), tower.order(k));
      CHECK(digest == digest_by_hand(tower.domain(), tower.order(k)));
      CHECK(digest == api::tree_k_sha256(b.value().full().domain(), b.value().full().order(k)));
      per_order.push_back(digest);
      // Champ publie : le manifeste porte la signature de l'ordre maximal K, calculee a la main.
      const std::string d = s.path("T" + std::to_string(published++));
      auto planned = io::OutputDirectory::plan(d.c_str(), {});
      REQUIRE(planned.ok());
      REQUIRE(api::publish(session, a.value(), planned.value(), provenance_of(p)).ok());
      CHECK_EQ(published_tree(read_text(d + "/manifeste.json")), hex_of(digest));
    }
    for (std::size_t i = 1; i < per_order.size(); ++i) CHECK(per_order[i] != per_order[i - 1]);
  }
  CHECK(published >= 40);
  CHECK(session.close().ok());
}

MHGP11_TEST(provenance, 64) {
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
  // Provenances refusees avant tout fichier : decimaux hors de leur forme, puis incoherences avec le nuage du produit
  // (3 points : 36 et 12 octets) ou un budget declare nul, que le lecteur officiel refuserait (audit abc30ed06).
  const api::Provenance coherent = provenance_of(fixtures()[1]);
  api::Provenance bad_step = coherent, partial = coherent, good = coherent, empty, ratio = coherent, count = coherent,
                  zero_budget = coherent;
  bad_step.grid_step = "1e-3";
  partial.origin = {"1", "", ""};
  good.grid_step = "0.001";
  good.origin = {"-1.5", "2", "0"};
  good.budget_bytes = 123456789;
  ratio.points_bytes = 33;                                   // 11 octets par point
  count.points_bytes = 72, count.ids_bytes = 24;             // rapport 12/4, mais six points
  zero_budget.budget_bytes = 0;
  for (const api::Provenance* p : {&bad_step, &partial, &empty, &ratio, &count, &zero_budget}) {
    auto planned = io::OutputDirectory::plan(d.c_str(), {});
    REQUIRE(planned.ok());
    const api::Publication refused = api::publish(session, product.value(), planned.value(), *p);
    CHECK(!refused.ok() && refused.outcome.reason == Reason::parameter_out_of_range);
    CHECK(refused.state == api::PublicationState::none);
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
