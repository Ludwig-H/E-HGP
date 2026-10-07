// mhgp12_mes_g1 : microbanc MES-G1 (levier G-L3 du contrat de la tour, CONTRAT_TOUR.md paragraphe 4.3), hors produit.
//
// Question : parmi les parties de descente dont la v11 fait un census SATURE (route 2 du vidage : support local de
// bounded_meb absent de la table S* -> boule, census de seuil k avec au moins k sites strictement interieurs), quelle
// part pourrait sauter SANS census, en exhibant k sites strictement interieurs parmi des candidats locaux, testes en
// exact contre la plus petite boule de la partie ?
//
// Pour chaque partie F de route 2 (k sites, SiteIdx croissants) :
//   1. plus petite boule exacte de F : bounded_meb de la v11 (liee) ;
//   2. cible de la v11 : census de la v11 (CensusWorkspace, seuil k) ; sature, il rend exactement les k plus petits
//      SiteIdx de I (parcours prefixe de l'index, listes croissantes) ; controle contre le vidage : partie suivante de
//      la trace, ou fin de trace par la table de populations (graine de fin 2) ;
//   3. ensembles de candidats (tries, sans doublon), testes par SiteIdx croissant au predicat exact num::side de la
//      v11 ; strictement interieur = cote < 0, un site SUR la sphere ne compte jamais ; arret au k-ieme interieur :
//        voisins           F et les K plus proches voisins de chaque site de F (voisins.hpp, K du catalogue) ;
//        fenetre_8/16      les sites de SiteIdx dans [i - W, i + W] pour chaque i de F (ordre de Morton de la v11) ;
//        voisins_ou_fenetre_16  reunion des deux precedents (mesure en plus, informative) ;
//      partie certifiee = au moins k candidats strictement interieurs ; cible du saut = les k plus petits SiteIdx
//      parmi eux (pas valide du theoreme D) ; comparee a la cible de la v11 ; « naissance directe » = population triee
//      I u U d'une boule de cat.bin a p + m = k (LEM-POP, table refaite par ordre) ;
//   4. juge (hors decision) : chaque site d'une cible est re-teste par la voie LatticeSphere de la v11 (celle du
//      census) ; cible egale a celle de la v11 <=> cible de la v11 incluse dans les candidats ; partie non certifiee =>
//      cible de la v11 non incluse ; un ecart rend le code 1.
// Pour chaque partie de route 3 (census complet) : census de seuil k (complet exige), p = |I|, q_min par le support
// canonique de la coquille entiere (mebcert::canonical_support, celui de la replique de mhgp12_vidage), spheres
// distinctes par (centre exact, rayon carre exact) ET par S* global (les deux comptes doivent egaler).
// LEM-HORS-CAT (CONTRAT_TOUR.md paragraphe 4.2), sur les parties dont la sphere est HORS de Cat_K (ball = kNone du
// vidage) : route 2 : p >= K - 2 (trivial si k >= K - 2, sinon census de seuil K - 2 sature exige) ; route 3 :
// p >= K - 2, k >= K - 1, p = K - 2 => q_min = 4, p + q_min >= K + 2. Un contre-exemple est une contradiction
// mathematique : ligne « contradiction », coordonnees ecrites dans le dossier du vidage (jamais verse), code 1, arret.
// Les parties de route 2 ou 3 dont la sphere est DANS Cat_K (S*(b) hors de F) sont hors de l'hypothese du lemme :
// comptees a part, et recoupees avec le catalogue (p, m, q, S*).
//
// Modes :
//   mhgp12_mes_g1 --porte                                  nuage grave ; code 0 conforme, 1 ecart
//   mhgp12_mes_g1 <dossier> [--ordres k1,k2,...] [--echantillon N]
//                                                          banc sur un vidage complet (cat.bin, ordre_<k>.bin)
// Codes : 0 conforme ; 1 ecart (juge, voisins contre force brute, contradiction) ; 2 usage ; 3 refus (section
// absente, vidage incoherent avec la v11, refus arithmetique, exception).
// Mutant causal compile a part (jamais une branche du chemin mesure) : mhgp12_mes_g1_mutant_cote_nul
// (MHGP12_MUTANT_G1_COTE_NUL) admet un site sur la sphere (cote nul) comme interieur ; la porte doit le tuer (code 1).
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "../mes_m3_m4_tour/mes_m3/meb_cert.hpp"
#include "voisins.hpp"

using namespace mhgp11;

