// MorseHGP3D v9 — sonde de la tour FULL de bout en bout sur un nuage fichier.
//
//   mhgp9_tower_probe <fichier .u32le|.u16le> K workers [--s=8] [--static=T]
//                     [--no-tower] [--n=prefixe] [--grid=libelle]
//                     [--catalogue-digest] [--certificate-capacity=N]
//                     [--certificate-judge] [--lanes-capacity=N]
//                     [--lanes-judge] [--lanes-events=N] [--lever=NOM=0|1 ...]
//
// Leviers (meme objet, travail different) : atlas_saturate_deep,
// q3_leaf_census, q34_dead_lanes, q34_witness_cache, q34_dead_core (ce dernier
// exige q34_dead_lanes, sinon la chaine refuse), tower_meb_proposal (MEB de la
// tour propose puis verifie exactement), q34_jobs_by_mass et q34_fine_jobs
// (ordonnancement des jobs du front q3/q4 : ordre par masse, grain fin),
// tower_overlap_static (phase A de la tour recouvrant la phase 0),
// q2_jobs_by_mass (plan de jobs q2 par masse, 64 jobs par fil),
// q34_batch_filter (filtre temoin q3/q4 par lots : front, puis un appel pour
// tous les rectangles et paires sans cache, puis les survivants) et
// q34_gpu_filter (cet appel sur le GPU, exige q34_batch_filter ; sans GPU la
// chaine refuse), q34_batch_certificates (certificats de voie morte de tous
// les survivants en un appel, exige q34_batch_filter et q34_dead_lanes) et
// q34_gpu_certificates (cet appel sur le GPU), q34_batch_q3 (v20 S4a : voie
// q3 des survivants certifies en un appel, sans atlas, exige
// q34_batch_certificates) et q34_gpu_q3 (cet appel sur le GPU pendant les
// voies q4 des ouvriers, exige q34_batch_q3), q34_batch_q4 (v21 S4b : la voie
// q4 dans le meme appel, sans atlas, exige q34_batch_q3). Tous sont publies dans
// options.levers ; un plan G4 les epingle explicitement, un nom inconnu est
// refuse (code 2).
//
// Chronometre du contrat : du nuage prepare en memoire a la tour complete en
// memoire (ChainTimes, sans la lecture). La lecture et son empreinte sont
// mesurees a part, de meme que le condense de verification de la tour
// (times_ms.digest, apres chain_total). Sortie : un objet JSON sur stdout.
// Code 0 conforme, 2 refus
// d'arguments ou d'entree, 3 statut de chaine non complet.
#include <sys/resource.h>

#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../src/chain/tower_chain.hpp"

namespace {

using mhgp9::gen::Point3;

struct Input {
  std::vector<Point3> points;
  std::uint64_t hash = 14695981039346656037ull;  // FNV-1a 64 de n puis x, y, z (u64 LE)
  std::string format;
};

void fnv_word(std::uint64_t& h, std::uint64_t w) {
  for (int i = 0; i < 8; ++i) {
    h ^= (w >> (8 * i)) & 0xffu;
    h *= 1099511628211ull;
  }
}

Input read_points(const std::string& path, std::size_t prefix) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::invalid_argument("cannot open input");
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  Input out;
  std::size_t width = 0;
  if (path.ends_with(".u32le")) { width = 4; out.format = "u32le"; }
  else if (path.ends_with(".u16le")) { width = 2; out.format = "u16le"; }
  else throw std::invalid_argument("input must be .u32le or .u16le");
  if (bytes.size() % (3 * width) != 0) throw std::invalid_argument("input length is not a multiple of a site record");
  std::size_t n = bytes.size() / (3 * width);
  if (prefix) {
    if (prefix > n) throw std::invalid_argument("prefix exceeds input size");
    n = prefix;
  }
  out.points.resize(n);
  fnv_word(out.hash, n);
  for (std::size_t i = 0; i < n; ++i) {
    std::uint32_t c[3];
    for (int a = 0; a < 3; ++a) {
      std::uint32_t v = 0;
      for (std::size_t b = 0; b < width; ++b) v |= static_cast<std::uint32_t>(bytes[(3 * i + a) * width + b]) << (8 * b);
      if (v > 262143u) throw std::invalid_argument("coordinate outside [0, 2^18)");
      c[a] = v;
      fnv_word(out.hash, v);
    }
    out.points[i] = Point3{static_cast<mhgp9::gen::Coordinate>(c[0]), static_cast<mhgp9::gen::Coordinate>(c[1]),
                           static_cast<mhgp9::gen::Coordinate>(c[2])};
  }
  return out;
}

