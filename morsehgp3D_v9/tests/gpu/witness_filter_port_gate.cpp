// MorseHGP3D v9 — porte du port portable du filtre temoin q3/q4
// (src/gpu/witness_filter.hpp), compile ici pour l'hote.
//
// Pour trois familles de nuages (8 000 sites par defaut), K3, K5 et K10,
// s = 8 : le front WSPD q3/q4 de la chaine (MidpointSamples, voies 6) emet
// ses rectangles ; chaque rectangle est filtre par le produit
// (filter_q34_witnesses, bornes Affine) et par le port. Les rectangles
// survivants sont developpes en paires, echantillonnees par pas fixe ; chaque
// paire est filtree par le produit (surcharge points, sans cache) et par le
// port. Les masques doivent etre egaux requete par requete, et le port doit
// visiter exactement les memes noeuds que le produit (meme DFS, meme ordre).
//
// Non-vacuite : planchers de rectangles, de paires, de voies rejetees et
// survivantes ; temoin de sensibilite (le port a K-1 doit diverger).
//
// Garde d'entree brute (validate_filter_input, audit A « domaine u18 ») :
// l'index reel de chaque famille est accepte ; l'index minimal de A (a =
// (0,0,0), b = (2,0,0), deux feuilles) est accepte et garde son masque q4 ;
// sa racine forgee x = [1,1] est refusee, alors que le filtre y rendrait le
// faux rejet 4 -> 0 ; coordonnees -1, 262144, INT32_MIN, INT32_MAX et partition
// cassee refusees, extremes 0 et 262143 acceptes.
//
//   mhgp9_gpu_witness_filter_port_gate [--n=8000] [--pair-stride=17]
//
// Code 0 conforme, 1 masque different (`cause=`), 2 argument, 3 plancher.
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "../../src/gen/lanes/q34_witness_search.hpp"
#include "../../src/gen/pipeline/q2_census.hpp"
#include "../../src/gen/wspd/front.hpp"
#include "../../src/gpu/filter_runner.hpp"
#include "../../src/gpu/flat_index.hpp"
#include "../gen/front_fixtures.hpp"

namespace {

bool parse(std::string_view digits, std::size_t& value) {
  const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), value);
  return !digits.empty() && error == std::errc{} && end == digits.data() + digits.size();
}

struct Totals {
  std::uint64_t rectangles = 0, rectangle_survivors = 0, pairs = 0, pair_survivors = 0;
  std::uint64_t lanes_rejected = 0, lanes_open = 0, affine_rectangles = 0, general_rectangles = 0;
  std::uint64_t port_visits = 0, product_visits = 0, sensitivity = 0, mismatches = 0;
};

}  // namespace

// A's three-node fixture; `root_x` forges the root box as x = [root_x, root_x]
// (negative: the certified hull [xa, xb]).
struct Fixture {
  std::vector<mhgp9::gpu::FlatNode> nodes;
  std::vector<std::int32_t> points;
  std::vector<mhgp9::gpu::u32> a{1}, b{2};
  std::vector<mhgp9::gpu::u8> mask{4};
  mhgp9::gpu::FilterInput input() const {
    mhgp9::gpu::FilterInput in;
    in.nodes = nodes.data();
    in.node_count = nodes.size();
    in.rank_points = points.data();
    in.rank_count = points.size() / 3;
    in.rect_a = a.data();
    in.rect_b = b.data();
    in.rect_mask = mask.data();
    in.rect_count = a.size();
    in.kmax = 3;
    return in;
  }
};

Fixture fixture(std::int32_t xa, std::int32_t xb, std::int32_t root_x = -1) {
  using mhgp9::gpu::FlatNode;
  Fixture f;
  f.points = {xa, 0, 0, xb, 0, 0};
  const std::int32_t low = root_x < 0 ? xa : root_x, high = root_x < 0 ? xb : root_x;
  f.nodes = {FlatNode{{{low, 0, 0}, {high, 0, 0}}, 1, 2, 0, 2},
             FlatNode{{{xa, 0, 0}, {xa, 0, 0}}, mhgp9::gpu::absent32, mhgp9::gpu::absent32, 0, 1},
             FlatNode{{{xb, 0, 0}, {xb, 0, 0}}, mhgp9::gpu::absent32, mhgp9::gpu::absent32, 1, 2}};
  return f;
}

