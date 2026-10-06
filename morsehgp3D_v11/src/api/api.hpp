// En-tete public du module api : facade de la v11 pour l'executable mhgp11 et tout programme client (seul en-tete que
// cli/ inclut). Tranche S5 de la sortie parametree : Session, compute, publish, finish, sortie full.
//
// Une Session porte l'unique MemoryBudget et l'unique Pool (docs/ARCHITECTURE.md, regle 3 et paragraphe 7.1) et joue
// l'auto-test F5 de l'environnement flottant a sa creation. compute rend un produit ENTIER, compte dans le budget de la
// Session, ou un refus ; publish ecrit ce produit dans un dossier transactionnel (io::OutputDirectory), manifeste
// deterministe en dernier, puis le publie par un seul renommage, ou refuse sans rien publier ; finish ferme la
// Session et retire le dossier si elle refuse. Tout refus constate apres le commit retire le dossier publie
// (withdraw) ; si ce retrait echoue, le resultat le declare : etat published_complete et empreinte du manifeste
// (docs/SORTIES.md, paragraphes 3 et 9). Aucune fonction ne leve.
//
// Sorties livrees : full (tranche S5), supports (tranche S7) et points (tranche S9). full.mhgp11ful1 est octet pour
// octet le dump MHGP11FUL1 de la sonde bench/full_probe.cpp sur les memes entrees (porte mhgp11_cli_full_identity) ;
// supports.mhgp11sp est le format MHGP11SP version 2 (docs/SORTIES.md, paragraphe 6), relu par le lecteur
// bench/mhgp11_formats.py (portes mhgp11_cli_supports_*) ; points.mhgp11pt est le format MHGP11PT version 1
// (docs/SORTIES.md, paragraphe 7 : hierarchie de points H^r_{K+1}), relu par le meme lecteur (porte
// mhgp11_cli_points). La sortie plat (tranche S10) publie etiquettes.mhgp11et, format MHGP11ET version 1 : une
// etiquette i64 par point dans l'ordre du fichier d'entree, tiree de la meme hierarchie de points par la tete plate
// (module head : condensation, EOM ou feuilles a scores exacts ; porte mhgp11_cli_plat).
//
// Moteur : parametres FIXES, ceux du masque qualifie 16379 des sondes (bench/points_export.cpp : feuilles de 16 a 256
// sites, graphe de paires, tables de populations, ordres concurrents, sans memo) ; aucune option de moteur (regle 6).
// La sortie supports tire l'arbre d'ordre K de FULL au masque 16379, journal des graines pose sur l'ordre K
// (build_order_full, livraison L2b decidee par la regle de L2 de docs/SORTIES.md, paragraphe 11). La sortie points
// construit l'arbre d'ordre K seul (build_order) au masque 7035 : 16379 sans les trois options que build_order refuse,
// sans objet pour un ordre seul (verticales paralleles 128, reemploi des verticales regulieres 1024, ordres
// concurrents 8192 ; audit 238734f1d). Les deux voies donnent la meme foret et le meme rattachement (porte I10 ;
// MHGP11SP identique a l'octet par les deux voies : mhgp11_api_supports_route*).
// Le nombre de fils ne change aucun octet publie (portes mhgp11_cli_full_determinism, mhgp11_cli_supports_*).
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
#include "head/head.hpp"
#include "points/points.hpp"
#include "supports/supports.hpp"
#include "tower/tower.hpp"

namespace mhgp11::api {

// Plus grand ordre K admis par une requete : celui du catalogue et de la MEB bornee.
inline constexpr Order kMaxOrder = 12;
static_assert(kMaxOrder == kMaxMebSites, "api : K borne par la MEB bornee");

// Sorties livrees ; output_name(flat) vaut "plat" (--sortie=plat).
enum class OutputKind : u8 { full, supports, points, flat };
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
  // Destruction (docs/ARCHITECTURE.md, paragraphe 7.1, et regle 4) : un budget non revenu a zero, parce qu'un Product
  // ou un tampon de cette Session vit encore, est une violation d'invariant ; un destructeur n'a aucune issue a rendre,
  // donc le processus est termine (std::terminate), comme l'acces verifie a la valeur d'un Result refuse. close()
  // constate la meme violation sans arret, et peut etre rejoue apres la liberation. Une Session deplacee ne controle
  // rien. Portes : mhgp11_api_session_destroyed_live (arret anormal), mhgp11_api_session_destroyed_released.
  ~Session();

