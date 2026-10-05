// Juge a l'echelle de l'arbre d'ordre K seul et du rattachement (tranche S3), hors produit :
//   I10  foret de build_order = build_full(...).order(K) (mode 16379 qualifie), noeuds, enfants, rangs, cles de
//        naissance et champs logiques du registre (option --identity) ;
//   I1-I4 recomptes depuis le rattachement publie : W_K par la fenetre, roles et rangs, branches des fusions = leurs
//        enfants, registres de la foret ;
//   E2   lemme E sur toutes les boules (--e2=all) ou un tirage a graine fixe (--e2=<nombre>,<graine>) : descente
//        d'une K-partie de P_b puis ancetre FERME au rang de la boule (port de ball_nodes, bench/points_export.cpp,
//        fenetre au lieu du predicat fort), et seconde K-partie (T3, I7). Le juge garde initial <= lambda_b (une
//        K-partie quelconque peut avoir le niveau de la boule), le journal du produit initial < lambda_b.
//   voie l'ordre K est construit par la voie par lots (defaut : lots de 4 096, 48 voies, census reutilises, lookup
//        dense, table de populations, sur W fils) ou par la voie serielle (--serial : FullParams par defaut, sans Pool).
//        L'empreinte attache= (FNV-1a 64 de WindowAttachment entier) ne depend ni de la voie ni du nombre de fils.
//
//   mhgp11_tower_attach_judge <entree> --k=<K> [--workers=<W>] [--serial] [--identity] [--e2=all|<n>,<graine>]
//                             [--min-balls=N] [--min-merges=N] [--min-internals=N] [--min-e2=N] [--budget=octets]
//   entree  --input=<xyz.u32le>,<ids.u32le> | --data=<nom> (dossier MHGP11_DATA_DIR) | --uniform18=<n>,<graine>
//           (random.Random(graine).getrandbits(18) de CPython, comme bench/catalogue_euler.cpp)
// Codes : 0 conforme ; 1 desaccord (identite, E1 != E2, recompte) ; 2 refus (usage, entree, refus du produit hors
// invariant) ; 3 plancher non atteint ou invariant du produit (tower_invariant). Derniere ligne, sans duree :
//   attach_judge_verdict <conforme|ecart|refus|plancher> k=<K> n=<sites> boules=<|W_K|> naissances=<b> fusions=<f>
//   internes=<i> branches=<A> traces=<S> noeuds=<N> e2=<e> identite=<0|1> attache=<fnv1a64> entree=<fnv1a64>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "whole_input.hpp"
#include "sched/sched.hpp"
#include "tower/order_tree.hpp"

using namespace mhgp11;
using namespace mhgp11::bench;
using namespace mhgp11::tower_detail;

