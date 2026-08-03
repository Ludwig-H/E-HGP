#include "morsehgp3d/hierarchy/direct_projectable_contribution_window.hpp"
#include "scientific_residual_window_fixture.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::test::CompatibilityProvider;
using morsehgp3d::test::ScientificResidualFixture;
using morsehgp3d::test::initialize_capability;
using morsehgp3d::test::residual_scientific_fixture;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

static_assert(
    !std::is_copy_constructible_v<
        ExactDirectProjectableContributionWindowResult> &&
    !std::is_copy_assignable_v<
        ExactDirectProjectableContributionWindowResult> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectProjectableContributionWindowResult> &&
    std::is_nothrow_move_assignable_v<
        ExactDirectProjectableContributionWindowResult> &&
    !std::is_aggregate_v<ExactDirectProjectableContributionWindowResult>);

[[nodiscard]] ExactDirectProjectableContributionWindowBudget
unlimited_contribution_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
  };
}

[[nodiscard]] bool contribution_atom_less_for_test(
    const ExactDirectProjectableContributionAtom& left,
    const ExactDirectProjectableContributionAtom& right) {
  return std::tuple{
             left.source_coface_index,
             left.removed_union_point_index,
             left.stable_source_facet_token_index,
             left.removed_point_id,
             left.source_deletion_reference_index,
             static_cast<std::uint8_t>(left.source_kind),
             left.source_reference_index} <
         std::tuple{
             right.source_coface_index,
             right.removed_union_point_index,
             right.stable_source_facet_token_index,
             right.removed_point_id,
             right.source_deletion_reference_index,
             static_cast<std::uint8_t>(right.source_kind),
             right.source_reference_index};
}

void verify_projection_contents(
    const ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&
        window,
    const ExactDirectProjectableContributionWindowResult& projection) {
  const auto& owned = window.owned_window();
  const auto& plan = owned.local_plan;
  const auto& batch = plan.batches.front();
  const auto& receipt = projection.receipt();
  const auto definitions = projection.facet_definitions();
  const auto points = projection.facet_point_references();
  const auto atoms = projection.contribution_atoms();

  std::vector<std::size_t> expected_touched;
  expected_touched.reserve(plan.coface_facet_references.size());
  for (const auto& reference : plan.coface_facet_references) {
    expected_touched.push_back(reference.facet_token_index);
  }
  std::sort(expected_touched.begin(), expected_touched.end());
  expected_touched.erase(
      std::unique(expected_touched.begin(), expected_touched.end()),
      expected_touched.end());

  std::size_t direct_coface_count = 0U;
  for (const auto& reference : plan.direct_references) {
    direct_coface_count +=
        reference.role == ExactDirectMorseH0Role::saddle ? 1U : 0U;
  }
  const std::size_t residual_coface_count = plan.residual_references.size();
  const std::size_t coface_point_count = batch.order + 1U;
  check(
      projection.certified_exhaustive_projection() &&
          receipt.required_contribution_atom_count ==
              plan.coface_facet_references.size() &&
          atoms.size() == plan.coface_facet_references.size() &&
          definitions.size() == expected_touched.size() &&
          receipt.source_direct_coface_count == direct_coface_count &&
          receipt.source_residual_coface_count == residual_coface_count &&
          receipt.direct_contribution_atom_count ==
              direct_coface_count * coface_point_count &&
          receipt.residual_contribution_atom_count ==
              residual_coface_count * coface_point_count &&
          receipt.source_batch_index == owned.source_batch_index &&
          receipt.exact_squared_radius == batch.squared_level &&
          receipt.source_records_not_filtered_by_h0_node_presence &&
          !receipt.source_exactness_claimed &&
          !receipt.public_status_claimed,
      "one certified projection retains exactly every direct and residual coface deletion atom");

  std::size_t point_cursor = 0U;
  for (std::size_t definition_index = 0U;
       definition_index < definitions.size();
       ++definition_index) {
    const std::size_t local = expected_touched[definition_index];
    const auto& key = plan.facet_tokens[local].facet_key;
    const auto& definition = definitions[definition_index];
    check(
        definition.facet_definition_index == definition_index &&
            definition.stable_source_facet_token_index ==
                owned.local_to_stable_facet_token_indices[local] &&
            definition.order == batch.order &&
            definition.point_reference_offset == point_cursor &&
            definition.point_reference_count == key.point_count &&
            std::equal(
                points.begin() + static_cast<std::ptrdiff_t>(point_cursor),
                points.begin() + static_cast<std::ptrdiff_t>(
                                     point_cursor + key.point_count),
                key.point_ids.begin()),
        "touched facet definitions are stable-id canonical, deduplicated and backed by the exact point CSR");
    point_cursor += key.point_count;
  }
  check(
      point_cursor == points.size(),
      "the touched-facet point CSR has neither a gap nor an unused suffix");

  for (std::size_t atom_index = 0U; atom_index < atoms.size(); ++atom_index) {
    const auto& atom = atoms[atom_index];
    const auto deletion = std::find_if(
        plan.coface_facet_references.begin(),
        plan.coface_facet_references.end(),
        [&](const auto& reference) {
          return reference.coface_facet_reference_index ==
                 atom.source_deletion_reference_index;
        });
    check(
        atom.contribution_atom_index == atom_index &&
            atom.order == batch.order &&
            atom.exact_squared_radius == batch.squared_level &&
            deletion != plan.coface_facet_references.end() &&
            atom.source_coface_index ==
                deletion->source_star_coface_index &&
            atom.removed_union_point_index ==
                deletion->removed_union_point_index &&
            atom.removed_point_id == deletion->removed_point_id &&
            atom.stable_source_facet_token_index ==
                owned.local_to_stable_facet_token_indices
                    [deletion->facet_token_index] &&
            (atom_index == 0U ||
             !contribution_atom_less_for_test(atom, atoms[atom_index - 1U])),
        "each atom retains its exact source coface/deletion identity, stable facet and batch radius in canonical order");
  }
}

