// Arbre couvrant euclidien minimal EXACT du juge JUG-EMST : Boruvka sur un arbre k-d, distances carrees entieres
// exactes (u64 quand toutes les coordonnees tiennent sur 31 bits, u128 au-dela, profil 32 compris), ordre total
// strict des aretes (distance carree, plus petit site, plus grand site) : l'arbre couvrant minimal pour cet ordre est
// unique, et Boruvka le rend quel que soit l'ordre de visite (chaque composante choisit son arete sortante minimale,
// qui est dans l'arbre par la propriete de coupe ; les choix d'un tour ne forment pas de cycle).
//
// Elagages exacts, sans effet sur le resultat :
//  - un noeud k-d dont tous les sites sont dans la composante de la requete est saute ;
//  - un noeud dont la distance carree a la boite depasse strictement la meilleure arete de la composante est saute
//    (a egalite il est visite : une arete de meme longueur peut gagner au departage) ;
//  - voisin exact memorise : si le plus proche site hors composante d'un site (pour l'ordre (d2, site)) est encore
//    hors de sa composante au tour suivant, il l'est toujours (l'ensemble des sites exterieurs ne fait que
//    decroitre) ;
//  - minorant memorise : une requete bornee par la meilleure arete B de la composante et restee vide prouve que
//    toute arete sortante du site est >= B ; au tour suivant, le site est saute si la meilleure arete courante de sa
//    composante est <= B.
#pragma once
#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

#include "entier.hpp"
#include "nuage.hpp"

namespace mhgp12_emst {

struct Arete {
  u128 d2;
  u32 a, b;  // a < b, numeros canoniques
};

// Ordre total strict des aretes : (distance carree, plus petit site, plus grand site).
inline bool arete_inferieure(const Arete& p, const Arete& q) {
  if (p.d2 != q.d2) return p.d2 < q.d2;
#ifdef MHGP12_MUTANT_DEPARTAGE
  // MUTANT CAUSAL (copie compilee a part) : departage inverse des egalites (plus grands sites d'abord).
  if (p.a != q.a) return p.a > q.a;
  return p.b > q.b;
#else
  if (p.a != q.a) return p.a < q.a;
  return p.b < q.b;
#endif
}

struct StatsEmst {
  u32 tours = 0;
  u64 requetes = 0;      // requetes de plus proche site exterieur
  u64 voisins_gardes = 0;  // sites servis par leur voisin exact memorise
  u64 minorants = 0;     // sites sautes par leur minorant
  u64 distances = 0;     // distances carrees calculees dans les feuilles
  u32 profondeur = 0;    // profondeur de l'arbre k-d
};

template <class D>
class Boruvka {
 public:
  Boruvka(const Sites& sites, StatsEmst& stats) : sites_(sites), stats_(stats) {}

  std::vector<Arete> calculer() {
    const u32 n = sites_.taille();
    std::vector<Arete> arbre;
    if (n < 2) return arbre;
    arbre.reserve(n - 1);
    construire();
    parent_uf_.resize(n);
    taille_uf_.assign(n, 1);
    std::iota(parent_uf_.begin(), parent_uf_.end(), 0u);
    comp_id_.resize(n);
    comp_place_.resize(n);
    comp_noeud_.resize(noeuds_.size());
    meilleur_.resize(n);
    etat_.assign(n, Etat{});
    u32 composantes = n;
    std::vector<Arete> choix;
    while (composantes > 1) {
      ++stats_.tours;
      preparer_tour();
      for (u32 j = 0; j < n; ++j) servir_par_voisin(j);
      for (u32 j = 0; j < n; ++j) {
        if (etat_[j].genre == kVoisin) continue;
        const u32 c = comp_place_[j];
        if (etat_[j].genre == kMinorant && !inferieure(etat_[j].cle, meilleur_[c])) {
          ++stats_.minorants;
          continue;
        }
        requete(j, c);
      }
      choix.clear();
      for (u32 id = 0; id < n; ++id) {
        if (parent_uf_[id] == id) choix.push_back(arete(meilleur_[id]));
      }
      std::sort(choix.begin(), choix.end(), arete_inferieure);
      for (const Arete& e : choix) {
        const u32 ra = trouver(e.a), rb = trouver(e.b);
        if (ra == rb) continue;  // meme arete choisie par les deux composantes
        unir(ra, rb);
        arbre.push_back(e);
        --composantes;
      }
    }
    std::sort(arbre.begin(), arbre.end(), arete_inferieure);
    return arbre;
  }

