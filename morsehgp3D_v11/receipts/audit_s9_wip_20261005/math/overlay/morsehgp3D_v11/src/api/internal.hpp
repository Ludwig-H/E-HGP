// Declarations internes du module api, partagees par ses fichiers (et lues par ses portes) : parametres fixes du
// moteur, ecrivain MHGP11FUL1, manifeste. Aucun autre module ne les inclut (regle [inclusion]).
#pragma once

#include <string>

#include "api/api.hpp"
#include "catalogue/catalogue.hpp"

namespace mhgp11::api_detail {

// Parametres fixes du moteur pour l'ordre maximal k : ceux du masque qualifie 16379 des sondes, exactement ceux de
// bench/points_export.cpp (catalogue : feuilles de 16 a 256 sites, max_nodes 0, ball_limit kNone, lignes de centres
// en cache, tri indirect, frontiere adaptative, assemblage parallele, passe unique, graphe de paires ; forets : lots
// reguliers de 4096 sur 48 voies, verticales paralleles, census reutilise, naissances denses, verticales regulieres
// reutilisees, table de populations, ordres concurrents ; aucun memo). bench/full_probe.cpp les lit sous le masque
// 16379 avec les arguments 16 256 0 4294967295.
[[nodiscard]] CatalogueParams catalogue_params(Order k) noexcept;
[[nodiscard]] FullParams full_params() noexcept;
// Masque des sondes qui designe ces parametres (bench/full_probe.cpp, champ "optimizations" de sa ligne "full").
inline constexpr u64 kEngineMask = 16379;
// Arbre d'ordre K seul de la sortie supports (build_order) : full_params sans les trois options que build_order
// refuse, sans objet pour un ordre seul (verticales paralleles 128, reemploi des verticales regulieres 1024, ordres
// concurrents 8192) ; masque 7035 des sondes. Difference publiee (docs/SORTIES.md, paragraphe 1 ; audit 238734f1d) :
// FULL garde 16379, la mesure appariee de L2 compare ces deux configurations.
[[nodiscard]] FullParams order_params() noexcept;
inline constexpr u64 kOrderMask = 7035;
static_assert(kOrderMask == (kEngineMask & ~(u64{128} | u64{1024} | u64{8192})), "api : masque 7035 = 16379 - 9344");

// Ecrit la tour au format MHGP11FUL1 (paragraphe 6.2) : port octet pour octet de serialize, bench/full_probe.cpp.
// Refus : output_unwritable (ecriture), tower_invariant (sphere de naissance absente), arithmetic_invariant (num).
[[nodiscard]] Outcome write_full(io::FileWriter& out, const FullTower& tower) noexcept;

// Ecrit la hierarchie des supports au format MHGP11SP version 1 (docs/SORTIES.md, paragraphe 6) : en-tete, colonnes
// alignees sur 8 octets, bourrage nul, aucun compte. Refus : output_unwritable (ecriture), supports_invariant (arbre
// et hierarchie incoherents, sans porte possible sur un produit de compute).
[[nodiscard]] Outcome write_supports(io::FileWriter& out, const OrderTree& tree,
                                     const supports::SupportHierarchy& hierarchy) noexcept;

// Manifeste de la sortie full (paragraphe 6.6), terminee par un saut de ligne : JSON a cles en ordre fixe, sans
// espace, entiers decimaux. Peut lever std::bad_alloc (frontiere : guarded dans publish). Precondition : provenance
// controlee par check_provenance.
[[nodiscard]] std::string full_manifest(const api::Product& product, const api::Provenance& provenance,
                                        u64 file_bytes, const io::Digest& file_sha256);
// Manifeste de la sortie supports (docs/SORTIES.md, paragraphe 8) : memes cles d'en-tete que full, fichier
// supports.mhgp11sp (MHGP11SP, version 1), tree_k_sha256 de l'arbre d'ordre K, comptes et agregats de la hierarchie.
// Peut lever std::bad_alloc (guarded dans publish). Precondition : provenance controlee.
[[nodiscard]] std::string supports_manifest(const api::Product& product, const api::Provenance& provenance,
                                            u64 bytes, const io::Digest& sha256);
// Ecrit la hierarchie de points au format MHGP11PT version 1 (docs/SORTIES.md, paragraphe 7) : en-tete, colonnes
// alignees sur 8 octets, bourrage nul. Refus : output_unwritable (ecriture), points_invariant (arbre et hierarchie
// incoherents, sans porte possible sur un produit de compute).
[[nodiscard]] Outcome write_points(io::FileWriter& out, const OrderTree& tree, const points::PointHierarchy& h) noexcept;
// Manifeste de la sortie points (docs/SORTIES.md, paragraphe 8) : memes cles d'en-tete, fichier points.mhgp11pt
// (MHGP11PT, version 1), tree_k_sha256 de l'arbre d'ordre K, comptes recalculables depuis le fichier. Peut lever
// std::bad_alloc (guarded dans publish). Precondition : provenance controlee.
[[nodiscard]] std::string points_manifest(const api::Product& product, const api::Provenance& provenance, u64 bytes,
                                          const io::Digest& sha256);
// Debut commun des manifestes, jusqu'a la valeur de tree_k_sha256 comprise (schema, sortie, statuts, profil, K,
// parametres, entrees, fichier) ; le manifeste se termine par les comptes, puis "}\n". Peut lever std::bad_alloc.
[[nodiscard]] std::string manifest_head(const api::Product& product, const api::Provenance& provenance,
                                        std::string_view name, std::string_view format, u64 bytes,
                                        const io::Digest& sha256, const io::Digest& tree);
// Entier decimal du manifeste. Peut lever std::bad_alloc.
void manifest_number(std::string& out, u64 value);

// Provenance coherente avec le nuage du produit de `points` points (12 et 4 octets par point, budget declare non nul)
// et dans sa forme (pas, origine entiere ou absente, decimaux controles) : parameter_out_of_range sinon.
[[nodiscard]] Outcome check_provenance(const api::Provenance& provenance, u64 points) noexcept;

}  // namespace mhgp11::api_detail