void test_real_direct_residual_and_budget_projection(
    const ScientificResidualFixture& fixture) {
  CompatibilityProvider provider{fixture.manifest, fixture.compatibility_plan};
  auto initialized = initialize_capability(
      fixture,
      provider,
      UINT64_C(0xC071C701));
  check(
      initialized.certified_scientific_initialization() &&
          initialized.session.has_value(),
      "the contribution fixture starts from a genuinely authenticated scientific session");
  if (!initialized.session.has_value()) {
    return;
  }
  auto& session = *initialized.session;
  const auto& stamp = session.scientific_source_stamp();
  std::size_t observed_direct_atoms = 0U;
  std::size_t observed_residual_atoms = 0U;
  bool cap_minus_one_checked = false;
  bool canonical_permutation_rejected = false;
  bool move_checked = false;

  while (!session.complete()) {
    auto prepared = session.prepare_next();
    check(
        prepared.certified_scientific_preparation() &&
            prepared.ticket.has_value(),
        "each residual-fixture contribution batch is minted by the scientific window capability");
    if (!prepared.ticket.has_value()) {
      return;
    }
    auto& ticket = *prepared.ticket;
    auto projection = ExactDirectProjectableContributionWindow::build(
        stamp, ticket, unlimited_contribution_budget());
    verify_projection_contents(ticket, projection);
    observed_direct_atoms +=
        projection.receipt().direct_contribution_atom_count;
    observed_residual_atoms +=
        projection.receipt().residual_contribution_atom_count;

    if (!move_checked && projection.certified_exhaustive_projection()) {
      auto moved = std::move(projection);
      check(
          moved.certified_exhaustive_projection() &&
              !projection.certified_exhaustive_projection(),
          "moving a contribution capability transfers its private identities and revokes the source object");
      projection = std::move(moved);
      move_checked = true;
    }

    if (!cap_minus_one_checked &&
        projection.receipt().required_contribution_atom_count != 0U) {
      using BudgetMember =
          std::size_t ExactDirectProjectableContributionWindowBudget::*;
      const std::array<BudgetMember, 9U> members{
          &ExactDirectProjectableContributionWindowBudget::
              maximum_window_facet_scan_count,
          &ExactDirectProjectableContributionWindowBudget::
              maximum_window_facet_point_scan_count,
          &ExactDirectProjectableContributionWindowBudget::
              maximum_direct_reference_scan_count,
          &ExactDirectProjectableContributionWindowBudget::
              maximum_residual_reference_scan_count,
          &ExactDirectProjectableContributionWindowBudget::
              maximum_coface_facet_reference_scan_count,
          &ExactDirectProjectableContributionWindowBudget::
              maximum_touched_facet_definition_count,
          &ExactDirectProjectableContributionWindowBudget::
              maximum_facet_point_reference_count,
          &ExactDirectProjectableContributionWindowBudget::
              maximum_contribution_atom_count,
          &ExactDirectProjectableContributionWindowBudget::
              maximum_scratch_entry_count,
      };
      const auto& projected = projection.receipt();
      const std::array<std::size_t, 9U> required{
          projected.required_window_facet_scan_count,
          projected.required_window_facet_point_scan_count,
          projected.required_direct_reference_scan_count,
          projected.required_residual_reference_scan_count,
          projected.required_coface_facet_reference_scan_count,
          projected.required_touched_facet_definition_count,
          projected.required_facet_point_reference_count,
          projected.required_contribution_atom_count,
          projected.required_scratch_entry_count,
      };
      for (std::size_t index = 0U; index < members.size(); ++index) {
        if (required[index] == 0U) {
          continue;
        }
        auto capped = unlimited_contribution_budget();
        capped.*members[index] = required[index] - 1U;
        const auto rejected =
            ExactDirectProjectableContributionWindow::build(
                stamp, ticket, capped);
        check(
            !rejected.certified_exhaustive_projection() &&
                rejected.receipt().decision ==
                    ExactDirectProjectableContributionWindowDecision::
                        no_projection_budget_exhausted,
            "every positive contribution scan, scratch or output capacity rejects cap minus one");
      }
      cap_minus_one_checked = true;
    }

    auto& mutable_owned = const_cast<
        ExactDirectNormalizedH0ResidentOwnedBatchWindow&>(
        ticket.owned_window());
    if (!canonical_permutation_rejected &&
        mutable_owned.local_plan.coface_facet_references.size() >= 2U) {
      std::swap(
          mutable_owned.local_plan.coface_facet_references[0],
          mutable_owned.local_plan.coface_facet_references[1]);
      const auto rejected =
          ExactDirectProjectableContributionWindow::build(
              stamp, ticket, unlimited_contribution_budget());
      check(
          !rejected.certified_exhaustive_projection() &&
              (rejected.receipt().decision ==
                   ExactDirectProjectableContributionWindowDecision::
                       contradiction_authenticated_window_payload ||
               rejected.receipt().decision ==
                   ExactDirectProjectableContributionWindowDecision::
                       contradiction_batch_or_chain_digest),
          "a physical permutation of authenticated coface deletions cannot mint a differently ordered contribution authority");
      std::swap(
          mutable_owned.local_plan.coface_facet_references[0],
          mutable_owned.local_plan.coface_facet_references[1]);
      check(
          ticket.valid(),
          "restoring the authenticated coface order restores the still-unconsumed scientific ticket");
      canonical_permutation_rejected = true;
    }

    const auto committed = session.commit(std::move(ticket));
    check(
        committed.certified_scientific_commit(),
        "contribution projection never consumes or commits the scientific source window");
  }

  std::size_t expected_residual_atoms = 0U;
  for (const auto& batch : fixture.compatibility_plan.batches) {
    expected_residual_atoms +=
        batch.residual_reference_count * (batch.order + 1U);
  }
  check(
      observed_direct_atoms > 0U &&
          observed_residual_atoms > 0U &&
          observed_residual_atoms == 6U &&
          observed_residual_atoms == expected_residual_atoms &&
          cap_minus_one_checked && canonical_permutation_rejected &&
          move_checked,
      "the finite scientific stream covers direct and residual cofaces, plus canonical rejection, budgets and move-only ownership (direct_atoms=" +
          std::to_string(observed_direct_atoms) +
          ", residual_atoms=" + std::to_string(observed_residual_atoms) +
          ", cap=" + std::to_string(cap_minus_one_checked) +
          ", permutation=" +
          std::to_string(canonical_permutation_rejected) +
          ", move=" + std::to_string(move_checked) + ")");
}

