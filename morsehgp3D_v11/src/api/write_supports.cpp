// Ecrivain MHGP11SP version 2 de la sortie supports (docs/SORTIES.md, paragraphe 6 ; tranche S7, puis arbre couvrant
// d'ordre K du 6 octobre 2026). Ecrit a neuf, sans source portee : le format est celui du contrat, normatif. La
// hierarchie ecrite est l'arbre couvrant (supports::Selection::spanning) : naissances et fusions, un support S* par
// boule ; la version 2 retire donc la colonne support_count de la version 1 (toujours 1) et S = B.
//
// Petit-boutiste ; en-tete de 136 octets (magie "MHGP11SP", puis 16 mots u64 : version 2, coord_bits, k, n, N, root,
// B, S, Z, A, decalages de SITES, NODES, BALLS, SUPPORTS, PRIOR, taille totale) ; puis les sections dans cet ordre,
// colonne par colonne, chaque colonne commencant sur une frontiere de 8 octets, bourrage nul, une colonne vide
// n'occupant aucun octet :
//   SITES    x, y, z, point_id (u32[n]), lignes = SiteIdx (rang de Morton) ;
//   NODES    parent (kNone a la racine), rank (u32[N]), kind (u8[N] : 0 feuille de site a K = 1, 1 naissance de boule,
//            2 fusion), ball_count (u32[N], boules propres), numerotation canonique de la foret d'ordre K ;
//   BALLS    rank, prior_count (u32[B]), role (0 naissance, 1 fusion), p, m (u8[B]), dans l'ordre de la hierarchie
//            (postordre du noeud de rattachement, rang, puis S* lexicographique, egal a l'ordre des BallIdx) ;
//   SUPPORTS arity (u8[B] : qmin), puis sites (u32[Z]) : S* de chaque boule, dans l'ordre des boules ;
//   PRIOR    node (u32[A]) : ant(b) croissants, role fusion seulement.
// AUCUN compte n'est stocke (decision 4 de l'utilisateur) ; les tailles de colonnes et les decalages sont calcules
// avant l'ecriture, et l'ecriture les controle a chaque section (supports_invariant si l'arbre et la hierarchie se
// contredisent : gardes sans porte possible sur un produit calcule par compute). Valeurs ecrites par paquets sur la
// pile ; la premiere erreur d'ecriture est gardee et rendue (output_unwritable). Lecteur :
// bench/mhgp11_formats.py (read_supports). Portes : mhgp11_cli_supports_oracle, mhgp11_cli_supports_scale*,
// mhgp11_cli_supports_lidar_* ; mutants sp_* de tests/mutants/cli.json.
#include <span>

#include "api/internal.hpp"

namespace mhgp11::api_detail {

namespace {

constexpr u64 pad8(u64 bytes) noexcept { return (bytes + 7) & ~u64{7}; }

// Valeurs petit-boutistes d'une colonne, par paquets sur la pile ; la premiere erreur est gardee.
class Column {
 public:
  explicit Column(io::FileWriter& out) noexcept : out_(out) {}
  Column(const Column&) = delete;
  Column& operator=(const Column&) = delete;

  void u8v(u8 value) noexcept { put(value, 1); }
  void u32v(u32 value) noexcept { put(value, 4); }
  // Fin de colonne : vidange, puis bourrage nul jusqu'a la frontiere de 8 octets.
  [[nodiscard]] Outcome end() noexcept {
    flush();
    if (outcome_.ok()) outcome_ = out_.pad8();
    return outcome_;
  }

 private:
  void put(u64 value, std::size_t width) noexcept {
    if (count_ + width > buffer_.size()) flush();
    for (std::size_t b = 0; b < width; ++b) buffer_[count_++] = static_cast<u8>(value >> (8 * b));
  }
  void flush() noexcept {
    if (count_ != 0 && outcome_.ok()) outcome_ = out_.bytes(std::span<const u8>(buffer_.data(), count_));
    count_ = 0;
  }

  io::FileWriter& out_;
  std::array<u8, 8192> buffer_{};
  std::size_t count_ = 0;
  Outcome outcome_{};
};

// Dimensions et decalages du fichier (en-tete, mots 3 a 15).
struct Layout {
  u64 n = 0, nodes = 0, balls = 0, supports = 0, arities = 0, prior = 0;
  u64 sites_at = 0, nodes_at = 0, balls_at = 0, supports_at = 0, prior_at = 0, size = 0;

