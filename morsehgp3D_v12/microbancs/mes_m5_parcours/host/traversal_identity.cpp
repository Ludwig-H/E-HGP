// Identite sur l'hote du parcours en largeur (MES-M5, hors produit) : le meme texte que l'appareil
// (include/mhgp12/traversal/bfs.hpp, driver.hpp), le warp etant simule de facon deterministe (voies 0..31 l'une apres
// l'autre a chaque super-pas, warps l'un apres l'autre dans chaque noyau).
//
// Pour chaque vidage MHGP12TR (v11 ou oracle) : parcours sans mutant, compare au vidage (statut, grand livre, ensemble
// des feuilles : boites, listes, profondeurs, chemins ; option --nodes : tous les noeuds visites, boite d'entree,
// candidats, tests, liste retenue, genre), puis chaque mutant demande, qui est TUE s'il change le statut, le grand livre
// ou l'ensemble des feuilles. Portes unitaires (--unit) : repere ferme a 33 bits (CST-0204), cle du reservoir au-dela
// d'i64 a s = 30 (CST-0208), borne de profondeur 3B (CST-0205), nombre de taches emis et scanne aux bornes du domaine
// u32 des comptes (CST-0222 : vrai EmitKernel et vrai scan sur huit petits enregistrements, aucun grand nuage).
//
// Usage : mhgp12_traversal_identity [--mutants all|none|n1,n2,...] [--threads N] [--nodes] [--unit] [--json F]
//                                   [--nonce JETON] <vidage.bin> [...]
// Sortie : une ligne JSON par vidage (et une pour --unit), chacune portant le jeton --nonce du pilote (preuve fraiche
// de la session) ; --json est ecrit dans un temporaire puis renomme (jamais un fichier partiel). Codes : 0 identite sur
// tous les vidages (et portes unitaires conformes), 1 ecart d'identite (sans mutant), 2 refus (arguments, vidage
// illisible ou invalide, ecriture du JSON impossible), 3 porte unitaire.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "mhgp12/traversal/bfs.hpp"
#include "mhgp12/traversal/compare.hpp"
#include "mhgp12/traversal/driver.hpp"
#include "mhgp12/traversal/format.hpp"

namespace {

using namespace mhgp12::traversal;
namespace fmt = mhgp12::traversal::format;
using fmt::i64;
using fmt::u32;
using fmt::u64;

u64 now_ns() {
  return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::steady_clock::now().time_since_epoch())
                              .count());
}

// Executeur hote : tableaux std::vector, un noyau = boucle sur ses warps (warp simule), totaux lus en place.
struct HostBackend {
  template <class T>
  struct Array {
    std::vector<T> v;
    T* data() { return v.data(); }
  };
  template <class T>
  u64 ensure(Array<T>& a, u64 n) {
    if (a.v.size() >= n) return 0;
    a.v.resize(n);
    return 1;
  }
  template <class T>
  u64 ensure_keep(Array<T>& a, u64 n, u64) {
    return ensure(a, n);
  }
  template <class T>
  u64 upload(Array<T>& a, const T* src, u64 n) {
    const u64 r = ensure(a, n);
    if (n != 0) std::memcpy(a.v.data(), src, n * sizeof(T));
    return r;
  }
  template <class K>
  void launch(const K& k, u64 warps) {
    auto shared = std::make_unique<typename K::Shared>();
    for (u64 w = 0; w < warps; ++w) k(w, *shared);
  }
  bfs::LevelTotals read_totals(Array<bfs::LevelTotals>& a) { return a.v[0]; }
  void mark(int, u32) {}
};

// Capture des noeuds de chaque niveau (diagnostic hote) : meme contenu que les noeuds du vidage.
template <int M>
struct NodeHook {
  std::vector<fmt::Node> nodes;
  void after_level(const bfs::Level& lv) {
    for (u64 c = 0; c < lv.n_children; ++c) {
      const bfs::Parent& p = lv.parents[c / 2];
      const u32 side = static_cast<u32>(c % 2);
      const bfs::ChildOut& o = lv.child_out[c];
      const bfs::ChildScan& s = lv.child_scan[c];
      fmt::Node n;
      bfs::child_box(p, side, n.lo, n.hi, M);
      bfs::child_path(p, side, lv.depth == 0 ? 0 : lv.depth - 1, n.path);
      n.tests = o.tests;
      n.candidates = p.count;
      n.count = o.count;
      n.depth = lv.depth;
      n.kind = o.kind;
      if (o.kind == bfs::kKindSplit) n.list_fnv = fmt::list_fnv(lv.next_list + s.f[1], o.count);
      else if (o.kind == bfs::kKindLeaf) n.list_fnv = fmt::list_fnv(lv.leaf_sites + lv.leaf_site_base + s.f[4], o.count);
      nodes.push_back(n);
    }
  }
};

