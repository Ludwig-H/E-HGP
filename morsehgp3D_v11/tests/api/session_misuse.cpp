// Sonde de la destruction d'une Session de l'api (docs/ARCHITECTURE.md, paragraphe 7.1, et regle 4 ; audit general
// a65903a7b) :
//   mhgp11_api_session_misuse_probe vivant  la Session est detruite alors qu'un Product qu'elle a calcule vit encore :
//                                           budget non revenu a zero, violation d'invariant, std::terminate (porte
//                                           mhgp11_api_session_destroyed_live : arret anormal attendu) ;
//   mhgp11_api_session_misuse_probe rendu   temoin : le Product est rendu avant la destruction ; ligne
//                                           "session_misuse rendu", code 0 (porte
//                                           mhgp11_api_session_destroyed_released).
// Codes : 0 temoin conforme ; 2 usage ; 3 calcul refuse (precondition de la sonde).
#include <array>
#include <cstdio>
#include <optional>
#include <string_view>

#include "api/api.hpp"

int main(int argc, char** argv) {
  using namespace mhgp11;
  if (argc != 2) return 2;
  const std::string_view mode(argv[1]);
  if (mode != "vivant" && mode != "rendu") return 2;
  const std::array<u32, 2> x{0, 2}, y{0, 0}, z{0, 0};
  const std::array<PointId, 2> ids{make_id<PointId>(7), make_id<PointId>(91)};
  std::optional<api::Product> product;
  {
    Result<api::Session> made = api::Session::make({MemoryBudget::kUnlimited, 1});
    if (!made.ok()) return 3;
    api::Session session = std::move(made).take();
    Result<api::Product> computed = api::compute(session, {x, y, z, ids}, api::FullRequest{1});
    if (!computed.ok()) return 3;
    product.emplace(std::move(computed).take());
    if (mode == "rendu") product.reset();
  }  // destruction de la Session : arret du processus si le produit vit encore
  std::printf("session_misuse %.*s\n", static_cast<int>(mode.size()), mode.data());
  return 0;
}
