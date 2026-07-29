#include "morsehgp3d/gpu/exact_diametral_phi.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::PredicateSign;
using morsehgp3d::gpu::ExactDiametralPhiBatchAudit;
using morsehgp3d::gpu::ExactDiametralPhiBatchOptions;
using morsehgp3d::gpu::ExactDiametralPhiCertificationStage;
using morsehgp3d::gpu::ExactDiametralPhiContext;
using morsehgp3d::gpu::ExactDiametralPhiDecision;
using morsehgp3d::gpu::ExactDiametralPhiInput;

inline constexpr std::size_t deterministic_random_query_count = 4096U;
inline constexpr std::size_t symmetry_source_count = 256U;
inline constexpr std::size_t axis_permutation_count = 6U;
inline constexpr std::size_t symmetry_query_count =
    symmetry_source_count * (axis_permutation_count * 2U - 1U);
inline constexpr std::size_t near_shell_scale_count = 8U;
inline constexpr std::size_t near_shell_query_count =
    near_shell_scale_count * 3U * 2U * 3U;
inline constexpr std::size_t adversarial_query_count = 10U;
inline constexpr std::size_t expected_query_count =
    adversarial_query_count + deterministic_random_query_count +
    symmetry_query_count + near_shell_query_count;

struct KnownSign {
  std::size_t query_index{};
  PredicateSign sign{PredicateSign::zero};
};

[[nodiscard]] std::uint64_t bits(double value) {
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] std::array<std::uint64_t, 3U> point(
    double x,
    double y,
    double z) {
  return {bits(x), bits(y), bits(z)};
}

[[nodiscard]] ExactDiametralPhiInput make_input(
    std::uint64_t replay_id,
    std::array<std::uint64_t, 3U> witness,
    std::array<std::uint64_t, 3U> first,
    std::array<std::uint64_t, 3U> second) {
  return ExactDiametralPhiInput{replay_id, witness, first, second};
}

[[nodiscard]] ExactDiametralPhiInput accumulator_overflow(
    std::uint64_t replay_id) {
  constexpr std::uint64_t fraction_mask =
      (UINT64_C(1) << 52U) - UINT64_C(1);
  // The represented value is (2^53-1)*2^75. Subtracting -1 gives a
  // 128-bit difference. Two equal positive squared terms exceed 256 bits and
  // must therefore fail open to CPU multiprecision.
  constexpr std::uint64_t internal_exponent_75 = 1150U;
  const std::uint64_t high =
      (internal_exponent_75 << 52U) | fraction_mask;
  return ExactDiametralPhiInput{
      replay_id,
      {high, high, 0U},
      {bits(-1.0), bits(-1.0), 0U},
      {bits(-1.0), bits(-1.0), 0U}};
}

void append_known(
    std::vector<ExactDiametralPhiInput>& inputs,
    std::vector<KnownSign>& known_signs,
    std::uint64_t& next_replay_id,
    std::array<std::uint64_t, 3U> witness,
    std::array<std::uint64_t, 3U> first,
    std::array<std::uint64_t, 3U> second,
    PredicateSign sign) {
  known_signs.push_back(KnownSign{inputs.size(), sign});
  inputs.push_back(make_input(next_replay_id++, witness, first, second));
}

