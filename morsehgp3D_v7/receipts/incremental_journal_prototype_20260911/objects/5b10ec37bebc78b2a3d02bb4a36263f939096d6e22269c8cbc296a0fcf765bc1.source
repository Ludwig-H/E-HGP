// Small probe-policy gate, not a geometry oracle or an engine benchmark.
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

#include "../bench/full_gabriel_probe_limits.hpp"

#ifdef MHGP7_TESTING
#error "probe limits gate uses nominal product headers"
#endif

namespace {
using namespace mhgp7;
namespace policy = mhgp7::full_probe_limits;
u64 checks = 0;

void need(bool ok, const char* reason) {
  ++checks;
  if (!ok) throw std::runtime_error(reason);
}

void storage() {
  constexpr u64 bytes = 8ull << 30;
  const auto p = policy::make(bytes);
  const auto& c = p.full;
  need(p.valid, "policy.valid");
  need(c.max_points == std::min(policy::elements<InputPoint>(bytes), kMaxTreePositions), "storage.points");
  need(c.max_points > 50'000, "storage.not_old_32k");
  need(c.max_input_records == std::min(policy::elements<ForestEvent>(bytes),
       policy::elements<full_gabriel_detail::Record>(bytes)), "storage.input");
  need(c.max_aliases == policy::elements<policy::AliasEntryPayload>(bytes) &&
       p.max_cache_entries == c.max_aliases && p.max_cache_entries > 1'000'000, "storage.alias_cache");
  need(c.certificate.max_batches == policy::elements<FullBatch>(bytes), "storage.batches");
  need(c.certificate.max_nodes == std::min({policy::elements<FullNode>(bytes),
       policy::elements<FacetKey>(bytes), policy::elements<FullNodeId>(bytes), policy::elements<u8>(bytes)}), "storage.nodes");
  need(c.certificate.max_parent_refs == policy::elements<FullNodeId>(bytes), "storage.parents");
  need(p.max_read_point_refs == policy::elements<PointId>(bytes) && p.max_read_point_refs > 40'000'000,
       "storage.read_refs_not_old_40M");
  const std::array<u64, 7> counters{c.max_face_visits, c.max_portal_requests, c.max_chain_steps,
      c.max_meb_calls, c.max_query_nodes, c.max_meb_supports, c.max_successor_steps};
  for (u64 cap : counters) need(cap == policy::kCounterCeiling, "work.representation_ceiling");
  need(c.max_meb_proposal_supports == 0, "proposal.default_opt_out");
  need(kMaxRawCandidates == 0xFFFFFFFFull && kMaxWaveTasks == 0xFFFFFFFFull &&
       kMaxAliveRects == 0xFFFFFFFFull, "generation.structural_u32");
  need(meb_proposal_detail::kMaxPivots == 16, "proposal.algorithmic_pivot_bound");
  u64 scratch = 0;
  need(policy::product_sum(c.certificate.max_nodes, 3 * sizeof(FullNodeId) + sizeof(u8),
       c.certificate.max_parent_refs, sizeof(FullNodeId), scratch) &&
       scratch == p.max_digest_scratch_logical_bytes, "storage.digest_sum");
  need(policy::elements<u64>(0) == 0 && policy::elements<u64>(sizeof(u64) - 1) == 0 &&
       policy::elements<u64>(sizeof(u64)) == 1, "storage.byte_frontier");
  need(policy::elements<u64>(policy::kCounterCeiling) == policy::kDifferenceMax / sizeof(u64),
       "storage.addressable_frontier");
  const auto half = policy::make(bytes / 2);
  need(half.full.max_input_records <= c.max_input_records && half.max_cache_entries <= p.max_cache_entries &&
       half.full.certificate.max_nodes <= c.certificate.max_nodes, "storage.memory_monotone");
}

void arithmetic() {
  constexpr u64 max = policy::kCounterCeiling;
  u64 value = 17;
  need(policy::product_sum(max - 1, 1, 1, 1, value) && value == max, "sum.exact_MAX");
  value = 17;
  need(!policy::product_sum(max, 1, 1, 1, value) && value == 17, "sum.no_wrap");
  need(!policy::product_sum(max, 2, 0, 0, value) && value == 17, "multiply.no_wrap");
  need(policy::product_sum(max, 0, max, 0, value) && value == 0, "multiply.zero");
  u64 count = max - 1;
  need(full_certificate_detail::add(count, 1, max) && count == max, "actual_add.exact_MAX");
  need(!full_certificate_detail::add(count, 1, max) && count == max, "actual_add.no_wrap");
  std::vector<FullNodeId> successor{0};
  FullNodeId root = 99;
  u64 steps = max - 1, normalized = 0;
  need(full_gabriel_detail::normalize_successor(successor, 0, root, steps, normalized, max) ==
       full_gabriel_detail::SuccessorStatus::kOk && steps == max && root == 0 && normalized == 0,
       "actual_successor.exact_MAX");
  need(full_gabriel_detail::normalize_successor(successor, 0, root, steps, normalized, max) ==
       full_gabriel_detail::SuccessorStatus::kBudget && steps == max && normalized == 0,
       "actual_successor.no_wrap");
  successor = {1, 1}; steps = max - 2; normalized = max - 2;
  need(full_gabriel_detail::normalize_successor(successor, 0, root, steps, normalized, max) ==
       full_gabriel_detail::SuccessorStatus::kOk && steps == max && normalized == max - 1 && root == 1,
       "actual_successor.normalized_dominated");
}

void parsing() {
  u64 value = 17;
  policy::ProposalKind kind = policy::ProposalKind::kDisabled;
  for (const char* text : {"0", "1", "584000001", "18446744073709551614", "18446744073709551615"})
    need(policy::proposal(text, value, kind), "parse.full_unsigned_domain");
  need(value == policy::kCounterCeiling && kind == policy::ProposalKind::kFinite, "parse.numeric_MAX_finite");
  need(policy::proposal("unlimited", value, kind) && value == policy::kCounterCeiling &&
       kind == policy::ProposalKind::kUnlimited, "parse.unlimited_distinct");
  need(policy::proposal("0", value, kind) && value == 0 && kind == policy::ProposalKind::kDisabled,
       "parse.zero_disabled");
  for (const char* text : {"", "-1", "+1", " 1", "1 ", "1.0", "1e3", "18446744073709551616",
                           "184467440737095516150", "Unlimited", "unlimited1", "unlimited "}) {
    value = 17; kind = policy::ProposalKind::kFinite;
    need(!policy::proposal(text, value, kind) && value == 17 && kind == policy::ProposalKind::kFinite,
         "parse.reject_without_mutation");
  }
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || (std::strcmp(argv[1], "--selftest") != 0 && std::strcmp(argv[1], "--rejects") != 0)) return 2;
  try {
    storage(); arithmetic(); parsing();
    if (checks != 52) throw std::runtime_error("nonvacuum.expected_52");
    std::printf("{\"schema\":\"mhgp7-full-probe-limits-gate-v1\",\"status\":\"passed\","
                "\"public_status\":\"not_claimed\",\"checks\":%llu,\"work_ceilings\":7,"
                "\"actual_successor_MAX_cases\":3,\"full_hierarchy_built\":false}\n",
                static_cast<unsigned long long>(checks));
    return 0;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "probe limits rejected: %s\n", error.what());
    return 1;
  }
}
