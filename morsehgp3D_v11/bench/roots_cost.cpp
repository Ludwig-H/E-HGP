// Cout de construction de num::RootTable sur tous les rangs du catalogue d'ordre K (tranche S8, amendement L3 de la
// critique : mesure a 8000, 16000, 32000 points et sur les trames, a defaut des rangs references par la future
// sortie points). Hors produit.
//
//   mhgp11_num_roots_cost (--data=<nom> | --xyz=<fichier> --ids=<fichier>) --k=<K> --workers=<W>
//                         [--sample=<m>] [--min-levels=<l>]
//
// --data lit <nom>.u32le et <nom>.ids.u32le du dossier MHGP11_DATA_DIR. Catalogue d'ordre K par la voie de
// production (six options, W fils), puis la table de tous ses niveaux : une fois sur le pilote seul (fill sur
// [0, L)), une fois en parallele (fill par tranches de 4096 rangs sur le Pool) ; les deux tables doivent etre
// identiques rang par rang (positions fixes). Juge d'echantillon : m rangs repartis, certificat entier
// R^2 D <= N 2^128 < (R+1)^2 D refait en num::Big hors de la table. Sortie JSON d'une ligne par phase.
// Codes : 0 conforme ; 1 table differente entre les deux voies ou certificat faux ; 2 refus ; 3 plancher.
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include "catalogue/catalogue.hpp"
#include "sched/sched.hpp"
#include "whole_input.hpp"

using namespace mhgp11;
using namespace mhgp11::bench;

