// MorseHGP3D v9 — porte de la banque de populations FULL (surcharge deplacee).
//
// build_full_coverage_populations(domain, rows&&, threads) doit rendre la meme
// banque que la surcharge copiante sur des lignes valides, a 0, 1, 2, 4 et 8
// fils ; les memes refus sur des lignes invalides (non triees, vides, coquille
// qui recoupe l'interieur, point hors domaine) ; et un STATUT (jamais une
// exception) quand le lancement d'un fil echoue (crochet MHGP9_TESTING).
// v9 E4 : appartenance par domaine dense (0..n-1 : p < n), jugee contre un
// juge lineaire de la porte sur des domaines dense, decale, presque dense et
// creux, aux ids de bord ; mutant MHGP9_BANK_MUTANT_DENSE_INCLUSIVE tue.
//
//   mhgp9_tower_population_bank_gate --selftest
//
// Code 0 conforme, 1 desaccord (ligne `cause=`), 2 argument, 3 plancher.
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <limits>
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
  // v9 E4: membership by dense domain. The bank decides p in {0..n-1} by
  // p < n when the domain is exactly 0..n-1, by binary search otherwise. Every
  // decision must equal this gate's own linear judge, on a dense domain, a
  // shifted domain 1..n (strictly ordered, not dense), a domain 0..n-2 plus
  // a far last id (first 0, not dense) and a sparse one, for rows naming the
  // boundary ids and ids around every hole, on both overloads and 0, 1 and
  // 4 threads (single-row and whole banks).
  {
    const auto judge = [](const std::vector<PointId>& d, const FullCoveragePopulation& row) {
      const auto member = [&](PointId p) { return std::find(d.begin(), d.end(), p) != d.end(); };
      const auto ordered = [](const std::vector<PointId>& v) {
        for (std::size_t i = 1; i < v.size(); ++i) if (!(v[i - 1] < v[i])) return false;
        return true;
      };
      if (row.shell.size() > 16 || (row.interior.empty() && row.shell.empty()) || !ordered(row.interior) ||
          !ordered(row.shell)) return false;
      for (const auto* part : {&row.interior, &row.shell})
        for (const PointId p : *part) if (!member(p)) return false;
      for (const PointId p : row.shell)
        if (std::find(row.interior.begin(), row.interior.end(), p) != row.interior.end()) return false;
      return true;
    };
    const std::size_t n = 61;
    std::vector<std::vector<PointId>> domains(4);
    for (std::size_t i = 0; i < n; ++i) domains[0].push_back(static_cast<PointId>(i));      // dense
    for (std::size_t i = 0; i < n; ++i) domains[1].push_back(static_cast<PointId>(i + 1));  // shifted
    for (std::size_t i = 0; i + 1 < n; ++i) domains[2].push_back(static_cast<PointId>(i));  // 0..n-2 and a far id
    domains[2].push_back(static_cast<PointId>(4 * n));
    for (std::size_t i = 0; i < n; ++i) domains[3].push_back(static_cast<PointId>(3 * i + (i % 2)));  // sparse
    std::size_t dense_rejected_n = 0, dense_accepted_last = 0, judged = 0, accepted = 0, rejected = 0;
    for (std::size_t d = 0; d < domains.size(); ++d) {
      const auto& domain_d = domains[d];
      std::vector<PointId> probes = {0, 1, static_cast<PointId>(n - 2), static_cast<PointId>(n - 1),
                                     static_cast<PointId>(n), static_cast<PointId>(n + 1), static_cast<PointId>(4 * n),
                                     static_cast<PointId>(4 * n + 1), std::numeric_limits<PointId>::max()};
      for (const PointId member : domain_d)
        for (const PointId delta : {PointId{0}, PointId{1}}) probes.push_back(member + delta);
      std::vector<FullCoveragePopulation> whole;
      for (const PointId p : probes) {
        for (int shape = 0; shape < 3; ++shape) {
          FullCoveragePopulation row;
          if (shape == 0) row.shell = {p};
          else if (shape == 1) row.interior = {p};
          else { row.interior = {domain_d.front()}; if (p > domain_d.front()) row.shell = {p}; else row.shell = {domain_d.back()}; }
          const bool expected = judge(domain_d, row);
          const auto copied_one = build_full_coverage_populations(domain_d, std::vector<FullCoveragePopulation>{row});
          for (const int threads : {0, 1, 4}) {
            std::vector<FullCoveragePopulation> one{row};
            const auto moved_one = build_full_coverage_populations(domain_d, std::move(one), threads);
            ++checks; ++judged;
            if ((moved_one.status == FullCertificateStatus::kOk) != expected ||
                (copied_one.status == FullCertificateStatus::kOk) != expected) {
              std::printf("cause=bank.dense_membership domain=%zu id=%u shape=%d expected=%d\n", d,
                          static_cast<unsigned>(p), shape, expected ? 1 : 0);
              return 1;
            }
          }
          if (expected) { ++accepted; whole.push_back(row); } else ++rejected;
          if (d == 0 && shape == 0 && p == n) ++dense_rejected_n;
          if (d == 0 && shape == 0 && p == n - 1 && expected) ++dense_accepted_last;
        }
      }
      // A whole bank of accepted rows, then the same bank with one rejected
      // boundary row appended: accepted, then refused, on both overloads.
      for (const bool spoil : {false, true}) {
        auto rows_d = whole;
        if (spoil) rows_d.push_back(FullCoveragePopulation{{}, {static_cast<PointId>(domain_d.back() + 1)}});
        const auto by_copy = build_full_coverage_populations(domain_d, rows_d);
        auto moved_d = rows_d;
        const auto by_move = build_full_coverage_populations(domain_d, std::move(moved_d), 4);
        ++checks;
        if ((by_copy.status == FullCertificateStatus::kOk) == spoil || by_move.status != by_copy.status) {
          std::printf("cause=bank.dense_membership_whole domain=%zu spoil=%d\n", d, spoil ? 1 : 0);
          return 1;
        }
      }
    }
    std::printf("population_bank_dense judged=%zu accepted=%zu rejected=%zu\n", judged, accepted, rejected);
    if (dense_rejected_n == 0 || dense_accepted_last == 0 || accepted < 100 || rejected < 100) {
      std::printf("cause=floor.dense_membership\n");
      return 3;
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
