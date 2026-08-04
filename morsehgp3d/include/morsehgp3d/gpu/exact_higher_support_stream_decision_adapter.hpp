#pragma once

#include "morsehgp3d/gpu/exact_higher_support_product_cuda.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <limits>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace morsehgp3d::gpu {

// Audit local to the accelerator seam.  These counters are deliberately not
// mixed into the scientific stream audit: a CPU-only replay and an
// accelerator-assisted run must produce the same authenticated stream.
struct ExactHigherSupportProductCudaPositiveDecisionAdapterAudit {
  std::size_t submitted_task_count{};
  std::size_t prefetched_task_count{};
  std::size_t on_demand_task_count{};
  std::size_t support_prune_task_count{};
  std::size_t query_strict_interior_task_count{};
  std::size_t terminal_support_geometry_task_count{};
  std::size_t prefetch_call_count{};
  std::size_t support_frontier_prefetch_call_count{};
  std::size_t query_plan_prefetch_call_count{};
  std::size_t terminal_geometry_prefetch_call_count{};
  std::size_t prefetch_requested_task_count{};
  std::size_t prefetch_already_cached_task_count{};
  std::size_t evaluate_call_count{};
  std::size_t synchronization_count{};
  std::size_t maximum_batch_size{};
  std::size_t cache_hit_count{};
  std::size_t cache_miss_count{};
  std::size_t cache_eviction_count{};
  std::size_t cache_capacity{};
  std::size_t cache_entry_count{};
  std::size_t maximum_cache_entry_count{};
  std::size_t certified_positive_count{};
  std::size_t fail_open_count{};
  std::size_t invalid_result_fail_open_count{};
  std::size_t exception_fail_open_count{};
  std::size_t disabled_source_fallback_count{};
  std::size_t bounded_dyadic_int256_count{};
  std::size_t bounded_dyadic_int512_count{};
  std::size_t bounded_dyadic_int1024_count{};
  std::size_t arbitrary_precision_rational_count{};
  std::size_t component_cpu_fallback_count{};
  std::size_t native_certified_positive_count{};
  std::size_t native_kernel_certified_positive_count{};
  std::size_t native_component_cpu_fallback_certified_positive_count{};
  std::size_t host_fake_positive_proposal_count{};
  std::size_t support_size_3_certified_positive_count{};
  std::size_t support_size_4_certified_positive_count{};
  std::size_t support_prune_certified_positive_count{};
  std::size_t query_strict_interior_certified_positive_count{};
  std::size_t terminal_geometry_decision_count{};
  std::size_t terminal_affinely_dependent_count{};
  std::size_t terminal_boundary_reduced_count{};
  std::size_t terminal_exterior_circumcenter_count{};
  std::size_t terminal_minimal_count{};
  std::size_t terminal_geometry_host_fake_decision_count{};
  std::size_t terminal_geometry_native_rejection_count{};
  bool source_binding_validated{false};
  bool native_exact_authority{false};
  bool host_fake_positive_proposals_require_cpu_replay{false};
  bool disabled_after_failure{false};
  bool terminal_geometry_decision_native_cuda{
      exact_higher_support_terminal_geometry_native_cuda};
  bool floating_point_decision_performed{false};
  bool global_product_frontier_mutated{false};
  bool higher_order_delaunay_materialized{false};
  bool hierarchy_or_public_status_claimed{false};

  friend bool operator==(
      const ExactHigherSupportProductCudaPositiveDecisionAdapterAudit&,
      const ExactHigherSupportProductCudaPositiveDecisionAdapterAudit&) =
      default;
};

