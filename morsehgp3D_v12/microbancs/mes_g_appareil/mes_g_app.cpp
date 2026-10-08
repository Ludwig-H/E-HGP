// MES-G-APP (microbanc hors produit) : census par lots et premieres sondes de l'etage G, sur l'hote (le produit) et sur
// l'appareil (noyau_g.hpp, source unique), sur les memes requetes, avec identite exigee a l'octet.
//
//   mes_g_app --trame=<xyz.u32le>,<ids.u32le>[,NOM] [--k=5] [--fils=W] [--passes=N] [--appareil] [--sans-hote-hd]
//
// 1. Recolte : le produit (libmhgp12.a) construit nuage, index, Cat_K et l'etage G (resolve_tower, W fils). Chaque
//    census que G demande (CensusWorkspace::query garde a temoins) est INTERCEPTE a l'edition de liens
//    (-Wl,--wrap=<symbole>, sans copie ni modification du resolveur) : boule certifiee, seuil, temoins et resultat du
//    produit (genre, I, U, registre) sont enregistres. Controle : autant de census que les compteurs du produit
//    (census_saturated + census_complete), memes sommes de sites testes et de noeuds (census_sites, census_nodes).
// 2. Lot hote du census : les requetes recoltees (ordre canonique) rejouees par le produit (CensusWorkspace::query),
//    W fils, un espace par fil ; lot hote des premieres sondes : la voie G-L7 du produit (passes.cpp : trace, empreinte,
//    file de 16 representants prechargee, PopulationTable::find), W fils, tranches de 256 cellules.
// 3. Rejeu du noyau en source unique sur l'hote (W fils) : identite avec le produit (porte locale, sans appareil).
// 4. Appareil (--appareil, construction MESG_APPAREIL) : donnees chargees une fois (regime resident), puis par passe
//    un lancement de chaque noyau (un fil par requete ; un fil par cellule), duree des noyaux seuls ; resultats relus et
//    compares a l'octet au produit (genre, p, m, I, U et neuf compteurs du travail par requete ; naissance par
//    representant).
// Passes : une passe d'echauffement (publiee, marquee) puis N passes chronometrees ; chaque mesure est jouee deux fois
// par passe (A/A) ; l'ordre hote/appareil alterne d'une passe a l'autre. Sortie : une ligne JSON par etape.
// Codes : 0 identite complete ; 1 ecart d'identite ; 2 refus (usage, entree, appareil absent) ; 3 recolte incoherente.
#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cstring>
#include <chrono>
#include <cstdio>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "catalogue/catalogue.hpp"
#include "index/access.hpp"
#include "index/index.hpp"
#include "io/io.hpp"
#include "num/center_view.hpp"
#include "sched/sched.hpp"
#include "tower/internal.hpp"
#include "tower/proposal.hpp"  // DWelzl du produit (temoin de la proposition)
#include "tower/stage.hpp"  // kCellGrain : tranches de 256 cellules, comme l'etage G
#include "tower/tower.hpp"

#include "noyau_g.hpp"
#if defined(MESG_APPAREIL)
#include "appareil.hpp"
#endif

using namespace mhgp12;
using mesg::Noeud;
using mesg::Requete;
using mesg::Resultat;

// ------------------------------------------------------------------------------------------ interception du census
// Symbole de CensusWorkspace::query(index, boule certifiee, seuil, temoins, contexte, rappel), appele par
// src/tower/resolve.cpp ; le pilote verifie par nm que libmhgp12.a le definit et que resolve.cpp.o le reference.
#define MESG_SYMBOLE_CENSUS                                                                                      \
  "_ZN6mhgp1215CensusWorkspace5queryERKNS_11GlobalIndexERKNS_3num13CertifiedBallEjSt4spanIKNS_7SiteIdxELm1844" \
  "6744073709551615EEPvPFNS_7OutcomeESC_RKNS_14BorrowedCensusEE"

using Rappel = CensusWorkspace::Callback;
Outcome census_reel(CensusWorkspace* self, const GlobalIndex& index, const num::CertifiedBall& boule, u32 seuil,
                    std::span<const SiteIdx> temoins, void* contexte, Rappel rappel) noexcept
    __asm__("__real_" MESG_SYMBOLE_CENSUS);
Outcome census_intercepte(CensusWorkspace* self, const GlobalIndex& index, const num::CertifiedBall& boule, u32 seuil,
                          std::span<const SiteIdx> temoins, void* contexte, Rappel rappel) noexcept
    __asm__("__wrap_" MESG_SYMBOLE_CENSUS);

namespace {

struct Enregistrement {
  num::CertifiedBall boule;
  u32 seuil = 0, temoins_n = 0;
  std::array<u32, 4> temoins{};
  bool sature = false;
  CensusLedger registre{};
  std::vector<u32> interieur, coquille;
};

std::atomic<bool> g_actif{false};
std::mutex g_verrou;
std::vector<std::unique_ptr<std::vector<Enregistrement>>> g_tampons;
thread_local std::vector<Enregistrement>* t_tampon = nullptr;

std::vector<Enregistrement>& tampon_du_fil() {
  if (t_tampon == nullptr) {
    const std::lock_guard<std::mutex> garde(g_verrou);
    g_tampons.push_back(std::make_unique<std::vector<Enregistrement>>());
    t_tampon = g_tampons.back().get();
  }
  return *t_tampon;
}

struct ContexteInterception {
  void* contexte;
  Rappel rappel;
  const num::CertifiedBall* boule;
  u32 seuil;
  std::span<const SiteIdx> temoins;
};

Outcome rappel_enregistreur(void* brut, const BorrowedCensus& bc) noexcept {
  auto& c = *static_cast<ContexteInterception*>(brut);
  try {
    Enregistrement e{*c.boule, c.seuil, static_cast<u32>(c.temoins.size()), {}, bc.kind() == CensusKind::saturated,
                     bc.ledger(), {}, {}};
    for (std::size_t i = 0; i < c.temoins.size() && i < 4; ++i) e.temoins[i] = idx(c.temoins[i]);
    for (const SiteIdx s : bc.interior()) e.interieur.push_back(idx(s));
    for (const SiteIdx s : bc.shell()) e.coquille.push_back(idx(s));
    tampon_du_fil().push_back(std::move(e));
  } catch (...) {
    return fail(Reason::memory_budget);
  }
  return c.rappel(c.contexte, bc);
}

}  // namespace

