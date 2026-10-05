// Fixture native proposee ; non compilee dans le Codespace.
// Executer uniquement sur G4, dans un dossier temporaire neuf donne en argv[1].
// Attendu apres correction S5 : refus avant creation pour produit issu de A et publication dans B.
#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include "api/api.hpp"

int main(int argc, char** argv) {
  if (argc != 2) return 3;
  auto a = mhgp11::api::Session::make({mhgp11::MemoryBudget::kUnlimited, 1});
  auto b = mhgp11::api::Session::make({0, 1});
  if (!a.ok() || !b.ok()) return 3;
  const std::array<mhgp11::u32, 2> x{0, 2}, y{0, 0}, z{0, 0};
  const std::array<mhgp11::PointId, 2> ids{mhgp11::PointId{7}, mhgp11::PointId{91}};
  auto made = mhgp11::api::compute(a.value(), {x, y, z, ids}, mhgp11::api::FullRequest{1});
  if (!made.ok()) return 3;
  std::optional<mhgp11::api::Product> product(std::move(made).take());
  if (a.value().budget().used() == 0 || b.value().budget().used() != 0) return 3;
  auto planned = mhgp11::io::OutputDirectory::plan(argv[1], {});
  if (!planned.ok()) return 3;
  mhgp11::api::RunReport report;
  report.at(mhgp11::api::Stage::output) = {11, 22};
  report.at(mhgp11::api::Stage::write) = {33, 44};
  const auto published = mhgp11::api::publish(b.value(), *product, planned.value(), {}, &report);
  const bool rejected = !published.ok() && published.outcome.reason == mhgp11::Reason::parameter_out_of_range;
  const bool untouched = report.at(mhgp11::api::Stage::output).nanoseconds == 11 &&
                         report.at(mhgp11::api::Stage::output).peak_bytes == 22 &&
                         report.at(mhgp11::api::Stage::write).nanoseconds == 33 &&
                         report.at(mhgp11::api::Stage::write).peak_bytes == 44;
  const bool absent = !std::filesystem::exists(argv[1]) &&
                      !std::filesystem::exists(std::string(argv[1]) + ".pending");
  // Nettoyage si le WIP fautif a publie ; aucune sortie de fixture conservee.
  if (planned.value().committed()) {
    const auto cleaned = mhgp11::api::withdraw(mhgp11::fail(mhgp11::Reason::parameter_out_of_range), planned.value());
    if (cleaned.state != mhgp11::api::PublicationState::none) return 3;
  }
  if (a.value().close().reason != mhgp11::Reason::budget_not_released) return 3;
  product.reset();
  if (!a.value().close().ok() || !b.value().close().ok()) return 3;
  return rejected && untouched && absent ? 0 : 1;
}