 private:
  struct Cle {
    D d2 = static_cast<D>(~static_cast<D>(0));  // numeric_limits n'est pas specialise pour u128 en C++ strict
    u32 a = kAucun, b = kAucun;
  };
  enum : u8 { kRien = 0, kVoisin = 1, kMinorant = 2 };
  struct Etat {
    Cle cle;  // kVoisin : (d2, voisin, -) ; kMinorant : minorant de toute arete sortante du site
    u8 genre = kRien;
  };
  struct Noeud {
    u32 debut, fin, gauche, droite;
    u32 bmin[3], bmax[3];
  };
  struct Item {
    u32 noeud;
    D dist;
  };

  static bool inferieure(const Cle& p, const Cle& q) {
    if (p.d2 != q.d2) return p.d2 < q.d2;
    return arete_inferieure(Arete{0, p.a, p.b}, Arete{0, q.a, q.b});
  }
  static Arete arete(const Cle& c) { return Arete{static_cast<u128>(c.d2), c.a, c.b}; }

  static u64 carre_ecart(u32 p, u32 q) {
    const u64 e = p > q ? u64{p - q} : u64{q - p};
    return e * e;  // < 2^64 : ecart < 2^32
  }
  static u64 carre_hors(u32 p, u32 lo, u32 hi) {
    if (p < lo) return u64{lo - p} * u64{lo - p};
    if (p > hi) return u64{p - hi} * u64{p - hi};
    return 0;
  }
  D distance(u32 j, u32 s) const {
    return D{carre_ecart(px_[j], px_[s])} + D{carre_ecart(py_[j], py_[s])} + D{carre_ecart(pz_[j], pz_[s])};
  }
  D distance_boite(u32 j, const Noeud& nd) const {
    return D{carre_hors(px_[j], nd.bmin[0], nd.bmax[0])} + D{carre_hors(py_[j], nd.bmin[1], nd.bmax[1])} +
           D{carre_hors(pz_[j], nd.bmin[2], nd.bmax[2])};
  }

  u32 trouver(u32 i) {
    while (parent_uf_[i] != i) {
      parent_uf_[i] = parent_uf_[parent_uf_[i]];
      i = parent_uf_[i];
    }
    return i;
  }
  void unir(u32 a, u32 b) {
    if (taille_uf_[a] < taille_uf_[b]) std::swap(a, b);
    parent_uf_[b] = a;
    taille_uf_[a] += taille_uf_[b];
  }

