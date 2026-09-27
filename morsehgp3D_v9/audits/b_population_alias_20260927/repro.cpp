// Audit-only public API reproducer. No casts, races, or private-field access.
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "tower/forest/full_coverage_certificate.hpp"

namespace t = mhgp9::tower;

void need(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

int main(int argc, char** argv) {
  try {
    const bool shift = argc == 2 && std::string_view(argv[1]) == "--shift";
    need(argc == 1 || shift, "usage: repro [--shift]");
    const std::vector<t::PointId> domain{0, 1, 2};
    std::vector<t::FullCoveragePopulation> copied_rows{{{}, {0, 1}}};
    auto* copied_alias = copied_rows.front().shell.data();
    const auto copied = t::build_full_coverage_populations(domain, copied_rows);
    need(copied.status == t::FullCertificateStatus::kOk, "copy.bank_rejected");
    *copied_alias = 99;
    need(copied.value->rows()[0].shell[0] == 0, "copy.not_isolated");

    std::vector<t::FullCoveragePopulation> rows{{{}, {0, 1}}};
    auto* row_alias = rows.data();
    auto* point_alias = rows[0].shell.data();
    const auto bank = t::build_full_coverage_populations(domain, std::move(rows), 1);
    need(bank.status == t::FullCertificateStatus::kOk, "move.bank_rejected");
    need(row_alias == bank.value->rows().data(), "move.row_alias_not_retained");
    need(point_alias == bank.value->rows()[0].shell.data(), "move.point_alias_not_retained");

    const t::ExactLevel level{{1, 0, 0}, 1};
    t::FullCoverageBatch batch;
    batch.level = level;
    t::FullCoverageAction birth;
    birth.contributions.push_back({0, 3, false});
    batch.actions.push_back(std::move(birth));
    const std::vector<t::FullCoverageBatch> batches{batch};
    auto forest = t::build_full_coverage_certificate(2, bank.value, batches);
    need(forest.status == t::FullCertificateStatus::kOk, "forest.initial_rejection");
    const auto before = t::full_coverage_at(forest.value, 0, level, true);
    need(before.status == t::FullCertificateStatus::kOk &&
         before.values == std::vector<t::PointId>{0, 1}, "forest.initial_read");

    if (shift) {
      // The outer vector element is still valid. This resizes its owned shell;
      // point_alias is not dereferenced after this potential reallocation.
      row_alias->shell.resize(32, 2);
      std::cout << "{\"stage\":\"before_product_shift\",\"shell_size\":32}\n" << std::flush;
      const auto rebuilt = t::build_full_coverage_certificate(2, bank.value, batches);
      (void)rebuilt;
      throw std::runtime_error("shift.sanitizer_did_not_stop");
    }

    *point_alias = 99;
    const auto revalidation = t::build_full_coverage_populations(domain, bank.value->rows());
    need(revalidation.status == t::FullCertificateStatus::kInvalidInput,
         "mutation.fresh_validation_did_not_reject");
    const auto after = t::full_coverage_at(forest.value, 0, level, true);
    need(after.status == t::FullCertificateStatus::kOk &&
         after.values == std::vector<t::PointId>{1, 99}, "mutation.certified_reader_not_changed");
    const auto rebuilt = t::build_full_coverage_certificate(2, bank.value, batches);
    need(rebuilt.status == t::FullCertificateStatus::kOk, "mutation.rebuild_rejected");
    std::cout << "{\"schema\":\"mhgp9_population_alias_audit_v2\","
                 "\"status\":\"reproduced\",\"copy_isolated\":true,"
                 "\"move_row_alias_retained\":true,\"move_point_alias_retained\":true,"
                 "\"revalidation_rejects\":true,\"certified_forest_changed\":true,"
                 "\"rebuilt_forest_accepts\":true,\"before\":[0,1],\"after\":[1,99],"
                 "\"GCP_used\":false}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 2;
  }
}