void append_adversarial_queries(
    std::vector<ExactDiametralPhiInput>& inputs,
    std::vector<KnownSign>& known_signs,
    std::uint64_t& next_replay_id) {
  append_known(
      inputs, known_signs, next_replay_id, point(0.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0), point(1.0, 0.0, 0.0),
      PredicateSign::negative);
  append_known(
      inputs, known_signs, next_replay_id, point(2.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0), point(1.0, 0.0, 0.0),
      PredicateSign::positive);
  append_known(
      inputs, known_signs, next_replay_id, point(0.0, 0.0, 0.0),
      point(-2.0, -3.0, -4.0), point(2.0, 3.0, 4.0),
      PredicateSign::negative);
  append_known(
      inputs, known_signs, next_replay_id, point(10.0, 10.0, 10.0),
      point(0.0, 0.0, 0.0), point(1.0, 1.0, 1.0),
      PredicateSign::positive);
  append_known(
      inputs, known_signs, next_replay_id, point(3.0, -2.0, 7.0),
      point(3.0, -2.0, 7.0), point(-4.0, 5.0, 9.0),
      PredicateSign::zero);
  append_known(
      inputs, known_signs, next_replay_id, point(-4.0, 5.0, 9.0),
      point(3.0, -2.0, 7.0), point(-4.0, 5.0, 9.0),
      PredicateSign::zero);
  append_known(
      inputs, known_signs, next_replay_id, point(2.0, 2.0, 0.0),
      point(1.0, 1.0, 1.0), point(1.0, 1.0, -2.0),
      PredicateSign::zero);
  append_known(
      inputs, known_signs, next_replay_id, {1U, 0U, 0U}, {0U, 0U, 0U},
      {2U, 0U, 0U}, PredicateSign::negative);
  append_known(
      inputs, known_signs, next_replay_id,
      {UINT64_C(0x7fefffffffffffff), 0U, 0U}, {1U, 0U, 0U},
      {0U, 0U, 0U}, PredicateSign::positive);
  known_signs.push_back(KnownSign{inputs.size(), PredicateSign::positive});
  inputs.push_back(accumulator_overflow(next_replay_id++));
}

[[nodiscard]] std::uint64_t next_random(std::uint64_t& state) noexcept {
  state ^= state << 13U;
  state ^= state >> 7U;
  state ^= state << 17U;
  return state;
}

[[nodiscard]] std::uint64_t bounded_finite_word(
    std::uint64_t random) noexcept {
  constexpr std::uint64_t fraction_mask =
      (UINT64_C(1) << 52U) - UINT64_C(1);
  const std::uint64_t sign = random & (UINT64_C(1) << 63U);
  const std::uint64_t fraction = random & fraction_mask;
  // Unbiased exponents -23..+23. This keeps the random campaign inside the
  // fixed-limb sufficient envelope while retaining heterogeneous scales.
  const std::uint64_t exponent = 1000U + ((random >> 53U) % 47U);
  return sign | (exponent << 52U) | fraction;
}

void append_random_queries(
    std::vector<ExactDiametralPhiInput>& inputs,
    std::vector<std::size_t>& symmetry_sources,
    std::uint64_t& next_replay_id) {
  std::uint64_t random_state = UINT64_C(0x42d1a6f3789bc05e);
  for (std::size_t index = 0U; index < deterministic_random_query_count;
       ++index) {
    ExactDiametralPhiInput value{};
    value.replay_id = next_replay_id++;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      value.witness_bits[axis] =
          bounded_finite_word(next_random(random_state));
      value.first_support_bits[axis] =
          bounded_finite_word(next_random(random_state));
      value.second_support_bits[axis] =
          bounded_finite_word(next_random(random_state));
    }
    if (index < symmetry_source_count) {
      symmetry_sources.push_back(inputs.size());
    }
    inputs.push_back(value);
  }
}

[[nodiscard]] std::array<std::uint64_t, 3U> permute_point(
    const std::array<std::uint64_t, 3U>& value,
    const std::array<std::size_t, 3U>& permutation) {
  return {
      value[permutation[0]], value[permutation[1]], value[permutation[2]]};
}