  // Arbre k-d equilibre : coupe a la mediane de l'axe le plus etendu, feuilles d'au plus 16 sites ; les sites sont
  // ranges par place (ordre des feuilles) pour des parcours contigus.
  void construire() {
    const u32 n = sites_.taille();
    std::vector<u32> place(n);
    std::iota(place.begin(), place.end(), 0u);
    noeuds_.clear();
    noeuds_.reserve(2 * (n / 8 + 1));
    struct Tache {
      u32 noeud, debut, fin, profondeur;
    };
    std::vector<Tache> pile;
    noeuds_.push_back(Noeud{0, n, kAucun, kAucun, {0, 0, 0}, {0, 0, 0}});
    pile.push_back({0, 0, n, 1});
    const std::vector<u32>* coord[3] = {&sites_.x, &sites_.y, &sites_.z};
    while (!pile.empty()) {
      const Tache t = pile.back();
      pile.pop_back();
      stats_.profondeur = std::max(stats_.profondeur, t.profondeur);
      u32 lo[3] = {kAucun, kAucun, kAucun}, hi[3] = {0, 0, 0};
      for (u32 s = t.debut; s < t.fin; ++s) {
        for (int a = 0; a < 3; ++a) {
          const u32 v = (*coord[a])[place[s]];
          lo[a] = std::min(lo[a], v);
          hi[a] = std::max(hi[a], v);
        }
      }
      Noeud& nd = noeuds_[t.noeud];
      for (int a = 0; a < 3; ++a) {
        nd.bmin[a] = lo[a];
        nd.bmax[a] = hi[a];
      }
      if (t.fin - t.debut <= 16) continue;
      int axe = 0;
      for (int a = 1; a < 3; ++a) {
        if (u64{hi[a]} - lo[a] > u64{hi[axe]} - lo[axe]) axe = a;
      }
      const u32 milieu = t.debut + (t.fin - t.debut) / 2;
      const std::vector<u32>& c = *coord[axe];
      std::nth_element(place.begin() + t.debut, place.begin() + milieu, place.begin() + t.fin,
                       [&](u32 p, u32 q) { return c[p] != c[q] ? c[p] < c[q] : p < q; });
      const u32 g = static_cast<u32>(noeuds_.size());
      noeuds_.push_back(Noeud{t.debut, milieu, kAucun, kAucun, {0, 0, 0}, {0, 0, 0}});
      noeuds_.push_back(Noeud{milieu, t.fin, kAucun, kAucun, {0, 0, 0}, {0, 0, 0}});
      noeuds_[t.noeud].gauche = g;
      noeuds_[t.noeud].droite = g + 1;
      pile.push_back({g, t.debut, milieu, t.profondeur + 1});
      pile.push_back({g + 1, milieu, t.fin, t.profondeur + 1});
    }
    px_.resize(n);
    py_.resize(n);
    pz_.resize(n);
    id_.resize(n);
    for (u32 s = 0; s < n; ++s) {
      px_[s] = sites_.x[place[s]];
      py_[s] = sites_.y[place[s]];
      pz_[s] = sites_.z[place[s]];
      id_[s] = place[s];
    }
    place_de_.resize(n);
    for (u32 s = 0; s < n; ++s) place_de_[id_[s]] = s;
    pile_requete_.resize(2 * stats_.profondeur + 4);
  }

  void preparer_tour() {
    const u32 n = sites_.taille();
    for (u32 id = 0; id < n; ++id) comp_id_[id] = trouver(id);
    for (u32 s = 0; s < n; ++s) comp_place_[s] = comp_id_[id_[s]];
    // Les enfants sont crees apres leur parent : un parcours a rebours voit les enfants d'abord.
    for (u32 k = static_cast<u32>(noeuds_.size()); k-- > 0;) {
      const Noeud& nd = noeuds_[k];
      u32 c;
      if (nd.gauche == kAucun) {
        c = comp_place_[nd.debut];
        for (u32 s = nd.debut + 1; s < nd.fin && c != kAucun; ++s) {
          if (comp_place_[s] != c) c = kAucun;
        }
      } else {
        const u32 g = comp_noeud_[nd.gauche];
        c = (g != kAucun && g == comp_noeud_[nd.droite]) ? g : kAucun;
      }
      comp_noeud_[k] = c;
    }
    for (u32 id = 0; id < n; ++id) {
      if (parent_uf_[id] == id) meilleur_[id] = Cle{};
    }
  }