// Code 1 on a wrong acceptance or refusal, 0 when every case holds.
int input_guard_cases(std::uint64_t& checked) {
  using namespace mhgp9;
  const auto accepted = [&](const Fixture& f) { ++checked; return gpu::validate_filter_input(f.input()).empty(); };
  const auto good = fixture(0, 2);
  std::uint64_t visits = 0;
  if (!accepted(good) || gpu::filter_boxes(good.nodes.data(), good.nodes[1].box, good.nodes[2].box, 3, 4, visits) != 4) {
    std::printf("cause=guard.certified_fixture\n");
    return 1;
  }
  const auto forged = fixture(0, 2, 1);
  if (accepted(forged) ||
      gpu::filter_boxes(forged.nodes.data(), forged.nodes[1].box, forged.nodes[2].box, 3, 4, visits) != 0) {
    std::printf("cause=guard.forged_root_box\n");  // refused, and the refusal matters (false q4 rejection)
    return 1;
  }
  for (const std::int32_t bad : {-1, 262144, std::numeric_limits<std::int32_t>::min(),
                                 std::numeric_limits<std::int32_t>::max()}) {
    auto f = fixture(0, 2);
    f.points[1] = bad;
    if (accepted(f)) { std::printf("cause=guard.coordinate value=%d\n", bad); return 1; }
    f = fixture(0, 2);
    f.nodes[0].box.high[1] = bad;
    if (bad >= 0 && accepted(f)) { std::printf("cause=guard.box_bound value=%d\n", bad); return 1; }
  }
  if (!accepted(fixture(0, 262143))) { std::printf("cause=guard.domain_extremes\n"); return 1; }
  auto broken = fixture(0, 2);
  broken.nodes[1].last = 2;  // left child overlaps the right one
  if (accepted(broken)) { std::printf("cause=guard.partition\n"); return 1; }
  return 0;
}

