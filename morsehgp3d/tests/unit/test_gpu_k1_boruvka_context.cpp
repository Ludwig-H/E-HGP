#include "fake_gpu_k1_boruvka_launchers.hpp"

#include "morsehgp3d/gpu/k1_boruvka.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::gpu::K1BoruvkaCandidate;
using morsehgp3d::gpu::K1BoruvkaCandidateContext;
using morsehgp3d::gpu::K1BoruvkaRoundProposal;
using morsehgp3d::gpu::test_support::FakeK1BoruvkaConfiguration;
using morsehgp3d::gpu::test_support::FakeK1BoruvkaCorruption;
using morsehgp3d::gpu::test_support::configure_fake_gpu_k1_boruvka;
using morsehgp3d::gpu::test_support::fake_gpu_k1_boruvka_last_node_count;
using morsehgp3d::gpu::test_support::fake_gpu_k1_boruvka_last_point_count;
using morsehgp3d::gpu::test_support::fake_gpu_k1_boruvka_launch_count;
using morsehgp3d::gpu::test_support::reset_fake_gpu_k1_boruvka;
using morsehgp3d::hierarchy::K1BoruvkaComponentMinimum;
using morsehgp3d::hierarchy::K1ExactBoruvkaResult;
using morsehgp3d::hierarchy::build_exact_lbvh_boruvka;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

static_assert(!std::is_copy_constructible_v<K1BoruvkaCandidateContext>);
static_assert(!std::is_copy_assignable_v<K1BoruvkaCandidateContext>);
static_assert(
    std::is_nothrow_move_constructible_v<K1BoruvkaCandidateContext>);