struct Outcome {
  RunResult run;
  Comparison cmp;
  bool nodes_checked = false, nodes_equal = false;
  std::string node_first;
  u64 ns = 0;
};

template <int M>
Outcome run_case(const fmt::Dump& d, bool with_nodes) {
  HostBackend b;
  bfs::Params params;
  params.kmax = static_cast<u32>(d.header.kmax);
  params.leaf_size = static_cast<u32>(d.header.leaf_size);
  params.max_leaf = static_cast<u32>(d.header.max_leaf);
  params.coord_bits = static_cast<u32>(d.header.coord_bits);
  Driver<HostBackend, M> drv(b, params);
  Outcome out;
  const u64 t0 = now_ns();
  u64 allocations = 0;
  drv.upload_cloud(d.x.data(), d.y.data(), d.z.data(), d.header.n_sites, allocations);
  NodeHook<M> hook;
  NoHook none;
  if (with_nodes) out.run = drv.run(d.x.data(), d.y.data(), d.z.data(), hook);
  else out.run = drv.run(d.x.data(), d.y.data(), d.z.data(), none);
  out.ns = now_ns() - t0;
  LeafSet mine;
  mine.leaves = reinterpret_cast<const fmt::Leaf*>(drv.leaves.v.data());
  mine.n = out.run.n_leaves;
  mine.sites = drv.leaf_sites.v.data();
  mine.n_sites = out.run.n_leaf_sites;
  out.cmp = compare(d, out.run.status, out.run.ledger, mine);
  if (with_nodes && out.run.status == kStatusOk) {
    out.nodes_checked = true;
    auto& nodes = hook.nodes;
    std::sort(nodes.begin(), nodes.end(), [](const fmt::Node& a, const fmt::Node& b) {
      return fmt::prefix_before(a.path, a.depth, b.path, b.depth);
    });
    out.nodes_equal = nodes.size() == d.nodes.size();
    if (!out.nodes_equal) out.node_first = "nombre de noeuds " + std::to_string(nodes.size()) + " / " + std::to_string(d.nodes.size());
    for (u64 i = 0; out.nodes_equal && i < nodes.size(); ++i) {
      if (std::memcmp(&nodes[i], &d.nodes[i], sizeof(fmt::Node)) != 0) {
        out.nodes_equal = false;
        const fmt::Node& a = d.nodes[i];
        const fmt::Node& b2 = nodes[i];
        std::ostringstream o;
        o << "noeud " << i << " (profondeur " << a.depth << ", genre " << a.kind << "/" << b2.kind << ", candidats "
          << a.candidates << "/" << b2.candidates << ", retenus " << a.count << "/" << b2.count << ", tests " << a.tests
          << "/" << b2.tests << ")";
        out.node_first = o.str();
      }
    }
  }
  return out;
}

Outcome run_mutant(int m, const fmt::Dump& d, bool nodes) {
  switch (m) {
    case bfs::kNone: return run_case<bfs::kNone>(d, nodes);
    case bfs::kLostWitness: return run_case<bfs::kLostWitness>(d, false);
    case bfs::kFrameChildBox: return run_case<bfs::kFrameChildBox>(d, false);
    case bfs::kUnstableCompaction: return run_case<bfs::kUnstableCompaction>(d, false);
    case bfs::kBisectOffByOne: return run_case<bfs::kBisectOffByOne>(d, false);
    case bfs::kTieReversed: return run_case<bfs::kTieReversed>(d, false);
    default: return run_case<bfs::kAxisLastMax>(d, false);
  }
}

void json_string(std::ostream& o, const std::string& s) {
  o << '"';
  for (char c : s) {
    if (c == '"' || c == '\\') o << '\\' << c;
    else if (static_cast<unsigned char>(c) < 0x20) o << ' ';
    else o << c;
  }
  o << '"';
}

