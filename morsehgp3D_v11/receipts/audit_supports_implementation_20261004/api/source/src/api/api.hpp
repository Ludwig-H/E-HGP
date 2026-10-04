// En-tete public du module api : facade de la v11 pour l'executable mhgp11 et tout programme client (seul en-tete que
// cli/ inclut). Tranche S5 de la sortie parametree : Session, compute, publish, sortie full.
//
// Une Session porte l'unique MemoryBudget et l'unique Pool (docs/ARCHITECTURE.md, regle 3 et paragraphe 7.1) et joue
// l'auto-test F5 de l'environnement flottant a sa creation. compute rend un produit ENTIER, compte dans le budget de la
// Session, ou un refus ; publish ecrit ce produit dans un dossier transactionnel (io::OutputDirectory), manifeste
// deterministe en dernier, puis le publie par un seul renommage, ou refuse sans rien publier. Aucune fonction ne leve.
//
// Seule la sortie full est livree : full.mhgp11ful1 est octet pour octet le dump MHGP11FUL1 de la sonde
// bench/full_probe.cpp sur les memes entrees (porte mhgp11_cli_full_identity). Les sorties supports, points et plat
// viendront avec leurs tranches (S7, S9, S10).
//
// Moteur : parametres FIXES, ceux du masque qualifie 16379 des sondes (bench/points_export.cpp : feuilles de 16 a 256
// sites, graphe de paires, tables de populations, ordres concurrents, sans memo) ; aucune option de moteur (regle 6).
// Le nombre de fils ne change aucun octet publie (portes mhgp11_cli_full_determinism).
#pragma once

#include <array>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <variant>

#include "core/core.hpp"
#include "io/io.hpp"
#include "sched/sched.hpp"
#include "tower/tower.hpp"

namespace mhgp11::api {

// Plus grand ordre K admis par une requete : celui du catalogue et de la MEB bornee.
inline constexpr Order kMaxOrder = 12;
static_assert(kMaxOrder == kMaxMebSites, "api : K borne par la MEB bornee");

// Sorties livrees. --sortie=supports|points|plat est refuse (parameter_out_of_range) tant que sa tranche manque.
enum class OutputKind : u8 { full };
[[nodiscard]] std::string_view output_name(OutputKind kind) noexcept;

struct SessionParams {
  u64 budget_bytes = MemoryBudget::kUnlimited;  // limite de l'unique budget de la Session
  u32 workers = 1;                              // fils du Pool, appelant compris : 1..sched::kMaxWorkers
};

// Session : budget et Pool uniques, passes explicitement a chaque etage ; elle survit a tout Product qu'elle sert.
// Une Session deplacee est vide : seul close() y reste permis (succes).
class Session {
 public:
  // Ordre des refus : parameter_out_of_range (workers hors de 1..256), session_overhead (Pool), memory_budget
  // (compte du budget), environment_selftest (auto-test F5 : invariant viole, code 3).
  [[nodiscard]] static Result<Session> make(const SessionParams& params) noexcept;
  Session(Session&&) noexcept = default;
  Session(const Session&) = delete;
  Session& operator=(const Session&) = delete;
  Session& operator=(Session&&) = delete;
  ~Session() = default;

  MemoryBudget& budget() noexcept { return *budget_; }
  sched::Pool& pool() noexcept { return *pool_; }
  u32 workers() const noexcept { return pool_ == nullptr ? 0 : pool_->size(); }
  // Fin de vie : budget_not_released (invariant viole) si un Product ou un tampon de cette Session vit encore. Un
  // appel en echec peut etre rejoue apres la liberation.
  [[nodiscard]] Outcome close() noexcept;