  MemoryBudget& budget() noexcept { return *budget_; }
  sched::Pool& pool() noexcept { return *pool_; }
  u32 workers() const noexcept { return pool_ == nullptr ? 0 : pool_->size(); }
  // Jeton d'identite : l'adresse de l'unique budget de la Session, sur le tas, stable au deplacement de la Session et
  // distincte entre Sessions vivantes ; nul pour une Session deplacee. Un Product le garde (compute) et publish refuse
  // le produit d'une autre Session (audit general a65903a7b, P1).
  const void* identity() const noexcept { return budget_.get(); }
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
// Sortie supports : arbre couvrant d'ordre K (tire de FULL, L2b ; decision du 6 octobre 2026) : naissances et
// fusions rattachees, un support S* par boule.
struct SupportsRequest {
  Order k = 0;
};
// Sortie points : arbre d'ordre K seul, hierarchie de points H^r_{K+1} (qualification m(K), kappa = 1). K >= n est
// refuse pour K >= 2 (aucune composante n'atteint K + 1 sites : decision K = n de docs/SORTIES.md, paragraphe 3).
struct PointsRequest {
  Order k = 0;
};
// Sortie plat : etiquettes plates de la hierarchie de points de la sortie points (meme arbre, meme refus de K >= n a
// K >= 2), par la tete plate : condensation au critere A (mcs >= 2), selection EOM a phi(r) = r^-z (z dans 1..3) ou
// feuilles ; decision LiDAR publiee par defaut : EOM, z = 1, mcs = 20 (docs/SORTIE_PLATE.md, paragraphe 3.4).
struct FlatRequest {
  Order k = 0;
  u32 mcs = 20;
  u32 z = 1;
  head::Selection selection = head::Selection::eom;
};
using Request = std::variant<FullRequest, SupportsRequest, PointsRequest, FlatRequest>;
[[nodiscard]] OutputKind request_kind(const Request& request) noexcept;

// Etages d'un appel. compute remplit cloud (preparation du nuage ; le CLI y ajoute la lecture), index, domain, tree
// (forets 1 a K de FULL ; pour supports, FULL avec le journal sur l'ordre K puis extraction de l'ordre K, balayage du
// rattachement exclu ; pour points, arbre d'ordre K seul, balayage exclu), attach (supports et points : diagnostic
// attach_ns, balayage du lemme D et controles I1 a I4 ; nul pour full) et, pour supports, output (assemblage de la
// hierarchie des supports, build_support_hierarchy) ; publish remplit output pour full (vide : le produit est la tour)
// et write (fichiers, empreinte de l'arbre, manifeste, synchronisations et renommage) ; total revient a l'appelant.
// Pic : octets reserves les plus hauts pendant l'etage (MemoryBudget::restart_peak), mesure.
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
}  // namespace mhgp11::api

namespace mhgp11::api_detail {
// Fabrique interne des produits (api/internal.hpp) : voie de la sortie supports choisie par les portes (L2b).
struct ProductAccess;
}  // namespace mhgp11::api_detail

namespace mhgp11::api {

// Calcule le produit d'une requete. Ordre des refus (docs/SORTIES.md, paragraphe 3, etapes 5 a 7) :
//   parameter_out_of_range (k hors de 1..12, requete inconnue) ;
//   preparation du nuage (prepare_cloud) : empty_input, size_mismatch, index_overflow_u32, coordinate_out_of_domain,
//     memory_budget (tri), duplicate_point_id, memory_budget (tableaux du nuage) ;
//   multiplicity_unsupported (positions repetees : la tour exige des sites de poids un) ;
//   parameter_out_of_range (k superieur au nombre de sites ; pour points, k >= 2 egal au nombre de sites) : le nombre
//     de sites n'est connu qu'apres la preparation du nuage, dont le memory_budget precede donc ce refus (K = n + 1
//     sous un budget trop petit : memory_budget) ;
//   calcul : memory_budget, tower_capacity, invariants des modules ; pour supports, support_shell_capacity (une
//     coquille etendue de W_K de plus de 24 sites : l'appel entier est refuse, avant tout calcul de Q_b) et
//     supports_invariant ; pour points, radical_sign_budget et points_invariant ; pour plat, ceux de points, puis
//     parameter_out_of_range (mcs < 2, z hors de 1..3, selection inconnue), radical_sign_budget et head_invariant.
// Un refus ne laisse aucune reservation dans le budget de la Session ; le rapport n'est ecrit qu'en cas de succes.
[[nodiscard]] Result<Product> compute(Session& session, const CloudView& cloud, const Request& request,
                                      RunReport* report = nullptr) noexcept;

// Resultat entier d'une requete, possede ; ses tampons sont comptes dans le budget de la Session qui l'a calcule et
// doivent etre rendus avant Session::close et avant la destruction de cette Session. Il garde le jeton d'identite de
// cette Session (Session::identity) : publish refuse de le publier par une autre Session.
class Product {
 public:
  Product(Product&&) noexcept = default;
  Product(const Product&) = delete;
  Product& operator=(const Product&) = delete;
  Product& operator=(Product&&) = delete;
  ~Product() = default;

