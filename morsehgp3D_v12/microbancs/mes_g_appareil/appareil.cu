// MES-G-APP : executeur CUDA des deux noyaux de noyau_g.hpp (census garde a temoins, premieres sondes), hors produit.
// Un flux, des evenements pour la duree des noyaux seuls ; copies synchrones chronometrees a part (information : dans
// une voie G sur l'appareil, ces donnees seraient produites sur l'appareil par la fin d'etage du catalogue). Une erreur
// du pilote CUDA rend faux avec son message ; aucun repli silencieux.
#include <cuda_runtime.h>

#include <chrono>
#include <vector>

#include "appareil.hpp"

namespace mesg {
namespace {

constexpr u32 kFilsCensus = 128;
constexpr u32 kFilsSondes = 256;

__global__ void __launch_bounds__(kFilsCensus) noyau_census(DonneesCensus d, Resultat* resultats, u32* sites) {
  const u64 i = u64{blockIdx.x} * blockDim.x + threadIdx.x;
  if (i >= d.n_requetes) return;
  u32* s = sites + i * kSitesParRequete;
  census(d.requetes[i], d.noeuds, d.n_noeuds, d.x, d.y, d.z, d.taille_feuille, s, s + kMaxPartie, resultats[i]);
}

struct OrdreAppareil {
  Table table{};
  const u32* cellule_boule = nullptr;
  const u64* rep_debut = nullptr;
  const u64* masques = nullptr;
  u32* sondes = nullptr;
  u64 cellules = 0, representants = 0;
};

__global__ void __launch_bounds__(kFilsSondes) noyau_sondes(OrdreAppareil o, DonneesCatalogue cat) {
  const u64 c = u64{blockIdx.x} * blockDim.x + threadIdx.x;
  if (c >= o.cellules) return;
  sonder_cellule(o.table, cat.pop_debut, cat.pop_sites, cat.boule_p, o.cellule_boule[c], o.rep_debut, o.masques,
                 static_cast<u32>(c), o.sondes);
}

constexpr u32 kFilsPropositions = 128;

__global__ void __launch_bounds__(kFilsPropositions) noyau_propositions(DonneesPropositions d, const u32* x,
                                                                        const u32* y, const u32* z,
                                                                        Proposition* sortie) {
  const u64 i = u64{blockIdx.x} * blockDim.x + threadIdx.x;
  if (i >= d.n) return;
  proposer(d.parties + i * d.pas, d.tailles[i], x, y, z, sortie[i]);
}

double depuis(std::chrono::steady_clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

}  // namespace

struct Appareil::Etat {
  cudaStream_t flux = nullptr;
  cudaEvent_t debut = nullptr, fin = nullptr;
  std::vector<void*> allocations;
  DonneesCensus census{};  // pointeurs de l'appareil
  Resultat* resultats = nullptr;
  u32* sites = nullptr;
  DonneesCatalogue catalogue{};
  std::vector<OrdreAppareil> ordres;
  DonneesPropositions propositions{};
  Proposition* sorties_propositions = nullptr;
  bool ouvert = false;