void append_symmetry_queries(
    std::vector<ExactDiametralPhiInput>& inputs,
    const std::vector<std::size_t>& symmetry_sources,
    std::vector<std::pair<std::size_t, std::size_t>>& symmetry_pairs,
    std::uint64_t& next_replay_id) {
  constexpr std::array<std::array<std::size_t, 3U>, axis_permutation_count>
      permutations{{
          {0U, 1U, 2U},
          {0U, 2U, 1U},
          {1U, 0U, 2U},
          {1U, 2U, 0U},
          {2U, 0U, 1U},
          {2U, 1U, 0U},
      }};
  for (const std::size_t source_index : symmetry_sources) {
    const ExactDiametralPhiInput source = inputs[source_index];
    for (std::size_t permutation_index = 0U;
         permutation_index < permutations.size(); ++permutation_index) {
      for (std::size_t swap_index = 0U; swap_index < 2U; ++swap_index) {
        if (permutation_index == 0U && swap_index == 0U) {
          continue;
        }
        const auto witness =
            permute_point(source.witness_bits, permutations[permutation_index]);
        auto first = permute_point(
            source.first_support_bits, permutations[permutation_index]);
        auto second = permute_point(
            source.second_support_bits, permutations[permutation_index]);
        if (swap_index != 0U) {
          std::swap(first, second);
        }
        symmetry_pairs.emplace_back(source_index, inputs.size());
        inputs.push_back(
            make_input(next_replay_id++, witness, first, second));
      }
    }
  }
}

void append_near_shell_queries(
    std::vector<ExactDiametralPhiInput>& inputs,
    std::vector<KnownSign>& known_signs,
    std::uint64_t& next_replay_id) {
  // Positive finite encodings ordered by numerical value. For every scale s,
  // the witnesses predecessor(s), s and successor(s) are respectively one
  // ulp inside, exactly on, and one ulp outside the diametral sphere [-s,+s].
  constexpr std::array<std::uint64_t, near_shell_scale_count> scales{
      UINT64_C(0x0000000000000001),
      UINT64_C(0x0010000000000000),
      UINT64_C(0x1ff0000000000000),
      UINT64_C(0x2ff0000000000000),
      UINT64_C(0x3ff0000000000000),
      UINT64_C(0x4ff0000000000000),
      UINT64_C(0x5ff0000000000000),
      UINT64_C(0x7e70000000000000),
  };
  constexpr std::array<PredicateSign, 3U> signs{
      PredicateSign::negative,
      PredicateSign::zero,
      PredicateSign::positive,
  };
  for (const std::uint64_t scale : scales) {
    const std::array<std::uint64_t, 3U> witnesses{
        scale - 1U, scale, scale + 1U};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      for (std::size_t swap_index = 0U; swap_index < 2U; ++swap_index) {
        std::array<std::uint64_t, 3U> first{};
        std::array<std::uint64_t, 3U> second{};
        first[axis] = scale | (UINT64_C(1) << 63U);
        second[axis] = scale;
        if (swap_index != 0U) {
          std::swap(first, second);
        }
        for (std::size_t shell_position = 0U;
             shell_position < witnesses.size(); ++shell_position) {
          std::array<std::uint64_t, 3U> witness{};
          witness[axis] = witnesses[shell_position];
          append_known(
              inputs, known_signs, next_replay_id, witness, first, second,
              signs[shell_position]);
        }
      }
    }
  }
}

void validate_known_signs(
    std::span<const ExactDiametralPhiDecision> decisions,
    std::span<const KnownSign> known_signs) {
  for (const KnownSign& known : known_signs) {
    if (decisions[known.query_index].sign != known.sign) {
      throw std::runtime_error("qualification known-sign fixture mismatch");
    }
  }
}

void validate_symmetries(
    std::span<const ExactDiametralPhiDecision> decisions,
    std::span<const std::pair<std::size_t, std::size_t>> symmetry_pairs) {
  for (const auto& [source_index, transformed_index] : symmetry_pairs) {
    if (decisions[source_index].sign != decisions[transformed_index].sign) {
      throw std::runtime_error(
          "qualification axis/support symmetry changed an exact sign");
    }
  }
}

