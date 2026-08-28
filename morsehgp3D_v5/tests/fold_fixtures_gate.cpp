// MorseHGP3D v5 — SIX FIXTURES GRAVEES DU FOLD (docs/ECHELLE.md § 8 bis,
// item 1). Elles ne passent pas par le pipeline : les evenements sont ecrits
// a la main, aux identifiants et aux niveaux exacts, et la SORTIE ATTENDUE
// est gravee litteralement — deltas dans l'ordre, `output`, `parents`,
// `born`. Chaque fixture est verifiee trois fois :
//   (R) le fold RESIDENT (`build_forest`) egale le texte grave ;
//   (V) le reducteur VIVANT (`reduce_fold_live`) egale le meme texte ;
//   (H) les deux, avec l'empreinte d'adressage forcee a une CONSTANTE
//       (`fold_hash_constant`), donnent EXACTEMENT le meme texte : la
//       frontiere externe (partition, domicile, sondage) ne participe a
//       aucune decision — seule la comparaison exacte de cle le fait.
// Les six : A etoile K = 1 de 300 aretes a niveaux croissants ; B chaine
// K = 1 {0,1} puis {0,2} ; C deux simplexes K = 2 partageant une facette ;
// D plateau mono-lot a pic transitoire 3 (l'ordre des deltas y est celui des
// racines LOGIQUES, pas des cles) ; E grand composant absorbe logiquement par
// un singleton (racine logique = singleton, conteneur physique = le gros) ;
// F frontieres externes a hachage constant (les cinq precedentes).
// Codes : 0 conforme ; 1 desaccord ; 2 refus ; 3 plancher.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "../src/forest/fold_live.hpp"

using namespace mhgp5;

namespace {

ExactLevel lvl(u64 n) { return ExactLevel{{n, 0, 0}, 1}; }

// Un evenement K = q + d - 1 : `sup` est l'ordre des supports (il fixe la
// facette `first`, donc la racine logique), `act` le masque des ACTIVES.
ForestEvent ev(std::vector<PointId> sup, std::vector<PointId> in, u16 act, u64 level) {
  ForestEvent e;
  e.q = (u8)sup.size();
  e.d = (u8)in.size();
  e.active_mask = act;
  for (size_t i = 0; i < sup.size(); ++i) e.support[i] = sup[i];
  for (size_t i = 0; i < in.size(); ++i) e.interior[i] = in[i];
  e.level = lvl(level);
  return e;
}

std::string key_str(const FacetKey& f) {
  std::string s = "{";
  for (u8 i = 0; i < f.k; ++i) {
    if (i) s += ",";
    s += std::to_string((unsigned long long)f.p[i]);
  }
  return s + "}";
}

std::string render(const ForestResult& r) {
  std::string s;
  for (const ComponentDelta& d : r.deltas) {
    s += "b=" + std::to_string((unsigned long long)d.batch) + " out=" + key_str(d.output) + " par=[";
    for (size_t i = 0; i < d.parents.size(); ++i) s += (i ? "," : "") + key_str(d.parents[i]);
    s += "] born=[";
    for (size_t i = 0; i < d.born.size(); ++i) s += (i ? "," : "") + key_str(d.born[i]);
    s += "]\n";
  }
  return s;
}

u64 g_checked = 0, g_bad = 0, g_deltas = 0;

// Compare (R), (V) et (H) au texte grave. `expect` vide = la fixture n'est
// verifiee qu'en EGALITE resident/vivant (fixtures a 300 aretes : le texte
// grave serait illisible, le plancher les couvre par la somme des deltas).
void check(const char* name, const std::vector<ForestEvent>& events, const char* expect) {
  ++g_checked;
  const ForestResult res = build_forest(events, 1);
  const ForestResult liv = reduce_fold_live(prepare_fold(events, 1));
  const std::string sr = render(res), sv = render(liv);
  g_deltas += res.deltas.size();
  fold_detail::fold_hash_constant() = true;
  const ForestResult resh = build_forest(events, 1);
  const ForestResult livh = reduce_fold_live(prepare_fold(events, 1));
  fold_detail::fold_hash_constant() = false;
  const std::string srh = render(resh), svh = render(livh);
  bool bad = false;
  if (expect && sr != expect) {
    std::printf("%s : (R) DESACCORD avec la fixture gravee\n--- obtenu ---\n%s--- attendu ---\n%s", name, sr.c_str(), expect);
    bad = true;
  }
  if (sv != sr) {
    std::printf("%s : (V) le vivant differe du resident\n--- vivant ---\n%s--- resident ---\n%s", name, sv.c_str(), sr.c_str());
    bad = true;
  }
  if (srh != sr || svh != sr) {
    std::printf("%s : (H) le hachage constant change la sortie\n", name);
    bad = true;
  }
  if (res.facets != resh.facets || res.fusions != resh.fusions || res.batches != resh.batches ||
      res.final_canon_fid != resh.final_canon_fid || res.facet_keys != resh.facet_keys) {
    std::printf("%s : (H) catalogue ou partition changes par le hachage constant\n", name);
    bad = true;
  }
  if (res.attach_violations || res.birth_violations || res.partition_violations) {
    std::printf("%s : invariants du fold non nuls (%llu/%llu/%llu)\n", name, (unsigned long long)res.attach_violations,
                (unsigned long long)res.birth_violations, (unsigned long long)res.partition_violations);
    bad = true;
  }
  if (bad) ++g_bad;
}

}  // namespace