int main(int argc, char** argv) {
  std::size_t n = 8000, stride = 17;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (arg.starts_with("--n=")) {
      if (!parse(arg.substr(4), n)) return 2;
    } else if (arg.starts_with("--pair-stride=")) {
      if (!parse(arg.substr(14), stride)) return 2;
    } else return 2;
  }
  if (n < 64 || n > 65536 || stride == 0) {
    std::fprintf(stderr, "usage: mhgp9_gpu_witness_filter_port_gate [--n=8000] [--pair-stride=17]\n");
    return 2;
  }
  using namespace mhgp9;
  std::uint64_t guard_checks = 0;
  if (input_guard_cases(guard_checks) != 0) return 1;
  Totals all;
  for (const std::string_view family : {"uniform", "terrain", "clusters"}) {
    const auto fixture = gen::bench::make_front_fixture(n, family, 3);
    const auto cloud = gen::prepare_cloud(fixture.points);
    const auto index = gen::make_q2_cloud_index(cloud);
    const auto flat = gpu::flatten_nodes(*index);
    const auto nodes = index->spatial_nodes();
    const auto order = index->spatial_order();
    const auto points = index->cloud().points();
    {
      // The engine's certified index passes the raw-input guard.
      std::vector<std::int32_t> rank_points(3 * order.size());
      for (std::size_t r = 0; r < order.size(); ++r) {
        rank_points[3 * r] = points[order[r]].x;
        rank_points[3 * r + 1] = points[order[r]].y;
        rank_points[3 * r + 2] = points[order[r]].z;
      }
      gpu::FilterInput in;
      in.nodes = flat.data();
      in.node_count = flat.size();
      in.rank_points = rank_points.data();
      in.rank_count = order.size();
      in.kmax = 5;
      ++guard_checks;
      if (const auto error = gpu::validate_filter_input(in); !error.empty()) {
        std::printf("cause=guard.real_index family=%s error=%s\n", std::string(family).c_str(), error.c_str());
        return 1;
      }
    }
    for (const unsigned kmax : {3u, 5u, 10u}) {
      std::vector<gen::WspdRectangle> rectangles;
      const auto front = gen::run_wspd_front(*index, kmax, 8, gen::WspdFrontMode::MidpointSamples,
          [&](const gen::WspdRectangle& r) { rectangles.push_back(r); }, 6);
      static_cast<void>(front);
      Totals t;
      std::uint64_t pair_counter = 0;
      for (const auto& r : rectangles) {
        ++t.rectangles;
        const auto& an = nodes[r.a_node];
        const auto& bn = nodes[r.b_node];
        gen::Q34WitnessSearchWork work;
        gen::Q34WitnessBoundsWork bounds;
        const auto product = gen::filter_q34_witnesses(*index, an.box, bn.box, static_cast<std::uint8_t>(kmax),
            r.lane_mask, work, gen::Q34WitnessBoundsMode::Affine, bounds);
        std::uint64_t visits = 0;
        const auto fa = gpu::flat_box(an.box), fb = gpu::flat_box(bn.box);
        const auto port = gpu::filter_boxes(flat.data(), fa, fb, kmax, r.lane_mask, visits);
        t.port_visits += visits;
        t.product_visits += work.node_visits;
        const bool singleton = an.box.low == an.box.high && bn.box.low == bn.box.high;
        ++(singleton ? t.affine_rectangles : t.general_rectangles);
        if (port != product) {
          std::printf("cause=rectangle.mask family=%s kmax=%u a=%zu b=%zu product=%u port=%u\n",
                      std::string(family).c_str(), kmax, r.a_node, r.b_node, product, port);
          return 1;
        }
        std::uint64_t unused = 0;
        if (gpu::filter_boxes(flat.data(), fa, fb, kmax - 1, r.lane_mask, unused) != product) ++t.sensitivity;
        if (product == 0) continue;
        ++t.rectangle_survivors;
        for (auto ai = an.range.first; ai < an.range.last; ++ai)
          for (auto bi = bn.range.first; bi < bn.range.last; ++bi) {
            if (pair_counter++ % stride != 0) continue;
            ++t.pairs;
            const auto& pa = points[order[ai]];
            const auto& pb = points[order[bi]];
            gen::Q34WitnessSearchWork pw;
            gen::Q34WitnessBoundsWork pbw;
            std::vector<gen::Q34WitnessNode> trace;
            const auto pair_product = gen::filter_q34_witnesses(*index, pa, pb, static_cast<std::uint8_t>(kmax),
                                                                product, pw, pbw, trace);
            std::uint64_t pv = 0;
            const auto pair_port = gpu::filter_boxes(flat.data(), gpu::flat_point(pa), gpu::flat_point(pb), kmax,
                                                     product, pv);
            t.port_visits += pv;
            t.product_visits += pw.node_visits;
            if (pair_port != pair_product) {
              std::printf("cause=pair.mask family=%s kmax=%u a=%zu b=%zu product=%u port=%u\n",
                          std::string(family).c_str(), kmax, order[ai], order[bi], pair_product, pair_port);
              return 1;
            }
            for (const unsigned bit : {2u, 4u}) {
              if ((product & bit) == 0) continue;
              ++((pair_product & bit) != 0 ? t.lanes_open : t.lanes_rejected);
            }
            if (pair_product != 0) ++t.pair_survivors;
          }
      }
      // Same DFS, same order: the port pops exactly the nodes the product visits.
      if (t.port_visits != t.product_visits) {
        std::printf("cause=visits family=%s kmax=%u product=%llu port=%llu\n", std::string(family).c_str(), kmax,
                    static_cast<unsigned long long>(t.product_visits), static_cast<unsigned long long>(t.port_visits));
        return 1;
      }
      std::printf("port family=%s kmax=%u rectangles=%llu affine=%llu general=%llu survivors=%llu pairs=%llu "
                  "pair_survivors=%llu lanes_rejected=%llu lanes_open=%llu sensitivity=%llu visits=%llu\n",
                  std::string(family).c_str(), kmax, static_cast<unsigned long long>(t.rectangles),
                  static_cast<unsigned long long>(t.affine_rectangles),
                  static_cast<unsigned long long>(t.general_rectangles),
                  static_cast<unsigned long long>(t.rectangle_survivors), static_cast<unsigned long long>(t.pairs),
                  static_cast<unsigned long long>(t.pair_survivors),
                  static_cast<unsigned long long>(t.lanes_rejected), static_cast<unsigned long long>(t.lanes_open),
                  static_cast<unsigned long long>(t.sensitivity), static_cast<unsigned long long>(t.port_visits));
      all.rectangles += t.rectangles; all.rectangle_survivors += t.rectangle_survivors; all.pairs += t.pairs;
      all.pair_survivors += t.pair_survivors; all.lanes_rejected += t.lanes_rejected;
      all.lanes_open += t.lanes_open; all.affine_rectangles += t.affine_rectangles;
      all.general_rectangles += t.general_rectangles; all.sensitivity += t.sensitivity;
    }
  }
  std::printf("witness_filter_port_gate guard_checks=%llu\n", static_cast<unsigned long long>(guard_checks));
  std::printf("witness_filter_port_gate n=%zu rectangles=%llu survivors=%llu pairs=%llu lanes_rejected=%llu "
              "lanes_open=%llu affine=%llu general=%llu sensitivity=%llu\n", n,
              static_cast<unsigned long long>(all.rectangles), static_cast<unsigned long long>(all.rectangle_survivors),
              static_cast<unsigned long long>(all.pairs), static_cast<unsigned long long>(all.lanes_rejected),
              static_cast<unsigned long long>(all.lanes_open), static_cast<unsigned long long>(all.affine_rectangles),
              static_cast<unsigned long long>(all.general_rectangles), static_cast<unsigned long long>(all.sensitivity));
  if (all.rectangles < 10000 || all.rectangle_survivors < 1000 || all.pairs < 10000 || all.lanes_rejected < 1000 ||
      all.lanes_open < 1000 || all.affine_rectangles == 0 || all.general_rectangles < 1000 || all.sensitivity < 100) {
    std::printf("cause=floor.port\n");
    return 3;
  }
  return 0;
}
