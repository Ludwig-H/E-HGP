// En-tete public du module catalogue (tranche T1 de la v12, docs/CONTRAT_CATALOGUE.md) : le catalogue critique
// POSITIF Cat_K (q = q_min >= 2, p + q_min <= K + 1) d'un nuage, voie CPU de reference.
//
// Algorithme (changements declares d'avance, CONTRAT_CATALOGUE.md, paragraphe 2) : parcours des boites de centres EN
// LARGEUR (source de MES-M5, warp simule sur l'hote, warps repartis sur le Pool) ; feuilles J3 (source de MES-M2)
// consommees en flux, niveau par niveau, par lots repartis sur les fils ; une feuille d'etendue s <= 16 est jouee en
// arithmetique native, une feuille plus etendue par la MEME source en arithmetique exacte plus large (repli exact) ;
// une feuille de plus de 32 sites (au plus max_leaf = 256) par la meme source sur un warp virtuel de 256 voies ; fin
// d'etage commune aux voies CPU et appareil (une source, deux executeurs : Pool et CUDA) : ordre canonique (niveau
// exact, puis S*) par tri par base des cles F3 et des positions de S* avec verification exacte des voisins non
// certainement ordonnes, rangs de niveau et CSR des populations (I puis U) par sommes prefixes, table S* -> boule
// (LEM-T1) par tri par base. Le catalogue est le meme ensemble que celui de la v11 gelee (ac081a06f), a un ecart declare
// pres : S* se departage par la liste triee des POSITIONS de ses sites (ordre lexicographique des coordonnees), non par
// les rangs de Morton (CST-0113) ; l'ordre canonique des boules de meme niveau suit la meme regle (ARCHITECTURE.md,
// paragraphe 1, regle 5).
//
// Capacite (CONTRAT_CATALOGUE.md, paragraphe 4) : chaque lot de feuilles compte exactement ses boules et incidences
// avant de les ecrire ; tableaux du front, lots et fin d'etage sont des Buffer du budget ; un depassement rend
// memory_budget sans rien publier. Domaines : sites, boules et feuilles sur 32 bits avec refus a la vraie limite
// (index_overflow_u32), decalages et compteurs sur 64 bits. Refus explicites : parametres (kmax_out_of_range,
// parameter_out_of_range), multiplicites (multiplicity_unsupported, decision D8), feuille plus large que max_leaf
// (wide_leaf), coquille d'une boule emise au-dela de 64 sites (shell_capacity, WIT-SPHERE50). Jamais un prefixe publie.
#pragma once

#include <array>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "cloud/cloud.hpp"
#include "io/io.hpp"
#include "num/num.hpp"

