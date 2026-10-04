// Sonde du juge d'Euler a K+2 et de la restriction J1 (bench/catalogue_euler.hpp) : construit Cat_K et Cat_{K+2} sur
// une entree entiere, juge Euler aux ordres 1..min(K, n) sur Cat_{K+2} et compare Cat_K au filtre de Cat_{K+2}.
// Hors produit, hors chrono FULL : un filet de securite du catalogue, jamais un certificat de completude.
//
//   mhgp11_catalogue_euler <entree> --k=<K> [options]
//   entree  --input=<xyz.u32le>,<ids.u32le>
//           --data=<nom>               <nom>.u32le et <nom>.ids.u32le du dossier de la variable MHGP11_DATA_DIR
//           --uniform18=<n>,<graine>   famille de random.Random(graine).getrandbits(18) de CPython (x, y, z), PointId
//                                      0..n-1 ; un triplet en double est refuse (aucun rejet silencieux)
//   options --workers=<W>              0 (defaut) : voie sequentielle de reference, sans Pool
//           --production               les six options de la voie FULL courante (exige W >= 1), ou une a une :
//           --cache-center-lines --indirect-sort --adaptive-frontier --parallel-assembly --single-pass --pair-graph
//           --budget=<octets>          plafond du MemoryBudget (defaut 32 Gio)
//           --omit=<PointId>,...       harnais des fixtures de limite : retire de Cat_K et de Cat_{K+2} la boule de ce
//                                      support canonique (refus si elle n'est pas dans Cat_{K+2}) ; --omit-k=... et
//                                      --omit-k2=... la retirent d'un seul des deux (refus si elle n'y est pas)
//           --min-balls=<N> --min-orders=<N> --min-extended=<N> --min-compared=<N>   planchers de couverture
// Codes : 0 conforme ; 1 ecart (Euler different de 1 a un ordre verifiable, restriction, ecart local d'une boule) ;
// 2 refus (usage, entree, catalogue, coquille etendue de plus de 24 sites, omission introuvable) ; 3 plancher non
// atteint, ou invariant viole par le produit. Sorties : une ligne JSON par phase, puis la ligne de verdict, sans
// duree, gravable par LINE :
//   catalogue_euler_verdict <conforme|ecart|refus|plancher> k=<K> n=<sites> ordres=<v> boules_k=<|Cat_K|>
//   boules_k2=<|Cat_K+2|> etendues=<e> coquille_max=<m> entree=<fnv1a64 des octets xyz puis ids>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "catalogue_euler.hpp"
#include "whole_input.hpp"

using namespace mhgp11;
using namespace mhgp11::bench;

