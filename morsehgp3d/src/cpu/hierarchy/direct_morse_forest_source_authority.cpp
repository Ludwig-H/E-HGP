#include "morsehgp3d/hierarchy/direct_morse_forest_source_authority.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::string_view source_identity_domain =
    "MorseHGP3D/phase15/source-authority/identity/v1/sha256/";
constexpr std::string_view initial_chain_domain =
    "MorseHGP3D/phase15/source-authority/initial/v1/sha256/";
constexpr std::string_view batch_identity_domain =
    "MorseHGP3D/phase15/source-authority/batch/v1/sha256/";
constexpr std::string_view chain_step_domain =
    "MorseHGP3D/phase15/source-authority/chain-step/v1/sha256/";
constexpr std::string_view manifest_domain =
    "MorseHGP3D/phase15/source-authority/manifest/v1/sha256/";

void append_bytes(
    contract::CanonicalSha256Builder& builder,
    std::span<const std::uint8_t> bytes) {
  builder.update(bytes);
}

void append_u8(
    contract::CanonicalSha256Builder& builder, std::uint8_t value) {
  const std::array<std::uint8_t, 1U> bytes{value};
  append_bytes(builder, bytes);
}

void append_u32(
    contract::CanonicalSha256Builder& builder, std::uint32_t value) {
  std::array<std::uint8_t, 4U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - 1U - index) * 8U));
  }
  append_bytes(builder, bytes);
}

void append_u64(
    contract::CanonicalSha256Builder& builder, std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - 1U - index) * 8U));
  }
  append_bytes(builder, bytes);
}

void append_size(
    contract::CanonicalSha256Builder& builder, std::size_t value) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  append_u64(builder, static_cast<std::uint64_t>(value));
}

void append_bool(
    contract::CanonicalSha256Builder& builder, bool value) {
  append_u8(builder, value ? std::uint8_t{1U} : std::uint8_t{0U});
}

void append_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  append_bytes(builder, value.bytes());
}

void append_text(
    contract::CanonicalSha256Builder& builder, std::string_view text) {
  append_size(builder, text.size());
  builder.update(text);
}

void append_level(
    contract::CanonicalSha256Builder& builder,
    const exact::ExactLevel& level) {
  const std::string key = level.canonical_key();
  append_text(builder, key);
}

void append_event(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSupportEvent& event) {
  if (event.support_size == 0U ||
      static_cast<std::size_t>(event.support_size) >
          event.support_ids.size()) {
    throw std::logic_error(
        "a source window event has an invalid support size");
  }
  append_size(builder, event.event_index);
  append_u8(builder, event.support_size);
  for (std::size_t index = 0U;
       index < static_cast<std::size_t>(event.support_size);
       ++index) {
    append_u64(builder, static_cast<std::uint64_t>(event.support_ids[index]));
  }
  append_size(builder, event.interior_ids.size());
  for (const spatial::PointId point_id : event.interior_ids) {
    append_u64(builder, static_cast<std::uint64_t>(point_id));
  }
  append_level(builder, event.squared_level);
  append_size(builder, event.closed_rank);
  append_bool(builder, event.birth_order.has_value());
  if (event.birth_order.has_value()) {
    append_size(builder, *event.birth_order);
  }
  append_bool(builder, event.saddle_order.has_value());
  if (event.saddle_order.has_value()) {
    append_size(builder, *event.saddle_order);
  }
}