namespace {
// Fenetre de W_K ecrite ici, independamment du produit : p+q-1 <= K <= p+m, evenements faibles compris.
bool window_ball(const CatalogueBall& ball, u64 k) {
  return u64{ball.p} + ball.qmin <= k + 1 && k <= u64{ball.p} + ball.m;
}

struct Options {
  std::string xyz, ids, data;
  u64 uniform = 0, seed = 0, k = 0, workers = 1, budget = u64{32} << 30, sample = 0, sample_seed = 0;
  bool identity = false, all = false, serial = false;
  u64 min_balls = 0, min_merges = 0, min_internals = 0, min_e2 = 0;
};
struct Counts {
  u64 n = 0, balls = 0, births = 0, merges = 0, internals = 0, prior = 0, traces = 0, nodes = 0, e2 = 0;
  u64 fnv = 0, attachment = 0;
  bool identity = false;
};

bool split_pair(std::string_view text, u64& a, u64& b) {
  const auto comma = text.find(',');
  return comma != std::string_view::npos && parse(text.substr(0, comma), a) && parse(text.substr(comma + 1), b);
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
      o.xyz = std::string(v.substr(0, comma)); o.ids = std::string(v.substr(comma + 1)); ++inputs;
    } else if (value("--data=")) {
      if (v.empty() || v.find('/') != std::string_view::npos) return false;
      o.data = std::string(v); ++inputs;
    } else if (value("--uniform18=")) {
      if (!split_pair(v, o.uniform, o.seed) || o.uniform == 0 || o.uniform > 4000000 || o.seed > 0xFFFFFFFFull)
        return false;
      ++inputs;
    } else if (value("--k=")) { if (!parse(v, o.k)) return false; }
    else if (value("--workers=")) {
      if (!parse(v, o.workers) || o.workers == 0 || o.workers > sched::kMaxWorkers) return false;
    }
    else if (value("--budget=")) { if (!parse(v, o.budget)) return false; }
    else if (value("--e2=")) {
      if (v == "all") o.all = true;
      else if (!split_pair(v, o.sample, o.sample_seed) || o.sample == 0 || o.sample_seed > 0xFFFFFFFFull) return false;
    } else if (value("--min-balls=")) { if (!parse(v, o.min_balls)) return false; }
    else if (value("--min-merges=")) { if (!parse(v, o.min_merges)) return false; }
    else if (value("--min-internals=")) { if (!parse(v, o.min_internals)) return false; }
    else if (value("--min-e2=")) { if (!parse(v, o.min_e2)) return false; }
    else if (a == "--identity") o.identity = true;
    else if (a == "--serial") o.serial = true;
    else return false;
  }
  return inputs == 1 && o.k >= 1 && o.k <= kMaxMebSites;
}

