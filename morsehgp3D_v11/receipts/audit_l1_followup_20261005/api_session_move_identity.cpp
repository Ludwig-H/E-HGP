// Fixture proposee, NON compilee : executer uniquement sur G4, argv[1] = chemin absent dans dossier temporaire.
// Tue deux erreurs distinctes : absence de provenance Session, puis jeton attache a l'adresse de la Session source.
#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include "api/api.hpp"

int main(int argc, char** argv) {
  if (argc != 2) return 3;
  auto original = mhgp11::api::Session::make({mhgp11::MemoryBudget::kUnlimited, 1});
  auto foreign = mhgp11::api::Session::make({0, 1});
  if (!original.ok() || !foreign.ok()) return 3;
  const std::array<mhgp11::u32, 2> x{0, 2}, y{0, 0}, z{0, 0};
  const std::array<mhgp11::PointId, 2> ids{mhgp11::PointId{7}, mhgp11::PointId{91}};
  auto computed = mhgp11::api::compute(original.value(), {x, y, z, ids}, mhgp11::api::FullRequest{1});
  if (!computed.ok()) return 3;
  std::optional<mhgp11::api::Product> first(std::move(computed).take());
  const auto digest = mhgp11::api::tree_k_sha256(first->full().domain(), first->full().order(1));
  const auto reserved = original.value().budget().used();
  if (reserved == 0) return 3;
  // Produit vivant au deplacement de Session, puis deplacement du produit lui-meme.
  mhgp11::api::Session owner(std::move(original.value()));
  std::optional<mhgp11::api::Product> product(std::move(*first));
  first.reset();
  if (!original.value().close().ok() || owner.budget().used() != reserved || owner.close().reason != mhgp11::Reason::budget_not_released ||
      mhgp11::api::tree_k_sha256(product->full().domain(), product->full().order(1)) != digest) return 3;
  auto planned = mhgp11::io::OutputDirectory::plan(argv[1], {});
  if (!planned.ok()) return 3;
  mhgp11::api::RunReport report;
  for (std::size_t i = 0; i < report.stages.size(); ++i) report.stages[i] = {11 + i, 22 + i};
  const auto untouched = report;
  const auto rejected = mhgp11::api::publish(foreign.value(), *product, planned.value(), {}, &report);
  bool same_report = true;
  for (std::size_t i = 0; i < report.stages.size(); ++i)
    same_report = same_report && report.stages[i].nanoseconds == untouched.stages[i].nanoseconds &&
                  report.stages[i].peak_bytes == untouched.stages[i].peak_bytes;
  const bool correct_rejection = !rejected.ok() &&
      rejected.outcome.reason == mhgp11::Reason::parameter_out_of_range &&
      rejected.state == mhgp11::api::PublicationState::none && same_report &&
      !std::filesystem::exists(argv[1]) && !std::filesystem::exists(std::string(argv[1]) + ".pending") &&
      foreign.value().budget().used() == 0 && owner.budget().used() == reserved;
  if (!correct_rejection) {
    if (planned.value().committed()) {
      const auto cleanup = mhgp11::api::withdraw(mhgp11::fail(mhgp11::Reason::parameter_out_of_range), planned.value());
      if (cleanup.state != mhgp11::api::PublicationState::none) return 3;
    }
    product.reset();
    return 1;
  }
  // Le meme jeton apres les deux deplacements doit encore accepter la Session proprietaire.
  const auto accepted = mhgp11::api::publish(owner, *product, planned.value(), {}, &report);
  if (!accepted.ok() || accepted.state != mhgp11::api::PublicationState::published_complete) return 1;
  if (owner.close().reason != mhgp11::Reason::budget_not_released) return 3;
  product.reset();
  const auto finished = mhgp11::api::finish(owner, planned.value());
  if (!finished.ok() || owner.budget().used() != 0 || !foreign.value().close().ok()) return 3;
  const auto cleanup = mhgp11::api::withdraw(mhgp11::fail(mhgp11::Reason::parameter_out_of_range), planned.value());
  return cleanup.state == mhgp11::api::PublicationState::none ? 0 : 3;
}
