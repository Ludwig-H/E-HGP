#include "pipeline/prepared_cloud.hpp"
#include "pipeline/q2_census.hpp"

#include <openssl/sha.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace mhgp9::gen;
using Clock = std::chrono::steady_clock;
using Vec = std::array<i64, 3>;
constexpr std::size_t kSites = 1288;
constexpr std::size_t kSurvivors = 27099;
constexpr unsigned kMax = 5, kT3 = kMax - 1, kT4 = kMax - 2;
constexpr i64 q = i64{1} << 20;
constexpr u64 kNodeBudget = 1000000, kCellBudget = 50000;
constexpr auto kWallBudget = std::chrono::seconds(30);
constexpr unsigned kRefinePerCell = 128;
constexpr std::string_view kResultSha = "28a3dbf011b31371e6eab3854f3a6fb294462a4a20d3420aa017b4ece4ddabe9";
constexpr std::string_view kXyzSha = "33630aea9492d3b059001e10c74ba30bec143dd7a1a0be6b25f8701b2f2a5e8f";
constexpr std::string_view kIdsSha = "d473e6314cf322c9213998906d3c43f59ee5044b809a8545fe4febe9113e053a";
constexpr std::string_view kTraceSha = "7d5823ea50c45ef0dd7a95ddd17bf09cf2819f13ae11355d92fc9100e2e40297";

std::vector<unsigned char> bytes(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) throw std::runtime_error("cannot open " + path.string());
  return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

std::string sha256(const std::vector<unsigned char>& data) {
  std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
  if (SHA256(data.data(), data.size(), digest.data()) == nullptr)
    throw std::runtime_error("SHA256 failed");
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (const auto byte : digest) out << std::setw(2) << unsigned(byte);
  return out.str();
}

std::string json_text(const std::string& doc, const std::string& key) {
  const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
  std::smatch match;
  if (!std::regex_search(doc, match, pattern)) throw std::runtime_error("missing RESULT key " + key);
  return match[1];
}

u64 json_count(const std::string& doc, const std::string& key) {
  const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*([0-9]+)");
  std::smatch match;
  if (!std::regex_search(doc, match, pattern)) throw std::runtime_error("missing RESULT key " + key);
  return std::stoull(match[1]);
}

void require(bool ok, const std::string& message) {
  if (!ok) throw std::runtime_error(message);
}