// random.Random(graine) de CPython (MT19937, init_by_array d'un mot), getrandbits(k <= 32) : port de
// bench/catalogue_euler.cpp (PythonRandom), memes nuages que les portes catalogue_euler_scale*.
class PythonRandom {
 public:
  explicit PythonRandom(u32 seed) noexcept {
    mt_[0] = 19650218u;
    for (u32 i = 1; i < 624; ++i) mt_[i] = 1812433253u * (mt_[i - 1] ^ (mt_[i - 1] >> 30)) + i;
    u32 i = 1;
    for (u32 k = 624; k != 0; --k) {
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
  MHGP11_TRY(input.x.allocate(count, budget)); MHGP11_TRY(input.y.allocate(count, budget));
  MHGP11_TRY(input.z.allocate(count, budget)); MHGP11_TRY(input.ids.allocate(count, budget));
  MHGP11_TRY(keys.allocate(count, budget));
  PythonRandom random(seed);
  for (u64 i = 0; i < count; ++i) {
    input.x[i] = random.bits(18); input.y[i] = random.bits(18); input.z[i] = random.bits(18);
    input.ids[i] = make_id<PointId>(static_cast<u32>(i));
    keys[i] = (u64{input.x[i]} << 36) | (u64{input.y[i]} << 18) | input.z[i];
  }
  std::sort(keys.begin(), keys.end());
  if (std::adjacent_find(keys.begin(), keys.end()) != keys.end()) return fail(Reason::parameter_out_of_range);
  return input;
}

// FNV-1a 64 de WindowAttachment entier, dans l'ordre des tableaux : boules, noeuds, roles, traces, composantes,
// decalages et branches. Meme valeur attendue quelle que soit la voie de construction et le nombre de fils.
u64 attachment_digest(const WindowAttachment& a) {
  u64 hash = 0xcbf29ce484222325ull;
  auto feed = [&](u64 word, int bytes) {
    for (int b = 0; b < bytes; ++b) hash = (hash ^ ((word >> (8 * b)) & 255)) * 0x100000001b3ull;
  };
  feed(a.size(), 4);
  for (BallIdx b : a.balls()) feed(idx(b), 4);
  for (NodeIdx v : a.node()) feed(idx(v), 4);
  for (BallRole r : a.role()) feed(static_cast<u8>(r), 1);
  for (u32 s : a.strict_traces()) feed(s, 4);
  for (u32 c : a.components()) feed(c, 4);
  for (u64 o : a.prior_offsets()) feed(o, 8);
  for (NodeIdx v : a.prior()) feed(idx(v), 4);
  return hash;
}

std::string hex64(u64 value) {
  std::string out(16, '0');
  for (int i = 15; i >= 0; --i, value >>= 4) out[i] = "0123456789abcdef"[value & 15];
  return out;
}

u64 fnv1a(const Input& input) {
  u64 hash = 0xcbf29ce484222325ull;
  auto feed = [&](u32 word) {
    for (int b = 0; b < 4; ++b) hash = (hash ^ ((word >> (8 * b)) & 255)) * 0x100000001b3ull;
  };
  for (u64 i = 0; i < input.x.size(); ++i) { feed(input.x[i]); feed(input.y[i]); feed(input.z[i]); }
  for (PointId id : input.ids) feed(idx(id));
  return hash;
}

CatalogueParams catalogue_params(u64 k) {
  CatalogueParams params;
  params.kmax = static_cast<int>(k); params.leaf_size = 16; params.max_leaf = 256;
  params.cache_center_lines = params.indirect_sort = params.adaptive_frontier = true;
  params.parallel_assembly = params.single_pass = params.pair_graph = true;
  return params;
}

// Mode qualifie 16379 (bench/points_export.cpp) ; pour un ordre seul, sans ordres concurrents ni verticales.
FullParams full_params(bool order) {
  FullParams p;
  p.regular_batch_capacity = 4096; p.descent_lanes = 48;
  p.reuse_census_workspace = p.dense_birth_lookup = p.population_lookup = true;
  p.parallel_verticals = p.reuse_regular_verticals = p.concurrent_orders = !order;
  return p;
}

bool same_tree(const OrderForest& a, const OrderForest& b) {
  if (a.order() != b.order() || a.births() != b.births() || a.root() != b.root() ||
      a.nodes().size() != b.nodes().size() || a.edges().size() != b.edges().size() ||
      !std::equal(a.edges().begin(), a.edges().end(), b.edges().begin())) return false;
  for (u64 i = 0; i < a.nodes().size(); ++i) {
    const auto& x = a.nodes()[i]; const auto& y = b.nodes()[i];
    if (x.rank != y.rank || x.parent != y.parent || x.child_count != y.child_count ||
        x.child_begin != y.child_begin || x.birth_key != y.birth_key) return false;
  }
  return true;
}
ForestLedger logical(ForestLedger value) {
  value.descent = {};
  value.ancestor_hops = value.vertical_descents = value.vertical_checks = value.vertical_reuses = 0;
  value.ancestor_queries = value.ancestor_activations = value.ancestor_unions = value.ancestor_find_steps = 0;
  return value;
}

// I1 a I4 recomptes depuis le rattachement publie et la foret, sans le journal ni le balayage du produit.
bool recount(const OrderTree& tree, Counts& c) {
  const auto& domain = tree.domain();
  const auto& forest = tree.forest();
  const auto& a = tree.attachment();
  const auto balls = domain.catalogue().balls_data();
  const auto nodes = forest.nodes();
  const u32 k = forest.order();
  u64 window = 0;
  for (const auto& ball : balls) window += window_ball(ball, k) ? 1 : 0;
  std::vector<u8> covered(nodes.size(), 0);
  std::vector<u32> group;  // noeuds internes du plateau courant
  u64 cells = 0, plateaus = 0, combinations = 0, continuations = 0, distinct_prior = 0;
  u32 last_rank = kNone, group_rank = kNone;
  auto flush = [&]() {
    std::sort(group.begin(), group.end());
    continuations += static_cast<u64>(std::unique(group.begin(), group.end()) - group.begin());
    group.clear();
  };
  bool ok = a.size() == window && a.prior_offsets().size() == window + 1 && a.prior_offsets()[0] == 0;
  for (u64 at = 0; ok && at < a.size(); ++at) {
    const auto& data = balls[idx(a.balls()[at])];
    const NodeIdx att = a.node()[at];
    const u64 begin = a.prior_offsets()[at], end = a.prior_offsets()[at + 1];
    ok = window_ball(data, k) && (at == 0 || idx(a.balls()[at - 1]) < idx(a.balls()[at])) &&
         idx(att) < nodes.size() && begin <= end && end <= a.prior().size();
    if (!ok) break;
    const auto& node = nodes[idx(att)];
    if (a.role()[at] == BallRole::birth) {
      // Une naissance est forte (p+q <= K) : une boule faible n'est jamais une naissance (contrat, reponse D.4).
      ok = k >= 2 && u64{data.p} + data.qmin <= k && idx(att) < forest.births() &&
           node.birth_key == idx(a.balls()[at]) && node.rank == data.rank && a.strict_traces()[at] == 0 &&
           a.components()[at] == 0 && begin == end;
      ++c.births;
      continue;
    }
    if (idx(data.rank) != last_rank) { ++plateaus; last_rank = idx(data.rank); }
    ++cells; c.traces += a.strict_traces()[at];
    auto universe = cell_binomial(data.m, k - data.p);
    ok = universe.ok() && a.strict_traces()[at] >= 1 && a.components()[at] >= 1;
    if (!ok) break;
    combinations += universe.value();
    if (a.role()[at] == BallRole::merge) {
      ok = idx(att) >= forest.births() && node.rank == data.rank && end - begin == a.components()[at];
      for (u64 j = begin; ok && j < end; ++j) {
        const u32 u = idx(a.prior()[j]);
        ok = u < nodes.size() && nodes[u].parent == att && (j == begin || idx(a.prior()[j - 1]) < u);
        if (ok && covered[u] == 0) { covered[u] = 1; ++distinct_prior; }
      }
      ++c.merges; c.prior += end - begin;
    } else {
      const bool alive = node.parent == NodeIdx{kNone} || idx(nodes[idx(node.parent)].rank) > idx(data.rank);
      ok = a.role()[at] == BallRole::internal && a.components()[at] == 1 && begin == end && alive &&
           idx(node.rank) < idx(data.rank);
      if (group_rank != idx(data.rank)) { flush(); group_rank = idx(data.rank); }
      group.push_back(idx(att));
      ++c.internals;
    }
  }
  flush();
  const auto& ledger = forest.ledger();
  return ok && c.births == (k == 1 ? 0u : forest.births()) && distinct_prior == forest.edges().size() &&
         c.traces == ledger.trace_resolutions && combinations == ledger.cells.combinations &&
         cells == ledger.replayed_cells && plateaus == ledger.plateaus && continuations == ledger.continuations &&
         distinct_prior + continuations == ledger.touched_components;
}

// Lemme E : naissance de la descente d'une K-partie de P_b (memo et census comme ball_nodes), relevee au rang de b.
Result<NodeIdx> lemma_e(const OrderTree& tree, BallIdx ball, bool first, DescentMemo& memo, CensusWorkspace& scratch,
                        MemoryBudget& budget) {
  const auto& domain = tree.domain();
  const auto& catalogue = domain.catalogue();
  const u32 k = tree.order();
  const auto inner = catalogue.interior(ball), shell = catalogue.shell(ball);
  std::array<SiteIdx, kMaxMebSites> part{};
  u32 used = 0;
  if (first) {  // les K premiers sites : interieurs puis coquille, comme ball_nodes
    for (SiteIdx s : inner) { if (used == k) break; part[used++] = s; }
    for (SiteIdx s : shell) { if (used == k) break; part[used++] = s; }
  } else {      // les K derniers : fin de la coquille, puis fin de l'interieur
    for (u64 j = shell.size(); j != 0 && used < k; --j) part[used++] = shell[j - 1];
    for (u64 j = inner.size(); j != 0 && used < k; --j) part[used++] = inner[j - 1];
  }
  if (used != k) return fail(Reason::tower_invariant);
  std::sort(part.begin(), part.begin() + k, [](SiteIdx a, SiteIdx b) { return idx(a) < idx(b); });
  const auto& data = catalogue.balls_data()[idx(ball)];
  auto down = resolve_descent(domain, std::span<const SiteIdx>(part.data(), k), k, budget, &memo, &scratch);
  if (!down.ok()) return down.outcome();
  if (num::compare(down.value().initial_level(), catalogue.levels()[idx(data.rank)]) > 0)
    return fail(Reason::tower_invariant);
  const auto birth = tree.forest().birth_node(down.value().seed());
  if (!birth) return fail(Reason::tower_invariant);
  u64 hops = 0;
  return tree.forest().ancestor_closed(*birth, data.rank, hops);
}

// Positions de W_K jugees par E2 : toutes, ou un tirage sans remise a graine fixe (Fisher-Yates partiel, mt19937
// brut : meme tirage sur toute plateforme).
std::vector<u64> e2_positions(const Options& o, u64 size) {
  std::vector<u64> all(size);
  for (u64 i = 0; i < size; ++i) all[i] = i;
  if (o.all || o.sample >= size) return all;
  std::mt19937_64 rng(o.sample_seed);
  for (u64 i = 0; i < o.sample; ++i) std::swap(all[i], all[i + rng() % (size - i)]);
  all.resize(o.sample);
  std::sort(all.begin(), all.end());
  return all;
}

struct Run {
  Options options;
  Counts counts;
  u64 mismatches = 0;
  bool recounted = true, below_floor = false;
};

Result<FullDomain> domain_of(const Input& input, u64 k, MemoryBudget& budget, sched::Pool& pool) {
  auto cloud = prepare_cloud(input.x.span(), input.y.span(), input.z.span(), input.ids.span(), CoordWidth(), budget);
  if (!cloud.ok()) return cloud.outcome();
  if (cloud.value().weight() != cloud.value().sites()) return fail(Reason::multiplicity_unsupported);
  auto index = build_index(std::move(cloud.value()), {}, budget);
  if (!index.ok()) return index.outcome();
  return prepare_full_domain(std::move(index.value()), catalogue_params(k), budget, pool);
}

Result<Input> read(const Options& o, MemoryBudget& budget) {
  if (o.uniform != 0) return uniform18(o.uniform, static_cast<u32>(o.seed), budget);
  if (o.data.empty()) return read_input(o.xyz.c_str(), o.ids.c_str(), budget);
  const char* folder = std::getenv("MHGP11_DATA_DIR");
  if (folder == nullptr || *folder == '\0') return fail(Reason::input_unreadable);
  const std::string base = std::string(folder) + "/" + o.data;
  return read_input((base + ".u32le").c_str(), (base + ".ids.u32le").c_str(), budget);
}

// I10 : meme foret que l'ordre K de build_full en mode 16379 (pipeline des ordres concurrents, verticales).
Outcome identity(Run& run, const Input& input, const OrderTree& tree, MemoryBudget& budget, sched::Pool& pool) {
  auto domain = domain_of(input, run.options.k, budget, pool);
  if (!domain.ok()) return domain.outcome();
  Stopwatch clock;
  auto full = build_full(std::move(domain.value()), budget, nullptr, full_params(false), &pool);
  if (!full.ok()) return full.outcome();
  const auto& other = full.value().order(static_cast<Order>(run.options.k));
  run.counts.identity = same_tree(tree.forest(), other) && logical(tree.forest().ledger()) == logical(other.ledger());
  std::cout << "{\"phase\":\"identity\",\"full_ns\":" << clock.nanoseconds() << ",\"same\":"
            << (run.counts.identity ? "true" : "false") << "}\n";
  if (!run.counts.identity) ++run.mismatches;
  return {};
}

Outcome lemma_e_all(Run& run, const OrderTree& tree, MemoryBudget& budget) {
  auto scratch = CensusWorkspace::make(tree.domain().index(), budget);
  if (!scratch.ok()) return scratch.outcome();
  auto memo = DescentMemo::make(tree.domain(), 4096, budget, scratch.value().get());
  if (!memo.ok()) return memo.outcome();
  const auto& a = tree.attachment();
  Stopwatch clock;
  for (u64 at : e2_positions(run.options, a.size())) {
    for (bool first : {true, false}) {
      auto node = lemma_e(tree, a.balls()[at], first, memo.value(), *scratch.value(), budget);
      if (!node.ok()) return node.outcome();
      if (node.value() != a.node()[at]) {
        if (run.mismatches < 5)
          std::cout << "{\"phase\":\"e2_ecart\",\"ball\":" << idx(a.balls()[at]) << ",\"e1\":" << idx(a.node()[at])
                    << ",\"e2\":" << idx(node.value()) << ",\"first\":" << (first ? "true" : "false") << "}\n";
        ++run.mismatches;
      }
    }
    ++run.counts.e2;
  }
  std::cout << "{\"phase\":\"e2\",\"judged\":" << run.counts.e2 << ",\"e2_ns\":" << clock.nanoseconds() << "}\n";
  return {};
}

Outcome execute(Run& run) {
  const Options& o = run.options;
  MemoryBudget budget(o.budget);
  auto input = read(o, budget);
  if (!input.ok()) return input.outcome();
  run.counts.fnv = fnv1a(input.value());
  auto pool = sched::make_pool({static_cast<u32>(o.workers)});
  if (!pool.ok()) return pool.outcome();
  auto domain = domain_of(input.value(), o.k, budget, *pool.value());
  if (!domain.ok()) return domain.outcome();
  run.counts.n = domain.value().index().cloud().sites();
  if (o.k > run.counts.n) return fail(Reason::parameter_out_of_range);
  OrderTimings timings;
  u64 attach_ns = 0;
  Stopwatch clock;
  auto tree = build_order(std::move(domain.value()), static_cast<Order>(o.k), budget,
                          o.serial ? FullParams{} : full_params(true), o.serial ? nullptr : pool.value().get(),
                          &timings, &attach_ns);
  if (!tree.ok()) return tree.outcome();
  const auto& a = tree.value().attachment();
  run.counts.balls = a.size(); run.counts.nodes = tree.value().forest().nodes().size();
  run.counts.attachment = attachment_digest(a);
  std::cout << "{\"phase\":\"order\",\"order_ns\":" << clock.nanoseconds() << ",\"attach_ns\":" << attach_ns
            << ",\"classify_ns\":" << timings.classify_ns << ",\"births_ns\":" << timings.births_ns
            << ",\"plateaus_ns\":" << timings.plateaus_ns << ",\"peak_bytes\":" << budget.peak()
            << ",\"serial\":" << (o.serial ? "true" : "false")
            << ",\"nodes\":" << run.counts.nodes << ",\"balls\":" << run.counts.balls << "}\n";
  run.recounted = recount(tree.value(), run.counts);
  if (!run.recounted) ++run.mismatches;
  if (o.identity) MHGP11_TRY(identity(run, input.value(), tree.value(), budget, *pool.value()));
  if (o.all || o.sample != 0) MHGP11_TRY(lemma_e_all(run, tree.value(), budget));
  run.below_floor = run.counts.balls < o.min_balls || run.counts.merges < o.min_merges ||
                    run.counts.internals < o.min_internals || run.counts.e2 < o.min_e2;
  return {};
}
}  // namespace

int main(int argc, char** argv) {
  Run run;
  if (!parse_options(argc, argv, run.options)) {
    std::cout << "attach_judge_verdict refus usage\n";
    return 2;
  }
  const Outcome outcome = guarded([&]() { return execute(run); });
  const Counts& c = run.counts;
  const char* verdict = !outcome.ok() ? "refus" : run.mismatches != 0 ? "ecart" : run.below_floor ? "plancher" : "conforme";
  if (!outcome.ok())
    std::cout << "{\"phase\":\"exit\",\"reason\":\"" << reason_name(outcome.reason) << "\"}\n";
  std::cout << "attach_judge_verdict " << verdict << " k=" << run.options.k << " n=" << c.n << " boules=" << c.balls
            << " naissances=" << c.births << " fusions=" << c.merges << " internes=" << c.internals
            << " branches=" << c.prior << " traces=" << c.traces << " noeuds=" << c.nodes << " e2=" << c.e2
            << " identite=" << (c.identity ? 1 : 0) << " attache=" << hex64(c.attachment)
            << " entree=" << hex64(c.fnv) << "\n";
  if (!outcome.ok()) return exit_code(outcome);
  if (run.mismatches != 0) return 1;
  return run.below_floor ? 3 : 0;
}
