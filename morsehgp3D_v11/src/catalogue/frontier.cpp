// Prelude sequentiel DFS gauche/droite, taches immuables par ordinal, rejeu sans seconde frontiere.
#include "catalogue/frontier.hpp"

namespace mhgp11::catalogue_detail {
namespace {

template <class Emit>
Outcome prefix(Run& run, std::span<const SiteIdx> parent, const Box& box, u32 depth, u32 cut_depth,
               Emit& emit) noexcept {
  ReadyNode ready;
  MHGP11_TRY(prepare_node(run, parent, box, depth, ready));
  if (ready.count == 0) return {};
  Box left, right;
  if (depth == cut_depth || !split_ready(ready, run.params, left, right)) return emit(std::move(ready));
  MHGP11_TRY(prefix(run, ready.sites(), left, depth + 1, cut_depth, emit));
  return prefix(run, ready.sites(), right, depth + 1, cut_depth, emit);
}

template <class Emit>
Outcome visit(Run& run, u32 cut_depth, Emit& emit) noexcept {
  Buffer<SiteIdx> root;
  Box box;
  MHGP11_TRY(make_root(run, root, box));
  return prefix(run, root.span(), box, 0, cut_depth, emit);
}

}  // namespace

Outcome frontier_memory_bound(u32 sites, u32 cut_depth, u64& bytes) noexcept {
  bytes = 0;
  if (cut_depth > kFrontierDepth) return fail(Reason::parameter_out_of_range);
  // <=256 listes finales, <=9 listes de la pile et une liste racine ; produit <2^43 octets.
  return add_bytes<SiteIdx>(bytes, u64(sites) * ((u64{1} << cut_depth) + cut_depth + 2));
}

void Frontier::clear() noexcept {
  for (u32 i = 0; i < count_; ++i) tasks_[i] = ReadyNode{};
  cloud_ = nullptr;
  ledger_ = {};
  count_ = 0;
  prepared_ = false;
}

bool Frontier::matches(const Run& run) const noexcept {
  return prepared_ && cloud_ == &run.cloud && params_.kmax == run.params.kmax &&
         params_.leaf_size == run.params.leaf_size && params_.max_leaf == run.params.max_leaf &&
         params_.max_nodes == run.params.max_nodes && params_.ball_limit == run.params.ball_limit &&
         params_.pair_graph == run.params.pair_graph;
}

Outcome Frontier::prepare(Run& run, u32 cut_depth) noexcept {
  if (prepared_ || run.ledger != CatalogueLedger{}) return fail(Reason::catalogue_invariant);
  if (cut_depth > kFrontierDepth) return fail(Reason::parameter_out_of_range);
  clear();
  cut_depth_ = cut_depth;
  auto save = [this](ReadyNode&& ready) noexcept -> Outcome {
    if (count_ >= kFrontierTasks) return fail(Reason::catalogue_invariant);
    tasks_[count_++] = std::move(ready);
    return {};
  };
  const auto outcome = visit(run, cut_depth_, save);
  if (!outcome.ok()) {
    clear();
    return outcome;
  }
  cloud_ = &run.cloud;
  params_ = run.params;
  ledger_ = run.ledger;
  prepared_ = true;
  return {};
}

Outcome Frontier::verify(Run& run) const noexcept {
  if (!matches(run) || run.ledger != CatalogueLedger{}) return fail(Reason::catalogue_invariant);
  u32 ordinal = 0;
  auto compare = [this, &ordinal](ReadyNode&& ready) noexcept -> Outcome {
    if (ordinal >= count_) return fail(Reason::catalogue_invariant);
    const auto& expected = tasks_[ordinal++];
    if (ready.count != expected.count || ready.depth != expected.depth || ready.box.lo != expected.box.lo ||
        ready.box.hi != expected.box.hi || ready.storage.size() != expected.storage.size() ||
        !std::equal(ready.sites().begin(), ready.sites().end(), expected.sites().begin()))
      return fail(Reason::catalogue_invariant);
    return {};
  };
  MHGP11_TRY(visit(run, cut_depth_, compare));
  return ordinal == count_ && run.ledger == ledger_ ? Outcome{} : fail(Reason::catalogue_invariant);
}

Outcome Frontier::execute_task(u32 i, Run& run) const noexcept {
  if (!matches(run) || i >= count_) return fail(Reason::catalogue_invariant);
  return run_ready(run, tasks_[i]);
}

Outcome Frontier::owned_bytes(u64& bytes) const noexcept {
  bytes = 0;
  if (!prepared_) return fail(Reason::catalogue_invariant);
  for (u32 i = 0; i < count_; ++i) MHGP11_TRY(add_bytes<SiteIdx>(bytes, tasks_[i].storage.size()));
  return {};
}

Outcome Frontier::verify_memory_bound(u64& bytes) const noexcept {
  bytes = 0;
  if (!prepared_) return fail(Reason::catalogue_invariant);
  return add_bytes<SiteIdx>(bytes, u64(cloud_->sites()) * (cut_depth_ + 2));
}

Outcome Frontier::suffix_memory_bound(u32 workers, u64& bytes) const noexcept {
  bytes = 0;
  if (!prepared_) return fail(Reason::catalogue_invariant);
  if (workers == 0 || workers > kFrontierTasks) return fail(Reason::parameter_out_of_range);
  std::array<u64, kFrontierTasks> largest{};
  for (u32 i = 0; i < count_; ++i) {
    const auto& node = tasks_[i];
    Box left, right;
    if (!split_ready(node, params_, left, right)) continue;
    if (node.depth >= kMaxDepth) return fail(Reason::catalogue_invariant);
    u64 bound = 0;
    // Le noeud ready ne realloue rien. Au plus D-depth buffers descendants, chacun de capacite <=count.
    MHGP11_TRY(add_bytes<SiteIdx>(bound, u64(node.count) * (kMaxDepth - node.depth)));
    // Tableau fixe ordonne : pas de tri susceptible d'allouer et pas de stockage proportionnel au nuage.
    for (u32 j = 0; j < workers; ++j)
      if (bound > largest[j]) std::swap(bound, largest[j]);
  }
  for (u32 i = 0; i < workers; ++i) MHGP11_TRY(checked_add(bytes, largest[i]));
  return {};
}

}  // namespace mhgp11::catalogue_detail