void validate_audit(
    const ExactDiametralPhiBatchAudit& audit,
    std::size_t query_count) {
  if (audit.input_count != query_count ||
      audit.maximum_query_count != query_count ||
      audit.gpu_fp64_interval_count == 0U ||
      audit.gpu_fixed_limb_dyadic_count == 0U ||
      audit.gpu_exact_fallback_count == 0U ||
      audit.cpu_multiprecision_count == 0U || audit.exact_zero_count == 0U ||
      audit.gpu_fp64_interval_count + audit.gpu_fixed_limb_dyadic_count +
              audit.gpu_exact_fallback_count !=
          query_count ||
      audit.gpu_exact_fallback_count != audit.cpu_multiprecision_count ||
      audit.gpu_fp64_interval_count + audit.gpu_fixed_limb_dyadic_count !=
          audit.gpu_known_audited_count ||
      audit.disagreement_count != 0U || audit.launcher_call_count != 1U ||
      audit.cuda_kernel_launch_count != 1U ||
      audit.cuda_synchronization_count != 1U || audit.cuda_device < 0 ||
      !audit.cuda_execution_performed || !audit.all_results_exact ||
      !audit.interval_zero_certification_forbidden || audit.epsilon_used ||
      audit.scientific_catalog_decision_published ||
      audit.public_status_claimed) {
    throw std::runtime_error("qualification audit mismatch");
  }
}

void fnv1a_byte(std::uint64_t& digest, std::uint8_t byte) noexcept {
  digest ^= byte;
  digest *= UINT64_C(1099511628211);
}

[[nodiscard]] std::uint8_t sign_code(PredicateSign sign) {
  switch (sign) {
    case PredicateSign::negative:
      return 0U;
    case PredicateSign::zero:
      return 1U;
    case PredicateSign::positive:
      return 2U;
  }
  throw std::runtime_error("qualification encountered an invalid exact sign");
}

[[nodiscard]] std::uint64_t decision_digest_fnv1a(
    std::span<const ExactDiametralPhiDecision> decisions) {
  std::uint64_t digest = UINT64_C(14695981039346656037);
  for (const ExactDiametralPhiDecision& decision : decisions) {
    for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
      fnv1a_byte(
          digest,
          static_cast<std::uint8_t>(decision.replay_id >> shift));
    }
    fnv1a_byte(digest, sign_code(decision.sign));
    fnv1a_byte(
        digest,
        static_cast<std::uint8_t>(decision.certification_stage));
  }
  return digest;
}