// Bridges the native exact P6a predicates into the sparse higher-support
// traversal.  It owns no cloud, index or CUDA context: all three authorities
// must remain at stable addresses without being moved from, and they and this
// non-movable adapter must outlive every stream/session using source().
// A launcher error disables the seam and fails open to the CPU decision DAG.
class ExactHigherSupportProductCudaPositiveDecisionAdapter final {
 public:
  ExactHigherSupportProductCudaPositiveDecisionAdapter(
      ExactHigherSupportProductCudaContext& context,
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud)
      : context_(&context),
        index_(&index),
        cloud_(&cloud),
        native_exact_authority_(!context.host_fake()),
        terminal_geometry_source_(
            this,
            &bound_to_callback,
            &terminal_geometry_native_exact_authority_callback,
            &terminal_geometry_maximum_prefetch_task_count_callback,
            &prefetch_terminal_geometry_callback,
            &decide_terminal_geometry_callback),
        source_(
            this,
            &bound_to_callback,
            &native_exact_authority_callback,
            &maximum_prefetch_task_count_callback,
            &prefetch_no_well_centered_supports_callback,
            &prefetch_query_strictly_inside_callback,
            &certify_no_well_centered_support_callback,
            &certify_query_strictly_inside_callback,
            &terminal_geometry_source_) {
    if (context.maximum_task_count() == 0U ||
        context.source_snapshot_epoch() == 0U ||
        !context.bound_to(index, cloud)) {
      throw std::invalid_argument(
          "the exact higher-support CUDA adapter requires a matching live "
          "context");
    }
    audit_.source_binding_validated = true;
    audit_.native_exact_authority = native_exact_authority_;
    audit_.host_fake_positive_proposals_require_cpu_replay =
        !native_exact_authority_;
    const std::size_t local_plan_capacity = hierarchy::
        higher_support_local_rank_probe_maximum_evaluation_count;
    cache_capacity_ = context.maximum_task_count() >
            std::numeric_limits<std::size_t>::max() - local_plan_capacity
        ? std::numeric_limits<std::size_t>::max()
        : context.maximum_task_count() + local_plan_capacity;
    audit_.cache_capacity = cache_capacity_;
  }

  ExactHigherSupportProductCudaPositiveDecisionAdapter(
      const ExactHigherSupportProductCudaPositiveDecisionAdapter&) = delete;
  ExactHigherSupportProductCudaPositiveDecisionAdapter& operator=(
      const ExactHigherSupportProductCudaPositiveDecisionAdapter&) = delete;
  ExactHigherSupportProductCudaPositiveDecisionAdapter(
      ExactHigherSupportProductCudaPositiveDecisionAdapter&&) = delete;
  ExactHigherSupportProductCudaPositiveDecisionAdapter& operator=(
      ExactHigherSupportProductCudaPositiveDecisionAdapter&&) = delete;

  [[nodiscard]] const hierarchy::ExactHigherSupportPositiveDecisionSource&
  source() & noexcept {
    return source_;
  }
  const hierarchy::ExactHigherSupportPositiveDecisionSource&
  source() const & = delete;
  const hierarchy::ExactHigherSupportPositiveDecisionSource&
  source() && = delete;
  const hierarchy::ExactHigherSupportPositiveDecisionSource&
  source() const && = delete;

  [[nodiscard]] ExactHigherSupportProductCudaPositiveDecisionAdapterAudit
  audit() const {
    const std::lock_guard<std::mutex> lock{mutex_};
    return audit_;
  }

 private:
  [[nodiscard]] static bool bound_to_callback(
      void* state,
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) noexcept {
    const auto* self = static_cast<
        ExactHigherSupportProductCudaPositiveDecisionAdapter*>(state);
    return self != nullptr && self->context_ != nullptr &&
        self->index_ == &index && self->cloud_ == &cloud &&
        self->context_->bound_to(index, cloud);
  }

  [[nodiscard]] static bool native_exact_authority_callback(
      void* state) noexcept {
    const auto* self = static_cast<
        ExactHigherSupportProductCudaPositiveDecisionAdapter*>(state);
    if (self == nullptr) {
      return false;
    }
    const std::lock_guard<std::mutex> lock{self->mutex_};
    return self->native_exact_authority_ && !self->disabled_;
  }

  // Schema 4 is deliberately host-first for the categorical classifier.
  // A native context retains authority for the two already-qualified
  // positive predicates, but can never authorize terminal geometry.
  [[nodiscard]] static bool
  terminal_geometry_native_exact_authority_callback(void*) noexcept {
    return false;
  }

