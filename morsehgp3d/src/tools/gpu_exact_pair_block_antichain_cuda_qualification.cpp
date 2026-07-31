#include "morsehgp3d/gpu/exact_pair_block_antichain_cuda.hpp"
#include "morsehgp3d/gpu/exact_pair_block_frontier.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/exact_block_rank_prune_receipt.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/aabb.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ExactPairBlockAntichainCudaConfig;
using morsehgp3d::gpu::ExactPairBlockAntichainCudaDecision;
using morsehgp3d::gpu::ExactPairBlockAntichainCudaStatus;
using morsehgp3d::gpu::ExactPairBlockAntichainCudaTransaction;
using morsehgp3d::gpu::ExactPairBlockAuthority;
using morsehgp3d::gpu::ExactPairBlockAuthorityKind;
using morsehgp3d::gpu::ExactPairBlockFrontierCapacity;
using morsehgp3d::gpu::ExactPairBlockFrontierConfig;
using morsehgp3d::gpu::ExactPairBlockFrontierContext;
using morsehgp3d::gpu::ExactPairBlockFrontierStepKind;
using morsehgp3d::gpu::ExactPairBlockNodeAuthority;
using morsehgp3d::gpu::ExactPairBlockOpenKind;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::hierarchy::ExactBlockRankPruneStatus;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::MortonLbvhIndex;

inline constexpr std::size_t qualification_point_count = 128U;
inline constexpr std::size_t certified_transaction_target = 512U;
inline constexpr std::size_t massive_singleton_authority_target = 128U;

struct MassiveOptions {
  std::size_t maximum_closed_rank{};
  std::size_t predicate_target{};
};

struct HarvestedAuthorities {
  std::vector<ExactPairBlockNodeAuthority> singletons;
  std::vector<ExactPairBlockNodeAuthority> native_nodes;
};

struct Candidate {
  ExactPairBlockAuthority support{};
  std::array<ExactPairBlockNodeAuthority, 10U> negative_witnesses{};
  std::size_t negative_witness_count{};
  std::optional<ExactPairBlockNodeAuthority> nonnegative_witness;
};

struct ExpectedTransaction {
  ExactPairBlockAntichainCudaTransaction transaction{};
  ExactPairBlockAntichainCudaDecision decision{};
};

struct BatchSummary {
  std::size_t maximum_closed_rank{};
  std::size_t transaction_count{};
  std::size_t predicate_task_count{};
  std::size_t certified_count{};
  std::size_t nonnegative_count{};
  std::size_t invalid_count{};
  std::size_t insufficient_count{};
  std::size_t multileaf_witness_transaction_count{};
  std::size_t support_mass_gt_one_transaction_count{};
  std::uint64_t submitted_transaction_digest{};
  std::uint64_t canonical_transaction_digest{};
  std::uint64_t completed_transaction_digest{};
  std::uint64_t predicate_task_digest{};
  std::uint64_t predicate_result_digest{};
  std::uint64_t process_local_transcript_digest{};
  std::uint64_t kernel_nanoseconds{};
  std::uint64_t wall_nanoseconds{};
};

[[nodiscard]] std::size_t checked_product(
    std::size_t first,
    std::size_t second,
    const char* message) {
  if (first != 0U &&
      second > std::numeric_limits<std::size_t>::max() / first) {
    throw std::overflow_error(message);
  }
  return first * second;
}

[[nodiscard]] std::size_t checked_sum(
    std::size_t first,
    std::size_t second,
    const char* message) {
  if (second > std::numeric_limits<std::size_t>::max() - first) {
    throw std::overflow_error(message);
  }
  return first + second;
}

[[nodiscard]] std::size_t ceiling_divide(
    std::size_t numerator,
    std::size_t denominator) {
  return numerator / denominator +
      (numerator % denominator == 0U ? 0U : 1U);
}

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    const char* message) {
  std::size_t value = 0U;
  const auto parsed = std::from_chars(
      text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc{} ||
      parsed.ptr != text.data() + text.size()) {
    throw std::invalid_argument(message);
  }
  return value;
}

[[nodiscard]] MassiveOptions parse_massive_options(
    int argc,
    char** argv) {
  MassiveOptions options;
  bool rank_seen = false;
  bool target_seen = false;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (index + 1 >= argc) {
      throw std::invalid_argument(
          "usage: --maximum-closed-rank 6|11 --predicate-target P");
    }
    const std::string_view value{argv[++index]};
    if (argument == "--maximum-closed-rank" && !rank_seen) {
      options.maximum_closed_rank =
          parse_size(value, "invalid --maximum-closed-rank");
      rank_seen = true;
    } else if (argument == "--predicate-target" && !target_seen) {
      options.predicate_target =
          parse_size(value, "invalid --predicate-target");
      target_seen = true;
    } else {
      throw std::invalid_argument(
          "usage: --maximum-closed-rank 6|11 --predicate-target P");
    }
  }
  if (!rank_seen || !target_seen ||
      (options.maximum_closed_rank != 6U &&
       options.maximum_closed_rank != 11U) ||
      options.predicate_target == 0U) {
    throw std::invalid_argument(
        "massive mode requires --maximum-closed-rank 6|11 and a positive "
        "--predicate-target");
  }
  const std::size_t witness_count =
      options.maximum_closed_rank - 1U;
  if (options.predicate_target % witness_count != 0U) {
    throw std::invalid_argument(
        "--predicate-target must be divisible by maximum_closed_rank - 1");
  }
  return options;
}

[[nodiscard]] CanonicalPointCloud line_cloud(std::size_t point_count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    const double x = static_cast<double>(index) - 64.0;
    points.push_back(CertifiedPoint3::from_binary64(x, 0.0, 0.0));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] bool disjoint(
    const ExactPairBlockNodeAuthority& first,
    const ExactPairBlockNodeAuthority& second) noexcept {
  return first.leaf_end <= second.leaf_begin ||
      second.leaf_end <= first.leaf_begin;
}

