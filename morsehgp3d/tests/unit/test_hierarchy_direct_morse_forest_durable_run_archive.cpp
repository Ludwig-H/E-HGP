#include "morsehgp3d/hierarchy/direct_morse_forest_durable_run_archive.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::contract::CanonicalId;
using morsehgp3d::contract::CanonicalSha256Builder;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactCenter3;
using morsehgp3d::exact::ExactLevel;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CanonicalId digest(std::string_view text) {
  CanonicalSha256Builder builder;
  builder.update(text);
  return builder.finalize();
}

[[nodiscard]] ExactDirectMorseForestSegmentCodecLimits generous_limits() {
  ExactDirectMorseForestSegmentCodecLimits limits;
  limits.segment_limits = {64U, 64U, 64U, 64U, 64U, 64U};
  limits.maximum_final_root_count = 64U;
  return limits;
}

[[nodiscard]] ExactDirectSparseFacetKey facet_key(
    std::initializer_list<std::uint64_t> ids) {
  ExactDirectSparseFacetKey key{};
  key.point_count = ids.size();
  std::size_t index = 0U;
  for (const auto id : ids) {
    key.point_ids[index++] = id;
  }
  return key;
}

// One synthetic non-singleton committed segment whose cursors, batch facts
// and logical-output arithmetic satisfy certified_structure() exactly, with
// adversarial exact payloads (huge reduced rationals, negative center
// numerators, both optional shapes, every enum kind reachable in one
// segment).
[[nodiscard]] ExactDirectMorseForestBatchSegment make_segment() {
  ExactDirectMorseForestBatchSegment segment{};
  segment.begin_cursor = {
      1U, 5U, 2U, 1U, 1U, 1U, 2U, 1U, 3U, 100U, digest("begin-chain")};
  segment.payload_digest = digest("resident-payload");

  segment.birth_records.push_back(
      {2U,
       7U,
       1U,
       2U,
       facet_key({4U, 9U}),
       11U,
       std::nullopt,
       {21U, 43U}});
  segment.birth_records.push_back(
      {3U,
       8U,
       1U,
       3U,
       facet_key({1U, 5U, 6U}),
       12U,
       ExactDirectMorseForestNodeId{77U},
       {21U, 44U}});

  ExactDirectMorseForestArmRootBinding binding{};
  binding.binding_index = 1U;
  binding.source_arm_seed_index = 6U;
  binding.source_family_index = 2U;
  binding.strict_arm_key = facet_key({3U, 8U});
  binding.frozen_carrier_component_handle = 9U;
  binding.prior_reduced_root_node_id = ExactDirectMorseForestNodeId{55U};
  binding.terminal_birth_record_index = 2U;
  binding.terminal_birth_facet_key = facet_key({4U, 9U});
  binding.terminal_birth_binding_witness = {21U, 43U};
  binding.removed_support_point_id = 17U;
  binding.terminal_birth_exact_center = ExactCenter3{
      BigInt{"-123456789012345678901234567890123456789"},
      BigInt{"98765432109876543210987654321"},
      BigInt{"-3"},
      BigInt{"123456789012345678901234567891"}};
  binding.terminal_birth_exact_squared_level = ExactLevel{
      BigInt{"340282366920938463463374607431768211455"},
      BigInt{"170141183460469231731687303715884105727"}};
  segment.arm_root_bindings.push_back(binding);

  segment.saddle_records.push_back(
      {1U, 2U, 5U, 1U, 1U, 1U, 1U, 0U, 1U, 1U, 9U, digest("saddle-arms")});

  segment.atomic_groups.push_back(
      {1U,
       1U,
       1U,
       1U,
       1U,
       0U,
       1U,
       2U,
       2U,
       ExactDirectMorseForestNodeId{88U},
       88U,
       ExactDirectMorseForestAtomicGroupKind::multifusion});

  segment.child_node_ids = {55U, 77U};

  ExactDirectMorseForestNode first_node{};
  first_node.node_id = 77U;
  first_node.order = 3U;
  first_node.squared_level =
      ExactLevel{BigInt{"7"}, BigInt{"12"}};
  first_node.kind = ExactDirectMorseForestNodeKind::order_one_birth;
  first_node.child_offset = 0U;
  first_node.child_count = 0U;
  first_node.birth_record_index = 3U;
  first_node.atomic_group_index = std::nullopt;
  ExactDirectMorseForestNode second_node{};
  second_node.node_id = 88U;
  second_node.order = 2U;
  second_node.squared_level = ExactLevel{BigInt{"0"}};
  second_node.kind = ExactDirectMorseForestNodeKind::multifusion;
  second_node.child_offset = 2U;
  second_node.child_count = 2U;
  second_node.birth_record_index = std::nullopt;
  second_node.atomic_group_index = 1U;
  segment.nodes = {first_node, second_node};

  segment.batch.batch_index = 1U;
  segment.batch.source_journal_batch_index = 1U;
  segment.batch.order = 2U;
  segment.batch.squared_level = ExactLevel{BigInt{"5"}, BigInt{"8"}};
  segment.batch.birth_record_offset = 7U;
  segment.batch.birth_record_count = segment.birth_records.size();
  segment.batch.saddle_record_offset = 1U;
  segment.batch.saddle_record_count = segment.saddle_records.size();
  segment.batch.atomic_group_offset = 1U;
  segment.batch.atomic_group_count = segment.atomic_groups.size();
  segment.batch.strict_pre_batch_carrier_count = 6U;
  segment.batch.strict_pre_batch_reduced_root_count = 2U;
  segment.batch.closed_post_batch_carrier_count = 5U;
  segment.batch.closed_post_batch_reduced_root_count = 3U;
  segment.batch.strict_pre_batch_stamp = {
      3U, 21U, 4U, 30U, 12U, 18U, digest("pre-stamp")};
  segment.batch.committed_batch_stamp = {
      3U, 21U, 5U, 33U, 14U, 19U, digest("committed-stamp")};
  segment.batch.strict_arms_resolved_before_mutation = true;
  segment.batch.quotient_resolved_before_mutation = true;
  segment.batch.unions_then_births_committed_atomically = true;

  ExactDirectMorseForestSegmentCursor end = segment.begin_cursor;
  end.segment_count += 1U;
  end.birth_record_count += segment.birth_records.size();
  end.arm_root_binding_count += segment.arm_root_bindings.size();
  end.saddle_record_count += segment.saddle_records.size();
  end.atomic_group_count += segment.atomic_groups.size();
  end.child_reference_count += segment.child_node_ids.size();
  end.batch_record_count += 1U;
  end.node_count += segment.nodes.size();
  std::size_t logical = segment.begin_cursor.logical_output_entry_count;
  for (const auto& record : segment.birth_records) {
    logical += record.facet_key.point_count;
  }
  for (const auto& record : segment.arm_root_bindings) {
    logical += record.strict_arm_key.point_count;
  }
  logical += segment.birth_records.size() +
             segment.arm_root_bindings.size() +
             segment.saddle_records.size() + segment.atomic_groups.size() +
             segment.child_node_ids.size() + 1U + segment.nodes.size();
  end.logical_output_entry_count = logical;
  segment.end_cursor = end;
  segment.payload_digest =
      exact_direct_morse_forest_segment_payload_digest(segment);
  segment.end_cursor.chain_digest =
      exact_direct_morse_forest_segment_chain_digest(segment);
  return segment;
}

