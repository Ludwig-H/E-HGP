// Calcul d'un produit de l'api (compute, api.hpp) : nuage, index global, domaine FULL et forets, aux parametres FIXES
// du moteur (masque 16379 des sondes). Meme enchainement que bench/full_probe.cpp (run puis full_pass) et
// bench/points_export.cpp (run) : prepare_cloud, build_index, prepare_full_domain sur le Pool, build_full ; ce qui
// change est l'ordre des refus du paragraphe 5 de la specification (positions repetees, puis K superieur au nombre de
// sites, avant tout calcul) et le rapport d'etages (temps et pic d'octets reserves, mesures par le pilote seul).
// Sortie supports (tranche S7) : meme nuage, index et domaine, puis l'arbre d'ordre K seul (build_order, masque 7035 :
// order_params) et l'assemblage de la hierarchie des supports (build_support_hierarchy), sur le Pool de la Session.
// Sortie points (tranche S9) : meme arbre d'ordre K seul, puis la hierarchie de points H^r_{K+1} (points::hang, etage
// output) ; K >= n refuse pour K >= 2 avec K > n, au meme point du parcours (decision K = n de docs/SORTIES.md).
// Portes : mhgp11_api_session_* (refus, identite des forets avec build_full), mhgp11_cli_full_identity,
// mhgp11_cli_supports_oracle, mhgp11_cli_supports_scale*, mhgp11_cli_points, mhgp11_points_*.
#include <algorithm>
#include <variant>

#include "api/internal.hpp"
#include "cloud/cloud.hpp"
#include "index/index.hpp"

namespace mhgp11::api_detail {

CatalogueParams catalogue_params(Order k) noexcept {
  CatalogueParams params;
  params.kmax = static_cast<int>(k);
  params.leaf_size = 16;
  params.max_leaf = 256;
  params.cache_center_lines = true;
  params.indirect_sort = true;
  params.adaptive_frontier = true;
  params.parallel_assembly = true;
  params.single_pass = true;
  params.pair_graph = true;
  return params;
}

FullParams full_params() noexcept {
  FullParams params;
  params.regular_batch_capacity = 4096;
  params.descent_lanes = 48;
  params.parallel_verticals = true;
  params.reuse_census_workspace = true;
  params.dense_birth_lookup = true;
  params.reuse_regular_verticals = true;
  params.population_lookup = true;
  params.concurrent_orders = true;
  return params;
}

FullParams order_params() noexcept {
  FullParams params = full_params();
  params.parallel_verticals = false;       // 128
  params.reuse_regular_verticals = false;  // 1024
  params.concurrent_orders = false;        // 8192
  return params;
}

}  // namespace mhgp11::api_detail

