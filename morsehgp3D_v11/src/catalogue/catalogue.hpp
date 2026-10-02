// Catalogue critique positif : boites de centres T0, listes K-certifiees et supports locaux de 2 a 4 sites.
// Port explicite du generateur R2 865f5e6 (empreintes : source_pins.json), requalification v11 distincte.
// Identite d'une boule : (centre, rayon carre). S* est son support minimal canonique dans le Cloud source,
// pas une definition de l'identite geometrique. Les sites de niveau zero appartiennent a FULL, pas a ce catalogue.
#pragma once

#include <array>
#include <span>
#include <utility>

#include "cloud/cloud.hpp"
#include "num/num.hpp"

namespace mhgp11 {

namespace sched {
class Pool;
}

namespace catalogue_detail {
struct Assembly;
}

struct CatalogueParams {
  int kmax = 5;            // 1..12 ; K>nombre de sites reste un diagnostic admis
  u32 leaf_size = 32;      // K+3 <= leaf_size <= max_leaf
  u32 max_leaf = 256;      // capacite declaree, <=1024 ; au-dela : wide_leaf
  u64 max_nodes = 0;       // 0 : illimite, sous la limite implicite des compteurs u64
  u64 ball_limit = kNone;  // borne EXCLUSIVE sur le nombre de boules ; 1..kNone
  bool cache_center_lines = false;  // J2 memo exact borne a 32 sites ; repli direct au-dela
  bool indirect_sort = false;      // prototype de tri exact optionnel, qualification distincte
};

struct CatalogueBall {
  std::array<SiteIdx, 4> support{};  // S* croissant ; kNone dans les positions au-dela de qmin
  LevelRank rank{};                 // >=1 ; levels()[0] est toujours le niveau nul
  u32 p = 0, m = 0;                 // nombres de sites de l'interieur strict et de la coquille
  u8 qmin = 0;                      // 2..4 ; p+qmin <= kmax+1 ; tous les poids valent un
};

// Compteurs LOGIQUES d'une passe. La construction fait deux passes completes identiques ; son travail de
// generation est donc deux fois ces compteurs. Le tri/assemblage s'ajoute. Aucun temps ni pic n'est estime ici :
// le pilote mesure le temps de l'appel et le MemoryBudget (reservations preexistantes comprises).
struct CatalogueLedger {
  u64 nodes = 0, leaves = 0, filter_tests = 0, dominance_tests = 0;
  u64 prefixes = 0, judged = 0, census_tests = 0, emitted = 0, incidences = 0;
  // Q4 non degeneres avant positivite/propriete ; niveaux materialises apres admission canonique.
  u64 q4_candidates = 0, q4_levels = 0;
  // J2 : lectures de couples, demandes de droites ; rejets par prefixe, contacts conserves.
  u64 region_pair_tests = 0, region_pair_rejects = 0, region_line_tests = 0, region_line_rejects = 0;
  // tests=evaluations+cache_hits. Fallbacks : demandes avec option active et feuille de plus de 32 sites.
  u64 region_line_evaluations = 0, region_line_cache_hits = 0, region_line_fallbacks = 0;
  u64 max_leaf = 0, max_depth = 0;
  friend bool operator==(const CatalogueLedger&, const CatalogueLedger&) = default;
};

// Diagnostic optionnel non canonique. Intervalles murs disjoints, sans partition exhaustive du temps API.
// Allocations cumulees en trois etages. Aucune lecture d'horloge interne lorsque ce diagnostic est absent.
// Les sommes/maxima par tache sont distincts du mur du Pool : ne jamais les soustraire a ce dernier.
// Toute valeur est remise au caller seulement apres succes ; pas de transfert aux anciens benchmarks.
struct CatalogueTimings {
  u64 prefix_ns = 0, count_ns = 0, replay_ns = 0, fill_ns = 0;
  u64 sort_ns = 0, level_scan_ns = 0, allocation_ns = 0, assembly_ns = 0;
  u64 count_task_sum_ns = 0, count_task_max_ns = 0, fill_task_sum_ns = 0, fill_task_max_ns = 0;
  u64 sort_comparisons = 0;
  u32 tasks = 0;
};

// Proprietaire immuable des tableaux ; aucune vue d'un brouillon ne s'echappe. Les SiteIdx se rapportent au
// Cloud source : le catalogue n'emprunte pas ses octets, mais toute interpretation geometrique de ses indices
// exige ce meme Cloud. Le budget doit survivre au resultat. Construction deplacement seulement ; ni copie ni
// affectation. Le resultat deplace est vide (ses vues precedentes suivent le nouveau proprietaire).
class Catalogue {
 public:
  Catalogue(const Catalogue&) = delete;
  Catalogue& operator=(const Catalogue&) = delete;
  Catalogue& operator=(Catalogue&&) = delete;
  Catalogue(Catalogue&& other) noexcept
      : balls_(std::move(other.balls_)), levels_(std::move(other.levels_)), population_(std::move(other.population_)),
        kmax_(std::exchange(other.kmax_, 0)), ledger_(std::exchange(other.ledger_, {})) {}

