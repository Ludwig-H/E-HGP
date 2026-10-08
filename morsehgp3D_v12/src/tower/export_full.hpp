// Export FUL1 de la tour, hors du chemin chronometre (contrat, paragraphe 1) : octet pour octet le vidage de la sonde
// FULL de la v11 gelee (bench/full_probe.cpp, fonction serialize, commit ac081a06f ; lecteur strict
// bench/full_semantic.py). La regle [mhgp11] du controle de style interdit d'ecrire le nom de la v11 dans le code : le
// format est appele FUL1 ici, et sa signature de dix octets est ecrite caractere par caractere (export_full.cpp).
//
// Contenu, mots u64 petit-boutistes apres la signature : profil B, K, sites, poids ; par site dans l'ordre de Morton
// exact, x, y, z, w puis ses PointId ; par ordre k : k, naissances, noeuds, aretes, racine, puis par noeud parent,
// debut et nombre d'enfants, niveau en rationnel exact NON reduit (forme de la premiere boule du rang, celle du
// catalogue), pour une naissance son centre exact (trois numerateurs globaux a_j D + N_j et le denominateur D de la
// sphere de son support), a k >= 2 sa verticale ; enfin les enfants en CSR. Un entier exact est ecrit (signe, nombre de
// mots, mots) a la largeur de son type au profil : niveaux de (8B + 12) et (6B + 8) bits, centres i128 aux profils 21
// et 24 (comme la v11), trois mots au profil 32 (empreinte semantique seulement, paragraphe 9).
#pragma once

#include "io/io.hpp"
#include "tower/forest.hpp"

namespace mhgp12::tower {

// Source de l'export : nuage (sites, poids, PointId), niveaux exacts par LevelRank (niveau nul au rang 0), supports
// des boules (centres des naissances), registre de la tour.
struct FullSource {
  const Cloud* cloud = nullptr;
  std::span<const num::Level> levels;
  BallSource balls;
  const TowerForests* forests = nullptr;
};

// Ecrit le vidage dans `out` ; refus tower_invariant (rang sans niveau, support degenere, registre incoherent),
// output_unwritable (ecriture).
[[nodiscard]] Outcome export_full(const FullSource& source, io::FileWriter& out) noexcept;

// Empreinte SHA-256 des octets exacts de l'export, sans fichier ; bytes : longueur.
[[nodiscard]] Result<io::Digest> full_digest(const FullSource& source, u64* bytes = nullptr) noexcept;

}  // namespace mhgp12::tower
