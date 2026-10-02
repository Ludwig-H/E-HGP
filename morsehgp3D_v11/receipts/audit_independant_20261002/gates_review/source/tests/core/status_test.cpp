// Portes de core : types, table des raisons, Outcome (priorite et fusion deterministes), Result, garde de frontiere.
#include <algorithm>
#include <array>
#include <concepts>
#include <new>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "core/core.hpp"
#include "test.hpp"

using namespace mhgp11;

namespace {

// Les identifiants forts sont des types distincts deux a deux : un rang n'est jamais pris pour un identifiant.
static_assert(!std::same_as<PointId, SiteIdx> && !std::same_as<PointId, BallIdx> && !std::same_as<PointId, LevelRank> &&
              !std::same_as<PointId, NodeIdx> && !std::same_as<SiteIdx, BallIdx> && !std::same_as<SiteIdx, LevelRank> &&
              !std::same_as<SiteIdx, NodeIdx> && !std::same_as<BallIdx, LevelRank> && !std::same_as<BallIdx, NodeIdx> &&
              !std::same_as<LevelRank, NodeIdx>);
static_assert(StrongId<PointId> && StrongId<SiteIdx> && StrongId<BallIdx> && StrongId<LevelRank> && StrongId<NodeIdx>);
static_assert(!StrongId<u32> && !StrongId<Status> && !StrongId<Reason> && !StrongId<int>);
static_assert(!std::convertible_to<PointId, u32> && !std::convertible_to<u32, PointId>);
static_assert(kCoordBits == MHGP11_COORD_BITS);

// Copie gravee de la table des raisons, dans l'ordre : toute insertion, suppression ou permutation fait echouer la
// porte ; une raison nouvelle s'ajoute en fin de table ET ici.
struct Row {
  std::string_view name;
  Status status;
};
constexpr std::array<Row, 14> kTable = {{
    {"none", Status::ok},
    {"empty_input", Status::invalid_input},
    {"size_mismatch", Status::invalid_input},
    {"coordinate_out_of_domain", Status::invalid_input},
    {"duplicate_point_id", Status::invalid_input},
    {"parameter_out_of_range", Status::invalid_input},
    {"memory_budget", Status::resource_exhausted},
    {"index_overflow_u32", Status::resource_exhausted},
    {"csr_bounds", Status::invariant_violated},
    {"session_overhead", Status::resource_exhausted},
    {"input_unreadable", Status::invalid_input},
    {"output_unwritable", Status::resource_exhausted},
    {"output_conflict", Status::invalid_input},
    {"refusal_without_reason", Status::invariant_violated},
}};

// Echantillon d'issues : un succes, des refus a plusieurs ordres et de plusieurs raisons, valeurs toutes distinctes.
std::vector<Outcome> sample() {
  return {Outcome{},
          fail(Reason::empty_input),
          fail(Reason::memory_budget),
          fail(Reason::csr_bounds),
          fail(Reason::memory_budget, 2),
          fail(Reason::csr_bounds, 2),
          fail(Reason::empty_input, 3),
          fail(Reason::output_conflict, 10)};
}

Outcome check_positive(int v) {
  MHGP11_CHECK(v > 0, parameter_out_of_range);
  return {};
}

Outcome try_twice(int a, int b, int& reached) {
  MHGP11_TRY(check_positive(a));
  ++reached;
  MHGP11_TRY(check_positive(b));
  ++reached;
  return {};
}

Result<int> half_of(int v) {
  MHGP11_CHECK(v % 2 == 0, parameter_out_of_range);
  return v / 2;
}

}  // namespace

MHGP11_TEST(types, 11) {
  CHECK_EQ(kNone, 4294967295u);
  CHECK_EQ(kCoordBits, MHGP11_COORD_BITS);
  CHECK(kCoordBits == 18 || kCoordBits == 21 || kCoordBits == 24);
  CHECK_EQ(u64{kCoordMax} + 1, u64{1} << kCoordBits);
  CHECK_EQ(sizeof(PointId), 4u);
  CHECK_EQ(sizeof(i128), 16u);
  CHECK_EQ(idx(make_id<PointId>(7)), 7u);
  CHECK_EQ(idx(make_id<SiteIdx>(kNone)), kNone);
  CHECK(make_id<BallIdx>(3) == make_id<BallIdx>(3));
  CHECK(make_id<LevelRank>(0) != make_id<LevelRank>(1));
  CHECK_EQ(idx(NodeIdx{}), 0u);
}

