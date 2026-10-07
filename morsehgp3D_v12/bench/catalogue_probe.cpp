// Sonde du catalogue (tranches T1 et T1-b) : lit une entree u32le et ses ids.u32le, calcule Cat_K par la voie CPU de
// reference du module catalogue, ou par la voie appareil (--device : contexte CatalogueDevice ouvert une fois, puis une
// passe par appel, regime resident), ecrit une ligne JSON par passe (voie, statut, comptes, grand livre logique,
// diagnostics physiques, duree murale) et, sur demande, exporte le catalogue au format MHGP12DP (dossier transactionnel
// du module io : cat.bin puis manifeste.json), hors du chemin chronometre.
//
//   mhgp12_catalogue_probe <xyz.u32le> <ids.u32le> [options]
//   mhgp12_catalogue_probe --uniform=N,GRAINE,BITS [options]      nuage synthetique (SplitMix64, positions distinctes)
//   options : [--k=K] [--leaf=L] [--max-leaf=M] [--threads=W] [--passes=P] [--budget=OCTETS] [--digest]
//             [--out=DOSSIER] [--frame=NOM] [--device]
//
// Defauts : K = 5, feuille 24, max_leaf 256, un fil, une passe, budget illimite, voie CPU. --digest ecrit l'empreinte
// canonique (catalogue_digest : SHA-256 de l'export MHGP12DP) de chaque passe. --device : la ligne "open" donne la
// duree d'ouverture du contexte (a froid) ; wall_ns de chaque passe est le temps de l'etage C, transferts compris.
// Codes : 0 conforme, 2 refus (usage, entree, parametres, ressources, degenerescence, appareil indisponible),
// 3 invariant viole (dont device_fault).
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>

#include "catalogue/catalogue.hpp"
#include "io/io.hpp"
#include "sched/sched.hpp"

using namespace mhgp12;