std::uint32_t u32le(const unsigned char* p) {
  return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
         (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}

u64 decimal(const std::string& value) {
  require(!value.empty() && std::all_of(value.begin(), value.end(),
          [](unsigned char c) { return c >= '0' && c <= '9'; }), "nondecimal trace field");
  std::size_t used = 0;
  const auto result = std::stoull(value, &used);
  require(used == value.size(), "invalid trace integer");
  return result;
}

std::vector<std::string> fields(const std::string& line) {
  std::vector<std::string> out;
  std::size_t begin = 0;
  for (;;) {
    const auto end = line.find('\t', begin);
    out.push_back(line.substr(begin, end == std::string::npos ? end : end - begin));
    if (end == std::string::npos) break;
    begin = end + 1;
  }
  return out;
}

struct TraceRow {
  std::uint32_t local_a{}, local_b{}, raw_a{}, raw_b{}, F{};
  std::uint8_t s2{}, post_core{};
};

struct Input {
  std::vector<Point3> points;
  std::vector<std::uint32_t> raw_ids;
  std::vector<TraceRow> rows;
};

Input read_verified(const std::filesystem::path& xyz_path, const std::filesystem::path& ids_path,
                    const std::filesystem::path& trace_path, const std::filesystem::path& result_path) {
  const auto result_data = bytes(result_path);
  require(sha256(result_data) == kResultSha, "pinned RESULT.json SHA mismatch");
  const std::string result(result_data.begin(), result_data.end());
  require(json_text(result, "scope") == "audit_only_no_ground_quarter_08_000200_K5", "RESULT scope");
  require(json_count(result, "sites") == kSites && json_count(result, "survivors") == kSurvivors &&
          json_count(result, "K") == kMax && json_count(result, "s") == 8 &&
          json_count(result, "sum_F") == 298205 && json_count(result, "core_closed_F") == 146394,
          "RESULT ledger differs from pinned pilot");
  const auto xyz = bytes(xyz_path), ids = bytes(ids_path), trace = bytes(trace_path);
  require(sha256(xyz) == kXyzSha && sha256(xyz) == json_text(result, "input_sha256") &&
          sha256(ids) == kIdsSha && sha256(ids) == json_text(result, "raw_return_ids_sha256") &&
          sha256(trace) == kTraceSha && sha256(trace) == json_text(result, "trace_sha256"),
          "fixture SHA differs from pinned RESULT.json");
  require(xyz.size() == 12 * kSites && ids.size() == 4 * kSites, "fixture size");
  Input input;
  input.points.reserve(kSites);
  input.raw_ids.reserve(kSites);
  for (std::size_t j = 0; j < kSites; ++j) {
    const auto x = u32le(xyz.data() + 12 * j), y = u32le(xyz.data() + 12 * j + 4);
    const auto z = u32le(xyz.data() + 12 * j + 8);
    require(x < (1U << 18) && y < (1U << 18) && z < (1U << 18), "u18 fixture range");
    input.points.push_back({Coordinate(x), Coordinate(y), Coordinate(z)});
    input.raw_ids.push_back(u32le(ids.data() + 4 * j));
  }
  auto sorted_ids = input.raw_ids;
  std::sort(sorted_ids.begin(), sorted_ids.end());
  require(std::adjacent_find(sorted_ids.begin(), sorted_ids.end()) == sorted_ids.end(), "duplicate raw ID");
  const std::string trace_text(trace.begin(), trace.end());
  std::istringstream stream(trace_text);
  std::string line;
  require(bool(std::getline(stream, line)) && line ==
          "s2_ordinal\trectangle_ordinal\tlocal_a\tlocal_b\traw_a\traw_b\ts2_mask\tF\tpost_core_mask\tpost_s3_mask",
          "trace header");
  u64 sum_f = 0, closed_f = 0;
  while (std::getline(stream, line)) {
    const auto parts = fields(line);
    require(parts.size() == 10, "trace width");
    require(decimal(parts[0]) == input.rows.size(), "noncontiguous trace ordinal");
    const auto a = decimal(parts[2]), b = decimal(parts[3]);
    const auto raw_a = decimal(parts[4]), raw_b = decimal(parts[5]);
    const auto mask = decimal(parts[6]), f = decimal(parts[7]);
    const auto post = decimal(parts[8]), s3 = decimal(parts[9]);
    require(a < kSites && b < kSites && a != b && raw_a == input.raw_ids[a] &&
            raw_b == input.raw_ids[b] && mask != 0 && (mask & ~6U) == 0 &&
            f >= 2 && f <= kSites && (post & ~mask) == 0 && (s3 & ~post) == 0,
            "trace endpoint, mask, or F mismatch");
    input.rows.push_back({std::uint32_t(a), std::uint32_t(b), std::uint32_t(raw_a),
                          std::uint32_t(raw_b), std::uint32_t(f),
                          std::uint8_t(mask), std::uint8_t(post)});
    sum_f += f;
    if (post == 0) closed_f += f;
  }
  require(input.rows.size() == kSurvivors && sum_f == json_count(result, "sum_F") &&
          closed_f == json_count(result, "core_closed_F"), "trace count/F ledger mismatch");
  return input;
}

u64 mix(u64 x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

std::vector<std::size_t> sample(const std::vector<TraceRow>& rows) {
  std::array<std::vector<std::size_t>, 24> strata;
  for (std::size_t j = 0; j < rows.size(); ++j) {
    const auto& row = rows[j];
    const unsigned mask = row.s2 == 2 ? 0 : row.s2 == 4 ? 1 : 2;
    const unsigned f = row.F <= 8 ? 0 : row.F <= 32 ? 1 : row.F <= 128 ? 2 : 3;
    const unsigned proved = row.post_core == row.s2 ? 0 : 1;
    strata[(mask * 4 + f) * 2 + proved].push_back(j);
  }
  std::vector<std::size_t> chosen;
  for (auto& group : strata) {
    std::sort(group.begin(), group.end(), [](std::size_t a, std::size_t b) {
      return std::pair{mix(a), a} < std::pair{mix(b), b};
    });
    const auto take = std::min<std::size_t>(10, group.size());
    chosen.insert(chosen.end(), group.begin(), group.begin() + static_cast<std::ptrdiff_t>(take));
  }
  std::vector<bool> used(rows.size(), false);
  for (const auto j : chosen) used[j] = true;
  std::vector<std::size_t> remaining;
  for (std::size_t j = 0; j < rows.size(); ++j) if (!used[j]) remaining.push_back(j);
  std::sort(remaining.begin(), remaining.end(), [](std::size_t a, std::size_t b) {
    return std::pair{mix(a), a} < std::pair{mix(b), b};
  });
  const auto fill = std::min<std::size_t>(256 - chosen.size(), remaining.size());
  chosen.insert(chosen.end(), remaining.begin(), remaining.begin() + static_cast<std::ptrdiff_t>(fill));
  std::sort(chosen.begin(), chosen.end());
  require(chosen.size() == 256 && std::adjacent_find(chosen.begin(), chosen.end()) == chosen.end(),
          "sample size or duplicate");
  return chosen;
}

struct Cell { i64 left, right, bottom, top; };
struct Work {
  u64 cells{}, outside_cells{}, disk_tests{}, center_membership_tests{};
  u64 node_tests{}, corner_tests{}, corner_evals{}, lb_tests{};
  u64 credited_nodes{}, credited_sites{}, excluded_nodes{}, leaf_ambiguous{}, splits{};
  u64 frontier_copies{}, point_tests{}, fixed_centers{}, fixed_refutations{};
  u64 gate_nodes{}, gate_sites{}, exclude_gate_nodes{}, exclude_gate_sites{};
  u64 gate_ns{};
};
struct BudgetStop {};

class Prover {
 public:
  Prover(const Q2CensusIndex& index, const TraceRow& row, std::size_t a_rank,
         std::size_t b_rank, Clock::time_point began, u64& global_nodes, u64& global_cells)
      : index_(index), began_(began), global_nodes_(global_nodes),
        global_cells_(global_cells), a_rank_(a_rank), b_rank_(b_rank) {
    const auto points = index.cloud().points();
    const auto a = points[row.local_a], b = points[row.local_b];
    std::size_t main_axis = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      v_[axis] = i64(b[axis]) - a[axis];
      m2_[axis] = i64(a[axis]) + b[axis];
      if (std::llabs(v_[axis]) > std::llabs(v_[main_axis])) main_axis = axis;
    }
    const auto axis_i = (main_axis + 1) % 3, axis_j = (main_axis + 2) % 3;
    const i64 h = std::llabs(v_[main_axis]), sign = v_[main_axis] > 0 ? 1 : -1;
    A_[axis_i] = h; A_[main_axis] = -sign * v_[axis_i];
    B_[axis_j] = h; B_[main_axis] = -sign * v_[axis_j];
    D_ = dot(v_, v_);
    require(D_ > 0, "zero-length edge");
    const auto order = index.spatial_order();
    require(a_rank_ < order.size() && b_rank_ < order.size() &&
            order[a_rank_] == row.local_a && order[b_rank_] == row.local_b,
            "endpoint rank inverse");
  }

  [[nodiscard]] std::uint8_t prove(std::uint8_t lanes) {
    return cell({-2 * q, 2 * q, -2 * q, 2 * q}, 0, {0}, 0, lanes);
  }
  [[nodiscard]] const Work& work() const { return work_; }
  [[nodiscard]] bool selftest_uniform(std::size_t node_id, bool allow_shell) {
    const Cell zero{0, 0, 0, 0};
    require(node_id < index_.spatial_nodes().size(), "selftest node ID");
    const auto& node = index_.spatial_nodes()[node_id];
    const bool credited = uniform(node, zero, allow_shell);
    if (credited) direct_gate(node, zero);
    return credited;
  }
  [[nodiscard]] bool selftest_excluded(std::size_t node_id, bool reverse_sign) {
    const Cell zero{0, 0, 0, 0};
    require(node_id < index_.spatial_nodes().size(), "selftest node ID");
    const auto& node = index_.spatial_nodes()[node_id];
    const bool excluded_now = excluded(node, zero, reverse_sign);
    if (excluded_now) {
      for (auto rank = node.range.first; rank < node.range.last; ++rank)
        require(form(index_.spatial_points()[rank], 0, 0) >= 0,
                "gate: excluded node contains interior site");
    }
    return excluded_now;
  }

 private:
  static i128 dot(const Vec& a, const Vec& b) {
    i128 sum = 0;
    for (std::size_t i = 0; i < 3; ++i) sum += i128(a[i]) * b[i];
    return sum;
  }
  void budget_cell() {
    ++work_.cells;
    if (++global_cells_ > kCellBudget || Clock::now() - began_ >= kWallBudget) throw BudgetStop{};
  }
  void budget_node() {
    ++work_.node_tests;
    if (++global_nodes_ > kNodeBudget ||
        ((global_nodes_ & 1023U) == 0U && Clock::now() - began_ >= kWallBudget))
      throw BudgetStop{};
  }
  [[nodiscard]] Vec w(const Point3& z) const {
    return {2 * i64(z.x) - m2_[0], 2 * i64(z.y) - m2_[1], 2 * i64(z.z) - m2_[2]};
  }
  [[nodiscard]] i128 form(const Point3& z, i64 alpha, i64 beta) const {
    const auto wz = w(z);
    i128 sq = 0, wa = 0, wb = 0;
    for (std::size_t i = 0; i < 3; ++i) {
      sq += i128(wz[i]) * wz[i];
      wa += i128(wz[i]) * A_[i];
      wb += i128(wz[i]) * B_[i];
    }
    return i128(q) * (sq - D_) - 2 * (wa * alpha + wb * beta);
  }
  [[nodiscard]] bool uniform(const Q2SpatialNode& node, const Cell& c, bool allow_shell = false) {
    ++work_.corner_tests;
    i128 maximum = std::numeric_limits<i64>::min();
    for (unsigned corner = 0; corner < 8; ++corner) {
      const auto z = box_corner(node.box, corner);
      for (const auto alpha : {c.left, c.right}) for (const auto beta : {c.bottom, c.top}) {
        maximum = std::max(maximum, form(z, alpha, beta));
        ++work_.corner_evals;
      }
    }
    return allow_shell ? maximum <= 0 : maximum < 0;
  }
  [[nodiscard]] bool excluded(const Q2SpatialNode& node, const Cell& c,
                              bool reverse_sign = false) {
    ++work_.lb_tests;
    i128 min_sq = 0, max_wr = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const i128 wl = 2 * i128(node.box.low[axis]) - m2_[axis];
      const i128 wh = 2 * i128(node.box.high[axis]) - m2_[axis];
      const i128 near = wl > 0 ? wl : wh < 0 ? wh : 0;
      min_sq += near * near;
      const i128 a0 = i128(A_[axis]) * c.left, a1 = i128(A_[axis]) * c.right;
      const i128 b0 = i128(B_[axis]) * c.bottom, b1 = i128(B_[axis]) * c.top;
      const i128 rl = std::min(a0, a1) + std::min(b0, b1);
      const i128 rh = std::max(a0, a1) + std::max(b0, b1);
      max_wr += std::max({wl * rl, wl * rh, wh * rl, wh * rh});
    }
    const i128 lower = i128(q) * (min_sq - D_) - 2 * max_wr;
    return reverse_sign ? lower <= 0 : lower >= 0;
  }
  [[nodiscard]] bool outside(const Cell& c, i64 factor) {
    ++work_.disk_tests;
    i128 norm = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const i64 x = A_[axis], y = B_[axis];
      const i64 low = x * (x < 0 ? c.right : c.left) + y * (y < 0 ? c.top : c.bottom);
      const i64 high = x * (x < 0 ? c.left : c.right) + y * (y < 0 ? c.bottom : c.top);
      const i128 nearest = low > 0 ? low : high < 0 ? high : 0;
      norm += nearest * nearest;
    }
    return factor * norm > D_ * i128(q) * q;
  }
  [[nodiscard]] bool center_inside(i64 alpha, i64 beta, i64 factor) {
    ++work_.center_membership_tests;
    i128 norm = 0;
    for (std::size_t i = 0; i < 3; ++i) {
      const i128 t = i128(A_[i]) * alpha + i128(B_[i]) * beta;
      norm += t * t;
    }
    return factor * norm <= D_ * i128(q) * q;
  }
  [[nodiscard]] bool refuted_at_witness(const Cell& c, std::uint8_t lane) {
    const i64 factor = lane == 2 ? 3 : 2;
    const std::array<std::array<i64, 2>, 5> candidates{{
        {c.left, c.bottom}, {c.left, c.top}, {c.right, c.bottom}, {c.right, c.top},
        {c.left + (c.right - c.left) / 2, c.bottom + (c.top - c.bottom) / 2}}};
    for (const auto& xy : candidates) {
      if (!center_inside(xy[0], xy[1], factor)) continue;
      ++work_.fixed_centers;
      const unsigned target = lane == 2 ? kT3 : kT4;
      unsigned count = 0;
      for (const auto& z : index_.cloud().points()) {
        ++work_.point_tests;
        if (form(z, xy[0], xy[1]) < 0 && ++count >= target) break;
      }
      if (count < target) { ++work_.fixed_refutations; return true; }
      return false;
    }
    return false;
  }
  void direct_gate(const Q2SpatialNode& node, const Cell& c) {
    const auto began = Clock::now();
    ++work_.gate_nodes;
    const auto ordered = index_.spatial_points();
    for (auto rank = node.range.first; rank < node.range.last; ++rank) {
      const auto& z = ordered[rank];
      require(z.x >= node.box.low.x && z.x <= node.box.high.x &&
              z.y >= node.box.low.y && z.y <= node.box.high.y &&
              z.z >= node.box.low.z && z.z <= node.box.high.z, "gate: point outside node box");
      for (const auto alpha : {c.left, c.right}) for (const auto beta : {c.bottom, c.top})
        require(form(z, alpha, beta) < 0, "gate: credited point not strictly interior");
      ++work_.gate_sites;
    }
    work_.gate_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - began).count();
  }
  void direct_exclusion_gate(const Q2SpatialNode& node, const Cell& c, std::size_t id) {
    if (node.range.size() > 32 || (mix((u64(work_.cells) << 32) ^ id) & 31U) != 0) return;
    const auto began = Clock::now();
    ++work_.exclude_gate_nodes;
    const auto ordered = index_.spatial_points();
    for (auto rank = node.range.first; rank < node.range.last; ++rank) {
      const auto& z = ordered[rank];
      for (const auto alpha : {c.left, c.right}) for (const auto beta : {c.bottom, c.top})
        require(form(z, alpha, beta) >= 0, "gate: excluded node contains interior site");
      ++work_.exclude_gate_sites;
    }
    work_.gate_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - began).count();
  }

  [[nodiscard]] std::uint8_t cell(const Cell& c, unsigned depth, std::vector<std::size_t> frontier,
                                  std::size_t inherited, std::uint8_t lanes) {
    budget_cell();
    std::uint8_t need = 0;
    if ((lanes & 2U) != 0 && !outside(c, 3)) need |= 2;
    if ((lanes & 4U) != 0 && !outside(c, 2)) need |= 4;
    if (need == 0) { ++work_.outside_cells; return lanes; }
    std::uint8_t certified = lanes & ~need;
    if (depth >= 2) {
      for (const auto lane : {std::uint8_t{2}, std::uint8_t{4}})
        if ((need & lane) != 0 && refuted_at_witness(c, lane)) need &= ~lane;
      if (need == 0) return certified;
      std::deque<std::size_t> pending(frontier.begin(), frontier.end());
      std::vector<std::size_t> next;
      const auto target = (need & 2U) != 0 ? kT3 : kT4;
      unsigned tested_here = 0;
      while (!pending.empty() && inherited < target && tested_here < kRefinePerCell) {
        const auto id = pending.front(); pending.pop_front();
        const auto& node = index_.spatial_nodes()[id];
        budget_node(); ++tested_here;
        if (uniform(node, c)) {
          require(!(node.range.first <= a_rank_ && a_rank_ < node.range.last) &&
                  !(node.range.first <= b_rank_ && b_rank_ < node.range.last),
                  "credited node contains endpoint");
          direct_gate(node, c);
          ++work_.credited_nodes;
          work_.credited_sites += node.range.size();
          inherited += node.range.size();
        } else if (excluded(node, c)) {
          direct_exclusion_gate(node, c, id);
          ++work_.excluded_nodes;
        } else if (node.left != Q2SpatialNode::absent) {
          ++work_.splits;
          pending.push_back(node.left);
          pending.push_back(node.right);
        } else {
          ++work_.leaf_ambiguous;
          next.push_back(id);
        }
      }
      if (inherited >= target) return certified | need;
      while (!pending.empty()) { next.push_back(pending.front()); pending.pop_front(); }
      frontier = std::move(next);
      if ((need & 4U) != 0 && inherited >= kT4) { certified |= 4; need &= ~4; }
      if ((need & 2U) != 0 && inherited >= kT3) { certified |= 2; need &= ~2; }
      if (need == 0) return certified;
    }
    if (depth == 6) return certified;
    const i64 mx = c.left + (c.right - c.left) / 2;
    const i64 my = c.bottom + (c.top - c.bottom) / 2;
    const std::array<Cell, 4> children{{
        {c.left, mx, c.bottom, my}, {mx, c.right, c.bottom, my},
        {c.left, mx, my, c.top}, {mx, c.right, my, c.top}}};
    std::uint8_t all = need;
    for (const auto& child : children) {
      work_.frontier_copies += frontier.size();
      all &= cell(child, depth + 1, frontier, inherited, all);
      if (all == 0) break;
    }
    return certified | all;
  }

  const Q2CensusIndex& index_;
  Clock::time_point began_;
  u64& global_nodes_;
  u64& global_cells_;
  Vec v_{}, m2_{}, A_{}, B_{};
  i128 D_{};
  std::size_t a_rank_{kSites}, b_rank_{kSites};
  Work work_{};
};

