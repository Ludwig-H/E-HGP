// Auditeur C, 23 septembre 2026 — differentiel moteur / lots du chemin q3/q4 S2 (audit, hors produit).
//
// Deux chaines sur les memes points : chemin moteur (q34_batch_filter = false) et chemin par lots, reference
// CPU (q34_batch_filter = true, sans GPU). Les deux catalogues complets sont compares champ par champ apres tri
// canonique : cle, niveau, arite, interieurs (ids tries), coquille (ids tries) ; puis les ordres et le condense
// FULL. Echec a la premiere difference, avec son rang. Si les catalogues sont egaux cle par cle, les verdicts
// des juges d'echantillon q2/q3 et d'Euler rendus sur le chemin moteur valent pour le chemin par lots.
//
//   batch_diff file <cut.u32le> <Kmax> <workers> [--inject=drop-one]
//   batch_diff family <uniform|terrain|clusters> <n> <Kmax> <workers> [--inject=drop-one]
// --inject=drop-one : retire la boule mediane du catalogue du chemin par lots avant comparaison (doit rendre 1).
//
// Code 0 egaux ; 1 difference ; 2 argument ou chaine incomplete.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "src/chain/tower_chain.hpp"
#include "tests/gen/front_fixtures.hpp"

using mhgp9::gen::Point3;
using mhgp9::tower::BallData;

namespace {

std::vector<Point3> read_u32le(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::invalid_argument("cannot open input");
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (bytes.size() % 12) throw std::invalid_argument("bad length");
  std::vector<Point3> pts(bytes.size() / 12);
  for (std::size_t i = 0; i < pts.size(); ++i) {
    std::uint32_t c[3];
    for (int a = 0; a < 3; ++a) {
      std::uint32_t v = 0;
      for (int b = 0; b < 4; ++b) v |= std::uint32_t(bytes[(3 * i + a) * 4 + b]) << (8 * b);
      if (v > 262143u) throw std::invalid_argument("coordinate outside 18 bits");
      c[a] = v;
    }
    pts[i] = Point3{(mhgp9::gen::Coordinate)c[0], (mhgp9::gen::Coordinate)c[1], (mhgp9::gen::Coordinate)c[2]};
  }
  return pts;
}

// Vue canonique d'une boule : cle, niveau, arite, interieurs et coquille tries.
struct Row {
  mhgp9::tower::BallKey key;
  mhgp9::tower::ExactLevel level;
  unsigned arity;
  std::vector<std::int32_t> interior, shell;
  bool operator<(const Row& o) const { return key < o.key; }
  bool operator==(const Row& o) const {
    return key == o.key && level == o.level && arity == o.arity && interior == o.interior && shell == o.shell;
  }
};

std::vector<Row> canonical(const std::vector<BallData>& cat) {
  std::vector<Row> rows;
  rows.reserve(cat.size());
  for (const auto& b : cat) {
    Row r{b.key, b.level, b.arity, {b.interior().begin(), b.interior().end()}, {b.shell().begin(), b.shell().end()}};
    std::sort(r.interior.begin(), r.interior.end());
    std::sort(r.shell.begin(), r.shell.end());
    rows.push_back(std::move(r));
  }
  std::sort(rows.begin(), rows.end());
  return rows;
}

mhgp9::ChainResult run(const std::vector<Point3>& pts, unsigned kmax, std::size_t workers, bool batch) {
  mhgp9::ChainOptions o;
  o.kmax = kmax;
  o.workers = workers;
  o.tower_static_threads = static_cast<int>(workers);
  o.keep_catalogue = true;
  o.run_tower = true;
  o.q34_batch_filter = batch;
  return mhgp9::run_tower_chain(pts, o);
}

bool g_drop_one = false;

int diff(const std::string& label, const std::vector<Point3>& pts, unsigned kmax, std::size_t workers) {
  const auto e = run(pts, kmax, workers, false);
  const auto b = run(pts, kmax, workers, true);
  if (e.status != mhgp9::ChainStatus::kComplete || b.status != mhgp9::ChainStatus::kComplete) {
    std::printf("%s kmax=%u engine=%s/%s batch=%s/%s\n", label.c_str(), kmax, mhgp9::chain_status_name(e.status),
                e.reason.c_str(), mhgp9::chain_status_name(b.status), b.reason.c_str());
    return 2;
  }
  const auto re = canonical(e.catalogue_balls);
  auto rb = canonical(b.catalogue_balls);
  if (g_drop_one && !rb.empty()) rb.erase(rb.begin() + static_cast<std::ptrdiff_t>(rb.size() / 2));
  std::size_t first = std::min(re.size(), rb.size());
  for (std::size_t i = 0; i < std::min(re.size(), rb.size()); ++i)
    if (!(re[i] == rb[i])) { first = i; break; }
  const bool same_catalogue = re.size() == rb.size() && first == re.size();
  bool same_orders = e.orders.size() == b.orders.size();
  for (std::size_t k = 0; same_orders && k < e.orders.size(); ++k) {
    const auto& x = e.orders[k];
    const auto& y = b.orders[k];
    same_orders = x.k == y.k && x.nodes == y.nodes && x.births == y.births && x.merges == y.merges &&
                  x.contributions == y.contributions && x.parents == y.parents;
  }
  const bool same_digest = e.tower_digest == b.tower_digest;
  const bool same_euler = e.catalogue.euler_by_k == b.catalogue.euler_by_k &&
                          e.catalogue.euler_status == b.catalogue.euler_status;
  std::printf("%s n=%zu kmax=%u balls_engine=%zu balls_batch=%zu first_difference=%s digest_engine=%016llx "
              "digest_batch=%016llx same_catalogue=%d same_orders=%d same_digest=%d same_euler=%d euler=%s "
              "batch_used=%d batch_backend=%s batch_rectangles=%llu batch_survivors=%llu\n",
              label.c_str(), pts.size(), kmax, re.size(), rb.size(),
              same_catalogue ? "none" : std::to_string(first).c_str(), (unsigned long long)e.tower_digest,
              (unsigned long long)b.tower_digest, same_catalogue ? 1 : 0, same_orders ? 1 : 0, same_digest ? 1 : 0,
              same_euler ? 1 : 0, mhgp9::euler_status_name(b.catalogue.euler_status), b.q34_batch.used ? 1 : 0,
              b.q34_batch.backend.c_str(), (unsigned long long)b.q34_batch.rectangles,
              (unsigned long long)b.q34_batch.survivors);
  if (!b.q34_batch.used) return 2;  // le chemin par lots n'a pas ete emprunte : comparaison vide
  return same_catalogue && same_orders && same_digest && same_euler ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    std::vector<std::string> a(argv + 1, argv + argc);
    if (!a.empty() && a.back() == "--inject=drop-one") { g_drop_one = true; a.pop_back(); }
    if (a.size() == 4 && a[0] == "file") {
      const auto slash = a[1].find_last_of('/');
      return diff(a[1].substr(slash == std::string::npos ? 0 : slash + 1), read_u32le(a[1]), std::stoul(a[2]),
                  std::stoul(a[3]));
    }
    if (a.size() == 5 && a[0] == "family") {
      const auto fx = mhgp9::gen::bench::make_front_fixture(std::stoul(a[2]), a[1], 3);
      return diff(a[1] + "_" + a[2], fx.points, std::stoul(a[3]), std::stoul(a[4]));
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "error: %s\n", e.what());
    return 2;
  }
  std::fprintf(stderr, "usage: batch_diff file <cut.u32le> <Kmax> <workers> | family <name> <n> <Kmax> <workers>\n");
  return 2;
}
