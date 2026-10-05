// Ecrivain MHGP11PT version 1 de la sortie points (docs/SORTIES.md, paragraphe 7 ; tranche S9) et son manifeste.
// Ecrit a neuf, sans source portee : le format est celui du contrat (specification, paragraphe 6.4), normatif.
//
// Petit-boutiste ; en-tete de 144 octets (magie "MHGP11PT", puis 17 mots u64 : version 1, coord_bits, k, m
// (qualification), kappa (1), n, L (rangs references), W (mots par niveau), N, P (plateaux), Bk (blocs), decalages de
// SITES, LEVELS, NODES, HANGING, TREE, taille totale) ; puis les sections dans cet ordre, colonne par colonne, chaque
// colonne commencant sur une frontiere de 8 octets, bourrage nul, une colonne vide n'occupant aucun octet :
//   SITES    x, y, z, point_id (u32[n]), lignes = SiteIdx (rang de Morton) ;
//   LEVELS   rank (u32[L], croissants), num (u64[W L]), den (u64[W L]) : valeur exacte NON reduite l = num / den de
//            chaque rang reference, W mots petit-boutistes par valeur (niveau apres niveau) ;
//   NODES    parent (kNone a la racine), rank (u32[N]) : numerotation canonique de la foret d'ordre K ;
//   HANGING  t, M, Q, owner, floor (u32[n]), strict (u8[n]) : date sqrt(l_t) + sqrt(l_M) - sqrt(l_Q) (M = Q = 0 sans
//            rival), proprietaire, rang plancher, date strictement entre deux niveaux ;
//   TREE     plateau_t, plateau_M, plateau_Q (u32[P]), block_plateau, block_parent (u32[Bk], kNone a une racine),
//            site_block, site_plateau (u32[n]).
// Lecteur : bench/mhgp11_formats.py (read_points). Porte : mhgp11_cli_points.
#include <array>

#include "api/internal.hpp"