  [[nodiscard]] static std::size_t maximum_prefetch_task_count_callback(
      void* state) noexcept {
    const auto* self = static_cast<
        ExactHigherSupportProductCudaPositiveDecisionAdapter*>(state);
    if (self == nullptr) {
      return 0U;
    }
    const std::lock_guard<std::mutex> lock{self->mutex_};
    return !self->disabled_ && self->context_ != nullptr
        ? self->context_->maximum_task_count()
        : 0U;
  }

  [[nodiscard]] static std::size_t
  terminal_geometry_maximum_prefetch_task_count_callback(
      void* state) noexcept {
    const auto* self = static_cast<
        ExactHigherSupportProductCudaPositiveDecisionAdapter*>(state);
    if (self == nullptr) {
      return 0U;
    }
    const std::lock_guard<std::mutex> lock{self->mutex_};
    return !self->disabled_ && !self->native_exact_authority_ &&
            self->context_ != nullptr
        ? self->context_->maximum_task_count()
        : 0U;
  }

  static void prefetch_no_well_centered_supports_callback(
      void* state,
      std::span<const hierarchy::ExactHigherSupportFrontierEntry> products)
      noexcept {
    auto* self = static_cast<
        ExactHigherSupportProductCudaPositiveDecisionAdapter*>(state);
    if (self != nullptr) {
      self->prefetch_support_products(products);
    }
  }

  static void prefetch_query_strictly_inside_callback(
      void* state,
      std::span<const hierarchy::ExactHigherSupportQueryDecisionRequest>
          requests) noexcept {
    auto* self = static_cast<
        ExactHigherSupportProductCudaPositiveDecisionAdapter*>(state);
    if (self != nullptr) {
      self->prefetch_query_products(requests);
    }
  }

  static void prefetch_terminal_geometry_callback(
      void* state,
      std::span<const hierarchy::ExactHigherSupportFrontierEntry> products)
      noexcept {
    auto* self = static_cast<
        ExactHigherSupportProductCudaPositiveDecisionAdapter*>(state);
    if (self != nullptr) {
      self->prefetch_terminal_products(products);
    }
  }

  [[nodiscard]] static bool certify_no_well_centered_support_callback(
      void* state,
      const hierarchy::ExactHigherSupportFrontierEntry& product) {
    auto* self = static_cast<
        ExactHigherSupportProductCudaPositiveDecisionAdapter*>(state);
    return self != nullptr && self->evaluate_positive(
        ExactHigherSupportProductCudaTaskKind::support_prune,
        product,
        nullptr);
  }

  [[nodiscard]] static bool certify_query_strictly_inside_callback(
      void* state,
      const hierarchy::ExactHigherSupportFrontierEntry& product,
      const hierarchy::ExactHigherSupportNodeReceipt& query_node) {
    auto* self = static_cast<
        ExactHigherSupportProductCudaPositiveDecisionAdapter*>(state);
    return self != nullptr && self->evaluate_positive(
        ExactHigherSupportProductCudaTaskKind::query_strict_interior,
        product,
        &query_node);
  }

  [[nodiscard]] static
  std::optional<hierarchy::ExactHigherSupportTerminalGeometryDecision>
  decide_terminal_geometry_callback(
      void* state,
      const hierarchy::ExactHigherSupportFrontierEntry& product) noexcept {
    auto* self = static_cast<
        ExactHigherSupportProductCudaPositiveDecisionAdapter*>(state);
    return self != nullptr
        ? self->evaluate_terminal_geometry(product)
        : std::nullopt;
  }

  [[nodiscard]] bool evaluate_positive(
      ExactHigherSupportProductCudaTaskKind kind,
      const hierarchy::ExactHigherSupportFrontierEntry& product,
      const hierarchy::ExactHigherSupportNodeReceipt* query_node) noexcept {
    try {
      const std::lock_guard<std::mutex> lock{mutex_};
      InternalRequest request;
      request.kind = kind;
      request.product = product;
      if (query_node != nullptr) {
        request.query_node = *query_node;
      }
      if (!live_source_locked()) {
        ++audit_.disabled_source_fallback_count;
        return false;
      }
      if (const CachedDecision* cached = find_cached_locked(request)) {
        ++audit_.cache_hit_count;
        return cached->record.outcome ==
            ExactHigherSupportProductCudaOutcome::certified;
      }
      ++audit_.cache_miss_count;
      if (!evaluate_requests_locked(
              std::span<const InternalRequest>{&request, 1U}, false)) {
        return false;
      }
      const CachedDecision* cached = find_cached_locked(request);
      if (cached == nullptr) {
        ++audit_.invalid_result_fail_open_count;
        disable_after_failure();
        return false;
      }
      return cached->record.outcome ==
          ExactHigherSupportProductCudaOutcome::certified;
    } catch (...) {
      fail_open_after_callback_exception();
      return false;
    }
  }

