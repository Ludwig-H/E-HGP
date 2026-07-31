#include "morsehgp3d/gpu/exact_pair_block_frontier.hpp"
#include "morsehgp3d/gpu/exact_pair_block_witness_cuda.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/aabb.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ExactPairBlockAuthority;
using morsehgp3d::gpu::ExactPairBlockAuthorityKind;
using morsehgp3d::gpu::ExactPairBlockFrontierCapacity;
using morsehgp3d::gpu::ExactPairBlockFrontierConfig;
using morsehgp3d::gpu::ExactPairBlockFrontierContext;
using morsehgp3d::gpu::ExactPairBlockFrontierStepKind;
using morsehgp3d::gpu::ExactPairBlockNodeAuthority;
using morsehgp3d::gpu::ExactPairBlockOpenKind;
using morsehgp3d::gpu::ExactPairBlockWitnessCudaConfig;
using morsehgp3d::gpu::ExactPairBlockWitnessCudaDecision;
using morsehgp3d::gpu::ExactPairBlockWitnessCudaStatus;
using morsehgp3d::gpu::ExactPairBlockWitnessCudaTask;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::MortonLbvhIndex;

inline constexpr std::size_t qualification_point_count = 128U;
inline constexpr std::size_t qualification_task_count =
    qualification_point_count * (qualification_point_count - 1U) / 2U;
inline constexpr std::size_t qualification_k_max = 5U;
inline constexpr std::size_t qualification_maximum_closed_rank =
    qualification_k_max + 1U;

struct HarvestedAuthorities {
  std::vector<ExactPairBlockNodeAuthority> singletons;
  std::vector<ExactPairBlockNodeAuthority> native_nodes;
};

