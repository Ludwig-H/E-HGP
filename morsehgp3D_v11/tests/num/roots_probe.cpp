// Sonde de num::RootTable pour la porte mhgp11_num_roots (roots_gate.py). Une requete par ligne, hexadecimal :
//   root <N> <D>              R = isqrt(floor(N 2^128 / D)) du niveau N / D (Level::make, puis root_of)
//   level <N> <D>             ajoute un niveau a la liste courante (refus de Level::make ecrit, niveau non ajoute)
//   build                     table de tous les niveaux de la liste (RootTable::build)
//   partial <r>...            table de la liste ou seuls les rangs donnes sont calcules (allocate, fill_ranks)
//   get <r>                   R_r de la table courante, ou "absent"
//   sum <r> <s> [<r> <s>...]  encadrement et signe de sum s_i sqrt(l_{r_i}) (s = 1 ou -1, decimal)
//                             -> ok <lo> <hi> <signe> <repli 0|1> <classes> <bits>
// ou "refused <raison>". Premiere ligne : "bits <B>". Budget fini, rendu a la fin (code 3 sinon) ; 2 sur une ligne
// illisible.
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "big_io.hpp"

using namespace mhgp11;
using namespace mhgp11::num;
using namespace mhgp11::num::probe;

namespace {

struct State {
  std::vector<Level> levels;
  RootTable table;
  bool has_table = false;
};

Outcome parse_level(std::istringstream& in, Level& out) {
  std::string n_text, d_text;
  if (!(in >> n_text >> d_text)) return fail(Reason::parameter_out_of_range);
  Big n, d;
  MHGP11_TRY(parse_big(n_text, n));
  MHGP11_TRY(parse_big(d_text, d));
  if (n.size() > 5 || d.size() > 5) return fail(Reason::parameter_out_of_range);
  Wide<5> wn, wd;
  for (u32 i = 0; i < n.size(); ++i) wn.words[static_cast<std::size_t>(i)] = n.word(i);
  for (u32 i = 0; i < d.size(); ++i) wd.words[static_cast<std::size_t>(i)] = d.word(i);
  wn.neg = n.negative();
  wd.neg = d.negative();
  auto level = Level::make(wn, wd);
  if (!level.ok()) return level.outcome();
  out = level.value();
  return {};
}

void sum_line(std::istringstream& in, State& state, RadicalSum& scratch) {
  std::vector<SignedRank> terms;
  long long r = 0, s = 0;
  while (in >> r >> s) terms.push_back({static_cast<u32>(r), static_cast<i32>(s)});
  i128 lo = 0, hi = 0;
  int sign = 0;
  bool fallback = false;
  Outcome outcome = state.has_table ? state.table.bracket(terms, lo, hi) : fail(Reason::arithmetic_invariant);
  if (outcome.ok()) outcome = state.table.sign(terms, scratch, sign, &fallback);
  if (!outcome.ok()) {
    refused(outcome);
    return;
  }
  std::cout << "ok " << hex_signed128(lo) << ' ' << hex_signed128(hi) << ' ' << sign << ' ' << (fallback ? 1 : 0)
            << ' ' << (fallback ? scratch.trace().classes : 0) << ' ' << (fallback ? scratch.trace().bits : 0) << '\n';
}

bool run_line(const std::string& line, State& state, MemoryBudget& budget, RadicalSum& scratch) {
  std::istringstream in(line);
  std::string op;
  if (!(in >> op)) return false;
  if (op == "root" || op == "level") {
    Level level;
    const Outcome parsed = parse_level(in, level);
    if (!parsed.ok()) {
      refused(parsed);
      return true;
    }
    if (op == "level") {
      state.has_table = false;  // la table emprunte la liste : elle est perimee des que la liste change
      state.table = RootTable();
      state.levels.push_back(level);
      std::cout << "ok " << state.levels.size() - 1 << '\n';
      return true;
    }
    u128 root = 0;
    const Outcome outcome = RootTable::root_of(level, root);
    if (!outcome.ok()) refused(outcome);
    else std::cout << "ok " << hex128(root) << '\n';
    return true;
  }
  if (op == "build" || op == "partial") {
    state.table = RootTable();
    state.has_table = false;
    auto made = op == "build" ? RootTable::build(state.levels, budget) : RootTable::allocate(state.levels, budget);
    if (!made.ok()) {
      refused(made.outcome());
      return true;
    }
    state.table = std::move(made).take();
    std::vector<u32> ranks;
    long long r = 0;
    while (in >> r) ranks.push_back(static_cast<u32>(r));
    const Outcome filled = op == "partial" ? state.table.fill_ranks(ranks) : Outcome{};
    if (!filled.ok()) {
      refused(filled);
      return true;
    }
    state.has_table = true;
    std::cout << "ok " << state.table.size() << '\n';
    return true;
  }
  if (op == "get") {
    long long r = 0;
    if (!(in >> r)) return false;
    const auto root = state.has_table ? state.table.root(static_cast<u32>(r)) : std::nullopt;
    if (root) std::cout << "ok " << hex128(*root) << '\n';
    else std::cout << "absent\n";
    return true;
  }
  if (op == "sum") {
    sum_line(in, state, scratch);
    return true;
  }
  return false;
}

}  // namespace

int main() {
  std::cout << "bits " << kCoordBits << '\n';
  MemoryBudget budget(u64{1} << 30);
  int code = 0;
  {
    auto made = RadicalSum::make(budget);
    if (!made.ok()) return exit_code(made.outcome());
    RadicalSum scratch = std::move(made).take();
    State state;
    std::string line;
    while (std::getline(std::cin, line)) {
      if (line.empty()) continue;
      if (!run_line(line, state, budget, scratch)) {
        code = 2;
        break;
      }
    }
  }
  if (!budget.released().ok()) return 3;
  return code;
}
