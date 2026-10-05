// Sonde de la hierarchie des supports (tranche S6b), hors produit. Deux modes.
//
// Requetes (sans entree en option) : sur l'entree standard, des requetes "K budget n" puis n lignes "x y z PointId"
// (n <= 14). Pour chacune : Cat_K (kmax = K), arbre d'ordre K (build_order, voie serielle ou par lots selon
// --workers), build_support_hierarchy sur le meme Pool, juge des invariants (hierarchy_support.hpp), puis une ligne
// JSON (cles triees, sans espace) pour le differentiel mhgp11_supports_hierarchy_fraction :
//   format "hgp11_hierarchy_probe", version 1, k, n, check ("" si le juge est conforme, sinon son ecart), sites
//   (coordonnees en ordre lexicographique), ids (PointId dans le meme ordre) ;
//   nodes[v] (numerotation canonique) : level ([numerateur, denominateur] en hexadecimal), parent (null a la racine),
//   children, kind (0 feuille-site a K = 1, 1 naissance, 2 fusion), post, size, balls (positions des boules propres,
//   ordre natif), birth_key (BallIdx de la naissance, null sinon), birth_site (coordonnees du site a K = 1) ;
//   balls, ORDRE NATIF de la hierarchie : key, rank, level, node, role, p, m, qmin, components, prior, supports
//   (coordonnees, ordre natif), sites (les memes en SiteIdx), kparties_reliees, compressed_parts, strict_traces,
//   cofaces, gabriel_cofaces, cofaces_support, gabriel_cofaces_support.
//   Un refus du produit donne {"k", "reason", "status"}.
//
// Echelle : mhgp11_supports_hierarchy_probe <entree> --k=<K> [--workers=<W>,...] [--tree-workers=<T>]
//           [--permute=<graine>] [--budget=<octets>] [--min-balls=N] [--min-extended=N] [--min-multiple=N]
//   entree  --input=<xyz.u32le>,<ids.u32le> | --data=<nom> (dossier MHGP11_DATA_DIR) | --uniform18=<n>,<graine>
//           (random.Random(graine).getrandbits(18) de CPython, comme attach_judge.cpp et catalogue_euler.cpp) |
//           --grid=<n>,<cote>,<pas>,<graine> : n cases distinctes de la grille cote^3 de pas donne, tirees par
//           Fisher-Yates partiel (splitmix64) ; cospherique, riche en coquilles etendues (propre a cette sonde)
//   L'entree est permutee a graine fixe (--permute, splitmix64 et Fisher-Yates) avant le nuage. Catalogue et arbre
//   d'ordre K une fois, par la voie par lots (mode 16379 sans ordres concurrents ni verticales) sur T fils (defaut 4) ;
//   puis une hierarchie par valeur de --workers (W = 1 : sans Pool ; W > 1 : Pool de W fils), jugee (I5, I6, I11,
//   rattachement), et son empreinte comparee entre les W (determinisme). Une ligne JSON de mesure par W.
// Codes : 0 conforme ; 1 ecart (juge, empreintes differentes) ; 2 refus (usage, entree, refus du produit) ; 3
// plancher, ou invariant du produit. Derniere ligne, sans duree :
//   hierarchy_probe_verdict conforme k=<K> n=<n> boules=<B> noeuds=<N> naissances=<b> fusions=<f> internes=<i>
//     supports=<S> etendues=<e> multiples=<m> tetraedres=<t> branches=<A> fermetures=<c> kparties=<somme>
//     cofaces=<somme> incidences=<somme> coquille_max=<m> empreinte=<fnv1a64> entree=<fnv1a64 avant permutation>
#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../../bench/whole_input.hpp"
#include "hierarchy_support.hpp"
#include "sched/sched.hpp"

using namespace mhgp11;
using namespace mhgp11::bench;
using namespace hierarchy_test;