struct QualifiedTask {
  ExactPairBlockWitnessCudaTask task{};
  int expected_sign{};
};

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud adversarial_cloud() {
  const double denormal = std::numeric_limits<double>::denorm_min();
  const double maximum = std::numeric_limits<double>::max();
  std::vector<CertifiedPoint3> points;
  points.reserve(qualification_point_count);
  points.push_back(point(-0.0, 0.0, 0.0));
  points.push_back(point(+0.0, 1.0, 0.0));
  points.push_back(point(-denormal, 0.0, 0.0));
  points.push_back(point(denormal, 0.0, 0.0));
  points.push_back(point(-1.0, 0.0, 0.0));
  points.push_back(point(1.0, 0.0, 0.0));
  points.push_back(point(0.0, 1.0, denormal));
  points.push_back(point(maximum, 0.0, 0.0));
  points.push_back(point(std::nextafter(maximum, 0.0), 1.0, 0.0));
  points.push_back(point(-maximum, 0.0, 0.0));
  points.push_back(point(std::nextafter(-maximum, 0.0), -1.0, 0.0));
  for (std::size_t index = points.size();
       index < qualification_point_count;
       ++index) {
    const double x = static_cast<double>(index);
    const double y = static_cast<double>((index * index + 17U) % 97U) / 8.0;
    const double z = static_cast<double>((index * 19U + 3U) % 89U) / 32.0;
    points.push_back(point(x, y, z));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] HarvestedAuthorities harvest_authorities(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) {
  std::vector<std::optional<ExactPairBlockNodeAuthority>> by_leaf(
      cloud.size());
  ExactPairBlockFrontierContext frontier =
      ExactPairBlockFrontierContext::start(
          index, cloud, ExactPairBlockFrontierConfig{2U, false});
  const ExactPairBlockFrontierCapacity capacity{};
  std::vector<ExactPairBlockNodeAuthority> native_nodes;
  const auto retain_native = [&native_nodes](
                                 ExactPairBlockNodeAuthority authority) {
    const auto existing = std::find_if(
        native_nodes.begin(),
        native_nodes.end(),
        [authority](const ExactPairBlockNodeAuthority& candidate) {
          return candidate.node_index == authority.node_index;
        });
    if (existing == native_nodes.end()) {
      native_nodes.push_back(authority);
    } else if (*existing != authority) {
      throw std::logic_error(
          "the qualification observed conflicting native node authority");
    }
  };
  std::size_t step_count = 0U;
  while (!frontier.complete()) {
    if (++step_count > 8U * cloud.size() * cloud.size()) {
      throw std::logic_error(
          "the qualification authority harvest exceeded its bounded steps");
    }
    const auto step = frontier.advance(index, cloud, capacity);
    if (step.kind == ExactPairBlockFrontierStepKind::complete) {
      break;
    }
    if (step.kind != ExactPairBlockFrontierStepKind::cross_block ||
        !step.cross_block.has_value()) {
      throw std::logic_error(
          "the qualification authority harvest exhausted host capacity");
    }
    retain_native(step.cross_block->first);
    retain_native(step.cross_block->second);
    const auto opened = frontier.open_cross_block(index, cloud, capacity);
    if (opened.kind == ExactPairBlockOpenKind::terminal_pair) {
      if (opened.source.first.leaf_count() != 1U ||
          opened.source.second.leaf_count() != 1U) {
        throw std::logic_error(
            "the qualification terminal authority is not singleton");
      }
      by_leaf[opened.source.first.leaf_begin] = opened.source.first;
      by_leaf[opened.source.second.leaf_begin] = opened.source.second;
    } else if (
        opened.kind != ExactPairBlockOpenKind::first_support_split &&
        opened.kind != ExactPairBlockOpenKind::second_support_split) {
      throw std::logic_error(
          "the qualification authority harvest could not open a block");
    }
  }

  HarvestedAuthorities result;
  result.singletons.reserve(by_leaf.size());
  for (const auto& authority : by_leaf) {
    if (!authority.has_value()) {
      throw std::logic_error(
          "the qualification authority harvest missed a singleton node");
    }
    result.singletons.push_back(*authority);
  }
  std::sort(
      native_nodes.begin(),
      native_nodes.end(),
      [](const ExactPairBlockNodeAuthority& left,
         const ExactPairBlockNodeAuthority& right) {
        if (left.leaf_count() != right.leaf_count()) {
          return left.leaf_count() < right.leaf_count();
        }
        if (left.leaf_begin != right.leaf_begin) {
          return left.leaf_begin < right.leaf_begin;
        }
        return left.node_index < right.node_index;
      });
  result.native_nodes = std::move(native_nodes);
  return result;
}

[[nodiscard]] bool disjoint(
    const ExactPairBlockNodeAuthority& first,
    const ExactPairBlockNodeAuthority& second) noexcept {
  return first.leaf_end <= second.leaf_begin ||
      second.leaf_end <= first.leaf_begin;
}

[[nodiscard]] ExactDyadicAabb3 authority_box(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const ExactPairBlockNodeAuthority& authority) {
  if (authority.leaf_begin >= authority.leaf_end ||
      authority.leaf_end > index.leaves().size()) {
    throw std::logic_error(
        "the qualification cannot bound an invalid native authority");
  }
  ExactDyadicAabb3 box{};
  const auto first_words = cloud
      .point(index.leaves()[authority.leaf_begin].point_id)
      .canonical_input_bits();
  box.lower_binary64_bits = first_words;
  box.upper_binary64_bits = first_words;
  for (std::size_t leaf = authority.leaf_begin + 1U;
       leaf < authority.leaf_end;
       ++leaf) {
    const auto words =
        cloud.point(index.leaves()[leaf].point_id).canonical_input_bits();
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      if (morsehgp3d::exact::binary64_total_order_key(words[axis]) <
          morsehgp3d::exact::binary64_total_order_key(
              box.lower_binary64_bits[axis])) {
        box.lower_binary64_bits[axis] = words[axis];
      }
      if (morsehgp3d::exact::binary64_total_order_key(words[axis]) >
          morsehgp3d::exact::binary64_total_order_key(
              box.upper_binary64_bits[axis])) {
        box.upper_binary64_bits[axis] = words[axis];
      }
    }
  }
  return box;
}