  bool verifier(cudaError_t e, const char* quoi, std::string& erreur) {
    if (e == cudaSuccess) return true;
    erreur = std::string(quoi) + " : " + cudaGetErrorString(e);
    return false;
  }
  template <class T>
  bool copier_vers(const T* hote, u64 n, T*& appareil, Transferts& t, std::string& erreur) {
    appareil = nullptr;
    if (n == 0) return true;
    void* p = nullptr;
    if (!verifier(cudaMalloc(&p, n * sizeof(T)), "cudaMalloc", erreur)) return false;
    allocations.push_back(p);
    const auto t0 = std::chrono::steady_clock::now();
    if (!verifier(cudaMemcpy(p, hote, n * sizeof(T), cudaMemcpyHostToDevice), "cudaMemcpy H2D", erreur)) return false;
    t.h2d_ms += depuis(t0);
    t.h2d_octets += n * sizeof(T);
    appareil = static_cast<T*>(p);
    return true;
  }
  template <class T>
  bool allouer(u64 n, T*& appareil, std::string& erreur) {
    void* p = nullptr;
    if (!verifier(cudaMalloc(&p, (n == 0 ? 1 : n) * sizeof(T)), "cudaMalloc", erreur)) return false;
    allocations.push_back(p);
    appareil = static_cast<T*>(p);
    return true;
  }
  template <class T>
  bool copier_depuis(T* hote, const T* appareil, u64 n, Transferts& t, std::string& erreur) {
    if (n == 0) return true;
    const auto t0 = std::chrono::steady_clock::now();
    if (!verifier(cudaMemcpy(hote, appareil, n * sizeof(T), cudaMemcpyDeviceToHost), "cudaMemcpy D2H", erreur))
      return false;
    t.d2h_ms += depuis(t0);
    t.d2h_octets += n * sizeof(T);
    return true;
  }
};

Appareil::Appareil() : etat_(new Etat) {}

Appareil::~Appareil() {
  if (etat_->ouvert) {
    cudaStreamSynchronize(etat_->flux);
    for (void* p : etat_->allocations) cudaFree(p);
    cudaEventDestroy(etat_->debut);
    cudaEventDestroy(etat_->fin);
    cudaStreamDestroy(etat_->flux);
  }
  delete etat_;
}

bool Appareil::ouvrir(Proprietes& props, std::string& erreur) {
  Etat& e = *etat_;
  int n = 0;
  if (!e.verifier(cudaGetDeviceCount(&n), "cudaGetDeviceCount", erreur)) return false;
  if (n < 1) {
    erreur = "aucun appareil CUDA";
    return false;
  }
  // Attente en mode yield (MES-M6), fixee avant le contexte ; deja actif : sans effet (comme le produit).
  const cudaError_t drapeaux = cudaSetDeviceFlags(cudaDeviceScheduleYield);
  if (drapeaux != cudaSuccess && drapeaux != cudaErrorSetOnActiveProcess)
    return e.verifier(drapeaux, "cudaSetDeviceFlags", erreur);
  (void)cudaGetLastError();
  if (!e.verifier(cudaSetDevice(0), "cudaSetDevice", erreur)) return false;
  if (!e.verifier(cudaFree(nullptr), "contexte", erreur)) return false;
  // Pile par fil : DWelzl (recursion bornee a quatre niveaux) tient en 608 octets selon ptxas ; marge explicite.
  if (!e.verifier(cudaDeviceSetLimit(cudaLimitStackSize, 4096), "cudaDeviceSetLimit", erreur)) return false;
  cudaDeviceProp p{};
  if (!e.verifier(cudaGetDeviceProperties(&p, 0), "cudaGetDeviceProperties", erreur)) return false;
  props.nom = p.name;
  props.sm = p.multiProcessorCount;
  props.cc_majeur = p.major;
  props.cc_mineur = p.minor;
  props.memoire = p.totalGlobalMem;
  props.l2 = static_cast<u64>(p.l2CacheSize);
  int horloge = 0, horloge_memoire = 0, bus = 0;
  cudaDeviceGetAttribute(&horloge, cudaDevAttrClockRate, 0);
  cudaDeviceGetAttribute(&horloge_memoire, cudaDevAttrMemoryClockRate, 0);
  cudaDeviceGetAttribute(&bus, cudaDevAttrGlobalMemoryBusWidth, 0);
  props.horloge_khz = horloge;
  props.horloge_memoire_khz = horloge_memoire;
  props.bus_bits = bus;
  cudaDriverGetVersion(&props.pilote);
  cudaRuntimeGetVersion(&props.execution);
  if (!e.verifier(cudaStreamCreateWithFlags(&e.flux, cudaStreamNonBlocking), "cudaStreamCreate", erreur)) return false;
  if (!e.verifier(cudaEventCreate(&e.debut), "cudaEventCreate", erreur)) return false;
  if (!e.verifier(cudaEventCreate(&e.fin), "cudaEventCreate", erreur)) return false;
  e.ouvert = true;
  return true;
}

bool Appareil::charger_census(const DonneesCensus& d, Transferts& t, std::string& erreur) {
  Etat& e = *etat_;
  DonneesCensus a = d;
  Noeud* noeuds = nullptr;
  u32 *x = nullptr, *y = nullptr, *z = nullptr;
  Requete* requetes = nullptr;
  if (!e.copier_vers(d.noeuds, d.n_noeuds, noeuds, t, erreur) || !e.copier_vers(d.x, u64{d.n_sites}, x, t, erreur) ||
      !e.copier_vers(d.y, u64{d.n_sites}, y, t, erreur) || !e.copier_vers(d.z, u64{d.n_sites}, z, t, erreur) ||
      !e.copier_vers(d.requetes, d.n_requetes, requetes, t, erreur))
    return false;
  a.noeuds = noeuds;
  a.x = x;
  a.y = y;
  a.z = z;
  a.requetes = requetes;
  e.census = a;
  return e.allouer(d.n_requetes, e.resultats, erreur) && e.allouer(d.n_requetes * kSitesParRequete, e.sites, erreur);
}

bool Appareil::jouer_census(float& noyau_ms, std::string& erreur) {
  Etat& e = *etat_;
  noyau_ms = 0;
  const u64 n = e.census.n_requetes;
  if (n == 0) return true;
  const u64 blocs = (n + kFilsCensus - 1) / kFilsCensus;
  if (!e.verifier(cudaEventRecord(e.debut, e.flux), "cudaEventRecord", erreur)) return false;
  noyau_census<<<static_cast<unsigned>(blocs), kFilsCensus, 0, e.flux>>>(e.census, e.resultats, e.sites);
  if (!e.verifier(cudaGetLastError(), "noyau_census", erreur)) return false;
  if (!e.verifier(cudaEventRecord(e.fin, e.flux), "cudaEventRecord", erreur)) return false;
  if (!e.verifier(cudaEventSynchronize(e.fin), "cudaEventSynchronize", erreur)) return false;
  return e.verifier(cudaEventElapsedTime(&noyau_ms, e.debut, e.fin), "cudaEventElapsedTime", erreur);
}

bool Appareil::lire_census(Resultat* resultats, u32* sites, Transferts& t, std::string& erreur) {
  Etat& e = *etat_;
  return e.copier_depuis(resultats, e.resultats, e.census.n_requetes, t, erreur) &&
         e.copier_depuis(sites, e.sites, e.census.n_requetes * kSitesParRequete, t, erreur);
}

bool Appareil::charger_catalogue(const DonneesCatalogue& d, Transferts& t, std::string& erreur) {
  Etat& e = *etat_;
  u64* debut = nullptr;
  u32 *sites = nullptr, *p = nullptr;
  if (!e.copier_vers(d.pop_debut, d.boules + 1, debut, t, erreur) ||
      !e.copier_vers(d.pop_sites, d.incidences, sites, t, erreur) || !e.copier_vers(d.boule_p, d.boules, p, t, erreur))
    return false;
  e.catalogue = DonneesCatalogue{debut, sites, p, d.boules, d.incidences};
  return true;
}

bool Appareil::charger_sondes(const DonneesSondes& d, Transferts& t, std::string& erreur) {
  Etat& e = *etat_;
  OrdreAppareil o;
  u32 *fiches = nullptr, *repertoire = nullptr, *cellule_boule = nullptr;
  u64 *rep_debut = nullptr, *masques = nullptr;
  if (!e.copier_vers(d.fiches, d.entrees * (3 + d.k), fiches, t, erreur) ||
      !e.copier_vers(d.repertoire, d.seaux + 1, repertoire, t, erreur) ||
      !e.copier_vers(d.cellule_boule, d.cellules, cellule_boule, t, erreur) ||
      !e.copier_vers(d.rep_debut, d.cellules + 1, rep_debut, t, erreur) ||
      !e.copier_vers(d.masques, d.representants, masques, t, erreur) ||
      !e.allouer(d.representants, o.sondes, erreur))
    return false;
  o.table = Table{fiches, repertoire, d.entrees, d.decalage, d.k};
  o.cellule_boule = cellule_boule;
  o.rep_debut = rep_debut;
  o.masques = masques;
  o.cellules = d.cellules;
  o.representants = d.representants;
  e.ordres.push_back(o);
  return true;
}

bool Appareil::jouer_sondes(float& noyau_ms, std::string& erreur) {
  Etat& e = *etat_;
  noyau_ms = 0;
  if (!e.verifier(cudaEventRecord(e.debut, e.flux), "cudaEventRecord", erreur)) return false;
  for (const OrdreAppareil& o : e.ordres) {
    if (o.cellules == 0) continue;
    const u64 blocs = (o.cellules + kFilsSondes - 1) / kFilsSondes;
    noyau_sondes<<<static_cast<unsigned>(blocs), kFilsSondes, 0, e.flux>>>(o, e.catalogue);
    if (!e.verifier(cudaGetLastError(), "noyau_sondes", erreur)) return false;
  }
  if (!e.verifier(cudaEventRecord(e.fin, e.flux), "cudaEventRecord", erreur)) return false;
  if (!e.verifier(cudaEventSynchronize(e.fin), "cudaEventSynchronize", erreur)) return false;
  return e.verifier(cudaEventElapsedTime(&noyau_ms, e.debut, e.fin), "cudaEventElapsedTime", erreur);
}

bool Appareil::lire_sondes(u32 i, u32* sondes, Transferts& t, std::string& erreur) {
  Etat& e = *etat_;
  if (i >= e.ordres.size()) {
    erreur = "ordre non charge";
    return false;
  }
  return e.copier_depuis(sondes, e.ordres[i].sondes, e.ordres[i].representants, t, erreur);
}

bool Appareil::charger_propositions(const DonneesPropositions& d, Transferts& t, std::string& erreur) {
  Etat& e = *etat_;
  if (e.census.x == nullptr && d.n > 0) {
    erreur = "charger_census d'abord (coordonnees)";
    return false;
  }
  u32 *parties = nullptr, *tailles = nullptr;
  if (!e.copier_vers(d.parties, d.n * d.pas, parties, t, erreur) || !e.copier_vers(d.tailles, d.n, tailles, t, erreur) ||
      !e.allouer(d.n, e.sorties_propositions, erreur))
    return false;
  e.propositions = DonneesPropositions{parties, tailles, d.pas, d.n};
  return true;
}

bool Appareil::jouer_propositions(float& noyau_ms, std::string& erreur) {
  Etat& e = *etat_;
  noyau_ms = 0;
  const u64 n = e.propositions.n;
  if (n == 0) return true;
  const u64 blocs = (n + kFilsPropositions - 1) / kFilsPropositions;
  if (!e.verifier(cudaEventRecord(e.debut, e.flux), "cudaEventRecord", erreur)) return false;
  noyau_propositions<<<static_cast<unsigned>(blocs), kFilsPropositions, 0, e.flux>>>(
      e.propositions, e.census.x, e.census.y, e.census.z, e.sorties_propositions);
  if (!e.verifier(cudaGetLastError(), "noyau_propositions", erreur)) return false;
  if (!e.verifier(cudaEventRecord(e.fin, e.flux), "cudaEventRecord", erreur)) return false;
  if (!e.verifier(cudaEventSynchronize(e.fin), "cudaEventSynchronize", erreur)) return false;
  return e.verifier(cudaEventElapsedTime(&noyau_ms, e.debut, e.fin), "cudaEventElapsedTime", erreur);
}

bool Appareil::lire_propositions(Proposition* sortie, Transferts& t, std::string& erreur) {
  Etat& e = *etat_;
  return e.copier_depuis(sortie, e.sorties_propositions, e.propositions.n, t, erreur);
}

}  // namespace mesg