[[nodiscard]] ExactDirectMorseForestFinalSeal make_seal(
    const ExactDirectMorseForestBatchSegment& segment) {
  ExactDirectMorseForestFinalSeal seal{};
  seal.point_count = 40U;
  seal.effective_maximum_order = 3U;
  seal.source_higher_canonical_cloud_digest = digest("cloud");
  seal.final_cursor = segment.end_cursor;
  seal.final_locator_stamp = segment.batch.committed_batch_stamp;
  seal.counters.birth_record_count = 4U;
  seal.counters.node_count = 5U;
  seal.counters.final_root_count = 2U;
  seal.counters.maximum_batch_merge_arity = 2U;
  seal.final_roots.push_back({0U, 1U, 3U, 12U});
  seal.final_roots.push_back({1U, 2U, 9U, 88U});
  seal.logical_output_entry_count =
      segment.end_cursor.logical_output_entry_count;
  seal.all_source_batches_committed = true;
  seal.every_segment_acknowledged = true;
  return seal;
}

void test_round_trip_and_determinism() {
  const auto limits = generous_limits();
  const auto segment = make_segment();
  check(
      segment.certified_structure(),
      "the synthetic committed segment satisfies the 15I invariant");
  const auto encoded =
      encode_exact_direct_morse_forest_batch_segment(segment, limits);
  check(
      encoded.certified_encoded(),
      "the canonical segment encoding certifies");
  const auto encoded_again =
      encode_exact_direct_morse_forest_batch_segment(segment, limits);
  check(
      encoded_again.certified_encoded() &&
          encoded_again.bytes == encoded.bytes &&
          encoded_again.encoded_payload_digest ==
              encoded.encoded_payload_digest,
      "the canonical segment encoding is deterministic");
  const auto decoded = decode_exact_direct_morse_forest_batch_segment(
      encoded.bytes, limits);
  check(
      decoded.certified_decoded() && decoded.segment == segment &&
          decoded.consumed_byte_count == encoded.bytes.size(),
      "the decoded segment is field-exact");

  const auto seal = make_seal(segment);
  const auto encoded_seal =
      encode_exact_direct_morse_forest_final_seal(seal, limits);
  check(
      encoded_seal.certified_encoded(),
      "the canonical final-seal encoding certifies");
  const auto decoded_seal = decode_exact_direct_morse_forest_final_seal(
      encoded_seal.bytes, limits);
  check(
      decoded_seal.certified_decoded() && decoded_seal.seal == seal,
      "the decoded final seal is field-exact");
  check(
      encode_exact_direct_morse_forest_final_seal(seal, limits).bytes ==
          encoded_seal.bytes,
      "the canonical final-seal encoding is deterministic");
}