  // Voisin exact memorise : encore exterieur, il reste le plus proche site exterieur.
  void servir_par_voisin(u32 j) {
    Etat& e = etat_[j];
    if (e.genre != kVoisin) return;
    const u32 c = comp_place_[j];
    const u32 q = e.cle.a;
    if (comp_id_[q] == c) {
      e.genre = kRien;
      return;
    }
    ++stats_.voisins_gardes;
    const u32 p = id_[j];
    const Cle candidat{e.cle.d2, std::min(p, q), std::max(p, q)};
    if (inferieure(candidat, meilleur_[c])) meilleur_[c] = candidat;
  }

  // Plus proche site exterieur a la composante c, borne par la meilleure arete de c (mise a jour en place).
  void requete(u32 j, u32 c) {
    ++stats_.requetes;
    Cle& borne = meilleur_[c];
    const Cle initiale = borne;
    const u32 p = id_[j];
    u32 voisin = kAucun;
    D d_voisin = 0;
    std::size_t haut = 0;
    if (comp_noeud_[0] != c) pile_requete_[haut++] = Item{0, distance_boite(j, noeuds_[0])};
    while (haut > 0) {
      const Item it = pile_requete_[--haut];
      if (it.dist > borne.d2) continue;
      const Noeud& nd = noeuds_[it.noeud];
      if (nd.gauche == kAucun) {
        for (u32 s = nd.debut; s < nd.fin; ++s) {
          if (comp_place_[s] == c) continue;
          ++stats_.distances;
          const D d2 = distance(j, s);
          if (d2 > borne.d2) continue;
          const u32 q = id_[s];
          const Cle candidat{d2, std::min(p, q), std::max(p, q)};
          if (!inferieure(candidat, borne)) continue;
          borne = candidat;
          voisin = q;
          d_voisin = d2;
        }
        continue;
      }
      const u32 g = nd.gauche, d = nd.droite;
      const bool g_vivant = comp_noeud_[g] != c, d_vivant = comp_noeud_[d] != c;
      const D dg = g_vivant ? distance_boite(j, noeuds_[g]) : 0;
      const D dd = d_vivant ? distance_boite(j, noeuds_[d]) : 0;
      // L'enfant le plus proche est empile en dernier, donc visite le premier.
      if (g_vivant && d_vivant && dg <= dd) {
        if (dd <= borne.d2) pile_requete_[haut++] = Item{d, dd};
        if (dg <= borne.d2) pile_requete_[haut++] = Item{g, dg};
      } else {
        if (g_vivant && dg <= borne.d2) pile_requete_[haut++] = Item{g, dg};
        if (d_vivant && dd <= borne.d2) pile_requete_[haut++] = Item{d, dd};
      }
    }
    Etat& e = etat_[j];
    if (voisin != kAucun) {
      e.cle = Cle{d_voisin, voisin, kAucun};
      e.genre = kVoisin;
    } else if (initiale.a != kAucun) {
      e.cle = initiale;
      e.genre = kMinorant;
    } else {
      e.genre = kRien;
    }
  }

  const Sites& sites_;
  StatsEmst& stats_;
  std::vector<Noeud> noeuds_;
  std::vector<u32> px_, py_, pz_, id_, place_de_;
  std::vector<u32> parent_uf_, taille_uf_, comp_id_, comp_place_, comp_noeud_;
  std::vector<Cle> meilleur_;
  std::vector<Etat> etat_;
  std::vector<Item> pile_requete_;
};

// Arbre couvrant minimal exact ; arithmetique u64 si toutes les coordonnees sont < 2^31 (somme de trois carres
// < 3 * 2^62 < 2^64), u128 sinon ou sur demande.
inline std::vector<Arete> emst_exact(const Sites& sites, bool forcer_u128, StatsEmst& stats, std::string& voie) {
  if (!forcer_u128 && sites.coordonnee_max < 0x80000000u) {
    voie = "u64";
    return Boruvka<u64>(sites, stats).calculer();
  }
  voie = "u128";
  return Boruvka<u128>(sites, stats).calculer();
}

}  // namespace mhgp12_emst
