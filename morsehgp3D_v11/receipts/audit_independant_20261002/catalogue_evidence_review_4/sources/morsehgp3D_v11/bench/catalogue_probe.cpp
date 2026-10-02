// Banc CPU du catalogue sur l'entree ENTIERE u32 little-endian, avec PointId externes preserves.
// Hors produit : format de preuve binaire explicite, hache ensuite par le pilote Python.
#include <algorithm>
#include <array>
#include <charconv>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string_view>

#include "catalogue/catalogue.hpp"

using namespace mhgp11;

namespace {
struct Input {
  Buffer<u32> x, y, z;
  Buffer<PointId> ids;
};

bool parse(std::string_view text, u64& value) {
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  return error == std::errc{} && end == text.data() + text.size();
}

u32 little(const unsigned char* p) {
  return u32{p[0]} | (u32{p[1]} << 8) | (u32{p[2]} << 16) | (u32{p[3]} << 24);
}

Result<Input> read_input(const char* xyz, const char* ids, MemoryBudget& budget) {
  std::ifstream points(xyz, std::ios::binary | std::ios::ate), identities(ids, std::ios::binary | std::ios::ate);
  if (!points || !identities) return fail(Reason::input_unreadable);
  const auto bytes = points.tellg(), id_bytes = identities.tellg();
  if (bytes <= 0 || bytes % 12 != 0 || id_bytes < 0 || id_bytes != bytes / 3)
    return fail(Reason::input_unreadable);
  const u64 count = static_cast<u64>(bytes / 12);
  if (count >= kNone) return fail(Reason::index_overflow_u32);
  Input input;
  MHGP11_TRY(input.x.allocate(count, budget));
  MHGP11_TRY(input.y.allocate(count, budget));
  MHGP11_TRY(input.z.allocate(count, budget));
  MHGP11_TRY(input.ids.allocate(count, budget));
  points.seekg(0); identities.seekg(0);
  std::array<unsigned char, 12 * 4096> block{};
  std::array<unsigned char, 4 * 4096> names{};
  for (u64 begin = 0; begin < count; begin += 4096) {
    const u64 n = std::min(u64{4096}, count - begin);
    points.read(reinterpret_cast<char*>(block.data()), static_cast<std::streamsize>(12 * n));
    identities.read(reinterpret_cast<char*>(names.data()), static_cast<std::streamsize>(4 * n));
    if (!points || !identities) return fail(Reason::input_unreadable);
    for (u64 j = 0; j < n; ++j) {
      input.x[begin + j] = little(block.data() + 12 * j);
      input.y[begin + j] = little(block.data() + 12 * j + 4);
      input.z[begin + j] = little(block.data() + 12 * j + 8);
      input.ids[begin + j] = make_id<PointId>(little(names.data() + 4 * j));
    }
  }
  return input;
}

void word(std::ostream& out, u64 value) {
  std::array<char, 8> bytes{};
  for (unsigned i = 0; i < 8; ++i) bytes[i] = static_cast<char>((value >> (8 * i)) & 255);
  out.write(bytes.data(), bytes.size());
}

template <class T>
void integer(std::ostream& out, const T& value) {
  const auto wide = num::to_wide(value);
  word(out, wide.neg ? 1 : 0);
  word(out, wide.words.size());
  for (u64 w : wide.words) word(out, w);
}

Outcome serialize(const char* path, const Cloud& cloud, const Catalogue& catalogue) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) return fail(Reason::output_unwritable);
  out.write("MHGP11CAT1", 10);
  word(out, kCoordBits); word(out, catalogue.kmax()); word(out, cloud.sites());
  for (u32 i = 0; i < cloud.sites(); ++i) {
    word(out, cloud.x()[i]); word(out, cloud.y()[i]); word(out, cloud.z()[i]);
    word(out, cloud.w()[i]);
    for (PointId id : cloud.points(make_id<SiteIdx>(i))) word(out, idx(id));
  }
  word(out, catalogue.levels().size());
  for (const auto& level : catalogue.levels()) {
    integer(out, level.numerator()); integer(out, level.denominator());
  }
  word(out, catalogue.balls());
  for (const auto& ball : catalogue.balls_data()) {
    word(out, ball.qmin); word(out, ball.p); word(out, ball.m); word(out, idx(ball.rank));
    for (SiteIdx s : ball.support) word(out, idx(s));
  }
  for (u64 offset : catalogue.population_offsets()) word(out, offset);
  for (SiteIdx site : catalogue.population()) word(out, idx(site));
  out.close();
  return out ? Outcome{} : fail(Reason::output_unwritable);
}

