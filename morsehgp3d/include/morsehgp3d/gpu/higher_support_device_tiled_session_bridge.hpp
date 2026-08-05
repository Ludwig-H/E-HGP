#pragma once

#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/gpu/higher_support_device_tiled_frontier.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/hierarchy/sparse_higher_support_h0_chunk_run.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    higher_support_device_tiled_session_bridge_schema_version = 1U;

inline constexpr std::string_view
    higher_support_device_tiled_session_bridge_backend =
        "exact_host_session_bridge_native_cuda_pending";
inline constexpr std::string_view
    higher_support_device_tiled_session_bridge_profile = "hgp_reduced";
inline constexpr std::string_view
    higher_support_device_tiled_session_bridge_mode =
        "anchored_session_tile_transaction_supports_3_4_v1";
inline constexpr std::string_view
    higher_support_device_tiled_session_bridge_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    higher_support_device_tiled_session_bridge_public_status = "not_claimed";
// The scientific transition authority is the anchored host session: every
// commit is Q_{j+1} = T_{b_j}(Q_j) verified by the session's own exact fresh
// CPU replay, and the tile is the suffix of frontier entries that one such
// transition resolves completely.  The device-tiled engine resolves exactly
// that suffix (after a sealed deterministic pre-expansion that refines but
// never changes the resolved universe) and its 128-bit slot masses, replayed
// prune records and host-classified terminal records must agree with the
// prepared candidate before the session is allowed to commit.  A censored or
// fatal tile never commits: the anchored checkpoint Q_j is retained and an
// idempotent retry reproduces the identical candidate.
inline constexpr std::string_view
    higher_support_device_tiled_session_bridge_proof_basis =
        "anchored_session_defined_tile_transitions_minimal_work_unit_"
        "boundary_theorem_pre_charge_product_claim_session_counted_"
        "canonical_pre_expansion_sealed_v1_largest_mass_device_refinement_"
        "bigint_mass_cross_validation_per_record_prune_replay_host_closed_"
        "ball_bidirectional_event_and_extra_shell_equality_no_commit_on_"
        "censure_v1";

// The transition budget is never guessed: the anchored stream builder
// claims a new product only after consuming a work unit, so the minimal
// work-unit budget W* that makes a monotone frontier predicate true always
// stops the prepared candidate exactly between products, with no pending
// cursor and a clean successor frontier.  The bridge finds W* by doubling
// from the initial probe and then binary-searching; these caps bound that
// deterministic search fail-closed.
inline constexpr std::size_t
    higher_support_device_tiled_session_bridge_default_work_unit_probe =
        64U;
inline constexpr std::size_t
    higher_support_device_tiled_session_bridge_default_probe_doublings =
        48U;
inline constexpr std::size_t
    higher_support_device_tiled_session_bridge_default_pre_expansion_target =
        16U;
inline constexpr std::size_t
    higher_support_device_tiled_session_bridge_default_expansion_transitions =
        65536U;

struct HigherSupportDeviceTiledSessionBridgeConfig {
  std::size_t initial_work_unit_probe{
      higher_support_device_tiled_session_bridge_default_work_unit_probe};
  std::size_t maximum_work_unit_probe_doublings{
      higher_support_device_tiled_session_bridge_default_probe_doublings};
  // Session-visible pre-expansion target.  While the anchored frontier
  // holds fewer entries and its back entry is still splittable, the
  // bridge commits pure expansion transitions (the canonical CPU split
  // counted inside T_b, resolving zero mass); otherwise the boundary
  // search commits a tile transaction that fully resolves the frontier
  // suffix at the first clean-prefix flip point it converges to.  The
  // entry-count rule is the sealed v1 criterion; a mass-target split rule
  // remains an open design question.
  std::size_t pre_expansion_target_entry_count{
      higher_support_device_tiled_session_bridge_default_pre_expansion_target};
  // Fail-closed backstop on consecutive session pre-expansion transitions
  // (single-child canonical splits descend one LBVH level at a time, so
  // the certified depth bounds the true requirement far below this).
  std::size_t maximum_session_pre_expansion_transition_count{
      higher_support_device_tiled_session_bridge_default_expansion_transitions};
  // Device engine configuration.  The bridge only admits the
  // closed_rank_window policy: the anchored session induction has no
  // strict-interior carrier semantics, so a carrier configuration is a
  // caller error surfaced at construction, never silently mapped.  The
  // maximum_relevant_closed_rank and certified LBVH depth are derived from
  // the authority and must not be preset inconsistently.
  HigherSupportDeviceTiledFrontierConfig frontier_config{};

  friend bool operator==(
      const HigherSupportDeviceTiledSessionBridgeConfig&,
      const HigherSupportDeviceTiledSessionBridgeConfig&) = default;
};

