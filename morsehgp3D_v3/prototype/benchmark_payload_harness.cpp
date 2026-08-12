// MorseHGP3D v3 — SQUELETTE DU PAYLOAD OFFICIEL `BenchmarkOutputContract-v1`.
//
// Sujet : le point un de l'ordre de travail de
// `audits/AUDIT_ETAT_COURANT.md` — installer le squelette du contrat de sortie,
// son payload et l'interface verticale avec des producteurs explicitement
// `incomplete`, et taguer chaque chantier `slo_critical_path=yes/no`.
//
// POURQUOI CE FICHIER EXISTE. La section 14.4 du plan de tests chronometre
// `warm_e2e` depuis l'entree jusqu'au retour hote du payload : dix forets,
// applications verticales, lots et certificat minimal, copies en memoire hote
// EPINGLEE avant l'arret du chronometre. Tant que ce payload n'a pas de
// structure declaree, aucune mesure ne peut etre comparee au seuil, et aucun
// audit ne peut verifier ce qui manque. Ce programme ne calcule donc PAS la
// hierarchie : il declare l'inventaire des etages, leur statut, leur producteur
// et leur appartenance au chemin critique, puis REFUSE toute eligibilite.
//
// CE QU'IL N'EST PAS. Ce n'est ni un benchmark, ni une mesure, ni une preuve.
// Il n'imprime aucun temps de reference et ne peut pas rendre
// `slo_eligible=true` : la seule facon de le faire serait que tous les etages du
// chemin critique soient `complete` ET nomment un producteur et une porte
// existants. Le mutant `claim-complete` tente exactement cette fraude et doit
// etre refuse.
//
// Codes de sortie : 0 inventaire coherent, 2 refus avant calcul,
// 3 exigence de completude non satisfaite, 4 mutant tue.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

enum class Status { kComplete, kIncomplete, kAbsent };

const char* status_name(Status s) {
  switch (s) {
    case Status::kComplete: return "complete";
    case Status::kIncomplete: return "incomplete";
    case Status::kAbsent: return "absent";
  }
  return "?";
}

// Un etage du pipeline officiel.
//
// `critical` dit si l'etage est DANS le chronometre `warm_e2e` de la section
// 14.4. Un etage hors chemin critique — catalogue de replay complet, expansion
// des unions de points, ecriture disque — est chronometre separement et
// n'empeche pas l'eligibilite.
//
// `producer` nomme le fichier qui le realise, `gate` la porte CTest qui le
// recoit. Un etage `complete` sans producteur ni porte est une fraude.
struct Stage {
  const char* name;
  bool critical;
  Status status;
  const char* producer;
  const char* gate;
  const char* note;
};

