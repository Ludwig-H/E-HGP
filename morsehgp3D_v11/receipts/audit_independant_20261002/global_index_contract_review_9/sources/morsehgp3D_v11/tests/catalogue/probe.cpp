// Pilote borne de test : requetes texte en lots, un JSON transactionnel par requete, meme sur refus du produit.
// Entree : K leaf maxleaf maxnodes balllimit budget n, puis n lignes x y z PointId. --profile rend le profil.
// Les vecteurs d'entree et le JSON sont hors compte (harnais) ; cloud et catalogue partagent le budget mesure.
#include <charconv>
#include <chrono>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "catalogue/catalogue.hpp"

using namespace mhgp11;

namespace {
struct Request {
  CatalogueParams params;
  u64 budget = 0;
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
};

template <class T>
bool number(std::string_view word, T& value) {
  if (word.empty()) return false;
  const auto parsed = std::from_chars(word.data(), word.data() + word.size(), value);
  return parsed.ec == std::errc{} && parsed.ptr == word.data() + word.size();
}
template <class T>
bool read(T& value) {
  std::string word;
  return bool(std::cin >> word) && number(word, value);
}

bool request(std::string_view first, Request& out) {
  u64 n = 0;
  if (!number(first, out.params.kmax) || !read(out.params.leaf_size) || !read(out.params.max_leaf) ||
      !read(out.params.max_nodes) || !read(out.params.ball_limit) || !read(out.budget) || !read(n)) return false;
  if (n > 1000000) return false;  // domaine du pilote borne, pas du moteur ; le pilote LiDAR est distinct.
  out.x.resize(n);
  out.y.resize(n);
  out.z.resize(n);
  out.ids.resize(n);
  for (u64 i = 0; i < n; ++i) {
    u32 id = 0;
    if (!read(out.x[i]) || !read(out.y[i]) || !read(out.z[i]) || !read(id)) return false;
    out.ids[i] = make_id<PointId>(id);
  }
  return true;
}

template <class T>
std::string decimal(const T& value) {
  auto wide = num::to_wide(value);
  const bool negative = wide.sign() < 0;
  std::string reversed;
  do {
    u128 remainder = 0;
    for (std::size_t i = wide.words.size(); i != 0; --i) {
      const u128 dividend = (remainder << 64) | wide.words[i - 1];
      wide.words[i - 1] = static_cast<u64>(dividend / 10);
      remainder = dividend % 10;
    }
    reversed.push_back(static_cast<char>('0' + static_cast<int>(remainder)));
  } while (!wide.is_zero());
  if (negative) reversed.push_back('-');
  return std::string(reversed.rbegin(), reversed.rend());
}

void level(std::ostream& out, const num::Level& value) {
  out << "[\"" << decimal(value.numerator()) << "\",\"" << decimal(value.denominator()) << "\"]";
}
template <StrongId Id>
void indices(std::ostream& out, std::span<const Id> values) {
  out << '[';
  bool first = true;
  for (Id value : values) {
    if (!first) out << ',';
    first = false;
    out << idx(value);
  }
  out << ']';
}

void cloud_json(std::ostream& out, const Cloud& cloud) {
  out << "\"sites\":[";
  for (u32 i = 0; i < cloud.sites(); ++i) {
    if (i != 0) out << ',';
    out << '[' << cloud.x()[i] << ',' << cloud.y()[i] << ',' << cloud.z()[i] << ']';
  }
  out << "],\"site_ids\":[";
  for (u32 i = 0; i < cloud.sites(); ++i) {
    if (i != 0) out << ',';
    indices(out, cloud.points(make_id<SiteIdx>(i)));
  }
  out << ']';
}

void catalogue_json(std::ostream& out, const Catalogue& cat) {
  out << ",\"levels\":[";
  for (std::size_t i = 0; i < cat.levels().size(); ++i) {
    if (i != 0) out << ',';
    level(out, cat.levels()[i]);
  }
  out << "],\"balls\":[";
  for (u32 i = 0; i < cat.balls(); ++i) {
    if (i != 0) out << ',';
    const auto& ball = cat.balls_data()[i];
    out << "{\"qmin\":" << static_cast<unsigned>(ball.qmin) << ",\"p\":" << ball.p << ",\"m\":" << ball.m
        << ",\"rank\":" << idx(ball.rank) << ",\"support\":";
    indices(out, std::span<const SiteIdx>(ball.support).first(ball.qmin));
    out << ",\"level\":";
    level(out, cat.levels()[idx(ball.rank)]);
    out << ",\"inner\":";
    indices(out, cat.interior(make_id<BallIdx>(i)));
    out << ",\"shell\":";
    indices(out, cat.shell(make_id<BallIdx>(i)));
    out << '}';
  }
  const auto& l = cat.ledger();
  out << "],\"ledger\":{\"nodes\":" << l.nodes << ",\"leaves\":" << l.leaves
      << ",\"filter_tests\":" << l.filter_tests << ",\"dominance_tests\":" << l.dominance_tests
      << ",\"prefixes\":" << l.prefixes << ",\"judged\":" << l.judged << ",\"census_tests\":" << l.census_tests
      << ",\"emitted\":" << l.emitted
      << ",\"incidences\":" << l.incidences << ",\"max_leaf\":" << l.max_leaf
      << ",\"q4_candidates\":" << l.q4_candidates << ",\"q4_levels\":" << l.q4_levels
      << ",\"max_depth\":" << l.max_depth << '}';
}

Result<std::string> payload(const Request& req, MemoryBudget& budget) {
  MHGP11_TRY(check_catalogue_params(req.params));
  auto cloud = prepare_cloud(req.x, req.y, req.z, req.ids, CoordWidth{}, budget);
  if (!cloud.ok()) return cloud.outcome();
  const auto start = std::chrono::steady_clock::now();
  auto cat = build_catalogue(cloud.value(), req.params, budget);
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
  if (!cat.ok()) return cat.outcome();
  std::ostringstream out;
  cloud_json(out, cloud.value());
  catalogue_json(out, cat.value());
  out << ",\"catalogue_ns\":" << ns;
  return out.str();
}

void execute(const Request& req) {
  MemoryBudget budget(req.budget);
  const u64 before = budget.used();
  auto encoded = guarded([&]() { return payload(req, budget); });
  const auto issue = merge(encoded.outcome(), budget.released());
  std::cout << "{\"status\":\"" << status_name(issue.status()) << "\",\"reason\":\"" << reason_name(issue.reason)
            << "\",\"coord_bits\":" << kCoordBits << ",\"kmax\":" << req.params.kmax
            << ",\"used_before\":" << before << ",\"used_after\":" << budget.used() << ",\"peak\":" << budget.peak() << ',';
  if (issue.ok()) std::cout << encoded.value();
  else std::cout << "\"sites\":[],\"site_ids\":[],\"levels\":[],\"balls\":[],\"ledger\":{}";
  std::cout << "}\n";
}
}  // namespace

int main(int argc, char** argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--profile") {
    std::cout << "{\"coord_bits\":" << kCoordBits << "}\n";
    return 0;
  }
  if (argc != 1) return 2;
  try {
    std::string first;
    while (std::cin >> first) {
      Request req;
      if (!request(first, req)) return 2;
      execute(req);
    }
  } catch (const std::bad_alloc&) {
    return 2;  // panne du harnais (entrees, compte ou encodage), pas resultat partiel du produit.
  }
  return std::cin.eof() ? 0 : 2;
}
