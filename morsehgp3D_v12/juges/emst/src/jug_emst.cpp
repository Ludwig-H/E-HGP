// Juge d'echelle JUG-EMST de la v12 (hors produit) : l'ordre un de la tour FULL est l'arbre de fusion du lien simple
// sur l'arbre couvrant euclidien minimal exact des sites, avec plateaux, au niveau d2 / 4. Independant du catalogue,
// de la tour, du code du produit et de la v11.
//
//   mhgp12_jug_emst <nuage.u32le> [--ids <nuage.ids.u32le>] [--vidage <FULL MHGP11FUL1>] [--bits 21|24|32]
//                   [--arbre <sortie texte>] [--emst <sortie texte>] [--force-u128]
//
// Codes : 0 arbre calcule (et vidage identique s'il est donne) ; 1 ecart avec le vidage (premiere difference
// publiee) ; 2 usage ; 3 entree invalide (nuage ou vidage illisible, doublon, coordonnee hors du profil).
// Sortie : un objet JSON sur une ligne (comptes, empreintes SHA-256, temps), messages sur la sortie d'erreur.
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "emst.hpp"
#include "nuage.hpp"
#include "plateaux.hpp"
#include "vidage.hpp"

using namespace mhgp12_emst;

namespace {

struct Options {
  std::string nuage, ids, vidage, arbre, emst;
  u32 bits = 32;
  bool force_u128 = false;
};

int usage(const std::string& message) {
  std::cerr << "usage : " << message << "\n"
            << "  mhgp12_jug_emst <nuage.u32le> [--ids <ids.u32le>] [--vidage <FULL>] [--bits 21|24|32]\n"
            << "                  [--arbre <sortie>] [--emst <sortie>] [--force-u128]\n";
  return 2;
}

bool lire_options(int argc, char** argv, Options& o, std::string& erreur) {
  std::vector<std::string> positionnels;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto valeur = [&](std::string& cible) {
      if (i + 1 >= argc) {
        erreur = a + " sans valeur";
        return false;
      }
      cible = argv[++i];
      return true;
    };
    if (a == "--ids" || a == "--vidage" || a == "--arbre" || a == "--emst") {
      std::string& cible = a == "--ids" ? o.ids : a == "--vidage" ? o.vidage : a == "--arbre" ? o.arbre : o.emst;
      if (!cible.empty()) {
        erreur = a + " repete";
        return false;
      }
      if (!valeur(cible)) return false;
    } else if (a == "--bits") {
      std::string b;
      if (!valeur(b)) return false;
      if (b != "21" && b != "24" && b != "32") {
        erreur = "--bits " + b + " : 21, 24 ou 32 attendu";
        return false;
      }
      o.bits = static_cast<u32>(std::stoul(b));
    } else if (a == "--force-u128") {
      o.force_u128 = true;
    } else if (a.size() > 1 && a[0] == '-') {
      erreur = "option inconnue " + a;
      return false;
    } else {
      positionnels.push_back(a);
    }
  }
  if (positionnels.size() != 1) {
    erreur = "un seul nuage attendu";
    return false;
  }
  o.nuage = positionnels[0];
  return true;
}

double ms_depuis(std::chrono::steady_clock::time_point t) {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t).count();
}

std::string json_texte(const std::string& s) {
  std::string out = "\"";
  for (char c : s) {
    if (c == '"' || c == '\\') out += '\\';
    out += c;
  }
  return out + "\"";
}

}  // namespace

