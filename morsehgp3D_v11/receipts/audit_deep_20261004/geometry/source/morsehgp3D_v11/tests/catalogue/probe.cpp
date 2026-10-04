// Pilote borne de test : requetes texte en lots, un JSON transactionnel par requete, meme sur refus du produit.
// Entree : K leaf maxleaf maxnodes balllimit budget n, puis n lignes x y z PointId. --profile rend le profil.
// --workers W : overload parallele avec un Pool persistant ; sans option, reference sequentielle trois arguments.
// Les vecteurs d'entree et le JSON sont hors compte (harnais) ; cloud et catalogue partagent le budget mesure.
#include <charconv>
#include <chrono>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "catalogue/catalogue.hpp"
#include "sched/sched.hpp"

using namespace mhgp11;

namespace {
struct Request {
  CatalogueParams params;
  u64 budget = 0;
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
};

template <class T>
bool number(std::string_view word, T& value) {
  if (word.empty()) return false;
  const auto parsed = std::from_chars(word.data(), word.data() + word.size(), value);
  return parsed.ec == std::errc{} && parsed.ptr == word.data() + word.size();
}
template <class T>
bool read(T& value) {
  std::string word;
  return bool(std::cin >> word) && number(word, value);
}

bool request(std::string_view first, Request& out) {
  u64 n = 0;
  if (!number(first, out.params.kmax) || !read(out.params.leaf_size) || !read(out.params.max_leaf) ||
      !read(out.params.max_nodes) || !read(out.params.ball_limit) || !read(out.budget) || !read(n)) return false;
  if (n > 1000000) return false;  // domaine du pilote borne, pas du moteur ; le pilote LiDAR est distinct.
  out.x.resize(n);
  out.y.resize(n);
  out.z.resize(n);
  out.ids.resize(n);
  for (u64 i = 0; i < n; ++i) {
    u32 id = 0;
    if (!read(out.x[i]) || !read(out.y[i]) || !read(out.z[i]) || !read(id)) return false;
    out.ids[i] = make_id<PointId>(id);
  }
  return true;
}

template <class T>
std::string decimal(const T& value) {
  auto wide = num::to_wide(value);
  const bool negative = wide.sign() < 0;
  std::string reversed;
  do {
    u128 remainder = 0;
    for (std::size_t i = wide.words.size(); i != 0; --i) {
      const u128 dividend = (remainder << 64) | wide.words[i - 1];
      wide.words[i - 1] = static_cast<u64>(dividend / 10);
      remainder = dividend % 10;
    }
    reversed.push_back(static_cast<char>('0' + static_cast<int>(remainder)));
  } while (!wide.is_zero());
  if (negative) reversed.push_back('-');
  return std::string(reversed.rbegin(), reversed.rend());
}

void level(std::ostream& out, const num::Level& value) {
  out << "[\"" << decimal(value.numerator()) << "\",\"" << decimal(value.denominator()) << "\"]";
}
template <StrongId Id>
void indices(std::ostream& out, std::span<const Id> values) {
  out << '[';
  bool first = true;
  for (Id value : values) {
    if (!first) out << ',';
    first = false;
    out << idx(value);
  }
  out << ']';
}

void cloud_json(std::ostream& out, const Cloud& cloud) {
  out << "\"sites\":[";
  for (u32 i = 0; i < cloud.sites(); ++i) {
    if (i != 0) out << ',';
    out << '[' << cloud.x()[i] << ',' << cloud.y()[i] << ',' << cloud.z()[i] << ']';
  }
  out << "],\"site_ids\":[";
  for (u32 i = 0; i < cloud.sites(); ++i) {
    if (i != 0) out << ',';
    indices(out, cloud.points(make_id<SiteIdx>(i)));
  }
  out << ']';
}

void ledger_json(std::ostream& out, const CatalogueLedger& l) {
  out << "{\"nodes\":" << l.nodes << ",\"leaves\":" << l.leaves
      << ",\"filter_tests\":" << l.filter_tests << ",\"dominance_tests\":" << l.dominance_tests
      << ",\"prefixes\":" << l.prefixes << ",\"judged\":" << l.judged << ",\"census_tests\":" << l.census_tests
      << ",\"emitted\":" << l.emitted
      << ",\"incidences\":" << l.incidences << ",\"max_leaf\":" << l.max_leaf
      << ",\"region_pair_tests\":" << l.region_pair_tests
      << ",\"region_pair_rejects\":" << l.region_pair_rejects
      << ",\"region_line_tests\":" << l.region_line_tests
      << ",\"region_line_rejects\":" << l.region_line_rejects
      << ",\"region_line_evaluations\":" << l.region_line_evaluations
      << ",\"region_line_cache_hits\":" << l.region_line_cache_hits
      << ",\"region_line_fallbacks\":" << l.region_line_fallbacks
      << ",\"q4_candidates\":" << l.q4_candidates << ",\"q4_levels\":" << l.q4_levels
      << ",\"max_depth\":" << l.max_depth << '}';
}

void catalogue_json(std::ostream& out, const Catalogue& cat) {
  out << ",\"levels\":[";
  for (std::size_t i = 0; i < cat.levels().size(); ++i) {
    if (i != 0) out << ',';
    level(out, cat.levels()[i]);
  }
  out << "],\"balls\":[";
  for (u32 i = 0; i < cat.balls(); ++i) {
    if (i != 0) out << ',';
    const auto& ball = cat.balls_data()[i];
    out << "{\"qmin\":" << static_cast<unsigned>(ball.qmin) << ",\"p\":" << ball.p << ",\"m\":" << ball.m
        << ",\"rank\":" << idx(ball.rank) << ",\"support\":";
    indices(out, std::span<const SiteIdx>(ball.support).first(ball.qmin));
    out << ",\"level\":";
    level(out, cat.levels()[idx(ball.rank)]);
    out << ",\"inner\":";
    indices(out, cat.interior(make_id<BallIdx>(i)));
    out << ",\"shell\":";
    indices(out, cat.shell(make_id<BallIdx>(i)));
    out << '}';
  }
  out << "],\"ledger\":";
  ledger_json(out,cat.ledger());
  const auto& e=cat.execution();
  out << ",\"execution\":{\"geometry_passes\":" << e.geometry_passes << ",\"arena_blocks\":" << e.arena_blocks
      << ",\"arena_capacity_bytes\":" << e.arena_capacity_bytes << ",\"arena_metadata_bytes\":" << e.arena_metadata_bytes
      << ",\"compact_records\":" << e.compact_records << ",\"compact_population\":" << e.compact_population << '}';
}

void diagnostics_json(std::ostream& out, const CatalogueDiagnostics& diagnostic) {
  const auto& p = diagnostic.planning();
  out << ",\"planning\":{\"adaptive\":" << (p.adaptive ? "true" : "false")
      << ",\"memory_fallback\":" << (p.memory_fallback ? "true" : "false")
      << ",\"plan_nodes\":" << p.plan_nodes << ",\"plan_leaves\":" << p.plan_leaves
      << ",\"empty_leaves\":" << p.empty_leaves << ",\"rounds\":" << p.rounds
      << ",\"priority_tests\":" << p.priority_tests << ",\"replay_bytes\":" << p.replay_bytes << "},\"tasks\":[";
  bool first = true;
  for (const auto& t : diagnostic.tasks()) {
    if (!first) out << ',';
    first = false;
    out << "{\"path\":[" << t.path[0] << ',' << t.path[1] << "],\"path_known\":"
        << (t.path_known ? "true" : "false") << ",\"inside_known\":" << (t.inside_known ? "true" : "false")
        << ",\"lo\":[" << t.lo[0] << ',' << t.lo[1] << ',' << t.lo[2]
        << "],\"hi\":[" << t.hi[0] << ',' << t.hi[1] << ',' << t.hi[2]
        << "],\"depth\":" << t.depth << ",\"count\":" << t.count << ",\"capacity\":" << t.capacity
        << ",\"inside\":" << t.inside << ",\"count_ns\":" << t.count_ns << ",\"fill_ns\":" << t.fill_ns
        << ",\"ledger\":";
    ledger_json(out,t.ledger); out << '}';
  }
  out << ']';
}

Result<std::string> payload(const Request& req, MemoryBudget& budget, sched::Pool* pool) {
  MHGP11_TRY(check_catalogue_params(req.params));
  auto cloud = prepare_cloud(req.x, req.y, req.z, req.ids, CoordWidth{}, budget);
  if (!cloud.ok()) return cloud.outcome();
  const auto start = std::chrono::steady_clock::now();
  CatalogueDiagnostics diagnostic;
  auto cat = pool == nullptr ? build_catalogue(cloud.value(), req.params, budget)
                            : build_catalogue(cloud.value(), req.params, budget, *pool, nullptr,
                                              req.params.adaptive_frontier || req.params.single_pass ? &diagnostic : nullptr);
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
  if (!cat.ok()) return cat.outcome();
  std::ostringstream out;
  cloud_json(out, cloud.value());
  catalogue_json(out, cat.value());
  if (req.params.adaptive_frontier || req.params.single_pass) diagnostics_json(out,diagnostic);
  out << ",\"catalogue_ns\":" << ns;
  return out.str();
}

void execute(const Request& req, sched::Pool* pool) {
  MemoryBudget budget(req.budget);
  const u64 before = budget.used();
  auto encoded = guarded([&]() { return payload(req, budget, pool); });
  const auto issue = merge(encoded.outcome(), budget.released());
  std::cout << "{\"status\":\"" << status_name(issue.status()) << "\",\"reason\":\"" << reason_name(issue.reason)
            << "\",\"coord_bits\":" << kCoordBits << ",\"kmax\":" << req.params.kmax
            << ",\"workers\":" << (pool == nullptr ? 0 : pool->size())
            << ",\"cache_center_lines\":" << (req.params.cache_center_lines ? "true" : "false")
            << ",\"indirect_sort\":" << (req.params.indirect_sort ? "true" : "false")
            << ",\"adaptive_frontier\":" << (req.params.adaptive_frontier ? "true" : "false")
            << ",\"single_pass\":" << (req.params.single_pass ? "true" : "false")
            << ",\"used_before\":" << before << ",\"used_after\":" << budget.used() << ",\"peak\":" << budget.peak() << ',';
  if (issue.ok()) std::cout << encoded.value();
  else std::cout << "\"sites\":[],\"site_ids\":[],\"levels\":[],\"balls\":[],\"ledger\":{}";
  std::cout << "}\n";
}
}  // namespace