void test_foreign_sibling_private_identity_is_rejected(
    const ScientificResidualFixture& fixture) {
  CompatibilityProvider first_provider{
      fixture.manifest, fixture.compatibility_plan};
  CompatibilityProvider second_provider{
      fixture.manifest, fixture.compatibility_plan};
  constexpr std::uint64_t shared_public_authority = UINT64_C(0xC071C702);
  auto first = initialize_capability(
      fixture,
      first_provider,
      shared_public_authority);
  auto second = initialize_capability(
      fixture,
      second_provider,
      shared_public_authority);
  check(
      first.certified_scientific_initialization() &&
          second.certified_scientific_initialization() &&
          first.session.has_value() && second.session.has_value(),
      "two authentic sibling sessions can expose deliberately equal public scientific values");
  if (!first.session.has_value() || !second.session.has_value()) {
    return;
  }
  auto first_window = first.session->prepare_next();
  auto second_window = second.session->prepare_next();
  if (!first_window.ticket.has_value() || !second_window.ticket.has_value()) {
    check(false, "both authentic sibling sessions prepare their first window");
    return;
  }
  const auto& first_stamp = first.session->scientific_source_stamp();
  const auto& second_stamp = second.session->scientific_source_stamp();
  check(
      first_stamp.scientific_authority_id() ==
              second_stamp.scientific_authority_id() &&
          first_stamp.manifest_digest() == second_stamp.manifest_digest() &&
          first_window.ticket->owned_window().batch_identity_digest ==
              second_window.ticket->owned_window().batch_identity_digest,
      "the sibling rejection fixture is equal in every exposed source and first-batch identity");
  const auto rejected = ExactDirectProjectableContributionWindow::build(
      first_stamp,
      *second_window.ticket,
      unlimited_contribution_budget());
  check(
      !rejected.certified_exhaustive_projection() &&
          rejected.receipt().decision ==
              ExactDirectProjectableContributionWindowDecision::
                  no_private_source_or_window_identity_mismatch,
      "equal public digests cannot rebind a prepared window to a foreign private scientific source control");

  const auto first_valid = ExactDirectProjectableContributionWindow::build(
      first_stamp, *first_window.ticket, unlimited_contribution_budget());
  const auto second_valid = ExactDirectProjectableContributionWindow::build(
      second_stamp, *second_window.ticket, unlimited_contribution_budget());
  check(
      first_valid.certified_exhaustive_projection() &&
          second_valid.certified_exhaustive_projection() &&
          first_valid.receipt().contribution_window_digest ==
              second_valid.receipt().contribution_window_digest,
      "each legitimate sibling pair independently certifies the same canonical public contribution content");
}