  void place() noexcept {
    sites_at = kHeaderBytes;
    nodes_at = sites_at + 4 * pad8(4 * n);
    balls_at = nodes_at + 3 * pad8(4 * nodes) + pad8(nodes);
    supports_at = balls_at + 2 * pad8(4 * balls) + 3 * pad8(balls);
    prior_at = supports_at + pad8(supports) + pad8(4 * arities);
    size = prior_at + pad8(4 * prior);
  }
  static constexpr u64 kHeaderBytes = 8 + 16 * 8;
};

// Genre d'un noeud (colonne kind) : feuille de site a K = 1, naissance de boule, fusion.
u8 node_kind(const OrderForest& forest, const ForestNode& node) noexcept {
  if (node.birth_key == kNone) return 2;
  return forest.order() == 1 ? 0 : 1;
}

// Ecriture d'un fichier : disposition, en-tete, puis une methode par section, chacune controlant sa fin.
struct SupportsWriter {
  SupportsWriter(io::FileWriter& f, const OrderTree& t, const supports::SupportHierarchy& hierarchy) noexcept
      : file(f), tree(t), h(hierarchy), out(f) {}
  io::FileWriter& file;
  const OrderTree& tree;
  const supports::SupportHierarchy& h;
  Column out;
  Layout at;

