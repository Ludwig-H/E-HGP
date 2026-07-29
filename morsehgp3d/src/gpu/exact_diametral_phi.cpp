#include "morsehgp3d/gpu/exact_diametral_phi.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/rational.hpp"
#include "phase15_exact_diametral_phi_internal.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

class PostGpuPublicationGuard final {
 public:
  explicit PostGpuPublicationGuard(
      detail::ExactDiametralPhiContextState& context) noexcept
      : context_(context) {}

  PostGpuPublicationGuard(const PostGpuPublicationGuard&) = delete;
  PostGpuPublicationGuard& operator=(const PostGpuPublicationGuard&) = delete;

  ~PostGpuPublicationGuard() {
    if (armed_) {
      context_.mark_poisoned();
    }
  }

  void arm() noexcept { armed_ = true; }
  void release() noexcept { armed_ = false; }

 private:
  detail::ExactDiametralPhiContextState& context_;
  bool armed_{false};
};

void require_finite_words(
    const std::array<std::uint64_t, 3U>& words,
    const char* label) {
  for (const std::uint64_t word : words) {
    if (!exact::is_finite_binary64_bits(word)) {
      throw std::invalid_argument(
          std::string{"exact diametral-Phi "} + label +
          " coordinates must be finite binary64 words");
    }
  }
}

void validate_inputs(
    std::span<const ExactDiametralPhiInput> inputs,
    std::size_t maximum_query_count) {
  if (inputs.size() > maximum_query_count) {
    throw std::length_error(
        "the exact diametral-Phi batch exceeds its fixed context capacity");
  }
  std::unordered_set<std::uint64_t> replay_ids;
  replay_ids.reserve(inputs.size());
  for (const ExactDiametralPhiInput& input : inputs) {
    if (!replay_ids.insert(input.replay_id).second) {
      throw std::invalid_argument(
          "exact diametral-Phi replay identifiers must be unique per batch");
    }
    require_finite_words(input.witness_bits, "witness");
    require_finite_words(input.first_support_bits, "first-support");
    require_finite_words(input.second_support_bits, "second-support");
  }
}

[[nodiscard]] exact::PredicateSign cpu_exact_sign(
    const ExactDiametralPhiInput& input) {
  exact::ExactRational phi;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational witness =
        exact::ExactRational::from_binary64_bits(input.witness_bits[axis]);
    const exact::ExactRational first =
        exact::ExactRational::from_binary64_bits(
            input.first_support_bits[axis]);
    const exact::ExactRational second =
        exact::ExactRational::from_binary64_bits(
            input.second_support_bits[axis]);
    phi = phi + (witness - first) * (witness - second);
  }
  return exact::predicate_sign(phi.sign());
}

[[nodiscard]] exact::PredicateSign decode_device_sign(
    detail::ExactDiametralPhiDeviceSign sign) {
  switch (sign) {
    case detail::ExactDiametralPhiDeviceSign::negative:
      return exact::PredicateSign::negative;
    case detail::ExactDiametralPhiDeviceSign::zero:
      return exact::PredicateSign::zero;
    case detail::ExactDiametralPhiDeviceSign::positive:
      return exact::PredicateSign::positive;
    case detail::ExactDiametralPhiDeviceSign::unresolved:
    case detail::ExactDiametralPhiDeviceSign::invalid:
      break;
  }
  throw std::runtime_error(
      "an unresolved exact diametral-Phi device sign was promoted");
}

