// Vidage du parcours des boites du catalogue de la v11 (microbanc MES-M5 de la v12, hors produit).
//
// Lie a la bibliotheque v11 gelee (libmhgp11.a, moteur ac081a06f, profil u21) et a ses en-tetes internes, sans les
// modifier. Deux parcours du meme nuage :
//   1. capture : un parcours en profondeur gauche/droite qui reprend mot pour mot le controle de walk / process /
//      run_ready (boxes.cpp) mais appelle les fonctions GELEES prepare_node et split_ready ; chaque appel de
//      prepare_node devient un noeud (boite d'entree, candidats, tests G1, liste retenue, genre, chemin), chaque feuille
//      une feuille (liste, boite ajustee transmise a enumerate_leaf, profondeur, chemin) ;
//   2. temoin : le walk gele lui-meme, file differee branchee sur Run::deferred (comme MES-M2) ; son grand livre
//      (noeuds, feuilles, tests G1, profondeur et feuille maximales) et ses feuilles mises en file doivent egaler la
//      capture, sinon code 3. La capture est donc le parcours de la v11, pas une reecriture.
// Les chronos sont indicatifs (machine locale, un fil).
//
// Usage : mhgp12_traversal_dump <xyz.u32le> <ids.u32le> <K> <taille_feuille> <sortie.bin> [--crop N] [--max-leaf M]
//   --crop N : seulement les N premiers sites dans l'ordre de Morton du nuage prepare (une region compacte), pour
//              les controles sous Compute Sanitizer ; le nuage est re-prepare (meme ordre relatif).
// Sortie standard : une ligne JSON de synthese. Codes : 0 conforme, 2 refus d'entree ou d'arguments, 3 invariant viole.
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <new>
#include <optional>
#include <string>
#include <vector>

#include "catalogue/internal.hpp"
#include "mhgp12/traversal/format.hpp"
#include "whole_input.hpp"

using namespace mhgp11;
using namespace mhgp11::catalogue_detail;
namespace fmt = mhgp12::traversal::format;

namespace {

u64 now_ns() {
  return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::steady_clock::now().time_since_epoch())
                              .count());
}

struct LevelStats {
  u64 nodes = 0, candidates = 0, max_candidates = 0, tests = 0, kept = 0, leaves = 0, splits = 0, empties = 0;
};

struct Capture {
  fmt::Dump& d;
  std::vector<LevelStats> levels;
  bool depth_refusal = false;
};

void copy_box(const Box& b, i64* lo, i64* hi) {
  for (int a = 0; a < 3; ++a) {
    lo[a] = b.lo[a];
    hi[a] = b.hi[a];
  }
}

