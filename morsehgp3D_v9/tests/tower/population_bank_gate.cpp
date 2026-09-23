// MorseHGP3D v9 — porte de la banque de populations FULL (surcharge deplacee).
//
// build_full_coverage_populations(domain, rows&&, threads) doit rendre la meme
// banque que la surcharge copiante sur des lignes valides, a 0, 1, 2, 4 et 8
// fils ; les memes refus sur des lignes invalides (non triees, vides, coquille
// qui recoupe l'interieur, point hors domaine) ; et un STATUT (jamais une
// exception) quand le lancement d'un fil echoue (crochet MHGP9_TESTING).
//
//   mhgp9_tower_population_bank_gate --selftest
//
// Code 0 conforme, 1 desaccord (ligne `cause=`), 2 argument.
#include <cstdio>
#include <string_view>
#include <vector>

#include "../../src/tower/forest/full_coverage_certificate.hpp"

using namespace mhgp9::tower;

namespace {
std::vector<FullCoveragePopulation> rows_for(std::size_t count, std::size_t domain_size) {
  std::vector<FullCoveragePopulation> rows;
  for (std::size_t i = 0; i < count; ++i) {
    FullCoveragePopulation row;
    const PointId a = static_cast<PointId>((i * 7) % domain_size), b = static_cast<PointId>((i * 7 + 3) % domain_size);
    const PointId c = static_cast<PointId>((i * 7 + 5) % domain_size);
    row.interior = {std::min(a, b), std::max(a, b)};
    if (row.interior[0] == row.interior[1]) row.interior.pop_back();
    if (c != a && c != b) row.shell = {c};
    rows.push_back(row);
  }
  return rows;
}
bool same_bank(const FullCoveragePopulationResult& x, const FullCoveragePopulationResult& y) {
  if (x.status != y.status) return false;
  if (x.status != FullCertificateStatus::kOk) return true;
  if (x.value->domain() != y.value->domain() || x.value->rows().size() != y.value->rows().size()) return false;
  for (std::size_t i = 0; i < x.value->rows().size(); ++i)
    if (x.value->rows()[i].interior != y.value->rows()[i].interior ||
        x.value->rows()[i].shell != y.value->rows()[i].shell) return false;
  return true;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::fprintf(stderr, "usage: mhgp9_tower_population_bank_gate --selftest\n");
    return 2;
  }
  std::vector<PointId> domain(97);
  for (std::size_t i = 0; i < domain.size(); ++i) domain[i] = static_cast<PointId>(i);
  std::size_t checks = 0;
  const auto rows = rows_for(20000, domain.size());
  const auto copied = build_full_coverage_populations(domain, rows);
  if (copied.status != FullCertificateStatus::kOk) { std::printf("cause=bank.valid_rows_refused\n"); return 1; }
  for (const int threads : {0, 1, 2, 4, 8}) {
    auto moved_rows = rows;
    const auto moved = build_full_coverage_populations(domain, std::move(moved_rows), threads);
    ++checks;
    if (!same_bank(copied, moved)) { std::printf("cause=bank.move_differs threads=%d\n", threads); return 1; }
  }
  // Malformed rows: the same refusal as the copying overload, on every thread count.
  const auto mutate = [&](int kind) {
    auto bad = rows;
    auto& row = bad[bad.size() / 2];
    if (kind == 0) row.interior = {5, 3};                      // not ordered
    else if (kind == 1) row = {};                              // empty
    else if (kind == 2) { row.interior = {3}; row.shell = {3}; }  // shell meets interior
    else row.shell = {static_cast<PointId>(domain.size() + 1)};  // outside the domain
    return bad;
  };
  for (int kind = 0; kind < 4; ++kind)
    for (const int threads : {0, 1, 4}) {
      const auto bad = mutate(kind);
      const auto by_copy = build_full_coverage_populations(domain, bad);
      auto moved_rows = bad;
      const auto by_move = build_full_coverage_populations(domain, std::move(moved_rows), threads);
      ++checks;
      if (by_copy.status == FullCertificateStatus::kOk || by_move.status != by_copy.status) {
        std::printf("cause=bank.malformed_status kind=%d threads=%d\n", kind, threads);
        return 1;
      }
    }
#if defined(MHGP9_TESTING)
  // A thread that cannot be launched is a resource status, not an exception.
  parallel_detail::launch_fail_after = 1;
  auto moved_rows = rows;
  FullCoveragePopulationResult failed;
  try {
    failed = build_full_coverage_populations(domain, std::move(moved_rows), 4);
  } catch (...) {
    parallel_detail::launch_fail_after = static_cast<std::size_t>(-1);
    std::printf("cause=bank.launch_failure_escaped\n");
    return 1;
  }
  parallel_detail::launch_fail_after = static_cast<std::size_t>(-1);
  ++checks;
  if (failed.status != FullCertificateStatus::kResourceExhausted) {
    std::printf("cause=bank.launch_failure_status\n");
    return 1;
  }
#endif
  std::printf("population_bank_gate checks=%zu\n", checks);
  return 0;
}
