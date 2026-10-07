// Lecture stricte d'un vidage MHGP11FUL1 (en-tete, sites, ordre un ; les ordres superieurs ne sont pas lus) et
// comparaison champ par champ a l'arbre canonique du juge JUG-EMST.
//
// Format (bench/full_probe.cpp et bench/full_semantic.py de la v11 gelee ac081a06f) : "MHGP11FUL1", puis mots u64
// petit-boutistes : bits, kmax, sites, points ; par site (ordre de Morton exact) x, y, z, poids, PointId ; par ordre :
// ordre, naissances, noeuds, aretes, racine ; par noeud : parent, debut, cardinal, niveau (entier exact numerateur,
// entier exact denominateur ; un entier exact = signe, nombre de mots (1 a 64), mots), centre des naissances (trois
// numerateurs et un denominateur exacts), image verticale (ordres >= 2 seulement) ; enfin les enfants (CSR).
//
// Deux verdicts : entree invalide (code 3) quand le fichier ne peut pas etre lu comme ce format ; ecart (code 1)
// quand il se lit mais differe de l'arbre attendu. Toute difference de champ (parent, debut, cardinal, niveau exact,
// centre exact, enfant) est un ecart ; l'egalite de tous les champs donne l'arbre attendu, donc un vidage coherent.
#pragma once
#include <algorithm>
#include <array>
#include <fstream>
#include <string>
#include <vector>

#include "entier.hpp"
#include "nuage.hpp"
#include "plateaux.hpp"

namespace mhgp12_emst {

class Lecteur {
 public:
  bool ouvrir(const std::string& chemin) {
    flux_.open(chemin, std::ios::binary);
    tampon_.resize(1 << 20);
    return static_cast<bool>(flux_);
  }
  bool octets(unsigned char* sortie, std::size_t n) {
    while (n > 0) {
      if (debut_ == fin_ && !remplir()) return false;
      const std::size_t pris = std::min(n, fin_ - debut_);
      std::copy(tampon_.begin() + static_cast<std::ptrdiff_t>(debut_),
                tampon_.begin() + static_cast<std::ptrdiff_t>(debut_ + pris), sortie);
      debut_ += pris;
      sortie += pris;
      n -= pris;
      lus_ += pris;
    }
    return true;
  }
  bool mot(u64& valeur) {
    unsigned char b[8];
    if (!octets(b, 8)) return false;
    valeur = 0;
    for (int i = 7; i >= 0; --i) valeur = (valeur << 8) | b[i];
    return true;
  }
  u64 lus() const { return lus_; }

 private:
  bool remplir() {
    flux_.read(reinterpret_cast<char*>(tampon_.data()), static_cast<std::streamsize>(tampon_.size()));
    debut_ = 0;
    fin_ = static_cast<std::size_t>(flux_.gcount());
    return fin_ > 0;
  }
  std::ifstream flux_;
  std::vector<unsigned char> tampon_;
  std::size_t debut_ = 0, fin_ = 0;
  u64 lus_ = 0;
};

struct Verdict {
  int code = 0;               // 0 identique, 1 ecart, 3 vidage invalide
  std::string message;        // premiere difference ou cause du refus
  u64 differences = 0;        // champs differents (lecture menee jusqu'a la fin de l'ordre un)
  u64 bits = 0, kmax = 0, sites = 0, noeuds = 0;
  u64 octets_lus = 0;
};

class ComparaisonVidage {
 public:
  ComparaisonVidage(const Sites& sites, const Arbre& arbre, const std::vector<u32>& morton,
                    const std::vector<u32>* ids)
      : sites_(sites), arbre_(arbre), morton_(morton), ids_(ids) {}

  Verdict juger(const std::string& chemin) {
    Verdict v;
    if (!lecteur_.ouvrir(chemin)) return refus(v, "vidage illisible : " + chemin);
    if (!lire_en_tete(v) || !lire_sites(v) || !lire_ordre_un(v)) {
      v.octets_lus = lecteur_.lus();
      return v;
    }
    v.octets_lus = lecteur_.lus();
    v.differences = differences_;
    if (differences_ > 0) {
      // La premiere difference de noeud dit ou l'arbre diverge ; les ecarts d'en-tete (taille, racine)
      // l'accompagnent.
      v.code = 1;
      v.message = premiere_.empty() ? taille_ : taille_.empty() ? premiere_ : premiere_ + " ; " + taille_;
    }
    return v;
  }

 private:
  struct Entier {
    bool negatif = false;
    Naturel valeur;
  };

