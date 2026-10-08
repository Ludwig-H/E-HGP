// Sonde FULL residente de la tour (contrat de la v12 : FULL K1..K en memoire, verticales comprises, a chaud). Frontiere
// du mur proposee par l'auditeur Codex le 8 octobre (receipts/audit_reponses_20261008/frontiere_full_proposee) :
//   - hors du mur, une fois : lecture des entrees (trames deja quantifiees, buffers u32le en memoire), ouverture de la
//     Session (budget, cache de blocs facultatif, Pool, contexte de l'appareil avec --device : ligne "open", a froid) ;
//   - mur d'une passe : de l'entree en memoire (avant nuage, tri de Morton et index) a la tour complete en memoire :
//     P (nuage, index), C (catalogue, appareil ou CPU, transferts et fin d'etage compris), G (resolution), raccord des
//     entrees de foret, T, M, V et R (noyau, contraction, verticales, registre) ; allocations comprises ;
//   - hors du mur, publies a part : validation des forets, empreinte FUL1 (--digest), puis liberation (ligne
//     "liberation").
// La passe p joue la trame p mod n (--trame repete : Session qui enchaine des trames successives). Une ligne JSON par
// passe ; la premiere est publiee comme les autres (aucun prechauffage cache).
//
//   mhgp12_full_probe (--trame=<xyz.u32le>,<ids.u32le>[,NOM] ... | --uniform=N,GRAINE,BITS) [--k=K] [--leaf=L]
//                     [--threads=W] [--passes=P] [--device] [--digest] [--budget=OCTETS] [--cache=OCTETS]
//
// Codes : 0 conforme ; 2 refus (usage, entree, ressources, appareil indisponible, degenerescence) ; 3 invariant viole.
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "catalogue/catalogue.hpp"
#include "index/index.hpp"
#include "io/io.hpp"
#include "sched/sched.hpp"
#include "tower/tower.hpp"

using namespace mhgp12;

namespace {

using Clock = std::chrono::steady_clock;

struct FrameSpec {
  std::string xyz, ids, name;
};

struct Options {
  std::vector<FrameSpec> frames;
  u64 uniform = 0, seed = 0, bits = 0;
  int kmax = 5;
  u32 leaf = 24, threads = 1;
  u64 passes = 1, budget = MemoryBudget::kUnlimited, cache = 0;
  bool device = false, digest = false;
};

struct Frame {
  std::string name;
  io::InputFiles input;
};

u64 since(Clock::time_point t0) {
  return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - t0).count());
}

bool parse_u64(std::string_view text, u64& out) {
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), out);
  return error == std::errc{} && end == text.data() + text.size();
}

bool parse_frame(std::string_view text, FrameSpec& f) {
  const auto a = text.find(',');
  if (a == std::string_view::npos) return false;
  const auto b = text.find(',', a + 1);
  f.xyz = std::string(text.substr(0, a));
  f.ids = std::string(text.substr(a + 1, b == std::string_view::npos ? std::string_view::npos : b - a - 1));
  f.name = b == std::string_view::npos ? "trame" : std::string(text.substr(b + 1));
  return !f.xyz.empty() && !f.ids.empty() && !f.name.empty() && f.name.size() < 24;
}

bool parse_uniform(std::string_view text, Options& o) {
  const auto a = text.find(','), b = text.rfind(',');
  if (a == std::string_view::npos || a == b) return false;
  return parse_u64(text.substr(0, a), o.uniform) && parse_u64(text.substr(a + 1, b - a - 1), o.seed) &&
         parse_u64(text.substr(b + 1), o.bits) && o.uniform >= 1 && o.uniform <= (u64{1} << 26) && o.bits >= 1 &&
         o.bits <= static_cast<u64>(kCoordBits);
}

bool parse(int argc, char** argv, Options& o) {
  for (int i = 1; i < argc; ++i) {
    const std::string_view a = argv[i];
    u64 v = 0;
    FrameSpec f;
    if (a.substr(0, 8) == "--trame=" && parse_frame(a.substr(8), f)) o.frames.push_back(f);
    else if (a.substr(0, 10) == "--uniform=" && o.uniform == 0 && parse_uniform(a.substr(10), o)) continue;
    else if (a.substr(0, 4) == "--k=" && parse_u64(a.substr(4), v) && v >= 1 && v <= 12) o.kmax = static_cast<int>(v);
    else if (a.substr(0, 7) == "--leaf=" && parse_u64(a.substr(7), v) && v >= 1 && v <= 256) o.leaf = static_cast<u32>(v);
    else if (a.substr(0, 10) == "--threads=" && parse_u64(a.substr(10), v) && v >= 1 && v <= 1024)
      o.threads = static_cast<u32>(v);
    else if (a.substr(0, 9) == "--passes=" && parse_u64(a.substr(9), v) && v >= 1 && v <= 100000) o.passes = v;
    else if (a.substr(0, 9) == "--budget=" && parse_u64(a.substr(9), v) && v > 0) o.budget = v;
    else if (a.substr(0, 8) == "--cache=" && parse_u64(a.substr(8), v)) o.cache = v;
    else if (a == "--device") o.device = true;
    else if (a == "--digest") o.digest = true;
    else return false;
  }
  return (o.uniform == 0) != o.frames.empty();
}

