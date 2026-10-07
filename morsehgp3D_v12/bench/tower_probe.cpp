// Sonde de l'etage G de la tour (tranche T2) : lit une entree u32le et ses ids.u32le (ou un nuage synthetique),
// construit l'index, le catalogue Cat_K et la resolution (module tower, voie CPU de reference), ecrit une ligne JSON
// par passe et par ordre (compteurs de l'objet et du travail, diagnostics physiques), et, sur demande, l'empreinte
// de la resolution et un export (exportateur de TEST, hors du chemin chronometre) : cat.bin (MHGP12DP genre 1, export
// du catalogue) et res.bin (MHGP12DP genre 4, << resolution >>), dans un dossier transactionnel.
//
//   mhgp12_tower_probe <xyz.u32le> <ids.u32le> [options]
//   mhgp12_tower_probe --uniform=N,GRAINE,BITS [options]
//   options : [--k=K] [--leaf=L] [--threads=W] [--passes=P] [--budget=OCTETS] [--digest] [--out=DOSSIER]
//             [--frame=NOM]
//
// res.bin : en-tete de 64 octets (magie MHGP12DP, version 1, genre 4, bits, K, nombre d'ordres, nombre de sections,
// sites, trame), puis par ordre k (deux chiffres kk) les sections BKEY_kk (u32), BRNK_kk (u32), CBAL_kk (u32),
// CRNK_kk (u32), CFLG_kk (u8), COFF_kk (u64), TMSK_kk (u64), TARG_kk (u32) et CNTR_kk (u64, compteurs dans l'ordre de
// counter_values). Codes : 0 conforme, 2 refus (usage, entree, ressources, degenerescence), 3 invariant viole.
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

#include "catalogue/catalogue.hpp"
#include "index/index.hpp"
#include "io/io.hpp"
#include "sched/sched.hpp"
#include "tower/tower.hpp"
#include "tower_export.hpp"

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
};

bool number(std::string_view text, u64& value) {
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  return error == std::errc{} && end == text.data() + text.size();
}

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
  o.params.leaf_size = 0;
  for (int i = first; i < argc; ++i) {
    const std::string_view a = argv[i];
    u64 v = 0;
    auto value = [&](std::string_view key) { return a.substr(0, key.size()) == key ? a.substr(key.size()) : "\x01"; };
    if (number(value("--k="), v) && v <= 64) o.params.kmax = static_cast<int>(v);
    else if (number(value("--leaf="), v) && v <= 1u << 20) o.params.leaf_size = static_cast<u32>(v);
    else if (number(value("--threads="), v) && v >= 1 && v <= sched::kMaxWorkers) o.threads = v;
    else if (number(value("--passes="), v) && v >= 1 && v <= 1000) o.passes = v;
    else if (number(value("--budget="), v)) o.budget = v;
    else if (a == "--digest") o.digest = true;
    else if (a.substr(0, 6) == "--out=" && a.size() > 6) o.out = argv[i] + 6;
    else if (a.substr(0, 8) == "--frame=" && a.size() > 8 && a.size() < 8 + 24) o.frame = a.substr(8);
    else return false;
  }
  if (o.params.leaf_size == 0)  // feuille par defaut : K + 3 au moins, 24 comme la sonde du catalogue
    o.params.leaf_size = static_cast<u32>(std::max(24, o.params.kmax + 3));
  return true;
}

// Nuage synthetique de la sonde du catalogue : SplitMix64, positions distinctes, PointId = rang lexicographique.
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

void order_json(const ResolvedOrder& order) {
  const OrderCounters& c = order.counters();
  const auto v = probe::counter_values(c);
  std::printf("{\"phase\":\"ordre\",\"k\":%u,\"objet\":{", unsigned{order.order()});
  for (std::size_t i = 0; i < probe::kObjectCounters; ++i)
    std::printf("%s\"%s\":%llu", i ? "," : "", probe::kCounterNames[i], (unsigned long long)v[i]);
  std::printf("},\"travail\":{");
  for (std::size_t i = probe::kObjectCounters; i < probe::kScalarCounters; ++i)
    std::printf("%s\"%s\":%llu", i > probe::kObjectCounters ? "," : "", probe::kCounterNames[i],
                (unsigned long long)v[i]);
  std::printf(",\"chaines\":[");
  for (int i = 0; i < kChainBins; ++i) std::printf("%s%llu", i ? "," : "", (unsigned long long)c.chain_histogram[i]);
  std::printf("]}}\n");
}

