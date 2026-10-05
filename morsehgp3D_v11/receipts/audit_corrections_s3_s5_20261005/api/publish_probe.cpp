// Sonde de l'aller-retour publication de l'API -> lecteur officiel (audit abc30ed06, porte mhgp11_api_publish_reader).
// Publie growth_ABCZ (fixtures()[2], quatre points) a K = 3 dans le dossier donne, avec une provenance du cas demande :
//   valide          12 et 4 octets par point, budget non declare ;
//   tailles_nulles  provenance vide (tailles nulles pour un produit non vide) ;
//   rapport_faux    11 octets par point de coordonnees ;
//   compte_faux     rapport 12/4, mais pour huit points ;
//   budget_nul      budget declare nul.
// Ecrit « publish_probe <cas> <raison> <etat> » ; code de sortie de l'appel (0 conforme, 2 refus, 3 invariant).
#include <cstdio>
#include <string>

#include "api_support.hpp"

using namespace mhgp11;
using namespace mhgp11::api_test;

int main(int argc, char** argv) {
  if (argc != 3) {
    std::fprintf(stderr, "usage : mhgp11_api_publish_probe <dossier> <cas>\n");
    return 2;
  }
  const std::string directory = argv[1];
  const std::string which = argv[2];
  const Points points = fixtures()[2];
  api::Provenance provenance = provenance_of(points);
  if (which == "tailles_nulles") provenance = api::Provenance{};
  else if (which == "rapport_faux") provenance.points_bytes = u64{11} * points.size();
  else if (which == "compte_faux") provenance.points_bytes = 96, provenance.ids_bytes = 32;
  else if (which == "budget_nul") provenance.budget_bytes = 0;
  else if (which != "valide") {
    std::fprintf(stderr, "cas inconnu : %s\n", which.c_str());
    return 2;
  }
  api::Session session = session_of(1);
  auto planned = io::OutputDirectory::plan(directory.c_str(), {});
  if (!planned.ok()) return exit_code(planned.outcome());
  api::Publication result{};
  {
    auto product = api::compute(session, points.view(), api::FullRequest{3});
    if (!product.ok()) return exit_code(product.outcome());
    result = api::publish(session, product.value(), planned.value(), provenance);
  }
  if (result.ok()) result = api::finish(session, planned.value());
  const std::string_view reason = reason_name(result.outcome.reason);
  const std::string_view state = api::publication_state_name(result.state);
  std::printf("publish_probe %s %.*s %.*s\n", which.c_str(), static_cast<int>(reason.size()), reason.data(),
              static_cast<int>(state.size()), state.data());
  return exit_code(result.outcome);
}
