// Sonde du module supports (hors produit) : Q_b, fermeture et comptes du lemme G de boules de Cat_K sur une entree
// entiere, en lignes JSON canoniques ; mesure de ball_supports sur toute la fenetre W_K ; contre-epreuve des registres
// de la foret d'ordre K (voie sequentielle de reference de tower).
//
//   mhgp11_supports_probe <entree> --k=<K> <selection> [--forest] [--workers=<W>] [--budget=<octets>]
//                         [--min-balls=<N>] [--min-extended=<N>] [--min-supports=<N>]
//   entree     --input=<xyz.u32le>,<ids.u32le>   fichiers petits-boutistes (bench/whole_input.hpp)
//              --data=<nom>                      <nom>.u32le et <nom>.ids.u32le du dossier MHGP11_DATA_DIR
//              --sphere=<r2>,<t>                 les points entiers de x^2 + y^2 + z^2 = r2 translates de (t, t, t),
//                                                ordre lexicographique, PointId 0.. (r2 = 50 : 84 points, fixture 13)
//   selection  --all                             une ligne par boule de Cat_K (petits nuages)
//              --sample=<N>,<graine> [--extended=<E>]
//                                                N boules distinctes de Cat_K tirees a graine fixe (splitmix64), plus
//                                                E coquilles etendues tirees de meme (toutes s'il y en a moins)
//              --window                          toutes les boules de W_K = { p + m >= K } : agregats seulement
//   --workers  W >= 1 : catalogue par l'API parallele publique (meme Cat_K) ; defaut 0, voie sequentielle de
//              reference. Les supports et les comptes restent calcules en sequence, dans l'ordre des BallIdx.
//   --forest   avec --window : foret d'ordre K (build_forest, sans memo ni parallelisme) ; ses registres doivent
//              egaler les comptes : classified_cells = |W_K|, classification.combinations = somme C(m,t),
//              replayed_cells = nombre de cellules (strict_traces > 0), cells.combinations = somme C(m,t) des
//              cellules, trace_resolutions = somme strict_traces, births = naissances (K >= 2) ou sites (K = 1).
// Pre-passe : plafond check_shell sur toutes les coquilles etendues choisies, avant tout calcul ; un refus ne publie
// aucune ligne de boule (les lignes sont retenues jusqu'a la fin). Codes : 0 conforme ; 1 ecart des registres ;
// 2 refus (usage, entree, catalogue, support_shell_capacity) ; 3 invariant viole ou plancher non atteint.
// Derniere ligne, sans duree, gravable :
//   supports_probe_verdict conforme k=<K> n=<sites> boules=<|Cat_K|> choisies=<c> etendues=<e> supports=<s>
//     multiples=<boules a plusieurs supports> coquille_max=<m> entree=<fnv1a64 des octets xyz puis ids>
//   supports_probe_verdict <ecart|plancher|refus <raison>>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "../../bench/whole_input.hpp"
#include "sched/sched.hpp"
#include "supports/supports.hpp"
#include "tower/forest.hpp"

using namespace mhgp11;