void test_corruption_truncation_and_suffix() {
  const auto limits = generous_limits();
  const auto segment = make_segment();
  const auto encoded =
      encode_exact_direct_morse_forest_batch_segment(segment, limits);
  check(encoded.certified_encoded(), "the corruption fixture encodes");
  if (!encoded.certified_encoded()) {
    return;
  }

  const std::vector<std::size_t> flip_positions{
      0U, 9U, 20U, 40U, 70U, encoded.bytes.size() - 1U};
  for (const std::size_t position : flip_positions) {
    auto corrupted = encoded.bytes;
    corrupted[position] ^= 0x5AU;
    const auto decoded = decode_exact_direct_morse_forest_batch_segment(
        corrupted, limits);
    check(
        !decoded.certified_decoded(),
        "a corrupted byte at position " + std::to_string(position) +
            " rejects fail-closed");
  }
  for (const std::size_t length :
       {std::size_t{0U}, std::size_t{63U}, encoded.bytes.size() - 1U}) {
    const auto decoded = decode_exact_direct_morse_forest_batch_segment(
        std::span<const std::uint8_t>{encoded.bytes.data(), length},
        limits);
    check(
        !decoded.certified_decoded() &&
            decoded.decision ==
                ExactDirectMorseForestSegmentCodecDecision::
                    no_truncation_or_suffix_rejected,
        "a truncation to " + std::to_string(length) + " bytes rejects");
  }
  auto suffixed = encoded.bytes;
  suffixed.push_back(0U);
  const auto suffix_decoded =
      decode_exact_direct_morse_forest_batch_segment(suffixed, limits);
  check(
      !suffix_decoded.certified_decoded() &&
          suffix_decoded.decision ==
              ExactDirectMorseForestSegmentCodecDecision::
                  no_truncation_or_suffix_rejected,
      "a one-byte suffix rejects");
}