  static Verdict& refus(Verdict& v, const std::string& message) {
    v.code = 3;
    v.message = message;
    return v;
  }
  void ecart(const std::string& description) {
    if (premiere_.empty()) premiere_ = description;
    ++differences_;
  }
  bool mot(Verdict& v, u64& valeur, const char* quoi) {
    if (lecteur_.mot(valeur)) return true;
    refus(v, std::string("vidage tronque (") + quoi + ")");
    return false;
  }
  bool entier(Verdict& v, Entier& e, const char* quoi) {
    u64 signe, mots;
    if (!mot(v, signe, quoi) || !mot(v, mots, quoi)) return false;
    if (signe > 1 || mots < 1 || mots > 64) {
      refus(v, std::string("entier exact mal forme (") + quoi + ") : signe " + std::to_string(signe) + ", " +
                   std::to_string(mots) + " mots");
      return false;
    }
    e.negatif = signe == 1;
    e.valeur.mots.resize(mots);
    for (u64 i = 0; i < mots; ++i) {
      if (!mot(v, e.valeur.mots[i], quoi)) return false;
    }
    e.valeur.normaliser();
    if (e.negatif && e.valeur.nul()) {
      refus(v, std::string("zero negatif (") + quoi + ")");
      return false;
    }
    return true;
  }

  bool lire_en_tete(Verdict& v) {
    unsigned char signature[10];
    if (!lecteur_.octets(signature, 10) || std::string(reinterpret_cast<char*>(signature), 10) != "MHGP11FUL1") {
      refus(v, "signature MHGP11FUL1 absente");
      return false;
    }
    u64 points;
    if (!mot(v, v.bits, "en-tete") || !mot(v, v.kmax, "en-tete") || !mot(v, v.sites, "en-tete") ||
        !mot(v, points, "en-tete")) {
      return false;
    }
    if (v.bits != 18 && v.bits != 21 && v.bits != 24 && v.bits != 32) {
      refus(v, "profil " + std::to_string(v.bits) + " : 18, 21, 24 ou 32 attendu");
      return false;
    }
    if (v.kmax < 1 || v.kmax > 12 || v.sites < 1 || v.sites >= kAucun || v.kmax > v.sites || points != v.sites) {
      refus(v, "en-tete : kmax " + std::to_string(v.kmax) + ", sites " + std::to_string(v.sites) + ", points " +
                   std::to_string(points) + " (poids unitaires et 1 <= kmax <= min(sites, 12) attendus)");
      return false;
    }
    if (v.sites != sites_.taille()) {
      v.code = 1;
      v.message = "nombre de sites : vidage " + std::to_string(v.sites) + ", nuage " + std::to_string(sites_.taille());
      v.differences = 1;
      return false;
    }
    return true;
  }

  bool lire_sites(Verdict& v) {
    std::vector<u32> identites;
    for (u64 s = 0; s < v.sites; ++s) {
      std::array<u64, 5> w{};
      for (u64& x : w) {
        if (!mot(v, x, "sites")) return false;
      }
      if (w[3] != 1) {
        refus(v, "site " + std::to_string(s) + " : poids " + std::to_string(w[3]) + " (1 attendu)");
        return false;
      }
      if ((w[0] >> v.bits) != 0 || (w[1] >> v.bits) != 0 || (w[2] >> v.bits) != 0 || w[4] > kAucun) {
        refus(v, "site " + std::to_string(s) + " hors du domaine du profil ou PointId > 2^32 - 1");
        return false;
      }
      const u32 attendu = morton_[s];
      if (w[0] != sites_.x[attendu] || w[1] != sites_.y[attendu] || w[2] != sites_.z[attendu]) {
        ecart("site " + std::to_string(s) + " (ordre de Morton) : vidage (" + std::to_string(w[0]) + ", " +
              std::to_string(w[1]) + ", " + std::to_string(w[2]) + "), attendu (" + std::to_string(sites_.x[attendu]) +
              ", " + std::to_string(sites_.y[attendu]) + ", " + std::to_string(sites_.z[attendu]) + ")");
      }
      if (ids_ != nullptr) {
        const u32 id = (*ids_)[sites_.entree[attendu]];
        if (w[4] != id) {
          ecart("site " + std::to_string(s) + " : PointId " + std::to_string(w[4]) + ", attendu " + std::to_string(id));
        }
      } else {
        identites.push_back(static_cast<u32>(w[4]));
      }
    }
    std::sort(identites.begin(), identites.end());
    if (std::adjacent_find(identites.begin(), identites.end()) != identites.end()) {
      refus(v, "PointId en double dans les sites du vidage");
      return false;
    }
    return true;
  }

