// MorseHGP3D v3 — entier signe de precision arbitraire, pour l'ORACLE seul.
//
// Representation deliberement DIFFERENTE de celle de production : signe et
// magnitude, chiffres de 32 bits en petit-boutien, longueur variable — la
// production emploie un complement a deux de largeur fixe (mhgp::BigInt<N>).
// Un defaut commun aux deux ne peut donc pas se compenser.
//
// Le jeu d'operations est reduit au strict necessaire : +, -, *, comparaison,
// decalages, et division par un petit entier pour l'ecriture decimale du recu.
// Il n'y a NI division generale NI PGCD, parce que l'oracle n'en a pas besoin :
// ses rationnels ne sont jamais normalises (rational.hpp), la division de deux
// rationnels est une multiplication croisee, et leur comparaison aussi. C'est
// la partie la plus facile a rendre juste, et un juge doit etre le composant le
// plus simple du systeme.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mhgp3v {

class BigInt {
 public:
  BigInt() = default;
  BigInt(long long v) {  // NOLINT(google-explicit-constructor)
    unsigned long long m = 0;
    if (v < 0) {
      negative_ = true;
      m = ~static_cast<unsigned long long>(v) + 1ULL;  // defini pour LLONG_MIN
    } else {
      m = static_cast<unsigned long long>(v);
    }
    while (m != 0) {
      digits_.push_back(static_cast<std::uint32_t>(m & 0xFFFFFFFFULL));
      m >>= 32;
    }
    trim();
  }

  static BigInt from_i128(__int128 v) {
    BigInt r;
    unsigned __int128 m = 0;
    if (v < 0) {
      r.negative_ = true;
      m = ~static_cast<unsigned __int128>(v) + 1;
    } else {
      m = static_cast<unsigned __int128>(v);
    }
    while (m != 0) {
      r.digits_.push_back(static_cast<std::uint32_t>(m & 0xFFFFFFFFU));
      m >>= 32;
    }
    r.trim();
    return r;
  }

  bool is_zero() const { return digits_.empty(); }
  int sign() const { return digits_.empty() ? 0 : (negative_ ? -1 : 1); }
  std::size_t digit_count() const { return digits_.size(); }

  // Nombre de bits de la magnitude ; 0 pour zero.
  std::size_t bit_length() const {
    if (digits_.empty()) return 0;
    std::size_t top = digits_.size() - 1;
    std::uint32_t w = digits_[top];
    std::size_t bits = 0;
    while (w != 0) { ++bits; w >>= 1; }
    return top * 32 + bits;
  }

  BigInt operator-() const {
    BigInt r = *this;
    if (!r.digits_.empty()) r.negative_ = !r.negative_;
    return r;
  }

  friend BigInt operator+(const BigInt& a, const BigInt& b) {
    if (a.negative_ == b.negative_) {
      BigInt r;
      r.digits_ = add_magnitude(a.digits_, b.digits_);
      r.negative_ = a.negative_;
      r.trim();
      return r;
    }
    const int c = compare_magnitude(a.digits_, b.digits_);
    if (c == 0) return BigInt();
    BigInt r;
    if (c > 0) {
      r.digits_ = subtract_magnitude(a.digits_, b.digits_);
      r.negative_ = a.negative_;
    } else {
      r.digits_ = subtract_magnitude(b.digits_, a.digits_);
      r.negative_ = b.negative_;
    }
    r.trim();
    return r;
  }

  friend BigInt operator-(const BigInt& a, const BigInt& b) { return a + (-b); }

  friend BigInt operator*(const BigInt& a, const BigInt& b) {
    if (a.digits_.empty() || b.digits_.empty()) return BigInt();
    BigInt r;
    r.digits_.assign(a.digits_.size() + b.digits_.size(), 0U);
    for (std::size_t i = 0; i < a.digits_.size(); ++i) {
      std::uint64_t carry = 0;
      const std::uint64_t ai = a.digits_[i];
      for (std::size_t j = 0; j < b.digits_.size(); ++j) {
        const std::uint64_t cur =
            static_cast<std::uint64_t>(r.digits_[i + j]) + ai * b.digits_[j] + carry;
        r.digits_[i + j] = static_cast<std::uint32_t>(cur & 0xFFFFFFFFULL);
        carry = cur >> 32;
      }
      std::size_t k = i + b.digits_.size();
      while (carry != 0) {
        const std::uint64_t cur = static_cast<std::uint64_t>(r.digits_[k]) + carry;
        r.digits_[k] = static_cast<std::uint32_t>(cur & 0xFFFFFFFFULL);
        carry = cur >> 32;
        ++k;
      }
    }
    r.negative_ = (a.negative_ != b.negative_);
    r.trim();
    return r;
  }

