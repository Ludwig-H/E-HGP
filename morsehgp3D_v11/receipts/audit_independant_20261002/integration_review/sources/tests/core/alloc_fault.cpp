// Porte de core : echec de l'allocation APRES la reservation dans le budget. operator new sans exception est remplace
// dans ce seul executable (hors produit) et echoue sur commande : le tampon doit refuser memory_budget et rendre au
// budget ce qu'il avait reserve. Sans injection, ce chemin ne se joue qu'en epuisant la memoire de la machine.
//
// Les quatre formes scalaires sont remplacees ensemble (new, new sans exception, delete, delete avec taille) : sous
// un sanitizer, un bloc de malloc rendu par l'operator delete du runtime serait un desaccord d'allocateur.
#include <cstdlib>
#include <new>

#include "core/core.hpp"
#include "test.hpp"

namespace {

bool fail_nothrow_new = false;  // etat de test, un seul fil

}  // namespace

void* operator new(std::size_t size) {
  void* block = std::malloc(size == 0 ? 1 : size);
  if (block == nullptr) throw std::bad_alloc();
  return block;
}
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
  if (fail_nothrow_new) return nullptr;
  return std::malloc(size == 0 ? 1 : size);
}
void operator delete(void* block) noexcept { std::free(block); }
void operator delete(void* block, std::size_t) noexcept { std::free(block); }

using namespace mhgp11;

MHGP11_TEST(alloc_fault, 18) {
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

  fail_nothrow_new = true;
  const Outcome refused_zero = a.allocate_zero(50, budget);
  fail_nothrow_new = false;
  CHECK_EQ(refused_zero.reason, Reason::memory_budget);
  CHECK(a.empty());
  CHECK_EQ(budget.used(), 0u);

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

MHGP11_TEST_MAIN()
