#include "tower/forest/full_coverage_certificate.hpp"

#include <array>
#include <iostream>
#include <span>

int main() {
  using namespace mhgp9::tower;
  const std::array<PointId, 1> domain{0};
  FullCoveragePopulation row;
  row.shell = {0};
  const std::array<FullCoveragePopulation, 1> rows{row};
  const auto bank = build_full_coverage_populations(
      std::span<const PointId>(domain),
      std::span<const FullCoveragePopulation>(rows));
  if (bank.status != FullCertificateStatus::kOk) return 2;

  FullCoverageFlatDraft draft;
  draft.level.push_back(ExactLevel{{0, 0, 0}, 1});
  // Invalid public CSR: one batch requires two batch_begin offsets, but the
  // default vector still has only its initial zero. The API must reject this
  // as kInvalidInput without reading batch_begin[1].
  const auto result = build_full_coverage_certificate(1, bank.value, draft);
  std::cout << "status=" << static_cast<int>(result.status)
            << " reason=" << result.reason << '\n';
  return result.status == FullCertificateStatus::kInvalidInput ? 0 : 1;
}