  [[nodiscard]]
  std::optional<hierarchy::ExactHigherSupportTerminalGeometryDecision>
  evaluate_terminal_geometry(
      const hierarchy::ExactHigherSupportFrontierEntry& product) noexcept {
    try {
      const std::lock_guard<std::mutex> lock{mutex_};
      if (native_exact_authority_) {
        ++audit_.terminal_geometry_native_rejection_count;
        return std::nullopt;
      }
      InternalRequest request;
      request.kind =
          ExactHigherSupportProductCudaTaskKind::terminal_support_geometry;
      request.product = product;
      if (!live_source_locked()) {
        ++audit_.disabled_source_fallback_count;
        return std::nullopt;
      }
      if (const CachedDecision* cached = find_cached_locked(request)) {
        ++audit_.cache_hit_count;
        return cached->record.terminal_geometry_decision;
      }
      ++audit_.cache_miss_count;
      if (!evaluate_requests_locked(
              std::span<const InternalRequest>{&request, 1U}, false)) {
        return std::nullopt;
      }
      const CachedDecision* cached = find_cached_locked(request);
      if (cached == nullptr ||
          !cached->record.terminal_geometry_decision.has_value()) {
        ++audit_.invalid_result_fail_open_count;
        disable_after_failure();
        return std::nullopt;
      }
      return cached->record.terminal_geometry_decision;
    } catch (...) {
      fail_open_after_callback_exception();
      return std::nullopt;
    }
  }

  struct InternalRequest {
    ExactHigherSupportProductCudaTaskKind kind{
        ExactHigherSupportProductCudaTaskKind::support_prune};
    hierarchy::ExactHigherSupportFrontierEntry product{};
    std::optional<hierarchy::ExactHigherSupportNodeReceipt> query_node;

    friend bool operator==(
        const InternalRequest&, const InternalRequest&) = default;
  };

  struct CachedDecision {
    InternalRequest request{};
    ExactHigherSupportProductCudaRecord record{};
  };

  void prefetch_support_products(
      std::span<const hierarchy::ExactHigherSupportFrontierEntry> products)
      noexcept {
    try {
      const std::lock_guard<std::mutex> lock{mutex_};
      ++audit_.prefetch_call_count;
      ++audit_.support_frontier_prefetch_call_count;
      audit_.prefetch_requested_task_count += products.size();
      if (!live_source_locked()) {
        ++audit_.disabled_source_fallback_count;
        return;
      }
      std::vector<InternalRequest> requests;
      requests.reserve(products.size());
      for (const auto& product : products) {
        InternalRequest request;
        request.kind = ExactHigherSupportProductCudaTaskKind::support_prune;
        request.product = product;
        append_prefetch_miss_locked(requests, request);
      }
      static_cast<void>(evaluate_requests_locked(requests, true));
    } catch (...) {
      fail_open_after_callback_exception();
    }
  }

  void prefetch_query_products(
      std::span<const hierarchy::ExactHigherSupportQueryDecisionRequest>
          query_requests) noexcept {
    try {
      const std::lock_guard<std::mutex> lock{mutex_};
      ++audit_.prefetch_call_count;
      ++audit_.query_plan_prefetch_call_count;
      audit_.prefetch_requested_task_count += query_requests.size();
      if (!live_source_locked()) {
        ++audit_.disabled_source_fallback_count;
        return;
      }
      std::vector<InternalRequest> requests;
      requests.reserve(query_requests.size());
      for (const auto& query_request : query_requests) {
        InternalRequest request;
        request.kind =
            ExactHigherSupportProductCudaTaskKind::query_strict_interior;
        request.product = query_request.product;
        request.query_node = query_request.query_node;
        append_prefetch_miss_locked(requests, request);
      }
      static_cast<void>(evaluate_requests_locked(requests, true));
    } catch (...) {
      fail_open_after_callback_exception();
    }
  }