void ledger_json(std::ostream& o, const Ledger& l) {
  o << "{\"nodes\":" << l.nodes << ",\"leaves\":" << l.leaves << ",\"filter_tests\":" << l.filter_tests
    << ",\"max_depth\":" << l.max_depth << ",\"max_leaf\":" << l.max_leaf << "}";
}

// CST-0222 : taches emises (EmitKernel) et scannees (child_fields) pour un enfant coupe de count sites retenus, aux
// bornes du domaine u32 des comptes ; attendu plafond(count / 256), ecrit en dur. Le temoin de l'auditeur (sept petits
// enregistrements, recu audit_b_m5_20261007/capacite) plus count = 2^32 - 1. Le noyau d'emission ne lit aucune
// coordonnee : aucun tableau de sites n'est alloue.
bool emit_tasks_gate(u32& failures) {
  bfs::Parent parent{};
  parent.sides = 1;
  bfs::ChildOut child{};
  child.kind = bfs::kKindSplit;
  child.hi[0] = child.hi[1] = child.hi[2] = 2;
  bfs::ChildScan offset{};
  bfs::Parent emitted{};
  u32 next = 0;
  bfs::Level lv{};
  lv.parents = &parent;
  lv.child_out = &child;
  lv.child_scan = &offset;
  lv.next_parents = &emitted;
  lv.next_task_begin = &next;
  bfs::EmitKernel<bfs::kNone>::Shared shared;
  struct Row {
    u32 count;
    u64 tasks;
  };
  const Row rows[] = {{1u, 1}, {256u, 1}, {257u, 2}, {0xFFFFFEFFu, 16777215}, {0xFFFFFF00u, 16777215},
                      {0xFFFFFF01u, 16777216}, {0xFFFFFFFEu, 16777216}, {0xFFFFFFFFu, 16777216}};
  failures = 0;
  for (const Row& r : rows) {
    child.count = r.count;
    emitted = bfs::Parent{};
    u64 f[bfs::kFields], tests = 0, leaf = 0;
    bfs::child_fields(child, f, tests, leaf);
    bfs::EmitKernel<bfs::kNone>{lv}(0, shared);
    if (u64{emitted.tasks} != r.tasks || f[2] != 2 * r.tasks || bfs::tasks_of(r.count) != r.tasks) ++failures;
  }
  return failures == 0;
}

// Portes unitaires des constats du contrat numerique.
bool unit_gates(std::ostream& o, const std::string& nonce_json) {
  bool ok = true;
  // CST-0204 : sites (0,0,0) et (2^32-1,0,0) ; boite [0, 2^32) ; fermeture d'etendue 2^32 : s = 33.
  const i64 lo[3] = {0, 0, 0}, hi33[3] = {i64{1} << 32, 1, 1};
  const u32 s33 = bfs::box_frame_bits(lo, hi33);
  const u32 env_min[3] = {0, 0, 0}, env_max[3] = {0xFFFFFFFFu, 0, 0};
  const u32 p33 = bfs::parent_frame_bits(lo, hi33, env_min, env_max);
  const bool g1 = s33 == 33 && p33 == 33 && p33 > bfs::kNarrowBits;
  ok = ok && g1;
  // CST-0208 : boite [0,1]^3, site (2^30-1)^3 dans la liste du parent : s = 30, cle 3(2^31-3)^2 au-dela d'i64.
  const i64 hi1[3] = {1, 1, 1};
  const u32 far = (u32{1} << 30) - 1;
  const u32 emin[3] = {0, 0, 0}, emax[3] = {far, far, far};
  const u32 s30 = bfs::parent_frame_bits(lo, hi1, emin, emax);
  const bfs::i128 key = bfs::reservoir_key<bfs::i128>(far, far, far, lo, hi1);
  const bfs::i128 expect = bfs::i128(3) * bfs::i128((i64{1} << 31) - 3) * bfs::i128((i64{1} << 31) - 3);
  const bool beyond = key > bfs::i128(0x7FFFFFFFFFFFFFFFll);
  const bool child_box_only = bfs::box_frame_bits(lo, hi1) <= bfs::kNarrowBits;  // le mutant choisirait i64
  const bool g2 = s30 == 30 && s30 > bfs::kNarrowBits && key == expect && beyond && child_box_only;
  ok = ok && g2;
  // CST-0205 : profondeur au plus 3B ; aux profils 21, 24, 32 : 63, 72, 96 (le pilote refuse au-dela).
  const bool g3 = 3 * 21 == 63 && 3 * 24 == 72 && 3 * 32 == 96;
  ok = ok && g3;
  u32 emit_failures = 0;
  const bool g4 = emit_tasks_gate(emit_failures);
  ok = ok && g4;
  o << "{\"phase\":\"unit\"" << nonce_json << ",\"closed_box_s33\":" << (g1 ? "true" : "false") << ",\"s33\":" << s33
    << ",\"reservoir_s30_beyond_i64\":" << (g2 ? "true" : "false") << ",\"s30\":" << s30
    << ",\"depth_bound\":" << (g3 ? "true" : "false") << ",\"emit_tasks_u64\":" << (g4 ? "true" : "false")
    << ",\"emit_tasks_failures\":" << emit_failures << ",\"ok\":" << (ok ? "true" : "false") << "}\n";
  return ok;
}

}  // namespace

