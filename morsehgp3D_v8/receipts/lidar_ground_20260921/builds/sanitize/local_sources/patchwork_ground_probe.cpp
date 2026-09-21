// Standalone mask adapter. The pinned third-party sources remain unmodified.
// This is approximate ground classification, not an exact HGP primitive.
#include <array>
#include <bit>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <patchwork/patchworkpp.h>

#ifndef MHGP8_PATCHWORK_COMMIT
#error "The reproducible builder must supply the pinned upstream commit"
#endif
#ifndef EIGEN_DONT_PARALLELIZE
#error "This adapter is the single-thread reference"
#endif
#if defined(__FAST_MATH__) || defined(_OPENMP)
#error "No fast-math or OpenMP for the single-thread mask adapter"
#endif

namespace mhgp8 {
namespace {
using Clock = std::chrono::steady_clock;
constexpr double numeric_limit = 0x1p40;
constexpr std::string_view schema = "mhgp8_patchwork_ground_probe_v1";
constexpr std::string_view encoding = "u8_0_unknown_1_ground_2_nonground";
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);

struct Options {
    std::filesystem::path input, output;
    double sensor_height{1.723}, min_range{2.7}, max_range{80.0};
    double distance{0.125}, seed_distance{0.125}, uprightness{0.707};
};

double number(std::string_view text) {
    double value{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || !std::isfinite(value))
        throw std::invalid_argument("expected a finite numeric option");
    return value;
}

Options options(int argc, char** argv) {
    Options result;
    std::vector<std::string_view> seen;
    for (int i = 1; i < argc; i += 2) {
        const std::string_view key(argv[i]);
        if (i + 1 >= argc) throw std::invalid_argument("option lacks a value");
        for (const auto previous : seen)
            if (key == previous) throw std::invalid_argument("duplicate option");
        seen.push_back(key);
        const std::string_view value(argv[i + 1]);
        if (key == "--input") result.input = value;
        else if (key == "--output") result.output = value;
        else if (key == "--sensor-height") result.sensor_height = number(value);
        else if (key == "--min-range") result.min_range = number(value);
        else if (key == "--max-range") result.max_range = number(value);
        else if (key == "--distance") result.distance = number(value);
        else if (key == "--seed-distance") result.seed_distance = number(value);
        else if (key == "--uprightness") result.uprightness = number(value);
        else throw std::invalid_argument("unknown option");
    }
    if (result.input.empty() || result.output.empty())
        throw std::invalid_argument("--input and --output are required");
    if (!(result.sensor_height > 0 && result.sensor_height <= numeric_limit &&
          result.min_range >= 0 && result.max_range > result.min_range &&
          result.max_range <= numeric_limit && result.distance > 0 &&
          result.distance <= numeric_limit && result.seed_distance > 0 &&
          result.seed_distance <= numeric_limit && result.uprightness > 0 && result.uprightness <= 1))
        throw std::invalid_argument("invalid ground parameters");
    const std::array<double, 5> boundaries{
        result.min_range, (7 * result.min_range + result.max_range) / 8,
        (3 * result.min_range + result.max_range) / 4,
        (result.min_range + result.max_range) / 2, result.max_range};
    for (std::size_t i = 1; i < boundaries.size(); ++i)
        if (!(boundaries[i] > boundaries[i - 1]))
            throw std::invalid_argument("range zones collapse numerically");
    return result;
}

patchwork::Params parameters(const Options& opts) {
    patchwork::Params p;
    p.verbose = false;
    p.enable_RNR = false;
    p.enable_RVPF = true;
    p.enable_TGR = true;
    p.sensor_height = opts.sensor_height;
    p.min_range = opts.min_range;
    p.max_range = opts.max_range;
    p.th_dist = opts.distance;
    p.th_seeds = opts.seed_distance;
    p.uprightness_thr = opts.uprightness;
    p.intensity_thr = 0;  // Upstream field has no initializer and is unused here.
    return p;
}

template<class T>
void json_array(const std::vector<T>& values) {
    std::cout << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) std::cout << ',';
        std::cout << values[i];
    }
    std::cout << ']';
}