[[nodiscard]] HarvestedAuthorities harvest_authorities(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t required_singleton_count = qualification_point_count,
    bool retain_native_nodes = true) {
  if (required_singleton_count == 0U ||
      required_singleton_count > cloud.size()) {
    throw std::invalid_argument(
        "the antichain qualification requested an invalid singleton count");
  }
  std::vector<std::optional<ExactPairBlockNodeAuthority>> by_leaf(
      cloud.size());
  ExactPairBlockFrontierContext frontier =
      ExactPairBlockFrontierContext::start(
          index, cloud, ExactPairBlockFrontierConfig{11U, false});
  const ExactPairBlockFrontierCapacity capacity{};
  std::vector<ExactPairBlockNodeAuthority> native_nodes;
  const auto retain = [&native_nodes](
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
          "the antichain qualification observed conflicting native nodes");
    }
  };
  std::size_t step_count = 0U;
  std::size_t singleton_count = 0U;
  while (!frontier.complete() &&
         singleton_count < required_singleton_count) {
    if (++step_count > 8U * cloud.size() * cloud.size()) {
      throw std::logic_error(
          "the antichain qualification authority harvest did not finish");
    }
    const auto step = frontier.advance(index, cloud, capacity);
    if (step.kind == ExactPairBlockFrontierStepKind::complete) {
      break;
    }
    if (step.kind != ExactPairBlockFrontierStepKind::cross_block ||
        !step.cross_block.has_value()) {
      throw std::logic_error(
          "the antichain qualification exhausted host frontier capacity");
    }
    if (retain_native_nodes) {
      retain(step.cross_block->first);
      retain(step.cross_block->second);
    }
    const auto opened = frontier.open_cross_block(index, cloud, capacity);
    if (opened.kind == ExactPairBlockOpenKind::terminal_pair) {
      if (!by_leaf[opened.source.first.leaf_begin].has_value()) {
        by_leaf[opened.source.first.leaf_begin] = opened.source.first;
        ++singleton_count;
      }
      if (!by_leaf[opened.source.second.leaf_begin].has_value()) {
        by_leaf[opened.source.second.leaf_begin] = opened.source.second;
        ++singleton_count;
      }
    } else if (
        opened.kind != ExactPairBlockOpenKind::first_support_split &&
        opened.kind != ExactPairBlockOpenKind::second_support_split) {
      throw std::logic_error(
          "the antichain qualification could not open a native block");
    }
  }
  HarvestedAuthorities result;
  result.singletons.reserve(required_singleton_count);
  for (const auto& authority : by_leaf) {
    if (authority.has_value()) {
      result.singletons.push_back(*authority);
    }
  }
  if (result.singletons.size() < required_singleton_count) {
    throw std::logic_error(
        "the antichain qualification missed required singleton authority");
  }
  std::sort(
      native_nodes.begin(),
      native_nodes.end(),
      [](const ExactPairBlockNodeAuthority& first,
         const ExactPairBlockNodeAuthority& second) {
        if (first.leaf_count() != second.leaf_count()) {
          return first.leaf_count() < second.leaf_count();
        }
        if (first.leaf_begin != second.leaf_begin) {
          return first.leaf_begin < second.leaf_begin;
        }
        return first.node_index < second.node_index;
      });
  result.native_nodes = std::move(native_nodes);
  return result;
}