Outcome census_intercepte(CensusWorkspace* self, const GlobalIndex& index, const num::CertifiedBall& boule, u32 seuil,
                          std::span<const SiteIdx> temoins, void* contexte, Rappel rappel) noexcept {
  if (!g_actif.load(std::memory_order_relaxed) || rappel == nullptr)
    return census_reel(self, index, boule, seuil, temoins, contexte, rappel);
  ContexteInterception c{contexte, rappel, &boule, seuil, temoins};
  return census_reel(self, index, boule, seuil, temoins, &c, &rappel_enregistreur);
}

namespace {

using Clock = std::chrono::steady_clock;
double ms_depuis(Clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

struct Options {
  std::string xyz, ids, nom = "trame";
  int k = 5;
  u32 fils = 1, passes = 5;
  bool appareil = false, hote_hd = true;
};

bool nombre(std::string_view t, u64& v) {
  const auto [fin, err] = std::from_chars(t.data(), t.data() + t.size(), v);
  return err == std::errc{} && fin == t.data() + t.size();
}

bool lire_options(int argc, char** argv, Options& o) {
  for (int i = 1; i < argc; ++i) {
    const std::string_view a = argv[i];
    u64 v = 0;
    if (a.rfind("--trame=", 0) == 0) {
      const std::string s(a.substr(8));
      const auto c1 = s.find(','), c2 = s.find(',', c1 == std::string::npos ? c1 : c1 + 1);
      if (c1 == std::string::npos) return false;
      o.xyz = s.substr(0, c1);
      o.ids = s.substr(c1 + 1, c2 == std::string::npos ? std::string::npos : c2 - c1 - 1);
      if (c2 != std::string::npos) o.nom = s.substr(c2 + 1);
    } else if (a.rfind("--k=", 0) == 0 && nombre(a.substr(4), v) && v >= 2 && v <= 12) {
      o.k = static_cast<int>(v);
    } else if (a.rfind("--fils=", 0) == 0 && nombre(a.substr(7), v) && v >= 1 && v <= sched::kMaxWorkers) {
      o.fils = static_cast<u32>(v);
    } else if (a.rfind("--passes=", 0) == 0 && nombre(a.substr(9), v) && v >= 1 && v <= 100) {
      o.passes = static_cast<u32>(v);
    } else if (a == "--appareil") {
      o.appareil = true;
    } else if (a == "--sans-hote-hd") {
      o.hote_hd = false;
    } else {
      return false;
    }
  }
  return !o.xyz.empty() && !o.ids.empty();
}

// Cle canonique d'un enregistrement : seuil, temoins, puis la boule (ancre, coefficients, repere).
using Cle = std::tuple<u32, u32, std::array<u32, 4>, std::array<u32, 3>, std::array<i128, 4>, std::array<u32, 3>, int>;
Cle cle_de(const Enregistrement& e) {
  const auto& sphere = e.boule.sphere();
  const num::detail::CenterView vue(sphere);
  std::array<i128, 4> c{};
  if (vue.native_coefficients()) c = {vue.n128()[0], vue.n128()[1], vue.n128()[2], vue.d128()};
  return {e.seuil, e.temoins_n, e.temoins, sphere.anchor().coordinates(), c, e.boule.corner(), e.boule.span()};
}

Requete vers_requete(const Enregistrement& e) {
  const auto& sphere = e.boule.sphere();
  const num::detail::CenterView vue(sphere);
  Requete r{};
  const int s = e.boule.span();
  for (int j = 0; j < 3; ++j) {
    r.ancre[j] = sphere.anchor().coordinates()[j];
    r.coin[j] = e.boule.corner()[j];
  }
  r.etendue = static_cast<u32>(s);
  r.seuil = e.seuil;
  r.temoins_n = e.temoins_n;
  for (int t = 0; t < 4; ++t) r.temoins[t] = e.temoins[t];
  if (!vue.native_coefficients()) {
    r.voie = mesg::kLarge;
    return r;
  }
  for (int j = 0; j < 3; ++j) r.n[j] = vue.n128()[j];
  r.d = vue.d128();
  // Voie de GuardedSphere::GuardedSphere (src/num/guard.cpp).
  if (num::tier_of(s) == num::Tier::narrow) r.voie = mesg::kNative;
  else if (sphere.power_domain() >= s + 2) r.voie = mesg::kCertifiee;
  else r.voie = mesg::kControlee;
  return r;
}

// Ecart d'un resultat (noyau en source unique) au produit ; vrai si identique.
bool identique(const Enregistrement& e, const Resultat& r, const u32* sites) {
  if ((r.drapeaux & mesg::kDrapeauInvariant) != 0) return false;
  const bool sature = (r.drapeaux & mesg::kDrapeauSature) != 0;
  if (sature != e.sature || r.p != e.interieur.size()) return false;
  for (u32 i = 0; i < r.p; ++i)
    if (sites[i] != e.interieur[i]) return false;
  if (!sature) {
    if (r.m != e.coquille.size()) return false;
    const u32 n = std::min<u32>(r.m, mesg::kMaxCoquille);
    for (u32 i = 0; i < n; ++i)
      if (sites[mesg::kMaxPartie + i] != e.coquille[i]) return false;
  }
  const auto& g = e.registre;
  return r.c.noeuds == g.nodes && r.c.tests == g.point_tests && r.c.dedans == g.inside_blocks &&
         r.c.dehors == g.outside_blocks && r.c.temoins == g.guard_witness && r.c.disjointes == g.guard_disjoint &&
         r.c.partielles == g.guard_partial && r.c.garde_dehors == g.guard_outside && r.c.voies == g.lanes.total();
}

// ----------------------------------------------------------------------------------------- lots de l'hote
struct LotCensusProduit {
  const GlobalIndex& index;
  const std::vector<Enregistrement>& recs;
  std::vector<std::unique_ptr<CensusWorkspace>>& espaces;
  std::vector<u64>& sortie;
  static Outcome rappel(void* brut, const BorrowedCensus& bc) noexcept {
    *static_cast<u64*>(brut) = (u64{bc.kind() == CensusKind::saturated} << 63) | (u64{bc.interior().size()} << 32) |
                                bc.shell().size();
    return {};
  }
  static Outcome corps(void* brut, u64 debut, u64 fin, u32 fil) noexcept {
    auto& s = *static_cast<LotCensusProduit*>(brut);
    for (u64 i = debut; i < fin; ++i) {
      const Enregistrement& e = s.recs[i];
      std::array<SiteIdx, 4> t{};
      for (u32 j = 0; j < e.temoins_n; ++j) t[j] = make_id<SiteIdx>(e.temoins[j]);
      MHGP12_TRY(census_reel(s.espaces[fil].get(), s.index, e.boule, e.seuil,
                             std::span<const SiteIdx>(t.data(), e.temoins_n), &s.sortie[i], &rappel));
    }
    return {};
  }
};

struct LotCensusHd {
  const std::vector<Requete>& req;
  const std::vector<Noeud>& noeuds;
  std::span<const u32> x, y, z;
  u32 feuille;
  std::vector<Resultat>& res;
  std::vector<u32>& sites;
  static Outcome corps(void* brut, u64 debut, u64 fin, u32) noexcept {
    auto& s = *static_cast<LotCensusHd*>(brut);
    for (u64 i = debut; i < fin; ++i) {
      u32* t = s.sites.data() + i * mesg::kSitesParRequete;
      mesg::census(s.req[i], s.noeuds.data(), s.noeuds.size(), s.x.data(), s.y.data(), s.z.data(), s.feuille, t,
                   t + mesg::kMaxPartie, s.res[i]);
    }
    return {};
  }
};

// Premieres sondes, voie G-L7 du produit (passes.cpp : OrderPass::body sans le reste de resolve_part).
bool trace_produit(std::span<const SiteIdx> inner, std::span<const SiteIdx> shell, u64 mask, tower_detail::Part& f) {
  std::size_t i = 0;
  for (u64 rest = mask; i < inner.size() || rest != 0;) {
    const u32 j = rest != 0 ? static_cast<u32>(__builtin_ctzll(rest)) : 0;
    const bool from_inner = rest == 0 || (i < inner.size() && idx(inner[i]) < idx(shell[j]));
    if (f.k == tower_detail::kMaxPart) return false;
    f.id[f.k++] = idx(from_inner ? inner[i++] : shell[j]);
    if (!from_inner) rest &= rest - 1;
  }
  return true;
}

struct LotSondesProduit {
  const Catalogue& cat;
  const ResolvedOrder& ordre;
  const tower_detail::PopulationTable& table;
  u32 k;
  std::vector<u32>& sortie;
  static constexpr u64 kLag = 16, kDemi = kLag / 2;
  struct Attente {
    tower_detail::Part f;
    u64 cle = 0, rep = 0;
  };
  static Outcome corps(void* brut, u64 debut, u64 fin, u32) noexcept {
    auto& s = *static_cast<LotSondesProduit*>(brut);
    const auto boules = s.ordre.cell_balls();
    const auto decalages = s.ordre.cell_offsets();
    const auto masques = s.ordre.trace_masks();
    std::array<Attente, kLag> file;
    std::array<u64, tower_detail::kMaxShell> cles_coquille;
    u64 entree = 0, sortie = 0;
    auto finir = [&](const Attente& a) {
      const auto hit = s.table.find(a.f, a.cle);
      s.sortie[a.rep] = hit ? hit->birth : mesg::kAucun;
    };
    for (u64 c = debut; c < fin; ++c) {
      const auto inner = s.cat.interior(boules[c]), shell = s.cat.shell(boules[c]);
      if (shell.size() > tower_detail::kMaxShell) return fail(Reason::tower_invariant);
      u64 base = 0;
      for (const SiteIdx site : inner) base += tower_detail::site_key(idx(site));
      for (std::size_t j = 0; j < shell.size(); ++j) cles_coquille[j] = tower_detail::site_key(idx(shell[j]));
      for (u64 r = decalages[c]; r < decalages[c + 1]; ++r) {
        Attente& a = file[entree % kLag];
        a.f = tower_detail::Part{};
        if (!trace_produit(inner, shell, masques[r], a.f) || a.f.k != s.k) return fail(Reason::tower_invariant);
        a.rep = r;
        u64 cle = base;
        for (u64 rest = masques[r]; rest != 0; rest &= rest - 1) cle += cles_coquille[__builtin_ctzll(rest)];
        a.cle = cle & s.table.key_mask();
        s.table.prefetch_directory(a.cle);
        if (entree >= kDemi) s.table.prefetch_bucket(file[(entree - kDemi) % kLag].cle);
        ++entree;
        if (entree - sortie == kLag) finir(file[sortie++ % kLag]);
      }
    }
    while (sortie < entree) finir(file[sortie++ % kLag]);
    return {};
  }
};

// Propositions : DWelzl du produit (resolve.cpp, propose, sans le tri final des SiteIdx) et noyau en source unique.
struct LotPropositions {
  const std::vector<u32>& parties;
  const std::vector<u32>& tailles;
  u32 pas;
  std::span<const u32> x, y, z;
  std::vector<mesg::Proposition>& sortie;
  bool produit;
  static Outcome corps(void* brut, u64 debut, u64 fin, u32) noexcept {
    auto& s = *static_cast<LotPropositions*>(brut);
    for (u64 i = debut; i < fin; ++i) {
      const u32* f = s.parties.data() + i * s.pas;
      const u32 k = s.tailles[i];
      if (!s.produit) {
        mesg::proposer(f, k, s.x.data(), s.y.data(), s.z.data(), s.sortie[i]);
        continue;
      }
      tower_detail::DWelzl w;
      const u32 o = f[0];
      for (u32 j = 0; j < k; ++j) {
        w.p[j][0] = static_cast<double>(s.x[f[j]]) - static_cast<double>(s.x[o]);
        w.p[j][1] = static_cast<double>(s.y[f[j]]) - static_cast<double>(s.y[o]);
        w.p[j][2] = static_cast<double>(s.z[f[j]]) - static_cast<double>(s.z[o]);
      }
      const tower_detail::DBall b = w.run(static_cast<int>(k));
      mesg::Proposition& out = s.sortie[i];
      for (int a = 0; a < 3; ++a) out.c[a] = b.c[a];
      out.r2 = b.r2;
      out.ok = w.ok ? 1u : 0u;
      out.nr = static_cast<u32>(b.nr);
      for (int j = 0; j < 4; ++j) out.r[j] = static_cast<u32>(b.R[j]);
    }
    return {};
  }
};

bool memes_bits(const mesg::Proposition& a, const mesg::Proposition& b) {
  if (a.ok != b.ok || a.nr != b.nr) return false;
  for (int j = 0; j < 4; ++j)
    if (a.r[j] != b.r[j]) return false;
  return std::memcmp(a.c, b.c, sizeof(a.c)) == 0 && std::memcmp(&a.r2, &b.r2, sizeof(a.r2)) == 0;
}

// Table des naissances au format de l'appareil (noyau_g.hpp : fiches de 3 + k mots dans l'ordre canonique
// (empreinte, population), repertoire de 2^d + 1 places, 2^d >= entrees).
struct TableHote {
  std::vector<u32> fiches, repertoire;
  u64 entrees = 0;
  u32 decalage = 64;
};

bool construire_table(const Catalogue& cat, const ResolvedOrder& ordre, u32 k, TableHote& t) {
  // Seules les naissances de population EXACTE k (p + m = k) y entrent (populations.cpp, exact_population) : une
  // coquille etendue sans trace separable est aussi une naissance, de population plus grande, jamais sondee.
  const auto naissances = ordre.birth_keys();
  std::vector<std::pair<u64, u32>> ordre_fiches;
  std::vector<u32> pop(naissances.size() * k);
  for (u64 i = 0; i < naissances.size(); ++i) {
    const auto inner = cat.interior(make_id<BallIdx>(naissances[i])),
               shell = cat.shell(make_id<BallIdx>(naissances[i]));
    if (inner.size() + shell.size() != k) continue;
    std::size_t a = 0, b = 0;
    u64 cle = 0;
    for (u32 j = 0; j < k; ++j) {
      const bool depuis_i = b == shell.size() || (a < inner.size() && idx(inner[a]) < idx(shell[b]));
      pop[i * k + j] = idx(depuis_i ? inner[a++] : shell[b++]);
      cle += mesg::cle_site(pop[i * k + j]);
    }
    ordre_fiches.push_back({cle, static_cast<u32>(i)});
  }
  const u64 e = ordre_fiches.size();
  if (e == 0) return false;
  std::sort(ordre_fiches.begin(), ordre_fiches.end(), [&](const auto& u, const auto& v) {
    if (u.first != v.first) return u.first < v.first;
    return std::lexicographical_compare(pop.begin() + u.second * k, pop.begin() + u.second * k + k,
                                        pop.begin() + v.second * k, pop.begin() + v.second * k + k);
  });
  u32 d = 0;
  while ((u64{1} << d) < e) ++d;
  t.decalage = d == 0 ? 64 : 64 - d;
  t.entrees = e;
  t.fiches.assign(e * (3 + k), 0);
  for (u64 pos = 0; pos < e; ++pos) {
    const auto [cle, i] = ordre_fiches[pos];
    u32* f = t.fiches.data() + pos * (3 + k);
    f[0] = static_cast<u32>(cle);
    f[1] = static_cast<u32>(cle >> 32);
    f[2] = i;
    for (u32 j = 0; j < k; ++j) f[3 + j] = pop[u64{i} * k + j];
  }
  const u64 seaux = u64{1} << d;
  t.repertoire.assign(seaux + 1, static_cast<u32>(e));
  u64 pos = 0;
  for (u64 s = 0; s < seaux; ++s) {
    while (pos < e) {
      const u64 cle = ordre_fiches[pos].first;
      if ((t.decalage >= 64 ? 0 : cle >> t.decalage) >= s) break;
      ++pos;
    }
    t.repertoire[s] = static_cast<u32>(pos);
  }
  return true;
}

int executer(const Options& o) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto entree = io::read_u32le(o.xyz.c_str(), o.ids.c_str(), budget);
  if (!entree.ok()) return 2;
  auto nuage = prepare_cloud(entree.value().x.span(), entree.value().y.span(), entree.value().z.span(),
                             entree.value().ids.span(), CoordWidth(), budget);
  if (!nuage.ok()) return 2;
  auto index = build_index(std::move(nuage.value()), IndexParams{}, budget);
  if (!index.ok()) return 2;
  auto pool = sched::make_pool({o.fils});
  if (!pool.ok()) return 2;
  CatalogueParams params;
  params.kmax = o.k;
  params.leaf_size = static_cast<u32>(std::max(24, o.k + 3));
  auto catalogue = build_catalogue(index.value().cloud(), params, budget, *pool.value());
  if (!catalogue.ok()) return 2;
  const GlobalIndex& ix = index.value();
  const Catalogue& cat = catalogue.value();
  const Cloud& cloud = ix.cloud();

  // 1. Recolte.
  g_actif.store(true);
  const auto t_g = Clock::now();
  auto resolution = resolve_tower(ix, cat, budget, *pool.value());
  const double g_ms = ms_depuis(t_g);
  g_actif.store(false);
  if (!resolution.ok()) return 3;
  const Resolution& res = resolution.value();
  std::vector<Enregistrement> recs;
  for (auto& tampon : g_tampons) {
    for (auto& e : *tampon) recs.push_back(std::move(e));
    tampon->clear();
  }
  std::vector<std::pair<Cle, u64>> cles(recs.size());
  for (u64 i = 0; i < recs.size(); ++i) cles[i] = {cle_de(recs[i]), i};
  std::sort(cles.begin(), cles.end());
  {
    std::vector<Enregistrement> tries;
    tries.reserve(recs.size());
    for (const auto& c : cles) tries.push_back(std::move(recs[c.second]));
    recs = std::move(tries);
  }
  u64 distinctes = recs.empty() ? 0 : 1;
  for (u64 i = 1; i < cles.size(); ++i) distinctes += cles[i].first != cles[i - 1].first;
  u64 attendu = 0, attendu_sites = 0, attendu_noeuds = 0, attendu_satures = 0, reps = 0, premieres = 0;
  for (Order k = 2; k <= res.orders(); ++k) {
    const OrderCounters& c = res.order(k).counters();
    attendu += c.census_saturated + c.census_complete;
    attendu_satures += c.census_saturated;
    attendu_sites += c.census_sites;
    attendu_noeuds += c.census_nodes;
    reps += res.order(k).representatives();
    premieres += c.first_probe_hits;
  }
  u64 sites_vus = 0, noeuds_vus = 0, satures_vus = 0;
  std::array<u64, 4> voies{};
  std::vector<Requete> req(recs.size());
  for (u64 i = 0; i < recs.size(); ++i) {
    sites_vus += recs[i].registre.point_tests;
    noeuds_vus += recs[i].registre.nodes;
    satures_vus += recs[i].sature;
    req[i] = vers_requete(recs[i]);
    ++voies[req[i].voie];
  }
  const bool recolte_ok = recs.size() == attendu && sites_vus == attendu_sites && noeuds_vus == attendu_noeuds &&
                          satures_vus == attendu_satures;
  std::printf("{\"phase\":\"recolte\",\"trame\":\"%s\",\"sites\":%u,\"k\":%d,\"fils\":%u,\"g_ms\":%.3f,"
              "\"requetes\":%llu,\"distinctes\":%llu,\"saturees\":%llu,\"attendues\":%llu,\"sites_testes\":%llu,"
              "\"noeuds\":%llu,\"voies\":{\"native\":%llu,\"certifiee\":%llu,\"controlee\":%llu,\"large\":%llu},"
              "\"representants\":%llu,\"premieres_sondes_reussies\":%llu,\"boules\":%u,\"noeuds_index\":%llu,"
              "\"incidences\":%llu,\"niveaux\":%llu,\"octets_niveau\":%llu,\"octets_boule\":%llu,"
              "\"recolte_ok\":%s}\n",
              o.nom.c_str(), cloud.sites(), o.k, o.fils, g_ms, (unsigned long long)recs.size(),
              (unsigned long long)distinctes, (unsigned long long)satures_vus, (unsigned long long)attendu,
              (unsigned long long)sites_vus, (unsigned long long)noeuds_vus, (unsigned long long)voies[0],
              (unsigned long long)voies[1], (unsigned long long)voies[2], (unsigned long long)voies[3],
              (unsigned long long)reps, (unsigned long long)premieres, cat.balls(), (unsigned long long)ix.nodes(),
              (unsigned long long)cat.population().size(), (unsigned long long)cat.levels().size(),
              (unsigned long long)sizeof(num::Level), (unsigned long long)sizeof(CatalogueBall),
              recolte_ok ? "true" : "false");
  std::fflush(stdout);
  if (!recolte_ok) return 3;

  // Index et sites au format du noyau.
  std::vector<Noeud> noeuds;
  for (const auto& n : index_detail::Access::nodes(ix)) {
    const auto lo = n.box.lo().coordinates(), hi = n.box.hi().coordinates();
    noeuds.push_back(Noeud{{lo[0], lo[1], lo[2]}, {hi[0], hi[1], hi[2]}, n.begin, n.end, n.escape});
  }
  const u32 feuille = ix.leaf_size();

  // Premieres sondes : tables du produit et de l'appareil, par ordre.
  struct Ordre {
    u32 k = 0;
    tower_detail::PopulationTable produit;
    TableHote table;
    std::vector<u32> attendu, sondes;
  };
  std::vector<std::unique_ptr<Ordre>> ordres;
  for (Order k = 2; k <= res.orders(); ++k) {
    auto ord = std::make_unique<Ordre>();
    ord->k = k;
    const ResolvedOrder& ro = res.order(k);
    if (const Outcome b = ord->produit.build(cat, ro.birth_keys(), ro.birth_ranks(), k, budget, *pool.value());
        !b.ok()) {
      std::printf("{\"phase\":\"refus\",\"raison\":\"table_produit\",\"ordre\":%u,\"motif\":\"%s\"}\n",
                  unsigned{k}, std::string(reason_name(b.reason)).c_str());
      return 3;
    }
    if (!construire_table(cat, ro, k, ord->table)) {
      std::printf("{\"phase\":\"refus\",\"raison\":\"table_appareil\",\"ordre\":%u}\n", unsigned{k});
      return 3;
    }
    ord->attendu.assign(ro.representatives(), mesg::kAucun);
    ordres.push_back(std::move(ord));
  }

  // Lots de l'hote.
  std::vector<std::unique_ptr<CensusWorkspace>> espaces;
  for (u32 w = 0; w < pool.value()->size(); ++w) {
    auto e = CensusWorkspace::make(ix, budget);
    if (!e.ok()) return 2;
    espaces.push_back(std::move(e.value()));
  }
  std::vector<u64> sortie_produit(recs.size());
  LotCensusProduit lot_produit{ix, recs, espaces, sortie_produit};
  auto jouer_census_hote = [&]() {
    const auto t0 = Clock::now();
    const Outcome r = pool.value()->parallel_for(recs.size(), 64, &lot_produit, &LotCensusProduit::corps);
    return r.ok() ? ms_depuis(t0) : -1.0;
  };
  auto jouer_sondes_hote = [&]() {
    const auto t0 = Clock::now();
    for (auto& ord : ordres) {
      LotSondesProduit lot{cat, res.order(static_cast<Order>(ord->k)), ord->produit, ord->k, ord->attendu};
      if (!pool.value()->parallel_for(res.order(static_cast<Order>(ord->k)).cells(), tower_detail::kCellGrain, &lot,
                                      &LotSondesProduit::corps)
               .ok())
        return -1.0;
    }
    return ms_depuis(t0);
  };
  std::vector<Resultat> res_hd(recs.size());
  std::vector<u32> sites_hd(recs.size() * mesg::kSitesParRequete);
  LotCensusHd lot_hd{req, noeuds, cloud.x(), cloud.y(), cloud.z(), feuille, res_hd, sites_hd};
  auto jouer_census_hd = [&]() {
    const auto t0 = Clock::now();
    const Outcome r = pool.value()->parallel_for(recs.size(), 64, &lot_hd, &LotCensusHd::corps);
    return r.ok() ? ms_depuis(t0) : -1.0;
  };

  // Pre-passe (non chronometree) : premieres sondes du produit, puis parties proposees = traces dont la premiere
  // sonde a echoue (celles que G envoie a la proposition au premier pas de leur chaine).
  if (jouer_sondes_hote() < 0) {
    std::printf("{\"phase\":\"refus\",\"raison\":\"sondes_produit\"}\n");
    return 3;
  }
  std::vector<u32> parties, tailles;
  const u32 pas = static_cast<u32>(o.k);
  for (auto& ord : ordres) {
    const ResolvedOrder& ro = res.order(static_cast<Order>(ord->k));
    const auto boules = ro.cell_balls();
    const auto decalages = ro.cell_offsets();
    const auto masques = ro.trace_masks();
    for (u64 c = 0; c < ro.cells(); ++c) {
      const auto inner = cat.interior(boules[c]), shell = cat.shell(boules[c]);
      for (u64 r = decalages[c]; r < decalages[c + 1]; ++r) {
        if (ord->attendu[r] != mesg::kAucun) continue;
        tower_detail::Part f;
        if (!trace_produit(inner, shell, masques[r], f) || f.k != ord->k) {
          std::printf("{\"phase\":\"refus\",\"raison\":\"trace\",\"ordre\":%u}\n", ord->k);
          return 3;
        }
        for (u32 j = 0; j < pas; ++j) parties.push_back(j < f.k ? f.id[j] : 0u);
        tailles.push_back(f.k);
      }
    }
  }
  std::vector<mesg::Proposition> prop_produit(tailles.size()), prop_hd(tailles.size());
  LotPropositions lot_prop{parties, tailles, pas, cloud.x(), cloud.y(), cloud.z(), prop_produit, true};
  LotPropositions lot_prop_hd{parties, tailles, pas, cloud.x(), cloud.y(), cloud.z(), prop_hd, false};
  auto jouer_propositions_hote = [&](LotPropositions& lot) {
    const auto t0 = Clock::now();
    const Outcome r = pool.value()->parallel_for(tailles.size(), 256, &lot, &LotPropositions::corps);
    return r.ok() ? ms_depuis(t0) : -1.0;
  };

#if defined(MESG_APPAREIL)
  mesg::Appareil appareil;
  mesg::Proprietes props;
  mesg::Transferts tr_census, tr_sondes, tr_props;
  std::string erreur;
  const bool avec_appareil = o.appareil;
  if (avec_appareil) {
    if (!appareil.ouvrir(props, erreur)) {
      std::printf("{\"phase\":\"refus\",\"raison\":\"appareil\",\"message\":\"%s\"}\n", erreur.c_str());
      return 2;
    }
    mesg::DonneesCensus dc{noeuds.data(), noeuds.size(), cloud.x().data(), cloud.y().data(), cloud.z().data(),
                           cloud.sites(), feuille, req.data(), req.size()};
    std::vector<u32> boule_p(cat.balls());
    for (u32 b = 0; b < cat.balls(); ++b) boule_p[b] = cat.balls_data()[b].p;
    static_assert(sizeof(SiteIdx) == sizeof(u32));
    mesg::DonneesCatalogue dk{cat.population_offsets().data(),
                              reinterpret_cast<const u32*>(cat.population().data()), boule_p.data(), cat.balls(),
                              cat.population().size()};
    if (!appareil.charger_census(dc, tr_census, erreur) || !appareil.charger_catalogue(dk, tr_sondes, erreur)) {
      std::printf("{\"phase\":\"refus\",\"raison\":\"chargement\",\"message\":\"%s\"}\n", erreur.c_str());
      return 2;
    }
    for (auto& ord : ordres) {
      const ResolvedOrder& ro = res.order(static_cast<Order>(ord->k));
      static_assert(sizeof(BallIdx) == sizeof(u32));
      mesg::DonneesSondes ds{ord->k,
                             ord->table.fiches.data(),
                             ord->table.entrees,
                             ord->table.repertoire.data(),
                             ord->table.repertoire.size() - 1,
                             ord->table.decalage,
                             reinterpret_cast<const u32*>(ro.cell_balls().data()),
                             ro.cell_offsets().data(),
                             ro.cells(),
                             ro.trace_masks().data(),
                             ro.representatives()};
      if (!appareil.charger_sondes(ds, tr_sondes, erreur)) {
        std::printf("{\"phase\":\"refus\",\"raison\":\"chargement\",\"message\":\"%s\"}\n", erreur.c_str());
        return 2;
      }
    }
    mesg::DonneesPropositions dp{parties.data(), tailles.data(), pas, tailles.size()};
    if (!appareil.charger_propositions(dp, tr_props, erreur)) {
      std::printf("{\"phase\":\"refus\",\"raison\":\"chargement\",\"message\":\"%s\"}\n", erreur.c_str());
      return 2;
    }
    std::printf("{\"phase\":\"appareil\",\"nom\":\"%s\",\"sm\":%d,\"cc\":\"%d.%d\",\"memoire\":%llu,\"l2\":%llu,"
                "\"horloge_khz\":%d,\"horloge_memoire_khz\":%d,\"bus_bits\":%d,\"pilote\":%d,\"execution\":%d,"
                "\"census_h2d_ms\":%.3f,\"census_h2d_octets\":%llu,\"sondes_h2d_ms\":%.3f,\"sondes_h2d_octets\":%llu,"
                "\"propositions_h2d_ms\":%.3f,\"propositions_h2d_octets\":%llu}\n",
                props.nom.c_str(), props.sm, props.cc_majeur, props.cc_mineur, (unsigned long long)props.memoire,
                (unsigned long long)props.l2, props.horloge_khz, props.horloge_memoire_khz, props.bus_bits,
                props.pilote, props.execution, tr_census.h2d_ms, (unsigned long long)tr_census.h2d_octets,
                tr_sondes.h2d_ms, (unsigned long long)tr_sondes.h2d_octets, tr_props.h2d_ms,
                (unsigned long long)tr_props.h2d_octets);
  }
#else
  const bool avec_appareil = false;
  if (o.appareil) {
    std::printf("{\"phase\":\"refus\",\"raison\":\"construction_sans_appareil\"}\n");
    return 2;
  }
#endif

  // 2-4. Passes : echauffement (0) puis o.passes passes chronometrees, deux prises par mesure (A/A).
  for (u32 passe = 0; passe <= o.passes; ++passe) {
    double c_cpu[2] = {0, 0}, s_cpu[2] = {0, 0}, p_cpu[2] = {0, 0}, c_hd = -1, p_hd = -1;
    float c_gpu[2] = {0, 0}, s_gpu[2] = {0, 0}, p_gpu[2] = {0, 0};
    bool ok = true;
    auto hote = [&]() {
      for (int a = 0; a < 2; ++a) {
        c_cpu[a] = jouer_census_hote();
        s_cpu[a] = jouer_sondes_hote();
        p_cpu[a] = jouer_propositions_hote(lot_prop);
        ok = ok && c_cpu[a] >= 0 && s_cpu[a] >= 0 && p_cpu[a] >= 0;
      }
      if (o.hote_hd) {
        c_hd = jouer_census_hd();
        p_hd = jouer_propositions_hote(lot_prop_hd);
        ok = ok && c_hd >= 0 && p_hd >= 0;
      }
    };
    auto dispositif = [&]() {
#if defined(MESG_APPAREIL)
      if (!avec_appareil) return;
      for (int a = 0; a < 2; ++a) {
        ok = ok && appareil.jouer_census(c_gpu[a], erreur) && appareil.jouer_sondes(s_gpu[a], erreur) &&
             appareil.jouer_propositions(p_gpu[a], erreur);
      }
#endif
    };
    if (passe % 2 == 0) {
      hote();
      dispositif();
    } else {
      dispositif();
      hote();
    }
    std::printf("{\"phase\":\"passe\",\"passe\":%u,\"echauffement\":%s,\"ok\":%s,\"census\":{\"hote_ms\":[%.4f,%.4f],"
                "\"hote_hd_ms\":%.4f,\"appareil_ms\":[%.4f,%.4f]},\"sondes\":{\"hote_ms\":[%.4f,%.4f],"
                "\"appareil_ms\":[%.4f,%.4f]},\"propositions\":{\"hote_ms\":[%.4f,%.4f],\"hote_hd_ms\":%.4f,"
                "\"appareil_ms\":[%.4f,%.4f]}}\n",
                passe, passe == 0 ? "true" : "false", ok ? "true" : "false", c_cpu[0], c_cpu[1], c_hd, c_gpu[0],
                c_gpu[1], s_cpu[0], s_cpu[1], s_gpu[0], s_gpu[1], p_cpu[0], p_cpu[1], p_hd, p_gpu[0], p_gpu[1]);
    std::fflush(stdout);
    if (!ok) return 2;
  }

  // Identite : noyau en source unique (hote, puis appareil) contre le produit ; sondes de l'appareil contre
  // PopulationTable::find du produit.
  u64 non_resolues = 0, coquilles_larges = 0, ecarts_hd = 0, comparees_hd = 0;
  for (u64 i = 0; i < recs.size(); ++i) {
    if (req[i].voie != mesg::kNative && req[i].voie != mesg::kCertifiee) {
      ++non_resolues;
      continue;
    }
    if (!o.hote_hd) continue;
    ++comparees_hd;
    if ((res_hd[i].drapeaux & mesg::kDrapeauCoquilleLarge) != 0) ++coquilles_larges;
    if (!identique(recs[i], res_hd[i], sites_hd.data() + i * mesg::kSitesParRequete)) ++ecarts_hd;
  }
  u64 ecarts_produit = 0;  // lot hote rejoue contre la recolte
  for (u64 i = 0; i < recs.size(); ++i) {
    const u64 attendu_lot = (u64{recs[i].sature} << 63) | (u64{recs[i].interieur.size()} << 32) |
                            (recs[i].sature ? 0 : recs[i].coquille.size());
    ecarts_produit += sortie_produit[i] != attendu_lot;
  }
  u64 ecarts_prop_hd = 0, ecarts_prop_gpu = 0, prop_ok = 0;
  for (u64 i = 0; i < tailles.size(); ++i) {
    prop_ok += prop_produit[i].ok;
    if (o.hote_hd) ecarts_prop_hd += !memes_bits(prop_produit[i], prop_hd[i]);
  }
  u64 ecarts_gpu = 0, comparees_gpu = 0, ecarts_sondes = 0, comparees_sondes = 0, reussies_gpu = 0,
      reussies_produit = 0;
#if defined(MESG_APPAREIL)
  if (avec_appareil) {
    std::vector<Resultat> res_gpu(recs.size());
    std::vector<u32> sites_gpu(recs.size() * mesg::kSitesParRequete);
    if (!appareil.lire_census(res_gpu.data(), sites_gpu.data(), tr_census, erreur)) return 2;
    for (u64 i = 0; i < recs.size(); ++i) {
      if (req[i].voie != mesg::kNative && req[i].voie != mesg::kCertifiee) {
        if ((res_gpu[i].drapeaux & mesg::kDrapeauNonResolu) == 0) ++ecarts_gpu;
        continue;
      }
      ++comparees_gpu;
      if (!identique(recs[i], res_gpu[i], sites_gpu.data() + i * mesg::kSitesParRequete)) ++ecarts_gpu;
    }
    for (u32 i = 0; i < ordres.size(); ++i) {
      auto& ord = *ordres[i];
      ord.sondes.assign(ord.attendu.size(), 0);
      if (!appareil.lire_sondes(i, ord.sondes.data(), tr_sondes, erreur)) return 2;
      for (u64 r = 0; r < ord.attendu.size(); ++r) {
        ++comparees_sondes;
        ecarts_sondes += ord.sondes[r] != ord.attendu[r];
        reussies_gpu += ord.sondes[r] != mesg::kAucun;
        reussies_produit += ord.attendu[r] != mesg::kAucun;
      }
    }
    std::vector<mesg::Proposition> prop_gpu(tailles.size());
    if (!appareil.lire_propositions(prop_gpu.data(), tr_props, erreur)) return 2;
    for (u64 i = 0; i < tailles.size(); ++i) ecarts_prop_gpu += !memes_bits(prop_produit[i], prop_gpu[i]);
    std::printf("{\"phase\":\"transferts\",\"census_d2h_ms\":%.3f,\"census_d2h_octets\":%llu,\"sondes_d2h_ms\":%.3f,"
                "\"sondes_d2h_octets\":%llu,\"propositions_d2h_ms\":%.3f,\"propositions_d2h_octets\":%llu}\n",
                tr_census.d2h_ms, (unsigned long long)tr_census.d2h_octets, tr_sondes.d2h_ms,
                (unsigned long long)tr_sondes.d2h_octets, tr_props.d2h_ms, (unsigned long long)tr_props.d2h_octets);
  }
#endif
  if (!avec_appareil)
    for (auto& ord : ordres)
      for (const u32 v : ord->attendu) reussies_produit += v != mesg::kAucun;
  const bool sondes_coherentes = reussies_produit == premieres;
  const bool identite = ecarts_hd == 0 && ecarts_gpu == 0 && ecarts_sondes == 0 && ecarts_produit == 0 &&
                        sondes_coherentes && coquilles_larges == 0 && ecarts_prop_hd == 0 && ecarts_prop_gpu == 0;
  std::printf("{\"phase\":\"identite\",\"census\":{\"requetes\":%llu,\"non_resolues\":%llu,\"comparees_hote_hd\":%llu,"
              "\"ecarts_hote_hd\":%llu,\"comparees_appareil\":%llu,\"ecarts_appareil\":%llu,\"ecarts_lot_produit\":%llu,"
              "\"coquilles_larges\":%llu},\"sondes\":{\"comparees_appareil\":%llu,\"ecarts_appareil\":%llu,"
              "\"reussies_produit\":%llu,\"reussies_appareil\":%llu,\"premieres_sondes_reussies_g\":%llu,"
              "\"coherentes\":%s},\"propositions\":{\"parties\":%llu,\"abouties\":%llu,\"ecarts_hote_hd\":%llu,"
              "\"ecarts_appareil\":%llu},\"appareil\":%s,\"identite\":%s}\n",
              (unsigned long long)recs.size(), (unsigned long long)non_resolues, (unsigned long long)comparees_hd,
              (unsigned long long)ecarts_hd, (unsigned long long)comparees_gpu, (unsigned long long)ecarts_gpu,
              (unsigned long long)ecarts_produit, (unsigned long long)coquilles_larges,
              (unsigned long long)comparees_sondes, (unsigned long long)ecarts_sondes,
              (unsigned long long)reussies_produit, (unsigned long long)reussies_gpu, (unsigned long long)premieres,
              sondes_coherentes ? "true" : "false", (unsigned long long)tailles.size(), (unsigned long long)prop_ok,
              (unsigned long long)ecarts_prop_hd, (unsigned long long)ecarts_prop_gpu, avec_appareil ? "true" : "false",
              identite ? "true" : "false");
  std::fflush(stdout);
  return identite ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
  Options o;
  if (!lire_options(argc, argv, o)) {
    std::fprintf(stderr, "usage : mes_g_app --trame=<xyz.u32le>,<ids.u32le>[,NOM] [--k=5] [--fils=W] [--passes=N] "
                         "[--appareil] [--sans-hote-hd]\n");
    return 2;
  }
  try {
    return executer(o);
  } catch (const std::bad_alloc&) {
    std::printf("{\"phase\":\"refus\",\"raison\":\"memoire\"}\n");
    return 2;
  }
}