MHGP11_TEST(reasons, 53) {
  REQUIRE(kReasonCount == kTable.size());  // 1
  for (u16 i = 0; i < kReasonCount; ++i) {  // 14 x 3 = 42
    const Reason r = static_cast<Reason>(i);
    CHECK_EQ(reason_name(r), kTable[i].name);
    CHECK_EQ(status_of(r), kTable[i].status);
    CHECK_EQ(status_of(r) == Status::ok, r == Reason::none);  // seule none est un succes
  }
  CHECK_EQ(static_cast<u16>(Reason::none), 0);
  CHECK_EQ(static_cast<u16>(Reason::refusal_without_reason), kReasonCount - 1);
  // hors table : jamais un succes
  CHECK_EQ(status_of(static_cast<Reason>(kReasonCount)), Status::invariant_violated);
  CHECK_EQ(reason_name(static_cast<Reason>(kReasonCount)), "unknown");
  CHECK_EQ(status_name(Status::ok), "ok");
  CHECK_EQ(status_name(Status::invalid_input), "invalid_input");
  CHECK_EQ(status_name(Status::unsupported_degeneracy), "unsupported_degeneracy");
  CHECK_EQ(status_name(Status::resource_exhausted), "resource_exhausted");
  CHECK_EQ(status_name(Status::invariant_violated), "invariant_violated");
  CHECK_EQ(status_name(static_cast<Status>(200)), "unknown");
}

MHGP11_TEST(outcome, 618) {
  // issues elementaires : 9
  CHECK(Outcome{}.ok());
  CHECK_EQ(Outcome{}.status(), Status::ok);
  CHECK(!fail(Reason::memory_budget).ok());
  CHECK_EQ(fail(Reason::memory_budget).status(), Status::resource_exhausted);
  CHECK_EQ(fail(Reason::csr_bounds, 4).order, 4);
  CHECK_EQ(fail(Reason::none).reason, Reason::refusal_without_reason);  // un refus a toujours une raison
  CHECK(!fail(Reason::none, 3).ok());
  CHECK_EQ(fail(Reason::none, 3).order, 3);
  CHECK(fail(Reason::csr_bounds, 2) == fail(Reason::csr_bounds, 2));

  // priorite : 7
  CHECK(fail(Reason::csr_bounds, 2).precedes(fail(Reason::memory_budget, 3)));   // plus petit K d'abord
  CHECK(!fail(Reason::memory_budget, 3).precedes(fail(Reason::csr_bounds, 2)));
  CHECK(fail(Reason::memory_budget, 2).precedes(fail(Reason::csr_bounds, 2)));   // meme K : rang dans la table
  CHECK(!fail(Reason::csr_bounds, 2).precedes(fail(Reason::memory_budget, 2)));
  CHECK(fail(Reason::output_conflict, 10).precedes(Outcome{}));                  // un refus precede un succes
  CHECK(!Outcome{}.precedes(fail(Reason::output_conflict, 10)));
  CHECK(fail(Reason::csr_bounds).precedes(fail(Reason::empty_input, 1)));        // K = 0 (sans objet) d'abord

  // ordre total strict sur l'echantillon : 8 x 8 = 64 paires, puis 8^3 = 512 triples
  const std::vector<Outcome> s = sample();
  for (const Outcome& a : s)
    for (const Outcome& b : s) CHECK_EQ(int{a.precedes(b)} + int{b.precedes(a)} + int{a == b}, 1);
  for (const Outcome& a : s)
    for (const Outcome& b : s)
      for (const Outcome& c : s) CHECK(!(a.precedes(b) && b.precedes(c)) || a.precedes(c));

  // fusion : 8 + 8 + 4 + 1 + 5 = 26
  for (const Outcome& a : s) CHECK(merge(a, Outcome{}) == a);  // le succes est neutre
  for (const Outcome& a : s) CHECK(merge(a, a) == a);
  CHECK(merge(s[4], s[2]) == s[2]);
  CHECK(merge(s[2], s[4]) == s[2]);
  CHECK(merge(s[5], s[4]) == s[4]);
  CHECK(merge(s[4], s[5]) == s[4]);
  std::array<int, 5> order = {{1, 3, 4, 6, 7}};  // toute permutation de cinq refus donne la meme fusion
  bool same = true;
  do {
    Outcome acc{};
    for (int i : order) acc = merge(acc, s[static_cast<std::size_t>(i)]);
    same = same && acc == s[1];
  } while (std::next_permutation(order.begin(), order.end()));
  CHECK(same);
  CHECK_EQ(exit_code(Outcome{}), 0);
  CHECK_EQ(exit_code(fail(Reason::empty_input)), 2);
  CHECK_EQ(exit_code(fail(Reason::memory_budget, 3)), 2);
  CHECK_EQ(exit_code(fail(Reason::csr_bounds)), 3);
  CHECK_EQ(exit_code(fail(Reason::none)), 3);
}