void stage_json(u64 pass, const Options& o, u32 sites, const Outcome& outcome, u64 wall,
                const ResolutionDiagnostics& d) {
  std::printf("{\"phase\":\"tour_g\",\"pass\":%llu,\"status\":\"%s\",\"reason\":\"%s\",\"order\":%u,\"coord_bits\":%d,"
              "\"kmax\":%d,\"threads\":%llu,\"sites\":%u,\"wall_ns\":%llu",
              (unsigned long long)pass, std::string(status_name(outcome.status())).c_str(),
              std::string(reason_name(outcome.reason)).c_str(), unsigned{outcome.order}, kCoordBits, o.params.kmax,
              (unsigned long long)o.threads, sites, (unsigned long long)wall);
  if (outcome.ok()) {
    std::printf(",\"diagnostics\":{\"count_ns\":%llu,\"fill_ns\":%llu,\"tables_ns\":%llu,\"resolve_ns\":%llu,"
                "\"workspace_bytes\":%llu,\"table_bytes\":%llu,\"peak_bytes\":%llu,\"order_ns\":[",
                (unsigned long long)d.count_ns, (unsigned long long)d.fill_ns, (unsigned long long)d.tables_ns,
                (unsigned long long)d.resolve_ns, (unsigned long long)d.workspace_bytes,
                (unsigned long long)d.table_bytes, (unsigned long long)d.peak_bytes);
    for (int k = 1; k <= o.params.kmax && k < 13; ++k)
      std::printf("%s%llu", k > 1 ? "," : "", (unsigned long long)d.order_ns[k]);
    std::printf("]}");
  }
  std::printf("}\n");
  std::fflush(stdout);
}

Outcome export_to(const Options& o, const Cloud& cloud, const Catalogue& catalogue, const Resolution& r) {
  const char* inputs[2] = {o.xyz, o.ids};
  auto dir = io::OutputDirectory::plan(o.out, std::span<const char* const>(inputs, o.xyz != nullptr ? 2u : 0u));
  if (!dir.ok()) return dir.outcome();
  auto cat = dir.value().create("cat.bin");
  if (!cat.ok()) return cat.outcome();
  MHGP12_TRY(export_catalogue(cloud, catalogue, o.frame, *cat.value()));
  auto res = dir.value().create("res.bin");
  if (!res.ok()) return res.outcome();
  MHGP12_TRY(probe::write_resolution(cloud, r, o.frame, *res.value()));
  const auto hc = io::to_hex(cat.value()->digest()), hr = io::to_hex(res.value()->digest());
  const std::string manifest = "{\"schema\":\"mhgp12_tower_g_dump_v1\",\"cat_bin_sha256\":\"" +
                               std::string(hc.data(), hc.size()) + "\",\"res_bin_sha256\":\"" +
                               std::string(hr.data(), hr.size()) + "\"}";
  MHGP12_TRY(dir.value().commit(manifest));
  std::printf("{\"phase\":\"export\",\"res_bin_sha256\":\"%s\"}\n", std::string(hr.data(), hr.size()).c_str());
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
  auto index = build_index(std::move(cloud.value()), IndexParams{}, budget);
  if (!index.ok()) return index.outcome();
  auto pool = sched::make_pool({static_cast<u32>(o.threads)});
  if (!pool.ok()) return pool.outcome();
  auto catalogue = build_catalogue(index.value().cloud(), o.params, budget, *pool.value());
  if (!catalogue.ok()) return catalogue.outcome();
  for (u64 pass = 0; pass < o.passes; ++pass) {
    ResolutionDiagnostics diag;
    const Stopwatch watch;
    auto r = resolve_tower(index.value(), catalogue.value(), budget, *pool.value(), &diag);
    stage_json(pass, o, index.value().cloud().sites(), r.ok() ? Outcome{} : r.outcome(), watch.nanoseconds(), diag);
    if (!r.ok()) return r.outcome();
    if (pass + 1 < o.passes) continue;
    for (Order k = 1; k <= r.value().orders(); ++k) order_json(r.value().order(k));
    if (o.digest) {
      auto digest = probe::resolution_digest(index.value().cloud(), r.value(), o.frame);
      if (!digest.ok()) return digest.outcome();
      const auto hex = io::to_hex(digest.value());
      std::printf("{\"phase\":\"digest\",\"resolution_sha256\":\"%s\"}\n", std::string(hex.data(), hex.size()).c_str());
    }
    if (o.out != nullptr) MHGP12_TRY(export_to(o, index.value().cloud(), catalogue.value(), r.value()));
  }
  return {};
}

}  // namespace

int main(int argc, char** argv) {
  Options o;
  if (!parse(argc, argv, o)) {
    std::fprintf(stderr, "usage : mhgp12_tower_probe (<xyz.u32le> <ids.u32le> | --uniform=N,GRAINE,BITS) [--k=K] "
                         "[--leaf=L] [--threads=W] [--passes=P] [--budget=OCTETS] [--digest] [--out=DOSSIER] "
                         "[--frame=NOM]\n");
    return 2;
  }
  const Outcome outcome = guarded([&]() { return run(o); });
  std::printf("{\"phase\":\"exit\",\"status\":\"%s\",\"reason\":\"%s\",\"order\":%u}\n",
              std::string(status_name(outcome.status())).c_str(), std::string(reason_name(outcome.reason)).c_str(),
              unsigned{outcome.order});
  return exit_code(outcome);
}