void test_cap_minus_one_classes() {
  const auto segment = make_segment();
  const auto generous = generous_limits();
  const auto encoded =
      encode_exact_direct_morse_forest_batch_segment(segment, generous);
  check(encoded.certified_encoded(), "the cap fixture encodes");
  if (!encoded.certified_encoded()) {
    return;
  }

  auto record_capped = generous;
  record_capped.segment_limits.maximum_birth_record_count =
      segment.birth_records.size() - 1U;
  check(
      encode_exact_direct_morse_forest_batch_segment(segment, record_capped)
              .decision ==
          ExactDirectMorseForestSegmentCodecDecision::
              no_record_limit_exceeded,
      "encoding one record above the cap rejects");
  check(
      decode_exact_direct_morse_forest_batch_segment(
          encoded.bytes, record_capped)
              .decision ==
          ExactDirectMorseForestSegmentCodecDecision::
              no_record_limit_exceeded,
      "decoding one record above the cap rejects");

  auto text_capped = generous;
  text_capped.maximum_exact_text_byte_count = 8U;
  check(
      encode_exact_direct_morse_forest_batch_segment(segment, text_capped)
              .decision ==
          ExactDirectMorseForestSegmentCodecDecision::
              no_exact_text_limit_exceeded,
      "encoding a long exact text above the per-text cap rejects");

  auto total_text_capped = generous;
  total_text_capped.maximum_total_exact_text_byte_count = 64U;
  check(
      encode_exact_direct_morse_forest_batch_segment(
          segment, total_text_capped)
              .decision ==
          ExactDirectMorseForestSegmentCodecDecision::
              no_total_exact_text_limit_exceeded,
      "encoding above the cumulative exact-text cap rejects");

  auto byte_capped = generous;
  byte_capped.maximum_encoded_byte_count = encoded.bytes.size() - 1U;
  check(
      encode_exact_direct_morse_forest_batch_segment(segment, byte_capped)
              .decision ==
          ExactDirectMorseForestSegmentCodecDecision::
              no_encoded_byte_limit_exceeded,
      "encoding one byte above the envelope cap rejects");
  check(
      decode_exact_direct_morse_forest_batch_segment(
          encoded.bytes, byte_capped)
              .decision ==
          ExactDirectMorseForestSegmentCodecDecision::
              no_encoded_byte_limit_exceeded,
      "decoding one byte above the envelope cap rejects");

  auto structural = segment;
  structural.end_cursor.node_count += 1U;
  check(
      !structural.certified_structure() &&
          encode_exact_direct_morse_forest_batch_segment(
              structural, generous)
                  .decision ==
              ExactDirectMorseForestSegmentCodecDecision::
                  no_schema_rejected,
      "a structurally broken segment never encodes");

  const auto seal = make_seal(segment);
  auto root_capped = generous;
  root_capped.maximum_final_root_count = seal.final_roots.size() - 1U;
  check(
      encode_exact_direct_morse_forest_final_seal(seal, root_capped)
              .decision ==
          ExactDirectMorseForestSegmentCodecDecision::
              no_record_limit_exceeded,
      "encoding one final root above the cap rejects");
}

}  // namespace

int main() {
  try {
    test_round_trip_and_determinism();
    test_corruption_truncation_and_suffix();
    test_cap_minus_one_classes();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
    return 1;
  }
  if (failures != 0) {
    std::cerr << failures << " durable run archive codec checks failed\n";
    return 1;
  }
  std::cout << "durable run archive codec checks passed\n";
  return 0;
}
