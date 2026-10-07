// Arbre de fusion canonique d'ordre un du juge JUG-EMST : lien simple sur l'arbre couvrant minimal, avec plateaux.
//
// A un niveau d2 donne, les composantes de la coupe ouverte (aretes de longueur < d2) reunies par les aretes de
// longueur exactement d2 forment, par groupe connexe, UNE fusion N-aire (jamais une chaine de fusions binaires), au
// niveau d2 / 4 (rayon carre de la boule diametrale). Les aretes d'un meme niveau forment une foret sur les
// composantes de la coupe ouverte : deux aretes de l'arbre couvrant entre les memes composantes fermeraient un cycle.
//
// Numerotation canonique (celle de l'oracle borne et des vidages MHGP11FUL1) : naissances d'abord, par (niveau 0,
// position (x, y, z)) ; fusions ensuite, par (niveau, plus petite naissance du sous-arbre) ; enfants tries.
#pragma once
#include <algorithm>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "emst.hpp"
#include "entier.hpp"
#include "nuage.hpp"
#include "sha256.hpp"

namespace mhgp12_emst {

struct Arbre {
  u32 naissances = 0;
  std::vector<u128> d2;           // par noeud : distance carree de la fusion (niveau d2 / 4) ; 0 pour une naissance
  std::vector<u32> parent;        // kAucun pour la racine
  std::vector<u32> debut, cardinal;  // CSR des enfants (naissances : 0, 0)
  std::vector<u32> enfants;
  std::vector<u32> plus_petite;   // plus petite naissance du sous-arbre
  u32 racine = 0;
  u32 noeuds() const { return static_cast<u32>(parent.size()); }
};

struct StatsArbre {
  u64 fusions = 0, multifusions = 0, arite_max = 0, niveaux = 0, niveaux_plusieurs = 0, aretes_egalite = 0;
};

class UnionPlateau {
 public:
  explicit UnionPlateau(u32 n) : parent_(n), taille_(n, 1) {
    for (u32 i = 0; i < n; ++i) parent_[i] = i;
  }
  u32 trouver(u32 i) {
    while (parent_[i] != i) {
      parent_[i] = parent_[parent_[i]];
      i = parent_[i];
    }
    return i;
  }
  u32 unir(u32 a, u32 b) {
    a = trouver(a);
    b = trouver(b);
    if (a == b) return a;
    if (taille_[a] < taille_[b]) std::swap(a, b);
    parent_[b] = a;
    taille_[a] += taille_[b];
    return a;
  }

