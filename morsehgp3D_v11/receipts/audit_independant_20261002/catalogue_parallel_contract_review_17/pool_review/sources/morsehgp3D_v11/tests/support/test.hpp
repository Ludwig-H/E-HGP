// Mini cadre de test des portes C++ de la v11 (hors produit). Un seul en-tete.
//
//   #include "test.hpp"
//   MHGP11_TEST(nom, plancher) {      // plancher : nombre minimal de controles que ce test doit jouer
//     CHECK(condition);                // compte un controle ; un echec est ecrit et le test continue
//     CHECK_EQ(obtenu, attendu);       // egalite, valeurs ecrites en cas d'echec ; entiers compares sans piege
//                                      // de signe (std::cmp_equal)
//     REQUIRE(condition);              // comme CHECK, mais quitte le test si la condition est fausse
//   }
//   MHGP11_TEST_MAIN()                 // une fois par executable
//
// Ligne de commande de l'executable :
//   <exe>                      joue tous les tests
//   <exe> <nom>...             joue les tests nommes (nom inconnu ou repete : code 2)
//   <exe> --liste              ecrit les noms des tests
//   <exe> --inventaire <nom>.. code 0 si la liste donnee est exactement celle des tests, 3 sinon
// Codes de sortie : 0 conforme ; 1 au moins un controle en echec (ou une exception sortie d'un test) ; 2 usage ;
// 3 plancher non atteint (un test a joue moins de controles que son plancher : vert par vacuite refuse), aucun test,
// noms en double, inventaire different.
// Les controles ne sont pas proteges contre la concurrence : un test a plusieurs fils range ses resultats dans des
// cases par fil, puis controle apres la jointure.
#pragma once

#include <algorithm>
#include <concepts>
#include <cstdio>
#include <cstring>
#include <exception>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace mhgp11::test {

__extension__ typedef __int128 Wide;
__extension__ typedef unsigned __int128 UWide;

struct Case {
  const char* name;
  unsigned long long floor;
  void (*run)();
};

struct State {
  std::vector<Case> cases;
  unsigned long long checks = 0;    // controles du test en cours
  unsigned long long failures = 0;  // echecs du test en cours
};

inline State& state() {
  static State s;
  return s;
}

struct Registrar {
  Registrar(const char* name, unsigned long long floor, void (*run)()) { state().cases.push_back({name, floor, run}); }
};

inline const char* base_name(const char* path) {
  const char* slash = std::strrchr(path, '/');
  return slash != nullptr ? slash + 1 : path;
}

inline bool check(bool ok, const char* file, int line, const char* what) {
  State& s = state();
  ++s.checks;
  if (!ok) {
    ++s.failures;
    std::printf("ECHEC %s:%d: %s\n", base_name(file), line, what);
    std::fflush(stdout);  // garde le diagnostic si le processus tombe ensuite
  }
  return ok;
}

inline std::string text_of_unsigned(UWide v) {
  std::string digits;
  do {
    digits.push_back(static_cast<char>('0' + static_cast<int>(v % 10)));
    v /= 10;
  } while (v != 0);
  return std::string(digits.rbegin(), digits.rend());
}

// Entier compare par std::cmp_equal : ni bool ni type caractere.
template <class T>
concept PlainInteger = std::is_integral_v<T> && !std::same_as<T, bool> && !std::same_as<T, char> &&
                       !std::same_as<T, wchar_t> && !std::same_as<T, char8_t> && !std::same_as<T, char16_t> &&
                       !std::same_as<T, char32_t>;

template <class T>
std::string text_of(const T& v) {
  if constexpr (std::same_as<T, bool>) {
    return v ? "true" : "false";
  } else if constexpr (std::same_as<T, UWide>) {
    return text_of_unsigned(v);
  } else if constexpr (std::same_as<T, Wide>) {
    return v < 0 ? "-" + text_of_unsigned(UWide{0} - static_cast<UWide>(v)) : text_of_unsigned(static_cast<UWide>(v));
  } else if constexpr (std::is_enum_v<T>) {
    return text_of(static_cast<std::underlying_type_t<T>>(v));
  } else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
    return text_of(static_cast<Wide>(v));
  } else if constexpr (std::is_integral_v<T>) {
    return text_of_unsigned(static_cast<UWide>(v));
  } else if constexpr (std::is_floating_point_v<T>) {
    char buffer[64];
    std::snprintf(buffer, sizeof buffer, "%.21Lg", static_cast<long double>(v));
    return buffer;
  } else if constexpr (std::is_convertible_v<const T&, std::string_view>) {
    return "\"" + std::string(std::string_view(v)) + "\"";
  } else {
    return "<non imprimable>";
  }
}

template <class A, class B>
bool equal(const A& a, const B& b) {
  if constexpr (PlainInteger<A> && PlainInteger<B>) {
    return std::cmp_equal(a, b);
  } else {
    return a == b;
  }
}