[[nodiscard]] ExactDyadicAabb3 authority_box(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const ExactPairBlockNodeAuthority& authority) {
  if (authority.leaf_begin >= authority.leaf_end ||
      authority.leaf_end > index.leaves().size()) {
    throw std::logic_error(
        "the antichain qualification cannot bound an invalid authority");
  }
  ExactDyadicAabb3 box{};
  const auto first = cloud
      .point(index.leaves()[authority.leaf_begin].point_id)
      .canonical_input_bits();
  box.lower_binary64_bits = first;
  box.upper_binary64_bits = first;
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

[[nodiscard]] int exact_q_sign(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const ExactPairBlockNodeAuthority& first,
    const ExactPairBlockNodeAuthority& second,
    const ExactPairBlockNodeAuthority& witness) {
  return morsehgp3d::hierarchy::exact_diametral_phi_aabb_maximum_sign(
      authority_box(index, cloud, first),
      authority_box(index, cloud, second),
      authority_box(index, cloud, witness));
}

[[nodiscard]] ExactPairBlockAuthority support(
    ExactPairBlockNodeAuthority first,
    ExactPairBlockNodeAuthority second) {
  if (second.leaf_begin < first.leaf_begin) {
    std::swap(first, second);
  }
  if (!disjoint(first, second)) {
    throw std::logic_error(
        "the antichain qualification selected overlapping supports");
  }
  return ExactPairBlockAuthority{
      ExactPairBlockAuthorityKind::cross,
      first,
      second,
      first.leaf_count() * second.leaf_count()};
}

[[nodiscard]] std::vector<Candidate> collect_candidates(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const HarvestedAuthorities& authorities) {
  std::vector<Candidate> candidates;
  candidates.reserve(certified_transaction_target);
  bool rollback_candidate_found = false;
  for (std::size_t first = 0U;
       first < authorities.singletons.size();
       ++first) {
    for (std::size_t second = first + 1U;
         second < authorities.singletons.size();
         ++second) {
      Candidate candidate;
      candidate.support = support(
          authorities.singletons[first],
          authorities.singletons[second]);
      for (const ExactPairBlockNodeAuthority& witness :
           authorities.singletons) {
        if (!disjoint(candidate.support.first, witness) ||
            !disjoint(candidate.support.second, witness)) {
          continue;
        }
        const int sign = exact_q_sign(
            index,
            cloud,
            candidate.support.first,
            candidate.support.second,
            witness);
        if (sign < 0 &&
            candidate.negative_witness_count <
                candidate.negative_witnesses.size()) {
          candidate.negative_witnesses[
              candidate.negative_witness_count] = witness;
          ++candidate.negative_witness_count;
        } else if (
            sign >= 0 && !candidate.nonnegative_witness.has_value()) {
          candidate.nonnegative_witness = witness;
        }
      }
      if (candidate.negative_witness_count ==
          candidate.negative_witnesses.size()) {
        rollback_candidate_found = rollback_candidate_found ||
            candidate.nonnegative_witness.has_value();
        candidates.push_back(candidate);
      }
      if (candidates.size() >= certified_transaction_target &&
          rollback_candidate_found) {
        return candidates;
      }
    }
  }
  throw std::logic_error(
      "the antichain qualification found too few rank-eleven candidates");
}

[[nodiscard]] Candidate collect_massive_model_candidate(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const HarvestedAuthorities& authorities,
    std::size_t required_witness_count) {
  for (std::size_t first = 0U;
       first < authorities.singletons.size();
       ++first) {
    for (std::size_t second = first + 1U;
         second < authorities.singletons.size();
         ++second) {
      Candidate candidate;
      candidate.support = support(
          authorities.singletons[first], authorities.singletons[second]);
      for (const ExactPairBlockNodeAuthority& witness :
           authorities.singletons) {
        if (!disjoint(candidate.support.first, witness) ||
            !disjoint(candidate.support.second, witness)) {
          continue;
        }
        if (exact_q_sign(
                index,
                cloud,
                candidate.support.first,
                candidate.support.second,
                witness) < 0) {
          candidate.negative_witnesses[
              candidate.negative_witness_count] = witness;
          ++candidate.negative_witness_count;
          if (candidate.negative_witness_count ==
              required_witness_count) {
            return candidate;
          }
        }
      }
    }
  }
  throw std::logic_error(
      "the massive antichain qualification found no model transaction");
}

[[nodiscard]] ExactPairBlockAntichainCudaTransaction transaction(
    std::uint64_t transaction_id,
    ExactPairBlockAuthority block,
    std::span<const ExactPairBlockNodeAuthority> witnesses,
    bool reverse_witnesses) {
  if (witnesses.size() > 10U) {
    throw std::logic_error(
        "the antichain qualification exceeded its witness array");
  }
  ExactPairBlockAntichainCudaTransaction result;
  result.transaction_id = transaction_id;
  result.support_block = block;
  result.witness_node_count = witnesses.size();
  for (std::size_t index = 0U; index < witnesses.size(); ++index) {
    const std::size_t source_index = reverse_witnesses
        ? witnesses.size() - 1U - index
        : index;
    result.witness_nodes[index] = witnesses[source_index];
  }
  return result;
}

void require_cpu_status(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const ExactPairBlockAntichainCudaTransaction& input,
    std::size_t maximum_closed_rank,
    ExactBlockRankPruneStatus expected) {
  std::array<std::size_t, 10U> witness_indices{};
  for (std::size_t witness = 0U;
       witness < input.witness_node_count;
       ++witness) {
    witness_indices[witness] = input.witness_nodes[witness].node_index;
  }
  auto oracle =
      morsehgp3d::hierarchy::certify_exact_block_rank_prune_receipt(
          index,
          cloud,
          input.support_block.first.node_index,
          input.support_block.second.node_index,
          std::span<const std::size_t>{
              witness_indices.data(), input.witness_node_count},
          maximum_closed_rank);
  if (oracle.status() != expected) {
    throw std::logic_error(
        "the CPU receipt oracle rejected an antichain fixture decision");
  }
  if (expected != ExactBlockRankPruneStatus::certified_above_rank) {
    if (oracle.certified() || oracle.receipt() != nullptr) {
      throw std::logic_error(
          "the CPU oracle attached a receipt to an inconclusive fixture");
    }
    return;
  }
  const auto* receipt = oracle.receipt();
  if (!oracle.certified() || receipt == nullptr ||
      !receipt->validated_for(index, cloud) ||
      !receipt->certifies(
          index,
          cloud,
          input.support_block.first.node_index,
          input.support_block.second.node_index,
          maximum_closed_rank) ||
      receipt->unordered_pair_mass() !=
          input.support_block.unordered_pair_mass ||
      receipt->maximum_closed_rank() != maximum_closed_rank ||
      receipt->required_witness_point_count() !=
          maximum_closed_rank - 1U) {
    throw std::logic_error(
        "the CPU antichain receipt did not retain process-local authority");
  }
  std::array<ExactPairBlockNodeAuthority, 10U> canonical{};
  for (std::size_t witness = 0U;
       witness < input.witness_node_count;
       ++witness) {
    canonical[witness] = input.witness_nodes[witness];
  }
  const auto precedes =
      [](const ExactPairBlockNodeAuthority& first,
         const ExactPairBlockNodeAuthority& second) {
        if (first.leaf_begin != second.leaf_begin) {
          return first.leaf_begin < second.leaf_begin;
        }
        if (first.leaf_end != second.leaf_end) {
          return first.leaf_end < second.leaf_end;
        }
        return first.node_index < second.node_index;
      };
  for (std::size_t sorted_count = 1U;
       sorted_count < input.witness_node_count;
       ++sorted_count) {
    const ExactPairBlockNodeAuthority inserted = canonical[sorted_count];
    std::size_t destination = sorted_count;
    while (destination != 0U &&
           precedes(inserted, canonical[destination - 1U])) {
      canonical[destination] = canonical[destination - 1U];
      --destination;
    }
    canonical[destination] = inserted;
  }
  const auto retained = receipt->witness_nodes();
  if (retained.size() != input.witness_node_count) {
    throw std::logic_error(
        "the CPU antichain receipt lost canonical witness nodes");
  }
  std::size_t retained_mass = 0U;
  for (std::size_t witness = 0U; witness < retained.size(); ++witness) {
    if (retained[witness].node_index != canonical[witness].node_index ||
        retained[witness].leaf_begin != canonical[witness].leaf_begin ||
        retained[witness].leaf_end != canonical[witness].leaf_end) {
      throw std::logic_error(
          "the CPU antichain receipt changed a witness authority");
    }
    retained_mass += retained[witness].leaf_count();
  }
  if (retained_mass != receipt->certified_witness_point_count()) {
    throw std::logic_error(
        "the CPU antichain receipt changed certified witness mass");
  }
}

[[nodiscard]] Candidate collect_multi_support_candidate(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const HarvestedAuthorities& authorities) {
  for (std::size_t first_index = 0U;
       first_index < authorities.native_nodes.size();
       ++first_index) {
    const ExactPairBlockNodeAuthority& first =
        authorities.native_nodes[first_index];
    if (first.leaf_count() <= 1U || first.leaf_count() > 16U) {
      continue;
    }
    for (std::size_t second_index = first_index + 1U;
         second_index < authorities.native_nodes.size();
         ++second_index) {
      const ExactPairBlockNodeAuthority& second =
          authorities.native_nodes[second_index];
      if (second.leaf_count() <= 1U || second.leaf_count() > 16U ||
          !disjoint(first, second)) {
        continue;
      }
      Candidate candidate;
      candidate.support = support(first, second);
      for (const ExactPairBlockNodeAuthority& witness :
           authorities.singletons) {
        if (!disjoint(candidate.support.first, witness) ||
            !disjoint(candidate.support.second, witness)) {
          continue;
        }
        const int sign = exact_q_sign(
            index,
            cloud,
            candidate.support.first,
            candidate.support.second,
            witness);
        if (sign < 0 &&
            candidate.negative_witness_count <
                candidate.negative_witnesses.size()) {
          candidate.negative_witnesses[
              candidate.negative_witness_count] = witness;
          ++candidate.negative_witness_count;
        } else if (
            sign >= 0 && !candidate.nonnegative_witness.has_value()) {
          candidate.nonnegative_witness = witness;
        }
      }
      if (candidate.support.unordered_pair_mass > 1U &&
          candidate.negative_witness_count ==
              candidate.negative_witnesses.size() &&
          candidate.nonnegative_witness.has_value()) {
        return candidate;
      }
    }
  }
  throw std::logic_error(
      "the antichain qualification found no multileaf support product");
}

[[nodiscard]] std::optional<ExactPairBlockAntichainCudaTransaction>
multileaf_transaction(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const HarvestedAuthorities& authorities,
    const std::vector<Candidate>& candidates,
    std::size_t maximum_closed_rank,
    std::uint64_t transaction_id) {
  const std::size_t required = maximum_closed_rank - 1U;
  for (const Candidate& candidate : candidates) {
    for (const ExactPairBlockNodeAuthority& witness :
         authorities.native_nodes) {
      if (witness.leaf_count() <= 1U ||
          witness.leaf_count() < required ||
          !disjoint(candidate.support.first, witness) ||
          !disjoint(candidate.support.second, witness) ||
          exact_q_sign(
              index,
              cloud,
              candidate.support.first,
              candidate.support.second,
              witness) >= 0) {
        continue;
      }
      const std::array<ExactPairBlockNodeAuthority, 1U> witnesses{witness};
      auto result = transaction(
          transaction_id, candidate.support, witnesses, false);
      require_cpu_status(
          index,
          cloud,
          result,
          maximum_closed_rank,
          ExactBlockRankPruneStatus::certified_above_rank);
      return result;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<ExpectedTransaction> build_transactions(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const HarvestedAuthorities& authorities,
    std::size_t maximum_closed_rank) {
  const std::vector<Candidate> candidates =
      collect_candidates(index, cloud, authorities);
  const Candidate multi_support =
      collect_multi_support_candidate(index, cloud, authorities);
  const std::size_t required = maximum_closed_rank - 1U;
  std::vector<ExpectedTransaction> result;
  result.reserve(certified_transaction_target + 5U);
  const std::uint64_t id_base =
      static_cast<std::uint64_t>(maximum_closed_rank) * UINT64_C(1000000);
  for (std::size_t index_value = 0U;
       index_value < certified_transaction_target;
       ++index_value) {
    const Candidate& candidate = candidates[index_value];
    const auto input = transaction(
        id_base + static_cast<std::uint64_t>(index_value),
        candidate.support,
        std::span<const ExactPairBlockNodeAuthority>{
            candidate.negative_witnesses.data(), required},
        index_value % 2U != 0U);
    require_cpu_status(
        index,
        cloud,
        input,
        maximum_closed_rank,
        ExactBlockRankPruneStatus::certified_above_rank);
    result.push_back(ExpectedTransaction{
        input, ExactPairBlockAntichainCudaDecision::certified_closed});
  }

  const auto multi = multileaf_transaction(
      index,
      cloud,
      authorities,
      candidates,
      maximum_closed_rank,
      id_base + UINT64_C(900000));
  if (!multi.has_value()) {
    throw std::logic_error(
        "the antichain qualification found no exact multileaf witness");
  }
  result.push_back(ExpectedTransaction{
      *multi, ExactPairBlockAntichainCudaDecision::certified_closed});

  const auto multi_support_input = transaction(
      id_base + UINT64_C(900001),
      multi_support.support,
      std::span<const ExactPairBlockNodeAuthority>{
          multi_support.negative_witnesses.data(), required},
      true);
  require_cpu_status(
      index,
      cloud,
      multi_support_input,
      maximum_closed_rank,
      ExactBlockRankPruneStatus::certified_above_rank);
  result.push_back(ExpectedTransaction{
      multi_support_input,
      ExactPairBlockAntichainCudaDecision::certified_closed});

  std::array<ExactPairBlockNodeAuthority, 10U> rollback_witnesses{};
  for (std::size_t witness = 0U; witness + 1U < required; ++witness) {
    rollback_witnesses[witness] =
        multi_support.negative_witnesses[witness];
  }
  rollback_witnesses[required - 1U] =
      *multi_support.nonnegative_witness;
  const auto rollback_input = transaction(
      id_base + UINT64_C(900002),
      multi_support.support,
      std::span<const ExactPairBlockNodeAuthority>{
          rollback_witnesses.data(), required},
      false);
  require_cpu_status(
      index,
      cloud,
      rollback_input,
      maximum_closed_rank,
      ExactBlockRankPruneStatus::inconclusive_nonnegative_phi);
  result.push_back(ExpectedTransaction{
      rollback_input,
      ExactPairBlockAntichainCudaDecision::residual_nonnegative_q});

  auto invalid = multi_support_input;
  invalid.transaction_id = id_base + UINT64_C(900003);
  const ExactPairBlockNodeAuthority original = invalid.witness_nodes[0];
  const auto different = std::find_if(
      authorities.native_nodes.begin(),
      authorities.native_nodes.end(),
      [original](const ExactPairBlockNodeAuthority& candidate) {
        return candidate.node_index != original.node_index &&
            (candidate.leaf_begin != original.leaf_begin ||
             candidate.leaf_end != original.leaf_end);
      });
  if (different == authorities.native_nodes.end()) {
    throw std::logic_error(
        "the antichain qualification could not stage a native mutation");
  }
  invalid.witness_nodes[0].node_index = different->node_index;
  result.push_back(ExpectedTransaction{
      invalid,
      ExactPairBlockAntichainCudaDecision::
          residual_invalid_native_authority});

  const auto insufficient = transaction(
      id_base + UINT64_C(900004),
      multi_support.support,
      std::span<const ExactPairBlockNodeAuthority>{
          multi_support.negative_witnesses.data(), required - 1U},
      true);
  require_cpu_status(
      index,
      cloud,
      insufficient,
      maximum_closed_rank,
      ExactBlockRankPruneStatus::insufficient_witness_mass);
  result.push_back(ExpectedTransaction{
      insufficient,
      ExactPairBlockAntichainCudaDecision::
          inconclusive_insufficient_witness_mass});
  return result;
}

[[nodiscard]] BatchSummary run_batch(
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank) {
  MortonLbvhBuildContext builder{qualification_point_count};
  auto build = builder.build(cloud);
  if (!build.cuda_qualified_build()) {
    throw std::logic_error(
        "the antichain qualification requires a CUDA-certified LBVH");
  }
  const MortonLbvhIndex& index = build.certified_index();
  const HarvestedAuthorities authorities =
      harvest_authorities(index, cloud);
  const std::vector<ExpectedTransaction> expected =
      build_transactions(index, cloud, authorities, maximum_closed_rank);
  std::vector<ExactPairBlockAntichainCudaTransaction> transactions;
  transactions.reserve(expected.size());
  for (const ExpectedTransaction& entry : expected) {
    transactions.push_back(entry.transaction);
  }

  auto traversal = builder.release_device_traversal_lease(build);
  const auto begin = std::chrono::steady_clock::now();
  auto result = morsehgp3d::gpu::
      qualify_exact_pair_block_antichain_transactions_cuda(
          std::move(traversal),
          transactions,
          ExactPairBlockAntichainCudaConfig{maximum_closed_rank, 12U});
  const auto end = std::chrono::steady_clock::now();
  const std::uint64_t wall_nanoseconds = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin)
          .count());

  if (result.status !=
          ExactPairBlockAntichainCudaStatus::qualified_component_batch ||
      !result.qualified_cuda_batch() ||
      result.records.size() != expected.size()) {
    throw std::logic_error(
        "the native antichain component did not qualify its complete batch");
  }
  std::size_t mismatch_count = 0U;
  std::size_t multileaf_count = 0U;
  std::size_t support_mass_gt_one_count = 0U;
  std::size_t expected_pruned_mass = 0U;
  std::size_t expected_residual_mass = 0U;
  for (std::size_t index_value = 0U;
       index_value < expected.size();
       ++index_value) {
    const auto& observed = result.records[index_value];
    const auto& wanted = expected[index_value];
    if (observed.transaction_id != wanted.transaction.transaction_id ||
        observed.decision != wanted.decision ||
        observed.unordered_pair_mass !=
            wanted.transaction.support_block.unordered_pair_mass) {
      ++mismatch_count;
    }
    if (wanted.transaction.support_block.unordered_pair_mass > 1U) {
      ++support_mass_gt_one_count;
    }
    if (wanted.decision ==
        ExactPairBlockAntichainCudaDecision::certified_closed) {
      expected_pruned_mass +=
          wanted.transaction.support_block.unordered_pair_mass;
    } else {
      expected_residual_mass +=
          wanted.transaction.support_block.unordered_pair_mass;
    }
    if (wanted.decision ==
            ExactPairBlockAntichainCudaDecision::
                residual_invalid_native_authority &&
        observed.authenticated_witness_point_count != 0U) {
      ++mismatch_count;
    }
    if (wanted.decision ==
            ExactPairBlockAntichainCudaDecision::
                inconclusive_insufficient_witness_mass &&
        observed.authenticated_witness_point_count != 0U) {
      ++mismatch_count;
    }
    for (std::size_t witness = 0U;
         witness < wanted.transaction.witness_node_count;
         ++witness) {
      if (wanted.transaction.witness_nodes[witness].leaf_count() > 1U) {
        ++multileaf_count;
        break;
      }
    }
  }
  const auto& audit = result.audit;
  if (mismatch_count != 0U || multileaf_count == 0U ||
      support_mass_gt_one_count < 4U ||
      audit.certified_transaction_count !=
          certified_transaction_target + 2U ||
      audit.nonnegative_transaction_count != 1U ||
      audit.invalid_native_authority_transaction_count != 1U ||
      audit.insufficient_witness_transaction_count != 1U ||
      audit.completed_transaction_count != transactions.size() ||
      audit.pruned_unordered_pair_mass != expected_pruned_mass ||
      audit.residual_unordered_pair_mass != expected_residual_mass ||
      audit.submitted_unordered_pair_mass !=
          expected_pruned_mass + expected_residual_mass ||
      audit.kernel_launch_count != 1U ||
      audit.synchronization_count != 1U ||
      audit.per_transaction_allocation_count != 0U ||
      audit.per_transaction_synchronization_count != 0U ||
      !audit.native_lbvh_authority_consumed ||
      !audit.native_lbvh_nodes_read_on_device ||
      !audit.cuda_execution_performed ||
      !audit.canonical_witness_antichains_validated ||
      !audit.transactional_product_mass_conservation_validated ||
      !audit.every_certified_transaction_has_sufficient_disjoint_exact_negative_witness_mass ||
      !audit.one_batched_predicate_submission_validated ||
      !audit.fixed_linear_transaction_capacity_validated ||
      !audit.fixed_linear_predicate_capacity_validated ||
      !audit.compact_index_width_validated ||
      !audit.unique_transaction_ids_validated ||
      !audit.predicate_result_identities_validated ||
      !audit.process_local_transcript_validated ||
      audit.submitted_transaction_digest == 0U ||
      audit.canonical_transaction_digest == 0U ||
      audit.completed_transaction_digest == 0U ||
      audit.predicate_task_digest == 0U ||
      audit.predicate_result_digest == 0U ||
      audit.process_local_transcript_digest == 0U ||
      audit.submitted_transaction_digest ==
          audit.canonical_transaction_digest ||
      audit.pairwise_disjoint_support_products_validated ||
      audit.double_buffered_transactional_frontier_claimed ||
      audit.global_pair_coverage_closed || audit.pair_catalog_complete_claimed ||
      audit.hierarchy_or_tree_claimed || audit.slo_claimed ||
      audit.global_pair_matrix_materialized ||
      audit.ordinary_or_higher_order_delaunay_materialized ||
      audit.durable_receipt_claimed || audit.public_status_claimed) {
    throw std::logic_error(
        "the native antichain batch violated its local exactness contract");
  }

  return BatchSummary{
      maximum_closed_rank,
      transactions.size(),
      audit.predicate_task_count,
      audit.certified_transaction_count,
      audit.nonnegative_transaction_count,
      audit.invalid_native_authority_transaction_count,
      audit.insufficient_witness_transaction_count,
      multileaf_count,
      support_mass_gt_one_count,
      audit.submitted_transaction_digest,
      audit.canonical_transaction_digest,
      audit.completed_transaction_digest,
      audit.predicate_task_digest,
      audit.predicate_result_digest,
      audit.process_local_transcript_digest,
      audit.predicate_kernel_elapsed_nanoseconds,
      wall_nanoseconds};
}

[[nodiscard]] std::uint64_t elapsed_nanoseconds(
    std::chrono::steady_clock::time_point begin,
    std::chrono::steady_clock::time_point end) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin)
          .count());
}

