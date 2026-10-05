// Supports positifs minimaux Q_b d'une boule du catalogue et fermeture de Q_b (lemme F de la sortie parametree).
//
// Port explicite de mark_supports et closure_counts du juge d'Euler, bench/catalogue_euler.hpp (commit 462dca187,
// sha256 f293df6ea1a2cc8d5fef6e78453de66343728a61d3013a5d1613cfdbb374dd18 ; docs/PROVENANCE.md, section du module
// supports). Garde : les trois boucles de positions croissantes et leurs predicats (is_midpoint ; strictly_acute puis
// orientation nulle du centre ; strictly_inside), l'ordre (arite, positions) qui place S* en tete, les masques de
// positions sur u32, la transformee de zeta en OU par mots et le decompte par poids. Change : chaque support est ECRIT
// au fil de l'enumeration, avant la fermeture, qui reecrit les memes mots (audit de4ab58a8 : les 6 supports du cube
// deviennent 177 parties apres la fermeture) ; la boule est controlee ici et non par l'appelant (BallIdx, plafond,
// tailles des tampons, niveau, premier support = S*) ; un ecart est un refus supports_invariant, jamais un compte de
// fautes ; les comptes N_j sont des u32 ; la coquille reguliere ne construit ni sphere ni predicat.
//
// Gardes SANS PORTE POSSIBLE (invariants du catalogue, G2 ; contre-lecture S6a, constat F3). Marquees "sans porte"
// ci-dessous : champs qmin et m hors de leur domaine (ball_supports) ; site hors du nuage (site_point) ; S* degenere,
// rang hors de la table des niveaux, niveau de la sphere de S* different de celui du catalogue (canonical_sphere) ;
// brouillon nul apres check_shell (defense en profondeur du plafond) et coquille de taille differente de m
// (extended_supports) ; premier support absent ou different de S* (lemme F, point 3) ; propagation des refus de num
// (Point::make, Sphere::through, orientation, strictly_inside), dont les budgets sont prouves a la compilation. Elles
// ne se declenchent que sur un domaine corrompu : FullDomain ne se construit que par prepare_full_domain, et aucun
// crochet n'est permis dans le produit (ARCHITECTURE.md, regle 6). Un mutant qui en retire une est equivalent sur tout
// domaine prepare : il survit par construction a toute porte (contre-lecture S6a : niveau_non_controle,
// premier_support_non_controle) et n'entre pas au manifeste. Ce qu'elles protegent est juge en amont par le juge
// d'Euler du catalogue (bench/catalogue_euler.hpp : well_formed, fautes degenerate, level et canonical ; portes
// mhgp11_catalogue_euler_*). Les autres refus de ce fichier ont leur porte (docs/PROVENANCE.md, section supports).
#include <algorithm>
#include <bit>

#include "supports/supports.hpp"

namespace mhgp11::supports {

namespace supports_detail {

// Seul constructeur des fermetures (Closure, counts.hpp).
struct Filler {
  // Coquille reguliere : U_b = S*, seule partie qui contient un support : N_j = [j = q].
  static Closure regular(u32 q) noexcept {
    Closure closure;
    closure.m_ = q;
    closure.n_[q] = 1;
    return closure;
  }

  // Decompte par cardinal des masques marques de words (fermes vers le haut) : N_j. Port de la seconde moitie de
  // closure_counts : le mot w porte les masques w * 64 + b, de cardinal popcount(w) + popcount(b).
  static Closure counted(std::span<const u64> words, u32 m) noexcept {
    static constexpr auto kWeight = [] {
      std::array<u64, 7> weight{};
      for (u32 b = 0; b < 64; ++b) weight[std::popcount(b)] |= u64{1} << b;
      return weight;
    }();
    Closure closure;
    closure.m_ = m;
    for (u64 w = 0; w < words.size(); ++w)
      if (words[w] != 0)
        for (u32 r = 0; r <= 6; ++r)
          closure.n_[static_cast<u32>(std::popcount(w)) + r] += static_cast<u32>(std::popcount(words[w] & kWeight[r]));
    return closure;
  }
};

}  // namespace supports_detail

namespace {

using supports_detail::Filler;

// Ecrit les supports trouves (positions croissantes de U_b) et marque leurs masques de positions.
struct Enumeration {
  std::span<const SiteIdx> shell;
  std::span<Support> out;
  std::span<u64> words;
  u32 count = 0;

