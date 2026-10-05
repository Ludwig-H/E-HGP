// Sonde des comparaisons exactes de racines pour la porte mhgp11_num_radical (radical_gate.py). Une requete par ligne,
// rationnels en hexadecimal signe "<n>/<d>" :
//   cmp2 <a> <b> <c> <d>          signe de (sqrt a + sqrt b) - (sqrt c + sqrt d)            -> ok <signe>
//   diff <x> <y> <u>              signe de sqrt x - sqrt y - u                              -> ok <signe>
//   sum <c1> <f1> [<c2> <f2> ...] signe de sum c_i sqrt(f_i)                                -> ok <signe> <classes> <bits>
//   dates <t> <m> <q> <t2> <m2> <q2>  ordre de sqrt t + sqrt m - sqrt q et de la seconde date -> ok <signe> <classes> <bits>
// ou "refused <raison>". Le RadicalSum est alloue une fois dans un budget fini et rendu a la fin (code 3 sinon).
// Code 0, 2 sur une ligne illisible.
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "big_io.hpp"

using namespace mhgp11;
using namespace mhgp11::num;
using namespace mhgp11::num::probe;

namespace {

void answer(const Outcome& outcome, int sign, const RadicalSum* sum) {
  if (!outcome.ok()) {
    refused(outcome);
    return;
  }
  std::cout << "ok " << sign;
  if (sum != nullptr) std::cout << ' ' << sum->trace().classes << ' ' << sum->trace().bits;
  std::cout << '\n';
}

// Lit les rationnels de la ligne ; un refus de lecture est rendu comme une reponse.
bool read_all(std::istringstream& in, std::vector<Rational>& values, Outcome& parsed) {
  std::string word;
  while (in >> word) {
    values.emplace_back();
    const Outcome one = parse_rational(word, values.back());
    if (!one.ok() && parsed.ok()) parsed = one;
  }
  return true;
}

bool run_line(const std::string& line, RadicalSum& sum) {
  std::istringstream in(line);
  std::string op;
  if (!(in >> op)) return false;
  std::vector<Rational> v;
  Outcome parsed{};
  read_all(in, v, parsed);
  if (!parsed.ok()) {
    refused(parsed);
    return true;
  }
  // L'issue est calculee avant de lire le signe : l'ordre d'evaluation des arguments n'est pas specifie.
  int sign = 0;
  if (op == "cmp2" && v.size() == 4) {
    const Outcome outcome = sqrt_cmp2(v[0], v[1], v[2], v[3], sign);
    answer(outcome, sign, nullptr);
  } else if (op == "diff" && v.size() == 3) {
    const Outcome outcome = sqrt_diff_cmp(v[0], v[1], v[2], sign);
    answer(outcome, sign, nullptr);
  } else if (op == "dates" && v.size() == 6) {
    const Outcome outcome = compare_dates({v[0], v[1], v[2]}, {v[3], v[4], v[5]}, sum, sign);
    answer(outcome, sign, &sum);
  } else if (op == "sum" && !v.empty() && v.size() % 2 == 0) {
    sum.clear();
    Outcome added{};
    for (std::size_t i = 0; i < v.size() && added.ok(); i += 2) added = sum.add(v[i], v[i + 1]);
    if (!added.ok()) {
      refused(added);
      return true;
    }
    const Outcome outcome = sum.sign(sign);
    answer(outcome, sign, &sum);
  } else {
    return false;
  }
  return true;
}

}  // namespace

int main() {
  MemoryBudget budget(RadicalSum::bytes());
  int code = 0;
  {
    auto made = RadicalSum::make(budget);
    if (!made.ok()) return exit_code(made.outcome());
    RadicalSum sum = std::move(made).take();
    std::string line;
    while (std::getline(std::cin, line)) {
      if (line.empty()) continue;
      if (!run_line(line, sum)) {
        code = 2;
        break;
      }
    }
  }
  if (!budget.released().ok()) return 3;
  return code;
}