  void prefetch_terminal_products(
      std::span<const hierarchy::ExactHigherSupportFrontierEntry> products)
      noexcept {
    try {
      const std::lock_guard<std::mutex> lock{mutex_};
      ++audit_.prefetch_call_count;
      ++audit_.terminal_geometry_prefetch_call_count;
      audit_.prefetch_requested_task_count += products.size();
      if (native_exact_authority_) {
        audit_.terminal_geometry_native_rejection_count += products.size();
        return;
      }
      if (!live_source_locked()) {
        ++audit_.disabled_source_fallback_count;
        return;
      }
      std::vector<InternalRequest> requests;
      requests.reserve(products.size());
      for (const auto& product : products) {
        InternalRequest request;
        request.kind = ExactHigherSupportProductCudaTaskKind::
            terminal_support_geometry;
        request.product = product;
        append_prefetch_miss_locked(requests, request);
      }
      static_cast<void>(evaluate_requests_locked(requests, true));
    } catch (...) {
      fail_open_after_callback_exception();
    }
  }

  [[nodiscard]] bool live_source_locked() noexcept {
    if (disabled_) {
      return false;
    }
    if (context_ == nullptr || index_ == nullptr || cloud_ == nullptr ||
        !context_->bound_to(*index_, *cloud_)) {
      disable_after_failure();
      return false;
    }
    return true;
  }

  [[nodiscard]] const CachedDecision* find_cached_locked(
      const InternalRequest& request) const noexcept {
    const auto found = std::find_if(
        cache_.rbegin(),
        cache_.rend(),
        [&request](const CachedDecision& cached) {
          return cached.request == request;
        });
    return found == cache_.rend() ? nullptr : &*found;
  }

  void append_prefetch_miss_locked(
      std::vector<InternalRequest>& requests,
      const InternalRequest& request) {
    const auto cached = std::find_if(
        cache_.begin(),
        cache_.end(),
        [&request](const CachedDecision& decision) {
          return decision.request == request;
        });
    if (cached != cache_.end()) {
      // Keep the just-requested sparse Morton slice resident while new child
      // products enter the bounded cache.
      std::rotate(cached, std::next(cached), cache_.end());
      ++audit_.prefetch_already_cached_task_count;
      return;
    }
    if (std::find(requests.begin(), requests.end(), request) !=
        requests.end()) {
      ++audit_.prefetch_already_cached_task_count;
      return;
    }
    requests.push_back(request);
  }