static_assert(std::is_nothrow_move_assignable_v<K1BoruvkaCandidateContext>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message
              << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

template <typename Function>
void check_throws_standard(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const std::exception&) {
    return;
  } catch (...) {
    ++failures;
    std::cerr << "FAIL: " << message << " (non-standard exception)\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CertifiedPoint3 point(
    double x, double y = 0.0, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] std::size_t distinct_component_count(
    std::span<const PointId> labels) {
  std::vector<PointId> distinct{labels.begin(), labels.end()};
  std::sort(distinct.begin(), distinct.end());
  distinct.erase(std::unique(distinct.begin(), distinct.end()), distinct.end());
  return distinct.size();
}

[[nodiscard]] std::span<const K1BoruvkaCandidate> candidate_segment(
    const K1BoruvkaRoundProposal& proposal, PointId source) {
  const std::size_t source_index = static_cast<std::size_t>(source);
  if (source_index + 1U >= proposal.candidate_offsets.size()) {
    return {};
  }
  const std::size_t begin = proposal.candidate_offsets[source_index];
  const std::size_t end = proposal.candidate_offsets[source_index + 1U];
  if (begin > end || end > proposal.candidates.size()) {
    return {};
  }
  return std::span<const K1BoruvkaCandidate>{proposal.candidates}.subspan(
      begin, end - begin);
}

[[nodiscard]] bool has_candidate(
    const K1BoruvkaRoundProposal& proposal,
    PointId source,
    PointId target) {
  const std::span<const K1BoruvkaCandidate> segment =
      candidate_segment(proposal, source);
  return std::any_of(
      segment.begin(), segment.end(),
      [source, target](const K1BoruvkaCandidate& candidate) {
        return candidate.source_point_id == source &&
               candidate.target_point_id == target;
      });
}

[[nodiscard]] PointId other_endpoint(
    const K1BoruvkaComponentMinimum& minimum) {
  return minimum.outgoing_edge.u == minimum.source_point_id
             ? minimum.outgoing_edge.v
             : minimum.outgoing_edge.u;
}

void check_normal_proposal(
    const K1BoruvkaRoundProposal& proposal,
    const MortonLbvhIndex& index,
    std::span<const PointId> labels,
    const std::vector<K1BoruvkaComponentMinimum>& expected_minima,
    std::uint64_t expected_epoch,
    const std::string& fixture) {
  const std::size_t point_count = labels.size();
  const std::size_t component_count = distinct_component_count(labels);
  check(
      proposal.frozen_component_labels ==
          std::vector<PointId>{labels.begin(), labels.end()},
      fixture + " preserves the certified frozen labels");
  check(
      proposal.candidate_offsets.size() == point_count + 1U &&
          proposal.candidate_offsets.front() == 0U &&
          proposal.candidate_offsets.back() == proposal.candidates.size() &&
          std::is_sorted(
              proposal.candidate_offsets.begin(),
              proposal.candidate_offsets.end()),
      fixture + " publishes one closed monotone CSR segment per source");

  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    for (const K1BoruvkaCandidate& candidate : candidate_segment(
             proposal, static_cast<PointId>(source_index))) {
      const std::size_t target_index =
          static_cast<std::size_t>(candidate.target_point_id);
      check(
          candidate.source_point_id == static_cast<PointId>(source_index) &&
              target_index < point_count && target_index != source_index &&
              labels[target_index] != labels[source_index],
          fixture + " emits only in-range outgoing candidates in its segment");
    }
  }

  check(
      proposal.cpu_exact_component_minima == expected_minima,
      fixture + " resolves the same exact kappa minima as the CPU anchor");
  for (const K1BoruvkaComponentMinimum& minimum : expected_minima) {
    check(
        has_candidate(
            proposal, minimum.source_point_id, other_endpoint(minimum)),
        fixture + " retains every CPU-required exact minimum in the GPU superset");
  }

  const auto& audit = proposal.audit;
  check(
      audit.resident_point_count == point_count &&
          audit.resident_node_count == index.build_counters().node_count &&
          audit.frozen_component_count == component_count &&
          audit.uniform_lbvh_node_count + audit.mixed_lbvh_node_count ==
              audit.resident_node_count,
      fixture + " closes resident, component and node-tag counts");
  check(
      audit.exact_seed_count == proposal.seeds.size() &&
          audit.gpu_candidate_count == proposal.candidates.size() &&
          audit.gpu_output_capacity == proposal.candidates.size() &&
          audit.cpu_exact_candidate_distance_evaluation_count ==
              proposal.candidates.size(),
      fixture + " closes seed, capacity and exact candidate counts");
  check(
      audit.gpu_kernel_launch_count == 2U &&
          audit.gpu_synchronization_count == 2U &&
          audit.gpu_count_pass_node_visit_count ==
              audit.gpu_emit_pass_node_visit_count &&
          audit.gpu_count_pass_node_visit_count > 0U &&
          audit.buffer_epoch == expected_epoch,
      fixture + " records deterministic count and emit passes in one epoch");
  check(
      audit.frozen_labels_certified && audit.rope_topology_certified &&
          audit.exact_capacity_certified && audit.no_truncation_certified &&
          audit.candidate_superset_certified &&
          audit.cpu_exact_resolution_complete,
      fixture + " certifies only the proposal superset and CPU resolution contract");
}