namespace mhgp12 {
namespace {
namespace d = ::mhgp12::dump;
using Clock = std::chrono::steady_clock;
using g1::kAucun;

#ifdef MHGP12_MUTANT_G1_COTE_NUL
constexpr bool kMutantCoteNul = true;
#else
constexpr bool kMutantCoteNul = false;
#endif

double secondes(Clock::time_point t) { return std::chrono::duration<double>(Clock::now() - t).count(); }

[[noreturn]] void refus(const std::string& quoi) { throw std::runtime_error(quoi); }

// Strictement interieur : cote < 0. Le mutant admet le cote nul (site sur la sphere).
inline bool compte_interieur(int cote) { return kMutantCoteNul ? cote <= 0 : cote < 0; }

// ---- Domaine : nuage v11 refait depuis des coordonnees, index global et espace de census -------------------------
struct Domaine {
  MemoryBudget budget{u64{8} << 30};
  std::optional<GlobalIndex> index;
  std::unique_ptr<CensusWorkspace> ws;
  std::vector<u32> xyz;  // trois coordonnees par SiteIdx (ordre de Morton de la v11)
  std::vector<num::Point> points;
  u32 n = 0;
  const Cloud& cloud() const { return index->cloud(); }
};

// Prepare le nuage (prepare_cloud trie par cle de Morton), puis l'index (feuilles de 8, comme le domaine FULL de la
// v11 dans mhgp12_vidage) et un espace de census. Les coordonnees du domaine suivent l'ordre des SiteIdx.
void preparer(Domaine& dom, const std::vector<u32>& x, const std::vector<u32>& y, const std::vector<u32>& z) {
  const u32 n = static_cast<u32>(x.size());
  std::vector<PointId> ids(n);
  for (u32 i = 0; i < n; ++i) ids[i] = PointId{i};
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth(), dom.budget);
  if (!cloud.ok()) refus("prepare_cloud refuse");
  if (cloud.value().sites() != n) refus("nuage : positions repetees");
  auto index = build_index(std::move(cloud.value()), IndexParams{}, dom.budget);
  if (!index.ok()) refus("build_index refuse");
  dom.index.emplace(std::move(index.value()));
  auto ws = CensusWorkspace::make(*dom.index, dom.budget);
  if (!ws.ok()) refus("CensusWorkspace refuse");
  dom.ws = std::move(ws.value());
  const Cloud& cl = dom.cloud();
  dom.n = cl.sites();
  dom.xyz.resize(3 * u64{dom.n});
  dom.points.resize(dom.n);
  for (u32 s = 0; s < dom.n; ++s) {
    dom.xyz[3 * u64{s}] = cl.x()[s];
    dom.xyz[3 * u64{s} + 1] = cl.y()[s];
    dom.xyz[3 * u64{s} + 2] = cl.z()[s];
    auto p = num::Point::make(cl.x()[s], cl.y()[s], cl.z()[s]);
    if (!p.ok()) refus("Point::make refuse");
    dom.points[s] = p.value();
  }
}

struct Recensement {
  std::vector<u32> interieur, coquille;
  bool complet = false;
  static Outcome consume(void* raw, const BorrowedCensus& pop) noexcept {
    auto& r = *static_cast<Recensement*>(raw);
    r.complet = pop.kind() == CensusKind::complete;
    r.interieur.clear();
    r.coquille.clear();
    for (SiteIdx s : pop.interior()) r.interieur.push_back(idx(s));
    for (SiteIdx s : pop.shell()) r.coquille.push_back(idx(s));
    return {};
  }
};

void recenser(Domaine& dom, const num::Sphere& sphere, u32 seuil, Recensement& r) {
  const Outcome o = dom.ws->query(*dom.index, sphere, seuil, &r, Recensement::consume);
  if (!o.ok()) refus("census refuse : " + std::string(reason_name(o.reason)));
}

num::Sphere plus_petite_boule(const Domaine& dom, const u32* f, u32 k) {
  std::array<SiteIdx, kMaxMebSites> ids{};
  for (u32 i = 0; i < k; ++i) ids[i] = SiteIdx{f[i]};
  auto meb = bounded_meb(dom.cloud(), {ids.data(), k});
  if (!meb.ok()) refus("bounded_meb refuse");
  return meb.value().sphere();
}

// ---- Candidats ------------------------------------------------------------------------------------------------------
// (a) F et les K plus proches voisins de chaque site de F.
void candidats_voisins(const std::vector<u32>& table, u32 K, const u32* f, u32 k, std::vector<u32>& out) {
  out.clear();
  for (u32 i = 0; i < k; ++i) {
    out.push_back(f[i]);
    for (u32 j = 0; j < K; ++j) {
      const u32 v = table[u64{f[i]} * K + j];
      if (v != kAucun) out.push_back(v);
    }
  }
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
}

// (b) Fenetre de Morton : SiteIdx dans [i - W, i + W] pour chaque i de F (F strictement croissant), sans doublon.
void candidats_fenetre(u32 n, u32 W, const u32* f, u32 k, std::vector<u32>& out) {
  out.clear();
  u64 suivant = 0;  // premier SiteIdx non encore emis
  for (u32 i = 0; i < k; ++i) {
    const u64 lo = std::max<u64>(f[i] >= W ? u64{f[i]} - W : 0, suivant);
    const u64 hi = std::min<u64>(u64{n} - 1, u64{f[i]} + W);
    for (u64 s = lo; s <= hi; ++s) out.push_back(static_cast<u32>(s));
    suivant = std::max<u64>(suivant, hi + 1);
  }
}

struct Essai {
  u32 tests = 0, trouves = 0;
  std::array<u32, kMaxMebSites> cible{};
};

// Teste les candidats par SiteIdx croissant ; arret au k-ieme strictement interieur : la cible est alors exactement
// les k plus petits SiteIdx des candidats strictement interieurs.
Essai essayer(const num::Sphere& sphere, const std::vector<u32>& cand, u32 k, const std::vector<num::Point>& pts) {
  Essai e;
  for (u32 c : cand) {
    ++e.tests;
    auto cote = num::side(sphere, pts[c]);
    if (!cote.ok()) refus("num::side refuse");
    if (compte_interieur(cote.value())) {
      e.cible[e.trouves++] = c;
      if (e.trouves == k) break;
    }
  }
  return e;
}

bool inclus(const u32* a, u32 na, const std::vector<u32>& trie) {
  for (u32 i = 0; i < na; ++i)
    if (!std::binary_search(trie.begin(), trie.end(), a[i])) return false;
  return true;
}

// ---- Table de populations d'un ordre (LEM-POP) : population triee I u U des boules a p + m = k ---------------------
class TablePop {
 public:
  void construire(const mebcert::Cat& cat, u32 k) {
    k_ = k;
    lignes_.clear();
    for (u32 b = 0; b < cat.balls; ++b) {
      const auto& r = cat.rec[b];
      if (u64{r.p} + r.m != k) continue;
      const auto in = cat.interior(b), sh = cat.shell(b);
      const std::size_t at = lignes_.size();
      lignes_.resize(at + k);
      std::merge(in.begin(), in.end(), sh.begin(), sh.end(), lignes_.begin() + static_cast<std::ptrdiff_t>(at));
    }
    const u64 entrees = k == 0 ? 0 : lignes_.size() / k;
    u64 capacite = 16;
    while (capacite < 2 * entrees) capacite *= 2;
    cases_.assign(capacite, kAucun);
    masque_ = capacite - 1;
    for (u64 e = 0; e < entrees; ++e) {
      const u32* ligne = lignes_.data() + e * k;
      for (u64 at = hacher(ligne) & masque_;; at = (at + 1) & masque_) {
        if (cases_[at] == kAucun) {
          cases_[at] = static_cast<u32>(e);
          break;
        }
        if (std::equal(ligne, ligne + k, lignes_.data() + u64{cases_[at]} * k))
          refus("table de populations : population en double (unicite de la plus petite boule)");
      }
    }
  }
  // Partie de k sites strictement croissants : vrai si c'est la population d'une boule a p + m = k.
  bool contient(const u32* sites) const {
    for (u64 at = hacher(sites) & masque_;; at = (at + 1) & masque_) {
      const u32 e = cases_[at];
      if (e == kAucun) return false;
      if (std::equal(sites, sites + k_, lignes_.data() + u64{e} * k_)) return true;
    }
  }
  u64 entrees() const { return k_ == 0 ? 0 : lignes_.size() / k_; }