int main(int argc, char** argv) {
  u64 min_fixtures = 6, min_deltas = 350;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--min-fixtures=", 0) == 0) min_fixtures = (u64)std::atoll(a.c_str() + 15);
    else if (a.rfind("--min-deltas=", 0) == 0) min_deltas = (u64)std::atoll(a.c_str() + 13);
    else return 2;
  }

  // ---- A. Etoile K = 1 de 300 aretes a niveaux STRICTEMENT croissants.
  // Arete {c, i} avec support [i, c] : slot 0 = {c}, slot 1 = {i}. Au premier
  // niveau les deux facettes naissent ; ensuite {c} est ACTIVE (bit 0) et {i}
  // s'attache. Un lot par niveau : 300 lots, 300 deltas, tous de sortie {0}.
  {
    std::vector<ForestEvent> e;
    for (u32 i = 1; i <= 300; ++i) e.push_back(ev({i, 0}, {}, i == 1 ? 0 : 1u, i));
    check("A_etoile_300", e, nullptr);
    // Les trois premiers deltas sont graves explicitement.
    std::vector<ForestEvent> e3(e.begin(), e.begin() + 3);
    check("A_etoile_3_gravee", e3,
          "b=0 out={0} par=[] born=[{0},{1}]\n"
          "b=1 out={0} par=[{0}] born=[{2}]\n"
          "b=2 out={0} par=[{0}] born=[{3}]\n");
  }

  // ---- B. Chaine K = 1 : {0,1} au niveau 1 puis {0,2} au niveau 2.
  check("B_chaine", {ev({1, 0}, {}, 0, 1), ev({2, 0}, {}, 1u, 2)},
        "b=0 out={0} par=[] born=[{0},{1}]\n"
        "b=1 out={0} par=[{0}] born=[{2}]\n");

  // ---- C. Deux simplexes K = 2 partageant la facette {0,1}.
  // σ1 = {0,1,2} au niveau 1 : facettes {1,2}, {0,2}, {0,1}, toutes nees.
  // σ2 = {0,1,3} au niveau 2 : {1,3} et {0,3} naissent, {0,1} est ACTIVE
  // (slot 2, masque 0b100 = 4).
  check("C_deux_simplexes", {ev({0, 1, 2}, {}, 0, 1), ev({0, 1, 3}, {}, 4u, 2)},
        "b=0 out={0,1} par=[] born=[{0,1},{0,2},{1,2}]\n"
        "b=1 out={0,1} par=[{0,1}] born=[{0,3},{1,3}]\n");

  // ---- D. Plateau MONO-LOT a pic transitoire 3 : trois simplexes K = 2
  // DISJOINTS au MEME niveau exact. Un seul lot, trois composantes, donc
  // trois deltas — dans l'ordre des RACINES LOGIQUES (la facette `first` de
  // chaque evenement, c'est-a-dire σ prive de son PREMIER support), et cet
  // ordre DIFFERE ici de l'ordre des cles de sortie : la fixture separe donc
  // les deux regles.
  //   fids par cle croissante : {0,1}=0 {0,2}=1 {0,8}=2 {0,9}=3 {1,9}=4
  //                             {2,8}=5 {3,4}=6 {3,5}=7 {4,5}=8
  //   X = {0,1,9} support [0,1,9] -> first = {1,9} = 4, sortie {0,1}
  //   Y = {0,2,8} support [8,0,2] -> first = {0,2} = 1, sortie {0,2}
  //   Z = {3,4,5} support [3,4,5] -> first = {4,5} = 8, sortie {3,4}
  //   ordre des racines : 1 (Y), 4 (X), 8 (Z) -> {0,2}, {0,1}, {3,4}
  //   ordre des CLES de sortie (refute) :        {0,1}, {0,2}, {3,4}
  check("D_plateau_pic3", {ev({0, 1, 9}, {}, 0, 7), ev({8, 0, 2}, {}, 0, 7), ev({3, 4, 5}, {}, 0, 7)},
        "b=0 out={0,2} par=[] born=[{0,2},{0,8},{2,8}]\n"
        "b=0 out={0,1} par=[] born=[{0,1},{0,9},{1,9}]\n"
        "b=0 out={3,4} par=[] born=[{3,4},{3,5},{4,5}]\n");

  // ---- E. Grand composant absorbe LOGIQUEMENT par un singleton.
  // Deux etoiles K = 1 (centres 100 et 101, supports [i, c] : `first` = {c},
  // la racine reste celle du gros), puis, au MEME niveau 60, deux aretes
  // ecrites support [c, s] : `first` = {s}, un SINGLETON. La racine logique de
  // chaque composante fusionnee devient celle du singleton alors que le
  // conteneur physique reste le gros record (small-to-large). Les deux deltas
  // du lot rendent l'ordre observable, et les singletons sont choisis pour que
  // l'ordre des racines CONTREDISE l'ordre des cles de sortie :
  //   fids : {1}=0 {2}=1 {3}=2 {20}=3 {50}=4 {51}=5 {52}=6 {70}=7 {100}=8 {101}=9
  //   P (centre 100, feuilles 1,2,3) absorbee par {70} -> racine 7, sortie {1}
  //   Q (centre 101, feuilles 50,51,52) absorbee par {20} -> racine 3, sortie {20}
  //   ordre des racines : 3 (Q), 7 (P) -> {20}, {1}
  //   ordre des CLES de sortie (refute) :  {1}, {20}
  {
    std::vector<ForestEvent> e;
    e.push_back(ev({1, 100}, {}, 0, 1));
    e.push_back(ev({2, 100}, {}, 1u, 2));
    e.push_back(ev({3, 100}, {}, 1u, 3));
    e.push_back(ev({50, 101}, {}, 0, 4));
    e.push_back(ev({51, 101}, {}, 1u, 5));
    e.push_back(ev({52, 101}, {}, 1u, 6));
    e.push_back(ev({100, 70}, {}, 2u, 60));   // slot 1 = {100} ACTIVE, slot 0 = {70} singleton
    e.push_back(ev({101, 20}, {}, 2u, 60));
    check("E_absorption_par_singleton", e,
          "b=0 out={1} par=[] born=[{1},{100}]\n"
          "b=1 out={1} par=[{1}] born=[{2}]\n"
          "b=2 out={1} par=[{1}] born=[{3}]\n"
          "b=3 out={50} par=[] born=[{50},{101}]\n"
          "b=4 out={50} par=[{50}] born=[{51}]\n"
          "b=5 out={50} par=[{50}] born=[{52}]\n"
          "b=6 out={20} par=[{50}] born=[{20}]\n"
          "b=6 out={1} par=[{1}] born=[{70}]\n");
    // Meme structure a 50 feuilles par etoile : le conteneur physique est
    // massivement plus gros que la racine logique.
    std::vector<ForestEvent> g;
    for (u32 i = 1; i <= 50; ++i) g.push_back(ev({i, 100}, {}, i == 1 ? 0 : 1u, i));
    for (u32 i = 1; i <= 50; ++i) g.push_back(ev({200 + i, 101}, {}, i == 1 ? 0 : 1u, i));
    g.push_back(ev({100, 300}, {}, 2u, 60));  // identifiants absorbants FRAIS (70 et 20 collisionneraient avec des feuilles)
    g.push_back(ev({101, 301}, {}, 2u, 60));
    check("E_absorption_50_feuilles", g, nullptr);
  }

  std::printf("fold_fixtures_gate fixtures=%llu deltas=%llu desaccords=%llu\n", (unsigned long long)g_checked, (unsigned long long)g_deltas,
              (unsigned long long)g_bad);
  if (g_checked < min_fixtures || g_deltas < min_deltas) {
    std::printf("PLANCHER\n");
    return 3;
  }
  if (g_bad) return 1;
  std::printf("fold_fixtures_gate OK\n");
  return 0;
}