[[nodiscard]] int run() {
  std::vector<ExactDiametralPhiInput> inputs;
  std::vector<KnownSign> known_signs;
  std::vector<std::size_t> symmetry_sources;
  std::vector<std::pair<std::size_t, std::size_t>> symmetry_pairs;
  inputs.reserve(expected_query_count);
  known_signs.reserve(adversarial_query_count + near_shell_query_count);
  symmetry_sources.reserve(symmetry_source_count);
  symmetry_pairs.reserve(symmetry_query_count);
  std::uint64_t next_replay_id = 0U;
  append_adversarial_queries(inputs, known_signs, next_replay_id);
  append_random_queries(inputs, symmetry_sources, next_replay_id);
  append_symmetry_queries(
      inputs, symmetry_sources, symmetry_pairs, next_replay_id);
  append_near_shell_queries(inputs, known_signs, next_replay_id);
  if (inputs.size() != expected_query_count ||
      symmetry_pairs.size() != symmetry_query_count ||
      next_replay_id != inputs.size()) {
    throw std::runtime_error("qualification campaign cardinality mismatch");
  }

  ExactDiametralPhiContext context{inputs.size()};
  const auto first = context.decide(
      inputs, ExactDiametralPhiBatchOptions{true});
  const auto second = context.decide(
      inputs, ExactDiametralPhiBatchOptions{true});
  validate_audit(first.audit, expected_query_count);
  validate_audit(second.audit, expected_query_count);
  if (first.decisions != second.decisions || first.audit != second.audit) {
    throw std::runtime_error(
        "qualification deterministic replay changed decisions or stages");
  }
  if (first.decisions.size() != inputs.size()) {
    throw std::runtime_error("qualification decision cardinality mismatch");
  }
  for (std::size_t index = 0U; index < inputs.size(); ++index) {
    if (first.decisions[index].replay_id != inputs[index].replay_id) {
      throw std::runtime_error("qualification replay order mismatch");
    }
  }
  validate_known_signs(first.decisions, known_signs);
  validate_symmetries(first.decisions, symmetry_pairs);
  const std::uint64_t digest = decision_digest_fnv1a(first.decisions);
  const auto& audit = first.audit;

  // Keys are emitted in lexicographic order so this one line is already the
  // canonical JSON representation required by the guarded GCP assembler.
  std::cout
      << "{\"adversarial_query_count\":" << adversarial_query_count
      << ",\"all_results_exact\":true"
      << ",\"backend\":\"" << morsehgp3d::gpu::exact_diametral_phi_backend
      << "\""
      << ",\"catalog_complete_claimed\":false"
      << ",\"component_exact_sign_qualification_passed\":true"
      << ",\"cuda_device\":" << audit.cuda_device
      << ",\"cuda_execution_performed\":true"
      << ",\"decision_digest_fnv1a\":" << digest
      << ",\"deployment_status\":\""
      << morsehgp3d::gpu::exact_diametral_phi_deployment_status << "\""
      << ",\"deterministic_random_query_count\":"
      << deterministic_random_query_count
      << ",\"deterministic_replay_passed\":true"
      << ",\"epsilon_used\":false"
      << ",\"interval_zero_certification_forbidden\":true"
      << ",\"mode\":\"" << morsehgp3d::gpu::exact_diametral_phi_mode
      << "\""
      << ",\"near_shell_query_count\":" << near_shell_query_count
      << ",\"per_run_cpu_multiprecision_count\":"
      << audit.cpu_multiprecision_count
      << ",\"per_run_cuda_kernel_launch_count\":1"
      << ",\"per_run_cuda_synchronization_count\":1"
      << ",\"per_run_disagreement_count\":" << audit.disagreement_count
      << ",\"per_run_exact_zero_count\":" << audit.exact_zero_count
      << ",\"per_run_gpu_exact_fallback_count\":"
      << audit.gpu_exact_fallback_count
      << ",\"per_run_gpu_fixed_limb_dyadic_count\":"
      << audit.gpu_fixed_limb_dyadic_count
      << ",\"per_run_gpu_fp64_interval_count\":"
      << audit.gpu_fp64_interval_count
      << ",\"per_run_gpu_known_audited_count\":"
      << audit.gpu_known_audited_count
      << ",\"per_run_launcher_call_count\":1"
      << ",\"per_run_query_count\":" << audit.input_count
      << ",\"phase\":\"15\""
      << ",\"profile\":\"" << morsehgp3d::gpu::exact_diametral_phi_profile
      << "\""
      << ",\"public_status\":\""
      << morsehgp3d::gpu::exact_diametral_phi_public_status << "\""
      << ",\"public_status_claimed\":false"
      << ",\"run_count\":2"
      << ",\"scalability_claimed\":false"
      << ",\"schema\":\"morsehgp3d.phase15.exact_diametral_phi_qualification.v2\""
      << ",\"scientific_catalog_decision_published\":false"
      << ",\"symmetry_query_count\":" << symmetry_query_count
      << ",\"total_cuda_kernel_launch_count\":2"
      << ",\"total_cuda_synchronization_count\":2"
      << ",\"total_evaluated_query_count\":"
      << 2U * audit.input_count
      << ",\"total_launcher_call_count\":2}\n";
  return 0;
}

}  // namespace

int main() {
  try {
    return run();
  } catch (const std::exception& error) {
    std::cerr << "exact diametral-Phi qualification failed: " << error.what()
              << '\n';
    return 1;
  }
}