  [[nodiscard]] bool evaluate_requests_locked(
      std::span<const InternalRequest> requests,
      bool prefetched) {
    if (requests.empty()) {
      return true;
    }
    if (!live_source_locked()) {
      ++audit_.disabled_source_fallback_count;
      return false;
    }
    const std::size_t maximum_batch_size = context_->maximum_task_count();
    if (maximum_batch_size == 0U) {
      ++audit_.invalid_result_fail_open_count;
      disable_after_failure();
      return false;
    }
    std::size_t begin = 0U;
    while (begin < requests.size()) {
      const std::size_t batch_size =
          std::min(maximum_batch_size, requests.size() - begin);
      if (batch_size >
          std::numeric_limits<std::uint64_t>::max() - next_task_id_) {
        ++audit_.invalid_result_fail_open_count;
        disable_after_failure();
        return false;
      }
      std::vector<ExactHigherSupportProductCudaTask> tasks;
      tasks.reserve(batch_size);
      for (std::size_t offset = 0U; offset < batch_size; ++offset) {
        const InternalRequest& request = requests[begin + offset];
        ExactHigherSupportProductCudaTask task;
        task.task_id = next_task_id_++;
        task.source_snapshot_epoch = context_->source_snapshot_epoch();
        task.kind = request.kind;
        task.product = request.product;
        task.query_node = request.query_node;
        tasks.push_back(std::move(task));
      }
      audit_.submitted_task_count += batch_size;
      if (prefetched) {
        audit_.prefetched_task_count += batch_size;
      } else {
        audit_.on_demand_task_count += batch_size;
      }
      for (const ExactHigherSupportProductCudaTask& task : tasks) {
        switch (task.kind) {
          case ExactHigherSupportProductCudaTaskKind::support_prune:
            ++audit_.support_prune_task_count;
            break;
          case ExactHigherSupportProductCudaTaskKind::query_strict_interior:
            ++audit_.query_strict_interior_task_count;
            break;
          case ExactHigherSupportProductCudaTaskKind::
              terminal_support_geometry:
            ++audit_.terminal_support_geometry_task_count;
            break;
        }
      }
      ++audit_.evaluate_call_count;
      audit_.maximum_batch_size = std::max(
          audit_.maximum_batch_size, batch_size);

      ExactHigherSupportProductCudaResult result;
      try {
        result = context_->evaluate(tasks);
      } catch (...) {
        ++audit_.exception_fail_open_count;
        disable_after_failure();
        return false;
      }
      if (!valid_batch_result(tasks, result)) {
        ++audit_.invalid_result_fail_open_count;
        disable_after_failure();
        return false;
      }
      audit_.synchronization_count += result.audit.synchronization_count;
      for (std::size_t offset = 0U; offset < batch_size; ++offset) {
        const ExactHigherSupportProductCudaRecord& record =
            result.records[offset];
        account_record(requests[begin + offset], record);
        insert_cached_locked(requests[begin + offset], record);
      }
      begin += batch_size;
    }
    return true;
  }