[[nodiscard]] contract::CanonicalId compute_source_identity_digest(
    const ExactDirectMorseForestSourceManifest& manifest) {
  contract::CanonicalSha256Builder builder;
  builder.update(source_identity_domain);
  append_u32(builder, manifest.schema_version);
  append_u32(builder, manifest.event_journal_schema_version);
  append_u32(builder, manifest.seed_journal_schema_version);
  append_size(
      builder,
      manifest.trusted_seed_budget.maximum_source_journal_replay_entry_count);
  append_size(builder, manifest.trusted_seed_budget.maximum_role_scan_count);
  append_size(
      builder, manifest.trusted_seed_budget.maximum_saddle_family_count);
  append_size(builder, manifest.trusted_seed_budget.maximum_arm_seed_count);
  for (const std::size_t value : {
           manifest.point_count,
           manifest.requested_maximum_order,
           manifest.effective_maximum_order,
           manifest.direct_event_count,
           manifest.logical_event_projection_count,
           manifest.logical_role_record_count,
           manifest.batch_count,
           manifest.direct_birth_count,
           manifest.family_count,
           manifest.arm_seed_count,
           manifest.logical_event_journal_storage_entry_count,
           manifest.logical_seed_journal_storage_entry_count}) {
    append_size(builder, value);
  }
  append_id(builder, manifest.source_pair_canonical_cloud_digest);
  append_id(builder, manifest.source_higher_canonical_cloud_digest);
  append_id(builder, manifest.source_pair_semantic_digest);
  append_id(builder, manifest.source_higher_semantic_digest);
  append_bool(builder, manifest.singleton_prefix_implicit);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_initial_chain_digest(
    const contract::CanonicalId& source_identity) {
  contract::CanonicalSha256Builder builder;
  builder.update(initial_chain_domain);
  append_id(builder, source_identity);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_successor_chain_digest(
    const contract::CanonicalId& source_chain,
    const contract::CanonicalId& batch_identity) {
  contract::CanonicalSha256Builder builder;
  builder.update(chain_step_domain);
  append_id(builder, source_chain);
  append_id(builder, batch_identity);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_manifest_digest(
    const ExactDirectMorseForestSourceManifest& manifest) {
  contract::CanonicalSha256Builder builder;
  builder.update(manifest_domain);
  append_id(builder, manifest.source_identity_digest);
  append_id(builder, manifest.initial_batch_chain_digest);
  append_id(builder, manifest.final_batch_chain_digest);
  append_size(builder, manifest.batch_count);
  return builder.finalize();
}

[[nodiscard]] const ExactDirectMorseEventProjection* resident_projection(
    const void* state, std::size_t logical_index) noexcept {
  const auto& journal =
      *static_cast<const ExactDirectMorseEventJournalResult*>(state);
  if (logical_index < journal.point_count) {
    return nullptr;
  }
  const std::size_t physical_index = logical_index - journal.point_count;
  if (physical_index >=
      journal.materialized_direct_event_projections.size()) {
    return nullptr;
  }
  const auto& projection =
      journal.materialized_direct_event_projections[physical_index];
  return projection.event_projection_index == logical_index
             ? &projection
             : nullptr;
}

[[nodiscard]] const ExactDirectSupportEvent* resident_event(
    const void* state, std::size_t source_index) noexcept {
  const auto& facade =
      *static_cast<const ExactDirectSupportTerminalFacade*>(state);
  if (source_index >= facade.events.size()) {
    return nullptr;
  }
  const auto& event = facade.events[source_index];
  return event.event_index == source_index ? &event : nullptr;
}

struct ResidentSlices {
  std::size_t logical_role_begin{};
  std::size_t implicit_singleton_count{};
  std::size_t family_begin{};
  std::size_t arm_begin{};
  std::span<const ExactDirectMorseH0RoleRecord> roles;
  std::span<const ExactDirectSaddleArmFamilyRecord> families;
  std::span<const ExactDirectSaddleArmSeedRecord> arm_seeds;
};

[[nodiscard]] ResidentSlices resident_slices(
    const ExactDirectMorseForestSourceManifest& manifest,
    const ExactDirectMorseEventJournalResult& event_journal,
    const ExactDirectSaddleArmSeedJournalResult& seed_journal,
    std::size_t batch_index) {
  if (batch_index >= event_journal.batches.size()) {
    throw std::out_of_range("a source batch index is out of range");
  }
  const auto& batch = event_journal.batches[batch_index];
  ResidentSlices slices;
  slices.logical_role_begin = batch.role_record_offset;
  if (batch.role_record_offset < manifest.point_count) {
    if (batch_index != 0U || batch.role_record_offset != 0U ||
        batch.role_record_count != manifest.point_count) {
      throw std::logic_error("the implicit singleton batch is malformed");
    }
    slices.implicit_singleton_count = manifest.point_count;
  } else {
    const std::size_t physical_offset =
        batch.role_record_offset - manifest.point_count;
    const auto direct_roles = std::span<const ExactDirectMorseH0RoleRecord>{
        event_journal.materialized_direct_role_records};
    if (physical_offset > direct_roles.size() ||
        batch.role_record_count > direct_roles.size() - physical_offset) {
      throw std::logic_error("a source batch role slice is malformed");
    }
    slices.roles = direct_roles.subspan(
        physical_offset, batch.role_record_count);
  }

  const auto all_families =
      std::span<const ExactDirectSaddleArmFamilyRecord>{seed_journal.families};
  const auto first = std::lower_bound(
      all_families.begin(),
      all_families.end(),
      batch_index,
      [](const ExactDirectSaddleArmFamilyRecord& family, std::size_t index) {
        return family.journal_batch_index < index;
      });
  const auto last = std::upper_bound(
      first,
      all_families.end(),
      batch_index,
      [](std::size_t index, const ExactDirectSaddleArmFamilyRecord& family) {
        return index < family.journal_batch_index;
      });
  slices.family_begin = static_cast<std::size_t>(
      std::distance(all_families.begin(), first));
  const std::size_t family_count =
      static_cast<std::size_t>(std::distance(first, last));
  slices.families = all_families.subspan(slices.family_begin, family_count);
  if (slices.families.size() != batch.saddle_role_count) {
    throw std::logic_error("a source batch family slice is malformed");
  }

  if (!slices.families.empty()) {
    slices.arm_begin = slices.families.front().arm_seed_offset;
    const auto& final_family = slices.families.back();
    const std::size_t arm_end =
        final_family.arm_seed_offset + final_family.arm_seed_count;
    if (slices.arm_begin > arm_end || arm_end > seed_journal.arm_seeds.size()) {
      throw std::logic_error("a source batch arm slice is malformed");
    }
    slices.arm_seeds =
        std::span<const ExactDirectSaddleArmSeedRecord>{seed_journal.arm_seeds}
            .subspan(slices.arm_begin, arm_end - slices.arm_begin);
  } else if (first == all_families.begin()) {
    slices.arm_begin = 0U;
  } else {
    const auto& previous = *(first - 1);
    slices.arm_begin = previous.arm_seed_offset + previous.arm_seed_count;
  }
  return slices;
}

[[nodiscard]] contract::CanonicalId compute_batch_identity_digest(
    const ExactDirectMorseForestSourceBatchWindow& window) {
  if (window.batch == nullptr || !window.projections || !window.events) {
    throw std::logic_error("a source batch window has no lookup authority");
  }
  const auto& batch = *window.batch;
  contract::CanonicalSha256Builder builder;
  builder.update(batch_identity_domain);
  append_id(builder, window.source_identity_digest);
  append_size(builder, window.source_batch_index);
  append_size(builder, window.logical_role_begin_index);
  append_size(builder, window.implicit_singleton_role_count);
  append_size(builder, window.family_begin_index);
  append_size(builder, window.arm_seed_begin_index);
  append_bool(builder, window.scientific_source_freshly_recertified);
  append_size(builder, batch.batch_index);
  append_size(builder, batch.order);
  append_level(builder, batch.squared_level);
  append_size(builder, batch.role_record_offset);
  append_size(builder, batch.role_record_count);
  append_size(builder, batch.birth_role_count);
  append_size(builder, batch.saddle_role_count);

  append_size(builder, window.roles.size());
  for (std::size_t local = 0U; local < window.roles.size(); ++local) {
    const auto& role = window.roles[local];
    if (role.role_record_index != window.logical_role_begin_index + local ||
        role.batch_index != window.source_batch_index) {
      throw std::logic_error("a source window role index is inconsistent");
    }
    const auto* projection =
        window.projections.find(role.event_projection_index);
    if (projection == nullptr) {
      throw std::logic_error("a source window projection is missing");
    }
    const auto* event = window.events.find(projection->source_index);
    if (event == nullptr) {
      throw std::logic_error("a source window event is missing");
    }
    append_size(builder, role.role_record_index);
    append_size(builder, role.batch_index);
    append_size(builder, role.event_projection_index);
    append_u8(builder, static_cast<std::uint8_t>(role.role));
    if (projection->support_size == 0U ||
        static_cast<std::size_t>(projection->support_size) >
            projection->support_ids.size()) {
      throw std::logic_error(
          "a source window projection has an invalid support size");
    }
    append_size(builder, projection->event_projection_index);
    append_u8(builder, static_cast<std::uint8_t>(projection->source));
    append_size(builder, projection->source_index);
    append_u8(builder, projection->support_size);
    for (std::size_t index = 0U;
         index < static_cast<std::size_t>(projection->support_size);
         ++index) {
      append_u64(
          builder, static_cast<std::uint64_t>(projection->support_ids[index]));
    }
    append_level(builder, projection->squared_level);
    append_size(builder, projection->closed_rank);
    append_bool(builder, projection->birth_order.has_value());
    if (projection->birth_order.has_value()) {
      append_size(builder, *projection->birth_order);
    }
    append_bool(builder, projection->saddle_order.has_value());
    if (projection->saddle_order.has_value()) {
      append_size(builder, *projection->saddle_order);
    }
    append_event(builder, *event);
  }

  append_size(builder, window.families.size());
  for (std::size_t local = 0U; local < window.families.size(); ++local) {
    const auto& family = window.families[local];
    if (family.family_index != window.family_begin_index + local ||
        family.journal_batch_index != window.source_batch_index) {
      throw std::logic_error("a source window family index is inconsistent");
    }
    const auto* event = window.events.find(family.source_event_index);
    if (event == nullptr) {
      throw std::logic_error("a source window family event is missing");
    }
    for (const std::size_t value : {
             family.family_index,
             family.source_event_index,
             family.journal_event_projection_index,
             family.journal_role_record_index,
             family.journal_batch_index,
             family.order,
             family.arm_seed_offset,
             family.arm_seed_count}) {
      append_size(builder, value);
    }
    append_level(builder, family.critical_squared_level);
    append_id(builder, family.source_event_arm_identity_digest);
    append_event(builder, *event);
  }

  append_size(builder, window.arm_seeds.size());
  for (std::size_t local = 0U; local < window.arm_seeds.size(); ++local) {
    const auto& seed = window.arm_seeds[local];
    if (seed.arm_seed_index != window.arm_seed_begin_index + local) {
      throw std::logic_error("a source window arm index is inconsistent");
    }
    append_size(builder, seed.arm_seed_index);
    append_size(builder, seed.family_index);
    append_u64(
        builder, static_cast<std::uint64_t>(seed.removed_support_point_id));
  }
  return builder.finalize();
}

[[nodiscard]] ExactDirectMorseForestSourceBatchWindow make_resident_window(
    const ExactDirectMorseForestSourceManifest& manifest,
    const ExactDirectSupportTerminalFacade& facade,
    const ExactDirectMorseEventJournalResult& event_journal,
    const ExactDirectSaddleArmSeedJournalResult& seed_journal,
    std::size_t batch_index,
    const contract::CanonicalId& source_chain) {
  const ResidentSlices slices =
      resident_slices(manifest, event_journal, seed_journal, batch_index);
  ExactDirectMorseForestSourceBatchWindow window;
  window.manifest_digest = manifest.manifest_digest;
  window.source_identity_digest = manifest.source_identity_digest;
  window.source_chain_digest = source_chain;
  window.source_batch_index = batch_index;
  window.logical_role_begin_index = slices.logical_role_begin;
  window.implicit_singleton_role_count = slices.implicit_singleton_count;
  window.family_begin_index = slices.family_begin;
  window.arm_seed_begin_index = slices.arm_begin;
  window.batch = &event_journal.batches[batch_index];
  window.roles = slices.roles;
  window.families = slices.families;
  window.arm_seeds = slices.arm_seeds;
  window.projections = ExactDirectMorseForestProjectionLookupView{
      &event_journal, resident_projection};
  window.events =
      ExactDirectMorseForestEventLookupView{&facade, resident_event};
  window.scientific_source_freshly_recertified = true;
  window.resident_records_borrowed_without_copy = true;
  window.batch_identity_digest = compute_batch_identity_digest(window);
  window.successor_chain_digest = compute_successor_chain_digest(
      window.source_chain_digest, window.batch_identity_digest);
  return window;
}

}  // namespace

bool ExactDirectMorseForestSourceManifest::certified() const noexcept {
  try {
    return schema_version ==
               direct_morse_forest_source_authority_schema_version &&
           event_journal_schema_version ==
               direct_morse_event_journal_schema_version &&
           seed_journal_schema_version ==
               direct_saddle_arm_seed_journal_schema_version &&
           point_count != 0U && effective_maximum_order != 0U &&
           effective_maximum_order <= requested_maximum_order &&
           effective_maximum_order <= point_count && batch_count != 0U &&
           logical_event_projection_count >= point_count &&
           logical_role_record_count >= point_count &&
           direct_birth_count <= logical_role_record_count - point_count &&
           source_identity_digest == compute_source_identity_digest(*this) &&
           initial_batch_chain_digest ==
               compute_initial_chain_digest(source_identity_digest) &&
           manifest_digest == compute_manifest_digest(*this) &&
           source_event_journal_freshly_verified &&
           source_seed_journal_freshly_verified &&
           singleton_prefix_implicit && batch_partition_dense_and_complete &&
           !forbidden_global_structure_materialized &&
           !public_status_claimed;
  } catch (...) {
    return false;
  }
}

bool ExactDirectMorseForestSourceBatchWindow::certified_relative_to(
    const ExactDirectMorseForestSourceManifest& manifest) const {
  if (!manifest.certified() || schema_version != manifest.schema_version ||
      manifest_digest != manifest.manifest_digest ||
      source_identity_digest != manifest.source_identity_digest ||
      source_batch_index >= manifest.batch_count || batch == nullptr ||
      batch->batch_index != source_batch_index ||
      batch->role_record_offset != logical_role_begin_index ||
      !scientific_source_freshly_recertified ||
      !synchronous_lifetime_only || !projections || !events ||
      (source_batch_index == 0U &&
       source_chain_digest != manifest.initial_batch_chain_digest) ||
      (source_batch_index + 1U == manifest.batch_count &&
       successor_chain_digest != manifest.final_batch_chain_digest)) {
    return false;
  }
  const bool singleton = implicit_singleton_role_count != 0U;
  if (singleton != (source_batch_index == 0U) ||
      (singleton &&
       (implicit_singleton_role_count != manifest.point_count ||
        !roles.empty())) ||
      (!singleton && roles.size() != batch->role_record_count) ||
      families.size() != batch->saddle_role_count) {
    return false;
  }
  const contract::CanonicalId expected_batch =
      compute_batch_identity_digest(*this);
  return batch_identity_digest == expected_batch &&
         successor_chain_digest == compute_successor_chain_digest(
                                       source_chain_digest, expected_batch);
}

ExactDirectMorseForestSourceManifest
build_exact_direct_morse_forest_source_manifest(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal) {
  const auto verification =
      verify_exact_direct_saddle_arm_seed_journal_streaming(
          cloud,
          source_facade,
          source_event_journal,
          trusted_seed_budget,
          source_seed_journal);
  if (!verification.result_certified ||
      !source_event_journal.certified_partial_refinement() ||
      !source_seed_journal.certified_partial_refinement() ||
      source_event_journal.point_count != cloud.size() ||
      source_seed_journal.point_count != cloud.size() ||
      source_event_journal.batches.empty()) {
    throw std::invalid_argument(
        "a forest source manifest requires freshly verified authorities");
  }

  ExactDirectMorseForestSourceManifest manifest;
  manifest.event_journal_schema_version = source_event_journal.schema_version;
  manifest.seed_journal_schema_version = source_seed_journal.schema_version;
  manifest.trusted_seed_budget = trusted_seed_budget;
  manifest.point_count = cloud.size();
  manifest.requested_maximum_order =
      source_event_journal.requested_maximum_order;
  manifest.effective_maximum_order =
      source_event_journal.effective_maximum_order;
  manifest.direct_event_count = source_facade.events.size();
  manifest.logical_event_projection_count =
      source_event_journal.event_projection_count;
  manifest.logical_role_record_count = source_event_journal.role_record_count;
  manifest.batch_count = source_event_journal.batches.size();
  manifest.direct_birth_count = static_cast<std::size_t>(std::count_if(
      source_event_journal.materialized_direct_role_records.begin(),
      source_event_journal.materialized_direct_role_records.end(),
      [](const ExactDirectMorseH0RoleRecord& role) {
        return role.role == ExactDirectMorseH0Role::birth;
      }));
  manifest.family_count = source_seed_journal.families.size();
  manifest.arm_seed_count = source_seed_journal.arm_seeds.size();
  manifest.logical_event_journal_storage_entry_count =
      source_event_journal.logical_linear_storage_entry_count;
  manifest.logical_seed_journal_storage_entry_count =
      source_seed_journal.combined_logical_storage_entry_count;
  manifest.source_pair_canonical_cloud_digest =
      source_seed_journal.source_pair_canonical_cloud_digest;
  manifest.source_higher_canonical_cloud_digest =
      source_seed_journal.source_higher_canonical_cloud_digest;
  manifest.source_pair_semantic_digest =
      source_seed_journal.source_pair_semantic_digest;
  manifest.source_higher_semantic_digest =
      source_seed_journal.source_higher_semantic_digest;
  manifest.source_event_journal_freshly_verified = true;
  manifest.source_seed_journal_freshly_verified = true;
  manifest.singleton_prefix_implicit =
      source_event_journal.canonical_singletons_implicit_and_unmaterialized;
  manifest.batch_partition_dense_and_complete = true;
  manifest.source_identity_digest = compute_source_identity_digest(manifest);
  manifest.initial_batch_chain_digest =
      compute_initial_chain_digest(manifest.source_identity_digest);

  contract::CanonicalId chain = manifest.initial_batch_chain_digest;
  for (std::size_t batch_index = 0U;
       batch_index < manifest.batch_count;
       ++batch_index) {
    auto window = make_resident_window(
        manifest,
        source_facade,
        source_event_journal,
        source_seed_journal,
        batch_index,
        chain);
    chain = window.successor_chain_digest;
  }
  manifest.final_batch_chain_digest = chain;
  manifest.manifest_digest = compute_manifest_digest(manifest);
  if (!manifest.certified()) {
    throw std::logic_error("a freshly built forest source manifest is invalid");
  }
  return manifest;
}

ExactDirectMorseForestResidentSourceAdapter::
    ExactDirectMorseForestResidentSourceAdapter(
        const spatial::CanonicalPointCloud& cloud,
        const ExactDirectSupportTerminalFacade& source_facade,
        const ExactDirectMorseEventJournalResult& source_event_journal,
        const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
        const ExactDirectSaddleArmSeedJournalResult& source_seed_journal)
    : source_facade_(&source_facade),
      source_event_journal_(&source_event_journal),
      source_seed_journal_(&source_seed_journal),
      manifest_(build_exact_direct_morse_forest_source_manifest(
          cloud,
          source_facade,
          source_event_journal,
          trusted_seed_budget,
          source_seed_journal)) {
  chain_prefixes_.reserve(manifest_.batch_count + 1U);
  chain_prefixes_.push_back(manifest_.initial_batch_chain_digest);
  for (std::size_t batch_index = 0U;
       batch_index < manifest_.batch_count;
       ++batch_index) {
    const auto window = make_resident_window(
        manifest_,
        *source_facade_,
        *source_event_journal_,
        *source_seed_journal_,
        batch_index,
        chain_prefixes_.back());
    chain_prefixes_.push_back(window.successor_chain_digest);
  }
  if (chain_prefixes_.back() != manifest_.final_batch_chain_digest) {
    throw std::logic_error("a resident source chain is inconsistent");
  }
}

ExactDirectMorseForestSourceBatchVisitDecision
ExactDirectMorseForestResidentSourceAdapter::visit_batch(
    std::size_t batch_index,
    ExactDirectMorseForestSourceBatchConsumerView consumer) const {
  if (!consumer) {
    return ExactDirectMorseForestSourceBatchVisitDecision::no_consumer_rejected;
  }
  if (batch_index >= manifest_.batch_count) {
    return ExactDirectMorseForestSourceBatchVisitDecision::
        no_batch_out_of_range;
  }
  try {
    const auto window = make_resident_window(
        manifest_,
        *source_facade_,
        *source_event_journal_,
        *source_seed_journal_,
        batch_index,
        chain_prefixes_[batch_index]);
    if (!window.certified_relative_to(manifest_)) {
      return ExactDirectMorseForestSourceBatchVisitDecision::
          no_window_inconsistent;
    }
    if (!consumer(window)) {
      return ExactDirectMorseForestSourceBatchVisitDecision::
          no_consumer_rejected;
    }
    return ExactDirectMorseForestSourceBatchVisitDecision::
        complete_synchronous_visit;
  } catch (const std::logic_error&) {
    return ExactDirectMorseForestSourceBatchVisitDecision::
        no_window_inconsistent;
  }
}

}  // namespace morsehgp3d::hierarchy