template <class A, class B>
bool check_eq(const A& a, const B& b, const char* file, int line, const char* what) {
  const bool ok = equal(a, b);
  check(ok, file, line, what);
  if (!ok) {
    std::printf("      obtenu %s, attendu %s\n", text_of(a).c_str(), text_of(b).c_str());
    std::fflush(stdout);
  }
  return ok;
}

inline const Case* find_case(std::string_view name) {
  for (const Case& c : state().cases)
    if (name == c.name) return &c;
  return nullptr;
}

// Joue un test ; rend faux si son plancher n'est pas atteint alors qu'aucun controle n'a echoue.
inline bool run_case(const Case& c, unsigned long long& checks, unsigned long long& failures) {
  State& s = state();
  s.checks = 0;
  s.failures = 0;
  try {
    c.run();
  } catch (const std::exception& e) {
    ++s.failures;
    std::printf("ECHEC %s : exception sortie du test : %s\n", c.name, e.what());
  } catch (...) {
    ++s.failures;
    std::printf("ECHEC %s : exception inconnue sortie du test\n", c.name);
  }
  std::printf("test %s controles=%llu echecs=%llu plancher=%llu\n", c.name, s.checks, s.failures, c.floor);
  std::fflush(stdout);
  checks += s.checks;
  failures += s.failures;
  return s.failures != 0 || s.checks >= c.floor;
}

inline int inventory(int argc, char** argv) {
  std::vector<std::string> given(argv + 2, argv + argc), known;
  for (const Case& c : state().cases) known.emplace_back(c.name);
  std::sort(given.begin(), given.end());
  if (given == known) {
    std::printf("inventaire_ok tests=%zu\n", known.size());
    return 0;
  }
  for (const std::string& name : known)
    if (!std::binary_search(given.begin(), given.end(), name))
      std::printf("INVENTAIRE test sans porte : %s\n", name.c_str());
  for (const std::string& name : given)
    if (!std::binary_search(known.begin(), known.end(), name))
      std::printf("INVENTAIRE porte sans test : %s\n", name.c_str());
  std::printf("INVENTAIRE different (tests=%zu, liste=%zu)\n", known.size(), given.size());
  return 3;
}

inline int run(int argc, char** argv) {
  std::vector<Case>& cases = state().cases;
  std::sort(cases.begin(), cases.end(), [](const Case& a, const Case& b) { return std::strcmp(a.name, b.name) < 0; });
  for (std::size_t i = 1; i < cases.size(); ++i)
    if (std::strcmp(cases[i - 1].name, cases[i].name) == 0) {
      std::printf("PLANCHER deux tests de meme nom : %s\n", cases[i].name);
      return 3;
    }
  if (cases.empty()) {
    std::printf("PLANCHER aucun test\n");
    return 3;
  }
  if (argc >= 2 && std::strcmp(argv[1], "--liste") == 0) {
    for (const Case& c : cases) std::printf("%s\n", c.name);
    return argc == 2 ? 0 : 2;
  }
  if (argc >= 2 && std::strcmp(argv[1], "--inventaire") == 0) return inventory(argc, argv);

  std::vector<const Case*> selected;
  for (int i = 1; i < argc; ++i) {
    const Case* c = find_case(argv[i]);
    if (c == nullptr || std::find(selected.begin(), selected.end(), c) != selected.end()) {
      std::printf("USAGE test inconnu ou repete : %s\n", argv[i]);
      return 2;
    }
    selected.push_back(c);
  }
  if (selected.empty())
    for (const Case& c : cases) selected.push_back(&c);

  unsigned long long checks = 0, failures = 0, below_floor = 0;
  for (const Case* c : selected)
    if (!run_case(*c, checks, failures)) {
      std::printf("PLANCHER non atteint : %s\n", c->name);
      ++below_floor;
    }
  if (failures != 0) {
    std::printf("ECHECS %llu\n", failures);
    return 1;
  }
  if (below_floor != 0) {
    std::printf("PLANCHER %llu test(s) sous leur plancher\n", below_floor);
    return 3;
  }
  std::printf("mhgp11_test_ok tests=%zu controles=%llu\n", selected.size(), checks);
  return 0;
}

}  // namespace mhgp11::test

#define MHGP11_TEST(name, floor)                                                                           \
  static void mhgp11_test_##name();                                                                        \
  [[maybe_unused]] static const ::mhgp11::test::Registrar mhgp11_registrar_##name(#name, floor,            \
                                                                                    &mhgp11_test_##name);  \
  static void mhgp11_test_##name()

#define CHECK(cond) ::mhgp11::test::check(static_cast<bool>(cond), __FILE__, __LINE__, #cond)
#define CHECK_EQ(a, b) ::mhgp11::test::check_eq((a), (b), __FILE__, __LINE__, #a " == " #b)
#define REQUIRE(cond)        \
  do {                       \
    if (!CHECK(cond)) return; \
  } while (0)

#define MHGP11_TEST_MAIN() \
  int main(int argc, char** argv) { return ::mhgp11::test::run(argc, argv); }