void validate_batch_metadata(
    const detail::ExactDiametralPhiDeviceBatch& batch,
    std::size_t input_count,
    std::size_t maximum_query_count) {
  if (batch.records.size() != input_count ||
      batch.maximum_query_count != maximum_query_count ||
      batch.launcher_call_count != 1U || !batch.exact_fixed_limb_implemented ||
      batch.epsilon_used) {
    throw std::runtime_error(
        "the exact diametral-Phi launcher returned inconsistent metadata");
  }
  switch (batch.execution_kind) {
    case detail::ExactDiametralPhiExecutionKind::host_fake:
      if (batch.cuda_device != -1 || batch.kernel_launch_count != 0U ||
          batch.synchronization_count != 0U) {
        throw std::runtime_error(
            "the exact diametral-Phi fake launcher forged CUDA execution");
      }
      break;
    case detail::ExactDiametralPhiExecutionKind::cuda:
      if (batch.cuda_device < 0 || batch.kernel_launch_count != 1U ||
          batch.synchronization_count != 1U) {
        throw std::runtime_error(
            "the exact diametral-Phi CUDA launcher metadata is incomplete");
      }
      break;
    default:
      throw std::runtime_error(
          "the exact diametral-Phi launcher returned an invalid backend");
  }
}

}  // namespace

ExactDiametralPhiContext::ExactDiametralPhiContext(
    std::size_t maximum_query_count)
    : state_(std::make_shared<detail::ExactDiametralPhiContextState>()),
      maximum_query_count_(maximum_query_count) {
  if (maximum_query_count_ == 0U) {
    throw std::invalid_argument(
        "the exact diametral-Phi context capacity must be positive");
  }
  if (maximum_query_count_ >
      std::numeric_limits<std::size_t>::max() /
          sizeof(ExactDiametralPhiInput)) {
    throw std::length_error(
        "the exact diametral-Phi context capacity overflows size_t");
  }
}

ExactDiametralPhiContext::~ExactDiametralPhiContext() noexcept = default;
ExactDiametralPhiContext::ExactDiametralPhiContext(
    ExactDiametralPhiContext&&) noexcept = default;
ExactDiametralPhiContext& ExactDiametralPhiContext::operator=(
    ExactDiametralPhiContext&&) noexcept = default;

