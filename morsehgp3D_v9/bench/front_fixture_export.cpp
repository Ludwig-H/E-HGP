// Export the engraved synthetic recipes as explicit little-endian u32 XYZ.
// Preparation only, outside every chain timer. No KITTI input is read here.
#include <charconv>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string_view>

#include "../tests/gen/front_fixtures.hpp"

int main(int argc, char** argv) {
  if (argc != 5) {
    std::fprintf(stderr, "usage: n family seed fresh-output.u32le\n");
    return 2;
  }
  try {
    std::size_t n = 0;
    mhgp9::gen::u64 seed = 0;
    const auto parse = [](std::string_view value, auto& out) {
      const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), out);
      if (value.empty() || error != std::errc{} || end != value.data() + value.size())
        throw std::invalid_argument("invalid integer");
    };
    parse(argv[1], n);
    parse(argv[3], seed);
    if (std::filesystem::exists(argv[4]) || std::filesystem::is_symlink(argv[4]))
      throw std::invalid_argument("output must be fresh");
    const auto fixture = mhgp9::gen::bench::make_front_fixture(n, argv[2], seed);
    std::ofstream out(argv[4], std::ios::binary);
    if (!out) throw std::runtime_error("cannot create output");
    for (const auto& p : fixture.points) for (const auto x : {p.x, p.y, p.z}) {
      const char bytes[4] = {static_cast<char>(x & 255U), static_cast<char>((x >> 8U) & 255U),
                             static_cast<char>((x >> 16U) & 255U), static_cast<char>((x >> 24U) & 255U)};
      out.write(bytes, sizeof(bytes));
    }
    out.close();
    if (!out) throw std::runtime_error("incomplete output");
    std::printf("{\"sites\":%zu,\"family\":\"%s\",\"seed\":%llu,\"fixture_hash\":\"%016llx\","
                "\"duplicate_rejections\":%llu,\"format\":\"u32le\",\"recipe_domain\":\"u16\"}\n",
                n, argv[2], static_cast<unsigned long long>(seed),
                static_cast<unsigned long long>(fixture.input_hash),
                static_cast<unsigned long long>(fixture.work.duplicate_rejections));
    return 0;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "%s\n", e.what());
    return 1;
  }
}
