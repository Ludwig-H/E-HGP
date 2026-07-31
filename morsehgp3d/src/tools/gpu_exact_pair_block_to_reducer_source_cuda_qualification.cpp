#include "morsehgp3d/gpu/exact_pair_block_to_direct_pair_terminal.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/hierarchy/direct_morse_terminal_reducer_source_bridge.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ExactPairBlockToDirectPairTerminalBudget;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaConfig;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaContext;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::hierarchy::ExactDirectSaddleArmSeedBudget;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalRunStatus;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalSession;
using morsehgp3d::spatial::CanonicalPointCloud;

#if defined(MORSEHGP3D_GIT_SHA)
inline constexpr std::string_view kGitSha = MORSEHGP3D_GIT_SHA;
#else
inline constexpr std::string_view kGitSha = "unbound";
#endif

struct Options {
  std::size_t maximum_order{};
  bool require_complete{false};
};

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    const char* role) {
  std::size_t value = 0U;
  const auto parsed = std::from_chars(
      text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc{} ||
      parsed.ptr != text.data() + text.size()) {
    throw std::invalid_argument(role);
  }
  return value;
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  bool order_seen = false;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--require-complete") {
      options.require_complete = true;
      continue;
    }
    if ((argument != "--K" && argument != "--maximum-order") ||
        order_seen || index + 1 >= argc) {
      throw std::invalid_argument(
          "usage: --K 5|10 [--require-complete]");
    }
    options.maximum_order = parse_size(argv[++index], "invalid --K");
    order_seen = true;
  }
  if (!order_seen ||
      (options.maximum_order != 5U && options.maximum_order != 10U) ||
      !options.require_complete) {
    throw std::invalid_argument(
        "full-chain qualification requires K equal to 5 or 10 and "
        "--require-complete");
  }
  return options;
}

[[nodiscard]] std::vector<CertifiedPoint3> moment_curve_points() {
  std::vector<CertifiedPoint3> points;
  points.reserve(12U);
  for (std::size_t index = 0U; index < 12U; ++index) {
    const double value = static_cast<double>(index);
    points.push_back(CertifiedPoint3::from_binary64(
        value, value * value, value * value * value));
  }
  return points;
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget unlimited_seed_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] std::uint64_t elapsed_nanoseconds(
    Clock::time_point begin,
    Clock::time_point end) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin)
          .count());
}

