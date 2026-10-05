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

// Ecrit la tour au format MHGP11FUL1 (paragraphe 6.2) : port octet pour octet de serialize, bench/full_probe.cpp.
// Refus : output_unwritable (ecriture), tower_invariant (sphere de naissance absente), arithmetic_invariant (num).
[[nodiscard]] Outcome write_full(io::FileWriter& out, const FullTower& tower) noexcept;

// Manifeste de la sortie full (paragraphe 6.6), terminee par un saut de ligne : JSON a cles en ordre fixe, sans
// espace, entiers decimaux. Peut lever std::bad_alloc (frontiere : guarded dans publish). Precondition : provenance
// controlee par check_provenance.
[[nodiscard]] std::string full_manifest(const api::Product& product, const api::Provenance& provenance,
                                        u64 file_bytes, const io::Digest& file_sha256);
// Provenance coherente avec le nuage du produit de `points` points (12 et 4 octets par point, budget declare non nul)
// et dans sa forme (pas, origine entiere ou absente, decimaux controles) : parameter_out_of_range sinon.
[[nodiscard]] Outcome check_provenance(const api::Provenance& provenance, u64 points) noexcept;

}  // namespace mhgp11::api_detail