 private:
  std::vector<u32> parent_, taille_;
};

// aretes : arbre couvrant minimal trie par (d2, a, b).
inline Arbre arbre_plateaux(u32 n, const std::vector<Arete>& aretes, StatsArbre& stats) {
  Arbre arbre;
  arbre.naissances = n;
  arbre.d2.assign(n, 0);
  arbre.parent.assign(n, kAucun);
  arbre.debut.assign(n, 0);
  arbre.cardinal.assign(n, 0);
  arbre.plus_petite.resize(n);
  for (u32 i = 0; i < n; ++i) arbre.plus_petite[i] = i;
  UnionPlateau uf(n);
  std::vector<u32> noeud_de(n);  // racine union-find -> noeud courant de sa composante
  for (u32 i = 0; i < n; ++i) noeud_de[i] = i;
  std::vector<std::pair<u32, u32>> anciens;  // (racine nouvelle, noeud ancien)
  std::vector<std::pair<u32, u32>> racines;  // (ra, rb) a la coupe ouverte
  struct Nouvelle {
    u32 racine, plus_petite, debut, fin;  // tranche de `anciens`
  };
  std::vector<Nouvelle> nouvelles;
  std::size_t i = 0;
  while (i < aretes.size()) {
    std::size_t j = i;
    while (j < aretes.size() && aretes[j].d2 == aretes[i].d2) ++j;
    const u128 d2 = aretes[i].d2;
    ++stats.niveaux;
    if (j - i > 1) stats.aretes_egalite += j - i;
    racines.clear();
    for (std::size_t e = i; e < j; ++e) racines.emplace_back(uf.trouver(aretes[e].a), uf.trouver(aretes[e].b));
    anciens.clear();
#ifdef MHGP12_MUTANT_BINAIRE
    // MUTANT CAUSAL (copie compilee a part) : une fusion binaire par arete, en chaine, au lieu du plateau N-aire.
    nouvelles.clear();
    for (std::size_t e = 0; e < racines.size(); ++e) {
      const u32 na = noeud_de[uf.trouver(racines[e].first)], nb = noeud_de[uf.trouver(racines[e].second)];
      const u32 r = uf.unir(racines[e].first, racines[e].second);
      const u32 debut = static_cast<u32>(anciens.size());
      anciens.emplace_back(r, std::min(na, nb));
      anciens.emplace_back(r, std::max(na, nb));
      const u32 id = arbre.noeuds();
      arbre.d2.push_back(d2);
      arbre.parent.push_back(kAucun);
      arbre.debut.push_back(static_cast<u32>(arbre.enfants.size()));
      arbre.cardinal.push_back(2);
      arbre.plus_petite.push_back(std::min(arbre.plus_petite[na], arbre.plus_petite[nb]));
      for (u32 k = debut; k < debut + 2; ++k) {
        arbre.enfants.push_back(anciens[k].second);
        arbre.parent[anciens[k].second] = id;
      }
      noeud_de[r] = id;
      ++stats.fusions;
    }
    (void)nouvelles;
#else
    for (const auto& r : racines) uf.unir(r.first, r.second);
    for (const auto& r : racines) {
      const u32 nouvelle = uf.trouver(r.first);
      anciens.emplace_back(nouvelle, noeud_de[r.first]);
      anciens.emplace_back(nouvelle, noeud_de[r.second]);
    }
    std::sort(anciens.begin(), anciens.end());
    anciens.erase(std::unique(anciens.begin(), anciens.end()), anciens.end());
    nouvelles.clear();
    for (std::size_t a = 0; a < anciens.size();) {
      std::size_t b = a;
      u32 petite = kAucun;
      while (b < anciens.size() && anciens[b].first == anciens[a].first) {
        petite = std::min(petite, arbre.plus_petite[anciens[b].second]);
        ++b;
      }
      nouvelles.push_back({anciens[a].first, petite, static_cast<u32>(a), static_cast<u32>(b)});
      a = b;
    }
    std::sort(nouvelles.begin(), nouvelles.end(),
              [](const Nouvelle& p, const Nouvelle& q) { return p.plus_petite < q.plus_petite; });
    if (nouvelles.size() > 1) ++stats.niveaux_plusieurs;
    for (const Nouvelle& f : nouvelles) {
      const u32 id = arbre.noeuds();
      const u32 arite = f.fin - f.debut;
      arbre.d2.push_back(d2);
      arbre.parent.push_back(kAucun);
      arbre.debut.push_back(static_cast<u32>(arbre.enfants.size()));
      arbre.cardinal.push_back(arite);
      arbre.plus_petite.push_back(f.plus_petite);
      for (u32 k = f.debut; k < f.fin; ++k) {  // noeuds anciens tries : enfants tries
        arbre.enfants.push_back(anciens[k].second);
        arbre.parent[anciens[k].second] = id;
      }
      noeud_de[f.racine] = id;
      ++stats.fusions;
      if (arite >= 3) ++stats.multifusions;
      stats.arite_max = std::max<u64>(stats.arite_max, arite);
    }
#endif
    i = j;
  }
  arbre.racine = arbre.noeuds() - 1;  // une seule fusion au niveau maximal : la derniere
  return arbre;
}

// Entier naturel en octets : longueur sur un mot de 64 bits puis octets petit-boutistes (au moins un), comme la
// fonction natural() de bench/catalogue_semantic.py de la v11.
inline void naturel_empreinte(Sha256& h, u128 v) {
  u8 octets[16];
  int longueur = 0;
  do {
    octets[longueur++] = static_cast<u8>(v);
    v >>= 8;
  } while (v != 0);
  h.mot(static_cast<u64>(longueur));
  h.ajouter(octets, static_cast<std::size_t>(longueur));
}

inline void empreinte_fusions(Sha256& h, const Arbre& arbre) {
  h.mot(arbre.noeuds() - arbre.naissances);
  for (u32 k = arbre.naissances; k < arbre.noeuds(); ++k) {
    u128 num, den;
    niveau_reduit(arbre.d2[k], num, den);
    naturel_empreinte(h, num);
    naturel_empreinte(h, den);
    h.mot(arbre.cardinal[k]);
    for (u32 c = 0; c < arbre.cardinal[k]; ++c) h.mot(arbre.enfants[arbre.debut[k] + c]);
  }
}

// Empreinte de l'arbre canonique : schema, sites (naissances), puis fusions (niveau reduit, enfants).
inline std::string sha256_arbre(const Arbre& arbre, const Sites& sites) {
  Sha256 h;
  const std::string schema = "ehgp.v12.jug_emst.ordre1.v1";
  h.ajouter(schema.data(), schema.size() + 1);
  h.mot(arbre.naissances);
  for (u32 i = 0; i < arbre.naissances; ++i) {
    h.mot(sites.x[i]);
    h.mot(sites.y[i]);
    h.mot(sites.z[i]);
  }
  empreinte_fusions(h, arbre);
  return h.hex();
}

// Empreinte des seules fusions : invariante par translation (les numeros de naissance le sont).
inline std::string sha256_fusions(const Arbre& arbre) {
  Sha256 h;
  const std::string schema = "ehgp.v12.jug_emst.fusions.v1";
  h.ajouter(schema.data(), schema.size() + 1);
  h.mot(arbre.naissances);
  empreinte_fusions(h, arbre);
  return h.hex();
}

// Empreinte de l'arbre couvrant minimal (unique pour l'ordre strict) : aretes triees (d2, a, b).
inline std::string sha256_emst(u32 n, const std::vector<Arete>& aretes) {
  Sha256 h;
  const std::string schema = "ehgp.v12.jug_emst.emst.v1";
  h.ajouter(schema.data(), schema.size() + 1);
  h.mot(n);
  for (const Arete& e : aretes) {
    naturel_empreinte(h, e.d2);
    h.mot(e.a);
    h.mot(e.b);
  }
  return h.hex();
}

// Forme texte de l'arbre canonique (portes de l'oracle) : "N n", n lignes "B x y z", puis "F num den c1 c2 ...".
inline bool ecrire_arbre(const std::string& chemin, const Arbre& arbre, const Sites& sites) {
  std::ofstream sortie(chemin, std::ios::trunc);
  if (!sortie) return false;
  sortie << "N " << arbre.naissances << '\n';
  for (u32 i = 0; i < arbre.naissances; ++i) sortie << "B " << sites.x[i] << ' ' << sites.y[i] << ' ' << sites.z[i] << '\n';
  for (u32 k = arbre.naissances; k < arbre.noeuds(); ++k) {
    u128 num, den;
    niveau_reduit(arbre.d2[k], num, den);
    sortie << "F " << decimal128(num) << ' ' << decimal128(den);
    for (u32 c = 0; c < arbre.cardinal[k]; ++c) sortie << ' ' << arbre.enfants[arbre.debut[k] + c];
    sortie << '\n';
  }
  return static_cast<bool>(sortie);
}

inline bool ecrire_emst(const std::string& chemin, const std::vector<Arete>& aretes) {
  std::ofstream sortie(chemin, std::ios::trunc);
  if (!sortie) return false;
  for (const Arete& e : aretes) sortie << decimal128(e.d2) << ' ' << e.a << ' ' << e.b << '\n';
  return static_cast<bool>(sortie);
}

}  // namespace mhgp12_emst