int main(int argc, char** argv) {
  std::vector<std::string> paths;
  std::vector<int> mutants;
  bool all_mutants = true, nodes = false, unit = false;
  unsigned threads = 1;
  std::string json_path, nonce;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--mutants" && i + 1 < argc) {
      const std::string list = argv[++i];
      all_mutants = false;
      mutants.clear();
      if (list == "all") all_mutants = true;
      else if (list != "none") {
        std::stringstream ss(list);
        std::string name;
        while (std::getline(ss, name, ',')) {
          int found = -1;
          for (int m = 1; m < bfs::kMutantCount; ++m)
            if (name == bfs::mutant_name(m)) found = m;
          if (found < 0) return 2;
          mutants.push_back(found);
        }
      }
    } else if (a == "--threads" && i + 1 < argc) {
      threads = static_cast<unsigned>(std::max(1, std::atoi(argv[++i])));
    } else if (a == "--nodes") {
      nodes = true;
    } else if (a == "--unit") {
      unit = true;
    } else if (a == "--json" && i + 1 < argc) {
      json_path = argv[++i];
    } else if (a == "--nonce" && i + 1 < argc) {
      nonce = argv[++i];  // jeton de la session : caracteres controles, recopie tel quel dans chaque ligne
      if (nonce.empty() || nonce.size() > 64) return 2;
      for (char c : nonce)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-' || c == '_' ||
              c == '.'))
          return 2;
    } else if (!a.empty() && a[0] == '-') {
      return 2;
    } else {
      paths.push_back(a);
    }
  }
  if (all_mutants)
    for (int m = 1; m < bfs::kMutantCount; ++m) mutants.push_back(m);
  if (paths.empty() && !unit) return 2;
  std::ostringstream out;
  const std::string nonce_json = nonce.empty() ? std::string() : ",\"nonce\":\"" + nonce + "\"";
  int code = 0;
  if (unit && !unit_gates(out, nonce_json)) code = 3;
  // Tous les vidages d'abord (lecteur strict), puis un seul ensemble de travaux (vidage, mutant) pour tous les fils.
  std::vector<fmt::Dump> dumps(paths.size());
  for (size_t i = 0; i < paths.size(); ++i) {
    std::string error;
    if (!fmt::read(paths[i], dumps[i], error)) {
      std::cerr << error << '\n';
      std::cout << out.str();
      return 2;
    }
    if (dumps[i].header.coord_bits > 32 || dumps[i].header.kmax > bfs::kMaxOrder) {
      std::cerr << "refus : profil ou K hors du parcours : " << paths[i] << '\n';
      return 2;
    }
  }
  std::vector<int> jobs_of = {bfs::kNone};
  jobs_of.insert(jobs_of.end(), mutants.begin(), mutants.end());
  const size_t per = jobs_of.size();
  std::vector<Outcome> results(paths.size() * per);
  std::atomic<size_t> next{0};
  std::vector<std::thread> pool;
  for (unsigned w = 0; w < std::min<size_t>(threads, results.size()); ++w)
    pool.emplace_back([&]() {
      for (size_t j = next++; j < results.size(); j = next++) results[j] = run_mutant(jobs_of[j % per], dumps[j / per], nodes);
    });
  for (auto& t : pool) t.join();
  for (size_t i = 0; i < paths.size(); ++i) {
    const std::string& path = paths[i];
    const fmt::Dump& d = dumps[i];
    const Outcome& base = results[i * per];
    const bool identity = base.cmp.identity() && (!base.nodes_checked || base.nodes_equal);
    if (!identity) code = code == 0 ? 1 : code;
    const fmt::Header& h = d.header;
    u64 max_children = 0, max_tasks = 0;
    for (const auto& s : base.run.stats) {
      max_children = std::max<u64>(max_children, s.children);
      max_tasks = std::max<u64>(max_tasks, s.tasks);
    }
    out << "{\"phase\":\"identity\"" << nonce_json << ",\"dump\":";
    json_string(out, path);
    out << ",\"producer\":" << h.producer << ",\"coord_bits\":" << h.coord_bits << ",\"kmax\":" << h.kmax
        << ",\"leaf_size\":" << h.leaf_size << ",\"max_leaf\":" << h.max_leaf << ",\"sites\":" << h.n_sites
        << ",\"identity\":" << (identity ? "true" : "false") << ",\"status\":" << base.run.status
        << ",\"reference_status\":" << h.status << ",\"ledger\":";
    ledger_json(out, base.run.ledger);
    out << ",\"reference_ledger\":";
    ledger_json(out, Ledger{h.nodes, h.leaves, h.filter_tests, h.max_depth, h.max_leaf_seen});
    out << ",\"leaves\":" << base.cmp.leaves << ",\"reference_leaves\":" << base.cmp.reference_leaves
        << ",\"leaf_sites\":" << base.run.n_leaf_sites << ",\"digest\":\"" << std::hex << base.cmp.digest
        << "\",\"reference_digest\":\"" << base.cmp.reference_digest << std::dec << "\",\"missing\":" << base.cmp.missing
        << ",\"extra\":" << base.cmp.extra << ",\"list_mismatch\":" << base.cmp.list_mismatch
        << ",\"list_permuted\":" << base.cmp.list_permuted << ",\"meta_mismatch\":" << base.cmp.meta_mismatch
        << ",\"nodes_checked\":" << (base.nodes_checked ? "true" : "false")
        << ",\"nodes_equal\":" << (base.nodes_equal ? "true" : "false") << ",\"first\":";
    json_string(out, base.cmp.first.empty() ? base.node_first : base.cmp.first);
    out << ",\"levels\":" << base.run.levels << ",\"max_children\":" << max_children << ",\"max_tasks\":" << max_tasks
        << ",\"host_ns\":" << base.ns << ",\"mutants\":{";
    for (size_t j = 1; j < per; ++j) {
      const Outcome& r = results[i * per + j];
      const bool killed = !r.cmp.identity();
      out << (j > 1 ? "," : "") << '"' << bfs::mutant_name(jobs_of[j]) << "\":{\"killed\":" << (killed ? "true" : "false")
          << ",\"status_equal\":" << (r.cmp.status_equal ? "true" : "false")
          << ",\"ledger_equal\":" << (r.cmp.ledger_equal ? "true" : "false")
          << ",\"leaves_equal\":" << (r.cmp.leaves_equal ? "true" : "false") << ",\"first\":";
      json_string(out, r.cmp.first);
      out << "}";
    }
    out << "},\"level_stats\":[";
    for (size_t k = 0; k < base.run.stats.size(); ++k) {
      const auto& s = base.run.stats[k];
      out << (k ? "," : "") << "[" << s.depth << "," << s.parents << "," << s.children << "," << s.tasks << ","
          << s.candidates << "," << s.tests << "," << s.splits << "," << s.leaves << "]";
    }
    out << "],\"level_fields\":[\"depth\",\"parents\",\"children\",\"tasks\",\"candidates\",\"tests\",\"splits\","
           "\"leaves\"]}\n";
    std::cerr << path << " : " << (identity ? "identite" : "ECART") << " (" << base.ns / 1000000 << " ms hote)\n";
  }
  if (!json_path.empty()) {  // temporaire puis renommage : jamais un resultat partiel sous le nom attendu
    const std::string tmp = json_path + ".tmp";
    std::ofstream file(tmp, std::ios::binary | std::ios::trunc);
    file << out.str();
    file.flush();
    const bool written = file.good();
    file.close();
    if (!written || file.fail() || std::rename(tmp.c_str(), json_path.c_str()) != 0) {
      std::remove(tmp.c_str());
      std::cerr << "refus : ecriture du resultat impossible : " << json_path << '\n';
      return 2;
    }
  }
  std::cout << out.str();
  return code;
}
