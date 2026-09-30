// Lecture stricte des options des sondes (cli/ et temoin tests/head/mreach_cluster) : UN seul parseur pour toutes les
// valeurs numeriques, listes (--k-list) et flottants (--z) compris, et pour les fichiers de configurations de tete
// (--configs).
//
// Frontiere (audits du 29 septembre 2026) : std::stoi / std::stoul consommaient un prefixe (--k=2not_an_integer lu 2,
// --k=0x2 lu 0), acceptaient un signe (--threads=-1 lu 2^64 - 1) et la conversion vers u32 reduisait modulo 2^32
// (--leaf=4294967304 lu 8, --threads=4294967297 lu 1), toujours avec le code 0. Ici :
//   - le jeton est consomme EN ENTIER : aucun prefixe, aucun espace, aucun suffixe ;
//   - entier : chiffres decimaux ASCII seulement (ni base implicite 0x ou 0, ni '+') ; un '-' en tete seulement si la
//     borne basse de l'option est negative ; zeros de tete admis (base 10 toujours) ;
//   - la borne [lo, hi] (au plus celle du type cible) est verifiee AVANT la conversion vers le type cible : la
//     magnitude est lue chiffre par chiffre en u64 avec detection du depassement, puis comparee en i128 ; aucune valeur
//     n'est jamais reduite modulo 2^32 ou 2^64 ;
//   - flottant : forme decimale [-]chiffres[.chiffres][(e|E)[+|-]chiffres] ou [-].chiffres[...] (ni inf, ni nan, ni
//     hexadecimal), valeur FINIE dans [lo, hi] ; la conversion est celle de std::stod (strtod, locale « C » : les sondes
//     n'appellent jamais setlocale), donc le meme double qu'avant sur toute valeur admise ;
//   - liste : elements separes par des virgules, chacun strict, aucun element vide.
// Refus : parameter_out_of_range (invalid_input). Les sondes produit l'ecrivent en une ligne JSON {status, reason} sur la
// sortie standard (print_refusal), code 2 ; le temoin mreach garde « refus <raison> » sur la sortie d'erreur.
//
// Fils : --threads borne a [0, kMaxThreads] AVANT la creation du pool (0 : std::thread::hardware_concurrency()).
#pragma once

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "core/status.hpp"

namespace mhgp10::cli {

// Plafond documente de --threads : au-dela de toute machine visee (8 coeurs en local, 48 vCPU sur la VM G4), bien en
// deca de la limite de fils d'un processus ; --threads=-1 demandait 2^32 - 1 fils (SIGABRT sous RLIMIT_AS).
inline constexpr unsigned kMaxThreads = 1024;

inline bool is_digit(char c) { return c >= '0' && c <= '9'; }

// Entier decimal strict dans [lo, hi] (voir l'en-tete). T entier, sur 64 bits au plus.
template <class T>
Result<T> parse_integer(std::string_view s, T lo, T hi) {
  static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool> && sizeof(T) <= 8, "entier de 64 bits au plus");
  const bool negative = !s.empty() && s[0] == '-';
  if (negative) {  // signe interdit si la borne basse de l'option n'est pas negative
    if constexpr (std::is_unsigned_v<T>) return fail(Reason::parameter_out_of_range);
    else if (lo >= 0) return fail(Reason::parameter_out_of_range);
    s.remove_prefix(1);
  }
  if (s.empty()) return fail(Reason::parameter_out_of_range);
  u64 magnitude = 0;
  for (char c : s) {
    if (!is_digit(c)) return fail(Reason::parameter_out_of_range);
    const u64 d = static_cast<u64>(c - '0');
    if (magnitude > (std::numeric_limits<u64>::max() - d) / 10) return fail(Reason::parameter_out_of_range);
    magnitude = magnitude * 10 + d;
  }
  const i128 value = negative ? -static_cast<i128>(magnitude) : static_cast<i128>(magnitude);
  if (value < static_cast<i128>(lo) || value > static_cast<i128>(hi)) return fail(Reason::parameter_out_of_range);
  return static_cast<T>(value);  // exacte : lo <= value <= hi, bornes de T
}

// Flottant decimal strict, fini, dans [lo, hi] (voir l'en-tete).
inline Result<double> parse_real(std::string_view s, double lo, double hi) {
  size_t i = 0, mantissa = 0;
  if (i < s.size() && s[i] == '-') {
    if (!(lo < 0)) return fail(Reason::parameter_out_of_range);
    ++i;
  }
  for (; i < s.size() && is_digit(s[i]); ++i) ++mantissa;
  if (i < s.size() && s[i] == '.')
    for (++i; i < s.size() && is_digit(s[i]); ++i) ++mantissa;
  if (mantissa == 0) return fail(Reason::parameter_out_of_range);
  if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
    ++i;
    if (i < s.size() && (s[i] == '+' || s[i] == '-')) ++i;
    size_t exponent = 0;
    for (; i < s.size() && is_digit(s[i]); ++i) ++exponent;
    if (exponent == 0) return fail(Reason::parameter_out_of_range);
  }
  if (i != s.size()) return fail(Reason::parameter_out_of_range);
  const std::string text(s);  // strtod lit une chaine terminee
  char* end = nullptr;
  const double v = std::strtod(text.c_str(), &end);
  if (end != text.c_str() + text.size() || !std::isfinite(v) || v < lo || v > hi)
    return fail(Reason::parameter_out_of_range);
  return v;
}