namespace {

enum class Mode : u8 { none, all, sample, window };

struct Options {
  std::string xyz, ids, data;
  u64 r2 = 0, shift = 0, k = 0, budget = u64{32} << 30, sample = 0, seed = 0, extended = 0, workers = 0;
  u64 min_balls = 0, min_extended = 0, min_supports = 0;
  Mode mode = Mode::none;
  bool forest = false, sphere = false;
};

bool pair_of(std::string_view text, u64& a, u64& b) {
  const auto comma = text.find(',');
  return comma != std::string_view::npos && bench::parse(text.substr(0, comma), a) &&
         bench::parse(text.substr(comma + 1), b);
}

bool parse_options(int argc, char** argv, Options& o) {
  int inputs = 0, modes = 0;
  for (int i = 1; i < argc; ++i) {
    const std::string_view a(argv[i]);
    std::string_view v;
    auto value = [&](std::string_view key) {
      if (a.substr(0, key.size()) != key) return false;
      v = a.substr(key.size());
      return true;
    };
    if (value("--input=")) {
      const auto comma = v.find(',');
      if (comma == std::string_view::npos) return false;
      o.xyz = std::string(v.substr(0, comma));
      o.ids = std::string(v.substr(comma + 1));
      ++inputs;
    } else if (value("--data=")) {
      if (v.empty() || v.find('/') != std::string_view::npos) return false;
      o.data = std::string(v);
      ++inputs;
    } else if (value("--sphere=")) {
      if (!pair_of(v, o.r2, o.shift) || o.r2 == 0 || o.r2 > 10000 || o.shift > 100000) return false;
      o.sphere = true;
      ++inputs;
    } else if (value("--k=")) {
      if (!bench::parse(v, o.k)) return false;
    } else if (value("--budget=")) {
      if (!bench::parse(v, o.budget)) return false;
    } else if (value("--workers=")) {
      if (!bench::parse(v, o.workers) || o.workers > sched::kMaxWorkers) return false;
    } else if (value("--sample=")) {
      if (!pair_of(v, o.sample, o.seed) || o.sample == 0 || o.sample > 100000) return false;
      o.mode = Mode::sample;
      ++modes;
    } else if (value("--extended=")) {
      if (!bench::parse(v, o.extended) || o.extended > 100000) return false;
    } else if (value("--min-balls=")) {
      if (!bench::parse(v, o.min_balls)) return false;
    } else if (value("--min-extended=")) {
      if (!bench::parse(v, o.min_extended)) return false;
    } else if (value("--min-supports=")) {
      if (!bench::parse(v, o.min_supports)) return false;
    } else if (a == "--all") {
      o.mode = Mode::all;
      ++modes;
    } else if (a == "--window") {
      o.mode = Mode::window;
      ++modes;
    } else if (a == "--forest") {
      o.forest = true;
    } else {
      return false;
    }
  }
  if (inputs != 1 || modes != 1 || o.k < 1 || o.k > supports::kMaxOrder) return false;
  if (o.extended != 0 && o.mode != Mode::sample) return false;
  return !o.forest || o.mode == Mode::window;
}

// Points entiers de la sphere x^2 + y^2 + z^2 = r2, translates de (t, t, t) ; ordre lexicographique de (x, y, z).
Result<bench::Input> sphere_points(u64 r2, u64 shift, MemoryBudget& budget) {
  std::vector<std::array<u32, 3>> points;
  const i64 r = static_cast<i64>(r2), t = static_cast<i64>(shift);
  for (i64 x = -100; x <= 100; ++x)
    for (i64 y = -100; y <= 100; ++y)
      for (i64 z = -100; z <= 100; ++z)
        if (x * x + y * y + z * z == r) {
          if (x + t < 0 || y + t < 0 || z + t < 0) return fail(Reason::parameter_out_of_range);
          points.push_back({static_cast<u32>(x + t), static_cast<u32>(y + t), static_cast<u32>(z + t)});
        }
  if (points.empty()) return fail(Reason::parameter_out_of_range);
  bench::Input input;
  MHGP11_TRY(input.x.allocate(points.size(), budget));
  MHGP11_TRY(input.y.allocate(points.size(), budget));
  MHGP11_TRY(input.z.allocate(points.size(), budget));
  MHGP11_TRY(input.ids.allocate(points.size(), budget));
  for (std::size_t i = 0; i < points.size(); ++i) {
    input.x[i] = points[i][0];
    input.y[i] = points[i][1];
    input.z[i] = points[i][2];
    input.ids[i] = make_id<PointId>(static_cast<u32>(i));
  }
  return input;
}

u64 fnv1a(const bench::Input& input) {
  u64 hash = 0xcbf29ce484222325ull;
  auto feed = [&](u32 word) {
    for (int b = 0; b < 4; ++b) hash = (hash ^ ((word >> (8 * b)) & 255)) * 0x100000001b3ull;
  };
  for (u64 i = 0; i < input.x.size(); ++i) {
    feed(input.x[i]);
    feed(input.y[i]);
    feed(input.z[i]);
  }
  for (PointId id : input.ids) feed(idx(id));
  return hash;
}

std::string hex64(u64 value) {
  std::string out(16, '0');
  for (int i = 15; i >= 0; --i, value >>= 4) out[i] = "0123456789abcdef"[value & 15];
  return out;
}

template <class T>
std::string hex(const T& value) {
  const auto wide = num::to_wide(value);
  std::ostringstream out;
  if (wide.sign() < 0) out << '-';
  out << std::hex << std::setfill('0');
  for (std::size_t i = wide.words.size(); i != 0; --i) out << std::setw(16) << wide.words[i - 1];
  return out.str();
}

// splitmix64 : tirages reproductibles, independants de la bibliotheque standard.
struct SplitMix {
  u64 state;
  u64 next() noexcept {
    u64 z = (state += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
  }
};

// `count` elements distincts de `pool` (tous s'il y en a moins), par tirages avec rejet ; resultat croissant.
std::vector<u32> draw(const std::vector<u32>& pool, u64 count, SplitMix& random) {
  if (pool.size() <= count) return pool;
  std::set<u32> chosen;
  while (chosen.size() < count) chosen.insert(pool[random.next() % pool.size()]);
  return {chosen.begin(), chosen.end()};
}

struct Totals {
  u64 balls = 0, extended = 0, supports = 0, multiple = 0, max_shell = 0, births = 0, cells = 0;
  u64 kparties = 0, compressed = 0, compressed_cells = 0, strict = 0, cofaces = 0, max_cofaces = 0, incidences = 0;
  std::array<u64, 5> by_arity{};
  std::array<u64, supports::kMaxShell + 1> by_shell{};
  supports::SupportLedger ledger;
};

struct Run {
  Options options;
  u64 n = 0, fnv = 0, catalogue_balls = 0;
  Totals totals;
  std::string lines;   // lignes de boules, publiees seulement apres le succes de toute la selection
  bool disagreement = false, floor = false;
};

void site_list(std::ostream& out, const Cloud& cloud, std::span<const SiteIdx> sites, bool ids) {
  out << '[';
  for (std::size_t i = 0; i < sites.size(); ++i) {
    out << (i == 0 ? "" : ",");
    if (ids) out << idx(cloud.points(sites[i])[0]);
    else out << idx(sites[i]);
  }
  out << ']';
}

// Une boule : Q_b, fermeture et comptes ; agregats ; ligne JSON si `record`.
Outcome one_ball(const FullDomain& domain, BallIdx ball, Order k, std::span<supports::Support> out,
                 std::span<u64> scratch, Run& run, bool record) {
  const auto made = supports::ball_supports(domain, ball, out, scratch, &run.totals.ledger);
  if (!made.ok()) return made.outcome();
  const auto shape = supports::ball_shape(domain, ball, k);
  if (!shape.ok()) return shape.outcome();
  const auto counts = supports::ball_counts(shape.value(), made.value().closure);
  if (!counts.ok()) return counts.outcome();
  const auto& data = domain.catalogue().balls_data()[idx(ball)];
  const auto& c = counts.value();
  Totals& t = run.totals;
  const u32 found = made.value().count;
  ++t.balls;
  t.extended += data.m > data.qmin ? 1 : 0;
  t.supports += found;
  t.multiple += found > 1 ? 1 : 0;
  t.max_shell = std::max<u64>(t.max_shell, data.m);
  ++t.by_shell[data.m];
  t.births += c.strict_traces == 0 ? 1 : 0;
  t.cells += c.strict_traces > 0 ? 1 : 0;
  t.kparties += c.kparties_reliees;
  t.compressed += c.compressed_parts;
  t.compressed_cells += c.strict_traces > 0 ? c.compressed_parts : 0;
  t.strict += c.strict_traces;
  t.cofaces += c.cofaces;
  t.max_cofaces = std::max<u64>(t.max_cofaces, c.cofaces);
  for (u32 i = 0; i < found; ++i) {
    ++t.by_arity[out[i].arity];
    t.incidences += supports::support_cofaces(shape.value(), out[i].arity);
  }
  if (!record) return {};
  const Cloud& cloud = domain.index().cloud();
  const auto& level = domain.catalogue().levels()[idx(data.rank)];
  std::ostringstream line;
  line << "{\"phase\":\"ball\",\"ball\":" << idx(ball) << ",\"rank\":" << idx(data.rank) << ",\"level\":[\""
       << hex(level.numerator()) << "\",\"" << hex(level.denominator()) << "\"],\"p\":" << data.p << ",\"m\":" << data.m
       << ",\"qmin\":" << unsigned{data.qmin} << ",\"star\":";
  site_list(line, cloud, std::span<const SiteIdx>(data.support).first(data.qmin), true);
  line << ",\"supports\":[";
  for (u32 i = 0; i < found; ++i) {
    line << (i == 0 ? "" : ",");
    site_list(line, cloud, std::span<const SiteIdx>(out[i].sites).first(out[i].arity), true);
  }
  line << "],\"sites\":[";
  for (u32 i = 0; i < found; ++i) {
    line << (i == 0 ? "" : ",");
    site_list(line, cloud, std::span<const SiteIdx>(out[i].sites).first(out[i].arity), false);
  }
  line << "],\"closure\":[";
  for (u32 j = 0; j <= data.m; ++j) line << (j == 0 ? "" : ",") << made.value().closure.parts(j);
  line << "],\"counts\":{\"kparties_reliees\":" << c.kparties_reliees << ",\"compressed_parts\":" << c.compressed_parts
       << ",\"strict_traces\":" << c.strict_traces << ",\"cofaces\":" << c.cofaces
       << ",\"gabriel_cofaces\":" << c.gabriel_cofaces << "},\"cofaces_support\":[";
  for (u32 i = 0; i < found; ++i) line << (i == 0 ? "" : ",") << supports::support_cofaces(shape.value(), out[i].arity);
  line << "],\"gabriel_cofaces_support\":[";
  for (u32 i = 0; i < found; ++i)
    line << (i == 0 ? "" : ",") << supports::support_gabriel_cofaces(shape.value(), out[i].arity);
  line << "]}\n";
  run.lines += line.str();
  return {};
}

// Boules choisies selon le mode (BallIdx croissants ; --sample : tirage, puis coquilles etendues).
std::vector<u32> selection(const FullDomain& domain, const Options& o) {
  const auto balls = domain.catalogue().balls_data();
  std::vector<u32> all, extended;
  for (u32 b = 0; b < balls.size(); ++b) {
    if (o.mode == Mode::window && u64{balls[b].p} + balls[b].m < o.k) continue;
    all.push_back(b);
    if (balls[b].m > balls[b].qmin) extended.push_back(b);
  }
  if (o.mode != Mode::sample) return all;
  SplitMix random{o.seed};
  std::vector<u32> chosen = draw(all, o.sample, random);
  const std::vector<u32> more = draw(extended, o.extended, random);
  chosen.insert(chosen.end(), more.begin(), more.end());
  std::sort(chosen.begin(), chosen.end());
  chosen.erase(std::unique(chosen.begin(), chosen.end()), chosen.end());
  return chosen;
}

// Mesure de ball_supports seul sur W_K (--window) : coquilles regulieres, puis etendues, sans comptes ni agregats.
Outcome measure(const FullDomain& domain, const std::vector<u32>& chosen, std::span<supports::Support> out,
                std::span<u64> scratch) {
  std::array<std::vector<u32>, 2> lists;
  for (u32 b : chosen) {
    const auto& data = domain.catalogue().balls_data()[b];
    lists[data.m > data.qmin ? 1 : 0].push_back(b);
  }
  std::array<i64, 2> ns{};
  for (int pass = 0; pass < 2; ++pass) {
    const auto begin = std::chrono::steady_clock::now();
    for (u32 b : lists[pass]) {
      const auto made = supports::ball_supports(domain, BallIdx{b}, out, scratch);
      if (!made.ok()) return made.outcome();
    }
    ns[pass] = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - begin).count();
  }
  std::cout << "{\"phase\":\"measure\",\"regular\":" << lists[0].size() << ",\"regular_ns\":" << ns[0]
            << ",\"extended\":" << lists[1].size() << ",\"extended_ns\":" << ns[1] << "}\n";
  return {};
}

void forest_check(const FullDomain& domain, Run& run, MemoryBudget& budget, Outcome& product) {
  const auto start = std::chrono::steady_clock::now();
  auto forest = tower_detail::build_forest(domain, static_cast<u32>(run.options.k), budget);
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start);
  if (!forest.ok()) {
    product = forest.outcome();
    return;
  }
  const auto& l = forest.value().ledger();
  const Totals& t = run.totals;
  const u64 births = run.options.k == 1 ? run.n : t.births;
  const std::array<std::array<u64, 2>, 6> pairs = {{{l.classified_cells, t.balls},
                                                    {l.classification.combinations, t.compressed},
                                                    {l.replayed_cells, t.cells},
                                                    {l.cells.combinations, t.compressed_cells},
                                                    {l.trace_resolutions, t.strict},
                                                    {forest.value().births(), births}}};
  const char* names[6] = {"classified_cells", "classification_combinations", "replayed_cells", "cells_combinations",
                          "trace_resolutions", "births"};
  std::cout << "{\"phase\":\"forest\",\"wall_ns\":" << ns.count();
  for (int i = 0; i < 6; ++i) {
    std::cout << ",\"" << names[i] << "\":[" << pairs[i][0] << ',' << pairs[i][1] << ']';
    run.disagreement = run.disagreement || pairs[i][0] != pairs[i][1];
  }
  std::cout << "}\n";
}

