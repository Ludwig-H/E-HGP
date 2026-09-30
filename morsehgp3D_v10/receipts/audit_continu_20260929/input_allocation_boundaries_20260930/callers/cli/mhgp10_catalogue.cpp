// Sonde du generateur : lit un nuage u32le (x y z par point) et produit le catalogue critique.
//
//   mhgp10_catalogue IN.u32le --k=K [--threads=W] [--leaf=M] [--max-leaf=L] [--max-nodes=N] [--allow-small-leaf]
//                    [--dump=FILE]
// Sortie standard : une ligne JSON (comptes par (q_min, p), grand-livre, temps mural).
// --dump : une ligne par boule, ordre canonique : rang q p u flags | S* (coordonnees) | I | U.
// --leaf : M = 0 (table M(K) par defaut) ou M >= K + 3 ; --allow-small-leaf (diagnostic d'audit) admet K <= M < K + 3
// avec un budget --max-nodes=N > 0 ; M < K toujours refuse ; M > --max-leaf (defaut 256, au plus kMaxLeafBound = 1024)
// refuse avant calcul. --max-nodes : budget de noeuds de l'arbre des boites (0 : aucun), refus node_budget au-dela
// (build_catalogue, check_catalogue_params).
// Options numeriques lues par le parseur strict commun (core/cli_options.hpp) : jeton entier, chiffres decimaux, borne
// du type cible verifiee avant conversion ; --threads dans [0, 1024] ; --dump= vide refuse (raccord R2 : il valait
// « pas de dump » en silence). Parametres du generateur juges avant la lecture. Entree lue en entier ou refusee avant
// calcul (cloud/u32le_input.hpp). Dump (core/cli_output.hpp) : declare puis reserve avant le calcul et avant le pool
// (un temporaire dans son dossier ; refus output_unwritable, ou output_conflict s'il designe l'entree), ecrit en
// entier, publie par renommage au commit seulement ; un refus ne touche jamais un fichier preexistant. La ligne
// status=ok n'est ecrite qu'apres le dump. Un refus ecrit une ligne JSON (statut, raison).
// Codes : 0 conforme, 2 refus.
#include <chrono>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "catalogue/catalogue.hpp"
#include "cloud/u32le_input.hpp"
#include "core/cli_options.hpp"
#include "core/cli_output.hpp"

