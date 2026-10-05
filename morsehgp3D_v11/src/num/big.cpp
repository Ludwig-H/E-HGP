// Operations de num::Big (tranche S8) : addition, soustraction, multiplication d'ecole, decalages, division de Knuth
// (algorithme D, Hacker's Delight divmnu64), PGCD binaire, racine entiere de Newton certifiee, residus et division
// par un petit entier. Chaque boucle ne parcourt que les mots utiles. Un resultat exact au-dela de la capacite rend
// radical_sign_budget ; une precondition interne violee rend arithmetic_invariant.
#include "num/big.hpp"

#include <algorithm>
#include <bit>
#include <cmath>

namespace mhgp11::num {

struct BigAccess {
  static u64* words(Big& b) noexcept { return b.w_; }
  static const u64* words(const Big& b) noexcept { return b.w_; }
  static void set(Big& b, u32 len, bool neg) noexcept {
    b.len_ = len;
    b.neg_ = neg;
    b.trim();
  }
};

namespace {

using A = BigAccess;

Outcome over_capacity() noexcept { return fail(Reason::radical_sign_budget); }

// |a| + |b| dans out (out peut designer a ou b) ; refus si la somme depasse la capacite.
Outcome add_magnitude(const Big& a, const Big& b, Big& out, bool negative) noexcept {
  const Big& big = a.size() >= b.size() ? a : b;
  const Big& small = a.size() >= b.size() ? b : a;
  const u32 lb = big.size(), ls = small.size();
  const u64* pb = A::words(big);
  const u64* ps = A::words(small);
  u64* po = A::words(out);
  u64 carry = 0;
  for (u32 i = 0; i < lb; ++i) {
    const u128 sum = static_cast<u128>(pb[i]) + (i < ls ? ps[i] : 0) + carry;
    po[i] = static_cast<u64>(sum);
    carry = static_cast<u64>(sum >> 64);
  }
  u32 len = lb;
  if (carry != 0) {
    if (lb == kBigWords) return over_capacity();
    po[lb] = carry;
    len = lb + 1;
  }
  A::set(out, len, negative);
  return {};
}

// |a| - |b| dans out, exige |a| >= |b| (out peut designer a ou b).
void subtract_magnitude(const Big& a, const Big& b, Big& out, bool negative) noexcept {
  const u32 la = a.size(), lb = b.size();
  const u64* pa = A::words(a);
  const u64* pb = A::words(b);
  u64* po = A::words(out);
  u64 borrow = 0;
  for (u32 i = 0; i < la; ++i) {
    const u128 difference = static_cast<u128>(pa[i]) - (i < lb ? pb[i] : 0) - borrow;
    po[i] = static_cast<u64>(difference);
    borrow = (difference >> 64) != 0 ? 1 : 0;
  }
  A::set(out, la, negative);
}

// Addition signee : a + (negate_b ? -b : b).
Outcome signed_add(const Big& a, const Big& b, bool negate_b, Big& out) noexcept {
  const bool nb = negate_b ? !b.negative() : b.negative();
  if (b.is_zero()) {
    if (&out != &a) out.assign(a);
    return {};
  }
  if (a.negative() == nb) return add_magnitude(a, b, out, a.negative());
  if (compare_magnitude(a, b) >= 0) {
    subtract_magnitude(a, b, out, a.negative());
  } else {
    subtract_magnitude(b, a, out, nb);
  }
  return {};
}

// Division d'une magnitude u (m mots) par v (n >= 2 mots), Knuth D. q recoit m - n + 1 mots, r n mots.
void knuth_divide(const u64* u, u32 m, const u64* v, u32 n, u64* q, u64* r) noexcept {
  const int s = std::countl_zero(v[n - 1]);
  u64 vn[kBigWords];
  u64 un[kBigWords + 1];
  for (u32 i = n - 1; i > 0; --i)
    vn[i] = s == 0 ? v[i] : (v[i] << s) | (v[i - 1] >> (64 - s));
  vn[0] = v[0] << s;
  un[m] = s == 0 ? 0 : u[m - 1] >> (64 - s);
  for (u32 i = m - 1; i > 0; --i)
    un[i] = s == 0 ? u[i] : (u[i] << s) | (u[i - 1] >> (64 - s));
  un[0] = u[0] << s;
  for (u32 jj = m - n + 1; jj > 0; --jj) {
    const u32 j = jj - 1;
    const u128 top = (static_cast<u128>(un[j + n]) << 64) | un[j + n - 1];
    u128 qhat = top / vn[n - 1];
    u128 rhat = top - qhat * vn[n - 1];
    while ((qhat >> 64) != 0 || qhat * vn[n - 2] > ((rhat << 64) | un[j + n - 2])) {
      --qhat;
      rhat += vn[n - 1];
      if ((rhat >> 64) != 0) break;
    }
    // Multiplier et soustraire.
    i128 t = 0, k = 0;  // emprunt : 0 <= k <= 2^64
    for (u32 i = 0; i < n; ++i) {
      const u128 p = qhat * vn[i];
      t = static_cast<i128>(un[i + j]) - k - static_cast<i128>(static_cast<u64>(p));
      un[i + j] = static_cast<u64>(t);
      k = static_cast<i128>(p >> 64) - (t >> 64);
    }
    t = static_cast<i128>(un[j + n]) - k;
    un[j + n] = static_cast<u64>(t);
    q[j] = static_cast<u64>(qhat);
    if (t < 0) {  // ajouter en retour
      --q[j];
      u64 carry = 0;
      for (u32 i = 0; i < n; ++i) {
        const u128 sum = static_cast<u128>(un[i + j]) + vn[i] + carry;
        un[i + j] = static_cast<u64>(sum);
        carry = static_cast<u64>(sum >> 64);
      }
      un[j + n] += carry;
    }
  }
  for (u32 i = 0; i < n; ++i)
    r[i] = s == 0 ? un[i] : (un[i] >> s) | (un[i + 1] << (64 - s));
}

// Magnitudes : |a| = q |b| + r, 0 <= r < |b|, |b| > 0. q et r ne designent ni a ni b.
void divide_magnitude(const Big& a, const Big& b, Big& q, Big& r) noexcept {
  const u32 la = a.size(), lb = b.size();
  if (compare_magnitude(a, b) < 0) {
    A::set(q, 0, false);
    r.assign(a);
    r.set_negative(false);
    return;
  }
  if (lb == 1) {
    const u32 len = la;
    u128 rest = 0;
    for (u32 ii = la; ii > 0; --ii) {
      const u128 current = (rest << 64) | A::words(a)[ii - 1];
      A::words(q)[ii - 1] = static_cast<u64>(current / A::words(b)[0]);
      rest = current % A::words(b)[0];
    }
    A::set(q, len, false);
    A::words(r)[0] = static_cast<u64>(rest);
    A::set(r, 1, false);
    return;
  }
  knuth_divide(A::words(a), la, A::words(b), lb, A::words(q), A::words(r));
  A::set(q, la - lb + 1, false);
  A::set(r, lb, false);
}

}  // namespace

Big Big::from_u64(u64 value) noexcept {
  Big out;
  out.w_[0] = value;
  out.len_ = value != 0 ? 1 : 0;
  return out;
}

Big Big::from_i64(i64 value) noexcept {
  Big out = from_u64(value < 0 ? u64{0} - static_cast<u64>(value) : static_cast<u64>(value));
  out.set_negative(value < 0);
  return out;
}

Big Big::from_u128(u128 value) noexcept {
  Big out;
  out.w_[0] = static_cast<u64>(value);
  out.w_[1] = static_cast<u64>(value >> 64);
  out.len_ = 2;
  out.trim();
  return out;
}

Big Big::from_i128(i128 value) noexcept {
  Big out = from_u128(value < 0 ? u128{0} - static_cast<u128>(value) : static_cast<u128>(value));
  out.set_negative(value < 0);
  return out;
}

u32 Big::bit_length() const noexcept {
  if (len_ == 0) return 0;
  return 64 * (len_ - 1) + 64 - static_cast<u32>(std::countl_zero(w_[len_ - 1]));
}

void Big::assign(const Big& other) noexcept {
  if (this == &other) return;
  std::copy(other.w_, other.w_ + other.len_, w_);
  len_ = other.len_;
  neg_ = other.neg_;
}

Outcome Big::assign_words(std::span<const u64> words, bool negative) noexcept {
  std::size_t len = words.size();
  while (len != 0 && words[len - 1] == 0) --len;
  if (len > kBigWords) return over_capacity();
  std::copy(words.begin(), words.begin() + static_cast<std::ptrdiff_t>(len), w_);
  len_ = static_cast<u32>(len);
  neg_ = negative && len_ != 0;
  return {};
}

std::optional<u128> Big::magnitude_u128() const noexcept {
  if (len_ > 2) return std::nullopt;
  u128 value = len_ >= 1 ? w_[0] : 0;
  if (len_ == 2) value |= static_cast<u128>(w_[1]) << 64;
  return value;
}

int compare_magnitude(const Big& a, const Big& b) noexcept {
  if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
  for (u32 ii = a.size(); ii > 0; --ii)
    if (a.word(ii - 1) != b.word(ii - 1)) return a.word(ii - 1) < b.word(ii - 1) ? -1 : 1;
  return 0;
}

int compare(const Big& a, const Big& b) noexcept {
  const int sa = a.sign(), sb = b.sign();
  if (sa != sb) return sa < sb ? -1 : 1;
  const int order = compare_magnitude(a, b);
  return sa >= 0 ? order : -order;
}

Outcome add(const Big& a, const Big& b, Big& out) noexcept { return signed_add(a, b, false, out); }

Outcome subtract(const Big& a, const Big& b, Big& out) noexcept { return signed_add(a, b, true, out); }

Outcome multiply(const Big& a, const Big& b, Big& out) noexcept {
  if (a.is_zero() || b.is_zero()) {
    A::set(out, 0, false);
    return {};
  }
  const u32 bits = a.bit_length() + b.bit_length();  // le produit a bits ou bits - 1 bits
  if (bits - 1 > kBigCapacityBits) return over_capacity();
  const u32 la = a.size(), lb = b.size();
  u64 acc[kBigWords + 1];
  std::fill(acc, acc + la + lb, u64{0});
  for (u32 i = 0; i < la; ++i) {
    u64 carry = 0;
    const u64 ai = a.word(i);
    for (u32 j = 0; j < lb; ++j) {
      const u128 value = static_cast<u128>(ai) * b.word(j) + acc[i + j] + carry;
      acc[i + j] = static_cast<u64>(value);
      carry = static_cast<u64>(value >> 64);
    }
    acc[i + lb] = carry;
  }
  u32 len = la + lb;
  while (len != 0 && acc[len - 1] == 0) --len;
  if (len > kBigWords) return over_capacity();
  const bool negative = a.negative() != b.negative();
  std::copy(acc, acc + len, A::words(out));
  A::set(out, len, negative);
  return {};
}

Outcome shift_left(const Big& a, u32 bits, Big& out) noexcept {
  if (a.is_zero()) {
    A::set(out, 0, false);
    return {};
  }
  if (bits > kBigCapacityBits || a.bit_length() + bits > kBigCapacityBits) return over_capacity();
  const u32 words = bits / 64, rest = bits % 64, la = a.size();
  const u64* pa = A::words(a);
  u64* po = A::words(out);
  const u32 len = (a.bit_length() + bits + 63) / 64;
  // De haut en bas : out peut designer a.
  for (u32 ii = len; ii > words; --ii) {
    const u32 i = ii - 1 - words;  // indice source du mot de rang ii - 1
    const u64 high = i < la ? pa[i] : 0;
    const u64 low = (rest != 0 && i >= 1 && i - 1 < la) ? pa[i - 1] : 0;
    po[ii - 1] = rest == 0 ? high : (high << rest) | (low >> (64 - rest));
  }
  std::fill(po, po + words, u64{0});
  A::set(out, len, a.negative());
  return {};
}

void shift_right(const Big& a, u32 bits, Big& out) noexcept {
  const u32 words = bits / 64, rest = bits % 64, la = a.size();
  if (words >= la) {
    A::set(out, 0, false);
    return;
  }
  const u64* pa = A::words(a);
  u64* po = A::words(out);
  const u32 len = la - words;
  for (u32 i = 0; i < len; ++i) {  // de bas en haut : out peut designer a
    const u64 low = pa[i + words];
    const u64 high = i + words + 1 < la ? pa[i + words + 1] : 0;
    po[i] = rest == 0 ? low : (low >> rest) | (high << (64 - rest));
  }
  A::set(out, len, a.negative());
}

Outcome divide(const Big& a, const Big& b, Big& quotient, Big& remainder) noexcept {
  if (b.is_zero()) return fail(Reason::arithmetic_invariant);
  if (&quotient == &remainder) return fail(Reason::arithmetic_invariant);
  Big q, r;
  divide_magnitude(a, b, q, r);
  // Troncature vers zero, puis plancher : si les signes different et que le reste est non nul, q := -(q+1),
  // r := |b| - r, du signe de b.
  const bool negative = a.negative() != b.negative();
  if (negative && !r.is_zero()) {
    MHGP11_TRY(add(q, Big::from_u64(1), q));
    Big magnitude_b;
    magnitude_b.assign(b);
    magnitude_b.set_negative(false);
    subtract_magnitude(magnitude_b, r, r, false);
  }
  q.set_negative(negative);
  r.set_negative(b.negative());
  quotient.assign(q);
  remainder.assign(r);
  return {};
}

u32 trailing_zeros(const Big& a) noexcept {
  for (u32 i = 0; i < a.size(); ++i)
    if (a.word(i) != 0) return 64 * i + static_cast<u32>(std::countr_zero(a.word(i)));
  return 0;
}

Outcome gcd(const Big& a, const Big& b, Big& out) noexcept {
  Big x, y;
  x.assign(a);
  x.set_negative(false);
  y.assign(b);
  y.set_negative(false);
  if (x.is_zero()) {
    out.assign(y);
    return {};
  }
  if (y.is_zero()) {
    out.assign(x);
    return {};
  }
  const u32 common = std::min(trailing_zeros(x), trailing_zeros(y));
  shift_right(x, trailing_zeros(x), x);
  while (!y.is_zero()) {
    shift_right(y, trailing_zeros(y), y);
    if (compare_magnitude(x, y) > 0) {
      Big t;
      t.assign(x);
      x.assign(y);
      y.assign(t);
    }
    subtract_magnitude(y, x, y, false);
  }
  return shift_left(x, common, out);
}

Outcome isqrt(const Big& a, Big& out) noexcept {
  if (a.negative()) return fail(Reason::arithmetic_invariant);
  if (a.is_zero()) {
    out.assign(a);
    return {};
  }
  // Proposition binary64 sur les 64 bits de tete (decalage pair) ; elle n'est qu'un point de depart.
  const u32 bits = a.bit_length();
  const u32 shift = bits > 64 ? ((bits - 63) & ~u32{1}) : 0;
  Big top;
  shift_right(a, shift, top);
  const double root = std::sqrt(static_cast<double>(top.word(0)));
  const u64 guess = root >= 0x1p32 ? (u64{1} << 32) : static_cast<u64>(root) + 1;
  Big x;
  MHGP11_TRY(shift_left(Big::from_u64(guess), shift / 2, x));
  // Un pas de Newton depuis tout x > 0 rend y >= isqrt(a) ; puis la suite decroit strictement jusqu'a isqrt(a).
  Big q, r, y;
  bool first = true;
  for (;;) {
    MHGP11_TRY(divide(a, x, q, r));
    MHGP11_TRY(add(x, q, y));
    shift_right(y, 1, y);
    if (!first && compare(y, x) >= 0) break;
    first = false;
    x.assign(y);
  }
  // Certificat entier : x^2 <= a et a - x^2 <= 2x, soit a < (x+1)^2.
  Big square, gap, twice;
  MHGP11_TRY(multiply(x, x, square));
  MHGP11_TRY(subtract(a, square, gap));
  MHGP11_TRY(shift_left(x, 1, twice));
  if (gap.negative() || compare(gap, twice) > 0) return fail(Reason::arithmetic_invariant);
  out.assign(x);
  return {};
}

Outcome perfect_square(const Big& a, bool& square, Big& root) noexcept {
  square = false;
  if (a.negative()) {
    A::set(root, 0, false);
    return {};
  }
  MHGP11_TRY(isqrt(a, root));
  Big check;
  MHGP11_TRY(multiply(root, root, check));
  square = compare(check, a) == 0;
  return {};
}

u32 residue(const Big& a, u32 p) noexcept {
  if (p <= 1) return 0;
  u64 rest = 0;
  for (u32 ii = a.size(); ii > 0; --ii)
    rest = static_cast<u64>(((static_cast<u128>(rest) << 64) | a.word(ii - 1)) % p);
  if (a.negative() && rest != 0) rest = p - rest;
  return static_cast<u32>(rest);
}

u32 divide_small(const Big& a, u32 p, Big& out) noexcept {
  if (p == 0) return 0;
  const u32 la = a.size();
  const u64* pa = A::words(a);
  u64* po = A::words(out);
  u64 rest = 0;
  for (u32 ii = la; ii > 0; --ii) {
    const u128 current = (static_cast<u128>(rest) << 64) | pa[ii - 1];
    po[ii - 1] = static_cast<u64>(current / p);
    rest = static_cast<u64>(current % p);
  }
  A::set(out, la, a.negative());
  return static_cast<u32>(rest);
}

}  // namespace mhgp11::num