 private:
  u64 hacher(const u32* s) const {
    u64 h = 0x9E3779B97F4A7C15ull * (u64{k_} + 1);
    for (u32 i = 0; i < k_; ++i) h = (h ^ (u64{s[i]} + 0x632BE59BD9B4E019ull)) * 0xff51afd7ed558ccdull;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ull;
    return h ^ (h >> 33);
  }
  u32 k_ = 0;
  std::vector<u32> lignes_;
  std::vector<u32> cases_;
  u64 masque_ = 0;
};

// ---- Statistiques -----------------------------------------------------------------------------------------------------
enum Ensemble : int { kVoisins = 0, kFenetre8, kFenetre16, kUnion, kEnsembles };
const char* const kNomsEnsembles[kEnsembles] = {"voisins", "fenetre_8", "fenetre_16", "voisins_ou_fenetre_16"};

struct StatEnsemble {
  u64 certifiees = 0, candidats = 0, candidats_max = 0, tests = 0, tests_max = 0;
  u64 cible_egale_v11 = 0, cible_naissance = 0, cible_v11_naissance = 0, ecarts_juge = 0;
  void ajouter(const StatEnsemble& o) {
    certifiees += o.certifiees;
    candidats += o.candidats;
    candidats_max = std::max(candidats_max, o.candidats_max);
    tests += o.tests;
    tests_max = std::max(tests_max, o.tests_max);
    cible_egale_v11 += o.cible_egale_v11;
    cible_naissance += o.cible_naissance;
    cible_v11_naissance += o.cible_v11_naissance;
    ecarts_juge += o.ecarts_juge;
  }
};

void ecrire_ensemble(std::ostream& o, const StatEnsemble& s, u64 parties) {
  const double np = parties ? double(parties) : 1.0;
  const double nc = s.certifiees ? double(s.certifiees) : 1.0;
  o << "{\"certifiees\":" << s.certifiees << ",\"part_certifiees\":" << (parties ? s.certifiees / np : 0.0)
    << ",\"candidats_moyen\":" << (parties ? s.candidats / np : 0.0) << ",\"candidats_max\":" << s.candidats_max
    << ",\"tests_moyen\":" << (parties ? s.tests / np : 0.0) << ",\"tests_max\":" << s.tests_max
    << ",\"cible_egale_v11\":" << s.cible_egale_v11 << ",\"part_cible_egale_v11\":"
    << (s.certifiees ? s.cible_egale_v11 / nc : 0.0) << ",\"cible_naissance\":" << s.cible_naissance
    << ",\"part_cible_naissance\":" << (s.certifiees ? s.cible_naissance / nc : 0.0)
    << ",\"cible_v11_naissance_sur_certifiees\":" << s.cible_v11_naissance
    << ",\"part_cible_v11_naissance_sur_certifiees\":" << (s.certifiees ? s.cible_v11_naissance / nc : 0.0)
    << ",\"ecarts_juge\":" << s.ecarts_juge << "}";
}

// Sphere d'une partie de route 3 (spheres distinctes) : sphere exacte, S* global, hors catalogue.
struct Sphere3 {
  num::Sphere sphere;
  std::array<u32, 4> sstar;
  bool hors_catalogue;
};

// Spheres distinctes : par (centre exact, rayon carre exact) et par S* global ; les deux comptes doivent egaler.
std::pair<u64, u64> spheres_distinctes(const std::vector<Sphere3>& v, bool hors_seulement) {
  std::vector<u32> ordre;
  for (u32 i = 0; i < v.size(); ++i)
    if (!hors_seulement || v[i].hors_catalogue) ordre.push_back(i);
  std::vector<u32> par_centre = ordre;
  std::sort(par_centre.begin(), par_centre.end(), [&](u32 a, u32 b) {
    const int c = num::compare_centers(v[a].sphere, v[b].sphere);
    if (c != 0) return c < 0;
    return num::compare(v[a].sphere.level(), v[b].sphere.level()) < 0;
  });
  u64 distinctes_centre = 0;
  for (std::size_t i = 0; i < par_centre.size(); ++i)
    if (i == 0 || num::compare_centers(v[par_centre[i]].sphere, v[par_centre[i - 1]].sphere) != 0 ||
        num::compare(v[par_centre[i]].sphere.level(), v[par_centre[i - 1]].sphere.level()) != 0)
      ++distinctes_centre;
  std::vector<std::array<u32, 4>> cles;
  for (u32 i : ordre) cles.push_back(v[i].sstar);
  std::sort(cles.begin(), cles.end());
  const u64 distinctes_sstar = static_cast<u64>(std::unique(cles.begin(), cles.end()) - cles.begin());
  return {distinctes_centre, distinctes_sstar};
}

// ---- Porte : nuage grave -------------------------------------------------------------------------------------------
// Trois grappes separees en x (cle de Morton : grappe 1 < grappe 2 < grappe 3, chacune contigue en SiteIdx), K = 3 :
//   grappe 1 : A1 B1 diametre de la sphere de centre C1 = (100, 100, 100), rayon carre 100 ; C1, D1, E1 strictement
//              interieurs (D1 et E1 a egale distance de A1 : departage par SiteIdx) ; F = {A1, B1} (k = 2) et
//              F = {A1, B1, C1} (k = 3, un site de F strictement interieur) : les voisins suffisent ;
//   grappe 2 : A2 B2 diametre (centre M1, rayon carre 400), M1 et M2 interieurs, N1..N4 juste dehors et plus proches de
//              A2 et B2 que M2 : les voisins ne suffisent pas (un seul interieur, M1), la fenetre si ;
//   grappe 3 : A3 B3 diametre (centre C3, rayon carre 100), C3 seul interieur, E3 EXACTEMENT sur la sphere : aucun
//              ensemble ne certifie ; le mutant « cote nul admis » y compte A3, B3 et E3.
struct Temoin {
  const char* nom;
  std::vector<std::string> f;
  bool sature;                                    // census de seuil k de la v11
  std::vector<std::string> cible_v11;             // k plus petits SiteIdx de I (si sature)
  std::array<bool, 3> certifiee;                  // voisins, fenetre 8, fenetre 16
  std::array<std::vector<std::string>, 3> cible;  // cible attendue si certifiee
};

int porte() {
  const std::vector<std::pair<std::string, std::array<u32, 3>>> graves = {
      {"A1", {90, 100, 100}},  {"B1", {110, 100, 100}}, {"C1", {100, 100, 100}}, {"D1", {100, 103, 100}},
      {"E1", {100, 100, 103}}, {"A2", {300, 100, 100}}, {"B2", {340, 100, 100}}, {"M1", {320, 100, 100}},
      {"M2", {320, 105, 100}}, {"N1", {299, 100, 100}}, {"N2", {298, 100, 100}}, {"N3", {341, 100, 100}},
      {"N4", {342, 100, 100}}, {"A3", {590, 100, 100}}, {"B3", {610, 100, 100}}, {"C3", {600, 100, 100}},
      {"E3", {600, 110, 100}}};
  constexpr u32 K = 3;
  Domaine dom;
  {
    std::vector<u32> x, y, z;
    for (const auto& [nom, c] : graves) {
      x.push_back(c[0]);
      y.push_back(c[1]);
      z.push_back(c[2]);
    }
    preparer(dom, x, y, z);
  }
  std::map<std::string, u32> site;
  std::vector<std::string> nom_de(dom.n);
  for (u32 s = 0; s < dom.n; ++s)
    for (const auto& [nom, c] : graves)
      if (dom.xyz[3 * u64{s}] == c[0] && dom.xyz[3 * u64{s} + 1] == c[1] && dom.xyz[3 * u64{s} + 2] == c[2]) {
        site[nom] = s;
        nom_de[s] = nom;
      }
  int ecarts = 0;
  auto verdict = [&](const std::string& quoi, bool ok) {
    if (!ok) ++ecarts;
    std::cout << "{\"controle\":\"" << quoi << "\",\"conforme\":" << (ok ? "true" : "false") << "}\n";
  };
  // Precondition du nuage : grappes contigues en SiteIdx (sinon les attentes de fenetre ne valent plus) : refus. La
  // grappe d'un site se lit a son abscisse (grappe 1 : x < 128 ; grappe 2 : 256 <= x < 512 ; grappe 3 : x >= 512).
  for (u32 g = 1; g <= 3; ++g) {
    u32 lo = kAucun, hi = 0, nb = 0;
    for (const auto& [nom, s] : site) {
      const u32 x = dom.xyz[3 * u64{s}];
      if ((x < 128 ? 1u : x < 512 ? 2u : 3u) != g) continue;
      lo = std::min(lo, s);
      hi = std::max(hi, s);
      ++nb;
    }
    if (nb == 0 || hi - lo + 1 != nb) refus("porte : grappe non contigue en SiteIdx");
  }
  // Voisins : arbre (feuilles de 2, noeuds internes exerces) contre force brute sur tous les sites, et voisins graves.
  const std::vector<u32> table = g1::table_des_voisins(dom.xyz.data(), dom.n, K, 2);
  bool brute = true;
  for (u32 q = 0; q < dom.n; ++q) {
    const auto ref = g1::voisins_force_brute(dom.xyz.data(), dom.n, q, K);
    for (u32 j = 0; j < K; ++j) brute = brute && table[u64{q} * K + j] == ref[j].site;
  }
  verdict("voisins_arbre_egal_force_brute", brute);
  const std::vector<std::pair<std::string, std::vector<std::string>>> voisins_graves = {
      {"A1", {"C1", "D1", "E1"}}, {"C1", {"D1", "E1", "A1"}}, {"A2", {"N1", "N2", "M1"}},
      {"B2", {"N3", "N4", "M1"}}, {"A3", {"C3", "E3", "B3"}}, {"B3", {"C3", "E3", "A3"}}};
  std::vector<u32> cand_porte;
  for (const auto& [q, attendus] : voisins_graves) {
    bool ok = true;
    for (u32 j = 0; j < K; ++j) ok = ok && table[u64{site[q]} * K + j] == site[attendus[j]];
    verdict("voisins_" + q, ok);
  }
  // E3 est EXACTEMENT sur la sphere de diametre A3 B3 (cote nul par les deux voies de la v11) et figure parmi les
  // candidats voisins de F = {A3, B3} : il est teste et ne doit pas compter.
  {
    const u32 f3[2] = {std::min(site["A3"], site["B3"]), std::max(site["A3"], site["B3"])};
    const num::Sphere s3 = plus_petite_boule(dom, f3, 2);
    const num::LatticeSphere l3(s3);
    auto c1 = num::side(s3, dom.points[site["E3"]]);
    auto c2 = l3.side(dom.points[site["E3"]]);
    candidats_voisins(table, K, f3, 2, cand_porte);
    verdict("E3_sur_la_sphere_et_candidat", c1.ok() && c2.ok() && c1.value() == 0 && c2.value() == 0 &&
                                                std::binary_search(cand_porte.begin(), cand_porte.end(), site["E3"]));
  }
  const std::vector<Temoin> temoins = {
      {"grappe1_k2_voisins_suffisent", {"A1", "B1"}, true, {"C1", "D1"}, {true, true, true},
       {std::vector<std::string>{"C1", "D1"}, {"C1", "D1"}, {"C1", "D1"}}},
      {"grappe1_k3_site_de_f_interieur", {"A1", "B1", "C1"}, true, {"C1", "D1", "E1"}, {true, true, true},
       {std::vector<std::string>{"C1", "D1", "E1"}, {"C1", "D1", "E1"}, {"C1", "D1", "E1"}}},
      {"grappe2_voisins_ne_suffisent_pas", {"A2", "B2"}, true, {"M1", "M2"}, {false, true, true},
       {std::vector<std::string>{}, {"M1", "M2"}, {"M1", "M2"}}},
      {"grappe3_site_sur_la_sphere", {"A3", "B3"}, false, {}, {false, false, false},
       {std::vector<std::string>{}, {}, {}}}};
  Recensement r;
  std::vector<u32> cand;
  for (const auto& t : temoins) {
    std::vector<u32> f;
    for (const auto& nom : t.f) f.push_back(site[nom]);
    std::sort(f.begin(), f.end());
    const u32 k = static_cast<u32>(f.size());
    const num::Sphere sphere = plus_petite_boule(dom, f.data(), k);
    recenser(dom, sphere, k, r);
    bool ok = r.complet == !t.sature;
    std::vector<u32> attendue_v11;
    for (const auto& nom : t.cible_v11) attendue_v11.push_back(site[nom]);
    std::sort(attendue_v11.begin(), attendue_v11.end());
    if (t.sature) ok = ok && r.interieur == attendue_v11;
    std::cout << "{\"temoin\":\"" << t.nom << "\",\"k\":" << k << ",\"census_sature\":" << (!r.complet ? "true" : "false");
    for (int e = 0; e < 3; ++e) {
      if (e == kVoisins) candidats_voisins(table, K, f.data(), k, cand);
      else candidats_fenetre(dom.n, e == kFenetre8 ? 8 : 16, f.data(), k, cand);
      const Essai essai = essayer(sphere, cand, k, dom.points);
      const bool certifiee = essai.trouves == k;
      std::vector<u32> cible(essai.cible.begin(), essai.cible.begin() + essai.trouves);
      std::vector<u32> attendue;
      for (const auto& nom : t.cible[e]) attendue.push_back(site[nom]);
      std::sort(attendue.begin(), attendue.end());
      const bool conforme = certifiee == t.certifiee[e] && (!certifiee || cible == attendue);
      ok = ok && conforme;
      std::cout << ",\"" << kNomsEnsembles[e] << "\":{\"certifiee\":" << (certifiee ? "true" : "false")
                << ",\"cible\":[";
      for (u32 i = 0; i < essai.trouves; ++i) std::cout << (i ? "," : "") << '"' << nom_de[cible[i]] << '"';
      std::cout << "],\"conforme\":" << (conforme ? "true" : "false") << "}";
    }
    std::cout << ",\"conforme\":" << (ok ? "true" : "false") << "}\n";
    if (!ok) ++ecarts;
  }
  std::cout << "{\"porte\":\"mes_g1\",\"mutant_cote_nul\":" << (kMutantCoteNul ? "true" : "false")
            << ",\"temoins\":" << temoins.size() << ",\"ecarts\":" << ecarts << "}\n";
  return ecarts == 0 ? 0 : 1;
}

// ---- Banc sur un vidage -------------------------------------------------------------------------------------------
struct Lem {
  u64 route2_triviales = 0, route2_par_seuil = 0, route3_verifiees = 0, hors_hypothese = 0;
};

// Contre-exemple de LEM-HORS-CAT : ligne JSON (comptes) et coordonnees dans le dossier du vidage, jamais verse.
[[noreturn]] void contradiction(const std::string& dir, const Domaine& dom, u32 k, u32 K, u64 partie, int route,
                                const u32* f, u32 p, u32 q, const std::string& quoi) {
  const std::string chemin = dir + "/contradiction_k" + std::to_string(k) + "_" + std::to_string(partie) + ".txt";
  std::ofstream out(chemin);
  out << "# contradiction LEM-HORS-CAT : " << quoi << " ; K=" << K << " k=" << k << " route=" << route << " p=" << p
      << " q_min=" << q << "\n";
  for (u32 i = 0; i < k; ++i)
    out << f[i] << " " << dom.xyz[3 * u64{f[i]}] << " " << dom.xyz[3 * u64{f[i]} + 1] << " "
        << dom.xyz[3 * u64{f[i]} + 2] << "\n";
  std::cout << "{\"phase\":\"contradiction\",\"lemme\":\"LEM-HORS-CAT\",\"quoi\":\"" << quoi << "\",\"K\":" << K
            << ",\"k\":" << k << ",\"partie\":" << partie << ",\"route\":" << route << ",\"p\":" << p
            << ",\"q_min\":" << q << ",\"fixture\":\"" << chemin << "\"}\n"
            << std::flush;
  std::exit(1);
}

int banc(const std::string& dir, const std::vector<u32>& seuls, u32 echantillon) {
  const auto t_debut = Clock::now();
  const d::Reader cat_file(dir + "/cat.bin");
  if (cat_file.header().kind != d::kCatalogue) refus("cat.bin : genre inattendu");
  const u32 K = cat_file.header().kmax;
  if (K < 2 || K > kMaxMebSites) refus("cat.bin : K hors domaine");
  mebcert::Cat cat;
  {
    const auto [xyz, n] = cat_file.get<u32>("SITEXYZ", 12);
    cat.xyz = xyz;
    cat.sites = static_cast<u32>(n);
    const auto [rec, nb] = cat_file.get<d::BallRec>("BALLS");
    cat.rec = rec;
    cat.balls = static_cast<u32>(nb);
    const auto [off, no] = cat_file.get<u64>("POPOFF");
    if (no != nb + 1) refus("cat.bin : POPOFF");
    cat.off = off;
    const auto [val, nv] = cat_file.get<u32>("POPVAL");
    if (nv != off[nb]) refus("cat.bin : POPVAL");
    cat.val = val;
  }
  if (cat_file.header().sites != cat.sites) refus("cat.bin : nombre de sites");
  Domaine dom;
  {
    std::vector<u32> x(cat.sites), y(cat.sites), z(cat.sites);
    for (u32 s = 0; s < cat.sites; ++s) {
      x[s] = cat.xyz[3 * u64{s}];
      y[s] = cat.xyz[3 * u64{s} + 1];
      z[s] = cat.xyz[3 * u64{s} + 2];
    }
    preparer(dom, x, y, z);
  }
  if (dom.n != cat.sites || !std::equal(dom.xyz.begin(), dom.xyz.end(), cat.xyz))
    refus("nuage : ordre des sites different du vidage (l'ordre de Morton refait doit etre l'identite)");
  mebcert::SupportTable table;
  table.build(cat.rec, cat.balls);
  std::cout << "{\"phase\":\"entree\",\"trame\":\"" << cat_file.frame() << "\",\"K\":" << K << ",\"sites\":" << dom.n
            << ",\"boules\":" << cat.balls << ",\"mutant_cote_nul\":" << (kMutantCoteNul ? "true" : "false") << "}\n"
            << std::flush;

  // Voisins exacts de tous les sites, puis juge par force brute sur un echantillon deterministe.
  auto t0 = Clock::now();
  const u32 feuille = 16;
  const std::vector<u32> voisins = g1::table_des_voisins(dom.xyz.data(), dom.n, K, feuille);
  const double t_voisins = secondes(t0);
  u64 ecarts_voisins = 0;
  const u32 nech = std::min(echantillon, dom.n);
  for (u32 j = 0; j < nech; ++j) {
    const u32 q = static_cast<u32>((u64{j} * dom.n) / nech);
    const auto ref = g1::voisins_force_brute(dom.xyz.data(), dom.n, q, K);
    for (u32 i = 0; i < K; ++i)
      if (voisins[u64{q} * K + i] != (i < ref.size() ? ref[i].site : kAucun)) {
        ++ecarts_voisins;
        break;
      }
  }
  std::cout << "{\"phase\":\"voisins\",\"K\":" << K << ",\"sites\":" << dom.n << ",\"feuille_arbre\":" << feuille
            << ",\"echantillon_force_brute\":" << nech << ",\"ecarts_force_brute\":" << ecarts_voisins
            << ",\"secondes\":" << t_voisins << "}\n"
            << std::flush;
  int code = ecarts_voisins == 0 ? 0 : 1;

  std::array<StatEnsemble, kEnsembles> total{};
  u64 total_route2 = 0, total_route3 = 0, total_route3_dedans = 0, total_v11_naissance = 0, total_ecarts = 0;
  Lem lem_total;
  std::vector<Sphere3> spheres_toutes;
  Recensement r, r2;
  std::vector<u32> cand_v, cand_f8, cand_f16, cand_u;
  TablePop pop;
  for (u32 k = 2; k <= K; ++k) {
    if (!seuls.empty() && std::find(seuls.begin(), seuls.end(), k) == seuls.end()) continue;
    const auto tk = Clock::now();
    const d::Reader ordre(dir + "/ordre_" + std::to_string(k) + ".bin");
    const auto& h = ordre.header();
    if (h.kind != d::kOrder || h.order != k || h.kmax != K || h.sites != dom.n || ordre.frame() != cat_file.frame())
      refus("ordre_" + std::to_string(k) + ".bin : en-tete");
    const auto [parts, np] = ordre.get<u32>("PARTS", 4 * k);
    const auto [info, ni] = ordre.get<d::PartRec>("PARTINF");
    const auto [part_off, npo] = ordre.get<u64>("PARTOFF");
    const auto [seeds, ns] = ordre.get<d::SeedRec>("SEEDS");
    if (ni != np || npo != ns + 1 || part_off[0] != 0 || part_off[ns] != np) refus("ordre : sections incoherentes");
    // Trace de chaque partie (pour la partie suivante et la graine de fin).
    std::vector<u32> trace_de(np);
    for (u64 t = 0; t < ns; ++t) {
      if (part_off[t + 1] < part_off[t]) refus("ordre : PARTOFF decroissant");
      for (u64 i = part_off[t]; i < part_off[t + 1]; ++i) trace_de[i] = static_cast<u32>(t);
    }
    pop.construire(cat, k);
    std::array<StatEnsemble, kEnsembles> st{};
    u64 route1 = 0, route2 = 0, route2_hors = 0, route2_dedans = 0, route3 = 0, route3_hors = 0, route3_dedans = 0;
    u64 v11_naissance = 0, v11_suivante = 0, ecarts = 0;
    Lem lem;
    std::map<std::pair<u32, u32>, u64> p_q;  // route 3 hors catalogue : (p, q_min) -> parties
    std::vector<Sphere3> spheres;
    for (u64 i = 0; i < np; ++i) {
      const u32* f = parts + i * k;
      for (u32 j = 0; j < k; ++j)
        if (f[j] >= dom.n || (j > 0 && f[j] <= f[j - 1])) refus("ordre : partie non strictement croissante");
      const auto& inf = info[i];
      if (inf.route == d::kRouteCatalogue) {
        ++route1;
        continue;
      }
      if (inf.route != d::kRouteCensusSaturated && inf.route != d::kRouteCensusComplete) refus("ordre : route");
      const bool hors = inf.ball == d::kNone;
      if (!hors && inf.ball >= cat.balls) refus("ordre : boule hors du catalogue");
      const num::Sphere sphere = plus_petite_boule(dom, f, k);
      recenser(dom, sphere, k, r);
      if (inf.route == d::kRouteCensusSaturated) {
        ++route2;
        ++(hors ? route2_hors : route2_dedans);
        if (r.complet || r.interieur.size() != k) refus("route 2 : census de seuil k non sature");
        const u32* v11 = r.interieur.data();
        const bool v11_nait = pop.contient(v11);
        // Coherence avec le vidage : partie suivante de la trace, sinon fin par la table (graine de fin 2).
        const u32 t = trace_de[i];
        if (i + 1 < part_off[t + 1]) {
          if (!std::equal(v11, v11 + k, parts + (i + 1) * k) || v11_nait)
            refus("route 2 : cible de la v11 differente de la partie suivante du vidage");
          ++v11_suivante;
        } else if (seeds[t].end != 2 || !v11_nait) {
          refus("route 2 : fin de trace sans la table de populations");
        }
        v11_naissance += v11_nait;
        // LEM-HORS-CAT (sphere hors de Cat_K) : p >= K - 2.
        if (hors) {
          if (k + 2 >= K) {
            ++lem.route2_triviales;
          } else {
            recenser(dom, sphere, K - 2, r2);
            if (r2.complet)
              contradiction(dir, dom, k, K, i, 2, f, static_cast<u32>(r2.interieur.size()), 0,
                            "route 2 hors de Cat_K avec p < K - 2");
            ++lem.route2_par_seuil;
          }
        } else {
          ++lem.hors_hypothese;
          if (cat.rec[inf.ball].p < k) refus("route 2 au catalogue : p < k");
        }
        // Ensembles de candidats.
        candidats_voisins(voisins, K, f, k, cand_v);
        candidats_fenetre(dom.n, 8, f, k, cand_f8);
        candidats_fenetre(dom.n, 16, f, k, cand_f16);
        cand_u.clear();
        std::set_union(cand_v.begin(), cand_v.end(), cand_f16.begin(), cand_f16.end(), std::back_inserter(cand_u));
        const num::LatticeSphere lattice(sphere);
        const std::array<const std::vector<u32>*, kEnsembles> ensembles = {&cand_v, &cand_f8, &cand_f16, &cand_u};
        for (int e = 0; e < kEnsembles; ++e) {
          const std::vector<u32>& cand = *ensembles[e];
          const Essai essai = essayer(sphere, cand, k, dom.points);
          StatEnsemble& s = st[e];
          s.candidats += cand.size();
          s.candidats_max = std::max<u64>(s.candidats_max, cand.size());
          s.tests += essai.tests;
          s.tests_max = std::max<u64>(s.tests_max, essai.tests);
          const bool v11_dans = inclus(v11, k, cand);
          if (essai.trouves == k) {
            ++s.certifiees;
            const bool egale = std::equal(v11, v11 + k, essai.cible.begin());
            s.cible_egale_v11 += egale;
            s.cible_naissance += pop.contient(essai.cible.data());
            s.cible_v11_naissance += v11_nait;
            // Juge : cible strictement interieure par la voie du census ; egalite <=> cible v11 dans les candidats.
            bool juste = egale == v11_dans;
            for (u32 j = 0; j < k && juste; ++j) {
              auto cote = lattice.side(dom.points[essai.cible[j]]);
              juste = cote.ok() && cote.value() < 0;
            }
            if (!juste) ++s.ecarts_juge;
          } else if (v11_dans) {
            ++s.ecarts_juge;  // k sites interieurs (la cible v11) etaient parmi les candidats
          }
        }
      } else {
        ++route3;
        ++(hors ? route3_hors : route3_dedans);
        if (!r.complet || r.interieur.size() >= k) refus("route 3 : census de seuil k non complet");
        const u32 p = static_cast<u32>(r.interieur.size()), m = static_cast<u32>(r.coquille.size());
        std::vector<num::Point> zp(m);
        for (u32 j = 0; j < m; ++j) zp[j] = dom.points[r.coquille[j]];
        std::array<u32, 4> sstar{d::kNone, d::kNone, d::kNone, d::kNone};
        const int q = mebcert::canonical_support(sphere, r.coquille.data(), zp.data(), m, sstar);
        if (q < 2) refus("route 3 : support canonique introuvable sur la coquille");
        if (hors) {
          if (table.find(sstar) != d::kNone) refus("route 3 hors catalogue : S* global present dans la table");
          if (p + 2 < K) contradiction(dir, dom, k, K, i, 3, f, p, static_cast<u32>(q), "p < K - 2");
          if (k + 1 < K) contradiction(dir, dom, k, K, i, 3, f, p, static_cast<u32>(q), "k < K - 1");
          if (p + 2 == K && q != 4) contradiction(dir, dom, k, K, i, 3, f, p, static_cast<u32>(q), "p = K - 2, q != 4");
          if (u64{p} + static_cast<u32>(q) < u64{K} + 2)
            contradiction(dir, dom, k, K, i, 3, f, p, static_cast<u32>(q), "p + q_min < K + 2");
          ++lem.route3_verifiees;
          ++p_q[{p, static_cast<u32>(q)}];
        } else {
          ++lem.hors_hypothese;
          const auto& b = cat.rec[inf.ball];
          if (b.p != p || b.m != m || b.q != static_cast<u32>(q) ||
              !std::equal(sstar.begin(), sstar.end(), std::begin(b.sstar)))
            refus("route 3 au catalogue : census et catalogue discordants");
          // Sphere du catalogue recensee : seulement si S*(b) n'est pas dans F (sinon le support local la trouve).
          if (mebcert::sorted_subset(b.sstar, b.q, f, k)) refus("route 3 au catalogue avec S* dans F");
        }
        spheres.push_back(Sphere3{sphere, sstar, hors});
      }
    }
    for (int e = 0; e < kEnsembles; ++e) ecarts += st[e].ecarts_juge;
    const auto [d_centre, d_sstar] = spheres_distinctes(spheres, false);
    const auto [dh_centre, dh_sstar] = spheres_distinctes(spheres, true);
    if (d_centre != d_sstar || dh_centre != dh_sstar) refus("spheres distinctes : centre et S* discordants");
    std::cout << "{\"phase\":\"ordre\",\"k\":" << k << ",\"parties\":" << np << ",\"route1\":" << route1
              << ",\"route2\":{\"parties\":" << route2 << ",\"hors_catalogue\":" << route2_hors
              << ",\"au_catalogue\":" << route2_dedans << ",\"cible_v11_naissance\":" << v11_naissance
              << ",\"cible_v11_partie_suivante\":" << v11_suivante << ",\"ensembles\":{";
    for (int e = 0; e < kEnsembles; ++e) {
      std::cout << (e ? "," : "") << '"' << kNomsEnsembles[e] << "\":";
      ecrire_ensemble(std::cout, st[e], route2);
    }
    std::cout << "}},\"route3\":{\"parties\":" << route3 << ",\"hors_catalogue\":" << route3_hors
              << ",\"au_catalogue\":" << route3_dedans
              << ",\"census_complets_sphere_au_catalogue_s_etoile_hors_de_f\":" << route3_dedans
              << ",\"spheres_distinctes\":" << d_centre
              << ",\"spheres_distinctes_hors_catalogue\":" << dh_centre << ",\"p_qmin_hors_catalogue\":{";
    bool premier = true;
    for (const auto& [pq, nb] : p_q) {
      std::cout << (premier ? "" : ",") << "\"p" << pq.first << "_q" << pq.second << "\":" << nb;
      premier = false;
    }
    std::cout << "}},\"lem_hors_cat\":{\"route2_triviales\":" << lem.route2_triviales
              << ",\"route2_census_seuil_K_moins_2\":" << lem.route2_par_seuil
              << ",\"route3_verifiees\":" << lem.route3_verifiees << ",\"hors_hypothese\":" << lem.hors_hypothese
              << ",\"contre_exemples\":0},\"table_populations\":" << pop.entrees() << ",\"ecarts_juge\":" << ecarts
              << ",\"secondes\":" << secondes(tk) << "}\n"
              << std::flush;
    for (int e = 0; e < kEnsembles; ++e) total[e].ajouter(st[e]);
    total_route2 += route2;
    total_route3 += route3;
    total_route3_dedans += route3_dedans;
    total_v11_naissance += v11_naissance;
    total_ecarts += ecarts;
    lem_total.route2_triviales += lem.route2_triviales;
    lem_total.route2_par_seuil += lem.route2_par_seuil;
    lem_total.route3_verifiees += lem.route3_verifiees;
    lem_total.hors_hypothese += lem.hors_hypothese;
    for (auto& s : spheres) spheres_toutes.push_back(std::move(s));
  }
  const auto [t_centre, t_sstar] = spheres_distinctes(spheres_toutes, false);
  const auto [th_centre, th_sstar] = spheres_distinctes(spheres_toutes, true);
  if (t_centre != t_sstar || th_centre != th_sstar) refus("spheres distinctes (tous ordres) : centre et S* discordants");
  if (total_ecarts != 0) code = 1;
  std::cout << "{\"phase\":\"bilan\",\"trame\":\"" << cat_file.frame() << "\",\"K\":" << K
            << ",\"route2\":{\"parties\":" << total_route2 << ",\"cible_v11_naissance\":" << total_v11_naissance
            << ",\"ensembles\":{";
  for (int e = 0; e < kEnsembles; ++e) {
    std::cout << (e ? "," : "") << '"' << kNomsEnsembles[e] << "\":";
    ecrire_ensemble(std::cout, total[e], total_route2);
  }
  std::cout << "}},\"route3\":{\"parties\":" << total_route3
            << ",\"census_complets_sphere_au_catalogue_s_etoile_hors_de_f\":" << total_route3_dedans
            << ",\"spheres_distinctes_tous_ordres\":" << t_centre
            << ",\"spheres_distinctes_hors_catalogue_tous_ordres\":" << th_centre
            << "},\"lem_hors_cat\":{\"route2_triviales\":" << lem_total.route2_triviales
            << ",\"route2_census_seuil_K_moins_2\":" << lem_total.route2_par_seuil
            << ",\"route3_verifiees\":" << lem_total.route3_verifiees << ",\"hors_hypothese\":" << lem_total.hors_hypothese
            << ",\"contre_exemples\":0},\"ecarts_voisins\":" << ecarts_voisins << ",\"ecarts_juge\":" << total_ecarts
            << ",\"secondes\":" << secondes(t_debut) << ",\"code\":" << code << "}\n"
            << std::flush;
  return code;
}

}  // namespace
}  // namespace mhgp12