  OutputKind kind() const noexcept { return request_kind(request_); }
  const Request& request() const noexcept { return request_; }
  Order k() const noexcept { return tower_ ? tower_->kmax() : order_->order(); }
  // Domaine du produit (nuage, index, catalogue), quelle que soit la sortie.
  const FullDomain& domain() const noexcept { return tower_ ? tower_->domain() : order_->domain(); }
  // Precondition : kind() == full.
  const FullTower& full() const noexcept { return *tower_; }
  // Precondition : kind() == supports, points ou flat. Arbre d'ordre K (et rattachement).
  const OrderTree& order_tree() const noexcept { return *order_; }
  // Precondition : kind() == supports. Hierarchie des supports.
  const supports::SupportHierarchy& hierarchy() const noexcept { return *hierarchy_; }
  // Precondition : kind() == points. Hierarchie de points H^r_{K+1}.
  const points::PointHierarchy& points() const noexcept { return *points_; }
  // Precondition : kind() == flat. Etiquettes dans l'ordre d'entree et compteurs de la tete plate.
  const head::FlatLabels& flat() const noexcept { return *flat_; }
  // Vrai si ce produit a ete calcule par cette Session (meme jeton d'identite, non nul).
  bool computed_by(const Session& session) const noexcept {
    return session_ != nullptr && session_ == session.identity();
  }

