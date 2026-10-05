// Calcul d'un produit de l'api (compute, api.hpp) : nuage, index global, domaine FULL et forets, aux parametres FIXES
// du moteur (masque 16379 des sondes). Meme enchainement que bench/full_probe.cpp (run puis full_pass) et
// bench/points_export.cpp (run) : prepare_cloud, build_index, prepare_full_domain sur le Pool, build_full ; ce qui
// change est l'ordre des refus du paragraphe 5 de la specification (positions repetees, puis K superieur au nombre de
// sites, avant tout calcul) et le rapport d'etages (temps et pic d'octets reserves, mesures par le pilote seul).
// Portes : mhgp11_api_session_* (refus, identite des forets avec build_full), mhgp11_cli_full_identity.
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

}  // namespace mhgp11::api_detail

namespace mhgp11::api {

namespace {

// Fin d'un etage : duree et pic reserve pendant l'etage, puis nouveau depart du pic pour le suivant (pilote seul,
// aucune tache ne reserve entre deux etages).
void close_stage(RunReport& report, Stage stage, const Stopwatch& clock, MemoryBudget& budget) noexcept {
  report.at(stage) = {clock.nanoseconds(), budget.restart_peak()};
}

Result<FullTower> full_tower(Session& session, const CloudView& view, Order k, RunReport& report) noexcept {
  if (k < 1 || k > kMaxOrder) return fail(Reason::parameter_out_of_range);
  MemoryBudget& budget = session.budget();
  budget.restart_peak();
  Stopwatch cloud_clock;
  Result<Cloud> cloud = prepare_cloud(view.x, view.y, view.z, view.ids, CoordWidth(), budget);
  if (!cloud.ok()) return cloud.outcome();
  // FULL exige des sites de poids un (docs/ARCHITECTURE.md, paragraphe 7.3), puis K au plus le nombre de sites.
  if (cloud.value().weight() != cloud.value().sites()) return fail(Reason::multiplicity_unsupported);
  if (k > cloud.value().sites()) return fail(Reason::parameter_out_of_range);
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
  Stopwatch tree_clock;
  Result<FullTower> tower =
      build_full(std::move(domain.value()), budget, nullptr, api_detail::full_params(), &session.pool());
  if (!tower.ok()) return tower.outcome();
  close_stage(report, Stage::tree, tree_clock, budget);
  return std::move(tower).take();
}

}  // namespace

Result<Product> compute(Session& session, const CloudView& cloud, const Request& request,
                        RunReport* report) noexcept {
  const FullRequest* full = std::get_if<FullRequest>(&request);
  if (full == nullptr) return fail(Reason::parameter_out_of_range);
  RunReport local;
  Result<FullTower> tower = guarded([&]() { return full_tower(session, cloud, full->k, local); });
  if (!tower.ok()) return tower.outcome();
  if (report != nullptr) {
    for (Stage stage : {Stage::cloud, Stage::index, Stage::domain, Stage::tree, Stage::attach})
      report->at(stage) = local.at(stage);
  }
  return Product(request, std::move(tower).take(), session.identity());
}

}  // namespace mhgp11::api
