// Portes de core sous penurie de memoire injectee. Les operateurs new sont remplaces dans ce seul executable (hors
// produit) : new sans exception echoue sur commande, new avec exception leve std::bad_alloc a la k-ieme allocation,
// et chaque appel est compte. Sans injection, ces chemins ne se jouent qu'en epuisant la memoire de la machine.
//   alloc_fault : l'allocation d'un Buffer echoue APRES la reservation dans le budget ;
//   refusal     : construire un refus n'alloue pas et ne leve pas, meme pour un T dont le constructeur par defaut
//                 alloue (contre-exemple de l'audit du 2 octobre 2026 : le refus de guarded levait a son tour) ;
//   ledger      : chaque operation du registre est tout ou rien sous penurie.
//
// Les quatre formes scalaires sont remplacees ensemble (new, new sans exception, delete, delete avec taille) : sous
// un sanitizer, un bloc de malloc rendu par l'operator delete du runtime serait un desaccord d'allocateur.
#include <cstdlib>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include "core/core.hpp"
#include "test.hpp"

namespace {

// Etat de test, un seul fil.
bool fail_nothrow_new = false;
long throwing_countdown = -1;  // >= 0 : nombre d'allocations encore admises avant que new leve
unsigned long long allocations = 0;

}  // namespace

void* operator new(std::size_t size) {
  ++allocations;
  if (throwing_countdown == 0) throw std::bad_alloc();
  if (throwing_countdown > 0) --throwing_countdown;
  void* block = std::malloc(size == 0 ? 1 : size);
  if (block == nullptr) throw std::bad_alloc();
  return block;
}
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
  ++allocations;
  if (fail_nothrow_new) return nullptr;
  return std::malloc(size == 0 ? 1 : size);
}
void operator delete(void* block) noexcept { std::free(block); }
void operator delete(void* block, std::size_t) noexcept { std::free(block); }

using namespace mhgp11;

namespace {

// Le contre-exemple de l'audit : un resultat dont le constructeur par defaut alloue.
struct Allocating {
  std::shared_ptr<int> payload = std::make_shared<int>(7);
};

}  // namespace

MHGP11_TEST(alloc_fault, 15) {
  MemoryBudget budget(1 << 20);
  Buffer<u64> a;

  fail_nothrow_new = true;
  const Outcome refused = a.allocate(100, budget);
  fail_nothrow_new = false;
  CHECK_EQ(refused.reason, Reason::memory_budget);
  CHECK_EQ(refused.status(), Status::resource_exhausted);
  CHECK(a.empty() && a.data() == nullptr);
  CHECK_EQ(budget.used(), 0u);    // la reservation est rendue
  CHECK_EQ(budget.peak(), 800u);  // elle a existe : le pic est le maximum de ce qui a ete reserve

  // le remplacement est bien celui que le produit appelle : sans la commande, l'allocation reussit
  CHECK(a.allocate(100, budget).ok());
  CHECK_EQ(a.size(), 100u);
  CHECK_EQ(budget.used(), 800u);
  a[99] = 5;
  CHECK_EQ(a[99], 5u);

  // un echec pendant une reallocation : le contenu precedent est deja rendu, le tampon est vide, rien n'est reserve
  fail_nothrow_new = true;
  const Outcome refused_again = a.allocate(10, budget);
  fail_nothrow_new = false;
  CHECK_EQ(refused_again.reason, Reason::memory_budget);
  CHECK(a.empty());
  CHECK_EQ(budget.used(), 0u);

  // apres les refus le budget reste utilisable jusqu'a sa limite exacte
  Buffer<u8> full;
  CHECK(full.allocate(1 << 20, budget).ok());
  CHECK_EQ(budget.used(), u64{1} << 20);
  CHECK_EQ(budget.peak(), u64{1} << 20);
}