// Meme controle que process() puis run_ready() (boxes.cpp), feuilles capturees au lieu d'enumerate_leaf.
Outcome visit(Run& run, std::span<const SiteIdx> parent, const Box& box, u32 depth, std::array<u64, 2> path,
              Capture& cap) {
  if (depth > kMaxDepth) cap.depth_refusal = true;  // prepare_node refuse aussitot (catalogue_invariant)
  const u64 before = run.ledger.filter_tests;
  ReadyNode ready;
  MHGP11_TRY(prepare_node(run, parent, box, depth, ready));
  fmt::Node node;
  copy_box(box, node.lo, node.hi);
  node.path[0] = path[0];
  node.path[1] = path[1];
  node.tests = run.ledger.filter_tests - before;
  node.candidates = static_cast<u32>(parent.size());
  node.count = ready.count;
  node.depth = depth;
  if (cap.levels.size() <= depth) cap.levels.resize(depth + 1);
  LevelStats& level = cap.levels[depth];
  ++level.nodes;
  level.candidates += parent.size();
  level.max_candidates = std::max<u64>(level.max_candidates, parent.size());
  level.tests += node.tests;
  level.kept += ready.count;
  if (ready.count == 0) {  // liste vide ou boite ajustee vide : process() s'arrete la
    node.kind = fmt::kKindEmpty;
    ++level.empties;
    cap.d.nodes.push_back(node);
    return {};
  }
  std::vector<u32> list(ready.count);
  for (u32 i = 0; i < ready.count; ++i) list[i] = idx(ready.sites()[i]);
  node.list_fnv = fmt::list_fnv(list.data(), list.size());
  // run_ready
  if (ready.count == 0 || ready.count > ready.storage.size() || ready.depth > kMaxDepth)
    return fail(Reason::catalogue_invariant);
  Box left, right;
  if (split_ready(ready, run.params, left, right)) {
    node.kind = fmt::kKindSplit;
    ++level.splits;
    cap.d.nodes.push_back(node);
    MHGP11_TRY(visit(run, ready.sites(), left, depth + 1, path, cap));
    std::array<u64, 2> right_path = path;
    right_path[depth / 64] |= u64{1} << (63 - depth % 64);
    return visit(run, ready.sites(), right, depth + 1, right_path, cap);
  }
  node.kind = fmt::kKindLeaf;
  ++level.leaves;
  cap.d.nodes.push_back(node);
  MHGP11_TRY(checked_add(run.ledger.leaves, 1));
  run.ledger.max_leaf = std::max<u64>(run.ledger.max_leaf, ready.count);
  if (ready.count > run.params.max_leaf) return fail(Reason::wide_leaf);
  fmt::Leaf leaf;
  leaf.begin = cap.d.sites.size();
  leaf.m = ready.count;
  leaf.depth = depth;
  copy_box(ready.box, leaf.lo, leaf.hi);
  leaf.path[0] = path[0];
  leaf.path[1] = path[1];
  cap.d.sites.insert(cap.d.sites.end(), list.begin(), list.end());
  cap.d.leaves.push_back(leaf);
  return {};
}

// File differee du walk gele : copie chaque feuille admissible au lot (la v11 la mettrait en file).
struct DumpQueue final : LeafQueue {
  std::vector<fmt::Leaf> leaves;
  std::vector<u32> sites;
  Outcome push(std::span<const SiteIdx> leaf, const Box& box) noexcept override {
    fmt::Leaf l;
    l.begin = sites.size();
    l.m = static_cast<u32>(leaf.size());
    copy_box(box, l.lo, l.hi);
    for (SiteIdx s : leaf) sites.push_back(idx(s));
    leaves.push_back(l);
    return {};
  }
};

int fail_with(const Outcome& o, const char* stage) {
  std::cout << "{\"phase\":\"exit\",\"stage\":\"" << stage << "\",\"reason\":\"" << reason_name(o.reason) << "\"}\n";
  return 2;
}