int main(int argc, char** argv) {
  Options o;
  std::string erreur;
  if (!lire_options(argc, argv, o, erreur)) return usage(erreur);
  const auto t0 = std::chrono::steady_clock::now();
  Sites sites;
  if (!lire_sites(o.nuage, sites, erreur)) {
    std::cerr << "jug_emst_entree_invalide : " << erreur << "\n";
    return 3;
  }
  if (o.bits < 32 && (sites.coordonnee_max >> o.bits) != 0) {
    std::cerr << "jug_emst_entree_invalide : coordonnee " << sites.coordonnee_max << " hors du profil " << o.bits
              << " bits\n";
    return 3;
  }
  std::vector<u32> ids;
  if (!o.ids.empty()) {
    if (!lire_mots(o.ids, 1, ids, erreur) || ids.size() != sites.taille()) {
      std::cerr << "jug_emst_entree_invalide : identifiants : "
                << (erreur.empty() ? "nombre different de celui des points" : erreur) << "\n";
      return 3;
    }
  }
  const double t_lecture = ms_depuis(t0);
  const auto t1 = std::chrono::steady_clock::now();
  StatsEmst stats_emst;
  std::string voie;
  const std::vector<Arete> aretes = emst_exact(sites, o.force_u128, stats_emst, voie);
  const double t_emst = ms_depuis(t1);
  const auto t2 = std::chrono::steady_clock::now();
  StatsArbre stats;
  const Arbre arbre = arbre_plateaux(sites.taille(), aretes, stats);
  const double t_arbre = ms_depuis(t2);
  const auto t3 = std::chrono::steady_clock::now();
  const std::string h_arbre = sha256_arbre(arbre, sites);
  const std::string h_fusions = sha256_fusions(arbre);
  const std::string h_emst = sha256_emst(sites.taille(), aretes);
  const double t_empreintes = ms_depuis(t3);
  if ((!o.arbre.empty() && !ecrire_arbre(o.arbre, arbre, sites)) || (!o.emst.empty() && !ecrire_emst(o.emst, aretes))) {
    std::cerr << "jug_emst_usage : sortie texte impossible a ecrire\n";
    return 2;
  }
  Verdict verdict;
  double t_vidage = 0;
  if (!o.vidage.empty()) {
    const auto t4 = std::chrono::steady_clock::now();
    const std::vector<u32> morton = ordre_morton(sites);
    ComparaisonVidage comparaison(sites, arbre, morton, o.ids.empty() ? nullptr : &ids);
    verdict = comparaison.juger(o.vidage);
    t_vidage = ms_depuis(t4);
  }
  std::cout << "{\"schema\":\"ehgp.v12.jug_emst.v1\",\"sites\":" << sites.taille() << ",\"arithmetique\":\"" << voie
            << "\",\"naissances\":" << arbre.naissances << ",\"fusions\":" << stats.fusions
            << ",\"multifusions\":" << stats.multifusions << ",\"arite_max\":" << stats.arite_max
            << ",\"niveaux_distincts\":" << stats.niveaux << ",\"niveaux_a_plusieurs_fusions\":"
            << stats.niveaux_plusieurs << ",\"aretes_emst\":" << aretes.size() << ",\"aretes_a_egalite\":"
            << stats.aretes_egalite << ",\"racine\":" << arbre.racine << ",\"sha256_arbre\":\"" << h_arbre
            << "\",\"sha256_fusions\":\"" << h_fusions << "\",\"sha256_emst\":\"" << h_emst
            << "\",\"boruvka\":{\"tours\":" << stats_emst.tours << ",\"requetes\":" << stats_emst.requetes
            << ",\"voisins_gardes\":" << stats_emst.voisins_gardes << ",\"minorants\":" << stats_emst.minorants
            << ",\"distances\":" << stats_emst.distances << ",\"profondeur_kd\":" << stats_emst.profondeur << "}";
  if (!o.vidage.empty()) {
    const char* nom = verdict.code == 0 ? "identique" : verdict.code == 1 ? "ecart" : "vidage_invalide";
    std::cout << ",\"vidage\":{\"verdict\":\"" << nom << "\",\"bits\":" << verdict.bits << ",\"kmax\":" << verdict.kmax
              << ",\"noeuds_ordre1\":" << verdict.noeuds << ",\"differences\":" << verdict.differences
              << ",\"octets_lus\":" << verdict.octets_lus << ",\"message\":" << json_texte(verdict.message) << "}";
  }
  std::cout << ",\"temps_ms\":{\"lecture\":" << t_lecture << ",\"emst\":" << t_emst << ",\"plateaux\":" << t_arbre
            << ",\"empreintes\":" << t_empreintes << ",\"vidage\":" << t_vidage << ",\"total\":" << ms_depuis(t0)
            << "},\"code\":" << verdict.code << "}\n";
  if (verdict.code == 1) std::cerr << "jug_emst_ecart : " << verdict.message << "\n";
  if (verdict.code == 3) std::cerr << "jug_emst_vidage_invalide : " << verdict.message << "\n";
  return verdict.code;
}
