// Source des portes de refus a la compilation de core (tests.cmake, mhgp11_expect_compile_failure). Sans definition,
// le fichier compile : c'est le temoin. Chaque valeur de MHGP11_NEGATIVE ajoute une faute que le compilateur doit
// refuser avec son jeton.
#include <string>

#include "core/core.hpp"

namespace {

[[maybe_unused]] mhgp11::Outcome witness() {
  const mhgp11::Outcome kept = mhgp11::fail(mhgp11::Reason::memory_budget);
  return kept;
}

#if defined(MHGP11_NEGATIVE) && MHGP11_NEGATIVE == 1
// un refus ignore : Outcome est [[nodiscard]]
[[maybe_unused]] void discard() { mhgp11::fail(mhgp11::Reason::memory_budget); }
#endif

#if defined(MHGP11_NEGATIVE) && MHGP11_NEGATIVE == 2
// idx n'accepte qu'un identifiant fort, pas un entier nu
[[maybe_unused]] mhgp11::u32 naked() { return mhgp11::idx(mhgp11::u32{3}); }
#endif

#if defined(MHGP11_NEGATIVE) && MHGP11_NEGATIVE == 3
// Buffer n'accepte que des types triviaux
[[maybe_unused]] mhgp11::Buffer<std::string> strings;
#endif

#if defined(MHGP11_NEGATIVE) && MHGP11_NEGATIVE == 4
// un identifiant fort ne se convertit pas en un autre
[[maybe_unused]] mhgp11::SiteIdx mixed() { return mhgp11::make_id<mhgp11::PointId>(1); }
#endif

#if defined(MHGP11_NEGATIVE) && MHGP11_NEGATIVE == 5
// Result exige un T qui se deplace sans lever : sinon rendre un succes pourrait lever
struct ThrowingMove {
  ThrowingMove() = default;
  ThrowingMove(ThrowingMove&&) noexcept(false) {}
};
[[maybe_unused]] mhgp11::Result<ThrowingMove> throwing() { return mhgp11::fail(mhgp11::Reason::memory_budget); }
#endif

}  // namespace

int main() { return 0; }