void print_parameters(const patchwork::Params& p) {
    std::cout << "{\"verbose\":false,\"enable_RNR\":false,\"enable_RVPF\":true,\"enable_TGR\":true"
              << ",\"num_iter\":" << p.num_iter << ",\"num_lpr\":" << p.num_lpr
              << ",\"num_min_pts\":" << p.num_min_pts << ",\"num_zones\":" << p.num_zones
              << ",\"num_rings_of_interest\":" << p.num_rings_of_interest
              << ",\"RNR_ver_angle_thr\":" << p.RNR_ver_angle_thr
              << ",\"RNR_intensity_thr\":" << p.RNR_intensity_thr
              << ",\"sensor_height\":" << p.sensor_height
              << ",\"th_seeds\":" << p.th_seeds << ",\"th_dist\":" << p.th_dist
              << ",\"th_seeds_v\":" << p.th_seeds_v << ",\"th_dist_v\":" << p.th_dist_v
              << ",\"max_range\":" << p.max_range << ",\"min_range\":" << p.min_range
              << ",\"uprightness_thr\":" << p.uprightness_thr
              << ",\"adaptive_seed_selection_margin\":" << p.adaptive_seed_selection_margin
              << ",\"intensity_thr\":" << p.intensity_thr
              << ",\"max_flatness_storage\":" << p.max_flatness_storage
              << ",\"max_elevation_storage\":" << p.max_elevation_storage
              << ",\"num_sectors_each_zone\":";
    json_array(p.num_sectors_each_zone);
    std::cout << ",\"num_rings_each_zone\":";
    json_array(p.num_rings_each_zone);
    std::cout << ",\"elevation_thr\":";
    json_array(p.elevation_thr);
    std::cout << ",\"flatness_thr\":";
    json_array(p.flatness_thr);
    std::cout << '}';
}

std::uint32_t word(const unsigned char* p) {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
           (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
float coordinate(const unsigned char* p) { return std::bit_cast<float>(word(p)); }
bool finite_word(std::uint32_t bits) { return (bits & 0x7f800000U) != 0x7f800000U; }

std::uint64_t fnv(std::span<const unsigned char> bytes) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const auto byte : bytes) { hash ^= byte; hash *= 1099511628211ULL; }
    return hash;
}
void print_hex(std::uint64_t value) {
    std::cout << '"' << std::hex << std::setfill('0') << std::setw(16) << value << std::dec << '"';
}
double ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

std::vector<unsigned char> read_raw(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("cannot open input");
    const auto end = input.tellg();
    if (end < 0) throw std::runtime_error("cannot size input");
    const auto bytes = static_cast<std::uintmax_t>(end);
    if (bytes % 16 != 0) throw std::invalid_argument("input size is not a multiple of 16");
    // The author's PointXYZ::idx and iteration variables are signed int.
    if (bytes / 16 > static_cast<std::uintmax_t>(std::numeric_limits<int>::max()))
        throw std::length_error("upstream signed-int return-ID capacity exceeded");
    if (bytes > std::numeric_limits<std::size_t>::max() ||
        bytes > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max()))
        throw std::length_error("input exceeds addressable stream capacity");
    std::vector<unsigned char> data(static_cast<std::size_t>(bytes));
    input.seekg(0);
    if (!data.empty()) input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!input || input.peek() != std::char_traits<char>::eof())
        throw std::runtime_error("input changed size or could not be read completely");
    return data;
}

void write_mask(const std::filesystem::path& path, std::span<const unsigned char> mask) {
    const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
    if (fd < 0) throw std::system_error(errno, std::generic_category(), "cannot create fresh mask");
    std::size_t done{};
    try {
        while (done < mask.size()) {
            const auto written = ::write(fd, mask.data() + done, mask.size() - done);
            if (written < 0 && errno == EINTR) continue;
            if (written <= 0) throw std::system_error(errno, std::generic_category(), "mask write failed");
            done += static_cast<std::size_t>(written);
        }
    } catch (...) {
        ::close(fd);  // Preserve partial output as a failed attempt; never announce success.
        throw;
    }
    if (::close(fd) != 0) throw std::system_error(errno, std::generic_category(), "mask close failed");
}

class RedirectAuthorOutput {
    std::streambuf* previous_;
public:
    RedirectAuthorOutput() : previous_(std::cout.rdbuf(std::cerr.rdbuf())) {}
    ~RedirectAuthorOutput() { std::cout.rdbuf(previous_); }
    RedirectAuthorOutput(const RedirectAuthorOutput&) = delete;
    RedirectAuthorOutput& operator=(const RedirectAuthorOutput&) = delete;
};