 private:
  friend Result<Product> compute(Session&, const CloudView&, const Request&, RunReport*) noexcept;
  friend struct api_detail::ProductAccess;
  Product(const Request& request, FullTower&& tower, const void* session) noexcept
      : request_(request), tower_(std::move(tower)), session_(session) {}
  Product(const Request& request, OrderTree&& tree, supports::SupportHierarchy&& hierarchy,
          const void* session) noexcept
      : request_(request), order_(std::move(tree)), hierarchy_(std::move(hierarchy)), session_(session) {}
  Product(const Request& request, OrderTree&& tree, points::PointHierarchy&& points, const void* session) noexcept
      : request_(request), order_(std::move(tree)), points_(std::move(points)), session_(session) {}
  Product(const Request& request, OrderTree&& tree, head::FlatLabels&& labels, const void* session) noexcept
      : request_(request), order_(std::move(tree)), flat_(std::move(labels)), session_(session) {}
  Request request_;
  std::optional<FullTower> tower_;
  std::optional<OrderTree> order_;
  std::optional<supports::SupportHierarchy> hierarchy_;
  std::optional<points::PointHierarchy> points_;
  std::optional<head::FlatLabels> flat_;
  const void* session_ = nullptr;
};

// Decimaux exacts recopies dans le manifeste (--pas, --origine) : chiffres ASCII, au plus un point suivi d'au moins
// un chiffre, au plus kMaxDecimal octets ; le pas est strictement positif et sans signe, une coordonnee d'origine peut
// porter un '-' initial. Aucun exposant, aucune normalisation : le texte est recopie tel quel.
inline constexpr std::size_t kMaxDecimal = 64;
[[nodiscard]] bool valid_grid_step(std::string_view text) noexcept;
[[nodiscard]] bool valid_origin_coordinate(std::string_view text) noexcept;

// Provenance d'un appel, recopiee dans le manifeste : empreintes et tailles des deux fichiers d'entree (jamais leurs
// chemins), budget declare et declarations de grille. Aucun temps ni nombre de fils. publish exige sa coherence avec
// le nuage du produit de n points : points_bytes = 12n, ids_bytes = 4n, budget declare absent ou strictement positif
// (le lecteur officiel refuse tout autre manifeste) ; les empreintes ne sont pas verifiables depuis la provenance.
struct Provenance {
  io::Digest points_sha256{}, ids_sha256{};
  u64 points_bytes = 0, ids_bytes = 0;
  std::optional<u64> budget_bytes;           // --budget ; vide : illimite
  std::string_view grid_step;                // --pas ; vide : non declare
  std::array<std::string_view, 3> origin{};  // --origine ; trois vides : non declaree
};

// Noms des fichiers des sorties full, supports, points et plat dans le dossier publie, schema du manifeste et version de la signature
// tree_k_sha256 que ce schema fixe (docs/SORTIES.md, paragraphe 8).
inline constexpr std::string_view kFullFileName = "full.mhgp11ful1";
inline constexpr std::string_view kSupportsFileName = "supports.mhgp11sp";
inline constexpr std::string_view kPointsFileName = "points.mhgp11pt";
inline constexpr std::string_view kFlatFileName = "etiquettes.mhgp11et";
inline constexpr std::string_view kManifestSchema = "ehgp.v11.output.v1";
inline constexpr u64 kTreeSignatureVersion = 2;

// Etat du dossier de sortie d'un appel (docs/SORTIES.md, paragraphes 3 et 9) : none, rien n'est publie ;
// published_complete, D est publie et complet, et son manifeste en fait foi. Apres un refus, published_complete ne
// vient que d'un double echec (retrait refuse) : ce n'est ni un succes de durabilite ni une sortie partielle.
enum class PublicationState : u8 { none, published_complete };
[[nodiscard]] std::string_view publication_state_name(PublicationState state) noexcept;

// Issue d'une publication ou d'une fin d'appel : conforme ou refus, etat du dossier, empreinte du manifeste de D publie
// (zeros si l'etat est none). Le code de sortie est celui de `outcome`, quel que soit l'etat.
struct Publication {
  Outcome outcome{};
  PublicationState state = PublicationState::none;
  io::Digest manifest_sha256{};
  bool ok() const noexcept { return outcome.ok(); }
};

// Ecrit le produit dans `directory` (planifie par io::OutputDirectory::plan, sans fichier cree), puis le manifeste en
// dernier, et publie (commit) : conforme, published_complete et l'empreinte du manifeste publie. Refus (etape 8 de
// docs/SORTIES.md, paragraphe 3) : parameter_out_of_range (produit calcule par une autre Session, puis provenance
// incoherente avec le nuage du produit ou hors de sa forme : avant toute creation de fichier et toute ecriture du
// rapport),
// output_unwritable, output_conflict (io), memory_budget, tower_invariant (sphere de naissance absente) ; rien n'est
// publie (none), le destructeur de `directory` retire D.pending. Seul double echec : le commit refuse alors que D est
// publie (synchronisation du parent, puis son retour, en echec) ; withdraw le retire alors, ou le declare.
[[nodiscard]] Publication publish(Session& session, const Product& product, io::OutputDirectory& directory,
                                  const Provenance& provenance, RunReport* report = nullptr) noexcept;

// Refus constate quand `directory` a peut-etre publie D (docs/SORTIES.md, paragraphe 3 : commit en double echec a
// l'etape 8, fin de session ou ligne d'etat a l'etape 9). Si D est publie, il est retire
// (io::OutputDirectory::retract).
// Rend `refusal` inchange et l'etat : none si plus rien n'est publie (retrait reussi, ou rien ne l'etait),
// published_complete et l'empreinte du manifeste si D reste publie (retrait refuse). Precondition : refus.
[[nodiscard]] Publication withdraw(const Outcome& refusal, io::OutputDirectory& directory) noexcept;

// Fin d'un appel publie (etape 9, premiere moitie) : Session::close ; sur refus (budget_not_released, invariant viole,
// code 3 : un Product ou un tampon de la Session vit encore), withdraw. Conforme : l'etat de `directory`
// (published_complete et l'empreinte apres un publish conforme). La ligne d'etat de l'appelant vient ensuite ; si elle
// echoue, l'appelant rend withdraw(output_unwritable, directory).
[[nodiscard]] Publication finish(Session& session, io::OutputDirectory& directory) noexcept;

// Signature tree_k_sha256 de l'arbre d'ordre K, version 2 (docs/SORTIES.md, paragraphe 8 ; reponse D.2 de
// l'auditeur) : SHA-256 de "MHGP11TK", des u64 2 (version), coord_bits, K, n et N, des 32 octets bruts du SHA-256 de
// la geometrie ("MHGP11GX", u64 coord_bits et n, puis u32 x, y, z de chaque site dans l'ordre des SiteIdx), puis, par
// noeud en numerotation canonique : u32 parent (kNone a la racine) et rang, u8 kind (0 feuille de site a K = 1,
// 1 naissance de boule, 2 fusion) et arite a de la naissance, les a SiteIdx de la naissance en u32 (le site a K = 1,
// S* de la boule de naissance sinon, rien pour une fusion), u32 nombre d'enfants, puis les enfants croissants en u32 ;
// petit-boutiste, sans bourrage. Ni PointId ni BallIdx : elle ne depend ni des etiquettes, ni de l'ordre d'entree,
// ni du nombre de fils. Precondition : `forest` est construite sur `domain` (FullTower::order sur FullTower::domain, ou
// OrderTree::forest sur OrderTree::domain : les deux voies donnent la meme foret, porte I10).
[[nodiscard]] io::Digest tree_k_sha256(const FullDomain& domain, const OrderForest& forest) noexcept;

}  // namespace mhgp11::api