namespace {

struct Options {
  std::string xyz, ids, data;
  u64 uniform = 0, seed = 0, k = 0, workers = 0, budget = u64{32} << 30;
  u64 min_balls = 0, min_orders = 0, min_extended = 0, min_compared = 0;
  CatalogueParams params;
  std::vector<std::pair<int, std::vector<u32>>> omit;  // portee : 0 les deux, 1 Cat_K seul, 2 Cat_{K+2} seul
};

bool split_pair(std::string_view text, u64& a, u64& b) {
  const auto comma = text.find(',');
  return comma != std::string_view::npos && parse(text.substr(0, comma), a) && parse(text.substr(comma + 1), b);
}

bool parse_options(int argc, char** argv, Options& o) {
  int inputs = 0;
  for (int i = 1; i < argc; ++i) {
    const std::string_view a(argv[i]);
    auto value = [&](std::string_view key, std::string_view& out) {
      if (a.substr(0, key.size()) != key) return false;
      out = a.substr(key.size());
      return true;
    };
    std::string_view v;
    if (value("--input=", v)) {
      const auto comma = v.find(',');
      if (comma == std::string_view::npos) return false;
      o.xyz = std::string(v.substr(0, comma));
      o.ids = std::string(v.substr(comma + 1));
      ++inputs;
    } else if (value("--data=", v)) {
      if (v.empty() || v.find('/') != std::string_view::npos) return false;
      o.data = std::string(v);
      ++inputs;
    } else if (value("--uniform18=", v)) {
      if (!split_pair(v, o.uniform, o.seed) || o.uniform == 0 || o.uniform > 4000000 || o.seed > 0xFFFFFFFFull)
        return false;
      ++inputs;
    } else if (value("--k=", v)) {
      if (!parse(v, o.k)) return false;
    } else if (value("--workers=", v)) {
      if (!parse(v, o.workers) || o.workers > sched::kMaxWorkers) return false;
    } else if (value("--budget=", v)) {
      if (!parse(v, o.budget)) return false;
    } else if (value("--omit=", v) || value("--omit-k=", v) || value("--omit-k2=", v)) {
      const int scope = a.substr(0, 9) == "--omit-k=" ? 1 : a.substr(0, 10) == "--omit-k2=" ? 2 : 0;
      std::vector<u32> ids;
      for (std::string_view rest = v; !rest.empty();) {
        const auto comma = rest.find(',');
        u64 id = 0;
        if (!parse(rest.substr(0, comma), id) || id > 0xFFFFFFFFull) return false;
        ids.push_back(static_cast<u32>(id));
        rest = comma == std::string_view::npos ? std::string_view{} : rest.substr(comma + 1);
        if (comma != std::string_view::npos && rest.empty()) return false;
      }
      if (ids.size() < 2 || ids.size() > 4) return false;
      o.omit.emplace_back(scope, ids);
    } else if (value("--min-balls=", v)) {
      if (!parse(v, o.min_balls)) return false;
    } else if (value("--min-orders=", v)) {
      if (!parse(v, o.min_orders)) return false;
    } else if (value("--min-extended=", v)) {
      if (!parse(v, o.min_extended)) return false;
    } else if (value("--min-compared=", v)) {
      if (!parse(v, o.min_compared)) return false;
    } else if (a == "--production") {
      o.params.cache_center_lines = o.params.indirect_sort = o.params.adaptive_frontier = true;
      o.params.parallel_assembly = o.params.single_pass = o.params.pair_graph = true;
    } else if (a == "--cache-center-lines") o.params.cache_center_lines = true;
    else if (a == "--indirect-sort") o.params.indirect_sort = true;
    else if (a == "--adaptive-frontier") o.params.adaptive_frontier = true;
    else if (a == "--parallel-assembly") o.params.parallel_assembly = true;
    else if (a == "--single-pass") o.params.single_pass = true;
    else if (a == "--pair-graph") o.params.pair_graph = true;
    else return false;
  }
  // K+2 <= 12 : le catalogue accepte K dans 1..12 (contrat K10 : Cat_12).
  if (inputs != 1 || o.k < 1 || o.k + 2 > euler::kMaxOrder) return false;
  return o.workers != 0 || (!o.params.adaptive_frontier && !o.params.single_pass);
}

// random.Random(graine) de CPython (MT19937, init_by_array sur les mots de 32 bits de la graine), getrandbits(k<=32).
class PythonRandom {
 public:
  explicit PythonRandom(u32 seed) noexcept {
    mt_[0] = 19650218u;
    for (u32 i = 1; i < 624; ++i) mt_[i] = 1812433253u * (mt_[i - 1] ^ (mt_[i - 1] >> 30)) + i;
    u32 i = 1;
    for (u32 k = 624; k != 0; --k) {  // cle d'un seul mot : j vaut toujours 0
      mt_[i] = (mt_[i] ^ ((mt_[i - 1] ^ (mt_[i - 1] >> 30)) * 1664525u)) + seed;
      if (++i >= 624) { mt_[0] = mt_[623]; i = 1; }
    }
    for (u32 k = 623; k != 0; --k) {
      mt_[i] = (mt_[i] ^ ((mt_[i - 1] ^ (mt_[i - 1] >> 30)) * 1566083941u)) - i;
      if (++i >= 624) { mt_[0] = mt_[623]; i = 1; }
    }
    mt_[0] = 0x80000000u;
  }
  u32 bits(int k) noexcept { return next() >> (32 - k); }