MHGP11_TEST(refusal, 10) {
  // temoin : hors penurie, ce type alloue bien a sa construction par defaut
  const unsigned long long before_control = allocations;
  { const Allocating control; }
  CHECK(allocations > before_control);

  // toute allocation leve des l'entree dans la garde : le refus memory_budget doit quand meme sortir, sans exception
  // et sans une seule tentative d'allocation
  bool escaped = false;
  unsigned long long attempts = 0;
  Reason reason = Reason::none;
  try {
    throwing_countdown = 0;
    const unsigned long long before = allocations;
    const Result<Allocating> refused = guarded([]() -> Result<Allocating> { throw std::bad_alloc(); });
    attempts = allocations - before;
    throwing_countdown = -1;
    reason = refused.outcome().reason;
  } catch (const std::bad_alloc&) {
    throwing_countdown = -1;
    escaped = true;
  }
  CHECK(!escaped);
  CHECK_EQ(reason, Reason::memory_budget);
  CHECK_EQ(attempts, 0u);

  // meme chose quand la penurie frappe au milieu du travail garde, et pour un refus ordinaire
  throwing_countdown = 1;
  const Result<std::vector<int>> starved = guarded([]() -> Result<std::vector<int>> {
    std::vector<int> first(10, 1);   // admise
    std::vector<int> second(10, 2);  // refusee : std::bad_alloc
    first.insert(first.end(), second.begin(), second.end());
    return first;
  });
  throwing_countdown = -1;
  CHECK(!starved.ok());
  CHECK_EQ(starved.outcome().reason, Reason::memory_budget);
  throwing_countdown = 0;
  const unsigned long long before_plain = allocations;
  const Result<Allocating> plain = fail(Reason::empty_input, 2);
  const unsigned long long plain_attempts = allocations - before_plain;
  throwing_countdown = -1;
  CHECK_EQ(plain_attempts, 0u);
  CHECK(plain.outcome() == fail(Reason::empty_input, 2));

  // hors penurie le succes se rend avec sa valeur
  const Result<Allocating> good = guarded([]() -> Result<Allocating> { return Allocating{}; });
  REQUIRE(good.ok());
  CHECK_EQ(*good.value().payload, 7);
}

// Registre sous penurie : la k-ieme allocation leve, pour k = 0, 1, 2, ... jusqu'au succes. Tant que l'operation
// leve, le registre cible est exactement ce qu'il etait ; quand elle reussit, il est exactement le resultat attendu.
MHGP11_TEST(ledger, 11) {
  Ledger source;
  source.count("a", 2);
  source.count("b", 3);
  source.time("etage", 5);
  const std::string before = "{\"counters\":{\"a\":1},\"nanoseconds\":{}}";
  const std::string after = "{\"counters\":{\"a\":3,\"b\":3},\"nanoseconds\":{\"etage\":5}}";

  unsigned refusals = 0, intact = 0;
  bool merged = false;
  for (long k = 0; k < 64 && !merged; ++k) {
    Ledger target;
    target.count("a", 1);
    throwing_countdown = k;
    const Outcome o = guarded([&]() -> Outcome {
      target.merge(source);
      return {};
    });
    throwing_countdown = -1;
    if (o.ok()) {
      merged = true;
      CHECK_EQ(target.to_json(), after);
    } else {
      ++refusals;
      intact += o.reason == Reason::memory_budget && target.to_json() == before ? 1 : 0;
    }
  }
  CHECK(merged);
  CHECK(refusals >= 3);          // au moins : copie des tables, cle "b", cle "etage"
  CHECK_EQ(intact, refusals);    // aucun prefixe de fusion ne reste dans le registre cible
  CHECK_EQ(source.to_json(), "{\"counters\":{\"a\":2,\"b\":3},\"nanoseconds\":{\"etage\":5}}");

  // count et time : la creation d'une cle qui leve laisse le registre inchange
  Ledger single;
  single.count("x", 4);
  throwing_countdown = 0;
  const Outcome count_refused = guarded([&]() -> Outcome {
    single.count("une cle assez longue pour ne pas tenir dans la chaine courte", 1);
    return {};
  });
  const Outcome time_refused = guarded([&]() -> Outcome {
    single.time("un etage assez long pour ne pas tenir dans la chaine courte", 1);
    return {};
  });
  throwing_countdown = -1;
  CHECK_EQ(count_refused.reason, Reason::memory_budget);
  CHECK_EQ(time_refused.reason, Reason::memory_budget);
  CHECK_EQ(single.to_json(), "{\"counters\":{\"x\":4},\"nanoseconds\":{}}");
  // une cle deja presente se met a jour sans allouer, meme sous penurie
  throwing_countdown = 0;
  const unsigned long long before_update = allocations;
  single.count("x", 6);
  const unsigned long long update_attempts = allocations - before_update;
  throwing_countdown = -1;
  CHECK_EQ(update_attempts, 0u);
  CHECK_EQ(single.counter("x"), 10u);
  CHECK(single.to_json() == "{\"counters\":{\"x\":10},\"nanoseconds\":{}}");
}

MHGP11_TEST_MAIN()