namespace mhgp12 {

namespace sched {
class Pool;
}
namespace catalogue_detail {
struct Assembly;
}

inline constexpr u32 kCatalogueMaxLeaf = 256;  // feuilles de plus de 256 sites : wide_leaf (capacite de la v11)
inline constexpr u32 kCatalogueMaxShell = 64;  // coquille d'une boule emise : au plus 64 sites (shell_capacity)

struct CatalogueParams {
  int kmax = 5;                         // 1..12
  u32 leaf_size = 24;                   // K+3 <= leaf_size <= max_leaf
  u32 max_leaf = kCatalogueMaxLeaf;     // leaf_size <= max_leaf <= 256 ; au-dela : wide_leaf
};

struct CatalogueBall {
  std::array<SiteIdx, 4> support{};  // S* en SiteIdx croissants ; kNone au-dela de qmin
  LevelRank rank{};                  // >= 1 ; levels()[0] est le niveau nul
  u32 p = 0, m = 0;                  // interieur strict et coquille
  u8 qmin = 0;                       // 2..4
};

// Compteurs LOGIQUES (contrat de la v11, champ par champ) : parcours (noeuds, feuilles, tests G1, maxima) et quinze
// compteurs de feuille. Independants de l'ordre de visite, de la voie et du nombre de fils ; une feuille compte une
// fois, apres succes.
struct CatalogueLedger {
  u64 nodes = 0, leaves = 0, filter_tests = 0, dominance_tests = 0;
  u64 prefixes = 0, judged = 0, census_tests = 0, emitted = 0, incidences = 0;
  u64 q4_candidates = 0, q4_levels = 0;
  u64 region_pair_tests = 0, region_pair_rejects = 0, region_line_tests = 0, region_line_rejects = 0;
  u64 region_line_evaluations = 0, region_line_cache_hits = 0, region_line_fallbacks = 0;
  u64 max_leaf = 0, max_depth = 0;
  friend bool operator==(const CatalogueLedger&, const CatalogueLedger&) = default;
};

// Diagnostics PHYSIQUES (jamais dans une empreinte) : niveaux et taches du parcours, feuilles par palier d'etendue de
// leur repere (etroit s <= 16, moyen s <= 24, large) et par voie, feuilles rejouees a l'ecriture, durees par etape ;
// fin d'etage : chaines de voisins incertains retriees en exact (repli) et leurs elements.
struct CatalogueDiagnostics {
  u64 levels = 0, tasks = 0, candidates = 0;
  u64 leaves_narrow = 0, leaves_medium = 0, leaves_wide = 0, leaves_exact = 0, leaves_virtual_warp = 0;
  u64 leaves_rewritten = 0, max_leaf_span = 0;
  u64 traversal_ns = 0, count_ns = 0, fill_ns = 0, levels_ns = 0, sort_ns = 0, assemble_ns = 0, table_ns = 0;
  u64 peak_bytes = 0;
  u64 chains_repaired = 0, chain_elements = 0;
  // Voie appareil seulement (nuls sur la voie CPU), voie HYBRIDE : l'appareil joue les feuilles d'au plus 32 sites et
  // d'etendue locale d'au plus 16 bits ; les autres (non resolues) sont rapatriees et rejouees EXACTEMENT sur l'hote
  // (LeafStage), puis remontees. Lots de feuilles ; reprises sur l'hote (feuilles, boules, par cause : plus de 32
  // sites, etendue au-dela de 16 ; une feuille peut avoir les deux) ; reecritures (feuille de plus de 64 emissions
  // rejouee par la meme source) sur l'appareil et dans la reprise de l'hote, dont la somme est leaves_rewritten (meme
  // sens que la voie CPU) ; octets de l'appareil et de la memoire epinglee reserves, reservations de l'appel, octets
  // de l'arene ; transferts du raccord complet (TOUTE copie hote <-> appareil de l'appel : duree, octets par sens,
  // operations) ; publication (niveaux materialises sur l'hote). Durees disjointes et nettes des transferts
  // (CST-0235) : count_ns porte les feuilles (classement, comptage J3, decalages), fill_ns l'emission (reprise des
  // non resolues, admission, ecriture). Tranche T2-d (sorties en flux) : les sorties passent par la memoire epinglee en
  // tranches (stream_chunks) et les niveaux sont materialises au fil du flux (publish_ns) ; preparation des sorties
  // (outputs_ns, outputs_bytes : Buffer hote reserves des que leur taille exacte est connue et pages touchees pendant
  // le calcul de l'appareil), duree disjointe des autres.
  u64 batches = 0, replayed_leaves = 0, replayed_balls = 0, replayed_wide = 0, replayed_span = 0;
  u64 rewritten_device = 0, rewritten_host = 0, device_bytes = 0, pinned_bytes = 0, allocations = 0;
  u64 arena_bytes = 0, transfer_ns = 0, transfer_h2d_bytes = 0, transfer_d2h_bytes = 0, transfer_ops = 0;
  u64 publish_ns = 0, outputs_ns = 0, outputs_bytes = 0, stream_chunks = 0;
  // Tranche T1-d (catalogue en flux) : tranches de la fin d'etage par tranches de cles (0 : voie complete) ; lots de
  // feuilles dont l'arene a ete rapatriee sur l'hote (voie appareil en flux ; 0 : arene residente).
  u64 finish_slices = 0, arena_streamed = 0;
};

// Proprietaire immuable des tableaux du catalogue ; ses SiteIdx se rapportent au Cloud source. Construction
// deplacement seulement ; le budget doit survivre au resultat.
class Catalogue {
 public:
  Catalogue(const Catalogue&) = delete;
  Catalogue& operator=(const Catalogue&) = delete;
  Catalogue& operator=(Catalogue&&) = delete;
  Catalogue(Catalogue&& other) noexcept
      : balls_(std::move(other.balls_)), levels_(std::move(other.levels_)), population_(std::move(other.population_)),
        table_(std::move(other.table_)), kmax_(std::exchange(other.kmax_, 0)),
        ledger_(std::exchange(other.ledger_, {})) {}

  Order kmax() const noexcept { return kmax_; }
  u32 balls() const noexcept { return static_cast<u32>(balls_.size()); }
  std::span<const CatalogueBall> balls_data() const noexcept { return balls_.span(); }
  std::span<const num::Level> levels() const noexcept { return levels_.span(); }
  std::span<const u64> population_offsets() const noexcept { return population_.off.span(); }
  std::span<const SiteIdx> population() const noexcept { return population_.val.span(); }
  const CatalogueLedger& ledger() const noexcept { return ledger_; }
  // Exigent idx(b) < balls(). I croissant puis U croissante, disjoints.
  std::span<const SiteIdx> interior(BallIdx b) const noexcept {
    return population_.row(idx(b)).first(balls_[idx(b)].p);
  }
  std::span<const SiteIdx> shell(BallIdx b) const noexcept { return population_.row(idx(b)).subspan(balls_[idx(b)].p); }
  // Table S* -> boule (certificats LEM-T1 de la tranche T2) : la boule dont S* est EXACTEMENT ce support (2 a 4
  // SiteIdx croissants), sinon rien. Un support minimal non canonique d'une boule n'y est pas (WIT-T1-CARRE).
  std::optional<BallIdx> find_support(std::span<const SiteIdx> support) const noexcept;

