// MorseHGP3D v6 — pilote en ligne de commande du pipeline (src/pipeline/run.hpp).
// Codes : 0 complete_regular, 2 refus transactionnel (avant OU pendant le
// calcul — jamais un prefixe publie), 3 invariant viole.
// Binaire PRODUIT : compile sans MHGP7_TESTING — aucun mutant n'y existe
// (--inject= y est un argument inconnu, code 2 : porte mhgp7_profile_refuse_inject) ;
// la cible de sonde mhgp7_profile_sonde (MHGP7_TESTING) accepte --inject=ablation-*.
// PARSING EXACT DE TOUTES LES OPTIONS (docs/PROVENANCE.md, dette v5 fermee) :
// suffixe, vide, signe explicite ou debordement ⟹ code 2, jamais un atoi
// tolerant qui selectionnerait silencieusement un autre regime.
#include <sys/resource.h>

#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>

#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/io/archive.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp7;

namespace {

bool parse_int_range(const char* text, long long lo, long long hi, long long* out) {
  i64 v = 0;
  if (!parse_i64_exact(text, &v)) return false;
  if ((long long)v < lo || (long long)v > hi) return false;
  *out = (long long)v;
  return true;
}

#ifdef MHGP7_TESTING
// ALLOWLIST de la cible de sonde (mhgp7_profile_sonde) : exactement les trois
// ablations du reduce, separees par des virgules, aucun item vide (`--inject=`
// et `--inject=,` refuses), jamais un mutant de production (render-active-only,
// csr-*, ...) meme s'il figure au registre kMutants.
bool sonde_inject_allowed(const char* csv) {
  static constexpr const char* kAllowed[] = {"ablation-mat-sans-copie", "ablation-mat-sans-tris",
                                             "ablation-post-cle-factice"};
  const std::string_view s = csv;
  if (s.empty()) return false;
  size_t start = 0;
  while (true) {
    const size_t comma = s.find(',', start);
    const std::string_view item = s.substr(start, comma == std::string_view::npos ? std::string_view::npos : comma - start);
    bool known = false;
    for (const char* a : kAllowed) known = known || item == a;
    if (item.empty() || !known) return false;
    if (comma == std::string_view::npos) return true;
    start = comma + 1;
  }
}
#endif

}  // namespace

