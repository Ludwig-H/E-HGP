// MorseHGP3D v9 — sonde de la tour FULL de bout en bout sur un nuage fichier.
//
//   mhgp9_tower_probe <fichier .u32le|.u16le> K workers [--s=8] [--static=T]
//                     [--no-tower] [--n=prefixe] [--grid=libelle]
//                     [--lever=NOM=0|1 ...]
//
// Leviers (meme objet, travail different) : atlas_saturate_deep,
// q3_leaf_census, q34_dead_lanes, q34_witness_cache, q34_dead_core (ce dernier
// exige q34_dead_lanes, sinon la chaine refuse). Tous sont publies dans
// options.levers ; un plan G4 les epingle explicitement, un nom inconnu est
// refuse (code 2).
//
// Chronometre du contrat : du nuage prepare en memoire a la tour complete en
// memoire (ChainTimes, sans la lecture). La lecture et son empreinte sont
// mesurees a part. Sortie : un objet JSON sur stdout. Code 0 conforme, 2 refus
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
  std::printf("{\"schema\":\"mhgp9_tower_probe_v7\",\"status\":\"%s\",\"reason\":\"%s\",", mhgp9::chain_status_name(r.status),
              r.reason.c_str());
  std::printf("\"input\":{\"format\":\"%s\",\"grid\":\"%s\",\"sites\":%zu,\"hash\":\"%016" PRIx64 "\"},", input.format.c_str(),
              grid.c_str(), input.points.size(), input.hash);
  std::printf("\"options\":{\"K\":%u,\"K_effective\":%u,\"s\":%u,\"workers\":%zu,\"tower_static_threads\":%d,\"run_tower\":%s,"
              "\"levers\":{\"atlas_saturate_deep\":%s,\"q3_leaf_census\":%s,\"q34_dead_lanes\":%s,"
              "\"q34_witness_cache\":%s,\"q34_dead_core\":%s}},",
              options.kmax, r.kmax_effective, options.separation_s, options.workers,
              options.tower_static_threads >= 0 ? options.tower_static_threads : r.tower_static_threads,
              options.run_tower ? "true" : "false", options.atlas_saturate_deep ? "true" : "false",
              options.q3_leaf_census ? "true" : "false", options.q34_dead_lanes ? "true" : "false",
              options.q34_witness_cache ? "true" : "false", options.q34_dead_core ? "true" : "false");
  std::printf("\"times_ms\":{\"read\":%.3f,\"prepare\":%.3f,\"gen_index\":%.3f,\"q2\":%.3f,\"q34\":%.3f,\"merge\":%.3f,"
              "\"tower_index\":%.3f,\"census\":%.3f,\"tower\":%.3f,\"chain_total\":%.3f},\"chain_cpu_s\":%.3f,",
              read_ms, t.prepare_ms, t.gen_index_ms, t.q2_ms, t.q34_ms, t.merge_ms, t.tower_index_ms, t.census_ms, t.tower_ms,
              t.total_ms, t.cpu_s);
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
  std::printf("]},");
  {
    const auto& l = r.ledger;
    const std::pair<const char*, std::uint64_t> rows[] = {{"expanded_pairs",l.expanded_pairs},{"cover_builds",l.cover_builds},{"cover_sites",l.cover_sites},{"cover_node_visits",l.cover_node_visits},{"q3_edges",l.q3_edges},{"q4_edges",l.q4_edges},{"both_edges",l.both_edges},{"witness_input_pair_mass",l.witness_input_pair_mass},{"witness_rejected_rectangles",l.witness_rejected_rectangles},{"witness_rejected_pairs",l.witness_rejected_pairs},{"q3_seeds",l.q3_seeds},{"q3_ball_builds",l.q3_ball_builds},{"q3_depth_rejections",l.q3_depth_rejections},{"q3_census_bounds",l.q3_census_bounds},{"q3_census_point_tests",l.q3_census_point_tests},{"q3_atlas_edges",l.q3_atlas_edges},{"q3_atlas_locations",l.q3_atlas_locations},{"q3_atlas_rejections",l.q3_atlas_rejections},{"q3_atlas_outside_domain",l.q3_atlas_outside_domain},{"atlas_cells",l.atlas_cells},{"atlas_leaf_cells",l.atlas_leaf_cells},{"atlas_deep_cells",l.atlas_deep_cells},{"atlas_outside_cells",l.atlas_outside_cells},{"atlas_splits",l.atlas_splits},{"atlas_node_visits",l.atlas_node_visits},{"atlas_block_bounds",l.atlas_block_bounds},{"atlas_point_tests",l.atlas_point_tests},{"atlas_ids_copied",l.atlas_ids_copied},{"q4_seeds",l.q4_seeds},{"q4_live_leaves",l.q4_live_leaves},{"q4_whole_atlas_skips",l.q4_whole_atlas_skips},{"q4_sweep_events",l.q4_sweep_events},{"q3_leaf_censuses",l.q3_leaf_censuses},{"q3_leaf_point_tests",l.q3_leaf_point_tests},{"q3_leaf_rejections",l.q3_leaf_rejections},{"q3_lower_bound_fallbacks",l.q3_lower_bound_fallbacks},{"dead_loads",l.dead_loads},{"dead_form_sites",l.dead_form_sites},{"dead_cells",l.dead_cells},{"dead_outside_cells",l.dead_outside_cells},{"dead_deep_cells",l.dead_deep_cells},{"dead_failed_cells",l.dead_failed_cells},{"dead_uniform_tests",l.dead_uniform_tests},{"dead_point_tests",l.dead_point_tests},{"dead_q3_proved",l.dead_q3_proved},{"dead_q3_open",l.dead_q3_open},{"dead_q4_proved",l.dead_q4_proved},{"dead_q4_open",l.dead_q4_open},{"witness_cache_queries",l.witness_cache_queries},{"witness_cache_node_tests",l.witness_cache_node_tests},{"witness_cache_rejected_pairs",l.witness_cache_rejected_pairs},{"core_builds",l.core_builds},{"core_sites",l.core_sites},{"core_closed_edges",l.core_closed_edges},{"dead_core_loads",l.dead_core_loads},{"dead_core_form_sites",l.dead_core_form_sites},{"dead_core_cells",l.dead_core_cells},{"dead_core_uniform_tests",l.dead_core_uniform_tests},{"dead_core_point_tests",l.dead_core_point_tests},{"dead_core_q3_proved",l.dead_core_q3_proved},{"dead_core_q3_open",l.dead_core_q3_open},{"dead_core_q4_proved",l.dead_core_q4_proved},{"dead_core_q4_open",l.dead_core_q4_open}};
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
              ",\"meb_supports_by_size\":[%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 "]},",
              ts.records, ts.extra_records, ts.representatives, ts.anchor_hits, ts.key_lookups, ts.intruder_queries,
              ts.intruder_nodes, ts.resolve_work.calls, ts.resolve_work.power_tests, ts.births, ts.merges, ts.contributions,
              ts.grouped_lots, ts.resolver_cache_hits, mhgp9::tower::kAnchorMebWorkAccounting,
              ts.resolve_work.pair_distances, ts.resolve_work.materializations, ts.resolve_work.supports_by_size[1],
              ts.resolve_work.supports_by_size[2], ts.resolve_work.supports_by_size[3], ts.resolve_work.supports_by_size[4]);
  std::printf("\"orders\":[");
  for (std::size_t i = 0; i < r.orders.size(); ++i) {
    const auto& o = r.orders[i];
    std::printf("%s{\"K\":%u,\"nodes\":%" PRIu64 ",\"births\":%" PRIu64 ",\"merges\":%" PRIu64 ",\"parents\":%" PRIu64
                ",\"contributions\":%" PRIu64 "}",
                i ? "," : "", o.k, o.nodes, o.births, o.merges, o.parents, o.contributions);
  }
  std::printf("],\"tower_digest\":\"%016" PRIx64 "\",\"peak_rss_kb\":%ld}\n", r.tower_digest, peak_rss_kb());
  return r.status == mhgp9::ChainStatus::kComplete ? 0 : 3;
}
