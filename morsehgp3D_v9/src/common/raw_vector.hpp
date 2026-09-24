#pragma once

// MorseHGP3D v9 (24 septembre 2026) — vecteur a initialisation PAR DEFAUT,
// commun a la tour, au generateur et au port GPU (C++17, lu aussi par nvcc).
// resize() ne met rien a zero pour un type trivialement constructible : il
// est reserve aux tampons dont CHAQUE case est ecrite avant toute lecture
// (arene des requetes de la phase 0, cibles statiques, seaux du tri,
// enregistrements des voies). Sur G4 ces mises a zero en serie pesaient
// autant que le calcul (plans des juges de la tour et des voies).

#include <cstddef>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace mhgp9 {

template <class T>
struct DefaultInitAllocator : std::allocator<T> {
  template <class U> struct rebind { using other = DefaultInitAllocator<U>; };
  DefaultInitAllocator() noexcept = default;
  template <class U> DefaultInitAllocator(const DefaultInitAllocator<U>&) noexcept {}
  template <class U> void construct(U* p) noexcept(std::is_nothrow_default_constructible<U>::value) {
    ::new (static_cast<void*>(p)) U;
  }
  template <class U, class... Args> void construct(U* p, Args&&... args) {
    ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
  }
};
template <class T> using RawVector = std::vector<T, DefaultInitAllocator<T>>;

// Builds de test (MHGP9_TESTING) : les cases [from, size) recoivent un motif
// 0xA5 avant leur premiere ecriture, pour qu'une case jamais ecrite casse
// les condenses au lieu de lire un zero de page neuve.
template <class T, class A>
inline void poison_unwritten(std::vector<T, A>& values, std::size_t from) {
  static_assert(std::is_trivially_copyable<T>::value, "poisoned buffers hold trivially copyable values");
#if defined(MHGP9_TESTING)
  if (from < values.size())
    std::memset(static_cast<void*>(values.data() + from), 0xA5, (values.size() - from) * sizeof(T));
#else
  static_cast<void>(values);
  static_cast<void>(from);
#endif
}

}  // namespace mhgp9