namespace mhgp11::api_detail {

namespace {

constexpr u64 pad8(u64 bytes) noexcept { return (bytes + 7) & ~u64{7}; }
constexpr u64 kHeaderBytes = 8 + 17 * 8;
// Mots u64 par numerateur ou denominateur de niveau, comme l'export MHGP11PH (bench/points_export.cpp) : trois en u18
// et u21, quatre en u24.
constexpr u64 kLevelWords = (u64{num::Budget::level_numerator} > u64{num::Budget::level_denominator}
                                 ? u64{num::Budget::level_numerator}
                                 : u64{num::Budget::level_denominator}) / 64 + 1;
static_assert(kLevelWords == 3 || kLevelWords == 4, "MHGP11PT : trois ou quatre mots par niveau");

struct Layout {
  u64 n = 0, levels = 0, nodes = 0, plateaus = 0, blocks = 0;
  u64 sites_at = 0, levels_at = 0, nodes_at = 0, hanging_at = 0, tree_at = 0, size = 0;
  void place() noexcept {
    sites_at = kHeaderBytes;
    levels_at = sites_at + 4 * pad8(4 * n);
    nodes_at = levels_at + pad8(4 * levels) + 2 * 8 * kLevelWords * levels;
    hanging_at = nodes_at + 2 * pad8(4 * nodes);
    tree_at = hanging_at + 5 * pad8(4 * n) + pad8(n);
    size = tree_at + 3 * pad8(4 * plateaus) + 2 * pad8(4 * blocks) + 2 * pad8(4 * n);
  }
};

Outcome column(io::FileWriter& file, std::span<const u32> values) noexcept {
  MHGP11_TRY(file.u32s(values));
  return file.pad8();
}

// Une valeur entiere non negative de niveau en kLevelWords mots (les mots au-dela doivent etre nuls).
template <class Int>
Outcome level_words(io::FileWriter& file, const Int& value) noexcept {
  const auto wide = num::to_wide(value);
  if (wide.neg) return fail(Reason::points_invariant);
  for (u64 j = kLevelWords; j < wide.words.size(); ++j)
    if (wide.words[j] != 0) return fail(Reason::points_invariant);
  std::array<u64, kLevelWords> words{};
  for (u64 j = 0; j < kLevelWords && j < wide.words.size(); ++j) words[j] = wide.words[j];
  return file.u64s(words);
}

Outcome reached(const io::FileWriter& file, u64 offset) noexcept {
  return file.size() == offset ? Outcome{} : fail(Reason::points_invariant);
}

}  // namespace

Outcome write_points(io::FileWriter& file, const OrderTree& tree, const points::PointHierarchy& h) noexcept {
  const Cloud& cloud = tree.domain().index().cloud();
  const auto levels = tree.domain().catalogue().levels();
  const auto nodes = tree.forest().nodes();
  const points::PointTree& pt = h.tree();
  Layout at;
  at.n = cloud.sites();
  at.levels = h.levels().size();
  at.nodes = nodes.size();
  at.plateaus = pt.plateau_t().size();
  at.blocks = pt.block_plateau().size();
  if (cloud.weight() != at.n || h.t().size() != at.n || pt.site_block().size() != at.n || h.order() != tree.order())
    return fail(Reason::points_invariant);
  for (const u32 r : h.levels())
    if (r >= levels.size()) return fail(Reason::points_invariant);
  at.place();
  static constexpr std::string_view kMagic = "MHGP11PT";
  MHGP11_TRY(file.bytes(std::span<const u8>(reinterpret_cast<const u8*>(kMagic.data()), kMagic.size())));
  const std::array<u64, 17> words = {1,           static_cast<u64>(kCoordBits), tree.order(), h.qualification(),
                                     points::kKappa, at.n,       at.levels,  kLevelWords,
                                     at.nodes,    at.plateaus, at.blocks,  at.sites_at,
                                     at.levels_at, at.nodes_at, at.hanging_at, at.tree_at,
                                     at.size};
  MHGP11_TRY(file.u64s(words));
  MHGP11_TRY(reached(file, at.sites_at));
  for (const auto values : {cloud.x(), cloud.y(), cloud.z()}) MHGP11_TRY(column(file, values));
  for (u32 s = 0; s < at.n; ++s) {
    const u32 id = idx(cloud.points(SiteIdx{s})[0]);
    MHGP11_TRY(file.u32s(std::span<const u32>(&id, 1)));
  }
  MHGP11_TRY(file.pad8());
  MHGP11_TRY(reached(file, at.levels_at));
  MHGP11_TRY(column(file, h.levels()));
  for (const u32 r : h.levels()) MHGP11_TRY(level_words(file, levels[r].numerator()));
  for (const u32 r : h.levels()) MHGP11_TRY(level_words(file, levels[r].denominator()));
  MHGP11_TRY(reached(file, at.nodes_at));
  for (const ForestNode& node : nodes) {
    const u32 parent = idx(node.parent);
    MHGP11_TRY(file.u32s(std::span<const u32>(&parent, 1)));
  }
  MHGP11_TRY(file.pad8());
  for (const ForestNode& node : nodes) {
    const u32 rank = idx(node.rank);
    MHGP11_TRY(file.u32s(std::span<const u32>(&rank, 1)));
  }
  MHGP11_TRY(file.pad8());
  MHGP11_TRY(reached(file, at.hanging_at));
  for (const auto values : {h.t(), h.m(), h.q(), h.owner(), h.floor()}) MHGP11_TRY(column(file, values));
  MHGP11_TRY(file.bytes(h.strict()));
  MHGP11_TRY(file.pad8());
  MHGP11_TRY(reached(file, at.tree_at));
  for (const auto values : {pt.plateau_t(), pt.plateau_m(), pt.plateau_q(), pt.block_plateau(), pt.block_parent(),
                            pt.site_block(), pt.site_plateau()})
    MHGP11_TRY(column(file, values));
  return reached(file, at.size);
}

std::string points_manifest(const api::Product& product, const api::Provenance& provenance, u64 bytes,
                            const io::Digest& sha256) {
  const OrderTree& tree = product.order_tree();
  const points::PointHierarchy& h = product.points();
  std::string out = manifest_head(product, provenance, api::kPointsFileName, "MHGP11PT", bytes, sha256,
                                  api::tree_k_sha256(tree.domain(), tree.forest()));
  u64 roots = 0, delayed = 0, strict = 0;
  for (const u32 parent : h.tree().block_parent()) roots += parent == kNone;
  for (const u32 m : h.m()) delayed += m != 0;
  for (const u8 flag : h.strict()) strict += flag;
  const std::array<std::pair<std::string_view, u64>, 10> counts = {{
      {"sites", tree.domain().index().cloud().sites()},
      {"nodes", tree.forest().nodes().size()},
      {"qualification", h.qualification()},
      {"kappa", points::kKappa},
      {"levels", h.levels().size()},
      {"plateaus", h.tree().plateau_t().size()},
      {"blocks", h.tree().block_plateau().size()},
      {"root_blocks", roots},
      {"delayed", delayed},
      {"strict", strict},
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