void selftest(bool mutate_shell, bool mutate_exclusion) {
  // Exercise the actual node predicate and direct gate on an endpoint leaf.
  // For a=(0,0,0), b=(4,0,0), H(a,0,0)=0; changing <0 to <=0
  // falsely credits a and must be rejected by direct enumeration.
  const std::array<Point3, 3> points{{{0, 0, 0}, {4, 0, 0}, {2, 0, 0}}};
  const auto index = make_q2_cloud_index(prepare_cloud(points));
  std::size_t a_rank = points.size(), b_rank = points.size();
  std::size_t a_leaf = Q2SpatialNode::absent, interior_leaf = Q2SpatialNode::absent;
  for (std::size_t rank = 0; rank < points.size(); ++rank) {
    if (index->spatial_order()[rank] == 0) a_rank = rank;
    if (index->spatial_order()[rank] == 1) b_rank = rank;
  }
  for (std::size_t id = 0; id < index->spatial_nodes().size(); ++id) {
    const auto& node = index->spatial_nodes()[id];
    if (node.range.first == a_rank && node.range.size() == 1) a_leaf = id;
    if (node.range.size() == 1 && index->spatial_order()[node.range.first] == 2)
      interior_leaf = id;
  }
  require(a_leaf != Q2SpatialNode::absent && interior_leaf != Q2SpatialNode::absent,
          "selftest leaf");
  u64 nodes = 0, cells = 0;
  Prover prover(*index, TraceRow{0, 1, 0, 1, 3, 4, 4}, a_rank, b_rank,
                Clock::now(), nodes, cells);
  require(!prover.selftest_uniform(a_leaf, mutate_shell), "selftest shell credited");
  require(!prover.selftest_excluded(interior_leaf, mutate_exclusion),
          "selftest interior node excluded");
  // The box corners of [0,4]x[0,4]x{0} are nonnegative at u=0 even
  // though z=(2,0,0) is strictly inside: failed max is not exclusion.
  const i128 corners[] = {0, 64 * i128(q), 0, 64 * i128(q)};
  require(*std::max_element(std::begin(corners), std::end(corners)) >= 0,
          "selftest: failed max cannot exclude interior site");
  std::cout << "selftest_ok\n";
}