MHGP11_TEST(macros, 9) {
  CHECK(check_positive(1).ok());
  CHECK_EQ(check_positive(0).reason, Reason::parameter_out_of_range);
  int reached = 0;
  CHECK(try_twice(1, 1, reached).ok());
  CHECK_EQ(reached, 2);
  reached = 0;
  CHECK_EQ(try_twice(1, -1, reached).reason, Reason::parameter_out_of_range);
  CHECK_EQ(reached, 1);  // le second controle a refuse apres le premier
  reached = 0;
  CHECK_EQ(try_twice(-1, 1, reached).reason, Reason::parameter_out_of_range);
  CHECK_EQ(reached, 0);
  CHECK_EQ(half_of(3).outcome().reason, Reason::parameter_out_of_range);
}

MHGP11_TEST(result, 13) {
  const Result<int> good = half_of(10);
  CHECK(good.ok());
  CHECK_EQ(good.value(), 5);
  CHECK(good.outcome().ok());
  const Result<int> bad = half_of(7);
  CHECK(!bad.ok());
  CHECK_EQ(bad.outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(bad.value(), 0);  // valeur par defaut, jamais un resultat partiel
  // un Result construit depuis une issue ok n'est pas un succes (la v10 l'admettait)
  const Result<int> empty = Outcome{};
  CHECK(!empty.ok());
  CHECK_EQ(empty.outcome().reason, Reason::refusal_without_reason);
  CHECK_EQ(empty.outcome().status(), Status::invariant_violated);
  const Result<int> ordered = fail(Reason::memory_budget, 4);
  CHECK_EQ(ordered.outcome().order, 4);
  Result<std::vector<int>> moved = std::vector<int>{1, 2, 3};
  REQUIRE(moved.ok());
  const std::vector<int> taken = moved.take();
  CHECK_EQ(taken.size(), 3u);
  CHECK_EQ(taken[2], 3);
}

MHGP11_TEST(guarded, 8) {
  const Outcome fine = guarded([]() -> Outcome { return {}; });
  CHECK(fine.ok());
  const Outcome refused = guarded([]() -> Outcome { return fail(Reason::csr_bounds, 2); });
  CHECK(refused == fail(Reason::csr_bounds, 2));
  const Outcome exhausted = guarded([]() -> Outcome { throw std::bad_alloc(); });
  CHECK_EQ(exhausted.reason, Reason::memory_budget);
  CHECK_EQ(exhausted.status(), Status::resource_exhausted);
  const Result<int> value = guarded([]() -> Result<int> { return 7; });
  CHECK(value.ok() && value.value() == 7);
  const Result<int> no_value = guarded([]() -> Result<int> { throw std::bad_alloc(); });
  CHECK(!no_value.ok());
  CHECK_EQ(no_value.outcome().reason, Reason::memory_budget);
  // seule std::bad_alloc est convertie : une autre exception sort inchangee
  bool propagated = false;
  try {
    const Outcome o = guarded([]() -> Outcome { throw std::runtime_error("erreur de programmation"); });
    CHECK(!o.ok());
  } catch (const std::runtime_error&) {
    propagated = true;
  }
  CHECK(propagated);
}

MHGP11_TEST_MAIN()