  Outcome layout() noexcept;
  Outcome header() noexcept;
  Outcome sites() noexcept;
  Outcome nodes() noexcept;
  Outcome balls_section() noexcept;
  Outcome supports_section() noexcept;
  Outcome prior() noexcept;
  // Fin de section : le fichier doit etre exactement au decalage annonce par l'en-tete.
  Outcome reached(u64 offset) const noexcept {
    return file.size() == offset ? Outcome{} : fail(Reason::supports_invariant);
  }
};

// Dimensions, controles de forme (gardes sans porte possible : la hierarchie vient de l'arbre de ce produit) et
// decalages.
Outcome SupportsWriter::layout() noexcept {
  const Cloud& cloud = tree.domain().index().cloud();
  const auto support_offsets = h.support_offsets();
  const auto prior_offsets = h.prior_offsets();
  at.n = cloud.sites();
  at.nodes = tree.forest().nodes().size();
  at.balls = h.balls().size();
  at.supports = h.supports().size();
  at.prior = h.prior().size();
  if (cloud.weight() != at.n || h.post().size() != at.nodes || h.ball_offsets().size() != at.nodes + 1 ||
      support_offsets.size() != at.balls + 1 || prior_offsets.size() != at.balls + 1 ||
      support_offsets[at.balls] != at.supports || prior_offsets[at.balls] != at.prior ||
      h.ball_offsets()[at.nodes] != at.balls || h.root() != tree.forest().root())
    return fail(Reason::supports_invariant);
  for (const supports::Support& support : h.supports()) {
    if (support.arity < 2 || support.arity > 4) return fail(Reason::supports_invariant);
    at.arities += support.arity;
  }
  // Arbre couvrant : un support par boule, et aucune liaison interne.
  if (at.supports != at.balls) return fail(Reason::supports_invariant);
  for (u64 b = 0; b < at.balls; ++b) {
    if (support_offsets[b + 1] - support_offsets[b] != 1 || h.balls()[b].role == BallRole::internal)
      return fail(Reason::supports_invariant);
  }
  at.place();
  return {};
}

Outcome SupportsWriter::header() noexcept {
  static constexpr std::string_view kMagic = "MHGP11SP";
  MHGP11_TRY(file.bytes(std::span<const u8>(reinterpret_cast<const u8*>(kMagic.data()), kMagic.size())));
  const OrderForest& forest = tree.forest();
  const std::array<u64, 16> words = {2,          static_cast<u64>(kCoordBits),
                                     forest.order(), at.n,
                                     at.nodes,   idx(forest.root()),
                                     at.balls,   at.supports,
                                     at.arities, at.prior,
                                     at.sites_at, at.nodes_at,
                                     at.balls_at, at.supports_at,
                                     at.prior_at, at.size};
  MHGP11_TRY(file.u64s(words));
  return reached(at.sites_at);
}

// SITES : x, y, z, point_id (un PointId par site : poids un).
Outcome SupportsWriter::sites() noexcept {
  const Cloud& cloud = tree.domain().index().cloud();
  for (const auto column : {cloud.x(), cloud.y(), cloud.z()}) {
    for (const u32 value : column) out.u32v(value);
    MHGP11_TRY(out.end());
  }
  for (u32 s = 0; s < at.n; ++s) out.u32v(idx(cloud.points(SiteIdx{s})[0]));
  MHGP11_TRY(out.end());
  return reached(at.nodes_at);
}

// NODES : parent, rank, kind, ball_count (boules propres, par le rang de postordre du noeud).
Outcome SupportsWriter::nodes() noexcept {
  const OrderForest& forest = tree.forest();
  for (const ForestNode& node : forest.nodes()) out.u32v(idx(node.parent));
  MHGP11_TRY(out.end());
  for (const ForestNode& node : forest.nodes()) out.u32v(idx(node.rank));
  MHGP11_TRY(out.end());
  for (const ForestNode& node : forest.nodes()) out.u8v(node_kind(forest, node));
  MHGP11_TRY(out.end());
  for (u64 v = 0; v < at.nodes; ++v) {
    const u32 j = h.post()[v];
    out.u32v(static_cast<u32>(h.ball_offsets()[j + 1] - h.ball_offsets()[j]));
  }
  MHGP11_TRY(out.end());
  return reached(at.balls_at);
}

// BALLS : rank, prior_count, role, p, m.
Outcome SupportsWriter::balls_section() noexcept {
  const auto balls = h.balls();
  const auto prior_offsets = h.prior_offsets();
  for (const supports::Ball& ball : balls) out.u32v(idx(ball.rank));
  MHGP11_TRY(out.end());
  for (u64 b = 0; b < at.balls; ++b) out.u32v(static_cast<u32>(prior_offsets[b + 1] - prior_offsets[b]));
  MHGP11_TRY(out.end());
  for (const supports::Ball& ball : balls) out.u8v(static_cast<u8>(ball.role));
  MHGP11_TRY(out.end());
  for (const supports::Ball& ball : balls) out.u8v(ball.p);
  MHGP11_TRY(out.end());
  for (const supports::Ball& ball : balls) out.u8v(ball.m);
  MHGP11_TRY(out.end());
  return reached(at.supports_at);
}

// SUPPORTS : arites, puis sites (croissants dans chaque support), boule par boule dans l'ordre de la hierarchie
// (arite, SiteIdx), donc S* en tete.
Outcome SupportsWriter::supports_section() noexcept {
  const auto support_offsets = h.support_offsets();
  const auto support_of = [&](u64 b, u64 j) -> const supports::Support& {
    return h.supports()[support_offsets[b] + j];
  };
  for (u64 b = 0; b < at.balls; ++b) {
    for (u64 j = 0; j < support_offsets[b + 1] - support_offsets[b]; ++j) out.u8v(support_of(b, j).arity);
  }
  MHGP11_TRY(out.end());
  for (u64 b = 0; b < at.balls; ++b) {
    for (u64 j = 0; j < support_offsets[b + 1] - support_offsets[b]; ++j) {
      const supports::Support& support = support_of(b, j);
      for (u32 i = 0; i < support.arity; ++i) out.u32v(idx(support.sites[i]));
    }
  }
  MHGP11_TRY(out.end());
  return reached(at.prior_at);
}

// PRIOR : ant(b), role fusion seulement.
Outcome SupportsWriter::prior() noexcept {
  for (const NodeIdx node : h.prior()) out.u32v(idx(node));
  MHGP11_TRY(out.end());
  return reached(at.size);
}

}  // namespace

Outcome write_supports(io::FileWriter& file, const OrderTree& tree, const supports::SupportHierarchy& h) noexcept {
  SupportsWriter writer(file, tree, h);
  MHGP11_TRY(writer.layout());
  MHGP11_TRY(writer.header());
  MHGP11_TRY(writer.sites());
  MHGP11_TRY(writer.nodes());
  MHGP11_TRY(writer.balls_section());
  MHGP11_TRY(writer.supports_section());
  return writer.prior();
}

}  // namespace mhgp11::api_detail
