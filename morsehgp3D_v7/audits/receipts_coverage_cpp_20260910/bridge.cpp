// Independent transport adapter only: all forest construction and reads below
// call the product header. The Python driver supplies the independent oracle.
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "forest/full_coverage_certificate.hpp"

namespace {
using namespace mhgp7;

std::string token() {
  std::string value;
  if (!(std::cin >> value)) throw std::runtime_error("missing protocol token");
  return value;
}

template <class T>
T read_unsigned() {
  static_assert(std::is_unsigned_v<T>);
  const auto value = token();
  T result = 0;
  size_t i = value.front() == '+' ? 1 : 0;
  if (i == value.size()) throw std::runtime_error("empty unsigned integer");
  for (; i < value.size(); ++i) {
    if (value[i] < '0' || value[i] > '9')
      throw std::runtime_error("invalid unsigned integer");
    const T digit = static_cast<T>(value[i] - '0');
    if (result > (std::numeric_limits<T>::max() - digit) / 10)
      throw std::runtime_error("unsigned integer overflow");
    result = static_cast<T>(result * 10 + digit);
  }
  return result;
}

i128 parse_i128(const std::string& value) {
  if (value.empty()) throw std::runtime_error("empty signed integer");
  const bool negative = value.front() == '-';
  size_t i = negative || value.front() == '+' ? 1 : 0;
  if (i == value.size()) throw std::runtime_error("empty signed integer");
  const u128 positive_max = (u128{1} << 127) - 1;
  const u128 limit = positive_max + static_cast<unsigned>(negative);
  u128 magnitude = 0;
  for (; i < value.size(); ++i) {
    if (value[i] < '0' || value[i] > '9')
      throw std::runtime_error("invalid signed integer");
    const auto digit = static_cast<unsigned>(value[i] - '0');
    if (magnitude > (limit - digit) / 10)
      throw std::runtime_error("signed integer overflow");
    magnitude = magnitude * 10 + digit;
  }
  if (!negative) return static_cast<i128>(magnitude);
  // Neither cast a magnitude of 2^127 to signed nor negate the signed minimum.
  if (magnitude == positive_max + 1)
    return -static_cast<i128>(positive_max) - 1;
  return -static_cast<i128>(magnitude);
}

std::string decimal(i128 value) {
  const bool negative = value < 0;
  u128 magnitude = negative ? static_cast<u128>(-(value + 1)) + 1
                            : static_cast<u128>(value);
  std::string out;
  do {
    out.push_back(static_cast<char>('0' + magnitude % 10));
    magnitude /= 10;
  } while (magnitude);
  if (negative) out.push_back('-');
  std::reverse(out.begin(), out.end());
  return out;
}

ExactLevel read_level() {
  ExactLevel value{{0, 0, 0}, 1};
  for (auto& limb : value.num) limb = read_unsigned<u64>();
  value.den = parse_i128(token());
  return value;
}

bool read_bool() {
  const auto value = read_unsigned<unsigned>();
  if (value > 1) throw std::runtime_error("boolean must be zero or one");
  return value != 0;
}

void quoted(std::ostream& out, const std::string& value) {
  static constexpr char hex[] = "0123456789abcdef";
  out << '"';
  for (unsigned char c : value) {
    if (c == '"' || c == '\\') out << '\\' << static_cast<char>(c);
    else if (c < 0x20) out << "\\u00" << hex[c >> 4] << hex[c & 15];
    else out << static_cast<char>(c);
  }
  out << '"';
}

void level_fields(std::ostream& out, const ExactLevel& level) {
  out << level.num[0] << ',' << level.num[1] << ',' << level.num[2] << ',';
  quoted(out, decimal(level.den));
}

template <class T>
void ids(std::ostream& out, const std::vector<T>& values) {
  out << '[';
  for (size_t i = 0; i < values.size(); ++i) {
    if (i) out << ',';
    out << values[i];
  }
  out << ']';
}

void structure(std::ostream& out, const FullCoverageCertificate& forest) {
  out << "\"order\":" << forest.order() << ",\"nodes\":[";
  for (size_t i = 0; i < forest.nodes().size(); ++i) {
    if (i) out << ',';
    const auto& node = forest.nodes()[i];
    out << '[';
    level_fields(out, node.level);
    out << ',' << node.first << ',' << node.parent_count << ']';
  }
  out << "],\"parents\":";
  ids(out, forest.parents());
  out << ",\"successors\":";
  ids(out, forest.successors());
  out << ",\"contributions\":[";
  for (size_t i = 0; i < forest.contributions().size(); ++i) {
    if (i) out << ',';
    const auto& record = forest.contributions()[i];
    out << '[';
    level_fields(out, record.level);
    out << ',' << record.segment << ',' << record.ref.population << ','
        << record.ref.shell_mask << ',' << static_cast<unsigned>(record.ref.include_interior)
        << ']';
  }
  out << ']';
}

std::string snapshot(const FullCoverageCertificate& forest) {
  std::ostringstream out;
  structure(out, forest);
  return out.str();
}

std::string bank_snapshot(const std::shared_ptr<const FullCoveragePopulations>& bank) {
  if (!bank) return "null";
  std::ostringstream out;
  ids(out, bank->domain());
  for (const auto& row : bank->rows()) {
    out << '|';
    ids(out, row.interior);
    ids(out, row.shell);
  }
  return out.str();
}

bool moved_source_unreadable(const FullCoverageCertificate& forest) {
  const ExactLevel cut{{std::numeric_limits<u64>::max(),
                        std::numeric_limits<u64>::max(),
                        std::numeric_limits<u64>::max()}, 1};
  if (forest.order() != 0 || forest.populations() || !forest.nodes().empty() ||
      !forest.parents().empty() || !forest.successors().empty() ||
      !forest.contributions().empty()) return false;
  for (const auto id : {FullNodeId{0}, kFullCoverageAbsent}) {
    if (full_coverage_root_at(forest, id, cut, true) != kFullCoverageAbsent) return false;
    const auto read = full_coverage_at(forest, id, cut, true);
    if (read.status != FullCertificateStatus::kInvalidInput || !read.values.empty())
      return false;
  }
  return true;
}

struct Query {
  ExactLevel cut;
  bool closed;
};

void run_case() {
  const auto n = read_unsigned<size_t>();
  const auto population_count = read_unsigned<size_t>();
  const auto order = read_unsigned<unsigned>();
  const auto batch_count = read_unsigned<size_t>();
  const auto query_count = read_unsigned<size_t>();
  std::vector<PointId> domain(n);
  for (auto& id : domain) id = read_unsigned<PointId>();
  std::vector<FullCoveragePopulation> rows(population_count);
  for (auto& row : rows) {
    const auto ni = read_unsigned<size_t>();
    const auto ns = read_unsigned<size_t>();
    row.interior.resize(ni);
    row.shell.resize(ns);
    for (auto& id : row.interior) id = read_unsigned<PointId>();
    for (auto& id : row.shell) id = read_unsigned<PointId>();
  }
  std::vector<FullCoverageBatch> batches(batch_count);
  for (auto& batch : batches) {
    batch.level = read_level();
    batch.actions.resize(read_unsigned<size_t>());
    for (auto& action : batch.actions) {
      action.parents.resize(read_unsigned<size_t>());
      for (auto& id : action.parents) id = read_unsigned<FullNodeId>();
      action.contributions.resize(read_unsigned<size_t>());
      for (auto& ref : action.contributions) {
        ref.population = read_unsigned<u64>();
        ref.shell_mask = read_unsigned<u16>();
        ref.include_interior = read_bool();
      }
    }
  }
  std::vector<Query> queries;
  queries.reserve(query_count);
  for (size_t i = 0; i < query_count; ++i) {
    const auto cut = read_level();
    queries.push_back({cut, read_bool()});
  }

  // Semantic rejection occurs only after the complete case has been consumed.
  const auto bank = build_full_coverage_populations(domain, rows);
  const auto* const original_bank = bank.value.get();
  const auto original_bank_data = bank_snapshot(bank.value);
  auto built = build_full_coverage_certificate(order, bank.value, batches);
  const auto original_structure = snapshot(built.value);

  // Poison caller-owned elements before clearing the vectors. The bank and
  // forest must own their copies even while these original allocations survive.
  std::fill(domain.begin(), domain.end(), std::numeric_limits<PointId>::max());
  for (auto& row : rows) {
    std::fill(row.interior.begin(), row.interior.end(), PointId{0});
    std::fill(row.shell.begin(), row.shell.end(), PointId{0});
    row.interior.clear();
    row.shell.clear();
  }
  for (auto& batch : batches) {
    batch.level = {{0, 0, 0}, 0};
    for (auto& action : batch.actions) {
      std::fill(action.parents.begin(), action.parents.end(), kFullCoverageAbsent);
      for (auto& ref : action.contributions) ref = {kFullCoverageAbsent, 0, false};
      action.parents.clear();
      action.contributions.clear();
    }
    batch.actions.clear();
  }
  domain.clear();
  rows.clear();
  batches.clear();

  FullCoverageCertificate moved(std::move(built.value));
  const bool move_construct_ok = moved_source_unreadable(built.value);
  FullCoverageCertificate forest;
  forest = std::move(moved);
  const bool move_assign_ok = moved_source_unreadable(moved);
  const bool bank_shared_ok = built.status == FullCertificateStatus::kOk
      ? forest.populations().get() == original_bank
      : !forest.populations();
  const bool ownership_ok = original_bank_data == bank_snapshot(bank.value) &&
      original_structure == snapshot(forest) && bank_shared_ok &&
      move_construct_ok && move_assign_ok;

  std::cout << "{\"status\":" << static_cast<int>(built.status) << ",\"reason\":";
  quoted(std::cout, built.reason);
  std::cout << ",\"bank_status\":" << static_cast<int>(bank.status) << ',';
  structure(std::cout, forest);
  std::cout << ",\"ownership_ok\":" << (ownership_ok ? "true" : "false")
            << ",\"moved_construct_source_unreadable\":"
            << (move_construct_ok ? "true" : "false")
            << ",\"moved_assign_source_unreadable\":"
            << (move_assign_ok ? "true" : "false")
            << ",\"bank_shared_ok\":" << (bank_shared_ok ? "true" : "false")
            << ",\"queries\":[";
  std::vector<FullNodeId> requested;
  for (size_t i = 0; i <= forest.nodes().size(); ++i) requested.push_back(i);
  requested.push_back(kFullCoverageAbsent);
  for (size_t q = 0; q < queries.size(); ++q) {
    if (q) std::cout << ',';
    const auto& query = queries[q];
    std::cout << "{\"roots\":[";
    for (size_t i = 0; i < requested.size(); ++i) {
      if (i) std::cout << ',';
      std::cout << full_coverage_root_at(forest, requested[i], query.cut, query.closed);
    }
    std::cout << "],\"reads\":[";
    for (size_t i = 0; i < requested.size(); ++i) {
      if (i) std::cout << ',';
      const auto read = full_coverage_at(forest, requested[i], query.cut, query.closed);
      std::cout << '[' << static_cast<int>(read.status) << ',';
      ids(std::cout, read.values);
      std::cout << ']';
    }
    std::cout << "]}";
  }
  std::cout << "]}\n";
}
}  // namespace

int main() {
  try {
    const auto count = read_unsigned<size_t>();
    for (size_t i = 0; i < count; ++i) run_case();
    std::string trailing;
    if (std::cin >> trailing) throw std::runtime_error("trailing protocol token");
    if (std::cin.bad()) throw std::runtime_error("protocol stream failure");
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "coverage_bridge_protocol_error: " << error.what() << '\n';
    return 2;
  }
}