namespace {

struct Options {
  std::string xyz, ids, data;
  u64 uniform = 0, seed = 0, grid = 0, side = 0, step = 0, k = 0, tree_workers = 4, permute = 0, budget = u64{32} << 30;
  u64 min_balls = 0, min_extended = 0, min_multiple = 0;
  std::vector<u32> workers{1};
  bool scale = false;
};

bool split_pair(std::string_view text, u64& a, u64& b) {
  const auto comma = text.find(',');
  return comma != std::string_view::npos && parse(text.substr(0, comma), a) && parse(text.substr(comma + 1), b);
}

bool parse_workers(std::string_view text, std::vector<u32>& out) {
  out.clear();
  while (!text.empty()) {
    const auto comma = text.find(',');
    u64 w = 0;
    if (!parse(text.substr(0, comma), w) || w == 0 || w > sched::kMaxWorkers) return false;
    if (std::find(out.begin(), out.end(), static_cast<u32>(w)) != out.end()) return false;
    out.push_back(static_cast<u32>(w));
    text = comma == std::string_view::npos ? std::string_view() : text.substr(comma + 1);
  }
  return !out.empty();
}

bool parse_options(int argc, char** argv, Options& o) {
  int inputs = 0;
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
    } else if (value("--uniform18=")) {
      if (!split_pair(v, o.uniform, o.seed) || o.uniform == 0 || o.uniform > 4000000 || o.seed > 0xFFFFFFFFull)
        return false;
      ++inputs;
    } else if (value("--grid=")) {
      std::array<u64, 4> g{};
      for (u64& part : g) {
        const auto comma = v.find(',');
        if (!parse(v.substr(0, comma), part)) return false;
        v = comma == std::string_view::npos ? std::string_view() : v.substr(comma + 1);
      }
      if (!v.empty() || g[0] == 0 || g[1] == 0 || g[1] > 128 || g[0] > g[1] * g[1] * g[1] || g[2] == 0 ||
          g[2] * g[1] > 1000000)
        return false;
      o.grid = g[0];
      o.side = g[1];
      o.step = g[2];
      o.seed = g[3];
      ++inputs;
    } else if (value("--k=")) {
      if (!parse(v, o.k)) return false;
    } else if (value("--workers=")) {
      if (!parse_workers(v, o.workers)) return false;
    } else if (value("--tree-workers=")) {
      if (!parse(v, o.tree_workers) || o.tree_workers == 0 || o.tree_workers > sched::kMaxWorkers) return false;
    } else if (value("--permute=")) {
      if (!parse(v, o.permute) || o.permute == 0) return false;
    } else if (value("--budget=")) {
      if (!parse(v, o.budget)) return false;
    } else if (value("--min-balls=")) {
      if (!parse(v, o.min_balls)) return false;
    } else if (value("--min-extended=")) {
      if (!parse(v, o.min_extended)) return false;
    } else if (value("--min-multiple=")) {
      if (!parse(v, o.min_multiple)) return false;
    } else {
      return false;
    }
  }
  o.scale = inputs == 1;
  if (!o.scale) return inputs == 0 && o.k == 0 && o.permute == 0 && o.workers.size() == 1;
  return o.k >= 1 && o.k <= supports::kMaxOrder;
}

// ------------------------------------------------------------------ document JSON (mode requetes)

template <class T>
std::string hex(const T& value) {
  const auto wide = num::to_wide(value);
  std::ostringstream out;
  if (wide.sign() < 0) out << '-';
  out << std::hex << std::setfill('0');
  for (std::size_t i = wide.words.size(); i != 0; --i) out << std::setw(16) << wide.words[i - 1];
  return out.str();
}
std::string level_hex(const num::Level& level) {
  return "[\"" + hex(level.numerator()) + "\",\"" + hex(level.denominator()) + "\"]";
}
std::string xyz(const Cloud& cloud, SiteIdx s) {
  return "[" + std::to_string(cloud.x()[idx(s)]) + "," + std::to_string(cloud.y()[idx(s)]) + "," +
         std::to_string(cloud.z()[idx(s)]) + "]";
}
template <class Range>
std::string list(const Range& values) {
  std::string out = "[";
  for (const auto& v : values) {
    if (out.size() > 1) out += ',';
    out += std::to_string(v);
  }
  return out + "]";
}
const char* role_name(BallRole role) {
  return role == BallRole::birth ? "naissance" : role == BallRole::merge ? "fusion" : "interne";
}