namespace {

struct Options {
  const char* xyz = nullptr;
  const char* ids = nullptr;
  u64 uniform = 0, seed = 0, bits = 0;
  bool digest = false;
  CatalogueParams params;
  u64 threads = 1, passes = 1, budget = MemoryBudget::kUnlimited;
  const char* out = nullptr;
  std::string_view frame = "probe";
  bool device = false;
};

bool number(std::string_view text, u64& value) {
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  return error == std::errc{} && end == text.data() + text.size();
}

// --uniform=N,GRAINE,BITS : trois entiers separes par des virgules.
bool triple(std::string_view text, Options& o) {
  const auto first = text.find(','), second = text.rfind(',');
  if (first == std::string_view::npos || second == first) return false;
  return number(text.substr(0, first), o.uniform) && number(text.substr(first + 1, second - first - 1), o.seed) &&
         number(text.substr(second + 1), o.bits) && o.uniform >= 1 && o.uniform <= (u64{1} << 26) && o.bits >= 1 &&
         o.bits <= static_cast<u64>(kCoordBits);
}

bool parse(int argc, char** argv, Options& o) {
  int first = 1;
  if (argc >= 2 && std::string_view(argv[1]).substr(0, 10) == "--uniform=") {
    if (!triple(std::string_view(argv[1]).substr(10), o)) return false;
    first = 2;
  } else {
    if (argc < 3) return false;
    o.xyz = argv[1];
    o.ids = argv[2];
    first = 3;
  }
  for (int i = first; i < argc; ++i) {
    const std::string_view a = argv[i];
    u64 v = 0;
    auto value = [&](std::string_view key) { return a.substr(0, key.size()) == key ? a.substr(key.size()) : "\x01"; };
    if (number(value("--k="), v) && v <= 64) o.params.kmax = static_cast<int>(v);
    else if (number(value("--leaf="), v) && v <= 1u << 20) o.params.leaf_size = static_cast<u32>(v);
    else if (number(value("--max-leaf="), v) && v <= 1u << 20) o.params.max_leaf = static_cast<u32>(v);
    else if (number(value("--threads="), v) && v >= 1 && v <= sched::kMaxWorkers) o.threads = v;
    else if (number(value("--passes="), v) && v >= 1 && v <= 1000) o.passes = v;
    else if (number(value("--budget="), v)) o.budget = v;
    else if (a == "--digest") o.digest = true;
    else if (a == "--device") o.device = true;
    else if (a.substr(0, 6) == "--out=" && o.xyz != nullptr) o.out = argv[i] + 6;
    else if (a.substr(0, 8) == "--frame=" && a.size() > 8 && a.size() < 8 + 24) o.frame = a.substr(8);
    else return false;
  }
  return true;
}

void ledger_json(const CatalogueLedger& l) {
  std::printf(",\"ledger\":{\"nodes\":%llu,\"leaves\":%llu,\"filter_tests\":%llu,\"max_depth\":%llu,\"max_leaf\":%llu,"
              "\"dominance_tests\":%llu,\"prefixes\":%llu,\"judged\":%llu,\"census_tests\":%llu,\"emitted\":%llu,"
              "\"incidences\":%llu,\"q4_candidates\":%llu,\"q4_levels\":%llu,\"region_pair_tests\":%llu,"
              "\"region_pair_rejects\":%llu,\"region_line_tests\":%llu,\"region_line_rejects\":%llu,"
              "\"region_line_evaluations\":%llu,\"region_line_cache_hits\":%llu,\"region_line_fallbacks\":%llu}",
              (unsigned long long)l.nodes, (unsigned long long)l.leaves, (unsigned long long)l.filter_tests,
              (unsigned long long)l.max_depth, (unsigned long long)l.max_leaf, (unsigned long long)l.dominance_tests,
              (unsigned long long)l.prefixes, (unsigned long long)l.judged, (unsigned long long)l.census_tests,
              (unsigned long long)l.emitted, (unsigned long long)l.incidences, (unsigned long long)l.q4_candidates,
              (unsigned long long)l.q4_levels, (unsigned long long)l.region_pair_tests,
              (unsigned long long)l.region_pair_rejects, (unsigned long long)l.region_line_tests,
              (unsigned long long)l.region_line_rejects, (unsigned long long)l.region_line_evaluations,
              (unsigned long long)l.region_line_cache_hits, (unsigned long long)l.region_line_fallbacks);
}

void diagnostics_json(const CatalogueDiagnostics& d) {
  std::printf(",\"diagnostics\":{\"levels\":%llu,\"tasks\":%llu,\"candidates\":%llu,\"leaves_narrow\":%llu,"
              "\"leaves_medium\":%llu,\"leaves_wide\":%llu,\"leaves_exact\":%llu,\"leaves_virtual_warp\":%llu,"
              "\"leaves_rewritten\":%llu,\"max_leaf_span\":%llu,\"traversal_ns\":%llu,\"count_ns\":%llu,"
              "\"fill_ns\":%llu,\"levels_ns\":%llu,\"sort_ns\":%llu,\"assemble_ns\":%llu,\"table_ns\":%llu,"
              "\"peak_bytes\":%llu,\"chains_repaired\":%llu,\"chain_elements\":%llu}",
              (unsigned long long)d.levels, (unsigned long long)d.tasks, (unsigned long long)d.candidates,
              (unsigned long long)d.leaves_narrow, (unsigned long long)d.leaves_medium,
              (unsigned long long)d.leaves_wide, (unsigned long long)d.leaves_exact,
              (unsigned long long)d.leaves_virtual_warp, (unsigned long long)d.leaves_rewritten,
              (unsigned long long)d.max_leaf_span, (unsigned long long)d.traversal_ns, (unsigned long long)d.count_ns,
              (unsigned long long)d.fill_ns, (unsigned long long)d.levels_ns, (unsigned long long)d.sort_ns,
              (unsigned long long)d.assemble_ns, (unsigned long long)d.table_ns, (unsigned long long)d.peak_bytes,
              (unsigned long long)d.chains_repaired, (unsigned long long)d.chain_elements);
  // Voie appareil (hybride) : reprises sur l'hote par cause, reecritures par lieu, transferts du raccord complet.
  std::printf(",\"device\":{\"batches\":%llu,\"replayed_leaves\":%llu,\"replayed_balls\":%llu,\"replayed_wide\":%llu,"
              "\"replayed_span\":%llu,\"rewritten_device\":%llu,\"rewritten_host\":%llu,\"device_bytes\":%llu,"
              "\"pinned_bytes\":%llu,\"allocations\":%llu,\"arena_bytes\":%llu,\"transfer_ns\":%llu,"
              "\"transfer_h2d_bytes\":%llu,\"transfer_d2h_bytes\":%llu,\"transfer_ops\":%llu,\"publish_ns\":%llu}",
              (unsigned long long)d.batches, (unsigned long long)d.replayed_leaves, (unsigned long long)d.replayed_balls,
              (unsigned long long)d.replayed_wide, (unsigned long long)d.replayed_span,
              (unsigned long long)d.rewritten_device, (unsigned long long)d.rewritten_host,
              (unsigned long long)d.device_bytes, (unsigned long long)d.pinned_bytes,
              (unsigned long long)d.allocations, (unsigned long long)d.arena_bytes,
              (unsigned long long)d.transfer_ns, (unsigned long long)d.transfer_h2d_bytes,
              (unsigned long long)d.transfer_d2h_bytes, (unsigned long long)d.transfer_ops,
              (unsigned long long)d.publish_ns);
}

// Nuage synthetique : N points de [0, 2^BITS)^3 tires par SplitMix64 (Steele, Lea, Flood 2014), positions rendues
// distinctes par tri et elimination des doublons ; PointId = rang dans l'ordre lexicographique.
Outcome synthetic(const Options& o, MemoryBudget& budget, io::InputFiles& files) {
  Buffer<u64> keys;
  MHGP12_TRY(keys.allocate(3 * o.uniform, budget));
  u64 state = o.seed;
  const u64 mask = (u64{1} << o.bits) - 1;
  for (u64 i = 0; i < 3 * o.uniform; ++i) {
    state += 0x9E3779B97F4A7C15ull;
    u64 z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    keys[i] = (z ^ (z >> 31)) & mask;
  }
  Buffer<std::array<u32, 3>> points;
  MHGP12_TRY(points.allocate(o.uniform, budget));
  for (u64 i = 0; i < o.uniform; ++i)
    points[i] = {static_cast<u32>(keys[3 * i]), static_cast<u32>(keys[3 * i + 1]), static_cast<u32>(keys[3 * i + 2])};
  std::sort(points.begin(), points.end());
  const u64 n = static_cast<u64>(std::unique(points.begin(), points.end()) - points.begin());
  MHGP12_TRY(files.x.allocate(n, budget));
  MHGP12_TRY(files.y.allocate(n, budget));
  MHGP12_TRY(files.z.allocate(n, budget));
  MHGP12_TRY(files.ids.allocate(n, budget));
  for (u64 i = 0; i < n; ++i) {
    files.x[i] = points[i][0];
    files.y[i] = points[i][1];
    files.z[i] = points[i][2];
    files.ids[i] = make_id<PointId>(static_cast<u32>(i));
  }
  return {};
}

Outcome export_to(const Options& o, const Cloud& cloud, const Catalogue& catalogue) {
  const char* inputs[2] = {o.xyz, o.ids};
  auto dir = io::OutputDirectory::plan(o.out, inputs);
  if (!dir.ok()) return dir.outcome();
  auto file = dir.value().create("cat.bin");
  if (!file.ok()) return file.outcome();
  MHGP12_TRY(export_catalogue(cloud, catalogue, o.frame, *file.value()));
  const auto hex = io::to_hex(file.value()->digest());
  const std::string sha(hex.data(), hex.size());
  const std::string manifest = "{\"schema\":\"mhgp12_catalogue_dump_v1\",\"cat_bin_sha256\":\"" + sha + "\"}";
  MHGP12_TRY(dir.value().commit(manifest));
  std::printf("{\"phase\":\"export\",\"cat_bin_sha256\":\"%s\",\"bytes\":%llu}\n", sha.c_str(),
              (unsigned long long)file.value()->size());
  return {};
}

// Passes du catalogue sur la voie CPU (device nul) ou la voie appareil.
Outcome passes(const Options& o, const Cloud& cloud, sched::Pool& pool, MemoryBudget& budget, CatalogueDevice* device) {
  for (u64 pass = 0; pass < o.passes; ++pass) {
    CatalogueDiagnostics diag;
    Stopwatch watch;
    auto catalogue = device != nullptr ? build_catalogue_device(cloud, o.params, *device, pool, &diag)
                                       : build_catalogue(cloud, o.params, budget, pool, &diag);
    const u64 wall = watch.nanoseconds();
    std::printf("{\"phase\":\"catalogue\",\"path\":\"%s\",\"pass\":%llu,\"status\":\"%s\",\"reason\":\"%s\","
                "\"coord_bits\":%d,\"kmax\":%d,\"leaf\":%u,\"threads\":%llu,\"sites\":%u,\"wall_ns\":%llu",
                device != nullptr ? "device" : "cpu", (unsigned long long)pass,
                std::string(status_name(catalogue.outcome().status())).c_str(),
                std::string(reason_name(catalogue.outcome().reason)).c_str(), kCoordBits, o.params.kmax,
                o.params.leaf_size, (unsigned long long)o.threads, cloud.sites(), (unsigned long long)wall);
    if (catalogue.ok()) {
      const auto& c = catalogue.value();
      std::printf(",\"balls\":%u,\"incidences\":%llu,\"levels\":%llu", c.balls(),
                  (unsigned long long)c.population().size(), (unsigned long long)c.levels().size());
      ledger_json(c.ledger());
      diagnostics_json(diag);
    }
    std::printf("}\n");
    std::fflush(stdout);
    if (!catalogue.ok()) return catalogue.outcome();
    if (o.digest) {
      const auto digest = catalogue_digest(cloud, catalogue.value(), o.frame);
      if (!digest.ok()) return digest.outcome();
      const auto hex = io::to_hex(digest.value());
      std::printf("{\"phase\":\"digest\",\"catalogue_sha256\":\"%s\"}\n", std::string(hex.data(), hex.size()).c_str());
    }
    if (pass + 1 == o.passes && o.out != nullptr) MHGP12_TRY(export_to(o, cloud, catalogue.value()));
  }
  return {};
}

Outcome run(const Options& o) {
  MemoryBudget budget(o.budget);
  Result<io::InputFiles> input = io::InputFiles{};
  if (o.xyz != nullptr) input = io::read_u32le(o.xyz, o.ids, budget);
  else MHGP12_TRY(synthetic(o, budget, input.value()));
  if (!input.ok()) return input.outcome();
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                             input.value().ids.span(), CoordWidth(), budget);
  if (!cloud.ok()) return cloud.outcome();
  input.value() = {};
  auto pool = sched::make_pool({static_cast<u32>(o.threads)});
  if (!pool.ok()) return pool.outcome();
  if (!o.device) return passes(o, cloud.value(), *pool.value(), budget, nullptr);
  Stopwatch open_watch;
  auto device = CatalogueDevice::open(budget);
  std::printf("{\"phase\":\"open\",\"status\":\"%s\",\"reason\":\"%s\",\"open_ns\":%llu}\n",
              std::string(status_name(device.outcome().status())).c_str(),
              std::string(reason_name(device.outcome().reason)).c_str(), (unsigned long long)open_watch.nanoseconds());
  if (!device.ok()) return device.outcome();
  return passes(o, cloud.value(), *pool.value(), budget, &device.value());
}

}  // namespace

int main(int argc, char** argv) {
  Options o;
  if (!parse(argc, argv, o)) {
    std::fprintf(stderr, "usage : mhgp12_catalogue_probe (<xyz.u32le> <ids.u32le> | --uniform=N,GRAINE,BITS) [--k=K] "
                         "[--leaf=L] [--max-leaf=M] [--threads=W] [--passes=P] [--budget=OCTETS] [--digest] "
                         "[--out=DOSSIER] [--frame=NOM] [--device]\n");
    return 2;
  }
  const Outcome outcome = guarded([&]() { return run(o); });
  std::printf("{\"phase\":\"exit\",\"status\":\"%s\",\"reason\":\"%s\"}\n",
              std::string(status_name(outcome.status())).c_str(), std::string(reason_name(outcome.reason)).c_str());
  return exit_code(outcome);
}