void run(const std::filesystem::path& xyz, const std::filesystem::path& ids,
         const std::filesystem::path& trace, const std::filesystem::path& result,
         const std::filesystem::path& detail_path, const std::filesystem::path& shadow_path) {
  require(!std::filesystem::exists(detail_path) && !std::filesystem::exists(shadow_path) &&
          detail_path != shadow_path, "output exists or paths collide");
  const auto input = read_verified(xyz, ids, trace, result);
  const auto index = make_q2_cloud_index(prepare_cloud(input.points));
  require(index->spatial_order().size() == kSites && index->spatial_nodes().size() == 2575,
          "index size differs from pinned pilot");
  std::vector<std::size_t> inverse(kSites, kSites);
  for (std::size_t rank = 0; rank < kSites; ++rank) {
    const auto id = index->spatial_order()[rank];
    require(id < kSites && inverse[id] == kSites, "spatial order is not a permutation");
    inverse[id] = rank;
    require(index->spatial_points()[rank] == input.points[id], "spatial point/ID mismatch");
  }
  for (const auto rank : inverse) require(rank < kSites, "missing spatial rank");
  const auto selected = sample(input.rows);
  std::ofstream detail(detail_path), shadow(shadow_path);
  require(bool(detail) && bool(shadow), "cannot create output files");
  detail << "s2_ordinal\ts2_mask\tpost_core_mask\tF\tproved_mask\tcomplete\twall_ms\tcpu_ms"
         << "\tcells\toutside_cells\tdisk_tests\tcenter_membership_tests"
         << "\tnode_tests\tcorner_tests\tcorner_evals\tlb_tests"
         << "\tcredited_nodes\tcredited_sites\texcluded_nodes\tleaf_ambiguous\tsplits"
         << "\tfrontier_copies\tpoint_tests\tfixed_centers\tfixed_refutations\tgate_nodes\tgate_sites"
         << "\texclude_gate_nodes\texclude_gate_sites\tgate_ms\n";
  shadow << "s2_ordinal\tproved_mask\n";
  const auto began = Clock::now();
  u64 global_nodes = 0, global_cells = 0;
  std::size_t visited = 0, complete = 0, incomplete = 0;
  for (const auto ordinal : selected) {
    if (Clock::now() - began >= kWallBudget || global_nodes >= kNodeBudget ||
        global_cells >= kCellBudget) break;
    const auto& row = input.rows[ordinal];
    const auto edge_began = Clock::now();
    const auto cpu_began = std::clock();
    Prover prover(*index, row, inverse[row.local_a], inverse[row.local_b],
                  began, global_nodes, global_cells);
    std::uint8_t proved = 0;
    bool finished = true;
    try { proved = prover.prove(row.s2); }
    catch (const BudgetStop&) { finished = false; proved = 0; }
    const double wall_ms = std::chrono::duration<double, std::milli>(Clock::now() - edge_began).count();
    const double cpu_ms = 1000.0 * double(std::clock() - cpu_began) / CLOCKS_PER_SEC;
    require((proved & ~row.s2) == 0, "proof widened S2 mask");
    const auto& w = prover.work();
    detail << ordinal << '\t' << unsigned(row.s2) << '\t' << unsigned(row.post_core) << '\t'
           << row.F << '\t' << unsigned(proved) << '\t' << unsigned(finished) << '\t'
           << std::fixed << std::setprecision(3) << wall_ms << '\t' << cpu_ms << '\t'
           << w.cells << '\t' << w.outside_cells << '\t' << w.disk_tests << '\t'
           << w.center_membership_tests << '\t' << w.node_tests << '\t'
           << w.corner_tests << '\t' << w.corner_evals << '\t' << w.lb_tests << '\t'
           << w.credited_nodes << '\t' << w.credited_sites << '\t' << w.excluded_nodes << '\t'
           << w.leaf_ambiguous << '\t' << w.splits << '\t' << w.frontier_copies << '\t'
           << w.point_tests << '\t' << w.fixed_centers << '\t' << w.fixed_refutations << '\t'
           << w.gate_nodes << '\t' << w.gate_sites << '\t'
           << w.exclude_gate_nodes << '\t' << w.exclude_gate_sites << '\t'
           << double(w.gate_ns) / 1000000.0 << '\n';
    shadow << ordinal << '\t' << unsigned(proved) << '\n';
    ++visited;
    if (finished) ++complete; else { ++incomplete; break; }
  }
  detail.close(); shadow.close();
  require(bool(detail) && bool(shadow), "failed writing output files");
  std::cout << "{\"selected\":" << selected.size() << ",\"visited\":" << visited
            << ",\"complete\":" << complete << ",\"incomplete\":" << incomplete
            << ",\"node_tests\":" << global_nodes << ",\"cells\":" << global_cells
            << ",\"wall_ms\":" << std::fixed << std::setprecision(3)
            << std::chrono::duration<double, std::milli>(Clock::now() - began).count() << "}\n";
}
}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string_view(argv[1]) == "--selftest") {
      selftest(false, false); return 0;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--mutate-allow-shell") {
      selftest(true, false); return 0;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--mutate-exclusion-sign") {
      selftest(false, true); return 0;
    }
    if (argc != 7) {
      std::cerr << "usage: probe points.u32le raw_ids.u32le trace.tsv RESULT.json detail.tsv shadow.tsv\n";
      return 2;
    }
    run(argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
  } catch (const std::exception& e) {
    std::cerr << "b_spatial_block_probe: " << e.what() << '\n';
    return 1;
  }
}