Outcome ball_json(std::ostream& out, const OrderTree& tree, const SupportHierarchy& h, u64 slot) {
  const auto& cloud = tree.domain().index().cloud();
  const auto& cat = tree.domain().catalogue();
  const Ball& b = h.balls()[slot];
  const auto shape = supports::make_shape(b.p, b.m, b.qmin, tree.order());
  if (!shape.ok()) return shape.outcome();
  std::vector<u32> prior, cofaces, gabriel;
  for (u64 j = h.prior_offsets()[slot]; j < h.prior_offsets()[slot + 1]; ++j) prior.push_back(idx(h.prior()[j]));
  std::string coords = "[", sites = "[";
  for (u64 s = h.support_offsets()[slot]; s < h.support_offsets()[slot + 1]; ++s) {
    const Support& q = h.supports()[s];
    coords += coords.size() > 1 ? ",[" : "[";
    sites += sites.size() > 1 ? ",[" : "[";
    for (u32 j = 0; j < q.arity; ++j) {
      coords += (j ? "," : "") + xyz(cloud, q.sites[j]);
      sites += (j ? "," : "") + std::to_string(idx(q.sites[j]));
    }
    coords += "]";
    sites += "]";
    cofaces.push_back(h.support_cofaces()[s]);
    gabriel.push_back(supports::support_gabriel_cofaces(shape.value(), q.arity));
  }
  out << "{\"cofaces\":" << b.cofaces << ",\"cofaces_support\":" << list(cofaces) << ",\"components\":" << b.components
      << ",\"compressed_parts\":" << b.compressed_parts << ",\"gabriel_cofaces\":" << b.gabriel_cofaces
      << ",\"gabriel_cofaces_support\":" << list(gabriel) << ",\"key\":" << idx(b.key)
      << ",\"kparties_reliees\":" << b.kparties_reliees << ",\"level\":" << level_hex(cat.levels()[idx(b.rank)])
      << ",\"m\":" << unsigned{b.m} << ",\"node\":" << idx(b.node) << ",\"p\":" << unsigned{b.p}
      << ",\"prior\":" << list(prior) << ",\"qmin\":" << unsigned{b.qmin} << ",\"rank\":" << idx(b.rank)
      << ",\"role\":\"" << role_name(b.role) << "\",\"sites\":" << sites << "],\"strict_traces\":" << b.strict_traces
      << ",\"supports\":" << coords << "]}";
  return {};
}

Outcome document(std::ostream& out, const OrderTree& tree, const SupportHierarchy& h, const std::string& check) {
  const auto& cloud = tree.domain().index().cloud();
  const auto& cat = tree.domain().catalogue();
  const auto& forest = tree.forest();
  const u32 k = tree.order(), n = cloud.sites();
  std::vector<u32> lex(n);
  for (u32 s = 0; s < n; ++s) lex[s] = s;
  std::sort(lex.begin(), lex.end(), [&](u32 l, u32 r) {
    return std::array<u32, 3>{cloud.x()[l], cloud.y()[l], cloud.z()[l]} <
           std::array<u32, 3>{cloud.x()[r], cloud.y()[r], cloud.z()[r]};
  });
  out << "{\"balls\":[";
  for (u64 slot = 0; slot < h.balls().size(); ++slot) {
    out << (slot ? "," : "");
    MHGP11_TRY(ball_json(out, tree, h, slot));
  }
  out << "],\"check\":\"" << check << "\",\"format\":\"hgp11_hierarchy_probe\",\"ids\":[";
  for (u32 j = 0; j < n; ++j) out << (j ? "," : "") << idx(cloud.ids()[cloud.offsets()[lex[j]]]);
  out << "],\"k\":" << k << ",\"n\":" << n << ",\"nodes\":[";
  for (u32 v = 0; v < forest.nodes().size(); ++v) {
    const auto& node = forest.nodes()[v];
    std::vector<u32> children, own;
    for (NodeIdx c : forest.children(NodeIdx{v})) children.push_back(idx(c));
    const u32 j = h.post()[v];
    for (u64 slot = h.ball_offsets()[j]; slot < h.ball_offsets()[j + 1]; ++slot) own.push_back(static_cast<u32>(slot));
    const bool birth = v < forest.births();
    out << (v ? "," : "") << "{\"balls\":" << list(own) << ",\"birth_key\":"
        << (birth && k >= 2 ? std::to_string(node.birth_key) : "null") << ",\"birth_site\":"
        << (birth && k == 1 ? xyz(cloud, SiteIdx{node.birth_key}) : "null") << ",\"children\":" << list(children)
        << ",\"kind\":" << (!birth ? 2 : k == 1 ? 0 : 1) << ",\"level\":" << level_hex(cat.levels()[idx(node.rank)])
        << ",\"parent\":" << (node.parent == NodeIdx{kNone} ? std::string("null") : std::to_string(idx(node.parent)))
        << ",\"post\":" << j << ",\"size\":" << h.subtree_size()[v] << '}';
  }
  out << "],\"sites\":[";
  for (u32 j = 0; j < n; ++j) out << (j ? "," : "") << xyz(cloud, SiteIdx{lex[j]});
  out << "],\"version\":1}\n";
  return {};
}