  bool lire_ordre_un(Verdict& v) {
    u64 ordre, naissances, compte, aretes, racine;
    if (!mot(v, ordre, "ordre 1") || !mot(v, naissances, "ordre 1") || !mot(v, compte, "ordre 1") ||
        !mot(v, aretes, "ordre 1") || !mot(v, racine, "ordre 1")) {
      return false;
    }
    if (ordre != 1 || naissances != v.sites || compte < naissances || compte >= kAucun ||
        compte > 2 * naissances - 1 || aretes != compte - 1 || racine >= compte) {
      refus(v, "en-tete de l'ordre 1 : ordre " + std::to_string(ordre) + ", naissances " + std::to_string(naissances) +
                   ", noeuds " + std::to_string(compte) + ", aretes " + std::to_string(aretes) + ", racine " +
                   std::to_string(racine));
      return false;
    }
    v.noeuds = compte;
    if (compte != arbre_.noeuds()) {
      taille_ = "ordre 1 : " + std::to_string(compte) + " noeuds, attendu " + std::to_string(arbre_.noeuds()) + " (" +
                std::to_string(compte - naissances) + " fusions contre " +
                std::to_string(arbre_.noeuds() - naissances) + ")";
      ++differences_;
    }
    if (racine != arbre_.racine) {
      taille_ += std::string(taille_.empty() ? "" : " ; ") + "ordre 1 : racine " + std::to_string(racine) +
                 ", attendu " + std::to_string(arbre_.racine);
      ++differences_;
    }
    for (u64 k = 0; k < compte; ++k) {
      if (!lire_noeud(v, k, naissances)) return false;
    }
    for (u64 e = 0; e < aretes; ++e) {
      u64 enfant;
      if (!mot(v, enfant, "enfants")) return false;
      if (e >= arbre_.enfants.size() || enfant != arbre_.enfants[e]) {
        ecart("ordre 1, enfant " + std::to_string(e) + " de la CSR : " + std::to_string(enfant) + ", attendu " +
              (e < arbre_.enfants.size() ? std::to_string(arbre_.enfants[e]) : std::string("rien")));
      }
    }
    return true;
  }

  bool lire_noeud(Verdict& v, u64 k, u64 naissances) {
    u64 parent, debut, cardinal;
    if (!mot(v, parent, "noeud") || !mot(v, debut, "noeud") || !mot(v, cardinal, "noeud")) return false;
    Entier num, den;
    if (!entier(v, num, "niveau") || !entier(v, den, "niveau")) return false;
    if (num.negatif || den.negatif || den.valeur.nul()) {
      refus(v, "ordre 1, noeud " + std::to_string(k) + " : niveau de signe invalide");
      return false;
    }
    std::array<Entier, 4> centre;
    if (k < naissances) {
      for (Entier& c : centre) {
        if (!entier(v, c, "centre")) return false;
      }
      if (centre[3].negatif || centre[3].valeur.nul()) {
        refus(v, "ordre 1, naissance " + std::to_string(k) + " : denominateur du centre <= 0");
        return false;
      }
    }
    if (k >= arbre_.noeuds()) return true;  // noeud en trop : deja compte comme ecart par l'en-tete
    const std::string ici = "ordre 1, noeud " + std::to_string(k);
    const u64 parent_attendu = arbre_.parent[k];
    if (parent != parent_attendu) {
      ecart(ici + " : parent " + std::to_string(parent) + ", attendu " + std::to_string(parent_attendu));
    }
    if (debut != arbre_.debut[k] || cardinal != arbre_.cardinal[k]) {
      ecart(ici + " : enfants [" + std::to_string(debut) + ", +" + std::to_string(cardinal) + "), attendu [" +
            std::to_string(arbre_.debut[k]) + ", +" + std::to_string(arbre_.cardinal[k]) + ")");
    }
    // Niveau : num / den = d2 / 4 exactement, soit 4 num = d2 den (naissances : num = 0).
    const Naturel gauche = produit(num.valeur, naturel(4));
    const Naturel droite = produit(naturel(arbre_.d2[k]), den.valeur);
    if (comparer(gauche, droite) != 0) {
      u128 an, ad;
      niveau_reduit(arbre_.d2[k], an, ad);
      ecart(ici + " : niveau " + decimal(num.valeur) + "/" + decimal(den.valeur) + ", attendu " + decimal128(an) +
            "/" + decimal128(ad));
    }
    if (k < naissances) {
      const u32 xyz[3] = {sites_.x[k], sites_.y[k], sites_.z[k]};
      for (int a = 0; a < 3; ++a) {
        if (centre[a].negatif || comparer(centre[a].valeur, produit(naturel(xyz[a]), centre[3].valeur)) != 0) {
          ecart(ici + " : centre de naissance different du site (" + std::to_string(xyz[0]) + ", " +
                std::to_string(xyz[1]) + ", " + std::to_string(xyz[2]) + ")");
          break;
        }
      }
    }
    return true;
  }

  const Sites& sites_;
  const Arbre& arbre_;
  const std::vector<u32>& morton_;
  const std::vector<u32>* ids_;
  Lecteur lecteur_;
  u64 differences_ = 0;
  std::string premiere_, taille_;
};

}  // namespace mhgp12_emst