 private:
  Session(std::unique_ptr<sched::Pool> pool, std::unique_ptr<MemoryBudget> budget) noexcept
      : pool_(std::move(pool)), budget_(std::move(budget)) {}
  std::unique_ptr<sched::Pool> pool_;
  std::unique_ptr<MemoryBudget> budget_;
};

// Nuage emprunte : quatre tableaux de meme longueur, dans l'ordre du fichier d'entree.
struct CloudView {
  std::span<const u32> x, y, z;
  std::span<const PointId> ids;
};

// Sortie full : la tour FULL entiere, ordres 1..k (k est l'ordre maximal).
struct FullRequest {
  Order k = 0;
};
using Request = std::variant<FullRequest>;

// Etages d'un appel. compute remplit cloud, index, domain, tree ; publish remplit output (manifeste et empreinte de
// l'arbre) et write (fichiers, synchronisation et renommage) ; attach reste nul pour full ; total revient a
// l'appelant. Pic : octets reserves les plus hauts pendant l'etage (MemoryBudget::restart_peak), jamais une estimation.
enum class Stage : u8 { cloud, index, domain, tree, attach, output, write, total };
inline constexpr std::size_t kStageCount = 8;
[[nodiscard]] std::string_view stage_name(Stage stage) noexcept;
struct StageReport {
  u64 nanoseconds = 0, peak_bytes = 0;
};
struct RunReport {
  std::array<StageReport, kStageCount> stages{};
  StageReport& at(Stage stage) noexcept { return stages[static_cast<std::size_t>(stage)]; }
  const StageReport& at(Stage stage) const noexcept { return stages[static_cast<std::size_t>(stage)]; }
};

class Product;

// Calcule le produit d'une requete. Ordre des refus (paragraphe 5 de la specification, etapes 5 a 7) :
//   parameter_out_of_range (k hors de 1..12) ;
//   nuage : empty_input, size_mismatch, coordinate_out_of_domain, duplicate_point_id, memory_budget ;
//   multiplicity_unsupported (positions repetees : la tour exige des sites de poids un) ;
//   parameter_out_of_range (k superieur au nombre de sites) ;
//   calcul : memory_budget, tower_capacity, invariants des modules.
// Un refus ne laisse aucune reservation dans le budget de la Session ; le rapport n'est ecrit qu'en cas de succes.
[[nodiscard]] Result<Product> compute(Session& session, const CloudView& cloud, const Request& request,
                                      RunReport* report = nullptr) noexcept;

// Resultat entier d'une requete, possede ; ses tampons sont comptes dans le budget de la Session qui l'a calcule et
// doivent etre rendus avant Session::close.
class Product {
 public:
  Product(Product&&) noexcept = default;
  Product(const Product&) = delete;
  Product& operator=(const Product&) = delete;
  Product& operator=(Product&&) = delete;
  ~Product() = default;

  OutputKind kind() const noexcept { return OutputKind::full; }
  const Request& request() const noexcept { return request_; }
  Order k() const noexcept { return tower_->kmax(); }
  const FullTower& full() const noexcept { return *tower_; }

 private:
  friend Result<Product> compute(Session&, const CloudView&, const Request&, RunReport*) noexcept;
  Product(const Request& request, FullTower&& tower) noexcept : request_(request), tower_(std::move(tower)) {}
  Request request_;
  std::optional<FullTower> tower_;
};

// Decimaux exacts recopies dans le manifeste (--pas, --origine) : chiffres ASCII, au plus un point suivi d'au moins
// un chiffre, au plus kMaxDecimal octets ; le pas est strictement positif et sans signe, une coordonnee d'origine peut
// porter un '-' initial. Aucun exposant, aucune normalisation : le texte est recopie tel quel.
inline constexpr std::size_t kMaxDecimal = 64;
[[nodiscard]] bool valid_grid_step(std::string_view text) noexcept;
[[nodiscard]] bool valid_origin_coordinate(std::string_view text) noexcept;

// Provenance d'un appel, recopiee dans le manifeste : empreintes et tailles des deux fichiers d'entree (jamais leurs
// chemins), budget declare et declarations de grille. Aucun temps ni nombre de fils.
struct Provenance {
  io::Digest points_sha256{}, ids_sha256{};
  u64 points_bytes = 0, ids_bytes = 0;
  std::optional<u64> budget_bytes;           // --budget ; vide : illimite
  std::string_view grid_step;                // --pas ; vide : non declare
  std::array<std::string_view, 3> origin{};  // --origine ; trois vides : non declaree
};

// Nom du fichier de la sortie full dans le dossier publie, et schema du manifeste (paragraphe 6.6).
inline constexpr std::string_view kFullFileName = "full.mhgp11ful1";
inline constexpr std::string_view kManifestSchema = "ehgp.v11.output.v1";

// Ecrit le produit dans `directory` (planifie par io::OutputDirectory::plan, sans fichier cree), puis le manifeste en
// dernier, et publie (commit). Rend l'empreinte du manifeste publie. Refus : parameter_out_of_range (provenance hors
// de sa forme), output_unwritable, output_conflict (io), memory_budget, tower_invariant (sphere de naissance absente).
// Sur refus, rien n'est publie : le destructeur de `directory` retire D.pending.
[[nodiscard]] Result<io::Digest> publish(Session& session, const Product& product, io::OutputDirectory& directory,
                                         const Provenance& provenance, RunReport* report = nullptr) noexcept;

// Empreinte tree_k_sha256 (paragraphe 6.6) de la foret d'ordre K : SHA-256 de "MHGP11TK", u64 K, u64 N, puis pour
// chaque noeud en numerotation canonique les u32 parent, rang, cle de naissance, nombre d'enfants, puis la liste
// concatenee des enfants en u32, tout en petit-boutiste. Commune aux sorties d'une meme entree et d'un meme K ; elle
// ne depend ni des PointId ni de l'ordre d'entree ni du nombre de fils.
[[nodiscard]] io::Digest tree_k_sha256(const OrderForest& forest) noexcept;

}  // namespace mhgp11::api