  [[nodiscard]] bool valid_batch_result(
      std::span<const ExactHigherSupportProductCudaTask> tasks,
      const ExactHigherSupportProductCudaResult& result) const noexcept {
    if (!result.complete() || result.records.size() != tasks.size() ||
        result.audit.schema_version !=
            exact_higher_support_product_cuda_schema_version ||
        result.audit.submitted_task_count != tasks.size() ||
        result.audit.completed_task_count != tasks.size() ||
        !result.audit.source_authority_validated ||
        !result.audit.resident_lease_index_identity_validated ||
        !result.audit.every_task_validated_before_launch ||
        !result.audit.every_result_validated_once ||
        !result.audit.output_published_atomically ||
        result.audit.floating_point_decision_performed ||
        result.audit.bigint_support_mass_transferred ||
        result.audit.global_product_frontier_mutated ||
        result.audit.ordinary_or_higher_order_delaunay_materialized ||
        result.audit.global_cell_coface_or_incidence_arena_materialized ||
        result.audit.hierarchy_or_tree_claimed ||
        result.audit.slo_claimed ||
        result.audit.public_status_claimed ||
        result.audit.terminal_geometry_decision_native_cuda) {
      return false;
    }
    std::size_t expected_support_task_count = 0U;
    std::size_t expected_query_task_count = 0U;
    std::size_t expected_terminal_task_count = 0U;
    for (const ExactHigherSupportProductCudaTask& task : tasks) {
      switch (task.kind) {
        case ExactHigherSupportProductCudaTaskKind::support_prune:
          ++expected_support_task_count;
          break;
        case ExactHigherSupportProductCudaTaskKind::query_strict_interior:
          ++expected_query_task_count;
          break;
        case ExactHigherSupportProductCudaTaskKind::
            terminal_support_geometry:
          ++expected_terminal_task_count;
          break;
      }
    }
    if (result.audit.support_prune_task_count !=
            expected_support_task_count ||
        result.audit.query_strict_interior_task_count !=
            expected_query_task_count ||
        result.audit.terminal_support_geometry_task_count !=
            expected_terminal_task_count ||
        expected_support_task_count + expected_query_task_count +
                expected_terminal_task_count !=
            tasks.size() ||
        result.audit.certified_count + result.audit.fail_open_count !=
            tasks.size() ||
        result.audit.terminal_affinely_dependent_count +
                result.audit.terminal_boundary_reduced_count +
                result.audit.terminal_exterior_circumcenter_count +
                result.audit.terminal_minimal_count !=
            expected_terminal_task_count ||
        result.audit.bounded_dyadic_int256_count +
                result.audit.bounded_dyadic_int512_count +
                result.audit.bounded_dyadic_int1024_count +
                result.audit.arbitrary_precision_rational_fallback_count !=
            tasks.size()) {
      return false;
    }
    std::size_t expected_terminal_affinely_dependent_count = 0U;
    std::size_t expected_terminal_boundary_reduced_count = 0U;
    std::size_t expected_terminal_exterior_circumcenter_count = 0U;
    std::size_t expected_terminal_minimal_count = 0U;
    for (std::size_t index = 0U; index < tasks.size(); ++index) {
      if (result.records[index].task_id != tasks[index].task_id ||
          result.records[index].kind != tasks[index].kind) {
        return false;
      }
      const bool terminal = tasks[index].kind ==
          ExactHigherSupportProductCudaTaskKind::terminal_support_geometry;
      if (result.records[index].terminal_geometry_decision.has_value() !=
          terminal) {
        return false;
      }
      if (terminal) {
        if (result.records[index].outcome !=
            ExactHigherSupportProductCudaOutcome::certified) {
          return false;
        }
        switch (*result.records[index].terminal_geometry_decision) {
          case hierarchy::ExactHigherSupportTerminalGeometryDecision::
              affinely_dependent:
            ++expected_terminal_affinely_dependent_count;
            break;
          case hierarchy::ExactHigherSupportTerminalGeometryDecision::
              boundary_reduced:
            ++expected_terminal_boundary_reduced_count;
            break;
          case hierarchy::ExactHigherSupportTerminalGeometryDecision::
              exterior_circumcenter:
            ++expected_terminal_exterior_circumcenter_count;
            break;
          case hierarchy::ExactHigherSupportTerminalGeometryDecision::
              minimal:
            ++expected_terminal_minimal_count;
            break;
          default:
            return false;
        }
      }
      switch (result.records[index].outcome) {
        case ExactHigherSupportProductCudaOutcome::certified:
        case ExactHigherSupportProductCudaOutcome::fail_open:
          break;
        default:
          return false;
      }
      switch (result.records[index].backend) {
        case ExactHigherSupportProductCudaBackend::bounded_dyadic_int256:
        case ExactHigherSupportProductCudaBackend::bounded_dyadic_int512:
        case ExactHigherSupportProductCudaBackend::bounded_dyadic_int1024:
        case ExactHigherSupportProductCudaBackend::
            arbitrary_precision_rational:
          break;
        default:
          return false;
      }
    }
    return result.audit.terminal_affinely_dependent_count ==
            expected_terminal_affinely_dependent_count &&
        result.audit.terminal_boundary_reduced_count ==
            expected_terminal_boundary_reduced_count &&
        result.audit.terminal_exterior_circumcenter_count ==
            expected_terminal_exterior_circumcenter_count &&
        result.audit.terminal_minimal_count ==
            expected_terminal_minimal_count;
  }

