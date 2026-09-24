// Audit-only, one-block per-edge moment shadow on the 1,288-site S2 trace.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"  // main's implicit return vanishes when renamed.
#define main old_shadow_entry_point
#include "../moments_rectangle_shadow_20260924/shadow.cpp"
#undef main
#pragma GCC diagnostic pop

#include <sstream>
#include <string>
#include <filesystem>

static std::vector<std::string> fields(const std::string& line) {
  std::istringstream input(line);
  std::vector<std::string> result;
  for (std::string field; std::getline(input, field, '\t');) result.push_back(field);
  return result;
}

int main(int argc, char** argv) try {
  if (argc != 4 && argc != 5)
    throw std::runtime_error("usage: moment_trace quarter.u32le trace.tsv proof.tsv [palette=1|8|32]");
  const auto palette = argc == 5 ? std::stoull(argv[4]) : 1;
  if (palette != 1 && palette != 8 && palette != 32)
    throw std::runtime_error("palette must be 1, 8 or 32");
  if (std::filesystem::exists(argv[3]) || std::string(argv[3]) == argv[1] ||
      std::string(argv[3]) == argv[2])
    throw std::runtime_error("proof output already exists or aliases an input");
  std::ifstream file(argv[1], std::ios::binary);
  if (!file) throw std::runtime_error("cannot open points input");
  const std::vector<unsigned char> raw{std::istreambuf_iterator<char>(file), {}};
  if (raw.size() != 1288 * 12) throw std::runtime_error("requires exactly 1,288 sites");
  std::vector<Point3> points(raw.size() / 12);
  for (std::size_t j = 0; j < points.size(); ++j)
    points[j] = {Coordinate(load32(raw.data() + 12 * j)),
                 Coordinate(load32(raw.data() + 12 * j + 4)),
                 Coordinate(load32(raw.data() + 12 * j + 8))};
  const auto index = make_q2_cloud_index(prepare_cloud(points));
  const auto nodes = index->spatial_nodes();
  std::vector<std::pair<i64, std::size_t>> candidates;
  candidates.reserve(nodes.size());
  std::uint64_t candidate_tests = 0;
  std::ifstream trace(argv[2]);
  if (!trace) throw std::runtime_error("cannot open S2 trace");
  std::string line;
  if (!std::getline(trace, line) ||
      line != "s2_ordinal\trectangle_ordinal\tlocal_a\tlocal_b\traw_a\traw_b\ts2_mask\tF\tpost_core_mask\tpost_s3_mask")
    throw std::runtime_error("unexpected S2 trace schema");
  std::vector<std::uint8_t> proof;
  proof.reserve(55657);
  std::uint64_t input_f = 0, proved_f = 0, proved_edges = 0;
  while (std::getline(trace, line)) {
    const auto row = fields(line);
    if (row.size() != 10) throw std::runtime_error("bad S2 trace row");
    const auto ordinal = std::stoull(row[0]), a = std::stoull(row[2]), b = std::stoull(row[3]);
    const auto mask = std::stoul(row[6]);
    const auto f = std::stoull(row[7]);
    if (ordinal != proof.size() || a >= points.size() || b >= points.size() || a == b ||
        (mask != 2 && mask != 4 && mask != 6) || f < 2 || f > points.size())
      throw std::runtime_error("S2 ordinal, endpoint, mask or F invalid");
    const auto& pa = points[a];
    const auto& pb = points[b];
    const V av{pa.x, pa.y, pa.z}, bv{pb.x, pb.y, pb.z};
    std::uint8_t yes = 0;
    if (palette == 1) {
      const auto group = choose_group(*index, Box3{pa, pa}, Box3{pb, pb});
      const auto moment = make_moments(*index, group);
      yes = point_test(av, bv, moment, 10, static_cast<std::uint8_t>(mask));
      ++candidate_tests;
    } else {
      const std::array<i64, 3> target{2 * (i64(pa.x) + pb.x), 2 * (i64(pa.y) + pb.y),
                                      2 * (i64(pa.z) + pb.z)};
      candidates.clear();
      for (std::size_t node = 0; node < nodes.size(); ++node) {
        const auto size = nodes[node].range.size();
        if (size >= 8 && size <= 64)
          candidates.emplace_back(distance_box4(target, nodes[node].box), node);
      }
      if (candidates.size() < palette) throw std::runtime_error("too few candidate blocks");
      std::partial_sort(candidates.begin(), candidates.begin() + palette, candidates.end());
      for (std::size_t p = 0; p < palette && yes != mask; ++p) {
        const auto moment = make_moments(*index, candidates[p].second);
        yes |= point_test(av, bv, moment, 10,
                          static_cast<std::uint8_t>(mask & ~yes));
        ++candidate_tests;
      }
    }
    if ((yes & ~mask) != 0) throw std::runtime_error("moment proof widened lane mask");
    input_f += f;
    if (yes == mask) { ++proved_edges; proved_f += f; }
    proof.push_back(yes);
  }
  if (proof.size() != 55657 || input_f != 1151766)
    throw std::runtime_error("S2 trace survivor or F ledger changed");
  std::ofstream output(argv[3], std::ios::binary | std::ios::trunc);
  if (!output) throw std::runtime_error("cannot create proof output");
  output << "s2_ordinal\tproved_mask\n";
  for (std::size_t j = 0; j < proof.size(); ++j)
    output << j << '\t' << unsigned(proof[j]) << '\n';
  output.close();
  if (!output) throw std::runtime_error("proof output write failed");
  std::cout << "edges=" << proof.size() << " F=" << input_f << " palette=" << palette
            << " candidate_tests=" << candidate_tests
            << " fully_proved=" << proved_edges << " fully_proved_F=" << proved_f << '\n';
} catch (const std::exception& e) {
  std::cerr << e.what() << '\n';
  return 1;
}
