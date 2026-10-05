// Sonde des deux voies de la sortie supports (livraison L2b, docs/SORTIES.md paragraphe 11), hors produit : pour une
// meme entree et un meme K, api_detail::compute_supports par la voie order_tree (build_order, masque 7035) puis par la
// voie full_tower (build_order_full : FULL au masque 16379, journal des graines sur l'ordre K, extraction de l'ordre
// K ; voie de compute), chacune dans une Session neuve de W fils, publiee (api::publish, provenance coherente sans
// empreintes) puis fermee (api::finish). Exige, octet pour octet, le meme supports.mhgp11sp et le meme manifeste par
// les deux voies, et les memes registres du journal (cellules, graines, empreinte FNV-1a 64 avant le balayage).
//
// Requetes (sans entree en option) : mhgp11_api_supports_route_probe --work=<dossier> [--workers=<W>]
//   sur l'entree standard, des requetes "K budget n" puis n lignes "x y z PointId" (format de
//   tests/tower/attach_fraction.py). Une ligne JSON par requete (cles triees, sans espace) : k, n, status ("ok" ou
//   "refus"), same (1 si les deux voies concordent : memes fichiers et registres, ou meme refus), et pour "ok" : file
//   et manifest (sha256), bytes, nodes, balls, supports, extended (boules a coquille etendue), multiple (boules a
//   plusieurs supports), cells, seeds, log (empreinte du journal) ; pour "refus" : reason.
// Echelle : mhgp11_api_supports_route_probe --work=<dossier> --k=<K> [--workers=<W>,...] [--min-balls=N]
//           [--min-cells=N] (--uniform18=<n>,<graine> | --data=<nom> | --input=<xyz.u32le>,<ids.u32le>)
//   uniform18 : random.Random(graine).getrandbits(18) de CPython, comme tests/supports/hierarchy_probe.cpp ; data : la
//   trame <nom> du dossier MHGP11_DATA_DIR (jamais recopiee). Les deux voies a chaque W ; fichiers et manifestes
//   identiques aussi entre les W. Une ligne JSON de mesure par (W, voie) : etages tree, attach, output et write en
//   nanosecondes et pics (diagnostic, aucune decision), puis la ligne de verdict, sans duree :
//     supports_route_empreintes fichier=<sha256, 16 premiers chiffres> manifeste=<idem> journal=<fnv1a64>
//     supports_route_verdict conforme k=<K> n=<n> noeuds=<N> boules=<B> supports=<S> etendues=<e> multiples=<m>
//       cellules=<c> graines=<g> fils=<W,...>
// A W1 (si W1 est demande), un troisieme calcul par l'appel public api::compute, sans diagnostic : memes fichier et
//   manifeste que full_tower, et pic comptable de l'etage tree egal a celui de full_tower et different de celui
//   d'order_tree (ligne JSON "route":"compute") ; observe la voie de production (relecture de L2b).
// Codes : 0 conforme ; 1 ecart entre les voies ou entre les W ; 2 refus (usage, entree, refus du produit en mode
// echelle) ; 3 plancher, ou invariant (Session non rendue).
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "../../bench/whole_input.hpp"
#include "api/internal.hpp"

using namespace mhgp11;
using namespace mhgp11::bench;