// Liste d'entiers stricts separes par des virgules, chacun dans [lo, hi] ; aucun element vide, liste non vide.
template <class T>
Result<std::vector<T>> parse_integer_list(std::string_view s, T lo, T hi) {
  std::vector<T> out;
  size_t pos = 0;
  while (true) {
    const size_t comma = s.find(',', pos);
    const std::string_view item = s.substr(pos, comma == std::string_view::npos ? std::string_view::npos : comma - pos);
    const Result<T> v = parse_integer<T>(item, lo, hi);
    if (!v.ok()) return v.outcome();
    out.push_back(v.value());
    if (comma == std::string_view::npos) return out;
    pos = comma + 1;
  }
}

// Valeur de l'option `name` (forme --name=valeur) si `arg` la porte : vrai et `value` rempli.
inline bool option_value(std::string_view arg, std::string_view name, std::string_view& value) {
  if (arg.size() < name.size() + 1 || arg.substr(0, name.size()) != name || arg[name.size()] != '=') return false;
  value = arg.substr(name.size() + 1);
  return true;
}

// Refus des sondes produit : une ligne JSON {status, reason} sur la sortie standard, code 2. Impression sans
// allocation (raccord R2 : elle sert aussi aux refus memory_budget et session_overhead) : noms lus en place.
inline int print_refusal(const Outcome& o) {
  const std::string_view s = status_name(o.status()), r = reason_name(o.reason);
  std::printf("{\"status\":\"%.*s\",\"reason\":\"%.*s\"}\n", static_cast<int>(s.size()), s.data(),
              static_cast<int>(r.size()), r.data());
  return 2;
}

// Fichier texte lu en entier (--configs) ; ouverture, erreur ou lecture incomplete : input_unreadable ; memoire :
// memory_budget.
inline Result<std::string> read_text_file(const char* path) {
  std::FILE* f = std::fopen(path, "rb");
  if (!f) return fail(Reason::input_unreadable);
  std::string text;
  bool io_error = false;
  try {
    char chunk[4096];
    size_t got;
    while ((got = std::fread(chunk, 1, sizeof chunk, f)) > 0) text.append(chunk, got);
    io_error = std::ferror(f) != 0 || !std::feof(f);
  } catch (const std::bad_alloc&) {
    std::fclose(f);
    return fail(Reason::memory_budget);
  }
  if (std::fclose(f) != 0 || io_error) return fail(Reason::input_unreadable);
  return text;
}

// Configuration de tete lue dans un fichier --configs : une par ligne, exactement quatre jetons
// « mcs z eom|leaf 0|1 » (mcs entier decimal u64, z flottant fini >= 0, selection eom ou leaf, allow_single 0 ou 1) ;
// lignes vides ignorees ; au moins une configuration. Tout autre contenu : parameter_out_of_range (auparavant fscanf
// arretait la liste au premier jeton illisible, lisait « xyz » comme eom et tout entier non nul comme 1).
struct HeadConfig {
  u64 min_cluster_size = 0;
  double z = 0;
  bool leaf = false;
  bool allow_single = false;
};

inline Result<std::vector<HeadConfig>> parse_head_configs(std::string_view text) {
  std::vector<HeadConfig> out;
  auto space = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f'; };
  size_t pos = 0;
  while (pos <= text.size()) {
    const size_t eol = text.find('\n', pos);
    const std::string_view line = text.substr(pos, eol == std::string_view::npos ? std::string_view::npos : eol - pos);
    std::vector<std::string_view> tok;
    for (size_t i = 0; i < line.size();) {
      if (space(line[i])) {
        ++i;
        continue;
      }
      size_t j = i;
      while (j < line.size() && !space(line[j])) ++j;
      tok.push_back(line.substr(i, j - i));
      i = j;
    }
    if (!tok.empty()) {
      if (tok.size() != 4) return fail(Reason::parameter_out_of_range);
      const Result<u64> mcs = parse_integer<u64>(tok[0], 0, std::numeric_limits<u64>::max());
      const Result<double> z = parse_real(tok[1], 0.0, std::numeric_limits<double>::max());
      if (!mcs.ok() || !z.ok() || (tok[2] != "eom" && tok[2] != "leaf") || (tok[3] != "0" && tok[3] != "1"))
        return fail(Reason::parameter_out_of_range);
      out.push_back(HeadConfig{mcs.value(), z.value(), tok[2] == "leaf", tok[3] == "1"});
    }
    if (eol == std::string_view::npos) break;
    pos = eol + 1;
  }
  if (out.empty()) return fail(Reason::parameter_out_of_range);
  return out;
}

}  // namespace mhgp10::cli