int run_massive(const MassiveOptions& options) {
  const std::size_t required_witness_count =
      options.maximum_closed_rank - 1U;
  const std::size_t transaction_count =
      options.predicate_target / required_witness_count;
  const std::size_t point_count = std::max(
      massive_singleton_authority_target,
      std::max(
          ceiling_divide(
              transaction_count,
              morsehgp3d::gpu::
                  exact_pair_block_antichain_cuda_maximum_transaction_capacity_per_point),
          ceiling_divide(
              options.predicate_target,
              morsehgp3d::gpu::
                  exact_pair_block_witness_cuda_maximum_task_capacity_per_point)));
  if (point_count > std::numeric_limits<std::uint32_t>::max() ||
      transaction_count >
          static_cast<std::size_t>(
              std::numeric_limits<std::uint64_t>::max())) {
    throw std::overflow_error(
        "the massive antichain qualification exceeds its compact ABI");
  }
  const std::size_t transaction_capacity = checked_product(
      point_count,
      morsehgp3d::gpu::
          exact_pair_block_antichain_cuda_maximum_transaction_capacity_per_point,
      "the massive antichain transaction capacity overflows");
  const std::size_t predicate_capacity = checked_product(
      point_count,
      morsehgp3d::gpu::
          exact_pair_block_witness_cuda_maximum_task_capacity_per_point,
      "the massive antichain predicate capacity overflows");
  if (transaction_count > transaction_capacity ||
      options.predicate_target > predicate_capacity) {
    throw std::logic_error(
        "the massive antichain qualification bypassed a linear capacity");
  }

  const auto total_begin = std::chrono::steady_clock::now();
  const auto cloud_begin = total_begin;
  const CanonicalPointCloud cloud = line_cloud(point_count);
  const auto cloud_end = std::chrono::steady_clock::now();

  MortonLbvhBuildContext builder{point_count};
  const auto build_begin = std::chrono::steady_clock::now();
  auto build = builder.build(cloud);
  const auto build_end = std::chrono::steady_clock::now();
  if (!build.cuda_qualified_build()) {
    throw std::logic_error(
        "the massive antichain qualification requires a CUDA-certified LBVH");
  }
  const MortonLbvhIndex& index = build.certified_index();

  const auto model_begin = std::chrono::steady_clock::now();
  const HarvestedAuthorities authorities = harvest_authorities(
      index,
      cloud,
      massive_singleton_authority_target,
      false);
  const Candidate candidate = collect_massive_model_candidate(
      index, cloud, authorities, required_witness_count);
  const auto model = transaction(
      0U,
      candidate.support,
      std::span<const ExactPairBlockNodeAuthority>{
          candidate.negative_witnesses.data(), required_witness_count},
      false);
  require_cpu_status(
      index,
      cloud,
      model,
      options.maximum_closed_rank,
      ExactBlockRankPruneStatus::certified_above_rank);
  const auto model_end = std::chrono::steady_clock::now();

  const auto materialize_begin = std::chrono::steady_clock::now();
  const ExactPairBlockAntichainCudaTransaction compact_model = model;
  const auto materialize_end = std::chrono::steady_clock::now();

  auto traversal = builder.release_device_traversal_lease(build);
  const std::size_t persistent_device_bytes =
      traversal.audit().persistent_device_byte_capacity;
  const auto compositor_begin = std::chrono::steady_clock::now();
  auto result = morsehgp3d::gpu::
      qualify_repeated_exact_pair_block_antichain_transaction_cuda(
          std::move(traversal),
          compact_model,
          transaction_count,
          ExactPairBlockAntichainCudaConfig{
              options.maximum_closed_rank,
              morsehgp3d::gpu::
                  exact_pair_block_antichain_cuda_maximum_transaction_capacity_per_point});
  const auto compositor_end = std::chrono::steady_clock::now();

  const std::size_t expected_mass = checked_product(
      transaction_count,
      model.support_block.unordered_pair_mass,
      "the massive antichain expected pair mass overflows");
  const auto& audit = result.audit;
  const auto& record = result.first_record;
  const bool records_valid =
      result.contains_transaction_id(0U) &&
      result.contains_transaction_id(
          static_cast<std::uint64_t>(transaction_count - 1U)) &&
      record.transaction_id == 0U &&
      record.decision ==
          ExactPairBlockAntichainCudaDecision::certified_closed &&
      record.witness_node_count == required_witness_count &&
      record.exact_negative_witness_node_count == required_witness_count &&
      record.authenticated_witness_point_count >= required_witness_count &&
      record.unordered_pair_mass == model.support_block.unordered_pair_mass;
  const bool qualified =
      result.status ==
          ExactPairBlockAntichainCudaStatus::qualified_component_batch &&
      result.qualified_cuda_batch() && records_valid &&
      audit.point_count == point_count &&
      audit.maximum_closed_rank == options.maximum_closed_rank &&
      audit.transaction_capacity == transaction_capacity &&
      audit.predicate_task_capacity == predicate_capacity &&
      audit.submitted_transaction_count == transaction_count &&
      audit.completed_transaction_count == transaction_count &&
      audit.submitted_witness_node_count == options.predicate_target &&
      audit.predicate_task_count == options.predicate_target &&
      audit.certified_transaction_count == transaction_count &&
      audit.nonnegative_transaction_count == 0U &&
      audit.invalid_native_authority_transaction_count == 0U &&
      audit.insufficient_witness_transaction_count == 0U &&
      audit.submitted_unordered_pair_mass == expected_mass &&
      audit.pruned_unordered_pair_mass == expected_mass &&
      audit.residual_unordered_pair_mass == 0U &&
      audit.unique_transaction_ids_validated &&
      audit.canonical_witness_antichains_validated &&
      audit.transactional_product_mass_conservation_validated &&
      audit.one_batched_predicate_submission_validated &&
      audit.predicate_pattern_task_count == required_witness_count &&
      audit.physical_predicate_task_count == options.predicate_target &&
      audit.compact_repeated_transaction_recipe_validated &&
      audit.affine_transaction_id_range_validated &&
      audit.every_logical_transaction_represented_once &&
      audit.per_transaction_host_materialization_avoided &&
      audit.physical_repeated_predicate_expansion_performed &&
      !audit.pairwise_disjoint_support_products_validated &&
      !audit.global_pair_coverage_closed &&
      !audit.hierarchy_or_tree_claimed && !audit.slo_claimed &&
      !audit.public_status_claimed;
  const auto total_end = std::chrono::steady_clock::now();

  const std::size_t device_batch_arena_bytes = checked_sum(
      audit.host_to_device_predicate_task_byte_count,
      audit.device_to_host_predicate_result_byte_count,
      "the massive antichain device batch arena overflows");
  const std::size_t accounted_device_bytes = checked_sum(
      persistent_device_bytes,
      device_batch_arena_bytes,
      "the massive antichain accounted device bytes overflow");
  const std::size_t transaction_input_bytes = checked_product(
      1U,
      sizeof(ExactPairBlockAntichainCudaTransaction),
      "the massive antichain transaction input bytes overflow");
  const std::size_t transaction_output_bytes = checked_product(
      1U,
      sizeof(morsehgp3d::gpu::ExactPairBlockAntichainCudaRecord),
      "the massive antichain transaction output bytes overflow");

  std::cout
      << "{\"accounted_device_bytes\":" << accounted_device_bytes << ','
      << "\"backend\":\"cuda_g4\","
      << "\"batch_semantics\":\"compact_repeated_exact_model_recipe_"
         "with_affine_unique_ids_and_physical_predicate_expansion\","
      << "\"cloud_generation_nanoseconds\":"
      << elapsed_nanoseconds(cloud_begin, cloud_end) << ','
      << "\"compositor_wall_nanoseconds\":"
      << elapsed_nanoseconds(compositor_begin, compositor_end) << ','
      << "\"cpu_oracle_model_certified\":true,"
      << "\"cpu_oracle_model_invocation_count\":1,"
      << "\"device_batch_arena_bytes\":" << device_batch_arena_bytes
      << ','
      << "\"device_to_host_result_bytes\":"
      << audit.device_to_host_predicate_result_byte_count << ','
      << "\"global_pair_coverage_closed\":false,"
      << "\"hierarchy_or_tree_claimed\":false,"
      << "\"host_to_device_task_bytes\":"
      << audit.host_to_device_predicate_task_byte_count << ','
      << "\"kernel_elapsed_nanoseconds\":"
      << audit.predicate_kernel_elapsed_nanoseconds << ','
      << "\"lbvh_build_nanoseconds\":"
      << elapsed_nanoseconds(build_begin, build_end) << ','
      << "\"massive_mode\":true,"
      << "\"materialization_nanoseconds\":"
      << elapsed_nanoseconds(materialize_begin, materialize_end) << ','
      << "\"maximum_closed_rank\":" << options.maximum_closed_rank << ','
      << "\"model_qualification_nanoseconds\":"
      << elapsed_nanoseconds(model_begin, model_end) << ','
      << "\"morse_order_k\":" << required_witness_count << ','
      << "\"pairwise_disjoint_support_products_validated\":false,"
      << "\"per_transaction_host_materialization_avoided\":true,"
      << "\"persistent_lbvh_device_bytes\":" << persistent_device_bytes
      << ','
      << "\"physical_predicate_task_count\":"
      << audit.physical_predicate_task_count << ','
      << "\"point_count\":" << point_count << ','
      << "\"predicate_capacity\":" << predicate_capacity << ','
      << "\"predicate_pattern_task_count\":"
      << audit.predicate_pattern_task_count << ','
      << "\"predicate_target\":" << options.predicate_target << ','
      << "\"process_local_transcript_digest\":"
      << audit.process_local_transcript_digest << ','
      << "\"profile\":\"hgp_reduced\","
      << "\"public_status\":\"not_claimed\","
      << "\"qualified\":" << (qualified ? "true" : "false") << ','
      << "\"repeated_model_transaction\":true,"
      << "\"schema_version\":1,"
      << "\"slo_claimed\":false,"
      << "\"submitted_support_products_may_overlap\":true,"
      << "\"total_wall_nanoseconds\":"
      << elapsed_nanoseconds(total_begin, total_end) << ','
      << "\"transaction_capacity\":" << transaction_capacity << ','
      << "\"transaction_count\":" << transaction_count << ','
      << "\"transaction_input_bytes\":" << transaction_input_bytes << ','
      << "\"transaction_output_bytes\":" << transaction_output_bytes
      << "}\n";
  return qualified ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 1) {
      return run_massive(parse_massive_options(argc, argv));
    }
    const CanonicalPointCloud cloud = line_cloud(qualification_point_count);
    const BatchSummary rank_six = run_batch(cloud, 6U);
    const BatchSummary rank_eleven = run_batch(cloud, 11U);
    const std::size_t total_predicate_tasks =
        rank_six.predicate_task_count + rank_eleven.predicate_task_count;
    const bool qualified =
        rank_six.certified_count == certified_transaction_target + 2U &&
        rank_eleven.certified_count ==
            certified_transaction_target + 2U &&
        total_predicate_tasks >= 7000U;
    std::cout
        << "{\"backend\":\"cuda_g4\","
        << "\"global_pair_coverage_closed\":false,"
        << "\"hierarchy_or_tree_claimed\":false,"
        << "\"k10_canonical_transaction_digest\":"
        << rank_eleven.canonical_transaction_digest << ','
        << "\"k10_certified_transaction_count\":"
        << rank_eleven.certified_count << ','
        << "\"k10_completed_transaction_digest\":"
        << rank_eleven.completed_transaction_digest << ','
        << "\"k10_invalid_transaction_count\":"
        << rank_eleven.invalid_count << ','
        << "\"k10_kernel_elapsed_nanoseconds\":"
        << rank_eleven.kernel_nanoseconds << ','
        << "\"k10_maximum_closed_rank\":"
        << rank_eleven.maximum_closed_rank << ','
        << "\"k10_nonnegative_transaction_count\":"
        << rank_eleven.nonnegative_count << ','
        << "\"k10_predicate_result_digest\":"
        << rank_eleven.predicate_result_digest << ','
        << "\"k10_predicate_task_count\":"
        << rank_eleven.predicate_task_count << ','
        << "\"k10_predicate_task_digest\":"
        << rank_eleven.predicate_task_digest << ','
        << "\"k10_process_local_transcript_digest\":"
        << rank_eleven.process_local_transcript_digest << ','
        << "\"k10_submitted_transaction_digest\":"
        << rank_eleven.submitted_transaction_digest << ','
        << "\"k10_transaction_count\":"
        << rank_eleven.transaction_count << ','
        << "\"k10_wall_nanoseconds\":"
        << rank_eleven.wall_nanoseconds << ','
        << "\"k5_canonical_transaction_digest\":"
        << rank_six.canonical_transaction_digest << ','
        << "\"k5_certified_transaction_count\":"
        << rank_six.certified_count << ','
        << "\"k5_completed_transaction_digest\":"
        << rank_six.completed_transaction_digest << ','
        << "\"k5_invalid_transaction_count\":"
        << rank_six.invalid_count << ','
        << "\"k5_kernel_elapsed_nanoseconds\":"
        << rank_six.kernel_nanoseconds << ','
        << "\"k5_maximum_closed_rank\":"
        << rank_six.maximum_closed_rank << ','
        << "\"k5_nonnegative_transaction_count\":"
        << rank_six.nonnegative_count << ','
        << "\"k5_predicate_result_digest\":"
        << rank_six.predicate_result_digest << ','
        << "\"k5_predicate_task_count\":"
        << rank_six.predicate_task_count << ','
        << "\"k5_predicate_task_digest\":"
        << rank_six.predicate_task_digest << ','
        << "\"k5_process_local_transcript_digest\":"
        << rank_six.process_local_transcript_digest << ','
        << "\"k5_submitted_transaction_digest\":"
        << rank_six.submitted_transaction_digest << ','
        << "\"k5_transaction_count\":"
        << rank_six.transaction_count << ','
        << "\"k5_wall_nanoseconds\":"
        << rank_six.wall_nanoseconds << ','
        << "\"mode\":\"native_transactional_multi_node_antichain_"
           "differential\","
        << "\"multileaf_witness_transaction_count\":"
        << rank_six.multileaf_witness_transaction_count +
               rank_eleven.multileaf_witness_transaction_count
        << ','
        << "\"point_count\":" << qualification_point_count << ','
        << "\"profile\":\"hgp_reduced\","
        << "\"public_status\":\"not_claimed\","
        << "\"qualified\":" << (qualified ? "true" : "false") << ','
        << "\"schema_version\":1,"
        << "\"slo_claimed\":false,"
        << "\"support_mass_gt_one_transaction_count\":"
        << rank_six.support_mass_gt_one_transaction_count +
               rank_eleven.support_mass_gt_one_transaction_count
        << ','
        << "\"total_predicate_task_count\":"
        << total_predicate_tasks << "}\n";
    return qualified ? 0 : 1;
  } catch (const std::exception& error) {
    std::cerr
        << "exact pair-block antichain CUDA qualification failed: "
        << error.what() << '\n';
    return 1;
  }
}