 private:
  u32 next() noexcept {
    if (index_ >= 624) {
      for (u32 kk = 0; kk < 624; ++kk) {
        const u32 y = (mt_[kk] & 0x80000000u) | (mt_[(kk + 1) % 624] & 0x7fffffffu);
        mt_[kk] = mt_[(kk + 397) % 624] ^ (y >> 1) ^ ((y & 1u) != 0 ? 0x9908b0dfu : 0u);
      }
      index_ = 0;
    }
    u32 y = mt_[index_++];
    y ^= y >> 11;
    y ^= (y << 7) & 0x9d2c5680u;
    y ^= (y << 15) & 0xefc60000u;
    return y ^ (y >> 18);
  }
  std::array<u32, 624> mt_{};
  u32 index_ = 624;
};

Result<Input> uniform18(u64 count, u32 seed, MemoryBudget& budget) {
  Input input;
  Buffer<u64> keys;
  MHGP11_TRY(input.x.allocate(count, budget));
  MHGP11_TRY(input.y.allocate(count, budget));
  MHGP11_TRY(input.z.allocate(count, budget));
  MHGP11_TRY(input.ids.allocate(count, budget));
  MHGP11_TRY(keys.allocate(count, budget));
  PythonRandom random(seed);
  for (u64 i = 0; i < count; ++i) {
    input.x[i] = random.bits(18);
    input.y[i] = random.bits(18);
    input.z[i] = random.bits(18);
    input.ids[i] = make_id<PointId>(static_cast<u32>(i));
    keys[i] = (u64{input.x[i]} << 36) | (u64{input.y[i]} << 18) | input.z[i];
  }
  std::sort(keys.begin(), keys.end());
  if (std::adjacent_find(keys.begin(), keys.end()) != keys.end()) return fail(Reason::parameter_out_of_range);
  return input;
}

u64 fnv1a(const Input& input) {
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

std::string decimal(i128 value) {
  const bool negative = value < 0;
  u128 magnitude = negative ? u128{0} - static_cast<u128>(value) : static_cast<u128>(value);
  std::string reversed;
  do {
    reversed.push_back(static_cast<char>('0' + static_cast<int>(magnitude % 10)));
    magnitude /= 10;
  } while (magnitude != 0);
  if (negative) reversed.push_back('-');
  return std::string(reversed.rbegin(), reversed.rend());
}

std::string hex64(u64 value) {
  std::string out(16, '0');
  for (int i = 15; i >= 0; --i, value >>= 4) out[i] = "0123456789abcdef"[value & 15];
  return out;
}

void outcome_json(const Outcome& outcome) {
  std::cout << "\"status\":\"" << status_name(outcome.status()) << "\",\"reason\":\"" << reason_name(outcome.reason)
            << '"';
}

Result<Catalogue> build(const Cloud& cloud, CatalogueParams params, int kmax, MemoryBudget& budget,
                        sched::Pool* pool) {
  params.kmax = kmax;
  return pool == nullptr ? build_catalogue(cloud, params, budget) : build_catalogue(cloud, params, budget, *pool);
}

// Boule de support canonique `sites` (SiteIdx croissants), ou kNoBall.
u64 find_ball(const Catalogue& cat, const std::array<SiteIdx, 4>& sites) {
  for (u64 b = 0; b < cat.balls(); ++b)
    if (cat.balls_data()[b].support == sites) return b;
  return euler::kNoBall;
}

struct Run {
  Options options;
  u64 n = 0, fnv = 0, balls_k = 0, balls_k2 = 0;
  bool disagreement = false, floor = false;
  std::string refusal;  // refus du juge ou du harnais (code 2), vide sinon
  Outcome product{};
};

// Omissions du harnais : PointId -> SiteIdx, support canonique, boule de chaque catalogue vise. La boule doit exister
// dans Cat_{K+2} (portee 0 ou 2) ou dans Cat_K (portee 1) ; sinon refus.
bool omissions(const Cloud& cloud, const Catalogue& small, const Catalogue& large, const Options& o,
               Buffer<u8>& omit_small, Buffer<u8>& omit_large, MemoryBudget& budget) {
  if (o.omit.empty()) return true;
  if (!omit_small.allocate(small.balls(), budget).ok() || !omit_large.allocate(large.balls(), budget).ok())
    return false;
  std::fill(omit_small.begin(), omit_small.end(), u8{0});
  std::fill(omit_large.begin(), omit_large.end(), u8{0});
  std::cout << "{\"phase\":\"omissions\",\"balls\":[";
  bool first = true, ok = true;
  for (const auto& [scope, ids] : o.omit) {
    std::vector<u32> sites;
    for (u32 id : ids)
      for (u32 s = 0; s < cloud.sites(); ++s)
        for (PointId p : cloud.points(make_id<SiteIdx>(s)))
          if (idx(p) == id) sites.push_back(s);
    std::sort(sites.begin(), sites.end());
    std::array<SiteIdx, 4> support{};
    support.fill(make_id<SiteIdx>(kNone));
    for (u64 j = 0; j < sites.size() && j < 4; ++j) support[j] = make_id<SiteIdx>(sites[j]);
    const bool known = sites.size() == ids.size();
    const u64 in_large = known ? find_ball(large, support) : euler::kNoBall;
    const u64 in_small = known ? find_ball(small, support) : euler::kNoBall;
    if (in_large != euler::kNoBall && scope != 1) omit_large[in_large] = 1;
    if (in_small != euler::kNoBall && scope != 2) omit_small[in_small] = 1;
    ok = ok && (scope == 1 ? in_small != euler::kNoBall : in_large != euler::kNoBall);
    std::cout << (first ? "" : ",") << "{\"scope\":" << scope << ",\"ids\":[";
    for (u64 j = 0; j < ids.size(); ++j) std::cout << (j ? "," : "") << ids[j];
    std::cout << "],\"in_k\":" << (in_small != euler::kNoBall ? "true" : "false")
              << ",\"in_k2\":" << (in_large != euler::kNoBall ? "true" : "false");
    if (in_large != euler::kNoBall) {
      const auto& ball = large.balls_data()[in_large];
      std::cout << ",\"p\":" << ball.p << ",\"qmin\":" << unsigned{ball.qmin} << ",\"m\":" << ball.m;
    }
    std::cout << '}';
    first = false;
  }
  std::cout << "]}\n";
  return ok;
}

void euler_json(const euler::Totals& t, int orders, u64 checkable, u64 n, u64 ns, std::vector<u64>& failing) {
  std::cout << "{\"phase\":\"euler\",\"orders\":" << orders << ",\"checkable\":" << checkable;
  if (t.refused_shell != 0) {  // refus du juge avant tout calcul : aucune somme publiee
    std::cout << ",\"refused_shell\":" << t.refused_shell << ",\"shell_bound\":" << euler::kMaxShell << "}\n";
    return;
  }
  const char* names[3] = {"euler_by_k", "regular_by_k", "extended_by_k"};
  for (int series = 0; series < 3; ++series) {
    std::cout << ",\"" << names[series] << "\":[";
    for (int k = 1; k <= orders; ++k) {
      const i128 value = series == 1 ? t.regular[k] : series == 2 ? t.extended[k]
                                     : t.regular[k] + t.extended[k] + (k == 1 ? i128(n) : i128(0));
      std::cout << (k > 1 ? "," : "") << decimal(value);
      if (series == 0 && u64(k) <= checkable && value != 1) failing.push_back(u64(k));
    }
    std::cout << ']';
  }
  std::cout << ",\"failing_orders\":[";
  for (u64 i = 0; i < failing.size(); ++i) std::cout << (i ? "," : "") << failing[i];
  std::cout << "],\"balls\":" << t.balls << ",\"omitted\":" << t.omitted << ",\"regular\":" << t.regular_balls
            << ",\"extended\":" << t.extended_balls << ",\"max_shell\":" << t.max_shell << ",\"extended_by_shell\":{";
  bool first = true;
  for (u32 m = 0; m <= euler::kMaxShell; ++m)
    if (t.extended_by_shell[m] != 0) {
      std::cout << (first ? "" : ",") << '"' << m << "\":" << t.extended_by_shell[m];
      first = false;
    }
  std::cout << "},\"by_qmin\":[" << t.by_qmin[2] << ',' << t.by_qmin[3] << ',' << t.by_qmin[4]
            << "],\"supports\":" << t.supports << ",\"census_sites\":" << t.census_sites << ",\"faults\":" << t.faults
            << ",\"first_fault\":{\"kind\":\"" << euler::fault_name(t.first_fault) << "\",\"ball\":"
            << (t.first_fault_ball == euler::kNoBall ? std::string("null") : std::to_string(t.first_fault_ball))
            << "},\"wall_ns\":" << ns << "}\n";
}

void restriction_json(const euler::Restriction& r, u64 k, u64 ns) {
  auto index = [](u64 b) { return b == euler::kNoBall ? std::string("null") : std::to_string(b); };
  std::cout << "{\"phase\":\"restriction\",\"k\":" << k << ",\"filtered\":" << r.filtered
            << ",\"compared\":" << r.compared << ",\"missing\":" << r.missing << ",\"extra\":" << r.extra
            << ",\"fields\":" << r.fields << ",\"raw_checks\":" << r.raw_checks
            << ",\"raw_recomputed\":" << r.raw_recomputed << ",\"first\":{\"kind\":\""
            << euler::mismatch_name(r.first) << "\",\"large\":" << index(r.first_large)
            << ",\"small\":" << index(r.first_small) << "},\"wall_ns\":" << ns << "}\n";
}

Outcome execute(Run& run) {
  const Options& o = run.options;
  MemoryBudget budget(o.budget);
  Stopwatch read_clock;
  Result<Input> input = fail(Reason::input_unreadable);
  if (o.uniform != 0) {
    input = uniform18(o.uniform, static_cast<u32>(o.seed), budget);
  } else if (!o.data.empty()) {
    const char* folder = std::getenv("MHGP11_DATA_DIR");
    if (folder != nullptr && *folder != '\0') {
      const std::string base = std::string(folder) + "/" + o.data;
      input = read_input((base + ".u32le").c_str(), (base + ".ids.u32le").c_str(), budget);
    }
  } else {
    input = read_input(o.xyz.c_str(), o.ids.c_str(), budget);
  }
  if (!input.ok()) return input.outcome();
  run.fnv = fnv1a(input.value());
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                             input.value().ids.span(), CoordWidth(), budget);
  if (!cloud.ok()) return cloud.outcome();
  input.value() = {};
  run.n = cloud.value().sites();
  std::cout << "{\"phase\":\"cloud\",\"points\":" << cloud.value().weight() << ",\"sites\":" << run.n
            << ",\"input_fnv1a64\":\"" << hex64(run.fnv) << "\",\"read_ns\":" << read_clock.nanoseconds() << "}\n";
  std::unique_ptr<sched::Pool> pool;
  if (o.workers != 0) {
    auto made = sched::make_pool({static_cast<u32>(o.workers)});
    if (!made.ok()) return made.outcome();
    pool = std::move(made.value());
  }
  std::vector<Result<Catalogue>> cats;
  cats.reserve(2);
  for (int kmax : {static_cast<int>(o.k), static_cast<int>(o.k) + 2}) {
    budget.restart_peak();
    Stopwatch clock;
    cats.push_back(build(cloud.value(), o.params, kmax, budget, pool.get()));
    const u64 ns = clock.nanoseconds();
    std::cout << "{\"phase\":\"catalogue\",\"kmax\":" << kmax << ",\"workers\":" << o.workers << ',';
    outcome_json(cats.back().outcome());
    if (cats.back().ok())
      std::cout << ",\"balls\":" << cats.back().value().balls() << ",\"levels\":" << cats.back().value().levels().size()
                << ",\"incidences\":" << cats.back().value().population().size();
    std::cout << ",\"wall_ns\":" << ns << ",\"peak_reserved_bytes\":" << budget.peak() << "}\n";
    if (!cats.back().ok()) return cats.back().outcome();
  }
  const Catalogue& small = cats[0].value();
  const Catalogue& large = cats[1].value();
  run.balls_k = small.balls();
  run.balls_k2 = large.balls();
  Buffer<u8> omit_small, omit_large;
  if (!omissions(cloud.value(), small, large, o, omit_small, omit_large, budget)) {
    run.refusal = "omission_introuvable";
    return {};
  }
  Stopwatch euler_clock;
  euler::Totals totals;
  MHGP11_TRY(euler::euler_totals(cloud.value(), large, omit_large.span(), pool.get(), budget, totals));
  const u64 checkable = std::min<u64>(o.k, run.n);
  std::vector<u64> failing;
  euler_json(totals, large.kmax(), checkable, run.n, euler_clock.nanoseconds(), failing);
  if (totals.refused_shell != 0) {
    run.refusal = "coquille_etendue m=" + std::to_string(totals.refused_shell) + " borne=" +
                  std::to_string(euler::kMaxShell);
    return {};
  }
  Stopwatch restriction_clock;
  euler::Restriction restricted;
  MHGP11_TRY(euler::restriction(cloud.value(), small, large, omit_small.span(), omit_large.span(), restricted));
  restriction_json(restricted, o.k, restriction_clock.nanoseconds());
  run.disagreement = !failing.empty() || totals.faults != 0 || restricted.mismatches() != 0;
  run.floor = totals.balls < o.min_balls || checkable < o.min_orders || totals.extended_balls < o.min_extended ||
              restricted.compared < o.min_compared;
  std::cout << "catalogue_euler_verdict " << (run.disagreement ? "ecart" : run.floor ? "plancher" : "conforme")
            << " k=" << o.k << " n=" << run.n << " ordres=" << checkable << " boules_k=" << run.balls_k
            << " boules_k2=" << run.balls_k2 << " etendues=" << totals.extended_balls
            << " coquille_max=" << totals.max_shell << " entree=" << hex64(run.fnv) << '\n';
  return {};
}

}  // namespace

int main(int argc, char** argv) {
  Run run;
  if (!parse_options(argc, argv, run.options)) {
    std::cout << "{\"phase\":\"exit\",\"code\":2,\"usage\":\"options invalides\"}\n";
    std::cout << "catalogue_euler_verdict refus usage\n";
    return 2;
  }
  run.product = guarded([&]() { return execute(run); });
  int code = 0;
  if (!run.product.ok()) code = exit_code(run.product);
  else if (!run.refusal.empty()) code = 2;
  else if (run.disagreement) code = 1;
  else if (run.floor) code = 3;
  std::cout << "{\"phase\":\"exit\",";
  outcome_json(run.product);
  std::cout << ",\"code\":" << code << "}\n";
  if (!run.product.ok())
    std::cout << "catalogue_euler_verdict refus " << reason_name(run.product.reason) << '\n';
  else if (!run.refusal.empty())
    std::cout << "catalogue_euler_verdict refus " << run.refusal << '\n';
  return code;
}