FullParams params_for(u32 workers) {
  FullParams p;
  if (workers > 1) {
    p.regular_batch_capacity = 4096;
    p.descent_lanes = 48;
    p.reuse_census_workspace = p.dense_birth_lookup = p.population_lookup = true;
  }
  return p;
}

template <class T>
bool number(std::string_view word, T& value) {
  const auto result = std::from_chars(word.data(), word.data() + word.size(), value);
  return !word.empty() && result.ec == std::errc{} && result.ptr == word.data() + word.size();
}
template <class T>
bool next(T& value) {
  std::string word;
  return bool(std::cin >> word) && number(word, value);
}

Outcome request(u32 k, u64 bytes, const std::vector<u32>& x, const std::vector<u32>& y, const std::vector<u32>& z,
                const std::vector<PointId>& ids, u32 workers) {
  MemoryBudget budget(bytes);
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth{}, budget);
  if (!cloud.ok()) return cloud.outcome();
  auto index = build_index(std::move(cloud.value()), IndexParams{2}, budget);
  if (!index.ok()) return index.outcome();
  CatalogueParams params;
  params.kmax = static_cast<int>(k);
  auto domain = prepare_full_domain(std::move(index.value()), params, budget);
  if (!domain.ok()) return domain.outcome();
  std::unique_ptr<sched::Pool> pool;
  if (workers > 1) {
    auto made = sched::make_pool({workers});
    if (!made.ok()) return made.outcome();
    pool = std::move(made.value());
  }
  auto tree = build_order(std::move(domain.value()), static_cast<Order>(k), budget, params_for(workers), pool.get());
  if (!tree.ok()) return tree.outcome();
  auto h = supports::build_support_hierarchy(tree.value(), budget, pool.get());
  if (!h.ok()) return h.outcome();
  Totals totals;
  const std::string check = hierarchy_test::check(tree.value(), h.value(), totals);
  return document(std::cout, tree.value(), h.value(), check);
}

int requests(u32 workers) {
  std::string first;
  int code = 0;
  while (std::cin >> first) {
    u32 k = 0, n = 0;
    u64 bytes = 0;
    if (!number(first, k) || !next(bytes) || !next(n) || n == 0 || n > 14) return 2;
    std::vector<u32> x(n), y(n), z(n);
    std::vector<PointId> ids(n);
    for (u32 i = 0; i < n; ++i) {
      u32 id = 0;
      if (!next(x[i]) || !next(y[i]) || !next(z[i]) || !next(id)) return 2;
      ids[i] = PointId{id};
    }
    const Outcome outcome = guarded([&]() { return request(k, bytes, x, y, z, ids, workers); });
    if (!outcome.ok()) {
      std::cout << "{\"k\":" << k << ",\"reason\":\"" << reason_name(outcome.reason) << "\",\"status\":\""
                << status_name(outcome.status()) << "\"}\n";
      code = std::max(code, exit_code(outcome));
    }
  }
  return code;
}

}  // namespace

int scale_main(const Options& o);

int main(int argc, char** argv) {
  Options o;
  if (!parse_options(argc, argv, o)) {
    std::cout << "hierarchy_probe_verdict refus usage\n";
    return 2;
  }
  if (!o.scale) return requests(o.workers[0]);
  return scale_main(o);
}

// ------------------------------------------------------------------ mode echelle