 private:
  Catalogue() = default;
  friend struct catalogue_detail::Assembly;
  Buffer<CatalogueBall> balls_;
  Buffer<num::Level> levels_;
  Csr<SiteIdx> population_;
  Csr<BallIdx> table_;  // ligne s : boules dont S* commence par le site s, rangees par (S*[1], S*[2], S*[3])
  Order kmax_ = 0;
  CatalogueLedger ledger_;
};

// Validation pure : K, puis tailles de feuille. Aucun calcul ni allocation.
[[nodiscard]] Outcome check_catalogue_params(const CatalogueParams& params) noexcept;

// Refus dans cet ordre : parametres, nuage vide, multiplicites, ressources et calcul. Toute sortie reussie est Cat_K
// COMPLET ; un refus ne publie rien. Le nuage, les parametres et le Pool sont empruntes pendant l'appel ; le budget a
// un seul pilote. diagnostics, s'il est donne, n'est rempli qu'au succes.
[[nodiscard]] Result<Catalogue> build_catalogue(const Cloud& cloud, const CatalogueParams& params,
                                               MemoryBudget& budget, sched::Pool& pool,
                                               CatalogueDiagnostics* diagnostics = nullptr) noexcept;

// Voie appareil du catalogue (tranche T1-b) : contexte resident de l'appareil (flux CUDA, tableaux de l'appareil,
// memoire epinglee), ouvert une fois et reutilise d'un appel a l'autre (decision D1, regime a chaud). Toutes ses
// reservations, appareil et memoire epinglee, sont comptees dans le budget donne a l'ouverture, qui doit lui
// survivre ; il les garde jusqu'a sa destruction. Seconde forme (regime des scenes de plusieurs millions de sites,
// ARCHITECTURE.md, paragraphe 4.6) : les tableaux de l'appareil sont comptes dans `device` (la memoire de la carte),
// la memoire epinglee et les tableaux de l'hote dans `budget` (la memoire de l'hote) ; les deux doivent lui survivre,
// et un meme budget donne deux fois vaut la premiere forme. Construction sans MHGP12_ENABLE_CUDA, ou aucun appareil
// utilisable : open rend device_unavailable.
class CatalogueDevice {
 public:
  struct Impl;
  [[nodiscard]] static Result<CatalogueDevice> open(MemoryBudget& budget) noexcept;
  [[nodiscard]] static Result<CatalogueDevice> open(MemoryBudget& budget, MemoryBudget& device) noexcept;
  CatalogueDevice(CatalogueDevice&& other) noexcept;
  CatalogueDevice(const CatalogueDevice&) = delete;
  CatalogueDevice& operator=(const CatalogueDevice&) = delete;
  CatalogueDevice& operator=(CatalogueDevice&&) = delete;
  ~CatalogueDevice();
  Impl& impl() noexcept { return *impl_; }

 private:
  explicit CatalogueDevice(std::unique_ptr<Impl> impl) noexcept;
  std::unique_ptr<Impl> impl_;
};

// Cat_K par la voie appareil : parcours en largeur et feuilles J3 sur l'appareil, feuilles non resolues (plus de 32
// sites ou etendue au-dela de 16) rejouees en exact sur l'hote avant admission, fin d'etage sur l'appareil. Meme
// Catalogue que build_catalogue, a l'octet pres (export MHGP12DP). Memes refus, dans le meme ordre, plus
// device_fault (erreur du pilote CUDA pendant le calcul) ; un refus ne publie rien et le contexte reste utilisable,
// sauf apres device_fault. Budget : celui du contexte ; le Pool sert le rejeu exact et la materialisation des niveaux.
[[nodiscard]] Result<Catalogue> build_catalogue_device(const Cloud& cloud, const CatalogueParams& params,
                                                      CatalogueDevice& device, sched::Pool& pool,
                                                      CatalogueDiagnostics* diagnostics = nullptr) noexcept;

// Export, hors du chemin chronometre, au format MHGP12DP version 1 de genre << catalogue >>
// (microbancs/mes_m3_m4_tour/common/format.hpp, CONTRAT_CATALOGUE.md, paragraphe 8 bis) : en-tete de 64 octets,
// sections SITEXYZ, BALLS (ordre canonique), POPOFF, POPVAL, NLEVELS. frame : nom de la trame, au plus 23 octets
// ASCII imprimables (parameter_out_of_range sinon).
[[nodiscard]] Outcome export_catalogue(const Cloud& cloud, const Catalogue& catalogue, std::string_view frame,
                                       io::FileWriter& out) noexcept;
// Empreinte canonique du catalogue : SHA-256 des octets exacts de cet export (portes de determinisme et d'identite).
[[nodiscard]] Result<io::Digest> catalogue_digest(const Cloud& cloud, const Catalogue& catalogue,
                                                  std::string_view frame) noexcept;

}  // namespace mhgp12