  Outcome mark(const std::array<u32, 4>& at, u32 arity) noexcept {
    if (count == out.size()) return fail(Reason::supports_invariant);
    Support found;
    found.sites.fill(make_id<SiteIdx>(kNone));
    u32 mask = 0;
    for (u32 j = 0; j < arity; ++j) {
      found.sites[j] = shell[at[j]];
      mask |= u32{1} << at[j];
    }
    found.arity = static_cast<u8>(arity);
    out[count++] = found;
    words[mask >> 6] |= u64{1} << (mask & 63);
    return {};
  }
};

// Port de mark_supports : paires, triplets puis quadruplets de positions croissantes ; tous les sites sont sur la
// sphere. Lemme F : un triplet strictement aigu dont le plan contient le centre a ce centre pour centre circonscrit,
// interieur strict ; un quadruplet est un support si et seulement si le centre est strictement dans son tetraedre.
// Sans porte : les refus de orientation et strictly_inside (budgets de num prouves a la compilation) ; la capacite de
// out, elle, a sa porte (refusals).
Outcome enumerate(const num::Sphere& sphere, std::span<const num::Point> u, Enumeration& found,
                  SupportLedger& work) noexcept {
  const u32 m = static_cast<u32>(u.size());
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j) {
      ++work.midpoint_tests;
      if (num::is_midpoint(sphere, u[i], u[j])) MHGP11_TRY(found.mark({i, j, 0, 0}, 2));
    }
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      for (u32 k = j + 1; k < m; ++k) {
        ++work.acute_tests;
        if (!num::strictly_acute(u[i], u[j], u[k])) continue;
        ++work.orientation_tests;
        const auto plane = num::orientation(u[i], u[j], u[k], sphere);
        if (!plane.ok()) return plane.outcome();
        if (plane.value() == 0) MHGP11_TRY(found.mark({i, j, k, 0}, 3));
      }
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      for (u32 k = j + 1; k < m; ++k)
        for (u32 l = k + 1; l < m; ++l) {
          ++work.inside_tests;
          const auto inside = num::strictly_inside(sphere, u[i], u[j], u[k], u[l]);
          if (!inside.ok()) return inside.outcome();
          if (inside.value()) MHGP11_TRY(found.mark({i, j, k, l}, 4));
        }
  return {};
}

// Port de la premiere moitie de closure_counts : fermeture vers le haut (zeta en OU) des masques marques. Le bit b du
// mot w est le masque w * 64 + b ; les six bits bas d'un masque se ferment dans le mot, les autres entre mots.
void zeta_or(std::span<u64> words, u32 m) noexcept {
  static constexpr std::array<u64, 6> kLow{0x5555555555555555ull, 0x3333333333333333ull, 0x0F0F0F0F0F0F0F0Full,
                                           0x00FF00FF00FF00FFull, 0x0000FFFF0000FFFFull, 0x00000000FFFFFFFFull};
  for (u32 i = 0; i < std::min<u32>(m, 6); ++i)
    for (u64 w = 0; w < words.size(); ++w) words[w] |= (words[w] & kLow[i]) << (1u << i);
  for (u32 i = 6; i < m; ++i)
    for (u64 w = 0; w < words.size(); ++w)
      if (((w >> (i - 6)) & 1) != 0) words[w] |= words[w ^ (u64{1} << (i - 6))];
}

// Sans porte : un SiteIdx du catalogue est un rang du nuage, et ses coordonnees sont dans le domaine de Point.
Result<num::Point> site_point(const Cloud& cloud, SiteIdx site) noexcept {
  const u32 i = idx(site);
  if (i >= cloud.sites()) return fail(Reason::supports_invariant);
  return num::Point::make(cloud.x()[i], cloud.y()[i], cloud.z()[i]);
}

// Sphere de la boule refaite depuis S* (arite qmin), de niveau exactement egal a celui du catalogue. Sans porte : S*
// est affinement independant, son rang est dans la table et son niveau est celui de ce rang (catalogue, G2) ;
// Sphere::through ne refuse pas des points du profil.
Result<num::Sphere> canonical_sphere(const FullDomain& domain, const CatalogueBall& data) noexcept {
  const Catalogue& cat = domain.catalogue();
  std::array<num::Point, 4> v{};
  for (u32 j = 0; j < data.qmin; ++j) {
    const auto point = site_point(domain.index().cloud(), data.support[j]);
    if (!point.ok()) return point.outcome();
    v[j] = point.value();
  }
  const auto made = data.qmin == 2   ? num::Sphere::through(v[0], v[1])
                    : data.qmin == 3 ? num::Sphere::through(v[0], v[1], v[2])
                                     : num::Sphere::through(v[0], v[1], v[2], v[3]);
  if (!made.ok()) return made.outcome();
  if (!made.value() || idx(data.rank) >= cat.levels().size()) return fail(Reason::supports_invariant);
  if (num::compare(made.value()->level(), cat.levels()[idx(data.rank)]) != 0) return fail(Reason::supports_invariant);
  return *made.value();
}

Result<BallSupports> regular_supports(const CatalogueBall& data, std::span<Support> out,
                                      SupportLedger* ledger) noexcept {
  if (out.empty()) return fail(Reason::supports_invariant);
  out[0] = Support{data.support, data.qmin};
  BallSupports result;
  result.count = 1;
  result.closure = Filler::regular(data.qmin);
  if (ledger != nullptr) ledger->add(SupportLedger{1, 1, 0, 1, 0, 0, 0, 0});
  return result;
}

Result<BallSupports> extended_supports(const FullDomain& domain, BallIdx ball, const CatalogueBall& data,
                                       std::span<Support> out, std::span<u64> scratch,
                                       SupportLedger* ledger) noexcept {
  const u32 m = data.m;
  const u64 words = closure_words(m);
  const auto shell = domain.catalogue().shell(ball);
  // Brouillon trop court : porte refusals. Sans porte : words == 0 (exclu par check_shell) et shell.size() != m.
  if (words == 0 || scratch.size() < words || shell.size() != m) return fail(Reason::supports_invariant);
  std::array<num::Point, kMaxShell> u{};
  for (u32 i = 0; i < m; ++i) {
    const auto point = site_point(domain.index().cloud(), shell[i]);
    if (!point.ok()) return point.outcome();
    u[i] = point.value();
  }
  const auto sphere = canonical_sphere(domain, data);
  if (!sphere.ok()) return sphere.outcome();
  const std::span<u64> marks = scratch.first(words);
  std::fill(marks.begin(), marks.end(), u64{0});
  SupportLedger work{1, 0, 1, 0, 0, 0, 0, 0};
  Enumeration found{shell, out, marks};
  MHGP11_TRY(enumerate(sphere.value(), std::span<const num::Point>(u.data(), m), found, work));
  // Le premier support doit etre S* : meme arite qmin, memes sites (lemme F, point 3). Sans porte : S* canonique.
  if (found.count == 0 || !(out[0] == Support{data.support, data.qmin})) return fail(Reason::supports_invariant);
  work.supports = found.count;
  zeta_or(marks, m);
  BallSupports result;
  result.count = found.count;
  result.closure = Filler::counted(marks, m);
  if (ledger != nullptr) ledger->add(work);
  return result;
}

}  // namespace

Outcome check_shell(u32 m) noexcept {
  if (m > kMaxShell) return fail(Reason::support_shell_capacity);
  return {};
}

Result<BallSupports> ball_supports(const FullDomain& domain, BallIdx ball, std::span<Support> out,
                                   std::span<u64> scratch, SupportLedger* ledger) noexcept {
  const Catalogue& cat = domain.catalogue();
  if (idx(ball) >= cat.balls()) return fail(Reason::parameter_out_of_range);
  const CatalogueBall& data = cat.balls_data()[idx(ball)];
  // Sans porte : 2 <= qmin <= 4 et m >= qmin pour toute boule du catalogue.
  if (data.qmin < 2 || data.qmin > 4 || data.m < data.qmin) return fail(Reason::supports_invariant);
  if (data.m == data.qmin) return regular_supports(data, out, ledger);
  MHGP11_TRY(check_shell(data.m));
  return extended_supports(domain, ball, data, out, scratch, ledger);
}

}  // namespace mhgp11::supports