void test_default_inputs_and_result_cannot_forge_projection() {
  ExactDirectNormalizedH0ScientificSourceStamp source_stamp;
  ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow window;
  const auto rejected = ExactDirectProjectableContributionWindow::build(
      source_stamp, window, unlimited_contribution_budget());
  ExactDirectProjectableContributionWindowResult result;
  check(
      !rejected.certified_exhaustive_projection() &&
          rejected.receipt().decision ==
              ExactDirectProjectableContributionWindowDecision::
                  no_scientific_source_stamp_rejected &&
          !result.certified_exhaustive_projection() &&
          result.facet_definitions().empty() &&
          result.facet_point_references().empty() &&
          result.contribution_atoms().empty(),
      "default source, window and result objects cannot forge contribution authority");
}

}  // namespace

int main() {
  try {
    failures = 0;
    test_default_inputs_and_result_cannot_forge_projection();
    const ScientificResidualFixture fixture = residual_scientific_fixture();
    test_real_direct_residual_and_budget_projection(fixture);
    test_foreign_sibling_private_identity_is_rejected(fixture);
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
  }
  if (failures != 0) {
    std::cerr << failures
              << " direct projectable contribution-window test(s) failed\n";
    return 1;
  }
  std::cout << "direct projectable contribution-window tests passed\n";
  return 0;
}