  void account_record(
      const InternalRequest& request,
      const ExactHigherSupportProductCudaRecord& record) noexcept {
    account_backend(record);
    if (record.cpu_fallback_performed) {
      ++audit_.component_cpu_fallback_count;
    }
    if (request.kind == ExactHigherSupportProductCudaTaskKind::
            terminal_support_geometry) {
      if (record.outcome == ExactHigherSupportProductCudaOutcome::fail_open ||
          !record.terminal_geometry_decision.has_value()) {
        ++audit_.fail_open_count;
        return;
      }
      ++audit_.terminal_geometry_decision_count;
      ++audit_.terminal_geometry_host_fake_decision_count;
      switch (*record.terminal_geometry_decision) {
        case hierarchy::ExactHigherSupportTerminalGeometryDecision::
            affinely_dependent:
          ++audit_.terminal_affinely_dependent_count;
          break;
        case hierarchy::ExactHigherSupportTerminalGeometryDecision::
            boundary_reduced:
          ++audit_.terminal_boundary_reduced_count;
          break;
        case hierarchy::ExactHigherSupportTerminalGeometryDecision::
            exterior_circumcenter:
          ++audit_.terminal_exterior_circumcenter_count;
          break;
        case hierarchy::ExactHigherSupportTerminalGeometryDecision::minimal:
          ++audit_.terminal_minimal_count;
          break;
      }
      return;
    }
    if (record.outcome == ExactHigherSupportProductCudaOutcome::fail_open) {
      ++audit_.fail_open_count;
      return;
    }
    ++audit_.certified_positive_count;
    if (native_exact_authority_) {
      ++audit_.native_certified_positive_count;
      if (record.cpu_fallback_performed) {
        ++audit_.
            native_component_cpu_fallback_certified_positive_count;
      } else {
        ++audit_.native_kernel_certified_positive_count;
      }
    } else {
      ++audit_.host_fake_positive_proposal_count;
    }
    if (request.product.support_size == 3U) {
      ++audit_.support_size_3_certified_positive_count;
    } else if (request.product.support_size == 4U) {
      ++audit_.support_size_4_certified_positive_count;
    }
    if (request.kind ==
        ExactHigherSupportProductCudaTaskKind::support_prune) {
      ++audit_.support_prune_certified_positive_count;
    } else {
      ++audit_.query_strict_interior_certified_positive_count;
    }
  }

  void insert_cached_locked(
      const InternalRequest& request,
      const ExactHigherSupportProductCudaRecord& record) {
    if (find_cached_locked(request) != nullptr) {
      return;
    }
    if (cache_.size() >= cache_capacity_) {
      cache_.erase(cache_.begin());
      ++audit_.cache_eviction_count;
    }
    cache_.push_back(CachedDecision{request, record});
    audit_.cache_entry_count = cache_.size();
    audit_.maximum_cache_entry_count = std::max(
        audit_.maximum_cache_entry_count, cache_.size());
  }

  void account_backend(
      const ExactHigherSupportProductCudaRecord& record) noexcept {
    switch (record.backend) {
      case ExactHigherSupportProductCudaBackend::bounded_dyadic_int256:
        ++audit_.bounded_dyadic_int256_count;
        break;
      case ExactHigherSupportProductCudaBackend::bounded_dyadic_int512:
        ++audit_.bounded_dyadic_int512_count;
        break;
      case ExactHigherSupportProductCudaBackend::bounded_dyadic_int1024:
        ++audit_.bounded_dyadic_int1024_count;
        break;
      case ExactHigherSupportProductCudaBackend::
          arbitrary_precision_rational:
        ++audit_.arbitrary_precision_rational_count;
        break;
    }
  }

  void disable_after_failure() noexcept {
    disabled_ = true;
    cache_.clear();
    audit_.cache_entry_count = 0U;
    audit_.disabled_after_failure = true;
  }

  // Callback-scope failures unwind the original lock before arriving here.
  // Reacquiring it keeps concurrent audit reads and the poison transition
  // data-race free; a mutex failure itself cannot be reported safely, so this
  // last-resort path remains noexcept and simply withholds acceleration.
  void fail_open_after_callback_exception() noexcept {
    try {
      const std::lock_guard<std::mutex> lock{mutex_};
      ++audit_.exception_fail_open_count;
      disable_after_failure();
    } catch (...) {
      return;
    }
  }

  ExactHigherSupportProductCudaContext* context_{};
  const spatial::MortonLbvhIndex* index_{};
  const spatial::CanonicalPointCloud* cloud_{};
  bool native_exact_authority_{false};
  bool disabled_{false};
  std::uint64_t next_task_id_{1U};
  std::size_t cache_capacity_{1U};
  std::vector<CachedDecision> cache_;
  mutable std::mutex mutex_;
  ExactHigherSupportProductCudaPositiveDecisionAdapterAudit audit_{};
  hierarchy::ExactHigherSupportTerminalGeometryDecisionSource
      terminal_geometry_source_;
  hierarchy::ExactHigherSupportPositiveDecisionSource source_;
};

}  // namespace morsehgp3d::gpu