int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string(argv[1]) == "--porte") return mhgp12::porte();
    if (argc < 2 || std::string(argv[1]).rfind("--", 0) == 0) {
      std::cerr << "usage : mhgp12_mes_g1 --porte | <dossier> [--ordres k1,k2,...] [--echantillon N]\n";
      return 2;
    }
    std::vector<mhgp11::u32> seuls;
    mhgp11::u32 echantillon = 512;
    for (int i = 2; i < argc; ++i) {
      const std::string opt = argv[i];
      if (opt == "--ordres" && i + 1 < argc) {
        const std::string list = argv[++i];
        std::size_t at = 0;
        while (at <= list.size()) {
          const std::size_t comma = list.find(',', at);
          const int k = std::atoi(list.substr(at, comma - at).c_str());
          if (k < 2 || k > 12) {
            std::cerr << "ordres : entiers de 2 a 12\n";
            return 2;
          }
          seuls.push_back(static_cast<mhgp11::u32>(k));
          if (comma == std::string::npos) break;
          at = comma + 1;
        }
      } else if (opt == "--echantillon" && i + 1 < argc) {
        const int v = std::atoi(argv[++i]);
        if (v < 1) {
          std::cerr << "echantillon : entier positif\n";
          return 2;
        }
        echantillon = static_cast<mhgp11::u32>(v);
      } else {
        std::cerr << "option inconnue : " << opt << "\n";
        return 2;
      }
    }
    return mhgp12::banc(argv[1], seuls, echantillon);
  } catch (const std::exception& e) {
    std::cout << "{\"phase\":\"exception\",\"message\":\"" << e.what() << "\"}\n";
    return 3;
  }
}