// L'INVENTAIRE. Il est la source unique de verite de ce qui existe et de ce qui
// manque. Toute promotion de statut doit s'accompagner d'un producteur et d'une
// porte, sans quoi le controle de coherence la refuse.
const Stage kStages[] = {
    {"entree_u16_validee", true, Status::kIncomplete, "prototype/cloud_families.hpp",
     "mhgp3v_cloud_family_gate",
     "les generateurs dedupliquent; la politique de doublons du produit n'est pas fixee"},
    {"transfert_hote_device", true, Status::kAbsent, "", "",
     "aucun noyau CUDA de cette route n'existe"},
    {"index_morton_lbvh_resident", true, Status::kIncomplete, "prototype/morton_lbvh.hpp",
     "",
     "construction hote recursive avec std::sort; aucune residence device recue"},
    {"source_q2_q3_q4_cellules_centres", true, Status::kIncomplete,
     "prototype/centre_cell_source.cpp", "mhgp3v_centre_cell_uniform",
     "exacte, jugee par un oracle rationnel independant, CPU seulement; deux pentes "
     "uniform vertes sur tous les compteurs a 12,5/25/50 k"},
    {"census_ferme_I_U_ballkey", true, Status::kIncomplete,
     "prototype/centre_cell_source.cpp", "mhgp3v_centre_cell_fixture_coquille",
     "census stratifie par budget, juge sur les identites (support, I_B, U_B) par "
     "elimination de Gauss rationnelle; RLE par cle primitive de sphere absent"},
    {"lane_k1_gabriel_kruskal_sparse", true, Status::kIncomplete,
     "prototype/centre_cell_source.cpp", "mhgp3v_centre_cell_k1_uniform",
     "EMST exact par les supports q2 a zero interieur, juge par un Prim exhaustif "
     "independant; il manque la politique de doublons et le raccord au fold"},
    {"ball_activation", true, Status::kAbsent, "", "",
     "aucun producteur; prerequis des facettes du coeur"},
    {"facettes_du_coeur", true, Status::kAbsent, "", "",
     "au plus quatre bras stricts par evenement direct, non implemente"},
    {"premieres_incidences_J_F", true, Status::kIncomplete,
     "prototype/first_incidence_dichotomy.cpp", "mhgp3v_first_incidence_e5",
     "probe borne des trois branches; pas de source globale"},
    {"gateways_et_resolver_strict", true, Status::kAbsent, "", "",
     "descente stricte memoisee vers un carrier pre-lot, non implementee"},
    {"msf_carriers_ou_fold_atomique", true, Status::kAbsent, "", "",
     "fold DSU par lots (k,beta) atomiques, non implemente"},
    {"dix_forets_hgp_reduced", true, Status::kAbsent, "", "",
     "profil hgp_reduced, K=10; aucune foret materialisee"},
    {"applications_verticales", true, Status::kAbsent, "", "",
     "hors contrat horizontal; specification propre exigee"},
    {"lots_k_beta_atomiques", true, Status::kAbsent, "", "",
     "groupement par niveau exact avant toute mutation DSU"},
    {"certificat_minimal", true, Status::kAbsent, "", "",
     "journal de niveaux, racines, parents et deltas de couverture"},
    {"copie_hote_epinglee", true, Status::kAbsent, "", "",
     "le chronometre s'arrete apres cette copie, pas avant"},
    {"catalogue_replay_complet", false, Status::kAbsent, "", "",
     "hors chronometre par la section 14.4"},
    {"expansion_unions_de_points", false, Status::kAbsent, "", "",
     "hors chronometre par la section 14.4"},
    {"ecriture_disque", false, Status::kAbsent, "", "",
     "hors chronometre par la section 14.4"},
};

constexpr int kStageCount = (int)(sizeof(kStages) / sizeof(kStages[0]));

// LA PORTE STRUCTURELLE de la section 14.4 precede tout chronometrage. Elle
// exige zero allocation indexee par les univers de facettes ou de cofaces, zero
// cellule top-m persistee et zero construction Gamma dans le target produit.
// Ces trois interdits sont declares ici pour que la porte existe AVANT le
// producteur; ils seront alimentes par les compteurs du producteur reel.
struct StructuralBan {
  const char* name;
  const char* meaning;
};

const StructuralBan kBans[] = {
    {"alloc_indexee_par_univers_de_facettes", "zero allocation en O(C(n,k))"},
    {"alloc_indexee_par_univers_de_cofaces", "zero allocation en O(C(n,k+1))"},
    {"cellule_top_m_persistee", "aucune cellule top-m ne survit a sa feuille"},
    {"construction_gamma_dans_le_target", "Gamma reste un oracle borne, jamais le produit"},
};

constexpr int kBanCount = (int)(sizeof(kBans) / sizeof(kBans[0]));

int parse_int(const char* text, bool* ok) {
  char* end = nullptr;
  const long long value = std::strtoll(text, &end, 10);
  *ok = end != nullptr && *end == '\0' && end != text && value >= 0 && value <= 100000000LL;
  return (int)value;
}

}  // namespace

