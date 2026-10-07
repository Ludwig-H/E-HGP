// Lecture et ecriture des entiers et rationnels de num en hexadecimal signe pour les sondes S8 (hors produit) :
// "0", "1f", "-1f" ; un rationnel s'ecrit "<hex>/<hex>" ou "<hex>". Le refus d'un entier trop large est celui de
// Big::assign_words (radical_sign_budget).
#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "num/num.hpp"

namespace mhgp12::num::probe {

inline int hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

// Refus : parameter_out_of_range si le texte n'est pas un hexadecimal signe, sinon celui de assign_words.
inline Outcome parse_big(std::string_view text, Big& out) {
  bool negative = false;
  if (!text.empty() && text.front() == '-') {
    negative = true;
    text.remove_prefix(1);
  }
  if (text.empty()) return fail(Reason::parameter_out_of_range);
  std::vector<u64> words((text.size() + 15) / 16, 0);
  for (std::size_t i = 0; i < text.size(); ++i) {
    const int digit = hex_digit(text[text.size() - 1 - i]);
    if (digit < 0) return fail(Reason::parameter_out_of_range);
    words[i / 16] |= static_cast<u64>(digit) << (4 * (i % 16));
  }
  return out.assign_words(words, negative);
}

inline Outcome parse_rational(std::string_view text, Rational& out) {
  const auto slash = text.find('/');
  Big n, d = Big::from_u64(1);
  MHGP12_TRY(parse_big(text.substr(0, slash), n));
  if (slash != std::string_view::npos) MHGP12_TRY(parse_big(text.substr(slash + 1), d));
  if (d.sign() <= 0) return fail(Reason::parameter_out_of_range);
  return Rational::make(n, d, out);
}

inline std::string hex(const Big& value) {
  if (value.is_zero()) return "0";
  static constexpr char kDigits[] = "0123456789abcdef";
  std::string out;
  for (u32 i = 0; i < value.size(); ++i) {
    u64 word = value.word(i);
    for (int j = 0; j < 16; ++j) {
      out.push_back(kDigits[word & 15]);
      word >>= 4;
    }
  }
  while (out.size() > 1 && out.back() == '0') out.pop_back();
  if (value.negative()) out.push_back('-');
  return {out.rbegin(), out.rend()};
}

inline std::string hex128(u128 value) { return hex(Big::from_u128(value)); }

inline std::string hex_signed128(i128 value) { return hex(Big::from_i128(value)); }

inline void refused(const Outcome& outcome) { std::cout << "refused " << reason_name(outcome.reason) << '\n'; }

}  // namespace mhgp12::num::probe