namespace {

struct Options {
  std::string data, xyz, ids;
  u64 k = 0, workers = 0, sample = 4096, min_levels = 1;
};

bool parse_options(int argc, char** argv, Options& o) {
  for (int i = 1; i < argc; ++i) {
    const std::string_view a = argv[i];
    auto value = [&](std::string_view key, std::string_view& out) {
      if (a.substr(0, key.size()) != key) return false;
      out = a.substr(key.size());
      return true;
    };
    std::string_view v;
    if (value("--data=", v)) o.data = v;
    else if (value("--xyz=", v)) o.xyz = v;
    else if (value("--ids=", v)) o.ids = v;
    else if (value("--k=", v)) { if (!parse(v, o.k)) return false; }
    else if (value("--workers=", v)) { if (!parse(v, o.workers)) return false; }
    else if (value("--sample=", v)) { if (!parse(v, o.sample)) return false; }
    else if (value("--min-levels=", v)) { if (!parse(v, o.min_levels)) return false; }
    else return false;
  }
  const bool one_input = o.data.empty() != (o.xyz.empty() || o.ids.empty());
  return one_input && o.k >= 1 && o.k <= 12 && o.workers >= 1 && o.workers <= sched::kMaxWorkers;
}

struct FillContext {
  num::RootTable* table;
};

Outcome fill_slice(void* context, u64 begin, u64 end, u32) noexcept {
  return static_cast<FillContext*>(context)->table->fill(begin, end);
}

// Certificat entier independant de isqrt : R^2 D <= N 2^128 < (R + 1)^2 D.
Outcome certify(const num::Level& level, u128 root, bool& good) {
  using num::Big;
  const Big n = Big::from_wide(num::to_wide(level.numerator()));
  const Big d = Big::from_wide(num::to_wide(level.denominator()));
  Big r = Big::from_u128(root), left, right, scaled, next;
  MHGP11_TRY(num::multiply(r, r, left));
  MHGP11_TRY(num::multiply(left, d, left));
  MHGP11_TRY(num::shift_left(n, 128, scaled));
  MHGP11_TRY(num::add(r, Big::from_u64(1), next));
  MHGP11_TRY(num::multiply(next, next, right));
  MHGP11_TRY(num::multiply(right, d, right));
  good = num::compare(left, scaled) <= 0 && num::compare(scaled, right) < 0;
  return {};
}

Outcome run(const Options& o, int& code) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  Result<Input> input = fail(Reason::input_unreadable);
  if (!o.data.empty()) {
    const char* folder = std::getenv("MHGP11_DATA_DIR");
    if (folder == nullptr || *folder == '\0') return fail(Reason::input_unreadable);
    const std::string base = std::string(folder) + "/" + o.data;
    input = read_input((base + ".u32le").c_str(), (base + ".ids.u32le").c_str(), budget);
  } else {
    input = read_input(o.xyz.c_str(), o.ids.c_str(), budget);
  }
  if (!input.ok()) return input.outcome();
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                             input.value().ids.span(), CoordWidth(), budget);
  if (!cloud.ok()) return cloud.outcome();
  input.value() = {};
  auto pool = sched::make_pool({static_cast<u32>(o.workers)});
  if (!pool.ok()) return pool.outcome();
  CatalogueParams params;
  params.kmax = static_cast<int>(o.k);
  params.cache_center_lines = params.indirect_sort = params.adaptive_frontier = true;
  params.parallel_assembly = params.single_pass = params.pair_graph = true;
  Stopwatch catalogue_clock;
  auto catalogue = build_catalogue(cloud.value(), params, budget, *pool.value(), nullptr, nullptr);
  const u64 catalogue_ns = catalogue_clock.nanoseconds();
  if (!catalogue.ok()) return catalogue.outcome();
  const auto levels = catalogue.value().levels();
  budget.restart_peak();
  const u64 before = budget.used();
  Stopwatch serial_clock;
  auto serial = num::RootTable::build(levels, budget);
  const u64 serial_ns = serial_clock.nanoseconds();
  if (!serial.ok()) return serial.outcome();
  const u64 table_bytes = budget.used() - before;
  auto parallel = num::RootTable::allocate(levels, budget);
  if (!parallel.ok()) return parallel.outcome();
  FillContext context{&parallel.value()};
  Stopwatch parallel_clock;
  MHGP11_TRY(pool.value()->parallel_for(levels.size(), 4096, &context, fill_slice));
  const u64 parallel_ns = parallel_clock.nanoseconds();
  u64 different = 0, sampled = 0, wrong = 0;
  for (u64 r = 0; r < levels.size(); ++r)
    different += serial.value().root(static_cast<u32>(r)) != parallel.value().root(static_cast<u32>(r));
  const u64 stride = o.sample == 0 ? 0 : std::max<u64>(1, levels.size() / o.sample);
  for (u64 r = 0; stride != 0 && r < levels.size(); r += stride) {
    bool good = false;
    MHGP11_TRY(certify(levels[r], *serial.value().root(static_cast<u32>(r)), good));
    ++sampled;
    wrong += !good;
  }
  std::cout << "{\"phase\":\"roots\",\"coord_bits\":" << kCoordBits << ",\"k\":" << o.k << ",\"sites\":"
            << cloud.value().sites() << ",\"balls\":" << catalogue.value().balls() << ",\"levels\":" << levels.size()
            << ",\"catalogue_ns\":" << catalogue_ns << ",\"serial_ns\":" << serial_ns
            << ",\"serial_ns_per_level\":" << (levels.empty() ? 0 : serial_ns / levels.size())
            << ",\"workers\":" << o.workers << ",\"parallel_ns\":" << parallel_ns << ",\"table_bytes\":"
            << table_bytes << ",\"different\":" << different << ",\"sampled\":" << sampled << ",\"wrong\":" << wrong
            << "}\n";
  if (different != 0 || wrong != 0) code = 1;
  else if (levels.size() < o.min_levels || sampled == 0) code = 3;
  return {};
}

}  // namespace

int main(int argc, char** argv) {
  Options o;
  if (!parse_options(argc, argv, o)) return 2;
  int code = 0;
  const Outcome outcome = guarded([&]() { return run(o, code); });
  if (!outcome.ok()) {
    std::cout << "{\"phase\":\"exit\",\"reason\":\"" << reason_name(outcome.reason) << "\"}\n";
    return exit_code(outcome);
  }
  std::cout << "roots_cost_verdict " << (code == 0 ? "conforme" : "ecart") << " k=" << o.k << '\n';
  return code;
}