int run(int argc, char** argv) {
  if (argc < 6) return 2;
  u64 kmax = 0, leaf_size = 0, crop = 0, max_leaf = 256;
  if (!bench::parse(argv[3], kmax) || !bench::parse(argv[4], leaf_size)) return 2;
  for (int i = 6; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--crop" && i + 1 < argc) {
      if (!bench::parse(argv[++i], crop) || crop == 0) return 2;
    } else if (a == "--max-leaf" && i + 1 < argc) {
      if (!bench::parse(argv[++i], max_leaf)) return 2;
    } else {
      return 2;
    }
  }
  const std::string out_path = argv[5];

  MemoryBudget budget(u64{64} << 30);
  auto input = bench::read_input(argv[1], argv[2], budget);
  if (!input.ok()) return fail_with(input.outcome(), "read_input");
  auto prepared = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                                input.value().ids.span(), CoordWidth(), budget);
  if (!prepared.ok()) return fail_with(prepared.outcome(), "prepare_cloud");
  for (u32 w : prepared.value().w())
    if (w != 1) return fail_with(fail(Reason::multiplicity_unsupported), "weights");
  // Decoupe : les N premiers sites de l'ordre de Morton, nuage re-prepare.
  std::optional<Cloud> cropped;
  if (crop != 0 && crop < prepared.value().sites()) {
    const Cloud& c = prepared.value();
    Buffer<u32> x, y, z;
    Buffer<PointId> ids;
    if (!x.allocate(crop, budget).ok() || !y.allocate(crop, budget).ok() || !z.allocate(crop, budget).ok() ||
        !ids.allocate(crop, budget).ok())
      return fail_with(fail(Reason::memory_budget), "crop");
    for (u32 i = 0; i < crop; ++i) {
      x[i] = c.x()[i];
      y[i] = c.y()[i];
      z[i] = c.z()[i];
      ids[i] = c.points(make_id<SiteIdx>(i))[0];
    }
    auto again = prepare_cloud(x.span(), y.span(), z.span(), ids.span(), CoordWidth(), budget);
    if (!again.ok()) return fail_with(again.outcome(), "crop_prepare");
    cropped.emplace(std::move(again.value()));
  }
  const Cloud& c = cropped ? *cropped : prepared.value();

  CatalogueParams params;
  params.kmax = static_cast<int>(kmax);
  params.leaf_size = static_cast<u32>(leaf_size);
  params.max_leaf = static_cast<u32>(max_leaf);
  params.cache_center_lines = true;
  params.pair_graph = true;
  if (const Outcome o = check_catalogue_params(params); !o.ok()) return fail_with(o, "params");

  fmt::Dump d;
  d.header.producer = fmt::kProducerV11;
  d.header.coord_bits = kCoordBits;
  d.header.kmax = kmax;
  d.header.leaf_size = leaf_size;
  d.header.max_leaf = max_leaf;
  d.header.n_sites = c.sites();
  d.x.assign(c.x().begin(), c.x().end());
  d.y.assign(c.y().begin(), c.y().end());
  d.z.assign(c.z().begin(), c.z().end());

  // 1. Capture.
  Workspace unused_space;
  Collector unused_collector;
  Capture cap{d, {}, false};
  Run capture_run{c, params, budget, unused_space, unused_collector, {}, nullptr, nullptr};
  const u64 t_capture = now_ns();
  Outcome captured;
  {
    Buffer<SiteIdx> root;
    Box box;
    captured = make_root(capture_run, root, box);
    if (captured.ok()) captured = visit(capture_run, root.span(), box, 0, {0, 0}, cap);
  }
  const u64 capture_ns = now_ns() - t_capture;

  // 2. Temoin : walk gele, feuilles admissibles en file.
  Workspace walk_space;
  if (const Outcome o = walk_space.allocate(params.max_leaf, budget, params.cache_center_lines, params.pair_graph);
      !o.ok())
    return fail_with(o, "workspace");
  Collector walk_collector;
  DumpQueue queue;
  Run walker{c, params, budget, walk_space, walk_collector, {}, nullptr, &queue};
  const u64 t_walk = now_ns();
  const Outcome walked = walk(walker);
  const u64 walk_ns = now_ns() - t_walk;

  // Refus : meme refus des deux cotes, rien de publie (aucun prefixe).
  if (!captured.ok() || !walked.ok()) {
    if (captured.ok() != walked.ok() || captured.reason != walked.reason) {
      std::cout << "{\"phase\":\"exit\",\"stage\":\"refusal_mismatch\"}\n";
      return 3;
    }
    u64 status = 0;
    if (captured.reason == Reason::wide_leaf) status = fmt::kStatusWideLeaf;
    else if (captured.reason == Reason::catalogue_invariant && cap.depth_refusal) status = fmt::kStatusDepth;
    else return fail_with(captured, "walk");
    d.header.status = status;
    d.nodes.clear();
    d.leaves.clear();
    d.sites.clear();
  } else {
    const CatalogueLedger& a = capture_run.ledger;
    const CatalogueLedger& b = walker.ledger;
    if (a.nodes != b.nodes || a.leaves != b.leaves || a.filter_tests != b.filter_tests || a.max_depth != b.max_depth ||
        a.max_leaf != b.max_leaf || a.nodes != d.nodes.size() || a.leaves != d.leaves.size()) {
      std::cout << "{\"phase\":\"exit\",\"stage\":\"ledger_mismatch\"}\n";
      return 3;
    }
    // Feuilles mises en file par le walk gele = feuilles capturees de m <= 32 (voie lot), dans le meme ordre.
    u64 q = 0;
    for (u64 j = 0; j < d.leaves.size(); ++j) {
      const fmt::Leaf& l = d.leaves[j];
      if (l.m > 32) continue;
      if (q >= queue.leaves.size()) {
        std::cout << "{\"phase\":\"exit\",\"stage\":\"queue_short\"}\n";
        return 3;
      }
      const fmt::Leaf& w = queue.leaves[q++];
      bool same = w.m == l.m && std::memcmp(w.lo, l.lo, sizeof(l.lo)) == 0 && std::memcmp(w.hi, l.hi, sizeof(l.hi)) == 0 &&
                  std::memcmp(queue.sites.data() + w.begin, d.sites.data() + l.begin, l.m * sizeof(u32)) == 0;
      if (!same) {
        std::cout << "{\"phase\":\"exit\",\"stage\":\"queue_mismatch\",\"leaf\":" << j << "}\n";
        return 3;
      }
    }
    if (q != queue.leaves.size()) {
      std::cout << "{\"phase\":\"exit\",\"stage\":\"queue_long\"}\n";
      return 3;
    }
    d.header.nodes = a.nodes;
    d.header.leaves = a.leaves;
    d.header.filter_tests = a.filter_tests;
    d.header.max_depth = a.max_depth;
    d.header.max_leaf_seen = a.max_leaf;
  }
  d.header.n_nodes = d.nodes.size();
  d.header.n_leaves = d.leaves.size();
  d.header.n_leaf_sites = d.sites.size();

  std::string error;
  const u64 t_write = now_ns();
  if (!fmt::write(out_path, d, error)) {
    std::cerr << error << '\n';
    return 2;
  }
  const u64 write_ns = now_ns() - t_write;
  // Relecture stricte de ce qui vient d'etre ecrit (le lecteur du banc).
  fmt::Dump check;
  if (!fmt::read(out_path, check, error)) {
    std::cout << "{\"phase\":\"exit\",\"stage\":\"reread\",\"error\":\"" << error << "\"}\n";
    return 3;
  }

  u64 max_m = 0;
  for (const auto& l : d.leaves) max_m = std::max<u64>(max_m, l.m);
  std::cout << "{\"phase\":\"dump\",\"format\":\"MHGP12TR\",\"version\":" << fmt::kVersion
            << ",\"coord_bits\":" << kCoordBits << ",\"kmax\":" << kmax << ",\"leaf_size\":" << leaf_size
            << ",\"max_leaf\":" << max_leaf << ",\"crop\":" << crop << ",\"sites\":" << c.sites()
            << ",\"status\":" << d.header.status << ",\"nodes\":" << d.header.nodes << ",\"leaves\":" << d.header.leaves
            << ",\"filter_tests\":" << d.header.filter_tests << ",\"max_depth\":" << d.header.max_depth
            << ",\"max_leaf_seen\":" << d.header.max_leaf_seen << ",\"leaf_sites\":" << d.sites.size()
            << ",\"max_m\":" << max_m << ",\"queued_leaves\":" << queue.leaves.size()
            << ",\"inline_balls\":" << walk_collector.balls << ",\"levels\":[";
  for (size_t l = 0; l < cap.levels.size(); ++l) {
    const LevelStats& s = cap.levels[l];
    std::cout << (l ? "," : "") << "[" << s.nodes << "," << s.candidates << "," << s.max_candidates << "," << s.tests
              << "," << s.kept << "," << s.leaves << "," << s.splits << "," << s.empties << "]";
  }
  std::cout << "],\"level_fields\":[\"nodes\",\"candidates\",\"max_candidates\",\"tests\",\"kept\",\"leaves\","
               "\"splits\",\"empties\"],\"capture_ns\":"
            << capture_ns << ",\"v11_walk_ns\":" << walk_ns << ",\"write_ns\":" << write_ns << "}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(argc, argv);
  } catch (const std::bad_alloc&) {
    std::cout << "{\"phase\":\"exit\",\"stage\":\"allocation\",\"reason\":\"memory_budget\"}\n";
    return 2;
  }
}
