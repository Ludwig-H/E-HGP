// En-tete public de head : sortie plate certifiee tiree de la hierarchie de points (tranche S10 de la sortie
// parametree ; docs/SORTIE_PLATE.md ; docs/SORTIES.md, paragraphe 7). Un autre module n'inclut que ce fichier.
//
// PORT EXPLICITE, decision par decision, de la tete Python qualifiee (docs/PROVENANCE.md, section S10) :
// bench/points_flat.py, condense (critere A), select (EOM N-aire ou feuilles), labels, et l'arithmetique exacte de
// repli (RadSum.sign, Level.phi_exact, _inverse_date, _mask_mul). Ce qui change :
//   - le filtre flottant a borne d'erreur de select est remplace par des intervalles ENTIERS en virgule fixe : pour
//     chaque plateau, E = 2^64 e est encadre par les racines floor(2^64 sqrt(l)) des niveaux (num::RootTable), puis
//     2^192 phi = 2^(192 + 64 z) / E^z est encadre par divisions entieres ; scores et sommes en entiers larges.
//     Aucune decision flottante (docs/ARCHITECTURE.md, regle F1) ;
//   - si l'encadrement ne separe pas, le repli exact somme les poids ENTIERS par plateau du score propre moins les
//     scores retenus des enfants (les plateaux communs se compensent avant tout radical), puis developpe chaque
//     plateau en radicaux et tranche par num::RadicalSum (classes de carres, encadrements) : 0 seulement sur egalite
//     certifiee, le parent l'emporte alors ; au-dela de kExactTerms termes ou du budget de precision, refus
//     radical_sign_budget de l'APPEL ENTIER (reponse de l'auditeur, 6eba951df), jamais un choix force ;
//   - les etiquettes sont rendues dans l'ordre du fichier d'entree, sans supposer des PointId denses : le plus petit
//     PointId du cluster retenu, ou -1 pour le bruit.
//
// Conventions de la tete Python gardees : un bloc est gros s'il compte au moins mcs sites engages (mcs >= 2) ; un
// cluster vivant continue, deux ou plus meurent et un parent nait (jamais binarise), aucun : un cluster nait ; les
// sites des petits blocs absorbes et les entrees rejoignent le cluster au plateau ; S(C) = somme des phi(sortie) - |C|
// phi(haut), phi(r) = r^-z ; racine exclue ; foret : racine virtuelle au niveau infini (phi = 0). Un phi(0) demande
// (jonction au niveau nul) est un head_invariant ; mcs >= 2 l'ecarte des feuilles a zero de K = 1.
#pragma once

#include <span>

#include "core/core.hpp"
#include "num/num.hpp"

namespace mhgp11::points {
class PointHierarchy;
}

namespace mhgp11::head {

enum class Selection : u8 { eom = 0, leaves = 1 };

struct FlatParams {
  u32 mcs = 20;
  u32 z = 1;  // 1, 2 ou 3
  Selection selection = Selection::eom;
};

// Plafond des termes radicaux d'une decision exacte (4 par plateau de date au plus) : 4096 termes, soit
// num::RadicalSum::bytes(kExactTerms) octets admis le temps de la decision seulement.
inline constexpr u32 kExactTerms = 4096;

// Compteurs d'un appel (manifeste) ; jamais une decision.
struct FlatStats {
  u64 clusters = 0;     // clusters condenses, racine virtuelle comprise
  u64 decisions = 0;    // comparaisons EOM
  u64 exact = 0;        // decisions tranchees par le repli exact
  u64 equalities = 0;   // egalites certifiees (le parent l'emporte)
  u64 unbracketed = 0;  // plateaux sans encadrement utile (date trop petite) : repli exact force s'ils comptent
  u64 selected = 0;     // clusters retenus
  u64 noise = 0;        // points etiquetes -1
};

// Source des niveaux d'un arbre de points : rang -> niveau carre exact l >= 0 (rang = indice de la table du
// catalogue pour le produit, d'une table de rationnels pour les fixtures abstraites).
class LevelSource {
 public:
  virtual ~LevelSource() = default;
  // R = floor(2^64 sqrt(l)) ; refus arithmetic_invariant hors de u128.
  [[nodiscard]] virtual Outcome root(u32 rank, u128& out) const noexcept = 0;
  // l exact, reduit.
  [[nodiscard]] virtual Outcome value(u32 rank, num::Rational& out) const noexcept = 0;
};

// Niveaux du catalogue (num::Level, valeurs non reduites).
class CatalogueLevels final : public LevelSource {
 public:
  explicit CatalogueLevels(std::span<const num::Level> levels) noexcept : levels_(levels) {}
  [[nodiscard]] Outcome root(u32 rank, u128& out) const noexcept override;
  [[nodiscard]] Outcome value(u32 rank, num::Rational& out) const noexcept override;

 private:
  std::span<const num::Level> levels_;
};

// Vue d'un arbre de points a plateaux atomiques (forme de points::PointTree) : plateaux strictement croissants, de
// valeur sqrt(l_t) + sqrt(l_m) - sqrt(l_q) (m = q : niveau sqrt(l_t)) ; blocs crees par une fusion (parent kNone a une
// racine, enfants : les blocs dont il est le parent) ou par une entree ; un plateau et un bloc d'entree par site.
// site_ids : PointId de chaque site, qui fixe l'identifiant canonique d'un cluster.
struct TreeView {
  std::span<const u32> plateau_t, plateau_m, plateau_q;
  std::span<const u32> block_plateau, block_parent;
  std::span<const u32> site_block, site_plateau;
  std::span<const PointId> site_ids;
};

// Etiquettes par site (indice de site de la vue) : plus petit PointId du cluster retenu, -1 pour le bruit.
struct SiteLabels {
  Buffer<i64> labels;
  FlatStats stats;
};

// Condensation, scores et selection sur une vue. Refus : parameter_out_of_range (mcs < 2, z hors de {1, 2, 3}),
// memory_budget, radical_sign_budget, arithmetic_invariant, head_invariant (forme de l'arbre, masse decroissante,
// phi(0) demande, date non positive). Aucun resultat partiel.
[[nodiscard]] Result<SiteLabels> flat_sites(const TreeView& tree, const LevelSource& levels, FlatParams params,
                                            MemoryBudget& budget) noexcept;

// Etiquettes dans l'ordre du fichier d'entree : input_ids[i] est le PointId du i-eme point lu, chaque PointId d'un
// site present exactement une fois (sinon head_invariant).
[[nodiscard]] Result<Buffer<i64>> in_input_order(std::span<const i64> site_labels, std::span<const PointId> site_ids,
                                                 std::span<const PointId> input_ids, MemoryBudget& budget) noexcept;

// Etiquettes d'une hierarchie de points, dans l'ordre d'entree. levels : niveaux du catalogue Cat_K de l'arbre ;
// site_ids : PointId par SiteIdx.
struct FlatLabels {
  Buffer<i64> labels;
  FlatStats stats;
};
[[nodiscard]] Result<FlatLabels> flat(const points::PointHierarchy& hierarchy, std::span<const num::Level> levels,
                                      std::span<const PointId> site_ids, std::span<const PointId> input_ids,
                                      FlatParams params, MemoryBudget& budget) noexcept;

}  // namespace mhgp11::head