enum class HigherSupportDeviceTiledSessionBridgeStatus : std::uint8_t {
  transaction_committed,
  session_terminal,
  censored_without_commit,
};

struct HigherSupportDeviceTiledSessionBridgeAudit {
  std::uint32_t schema_version{
      higher_support_device_tiled_session_bridge_schema_version};
  std::size_t point_count{};
  std::size_t maximum_relevant_closed_rank{};
  std::size_t committed_transaction_count{};
  std::size_t committed_tile_transaction_count{};
  std::size_t committed_expansion_transition_count{};
  std::size_t censored_without_commit_count{};
  std::size_t committed_tile_root_count{};
  std::size_t committed_tile_slot_count{};
  std::size_t device_pre_expansion_count{};
  std::size_t work_unit_probe_count{};
  std::size_t committed_event_count{};
  std::size_t committed_extra_shell_diagnostic_count{};
  std::size_t committed_prune_certificate_count{};
  std::size_t replayed_device_prune_record_count{};
  std::size_t replayed_device_terminal_record_count{};
  std::size_t host_closed_ball_classification_count{};
  std::size_t wire_round_trip_count{};
  std::size_t maximum_wire_numerator_byte_count{};
  std::size_t maximum_wire_denominator_byte_count{};
  // Exact BigInt masses accumulated across committed transactions only.
  // Device masses come from host-BigInt-revalidated 128-bit slot controls;
  // the session masses come from the committed chunk audit deltas.  Both
  // partitions of the same universe are asserted equal per transaction.
  exact::BigInt committed_universe_support_mass{0};
  exact::BigInt committed_device_well_prune_support_mass{0};
  exact::BigInt committed_device_rank_prune_support_mass{0};
  exact::BigInt committed_device_terminal_support_mass{0};
  exact::BigInt committed_session_resolved_support_mass{0};
  bool session_terminal_reached{false};
  bool induction_identity_reverified_each_commit{false};
  bool tile_boundary_prefix_criterion_enforced{false};
  bool minimal_work_unit_boundary_theorem_applied{false};
  bool session_counted_canonical_pre_expansion{false};
  bool sealed_largest_mass_pre_expansion_v1{false};
  bool pre_expansion_partition_bigint_verified{false};
  bool device_and_session_masses_cross_validated{false};
  bool bidirectional_event_equality_enforced{false};
  bool bidirectional_extra_shell_equality_enforced{false};
  bool per_record_prune_replay_enforced{false};
  bool terminal_geometry_replay_enforced{false};
  bool host_closed_ball_classification_performed{false};
  bool no_commit_performed_on_censure{false};
  bool strict_interior_carrier_rejected_at_construction{false};
  bool wire_caps_validated{false};
  bool frontier_context_rebind_required{false};
  bool bigint_support_mass_transferred{false};
  bool floating_point_decision_performed{false};
  bool dense_product_fallback_performed{false};
  bool global_product_frontier_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool global_cell_coface_or_incidence_arena_materialized{false};
  bool slo_claimed{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const HigherSupportDeviceTiledSessionBridgeAudit&,
      const HigherSupportDeviceTiledSessionBridgeAudit&) = default;
};

struct HigherSupportDeviceTiledSessionBridgeAdvance {
  HigherSupportDeviceTiledSessionBridgeStatus status{
      HigherSupportDeviceTiledSessionBridgeStatus::censored_without_commit};
  // Scientific records of the committed transition, moved out of the
  // session-verified candidate chunk (empty unless status is
  // transaction_committed).  Their closed-ball classification was produced
  // by the exact host stream inside the anchored transition and equality-
  // gated against the drained device terminal records.
  std::vector<hierarchy::ExactHigherSupportEvent> committed_events;
  std::vector<hierarchy::ExactHigherSupportExtraShellDiagnostic>
      committed_extra_shell_diagnostics;
  std::size_t committed_prune_certificate_count{};
  // A committed transition is either one session-counted canonical
  // pre-expansion of the back frontier entry (zero resolved mass, zero
  // records) or one whole-root tile transaction; expansion_transition
  // distinguishes them.
  bool expansion_transition{false};
  std::size_t tile_root_count{};
  std::size_t tile_slot_count{};
  std::size_t minimal_work_unit_budget{};
  HigherSupportDeviceTiledSessionBridgeAudit audit{};

