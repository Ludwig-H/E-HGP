// En-tete public du module cloud : controle du domaine d'un nuage, sites en ordre de Morton, multiplicites, table
// site -> PointId. Port de src/cloud/cloud.hpp et cloud.cpp de la v10 (raccord R2, commit 865f5e6).
//
// Un SITE est une position distincte. Les points de meme position forment un site de multiplicite w >= 1 (la
// specification compte les doublons avec multiplicite). Les sites sont ranges dans l'ordre croissant de leur cle de
// Morton (morton.hpp) ; le rang d'un site dans cet ordre est son SiteIdx. Dans un site, les PointId sont croissants.
// Cet ordre total, (cle de Morton, PointId), ne depend que de l'ensemble des couples (position, PointId) :
//   - permuter l'entree ne change rien au resultat (memes tableaux, octet pour octet) ;
//   - renumeroter les PointId (application injective) ne change ni les sites, ni leur ordre, ni leurs multiplicites ;
//     seule la table site -> PointId suit la renumerotation.
// Portes : mhgp11_cloud_unit_permutation, mhgp11_cloud_unit_renumbering.
//
// Ce qui change par rapport a la v10 :
//   - le type de la cle de Morton suit B, sans masque (morton.hpp) ;
//   - la largeur declaree des coordonnees est un type a valeur toujours valide (CoordWidth) : prepare_cloud n'a plus
//     de parametre entier qu'il faudrait refuser. La v10 rendait parameter_out_of_range pour bits hors de [1, 21] ;
//   - le tri est un tri par base sequentiel sur (cle, PointId), sans std::sort ni std::vector ; le controle des
//     identifiants en double se fait dans la meme suite de passes (cloud.cpp) ;
//   - tous les tableaux de travail sont des Buffer reserves dans le budget de l'appelant, et aucune exception ne
//     sort : la v10 tenait trois std::vector hors budget, dont un pouvait lever std::bad_alloc ;
//   - les identifiants entrent comme des PointId, les decalages de la table sont des u64.
// Dans cette tranche le tri est sequentiel : le module n'emploie pas sched.
#pragma once

#include <optional>
#include <span>

#include "cloud/morton.hpp"
#include "core/core.hpp"

namespace mhgp11 {

// Largeur declaree des coordonnees d'une entree, en bits : toute coordonnee est dans [0, 2^bits), avec
// 1 <= bits <= kCoordBits. Le profil du binaire (kCoordBits) est la largeur par defaut et la plus grande. Une largeur
// plus etroite ne change ni les types ni les budgets de bits, qui suivent le profil : elle resserre seulement le
// controle du domaine. Une largeur hors bornes n'est pas representable.
class CoordWidth {
 public:
  constexpr CoordWidth() noexcept = default;
  // Largeur de `bits` bits ; rien si bits est hors de [1, kCoordBits].
  static constexpr std::optional<CoordWidth> of(int bits) noexcept {
    if (bits < 1 || bits > kCoordBits) return std::nullopt;
    return CoordWidth(bits);
  }
  constexpr int bits() const noexcept { return bits_; }
  // Plus grande coordonnee admise, 2^bits - 1. bits <= kCoordBits <= 24 : le decalage tient dans u32.
  constexpr u32 max() const noexcept { return (u32{1} << bits_) - 1; }

 private:
  constexpr explicit CoordWidth(int bits) noexcept : bits_(bits) {}
  int bits_ = kCoordBits;
};
static_assert(CoordWidth().max() == kCoordMax, "cloud : la largeur par defaut est celle du profil");

// Nuage prepare. Tous les tableaux sont indexes par le rang de Morton du site, idx(SiteIdx).
struct Cloud {
  Buffer<u32> x, y, z;  // coordonnees des sites
  Buffer<u32> w;        // multiplicite de chaque site, >= 1
  Csr<PointId> ids;     // site -> ses PointId, croissants
  u64 weight = 0;       // somme des multiplicites = nombre de points de l'entree

  // Nombre de sites. Il est inferieur a kNone (controle par prepare_cloud) : la conversion est exacte.
  u32 sites() const noexcept { return static_cast<u32>(x.size()); }
  // PointId du site s, croissants. Exige idx(s) < sites().
  std::span<const PointId> points(SiteIdx s) const noexcept { return ids.row(idx(s)); }
};

// Controle des tailles d'une entree, sans la lire. Premier refus dans cet ordre : empty_input (aucun point),
// size_mismatch (les quatre tableaux n'ont pas la meme longueur), index_overflow_u32 (au moins kNone points : un
// rang de site ou un indice de point ne tiendrait plus dans un u32 distinct de kNone).
[[nodiscard]] Outcome check_cloud_sizes(u64 x, u64 y, u64 z, u64 ids) noexcept;

// Controle et prepare un nuage. Premier refus dans cet ordre :
//   1. tailles (check_cloud_sizes) ;
//   2. coordinate_out_of_domain : une coordonnee depasse width.max() ;
//   3. memory_budget : les tableaux de tri ne tiennent pas dans le budget ;
//   4. duplicate_point_id : deux points portent le meme PointId ;
//   5. memory_budget : les tableaux du resultat ne tiennent pas dans le budget.
// Un refus ne laisse rien de reserve dans le budget ; aucune exception ne sort. Les tableaux du resultat appartiennent
// au budget donne, qui doit leur survivre.
//
// Pic d'octets reserves pour n points et s sites, mesure par MemoryBudget::peak (R = taille d'un enregistrement de
// tri, 16 octets jusqu'a B = 21 et 32 a B = 24 ; H = octets des histogrammes, constante du profil) :
//   max(2 R n + H, R n + 4 n + 16 s + 8 (s + 1)).
// Le premier terme est le tri (enregistrements et tampon de travail), le second le remplissage (enregistrements
// tries et resultat).
[[nodiscard]] Result<Cloud> prepare_cloud(std::span<const u32> x, std::span<const u32> y, std::span<const u32> z,
                                         std::span<const PointId> ids, CoordWidth width,
                                         MemoryBudget& budget) noexcept;

}  // namespace mhgp11