void test_terminal_singleton_without_gpu_launch() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 1> input{point(2.0, -1.0, 4.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  K1BoruvkaCandidateContext context{index, cloud};
  const std::array<PointId, 1> labels{PointId{0}};

  const K1BoruvkaRoundProposal proposal = context.propose_round(
      cloud, std::span<const PointId>{labels});
  check(
      context.point_count() == 1U && context.node_count() == 1U,
      "singleton context snapshots its terminal leaf");
  check(
      proposal.frozen_component_labels ==
              std::vector<PointId>{PointId{0}} &&
          proposal.seeds.empty() && proposal.candidates.empty() &&
          proposal.cpu_exact_component_minima.empty() &&
          proposal.candidate_offsets == std::vector<std::size_t>{0U, 0U},
      "terminal singleton returns one empty certified CSR without seeds");
  check(
      proposal.audit.frozen_component_count == 1U &&
          proposal.audit.gpu_kernel_launch_count == 0U &&
          proposal.audit.gpu_synchronization_count == 0U &&
          proposal.audit.buffer_epoch == 0U &&
          proposal.audit.candidate_superset_certified &&
          proposal.audit.cpu_exact_resolution_complete,
      "terminal singleton is resolved before any GPU work");
  check(
      fake_gpu_k1_boruvka_launch_count() == 0U,
      "terminal singleton never invokes the fake launcher");
}

void test_square_ties_candidate_superset_and_threshold_equality() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 4> input{
      point(-1.0, -1.0),
      point(-1.0, 1.0),
      point(1.0, -1.0),
      point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const K1ExactBoruvkaResult anchor =
      build_exact_lbvh_boruvka(index, cloud);
  const std::array<PointId, 4> labels{
      PointId{0}, PointId{1}, PointId{2}, PointId{3}};
  K1BoruvkaCandidateContext context{index, cloud};

  const K1BoruvkaRoundProposal proposal = context.propose_round(
      cloud, std::span<const PointId>{labels});
  check_normal_proposal(
      proposal,
      index,
      std::span<const PointId>{labels},
      anchor.rounds.front().component_minima,
      1U,
      "square singleton partition");
  check(
      proposal.seeds.size() == 4U &&
          proposal.seeds.front().source_point_id == PointId{0} &&
          proposal.seeds.front().exact_squared_cutoff ==
              ExactLevel{BigInt{4}},
      "square fixes one exact outgoing seed and cutoff per source");
  check(
      has_candidate(proposal, PointId{0}, PointId{1}) &&
          has_candidate(proposal, PointId{0}, PointId{2}),
      "strict pruning descends at equality and retains both square neighbors at the cutoff");
  check(
      fake_gpu_k1_boruvka_launch_count() == 1U &&
          fake_gpu_k1_boruvka_last_point_count() == 4U &&
          fake_gpu_k1_boruvka_last_node_count() == 7U,
      "square uses one resident seven-node fake launch");

  const K1BoruvkaRoundProposal replay = context.propose_round(
      cloud, std::span<const PointId>{labels});
  check(
      replay.seeds == proposal.seeds &&
          replay.candidate_offsets == proposal.candidate_offsets &&
          replay.candidates == proposal.candidates &&
          replay.cpu_exact_component_minima ==
              proposal.cpu_exact_component_minima &&
          replay.audit.proposal_digest_fnv1a ==
              proposal.audit.proposal_digest_fnv1a &&
          replay.audit.buffer_epoch == 2U,
      "square proposal is deterministic across resident buffer epochs");
}

void test_already_contracted_two_component_partition() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 4> input{
      point(0.0), point(1.0), point(11.0), point(13.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const K1ExactBoruvkaResult anchor =
      build_exact_lbvh_boruvka(index, cloud);
  const std::array<PointId, 4> labels{
      PointId{0}, PointId{0}, PointId{2}, PointId{2}};
  K1BoruvkaCandidateContext context{index, cloud};

  const K1BoruvkaRoundProposal proposal = context.propose_round(
      cloud, std::span<const PointId>{labels});
  check_normal_proposal(
      proposal,
      index,
      std::span<const PointId>{labels},
      anchor.rounds.at(1U).component_minima,
      1U,
      "two-component contracted chain");
  check(
      proposal.cpu_exact_component_minima.size() == 2U &&
          proposal.cpu_exact_component_minima[0].outgoing_edge ==
              proposal.cpu_exact_component_minima[1].outgoing_edge &&
          proposal.cpu_exact_component_minima[0].source_point_id ==
              PointId{1} &&
          proposal.cpu_exact_component_minima[1].source_point_id ==
              PointId{2},
      "contracted chain resolves the shared exact bridge from both components");
  for (std::size_t source = 0U; source < labels.size(); ++source) {
    check(
        !candidate_segment(proposal, static_cast<PointId>(source)).empty(),
        "every point keeps at least its fixed exact outgoing seed candidate");
  }
}

void test_invalid_namespaces_labels_and_moved_from_objects() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 4> input{
      point(0.0), point(1.0), point(11.0), point(13.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const CanonicalPointCloud other_identity = canonical_cloud(input);
  const std::array<PointId, 4> labels{
      PointId{0}, PointId{1}, PointId{2}, PointId{3}};

  check_throws<std::invalid_argument>(
      [&index, &other_identity]() {
        K1BoruvkaCandidateContext rejected{index, other_identity};
        static_cast<void>(rejected);
      },
      "context rejects an LBVH from another cloud identity");

  K1BoruvkaCandidateContext context{index, cloud};
  check_throws<std::invalid_argument>(
      [&context, &other_identity, &labels]() {
        static_cast<void>(context.propose_round(
            other_identity, std::span<const PointId>{labels}));
      },
      "context rejects a query cloud from another namespace");
  check_throws<std::invalid_argument>(
      [&context, &cloud, &labels]() {
        static_cast<void>(context.propose_round(
            cloud, std::span<const PointId>{labels.data(), 3U}));
      },
      "context rejects a frozen label vector with the wrong size");
  const std::array<PointId, 4> out_of_range{
      PointId{0}, PointId{1}, PointId{2}, PointId{4}};
  check_throws_standard(
      [&context, &cloud, &out_of_range]() {
        static_cast<void>(context.propose_round(
            cloud, std::span<const PointId>{out_of_range}));
      },
      "context rejects an out-of-range frozen component label");
  const std::array<PointId, 4> noncanonical{
      PointId{1}, PointId{1}, PointId{2}, PointId{3}};
  check_throws<std::invalid_argument>(
      [&context, &cloud, &noncanonical]() {
        static_cast<void>(context.propose_round(
            cloud, std::span<const PointId>{noncanonical}));
      },
      "context rejects a component label that is not its least PointId");
  check(
      fake_gpu_k1_boruvka_launch_count() == 0U,
      "namespace and label validation fail before the GPU section");
  static_cast<void>(context.propose_round(
      cloud, std::span<const PointId>{labels}));
  check(
      fake_gpu_k1_boruvka_launch_count() == 1U,
      "pre-GPU validation errors do not poison the resident context");

  K1BoruvkaCandidateContext moved_to = std::move(context);
  check(
      context.point_count() == 0U && context.node_count() == 0U &&
          moved_to.point_count() == input.size(),
      "moved-from context exposes empty extents and moved-to state survives");
  check_throws<std::logic_error>(
      [&context, &cloud, &labels]() {
        static_cast<void>(context.propose_round(
            cloud, std::span<const PointId>{labels}));
      },
      "moved-from context rejects new proposal work");

  CanonicalPointCloud index_cloud = canonical_cloud(input);
  MortonLbvhIndex moved_index_source = MortonLbvhIndex::build(index_cloud);
  const MortonLbvhIndex retained_index = std::move(moved_index_source);
  check(retained_index.ready(), "moved-to LBVH remains ready");
  check_throws<std::invalid_argument>(
      [&moved_index_source, &index_cloud]() {
        K1BoruvkaCandidateContext rejected{
            moved_index_source, index_cloud};
        static_cast<void>(rejected);
      },
      "context rejects a moved-from LBVH index");

  CanonicalPointCloud moved_cloud_source = canonical_cloud(input);
  const MortonLbvhIndex cloud_index =
      MortonLbvhIndex::build(moved_cloud_source);
  const CanonicalPointCloud retained_cloud = std::move(moved_cloud_source);
  check(retained_cloud.size() == input.size(), "moved-to cloud remains valid");
  check_throws<std::invalid_argument>(
      [&cloud_index, &moved_cloud_source]() {
        K1BoruvkaCandidateContext rejected{
            cloud_index, moved_cloud_source};
        static_cast<void>(rejected);
      },
      "context rejects a moved-from canonical cloud");
}

void test_corrupt_batches_and_gpu_failure_poison_context() {
  const std::array<CertifiedPoint3, 4> input{
      point(-1.0, -1.0),
      point(-1.0, 1.0),
      point(1.0, -1.0),
      point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 4> labels{
      PointId{0}, PointId{1}, PointId{2}, PointId{3}};
  const std::array<FakeK1BoruvkaCorruption, 4> corruptions{
      FakeK1BoruvkaCorruption::offset_count_mismatch,
      FakeK1BoruvkaCorruption::missing_outgoing_candidate,
      FakeK1BoruvkaCorruption::same_component_target,
      FakeK1BoruvkaCorruption::out_of_range_target};

  for (const FakeK1BoruvkaCorruption corruption : corruptions) {
    reset_fake_gpu_k1_boruvka();
    K1BoruvkaCandidateContext poisoned{index, cloud};
    configure_fake_gpu_k1_boruvka(FakeK1BoruvkaConfiguration{corruption});
    check_throws<std::runtime_error>(
        [&poisoned, &cloud, &labels]() {
          static_cast<void>(poisoned.propose_round(
              cloud, std::span<const PointId>{labels}));
        },
        "malformed post-GPU candidate batch fails closed");
    check(
        fake_gpu_k1_boruvka_launch_count() == 1U,
        "malformed batch is rejected only after one GPU transaction");

    configure_fake_gpu_k1_boruvka(FakeK1BoruvkaConfiguration{});
    check_throws<std::runtime_error>(
        [&poisoned, &cloud, &labels]() {
          static_cast<void>(poisoned.propose_round(
              cloud, std::span<const PointId>{labels}));
        },
        "post-GPU corruption poisons its resident context");
    check(
        fake_gpu_k1_boruvka_launch_count() == 1U,
        "poisoned context cannot relaunch after malformed output");

    K1BoruvkaCandidateContext fresh{index, cloud};
    const K1BoruvkaRoundProposal recovered = fresh.propose_round(
        cloud, std::span<const PointId>{labels});
    check(
        recovered.audit.candidate_superset_certified &&
            fake_gpu_k1_boruvka_launch_count() == 2U,
        "poisoning remains isolated from a fresh independent context");
  }

  reset_fake_gpu_k1_boruvka();
  K1BoruvkaCandidateContext failed{index, cloud};
  configure_fake_gpu_k1_boruvka(FakeK1BoruvkaConfiguration{
      FakeK1BoruvkaCorruption::simulated_gpu_failure});
  check_throws<std::runtime_error>(
      [&failed, &cloud, &labels]() {
        static_cast<void>(failed.propose_round(
            cloud, std::span<const PointId>{labels}));
      },
      "simulated GPU failure propagates without a candidate batch");
  check(
      fake_gpu_k1_boruvka_launch_count() == 0U,
      "simulated GPU failure occurs before publication counters advance");
  configure_fake_gpu_k1_boruvka(FakeK1BoruvkaConfiguration{});
  check_throws<std::runtime_error>(
      [&failed, &cloud, &labels]() {
        static_cast<void>(failed.propose_round(
            cloud, std::span<const PointId>{labels}));
      },
      "GPU failure poisons the context before any retry");
  check(
      fake_gpu_k1_boruvka_launch_count() == 0U,
      "poisoned failed context never invokes a retry launcher");

  const std::array<PointId, 4> terminal_labels{
      PointId{0}, PointId{0}, PointId{0}, PointId{0}};
  check_throws<std::runtime_error>(
      [&failed, &cloud, &terminal_labels]() {
        static_cast<void>(failed.propose_round(
            cloud, std::span<const PointId>{terminal_labels}));
      },
      "poisoned context rejects a later terminal publication too");
  check(
      fake_gpu_k1_boruvka_launch_count() == 0U,
      "poisoned terminal retry remains free of GPU launches");
}

}  // namespace

int main() {
  test_terminal_singleton_without_gpu_launch();
  test_square_ties_candidate_superset_and_threshold_equality();
  test_already_contracted_two_component_partition();
  test_invalid_namespaces_labels_and_moved_from_objects();
  test_corrupt_batches_and_gpu_failure_poison_context();

  if (failures != 0) {
    std::cerr << failures << " GPU K1 Boruvka context test(s) failed\n";
    return 1;
  }
  return 0;
}