[[nodiscard]] bool fixed_safe_box(const ExactDyadicAabb3& box) noexcept {
  // A nonzero dynamic range contained in [2^-30, 2^30] aligns in far fewer
  // than the fixed kernel's proved 126 global coordinate bits.  Zero needs
  // no exponent alignment and remains admissible.
  constexpr double minimum_nonzero = 0x1p-30;
  constexpr double maximum_magnitude = 0x1p30;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::uint64_t words[2]{
        box.lower_binary64_bits[axis], box.upper_binary64_bits[axis]};
    for (const std::uint64_t word : words) {
      const double value = std::bit_cast<double>(word);
      const double magnitude = std::abs(value);
      if (magnitude > maximum_magnitude ||
          (magnitude != 0.0 && magnitude < minimum_nonzero)) {
        return false;
      }
    }
  }
  return true;
}

[[nodiscard]] QualifiedTask make_task(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    ExactPairBlockNodeAuthority first,
    ExactPairBlockNodeAuthority second,
    ExactPairBlockNodeAuthority witness) {
  if (second.leaf_begin < first.leaf_begin) {
    std::swap(first, second);
  }
  if (!disjoint(first, second) || !disjoint(first, witness) ||
      !disjoint(second, witness)) {
    throw std::logic_error(
        "the qualification attempted an overlapping A x B x W task");
  }
  const std::size_t mass = first.leaf_count() * second.leaf_count();
  const ExactDyadicAabb3 first_box = authority_box(index, cloud, first);
  const ExactDyadicAabb3 second_box = authority_box(index, cloud, second);
  const ExactDyadicAabb3 witness_box = authority_box(index, cloud, witness);
  return QualifiedTask{
      ExactPairBlockWitnessCudaTask{
          0U,
          ExactPairBlockAuthority{
              ExactPairBlockAuthorityKind::cross,
              first,
              second,
              mass},
          witness},
      morsehgp3d::hierarchy::exact_diametral_phi_aabb_maximum_sign(
          first_box, second_box, witness_box)};
}

}  // namespace