namespace {

constexpr int kConform = 0, kMismatch = 1, kRefusal = 2, kFloor = 3;

struct Options {
  std::string work, xyz, ids, data;
  u64 uniform = 0, seed = 0, k = 0, min_balls = 0, min_cells = 0;
  std::vector<u32> workers{1};
  bool scale = false;
};

bool parse_workers(std::string_view text, std::vector<u32>& out) {
  out.clear();
  while (!text.empty()) {
    const auto comma = text.find(',');
    u64 w = 0;
    if (!parse(text.substr(0, comma), w) || w == 0 || w > sched::kMaxWorkers) return false;
    if (std::find(out.begin(), out.end(), static_cast<u32>(w)) != out.end()) return false;
    out.push_back(static_cast<u32>(w));
    text = comma == std::string_view::npos ? std::string_view() : text.substr(comma + 1);
  }
  return !out.empty();
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
    if (value("--work=")) {
      o.work = std::string(v);
    } else if (value("--input=")) {
      const auto comma = v.find(',');
      if (comma == std::string_view::npos) return false;
      o.xyz = std::string(v.substr(0, comma));
      o.ids = std::string(v.substr(comma + 1));
      ++inputs;
    } else if (value("--data=")) {
      if (v.empty() || v.find('/') != std::string_view::npos) return false;
      o.data = std::string(v);
      ++inputs;
    } else if (value("--uniform18=")) {
      const auto comma = v.find(',');
      if (comma == std::string_view::npos || !parse(v.substr(0, comma), o.uniform) ||
          !parse(v.substr(comma + 1), o.seed) || o.uniform == 0 || o.uniform > 4000000 || o.seed > 0xFFFFFFFFull)
        return false;
      ++inputs;
    } else if (value("--k=")) {
      if (!parse(v, o.k)) return false;
    } else if (value("--workers=")) {
      if (!parse_workers(v, o.workers)) return false;
    } else if (value("--min-balls=")) {
      if (!parse(v, o.min_balls)) return false;
    } else if (value("--min-cells=")) {
      if (!parse(v, o.min_cells)) return false;
    } else {
      return false;
    }
  }
  if (o.work.empty()) return false;
  o.scale = inputs == 1;
  if (!o.scale) return inputs == 0 && o.k == 0 && o.workers.size() == 1 && o.min_balls == 0 && o.min_cells == 0;
  return o.k >= 1 && o.k <= api::kMaxOrder;
}

