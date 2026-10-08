// MES-G-APP : interface de l'executeur CUDA (appareil.cu), en types simples (aucun en-tete du produit, aucun type CUDA)
// pour que l'hote (mes_g_app.cpp, compile par g++) l'inclue. Les noyaux sont ceux de noyau_g.hpp (source unique).
#pragma once

#include <string>

#include "noyau_g.hpp"

namespace mesg {

struct Proprietes {
  std::string nom;
  int sm = 0, cc_majeur = 0, cc_mineur = 0, horloge_khz = 0, horloge_memoire_khz = 0, bus_bits = 0;
  u64 memoire = 0, l2 = 0;
  int pilote = 0, execution = 0;  // versions du pilote et de l'execution CUDA
};

// Donnees du census, residentes sur l'appareil apres charger_census.
struct DonneesCensus {
  const Noeud* noeuds = nullptr;
  u64 n_noeuds = 0;
  const u32 *x = nullptr, *y = nullptr, *z = nullptr;
  u32 n_sites = 0, taille_feuille = 0;
  const Requete* requetes = nullptr;
  u64 n_requetes = 0;
};

// Donnees des premieres sondes d'un ordre k, residentes apres charger_sondes.
struct DonneesSondes {
  u32 k = 0;
  const u32* fiches = nullptr;      // (3 + k) mots par fiche
  u64 entrees = 0;
  const u32* repertoire = nullptr;  // seaux + 1
  u64 seaux = 0;
  u32 decalage = 64;
  const u32* cellule_boule = nullptr;
  const u64* rep_debut = nullptr;   // cellules + 1
  u64 cellules = 0;
  const u64* masques = nullptr;     // representants
  u64 representants = 0;
};

// Catalogue (populations I puis U par boule), commun a tous les ordres.
struct DonneesCatalogue {
  const u64* pop_debut = nullptr;  // boules + 1
  const u32* pop_sites = nullptr;
  const u32* boule_p = nullptr;
  u64 boules = 0, incidences = 0;
};

// Parties proposees (traces dont la premiere sonde a echoue), pas fixe ; coordonnees : celles du census.
struct DonneesPropositions {
  const u32* parties = nullptr;  // n * pas SiteIdx
  const u32* tailles = nullptr;  // n
  u32 pas = 0;
  u64 n = 0;
};

struct Transferts {
  double h2d_ms = 0, d2h_ms = 0;
  u64 h2d_octets = 0, d2h_octets = 0;
};

// Contexte de l'appareil : ouvert une fois (regime resident, decision D1), tableaux gardes jusqu'a la destruction.
class Appareil {
 public:
  Appareil();
  ~Appareil();
  Appareil(const Appareil&) = delete;
  Appareil& operator=(const Appareil&) = delete;

  // Ouverture : premier appareil, flux, evenements ; faux et message si aucun appareil (ou construction sans CUDA).
  bool ouvrir(Proprietes& props, std::string& erreur);
  bool charger_census(const DonneesCensus& d, Transferts& t, std::string& erreur);
  // Un lancement du noyau de census (un fil par requete) ; duree du noyau seul (evenements).
  bool jouer_census(float& noyau_ms, std::string& erreur);
  // Resultats et sites (kSitesParRequete mots par requete : I puis U).
  bool lire_census(Resultat* resultats, u32* sites, Transferts& t, std::string& erreur);
  bool charger_catalogue(const DonneesCatalogue& d, Transferts& t, std::string& erreur);
  bool charger_sondes(const DonneesSondes& d, Transferts& t, std::string& erreur);  // un ordre ; appeler par ordre
  // Un lancement des premieres sondes de tous les ordres charges (un fil par cellule) ; duree des noyaux.
  bool jouer_sondes(float& noyau_ms, std::string& erreur);
  // Sondes de l'ordre d'indice i (ordre de chargement).
  bool lire_sondes(u32 i, u32* sondes, Transferts& t, std::string& erreur);
  // Propositions (DWelzl en binaire64, un fil par partie) ; exige charger_census (coordonnees).
  bool charger_propositions(const DonneesPropositions& d, Transferts& t, std::string& erreur);
  bool jouer_propositions(float& noyau_ms, std::string& erreur);
  bool lire_propositions(Proposition* sortie, Transferts& t, std::string& erreur);

  struct Etat;

 private:
  Etat* etat_;
};

}  // namespace mesg
