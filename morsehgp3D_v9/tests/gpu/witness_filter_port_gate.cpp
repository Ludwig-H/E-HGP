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
//   mhgp9_gpu_witness_filter_port_gate [--n=8000] [--pair-stride=17]
//
// Code 0 conforme, 1 masque different (`cause=`), 2 argument, 3 plancher.
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "../../src/gen/lanes/q34_witness_search.hpp"
#include "../../src/gen/pipeline/q2_census.hpp"
#include "../../src/gen/wspd/front.hpp"
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
  Totals all;
  for (const std::string_view family : {"uniform", "terrain", "clusters"}) {
    const auto fixture = gen::bench::make_front_fixture(n, family, 3);
    const auto cloud = gen::prepare_cloud(fixture.points);
    const auto index = gen::make_q2_cloud_index(cloud);
    const auto flat = gpu::flatten_nodes(*index);
    const auto nodes = index->spatial_nodes();
    const auto order = index->spatial_order();
    const auto points = index->cloud().points();
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