namespace mhgp11::api {

namespace {

// Fin d'un etage : duree et pic reserve pendant l'etage, puis nouveau depart du pic pour le suivant (pilote seul,
// aucune tache ne reserve entre deux etages).
void close_stage(RunReport& report, Stage stage, const Stopwatch& clock, MemoryBudget& budget) noexcept {
  report.at(stage) = {clock.nanoseconds(), budget.restart_peak()};
}

// Etages cloud, index et domain, communs aux sorties : nuage prepare, index global, domaine FULL de kmax = k. strict :
// sortie points, K = n refuse aussi pour K >= 2.
Result<FullDomain> prepare_domain(Session& session, const CloudView& view, Order k, RunReport& report,
                                  bool strict = false) noexcept {
  if (k < 1 || k > kMaxOrder) return fail(Reason::parameter_out_of_range);
  MemoryBudget& budget = session.budget();
  budget.restart_peak();
  Stopwatch cloud_clock;
  Result<Cloud> cloud = prepare_cloud(view.x, view.y, view.z, view.ids, CoordWidth(), budget);
  if (!cloud.ok()) return cloud.outcome();
  // FULL exige des sites de poids un (docs/ARCHITECTURE.md, paragraphe 7.3), puis K au plus le nombre de sites.
  if (cloud.value().weight() != cloud.value().sites()) return fail(Reason::multiplicity_unsupported);
  if (k > cloud.value().sites()) return fail(Reason::parameter_out_of_range);
  if (strict && k >= 2 && k == cloud.value().sites()) return fail(Reason::parameter_out_of_range);
  close_stage(report, Stage::cloud, cloud_clock, budget);
  Stopwatch index_clock;
  Result<GlobalIndex> index = build_index(std::move(cloud.value()), {}, budget);
  if (!index.ok()) return index.outcome();
  close_stage(report, Stage::index, index_clock, budget);
  Stopwatch domain_clock;
  Result<FullDomain> domain =
      prepare_full_domain(std::move(index.value()), api_detail::catalogue_params(k), budget, session.pool());
  if (!domain.ok()) return domain.outcome();
  close_stage(report, Stage::domain, domain_clock, budget);
  return domain;
}

Result<FullTower> full_tower(Session& session, const CloudView& view, Order k, RunReport& report) noexcept {
  Result<FullDomain> domain = prepare_domain(session, view, k, report);
  if (!domain.ok()) return domain.outcome();
  MemoryBudget& budget = session.budget();
  Stopwatch tree_clock;
  Result<FullTower> tower =
      build_full(std::move(domain.value()), budget, nullptr, api_detail::full_params(), &session.pool());
  if (!tower.ok()) return tower.outcome();
  close_stage(report, Stage::tree, tree_clock, budget);
  return std::move(tower).take();
}

// Produit de la sortie supports : arbre d'ordre K seul et hierarchie des supports.
struct SupportsParts {
  OrderTree tree;
  supports::SupportHierarchy hierarchy;
};

// Etages tree et attach : build_order, dont le balayage du rattachement (attach_ns, diagnostic de build_order) est
// retire de tree et publie a part. Le pic de build_order va a tree.
Result<OrderTree> order_tree(Session& session, const CloudView& view, Order k, RunReport& report,
                             bool strict) noexcept {
  Result<FullDomain> domain = prepare_domain(session, view, k, report, strict);
  if (!domain.ok()) return domain.outcome();
  MemoryBudget& budget = session.budget();
  Stopwatch tree_clock;
  u64 attach_ns = 0;
  Result<OrderTree> tree = build_order(std::move(domain.value()), k, budget, api_detail::order_params(),
                                       &session.pool(), nullptr, &attach_ns);
  if (!tree.ok()) return tree.outcome();
  const u64 tree_ns = tree_clock.nanoseconds();
  report.at(Stage::tree) = {tree_ns - std::min(tree_ns, attach_ns), budget.restart_peak()};
  report.at(Stage::attach) = {attach_ns, 0};
  return tree;
}

// Etages tree et attach (order_tree), puis output : build_support_hierarchy.
Result<SupportsParts> supports_parts(Session& session, const CloudView& view, Order k, RunReport& report) noexcept {
  Result<OrderTree> tree = order_tree(session, view, k, report, false);
  if (!tree.ok()) return tree.outcome();
  MemoryBudget& budget = session.budget();
  Stopwatch output_clock;
  Result<supports::SupportHierarchy> hierarchy =
      supports::build_support_hierarchy(tree.value(), budget, &session.pool());
  if (!hierarchy.ok()) return hierarchy.outcome();
  close_stage(report, Stage::output, output_clock, budget);
  return SupportsParts{std::move(tree).take(), std::move(hierarchy).take()};
}

// Produit de la sortie points : arbre d'ordre K seul et hierarchie de points.
struct PointsParts {
  OrderTree tree;
  points::PointHierarchy points;
};

// Etages tree et attach (order_tree, K = n refuse a K >= 2), puis output : points::hang.
Result<PointsParts> points_parts(Session& session, const CloudView& view, Order k, RunReport& report) noexcept {
  Result<OrderTree> tree = order_tree(session, view, k, report, true);
  if (!tree.ok()) return tree.outcome();
  MemoryBudget& budget = session.budget();
  Stopwatch output_clock;
  Result<points::PointHierarchy> hanging = points::hang(tree.value(), budget, &session.pool());
  if (!hanging.ok()) return hanging.outcome();
  close_stage(report, Stage::output, output_clock, budget);
  return PointsParts{std::move(tree).take(), std::move(hanging).take()};
}

}  // namespace

Result<Product> compute(Session& session, const CloudView& cloud, const Request& request,
                        RunReport* report) noexcept {
  RunReport local;
  if (const SupportsRequest* wanted = std::get_if<SupportsRequest>(&request)) {
    Result<SupportsParts> parts = guarded([&]() { return supports_parts(session, cloud, wanted->k, local); });
    if (!parts.ok()) return parts.outcome();
    if (report != nullptr) {
      for (Stage stage : {Stage::cloud, Stage::index, Stage::domain, Stage::tree, Stage::attach, Stage::output})
        report->at(stage) = local.at(stage);
    }
    SupportsParts made = std::move(parts).take();
    return Product(request, std::move(made.tree), std::move(made.hierarchy), session.identity());
  }
  if (const PointsRequest* wanted = std::get_if<PointsRequest>(&request)) {
    Result<PointsParts> parts = guarded([&]() { return points_parts(session, cloud, wanted->k, local); });
    if (!parts.ok()) return parts.outcome();
    if (report != nullptr) {
      for (Stage stage : {Stage::cloud, Stage::index, Stage::domain, Stage::tree, Stage::attach, Stage::output})
        report->at(stage) = local.at(stage);
    }
    PointsParts made = std::move(parts).take();
    return Product(request, std::move(made.tree), std::move(made.points), session.identity());
  }
  const FullRequest* full = std::get_if<FullRequest>(&request);
  if (full == nullptr) return fail(Reason::parameter_out_of_range);
  Result<FullTower> tower = guarded([&]() { return full_tower(session, cloud, full->k, local); });
  if (!tower.ok()) return tower.outcome();
  if (report != nullptr) {
    for (Stage stage : {Stage::cloud, Stage::index, Stage::domain, Stage::tree, Stage::attach})
      report->at(stage) = local.at(stage);
  }
  return Product(request, std::move(tower).take(), session.identity());
}

}  // namespace mhgp11::api
