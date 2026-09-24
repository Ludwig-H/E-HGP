// Audit-only bounded palette of nearby spatial blocks for the exact
// multisite moment certificate. Reuse the previously audited arithmetic;
// the renamed entry point is intentionally never called.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"  // main's implicit return vanishes when renamed.
#define main old_shadow_entry_point
#include "../moments_rectangle_shadow_20260924/shadow.cpp"
#undef main
#pragma GCC diagnostic pop

#include <numeric>

int main(int argc, char** argv) try {
  if (argc != 4 && argc != 5)
    throw std::runtime_error("usage: palette points.u32le K min_product [min_group_sites]");
  const auto parsed_k = std::stoull(argv[2]);
  if (parsed_k < 4 || parsed_k > 10) throw std::runtime_error("K outside [4,10]");
  const unsigned k = static_cast<unsigned>(parsed_k);
  const std::uint64_t min_product = std::stoull(argv[3]);
  const std::size_t min_group = argc == 5 ? std::stoull(argv[4]) : k - 2;
  if (min_group < k - 2 || min_group > 64) throw std::runtime_error("min_group outside bounds");
  std::ifstream file(argv[1], std::ios::binary);
  if (!file) throw std::runtime_error("cannot open input");
  const std::vector<unsigned char> raw{std::istreambuf_iterator<char>(file), {}};
  if (raw.size() % 12) throw std::runtime_error("bad input length");
  std::vector<Point3> points(raw.size() / 12);
  for (std::size_t j = 0; j < points.size(); ++j)
    points[j] = {Coordinate(load32(raw.data() + 12 * j)),
                 Coordinate(load32(raw.data() + 12 * j + 4)),
                 Coordinate(load32(raw.data() + 12 * j + 8))};
  const auto index = make_q2_cloud_index(prepare_cloud(points));
  const auto nodes = index->spatial_nodes();
  constexpr std::array<std::size_t, 4> limits{1, 8, 32, 128};
  std::array<std::uint64_t, limits.size()> full{}, q3{}, q4{}, full_mass{};
  std::uint64_t front = 0, open = 0, selected = 0, selected_mass = 0;
  std::uint64_t candidates_checked = 0, distance_scans = 0;
  const auto start = std::chrono::steady_clock::now();
  const auto result = run_wspd_front(*index, k, 8, WspdFrontMode::MidpointSamples,
      [&](const WspdRectangle& r) {
        ++front;
        const auto& an = nodes[r.a_node];
        const auto& bn = nodes[r.b_node];
        Q34WitnessSearchWork work{};
        Q34WitnessBoundsWork bounds{};
        const std::uint8_t mask = filter_q34_witnesses(*index, an.box, bn.box, k,
            r.lane_mask, work, Q34WitnessBoundsMode::Affine, bounds);
        if (!mask) return;
        ++open;
        const auto mass = std::uint64_t(an.range.size()) * bn.range.size();
        if (mass < min_product) return;
        if (++selected > 10000) throw std::runtime_error("selected rectangle safety cap");
        selected_mass += mass;
        std::array<i64, 3> target{};
        for (int axis = 0; axis < 3; ++axis)
          target[axis] = i64(an.box.low[axis]) + an.box.high[axis] +
                         bn.box.low[axis] + bn.box.high[axis];
        std::vector<std::pair<i64, std::size_t>> candidates;
        candidates.reserve(nodes.size());
        for (std::size_t id = 0; id < nodes.size(); ++id) {
          const auto size = nodes[id].range.size();
          if (size < min_group || size > 64) continue;
          candidates.emplace_back(distance_box4(target, nodes[id].box), id);
        }
        if (candidates.size() < limits.back())
          throw std::runtime_error("fewer than 128 eligible blocks: no partial palette summary");
        distance_scans += nodes.size();
        std::sort(candidates.begin(), candidates.end());
        std::uint8_t certified = 0;
        for (std::size_t c = 0; c < std::min(limits.back(), candidates.size()); ++c) {
          const auto moment = make_moments(*index, candidates[c].second);
          const auto yes = all_corners(an.box, bn.box, moment, k, mask);
          ++candidates_checked;
          certified |= yes;
          for (std::size_t j = 0; j < limits.size(); ++j) {
            if (c + 1 != limits[j]) continue;
            q3[j] += bool(certified & 2);
            q4[j] += bool(certified & 4);
            if ((certified & mask) == mask) {
              ++full[j];
              full_mass[j] += mass;
            }
          }
          // Preserve cumulative results for larger palette widths even if
          // no additional candidate will be tested after a full closure.
        }
      }, 6);
  const auto wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  std::cout << "sites=" << points.size() << " K=" << k << " min_group=" << min_group << " front=" << front
            << " open=" << open << " selected=" << selected
            << " selected_mass=" << selected_mass << " distance_scans=" << distance_scans
            << " certificates=" << candidates_checked << " wall_s=" << wall
            << " front_check=" << result.work.emitted_rectangles << '\n';
  for (std::size_t j = 0; j < limits.size(); ++j)
    std::cout << "palette=" << limits[j] << " full=" << full[j] << " full_mass="
              << full_mass[j] << " q3=" << q3[j] << " q4=" << q4[j] << '\n';
} catch (const std::exception& e) {
  std::cerr << e.what() << '\n';
  return 1;
}
