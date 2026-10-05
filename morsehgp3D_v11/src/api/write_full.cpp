// Ecrivain MHGP11FUL1 de la sortie full (paragraphe 6.2 de la specification de la sortie parametree) : port OCTET POUR
// OCTET de serialize et birth_sphere (bench/full_probe.cpp, lignes 14 a 75) et des aides word et integer
// (bench/whole_input.hpp), au commit f98aeed67 ; docs/PROVENANCE.md, section api.
//
// Repris de la source : magie de 10 octets sans bourrage ; en-tete (bits, kmax, sites, poids) ; table des sites en
// ordre de Morton (x, y, z, poids, puis les PointId du site) ; par ordre : k, naissances, noeuds, aretes, racine, puis
// par noeud parent, debut et nombre d'enfants, niveau exact (numerateur, denominateur), centre exact des naissances
// (ancre * D + N par axe, puis D) et verticale aux ordres k > 1, puis les enfants. Tout mot est un u64
// petit-boutiste ; un entier exact est ecrit signe-magnitude (signe, nombre de mots, mots) avec le nombre de mots du
// type du profil (num::to_wide), jamais reduit : les memes types que la sonde donnent les memes octets.
// Ce qui change : io::FileWriter au lieu d'un std::ofstream (taille et empreinte au fil de l'ecriture, erreurs
// controlees, aucune exception), mots groupes par paquets de 512 sur la pile ; la premiere erreur d'ecriture est
// gardee et rendue a la fin. Lecteur : bench/full_semantic.py (et bench/mhgp11_formats.py).
// Porte : mhgp11_cli_full_identity (sha256 brut egal au dump de la sonde) ; mutant ecriture_full_permutee.
#include <span>

#include "api/internal.hpp"
#include "num/num.hpp"

namespace mhgp11::api_detail {

namespace {

// Mots u64 petit-boutistes par paquets sur la pile ; la premiere erreur est gardee (FileWriter refuse ensuite toute
// ecriture) et rendue par finish.
class Words {
 public:
  explicit Words(io::FileWriter& out) noexcept : out_(out) {}
  Words(const Words&) = delete;
  Words& operator=(const Words&) = delete;

  void word(u64 value) noexcept {
    if (count_ == buffer_.size()) flush();
    buffer_[count_++] = value;
  }
  // Entier exact : signe, nombre de mots du type du profil, mots (bench/whole_input.hpp, integer).
  template <class T>
  void integer(const T& value) noexcept {
    const auto wide = num::to_wide(value);
    word(wide.neg ? 1 : 0);
    word(wide.words.size());
    for (u64 w : wide.words) word(w);
  }
  [[nodiscard]] Outcome finish() noexcept {
    flush();
    return outcome_;
  }

 private:
  void flush() noexcept {
    if (count_ != 0 && outcome_.ok()) outcome_ = out_.u64s(std::span<const u64>(buffer_.data(), count_));
    count_ = 0;
  }

  io::FileWriter& out_;
  std::array<u64, 512> buffer_{};
  std::size_t count_ = 0;
  Outcome outcome_{};
};

// Sphere de naissance d'un noeud (bench/full_probe.cpp, birth_sphere) : le site a l'ordre 1, sinon la sphere passant
// par le support S* de la boule de naissance.
Result<num::Sphere> birth_sphere(const FullTower& tower, const OrderForest& forest, const ForestNode& node) noexcept {
  const auto& domain = tower.domain();
  const auto& cloud = domain.index().cloud();
  std::array<num::Point, 4> points{};
  if (forest.order() == 1) {
    const u32 site = node.birth_key;
    auto p = num::Point::make(cloud.x()[site], cloud.y()[site], cloud.z()[site]);
    if (!p.ok()) return p.outcome();
    return num::Sphere::point(p.value());
  }
  const auto& ball = domain.catalogue().balls_data()[node.birth_key];
  for (u32 j = 0; j < ball.qmin; ++j) {
    const u32 site = idx(ball.support[j]);
    auto p = num::Point::make(cloud.x()[site], cloud.y()[site], cloud.z()[site]);
    if (!p.ok()) return p.outcome();
    points[j] = p.value();
  }
  auto made = ball.qmin == 2   ? num::Sphere::through(points[0], points[1])
              : ball.qmin == 3 ? num::Sphere::through(points[0], points[1], points[2])
                               : num::Sphere::through(points[0], points[1], points[2], points[3]);
  if (!made.ok()) return made.outcome();
  if (!made.value()) return fail(Reason::tower_invariant);
  return *made.value();
}

// Un ordre de la tour : en-tete, noeuds, enfants.
Outcome write_order(Words& out, const FullTower& tower, u32 k) noexcept {
  const auto& domain = tower.domain();
  const auto& forest = tower.order(static_cast<Order>(k));
  out.word(k);
  out.word(forest.births());
  out.word(forest.nodes().size());
  out.word(forest.edges().size());
  out.word(idx(forest.root()));
  for (u32 i = 0; i < forest.nodes().size(); ++i) {
    const auto& node = forest.nodes()[i];
    const auto& level = domain.catalogue().levels()[idx(node.rank)];
    out.word(idx(node.parent));
    out.word(node.child_begin);
    out.word(node.child_count);
    out.integer(level.numerator());
    out.integer(level.denominator());
    if (i < forest.births()) {
      auto sphere = birth_sphere(tower, forest, node);
      if (!sphere.ok()) return sphere.outcome();
      const auto anchor = sphere.value().anchor();
      const i128 den = sphere.value().denominator();
      for (u32 axis = 0; axis < 3; ++axis)
        out.integer(i128{anchor.coordinates()[axis]} * den + sphere.value().numerator()[axis]);
      out.integer(den);
    }
    if (k > 1) out.word(idx(forest.lower()[i]));
  }
  for (NodeIdx child : forest.edges()) out.word(idx(child));
  return {};
}

}  // namespace

Outcome write_full(io::FileWriter& file, const FullTower& tower) noexcept {
  static constexpr std::string_view kMagic = "MHGP11FUL1";
  MHGP11_TRY(file.bytes(std::span<const u8>(reinterpret_cast<const u8*>(kMagic.data()), kMagic.size())));
  Words out(file);
  const auto& cloud = tower.domain().index().cloud();
  out.word(kCoordBits);
  out.word(tower.kmax());
  out.word(cloud.sites());
  out.word(cloud.weight());
  for (u32 s = 0; s < cloud.sites(); ++s) {
    out.word(cloud.x()[s]);
    out.word(cloud.y()[s]);
    out.word(cloud.z()[s]);
    out.word(cloud.w()[s]);
    for (PointId id : cloud.points(SiteIdx{s})) out.word(idx(id));
  }
  for (u32 k = 1; k <= tower.kmax(); ++k) {
    const Outcome order = write_order(out, tower, k);
    if (!order.ok()) return order;
  }
  return out.finish();
}

}  // namespace mhgp11::api_detail