using namespace mhgp10;
using cli::print_refusal;

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: mhgp10_catalogue IN.u32le --k=K [--threads=W] [--leaf=M] [--max-leaf=L] "
                         "[--max-nodes=N] [--allow-small-leaf] [--dump=FILE]\n");
    return print_refusal(fail(Reason::parameter_out_of_range));
  }
  CatalogueParams params;
  unsigned threads = 0;
  std::string dump;
  for (int i = 2; i < argc; ++i) {
    const std::string_view a = argv[i];
    std::string_view v;
    Outcome bad;
    if (cli::option_value(a, "--k", v)) {
      const auto r = cli::parse_integer<int>(v, 0, std::numeric_limits<int>::max());  // domaine : build_catalogue
      if (r.ok()) params.kmax = r.value();
      bad = r.outcome();
    } else if (cli::option_value(a, "--threads", v)) {
      const auto r = cli::parse_integer<unsigned>(v, 0, cli::kMaxThreads);
      if (r.ok()) threads = r.value();
      bad = r.outcome();
    } else if (cli::option_value(a, "--leaf", v)) {
      const auto r = cli::parse_integer<u32>(v, 0, std::numeric_limits<u32>::max());
      if (r.ok()) params.leaf_size = r.value();
      bad = r.outcome();
    } else if (cli::option_value(a, "--max-leaf", v)) {
      const auto r = cli::parse_integer<u32>(v, 0, std::numeric_limits<u32>::max());
      if (r.ok()) params.max_leaf = r.value();
      bad = r.outcome();
    } else if (cli::option_value(a, "--max-nodes", v)) {
      const auto r = cli::parse_integer<u64>(v, 0, std::numeric_limits<u64>::max());
      if (r.ok()) params.max_nodes = r.value();
      bad = r.outcome();
    } else if (a == "--allow-small-leaf") {
      params.allow_small_leaf = true;
    } else if (cli::option_value(a, "--dump", v)) {
      if (v.empty()) bad = fail(Reason::parameter_out_of_range);  // --dump= vide : jamais « pas de dump » en silence
      dump = std::string(v);
    } else {
      std::fprintf(stderr, "option inconnue %s\n", argv[i]);
      bad = fail(Reason::parameter_out_of_range);
    }
    if (!bad.ok()) return print_refusal(bad);
  }
  // parametres du generateur juges avant toute lecture (feuille, budget, max_leaf)
  if (const Outcome o = check_catalogue_params(params); !o.ok()) return print_refusal(o);
  auto input = read_u32le_cloud(argv[1]);
  if (!input.ok()) return print_refusal(input.outcome());
  const U32leCloud& in = input.value();
  const u32 n = static_cast<u32>(in.x.size());  // < kNone : garde du lecteur
  const auto t0 = std::chrono::steady_clock::now();
  auto prepared = prepare_cloud(in.x, in.y, in.z, in.pid, kCoordinateBits);
  if (!prepared.ok()) return print_refusal(prepared.outcome());
  const Cloud& cloud = prepared.value();
  // dump declare (conflit avec l'entree refuse) puis reserve, avant le pool et le calcul ; sans commit, aucun fichier
  // preexistant n'est touche et le temporaire est retire
  cli::OutputSet outputs;
  if (!dump.empty()) {
    if (const Outcome o = outputs.add_input(argv[1]); !o.ok()) return print_refusal(o);
    if (const Outcome o = outputs.declare(dump); !o.ok()) return print_refusal(o);
    if (const Outcome o = outputs.reserve(); !o.ok()) return print_refusal(o);
  }
  // Fils du pool (raccord R2) : une creation impossible (fil refuse par le systeme, allocation) est refusee
  // resource_exhausted/session_overhead avant tout calcul et toute sortie ; impression sans allocation.
  auto made = sched::make_pool(threads);
  if (!made.ok()) return print_refusal(made.outcome());
  sched::Pool& pool = *made.value();
  const auto t1 = std::chrono::steady_clock::now();
  auto built = build_catalogue(cloud, params, pool);
  const auto t2 = std::chrono::steady_clock::now();
  if (!built.ok()) return print_refusal(built.outcome());
  const Catalogue& cat = built.value();
  if (!dump.empty()) {
    const Outcome written = outputs.write(dump, [&](std::FILE* o) {
      auto pt = [&](u32 s) { std::fprintf(o, " %u,%u,%u", cloud.x[s], cloud.y[s], cloud.z[s]); };
      for (u32 b = 0; b < cat.balls(); ++b) {
        std::fprintf(o, "%u %u %u %u %u |", cat.rank[b], cat.qmin[b], cat.p[b], cat.u[b], cat.flags[b]);
        for (u32 s : cat.support[b])
          if (s != kNone) pt(s);
        std::fprintf(o, " |");
        for (u32 s : cat.interior(b)) pt(s);
        std::fprintf(o, " |");
        for (u32 s : cat.shell(b)) pt(s);
        std::fprintf(o, "\n");
      }
    });
    if (!written.ok()) return print_refusal(written);
  }
  if (const Outcome o = outputs.commit(); !o.ok()) return print_refusal(o);
  std::vector<std::vector<u64>> by(5, std::vector<u64>(kMaxCatalogueOrder + 1, 0));
  for (u32 b = 0; b < cat.balls(); ++b)
    if (cat.p[b] <= u32(kMaxCatalogueOrder)) ++by[cat.qmin[b]][cat.p[b]];
  const auto& L = cat.ledger;
  std::printf("{\"status\":\"ok\",\"n\":%u,\"sites\":%u,\"K\":%d,\"threads\":%u,\"balls\":%u,\"levels\":%zu,", n,
              cloud.sites(), params.kmax, pool.size(), cat.balls(), cat.level.size());
  std::printf("\"prepare_s\":%.4f,\"catalogue_s\":%.4f,", std::chrono::duration<double>(t1 - t0).count(),
              std::chrono::duration<double>(t2 - t1).count());
  std::printf("\"catalogue_stages\":{\"t_frontier\":%.4f,\"t_boxes\":%.4f,\"t_order\":%.4f,\"t_assemble\":%.4f,"
              "\"t_collect\":%.4f,\"t_sort\":%.4f,\"t_bands\":%.4f,\"t_compare\":%.4f,\"t_ranks\":%.4f,\"t_copy\":%.4f,"
              "\"bands\":%llu,\"band_members\":%llu,\"tasks\":%llu,\"max_task_sites\":%llu},\"by_q_p\":{",
              cat.t_frontier, cat.t_boxes, cat.t_order, cat.t_assemble, cat.t_collect, cat.t_sort, cat.t_bands,
              cat.t_compare, cat.t_ranks, cat.t_copy, (unsigned long long)cat.bands, (unsigned long long)cat.band_members,
              (unsigned long long)cat.tasks, (unsigned long long)cat.max_task_sites);
  for (int q = 2; q <= 4; ++q) {
    std::printf("%s\"q%d\":[", q > 2 ? "," : "", q);
    for (int p = 0; p < params.kmax; ++p) std::printf("%s%llu", p ? "," : "", static_cast<unsigned long long>(by[q][p]));
    std::printf("]");
  }
  std::printf("},\"nodes\":%llu,\"leaves\":%llu,\"sum_m\":%llu,\"max_m\":%llu,\"skipped_bbox\":%llu,"
              "\"filter_tests\":%llu,\"leaf_dominance_tests\":%llu,\"pair_tests\":%llu,"
              "\"triple_tests\":%llu,\"line_hits\":%llu,\"quad_tests\":%llu,\"judged\":%llu,\"extended\":%llu,"
              "\"weighted\":%llu,\"max_shell\":%llu,\"stalled_leaves\":%llu}\n",
              (unsigned long long)L.nodes, (unsigned long long)L.leaves, (unsigned long long)L.sum_m,
              (unsigned long long)L.max_m, (unsigned long long)L.skipped_bbox,
              (unsigned long long)L.filter_tests, (unsigned long long)L.leaf_dominance_tests,
              (unsigned long long)L.pair_tests, (unsigned long long)L.triple_tests, (unsigned long long)L.line_hits,
              (unsigned long long)L.quad_tests, (unsigned long long)L.judged, (unsigned long long)L.extended,
              (unsigned long long)L.weighted, (unsigned long long)L.max_shell, (unsigned long long)L.stalled_leaves);
  return cli::finish_stdout(0);
}
