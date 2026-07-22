#include "morsehgp3d/gpu/pair_support_phi.hpp"

#include "morsehgp3d/exact/point.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <tuple>

namespace {

using morsehgp3d::gpu::PairSupportPhiBatchResult;
using morsehgp3d::gpu::PairSupportPhiDecision;
using morsehgp3d::gpu::PairSupportPhiWitnessQuery;

[[nodiscard]] auto query_key(const PairSupportPhiWitnessQuery& query) {
  return std::tuple{
      query.first_support_node_index,
      query.second_support_node_index,
      query.witness_node_index};
}

[[nodiscard]] const morsehgp3d::gpu::PairSupportPhiDecisionRecord&
find_decision(
    const PairSupportPhiBatchResult& result,
    const PairSupportPhiWitnessQuery& query) {
  const auto found = std::find_if(
      result.decisions.begin(),
      result.decisions.end(),
      [&query](const auto& record) { return record.query == query; });
  if (found == result.decisions.end()) {
    throw std::runtime_error(
        "the Phase 9 qualification lost a canonical query");
  }
  return *found;
}

void require_result(
    const PairSupportPhiBatchResult& result,
    const PairSupportPhiWitnessQuery& strict_query,
    const PairSupportPhiWitnessQuery& descend_query,
    std::uint64_t expected_epoch) {
  const auto& strict = find_decision(result, strict_query);
  const auto& descend = find_decision(result, descend_query);
  if (strict.decision != PairSupportPhiDecision::certified_strict_interior ||
      !strict.exact_receipt.has_value() ||
      strict.exact_receipt->exact_phi_maximum.maximum_phi !=
          morsehgp3d::exact::ExactRational{-1} ||
      descend.decision != PairSupportPhiDecision::descend ||
      descend.exact_receipt.has_value()) {
    throw std::runtime_error(
        "the Phase 9 qualification decisions differ from the exact fixture");
  }
  const auto& audit = result.audit;
  if (audit.canonical_query_count != 2U ||
      audit.gpu_output_record_count != 2U ||
      audit.gpu_strict_interior_proposal_count != 1U ||
      audit.gpu_requires_descent_count != 1U ||
      audit.gpu_kernel_launch_count != 1U ||
      audit.cpu_exact_phi_recertification_count != 1U ||
      audit.certified_strict_interior_receipt_count != 1U ||
      audit.buffer_epoch != expected_epoch ||
      !audit.minimum_certified_strict_margin.has_value() ||
      *audit.minimum_certified_strict_margin !=
          morsehgp3d::exact::ExactRational{1} ||
      !audit.immutable_lbvh_snapshot_validated ||
      !audit.canonical_query_order_validated ||
      !audit.exhaustive_proposal_permutation_validated ||
      !audit.cpu_exact_recertification_complete ||
      audit.global_support_product_prune_published ||
      audit.public_status_published) {
    throw std::runtime_error(
        "the Phase 9 qualification audit does not close");
  }
}

[[nodiscard]] bool deterministic_replay(
    const PairSupportPhiBatchResult& first,
    const PairSupportPhiBatchResult& second) {
  return first.proposals == second.proposals &&
         first.decisions == second.decisions &&
         first.audit.proposal_digest_fnv1a ==
             second.audit.proposal_digest_fnv1a &&
         first.audit.gpu_strict_interior_proposal_count ==
             second.audit.gpu_strict_interior_proposal_count &&
         first.audit.gpu_requires_descent_count ==
             second.audit.gpu_requires_descent_count &&
         first.audit.certified_strict_interior_receipt_count ==
             second.audit.certified_strict_interior_receipt_count;
}

}  // namespace

int main() {
  try {
    using morsehgp3d::exact::CertifiedPoint3;
    using morsehgp3d::gpu::PairSupportPhiContext;
    using morsehgp3d::spatial::CanonicalPointCloud;
    using morsehgp3d::spatial::MortonLbvhIndex;

    const std::array input_points{
        CertifiedPoint3::from_binary64(-1.0, 0.0, 0.0),
        CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
        CertifiedPoint3::from_binary64(1.0, 0.0, 0.0)};
    CanonicalPointCloud cloud =
        CanonicalPointCloud::rejecting_duplicates(input_points);
    MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    PairSupportPhiContext context{index, cloud, 2U};

    const PairSupportPhiWitnessQuery strict_query =
        context.make_leaf_witness_query(0U, 2U, 1U);
    const PairSupportPhiWitnessQuery descend_query =
        context.make_leaf_witness_query(0U, 1U, 2U);
    std::array queries{strict_query, descend_query};
    std::sort(
        queries.begin(),
        queries.end(),
        [](const auto& left, const auto& right) {
          return query_key(left) < query_key(right);
        });

    const PairSupportPhiBatchResult first =
        context.classify_witnesses(std::span{queries});
    const PairSupportPhiBatchResult second =
        context.classify_witnesses(std::span{queries});
    require_result(first, strict_query, descend_query, 1U);
    require_result(second, strict_query, descend_query, 2U);
    if (!deterministic_replay(first, second) || context.node_count() != 5U) {
      throw std::runtime_error(
          "the Phase 9 pair-support phi replay is not deterministic");
    }

    std::cout
        << "{\"backend\":\"cuda_g4\","
        << "\"certified_receipt_count\":1,"
        << "\"decision_semantics\":\""
        << morsehgp3d::gpu::PairSupportPhiAudit::decision_semantics << "\","
        << "\"descend_count\":1,"
        << "\"deterministic\":true,"
        << "\"first_epoch\":1,"
        << "\"global_support_product_prune_published\":false,"
        << "\"mode\":\"certified\","
        << "\"node_count\":5,"
        << "\"phase\":\"9\","
        << "\"profile\":\"hgp_reduced\","
        << "\"proposal_digest_fnv1a\":"
        << first.audit.proposal_digest_fnv1a << ','
        << "\"proposal_semantics\":\""
        << morsehgp3d::gpu::PairSupportPhiAudit::proposal_semantics << "\","
        << "\"public_status\":null,"
        << "\"schema\":\"morsehgp3d.phase9.pair_support_phi_cuda_qualification.v1\","
        << "\"second_epoch\":2,"
        << "\"strict_interior_count\":1}"
        << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Phase 9 pair-support phi qualification failed: "
              << error.what() << '\n';
    return 1;
  }
}