int run_cli(int argc, char** argv) {
  const auto t_begin = std::chrono::steady_clock::now();
  CloudFamily family = CloudFamily::kUniform;
  bool ok = true;
  bool generated_option = false;
  bool silent_cap_requested = false;
  std::string input_path, output_path;
  long long n = 8000, coord = 0, seed = 3;
  RunOptions opt;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return arg.compare(0, l, prefix) == 0 ? arg.c_str() + l : nullptr;
    };
    long long v = 0;
    i64 vi = 0;
    if (arg == "--help") {
      std::puts("MorseHGP3D v7 (public_status=not_claimed)\n"
                "  --input=FILE       exact text rows: PointId x y z (u32/u16/u16/u16)\n"
                "  --output=DIR       create-only archive, committed on terminal success\n"
                "  --family=NAME --n=N --seed=S  synthetic input (exclusive with --input)\n"
                "  --smax=2..11 --threads=N --layout=classic|csr --digest\n"
                "  --complete-incidences   candidate normalized horizontal H0, regular inputs only\n"
                "  --silent-core-records=N --silent-chain-steps=N --silent-cofaces=N\n"
                "  --silent-query-nodes=N --silent-meb-supports=N  hard work caps (zero rejects)\n"
                "  --require-exact    refuses while global exact qualification is incomplete");
      return 0;
    } else if (const char* s = val("--input=")) {
      ok = input_path.empty() && *s && ok;
      input_path = s;
    } else if (const char* s = val("--output=")) {
      ok = output_path.empty() && *s && ok;
      output_path = s;
    } else if (arg == "--complete-incidences") {
      opt.complete_silent_incidence = true;
    } else if (arg.starts_with("--silent-")) {
      silent_cap_requested = true;
      const size_t equal = arg.find('=');
      const std::string key = arg.substr(0, equal);
      u64* cap = key == "--silent-core-records" ? &opt.silent_limits.max_core_records
          : key == "--silent-chain-steps" ? &opt.silent_limits.max_chain_steps
          : key == "--silent-cofaces" ? &opt.silent_limits.max_added_cofaces
          : key == "--silent-query-nodes" ? &opt.silent_limits.max_query_nodes
          : key == "--silent-meb-supports" ? &opt.silent_limits.max_meb_supports : nullptr;
      if (!cap || equal == std::string::npos || !parse_i64_exact(arg.c_str() + equal + 1, &vi) || vi < 0)
        ok = false;
      else *cap = (u64)vi;
    } else if (arg == "--require-exact") {
      std::fprintf(stderr, "REFUS exact_hgp_not_qualified : normalized horizontal H0, vertical maps and scale contracts require independent qualification\n");
      return 2;
    } else if (const char* s = val("--family=")) {
      generated_option = true;
      ok = parse_cloud_family(s, &family) && ok;
    } else if (const char* s = val("--n=")) {
      generated_option = true;
      // Plafond de l'arbre radix (caps.hpp) applique DES l'analyse : la
      // famille est construite avant run_pipeline, la garde doit preceder.
      ok = parse_int_range(s, 2, (int)kMaxTreePositions, &n) && ok;
    } else if (const char* s = val("--mem-budget=")) {
      // Budget memoire DECLARE (octets) : refus resource_exhausted aux
      // residences dominantes (caps.hpp) — 0 = desactive.
      ok = parse_i64_exact(s, &vi) && vi >= 0 && ok;
      opt.memory_budget_bytes = (u64)vi;
    } else if (const char* s = val("--coord=")) {
      generated_option = true;
      ok = parse_int_range(s, 1, 65536, &coord) && ok;
    } else if (const char* s = val("--seed=")) {
      generated_option = true;
      ok = parse_i64_exact(s, &vi) && ok;
      seed = (long long)vi;
    } else if (const char* s = val("--s=")) {
      ok = parse_i64_exact(s, &opt.s) && ok;
    } else if (const char* s = val("--smax=")) {
      ok = parse_int_range(s, 2, 11, &v) && ok;
      opt.smax = (u64)v;
    } else if (const char* s = val("--threads=")) {
      ok = parse_int_range(s, 1, 1024, &v) && ok;
      opt.threads = (int)v;
    } else if (const char* s = val("--fold-inflight=")) {
      ok = parse_int_range(s, 1, kFoldInflightMax, &v) && ok;
      opt.fold_inflight = (int)v;
    } else if (const char* s = val("--fold-join=")) {
      // Mode diagnostic § 5.10 : B(K) joint avant A(K+1) — objet identique,
      // ordonnancement seul change (isolation de l'etage B pour le profil).
      ok = parse_int_range(s, 0, 1, &v) && ok;
      opt.fold_join_before_next_k = v != 0;
    } else if (const char* s = val("--layout=")) {
      // Route de STOCKAGE des deltas (palier KeyCSR) : classic (defaut) | csr ;
      // valeur inconnue ou vide = refus 2. Option PRODUIT (hors MHGP7_TESTING).
      ok = parse_forest_layout(s, &opt.forest_layout) && ok;
    } else if (const char* s = val("--pretest-query-min=")) {
      ok = parse_int_range(s, 0, std::numeric_limits<long long>::max(), &v) && ok;
      opt.pretest_query_min_points = (size_t)v;
    } else if (const char* s = val("--cell-min-sites=")) {
      ok = parse_int_range(s, 0, std::numeric_limits<long long>::max(), &v) && ok;
      opt.cell_grid_min_sites = (size_t)v;
    } else if (const char* s = val("--shell-cap=")) {
      ok = parse_int_range(s, 4, 12, &v) && ok;
      opt.shell_cap = (size_t)v;
    } else if (arg == "--sonde-e6") {
      opt.e6_probe = true;
    } else if (const char* s = val("--e3-g16=")) {
      const std::string b = s;
      if (b == "g8_lourdes") opt.e3_mode = E3G16Mode::kG8Lourdes;
      else if (b == "g16_politique") opt.e3_mode = E3G16Mode::kG16Politique;
      else if (b == "g16_nearm") opt.e3_mode = E3G16Mode::kG16NearM;
      else if (b == "g16_ratio") opt.e3_mode = E3G16Mode::kG16Ratio;
      else if (b == "g16_leve") opt.e3_mode = E3G16Mode::kG16Leve;
      else ok = false;
    } else if (arg == "--e6-grille") {
      // Alias historique du bras complet (recus du 31 aout).
      opt.e3_mode = E3G16Mode::kG16Leve;
    } else if (arg == "--digest") {
      opt.digest = true;
#ifdef MHGP7_TESTING
    } else if (const char* s = val("--inject=")) {
      // Cible de SONDE seulement (mhgp7_profile_sonde) : ALLOWLIST des trois
      // ablations du reduce ; vide, item vide ou nom hors allowlist => refus 2.
      // Le binaire produit ne connait pas l'option.
      ok = sonde_inject_allowed(s) && mutants_enable(s) && ok;
#endif
    } else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      ok = false;
    }
  }
  if (!ok || opt.s < kSeparationProfileMin || (!input_path.empty() && generated_option) ||
      (silent_cap_requested && !opt.complete_silent_incidence)) {
    std::fprintf(stderr, "REFUS : arguments (parsing exact ; profil s >= 8, smax dans [2, 11])\n");
    return 2;
  }
  if (coord <= 0) coord = cloud_family_default_coord(family, (int)n);
  const std::vector<InputPoint> in = input_path.empty()
      ? make_family_input(family, (int)n, (int)coord, seed) : read_u16_text(input_path);
  if (in.empty()) {
    std::fprintf(stderr, "REFUS : famille vide (parametres hors contrat de la famille)\n");
    return 2;
  }
  std::unique_ptr<ForestArchive> archive;
  if (!output_path.empty()) {
    archive = std::make_unique<ForestArchive>(output_path);
    archive->input(in);
    opt.digest = true;
    opt.on_forest = [&](u64 k, const std::vector<ForestEvent>&, const ForestResult& forest) {
      archive->forest(k, forest);
    };
  }
  const RunResult rr = run_pipeline(in, opt);
  if (rr.status != PipelineStatus::kCompleteRegular) {
    // FORMAT DU REFUS INCHANGE (ligne `REFUS <message>`), puis le DIAGNOSTIC
    // D'ETAGE (alerte G4 du 2 septembre) : l'etage atteint et les RSS par
    // etage, sur stderr, sans rien publier d'autre — aucun payload, aucun
    // digest, aucune cardinalite ne sort d'un refus.
    std::fprintf(stderr, "REFUS %s\n", rr.message.c_str());
    std::fprintf(stderr,
                 "refus_etage=%s rss_mb apres_generation=%.0f apres_rle=%.0f apres_prefiltre=%.0f "
                 "apres_census=%.0f max_fold=%.0f (frontiere de completion : dernier etage atteint)\n",
                 run_stage_name(rr.stage_reached), rr.rss_mb[0], rr.rss_mb[1], rr.rss_mb[2], rr.rss_mb[3],
                 rr.rss_mb[4]);
    if (opt.complete_silent_incidence) {
      std::fprintf(stderr, "silent_refusal_work census_balls=%llu plateau_balls=%llu total_ms=%.3f completion_ms=%.3f\n",
                   (unsigned long long)rr.census_balls, (unsigned long long)rr.plateau_balls,
                   rr.t_total_ms, rr.t_silent_ms);
      for (size_t k = 2; k < rr.silent_stats.size(); ++k) {
        const auto& s = rr.silent_stats[k];
        if (!s.core_records && !s.query_nodes && !s.meb_supports) continue;
        std::fprintf(stderr, "silent_refusal_K%zu core=%llu steps=%llu added_provisional=%llu query_nodes=%llu meb_supports=%llu\n",
                     k, (unsigned long long)s.core_facets, (unsigned long long)s.chain_steps,
                     (unsigned long long)s.added_cofaces, (unsigned long long)s.query_nodes,
                     (unsigned long long)s.meb_supports);
      }
    }
    return status_exit_code(rr.status);
  }
  if (archive) {
    archive->commit(rr.kmax_eff, opt.complete_silent_incidence ? "normalized_horizontal_h0_candidate" : "verified_events_only", rr.digest_all);
    if (!archive->parent_sync_confirmed()) std::fprintf(stderr, "archive_directory_sync=unconfirmed\n");
  }
  if (opt.memory_budget_bytes != 0)
    std::printf(
        "memory_budget_scope=partial_named_payload_proxy_v1 budget=%llu cap_brut_demande=%llu cap_brut_effectif=%llu cap_fusion_budgetaire=%llu\n",
        (unsigned long long)opt.memory_budget_bytes, (unsigned long long)opt.max_raw_candidates,
        (unsigned long long)effective_raw_cap(opt), (unsigned long long)budget_fusion_cap(opt));
  print_run(stdout, input_path.empty() ? cloud_family_name(family) : "input_u16", (int)in.size(),
            input_path.empty() ? (int)coord : 65536, seed, opt, rr);
  // Pic de residence MESURE (RSS max du processus), jamais un pic annonce.
  struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru) == 0) std::printf("rss_max_kb=%ld\n", ru.ru_maxrss);
  std::printf("cli_e2e_ms=%.3f includes_input_and_export=true archive_committed=%s\n",
              run_detail::ms(t_begin), archive ? "true" : "false");
  return 0;
}

int main(int argc, char** argv) {
  try {
    return run_cli(argc, argv);
  } catch (const std::bad_alloc&) {
    std::fprintf(stderr, "REFUS resource_exhausted : input, pipeline or archive allocation\n");
    return 2;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "REFUS %s\n", error.what());
    return 2;
  }
}