  Order kmax() const noexcept { return kmax_; }
  u32 balls() const noexcept { return static_cast<u32>(balls_.size()); }
  std::span<const CatalogueBall> balls_data() const noexcept { return balls_.span(); }
  std::span<const num::Level> levels() const noexcept { return levels_.span(); }
  std::span<const u64> population_offsets() const noexcept { return population_.off.span(); }
  std::span<const SiteIdx> population() const noexcept { return population_.val.span(); }
  const CatalogueLedger& ledger() const noexcept { return ledger_; }
  // Exigent idx(b)<balls(). Chaque population est I croissant puis U croissante, disjoints.
  std::span<const SiteIdx> interior(BallIdx b) const noexcept {
    return population_.row(idx(b)).first(balls_[idx(b)].p);
  }
  std::span<const SiteIdx> shell(BallIdx b) const noexcept {
    return population_.row(idx(b)).subspan(balls_[idx(b)].p);
  }

 private:
  Catalogue() = default;
  friend struct catalogue_detail::Assembly;
  Buffer<CatalogueBall> balls_;
  Buffer<num::Level> levels_;
  Csr<SiteIdx> population_;
  Order kmax_ = 0;
  CatalogueLedger ledger_;
};

// Validation pure : K d'abord, puis tailles de feuille et ball_limit. Aucun calcul ni allocation.
[[nodiscard]] Outcome check_catalogue_params(const CatalogueParams& params) noexcept;

// Sequence de refus : parametres, multiplicites, ressources/calcul. Poids !=1 refuse avant allocation.
// Toute sortie reussie est CatK COMPLET ; plafond de feuille/noeuds/memoire = refus, jamais un prefixe publie.
// Toutes les capacites (listes DFS, feuille, emissions, tri, resultat) sont reservees dans budget. Pas de
// std::vector, index global, table de cles par candidat ou enumeration globale de quadruplets de secours.
// Le nuage et les parametres sont empruntes stables pendant cet appel synchrone. Le pilote de budget est unique.
[[nodiscard]] Result<Catalogue> build_catalogue(const Cloud& cloud, const CatalogueParams& params,
                                               MemoryBudget& budget) noexcept;

// Meme objet et memes compteurs logiques, avec une frontiere possedee et un Pool emprunte pendant l'appel.
// Admission conservatrice de tous les scratchs simultanes avant les workers ; aucun quota par worker.
// L'appel rejoint toutes les taches avant restitution ou publication. Le budget a toujours un seul pilote.
[[nodiscard]] Result<Catalogue> build_catalogue(const Cloud& cloud, const CatalogueParams& params,
                                               MemoryBudget& budget, sched::Pool& pool,
                                               CatalogueTimings* timings = nullptr) noexcept;

}  // namespace mhgp11