// Nuage synthetique des sondes du catalogue et de la tour : SplitMix64, positions distinctes, PointId = rang.
Outcome synthetic(const Options& o, MemoryBudget& budget, io::InputFiles& files) {
  Buffer<std::array<u32, 3>> points;
  MHGP12_TRY(points.allocate(o.uniform, budget));
  u64 state = o.seed;
  const u64 mask = (u64{1} << o.bits) - 1;
  for (u64 i = 0; i < o.uniform; ++i)
    for (int c = 0; c < 3; ++c) {
      state += 0x9E3779B97F4A7C15ull;
      u64 z = state;
      z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
      z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
      points[i][c] = static_cast<u32>((z ^ (z >> 31)) & mask);
    }
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

// Durees d'une passe (nanosecondes) : etages du mur, sous-etapes publiees, hors du mur.
struct PassTimes {
  u64 wall = 0, p = 0, c = 0, g = 0, junction = 0, tmvr = 0;
  u64 validation = 0, digest = 0, peak = 0;
};

// Etat d'une passe : tout ce que le mur construit ; detruit (liberation chronometree) apres la publication.
struct PassState {
  std::optional<GlobalIndex> index;
  std::optional<Catalogue> catalogue;
  std::optional<Resolution> resolution;
  std::optional<tower::TowerForests> forests;
  std::vector<tower::ForestInput> inputs;
  CatalogueDiagnostics cat_diag;
  ResolutionDiagnostics res_diag;
  tower::ForestLedger ledger;
};

// Le mur : de l'entree en memoire a la tour complete (verticales et registre).
Outcome run_wall(const Options& o, const Frame& f, MemoryBudget& budget, sched::Pool& pool, CatalogueDevice* device,
                 PassState& s, PassTimes& t) {
  const auto t0 = Clock::now();
  auto cloud = prepare_cloud(f.input.x.span(), f.input.y.span(), f.input.z.span(), f.input.ids.span(), CoordWidth(),
                             budget);
  if (!cloud.ok()) return cloud.outcome();
  auto index = build_index(std::move(cloud).take(), IndexParams{}, budget);
  if (!index.ok()) return index.outcome();
  s.index.emplace(std::move(index).take());
  t.p = since(t0);
  CatalogueParams params;
  params.kmax = o.kmax;
  params.leaf_size = o.leaf;
  auto c0 = Clock::now();
  auto catalogue = device != nullptr ? build_catalogue_device(s.index->cloud(), params, *device, pool, &s.cat_diag)
                                     : build_catalogue(s.index->cloud(), params, budget, pool, &s.cat_diag);
  if (!catalogue.ok()) return catalogue.outcome();
  s.catalogue.emplace(std::move(catalogue).take());
  t.c = since(c0);
  c0 = Clock::now();
  auto resolution = resolve_tower(*s.index, *s.catalogue, budget, pool, &s.res_diag);
  if (!resolution.ok()) return resolution.outcome();
  s.resolution.emplace(std::move(resolution).take());
  t.g = since(c0);
  c0 = Clock::now();
  for (Order k = 1; k <= s.resolution->orders(); ++k) s.inputs.push_back(tower::forest_input(s.resolution->order(k)));
  const tower::BallSource balls = tower::catalogue_balls(*s.catalogue);
  t.junction = since(c0);
  c0 = Clock::now();
  tower::ForestParams forest_params;
  auto forests = tower::build_forests(s.index->cloud(), balls, s.inputs, forest_params, budget, pool, &s.ledger);
  if (!forests.ok()) return forests.outcome();
  s.forests.emplace(std::move(forests).take());
  t.tmvr = since(c0);
  t.wall = since(t0);
  return {};
}

// Hors du mur : validation, empreinte FUL1 facultative.
Outcome after_wall(const Options& o, MemoryBudget& budget, PassState& s, PassTimes& t, std::string& digest) {
  auto c0 = Clock::now();
  MHGP12_TRY(tower::validate_forests(*s.forests, budget));
  t.validation = since(c0);
  if (!o.digest) return {};
  c0 = Clock::now();
  const tower::BallSource balls = tower::catalogue_balls(*s.catalogue);
  const tower::FullSource source{&s.index->cloud(), s.catalogue->levels(), balls, &*s.forests};
  u64 bytes = 0;
  auto d = tower::full_digest(source, &bytes);
  if (!d.ok()) return d.outcome();
  const auto hex = io::to_hex(d.value());
  digest.assign(hex.data(), hex.size());
  t.digest = since(c0);
  return {};
}

void print_pass(const Options& o, u64 pass, const Frame& f, const PassState& s, const PassTimes& t,
                const std::string& digest) {
  const CatalogueDiagnostics& c = s.cat_diag;
  const tower::ForestLedger& l = s.ledger;
  std::printf("{\"phase\":\"full\",\"pass\":%llu,\"trame\":\"%s\",\"voie\":\"%s\",\"status\":\"ok\",\"coord_bits\":%d,"
              "\"kmax\":%d,\"threads\":%u,\"sites\":%u,\"wall_ns\":%llu,\"etapes_ns\":{\"P\":%llu,\"C\":%llu,"
              "\"G\":%llu,\"raccord\":%llu,\"TMVR\":%llu,\"T\":%llu,\"M\":%llu,\"V\":%llu,\"R\":%llu},",
              (unsigned long long)pass, f.name.c_str(), o.device ? "device" : "cpu", kCoordBits, o.kmax, o.threads,
              s.index->cloud().sites(), (unsigned long long)t.wall, (unsigned long long)t.p, (unsigned long long)t.c,
              (unsigned long long)t.g, (unsigned long long)t.junction, (unsigned long long)t.tmvr,
              (unsigned long long)l.kernels_ns, (unsigned long long)l.contraction_ns,
              (unsigned long long)(l.vertical_births_ns + l.vertical_merges_ns), (unsigned long long)l.registry_ns);
  std::printf("\"c_ns\":{\"parcours\":%llu,\"feuilles\":%llu,\"emission\":%llu,\"fin_etage\":%llu,\"transferts\":%llu,"
              "\"publication\":%llu},\"g_ns\":{\"tables\":%llu,\"resolution\":%llu},\"hors_mur_ns\":{\"validation\":%llu,"
              "\"empreinte\":%llu},\"pic_octets\":%llu",
              (unsigned long long)c.traversal_ns, (unsigned long long)c.count_ns, (unsigned long long)c.fill_ns,
              (unsigned long long)(c.levels_ns + c.sort_ns + c.assemble_ns + c.table_ns),
              (unsigned long long)c.transfer_ns, (unsigned long long)c.publish_ns,
              (unsigned long long)s.res_diag.tables_ns, (unsigned long long)s.res_diag.resolve_ns,
              (unsigned long long)t.validation, (unsigned long long)t.digest, (unsigned long long)t.peak);
  if (o.digest) std::printf(",\"full_sha256\":\"%s\"", digest.c_str());
  std::printf("}\n");
  std::fflush(stdout);
}

Outcome passes(const Options& o, std::vector<Frame>& frames, MemoryBudget& budget, sched::Pool& pool,
               CatalogueDevice* device) {
  for (u64 pass = 0; pass < o.passes; ++pass) {
    const Frame& f = frames[pass % frames.size()];
    PassTimes t;
    std::string digest;
    budget.restart_peak();
    auto s = std::make_unique<PassState>();
    MHGP12_TRY(run_wall(o, f, budget, pool, device, *s, t));
    t.peak = budget.peak();
    MHGP12_TRY(after_wall(o, budget, *s, t, digest));
    print_pass(o, pass, f, *s, t, digest);
    const auto r0 = Clock::now();
    s.reset();  // liberation, hors du mur, publiee sur sa propre ligne
    std::printf("{\"phase\":\"liberation\",\"pass\":%llu,\"liberation_ns\":%llu}\n", (unsigned long long)pass,
                (unsigned long long)since(r0));
  }
  return {};
}

Outcome run(const Options& o) {
  MemoryBudget budget(o.budget, o.cache);
  auto pool = sched::make_pool(sched::PoolParams{o.threads});
  if (!pool.ok()) return pool.outcome();
  std::vector<Frame> frames;
  if (o.uniform != 0) {
    frames.push_back(Frame{"uniforme", {}});
    MHGP12_TRY(synthetic(o, budget, frames.back().input));
  }
  for (const FrameSpec& spec : o.frames) {
    auto input = io::read_u32le(spec.xyz.c_str(), spec.ids.c_str(), budget);
    if (!input.ok()) return input.outcome();
    frames.push_back(Frame{spec.name, std::move(input).take()});
  }
  if (!o.device) return passes(o, frames, budget, *pool.value(), nullptr);
  const auto t0 = Clock::now();
  auto device = CatalogueDevice::open(budget);
  std::printf("{\"phase\":\"open\",\"status\":\"%s\",\"reason\":\"%s\",\"wall_ns\":%llu}\n",
              std::string(status_name(device.outcome().status())).c_str(),
              std::string(reason_name(device.outcome().reason)).c_str(), (unsigned long long)since(t0));
  if (!device.ok()) return device.outcome();
  return passes(o, frames, budget, *pool.value(), &device.value());
}

}  // namespace

int main(int argc, char** argv) {
  Options o;
  if (!parse(argc, argv, o)) {
    std::fprintf(stderr, "usage : mhgp12_full_probe (--trame=<xyz>,<ids>[,NOM] ... | --uniform=N,GRAINE,BITS) "
                         "[--k=K] [--leaf=L] [--threads=W] [--passes=P] [--device] [--digest] [--budget=OCTETS] "
                         "[--cache=OCTETS]\n");
    return 2;
  }
  const Outcome outcome = guarded([&]() { return run(o); });
  std::printf("{\"phase\":\"exit\",\"status\":\"%s\",\"reason\":\"%s\"}\n",
              std::string(status_name(outcome.status())).c_str(), std::string(reason_name(outcome.reason)).c_str());
  return exit_code(outcome);
}
