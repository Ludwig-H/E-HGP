// En-tete public de supports : supports positifs minimaux Q_b des boules du catalogue et comptes exacts du lemme G
// (sortie parametree, tranche S6). Un autre module n'inclut que ce fichier. L'assemblage (postordre, tri des boules,
// SupportHierarchy) viendra avec le rattachement de la tour (WindowAttachment, tranche S3).
//
// Q_b = { Q inclus dans U_b : Q affinement independant, c_b dans l'interieur relatif de conv(Q), 2 <= |Q| <= 4 } : les
// parties non separables MINIMALES de la coquille (lemme F, Caratheodory strict ; docs/MATHEMATIQUES.md, M1). Q_b est
// enumere sur TOUTE la coquille, jamais limite a qmin ni filtre par un compte de cofaces, par les trois predicats
// stricts publics de num : is_midpoint (|Q| = 2), strictly_acute puis orientation nulle du centre (|Q| = 3),
// strictly_inside (|Q| = 4). Aucun test d'independance ni de minimalite n'est necessaire : un triangle droit n'est
// pas un support (son hypotenuse l'est), et le drapeau q4_presentation_strictly_inside de num::Sphere ne vaut que pour
// le tetraedre generateur, jamais pour un autre quadruplet. Ordre publie : (arite, ordre lexicographique des SiteIdx),
// donc S* en tete (son plus petit cardinal est qmin).
#pragma once

#include <array>
#include <span>

#include "num/num.hpp"
#include "supports/counts.hpp"
#include "tower/tower.hpp"

namespace mhgp11::supports {

// Support positif minimal d'une boule : SiteIdx croissants, kNone au-dela de l'arite.
struct Support {
  std::array<SiteIdx, 4> sites{};
  u8 arity = 0;  // 2..4
  friend bool operator==(const Support&, const Support&) = default;
};

// Nombre de parties de 2 a 4 sites d'une coquille de m <= kMaxShell sites : majorant de |Q_b|.
constexpr u32 support_capacity(u32 m) noexcept {
  const i32 x = static_cast<i32>(m);
  return supports_detail::binomial(x, 2) + supports_detail::binomial(x, 3) + supports_detail::binomial(x, 4);
}
inline constexpr u32 kMaxSupports = support_capacity(kMaxShell);
static_assert(kMaxSupports == 12926, "supports : C(24,2) + C(24,3) + C(24,4)");

// Mots u64 du brouillon de fermeture d'une coquille etendue de m sites : 2^(m-6), un seul si m <= 6 ; 0 au-dela du
// plafond, qui n'a pas de brouillon (fonction totale : aucun decalage hors domaine).
constexpr u64 closure_words(u32 m) noexcept { return m > kMaxShell ? 0 : u64{1} << (m > 6 ? m - 6 : 0); }
inline constexpr u64 kMaxClosureWords = closure_words(kMaxShell);
static_assert(kMaxClosureWords * sizeof(u64) == (u64{2} << 20), "supports : brouillon de 2 Mio a m = 24");

// Plafond des coquilles etendues : refus support_shell_capacity si m > kMaxShell. Seul controle du plafond :
// ball_supports, make_shape et les pre-passes d'un appel entier l'appellent avant tout calcul.
[[nodiscard]] Outcome check_shell(u32 m) noexcept;

// Travail cumule par ball_supports, sur succes seulement (mesure et registres ; jamais une decision). Aucune
// protection contre la concurrence : add n'est pas atomique, un registre partage entre fils est une course. Un
// registre par fil, sommes par add apres la jointure (porte concurrency).
struct SupportLedger {
  u64 balls = 0, regular = 0, extended = 0, supports = 0;
  u64 midpoint_tests = 0, acute_tests = 0, orientation_tests = 0, inside_tests = 0;
  constexpr void add(const SupportLedger& part) noexcept {
    balls += part.balls;
    regular += part.regular;
    extended += part.extended;
    supports += part.supports;
    midpoint_tests += part.midpoint_tests;
    acute_tests += part.acute_tests;
    orientation_tests += part.orientation_tests;
    inside_tests += part.inside_tests;
  }
  friend bool operator==(const SupportLedger&, const SupportLedger&) = default;
};

struct BallSupports {
  u32 count = 0;    // |Q_b|, ecrits dans out[0, count)
  Closure closure;  // N_0 .. N_m
};

// Q_b d'une boule du catalogue du domaine, ecrit dans out[0, count) dans l'ordre publie (out[0] = S*), et sa fermeture.
// Coquille reguliere (m = qmin) : {S*} et N_j = [j = qmin], sans sphere ni predicat. Coquille etendue : plafond
// (check_shell) ; sphere refaite depuis S* (Sphere::through de l'arite qmin) et niveau exactement egal a celui du
// catalogue ; paires, triplets puis quadruplets de positions de U_b ; premier support egal a S* ; fermeture zeta en OU
// dans scratch[0, closure_words(m)), puis N_j. Q_b ne depend pas de K : aucun support n'est retire.
// Refus, avant toute ecriture pour les deux premiers : parameter_out_of_range (BallIdx hors du catalogue) ;
// support_shell_capacity (m > kMaxShell) ; supports_invariant (scratch trop court, plus de out.size() supports ; puis,
// gardes sans porte possible sur un domaine prepare, voir enumerate.cpp : champs du catalogue incoherents, S*
// degenere, niveau different, premier support different de S*) ; refus arithmetiques de num (sans porte possible
// non plus). Aucune allocation : out et scratch appartiennent a l'appelant (un brouillon par fil), leur contenu est
// indetermine apres un refus. Le domaine est lu en partage : appels concurrents permis sur des tampons distincts et
// des registres distincts. Le registre facultatif n'est pas protege (SupportLedger) : un registre par fil, sommes par
// SupportLedger::add apres la jointure, jamais un registre partage entre fils.
[[nodiscard]] Result<BallSupports> ball_supports(const FullDomain& domain, BallIdx ball, std::span<Support> out,
                                                 std::span<u64> scratch, SupportLedger* ledger = nullptr) noexcept;

// Forme d'une boule du domaine a l'ordre K, par make_shape (counts.hpp). Refus : parameter_out_of_range (BallIdx hors
// du catalogue) ; puis ceux de make_shape : support_shell_capacity (m > kMaxShell, par check_shell) et
// supports_invariant (forme hors du domaine de Shape : K hors de 1..kMaxOrder, ou boule hors de Cat_K a cet ordre,
// p + qmin > K + 1 ; les autres clauses de ce domaine sont des invariants du catalogue).
[[nodiscard]] Result<Shape> ball_shape(const FullDomain& domain, BallIdx ball, Order k) noexcept;

}  // namespace mhgp11::supports