namespace {

// random.Random(graine) de CPython (MT19937, init_by_array d'un mot), getrandbits(k <= 32) : port de
// bench/catalogue_euler.cpp (PythonRandom) comme tests/tower/attach_judge.cpp ; memes nuages que leurs portes.
class PythonRandom {
 public:
  explicit PythonRandom(u32 seed) noexcept {
    mt_[0] = 19650218u;
    for (u32 i = 1; i < 624; ++i) mt_[i] = 1812433253u * (mt_[i - 1] ^ (mt_[i - 1] >> 30)) + i;
    u32 i = 1;
    for (u32 k = 624; k != 0; --k) {
      mt_[i] = (mt_[i] ^ ((mt_[i - 1] ^ (mt_[i - 1] >> 30)) * 1664525u)) + seed;
      if (++i >= 624) {
        mt_[0] = mt_[623];
        i = 1;
      }
    }
    for (u32 k = 623; k != 0; --k) {
      mt_[i] = (mt_[i] ^ ((mt_[i - 1] ^ (mt_[i - 1] >> 30)) * 1566083941u)) - i;
      if (++i >= 624) {
        mt_[0] = mt_[623];
        i = 1;
      }
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

u64 splitmix(u64& state) {
  u64 z = (state += 0x9e3779b97f4a7c15ull);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
  return z ^ (z >> 31);
}

Result<Input> grid(const Options& o, MemoryBudget& budget) {
  const u64 cells = o.side * o.side * o.side;
  Buffer<u32> pool;
  MHGP11_TRY(pool.allocate(cells, budget));
  for (u64 c = 0; c < cells; ++c) pool[c] = static_cast<u32>(c);
  u64 state = o.seed;
  Input input;
  MHGP11_TRY(input.x.allocate(o.grid, budget));
  MHGP11_TRY(input.y.allocate(o.grid, budget));
  MHGP11_TRY(input.z.allocate(o.grid, budget));
  MHGP11_TRY(input.ids.allocate(o.grid, budget));
  for (u64 i = 0; i < o.grid; ++i) {
    std::swap(pool[i], pool[i + splitmix(state) % (cells - i)]);
    const u64 c = pool[i];
    input.x[i] = static_cast<u32>(c / (o.side * o.side) * o.step);
    input.y[i] = static_cast<u32>(c / o.side % o.side * o.step);
    input.z[i] = static_cast<u32>(c % o.side * o.step);
    input.ids[i] = make_id<PointId>(static_cast<u32>(i));
  }
  return input;
}

Result<Input> read(const Options& o, MemoryBudget& budget) {
  if (o.grid != 0) return grid(o, budget);
  if (o.uniform != 0) {
    Input input;
    MHGP11_TRY(input.x.allocate(o.uniform, budget));
    MHGP11_TRY(input.y.allocate(o.uniform, budget));
    MHGP11_TRY(input.z.allocate(o.uniform, budget));
    MHGP11_TRY(input.ids.allocate(o.uniform, budget));
    PythonRandom random(static_cast<u32>(o.seed));
    for (u64 i = 0; i < o.uniform; ++i) {
      input.x[i] = random.bits(18);
      input.y[i] = random.bits(18);
      input.z[i] = random.bits(18);
      input.ids[i] = make_id<PointId>(static_cast<u32>(i));
    }
    return input;
  }
  if (o.data.empty()) return read_input(o.xyz.c_str(), o.ids.c_str(), budget);
  const char* folder = std::getenv("MHGP11_DATA_DIR");
  if (folder == nullptr || *folder == '\0') return fail(Reason::input_unreadable);
  const std::string base = std::string(folder) + "/" + o.data;
  return read_input((base + ".u32le").c_str(), (base + ".ids.u32le").c_str(), budget);
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

// Fisher-Yates a graine fixe (splitmix64) : meme multiensemble de (x, y, z, PointId), autre ordre d'entree.
void permute(Input& input, u64 seed) {
  u64 state = seed;
  for (u64 i = input.x.size(); i > 1; --i) {
    const u64 j = splitmix(state) % i;
    std::swap(input.x[i - 1], input.x[j]);
    std::swap(input.y[i - 1], input.y[j]);
    std::swap(input.z[i - 1], input.z[j]);
    std::swap(input.ids[i - 1], input.ids[j]);
  }
}

std::string hex64(u64 value) {
  std::string out(16, '0');
  for (int i = 15; i >= 0; --i, value >>= 4) out[static_cast<std::size_t>(i)] = "0123456789abcdef"[value & 15];
  return out;
}

struct Run {
  Totals totals;
  u64 n = 0, input = 0, print = 0;
  std::string error;
};

Outcome hierarchies(const Options& o, const OrderTree& tree, MemoryBudget& budget, Run& run) {
  bool first = true;
  for (u32 w : o.workers) {
    std::unique_ptr<sched::Pool> pool;
    if (w > 1) {
      auto made = sched::make_pool({w});
      if (!made.ok()) return made.outcome();
      pool = std::move(made.value());
    }
    supports::HierarchyTimings timings;
    const u64 before = budget.used();
    budget.restart_peak();
    Stopwatch clock;
    auto h = supports::build_support_hierarchy(tree, budget, pool.get(), &timings);
    const u64 wall = clock.nanoseconds();
    if (!h.ok()) return h.outcome();
    const u64 print = fingerprint(h.value());
    std::cout << "{\"phase\":\"hierarchy\",\"workers\":" << w << ",\"wall_ns\":" << wall
              << ",\"tree_ns\":" << timings.tree_ns << ",\"count_ns\":" << timings.count_ns
              << ",\"fill_ns\":" << timings.fill_ns << ",\"admitted\":[" << timings.admitted[0] << ','
              << timings.admitted[1] << "],\"peak_bytes\":" << budget.peak() - before
              << ",\"widest\":" << timings.widest << ",\"empreinte\":\"" << hex64(print) << "\"}\n";
    if (first) {
      Stopwatch judge;
      run.error = check(tree, h.value(), run.totals);
      run.print = print;
      std::cout << "{\"phase\":\"judge\",\"judge_ns\":" << judge.nanoseconds() << ",\"error\":\"" << run.error
                << "\"}\n";
      first = false;
    } else if (print != run.print && run.error.empty()) {
      run.error = "empreinte differente a W" + std::to_string(w);
    }
  }
  return {};
}

Outcome execute(const Options& o, Run& run) {
  MemoryBudget budget(o.budget);
  auto input = read(o, budget);
  if (!input.ok()) return input.outcome();
  run.input = fnv1a(input.value());
  if (o.permute != 0) permute(input.value(), o.permute);
  auto pool = sched::make_pool({static_cast<u32>(o.tree_workers)});
  if (!pool.ok()) return pool.outcome();
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                             input.value().ids.span(), CoordWidth(), budget);
  if (!cloud.ok()) return cloud.outcome();
  if (cloud.value().weight() != cloud.value().sites()) return fail(Reason::multiplicity_unsupported);
  input.value() = {};
  run.n = cloud.value().sites();
  if (o.k > run.n) return fail(Reason::parameter_out_of_range);
  auto index = build_index(std::move(cloud.value()), {}, budget);
  if (!index.ok()) return index.outcome();
  CatalogueParams params;
  params.kmax = static_cast<int>(o.k);
  params.leaf_size = 16;
  params.max_leaf = 256;
  params.cache_center_lines = params.indirect_sort = params.adaptive_frontier = true;
  params.parallel_assembly = params.single_pass = params.pair_graph = true;
  Stopwatch clock;
  auto domain = prepare_full_domain(std::move(index.value()), params, budget, *pool.value());
  if (!domain.ok()) return domain.outcome();
  const u64 domain_ns = clock.nanoseconds();
  FullParams full = params_for(2);
  auto tree = build_order(std::move(domain.value()), static_cast<Order>(o.k), budget, full, pool.value().get());
  if (!tree.ok()) return tree.outcome();
  std::cout << "{\"phase\":\"order\",\"domain_ns\":" << domain_ns << ",\"order_ns\":" << clock.nanoseconds() - domain_ns
            << ",\"tree_workers\":" << o.tree_workers << ",\"nodes\":" << tree.value().forest().nodes().size()
            << ",\"balls\":" << tree.value().attachment().size() << "}\n";
  return hierarchies(o, tree.value(), budget, run);
}

}  // namespace

int scale_main(const Options& o) {
  Run run;
  const Outcome outcome = guarded([&]() { return execute(o, run); });
  if (!outcome.ok()) {
    std::cout << "hierarchy_probe_verdict refus " << reason_name(outcome.reason) << '\n';
    return exit_code(outcome);
  }
  const Totals& t = run.totals;
  const bool floor = t.balls < o.min_balls || t.extended < o.min_extended || t.multiple < o.min_multiple;
  if (!run.error.empty()) std::cout << "hierarchy_probe_ecart " << run.error << '\n';
  std::cout << "hierarchy_probe_verdict " << (!run.error.empty() ? "ecart" : floor ? "plancher" : "conforme")
            << " k=" << o.k << " n=" << run.n << " boules=" << t.balls << " noeuds=" << t.nodes
            << " naissances=" << t.births << " fusions=" << t.merges << " internes=" << t.internals
            << " supports=" << t.supports << " etendues=" << t.extended << " multiples=" << t.multiple
            << " tetraedres=" << t.tetra << " branches=" << t.prior << " fermetures=" << t.closures
            << " kparties=" << t.kparties << " cofaces=" << t.cofaces << " incidences=" << t.incidences
            << " coquille_max=" << t.max_shell << " empreinte=" << hex64(run.print) << " entree=" << hex64(run.input)
            << '\n';
  return !run.error.empty() ? 1 : floor ? 3 : 0;
}
