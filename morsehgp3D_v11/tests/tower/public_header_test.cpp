// Parapluie public de tower : "tower/tower.hpp" seul suffit a atteindre la tour FULL, la MEB bornee et l'arbre
// d'ordre K seul, sans nommer tower_detail ; les noms publics de mhgp11 designent les memes entites que tower_detail
// (aucun code deplace).
#include <type_traits>
#include <vector>

#include "test.hpp"
#include "tower/tower.hpp"

namespace {
using namespace mhgp11;
static_assert(std::is_same_v<ForestNode, tower_detail::ForestNode>);
static_assert(std::is_same_v<ForestLedger, tower_detail::ForestLedger>);
static_assert(std::is_same_v<OrderForest, tower_detail::OrderForest>);
static_assert(std::is_same_v<FullTower, tower_detail::FullTower>);
static_assert(std::is_same_v<FullParams, tower_detail::FullParams>);
static_assert(std::is_same_v<FullTimings, tower_detail::FullTimings>);
static_assert(std::is_same_v<OrderTimings, tower_detail::OrderTimings>);
static_assert(std::is_same_v<OrderTree, tower_detail::OrderTree>);
static_assert(std::is_same_v<WindowAttachment, tower_detail::WindowAttachment>);
static_assert(std::is_same_v<BallRole, tower_detail::BallRole>);

Result<FullDomain> square(MemoryBudget& budget) {
  const std::vector<u32> x{0, 2, 2, 0}, y{0, 0, 2, 2}, z{0, 0, 0, 0};
  const std::vector<PointId> ids{PointId{7}, PointId{3}, PointId{11}, PointId{5}};
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth{}, budget);
  if (!cloud.ok()) return cloud.outcome();
  auto index = build_index(std::move(cloud.value()), IndexParams{2}, budget);
  if (!index.ok()) return index.outcome();
  CatalogueParams params;
  params.kmax = 4;
  return prepare_full_domain(std::move(index.value()), params, budget);
}
}  // namespace

// Pointeur de fonction public = fonction de tower_detail ; carre a K=1..4 par les seuls noms publics.
MHGP11_TEST(umbrella, 19) {
  CHECK(&mhgp11::build_full == &mhgp11::tower_detail::build_full);
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = square(owner); REQUIRE(domain.ok());
  FullTimings timings;
  auto tower = build_full(std::move(domain.value()), work, &timings, FullParams{}); REQUIRE(tower.ok());
  const FullTower& full = tower.value();
  CHECK_EQ(full.kmax(), Order{4});
  CHECK_EQ(full.domain().index().cloud().sites(), 4u);
  for (Order k = 1; k <= full.kmax(); ++k) {
    const OrderForest& forest = full.order(k);
    const ForestNode& root = forest.nodes()[idx(forest.root())];
    const ForestLedger& ledger = forest.ledger();
    CHECK_EQ(forest.order(), k);
    CHECK(root.parent == NodeIdx{kNone});
    CHECK(ledger.classified_cells >= ledger.replayed_cells);
  }
  const std::vector<SiteIdx> part{SiteIdx{0}, SiteIdx{1}, SiteIdx{2}, SiteIdx{3}};
  auto meb = bounded_meb(full.domain().index().cloud(), part); REQUIRE(meb.ok());
  CHECK_EQ(meb.value().support().size(), std::size_t{2});
}

// Arbre d'ordre K seul par les noms publics (tranche S3) : carre a K=2, quatre naissances et la diagonale en fusion.
MHGP11_TEST(order_tree, 9) {
  CHECK(&mhgp11::build_order == &mhgp11::tower_detail::build_order);
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = square(owner); REQUIRE(domain.ok());
  auto tree = build_order(std::move(domain.value()), 2, work); REQUIRE(tree.ok());
  const OrderTree& order = tree.value();
  const WindowAttachment& attachment = order.attachment();
  CHECK_EQ(order.order(), Order{2});
  CHECK_EQ(order.forest().births(), 4u);
  CHECK_EQ(attachment.size(), 5u);
  u32 births = 0, merges = 0;
  for (BallRole role : attachment.role()) { births += role == BallRole::birth; merges += role == BallRole::merge; }
  CHECK_EQ(births, 4u); CHECK_EQ(merges, 1u);
  CHECK_EQ(attachment.prior().size(), std::size_t{4});
}

MHGP11_TEST_MAIN()