int run(const Options& options) {
  const Clock::time_point total_begin = Clock::now();
  const auto points = moment_curve_points();
  const Clock::time_point generation_end = Clock::now();
  const CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(
          std::span<const CertifiedPoint3>{points});
  const Clock::time_point canonical_end = Clock::now();

  MortonLbvhBuildContext builder{cloud.size()};
  auto build = builder.build(cloud);
  const Clock::time_point lbvh_end = Clock::now();
  if (!build.cuda_qualified_build()) {
    throw std::runtime_error(
        "the full-chain qualification requires a CUDA-certified LBVH");
  }
  const Clock::time_point scheduler_setup_begin = lbvh_end;
  const auto& index = build.certified_index();
  auto traversal = builder.release_device_traversal_lease(build);
  auto scheduler = ExactPairBlockTransactionalFrontierResidentCudaContext::
      start(
          std::move(traversal),
          index,
          cloud,
          ExactPairBlockTransactionalFrontierResidentCudaConfig{
              options.maximum_order + 1U, 16U, 8U, 16U, 4U});
  const Clock::time_point scheduler_begin = Clock::now();
  auto cut = scheduler.run({});
  const Clock::time_point scheduler_end = Clock::now();
  const Clock::time_point cut_validation_begin = Clock::now();
  const bool cut_certified =
      cut.qualified_cuda_terminal_authority() && cut.complete() &&
      cut.validated_for(index, cloud);
  const Clock::time_point cut_validation_end = Clock::now();
  const auto cut_audit = cut.audit();

  const Clock::time_point pair_adapter_begin = Clock::now();
  auto pair_attempt =
      morsehgp3d::gpu::build_exact_pair_block_to_direct_pair_terminal(
          index,
          cloud,
          options.maximum_order,
          ExactPairBlockToDirectPairTerminalBudget::unlimited(),
          std::move(cut));
  const Clock::time_point pair_adapter_end = Clock::now();
  const bool pair_authority_certified =
      pair_attempt.complete() &&
      pair_attempt.audit.qualified_cuda_execution &&
      !pair_attempt.audit.host_fake_execution;
  if (!pair_attempt.complete()) {
    throw std::runtime_error(
        "the CUDA pair cut did not produce a neutral terminal authority");
  }

  const ExactHigherSupportStreamBudget higher_budget =
      unlimited_higher_budget();
  const Clock::time_point higher_begin = Clock::now();
  ExactHigherSupportTerminalSession higher_session{
      index, cloud, options.maximum_order, higher_budget, 256U};
  const bool higher_terminal =
      higher_session.run_to_terminal() ==
      ExactHigherSupportTerminalRunStatus::terminal;
  auto higher_authority = std::move(higher_session).seal();
  const Clock::time_point higher_end = Clock::now();

  const Clock::time_point bridge_begin = Clock::now();
  auto bridge_result = morsehgp3d::hierarchy::
      build_exact_direct_morse_terminal_reducer_source_bridge(
          index,
          cloud,
          options.maximum_order,
          higher_budget,
          unlimited_seed_budget(),
          2U,
          std::move(*pair_attempt.authority),
          std::move(higher_authority));
  const Clock::time_point bridge_end = Clock::now();
  bool provider_replay_certified = bridge_result.certified();
  std::size_t visited_batch_count = 0U;
  std::size_t source_batch_count = 0U;
  std::size_t normalized_event_count = 0U;
  std::size_t source_seed_count = 0U;
  std::string pair_cloud_digest(64U, '0');
  std::string pair_lbvh_digest(64U, '0');
  std::string pair_output_digest(64U, '0');
  std::string pair_semantic_digest(64U, '0');
  std::string higher_semantic_digest(64U, '0');
  std::string higher_output_chain_digest(64U, '0');
  std::string higher_checkpoint_digest(64U, '0');
  std::string normalized_terminal_output_digest(64U, '0');
  std::string reducer_source_manifest_digest(64U, '0');
  Clock::time_point provider_replay_begin = bridge_end;
  if (bridge_result.bridge != nullptr) {
    source_batch_count = bridge_result.bridge->source_manifest().batch_count;
    normalized_event_count =
        bridge_result.bridge->direct_support_facade().events.size();
    source_seed_count =
        bridge_result.bridge->saddle_arm_seed_journal().arm_seeds.size();
    const auto& certificate =
        bridge_result.bridge->direct_support_facade().certificate;
    pair_cloud_digest = certificate.pair_canonical_cloud_digest.to_lower_hex();
    pair_lbvh_digest = certificate.pair_lbvh_digest.to_lower_hex();
    pair_output_digest =
        certificate.pair_terminal_output_digest.to_lower_hex();
    pair_semantic_digest = certificate.pair_semantic_digest.to_lower_hex();
    higher_semantic_digest =
        certificate.higher_semantic_digest.to_lower_hex();
    higher_output_chain_digest =
        certificate.higher_output_chain_digest.to_lower_hex();
    higher_checkpoint_digest =
        certificate.higher_terminal_checkpoint_digest.to_lower_hex();
    normalized_terminal_output_digest =
        certificate.normalized_terminal_output_digest.to_lower_hex();
    reducer_source_manifest_digest =
        bridge_result.bridge->source_manifest().manifest_digest.to_lower_hex();
    provider_replay_begin = Clock::now();
    auto provider = bridge_result.bridge->source_provider();
    for (std::size_t batch = 0U; batch < source_batch_count; ++batch) {
      auto verify_window = [&](const auto& window) {
        ++visited_batch_count;
        return window.certified_relative_to(
            bridge_result.bridge->source_manifest());
      };
      provider_replay_certified = provider_replay_certified &&
          provider(batch, verify_window) ==
              morsehgp3d::hierarchy::
                  ExactDirectMorseForestSourceBatchVisitDecision::
                      complete_synchronous_visit;
    }
  }
  provider_replay_certified = provider_replay_certified &&
      visited_batch_count == source_batch_count;
  const Clock::time_point provider_replay_end = Clock::now();
  const bool qualified = cut_certified && pair_authority_certified &&
      higher_terminal && bridge_result.certified() &&
      provider_replay_certified;

  std::cout
      << "{\"schema\":\"morsehgp3d.phase15.transactional_pair_to_"
         "reducer_source_qualification.v1\","
      << "\"backend\":\"cuda_g4_plus_reference_cpu\","
      << "\"git_sha\":\"" << kGitSha << "\","
      << "\"profile\":\"hgp_reduced\","
      << "\"mode\":\"complete_direct_terminal_source_chain\","
      << "\"public_status\":\"not_claimed\","
      << "\"fixture\":\"moment_curve_12\","
      << "\"point_count\":" << cloud.size() << ','
      << "\"maximum_order\":" << options.maximum_order << ','
      << "\"maximum_closed_rank\":"
      << pair_attempt.audit.maximum_closed_rank << ','
      << "\"require_complete\":"
      << (options.require_complete ? "true" : "false") << ','
      << "\"qualified\":" << (qualified ? "true" : "false") << ','
      << "\"cut_certified\":"
      << (cut_certified ? "true" : "false") << ','
      << "\"pair_authority_certified\":"
      << (pair_authority_certified ? "true" : "false") << ','
      << "\"higher_terminal\":"
      << (higher_terminal ? "true" : "false") << ','
      << "\"bridge_certified\":"
      << (bridge_result.certified() ? "true" : "false") << ','
      << "\"provider_replay_certified\":"
      << (provider_replay_certified ? "true" : "false") << ','
      << "\"pair_cut\":{\"universe\":"
      << cut_audit.unordered_pair_universe_mass
      << ",\"pruned\":" << cut_audit.pruned_unordered_pair_mass
      << ",\"terminal\":" << cut_audit.terminal_unordered_pair_mass
      << ",\"kernel_launches\":" << cut_audit.kernel_launch_count
      << ",\"synchronizations\":" << cut_audit.synchronization_count
      << ",\"kernel_elapsed_nanoseconds\":"
      << cut_audit.kernel_elapsed_nanoseconds
      << ",\"cuda_device\":" << cut_audit.cuda_device
      << ",\"serial_device_reference\":"
      << (cut_audit.serial_device_reference ? "true" : "false")
      << ",\"scale_eligible\":"
      << (cut_audit.scale_eligible ? "true" : "false") << "},"
      << "\"pair_classification\":{\"terminal_pairs\":"
      << pair_attempt.audit.classification_terminal_count
      << ",\"above_rank\":" << pair_attempt.audit.above_rank_count
      << ",\"records\":" << pair_attempt.audit.emitted_record_count
      << ",\"node_visits\":"
      << pair_attempt.audit.classification_node_visit_count << "},"
      << "\"reducer_source\":{\"events\":"
      << normalized_event_count
      << ",\"seeds\":" << source_seed_count
      << ",\"batches\":" << source_batch_count
      << ",\"visited_batches\":" << visited_batch_count << "},"
      << "\"digests\":{\"submitted_recipe_fnv1a\":"
      << cut_audit.submitted_recipe_digest
      << ",\"final_cut_fnv1a\":" << cut_audit.final_cut_digest
      << ",\"pair_cloud_sha256\":\"" << pair_cloud_digest
      << "\",\"pair_lbvh_sha256\":\"" << pair_lbvh_digest
      << "\",\"pair_output_sha256\":\"" << pair_output_digest
      << "\",\"pair_semantic_sha256\":\"" << pair_semantic_digest
      << "\",\"higher_semantic_sha256\":\"" << higher_semantic_digest
      << "\",\"higher_output_chain_sha256\":\""
      << higher_output_chain_digest
      << "\",\"higher_checkpoint_sha256\":\""
      << higher_checkpoint_digest
      << "\",\"normalized_terminal_output_sha256\":\""
      << normalized_terminal_output_digest
      << "\",\"reducer_source_manifest_sha256\":\""
      << reducer_source_manifest_digest << "\"},"
      << "\"timings_nanoseconds\":{\"generation\":"
      << elapsed_nanoseconds(total_begin, generation_end)
      << ",\"canonicalization\":"
      << elapsed_nanoseconds(generation_end, canonical_end)
      << ",\"lbvh_build\":"
      << elapsed_nanoseconds(canonical_end, lbvh_end)
      << ",\"scheduler_setup_wall\":"
      << elapsed_nanoseconds(scheduler_setup_begin, scheduler_begin)
      << ",\"scheduler_wall\":"
      << elapsed_nanoseconds(scheduler_begin, scheduler_end)
      << ",\"cut_validation_wall\":"
      << elapsed_nanoseconds(cut_validation_begin, cut_validation_end)
      << ",\"pair_adapter_wall\":"
      << elapsed_nanoseconds(pair_adapter_begin, pair_adapter_end)
      << ",\"higher_support_wall\":"
      << elapsed_nanoseconds(higher_begin, higher_end)
      << ",\"bridge_wall\":"
      << elapsed_nanoseconds(bridge_begin, bridge_end)
      << ",\"bridge_output_inspection_wall\":"
      << elapsed_nanoseconds(bridge_end, provider_replay_begin)
      << ",\"provider_replay_wall\":"
      << elapsed_nanoseconds(provider_replay_begin, provider_replay_end)
      << ",\"total\":"
      << elapsed_nanoseconds(total_begin, provider_replay_end) << "},"
      << "\"claims\":{\"ordinary_or_higher_order_delaunay\":false,"
         "\"global_pair_matrix\":false,\"hierarchy_reduction\":false,"
         "\"public_exact\":false},"
      << "\"qualified_scope\":\"terminal_direct_supports_to_bounded_"
         "reducer_source_only\"}"
      << '\n';
  return qualified ? 0 : 2;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(parse_options(argc, argv));
  } catch (const std::exception& error) {
    std::cerr << "full-chain qualification failed: " << error.what() << '\n';
    return 1;
  }
}