  // Ordre total signe.
  friend int compare(const BigInt& a, const BigInt& b) {
    const int sa = a.sign(), sb = b.sign();
    if (sa != sb) return sa < sb ? -1 : 1;
    if (sa == 0) return 0;
    const int c = compare_magnitude(a.digits_, b.digits_);
    return sa > 0 ? c : -c;
  }
  friend bool operator==(const BigInt& a, const BigInt& b) { return compare(a, b) == 0; }
  friend bool operator!=(const BigInt& a, const BigInt& b) { return compare(a, b) != 0; }
  friend bool operator<(const BigInt& a, const BigInt& b) { return compare(a, b) < 0; }
  friend bool operator>(const BigInt& a, const BigInt& b) { return compare(a, b) > 0; }

  // Nombre de facteurs deux de la magnitude ; indefini (0) pour zero.
  std::size_t trailing_zero_bits() const {
    if (digits_.empty()) return 0;
    std::size_t k = 0;
    while (digits_[k] == 0U) ++k;
    std::uint32_t w = digits_[k];
    std::size_t bits = 0;
    while ((w & 1U) == 0U) { ++bits; w >>= 1; }
    return k * 32 + bits;
  }

  BigInt shifted_right(std::size_t bits) const {
    if (digits_.empty() || bits == 0) return *this;
    const std::size_t words = bits / 32, rest = bits % 32;
    if (words >= digits_.size()) return BigInt();
    BigInt r;
    r.negative_ = negative_;
    r.digits_.assign(digits_.begin() + static_cast<std::ptrdiff_t>(words), digits_.end());
    if (rest != 0) {
      for (std::size_t i = 0; i < r.digits_.size(); ++i) {
        std::uint32_t hi = (i + 1 < r.digits_.size()) ? r.digits_[i + 1] : 0U;
        r.digits_[i] = (r.digits_[i] >> rest) | static_cast<std::uint32_t>(
                           static_cast<std::uint64_t>(hi) << (32 - rest));
      }
    }
    r.trim();
    return r;
  }

  // Ecriture decimale, pour le recu. Division par 10^9, seule division admise.
  std::string to_decimal() const {
    if (digits_.empty()) return "0";
    std::vector<std::uint32_t> work = digits_;
    std::string out;
    while (!work.empty()) {
      std::uint64_t remainder = 0;
      for (std::size_t i = work.size(); i-- > 0;) {
        const std::uint64_t cur = (remainder << 32) | work[i];
        work[i] = static_cast<std::uint32_t>(cur / 1000000000ULL);
        remainder = cur % 1000000000ULL;
      }
      while (!work.empty() && work.back() == 0U) work.pop_back();
      std::string chunk = std::to_string(remainder);
      if (!work.empty()) chunk.insert(0, 9 - chunk.size(), '0');
      out.insert(0, chunk);
    }
    return negative_ ? ("-" + out) : out;
  }

 private:
  void trim() {
    while (!digits_.empty() && digits_.back() == 0U) digits_.pop_back();
    if (digits_.empty()) negative_ = false;
  }

  static int compare_magnitude(const std::vector<std::uint32_t>& a,
                               const std::vector<std::uint32_t>& b) {
    if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
    for (std::size_t i = a.size(); i-- > 0;)
      if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    return 0;
  }

  static std::vector<std::uint32_t> add_magnitude(const std::vector<std::uint32_t>& a,
                                                  const std::vector<std::uint32_t>& b) {
    std::vector<std::uint32_t> r;
    r.reserve(std::max(a.size(), b.size()) + 1);
    std::uint64_t carry = 0;
    for (std::size_t i = 0; i < std::max(a.size(), b.size()); ++i) {
      const std::uint64_t cur = carry + (i < a.size() ? a[i] : 0U) + (i < b.size() ? b[i] : 0U);
      r.push_back(static_cast<std::uint32_t>(cur & 0xFFFFFFFFULL));
      carry = cur >> 32;
    }
    if (carry != 0) r.push_back(static_cast<std::uint32_t>(carry));
    return r;
  }

  // Exige magnitude(a) >= magnitude(b).
  static std::vector<std::uint32_t> subtract_magnitude(const std::vector<std::uint32_t>& a,
                                                       const std::vector<std::uint32_t>& b) {
    std::vector<std::uint32_t> r;
    r.reserve(a.size());
    std::int64_t borrow = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
      std::int64_t cur = static_cast<std::int64_t>(a[i]) - borrow
                       - (i < b.size() ? static_cast<std::int64_t>(b[i]) : 0);
      if (cur < 0) { cur += (std::int64_t{1} << 32); borrow = 1; } else { borrow = 0; }
      r.push_back(static_cast<std::uint32_t>(cur));
    }
    return r;
  }

  bool negative_ = false;
  std::vector<std::uint32_t> digits_;  // petit-boutien, sans zeros de tete
};

}  // namespace mhgp3v