void status(const Outcome& out) {
  std::cout << "\"status\":\"" << status_name(out.status()) << "\",\"reason\":\""
            << reason_name(out.reason) << '"';
}

Outcome run(char** argv, const CatalogueParams& params, u64 bytes) {
  MemoryBudget budget(bytes);
  Stopwatch read_clock;
  auto input = read_input(argv[1], argv[2], budget);
  const u64 read_ns = read_clock.nanoseconds();
  if (!input.ok()) return input.outcome();
  Stopwatch cloud_clock;
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                             input.value().ids.span(), CoordWidth(), budget);
  const u64 cloud_ns = cloud_clock.nanoseconds(), cloud_peak = budget.peak();
  if (!cloud.ok()) return cloud.outcome();
  input.value() = {};
  std::cout << "{\"phase\":\"cloud\",\"read_ns\":" << read_ns << ",\"cloud_ns\":" << cloud_ns
            << ",\"points\":" << cloud.value().weight() << ",\"sites\":" << cloud.value().sites()
            << ",\"cloud_peak_bytes\":" << cloud_peak << "}\n" << std::flush;
  budget.restart_peak();
  const auto cpu_start = std::clock();
  Stopwatch watch;
  auto catalogue = build_catalogue(cloud.value(), params, budget);
  const u64 catalogue_ns = watch.nanoseconds();
  const double cpu_seconds = double(std::clock() - cpu_start) / CLOCKS_PER_SEC;
  std::cout << "{\"phase\":\"catalogue\",";
  status(catalogue.outcome());
  std::cout << ",\"coord_bits\":" << kCoordBits << ",\"kmax\":" << params.kmax
            << ",\"wall_ns\":" << catalogue_ns << ",\"cpu_seconds\":" << std::setprecision(12) << cpu_seconds
            << ",\"peak_reserved_bytes\":" << budget.peak() << ",\"reserved_after_bytes\":" << budget.used();
  if (catalogue.ok()) {
    const auto& c = catalogue.value(); const auto& l = c.ledger();
    std::cout << ",\"balls\":" << c.balls() << ",\"levels\":" << c.levels().size()
              << ",\"incidences\":" << c.population().size() << ",\"generation_passes\":2"
              << ",\"logical\":{\"nodes\":" << l.nodes << ",\"leaves\":" << l.leaves
              << ",\"filter_tests\":" << l.filter_tests << ",\"dominance_tests\":" << l.dominance_tests
              << ",\"prefixes\":" << l.prefixes << ",\"judged\":" << l.judged
              << ",\"census_tests\":" << l.census_tests << ",\"max_leaf\":" << l.max_leaf
              << ",\"max_depth\":" << l.max_depth << '}';
  }
  std::cout << "}\n" << std::flush;
  if (!catalogue.ok()) return catalogue.outcome();
  MHGP11_TRY(serialize(argv[3], cloud.value(), catalogue.value()));
  return {};
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 10) return 2;
  std::array<u64, 6> options{};
  for (int i = 0; i < 6; ++i)
    if (!parse(argv[i + 4], options[i])) return 2;
  if (options[0] > 12 || options[1] > 1024 || options[2] > 1024) return 2;
  CatalogueParams params;
  params.kmax = static_cast<int>(options[0]); params.leaf_size = static_cast<u32>(options[1]);
  params.max_leaf = static_cast<u32>(options[2]); params.max_nodes = options[3]; params.ball_limit = options[4];
  const Outcome outcome = guarded([&]() { return run(argv, params, options[5]); });
  std::cout << "{\"phase\":\"exit\","; status(outcome); std::cout << "}\n";
  return exit_code(outcome);
}