int main(int argc, char** argv) {
  bool require_complete = false;
  std::string inject;
  int points = 50000;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    bool ok = true;
    if (arg == "--require-complete") require_complete = true;
    else if (arg.rfind("--points=", 0) == 0) points = parse_int(arg.substr(9).c_str(), &ok);
    else if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else {
      std::fprintf(stderr, "REFUS argument inconnu: %s\n", arg.c_str());
      return 2;
    }
    if (!ok) {
      std::fprintf(stderr, "REFUS valeur invalide: %s\n", arg.c_str());
      return 2;
    }
  }
  if (!inject.empty() && inject != "claim-complete") {
    std::fprintf(stderr, "REFUS injection inconnue: %s\n", inject.c_str());
    return 2;
  }
  if (points < 1) {
    std::fprintf(stderr, "REFUS domaine\n");
    return 2;
  }

  std::printf("BenchmarkOutputContractSkeleton-v1\n");
  std::printf("profile=hgp_reduced k_max=10 points=%d\n", points);
  std::printf("backend=cpu_reference mode=inventory public_status=not_claimed\n");
  std::printf("inject=%s\n", inject.empty() ? "aucun" : inject.c_str());

  int critical = 0, critical_complete = 0, absent = 0, incomplete = 0;
  int fraudulent = 0;
  for (int i = 0; i < kStageCount; ++i) {
    const Stage& s = kStages[i];
    // LE MUTANT : promouvoir un etage sans lui donner de producteur ni de porte.
    const Status status =
        inject == "claim-complete" ? Status::kComplete : s.status;
    const bool has_producer = s.producer[0] != '\0';
    const bool has_gate = s.gate[0] != '\0';
    if (status == Status::kComplete && (!has_producer || !has_gate)) ++fraudulent;
    if (s.critical) {
      ++critical;
      if (status == Status::kComplete) ++critical_complete;
    }
    if (status == Status::kAbsent) ++absent;
    if (status == Status::kIncomplete) ++incomplete;
    std::printf("etage=%s slo_critical_path=%s statut=%s producteur=%s porte=%s note=%s\n",
                s.name, s.critical ? "yes" : "no", status_name(status),
                has_producer ? s.producer : "-", has_gate ? s.gate : "-", s.note);
  }

  for (int i = 0; i < kBanCount; ++i)
    std::printf("interdit_structurel=%s sens=%s statut=non_instrumente\n", kBans[i].name,
                kBans[i].meaning);

  std::printf("etages=%d chemin_critique=%d critique_complets=%d incomplets=%d absents=%d\n",
              kStageCount, critical, critical_complete, incomplete, absent);

  // LE CONTROLE DE COHERENCE. Un etage `complete` doit nommer un producteur ET
  // une porte. C'est ce controle, et non une convention, qui interdit de
  // declarer un pipeline pret sans code ni reception.
  if (fraudulent > 0) {
    std::printf("INCOHERENT: %d etages complete sans producteur ni porte\n", fraudulent);
    if (!inject.empty()) {
      std::printf("MUTANT TUE: %s\n", inject.c_str());
      return 4;
    }
    std::fprintf(stderr, "REFUS : inventaire incoherent\n");
    return 3;
  }
  if (!inject.empty()) {
    std::fprintf(stderr, "MUTANT SURVIVANT: %s\n", inject.c_str());
    return 3;
  }

  // L'ELIGIBILITE. Elle exige que TOUT le chemin critique soit complet. Aucune
  // moyenne, aucun sous-ensemble, aucun temps de kernel ne s'y substitue.
  const bool eligible = (critical_complete == critical);
  std::printf("warm_e2e_mesurable=%s slo_eligible=%s\n", eligible ? "oui" : "non",
              eligible ? "oui" : "non");
  std::printf("raison=%s\n",
              eligible ? "chemin critique complet"
                       : "producteurs manquants sur le chemin critique");

  if (require_complete && !eligible) {
    std::fprintf(stderr,
                 "EXIGENCE non satisfaite : %d etages du chemin critique sur %d\n",
                 critical_complete, critical);
    return 3;
  }
  return 0;
}