int main() {
  try {
    CanonicalPointCloud cloud = adversarial_cloud();
    if (cloud.size() != qualification_point_count) {
      throw std::logic_error(
          "the qualification cloud lost its 128-point authority");
    }
    MortonLbvhBuildContext builder{qualification_point_count};
    auto build = builder.build(cloud);
    if (!build.cuda_qualified_build()) {
      throw std::logic_error(
          "the qualification requires a real CUDA-certified LBVH build");
    }
    const MortonLbvhIndex& index = build.certified_index();
    const HarvestedAuthorities authorities =
        harvest_authorities(index, cloud);
    if (authorities.singletons.size() != qualification_point_count) {
      throw std::logic_error(
          "the qualification did not harvest 128 singleton authorities");
    }

    std::vector<QualifiedTask> qualified_tasks;
    qualified_tasks.reserve(qualification_task_count);
    for (std::size_t first = 0U;
         first < qualification_point_count;
         ++first) {
      for (std::size_t second = first + 1U;
           second < qualification_point_count;
           ++second) {
        const auto witness = std::find_if(
            authorities.native_nodes.begin(),
            authorities.native_nodes.end(),
            [&](const ExactPairBlockNodeAuthority& candidate) {
              return candidate.leaf_count() >= qualification_k_max &&
                  disjoint(candidate, authorities.singletons[first]) &&
                  disjoint(candidate, authorities.singletons[second]);
            });
        if (witness == authorities.native_nodes.end()) {
          continue;
        }
        qualified_tasks.push_back(make_task(
            index,
            cloud,
            authorities.singletons[first],
            authorities.singletons[second],
            *witness));
      }
    }
    if (qualified_tasks.empty()) {
      throw std::logic_error(
          "the qualification found no rank-six singleton-support task");
    }
    const std::size_t distinct_base_task_count = qualified_tasks.size();
    while (qualified_tasks.size() < qualification_task_count) {
      qualified_tasks.push_back(
          qualified_tasks[qualified_tasks.size() % distinct_base_task_count]);
    }
    qualified_tasks.resize(qualification_task_count);

    struct AuthorityAndBox {
      ExactPairBlockNodeAuthority authority{};
      ExactDyadicAabb3 box{};
    };
    std::vector<AuthorityAndBox> safe_witnesses;
    std::vector<AuthorityAndBox> safe_multileaf_nodes;
    for (const ExactPairBlockNodeAuthority& authority :
         authorities.native_nodes) {
      if (authority.leaf_count() <= 1U) {
        continue;
      }
      const ExactDyadicAabb3 box = authority_box(index, cloud, authority);
      if (!fixed_safe_box(box)) {
        continue;
      }
      safe_multileaf_nodes.push_back(AuthorityAndBox{authority, box});
      if (authority.leaf_count() >= qualification_k_max) {
        safe_witnesses.push_back(AuthorityAndBox{authority, box});
      }
    }
    if (safe_witnesses.empty()) {
      throw std::logic_error(
          "the qualification found no fixed-safe five-leaf witness node");
    }

    std::vector<QualifiedTask> multileaf_tasks;
    constexpr std::size_t requested_multileaf_task_count = 8U;
    for (const AuthorityAndBox& first : safe_multileaf_nodes) {
      for (const AuthorityAndBox& second : safe_multileaf_nodes) {
        if (!disjoint(first.authority, second.authority)) {
          continue;
        }
        for (const AuthorityAndBox& witness : safe_witnesses) {
          if (!disjoint(first.authority, witness.authority) ||
              !disjoint(second.authority, witness.authority)) {
            continue;
          }
          multileaf_tasks.push_back(make_task(
              index,
              cloud,
              first.authority,
              second.authority,
              witness.authority));
          if (multileaf_tasks.size() ==
              requested_multileaf_task_count) {
            break;
          }
        }
        if (multileaf_tasks.size() == requested_multileaf_task_count) {
          break;
        }
      }
      if (multileaf_tasks.size() == requested_multileaf_task_count) {
        break;
      }
    }
    if (multileaf_tasks.size() != requested_multileaf_task_count) {
      throw std::logic_error(
          "the qualification did not find eight disjoint multi-leaf "
          "A x B x W tasks");
    }
    for (std::size_t task_index = 0U;
         task_index < multileaf_tasks.size();
         ++task_index) {
      qualified_tasks[task_index] = multileaf_tasks[task_index];
    }

    std::vector<ExactDyadicAabb3> singleton_boxes;
    singleton_boxes.reserve(authorities.singletons.size());
    for (const ExactPairBlockNodeAuthority& singleton :
         authorities.singletons) {
      singleton_boxes.push_back(authority_box(index, cloud, singleton));
    }
    std::optional<QualifiedTask> safe_negative;
    std::optional<QualifiedTask> safe_nonnegative;
    for (std::size_t first = 0U;
         first < qualification_point_count &&
             (!safe_negative.has_value() ||
              !safe_nonnegative.has_value());
         ++first) {
      if (!fixed_safe_box(singleton_boxes[first])) {
        continue;
      }
      for (std::size_t second = first + 1U;
           second < qualification_point_count &&
               (!safe_negative.has_value() ||
                !safe_nonnegative.has_value());
           ++second) {
        if (!fixed_safe_box(singleton_boxes[second])) {
          continue;
        }
        for (const AuthorityAndBox& witness : safe_witnesses) {
          if (!disjoint(
                  authorities.singletons[first], witness.authority) ||
              !disjoint(
                  authorities.singletons[second], witness.authority)) {
            continue;
          }
          const int sign = morsehgp3d::hierarchy::
              exact_diametral_phi_aabb_maximum_sign(
                  singleton_boxes[first],
                  singleton_boxes[second],
                  witness.box);
          if (sign < 0 && !safe_negative.has_value()) {
            safe_negative = make_task(
                index,
                cloud,
                authorities.singletons[first],
                authorities.singletons[second],
                witness.authority);
          } else if (sign >= 0 && !safe_nonnegative.has_value()) {
            safe_nonnegative = make_task(
                index,
                cloud,
                authorities.singletons[first],
                authorities.singletons[second],
                witness.authority);
          }
          if (safe_negative.has_value() && safe_nonnegative.has_value()) {
            break;
          }
        }
      }
    }
    if (!safe_negative.has_value() || !safe_nonnegative.has_value()) {
      throw std::logic_error(
          "the qualification could not stage fixed-safe negative and "
          "nonnegative rank-six tasks");
    }
    qualified_tasks[multileaf_tasks.size()] = *safe_negative;
    qualified_tasks[multileaf_tasks.size() + 1U] = *safe_nonnegative;

    std::vector<ExactPairBlockWitnessCudaTask> tasks;
    std::vector<int> expected_signs;
    tasks.reserve(qualified_tasks.size());
    expected_signs.reserve(qualified_tasks.size());
    std::size_t multileaf_first_support_task_count = 0U;
    std::size_t multileaf_second_support_task_count = 0U;
    std::size_t multileaf_witness_task_count = 0U;
    std::size_t all_multileaf_task_count = 0U;
    std::size_t rank_sufficient_witness_task_count = 0U;
    std::size_t support_mass_gt_one_task_count = 0U;
    for (std::size_t task_index = 0U;
         task_index < qualified_tasks.size();
         ++task_index) {
      QualifiedTask task = qualified_tasks[task_index];
      task.task.task_id = static_cast<std::uint64_t>(task_index);
      const bool first_multileaf =
          task.task.support_block.first.leaf_count() > 1U;
      const bool second_multileaf =
          task.task.support_block.second.leaf_count() > 1U;
      const bool witness_multileaf = task.task.witness.leaf_count() > 1U;
      multileaf_first_support_task_count += first_multileaf ? 1U : 0U;
      multileaf_second_support_task_count += second_multileaf ? 1U : 0U;
      multileaf_witness_task_count += witness_multileaf ? 1U : 0U;
      all_multileaf_task_count +=
          first_multileaf && second_multileaf && witness_multileaf ? 1U : 0U;
      rank_sufficient_witness_task_count +=
          task.task.witness.leaf_count() >= qualification_k_max ? 1U : 0U;
      support_mass_gt_one_task_count +=
          task.task.support_block.unordered_pair_mass > 1U ? 1U : 0U;
      tasks.push_back(task.task);
      expected_signs.push_back(task.expected_sign);
    }

    auto traversal = builder.release_device_traversal_lease(build);
    const auto begin = std::chrono::steady_clock::now();
    auto result =
        morsehgp3d::gpu::qualify_exact_pair_block_witnesses_cuda(
            std::move(traversal),
            tasks,
            ExactPairBlockWitnessCudaConfig{
                qualification_maximum_closed_rank, 64U});
    const auto end = std::chrono::steady_clock::now();
    const std::uint64_t wall_nanoseconds = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin)
            .count());

    std::size_t mismatch_count = 0U;
    std::size_t residual_count = 0U;
    std::size_t all_multileaf_nonresidual_task_count = 0U;
    for (std::size_t index_value = 0U;
         index_value < result.records.size();
         ++index_value) {
      const auto& record = result.records[index_value];
      if (record.decision == ExactPairBlockWitnessCudaDecision::
              residual_fixed_limb_overflow) {
        ++residual_count;
        continue;
      }
      const ExactPairBlockWitnessCudaTask& task = tasks[index_value];
      if (task.support_block.first.leaf_count() > 1U &&
          task.support_block.second.leaf_count() > 1U &&
          task.witness.leaf_count() > 1U) {
        ++all_multileaf_nonresidual_task_count;
      }
      const int expected = expected_signs[index_value];
      const bool sign_matches = record.exact_sign == expected;
      const bool decision_matches = expected < 0
          ? record.decision ==
                ExactPairBlockWitnessCudaDecision::certified_closed
          : record.decision == ExactPairBlockWitnessCudaDecision::
                inconclusive_nonnegative_q;
      if (!record.fixed_limb_evaluation_performed || !sign_matches ||
          !decision_matches) {
        ++mismatch_count;
      }
    }
    const bool all_nonresidual_decisions_match_exact_oracle =
        mismatch_count == 0U;
    const bool qualified =
        result.status ==
            ExactPairBlockWitnessCudaStatus::qualified_component_batch &&
        result.qualified_cuda_batch() &&
        all_nonresidual_decisions_match_exact_oracle &&
        tasks.size() == qualification_task_count &&
        result.records.size() == tasks.size() &&
        result.audit.certified_task_count != 0U &&
        result.audit.nonnegative_task_count != 0U &&
        multileaf_first_support_task_count != 0U &&
        multileaf_second_support_task_count != 0U &&
        multileaf_witness_task_count != 0U &&
        all_multileaf_task_count != 0U &&
        all_multileaf_nonresidual_task_count != 0U &&
        support_mass_gt_one_task_count != 0U &&
        rank_sufficient_witness_task_count == tasks.size() &&
        result.audit.maximum_closed_rank ==
            qualification_maximum_closed_rank &&
        result.audit.invalid_authority_task_count == 0U &&
        result.audit.support_overlap_task_count == 0U &&
        result.audit.insufficient_witness_task_count == 0U &&
        !result.audit.global_pair_coverage_closed &&
        !result.audit.slo_claimed;

    std::cout
        << "{\"all_multileaf_AxBxW_nonresidual_task_count\":"
        << all_multileaf_nonresidual_task_count << ','
        << "\"all_multileaf_AxBxW_task_count\":"
        << all_multileaf_task_count << ','
        << "\"all_nonresidual_decisions_match_exact_oracle\":"
        << (all_nonresidual_decisions_match_exact_oracle ? "true" : "false")
        << ','
        << "\"backend\":\"cuda_g4\","
        << "\"certified_count\":" << result.audit.certified_task_count << ','
        << "\"completed_result_digest\":"
        << result.audit.completed_result_digest << ','
        << "\"directed_ambiguous_count\":"
        << result.audit.directed_ambiguous_count << ','
        << "\"directed_negative_count\":"
        << result.audit.directed_negative_count << ','
        << "\"fixed_limb_residual_count\":" << residual_count << ','
        << "\"fixture_profile\":\"signed_zero_subnormal_cancellation_"
           "finite_extremes\","
        << "\"global_coverage_closed\":false,"
        << "\"k_max\":" << qualification_k_max << ','
        << "\"kernel_elapsed_nanoseconds\":"
        << result.audit.kernel_elapsed_nanoseconds << ','
        << "\"maximum_closed_rank\":"
        << qualification_maximum_closed_rank << ','
        << "\"mismatch_count\":" << mismatch_count << ','
        << "\"mode\":\"native_AxBxW_n128_differential\","
        << "\"multileaf_first_support_task_count\":"
        << multileaf_first_support_task_count << ','
        << "\"multileaf_second_support_task_count\":"
        << multileaf_second_support_task_count << ','
        << "\"multileaf_witness_task_count\":"
        << multileaf_witness_task_count << ','
        << "\"nonnegative_count\":"
        << result.audit.nonnegative_task_count << ','
        << "\"point_count\":" << qualification_point_count << ','
        << "\"profile\":\"hgp_reduced\","
        << "\"qualified\":" << (qualified ? "true" : "false") << ','
        << "\"rank_sufficient_witness_task_count\":"
        << rank_sufficient_witness_task_count << ','
        << "\"required_witness_point_count\":" << qualification_k_max
        << ','
        << "\"schema_version\":1,"
        << "\"submitted_task_digest\":"
        << result.audit.submitted_task_digest << ','
        << "\"support_mass_gt_one_task_count\":"
        << support_mass_gt_one_task_count << ','
        << "\"task_count\":" << tasks.size() << ','
        << "\"wall_nanoseconds\":" << wall_nanoseconds << "}\n";
    return qualified ? 0 : 1;
  } catch (const std::exception& error) {
    std::cerr << "exact pair-block witness CUDA qualification failed: "
              << error.what() << '\n';
    return 1;
  }
}