  HigherSupportDeviceTiledSessionBridgeAdvance() = default;
  HigherSupportDeviceTiledSessionBridgeAdvance(
      HigherSupportDeviceTiledSessionBridgeAdvance&&) noexcept = default;
  HigherSupportDeviceTiledSessionBridgeAdvance& operator=(
      HigherSupportDeviceTiledSessionBridgeAdvance&&) noexcept = default;
  HigherSupportDeviceTiledSessionBridgeAdvance(
      const HigherSupportDeviceTiledSessionBridgeAdvance&) = delete;
  HigherSupportDeviceTiledSessionBridgeAdvance& operator=(
      const HigherSupportDeviceTiledSessionBridgeAdvance&) = delete;
};

namespace detail {
class Phase15HigherSupportDeviceTiledSessionBridgeState;
}

// Transactional host integration of the slot-tiled supports-3/4 work engine
// with the anchored higher-support session.  The referenced authority (and
// its cloud and LBVH) and the optional wire-validation context must outlive
// this bridge.  The bridge is single-threaded and fail-closed: any
// cross-validation failure poisons it permanently without touching the
// anchored checkpoint.
class HigherSupportDeviceTiledSessionBridge final {
 public:
  static constexpr std::string_view backend =
      higher_support_device_tiled_session_bridge_backend;
  static constexpr std::string_view profile =
      higher_support_device_tiled_session_bridge_profile;
  static constexpr std::string_view mode =
      higher_support_device_tiled_session_bridge_mode;
  static constexpr std::string_view deployment_status =
      higher_support_device_tiled_session_bridge_deployment_status;
  static constexpr std::string_view public_status =
      higher_support_device_tiled_session_bridge_public_status;
  static constexpr std::string_view proof_basis =
      higher_support_device_tiled_session_bridge_proof_basis;

  // The optional wire-validation context must be built over the same
  // authority; when supplied, every committed event's exact center
  // coordinates and squared level are round-tripped through the durable
  // canonical rational wire and their byte counts are checked against the
  // sealed 6973/9503 caps.
  HigherSupportDeviceTiledSessionBridge(
      const hierarchy::ExactHigherSupportAuthorityContext& authority,
      MortonLbvhDeviceTraversalLease&& traversal_lease,
      HigherSupportDeviceTiledSessionBridgeConfig config = {},
      const hierarchy::ExactSparseHigherSupportH0ChunkRunContext*
          wire_validation_context = nullptr);
  ~HigherSupportDeviceTiledSessionBridge() noexcept;

  HigherSupportDeviceTiledSessionBridge(
      const HigherSupportDeviceTiledSessionBridge&) = delete;
  HigherSupportDeviceTiledSessionBridge& operator=(
      const HigherSupportDeviceTiledSessionBridge&) = delete;
  HigherSupportDeviceTiledSessionBridge(
      HigherSupportDeviceTiledSessionBridge&&) = delete;
  HigherSupportDeviceTiledSessionBridge& operator=(
      HigherSupportDeviceTiledSessionBridge&&) = delete;

  // Executes exactly one committed transition.  The bridge derives the
  // minimal work-unit budget whose prepared candidate ends on a clean
  // product boundary (monotone predicate, deterministic doubling plus
  // binary search): while the anchored frontier is below the pre-expansion
  // target this yields one session-counted canonical expansion of the back
  // entry; otherwise it yields one tile transaction that fully resolves
  // the back roots, which the device engine then resolves independently
  // (after the sealed device-side refinement) and cross-validates in
  // exact::BigInt before the session commits Q_{j+1} = T_{b_j}(Q_j)
  // through its own fresh-replay verification.  A censored tile returns
  // censored_without_commit with the anchored checkpoint unchanged; the
  // caller must rebind a fresh frontier engine before retrying.
  [[nodiscard]] HigherSupportDeviceTiledSessionBridgeAdvance
  advance_one_tile_transaction();

  // Installs a fresh device engine over a new traversal lease after a
  // censored or poisoned engine.  The lease is revalidated against the
  // authority counters fail-closed.  The anchored session state is not
  // touched, so the retried transaction reproduces the identical candidate.
  void rebind_frontier_context(
      MortonLbvhDeviceTraversalLease&& traversal_lease);

  [[nodiscard]] const hierarchy::ExactHigherSupportCheckpoint&
  trusted_checkpoint() const noexcept;
  [[nodiscard]] bool session_terminal() const;
  [[nodiscard]] bool poisoned() const noexcept;
  [[nodiscard]] bool frontier_context_rebind_required() const noexcept;
  [[nodiscard]] const HigherSupportDeviceTiledSessionBridgeAudit& audit()
      const noexcept;
  [[nodiscard]] const HigherSupportDeviceTiledSessionBridgeConfig& config()
      const noexcept;

 private:
  std::unique_ptr<detail::Phase15HigherSupportDeviceTiledSessionBridgeState>
      state_;
};

}  // namespace morsehgp3d::gpu