int run(const Options& opts) {
    const auto start = Clock::now();
    const auto data = read_raw(opts.input);
    const auto read_end = Clock::now();
    const auto n = data.size() / 16;
    std::vector<unsigned char> mask(n, 0), excluded(n, 0);
    std::vector<std::size_t> original_ids;
    original_ids.reserve(n);
    std::size_t intensity_nonfinite{}, outside{}, numeric{}, sentinel{};
    for (std::size_t id = 0; id < n; ++id) {
        const auto* point = data.data() + 16 * id;
        for (std::size_t axis = 0; axis < 3; ++axis)
            if (!finite_word(word(point + 4 * axis))) throw std::invalid_argument("nonfinite XYZ");
        intensity_nonfinite += !finite_word(word(point + 12));
        const auto x = coordinate(point), y = coordinate(point + 4), z = coordinate(point + 8);
        if (std::abs(static_cast<double>(x)) > numeric_limit ||
            std::abs(static_cast<double>(y)) > numeric_limit ||
            std::abs(static_cast<double>(z)) > numeric_limit) {
            excluded[id] = 1; ++numeric; continue;
        }
        if (word(point + 8) == 0x00800000U) {
            // pc2czm silently skips this z value even when RNR is disabled.
            excluded[id] = 1; ++sentinel; continue;
        }
        const auto radius = std::hypot(static_cast<double>(x), static_cast<double>(y));
        if (!(radius > opts.min_range && radius <= opts.max_range)) {
            excluded[id] = 1; ++outside; continue;
        }
        original_ids.push_back(id);
    }
    Eigen::setNbThreads(1);
    Eigen::MatrixXf cloud(static_cast<Eigen::Index>(original_ids.size()), 3);
    for (std::size_t row = 0; row < original_ids.size(); ++row)
        for (std::size_t axis = 0; axis < 3; ++axis)
            cloud(static_cast<Eigen::Index>(row), static_cast<Eigen::Index>(axis)) =
                coordinate(data.data() + 16 * original_ids[row] + 4 * axis);
    const auto p = parameters(opts);
    const auto input_hash = fnv(data);
    const auto prep_end = Clock::now();
    Eigen::VectorXi ground, nonground;
    if (!original_ids.empty()) {
        RedirectAuthorOutput redirect;
        patchwork::PatchWorkpp algorithm(p);  // Fresh state, exactly one whole-frame call.
        algorithm.estimateGround(cloud);
        ground = algorithm.getGroundIndices();
        nonground = algorithm.getNongroundIndices();
    }
    const auto segment_end = Clock::now();
    const auto consume = [&](const Eigen::VectorXi& ids, unsigned char status) {
        for (Eigen::Index i = 0; i < ids.size(); ++i) {
            const int local = ids(i);
            if (local < 0 || static_cast<std::size_t>(local) >= original_ids.size())
                throw std::runtime_error("upstream returned an invalid ID");
            const auto id = original_ids[static_cast<std::size_t>(local)];
            if (excluded[id] != 0 || mask[id] != 0)
                throw std::runtime_error("upstream returned a duplicate or conflicting ID");
            mask[id] = status;
        }
    };
    consume(ground, 1);
    consume(nonground, 2);
    std::array<std::size_t, 3> counts{};
    for (const auto status : mask) ++counts[status];
    const auto unassigned = counts[0] - numeric - sentinel - outside;
    const auto mask_hash = fnv(mask);
    const auto mask_end = Clock::now();
    write_mask(opts.output, mask);
    const auto finish = Clock::now();
    std::cout << std::setprecision(17) << "{\"schema\":\"" << schema
              << "\",\"upstream_commit\":\"" << MHGP8_PATCHWORK_COMMIT
              << "\",\"encoding\":\"" << encoding << "\",\"raw_returns\":" << n
              << ",\"processed_returns\":" << original_ids.size()
              << ",\"algorithm_invocations\":" << (original_ids.empty() ? 0 : 1)
              << ",\"counts\":{\"unknown\":" << counts[0] << ",\"ground\":" << counts[1]
              << ",\"nonground\":" << counts[2] << "},\"unknown_reasons\":{\"outside_range\":" << outside
              << ",\"numeric_domain\":" << numeric << ",\"upstream_z_sentinel\":" << sentinel
              << ",\"upstream_unassigned\":" << unassigned << "},\"nonfinite_reflectance\":" << intensity_nonfinite
              << ",\"numeric_abs_limit\":" << numeric_limit
              << ",\"rnr_enabled\":false,\"reflectance_policy\":\"ignored_preserved_in_raw\""
              << ",\"fresh_state\":true,\"eigen_threads\":1,\"openmp\":false,\"noise_status_available\":false"
              << ",\"parameters\":";
    print_parameters(p);
    std::cout << ",\"input_fnv1a64\":";
    print_hex(input_hash);
    std::cout << ",\"mask_fnv1a64\":";
    print_hex(mask_hash);
    std::cout << ",\"timing_ms\":{\"read\":" << ms(start, read_end)
              << ",\"prepare\":" << ms(read_end, prep_end)
              << ",\"segment\":" << ms(prep_end, segment_end)
              << ",\"mask\":" << ms(segment_end, mask_end)
              << ",\"write\":" << ms(mask_end, finish)
              << ",\"wall\":" << ms(start, finish) << "}}\n";
    if (!std::cout) throw std::runtime_error("receipt stdout failed");
    return 0;
}
}  // namespace
}  // namespace mhgp8

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--describe") {
            std::cout << std::setprecision(17) << "{\"schema\":\"" << mhgp8::schema
                      << "\",\"upstream_commit\":\"" << MHGP8_PATCHWORK_COMMIT
                      << "\",\"encoding\":\"" << mhgp8::encoding << "\",\"parameters\":";
            mhgp8::print_parameters(mhgp8::parameters(mhgp8::Options{}));
            std::cout << "}\n";
            return 0;
        }
        return mhgp8::run(mhgp8::options(argc, argv));
    } catch (const std::exception& error) {
        std::cerr << "patchwork ground probe: " << error.what() << '\n';
        return 1;
    }
}
