// K plus proches voisins EXACTS de chaque site, pour MES-G1 (candidats « voisins » du levier G-L3), hors produit.
//
// Ordre total : distance carree entiere exacte (u64), puis SiteIdx croissant ; le site lui-meme est exclu. Structure :
// arbre k-d sur les coordonnees entieres de la grille (coupe a la mediane de l'axe le plus etendu, boites exactes) ;
// une branche n'est ecartee que si la distance carree minimale de sa boite est STRICTEMENT superieure a celle du pire
// voisin retenu (a egalite, un site de plus petit SiteIdx peut encore entrer). Juge : force brute sur un echantillon
// (voisins_force_brute). Coordonnees < 2^32 par axe ; ici < 2^21 (profil u21) : la somme des carres tient dans u64.
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace mhgp12::g1 {

using u32 = std::uint32_t;
using u64 = std::uint64_t;
inline constexpr u32 kAucun = 0xFFFFFFFFu;

struct Voisin {
  u64 d2;
  u32 site;
};
inline bool avant(const Voisin& a, const Voisin& b) { return a.d2 != b.d2 ? a.d2 < b.d2 : a.site < b.site; }

// Distance carree exacte entre deux sites (xyz : trois u32 par site, ordre des SiteIdx).
inline u64 distance2(const u32* xyz, u32 a, u32 b) {
  u64 s = 0;
  for (int j = 0; j < 3; ++j) {
    const u64 u = xyz[3 * u64{a} + j], v = xyz[3 * u64{b} + j];
    const u64 e = u > v ? u - v : v - u;
    s += e * e;
  }
  return s;
}

class ArbreKd {
 public:
  ArbreKd(const u32* xyz, u32 n, u32 feuille) : xyz_(xyz), feuille_(std::max<u32>(1, feuille)) {
    if (n == 0) throw std::runtime_error("voisins : nuage vide");
    ordre_.resize(n);
    for (u32 i = 0; i < n; ++i) ordre_[i] = i;
    noeuds_.reserve(2 * (n / feuille_ + 1));
    construire(0, n);
  }

  // Les K plus proches voisins de q (q exclu), tries par (d2, SiteIdx) croissants, dans `sortie` (vide au depart).
  void voisins(u32 q, u32 K, std::vector<Voisin>& tas, std::vector<Voisin>& sortie) const {
    tas.clear();
    sortie.clear();
    if (K == 0) return;
    descendre(0, q, K, tas);
    sortie = tas;
    std::sort(sortie.begin(), sortie.end(), avant);
  }

 private:
  struct Noeud {
    u32 lo[3], hi[3];
    u32 debut, fin;
    u32 gauche, droite;  // kAucun pour une feuille
  };

  u32 coord(u32 site, int axe) const { return xyz_[3 * u64{site} + axe]; }

  u32 construire(u32 debut, u32 fin) {
    Noeud n{};
    for (int j = 0; j < 3; ++j) {
      n.lo[j] = 0xFFFFFFFFu;
      n.hi[j] = 0;
    }
    for (u32 i = debut; i < fin; ++i)
      for (int j = 0; j < 3; ++j) {
        n.lo[j] = std::min(n.lo[j], coord(ordre_[i], j));
        n.hi[j] = std::max(n.hi[j], coord(ordre_[i], j));
      }
    n.debut = debut;
    n.fin = fin;
    n.gauche = n.droite = kAucun;
    const u32 ici = static_cast<u32>(noeuds_.size());
    noeuds_.push_back(n);
    if (fin - debut <= feuille_) return ici;
    int axe = 0;
    for (int j = 1; j < 3; ++j)
      if (n.hi[j] - n.lo[j] > n.hi[axe] - n.lo[axe]) axe = j;
    const u32 milieu = debut + (fin - debut) / 2;
    std::nth_element(ordre_.begin() + debut, ordre_.begin() + milieu, ordre_.begin() + fin, [&](u32 a, u32 b) {
      const u32 ca = coord(a, axe), cb = coord(b, axe);
      return ca != cb ? ca < cb : a < b;
    });
    const u32 g = construire(debut, milieu);
    const u32 d = construire(milieu, fin);
    noeuds_[ici].gauche = g;
    noeuds_[ici].droite = d;
    return ici;
  }

  // Distance carree minimale exacte de q a la boite fermee du noeud.
  u64 minorant(const Noeud& n, u32 q) const {
    u64 s = 0;
    for (int j = 0; j < 3; ++j) {
      const u64 c = coord(q, j);
      u64 e = 0;
      if (c < n.lo[j]) e = n.lo[j] - c;
      else if (c > n.hi[j]) e = c - n.hi[j];
      s += e * e;
    }
    return s;
  }

  void proposer(std::vector<Voisin>& tas, u32 K, Voisin v) const {
    if (tas.size() < K) {
      tas.push_back(v);
      std::push_heap(tas.begin(), tas.end(), avant);
    } else if (avant(v, tas.front())) {
      std::pop_heap(tas.begin(), tas.end(), avant);
      tas.back() = v;
      std::push_heap(tas.begin(), tas.end(), avant);
    }
  }

  void descendre(u32 ni, u32 q, u32 K, std::vector<Voisin>& tas) const {
    const Noeud& n = noeuds_[ni];
    if (tas.size() == K && minorant(n, q) > tas.front().d2) return;  // egalite : on visite (departage par SiteIdx)
    if (n.gauche == kAucun) {
      for (u32 i = n.debut; i < n.fin; ++i) {
        const u32 s = ordre_[i];
        if (s != q) proposer(tas, K, Voisin{distance2(xyz_, q, s), s});
      }
      return;
    }
    const u64 mg = minorant(noeuds_[n.gauche], q), md = minorant(noeuds_[n.droite], q);
    if (mg <= md) {
      descendre(n.gauche, q, K, tas);
      descendre(n.droite, q, K, tas);
    } else {
      descendre(n.droite, q, K, tas);
      descendre(n.gauche, q, K, tas);
    }
  }

  const u32* xyz_;
  u32 feuille_;
  std::vector<u32> ordre_;
  std::vector<Noeud> noeuds_;
};

// Juge : les K plus proches voisins de q par force brute, meme ordre total.
inline std::vector<Voisin> voisins_force_brute(const u32* xyz, u32 n, u32 q, u32 K) {
  std::vector<Voisin> tous;
  tous.reserve(n);
  for (u32 s = 0; s < n; ++s)
    if (s != q) tous.push_back(Voisin{distance2(xyz, q, s), s});
  const std::size_t garde = std::min<std::size_t>(K, tous.size());
  std::partial_sort(tous.begin(), tous.begin() + garde, tous.end(), avant);
  tous.resize(garde);
  return tous;
}

// Table des voisins de tous les sites : K entrees par site (kAucun au-dela de n - 1 voisins).
inline std::vector<u32> table_des_voisins(const u32* xyz, u32 n, u32 K, u32 feuille) {
  const ArbreKd arbre(xyz, n, feuille);
  std::vector<u32> table(u64{n} * K, kAucun);
  std::vector<Voisin> tas, sortie;
  tas.reserve(K + 1);
  for (u32 q = 0; q < n; ++q) {
    arbre.voisins(q, K, tas, sortie);
    for (std::size_t j = 0; j < sortie.size(); ++j) table[u64{q} * K + j] = sortie[j].site;
  }
  return table;
}

}  // namespace mhgp12::g1
