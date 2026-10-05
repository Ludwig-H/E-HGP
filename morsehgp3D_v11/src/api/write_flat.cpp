// Ecrivain MHGP11ET version 1 de la sortie plat (docs/SORTIES.md, paragraphe 7 ; specification, paragraphe 6.5 ;
// tranche S10) et son manifeste. Ecrit a neuf, sans source portee : le format est celui du contrat.
//
// Petit-boutiste ; en-tete de 56 octets : magie "MHGP11ET", puis six mots u64 : version 1, n_points, k, mcs, z,
// selection (0 EOM, 1 feuilles) ; puis labels i64[n_points] DANS L'ORDRE DU FICHIER D'ENTREE : le plus petit PointId
// du cluster retenu, ou -1 pour le bruit (complement a deux). Aucun bourrage : la taille vaut 56 + 8 n_points.
// Les compteurs de la tete vont au manifeste. Lecteur : bench/mhgp11_formats.py (read_flat). Porte : mhgp11_cli_plat.
#include <algorithm>
#include <array>
#include <string_view>

#include "api/internal.hpp"

namespace mhgp11::api_detail {

Outcome write_flat(io::FileWriter& file, const api::FlatRequest& request, const head::FlatLabels& flat) noexcept {
  static constexpr std::string_view kMagic = "MHGP11ET";
  MHGP11_TRY(file.bytes(std::span<const u8>(reinterpret_cast<const u8*>(kMagic.data()), kMagic.size())));
  const std::array<u64, 6> words = {1, flat.labels.size(), request.k, request.mcs, request.z,
                                    static_cast<u64>(request.selection)};
  MHGP11_TRY(file.u64s(words));
  // Etiquettes par paquets : i64 en complement a deux, conversion definie en C++20.
  std::array<u64, 512> chunk{};
  const u64 n = flat.labels.size();
  for (u64 begin = 0; begin < n; begin += chunk.size()) {
    const u64 count = std::min<u64>(chunk.size(), n - begin);
    for (u64 i = 0; i < count; ++i) chunk[i] = static_cast<u64>(flat.labels[begin + i]);
    MHGP11_TRY(file.u64s(std::span<const u64>(chunk.data(), count)));
  }
  return file.size() == 56 + 8 * n ? Outcome{} : fail(Reason::head_invariant);
}

std::string flat_manifest(const api::Product& product, const api::Provenance& provenance, u64 bytes,
                          const io::Digest& sha256) {
  const OrderTree& tree = product.order_tree();
  const head::FlatStats& stats = product.flat().stats;
  std::string out = manifest_head(product, provenance, api::kFlatFileName, "MHGP11ET", bytes, sha256,
                                  api::tree_k_sha256(tree.domain(), tree.forest()));
  const std::array<std::pair<std::string_view, u64>, 9> counts = {{
      {"sites", tree.domain().index().cloud().sites()},
      {"nodes", tree.forest().nodes().size()},
      {"clusters", stats.clusters},
      {"selected", stats.selected},
      {"noise", stats.noise},
      {"decisions", stats.decisions},
      {"exact", stats.exact},
      {"equalities", stats.equalities},
      {"unbracketed", stats.unbracketed},
  }};
  out.append(",\"counts\":{");
  for (std::size_t i = 0; i < counts.size(); ++i) {
    out.append(i == 0 ? "\"" : ",\"");
    out.append(counts[i].first);
    out.append("\":");
    manifest_number(out, counts[i].second);
  }
  out.append("}}\n");
  return out;
}

}  // namespace mhgp11::api_detail
