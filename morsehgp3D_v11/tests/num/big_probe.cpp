// Sonde de num::Big pour la porte mhgp11_num_big (big_gate.py) : une operation par ligne, en hexadecimal signe.
//   add|sub|mul|div|gcd|cmp <a> <b>    isqrt|square|bits <a>    mod <a> <p decimal>    shl|shr <a> <k decimal>
// Reponse : "ok <resultats>" (div : quotient et reste ; square : 0|1 et racine) ou "refused <raison>".
// Premiere ligne : "capacity <bits>". Code 0, ou 2 sur une ligne illisible.
#include <iostream>
#include <sstream>
#include <string>

#include "big_io.hpp"

using namespace mhgp11;
using namespace mhgp11::num;
using namespace mhgp11::num::probe;

namespace {

bool run_line(const std::string& line) {
  std::istringstream in(line);
  std::string op, first, second;
  if (!(in >> op >> first)) return false;
  in >> second;
  Big a, b, out, other;
  if (const Outcome parsed = parse_big(first, a); !parsed.ok()) {
    refused(parsed);
    return true;
  }
  const bool unary = op == "isqrt" || op == "square" || op == "bits";
  const bool small = op == "mod" || op == "shl" || op == "shr";
  if (!unary && second.empty()) return false;
  u64 k = 0;
  if (small) {
    try {
      k = std::stoull(second);
    } catch (...) {
      return false;
    }
  } else if (!unary) {
    if (const Outcome parsed = parse_big(second, b); !parsed.ok()) {
      refused(parsed);
      return true;
    }
  }
  Outcome result{};
  if (op == "add") result = add(a, b, out);
  else if (op == "sub") result = subtract(a, b, out);
  else if (op == "mul") result = multiply(a, b, out);
  else if (op == "div") result = divide(a, b, out, other);
  else if (op == "gcd") result = gcd(a, b, out);
  else if (op == "isqrt") result = isqrt(a, out);
  else if (op == "shl") result = shift_left(a, static_cast<u32>(k), out);
  else if (op == "shr") shift_right(a, static_cast<u32>(k), out);
  else if (op == "square") {
    bool square = false;
    result = perfect_square(a, square, out);
    if (result.ok()) std::cout << "ok " << (square ? 1 : 0) << ' ' << hex(out) << '\n';
    return true;
  } else if (op == "cmp") {
    std::cout << "ok " << compare(a, b) << '\n';
    return true;
  } else if (op == "bits") {
    std::cout << "ok " << a.bit_length() << '\n';
    return true;
  } else if (op == "mod") {
    if (k == 0 || k > 0xffffffffu) return false;
    std::cout << "ok " << residue(a, static_cast<u32>(k)) << '\n';
    return true;
  } else {
    return false;
  }
  if (!result.ok()) {
    refused(result);
    return true;
  }
  std::cout << "ok " << hex(out);
  if (op == "div") std::cout << ' ' << hex(other);
  std::cout << '\n';
  return true;
}

}  // namespace

int main() {
  std::cout << "capacity " << kBigCapacityBits << '\n';
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;
    if (!run_line(line)) return 2;
  }
  return 0;
}