int main(int argc, char** argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--profile") {
    std::cout << "{\"coord_bits\":" << kCoordBits << "}\n";
    return 0;
  }
  u32 workers = 0;
  bool cache = false, sort = false, adaptive = false, single = false, assembly = false;
  for (int i = 1; i < argc; ++i) {
    const std::string_view option(argv[i]);
    if (option == "--workers" && workers == 0 && i + 1 < argc) {
      if (!number(argv[++i], workers) || workers < 1 || workers > sched::kMaxWorkers) return 2;
    } else if (option == "--cache-center-lines" && !cache) cache = true;
    else if (option == "--indirect-sort" && !sort) sort = true;
    else if (option == "--adaptive-frontier" && !adaptive) adaptive = true;
    else if (option == "--single-pass" && !single) single = true;
    else if (option == "--parallel-assembly" && !assembly) assembly = true;
    else return 2;
  }
  if ((adaptive || single) && workers == 0) return 2;
  try {
    std::unique_ptr<sched::Pool> pool;
    if (workers != 0) {
      auto made = sched::make_pool({workers});
      if (!made.ok()) return 2;
      pool = std::move(made.value());
    }
    std::string first;
    while (std::cin >> first) {
      Request req;
      if (!request(first, req)) return 2;
      req.params.cache_center_lines = cache;
      req.params.indirect_sort = sort;
      req.params.adaptive_frontier = adaptive;
      req.params.single_pass = single;
      req.params.parallel_assembly = assembly;
      execute(req, pool.get());
    }
  } catch (const std::bad_alloc&) {
    return 2;  // panne du harnais (entrees, compte ou encodage), pas resultat partiel du produit.
  }
  return std::cin.eof() ? 0 : 2;
}