unsigned long long parse_u(std::string_view s) {
  if (s.empty()) throw std::invalid_argument("empty number");
  unsigned long long v = 0;
  if (s.size() > 18) throw std::invalid_argument("number too long");
  for (char ch : s) {
    if (ch < '0' || ch > '9') throw std::invalid_argument("not a number");
    v = v * 10 + static_cast<unsigned>(ch - '0');
  }
  return v;
}

long peak_rss_kb() {
  rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) != 0) return -1;
  return usage.ru_maxrss;
}

}  // namespace

int main(int argc, char** argv) {
  mhgp9::ChainOptions options;
  std::string path, grid = "unspecified";
  std::size_t prefix = 0;
  try {
    if (argc < 4) throw std::invalid_argument("usage: mhgp9_tower_probe file K workers [options]");
    path = argv[1];
    const auto k = parse_u(argv[2]);
    if (k < 1 || k > 10) throw std::invalid_argument("K must be in 1..10");
    options.kmax = static_cast<unsigned>(k);
    const auto workers = parse_u(argv[3]);
    if (workers < 1 || workers > 4096) throw std::invalid_argument("workers must be in 1..4096");
    options.workers = static_cast<std::size_t>(workers);
    for (int i = 4; i < argc; ++i) {
      const std::string_view arg(argv[i]);
      if (arg.starts_with("--s=")) {
        const auto s = parse_u(arg.substr(4));
        if (s < 8 || s > 64) throw std::invalid_argument("s must be in 8..64 (never below 8)");
        options.separation_s = static_cast<unsigned>(s);
      } else if (arg.starts_with("--static=")) {
        const auto t = parse_u(arg.substr(9));
        if (t > 4096) throw std::invalid_argument("static threads must be in 0..4096");
        options.tower_static_threads = static_cast<int>(t);
      }
      else if (arg == "--no-tower") options.run_tower = false;
      else if (arg == "--catalogue-digest") options.catalogue_digest = true;
      else if (arg == "--certificate-judge") options.q34_certificate_judge = true;
      else if (arg == "--lanes-judge") options.q34_lanes_judge = true;
      else if (arg.starts_with("--lanes-capacity=")) {
        const auto capacity = parse_u(arg.substr(17));
        if (capacity < 2 || capacity > (1ULL << 20)) throw std::invalid_argument("lanes capacity outside 2..2^20");
        options.q34_lanes_capacity = static_cast<std::uint32_t>(capacity);
      }
      else if (arg.starts_with("--lanes-events=")) {
        const auto events = parse_u(arg.substr(15));
        if (events < 1 || events > (1ULL << 20)) throw std::invalid_argument("lanes events outside 1..2^20");
        options.q34_lanes_events = static_cast<std::uint32_t>(events);
      }
      else if (arg.starts_with("--certificate-capacity=")) {
        const auto capacity = parse_u(arg.substr(23));
        if (capacity < 2 || capacity > 0xffffffffULL) throw std::invalid_argument("certificate capacity outside 2..2^32-1");
        options.q34_certificate_capacity = static_cast<std::uint32_t>(capacity);
      }
      else if (arg.starts_with("--lever=")) {
        const auto spec = arg.substr(8);
        const auto eq = spec.find('=');
        if (eq == std::string_view::npos || (spec.substr(eq + 1) != "0" && spec.substr(eq + 1) != "1"))
          throw std::invalid_argument("lever must be --lever=NAME=0|1");
        const bool on = spec.substr(eq + 1) == "1";
        const auto name = spec.substr(0, eq);
        if (name == "atlas_saturate_deep") options.atlas_saturate_deep = on;
        else if (name == "q3_leaf_census") options.q3_leaf_census = on;
        else if (name == "q34_dead_lanes") options.q34_dead_lanes = on;
        else if (name == "q34_witness_cache") options.q34_witness_cache = on;
        else if (name == "q34_dead_core") options.q34_dead_core = on;
        else if (name == "tower_meb_proposal") options.tower_meb_proposal = on;
        else if (name == "q34_jobs_by_mass") options.q34_jobs_by_mass = on;
        else if (name == "q34_fine_jobs") options.q34_fine_jobs = on;
        else if (name == "tower_overlap_static") options.tower_overlap_static = on;
        else if (name == "q2_jobs_by_mass") options.q2_jobs_by_mass = on;
        else if (name == "q34_batch_filter") options.q34_batch_filter = on;
        else if (name == "q34_gpu_filter") options.q34_gpu_filter = on;
        else if (name == "q34_batch_certificates") options.q34_batch_certificates = on;
        else if (name == "q34_gpu_certificates") options.q34_gpu_certificates = on;
        else if (name == "q34_batch_q3") options.q34_batch_q3 = on;
        else if (name == "q34_gpu_q3") options.q34_gpu_q3 = on;
        else if (name == "q34_batch_q4") options.q34_batch_q4 = on;
        else throw std::invalid_argument("unknown lever");
      }
      else if (arg.starts_with("--n=")) prefix = static_cast<std::size_t>(parse_u(arg.substr(4)));
      else if (arg.starts_with("--grid=")) {
        grid = std::string(arg.substr(7));
        // Libelle injecte tel quel dans le JSON : alphabet sur, sans echappement.
        if (grid.empty() || grid.size() > 32 ||
            grid.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-") != std::string::npos)
          throw std::invalid_argument("grid label must match [A-Za-z0-9_.-]{1,32}");
      }
      else throw std::invalid_argument("unknown option");
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "argument refusal: %s\n", e.what());
    return 2;
  }
  Input input;
  const auto read_start = std::chrono::steady_clock::now();
  try {
    input = read_points(path, prefix);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "input refusal: %s\n", e.what());
    return 2;
  }
  const double read_ms =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - read_start).count();
  const auto r = mhgp9::run_tower_chain(input.points, options);
  const auto& t = r.times;
  const auto& c = r.catalogue;
  std::printf("{\"schema\":\"mhgp9_tower_probe_v23\",\"status\":\"%s\",\"reason\":\"%s\",", mhgp9::chain_status_name(r.status),
              r.reason.c_str());
  std::printf("\"input\":{\"format\":\"%s\",\"grid\":\"%s\",\"sites\":%zu,\"hash\":\"%016" PRIx64 "\"},", input.format.c_str(),
              grid.c_str(), input.points.size(), input.hash);
  std::printf("\"options\":{\"K\":%u,\"K_effective\":%u,\"s\":%u,\"workers\":%zu,\"tower_static_threads\":%d,\"run_tower\":%s,"
              "\"certificate_capacity\":%u,\"certificate_judge\":%s,\"lanes_capacity\":%u,\"lanes_judge\":%s,\"lanes_events\":%u,"
              "\"levers\":{\"atlas_saturate_deep\":%s,\"q3_leaf_census\":%s,\"q34_dead_lanes\":%s,"
              "\"q34_witness_cache\":%s,\"q34_dead_core\":%s,\"tower_meb_proposal\":%s,"
              "\"q34_jobs_by_mass\":%s,\"q34_fine_jobs\":%s,\"tower_overlap_static\":%s,\"q2_jobs_by_mass\":%s,"
              "\"q34_batch_filter\":%s,\"q34_gpu_filter\":%s,\"q34_batch_certificates\":%s,"
              "\"q34_gpu_certificates\":%s,\"q34_batch_q3\":%s,\"q34_gpu_q3\":%s,\"q34_batch_q4\":%s}},",
              options.kmax, r.kmax_effective, options.separation_s, options.workers,
              options.tower_static_threads >= 0 ? options.tower_static_threads : r.tower_static_threads,
              options.run_tower ? "true" : "false", options.q34_certificate_capacity,
              options.q34_certificate_judge ? "true" : "false", options.q34_lanes_capacity,
              options.q34_lanes_judge ? "true" : "false", options.q34_lanes_events,
              options.atlas_saturate_deep ? "true" : "false",
              options.q3_leaf_census ? "true" : "false", options.q34_dead_lanes ? "true" : "false",
              options.q34_witness_cache ? "true" : "false", options.q34_dead_core ? "true" : "false",
              options.tower_meb_proposal ? "true" : "false", options.q34_jobs_by_mass ? "true" : "false",
              options.q34_fine_jobs ? "true" : "false", options.tower_overlap_static ? "true" : "false",
              options.q2_jobs_by_mass ? "true" : "false", options.q34_batch_filter ? "true" : "false",
              options.q34_gpu_filter ? "true" : "false", options.q34_batch_certificates ? "true" : "false",
              options.q34_gpu_certificates ? "true" : "false", options.q34_batch_q3 ? "true" : "false",
              options.q34_gpu_q3 ? "true" : "false", options.q34_batch_q4 ? "true" : "false");
  std::printf("\"times_ms\":{\"read\":%.3f,\"prepare\":%.3f,\"gen_index\":%.3f,\"q2\":%.3f,\"q34\":%.3f,\"merge\":%.3f,"
              "\"tower_index\":%.3f,\"census\":%.3f,\"tower\":%.3f,\"chain_total\":%.3f,\"digest\":%.3f,"
              "\"catalogue_digest\":%.3f},"
              "\"chain_cpu_s\":%.3f,",
              read_ms, t.prepare_ms, t.gen_index_ms, t.q2_ms, t.q34_ms, t.merge_ms, t.tower_index_ms, t.census_ms, t.tower_ms,
              t.total_ms, t.digest_ms, t.catalogue_digest_ms, t.cpu_s);
  std::printf("\"generator\":{\"q2_front_rectangles\":%" PRIu64 ",\"q2_candidate_pairs\":%" PRIu64 ",\"q2_accepted_pairs\":%" PRIu64
              ",\"q34_expanded_pairs\":%" PRIu64 ",\"q34_cover_builds\":%" PRIu64 ",\"q3_emitted\":%" PRIu64 ",\"q4_emitted\":%" PRIu64 "},",
              r.q2_front_rectangles, r.q2_candidate_pairs, r.q2_accepted_pairs, r.q34_expanded_pairs, r.q34_cover_builds,
              r.q3_emitted, r.q4_emitted);
  std::printf("\"catalogue\":{\"q2_presentations\":%" PRIu64 ",\"q3_presentations\":%" PRIu64 ",\"q4_presentations\":%" PRIu64
              ",\"unique_keys\":%" PRIu64 ",\"balls\":%" PRIu64 ",\"extra_shell_balls\":%" PRIu64 ",\"shell_over_12\":%" PRIu64
              ",\"max_shell\":%" PRIu64 ",\"max_interior\":%" PRIu64 ",\"census_nodes\":%" PRIu64 ",\"census_leaf_tests\":%" PRIu64
              ",\"bytes\":%" PRIu64 ",\"by_qmin\":[%" PRIu64 ",%" PRIu64 ",%" PRIu64 "],\"by_shell\":[",
              c.q2_presentations, c.q3_presentations, c.q4_presentations, c.unique_keys, c.balls, c.extra_shell_balls,
              c.shell_over_cap, c.max_shell, c.max_interior, c.census_nodes, c.census_leaf_tests, c.bytes, c.balls_by_qmin[2],
              c.balls_by_qmin[3], c.balls_by_qmin[4]);
  for (std::size_t s = 0; s < c.balls_by_shell.size(); ++s) std::printf("%s%" PRIu64, s ? "," : "", c.balls_by_shell[s]);
  // Invariant d'Euler (condition necessaire de completude du catalogue) :
  // sommes par ordre K = 1..Kmax, ordres verifiables 1..checkable_max_k.
  std::printf("],\"euler\":{\"status\":\"%s\",\"checkable_max_k\":%u,\"by_k\":[",
              mhgp9::euler_status_name(c.euler_status), c.euler_checkable_max_k);
  for (unsigned k = 1; k <= options.kmax; ++k) std::printf("%s%" PRId64, k > 1 ? "," : "", c.euler_by_k[k]);
  std::printf("]}},");
  {
    // Occupation mesuree des ouvriers q3/q4 et chronos par phase de la tour
    // (mesures de mur, jamais comparees entre executions).
    const auto& o = r.q34_occupancy;
    std::printf("\"q34_occupancy\":{\"started_workers\":%" PRIu64 ",\"jobs\":%" PRIu64 ",\"tasks_published\":%" PRIu64
                ",\"tasks_consumed\":%" PRIu64 ",\"task_waits\":%" PRIu64 ",\"wall_max_ms\":%.3f,\"wall_min_ms\":%.3f"
                ",\"cpu_sum_s\":%.3f,\"wait_sum_s\":%.3f,\"job_sum_s\":%.3f,\"max_job_ms\":%.3f},",
                o.started_workers, o.jobs, o.tasks_published, o.tasks_consumed, o.task_waits, o.wall_max_ms,
                o.wall_min_ms, o.cpu_sum_s, o.wait_sum_s, o.job_sum_s, o.max_job_ms);
    // v17 : phases du chemin q3/q4 par lots (used=false sur le chemin moteur,
    // tous les champs a zero). Le backend est "cpu" ou le nom de l'appareil
    // (alphabet d'un nom de GPU ; guillemets et controles remplaces).
    const auto& b = r.q34_batch;
    // v18 : appel des certificats (S3), backend vide et zeros sans le levier.
    const auto clean = [](std::string name) {
      for (auto& ch : name)
        if (ch == '"' || ch == '\\' || static_cast<unsigned char>(ch) < 0x20) ch = '\'';
      return name;
    };
    const std::string backend = clean(b.backend), certificate_backend = clean(b.certificate_backend),
                      lanes_backend = clean(b.lanes_backend);
    std::printf("\"q34_batch\":{\"used\":%s,\"backend\":\"%s\",\"front_ms\":%.3f,\"filter_ms\":%.3f,\"edges_ms\":%.3f"
                ",\"device_ms\":%.3f,\"rectangles\":%" PRIu64 ",\"survivors\":%" PRIu64
                ",\"certificate_backend\":\"%s\",\"certificate_ms\":%.3f,\"certificate_device_ms\":%.3f"
                ",\"deferred\":%" PRIu64 ",\"judged_edges\":%" PRIu64 ",\"rebuilt_covers\":%" PRIu64
                ",\"certificate_warps\":%u,\"filter_kernel_ms\":%.3f,\"filter_transfer_ms\":%.3f"
                ",\"certificate_kernel_ms\":%.3f,\"certificate_transfer_ms\":%.3f"
                ",\"lanes_backend\":\"%s\",\"lanes_ms\":%.3f,\"lanes_device_ms\":%.3f,\"lanes_kernel_ms\":%.3f"
                ",\"lanes_transfer_ms\":%.3f,\"lanes_wait_ms\":%.3f,\"tail_ms\":%.3f,\"lanes_asked\":%" PRIu64
                ",\"lanes_decided\":%" PRIu64 ",\"lanes_deferred\":%" PRIu64 ",\"lanes_records\":%" PRIu64
                ",\"lanes_judged\":%" PRIu64 ",\"lanes_warps\":%u,\"lanes_setup_ms\":%.3f"
                ",\"lanes_finish_ms\":%.3f,\"lanes_convert_ms\":%.3f,\"lanes_tasks\":%" PRIu64
                ",\"lanes_max_task_steps\":%" PRIu64 ",\"lanes_plan_ms\":%.3f,\"lanes_task_ms\":%.3f"
                ",\"lanes_compact_ms\":%.3f},",
                b.used ? "true" : "false", backend.c_str(), b.front_ms, b.filter_ms, b.edges_ms, b.device_ms,
                b.rectangles, b.survivors, certificate_backend.c_str(), b.certificate_ms, b.certificate_device_ms,
                b.deferred, b.judged_edges, b.rebuilt_covers, b.certificate_warps, b.filter_kernel_ms,
                b.filter_transfer_ms, b.certificate_kernel_ms, b.certificate_transfer_ms, lanes_backend.c_str(),
                b.lanes_ms, b.lanes_device_ms, b.lanes_kernel_ms, b.lanes_transfer_ms, b.lanes_wait_ms, b.tail_ms,
                b.lanes_asked, b.lanes_decided, b.lanes_deferred, b.lanes_records, b.lanes_judged, b.lanes_warps,
                b.lanes_setup_ms, b.lanes_finish_ms, b.lanes_convert_ms, b.lanes_tasks, b.lanes_max_task_steps,
                b.lanes_plan_ms, b.lanes_task_ms, b.lanes_compact_ms);
    const auto& tt = r.tower_times;
    std::printf("\"tower_phases_ms\":{\"validate\":%.3f,\"static\":%.3f,\"lots\":%.3f,\"populations\":%.3f,"
                "\"images\":%.3f,\"bank\":%.3f,\"encode\":%.3f",
                tt.validate_ms, tt.static_ms, tt.lots_ms, tt.populations_ms, tt.images_ms, tt.bank_ms, tt.encode_ms);
    const std::pair<const char*, const std::array<double, 11>*> per_k[] = {
        {"static_by_k", &tt.static_by_k}, {"lots_by_k", &tt.lots_by_k}, {"images_by_k", &tt.images_by_k},
        {"encode_by_k", &tt.encode_by_k}, {"order_by_k", &tt.order_by_k},
        // v22 (E0): phase-0 sub-timers of each order.
        {"static_collect_by_k", &tt.static_collect_by_k}, {"static_sort_by_k", &tt.static_sort_by_k},
        {"static_groups_by_k", &tt.static_groups_by_k}, {"static_resolve_by_k", &tt.static_resolve_by_k}};
    for (const auto& [name, values] : per_k) {
      std::printf(",\"%s\":[", name);
      for (unsigned k = 1; k <= options.kmax; ++k) std::printf("%s%.3f", k > 1 ? "," : "", (*values)[k]);
      std::printf("]");
    }
    // v22 (E0): validation sub-timers (input, key sort, key index, pass 1,
    // pass 2, level sort, level runs, programs).
    std::printf(",\"validate_parts\":[");
    for (size_t p = 0; p < tt.validate_parts.size(); ++p) std::printf("%s%.3f", p ? "," : "", tt.validate_parts[p]);
    std::printf("]},");
  }
  {
    const auto& l = r.ledger;
    const std::pair<const char*, std::uint64_t> rows[] = {{"expanded_pairs",l.expanded_pairs},{"cover_builds",l.cover_builds},{"cover_sites",l.cover_sites},{"cover_node_visits",l.cover_node_visits},{"q3_edges",l.q3_edges},{"q4_edges",l.q4_edges},{"both_edges",l.both_edges},{"witness_input_pair_mass",l.witness_input_pair_mass},{"witness_rejected_rectangles",l.witness_rejected_rectangles},{"witness_rejected_pairs",l.witness_rejected_pairs},{"q3_seeds",l.q3_seeds},{"q3_ball_builds",l.q3_ball_builds},{"q3_depth_rejections",l.q3_depth_rejections},{"q3_census_bounds",l.q3_census_bounds},{"q3_census_point_tests",l.q3_census_point_tests},{"q3_atlas_edges",l.q3_atlas_edges},{"q3_atlas_locations",l.q3_atlas_locations},{"q3_atlas_rejections",l.q3_atlas_rejections},{"q3_atlas_outside_domain",l.q3_atlas_outside_domain},{"atlas_cells",l.atlas_cells},{"atlas_leaf_cells",l.atlas_leaf_cells},{"atlas_deep_cells",l.atlas_deep_cells},{"atlas_outside_cells",l.atlas_outside_cells},{"atlas_splits",l.atlas_splits},{"atlas_node_visits",l.atlas_node_visits},{"atlas_block_bounds",l.atlas_block_bounds},{"atlas_point_tests",l.atlas_point_tests},{"atlas_ids_copied",l.atlas_ids_copied},{"q4_seeds",l.q4_seeds},{"q4_live_leaves",l.q4_live_leaves},{"q4_whole_atlas_skips",l.q4_whole_atlas_skips},{"q4_sweep_events",l.q4_sweep_events},{"q3_leaf_censuses",l.q3_leaf_censuses},{"q3_leaf_point_tests",l.q3_leaf_point_tests},{"q3_leaf_rejections",l.q3_leaf_rejections},{"q3_lower_bound_fallbacks",l.q3_lower_bound_fallbacks},{"dead_loads",l.dead_loads},{"dead_form_sites",l.dead_form_sites},{"dead_cells",l.dead_cells},{"dead_outside_cells",l.dead_outside_cells},{"dead_deep_cells",l.dead_deep_cells},{"dead_failed_cells",l.dead_failed_cells},{"dead_uniform_tests",l.dead_uniform_tests},{"dead_point_tests",l.dead_point_tests},{"dead_q3_proved",l.dead_q3_proved},{"dead_q3_open",l.dead_q3_open},{"dead_q4_proved",l.dead_q4_proved},{"dead_q4_open",l.dead_q4_open},{"witness_cache_queries",l.witness_cache_queries},{"witness_cache_node_tests",l.witness_cache_node_tests},{"witness_cache_rejected_pairs",l.witness_cache_rejected_pairs},{"core_builds",l.core_builds},{"core_sites",l.core_sites},{"core_closed_edges",l.core_closed_edges},{"dead_core_loads",l.dead_core_loads},{"dead_core_form_sites",l.dead_core_form_sites},{"dead_core_cells",l.dead_core_cells},{"dead_core_uniform_tests",l.dead_core_uniform_tests},{"dead_core_point_tests",l.dead_core_point_tests},{"dead_core_q3_proved",l.dead_core_q3_proved},{"dead_core_q3_open",l.dead_core_q3_open},{"dead_core_q4_proved",l.dead_core_q4_proved},{"dead_core_q4_open",l.dead_core_q4_open},{"core_cover_node_visits",l.core_cover_node_visits},{"core_cover_bound_tests",l.core_cover_bound_tests},{"core_cover_point_tests",l.core_cover_point_tests},{"dead_core_outside_cells",l.dead_core_outside_cells},{"dead_core_deep_cells",l.dead_core_deep_cells},{"dead_core_failed_cells",l.dead_core_failed_cells},{"q34_input_rectangles",l.q34_input_rectangles},{"witness_rect_queries",l.witness_rect_queries},{"witness_rect_node_visits",l.witness_rect_node_visits},{"witness_pair_queries",l.witness_pair_queries},{"witness_pair_node_visits",l.witness_pair_node_visits},{"q3_edge_queries",l.q3_edge_queries},{"q3_seed_node_visits",l.q3_seed_node_visits},{"q3_seed_point_tests",l.q3_seed_point_tests},{"q3_seed_bound_tests",l.q3_seed_bound_tests},{"q4_geometry_preparations",l.q4_geometry_preparations},{"q4_domain_node_visits",l.q4_domain_node_visits},{"q4_cover_decomposition_node_visits",l.q4_cover_decomposition_node_visits},{"q4_seed_node_visits",l.q4_seed_node_visits},{"q4_seed_cell_queries",l.q4_seed_cell_queries},{"q4_sweep_active_sites",l.q4_sweep_active_sites},{"lanes_edges",l.lanes_edges},{"lanes_cover_sites",l.lanes_cover_sites},{"lanes_cover_node_visits",l.lanes_cover_node_visits},{"lanes_seed_tests",l.lanes_seed_tests},{"lanes_acute_sites",l.lanes_acute_sites},{"lanes_owner_rejections",l.lanes_owner_rejections},{"lanes_seeds",l.lanes_seeds},{"lanes_census_point_tests",l.lanes_census_point_tests},{"lanes_census_inside_sites",l.lanes_census_inside_sites},{"lanes_census_shell_sites",l.lanes_census_shell_sites},{"lanes_census_outside_sites",l.lanes_census_outside_sites},{"lanes_depth_rejections",l.lanes_depth_rejections},{"lanes_emitted",l.lanes_emitted},{"lanes_shell_ids",l.lanes_shell_ids},{"lanes_q3_edges",l.lanes_q3_edges},{"lanes_census_seeds",l.lanes_census_seeds},{"lanes4_edges",l.lanes4_edges},{"lanes4_seeds",l.lanes4_seeds},{"lanes4_certified",l.lanes4_certified},{"lanes4_certified_chunk1",l.lanes4_certified_chunk1},{"lanes4_survivors",l.lanes4_survivors},{"lanes4_pass_chunks",l.lanes4_pass_chunks},{"lanes4_pass_site_tests",l.lanes4_pass_site_tests},{"lanes4_buffered_events",l.lanes4_buffered_events},{"lanes4_max_buffered",l.lanes4_max_buffered},{"lanes4_live_buckets",l.lanes4_live_buckets},{"lanes4_filter_steps",l.lanes4_filter_steps},{"lanes4_bucket_events",l.lanes4_bucket_events},{"lanes4_candidates",l.lanes4_candidates},{"lanes4_foreign_candidates",l.lanes4_foreign_candidates},{"lanes4_groups",l.lanes4_groups},{"lanes4_compare_steps",l.lanes4_compare_steps},{"lanes4_depth_rejected_groups",l.lanes4_depth_rejected_groups},{"lanes4_positivity_tests",l.lanes4_positivity_tests},{"lanes4_groups_without_valid",l.lanes4_groups_without_valid},{"lanes4_emitted",l.lanes4_emitted},{"lanes4_emitting_seeds",l.lanes4_emitting_seeds},{"lanes4_multi_emission_seeds",l.lanes4_multi_emission_seeds},{"lanes4_max_emissions_per_seed",l.lanes4_max_emissions_per_seed},{"lanes4_shell_ids",l.lanes4_shell_ids},{"lanes4_max_group",l.lanes4_max_group},{"lanes4_constant_shell_sites",l.lanes4_constant_shell_sites},{"lanes4_list_steps",l.lanes4_list_steps},{"lanes4_group_steps",l.lanes4_group_steps}};
    std::printf("\"ledger\":{");
    bool first = true;
    for (const auto& [name, value] : rows) {
      std::printf("%s\"%s\":%" PRIu64, first ? "" : ",", name, value);
      first = false;
    }
    std::printf("},");
  }
  const auto& ts = r.tower_stats;
  std::printf("\"tower_work\":{\"records\":%" PRIu64 ",\"extra_records\":%" PRIu64 ",\"representatives\":%" PRIu64
              ",\"anchor_hits\":%" PRIu64 ",\"key_lookups\":%" PRIu64 ",\"intruder_queries\":%" PRIu64 ",\"intruder_nodes\":%" PRIu64
              ",\"meb_calls\":%" PRIu64 ",\"meb_power_tests\":%" PRIu64 ",\"births\":%" PRIu64 ",\"merges\":%" PRIu64
              ",\"contributions\":%" PRIu64 ",\"grouped_lots\":%" PRIu64 ",\"resolver_cache_hits\":%" PRIu64
              ",\"meb_accounting\":\"%s\",\"meb_pair_distances\":%" PRIu64 ",\"meb_materializations\":%" PRIu64
              ",\"meb_supports_by_size\":[%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 "]"
              ",\"meb_proposals\":%" PRIu64 ",\"meb_verified_proposals\":%" PRIu64
              ",\"meb_boundary_canonicalizations\":%" PRIu64 ",\"meb_proposal_fallbacks\":%" PRIu64 "},",
              ts.records, ts.extra_records, ts.representatives, ts.anchor_hits, ts.key_lookups, ts.intruder_queries,
              ts.intruder_nodes, ts.resolve_work.calls, ts.resolve_work.power_tests, ts.births, ts.merges, ts.contributions,
              ts.grouped_lots, ts.resolver_cache_hits,
              options.tower_meb_proposal ? mhgp9::tower::kAnchorMebProposedWorkAccounting
                                         : mhgp9::tower::kAnchorMebWorkAccounting,
              ts.resolve_work.pair_distances, ts.resolve_work.materializations, ts.resolve_work.supports_by_size[1],
              ts.resolve_work.supports_by_size[2], ts.resolve_work.supports_by_size[3], ts.resolve_work.supports_by_size[4],
              ts.resolve_work.proposals, ts.resolve_work.verified_proposals, ts.resolve_work.boundary_canonicalizations,
              ts.resolve_work.proposal_fallbacks);
  std::printf("\"orders\":[");
  for (std::size_t i = 0; i < r.orders.size(); ++i) {
    const auto& o = r.orders[i];
    std::printf("%s{\"K\":%u,\"nodes\":%" PRIu64 ",\"births\":%" PRIu64 ",\"merges\":%" PRIu64 ",\"parents\":%" PRIu64
                ",\"contributions\":%" PRIu64 "}",
                i ? "," : "", o.k, o.nodes, o.births, o.merges, o.parents, o.contributions);
  }
  // v18 : condense canonique du catalogue complet (--catalogue-digest), null sinon.
  char catalogue[24] = "null", presentations[24] = "null";
  if (options.catalogue_digest) {
    std::snprintf(catalogue, sizeof catalogue, "\"%016" PRIx64 "\"", r.catalogue_digest);
    std::snprintf(presentations, sizeof presentations, "\"%016" PRIx64 "\"", r.presentation_digest);
  }
  std::printf("],\"tower_digest\":\"%016" PRIx64 "\",\"catalogue_digest\":%s,\"presentation_digest\":%s,"
              "\"peak_rss_kb\":%ld}\n", r.tower_digest, catalogue, presentations, peak_rss_kb());
  return r.status == mhgp9::ChainStatus::kComplete ? 0 : 3;
}