ExactDiametralPhiBatchResult ExactDiametralPhiContext::decide(
    std::span<const ExactDiametralPhiInput> inputs,
    ExactDiametralPhiBatchOptions options) {
  if (!state_) {
    throw std::logic_error(
        "a moved-from exact diametral-Phi context cannot decide a batch");
  }
  validate_inputs(inputs, maximum_query_count_);
  // The lock covers launch, metadata validation, qualification replay and the
  // complete CPU fallback queue. No concurrent call can publish after another
  // call has poisoned this context.
  return state_->with_gpu_section([&]() -> ExactDiametralPhiBatchResult {
    ExactDiametralPhiBatchResult result;
    result.audit.input_count = inputs.size();
    result.audit.maximum_query_count = maximum_query_count_;
    if (inputs.empty()) {
      result.audit.all_results_exact = true;
      return result;
    }

    PostGpuPublicationGuard publication_guard{*state_};
    const detail::ExactDiametralPhiDeviceBatch batch =
        detail::decide_exact_diametral_phi_on_gpu(
            *state_, inputs, maximum_query_count_);
    publication_guard.arm();
    validate_batch_metadata(batch, inputs.size(), maximum_query_count_);

    result.decisions.resize(inputs.size());
    std::vector<std::size_t> fallback_indices;
    fallback_indices.reserve(inputs.size());
    for (std::size_t index = 0U; index < inputs.size(); ++index) {
      const detail::ExactDiametralPhiDeviceRecord& record = batch.records[index];
      if (record.query_index != index) {
        throw std::runtime_error(
            "the exact diametral-Phi device record order changed");
      }
      for (const std::uint8_t byte : record.reserved) {
        if (byte != 0U) {
          throw std::runtime_error(
              "the exact diametral-Phi device record reserved bytes changed");
        }
      }
      switch (record.stage) {
        case detail::ExactDiametralPhiDeviceStage::fp64_interval:
          if (record.sign != detail::ExactDiametralPhiDeviceSign::negative &&
              record.sign != detail::ExactDiametralPhiDeviceSign::positive) {
            throw std::runtime_error(
                "an interval attempted to certify zero or uncertainty");
          }
          ++result.audit.gpu_fp64_interval_count;
          result.decisions[index] = ExactDiametralPhiDecision{
              inputs[index].replay_id,
              decode_device_sign(record.sign),
              ExactDiametralPhiCertificationStage::gpu_fp64_interval};
          break;
        case detail::ExactDiametralPhiDeviceStage::fixed_limb_dyadic:
          if (record.sign != detail::ExactDiametralPhiDeviceSign::negative &&
              record.sign != detail::ExactDiametralPhiDeviceSign::zero &&
              record.sign != detail::ExactDiametralPhiDeviceSign::positive) {
            throw std::runtime_error(
                "the fixed-limb exact stage returned an invalid sign");
          }
          ++result.audit.gpu_fixed_limb_dyadic_count;
          result.decisions[index] = ExactDiametralPhiDecision{
              inputs[index].replay_id,
              decode_device_sign(record.sign),
              ExactDiametralPhiCertificationStage::gpu_fixed_limb_dyadic};
          break;
        case detail::ExactDiametralPhiDeviceStage::requires_cpu_fallback:
          if (record.sign != detail::ExactDiametralPhiDeviceSign::unresolved) {
            throw std::runtime_error(
                "the exact diametral-Phi fallback carried a guessed sign");
          }
          ++result.audit.gpu_exact_fallback_count;
          fallback_indices.push_back(index);
          break;
        case detail::ExactDiametralPhiDeviceStage::invalid:
        default:
          throw std::runtime_error(
              "the exact diametral-Phi launcher returned an invalid record");
      }
    }

    if (options.audit_gpu_signs) {
      for (std::size_t index = 0U; index < inputs.size(); ++index) {
        if (batch.records[index].stage ==
            detail::ExactDiametralPhiDeviceStage::requires_cpu_fallback) {
          continue;
        }
        const exact::PredicateSign oracle = cpu_exact_sign(inputs[index]);
        if (oracle != result.decisions[index].sign) {
          ++result.audit.disagreement_count;
          throw std::runtime_error(
              "the exact diametral-Phi GPU sign contradicted BigInt");
        }
        ++result.audit.gpu_known_audited_count;
      }
    }

    // This is one compact fallback queue, not a callback from each GPU thread.
    for (const std::size_t index : fallback_indices) {
      result.decisions[index] = ExactDiametralPhiDecision{
          inputs[index].replay_id,
          cpu_exact_sign(inputs[index]),
          ExactDiametralPhiCertificationStage::cpu_multiprecision};
      ++result.audit.cpu_multiprecision_count;
    }

    for (const ExactDiametralPhiDecision& decision : result.decisions) {
      if (decision.sign == exact::PredicateSign::zero) {
        ++result.audit.exact_zero_count;
      }
    }
    if (result.audit.gpu_fp64_interval_count +
            result.audit.gpu_fixed_limb_dyadic_count +
            result.audit.gpu_exact_fallback_count !=
        inputs.size()) {
      throw std::runtime_error(
          "the exact diametral-Phi stage partition does not close");
    }
    if (result.audit.gpu_exact_fallback_count !=
        result.audit.cpu_multiprecision_count) {
      throw std::runtime_error(
          "the exact diametral-Phi fallback queue did not close");
    }

    result.audit.launcher_call_count = batch.launcher_call_count;
    result.audit.cuda_kernel_launch_count = batch.kernel_launch_count;
    result.audit.cuda_synchronization_count = batch.synchronization_count;
    result.audit.cuda_device = batch.cuda_device;
    result.audit.host_fake_launcher_exercised =
        batch.execution_kind ==
        detail::ExactDiametralPhiExecutionKind::host_fake;
    result.audit.cuda_execution_performed =
        batch.execution_kind == detail::ExactDiametralPhiExecutionKind::cuda;
    result.audit.all_results_exact = true;
    publication_guard.release();
    return result;
  });
}

}  // namespace morsehgp3d::gpu