Outcome execute(Run& run) {
  const Options& o = run.options;
  MemoryBudget budget(o.budget);
  Result<bench::Input> input = fail(Reason::input_unreadable);
  if (o.sphere) {
    input = sphere_points(o.r2, o.shift, budget);
  } else if (!o.data.empty()) {
    const char* folder = std::getenv("MHGP11_DATA_DIR");
    if (folder != nullptr && *folder != '\0') {
      const std::string base = std::string(folder) + "/" + o.data;
      input = bench::read_input((base + ".u32le").c_str(), (base + ".ids.u32le").c_str(), budget);
    }
  } else {
    input = bench::read_input(o.xyz.c_str(), o.ids.c_str(), budget);
  }
  if (!input.ok()) return input.outcome();
  run.fnv = fnv1a(input.value());
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                             input.value().ids.span(), CoordWidth(), budget);
  if (!cloud.ok()) return cloud.outcome();
  input.value() = {};
  run.n = cloud.value().sites();
  auto index = build_index(std::move(cloud.value()), IndexParams{8}, budget);
  if (!index.ok()) return index.outcome();
  CatalogueParams params;
  params.kmax = static_cast<int>(o.k);
  std::unique_ptr<sched::Pool> pool;
  if (o.workers != 0) {
    auto made = sched::make_pool({static_cast<u32>(o.workers)});
    if (!made.ok()) return made.outcome();
    pool = std::move(made.value());
  }
  auto start = std::chrono::steady_clock::now();
  auto domain = pool == nullptr ? prepare_full_domain(std::move(index.value()), params, budget)
                                : prepare_full_domain(std::move(index.value()), params, budget, *pool);
  if (!domain.ok()) return domain.outcome();
  const FullDomain& d = domain.value();
  run.catalogue_balls = d.catalogue().balls();
  std::cout << "{\"phase\":\"domain\",\"sites\":" << run.n << ",\"balls\":" << run.catalogue_balls
            << ",\"input_fnv1a64\":\"" << hex64(run.fnv) << "\",\"wall_ns\":"
            << std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count()
            << "}\n";
  const std::vector<u32> chosen = selection(d, o);
  u32 widest = 0;  // pre-passe : plafond de toute la selection, avant tout calcul
  for (u32 b : chosen) {
    const auto& data = d.catalogue().balls_data()[b];
    if (data.m > data.qmin) {
      MHGP11_TRY(supports::check_shell(data.m));
      widest = std::max(widest, data.m);
    }
  }
  std::vector<supports::Support> out(std::max<u32>(1, supports::support_capacity(widest)));
  std::vector<u64> scratch(std::max<u64>(1, supports::closure_words(widest)));
  if (o.mode == Mode::window) MHGP11_TRY(measure(d, chosen, out, scratch));
  start = std::chrono::steady_clock::now();
  for (u32 b : chosen)
    MHGP11_TRY(one_ball(d, BallIdx{b}, static_cast<Order>(o.k), out, scratch, run, o.mode != Mode::window));
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start);
  std::cout << run.lines;
  const Totals& t = run.totals;
  const auto& l = t.ledger;
  std::cout << "{\"phase\":\"supports\",\"selected\":" << t.balls << ",\"extended\":" << t.extended
            << ",\"supports\":" << t.supports << ",\"by_arity\":[" << t.by_arity[2] << ',' << t.by_arity[3] << ','
            << t.by_arity[4] << "],\"multiple\":" << t.multiple << ",\"max_shell\":" << t.max_shell
            << ",\"extended_by_shell\":{";
  bool first = true;
  for (u32 m = 0; m <= supports::kMaxShell; ++m)
    if (t.by_shell[m] != 0 && m > 4) {
      std::cout << (first ? "" : ",") << '"' << m << "\":" << t.by_shell[m];
      first = false;
    }
  std::cout << "},\"births\":" << t.births << ",\"cells\":" << t.cells << ",\"kparties_reliees\":" << t.kparties
            << ",\"compressed_parts\":" << t.compressed << ",\"strict_traces\":" << t.strict
            << ",\"cofaces\":" << t.cofaces << ",\"max_cofaces\":" << t.max_cofaces
            << ",\"incidences\":" << t.incidences << ",\"ledger\":{\"regular\":" << l.regular
            << ",\"extended\":" << l.extended << ",\"midpoint_tests\":" << l.midpoint_tests
            << ",\"acute_tests\":" << l.acute_tests << ",\"orientation_tests\":" << l.orientation_tests
            << ",\"inside_tests\":" << l.inside_tests << "},\"wall_ns\":" << ns.count() << "}\n";
  Outcome product{};
  if (o.forest) forest_check(d, run, budget, product);
  MHGP11_TRY(product);
  run.floor = t.balls < o.min_balls || t.extended < o.min_extended || t.supports < o.min_supports;
  return {};
}

}  // namespace

int main(int argc, char** argv) {
  Run run;
  if (!parse_options(argc, argv, run.options)) {
    std::cout << "supports_probe_verdict refus usage\n";
    return 2;
  }
  const Outcome product = guarded([&]() { return execute(run); });
  if (!product.ok()) {
    std::cout << "supports_probe_verdict refus " << reason_name(product.reason) << '\n';
    return exit_code(product);
  }
  const Totals& t = run.totals;
  std::cout << "supports_probe_verdict "
            << (run.disagreement ? "ecart" : run.floor ? "plancher" : "conforme") << " k=" << run.options.k
            << " n=" << run.n << " boules=" << run.catalogue_balls << " choisies=" << t.balls
            << " etendues=" << t.extended << " supports=" << t.supports << " multiples=" << t.multiple
            << " coquille_max=" << t.max_shell << " entree=" << hex64(run.fnv) << '\n';
  return run.disagreement ? 1 : run.floor ? 3 : 0;
}