// random.Random(graine) de CPython (MT19937, init_by_array d'un mot), getrandbits(k <= 32) : port de
// tests/supports/hierarchy_probe.cpp ; memes nuages que les portes mhgp11_supports_hierarchy_scale*.
class PythonRandom {
 public:
  explicit PythonRandom(u32 seed) noexcept {
    mt_[0] = 19650218u;
    for (u32 i = 1; i < 624; ++i) mt_[i] = 1812433253u * (mt_[i - 1] ^ (mt_[i - 1] >> 30)) + i;
    u32 i = 1;
    for (u32 k = 624; k != 0; --k) {
      mt_[i] = (mt_[i] ^ ((mt_[i - 1] ^ (mt_[i - 1] >> 30)) * 1664525u)) + seed;
      if (++i >= 624) {
        mt_[0] = mt_[623];
        i = 1;
      }
    }
    for (u32 k = 623; k != 0; --k) {
      mt_[i] = (mt_[i] ^ ((mt_[i - 1] ^ (mt_[i - 1] >> 30)) * 1566083941u)) - i;
      if (++i >= 624) {
        mt_[0] = mt_[623];
        i = 1;
      }
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

Result<Input> read(const Options& o, MemoryBudget& budget) {
  if (o.uniform != 0) {
    Input input;
    MHGP11_TRY(input.x.allocate(o.uniform, budget));
    MHGP11_TRY(input.y.allocate(o.uniform, budget));
    MHGP11_TRY(input.z.allocate(o.uniform, budget));
    MHGP11_TRY(input.ids.allocate(o.uniform, budget));
    PythonRandom random(static_cast<u32>(o.seed));
    for (u64 i = 0; i < o.uniform; ++i) {
      input.x[i] = random.bits(18);
      input.y[i] = random.bits(18);
      input.z[i] = random.bits(18);
      input.ids[i] = make_id<PointId>(static_cast<u32>(i));
    }
    return input;
  }
  if (o.data.empty()) return read_input(o.xyz.c_str(), o.ids.c_str(), budget);
  const char* folder = std::getenv("MHGP11_DATA_DIR");
  if (folder == nullptr || *folder == '\0') return fail(Reason::input_unreadable);
  const std::string base = std::string(folder) + "/" + o.data;
  return read_input((base + ".u32le").c_str(), (base + ".ids.u32le").c_str(), budget);
}

std::string read_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::string sha_hex(const std::string& bytes) {
  io::Sha256 sha;
  sha.update(std::string_view(bytes));
  const auto hex = io::to_hex(sha.finish());
  return std::string(hex.data(), hex.size());
}

std::string hex64(u64 value) {
  std::string out(16, '0');
  for (int i = 15; i >= 0; --i, value >>= 4) out[static_cast<std::size_t>(i)] = "0123456789abcdef"[value & 15];
  return out;
}

// Une voie publiee : fichiers lus, comptes du produit, registres du journal, rapport d'etages, refus eventuel.
struct Published {
  Outcome outcome{};
  std::string file, manifest;
  u64 nodes = 0, balls = 0, supports = 0, extended = 0, multiple = 0;
  api_detail::SupportsDiagnostics diagnostics;
  api::RunReport report;
};

// route vide : voie de production, par l'appel public api::compute (sans diagnostic : journal non hache).
Published run_route(const api::CloudView& view, Order k, u32 workers, u64 budget,
                    std::optional<api_detail::SupportsRoute> route, const std::string& folder) {
  Published out;
  std::error_code error;
  std::filesystem::remove_all(folder, error);
  std::filesystem::create_directories(folder, error);
  Result<api::Session> made = api::Session::make({budget, workers});
  if (!made.ok()) {
    out.outcome = made.outcome();
    return out;
  }
  api::Session session = std::move(made).take();
  const std::string directory = folder + "/D";
  auto planned = io::OutputDirectory::plan(directory.c_str(), {});
  if (!planned.ok()) {
    out.outcome = planned.outcome();
    return out;
  }
  api::Provenance provenance;
  provenance.points_bytes = u64{12} * view.x.size();
  provenance.ids_bytes = u64{4} * view.x.size();
  api::Publication result{};
  {
    auto product = route ? api_detail::compute_supports(session, view, k, *route, &out.report, &out.diagnostics)
                         : api::compute(session, view, api::Request{api::SupportsRequest{k}}, &out.report);
    if (!product.ok()) {
      out.outcome = product.outcome();
      static_cast<void>(session.close());
      return out;
    }
    const auto& h = product.value().hierarchy();
    out.nodes = product.value().order_tree().forest().nodes().size();
    out.balls = h.balls().size();
    out.supports = h.supports().size();
    for (u64 b = 0; b < h.balls().size(); ++b) {
      if (h.balls()[b].m > h.balls()[b].qmin) ++out.extended;
      if (h.support_offsets()[b + 1] - h.support_offsets()[b] > 1) ++out.multiple;
    }
    result = api::publish(session, product.value(), planned.value(), provenance, &out.report);
  }
  if (result.ok()) result = api::finish(session, planned.value());
  out.outcome = result.outcome;
  if (!result.ok()) return out;
  out.file = read_file(directory + "/" + std::string(api::kSupportsFileName));
  out.manifest = read_file(directory + "/manifeste.json");
  std::filesystem::remove_all(folder, error);
  return out;
}

bool same(const Published& a, const Published& b) {
  if (!a.outcome.ok() || !b.outcome.ok())
    return !a.outcome.ok() && !b.outcome.ok() && a.outcome.reason == b.outcome.reason;
  return a.file == b.file && a.manifest == b.manifest && a.diagnostics.log == b.diagnostics.log && a.nodes == b.nodes &&
         a.balls == b.balls && a.supports == b.supports && !a.file.empty() && !a.manifest.empty();
}

std::string reason_text(const Outcome& outcome) {
  const auto name = reason_name(outcome.reason);
  return std::string(name.data(), name.size());
}

// ------------------------------------------------------------------------------------------------ requetes

int requests(const Options& o) {
  u32 workers = o.workers.front();
  u64 k = 0, budget = 0, n = 0, index = 0;
  bool all = true;
  while (std::cin >> k >> budget >> n) {
    std::vector<u32> x(n), y(n), z(n);
    std::vector<PointId> ids(n);
    for (u64 i = 0; i < n; ++i) {
      u64 a = 0, b = 0, c = 0, d = 0;
      if (!(std::cin >> a >> b >> c >> d) || a > 0xFFFFFFFFull || b > 0xFFFFFFFFull || c > 0xFFFFFFFFull ||
          d > 0xFFFFFFFFull) {
        std::fprintf(stderr, "requete %llu illisible\n", static_cast<unsigned long long>(index));
        return kRefusal;
      }
      x[i] = static_cast<u32>(a); y[i] = static_cast<u32>(b); z[i] = static_cast<u32>(c);
      ids[i] = make_id<PointId>(static_cast<u32>(d));
    }
    const api::CloudView view{x, y, z, ids};
    const std::string base = o.work + "/q" + std::to_string(index++);
    const Published a = run_route(view, static_cast<Order>(k), workers, budget, api_detail::SupportsRoute::order_tree,
                                  base + "_a");
    const Published b = run_route(view, static_cast<Order>(k), workers, budget, api_detail::SupportsRoute::full_tower,
                                  base + "_b");
    const bool agree = same(a, b);
    all = all && agree;
    std::ostringstream line;
    line << "{\"k\":" << k << ",\"n\":" << n;
    if (!a.outcome.ok() || !b.outcome.ok()) {
      line << ",\"reason\":\"" << reason_text(a.outcome.ok() ? b.outcome : a.outcome) << "\",\"same\":" << agree
           << ",\"status\":\"refus\"}";
    } else {
      line << ",\"balls\":" << b.balls << ",\"bytes\":" << b.file.size() << ",\"cells\":" << b.diagnostics.log.cells
           << ",\"extended\":" << b.extended << ",\"file\":\"" << sha_hex(b.file) << "\",\"log\":\""
           << hex64(b.diagnostics.log.digest) << "\",\"manifest\":\"" << sha_hex(b.manifest)
           << "\",\"multiple\":" << b.multiple << ",\"nodes\":" << b.nodes << ",\"same\":" << agree
           << ",\"seeds\":" << b.diagnostics.log.seeds << ",\"status\":\"ok\",\"supports\":" << b.supports << "}";
    }
    std::cout << line.str() << "\n";
  }
  std::cout.flush();
  return all ? kConform : kMismatch;
}

// ------------------------------------------------------------------------------------------------ echelle

void measure(const char* route, u32 workers, const Published& p) {
  std::printf("{\"route\":\"%s\",\"workers\":%u", route, workers);
  for (api::Stage stage : {api::Stage::tree, api::Stage::attach, api::Stage::output, api::Stage::write}) {
    const auto name = api::stage_name(stage);
    std::printf(",\"%.*s_ns\":%llu,\"%.*s_peak\":%llu", static_cast<int>(name.size()), name.data(),
                static_cast<unsigned long long>(p.report.at(stage).nanoseconds), static_cast<int>(name.size()),
                name.data(), static_cast<unsigned long long>(p.report.at(stage).peak_bytes));
  }
  std::printf("}\n");
}

int scale(const Options& o) {
  MemoryBudget owner(MemoryBudget::kUnlimited);
  auto input = read(o, owner);
  if (!input.ok()) {
    std::printf("supports_route_verdict refus %s\n", reason_text(input.outcome()).c_str());
    return kRefusal;
  }
  const api::CloudView view{input.value().x.span(), input.value().y.span(), input.value().z.span(),
                            input.value().ids.span()};
  std::optional<Published> first;
  bool agree = true;
  std::string workers_text;
  for (u32 w : o.workers) {
    workers_text += (workers_text.empty() ? "" : ",") + std::to_string(w);
    const std::string base = o.work + "/w" + std::to_string(w);
    Published a = run_route(view, static_cast<Order>(o.k), w, MemoryBudget::kUnlimited,
                            api_detail::SupportsRoute::order_tree, base + "_a");
    Published b = run_route(view, static_cast<Order>(o.k), w, MemoryBudget::kUnlimited,
                            api_detail::SupportsRoute::full_tower, base + "_b");
    if (!a.outcome.ok() || !b.outcome.ok()) {
      std::printf("supports_route_verdict refus %s W%u\n", reason_text(a.outcome.ok() ? b.outcome : a.outcome).c_str(),
                  w);
      return a.outcome.reason == Reason::budget_not_released || b.outcome.reason == Reason::budget_not_released
                 ? kFloor : kRefusal;
    }
    measure("order_tree", w, a);
    measure("full_tower", w, b);
    if (!same(a, b)) {
      std::printf("ECART W%u : voies differentes (fichier %d, manifeste %d, journal %d)\n", w, a.file == b.file,
                  a.manifest == b.manifest, a.diagnostics.log == b.diagnostics.log);
      agree = false;
    }
    if (first && !same(*first, b)) {
      std::printf("ECART W%u : sorties differentes de W%u\n", w, o.workers.front());
      agree = false;
    }
    if (w == 1) {
      // Voie de production observee : a W1, le pic comptable de l'etage tree est deterministe et distingue FULL
      // (masque 16379, tous les ordres) de l'arbre d'ordre K seul. compute doit suivre full_tower (regle de L2).
      Published c = run_route(view, static_cast<Order>(o.k), w, MemoryBudget::kUnlimited, std::nullopt, base + "_c");
      const u64 peak_a = a.report.at(api::Stage::tree).peak_bytes, peak_b = b.report.at(api::Stage::tree).peak_bytes,
                peak_c = c.report.at(api::Stage::tree).peak_bytes;
      std::printf("{\"route\":\"compute\",\"tree_peak\":%llu,\"order_tree_peak\":%llu,\"full_tower_peak\":%llu}\n",
                  static_cast<unsigned long long>(peak_c), static_cast<unsigned long long>(peak_a),
                  static_cast<unsigned long long>(peak_b));
      if (!c.outcome.ok() || c.file != b.file || c.manifest != b.manifest || peak_a == peak_b || peak_c != peak_b) {
        std::printf("ECART W1 : la voie de compute n'est pas full_tower (fichier %d, pics %d/%d)\n", c.file == b.file,
                    peak_c == peak_b, peak_a != peak_b);
        agree = false;
      }
    }
    if (!first) first.emplace(std::move(b));
  }
  if (!agree) return kMismatch;
  const Published& p = *first;
  if (p.balls < o.min_balls || p.diagnostics.log.cells < o.min_cells) {
    std::printf("PLANCHER boules=%llu cellules=%llu\n", static_cast<unsigned long long>(p.balls),
                static_cast<unsigned long long>(p.diagnostics.log.cells));
    return kFloor;
  }
  // Empreintes a part : MHGP11SP et son manifeste portent coord_bits, elles changent avec le profil ; la ligne de
  // verdict ne grave que des comptes, communs aux trois profils (qualification G4 du 5 octobre 2026).
  std::printf("supports_route_empreintes fichier=%s manifeste=%s journal=%s\n", sha_hex(p.file).substr(0, 16).c_str(),
              sha_hex(p.manifest).substr(0, 16).c_str(), hex64(p.diagnostics.log.digest).c_str());
  std::printf("supports_route_verdict conforme k=%llu n=%zu noeuds=%llu boules=%llu supports=%llu etendues=%llu "
              "multiples=%llu cellules=%llu graines=%llu fils=%s\n",
              static_cast<unsigned long long>(o.k), view.x.size(), static_cast<unsigned long long>(p.nodes),
              static_cast<unsigned long long>(p.balls), static_cast<unsigned long long>(p.supports),
              static_cast<unsigned long long>(p.extended), static_cast<unsigned long long>(p.multiple),
              static_cast<unsigned long long>(p.diagnostics.log.cells),
              static_cast<unsigned long long>(p.diagnostics.log.seeds), workers_text.c_str());
  return kConform;
}

}  // namespace

int main(int argc, char** argv) {
  Options o;
  if (!parse_options(argc, argv, o)) {
    std::fprintf(stderr,
                 "usage : mhgp11_api_supports_route_probe --work=<dossier> [--workers=<W>] (requetes sur l'entree "
                 "standard)\n        mhgp11_api_supports_route_probe --work=<dossier> --k=<K> [--workers=<W>,...] "
                 "[--min-balls=N] [--min-cells=N] (--uniform18=<n>,<graine> | --data=<nom> | --input=<xyz>,<ids>)\n");
    return kRefusal;
  }
  std::error_code error;
  std::filesystem::create_directories(o.work, error);
  return o.scale ? scale(o) : requests(o);
}
